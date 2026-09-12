/**
 * @file radio_task.cpp
 * @brief Implementation of the radio task, its queue and its snapshot.
 */
#include "radio_task.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <string.h>

/** Core 0 is the radio's. Core 1 runs the UI and the network. */
#define RADIO_TASK_CORE 0

/** Above the Arduino loop, so a busy web server cannot starve the tuner. */
#define RADIO_TASK_PRIORITY 3

/** Enough for the driver, the band plan and a little room. */
#define RADIO_TASK_STACK 4096

/*
 * The queue carries commands and nothing else.
 *
 * An earlier version let a caller wait for the answer by putting a semaphore
 * and a result pointer on the queue. That is a use after free waiting to
 * happen: a caller that times out deletes the semaphore and returns, and the
 * task then signals a handle that is gone and writes through a pointer into a
 * stack frame that has been reused.
 *
 * Callers that need an answer use radioWouldAccept instead, which runs the
 * same pure state machine over a copy. Nothing crosses the task boundary but
 * plain data.
 */
/** What travels on the queue: a command, and nothing else. */
typedef RadioCommand QueueItem;

static TaskHandle_t sTask = NULL;
static QueueHandle_t sQueue = NULL;
static SemaphoreHandle_t sLock = NULL;
static RadioSnapshot sSnapshot;
static BandPlanConfig sPlan;

/** Copy the working state out to where readers can see it. */
static void publish(const RadioSettings *settings, const Tef668xQuality *q,
                    bool qualityValid, Tef668xError lastError) {
  if (xSemaphoreTake(sLock, portMAX_DELAY) != pdTRUE) {
    return;
  }
  sSnapshot.settings = *settings;
  if (qualityValid) {
    sSnapshot.quality = *q;
  }
  sSnapshot.qualityValid = qualityValid;
  sSnapshot.tunerReady = tef668xCapabilities() != NULL;
  sSnapshot.lastError = lastError;
  sSnapshot.updatedMs = millis();
  sSnapshot.sequence++;
  xSemaphoreGive(sLock);
}

/**
 * Tell the tuner what the settings now say.
 *
 * Order matters. Mute first when muting, so nothing bursts out while the
 * frequency moves, and unmute last for the same reason.
 */
static Tef668xError pushToTuner(const RadioSettings *s) {
  Tef668xError err = tef668xSetMute(true);
  if (err != TEF668X_OK) {
    return err;
  }

  bool fm = bandModulation(s->band) == MODULATION_FM;
  if ((err = fm ? tef668xTuneFm(s->freqKHz) : tef668xTuneAm(s->freqKHz)) ==
      TEF668X_OK) {
    err = fm ? tef668xSetFmBandwidth(s->bandwidthKHz)
             : tef668xSetAmBandwidth(s->bandwidthKHz);
  }
  if (err == TEF668X_OK) {
    err = tef668xSetVolume(s->volumeDb);
  }

  /* The mute always comes off, even when something above failed.
   *
   * Returning early after the mute leaves the radio silent with no way back:
   * the next attempt mutes again, fails at the same place, and never reaches
   * the unmute. A radio that is wrong is recoverable. A radio that is silent
   * looks broken. So the unmute happens on every path, and the first real
   * error is what gets reported. */
  Tef668xError unmute = tef668xSetMute(s->muted);
  return err != TEF668X_OK ? err : unmute;
}

