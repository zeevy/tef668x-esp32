/* Implementation of the radio task, its queue and its snapshot. */
#include "radio_task.h"
#include "debug_log.h"

#include "band_scan_task.h"
#include "core/agc.h"
#include "core/auto_off.h"
#include "core/radio_round.h"
#include "core/signal.h"
#include "memory_store.h"
#include "net/update_check.h"

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <string.h>
#include <algorithm>
#include <atomic>

/* Core 0, which it shares with the Wi-Fi driver and lwIP: the framework pins
 * both there. Core 1 runs the Arduino loop: the panel, the keys and the web
 * server's requests. */
#define RADIO_TASK_CORE 0

/* Above the idle task and the timer service on core 0. lwIP at 18 and the
 * Wi-Fi driver above it still come first, so a burst of traffic can push a
 * round back by its length. */
#define RADIO_TASK_PRIORITY 3

/*
 * Measured, not guessed: `rad` in GET /api/state is the part of this stack
 * the task has never reached. Driven through start up, seeks, a band scan,
 * a DX level sweep, a series of AF checks and every band, the most it used
 * was 2372 bytes, nearly all of it at start up while the tuner is brought
 * up, which leaves 1724, 42 %. The rule is that at least a quarter of each
 * stack stays unreached, so this holds with room; measure again before
 * making it smaller.
 */
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
 * The slot a person last chose, by a recall or a knob step in memory mode.
 * Only the radio task touches it.
 *
 * Two slots can hold one station, and a lookup by frequency finds the lowest.
 * So this is kept across anything that walks the dial, a seek, a band scan or
 * a probe, and the radio names it again whenever the dial is back on the
 * station it holds.
 */
static int sChosenSlot = MEMORY_NO_SLOT;

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
 * capture that reads as one broadcast and is not, and nothing reading the
 * ring can tell.
 *
 * Atomic, because the radio task sets it without the lock and other tasks
 * read it under the lock. The radio task is the only writer. Once the retune
 * has happened the flag is true and stays true until the radio task itself
 * clears it, so a reader after a retune can never be handed the station
 * before it.
 */
static std::atomic<bool> sRdsRawStale{false};

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
 * be an ordinary looking line in a capture and nothing could tell.
 */
static uint32_t sProbeSeq = 0;
static uint32_t sProbeDoneSeq = 0;
static uint8_t sProbeReads = 1;
static uint16_t sProbeGapMs = 0;
static bool sProbeDone = false;
static bool sProbeOk = false;
/* A band scan is running. Written by the band scan on the loop task and read
 * by the radio task, so it is atomic rather than a plain bool. */
static std::atomic<bool> sScanning{false};

/* Auto off wants the sound faded down, set from the loop task. The task's
 * own record of when its fade began is beside it, read only here. */
static std::atomic<bool> sSleepFadeWanted{false};
/* How long the fade takes, radioSetSleepFade's `overMs`. */
static std::atomic<uint32_t> sSleepFadeMs{AUTO_OFF_FADE_MS};
static bool sSleepFading = false;
static uint32_t sSleepFromMs = 0;
/* On the way back up from it, from where it had got to. */
static bool sSleepBack = false;
static uint32_t sSleepBackFromMs = 0;
static int8_t sSleepBackFromDb = 0;
/* Where the caller said the dial is, or 0, and whether it was somewhere else
 * when the probe came to run. */
static uint32_t sProbeExpectKHz = 0;
static bool sProbeMoved = false;
static Tef668xQuality sProbeQuality[RADIO_PROBE_MAX_READS];

/*
 * Marks probe `seq` answered, with sLock held.
 *
 * The flag is cleared only if this is still the probe that was asked for. A
 * caller that gave up leaves its probe running, and clearing the flag without
 * looking would throw away the request that replaced it, so the next caller
 * waits out its whole timeout for a probe nobody kept.
 */
static void probeAnswered(uint32_t seq) {
  sProbeDone = true;
  sProbeDoneSeq = seq;
  if (sProbeSeq == seq) {
    sProbeWanted = false;
  }
}

/* The level sweep asked for, the caller's buffer, until it is done, and
 * whether the caller wants it ended. */
static std::atomic<DxSweep *> sSweepOut{nullptr};
static std::atomic<bool> sSweepCancel{false};
/* The plan of the sweep waiting or running; `sSweepPlanned` false for DX
 * mode's whole band at the tuner's width. Written before `sSweepOut` is set
 * and read by the radio task only after. */
static RadioSweepPlan sSweepPlan;
static bool sSweepPlanned = false;
/* The band the sweep was asked for on, which its plan was checked against. */
static BandId sSweepBand = BAND_FM;

/* The AF_Update checks: the caller's series while one runs, NULL once it has
 * ended. The rest is the radio task's own. */
static std::atomic<RadioAfSeries *> sAfOut{nullptr};
static std::atomic<bool> sAfCancel{false};

/* The web server is in a call that may be sending a reply: the Wi-Fi
 * transmitting raises the level read at that moment. */
static std::atomic<bool> sNetServing{false};

/* How long a sweep waits for a reply to finish before its first channel. A
 * reply is served in a few milliseconds; the cap keeps a web server that
 * never lets go from holding the sweep. */
#define SWEEP_NET_WAIT_MS 200
static std::atomic<uint16_t> sAfDone{0};
static bool sAfStarted = false;
static uint32_t sAfNextMs = 0;
static uint32_t sAfDialKHz = 0;

/* Whether the decoder runs. A setting, so it is read on every round rather
 * than captured once. */
static std::atomic<bool> sRdsEnabled{true};
/* Set when it is switched off, so the round that notices throws away what the
 * decoder held. Doing it in radioSetRdsEnabled would touch the decoder from
 * another task. */
static std::atomic<bool> sRdsForget{false};

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

/*
 * What the other tasks set for the radio task to judge by: the seek's rule,
 * the squelch's mode, threshold and thresholds, and the AGC's target and
 * boost. One struct under sLock, written by the setters and copied out once a
 * round, so one round judges by one consistent set however many fields
 * become settable later. A target that landed a round before its boost would
 * run one round at a gain nobody asked for. Of the squelch thresholds only
 * the level floor is settable; the rest are the measured defaults.
 */
typedef struct {
  SeekConfig seek;
  SquelchMode squelchMode;
  int16_t squelchThresholdTenths;
  SquelchConfig squelch;
  AgcConfig agc;
} RadioLive;
static RadioLive sLive;

/* ------------------------------------------------------------------ seek --
 *
 * Owned by the radio task and touched from nowhere else. Its rule is in
 * sLive, set under the lock like the squelch mode is.
 *
 * Seek is a state machine rather than a loop, so that a command arriving in
 * the middle of it is acted on within one channel, about 50 ms, instead of
 * after the radio has finished walking the band. That is what makes it
 * cancellable.
 */
static SeekWalk sSeek;

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
 * task every round. Atomic, as the two below are, so the other core sees it
 * and every write made before it in the order they were made, which the
 * hush and the resume rely on. */
static std::atomic<bool> sHushed{false};

/*
 * The volume AGC. What it is set to is in sLive.
 *
 * It decides a number of dB and this file decides whether that number reaches
 * the chip, which is what core/agc.h says the split is. The gain sits alongside
 * the fade and the duck: all three change the volume the tuner is given without
 * touching the volume the person asked for. The knob still owns
 * `settings.volumeDb` and nothing here writes it.
 */
static Agc sAgc;
/*
 * The gain that reached the chip, which is not always the gain the AGC asked
 * for.
 *
 * It is clamped to what the chip takes, so a boost of 8 dB on a volume
 * already at the top adds nothing. Publishing what was asked for would put a
 * number in the state document that the audio does not match, and the whole
 * reason that number is published is so the AGC can be tuned by reading it.
 */
static int8_t sAgcApplied = 0;

/*
 * Asked for by a shutdown, answered by the radio task: park and touch
 * nothing.
 *
 * sHushed stops the task writing to the tuner, which is enough for the two
 * hundred milliseconds of a reboot. A firmware write lasts fifteen seconds or
 * more, and without this the task keeps reading the quality every hundred
 * milliseconds, reading a group every forty three, running the squelch and
 * publishing a snapshot for all of it. A radio being replaced has no
 * business doing any of it.
 *
 * The reads are the part that matters. The bus lock keeps each transaction
 * whole, so the two tasks' bytes never mix on the wire, but it does not keep
 * two tasks from both changing the tuner. radioHush ramps the audio down from
 * the calling task, and between two of its writes the radio task could be in
 * a read, a retune or a push of its own. Parking it first leaves the tuner
 * one task's.
 *
 * Two flags and not one, because the caller has to know the task has
 * actually stopped rather than that it has been asked to. Asked on one side,
 * answered on the other, both written by one task and read by the other.
 */
static std::atomic<bool> sStopWanted{false};
static std::atomic<bool> sStopped{false};

/*
 * How long a caller waits for the radio task to park, in milliseconds.
 *
 * A round is a hundred milliseconds at its longest, plus the ramp of a fade
 * and the settle of a seek, so half a second is several times what it takes.
 * A caller that waits it out carries on anyway: a firmware write that
 * refused to start because the radio task was slow to answer would be worse
 * than one that shares the bus for a round.
 */
#define RADIO_STOP_WAIT_MS 500

/* How often a parked task looks to see whether it may run again. */
#define RADIO_STOP_POLL_MS 20

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

/* Whether the dial wrapping at a band edge makes a sound. Off unless asked.
 * Set from the loop task and read by the radio task, so atomic, like the RDS
 * switch and the ramp length below. */
static std::atomic<bool> sBeepEdge{false};

/* How long the audio ramps down before it is cut. Set from the settings. */
static std::atomic<uint16_t> sSoftMuteMs{RADIO_SOFT_MUTE_MS};

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

/* Counted at the read and not in publish, because publish runs every pass of
 * the loop and the tuner is read every hundred milliseconds. */
static uint32_t sQualityReads = 0;

