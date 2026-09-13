/*
 * The radio task, the command queue into it, and the snapshot out of it.
 *
 * This is decision 7 made real. The radio owns core 0 and the tuner, and
 * nothing else touches that chip. Everything else runs on core 1 and talks to
 * it two ways only: a command goes in through a queue, and a snapshot of the
 * state comes out from behind a lock. No shared variable, ever.
 *
 * That is what stops a slow web request making the radio drop RDS groups
 * later, which the firmware this replaces does because it runs everything in
 * one cooperative loop.
 *
 * This file sits outside the five layers on purpose. It is the composition
 * root: the one place allowed to know about both `core/` and `drivers/` at
 * once, in the same way main.cpp is. Nothing in `core/` may include it.
 */
#ifndef RADIO_TASK_H
#define RADIO_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/memory.h"
#include "core/radio.h"
#include "core/rds.h"
#include "core/seek.h"
#include "core/squelch.h"
#include "drivers/tef668x.h"

/*
 * How often the task reads the tuner, in milliseconds.
 *
 * The RDS read below keeps its own faster deadline, and the task wakes for
 * whichever comes first, so on FM it comes round about thirty times a second
 * rather than ten. Measured on 13 September 2026: 30.8 ms a round on FM
 * against 99.8 on medium wave, which is what 1/0.043 plus 1/0.1 predicts.
 * Only the deadline that is actually due does any work, but the state
 * snapshot goes out on every round either way.
 */
#define RADIO_POLL_INTERVAL_MS 100

/*
 * How often the RDS decoder is asked for a group, in milliseconds.
 *
 * A station sends about 11.4 groups a second, so a group arrives every 87 ms.
 * Asking twice as often is what stops one being overwritten by the next
 * before it has been read, and it is the cadence the reference firmware uses
 * on this same chip.
 */
#define RADIO_RDS_INTERVAL_MS 43

/*
 * How many raw groups are kept for capturing test fixtures.
 *
 * A station sends about 11.4 groups a second, so 128 of them is about eleven
 * seconds. That is long enough that a capture script polling every second
 * cannot miss one.
 *
 * It costs 2560 bytes, not 1280: the web handler keeps its own copy so that
 * the lock is released before anything goes out on the network. That is the
 * price of the only way to get real groups off this radio, because there is
 * no serial cable on it.
 */
#define RADIO_RDS_RAW_DEPTH 128

/*
 * How long the radio task will wait for the lock to store a raw group, in ms.
 *
 * Short on purpose. The ring is for capturing fixtures, and the tuner cadence
 * matters more than a capture does, so a group is given up rather than the
 * radio task held.
 */
#define RADIO_RDS_RING_WAIT_MS 5

/* One group exactly as the tuner handed it over. */
typedef struct {
  uint16_t block[4]; /* A, B, C, D. */
  /* Two bits per block, block A in the top pair, as the chip packs them. */
  uint8_t error;
} RadioRdsRaw;

/* How many commands can be waiting before a caller is told to try later. */
#define RADIO_QUEUE_DEPTH 8

/* How many command outcomes the snapshot remembers. One per queue slot. */
#define RADIO_OUTCOMES RADIO_QUEUE_DEPTH

/* What the state machine made of one command. */
typedef struct {
  uint32_t ticket;   /* Which command this was. 0 means an unused slot. */
  RadioError result; /* RADIO_OK, or why it was refused. */
} RadioOutcome;

