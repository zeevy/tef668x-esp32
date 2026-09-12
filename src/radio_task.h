/**
 * @file radio_task.h
 * @brief The radio task, the command queue into it, and the snapshot out of it.
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

#include "core/radio.h"
#include "core/squelch.h"
#include "drivers/tef668x.h"

/** How often the task reads the tuner, in milliseconds. */
#define RADIO_POLL_INTERVAL_MS 100

/** How many commands can be waiting before a caller is told to try later. */
#define RADIO_QUEUE_DEPTH 8

/** How many command outcomes the snapshot remembers. One per queue slot. */
#define RADIO_OUTCOMES RADIO_QUEUE_DEPTH

/** What the state machine made of one command. */
typedef struct {
  uint32_t ticket;   /**< Which command this was. 0 means an unused slot. */
  RadioError result; /**< RADIO_OK, or why it was refused. */
} RadioOutcome;

/** Everything a reader needs, copied out in one go so it cannot tear. */
typedef struct {
  RadioSettings settings;       /**< What the radio is set to. */
  Tef668xQuality quality;       /**< The last reading from the tuner. */
  Tef668xProcessing processing; /**< What the chip is doing to the audio. */
  bool processingValid;         /**< False when that read failed or is AM. */
  bool qualityValid;            /**< False when the last read failed. */
  bool tunerReady;              /**< The tuner started up. */
  Tef668xError lastError;       /**< What the tuner last complained about. */
  uint32_t updatedMs;           /**< When this was taken, ms since boot. */
  uint32_t sequence;  /**< Goes up every time. Spots a stalled task. */
  uint32_t applied;   /**< How many commands the task has worked through. */
  bool bandwidthWide; /**< The adaptive filter is allowed to open. */
  /**
   * What the tuner was last told about the mute.
   *
   * Not the same as settings.muted, which is what the person asked for. The
   * tuner is told that, or silence because the squelch is shut. When the
   * radio has gone quiet for no reason the page can give, the difference
   * between these two is the first thing worth seeing.
   */
  bool tunerMuted;
  SquelchMode squelchMode; /**< What decides whether the audio is open. */
  bool squelchOpen;        /**< Whether the squelch is letting sound through. */
  int16_t squelchThresholdTenths; /**< What Manual is set to. */
  /** What came of the last few commands, so a caller can be told the truth
   *  about its own one rather than about the state that followed it. */
  RadioOutcome outcomes[RADIO_OUTCOMES];
} RadioSnapshot;

/**
 * Start the radio task.
 *
 * Brings the tuner up, then pins a task to core 0 that owns it. Returns once
 * the task is running, whether or not the tuner came up: a radio with a dead
 * tuner still has to be reachable, because that is how a fix gets installed.
 *
 * @param settings  The stored settings. Everything a person has chosen comes
 *                  from here: the band and frequency to come up on, the FM
 *                  features, the blend levels, the blankers and the squelch
 *                  mode. NULL means the defaults.
 * @param plan  The regional band choices, from radioPlanFromSettings. Copied,
 *              not kept by reference.
 * @param startVolumeDb The volume to come up at, which is where the knob is
 *                      pointing.
 *
 * All of it is given here rather than posted as commands afterwards. The task
 * unmutes at the end of its first push, so anything sent after that is heard:
 * posting the frequency gave a burst of noise from the default frequency, and
 * posting the volume gave a moment at full volume before the knob's value
 * arrived.
 * @return true when the task started. False means out of memory, and the
 *         radio has no business continuing.
 */
bool radioTaskStart(const Settings *settings, const BandPlanConfig *plan,
                    int8_t startVolumeDb);

/**
 * Ask the radio to do something.
 *
 * Safe from any task. Returns as soon as the command is queued, not when it
 * has been carried out, so a caller that needs to see the result reads a
 * snapshot afterwards.
 *
 * @param command  What to do.
 * @return false when the queue is full or the task is not running.
 */
bool radioPost(const RadioCommand *command);

/** What came of asking the radio to do something and waiting for it. */
typedef enum {
  RADIO_POST_DONE, /**< The radio has worked through it. */
  RADIO_POST_BUSY, /**< The queue was full. Nothing was taken. */
  RADIO_POST_SLOW  /**< Taken, but not carried out inside the wait. */
} RadioPostResult;

/**
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
 *
 * @param command  What to do.
 * @param waitMs   How long to wait for the task to get to it.
 * @param result   Receives RADIO_OK, or why the radio refused it. Only
 *                 meaningful on RADIO_POST_DONE. May be NULL.
 * @return RADIO_POST_DONE, RADIO_POST_BUSY when there was no room on the
 *         queue, or RADIO_POST_SLOW when the wait ran out.
 */
RadioPostResult radioPostAndSettle(const RadioCommand *command, uint32_t waitMs,
                                   RadioError *result);

/**
 * The band plan the radio task is working to.
 *
 * A caller that has to work out which band a frequency is in must use this
 * one, not its own defaults. The two agree today and would stop agreeing the
 * moment the plan comes from settings, and then the answer given to a caller
 * would be about a different band from the one the radio tuned.
 *
 * @param out  Receives a copy of the plan.
 * @return false when the task is not running, in which case nothing is
 *         written.
 */
bool radioTaskPlan(BandPlanConfig *out);

/**
 * Choose what decides whether the audio is open.
 *
 * Safe from any task.
 *
 * @param mode  Off, Auto or Manual.
 */
void radioSetSquelchMode(SquelchMode mode);

/**
 * Set the threshold Manual mode works to.
 *
 * @param tenths  The level a signal has to beat, in tenths of a dBuV.
 */
void radioSetSquelchThreshold(int16_t tenths);

/**
 * What the squelch is set to now.
 *
 * Read from where it is kept, not from the snapshot. The snapshot is only
 * republished ten times a second, so a caller that sets the mode and then
 * reads it back from there gets the value from before it was set. That is
 * how the API came to answer "Off" to a request that turned it to Auto.
 *
 * @param thresholdTenths  Receives the manual threshold. May be NULL.
 * @return The mode.
 */
SquelchMode radioSquelchMode(int16_t *thresholdTenths);

/**
 * Take a copy of the radio's state.
 *
 * @param out  Receives the snapshot.
 * @return false when the task is not running, in which case nothing is
 *         written.
 */
bool radioGetSnapshot(RadioSnapshot *out);

#endif /* RADIO_TASK_H */