/* ------------------------------------------------------------ the round --
 *
 * What the radio task carries from one round to the next, and what the round
 * running has worked out so far. Each step of the loop below takes it.
 *
 * Kept here rather than on the task's stack, which it would take a third of.
 * The radio task alone touches it.
 */
typedef struct {
  /* Carried from round to round. */
  RadioSettings settings; /* What the last round settled on. */
  Tef668xError lastError;
  /* pushFailed carries a failure forward, so a retune is attempted again
   * next time round rather than being forgotten. Without it the task
   * records the settings as applied even when the push failed, and then
   * asking for the same frequency again changes nothing that
   * radioNeedsRetune can see, so the radio cannot be recovered by
   * repeating the command. */
  bool pushFailed;
  /* The first pass through the loop has already had its fade started, by the
   * opening push. Starting another one there would drop the volume the
   * moment the radio came up. */
  bool firstPass;
  Tef668xQuality quality;
  bool qualityOk;
  Tef668xProcessing processing;
  bool processingOk;
  TickType_t nextPoll;
  TickType_t nextRds;
  /* Carried across a round that applied commands but could not publish them,
   * so a drain is never lost and the count never falls behind for good.
   * Their results stay at the front of `results`. */
  uint32_t owed;
  /* The number of the probe whose readings are stored and whose answer goes
   * out with the next publish, kept across rounds while that publish cannot
   * take the lock. 0 is none; a probe is never given 0. */
  uint32_t probeOwed;
  /* What the stored slot was last worked out for. 0 is not a frequency any
   * band has, so the first round always works it out. */
  uint32_t lastMemoryFreqKHz;
  BandId lastMemoryBand;
  /* The list itself can move under a dial that has not. Storing the station
   * already playing has to show its slot straight away, not only once the
   * dial has been turned off it and back. */
  uint32_t lastMemoryGeneration;
  /* And a slot chosen by a recall can be on the frequency the dial was
   * already on, when two slots hold one station. */
  int lastChosenSlot;
  /* What the other tasks set, as last copied, and whether this round's copy
   * came. A copy that did not come leaves the last one. */
  RadioLive live;
  bool haveLive;

  /* This round. */
  RadioSettings wanted;   /* What the commands asked for. */
  RadioSettings heard;    /* What the tuner is told. */
  RadioSettings wasHeard; /* What it was last told. */
  RadioPush push;
  /* Read once, so one round sees one answer however the loop task moves
   * them meanwhile. */
  bool scanning;
  uint16_t softMuteMs;
  /* A jump is a band change or a typed frequency. A step is the knob. The
   * two are faded differently, so which one moved the dial has to be known
   * rather than worked out from the frequency afterwards. */
  bool jumped;
  /* Whether an Auto mode step has already been acted on this round. A
   * continuous spin can post several queued steps before the radio task
   * gets back to the queue, and reacting to each one against the seek's own
   * state as it goes is how a spin held in one direction would stop and
   * restart the seek every couple of items instead of leaving it alone. One
   * decision a round is a turn of the knob; several in the same round are
   * that same turn arriving in pieces. */
  bool autoSeekTurned;
  uint32_t drained;
  RadioError results[RADIO_QUEUE_DEPTH];
  bool filterMoving;
  /* A settle probe to run this round, and what it asked for. */
  bool probing;
  uint32_t probeKHz;
  uint16_t probeMs;
  uint8_t probeReads;
  uint16_t probeGapMs;
  uint32_t probeSeq;
  /* Or a level sweep to run, when there is no probe. */
  DxSweep *sweepOut;
} RadioRound;

static RadioRound sRound;

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
  /* A channel list runs across the bands, so walking it moves band. Recalling a
   * channel is not a person choosing a tuning mode, so whatever mode they were
   * in is kept across that band change. */
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
 *
 * A settle probe's answer, when `probeSeq` is not 0, goes out in the same step
 * for the same reason. A caller that walks the dial, like the band scan,
 * checks the snapshot against where it left the dial before its next probe.
 * Told sooner, it reads the channel before the probed one, takes that for
 * somebody tuning, and stops.
 */