/* Everything a reader needs, copied out in one go so it cannot tear. */
typedef struct {
  RadioSettings settings; /* What the radio is set to. */
  Tef668xQuality quality; /* The last reading from the tuner. */
  /*
   * The signal level, smoothed, in tenths of a dBuV.
   *
   * For anything a person looks at. One reading of this tuner moves several
   * dB between polls on a signal that is not moving, so a meter or a number
   * driven off `quality.levelDbuVTenths` jumps far more than the signal
   * does. Both are published rather than one replacing the other: a sweep
   * wants the reading it took, and a person wants the signal.
   *
   * It is the same running average core/signal.h applies before any of the
   * radio's own decisions. The average is started again on a retune and at
   * the end of a seek, so the first reading from the new station is taken as
   * the answer rather than averaged in with the old one.
   *
   * Starting the average again does not change this field. It holds the last
   * value a reading produced until the next reading arrives, which is what
   * `quality` beside it does, so the two are always from the same read. For
   * the one poll interval after a retune both still describe where the dial
   * was. Zeroing it instead would put a level of 0.0 dBuV on the panel, and
   * a reading of zero is a real reading, so that would be a worse lie than a
   * tenth of a second of an old one.
   */
  int16_t levelSmoothedTenths;
  /*
   * Whether that level was read at the station this snapshot describes.
   *
   * False for the round or two between a retune and the next reading. The
   * dial moves as soon as the command is worked through, but the reading
   * keeps its own hundred millisecond cadence, so there is a snapshot
   * carrying the new frequency and the previous station's level.
   *
   * Anything that holds a value still has to know. A screen that resets its
   * held reading on the frequency changing, and is then handed the old
   * station's level, latches that instead of the new one and sits there.
   */
  bool levelSmoothedValid;
  /*
   * What the RDS decoder has made of this station.
   *
   * Every band. On AM it is cleared and stays cleared, because the retune
   * onto an AM band resets it and nothing feeds it there, so `synchronised`
   * false and every `has` flag false is the truthful answer rather than a
   * leftover from the last FM station.
   */
  RdsInfo rds;
  Tef668xProcessing processing; /* What the chip is doing to the audio. */
  bool processingValid;         /* False when that read failed or is AM. */
  bool qualityValid;            /* False when the last read failed. */
  bool tunerReady;              /* The tuner started up. */
  Tef668xError lastError;       /* What the tuner last complained about. */
  uint32_t updatedMs;           /* When this was taken, ms since boot. */
  uint32_t sequence;            /* Goes up every time. Spots a stalled task. */
  uint32_t applied;   /* How many commands the task has worked through. */
  bool bandwidthWide; /* The adaptive filter is allowed to open. */
  /*
   * What the tuner was last told about the mute.
   *
   * Not the same as settings.muted, which is what the person asked for. The
   * tuner is told that, or silence because the squelch is shut. When the
   * radio has gone quiet for no reason the page can give, the difference
   * between these two is the first thing worth seeing.
   */
  bool tunerMuted;
  SquelchMode squelchMode; /* What decides whether the audio is open. */
  bool squelchOpen;        /* Whether the squelch is letting sound through. */
  /*
   * A seek is running, so the dial is moving on its own.
   *
   * Worth publishing rather than leaving the caller to guess from a
   * frequency that keeps changing. A screen that cannot tell seeking from a
   * person spinning the knob shows the same thing for both.
   */
  bool seeking;
  /* The last seek found a station. False means it came back empty. */
  bool seekFound;
  /* A tone is sounding now. */
  bool beeping;
  int16_t squelchThresholdTenths; /* What Manual is set to. */
  /*
   * Which stored channel the radio is on, or MEMORY_NO_SLOT, counted from 0.
   *
   * Worked out from the band and the frequency, not from what memory mode
   * last did, so it is true however the radio got there. A station reached
   * with the keypad that happens to be stored shows its slot.
   */
  int16_t memorySlot;
  /* What came of the last few commands, so a caller can be told the truth
   *  about its own one rather than about the state that followed it. */
  RadioOutcome outcomes[RADIO_OUTCOMES];
} RadioSnapshot;

/*
 * Start the radio task.
 *
 * Brings the tuner up, then pins a task to core 0 that owns it. Returns once
 * the task is running, whether or not the tuner came up: a radio with a dead
 * tuner still has to be reachable, because that is how a fix gets installed.
 *
 * All of it is given here rather than posted as commands afterwards. The task
 * unmutes at the end of its first push, so anything sent after that is heard:
 * posting the frequency gave a burst of noise from the default frequency, and
 * posting the volume gave a moment at full volume before the knob's value
 * arrived.
 */
