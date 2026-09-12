/**
 * @file radio_task.cpp
 * @brief Implementation of the radio task, its queue and its snapshot.
 */
#include "radio_task.h"

#include "core/signal.h"

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
 * The squelch. Only the radio task touches the state; the mode and the
 * threshold are set from other tasks and are guarded by sLock.
 */
static Squelch sSquelch;

/**
 * The mute the tuner was last told, which is not what the settings say.
 *
 * The settings hold what the person asked for. The tuner holds that or
 * silence from the squelch, and the difference between the two is what
 * decides whether anything needs sending.
 */
static bool sLastPushedMute = false;

/** The volume the tuner was last told, which the fade moves on its own. */
static int8_t sLastPushedVolume = 0;

/** When the current fade started, and how long it lasts. */
static uint32_t sFadeFromMs = 0;
static uint16_t sFadeMs = RADIO_FADE_MS;

/** Smoothed readings, for anything that decides on the signal. */
static SignalAverage sLevelAverage;
static SignalAverage sSnrAverage;

/** What the bandwidth extension was last set to. */
static bool sBandwidthWide = false;
static bool sBandwidthKnown = false;
static SquelchMode sSquelchMode = SQUELCH_OFF;
static int16_t sSquelchThreshold = 0;

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
                    const RadioError *results,
                    const Tef668xProcessing *processing, bool processingValid) {
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
  if (processingValid && processing != NULL) {
    sSnapshot.processing = *processing;
  }
  sSnapshot.processingValid = processingValid;
  sSnapshot.tunerReady = tef668xCapabilities() != NULL;
  sSnapshot.lastError = lastError;
  sSnapshot.bandwidthWide = sBandwidthWide;
  sSnapshot.tunerMuted = sLastPushedMute;
  sSnapshot.squelchMode = sSquelchMode;
  sSnapshot.squelchOpen = sSquelch.open;
  sSnapshot.squelchThresholdTenths = sSquelchThreshold;
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
/**
 * Everything the tuner is told about how to receive, beyond the dial.
 *
 * Written on every retune as well as on a change, because crossing to the AM
 * side and back puts the chip through its active mode again and what it keeps
 * across that is not documented.
 */
static Tef668xError pushFeatures(const RadioSettings *s) {
  /* The blankers come first, because they are the only part of this that
   * applies on the AM bands. Everything below returns early there. */
  Tef668xError blanker = tef668xSetAmNoiseBlanker(s->amNoiseBlankerStart);
  Tef668xError fmBlanker = tef668xSetFmNoiseBlanker(s->fmNoiseBlankerStart);
  if (blanker == TEF668X_OK) {
    blanker = fmBlanker;
  }

  if (bandModulation(s->band) != MODULATION_FM) {
    return blanker;
  }
  /* All three go out whatever happens to the first, and the first error is
   * what gets reported. Stopping at a failure would leave the other two
   * holding whatever the chip had, with the firmware believing it had set
   * them, which is the harder fault to find. The same reasoning as the
   * unmute above, and worth saying rather than leaving it to look accidental. */
  Tef668xError err = tef668xSetMultipathSuppression(s->multipathSuppression);
  Tef668xError eq = tef668xSetChannelEqualizer(s->equalizer);
  Tef668xError mono = tef668xSetMono(s->forcedMono);
  Tef668xError weak = tef668xSetWeakSignal(s->highCutStart, s->stereoBlendStart,
                                           s->stHiBlendStart);
  Tef668xError deemp = tef668xSetDeemphasis(s->deemphasisUs);
  if (err != TEF668X_OK) {
    return err;
  }
  if (eq != TEF668X_OK) {
    return eq;
  }
  if (mono != TEF668X_OK) {
    return mono;
  }
  if (weak != TEF668X_OK) {
    return weak;
  }
  return deemp != TEF668X_OK ? deemp : blanker;
}

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
    if (err == TEF668X_OK) {
      err = unmute;
    }
    if (push.features) {
      Tef668xError feat = pushFeatures(to);
      if (err == TEF668X_OK) {
        err = feat;
      }
    }
    return err;
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
  if (push.features) {
    if ((err = pushFeatures(to)) != TEF668X_OK) {
      return err;
    }
  }
  return TEF668X_OK;
}