static bool publish(const RadioRound *r) {
  if (xSemaphoreTake(sLock, pdMS_TO_TICKS(RADIO_PUBLISH_WAIT_MS)) != pdTRUE) {
    return false;
  }
  for (uint32_t i = 0; i < r->drained; i++) {
    uint32_t ticket = sSnapshot.applied + 1 + i;
    RadioOutcome *slot = &sSnapshot.outcomes[ticket % RADIO_OUTCOMES];
    slot->ticket = ticket;
    slot->result = r->results[i];
  }
  sSnapshot.applied += r->drained;
  sSnapshot.settings = r->settings;
  if (r->qualityOk) {
    sSnapshot.quality = r->quality;
  }
  sSnapshot.qualityValid = r->qualityOk;
  sSnapshot.qualityReads = sQualityReads;
  sSnapshot.levelSmoothedTenths = sLevelSmoothed;
  sSnapshot.levelSmoothedValid = sLevelSmoothedValid;

  /*
   * What the AGC is doing, published every round.
   *
   * Without this a working AGC and one that has decided to do nothing are the
   * same thing from outside. The average is carried in tenths of a per cent,
   * from the thousandths the AGC keeps internally.
   */
  /* The same test the AGC itself makes, rather than "not zero": a target
   * outside the range it accepts would otherwise report as on with a gain
   * that never moves off zero. */
  sSnapshot.agcOn = sLive.agc.targetPercent >= AGC_TARGET_MIN &&
                    sLive.agc.targetPercent <= AGC_TARGET_MAX;
  sSnapshot.agcGainDb = sAgcApplied;
  sSnapshot.agcAverageTenths = agcAverageTenths(&sAgc);
  sSnapshot.agcSettled = agcSettled(&sAgc);
  sSnapshot.rds = sRds.info;
  if (r->processingOk) {
    sSnapshot.processing = r->processing;
  }
  sSnapshot.processingValid = r->processingOk;
  sSnapshot.tunerReady = tef668xCapabilities() != NULL;
  sSnapshot.lastError = r->lastError;
  sSnapshot.bandwidthWide = sBandwidthWide;
  sSnapshot.tunerMuted = sLastPushedMute;
  sSnapshot.squelchMode = sLive.squelchMode;
  sSnapshot.seeking = sSeek.walking;
  sSnapshot.seekFromKHz = sSeek.walking ? sSeek.fromKHz : 0;
  sSnapshot.beeping = sBeeping;
  sSnapshot.seekFound = sSeek.found;
  sSnapshot.squelchOpen = sSquelch.open;
  sSnapshot.squelchThresholdTenths = sLive.squelchThresholdTenths;
  sSnapshot.memorySlot = (int16_t)sMemorySlot;
  sSnapshot.sequence++;
  if (r->probeOwed != 0) {
    probeAnswered(r->probeOwed);
  }
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
/* Keeps the first error: `next` counts only when nothing failed before it.
 * Every write still goes out, whatever happened to the ones before. */
static void keepFirst(Tef668xError *err, Tef668xError next) {
  if (*err == TEF668X_OK) {
    *err = next;
  }
}

static Tef668xError pushFeatures(const RadioSettings *s) {
  /* The blankers come first, because they apply on both sides. */
  Tef668xError blanker = tef668xSetAmNoiseBlanker(s->amNoiseBlankerStart);
  keepFirst(&blanker, tef668xSetFmNoiseBlanker(s->fmNoiseBlankerStart));

  if (bandModulation(s->band) != MODULATION_FM) {
    /* On AM the weak signal values depend on which AM band this is, which
     * is why a retune sends them. All go out whatever happens to the first,
     * for the same reason as the FM writes below. */
    AmWeakSignal weak = radioAmWeakSignal(s);
    keepFirst(&blanker,
              tef668xSetAmWeakSignal(weak.highCutStart, weak.softMuteStart,
                                     weak.softMuteSlope));
    keepFirst(&blanker, tef668xSetAmChannel());
    return blanker;
  }
  /* All of them go out whatever happens to the first, and the first error is
   * what gets reported. Stopping at a failure would leave the others holding
   * whatever the chip had, with the firmware believing it had set them,
   * which is the harder fault to find. The same reasoning as the unmute in
   * pushToTuner below, and worth saying rather than leaving it to look
   * accidental. */
  Tef668xError err = tef668xSetMultipathSuppression(s->multipathSuppression);
  keepFirst(&err, tef668xSetChannelEqualizer(s->equalizer));
  keepFirst(&err, tef668xSetMono(s->forcedMono));
  keepFirst(&err, tef668xSetWeakSignal(s->highCutStart, s->stereoBlendStart,
                                       s->stHiBlendStart));
  keepFirst(&err, tef668xSetDeemphasis(s->deemphasisUs));
  keepFirst(&err, blanker);
  return err;
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
    keepFirst(&err, tuned);
    /* Sent with every FM tune, not once at start up. It restarts the chip's
     * decoder, and without that the first read after the dial moves hands
     * over the group the previous station left in the register. */
    if (tuned == TEF668X_OK && fm) {
      keepFirst(&err, tef668xSetRds(false));
    }
    if (tuned == TEF668X_OK && push.bandwidth) {
      keepFirst(&err, fm ? tef668xSetFmBandwidth(to->bandwidthKHz)
                         : tef668xSetAmBandwidth(to->bandwidthKHz));
    }
    if (push.volume) {
      keepFirst(&err, tef668xSetVolume(to->volumeDb));
    }

    /* The mute always comes off, even when something above failed.
     *
     * Returning early after the mute leaves the radio silent with no way
     * back: the next attempt mutes again, fails at the same place, and never
     * reaches the unmute. A radio that is wrong is recoverable. A radio that
     * is silent looks broken. So the unmute happens on every path, and the
     * first real error is what gets reported. */
    keepFirst(&err, tef668xSetMute(to->muted));
    if (push.features) {
      keepFirst(&err, pushFeatures(to));
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
      keepFirst(&err, tef668xSetMute(false));
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
 * The level sweep: the band `at` is on, in its default step so every sweep of
 * it reads the same channels, all in one go and muted. The tuner is left on the
 * last channel, which is returned for the retune back to the dial.
 */
static uint32_t sweepBand(DxSweep *out, const RadioSettings *at,
                          uint16_t widthKHz) {
  /* A sweep asked for over the web starts while that request's reply is
   * still going out, and the web server serves no other while one runs,
   * so this is the one reply to wait out. */
  for (uint16_t waited = 0; sNetServing.load() && waited < SWEEP_NET_WAIT_MS;
       waited++) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  DxSweepRange range;
  out->count = 0;
  out->widthKHz = widthKHz;
  if (sSweepPlanned) {
    range = sSweepPlan.range;
  } else if (!dxSweepRange(at->band, &sPlan, at->freqKHz, 0, &range)) {
    return at->freqKHz;
  }
  const uint32_t lo = range.lowKHz;
  const uint16_t step = range.stepKHz;
  out->lowKHz = lo;
  out->stepKHz = step;
  out->count = range.count;
  const uint32_t startMs = millis();
  (void)tef668xSetMute(true);
  /* The retune back to the dial after the sweep sends the radio's own width
   * again, so a width for the sweep alone needs no undoing. A plan with
   * none, and AM always, reads through the radio's own. */
  const bool fm = bandModulation(at->band) == MODULATION_FM;
  if (sSweepPlanned && fm && sSweepPlan.widthKHz != 0 &&
      tef668xSetFmBandwidth(sSweepPlan.widthKHz) == TEF668X_OK) {
    out->widthKHz = sSweepPlan.widthKHz;
  }
  uint32_t khz = at->freqKHz;
  for (uint16_t i = 0; i < out->count; i++) {
    /* A command waiting, a key or a tune, ends it, so the radio never
     * leaves a key unanswered for the length of a band, and so does the
     * caller calling it off. So does a shutdown asking the task to park:
     * it waits half a second for that, and a sweep is about four. What was
     * swept is thrown away: part of a band is not a sweep of it. */
    if (uxQueueMessagesWaiting(sQueue) > 0 || sSweepCancel.load() ||
        sStopWanted) {
      out->count = 0;
      break;
    }
    /* Fed a channel at a time: an AM span of 431 channels takes about 23 s,
     * near the watchdog's limit for a whole round. */
    esp_task_wdt_reset();
    khz = lo + (uint32_t)i * step;
    int16_t reads[DX_SWEEP_READS] = {};
    uint8_t n = 0;
    const Tef668xError tuned = fm ? tef668xTuneFm(khz) : tef668xTuneAm(khz);
    if (tuned == TEF668X_OK) {
      vTaskDelay(
          pdMS_TO_TICKS(fm ? DX_SWEEP_SETTLE_MS : DX_SWEEP_SETTLE_AM_MS));
      for (uint8_t r = 0; r < DX_SWEEP_READS; r++) {
        int16_t level = 0;
        if (tef668xReadLevel(fm, &level) == TEF668X_OK) {
          reads[n++] = level;
        }
      }
    }
    out->level[i] = dxSweepMean(reads, n);
  }
  const uint32_t took = millis() - startMs;
  out->tookMs = (uint16_t)(took < UINT16_MAX ? took : UINT16_MAX);
  return khz;
}

static void afEnd(RadioAfSeries *af, bool stopped) {
  af->stopped = stopped;
  af->ended = true;
  sAfStarted = false;
  sAfOut.store(nullptr);
}

/*
 * The next check of a series, when it is due. A tune, a seek, a hush or a
 * band change ends the series, since a check is only worth anything against
 * the station it was started on.
 */
static void afStep(RadioAfSeries *af, const RadioSettings *at) {
  if (sAfCancel.load() || sSeek.walking || sHushed ||
      bandModulation(at->band) != MODULATION_FM ||
      (sAfStarted && at->freqKHz != sAfDialKHz)) {
    afEnd(af, true);
    return;
  }
  const uint32_t now = millis();
  if (!sAfStarted) {
    if (tef668xSetAfUpdateWidth(af->widthKHz) != TEF668X_OK) {
      afEnd(af, true);
      return;
    }
    sAfStarted = true;
    sAfDialKHz = at->freqKHz;
    sAfNextMs = now;
  }
  if (!radioRoundDue(now, sAfNextMs)) {
    return;
  }
  RadioAfCheck *c = &af->check[af->done];
  Tef668xQuality q;
  const uint32_t startUs = micros();
  c->err = (uint8_t)tef668xAfUpdate(af->khz, &q);
  const uint32_t tookUs = micros() - startUs;
  c->us = (uint16_t)(tookUs < UINT16_MAX ? tookUs : UINT16_MAX);
  c->atMs = now;
  c->level = q.levelDbuVTenths;
  c->usn = q.usnTenths;
  c->wam = q.multipathTenths;
  c->offset = q.offsetKHzTenths;
  c->status = q.status;
  af->done++;
  sAfDone.store(af->done);
  /* From now if a round ran late, so a late round is one check late rather
   * than a burst of them. */
  sAfNextMs = radioRoundNext(sAfNextMs, af->everyMs, now);
  if (af->done >= af->count) {
    afEnd(af, false);
  }
}

/* Start a seek from `from`, or turn round the one running. */
static void seekBegin(const RadioSettings *from, bool up) {
  seekWalkBegin(&sSeek, from->freqKHz, up, radioChannelsRound(from, &sPlan));
}

/*
 * The volume to aim at, with the AGC's gain folded in and clamped.
 *
 * One function, used by the fade and by the ramp that comes out of a duck, so
 * the two cannot disagree about where the volume is going. If they did,
 * cancelling a duck would step the audio by exactly the gain, which is the
 * click the ramp exists to remove.
 */
static int8_t agcTarget(const RadioRound *r, int8_t wantedDb) {
  if (r->live.agc.targetPercent == 0) {
    sAgcApplied = 0;
    return wantedDb;
  }
  const int16_t withGain = std::clamp<int16_t>(
      (int16_t)(wantedDb + agcGain(&sAgc)), RADIO_VOLUME_MIN, RADIO_VOLUME_MAX);
  sAgcApplied = (int8_t)(withGain - wantedDb);
  return (int8_t)withGain;
}

/*
 * Whether the RDS decoder is read on this round.
 *
 * The same conditions decide whether the round comes back early for RDS and
 * whether the read happens, and they are written once here so that the two
 * cannot come apart. If the wait were shortened while the read was skipped,
 * `nextRds` would never move and the wait would stay at zero, spinning on
 * core 0 with nothing yielding to the idle task.
 *
 * Not while seeking: the dial is passing channels nobody is listening to, and
 * a group picked up from one of them would be decoded as though it belonged
 * to wherever the seek stops. Not while hushed either, because then the tuner
 * belongs to whoever is taking the radio down.
 */
static bool rdsRunning(const RadioRound *r) {
  return sRdsEnabled && bandModulation(r->settings.band) == MODULATION_FM &&
         !sSeek.walking && !sHushed;
}

/* The decoder switched off since the last round. Taken and cleared in one
 * step, so a switch-off that lands between a read and a clear is not lost. */
static void roundRdsForget(const RadioRound *r) {
  if (sRdsForget.exchange(false)) {
    /* Everything held describes a station this radio is no longer listening
     * to for RDS, and leaving it would be a name that nothing is keeping true
     * any more. */
    rdsReset(&sRds, r->settings.freqKHz);
    sRdsRawStale = true;
  }
}

/*
 * How long this round may sleep on the queue, and the bookkeeping that goes
 * with deciding it.
 *
 * Sleep on the queue rather than on the clock, so a command is picked up in
 * about a millisecond instead of waiting out the rest of the poll interval.
 * That latency is what a person feels when they turn the knob, and it is the
 * whole of the wait an HTTP write sits through.
 */
static TickType_t roundWait(RadioRound *r) {
  /* Finished fades are put away rather than left to age. Left alone,
   * millis() minus the start climbs for forty nine days and then wraps back
   * through zero, and the radio would fade for no reason. */
  if (sFadeMs != 0 && (uint32_t)(millis() - sFadeFromMs) >= sFadeMs) {
    sFadeMs = 0;
  }
  /* RDS keeps its own cadence, faster than the poll interval, so the round
   * has to come back in time for it. */
  const bool rds = rdsRunning(r);
  const TickType_t now = xTaskGetTickCount();
  if (!rds) {
    /* Kept alongside the clock while it is not running, so a long spell on
     * medium wave or in a hush does not leave a deadline far enough in the
     * past for the comparison to wrap. */
    r->nextRds = now + pdMS_TO_TICKS(RADIO_RDS_INTERVAL_MS);
  }
  RadioRoundTimes t;
  t.now = now;
  t.nextPoll = r->nextPoll;
  t.seeking = sSeek.walking;
  t.stepping = sFadeMs != 0 || sDucking || sBeeping || sWakeSoon || sSleepBack;
  t.step = pdMS_TO_TICKS(RADIO_FADE_STEP_MS);
  /* Back in time for the next AF_Update check. */
  t.afWaiting = sAfOut.load() != nullptr;
  const int32_t afDueIn = sAfStarted ? (int32_t)(sAfNextMs - millis()) : 0;
  t.afIn = afDueIn > 0 ? pdMS_TO_TICKS(afDueIn) : 0;
  t.rds = rds;
  t.nextRds = r->nextRds;
  sWakeSoon = false;
  return radioRoundWait(&t);
}

/* In memory mode the knob walks the stored list, not the dial. The step is
 * turned into a tune here rather than in the caller, because only the radio
 * knows which slot it is on and a caller that read that, worked out the next
 * one and sent it would be racing the radio for the answer. */
static RadioError takeMemoryStep(RadioRound *r, const QueueItem *item) {
  /* A click that ends a seek steps from the ends of the list. The dial is
   * then on a channel nobody chose, so the list starts again from its
   * ends. */
  const bool endsSeek = sSeek.walking;
  seekWalkEnd(&sSeek, false);
  /* Otherwise from where this round has got to. `sMemorySlot` is only
   * brought up to date at the end of the round, and a fast spin queues
   * several steps while the radio retunes, so starting each of them from it
   * would start them all from the same slot and keep only the last. The slot
   * the last step chose is where the dial is now. */
  int slot =
      endsSeek ? MEMORY_NO_SLOT
               : memoryStoreSlotFor(sChosenSlot, sMemorySlot,
                                    (uint8_t)r->wanted.band, r->wanted.freqKHz);
  int32_t moves = item->steps < 0 ? -(int32_t)item->steps : item->steps;
  /* A walk longer than the list repeats itself, so there is nothing to gain
   * past one lap. Without this a fast spin of the knob sends thousands of
   * steps, each one taking the list's lock and reading all 99 slots, and the
   * radio task holds up its own cadence to do work whose answer it already
   * had. */
  if (moves > MEMORY_SLOT_COUNT) {
    moves = MEMORY_SLOT_COUNT;
  }
  slot = memoryStoreStep(&sPlan, slot, item->steps > 0, (int)moves);
  const RadioError result = recallChannel(&r->wanted, slot);
  if (result == RADIO_OK) {
    r->jumped = true;
    sChosenSlot = slot;
  }
  return result;
}

/* In Auto mode the knob seeks rather than steps the dial. Starting one belongs
 * here rather than in radioApply, the same reason memory mode does above: it
 * needs task state a settings struct and a band plan cannot supply on their
 * own.
 *
 * A turn the same way an already running seek is going stops it where it
 * stands; a turn the other way reverses it. Both act only once a round,
 * guarded by autoSeekTurned, or the later steps of one continuous spin would
 * each see the change the first one just made and undo it. */
static RadioError takeAutoStep(RadioRound *r, const QueueItem *item) {
  if (r->autoSeekTurned) {
    /* The same continuous spin, arriving as a second queued item this round.
     * Reported busy rather than done, so a caller asking what its own step
     * achieved is not told it moved something when a different queued item
     * already did. */
    return RADIO_ERR_BUSY;
  }
  r->autoSeekTurned = true;
  const bool up = item->steps > 0;
  if (sSeek.walking && up == sSeek.up) {
    seekWalkEnd(&sSeek, false);
  } else {
    seekBegin(&r->wanted, up);
  }
  return RADIO_OK;
}

/* Anything else stops a seek where it stands. A person reaching for the knob
 * while the radio is hunting means stop, and so does a script sending a tune.
 *
 * The find goes with it. It says where the radio is now, so once somebody has
 * tuned somewhere by hand it is no longer true. */
static RadioError takeCommand(RadioRound *r, const QueueItem *item) {
  seekWalkEnd(&sSeek, false);
  if (item->kind == RADIO_TUNE || item->kind == RADIO_TUNE_IN_BAND ||
      item->kind == RADIO_SET_BAND || item->kind == RADIO_CYCLE_BAND) {
    r->jumped = true;
  }
  const uint32_t was = r->wanted.freqKHz;
  const RadioError result = radioApply(&r->wanted, &sPlan, item);
  /* A step that comes back on the wrong side of where it started has wrapped
   * at a band edge. The encoder cannot tell, because it sends a number of
   * steps and never learns where they landed. */
  if (sBeepEdge && item->kind == RADIO_STEP && item->steps != 0 &&
      r->wanted.freqKHz != was) {
    const bool wrapped =
        item->steps > 0 ? r->wanted.freqKHz < was : r->wanted.freqKHz > was;
    if (wrapped) {
      sBeepUntilMs = millis() + RADIO_BEEP_MS;
      sBeeping = true;
    }
  }
  return result;
}

/* One command off the queue. Its result is recorded whether or not the state
 * machine took it. The caller waiting on this one wants to know the radio has
 * dealt with it, and a refusal is dealing with it. */
static RadioError take(RadioRound *r, const QueueItem *item) {
  if (item->kind == RADIO_BEEP) {
    /* Started here and stopped below once it has run long enough, so a tone
     * never stalls the queue or a seek walking the band. */
    if (item->beepMs != 0) {
      sBeepUntilMs = millis() + item->beepMs;
      sBeepHz = item->beepHz;
      sBeepHz2 = item->beepHz2;
      sBeeping = true;
    }
    return RADIO_OK;
  }
  if (item->kind == RADIO_SEEK) {
    seekBegin(&r->wanted, item->up);
    return RADIO_OK;
  }
  if (item->kind == RADIO_RECALL) {
    seekWalkEnd(&sSeek, false);
    const RadioError result = recallChannel(&r->wanted, item->memorySlot);
    if (result == RADIO_OK) {
      r->jumped = true;
      sChosenSlot = item->memorySlot;
    }
    return result;
  }
  const TuneMode knob = radioKnobMode(&r->wanted);
  if (item->kind == RADIO_STEP && item->steps != 0 &&
      knob == TUNE_MODE_MEMORY) {
    return takeMemoryStep(r, item);
  }
  if (item->kind == RADIO_STEP && item->steps != 0 && knob == TUNE_MODE_AUTO) {
    return takeAutoStep(r, item);
  }
  return takeCommand(r, item);
}

/* Wait up to `wait` for the commands, and take them into `wanted`. */
static void roundDrain(RadioRound *r, TickType_t wait) {
  if (r->drained >= RADIO_QUEUE_DEPTH) {
    /* Every slot is already owed to a command that could not be published.
     * Hold the cadence rather than spinning, and take no more until these
     * have gone out. */
    if (wait > 0) {
      vTaskDelay(wait);
    }
    return;
  }
  /* The first read waits, the rest take whatever is already there, so a
   * burst of commands costs one retune rather than one each. `drained` can
   * never pass the end of `results`, which is what publish reads. */
  QueueItem item;
  bool waited = false;
  while (r->drained < RADIO_QUEUE_DEPTH &&
         xQueueReceive(sQueue, &item, waited ? 0 : wait) == pdTRUE) {
    waited = true;
    r->results[r->drained++] = take(r, &item);
  }
}

/* What the other tasks set, copied out in one go for this round.
 *
 * Taken after the push, the probe and the sweep, where the seek and the poll
 * read it, so a busy lock never holds up a retune a person hears and never
 * stands between the drain and the probe's own try for the lock. The volume
 * before it works from the copy a round old.
 *
 * If the lock cannot be had in 50 ms, the seek judges nothing this round and
 * the squelch and the AGC are not updated, and the last copy stands for the
 * volume. The alternative was reading them unlocked, and a torn SquelchConfig
 * would have the seek judging against a floor nobody set, or a new AGC target
 * paired with the old boost, which is a wrong answer with nothing to show for
 * it. A rule that did not arrive is not a rule. The cost is walking past one
 * channel, or one update fewer in a tenth of a second, which changes nothing
 * that can be heard. */
static void roundCopyLive(RadioRound *r) {
  r->haveLive = xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) == pdTRUE;
  if (r->haveLive) {
    r->live = sLive;
    xSemaphoreGive(sLock);
  }
}

/*
 * A settle probe moves the dial the same way a seek does, so the retune it
 * measures is the ordinary one and not a path of its own.
 *
 * Never in a round that took a command off the queue. The dial the command
 * asked for would be overwritten here while the command had already been
 * recorded as applied, so a tune would answer success and never happen. The
 * probe waits for a quiet round instead.
 */
static void roundPickProbe(RadioRound *r) {
  r->probing = false;
  r->sweepOut = NULL;
  /* Not while an answer is still owed. That probe is still asked for and the
   * dial is already on its channel, so a caller walking the dial would be
   * told it moved, and any other would get the probe run again with no
   * retune: a settled reading under the settle time it asked for. */
  if (sSeek.walking || sHushed || r->drained != 0 || r->probeOwed != 0 ||
      xSemaphoreTake(sLock, 0) != pdTRUE) {
    return;
  }
  if (sProbeWanted && sProbeExpectKHz != 0 &&
      r->wanted.freqKHz != sProbeExpectKHz) {
    /* Tuned by somebody else since the caller last looked, maybe in the
     * round just before this one. Their dial stands, and the caller is
     * told. */
    sProbeMoved = true;
    sProbeOk = false;
    probeAnswered(sProbeSeq);
  } else if (sProbeWanted) {
    r->probing = true;
    r->probeKHz = sProbeKHz;
    r->probeMs = sProbeMs;
    r->probeReads = sProbeReads;
    r->probeGapMs = sProbeGapMs;
    r->probeSeq = sProbeSeq;
  }
  xSemaphoreGive(sLock);
  /* A probe first: its caller is waiting on it, a sweep's is not. */
  if (r->probing) {
    r->wanted.freqKHz = r->probeKHz;
  } else {
    r->sweepOut = sSweepOut.load();
  }
}

/*
 * What the tuner is actually told to do about the audio: what the person
 * asked for, or silence because the squelch is shut. The two are kept apart
 * everywhere else, so turning the squelch off can never leave a radio the
 * person deliberately muted playing, and the squelch can never unmute
 * something they muted on purpose.
 */
static void roundHear(RadioRound *r) {
  r->heard = r->wanted;
  /* The width the tuner gets, which is DX mode's while that is on. From
   * here on `heard` carries it in `bandwidthKHz`, so everything below that
   * compares, holds back or pushes a width works on the one the tuner has,
   * and `wanted` keeps the person's own for saving. */
  r->heard.bandwidthKHz = radioTunerBandwidth(&r->wanted);
  r->heard.dxBandwidthKHz = 0;
  /* Changing the filter clicks, so it goes down the same ramp as a mute:
   * quiet first, then the filter, then back up.
   *
   * Only while the dial is still, and only when there is a ramp to use. A
   * retune is already silent while the frequency moves and carries the
   * filter with it, and with the ramp off the mute inside pushToTuner is
   * as close to instant as this gets. */
  r->filterMoving = r->softMuteMs != 0 && !sSeek.walking &&
                    r->heard.bandwidthKHz != sLastPushedBandwidth &&
                    r->wanted.band == r->settings.band &&
                    r->wanted.freqKHz == r->settings.freqKHz;
  /* Muted while the dial is moving, or a seek is a second of every station
   * and every patch of noise between them. The mute comes off when it
   * stops, including when it stops empty handed. */
  const bool hushWanted = r->wanted.muted || !sSquelch.open || sSeek.walking ||
                          r->scanning || r->filterMoving;

  /* Going quiet is a ramp, not a step. The mute itself is held back until
   * the ramp has run, because a mute sent at the start would cut the audio
   * before the ramp had anything to walk down.
   *
   * A seek is exempt: it mutes and unmutes once per channel, and a ramp on
   * each would be most of the settle time. A band scan is exempt too: its
   * probes come faster than the ramp, so the first channels would be heard
   * on the way down. */
  const uint16_t softMuteMs = r->softMuteMs;
  if (softMuteMs != 0 && !sSeek.walking && !r->scanning) {
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
          int8_t nowDb =
              radioDuckVolume(sDuckFromDb, millis() - sDuckFromMs, softMuteMs);
          already = radioFadeElapsedAt(agcTarget(r, r->wanted.volumeDb), nowDb,
                                       softMuteMs);
        }
        sFadeFromMs = millis() - already;
        sFadeMs = softMuteMs;
      }
      sDucking = false;
    }
  } else {
    sDucking = false;
  }
  const bool duckDone =
      !sDucking || (uint32_t)(millis() - sDuckFromMs) >= softMuteMs;
  if (sDucking && duckDone) {
    sDucking = false;
  }
  r->heard.muted = hushWanted && (!sDucking || duckDone);
  /* The filter waits for silence. Sending it while the audio is still up
   * is the click the ramp is there to remove. */
  if (r->filterMoving && !r->heard.muted) {
    r->heard.bandwidthKHz = sLastPushedBandwidth;
  }
  r->wasHeard = r->settings;
  r->wasHeard.muted = sLastPushedMute;
  r->wasHeard.volumeDb = sLastPushedVolume;
  r->wasHeard.bandwidthKHz = sLastPushedBandwidth;
  r->push = radioPushNeeded(&r->wasHeard, &r->heard);
}