bool radioTaskStart(const Settings *settings, const BandPlanConfig *plan,
                    int8_t startVolumeDb);

/*
 * Ask the radio to do something.
 *
 * Safe from any task. Returns as soon as the command is queued, not when it
 * has been carried out, so a caller that needs to see the result reads a
 * snapshot afterwards.
 */
bool radioPost(const RadioCommand *command);

/* What came of asking the radio to do something and waiting for it. */
typedef enum {
  RADIO_POST_DONE, /* The radio has worked through it. */
  RADIO_POST_BUSY, /* The queue was full. Nothing was taken. */
  RADIO_POST_SLOW  /* Taken, but not carried out inside the wait. */
} RadioPostResult;

/*
 * Ask the radio to do something, and wait until it has been done.
 *
 * The same queue as radioPost, but it returns only once the task has taken
 * the command off the queue and published the state that came of it. A
 * snapshot read after this call shows the result, so a caller can report what
 * actually happened rather than what it hoped would happen.
 *
 * Waiting costs one poll interval at worst. It blocks the calling task, never
 * the radio task, so the HTTP API can use it and the radio keeps its cadence.
 *
 * The three answers are kept apart on purpose. A command that was taken but
 * not finished in time is not a command that failed: it is still on the queue
 * and will be carried out. Telling a caller it failed would have them send it
 * again, and the radio would do it twice.
 *
 * RADIO_POST_DONE means the radio dealt with the command, not that it liked
 * it. What it made of it comes back in `result`, which is the state machine's
 * own answer about this exact command rather than a guess made beforehand
 * against a state that may since have moved.
 */
RadioPostResult radioPostAndSettle(const RadioCommand *command, uint32_t waitMs,
                                   RadioError *result);

/*
 * The band plan the radio task is working to.
 *
 * A caller that has to work out which band a frequency is in must use this
 * one, not its own defaults. The two agree today and would stop agreeing the
 * moment the plan comes from settings, and then the answer given to a caller
 * would be about a different band from the one the radio tuned.
 */
bool radioTaskPlan(BandPlanConfig *out);

/*
 * Choose what decides whether the audio is open.
 *
 * Safe from any task.
 */
void radioSetSquelchMode(SquelchMode mode);

/*
 * Start hunting for the next station.
 *
 * Returns as soon as the command is queued. The seek itself takes as long as
 * it takes, up to one full pass of the band, and the snapshot says while it
 * is running. Any other command stops it where it stands.
 */
bool radioSeek(bool up);

/*
 * Sound a short tone.
 *
 * Safe from any task. Returns as soon as it is queued. The tone is played
 * through the tuner's own generator and stops on its own.
 *
 * A radio that is muted stays silent: the tone goes through the same output
 * mute as everything else, and lifting the mute to beep at somebody who asked
 * for quiet would be the wrong way round.
 */
bool radioBeep(uint16_t ms);

bool radioBeepAt(uint16_t ms, uint16_t hz, uint16_t hz2);

/*
 * How long the audio ramps down before it is cut, in milliseconds.
 *
 * Zero cuts instantly, which is what switching the ramp off has to mean.
 * Safe from any task.
 */
void radioSetSoftMuteMs(uint16_t ms);

/*
 * Whether the dial wrapping at a band edge makes a sound.
 *
 * Decided here rather than by the caller that turns the knob, because only
 * the radio knows the dial wrapped: the encoder sends a number of steps and
 * never learns where they landed.
 */
void radioSetEdgeBeep(bool on);

/*
 * The level an FM signal has to reach before the auto squelch opens.
 *
 * The channel beside a strong station passes every other test the squelch
 * applies, because the sidebands of the station next door really are in the
 * channel. Only level separates them, and how strong that channel reads
 * depends on where the radio is, so this is settable rather than fixed.
 */
void radioSetSquelchFloor(uint8_t dbuv);