/** The radio task. Owns the tuner for the life of the radio. */
static void radioTask(void *arg) {
  (void)arg;

  RadioSettings settings;
  radioDefaults(&settings, &sPlan);

  /* The AM side has no automatic bandwidth, so a band that starts there needs
   * one chosen before the first push. */
  if (bandModulation(settings.band) != MODULATION_FM &&
      settings.bandwidthKHz == 0) {
    settings.bandwidthKHz = 4;
  }

  Tef668xError lastError = pushToTuner(&settings);
  bool pushFailed = lastError != TEF668X_OK;
  Tef668xQuality quality;
  memset(&quality, 0, sizeof(quality));
  publish(&settings, &quality, false, lastError);

  TickType_t nextPoll = xTaskGetTickCount();

  for (;;) {
    /* Take everything that is waiting before touching the tuner, so a burst
     * of commands costs one retune rather than one each. */
    RadioSettings wanted = settings;
    QueueItem item;
    bool changed = false;
    while (xQueueReceive(sQueue, &item, 0) == pdTRUE) {
      if (radioApply(&wanted, &sPlan, &item) == RADIO_OK) {
        changed = true;
      }
    }

    /* pushFailed carries a failure forward, so a retune is attempted again
     * next time round rather than being forgotten. Without it the task
     * records the settings as applied even when the push failed, and then
     * asking for the same frequency again changes nothing that
     * radioNeedsRetune can see, so the radio cannot be recovered by
     * repeating the command. */
    if ((changed && radioNeedsRetune(&settings, &wanted)) || pushFailed) {
      lastError = pushToTuner(&wanted);
      pushFailed = lastError != TEF668X_OK;
    }
    settings = wanted;

    bool fm = bandModulation(settings.band) == MODULATION_FM;
    bool ok = tef668xReadQuality(fm, &quality) == TEF668X_OK;
    publish(&settings, &quality, ok, lastError);

    /* A fixed cadence rather than a delay after the work, so the poll rate
     * does not drift with how long the work took. */
    vTaskDelayUntil(&nextPoll, pdMS_TO_TICKS(RADIO_POLL_INTERVAL_MS));
  }
}

bool radioTaskStart(const BandPlanConfig *plan) {
  if (sTask != NULL) {
    return true;
  }
  if (plan != NULL) {
    sPlan = *plan;
  } else {
    bandPlanDefaults(&sPlan);
  }

  memset(&sSnapshot, 0, sizeof(sSnapshot));
  radioDefaults(&sSnapshot.settings, &sPlan);

  sQueue = xQueueCreate(RADIO_QUEUE_DEPTH, sizeof(QueueItem));
  sLock = xSemaphoreCreateMutex();
  if (sQueue != NULL && sLock != NULL &&
      xTaskCreatePinnedToCore(radioTask, "radio", RADIO_TASK_STACK, NULL,
                              RADIO_TASK_PRIORITY, &sTask,
                              RADIO_TASK_CORE) == pdPASS) {
    return true;
  }

  /* Give back whatever was taken, and leave nothing half built. A second
   * attempt then starts clean instead of leaking another queue and mutex. */
  if (sQueue != NULL) {
    vQueueDelete(sQueue);
    sQueue = NULL;
  }
  if (sLock != NULL) {
    vSemaphoreDelete(sLock);
    sLock = NULL;
  }
  sTask = NULL;
  return false;
}

bool radioPost(const RadioCommand *command) {
  /* The task has to exist, not just the queue. If the task failed to start,
   * the queue still accepts eight commands and nothing ever reads them, so a
   * caller is told the command was taken when it was quietly dropped. */
  if (sTask == NULL || sQueue == NULL || command == NULL) {
    return false;
  }
  QueueItem item = *command;
  return xQueueSend(sQueue, &item, 0) == pdTRUE;
}

bool radioWouldAccept(const RadioCommand *command, RadioError *result) {
  RadioError applied = RADIO_ERR_UNKNOWN;
  if (command != NULL) {
    RadioSnapshot now;
    if (radioGetSnapshot(&now)) {
      RadioSettings trial = now.settings;
      applied = radioApply(&trial, &sPlan, command);
    }
  }
  if (result != NULL) {
    *result = applied;
  }
  return applied == RADIO_OK;
}

bool radioGetSnapshot(RadioSnapshot *out) {
  if (sLock == NULL || out == NULL) {
    return false;
  }
  if (xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) != pdTRUE) {
    return false;
  }
  *out = sSnapshot;
  xSemaphoreGive(sLock);
  return true;
}