/* The volume the tuner is given: the fade, Auto off's fade, the ramp down and
 * the mute, each over what the person asked for. */
static void roundVolume(RadioRound *r) {
  /* The fade starts before the push that needs it, not after.
   *
   * Started afterwards, the retune would go out carrying the full volume,
   * so the radio would unmute loud, drop twenty five dB on the next pass,
   * and then ramp back. That is the opposite of the point, and it is subtle
   * enough to survive a listening test: it still ends in a ramp.
   *
   * Judged on push.retune as well as on which command arrived, because a
   * tune the state machine refused, or a band command naming the band the
   * radio is already on, moves nothing and should fade nothing.
   *
   * A step is not a jump. Turning the knob one click has to be instant, or
   * the dial feels slow, and a fade that restarts on every click never
   * finishes. So the fade is for a band change or a typed frequency. */
  if (r->push.retune && r->jumped && !r->firstPass) {
    sFadeFromMs = millis();
    sFadeMs = RADIO_BAND_FADE_MS;
  }

  /* The volume the tuner is actually given, which while a fade runs is on
   * its way up to the target. The target is read every time round, so the
   * knob still works during one, and it already carries the AGC's gain: see
   * `agcTarget`, and the ramp above works against the same number. */
  const int8_t target = agcTarget(r, r->wanted.volumeDb);
  r->heard.volumeDb = radioFadeVolume(target, millis() - sFadeFromMs, sFadeMs);
  /* Auto off's fade: from wherever the volume is down to the bottom, read
   * again each round so the knob and the AGC still act on it. Called off,
   * the sound comes back up over the unmute's ramp from where the fade had
   * got to, in a straight line, not in one step from near silence. */
  const bool sleepWanted = sSleepFadeWanted.load();
  const uint16_t sleepOverMs = (uint16_t)sSleepFadeMs.load();
  if (sleepWanted && !sSleepFading) {
    sSleepFading = true;
    sSleepBack = false;
    sSleepFromMs = millis();
  } else if (!sleepWanted && sSleepFading) {
    sSleepFading = false;
    sSleepBack = true;
    sSleepBackFromMs = millis();
    sSleepBackFromDb =
        radioDuckVolume(target, millis() - sSleepFromMs, sleepOverMs);
  }
  int8_t sleepDb = r->heard.volumeDb;
  if (sSleepFading) {
    sleepDb = radioDuckVolume(target, millis() - sSleepFromMs, sleepOverMs);
  } else if (sSleepBack) {
    const uint32_t back = millis() - sSleepBackFromMs;
    sleepDb = radioRampVolume(sSleepBackFromDb, target, back, r->softMuteMs);
    sSleepBack = back < r->softMuteMs;
  }
  if (sleepDb < r->heard.volumeDb) {
    r->heard.volumeDb = sleepDb;
  }
  if (sDucking) {
    /* On the way out, so the ramp down wins over any fade up. */
    r->heard.volumeDb =
        radioDuckVolume(sDuckFromDb, millis() - sDuckFromMs, r->softMuteMs);
    /* And the gain is not reaching the chip while that runs. Leaving the
     * published number where it was would report a gain the audio is not
     * getting, on every mute and every seek, which is the one thing that
     * field exists to prevent. */
    sAgcApplied = 0;
  } else if (r->heard.muted) {
    /* Held at the bottom for as long as the mute lasts.
     *
     * Without this the volume returns to the target on the round the mute
     * is applied, so the gain the tuner is carrying while it is silent is
     * the full listening level, and the unmute at the end of it is a step
     * from nothing straight to loud. That is a louder click than the one
     * the ramp exists to remove. */
    r->heard.volumeDb = RADIO_VOLUME_MIN;
    sAgcApplied = 0;
  }
  if (r->heard.volumeDb != r->wasHeard.volumeDb) {
    r->push.volume = true;
  }
}