/*
 * Take the audio down and mute, then return.
 *
 * For a reboot or a firmware update, so neither ends in a click. Blocks for
 * the length of the ramp, which is the point: the caller is about to stop the
 * radio, so there is nothing left to be responsive for.
 *
 * **The radio does not come back from this on its own.** The task stops
 * writing to the tuner once this has been called, so the knob, the buttons
 * and the API all stop reaching the audio. Call it on the way to a restart,
 * or call radioResume if the restart is called off.
 *
 * Safe when the radio task never started, in which case it mutes the tuner
 * and returns.
 */
void radioHush(void);

/*
 * Let the radio speak again after radioHush.
 *
 * For a caller that hushed the radio for a restart that then did not happen.
 * An update over the air is the case that needs it: the transfer is hushed
 * the moment it starts and can still fail halfway, and without this the
 * radio stays silent until somebody power cycles it while still answering
 * every request with a normal frequency and a good signal.
 *
 * The audio comes back up the same ramp an unmute uses. Safe to call when
 * the radio was never hushed, and safe when the task never started.
 */
void radioResume(void);

/*
 * How fussy seek is about what counts as a station.
 *
 * Safe from any task. Takes effect on the next seek, not on one already
 * running.
 */
void radioSetSeekConfig(const SeekConfig *cfg);

void radioSetSquelchThreshold(int16_t tenths);

/*
 * Whether the RDS decoder runs at all.
 *
 * Turning it off stops the 43 ms read, so the radio task goes back to coming
 * round ten times a second on FM instead of about thirty, and everything the
 * decoder held is thrown away rather than left to go stale. Turning it back
 * on starts from nothing on whatever station the dial is on.
 *
 * Safe from any task.
 */
void radioSetRdsEnabled(bool on);

/* Whether the RDS decoder is running. */
bool radioRdsEnabled(void);

/*
 * What the squelch is set to now.
 *
 * Read from where it is kept, not from the snapshot. The snapshot is only
 * republished when the radio task comes round, which is ten times a second on
 * AM and about thirty on FM, so a caller that sets the mode and then reads it
 * back from there gets the value from before it was set. That is how the API
 * came to answer "Off" to a request that turned it to Auto.
 */
SquelchMode radioSquelchMode(int16_t *thresholdTenths);

/*
 * Copy out the raw groups the tuner has handed over, oldest first.
 *
 * For capturing fixtures. The decoded answer is in the snapshot; this is the
 * data it was decoded from, which is what a test needs so that it replays a
 * real broadcast rather than a made up one.
 *
 * Returns how many groups were written, at most `max`. `firstSequence` is the
 * number of the first one written and `total` is how many have arrived on
 * this station, so a caller polling this can tell whether it missed any
 * between two calls rather than quietly joining two pieces of a broadcast.
 * `dropped` counts groups the ring could not be given because the lock was
 * busy, which is a hole in the capture and not a hole in the broadcast.
 *
 * The ring is emptied on every retune, so it never holds two stations. Until
 * that has happened it serves nothing rather than what it still holds.
 *
 * Safe from any task.
 */
uint16_t radioRdsRaw(RadioRdsRaw *out, uint16_t max, uint32_t *firstSequence,
                     uint32_t *total, uint32_t *dropped);

/*
 * The status word from the last RDS read, exactly as the chip sent it, and
 * whether that read worked at all.
 *
 * A station with no RDS and a bus that is not answering both show no groups.
 * This is what tells them apart without a serial cable.
 *
 * Returns false when the lock was busy and it could not find out, which is a
 * third answer and not the same as a read that has not happened. Nothing is
 * written to `status` or `read` then. A caller that reports false as "not
 * read" is saying something it does not know, and on the AM side "not read"
 * is the true answer, so the two would be indistinguishable.
 *
 * Safe from any task.
 */
bool radioRdsStatus(uint16_t *status, bool *read);

bool radioGetSnapshot(RadioSnapshot *out);

#endif /* RADIO_TASK_H */
