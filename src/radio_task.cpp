/* Implementation of the radio task, its queue and its snapshot. */
#include "radio_task.h"

#include "core/signal.h"
#include "memory_store.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <string.h>

/* Core 0 is the radio's. Core 1 runs the UI and the network. */
#define RADIO_TASK_CORE 0

/* Above the Arduino loop, so a busy web server cannot starve the tuner. */
#define RADIO_TASK_PRIORITY 3

/* Enough for the driver, the band plan and a little room. */
#define RADIO_TASK_STACK 4096

/*
 * The queue carries commands and nothing else.
 *
 * No semaphore and no result pointer. Putting those on the queue so a caller
 * can wait for its answer is a use after free waiting to happen: a caller
 * that times out deletes the semaphore and returns, and the task then signals
 * a handle that is gone and writes through a pointer into a stack frame that
 * has been reused.
 *
 * Callers that need an answer use radioPostAndSettle, which waits for the
 * task's own verdict. Nothing crosses the task boundary but plain data.
 */
/* What travels on the queue: a command, and nothing else. */
typedef RadioCommand QueueItem;

static TaskHandle_t sTask = NULL;
static QueueHandle_t sQueue = NULL;
static SemaphoreHandle_t sLock = NULL;
static RadioSnapshot sSnapshot;
static BandPlanConfig sPlan;

/* How many commands have been put on the queue. Guarded by sLock. */
static uint32_t sPosted;

/*
 * Which stored channel the radio is sitting on, or MEMORY_NO_SLOT.
 *
 * Only the radio task touches it. It is worked out again whenever the band
 * or the frequency moves, so it says what is true rather than only what
 * memory mode last did: tuning by hand onto a stored channel shows its slot,
 * and tuning off one clears it.
 */
static int sMemorySlot = MEMORY_NO_SLOT;

/*
 * The squelch. Only the radio task touches the state; the mode and the
 * threshold are set from other tasks and are guarded by sLock.
 */
static Squelch sSquelch;

/*
 * The mute the tuner was last told, which is not what the settings say.
 *
 * The settings hold what the person asked for. The tuner holds that or
 * silence from the squelch, and the difference between the two is what
 * decides whether anything needs sending.
 */
static bool sLastPushedMute = false;

/* The volume the tuner was last told, which the fade moves on its own. */
static int8_t sLastPushedVolume = 0;

/*
 * The filter width the tuner was last told.
 *
 * Tracked separately from the settings for the same reason the mute is: a
 * filter change waits for the ramp to reach silence, so for a few rounds
 * what the person asked for and what the tuner holds are different.
 */
static uint16_t sLastPushedBandwidth = 0;

/*
 * A round left something for the next one to finish quickly.
 *
 * The filter goes out on the round the ramp reaches the bottom, and the fade
 * back up is only started on the round after it. Without this the task
 * sleeps out the whole poll interval in between, which is more silence than
 * the two ramps put together.
 */
static bool sWakeSoon = false;

/* When the current fade started, and how long it lasts. */
static uint32_t sFadeFromMs = 0;
static uint16_t sFadeMs = RADIO_FADE_MS;

/* Smoothed readings, for anything that decides on the signal. */
static SignalAverage sLevelAverage;
static SignalAverage sSnrAverage;

/*
 * The same smoothing again, for anything a person looks at.
 *
 * A second average rather than a reading of the first one, because the first
 * is fed on the FM side only: the bandwidth extension it drives is an FM
 * feature, and AM readings in it held a strong FM station's filter narrow for
 * about two seconds after coming back from medium wave. A meter has to keep
 * working on both sides, so this one takes every reading that arrives.
 */
static SignalAverage sDisplayLevelAverage;
static int16_t sLevelSmoothed = 0;
/*
 * Put the radio on a stored channel.
 *
 * The one place a slot becomes a frequency, shared by the knob in memory mode
 * and by RADIO_RECALL, so the two cannot come to mean different things.
 */
static RadioError recallChannel(RadioSettings *wanted, int slot) {
  MemoryChannel channel;
  if (!memoryStoreRead(slot, &channel)) {
    return RADIO_ERR_NO_CHANNEL;
  }
  /* Checked here and not only by whoever chose the slot, so the knob and the
   * API agree about which slots can be reached. A tune goes by frequency
   * alone and the band plan picks the band for it, so a channel whose
   * frequency is not on the band it names would land somewhere else, with
   * that band's step size, while the list went on reporting the band it
   * says. */
  if (!memoryChannelTunable(&channel, &sPlan)) {
    return RADIO_ERR_CHANNEL_BAND;
  }
  /* A channel list runs across the bands, so walking it moves band. Decision
   * 29 gives every band its own tuning mode and a tune across an edge puts
   * that band's mode back, which would drop the radio out of memory mode on
   * the first channel that is not on this band. Recalling a channel is not a
   * person choosing a tuning mode, so whatever mode they were in is kept. */
  TuneMode was = wanted->tuneMode;
  RadioCommand tune = {};
  tune.kind = RADIO_TUNE;
  tune.freqKHz = channel.freqKHz;
  RadioError result = radioApply(wanted, &sPlan, &tune);
  if (result != RADIO_OK) {
    return result;
  }
  /* Unless the new band does not have that mode. Meter band stepping only
   * exists on shortwave, so a channel that leaves it cannot keep it. */
  if (radioTuneModeAllowed(was, wanted->band)) {
    wanted->tuneMode = was;
  }
  /* A width of 0 is the channel saying it has no opinion, so whatever the
   * band is already set to is left alone. A width the band does not offer is
   * left alone too: the channel is still worth tuning, and the tune has
   * already happened. */
  if (channel.bandwidthKHz != 0) {
    RadioCommand width = {};
    width.kind = RADIO_SET_BANDWIDTH;
    width.bandwidthKHz = channel.bandwidthKHz;
    radioApply(wanted, &sPlan, &width);
  }
  return RADIO_OK;
}

/* Whether that level was read where the dial is now. See the snapshot. */
static bool sLevelSmoothedValid = false;

/*
 * The RDS decoder, and the raw groups it was fed.
 *
 * Written only by the radio task. The raw ring is read from the web task, so
 * that one is copied out under the same lock as the snapshot.
 */
static Rds sRds;
static RadioRdsRaw sRdsRaw[RADIO_RDS_RAW_DEPTH];
static uint32_t sRdsRawTotal = 0;   /* How many are in the ring. */
static uint32_t sRdsRawDropped = 0; /* Groups the ring could not be given. */
static uint16_t sRdsStatusWord = 0; /* The last status word off the chip. */
static bool sRdsStatusRead = false; /* Whether that read worked. */
/*
 * The ring still holds the station the dial has left.
 *
 * Emptying it needs the lock, and the retune cannot wait on that: the audio
 * is muted while it runs. So the retune only says the ring is stale, and the
 * next RDS read empties it under the lock it was going to take anyway, which
 * is within 43 ms. Until then the ring serves nothing, because a ring
 * carrying two stations under one unbroken run of sequence numbers is a
 * capture that reads as one broadcast and is not, and neither the capture
 * script nor the header generator can tell.
 *
 * This is the one flag the radio task sets without the lock, and it is safe
 * in the only direction that matters. A bool is a single aligned byte, so a
 * reader sees true or false and never anything in between, and the radio task
 * is the only writer. Once the retune has happened the flag is true and stays
 * true until the radio task itself clears it, so a reader after a retune can
 * never be handed the station before it.
 */