/* Start every reading again for a dial that has just moved, since the ones
 * from before say nothing about where it is now. */
static void forgetReadings(void) {
  signalAverageReset(&sLevelAverage);
  signalAverageReset(&sSnrAverage);
  signalAverageReset(&sDisplayLevelAverage);
  sLevelSmoothedValid = false;
  sBandwidthKnown = false;
}

/* What moved goes to the tuner. No `changed` guard here. The squelch can move
 * the mute with no command having arrived at all, and a push that only
 * happens when something was drained would never act on it. Comparing the two
 * is the whole test. */
static void roundPush(RadioRound *r) {
  if (sHushed) {
    /* On the way to a reboot. radioHush has taken the audio down and is
     * the only thing allowed to talk to the tuner now. */
    r->push.retune = false;
    r->push.bandwidth = false;
    r->push.volume = false;
    r->push.mute = false;
    r->push.features = false;
    r->pushFailed = false;
  }
  const RadioPush push = r->push;
  if (push.retune || push.bandwidth || push.volume || push.mute ||
      push.features || r->pushFailed) {
    /* After a failure the tuner's state is not known, so everything goes
     * again rather than only what the settings say moved. */
    r->lastError = pushToTuner(r->pushFailed ? NULL : &r->wasHeard, &r->heard);
    r->pushFailed = r->lastError != TEF668X_OK;
    if (r->pushFailed) {
      /* The commands drained this round are in what did not reach the
       * chip, so a caller waiting on one is not told it was done. Those
       * carried over from a round whose publish could not take the lock
       * were pushed in that round, and keep their answer. */
      for (uint32_t i = r->owed; i < r->drained; i++) {
        if (r->results[i] == RADIO_OK) {
          r->results[i] = RADIO_ERR_TUNER;
        }
      }
    }
    sLastPushedMute = r->heard.muted;
    sLastPushedVolume = r->heard.volumeDb;
    sLastPushedBandwidth = r->heard.bandwidthKHz;
    if (r->filterMoving && push.bandwidth) {
      /* The filter has just gone out at the bottom of the ramp, so the
       * next round is the one that brings the audio back. */
      sWakeSoon = true;
    }
    if (push.retune) {
      /* The readings from before the dial moved say nothing about where it
       * is now, and the chip has been through its active mode, which may
       * have taken the bandwidth option with it. Both start again. */
      forgetReadings();
      /* The squelch keeps its own average of the level and it means
       * nothing here any more. Left running, landing on a station from the
       * shoulder of another one would hold the audio shut for about a
       * second while the average climbed, which is the front of the
       * station gone. Only the average is started again: whether the audio
       * is open carries across a retune. */
      squelchRetuned(&sSquelch);
      /* The average describes the station that has just gone. Carrying it
       * across would hold the new one at the old one's gain for the length
       * of the slow average, which is the pumping this is meant to avoid. */
      agcRetuned(&sAgc);
      /* Nothing the last station said is true of this one, and a name left
       * behind is a real name on the wrong station. The raw ring goes with
       * it: a capture holding the tail of the previous station and the
       * start of this one reads as one broadcast and is not. */
      rdsReset(&sRds, r->heard.freqKHz);
      sRdsRawStale = true;
    }
  }
  r->settings = r->wanted;
}

/* The tone, started when the command arrived and stopped here. Checked every
 * round rather than waited out, so it never holds the queue.
 *
 * Not while hushed. The shutdown owns the bus from then on, and a tone is the
 * one thing here that would be heard. */
static void roundTone(RadioRound *r) {
  if (!sBeeping || sHushed) {
    return;
  }
  if (!sToneOn) {
    sToneOn = tef668xTone(true, RADIO_BEEP_AMPLITUDE, sBeepHz, sBeepHz2) ==
              TEF668X_OK;
  }
  if ((int32_t)(millis() - sBeepUntilMs) < 0) {
    return;
  }
  /* Tried again next round if it fails. Turning the tone off is also what
   * puts the audio path back on the tuner, so giving up on it would leave
   * the radio on, tuned, unmuted, reporting a good signal and silent until
   * the next reboot. */
  if (tef668xTone(false, 0, 0, 0) == TEF668X_OK) {
    sToneOn = false;
    sBeeping = false;
    sWakeSoon = false;
    /* A fault left by a failed try is the tone's own and goes with it.
     * Every other fault comes with pushFailed set, and stays until the push
     * that clears it works. */
    if (!r->pushFailed) {
      r->lastError = TEF668X_OK;
    }
  } else {
    r->lastError = TEF668X_ERR_WRITE;
  }
}

