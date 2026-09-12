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
 * Callers that need an answer use radioPostAndSettle, which waits for the
 * task's own verdict. Nothing crosses the task boundary but plain data.
 */
/** What travels on the queue: a command, and nothing else. */
typedef RadioCommand QueueItem;

static TaskHandle_t sTask = NULL;
static QueueHandle_t sQueue = NULL;
static SemaphoreHandle_t sLock = NULL;
static RadioSnapshot sSnapshot;
static BandPlanConfig sPlan;

/** How many commands have been put on the queue. Guarded by sLock. */
static uint32_t sPosted;

/**
 * Copy the working state out to where readers can see it.
 *
 * The count of commands worked through moves in the same locked step as the
 * settings they produced. A reader that sees its own command counted is
 * therefore looking at the settings that came of it, never at the ones from
 * before.
 *
 * Each command's own answer goes out with it. Tickets are handed out in queue
 * order and the queue is first in first out, so the n'th command drained this
 * time round is ticket `applied + 1 + n`, and a waiter can find its own
 * result rather than inferring one from the state that followed.
 *
 * @param drained  How many commands were taken off the queue this time round.
 * @param results  What the state machine made of each, in the same order.
 * @return false when the lock could not be taken, in which case nothing was
 *         published and the caller still owes these drains.
 */
static bool publish(const RadioSettings *settings, const Tef668xQuality *q,
                    bool qualityValid, Tef668xError lastError, uint32_t drained,
                    const RadioError *results) {
  if (xSemaphoreTake(sLock, portMAX_DELAY) != pdTRUE) {
    return false;
  }
  for (uint32_t i = 0; i < drained; i++) {
    uint32_t ticket = sSnapshot.applied + 1 + i;
    RadioOutcome *slot = &sSnapshot.outcomes[ticket % RADIO_OUTCOMES];
    slot->ticket = ticket;
    slot->result = results != NULL ? results[i] : RADIO_OK;
  }
  sSnapshot.applied += drained;
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
  return true;
}

/**
 * Tell the tuner what the settings now say, and only what changed.
 *
 * Order matters on a retune. Mute first so nothing bursts out while the
 * frequency moves, and unmute last.
 *
 * Only on a retune. A volume change that mutes and unmutes around itself
 * chops the audio, and the volume knob sends one of those every fiftieth of
 * a second while it is being turned. That is what it sounded like: the sound
 * breaking up while the knob moved.
 *
 * @param from  What the tuner was last told, or NULL to send everything.
 * @param to    What it should be set to.
 */