static bool sRdsRawStale = false;

/*
 * Whether the decoder runs. A setting, so it is read on every round rather
 * than captured once.
 */
/*
 * A settle probe waiting to be carried out, and what came of it.
 *
 * Written by the caller under the lock and read by the radio task, which is
 * the only thing that touches the tuner.
 */
static bool sProbeWanted = false;
static uint32_t sProbeKHz = 0;
static uint16_t sProbeMs = 0;
/*
 * Which probe this is. A caller that gave up leaves its probe in flight, and
 * without a number to match on, the next caller is handed the readings the
 * last one abandoned, taken at a frequency it never asked about. That would
 * be an ordinary looking line in a fixture and nothing could tell.
 */
static uint32_t sProbeSeq = 0;
static uint32_t sProbeDoneSeq = 0;
static uint8_t sProbeReads = 1;
static uint16_t sProbeGapMs = 0;
static bool sProbeDone = false;
static bool sProbeOk = false;
static Tef668xQuality sProbeQuality[RADIO_PROBE_MAX_READS];

static bool sRdsEnabled = true;
/* Set when it is switched off, so the round that notices throws away what the
 * decoder held. Doing it in radioSetRdsEnabled would touch the decoder from
 * another task. */
static bool sRdsForget = false;

/* What the bandwidth extension was last set to. */
static bool sBandwidthWide = false;
static bool sBandwidthKnown = false;
/*
 * How long publish waits for the lock before giving up.
 *
 * Bounded rather than forever. The readers hold it only long enough to copy a
 * struct, so this is never reached in practice, but a wait with no end makes
 * the failure impossible and the recovery below unreachable. The commands are
 * carried forward and published on the next round.
 */
#define RADIO_PUBLISH_WAIT_MS 100

static SquelchMode sSquelchMode = SQUELCH_OFF;

/*
 * The squelch thresholds. Only the level floor is settable; the rest are the
 * measured defaults.
 *
 * Written from other tasks and copied out by the radio task under sLock, the
 * same as the squelch mode and threshold beside it. Copied rather than read
 * in place, so that one decision sees one consistent set however many fields
 * become settable later.
 */
static SquelchConfig sSquelchCfg;

/* ------------------------------------------------------------------ seek --
 *
 * Owned by the radio task and touched from nowhere else, except the config,
 * which is set under the lock like the squelch mode is.
 *
 * Seek is a state machine rather than a loop, so that a command arriving in
 * the middle of it is acted on within one channel, about 50 ms, instead of
 * after the radio has finished walking the band. That is what makes it
 * cancellable.
 */
static bool sSeeking = false;
static bool sSeekUp = true;
static bool sSeekFound = false;
static uint32_t sSeekVisited = 0;
static uint32_t sSeekLimit = 0;
static SeekConfig sSeekConfig;

/* ------------------------------------------------------------- the beep --
 *
 * The tone is started when the command arrives and stopped once it has run
 * long enough. Started and stopped rather than held with a delay, so a beep
 * does not stall the queue or a seek walking the band.
 */
static uint32_t sBeepUntilMs = 0;
static bool sBeeping = false;
static bool sToneOn = false;
static uint16_t sBeepHz = 2000;
static uint16_t sBeepHz2 = 2000;

/*
 * Set on the way to a reboot, and cleared again if that reboot is called
 * off. The task writes nothing to the tuner while it is set, so the shutdown
 * is the only thing on the bus.
 *
 * Every write in the loop has to check it, not only the push. The squelch
 * shortcut, the tone and the bandwidth extension all reach the tuner on
 * their own, and an update over the air holds the hush for the length of the
 * transfer rather than the two hundred milliseconds a reboot takes. A
 * squelch opening in that time would unmute the radio in the middle of an
 * update.
 */
/* Written by whichever task is shutting the radio down and read by the radio
 * task every round, so it is not the compiler's to cache. */
static volatile bool sHushed = false;

/*
 * The tone, in hertz, and how loud, in tenths of a dB below full scale.
 *
 * The reference firmware's figures, which it uses for its band edge beep on
 * this chip: 2000 Hz at -5 dB. Loud enough to hear over a station, not so
 * loud that it is startling.
 */
#define RADIO_BEEP_HZ 2000
/* How loud, in tenths of a dB below full scale. */
#define RADIO_BEEP_AMPLITUDE (-50)
/* How long a beep lasts. The reference firmware's figure. */
#define RADIO_BEEP_MS 50

/* Whether the dial wrapping at a band edge makes a sound. Off unless asked. */
static bool sBeepEdge = false;

/* How long the audio ramps down before it is cut. Set from the settings. */
static uint16_t sSoftMuteMs = RADIO_SOFT_MUTE_MS;

/*
 * The ramp down, while it is running.
 *
 * A mute cannot be sent the moment it is asked for, or the ramp has nothing
 * to run through. So the mute is held back for as long as the ramp lasts and
 * the volume is walked down in the meantime.
 */
static uint32_t sDuckFromMs = 0;
static bool sDucking = false;
static int8_t sDuckFromDb = 0;

/*
 * How long to wait after a retune before the reading means anything.
 *
 * The reference firmware's own figure, from the delay in its seek loop. It
 * runs on this board, so this is measured rather than chosen. A whole FM band
 * at 100 kHz steps is 206 channels, so a full pass takes about ten seconds.
 */
#define SEEK_SETTLE_MS 50
static int16_t sSquelchThreshold = 0;

/*
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
 */