/* The probe waits and reads exactly where a seek would, so what it reports is
 * what a seek at that settle time would have decided on. The readings are
 * stored here and the answer goes out with this round's publish. Nobody reads
 * them before that, since a caller only takes readings marked done with its
 * own number. */
static void roundProbe(RadioRound *r) {
  if (!r->probing) {
    return;
  }
  const bool fm = bandModulation(r->settings.band) == MODULATION_FM;
  Tef668xQuality look[RADIO_PROBE_MAX_READS];
  memset(look, 0, sizeof(look));
  /* A retune that did not go out leaves the tuner where it was, so the
   * readings would describe the parked channel while the answer named the
   * one that was asked for. */
  bool ok = !r->pushFailed;
  if (ok) {
    for (uint8_t i = 0; i < r->probeReads; i++) {
      vTaskDelay(pdMS_TO_TICKS(i == 0 ? r->probeMs : r->probeGapMs));
      if (tef668xReadQuality(fm, &look[i]) != TEF668X_OK) {
        ok = false;
      }
    }
  }
  if (xSemaphoreTake(sLock, pdMS_TO_TICKS(RADIO_PUBLISH_WAIT_MS)) == pdTRUE) {
    memcpy(sProbeQuality, look, sizeof(sProbeQuality));
    sProbeOk = ok;
    xSemaphoreGive(sLock);
    r->probeOwed = r->probeSeq;
  }
}

/* The level sweep, then back to the dial the way any retune goes: muted
 * across it, with the chip's RDS decoder restarted, and the audio put back as
 * it was. The poll below then reads the dial again. A sweep that ended on the
 * dial's own channel still retunes, from no channel, so the decoder does not
 * hand over a group of the channel before. */
static void roundSweep(RadioRound *r) {
  if (r->sweepOut == NULL) {
    return;
  }
  if (r->settings.band != sSweepBand) {
    /* A command since has left the band it was asked for on, whose channels
     * its plan holds, so it ends with nothing kept, as one ended by a key
     * does. */
    r->sweepOut->count = 0;
    r->sweepOut->tookMs = 0;
    sSweepOut.store(nullptr);
    return;
  }
  RadioSettings from = r->heard;
  from.freqKHz = sweepBand(r->sweepOut, &r->settings, sLastPushedBandwidth);
  if (from.freqKHz == r->heard.freqKHz) {
    from.freqKHz = 0;
  }
  from.muted = true;
  if (sHushed) {
    /* radioHush owns the tuner now, and putting the dial back would bring
     * the audio up just before it goes down. The tuner is left on a swept
     * channel, so the first push after a resume is a full one. */
    r->pushFailed = true;
  } else {
    r->lastError = pushToTuner(&from, &r->heard);
    r->pushFailed = r->lastError != TEF668X_OK;
    /* The chip has been through a retune for every channel, which may have
     * taken the bandwidth option with it, as any retune may. */
    sBandwidthKnown = false;
  }
  sSweepOut.store(nullptr);
}

/* The seek decision, taken on a reading of its own rather than on the one the
 * poll below takes. The poll runs on its own cadence and would often be
 * looking at the channel before this one, so the radio would stop one channel
 * past the station, or not at all. */
static void roundSeek(RadioRound *r) {
  if (!sSeek.walking) {
    return;
  }
  vTaskDelay(pdMS_TO_TICKS(SEEK_SETTLE_MS));
  const bool fm = bandModulation(r->settings.band) == MODULATION_FM;

  SeekConfig cfg = r->live.seek;
  /* Handed over whole rather than turned into a number here. The seek asks
   * the squelch the same question the squelch will ask itself, so the two
   * cannot disagree about the level, the multipath, how far off centre a
   * carrier may sit, or anything added later. */
  cfg.checkAudible = true;
  cfg.squelchMode = r->live.squelchMode;
  cfg.squelchCfg = r->live.squelch;
  cfg.squelchThresholdTenths = r->live.squelchThresholdTenths;

  Tef668xQuality look;
  memset(&look, 0, sizeof(look));
  SeekReading found;
  memset(&found, 0, sizeof(found));
  /* Not read at all when there is no rule to judge it by, so a channel is
   * never stopped on by a decision made against nothing. */
  found.valid = r->haveLive && tef668xReadQuality(fm, &look) == TEF668X_OK;
  found.levelTenths = look.levelDbuVTenths;
  found.noiseTenths = look.usnTenths;
  found.multipathTenths = look.multipathTenths;
  found.offsetTenths = look.offsetKHzTenths;

  const SeekWalkStep step =
      seekWalkJudge(&sSeek, &cfg, r->settings.band, &found);
  if (step == SEEK_WALK_FOUND) {
    /* The readings taken while walking say nothing about where it has
     * stopped, and the next thing to read them is the bandwidth extension.
     * The squelch starts again for the same reason, open, so that a station
     * it has just found is not held shut by a hold that began on a noise
     * channel. */
    forgetReadings();
    squelchInit(&sSquelch);
    /* Fade in, the same as a band change, so a station does not arrive at
     * full volume the instant the mute lifts. */
    sFadeFromMs = millis();
    sFadeMs = RADIO_BAND_FADE_MS;
  } else if (step == SEEK_WALK_EMPTY) {
    squelchInit(&sSquelch);
  }
}

/*
 * The ring and the status word are read from the web task, so they are written
 * under the same lock as the snapshot. The decoder is not: it belongs to this
 * task alone and is copied out at publish time, so nothing a person sees waits
 * on this.
 *
 * A group the lock was too busy for is counted rather than dropped quietly.
 * Losing one costs a capture one group, and a capture with a hole in it that
 * says so is usable where one that does not is not.
 */
static void rdsRingWrite(const Tef668xRdsRead *raw) {
  if (xSemaphoreTake(sLock, pdMS_TO_TICKS(RADIO_RDS_RING_WAIT_MS)) != pdTRUE) {
    if (raw->haveGroup) {
      sRdsRawDropped++;
    }
    return;
  }
  if (sRdsRawStale) {
    sRdsRawTotal = 0;
    sRdsRawDropped = 0;
    sRdsRawStale = false;
  }
  sRdsStatusWord = raw->status;
  sRdsStatusRead = raw->read;
  if (raw->haveGroup) {
    RadioRdsRaw *slot = &sRdsRaw[sRdsRawTotal % RADIO_RDS_RAW_DEPTH];
    for (int i = 0; i < 4; i++) {
      slot->block[i] = raw->block[i];
    }
    slot->error = (uint8_t)((raw->error[0] << 6) | (raw->error[1] << 4) |
                            (raw->error[2] << 2) | raw->error[3]);
    sRdsRawTotal++;
  }
  xSemaphoreGive(sLock);
}

/* The RDS decoder, on its own faster cadence. */
static void roundRds(RadioRound *r) {
  /* Asked again rather than taken from the wait. A command drained this
   * round can have started a seek or crossed to the AM side since the wait
   * was decided, and then reading RDS would take a group from a channel the
   * dial is only passing through. */
  if (!rdsRunning(r) || !radioRoundDue(xTaskGetTickCount(), r->nextRds)) {
    return;
  }
  Tef668xRdsRead raw;
  RdsRead read;
  memset(&read, 0, sizeof(read));
  memset(&raw, 0, sizeof(raw));
  if (tef668xReadRds(&raw) == TEF668X_OK) {
    read.synchronised = raw.synchronised;
    read.haveGroup = raw.haveGroup;
    for (int i = 0; i < 4; i++) {
      read.block[i] = raw.block[i];
      read.error[i] = raw.error[i];
    }
  }
  /* The last minute counts on the decoder page are timed by it. */
  read.atMs = millis();
  /* Fed whatever came back, including a read that failed, which counts as a
   * read with no lock and no group. Skipping it would hold the last lock
   * state for as long as the bus stayed broken. */
  rdsFeed(&sRds, &read);
  rdsRingWrite(&raw);
  r->nextRds = radioRoundNext(r->nextRds, pdMS_TO_TICKS(RADIO_RDS_INTERVAL_MS),
                              xTaskGetTickCount());
}

/* Only a reading that arrived, and only from the FM side, moves the
 * bandwidth extension. Smoothed, because one reading of this tuner jumps far
 * more than the signal does and the filter would open and shut several times
 * a second on a bare one. */
static void pollBandwidthExtension(const Tef668xQuality *q) {
  int16_t level = signalAverage(&sLevelAverage, q->levelDbuVTenths);
  int16_t snr = signalAverage(&sSnrAverage, q->snrDb);
  bool wantWide = signalWantsWideBandwidth(level, snr);
  if ((wantWide != sBandwidthWide || !sBandwidthKnown) && !sHushed) {
    if (tef668xSetBandwidthExtension(wantWide) == TEF668X_OK) {
      sBandwidthWide = wantWide;
      sBandwidthKnown = true;
    }
  }
}

/* Acted on now, not next time round.
 *
 * The push runs before the reading, so leaving it to that would hold the
 * decision back a whole poll interval. A tenth of a second of silence after
 * the dial lands on a station is exactly the clipped opening the squelch is
 * written to avoid, and the snapshot would meanwhile say open while the tuner
 * was still muted.
 *
 * Only the mute moves, so only the mute is sent. */
static void pollSquelchMute(RadioRound *r) {
  /* The same reasons the main push uses, seeking included. Without it a
   * seek sweeping past a strong station would open the squelch and blare
   * that channel until the next round re-muted it. */
  const bool wantMuted =
      r->settings.muted || !sSquelch.open || sSeek.walking || r->scanning;
  if (wantMuted == sLastPushedMute || sHushed) {
    return;
  }
  const Tef668xError muteErr = tef668xSetMute(wantMuted);
  if (muteErr == TEF668X_OK) {
    sLastPushedMute = wantMuted;
  } else {
    r->lastError = muteErr;
    /* Left for the push at the top of the next round to put right, which
     * re-sends everything after a failure. */
    r->pushFailed = true;
  }
}