static Tef668xError pushToTuner(const RadioSettings *from,
                                const RadioSettings *to) {
  RadioPush push = radioPushNeeded(from, to);
  Tef668xError err = TEF668X_OK;
  bool fm = bandModulation(to->band) == MODULATION_FM;

  if (push.retune) {
    /* The mute failing is not a reason to stop. It is a reason to carry on to
     * the unmute at the bottom, because a chip that may or may not be muted
     * and is never told otherwise is the permanent silence this whole comment
     * block exists to prevent. Returning here was exactly that: the next
     * attempt mutes, fails at the same place, and never unmutes. */
    err = tef668xSetMute(true);

    Tef668xError tuned =
        fm ? tef668xTuneFm(to->freqKHz) : tef668xTuneAm(to->freqKHz);
    if (err == TEF668X_OK) {
      err = tuned;
    }
    if (tuned == TEF668X_OK && push.bandwidth) {
      Tef668xError width = fm ? tef668xSetFmBandwidth(to->bandwidthKHz)
                              : tef668xSetAmBandwidth(to->bandwidthKHz);
      if (err == TEF668X_OK) {
        err = width;
      }
    }
    if (push.volume) {
      Tef668xError gain = tef668xSetVolume(to->volumeDb);
      if (err == TEF668X_OK) {
        err = gain;
      }
    }

    /* The mute always comes off, even when something above failed.
     *
     * Returning early after the mute leaves the radio silent with no way
     * back: the next attempt mutes again, fails at the same place, and never
     * reaches the unmute. A radio that is wrong is recoverable. A radio that
     * is silent looks broken. So the unmute happens on every path, and the
     * first real error is what gets reported. */
    Tef668xError unmute = tef668xSetMute(to->muted);
    return err != TEF668X_OK ? err : unmute;
  }

  /* Nothing moved the dial, so nothing is muted around these. */
  if (push.bandwidth) {
    err = fm ? tef668xSetFmBandwidth(to->bandwidthKHz)
             : tef668xSetAmBandwidth(to->bandwidthKHz);
    if (err != TEF668X_OK) {
      return err;
    }
  }
  if (push.volume) {
    if ((err = tef668xSetVolume(to->volumeDb)) != TEF668X_OK) {
      return err;
    }
  }
  if (push.mute) {
    if ((err = tef668xSetMute(to->muted)) != TEF668X_OK) {
      return err;
    }
  }
  return TEF668X_OK;
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

  /* Nothing has been sent to the tuner yet, so everything is. */
  Tef668xError lastError = pushToTuner(NULL, &settings);
  bool pushFailed = lastError != TEF668X_OK;
  Tef668xQuality quality;
  memset(&quality, 0, sizeof(quality));
  publish(&settings, &quality, false, lastError, 0, NULL);

  const TickType_t period = pdMS_TO_TICKS(RADIO_POLL_INTERVAL_MS);
  TickType_t nextPoll = xTaskGetTickCount() + period;
  bool qualityOk = false;
  /* Carried across a round that applied commands but could not publish them,
   * so a drain is never lost and the count never falls behind for good. */
  uint32_t owed = 0;
  RadioError owedResults[RADIO_QUEUE_DEPTH];

  for (;;) {
    RadioSettings wanted = settings;
    QueueItem item;
    bool changed = false;
    uint32_t drained = owed;
    RadioError results[RADIO_QUEUE_DEPTH];
    for (uint32_t i = 0; i < owed; i++) {
      results[i] = owedResults[i];
    }

    /* Sleep on the queue rather than on the clock, so a command is picked up
     * in about a millisecond instead of waiting out the rest of the poll
     * interval. That latency is what a person feels when they turn the knob,
     * and it is the whole of the wait an HTTP write sits through. */
    TickType_t now = xTaskGetTickCount();
    TickType_t wait = (int32_t)(nextPoll - now) > 0 ? nextPoll - now : 0;

    if (drained >= RADIO_QUEUE_DEPTH) {
      /* Every slot is already owed to a command that could not be published.
       * Hold the cadence rather than spinning, and take no more until these
       * have gone out. */
      if (wait > 0) {
        vTaskDelay(wait);
      }
    } else {
      /* The first read waits, the rest take whatever is already there, so a
       * burst of commands costs one retune rather than one each. `drained`
       * can never pass the end of `results`, which is what publish reads. */
      bool first = true;
      while (drained < RADIO_QUEUE_DEPTH &&
             xQueueReceive(sQueue, &item, first ? wait : 0) == pdTRUE) {
        first = false;
        /* Recorded whether or not the state machine took it. The caller
         * waiting on this one wants to know the radio has dealt with it, and
         * a refusal is dealing with it. */
        RadioError result = radioApply(&wanted, &sPlan, &item);
        results[drained++] = result;
        if (result == RADIO_OK) {
          changed = true;
        }
      }
    }

    /* pushFailed carries a failure forward, so a retune is attempted again
     * next time round rather than being forgotten. Without it the task
     * records the settings as applied even when the push failed, and then
     * asking for the same frequency again changes nothing that
     * radioNeedsRetune can see, so the radio cannot be recovered by
     * repeating the command. */
    if ((changed && radioNeedsRetune(&settings, &wanted)) || pushFailed) {
      /* After a failure the tuner's state is not known, so everything goes
       * again rather than only what the settings say moved. */
      lastError = pushToTuner(pushFailed ? NULL : &settings, &wanted);
      pushFailed = lastError != TEF668X_OK;
    }
    settings = wanted;

    /* The reading keeps its own cadence, whatever the commands are doing. */
    now = xTaskGetTickCount();
    if ((int32_t)(now - nextPoll) >= 0) {
      bool fm = bandModulation(settings.band) == MODULATION_FM;
      qualityOk = tef668xReadQuality(fm, &quality) == TEF668X_OK;
      nextPoll += period;
      /* A slow push can leave the next reading already in the past. Start
       * again from now rather than spinning to catch up. */
      if ((int32_t)(xTaskGetTickCount() - nextPoll) >= 0) {
        nextPoll = xTaskGetTickCount() + period;
      }
    }

    if (publish(&settings, &quality, qualityOk, lastError, drained, results)) {
      owed = 0;
    } else {
      /* The commands were applied but nobody was told. Keep them, so the
       * count never falls permanently behind and strands every later waiter. */
      owed = drained;
      for (uint32_t i = 0; i < owed; i++) {
        owedResults[i] = results[i];
      }
    }
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
  sPosted = 0;

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

/**
 * Put a command on the queue and say which one it was.
 *
 * The number and the queue move together under the lock, so a ticket is never
 * handed out for a command that was not queued, and two callers at once
 * cannot be given the same one.
 *
 * @param item    The command, already copied.
 * @param ticket  Receives the count this command will be at once the task has
 *                worked through it. May be NULL.
 * @return false when the queue is full.
 */
static bool post(const QueueItem *item, uint32_t *ticket) {
  if (xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) != pdTRUE) {
    return false;
  }
  bool sent = xQueueSend(sQueue, item, 0) == pdTRUE;
  if (sent) {
    sPosted++;
    if (ticket != NULL) {
      *ticket = sPosted;
    }
  }
  xSemaphoreGive(sLock);
  return sent;
}

bool radioPost(const RadioCommand *command) {
  /* The task has to exist, not just the queue. If the task failed to start,
   * the queue still accepts eight commands and nothing ever reads them, so a
   * caller is told the command was taken when it was quietly dropped. */
  if (sTask == NULL || sQueue == NULL || command == NULL) {
    return false;
  }
  QueueItem item = *command;
  return post(&item, NULL);
}

RadioPostResult radioPostAndSettle(const RadioCommand *command, uint32_t waitMs,
                                   RadioError *result) {
  uint32_t ticket = 0;
  if (result != NULL) {
    *result = RADIO_OK;
  }
  if (sTask == NULL || sQueue == NULL || command == NULL) {
    return RADIO_POST_BUSY;
  }
  QueueItem item = *command;
  if (!post(&item, &ticket)) {
    return RADIO_POST_BUSY;
  }

  /* Poll rather than wait on a notification. The task has no idea who posted,
   * and giving it a list of tasks to wake would put the waiters' business
   * inside the one place that must never be held up. */
  const TickType_t step = pdMS_TO_TICKS(2);
  TickType_t started = xTaskGetTickCount();
  for (;;) {
    RadioSnapshot now;
    /* Compared by subtraction, the same way the millis() deadlines in this
     * firmware are, so the answer stays right when the count wraps. */
    if (radioGetSnapshot(&now) && (int32_t)(now.applied - ticket) >= 0) {
      const RadioOutcome *slot = &now.outcomes[ticket % RADIO_OUTCOMES];
      /* The slot is only ours while fewer than RADIO_OUTCOMES commands have
       * been drained since. It cannot have been overwritten here: the queue
       * holds at most RADIO_QUEUE_DEPTH, and this poll is far faster than the
       * radio can work through that many. The check is for the case that
       * would otherwise report someone else's answer as ours. */
      if (result != NULL && slot->ticket == ticket) {
        *result = slot->result;
      }
      return RADIO_POST_DONE;
    }
    if ((xTaskGetTickCount() - started) >= pdMS_TO_TICKS(waitMs)) {
      return RADIO_POST_SLOW;
    }
    vTaskDelay(step);
  }
}

bool radioTaskPlan(BandPlanConfig *out) {
  if (sTask == NULL || out == NULL) {
    return false;
  }
  /* Written once before the task starts and never again, so this needs no
   * lock. If the plan ever becomes something a person can change while the
   * radio is running, it has to move inside the snapshot. */
  *out = sPlan;
  return true;
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