/** The radio task. Owns the tuner for the life of the radio. */
static void radioTask(void *arg) {
  (void)arg;

  /* Taken from the snapshot, which radioTaskStart has already filled in with
   * the defaults and the frequency the radio is to come up on. Calling
   * radioDefaults again here would throw that away, which is what left the
   * radio unmuting on the bottom of the FM band and then retuning. */
  sFadeFromMs = millis();
  sFadeMs = RADIO_FADE_MS;
  /* The first pass through the loop has already had its fade started, by the
   * opening push below. Starting another one there would drop the volume the
   * moment the radio came up. */
  bool firstPass = true;
  RadioSettings settings = sSnapshot.settings;

  /* The AM side has no automatic bandwidth, so a band that starts there needs
   * one chosen before the first push. */
  if (bandModulation(settings.band) != MODULATION_FM &&
      settings.bandwidthKHz == 0) {
    settings.bandwidthKHz = 4;
  }

  /* Nothing has been sent to the tuner yet, so everything is, and the volume
   * starts at the bottom of the fade rather than at the target. */
  RadioSettings opening = settings;
  opening.volumeDb = radioFadeVolume(settings.volumeDb, 0, RADIO_FADE_MS);
  sLastPushedVolume = opening.volumeDb;
  Tef668xError lastError = pushToTuner(NULL, &opening);
  bool pushFailed = lastError != TEF668X_OK;
  Tef668xQuality quality;
  memset(&quality, 0, sizeof(quality));
  Tef668xProcessing processing;
  memset(&processing, 0, sizeof(processing));
  bool processingOk = false;
  publish(&settings, &quality, false, lastError, 0, NULL, &processing, false);

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

    /* While a fade is running the loop has to come round far more often than
     * the poll interval, or the fade is delivered in as many steps as there
     * are polls. A band change fade is four tenths of a second, which at the
     * poll rate is four of them, and four steps is a jerk rather than a
     * fade. */
    /* Finished fades are put away rather than left to age. Left alone,
     * millis() minus the start climbs for forty nine days and then wraps back
     * through zero, and the radio would fade for no reason. */
    if (sFadeMs != 0 && (uint32_t)(millis() - sFadeFromMs) >= sFadeMs) {
      sFadeMs = 0;
    }
    bool fading = sFadeMs != 0;
    if (fading) {
      TickType_t step = pdMS_TO_TICKS(RADIO_FADE_STEP_MS);
      if (wait > step) {
        wait = step;
      }
    }

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
      bool waited = false;
      while (drained < RADIO_QUEUE_DEPTH &&
             xQueueReceive(sQueue, &item, waited ? 0 : wait) == pdTRUE) {
        waited = true;
        /* Recorded whether or not the state machine took it. The caller
         * waiting on this one wants to know the radio has dealt with it, and
         * a refusal is dealing with it. */
        results[drained++] = radioApply(&wanted, &sPlan, &item);
      }
    }

    /* pushFailed carries a failure forward, so a retune is attempted again
     * next time round rather than being forgotten. Without it the task
     * records the settings as applied even when the push failed, and then
     * asking for the same frequency again changes nothing that
     * radioNeedsRetune can see, so the radio cannot be recovered by
     * repeating the command. */
    /* What the tuner is actually told to do about the audio: what the person
     * asked for, or silence because the squelch is shut. The two are kept
     * apart everywhere else, so turning the squelch off can never leave a
     * radio the person deliberately muted playing, and the squelch can never
     * unmute something they muted on purpose. */
    RadioSettings heard = wanted;
    heard.muted = wanted.muted || !sSquelch.open;
    RadioSettings wasHeard = settings;
    wasHeard.muted = sLastPushedMute;
    wasHeard.volumeDb = sLastPushedVolume;
    RadioPush push = radioPushNeeded(&wasHeard, &heard);

    /* The fade starts before the push that needs it, not after.
     *
     * Starting it afterwards let the retune go out carrying the full volume,
     * so the radio unmuted loud, dropped twenty five dB on the next pass, and
     * then ramped back. That is the opposite of the point, and it is subtle
     * enough to survive a listening test: it still ends in a ramp.
     *
     * Judged on push.retune rather than on which commands arrived, because a
     * tune the state machine refused, or a band command naming the band the
     * radio is already on, moves nothing and should fade nothing. */
    if (push.retune && !firstPass) {
      sFadeFromMs = millis();
      sFadeMs = RADIO_BAND_FADE_MS;
    }

    /* The volume the tuner is actually given, which while a fade runs is on
     * its way up to the target. The target is read every time round, so the
     * knob still works during one. */
    heard.volumeDb =
        radioFadeVolume(wanted.volumeDb, millis() - sFadeFromMs, sFadeMs);
    if (heard.volumeDb != wasHeard.volumeDb) {
      push.volume = true;
    }

    /* No `changed` guard here. The squelch can move the mute with no command
     * having arrived at all, and a push that only happens when something was
     * drained would never act on it. Comparing the two is the whole test. */
    if (push.retune || push.bandwidth || push.volume || push.mute ||
        push.features || pushFailed) {
      /* After a failure the tuner's state is not known, so everything goes
       * again rather than only what the settings say moved. */
      lastError = pushToTuner(pushFailed ? NULL : &wasHeard, &heard);
      pushFailed = lastError != TEF668X_OK;
      sLastPushedMute = heard.muted;
      sLastPushedVolume = heard.volumeDb;
      if (push.retune) {
        /* The readings from before the dial moved say nothing about where it
         * is now, and the chip has been through its active mode, which may
         * have taken the bandwidth option with it. Both start again. */
        signalAverageReset(&sLevelAverage);
        signalAverageReset(&sSnrAverage);
        sBandwidthKnown = false;
      }
    }
    settings = wanted;

    /* The reading keeps its own cadence, whatever the commands are doing. */
    now = xTaskGetTickCount();
    if ((int32_t)(now - nextPoll) >= 0) {
      bool fm = bandModulation(settings.band) == MODULATION_FM;
      qualityOk = tef668xReadQuality(fm, &quality) == TEF668X_OK;

      /* The squelch gets a say on every fresh reading, and only on a fresh
       * one. Running it again between readings would make its hold measure
       * loop iterations rather than time. */
      SquelchReading reading;
      reading.valid = qualityOk;
      reading.levelTenths = quality.levelDbuVTenths;
      reading.noiseTenths = quality.usnTenths;
      reading.multipathTenths = quality.multipathTenths;
      reading.offsetTenths = quality.offsetKHzTenths;

      SquelchMode mode;
      int16_t threshold;
      if (xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE) {
        mode = sSquelchMode;
        threshold = sSquelchThreshold;
        xSemaphoreGive(sLock);
      } else {
        mode = sSquelchMode;
        threshold = sSquelchThreshold;
      }
      /* Only a reading that arrived, and only from the FM side.
       *
       * A failed read leaves the quality struct holding the previous one, so
       * feeding it in again would count the same sample twice. And the AM
       * readings are a different scale entirely: letting them into these
       * averages meant a strong FM station came back from a spell on medium
       * wave with the filter held narrow for about two seconds while the
       * smoothing forgot the AM numbers. */
      /* What the chip is actually doing with the audio, which is FM only and
       * is the only way to tell a blend that is working from one that was
       * never switched on. */
      processingOk = fm && tef668xReadProcessing(&processing) == TEF668X_OK;

      if (qualityOk && fm) {
        /* Smoothed, because one reading of this tuner jumps far more than the
         * signal does and the filter would open and shut several times a
         * second on a bare one. */
        int16_t level = signalAverage(&sLevelAverage, quality.levelDbuVTenths);
        int16_t snr = signalAverage(&sSnrAverage, quality.snrDb);

        /* The reference firmware's rule: a strong clean signal is allowed a
         * wider filter, which is more treble and better separation. Its
         * thresholds, on the level scale this firmware now shares with it. */
        bool wantWide = snr > 15 && level > 300;
        if (wantWide != sBandwidthWide || !sBandwidthKnown) {
          if (tef668xSetBandwidthExtension(wantWide) == TEF668X_OK) {
            sBandwidthWide = wantWide;
            sBandwidthKnown = true;
          }
        }
      }

      bool wasOpen = sSquelch.open;
      squelchUpdate(&sSquelch, NULL, mode, settings.band, &reading, threshold,
                    millis());

      /* Acted on now, not next time round.
       *
       * The push above this runs before the reading, so leaving it to that
       * would hold the decision back a whole poll interval. A tenth of a
       * second of silence after the dial lands on a station is exactly the
       * clipped opening the squelch is written to avoid, and the snapshot
       * would meanwhile say open while the tuner was still muted.
       *
       * Only the mute moves, so only the mute is sent. */
      if (sSquelch.open != wasOpen) {
        bool wantMuted = settings.muted || !sSquelch.open;
        if (wantMuted != sLastPushedMute) {
          Tef668xError muteErr = tef668xSetMute(wantMuted);
          if (muteErr == TEF668X_OK) {
            sLastPushedMute = wantMuted;
          } else {
            lastError = muteErr;
            /* Left for the push at the top of the next round to put right,
             * which re-sends everything after a failure. */
            pushFailed = true;
          }
        }
      }

      nextPoll += period;
      /* A slow push can leave the next reading already in the past. Start
       * again from now rather than spinning to catch up. */
      if ((int32_t)(xTaskGetTickCount() - nextPoll) >= 0) {
        nextPoll = xTaskGetTickCount() + period;
      }
    }

    firstPass = false;

    if (publish(&settings, &quality, qualityOk, lastError, drained, results,
                &processing, processingOk)) {
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

bool radioTaskStart(const Settings *settings, const BandPlanConfig *plan,
                    int8_t startVolumeDb) {
  if (sTask != NULL) {
    return true;
  }
  if (plan != NULL) {
    sPlan = *plan;
  } else {
    bandPlanDefaults(&sPlan);
  }

  memset(&sSnapshot, 0, sizeof(sSnapshot));

  /* Everything a person chose, before the task takes its first look. The
   * task's first push ends with an unmute, so anything changed after that is
   * heard: a burst of whatever was on the default frequency, or a moment of
   * the feature that was about to be switched off. */
  radioFromSettings(settings, &sPlan, &sSnapshot.settings);

  RadioCommand volume = {};
  volume.kind = RADIO_SET_VOLUME;
  volume.volumeDb = startVolumeDb;
  radioApply(&sSnapshot.settings, &sPlan, &volume);
  sPosted = 0;

  /* Open. A zeroed squelch is a shut one, and the radio would come up silent
   * and stay that way until the first reading arrived. */
  squelchInit(&sSquelch);
  signalAverageReset(&sLevelAverage);
  signalAverageReset(&sSnrAverage);
  sBandwidthKnown = false;
  sSquelchMode = SQUELCH_OFF;
  if (settings != NULL && settings->squelchMode < SQUELCH_MODE_COUNT) {
    sSquelchMode = (SquelchMode)settings->squelchMode;
  }
  sSquelchThreshold = 0;
  sLastPushedMute = false;

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

void radioSetSquelchMode(SquelchMode mode) {
  if (mode >= SQUELCH_MODE_COUNT) {
    return;
  }
  if (sLock != NULL && xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE) {
    sSquelchMode = mode;
    xSemaphoreGive(sLock);
    return;
  }
  sSquelchMode = mode;
}

void radioSetSquelchThreshold(int16_t tenths) {
  if (sLock != NULL && xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE) {
    sSquelchThreshold = tenths;
    xSemaphoreGive(sLock);
    return;
  }
  sSquelchThreshold = tenths;
}

SquelchMode radioSquelchMode(int16_t *thresholdTenths) {
  SquelchMode mode;
  int16_t threshold;
  if (sLock != NULL && xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE) {
    mode = sSquelchMode;
    threshold = sSquelchThreshold;
    xSemaphoreGive(sLock);
  } else {
    mode = sSquelchMode;
    threshold = sSquelchThreshold;
  }
  if (thresholdTenths != NULL) {
    *thresholdTenths = threshold;
  }
  return mode;
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