/* The reading keeps its own cadence, whatever the commands are doing. */
static void roundPoll(RadioRound *r) {
  if (!radioRoundDue(xTaskGetTickCount(), r->nextPoll)) {
    return;
  }
  const bool fm = bandModulation(r->settings.band) == MODULATION_FM;
  r->qualityOk = tef668xReadQuality(fm, &r->quality) == TEF668X_OK;
  const Tef668xQuality *q = &r->quality;
  if (r->qualityOk) {
    /* Every band and every reading that arrived. A failed read leaves the
     * struct holding the last one, and feeding that in again would count
     * the same sample twice and make the meter creep towards a number
     * nothing measured. Counted here for the same reason, so a reader
     * that has to act once per reading can tell one from the next. */
    sQualityReads++;
    sLevelSmoothed = signalAverage(&sDisplayLevelAverage, q->levelDbuVTenths);
    sLevelSmoothedValid = true;
  }

  /* What the chip is actually doing with the audio, which is FM only and
   * is the only way to tell a blend that is working from one that was
   * never switched on. */
  r->processingOk = fm && tef668xReadProcessing(&r->processing) == TEF668X_OK;

  /* Only a reading that arrived, and only from the FM side.
   *
   * A failed read leaves the quality struct holding the previous one, so
   * feeding it in again would count the same sample twice. And the AM
   * readings are a different scale entirely: letting them into these
   * averages meant a strong FM station came back from a spell on medium
   * wave with the filter held narrow for about two seconds while the
   * smoothing forgot the AM numbers. */
  if (r->qualityOk && fm) {
    pollBandwidthExtension(q);
  }

  /* The squelch gets a say on every fresh reading, and only on a fresh one.
   * Running it again between readings would make its hold measure loop
   * iterations rather than time. Not while seeking or scanning. The readings
   * then come from whatever channel the sweep is passing, which nobody is
   * listening to, and they would drive the hold and the hysteresis on noise.
   * The squelch's average is started again by the retune that ends either
   * one. */
  const bool wasOpen = sSquelch.open;
  if (r->haveLive && !sSeek.walking && !r->scanning) {
    SquelchReading reading;
    reading.valid = r->qualityOk;
    reading.levelTenths = q->levelDbuVTenths;
    reading.noiseTenths = q->usnTenths;
    reading.multipathTenths = q->multipathTenths;
    reading.offsetTenths = q->offsetKHzTenths;
    squelchUpdate(&sSquelch, &r->live.squelch, r->live.squelchMode,
                  r->settings.band, &reading, r->live.squelchThresholdTenths,
                  millis());
  }

  /*
   * The volume AGC, on the same fresh reading.
   *
   * After the squelch, because `listening` below asks whether the audio is
   * actually getting through and the squelch has just decided that for
   * this reading. A gain worked out from a station nobody can hear would
   * be applied the moment the squelch opened, which is the click the
   * whole feature is supposed to remove.
   *
   * `fresh` is true because this only runs when a reading came back from
   * the chip. The AGC ticks on the poll cadence and the chip is not always
   * read at that rate, and core/agc.h explains what a repeat does to the
   * settling count.
   */
  if (r->haveLive) {
    AgcReading agcRead;
    agcRead.valid = r->qualityOk;
    agcRead.modulationPercent = q->modulationPercent;
    agcRead.levelTenths = q->levelDbuVTenths;
    agcRead.noiseTenths = q->usnTenths;
    agcRead.fm = fm;
    agcRead.listening = !r->settings.muted && !sSeek.walking && !r->scanning &&
                        !sHushed && sSquelch.open;
    agcRead.fresh = true;
    agcUpdate(&sAgc, &r->live.agc, &agcRead);
  }

  if (sSquelch.open != wasOpen) {
    pollSquelchMute(r);
  }

  /* A slow push can leave the next reading already in the past. Start
   * again from now rather than spinning to catch up. */
  r->nextPoll = radioRoundNext(
      r->nextPoll, pdMS_TO_TICKS(RADIO_POLL_INTERVAL_MS), xTaskGetTickCount());
}

/* Worked out again whenever the dial has moved, the list has changed or a slot
 * was chosen, and only then, so the lock is taken once per retune rather than
 * on every round. It says where the radio is rather than what memory mode
 * last did, so a station reached with the keypad shows its slot if it has
 * one. It is the last thing before the publish, and reads what the publish
 * sends. So the slot names the channel the snapshot carries, even when the
 * dial moved or a seek ended later in the round. */
static void roundMemorySlot(RadioRound *r) {
  if (sSeek.walking) {
    /* A seek moves the dial every round, and while it runs the radio is
     * walking noise rather than sitting on a channel. Looking the slot up
     * each time would take the list's lock on every channel, about twenty
     * times a second, to answer about a frequency nobody is listening to.
     * Clearing the remembered frequency makes the round it stops in look it
     * up again. */
    sMemorySlot = MEMORY_NO_SLOT;
    r->lastMemoryFreqKHz = 0;
    return;
  }
  const uint32_t generation = memoryStoreGeneration();
  if (r->settings.freqKHz == r->lastMemoryFreqKHz &&
      r->settings.band == r->lastMemoryBand &&
      generation == r->lastMemoryGeneration &&
      sChosenSlot == r->lastChosenSlot) {
    return;
  }
  r->lastMemoryFreqKHz = r->settings.freqKHz;
  r->lastMemoryBand = r->settings.band;
  r->lastMemoryGeneration = generation;
  r->lastChosenSlot = sChosenSlot;
  /* The chosen slot, then the one the radio was on, while either holds this
   * station, so a duplicate does not flip to the lower slot. */
  sMemorySlot = memoryStoreSlotFor(
      sChosenSlot, sMemorySlot, (uint8_t)r->settings.band, r->settings.freqKHz);
}

/* The radio's first push, before the loop: everything, since nothing has been
 * sent to the tuner yet. */
static void roundOpen(RadioRound *r) {
  memset(r, 0, sizeof(*r));
  /* Taken from the snapshot, which radioTaskStart has already filled in with
   * the defaults and the frequency the radio is to come up on. Calling
   * radioDefaults again here would throw that away, and the radio would
   * unmute on the bottom of the FM band and then retune. */
  sFadeFromMs = millis();
  sFadeMs = RADIO_FADE_MS;
  r->firstPass = true;
  r->settings = sSnapshot.settings;
  roundCopyLive(r);

  /* The AM side has no automatic bandwidth, so a band that starts there needs
   * one chosen before the first push. */
  if (bandModulation(r->settings.band) != MODULATION_FM &&
      r->settings.bandwidthKHz == 0) {
    r->settings.bandwidthKHz = 4;
  }

  /* Nothing has been sent to the tuner yet, so everything is, and the volume
   * starts at the bottom of the fade rather than at the target. */
  RadioSettings opening = r->settings;
  opening.volumeDb = radioFadeVolume(r->settings.volumeDb, 0, RADIO_FADE_MS);
  sLastPushedVolume = opening.volumeDb;
  sLastPushedBandwidth = opening.bandwidthKHz;
  r->lastError = pushToTuner(NULL, &opening);
  r->pushFailed = r->lastError != TEF668X_OK;
  publish(r);

  r->lastMemoryBand = r->settings.band;
  r->lastChosenSlot = MEMORY_NO_SLOT;
  r->nextPoll = xTaskGetTickCount() + pdMS_TO_TICKS(RADIO_POLL_INTERVAL_MS);
  r->nextRds = xTaskGetTickCount() + pdMS_TO_TICKS(RADIO_RDS_INTERVAL_MS);
  rdsReset(&sRds, r->settings.freqKHz);
}