static bool publish(const RadioSettings *settings, const Tef668xQuality *q,
                    bool qualityValid, Tef668xError lastError, uint32_t drained,
                    const RadioError *results,
                    const Tef668xProcessing *processing, bool processingValid) {
  if (xSemaphoreTake(sLock, pdMS_TO_TICKS(RADIO_PUBLISH_WAIT_MS)) != pdTRUE) {
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
  sSnapshot.levelSmoothedTenths = sLevelSmoothed;
  sSnapshot.levelSmoothedValid = sLevelSmoothedValid;
  sSnapshot.rds = sRds.info;
  if (processingValid && processing != NULL) {
    sSnapshot.processing = *processing;
  }
  sSnapshot.processingValid = processingValid;
  sSnapshot.tunerReady = tef668xCapabilities() != NULL;
  sSnapshot.lastError = lastError;
  sSnapshot.bandwidthWide = sBandwidthWide;
  sSnapshot.tunerMuted = sLastPushedMute;
  sSnapshot.squelchMode = sSquelchMode;
  sSnapshot.seeking = sSeeking;
  sSnapshot.beeping = sBeeping;
  sSnapshot.seekFound = sSeekFound;
  sSnapshot.squelchOpen = sSquelch.open;
  sSnapshot.squelchThresholdTenths = sSquelchThreshold;
  sSnapshot.memorySlot = (int16_t)sMemorySlot;
  sSnapshot.updatedMs = millis();
  sSnapshot.sequence++;
  xSemaphoreGive(sLock);
  return true;
}

/*
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

/*
 * Tell the tuner what the settings now say, and only what changed.
 *
 * Order matters on a retune. Mute first so nothing bursts out while the
 * frequency moves, and unmute last. Only on a retune: a volume change that
 * muted and unmuted around itself would chop the audio, and the volume knob
 * sends one of those every fiftieth of a second while it is being turned.
 */
static Tef668xError pushToTuner(const RadioSettings *from,
                                const RadioSettings *to) {
  RadioPush push = radioPushNeeded(from, to);
  Tef668xError err = TEF668X_OK;
  bool fm = bandModulation(to->band) == MODULATION_FM;

  if (push.retune) {
    /* The mute failing is not a reason to stop. It is a reason to carry on to
     * the unmute at the bottom: a chip that may or may not be muted and is
     * never told otherwise stays silent for good, because every later attempt
     * mutes, fails at the same place, and never reaches the unmute. */
    err = tef668xSetMute(true);

    Tef668xError tuned =
        fm ? tef668xTuneFm(to->freqKHz) : tef668xTuneAm(to->freqKHz);
    if (err == TEF668X_OK) {
      err = tuned;
    }
    /* Sent with every FM tune, not once at start up. It restarts the chip's
     * decoder, and without that the first read after the dial moves hands
     * over the group the previous station left in the register. */
    if (tuned == TEF668X_OK && fm) {
      Tef668xError rdsOn = tef668xSetRds(false);
      if (err == TEF668X_OK) {
        err = rdsOn;
      }
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

  /* Going quiet comes before anything that clicks, and coming back comes
   * after it, the same order as a retune. Sending the filter first and the
   * mute second changes the filter while the audio is still live, which is
   * the click either of them is there to hide. */
  if (push.mute && to->muted) {
    if ((err = tef668xSetMute(true)) != TEF668X_OK) {
      return err;
    }
  }

  /* The dial did not move, but changing the filter clicks, so the audio is
   * muted across it the same way a retune is.
   *
   * This hush is the path taken when the ramp is off. With a ramp the task
   * has already walked the volume down, and the mute above has gone out, so
   * the radio is muted here and this does nothing.
   *
   * Only when the radio is not muted already: muting something that is muted
   * and unmuting it afterwards would turn the audio on. */
  if (push.bandwidth) {
    bool hush = !to->muted && !sLastPushedMute;
    if (hush) {
      /* A failure here is not a reason to stop, for the same reason as the
       * retune above: it has to reach the unmute. */
      (void)tef668xSetMute(true);
    }
    err = fm ? tef668xSetFmBandwidth(to->bandwidthKHz)
             : tef668xSetAmBandwidth(to->bandwidthKHz);
    if (hush) {
      Tef668xError back = tef668xSetMute(false);
      if (err == TEF668X_OK) {
        err = back;
      }
    }
    if (err != TEF668X_OK) {
      return err;
    }
  }
  if (push.volume) {
    if ((err = tef668xSetVolume(to->volumeDb)) != TEF668X_OK) {
      return err;
    }
  }
  if (push.mute && !to->muted) {
    if ((err = tef668xSetMute(false)) != TEF668X_OK) {
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

/*
 * How many channels one full pass of a band is.
 *
 * So a seek across a band with nothing on it ends instead of going round for
 * ever. Worked out from the band rather than fixed, because the FM band is
 * two hundred channels and long wave is sixteen.
 */
static uint32_t seekChannelsIn(BandId band, uint16_t stepKHz) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (stepKHz == 0 || !bandLimits(band, &sPlan, &lo, &hi)) {
    return 0;
  }
  return (hi - lo) / stepKHz + 1;
}

static void seekBegin(const RadioSettings *from, bool up) {
  sSeeking = true;
  sSeekUp = up;
  sSeekFound = false;
  sSeekVisited = 0;
  sSeekLimit = seekChannelsIn(from->band, from->stepKHz);
  if (sSeekLimit == 0) {
    sSeeking = false;
  }
}

/*
 * Stop a seek, whether it found something or not.
 *
 * Called from the drain loop for any other command, which is what makes seek
 * cancellable. The audio comes back either way: a radio left muted because a
 * seek was interrupted is a radio that has gone dead for no reason a person
 * can see.
 */
static void seekEnd(bool found) {
  sSeeking = false;
  sSeekFound = found;
}

static void radioTask(void *arg) {
  (void)arg;

  /* Taken from the snapshot, which radioTaskStart has already filled in with
   * the defaults and the frequency the radio is to come up on. Calling
   * radioDefaults again here would throw that away, and the radio would unmute
   * on the bottom of the FM band and then retune. */
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
  sLastPushedBandwidth = opening.bandwidthKHz;
  Tef668xError lastError = pushToTuner(NULL, &opening);
  bool pushFailed = lastError != TEF668X_OK;
  Tef668xQuality quality;
  memset(&quality, 0, sizeof(quality));
  Tef668xProcessing processing;
  memset(&processing, 0, sizeof(processing));
  bool processingOk = false;
  publish(&settings, &quality, false, lastError, 0, NULL, &processing, false);

  /* What the stored slot was last worked out for. 0 is not a frequency any
   * band has, so the first round always works it out. */
  uint32_t lastMemoryFreqKHz = 0;
  BandId lastMemoryBand = settings.band;
  /* The list itself can move under a dial that has not. Storing the station
   * already playing has to show its slot straight away, not only once the
   * dial has been turned off it and back. */
  uint32_t lastMemoryGeneration = 0;

  const TickType_t period = pdMS_TO_TICKS(RADIO_POLL_INTERVAL_MS);
  TickType_t nextPoll = xTaskGetTickCount() + period;
  const TickType_t rdsPeriod = pdMS_TO_TICKS(RADIO_RDS_INTERVAL_MS);
  TickType_t nextRds = xTaskGetTickCount() + rdsPeriod;
  rdsReset(&sRds, settings.freqKHz);
  bool qualityOk = false;
  /* Carried across a round that applied commands but could not publish them,
   * so a drain is never lost and the count never falls behind for good. */
  uint32_t owed = 0;
  RadioError owedResults[RADIO_QUEUE_DEPTH];

  for (;;) {
    RadioSettings wanted = settings;
    /* A jump is a band change or a typed frequency. A step is the knob. The
     * two are faded differently, so which one moved the dial has to be known
     * rather than worked out from the frequency afterwards. */
    bool jumped = false;
    /* Which slot a recall this round landed on. Kept rather than worked out
     * from the frequency afterwards, because two slots may hold the same
     * station and the one asked for is the one the radio is on. */
    int recalled = MEMORY_NO_SLOT;
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
    /* A seek walks one channel per round, so the round has to come quickly.
     * Waiting out the poll interval between channels would make a pass of the
     * FM band take a minute instead of ten seconds. */
    if (sSeeking) {
      wait = 0;
    }
    /* A ramp down needs the same treatment as a fade up, or it arrives in as
     * many steps as there are polls. A tone needs the loop back to stop it. */
    if (sDucking || sBeeping || sWakeSoon) {
      TickType_t step = pdMS_TO_TICKS(RADIO_FADE_STEP_MS);
      if (wait > step) {
        wait = step;
      }
    }
    /*
     * RDS keeps its own cadence, faster than the poll interval, so the round
     * has to come back in time for it.
     *
     * The same three conditions decide whether the round comes back early and
     * whether the read happens, and they are worked out once here so that the
     * two cannot come apart. Written separately, the wait was shortened while
     * the read was skipped, `nextRds` never moved, and the wait stayed at zero
     * for as long as the radio was hushed. That is the whole of an update over
     * the air, spent spinning on core 0 with nothing yielding to the idle task.
     */
    bool rdsRunning = sRdsEnabled &&
                      bandModulation(settings.band) == MODULATION_FM &&
                      !sSeeking && !sHushed;
    if (sRdsForget) {
      /* Switched off. Everything held describes a station this radio is no
       * longer listening to for RDS, and leaving it would be a name that
       * nothing is keeping true any more. */
      sRdsForget = false;
      rdsReset(&sRds, settings.freqKHz);
      sRdsRawStale = true;
    }
    if (rdsRunning) {
      TickType_t rdsWait = (int32_t)(nextRds - now) > 0 ? nextRds - now : 0;
      if (wait > rdsWait) {
        wait = rdsWait;
      }
    } else {
      /* Kept alongside the clock while it is not running, so a long spell on
       * medium wave or in a hush does not leave a deadline far enough in the
       * past for the comparison to wrap. */
      nextRds = now + rdsPeriod;
    }
    sWakeSoon = false;

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
        if (item.kind == RADIO_BEEP) {
          /* Started here and stopped below once it has run long enough, so a
           * tone never stalls the queue or a seek walking the band. */
          if (item.beepMs != 0) {
            sBeepUntilMs = millis() + item.beepMs;
            sBeepHz = item.beepHz;
            sBeepHz2 = item.beepHz2;
            sBeeping = true;
          }
          results[drained++] = RADIO_OK;
        } else if (item.kind == RADIO_SEEK) {
          seekBegin(&wanted, item.up);
          results[drained++] = RADIO_OK;
        } else if (item.kind == RADIO_RECALL) {
          if (sSeeking) {
            seekEnd(false);
          }
          sSeekFound = false;
          results[drained++] = recallChannel(&wanted, item.memorySlot);
          if (results[drained - 1] == RADIO_OK) {
            jumped = true;
            recalled = item.memorySlot;
          }
        } else if (item.kind == RADIO_STEP && item.steps != 0 &&
                   wanted.tuneMode == TUNE_MODE_MEMORY) {
          /* In memory mode the knob walks the stored list, not the dial. The
           * step is turned into a tune here rather than in the caller,
           * because only the radio knows which slot it is on and a caller
           * that read that, worked out the next one and sent it would be
           * racing the radio for the answer. */
          if (sSeeking) {
            seekEnd(false);
          }
          sSeekFound = false;
          int slot = sMemorySlot;
          int32_t moves = item.steps < 0 ? -(int32_t)item.steps : item.steps;
          /* A walk longer than the list repeats itself, so there is nothing
           * to gain past one lap. Without this a fast spin of the knob sends
           * thousands of steps, each one taking the list's lock and reading
           * all 99 slots, and the radio task holds up its own cadence to do
           * work whose answer it already had. */
          if (moves > MEMORY_SLOT_COUNT) {
            moves = MEMORY_SLOT_COUNT;
          }
          slot = memoryStoreStep(&sPlan, slot, item.steps > 0, (int)moves);
          results[drained++] = recallChannel(&wanted, slot);
          if (results[drained - 1] == RADIO_OK) {
            jumped = true;
            recalled = slot;
          }
        } else {
          /* This command may move the dial, so the slot a recall earlier in
           * this same drain landed on no longer describes where the radio
           * is. Without this the panel shows a slot number for a station the
           * radio is not on. */
          recalled = MEMORY_NO_SLOT;
          /* Anything else stops a seek where it stands. A person reaching
           * for the knob while the radio is hunting means stop, and so does
           * a script sending a tune.
           *
           * seekFound goes with it. It says where the radio is now, so once
           * somebody has tuned somewhere by hand it is no longer true. */
          if (sSeeking) {
            seekEnd(false);
          }
          sSeekFound = false;
          /* A step that comes back on the wrong side of where it started has
           * wrapped at a band edge. The encoder cannot tell, because it sends
           * a number of steps and never learns where they landed. */
          if (item.kind == RADIO_TUNE || item.kind == RADIO_SET_BAND ||
              item.kind == RADIO_CYCLE_BAND) {
            jumped = true;
          }
          uint32_t was = wanted.freqKHz;
          results[drained++] = radioApply(&wanted, &sPlan, &item);
          if (sBeepEdge && item.kind == RADIO_STEP && item.steps != 0 &&
              wanted.freqKHz != was) {
            bool wrapped =
                item.steps > 0 ? wanted.freqKHz < was : wanted.freqKHz > was;
            if (wrapped) {
              sBeepUntilMs = millis() + RADIO_BEEP_MS;
              sBeeping = true;
            }
          }
        }
      }
    }

    /* Worked out again whenever the dial has moved, and only then, so the
     * lock is taken once per retune rather than on every round. It says
     * where the radio is rather than what memory mode last did, so a station
     * reached with the keypad shows its slot if it has one. */
    if (sSeeking) {
      /* A seek moves the dial every round, and while it runs the radio is
       * walking noise rather than sitting on a channel. Looking the slot up
       * each time would take the list's lock hundreds of times a second to
       * answer about a frequency nobody is listening to. Clearing the
       * remembered frequency makes the first round after it stops look it up
       * again. */
      sMemorySlot = MEMORY_NO_SLOT;
      lastMemoryFreqKHz = 0;
    } else if (recalled != MEMORY_NO_SLOT) {
      lastMemoryFreqKHz = wanted.freqKHz;
      lastMemoryBand = wanted.band;
      lastMemoryGeneration = memoryStoreGeneration();
      sMemorySlot = recalled;
    } else {
      uint32_t generation = memoryStoreGeneration();
      if (wanted.freqKHz != lastMemoryFreqKHz ||
          wanted.band != lastMemoryBand || generation != lastMemoryGeneration) {
        lastMemoryFreqKHz = wanted.freqKHz;
        lastMemoryBand = wanted.band;
        lastMemoryGeneration = generation;
        sMemorySlot = memoryStoreFind((uint8_t)wanted.band, wanted.freqKHz);
      }
    }

    /* One channel per round while a seek is running. Moving the frequency
     * here means the push below carries the retune, so seeking goes through
     * exactly the same path as a person turning the knob and cannot drift
     * from it. */
    if (sSeeking) {
      wanted.freqKHz = sSeekUp ? bandStepUp(wanted.band, &sPlan, wanted.freqKHz,
                                            wanted.stepKHz)
                               : bandStepDown(wanted.band, &sPlan,
                                              wanted.freqKHz, wanted.stepKHz);
    }

    /*
     * A settle probe moves the dial the same way, so the retune it measures
     * is the ordinary one and not a path of its own.
     *
     * Never in a round that took a command off the queue. The dial the
     * command asked for would be overwritten here while the command had
     * already been recorded as applied, so a tune would answer success and
     * never happen. The probe waits for a quiet round instead.
     */
    bool probing = false;
    uint32_t probeKHz = 0;
    uint16_t probeMs = 0;
    uint8_t probeReads = 1;
    uint16_t probeGapMs = 0;
    uint32_t probeSeq = 0;
    if (!sSeeking && !sHushed && drained == 0 &&
        xSemaphoreTake(sLock, 0) == pdTRUE) {
      if (sProbeWanted) {
        probing = true;
        probeKHz = sProbeKHz;
        probeMs = sProbeMs;
        probeReads = sProbeReads;
        probeGapMs = sProbeGapMs;
        probeSeq = sProbeSeq;
      }
      xSemaphoreGive(sLock);
    }
    if (probing) {
      wanted.freqKHz = probeKHz;
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
    /* Changing the filter clicks, so it goes down the same ramp as a mute:
     * quiet first, then the filter, then back up.
     *
     * Only while the dial is still, and only when there is a ramp to use. A
     * retune is already silent while the frequency moves and carries the
     * filter with it, and with the ramp off the mute inside pushToTuner is
     * as close to instant as this gets. */
    bool filterMoving = sSoftMuteMs != 0 && !sSeeking &&
                        wanted.bandwidthKHz != sLastPushedBandwidth &&
                        wanted.band == settings.band &&
                        wanted.freqKHz == settings.freqKHz;
    /* Muted while the dial is moving, or a seek is a second of every station
     * and every patch of noise between them. The mute comes off when it
     * stops, including when it stops empty handed. */
    bool hushWanted =
        wanted.muted || !sSquelch.open || sSeeking || filterMoving;

    /* Going quiet is a ramp, not a step. The mute itself is held back until
     * the ramp has run, because a mute sent at the start would cut the audio
     * before the ramp had anything to walk down.
     *
     * A seek is exempt: it mutes and unmutes once per channel, and a ramp on
     * each would be most of the settle time. */
    if (sSoftMuteMs != 0 && !sSeeking) {
      if (hushWanted && !sLastPushedMute && !sDucking) {
        sDucking = true;
        sDuckFromMs = millis();
        sDuckFromDb = sLastPushedVolume;
      } else if (!hushWanted) {
        if (sLastPushedMute || sDucking) {
          /* Coming back. The fade up takes it from silence to the target over
           * the same length, so unmuting is a ramp as well rather than a step
           * from nothing to full. */
          uint32_t already = 0;
          if (sDucking) {
            /* Part way down rather than at the bottom, because the reason to
             * go quiet went away before the ramp finished. The fade starts
             * from the volume the radio is actually at, or it would drop the
             * rest of the way first and walk up from there. */
            int8_t nowDb = radioDuckVolume(sDuckFromDb, millis() - sDuckFromMs,
                                           sSoftMuteMs);
            already = radioFadeElapsedAt(wanted.volumeDb, nowDb, sSoftMuteMs);
          }
          sFadeFromMs = millis() - already;
          sFadeMs = sSoftMuteMs;
        }
        sDucking = false;
      }
    } else {
      sDucking = false;
    }
    bool duckDone =
        !sDucking || (uint32_t)(millis() - sDuckFromMs) >= sSoftMuteMs;
    if (sDucking && duckDone) {
      sDucking = false;
    }
    heard.muted = hushWanted && (!sDucking || duckDone);
    /* The filter waits for silence. Sending it while the audio is still up
     * is the click the ramp is there to remove. */
    if (filterMoving && !heard.muted) {
      heard.bandwidthKHz = sLastPushedBandwidth;
    }
    RadioSettings wasHeard = settings;
    wasHeard.muted = sLastPushedMute;
    wasHeard.volumeDb = sLastPushedVolume;
    wasHeard.bandwidthKHz = sLastPushedBandwidth;
    RadioPush push = radioPushNeeded(&wasHeard, &heard);

    /* The fade starts before the push that needs it, not after.
     *
     * Starting it afterwards let the retune go out carrying the full volume,
     * so the radio unmuted loud, dropped twenty five dB on the next pass, and
     * then ramped back. That is the opposite of the point, and it is subtle
     * enough to survive a listening test: it still ends in a ramp.
     *
     * Judged on push.retune as well as on which command arrived, because a
     * tune the state machine refused, or a band command naming the band the
     * radio is already on, moves nothing and should fade nothing.
     *
     * A step is not a jump. Turning the knob one click has to be instant, or
     * the dial feels slow, and a fade that restarts on every click never
     * finishes. So the fade is for a band change or a typed frequency. */
    if (push.retune && jumped && !firstPass) {
      sFadeFromMs = millis();
      sFadeMs = RADIO_BAND_FADE_MS;
    }

    /* The volume the tuner is actually given, which while a fade runs is on
     * its way up to the target. The target is read every time round, so the
     * knob still works during one. */
    heard.volumeDb =
        radioFadeVolume(wanted.volumeDb, millis() - sFadeFromMs, sFadeMs);
    if (sDucking) {
      /* On the way out, so the ramp down wins over any fade up. */
      heard.volumeDb =
          radioDuckVolume(sDuckFromDb, millis() - sDuckFromMs, sSoftMuteMs);
    } else if (heard.muted) {
      /* Held at the bottom for as long as the mute lasts.
       *
       * Without this the volume returns to the target on the round the mute
       * is applied, so the gain the tuner is carrying while it is silent is
       * the full listening level, and the unmute at the end of it is a step
       * from nothing straight to loud. That is a louder click than the one
       * the ramp exists to remove. */
      heard.volumeDb = RADIO_VOLUME_MIN;
    }
    if (heard.volumeDb != wasHeard.volumeDb) {
      push.volume = true;
    }

    /* No `changed` guard here. The squelch can move the mute with no command
     * having arrived at all, and a push that only happens when something was
     * drained would never act on it. Comparing the two is the whole test. */
    if (sHushed) {
      /* On the way to a reboot. radioHush has taken the audio down and is
       * the only thing allowed to talk to the tuner now. */
      push.retune = false;
      push.bandwidth = false;
      push.volume = false;
      push.mute = false;
      push.features = false;
      pushFailed = false;
    }
    if (push.retune || push.bandwidth || push.volume || push.mute ||
        push.features || pushFailed) {
      /* After a failure the tuner's state is not known, so everything goes
       * again rather than only what the settings say moved. */
      lastError = pushToTuner(pushFailed ? NULL : &wasHeard, &heard);
      pushFailed = lastError != TEF668X_OK;
      sLastPushedMute = heard.muted;
      sLastPushedVolume = heard.volumeDb;
      sLastPushedBandwidth = heard.bandwidthKHz;
      if (filterMoving && push.bandwidth) {
        /* The filter has just gone out at the bottom of the ramp, so the
         * next round is the one that brings the audio back. */
        sWakeSoon = true;
      }
      if (push.retune) {
        /* The readings from before the dial moved say nothing about where it
         * is now, and the chip has been through its active mode, which may
         * have taken the bandwidth option with it. Both start again. */
        signalAverageReset(&sLevelAverage);
        signalAverageReset(&sSnrAverage);
        signalAverageReset(&sDisplayLevelAverage);
        sLevelSmoothedValid = false;
        /* The squelch keeps its own average of the level and it means
         * nothing here any more. Left running, landing on a station from the
         * shoulder of another one would hold the audio shut for about a
         * second while the average climbed, which is the front of the
         * station gone. Only the average is started again: whether the audio
         * is open carries across a retune. */
        squelchRetuned(&sSquelch);
        sBandwidthKnown = false;
        /* Nothing the last station said is true of this one, and a name left
         * behind is a real name on the wrong station. The raw ring goes with
         * it: a capture holding the tail of the previous station and the
         * start of this one reads as one broadcast and is not. */
        rdsReset(&sRds, heard.freqKHz);
        sRdsRawStale = true;
      }
    }
    settings = wanted;

    /* The tone, started when the command arrived and stopped here. Checked
     * every round rather than waited out, so it never holds the queue.
     *
     * Not while hushed. The shutdown owns the bus from then on, and a tone
     * is the one thing here that would be heard. */
    if (sBeeping && !sHushed) {
      if (!sToneOn) {
        sToneOn = tef668xTone(true, RADIO_BEEP_AMPLITUDE, sBeepHz, sBeepHz2) ==
                  TEF668X_OK;
      }
      if ((int32_t)(millis() - sBeepUntilMs) >= 0) {
        /* Tried again next round if it fails. Turning the tone off is also
         * what puts the audio path back on the tuner, so giving up on it
         * would leave the radio on, tuned, unmuted, reporting a good signal
         * and silent until the next reboot. */
        if (tef668xTone(false, 0, 0, 0) == TEF668X_OK) {
          sToneOn = false;
          sBeeping = false;
          sWakeSoon = false;
        } else {
          lastError = TEF668X_ERR_WRITE;
        }
      }
    }

    /* The probe waits and reads exactly where a seek would, so what it
     * reports is what a seek at that settle time would have decided on. */
    if (probing) {
      bool fm = bandModulation(settings.band) == MODULATION_FM;
      Tef668xQuality look[RADIO_PROBE_MAX_READS];
      memset(look, 0, sizeof(look));
      /* A retune that did not go out leaves the tuner where it was, so the
       * readings would describe the parked channel while the answer named
       * the one that was asked for. */
      bool ok = !pushFailed;
      if (ok) {
        for (uint8_t i = 0; i < probeReads; i++) {
          vTaskDelay(pdMS_TO_TICKS(i == 0 ? probeMs : probeGapMs));
          if (tef668xReadQuality(fm, &look[i]) != TEF668X_OK) {
            ok = false;
          }
        }
      }
      if (xSemaphoreTake(sLock, pdMS_TO_TICKS(RADIO_PUBLISH_WAIT_MS)) ==
          pdTRUE) {
        memcpy(sProbeQuality, look, sizeof(sProbeQuality));
        sProbeOk = ok;
        sProbeDone = true;
        sProbeDoneSeq = probeSeq;
        /* Only if this is still the probe that was asked for. A caller that
         * gave up leaves its probe running, and clearing the flag without
         * looking would throw away the request that replaced it, so the next
         * caller waits out its whole timeout for a probe nobody kept. */
        if (sProbeSeq == probeSeq) {
          sProbeWanted = false;
        }
        xSemaphoreGive(sLock);
      }
    }

    /* The seek decision, taken on a reading of its own rather than on the one
     * the poll below takes. The poll runs on its own cadence and would often
     * be looking at the channel before this one, so the radio would stop one
     * channel past the station, or not at all. */
    if (sSeeking) {
      vTaskDelay(pdMS_TO_TICKS(SEEK_SETTLE_MS));
      bool seekFm = bandModulation(settings.band) == MODULATION_FM;

      /*
       * Everything the decision needs, copied out in one go.
       *
       * If the lock cannot be had, this channel is not judged at all. The
       * alternative was reading the four of them unlocked, and a torn
       * SquelchConfig would have the seek judging against a floor nobody set,
       * which is a wrong answer with nothing to show for it. This file
       * already takes the line that a reading which did not arrive is not a
       * station, and a rule that did not arrive is not a rule. The cost is
       * walking past one channel on a round where the lock was busy, and the
       * seek is still moving, so the next round judges the next one.
       */
      SeekConfig cfg;
      seekDefaults(&cfg);
      bool haveRules = xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE;
      if (haveRules) {
        cfg = sSeekConfig;
        /* Handed over whole rather than turned into a number here. The seek
         * asks the squelch the same question the squelch will ask itself, so
         * the two cannot disagree about the level, the multipath, how far off
         * centre a carrier may sit, or anything added later. */
        cfg.checkAudible = true;
        cfg.squelchMode = sSquelchMode;
        cfg.squelchCfg = sSquelchCfg;
        cfg.squelchThresholdTenths = sSquelchThreshold;
        xSemaphoreGive(sLock);
      }

      Tef668xQuality look;
      memset(&look, 0, sizeof(look));
      SeekReading found;
      memset(&found, 0, sizeof(found));
      /* Not read at all when there is no rule to judge it by, so a channel is
       * never stopped on by a decision made against nothing. */
      found.valid =
          haveRules && tef668xReadQuality(seekFm, &look) == TEF668X_OK;
      found.levelTenths = look.levelDbuVTenths;
      found.noiseTenths = look.usnTenths;
      found.multipathTenths = look.multipathTenths;
      found.offsetTenths = look.offsetKHzTenths;

      if (seekShouldStop(&cfg, settings.band, &found)) {
        seekEnd(true);
        /* The readings taken while walking say nothing about where it has
         * stopped, and the next thing to read them is the bandwidth
         * extension. The squelch starts again for the same reason, open, so
         * that a station it has just found is not held shut by a hold that
         * began on a noise channel. */
        signalAverageReset(&sLevelAverage);
        signalAverageReset(&sSnrAverage);
        signalAverageReset(&sDisplayLevelAverage);
        sLevelSmoothedValid = false;
        sBandwidthKnown = false;
        squelchInit(&sSquelch);
        /* Fade in, the same as a band change, so a station does not arrive
         * at full volume the instant the mute lifts. */
        sFadeFromMs = millis();
        sFadeMs = RADIO_BAND_FADE_MS;
      } else if (++sSeekVisited >= sSeekLimit) {
        /* One full pass and nothing. Stop rather than go round again, and
         * leave the radio where it ended up rather than pretending. */
        seekEnd(false);
        squelchInit(&sSquelch);
      }
    }

    /*
     * The RDS decoder, on its own faster cadence.
     *
     * Not while seeking: the dial is passing channels nobody is listening to,
     * and a group picked up from one of them would be decoded as though it
     * belonged to wherever the seek stops. Not while hushed either, because
     * then the tuner belongs to whoever is taking the radio down.
     */
    now = xTaskGetTickCount();
    /* Worked out again rather than reused. A command drained this round can
     * have started a seek or crossed to the AM side since the wait above was
     * decided, and then reading RDS would take a group from a channel the
     * dial is only passing through. */
    rdsRunning = sRdsEnabled &&
                 bandModulation(settings.band) == MODULATION_FM && !sSeeking &&
                 !sHushed;
    if (rdsRunning && (int32_t)(now - nextRds) >= 0) {
      Tef668xRdsRead raw;
      RdsRead read;
      memset(&read, 0, sizeof(read));
      memset(&raw, 0, sizeof(raw));
      Tef668xError rdsErr = tef668xReadRds(&raw);
      if (rdsErr == TEF668X_OK) {
        read.synchronised = raw.synchronised;
        read.haveGroup = raw.haveGroup;
        for (int i = 0; i < 4; i++) {
          read.block[i] = raw.block[i];
          read.error[i] = raw.error[i];
        }
      }
      /* Fed whatever came back, including a read that failed, which counts
       * as a read with no lock and no group. Skipping it would hold the last
       * lock state for as long as the bus stayed broken. */
      rdsFeed(&sRds, &read);

      /*
       * The ring and the status word are read from the web task, so they are
       * written under the same lock as the snapshot. The decoder above is
       * not: it belongs to this task alone and is copied out at publish
       * time, so nothing a person sees waits on this.
       *
       * A group the lock was too busy for is counted rather than dropped
       * quietly. Losing one costs a capture one group, and a capture with a
       * hole in it that says so is usable where one that does not is not.
       */
      if (xSemaphoreTake(sLock, pdMS_TO_TICKS(RADIO_RDS_RING_WAIT_MS)) ==
          pdTRUE) {
        if (sRdsRawStale) {
          sRdsRawTotal = 0;
          sRdsRawDropped = 0;
          sRdsRawStale = false;
        }
        sRdsStatusWord = raw.status;
        sRdsStatusRead = raw.read;
        if (raw.haveGroup) {
          RadioRdsRaw *slot = &sRdsRaw[sRdsRawTotal % RADIO_RDS_RAW_DEPTH];
          for (int i = 0; i < 4; i++) {
            slot->block[i] = raw.block[i];
          }
          slot->error = (uint8_t)((raw.error[0] << 6) | (raw.error[1] << 4) |
                                  (raw.error[2] << 2) | raw.error[3]);
          sRdsRawTotal++;
        }
        xSemaphoreGive(sLock);
      } else if (raw.haveGroup) {
        sRdsRawDropped++;
      }

      nextRds += rdsPeriod;
      /* A slow round can leave the next read already past. Start from now
       * rather than firing several in a row to catch up. */
      if ((int32_t)(xTaskGetTickCount() - nextRds) >= 0) {
        nextRds = xTaskGetTickCount() + rdsPeriod;
      }
    }

    /* The reading keeps its own cadence, whatever the commands are doing. */
    now = xTaskGetTickCount();
    if ((int32_t)(now - nextPoll) >= 0) {
      bool fm = bandModulation(settings.band) == MODULATION_FM;
      qualityOk = tef668xReadQuality(fm, &quality) == TEF668X_OK;
      if (qualityOk) {
        /* Every band and every reading that arrived. A failed read leaves the
         * struct holding the last one, and feeding that in again would count
         * the same sample twice and make the meter creep towards a number
         * nothing measured. */
        sLevelSmoothed =
            signalAverage(&sDisplayLevelAverage, quality.levelDbuVTenths);
        sLevelSmoothedValid = true;
      }

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
      /* Copied out, not read in place. The thresholds are written from other
       * tasks, and squelchUpdate reads them several times while it decides,
       * so a copy is what makes that decision see one consistent set. */
      SquelchConfig cfg;
      if (xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE) {
        mode = sSquelchMode;
        threshold = sSquelchThreshold;
        cfg = sSquelchCfg;
        xSemaphoreGive(sLock);
      } else {
        mode = sSquelchMode;
        threshold = sSquelchThreshold;
        cfg = sSquelchCfg;
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
        if ((wantWide != sBandwidthWide || !sBandwidthKnown) && !sHushed) {
          if (tef668xSetBandwidthExtension(wantWide) == TEF668X_OK) {
            sBandwidthWide = wantWide;
            sBandwidthKnown = true;
          }
        }
      }

      bool wasOpen = sSquelch.open;
      /* Not while seeking. The readings then come from whatever channel the
       * sweep is passing, which nobody is listening to, and they would drive
       * the hold and the hysteresis on noise. The squelch is started again
       * when the seek stops. */
      if (!sSeeking) {
        squelchUpdate(&sSquelch, &cfg, mode, settings.band, &reading, threshold,
                      millis());
      }

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
        /* Same three reasons the main push uses, seeking included. Without
         * it a seek sweeping past a strong station would open the squelch
         * and blare that channel until the next round re-muted it. */
        bool wantMuted = settings.muted || !sSquelch.open || sSeeking;
        if (wantMuted != sLastPushedMute && !sHushed) {
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
  /* Zero is slot 0, which reads as channel 1, and the task has not looked
   * anything up yet. Until it does, the honest answer is that the radio is on
   * no stored channel. */
  sSnapshot.memorySlot = MEMORY_NO_SLOT;

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
  squelchDefaults(&sSquelchCfg);
  if (settings != NULL) {
    sSquelchCfg.fmLevelFloorTenths =
        settings->fmSquelchFloor == 0
            ? SQUELCH_LEVEL_FLOOR_OFF
            : (int16_t)(settings->fmSquelchFloor * 10);
  }
  signalAverageReset(&sLevelAverage);
  signalAverageReset(&sSnrAverage);
  signalAverageReset(&sDisplayLevelAverage);
  sLevelSmoothed = 0;
  sLevelSmoothedValid = false;
  sBandwidthKnown = false;
  sSquelchMode = SQUELCH_OFF;
  if (settings != NULL && settings->squelchMode < SQUELCH_MODE_COUNT) {
    sSquelchMode = (SquelchMode)settings->squelchMode;
  }
  sSquelchThreshold = 0;
  sLastPushedMute = false;
  sSeeking = false;
  sSeekFound = false;
  sBeeping = false;
  sToneOn = false;
  sDucking = false;
  seekDefaults(&sSeekConfig);
  if (settings != NULL) {
    sSoftMuteMs = settings->softMuteMs;
  }
  if (settings != NULL) {
    sSeekConfig.fmSensitivity = settings->fmScanSensitivity;
    sSeekConfig.amSensitivity = settings->amScanSensitivity;
  }

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

/*
 * Put a command on the queue and say which one it was.
 *
 * The number and the queue move together under the lock, so a ticket is never
 * handed out for a command that was not queued, and two callers at once
 * cannot be given the same one.
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

bool radioBeep(uint16_t ms) {
  return radioBeepAt(ms, RADIO_BEEP_HZ, RADIO_BEEP_HZ);
}

bool radioBeepAt(uint16_t ms, uint16_t hz, uint16_t hz2) {
  if (ms == 0) {
    return true;
  }
  /* The pitch travels on the command, not in a global the caller writes and
   * the task reads later. Two beeps asked for at once would otherwise both
   * play at whichever pitch was written last. */
  RadioCommand cmd = {};
  cmd.kind = RADIO_BEEP;
  cmd.beepMs = ms;
  cmd.beepHz = hz;
  cmd.beepHz2 = hz2;
  return radioPost(&cmd);
}

void radioResume(void) {
  if (!sHushed) {
    /* Nothing was hushed, so there is nothing to undo. Without this the
     * lines below would tell the task the tuner is muted and quiet when it
     * is playing, and it would answer with an unmute and a fade up that
     * nobody asked for. */
    return;
  }
  if (sTask == NULL) {
    /* The mirror of radioHush with no task: it wrote the mute straight to
     * the chip, so this takes it off the same way. main.cpp unmutes the
     * tuner when the task fails to start, so a radio in that state is still
     * playing and has just as much to lose. */
    tef668xSetMute(false);
    sHushed = false;
    return;
  }

  /* What the tuner is actually holding, written here rather than left to
   * whatever the task last recorded.
   *
   * radioHush sets the flag first and writes these two last, with a ramp in
   * between that holds the bus for the length of the ramp. A round that read
   * the flag as false just before can still be inside pushToTuner for all of
   * that, and it records what it pushed afterwards. Its record would then be
   * unmuted and loud, which is the opposite of what the chip holds, and the
   * next round would find nothing to put right and leave the radio silent
   * for good. Saying it plainly here does not depend on who finished last. */
  sLastPushedMute = true;
  sLastPushedVolume = RADIO_VOLUME_MIN;
  sHushed = false;
}

void radioSetSoftMuteMs(uint16_t ms) {
  sSoftMuteMs = ms;
}

void radioSetSquelchFloor(uint8_t dbuv) {
  int16_t tenths = dbuv == 0 ? SQUELCH_LEVEL_FLOOR_OFF : (int16_t)(dbuv * 10);
  if (sLock != NULL && xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE) {
    sSquelchCfg.fmLevelFloorTenths = tenths;
    xSemaphoreGive(sLock);
    return;
  }
  /* The lock is only ever held long enough to copy a struct, so this is not
   * reached in practice. Writing anyway beats dropping the change: a single
   * aligned store cannot tear, and the radio task reads it on its next
   * round. */
  sSquelchCfg.fmLevelFloorTenths = tenths;
}

void radioSetEdgeBeep(bool on) {
  sBeepEdge = on;
}

void radioHush(void) {
  /* Walked down here rather than posted as a command, because the caller is
   * about to reboot and would not wait for the task to get round to it.
   *
   * The tuner belongs to the radio task, so this is the one place that
   * reaches past that. It is safe because of sHushed: the task stops pushing
   * once that is set, which it checks every round. Without it the task would
   * see its own record of the volume and the mute disagree with what this
   * wrote and put them straight back, and two tasks would be on the I2C bus
   * at once for the rest of the shutdown. */
  sHushed = true;
  if (sTask == NULL) {
    /* No task ever started, so nothing else is talking to the tuner. */
    tef668xSetMute(true);
    return;
  }

  int8_t from = sLastPushedVolume;
  uint16_t ms = sSoftMuteMs;
  if (ms != 0 && !sLastPushedMute) {
    uint32_t start = millis();
    for (;;) {
      uint32_t elapsed = millis() - start;
      if (elapsed >= ms) {
        break;
      }
      tef668xSetVolume(radioDuckVolume(from, elapsed, ms));
      vTaskDelay(pdMS_TO_TICKS(RADIO_FADE_STEP_MS));
    }
  }
  tef668xSetMute(true);
  sLastPushedMute = true;
  sLastPushedVolume = RADIO_VOLUME_MIN;
}

bool radioSeek(bool up) {
  RadioCommand cmd = {};
  cmd.kind = RADIO_SEEK;
  cmd.up = up;
  return radioPost(&cmd);
}

void radioSetSeekConfig(const SeekConfig *cfg) {
  SeekConfig wanted;
  if (cfg != NULL) {
    wanted = *cfg;
  } else {
    seekDefaults(&wanted);
  }
  if (sLock != NULL && xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE) {
    sSeekConfig = wanted;
    xSemaphoreGive(sLock);
    return;
  }
  sSeekConfig = wanted;
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

RadioProbeResult radioSettleProbe(uint32_t khz, uint16_t settleMs,
                                  uint8_t reads, uint16_t gapMs,
                                  Tef668xQuality *out, bool *moved) {
  if (out == NULL || sLock == NULL || sTask == NULL || settleMs == 0 ||
      settleMs > RADIO_PROBE_MAX_MS || reads == 0 ||
      reads > RADIO_PROBE_MAX_READS || gapMs > RADIO_PROBE_MAX_MS) {
    return RADIO_PROBE_REFUSED;
  }
  /* The radio task waits all of this out with the tuner untouched, so the
   * poll, the squelch and the RDS read stop for as long as it lasts. */
  if ((uint32_t)settleMs + (uint32_t)gapMs * (reads - 1) >
      RADIO_PROBE_MAX_TOTAL_MS) {
    return RADIO_PROBE_REFUSED;
  }

  RadioSnapshot now;
  if (!radioGetSnapshot(&now)) {
    return RADIO_PROBE_REFUSED;
  }
  /* The probe sets the frequency straight into the working state, so nothing
   * else checks it. A frequency outside the band would be pushed to the tuner
   * as it stands and the reading would describe wherever the chip landed. */
  if (!bandContains(now.settings.band, &sPlan, khz)) {
    return RADIO_PROBE_REFUSED;
  }
  if (now.seeking) {
    return RADIO_PROBE_REFUSED;
  }
  /* Refused while the radio is on its way down, the same as the header says.
   * The task will not run a probe then, so without this the caller sits out
   * the whole timeout and is then told something untrue about its
   * frequency. */
  if (sHushed) {
    return RADIO_PROBE_REFUSED;
  }
  if (moved != NULL) {
    *moved = now.settings.freqKHz != khz;
  }

  if (xSemaphoreTake(sLock, pdMS_TO_TICKS(RADIO_PUBLISH_WAIT_MS)) != pdTRUE) {
    return RADIO_PROBE_REFUSED;
  }
  uint32_t mine = ++sProbeSeq;
  sProbeKHz = khz;
  sProbeMs = settleMs;
  sProbeReads = reads;
  sProbeGapMs = gapMs;
  sProbeDone = false;
  sProbeOk = false;
  sProbeWanted = true;
  xSemaphoreGive(sLock);

  /* Long enough for the round it lands in plus the wait it asks for. */
  uint32_t waited = 0;
  const uint32_t limit = (uint32_t)settleMs + (uint32_t)gapMs * reads + 2000;
  while (waited < limit) {
    vTaskDelay(pdMS_TO_TICKS(10));
    waited += 10;
    bool done = false;
    bool ok = false;
    if (xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE) {
      /* Only this caller's own probe. One that gave up earlier can still be
       * in flight, and its readings are about a frequency nobody here
       * asked about. */
      done = sProbeDone && sProbeDoneSeq == mine;
      if (done) {
        for (uint8_t i = 0; i < reads; i++) {
          out[i] = sProbeQuality[i];
        }
        ok = sProbeOk;
      }
      xSemaphoreGive(sLock);
    }
    if (done) {
      return ok ? RADIO_PROBE_OK : RADIO_PROBE_NO_READ;
    }
  }

  /* Give up rather than leave a probe queued for whatever the dial does
   * next. One already running still finishes and still stamps its number, and
   * the next caller will not match it. */
  if (xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE) {
    if (sProbeSeq == mine) {
      sProbeWanted = false;
    }
    xSemaphoreGive(sLock);
  }
  return RADIO_PROBE_NO_ANSWER;
}

void radioSetRdsEnabled(bool on) {
  if (sRdsEnabled == on) {
    return;
  }
  sRdsEnabled = on;
  if (!on) {
    sRdsForget = true;
  }
}

bool radioRdsEnabled(void) {
  return sRdsEnabled;
}

bool radioRdsStatus(uint16_t *status, bool *read) {
  if (sLock == NULL || xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) != pdTRUE) {
    return false;
  }
  /* The same as the ring: what the chip last said was about the station the
   * dial has left. */
  if (status != NULL) {
    *status = sRdsRawStale ? 0 : sRdsStatusWord;
  }
  if (read != NULL) {
    *read = sRdsRawStale ? false : sRdsStatusRead;
  }
  xSemaphoreGive(sLock);
  return true;
}

uint16_t radioRdsRaw(RadioRdsRaw *out, uint16_t max, uint32_t *firstSequence,
                     uint32_t *total, uint32_t *dropped) {
  if (out == NULL || max == 0 || sLock == NULL) {
    return 0;
  }
  if (xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) != pdTRUE) {
    return 0;
  }
  /* Emptied on a retune, and the emptying may not have happened yet. Serving
   * what is in there now would hand back the station the dial has left. */
  uint32_t held = sRdsRawStale ? 0 : sRdsRawTotal;
  if (held > RADIO_RDS_RAW_DEPTH) {
    held = RADIO_RDS_RAW_DEPTH;
  }
  if (held > max) {
    /* The newest are the ones worth keeping. A caller with a small buffer
     * asking again would otherwise be handed the same oldest groups for ever
     * while the ring moved on underneath it. */
    held = max;
  }
  uint32_t first = sRdsRawTotal - held;
  for (uint32_t i = 0; i < held; i++) {
    out[i] = sRdsRaw[(first + i) % RADIO_RDS_RAW_DEPTH];
  }
  if (firstSequence != NULL) {
    *firstSequence = first;
  }
  if (total != NULL) {
    *total = sRdsRawStale ? 0 : sRdsRawTotal;
  }
  if (dropped != NULL) {
    *dropped = sRdsRawDropped;
  }
  xSemaphoreGive(sLock);
  return (uint16_t)held;
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