static void radioTask(void *arg) {
  (void)arg;
  RadioRound *r = &sRound;
  roundOpen(r);

  /* Watched from here, and fed once a round, parked or not. A round that
   * never ends restarts the radio rather than leaving the tuner and RDS
   * stopped for good. */
  esp_task_wdt_add(NULL);
  for (;;) {
    esp_task_wdt_reset();
    if (sStopWanted) {
      /* Parked. Nothing read, nothing written, nothing published, so the
       * tuner and the bus belong to whoever is taking the radio down. The
       * queue is not drained either, so a knob turned during an update is
       * refused once eight commands are waiting, which is what should happen
       * to a radio that is not listening. */
      sStopped = true;
      vTaskDelay(pdMS_TO_TICKS(RADIO_STOP_POLL_MS));
      continue;
    }
    if (sStopped) {
      /* Running again, which only happens when an update failed and the
       * radio was given back. Both deadlines are minutes in the past by now,
       * so they start from here: a reading and a group are taken on this
       * round, and the two catch up steps have nothing to catch up on. */
      sStopped = false;
      r->nextPoll = xTaskGetTickCount();
      r->nextRds = xTaskGetTickCount();
    }

    r->wanted = r->settings;
    r->scanning = sScanning.load();
    r->softMuteMs = sSoftMuteMs.load();
    r->jumped = false;
    r->autoSeekTurned = false;
    r->drained = r->owed;

    roundRdsForget(r);
    roundDrain(r, roundWait(r));
    /* One channel per round while a seek is running. Moving the frequency
     * here means the push carries the retune, so seeking goes through
     * exactly the same path as a person turning the knob and cannot drift
     * from it. */
    if (sSeek.walking) {
      r->wanted.freqKHz = radioChannelNext(&r->wanted, &sPlan, sSeek.up);
    }
    roundPickProbe(r);
    roundHear(r);
    roundVolume(r);
    roundPush(r);
    roundTone(r);
    roundProbe(r);
    roundSweep(r);
    /* An AF_Update check, when one is due. After the sweep, which refuses to
     * start while a series runs, so the two never meet. */
    RadioAfSeries *afOut = sAfOut.load();
    if (afOut != NULL) {
      afStep(afOut, &r->settings);
    }
    roundCopyLive(r);
    roundSeek(r);
    roundRds(r);
    roundPoll(r);
    roundMemorySlot(r);
    r->firstPass = false;

    /* The commands were applied but nobody was told when the publish cannot
     * take the lock. They are kept, so the count never falls permanently
     * behind and strands every later waiter. */
    if (publish(r)) {
      r->owed = 0;
      r->probeOwed = 0;
    } else {
      r->owed = r->drained;
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
  agcInit(&sAgc);
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
  squelchDefaults(&sLive.squelch);
  if (settings != NULL) {
    sLive.squelch.fmLevelFloorTenths =
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
  sLive.squelchMode = SQUELCH_OFF;
  if (settings != NULL && settings->squelchMode < SQUELCH_MODE_COUNT) {
    sLive.squelchMode = (SquelchMode)settings->squelchMode;
  }
  sLive.squelchThresholdTenths = 0;
  sLastPushedMute = false;
  memset(&sSeek, 0, sizeof(sSeek));
  sBeeping = false;
  sToneOn = false;
  sDucking = false;
  seekDefaults(&sLive.seek);
  if (settings != NULL) {
    sSoftMuteMs = settings->softMuteMs;
    sLive.seek.fmSensitivity = settings->fmScanSensitivity;
    sLive.seek.amSensitivity = settings->amScanSensitivity;
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

bool radioPostOk(const RadioCommand *command, uint32_t waitMs) {
  RadioError why = RADIO_OK;
  return radioPostAndSettle(command, waitMs, &why) == RADIO_POST_DONE &&
         why == RADIO_OK;
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

void radioSetScanning(bool on) {
  sScanning.store(on);
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
  /* Last, so the first round after the park sees the two lines above and the
   * hush already cleared. Unparked first, it could run a whole round on the
   * old record and put the mute back. */
  sStopWanted = false;
}

/*
 * Take the lock from a caller on another task.
 *
 * Waits as long as it takes. Every holder only copies a struct, so the wait is
 * short, and a caller that gave up could only drop the change or touch the
 * value without the lock. Nothing mutable is shared between the two tasks
 * without a lock. False before the task has started, when there is no lock and
 * nothing else is reading. Never call it from the radio task, which already
 * holds the lock where it needs it.
 */
static bool lockFromCaller(void) {
  if (sLock == NULL) {
    return false;
  }
  xSemaphoreTake(sLock, portMAX_DELAY);
  return true;
}

/* lockFromCaller for one scope: given back when the scope ends, on every
 * path out of it. */
struct CallerLock {
  const bool held = lockFromCaller();
  ~CallerLock() {
    if (held) {
      xSemaphoreGive(sLock);
    }
  }
};

void radioSetSleepFade(bool on, uint32_t overMs) {
  /* A fade under way keeps its start, so a shorter one asked for part way
   * reaches the bottom sooner, from where it has got to. */
  sSleepFadeMs.store(overMs == 0 || overMs > UINT16_MAX ? AUTO_OFF_FADE_MS
                                                        : overMs);
  sSleepFadeWanted.store(on);
}

void radioSetSoftMuteMs(uint16_t ms) {
  sSoftMuteMs = ms;
}

void radioSetSquelchFloor(uint8_t dbuv) {
  int16_t tenths = dbuv == 0 ? SQUELCH_LEVEL_FLOOR_OFF : (int16_t)(dbuv * 10);
  CallerLock lock;
  sLive.squelch.fmLevelFloorTenths = tenths;
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

  /* Ask the task to park, and wait until it says it has. Before the ramp,
   * because the ramp is this task writing to the bus the radio task reads
   * from. */
  sStopWanted = true;
  uint32_t asked = millis();
  while (!sStopped && (uint32_t)(millis() - asked) < RADIO_STOP_WAIT_MS) {
    vTaskDelay(pdMS_TO_TICKS(RADIO_STOP_POLL_MS));
  }
  if (!sStopped) {
    /* Said out loud rather than passed back, because every caller of this is
     * on its way to a reboot and none of them can do anything about it. A
     * shared bus for a round is a reading that comes back wrong, not a radio
     * that breaks. */
    DebugLog.println(F("[radio] the task did not park in time, carrying on"));
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
  CallerLock lock;
  sLive.seek = wanted;
}

void radioSetSquelchMode(SquelchMode mode) {
  if (mode >= SQUELCH_MODE_COUNT) {
    return;
  }
  CallerLock lock;
  sLive.squelchMode = mode;
}

void radioSetAgc(uint8_t targetPercent, uint8_t boostDb) {
  CallerLock lock;
  sLive.agc.targetPercent = targetPercent;
  sLive.agc.boostDb = boostDb;
}

void radioSetSquelchThreshold(int16_t tenths) {
  CallerLock lock;
  sLive.squelchThresholdTenths = tenths;
}

void radioSeekConfig(SeekConfig *out) {
  if (out == NULL) {
    return;
  }
  CallerLock lock;
  *out = sLive.seek;
}

SquelchMode radioSquelchMode(int16_t *thresholdTenths) {
  CallerLock lock;
  SquelchMode mode = sLive.squelchMode;
  int16_t threshold = sLive.squelchThresholdTenths;
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

bool radioSweepStart(DxSweep *out, const RadioSweepPlan *plan) {
  if (out == NULL || sLock == NULL || sTask == NULL || sHushed) {
    return false;
  }
  RadioSnapshot now;
  if (!radioGetSnapshot(&now) || now.seeking) {
    return false;
  }
  /* DX mode's sweep, with no plan, is FM's. */
  const bool fm = bandModulation(now.settings.band) == MODULATION_FM;
  if (plan == NULL && !fm) {
    return false;
  }
  /* A band scan's probes would wait behind the sweep, and the update
   * check's transmitting would raise every level read. */
  if (sSweepOut.load() != nullptr || sAfOut.load() != nullptr ||
      bandScanActive() || updateCheckRunning()) {
    return false;
  }
  if (plan != NULL) {
    const bool widthFits =
        plan->widthKHz == 0 ||
        (fm && bandBandwidthAllowed(BAND_FM, plan->widthKHz));
    if (!dxSweepRangeFits(now.settings.band, &sPlan, &plan->range) ||
        !widthFits) {
      return false;
    }
    sSweepPlan = *plan;
  }
  sSweepPlanned = plan != NULL;
  sSweepBand = now.settings.band;
  sSweepCancel.store(false);
  DxSweep *none = nullptr;
  return sSweepOut.compare_exchange_strong(none, out);
}

void radioSweepCancel(void) {
  sSweepCancel.store(true);
}

bool radioAfStart(RadioAfSeries *out) {
  if (out == NULL || out->check == NULL || sLock == NULL || sTask == NULL ||
      sHushed || out->count == 0 || out->count > RADIO_AF_MAX ||
      out->everyMs < 10 || out->everyMs > 10000 || out->widthKHz == 0 ||
      !bandBandwidthAllowed(BAND_FM, out->widthKHz)) {
    return false;
  }
  RadioSnapshot now;
  if (!radioGetSnapshot(&now) || now.seeking ||
      bandModulation(now.settings.band) != MODULATION_FM ||
      !bandContains(now.settings.band, &sPlan, out->khz) ||
      (out->khz % 10) != 0 || sSweepOut.load() != nullptr) {
    return false;
  }
  out->done = 0;
  out->ended = false;
  out->stopped = false;
  sAfDone.store(0);
  sAfCancel.store(false);
  RadioAfSeries *none = nullptr;
  return sAfOut.compare_exchange_strong(none, out);
}

void radioSetNetServing(bool serving) {
  sNetServing.store(serving);
}

bool radioAfBusy(void) {
  return sAfOut.load() != nullptr;
}

uint16_t radioAfProgress(void) {
  return sAfDone.load();
}

void radioAfCancel(void) {
  sAfCancel.store(true);
}

bool radioSweepBusy(void) {
  return sSweepOut.load() != nullptr;
}

RadioProbeResult radioSettleProbe(uint32_t khz, uint16_t settleMs,
                                  uint8_t reads, uint16_t gapMs,
                                  uint32_t expectKHz, Tef668xQuality *out,
                                  bool *moved) {
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
  /* Before the band check: a band change is a tune too, and it should read as
   * one rather than as a bad request. */
  if (expectKHz != 0 && (now.settings.freqKHz != expectKHz || now.seeking)) {
    return RADIO_PROBE_MOVED;
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
  /* Never 0, which the radio task reads as no probe. */
  if (++sProbeSeq == 0) {
    ++sProbeSeq;
  }
  uint32_t mine = sProbeSeq;
  sProbeKHz = khz;
  sProbeMs = settleMs;
  sProbeReads = reads;
  sProbeGapMs = gapMs;
  sProbeDone = false;
  sProbeOk = false;
  sProbeExpectKHz = expectKHz;
  sProbeMoved = false;
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
    bool movedAway = false;
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
        movedAway = sProbeMoved;
      }
      xSemaphoreGive(sLock);
    }
    if (done) {
      if (movedAway) {
        return RADIO_PROBE_MOVED;
      }
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

/* Copy part of the snapshot out under its lock, with the one wait every
 * reader shares. */
static bool copyFromSnapshot(void *out, const void *from, size_t size) {
  if (sLock == NULL || out == NULL) {
    return false;
  }
  if (xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) != pdTRUE) {
    return false;
  }
  memcpy(out, from, size);
  xSemaphoreGive(sLock);
  return true;
}

bool radioGetSnapshot(RadioSnapshot *out) {
  return copyFromSnapshot(out, &sSnapshot, sizeof(*out));
}

bool radioGetSettings(RadioSettings *out) {
  return copyFromSnapshot(out, &sSnapshot.settings, sizeof(*out));
}

uint32_t radioTaskStackFree(void) {
  /* Set once at start, before any caller can ask, and never cleared while
   * the task runs. */
  return sTask != NULL ? (uint32_t)uxTaskGetStackHighWaterMark(sTask) : 0;
}

bool radioAgcGain(int8_t *db) {
  if (db == NULL || sLock == NULL ||
      xSemaphoreTake(sLock, pdMS_TO_TICKS(50)) != pdTRUE) {
    return false;
  }
  const bool on = sSnapshot.agcOn;
  *db = sSnapshot.agcGainDb;
  xSemaphoreGive(sLock);
  return on;
}
