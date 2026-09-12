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
#include "drivers/tef668x.h"

/** How often the task reads the tuner, in milliseconds. */
#define RADIO_POLL_INTERVAL_MS 100

/** How many commands can be waiting before a caller is told to try later. */
#define RADIO_QUEUE_DEPTH 8

/** Everything a reader needs, copied out in one go so it cannot tear. */
typedef struct {
  RadioSettings settings; /**< What the radio is set to. */
  Tef668xQuality quality; /**< The last reading from the tuner. */
  bool qualityValid;      /**< False when the last read failed. */
  bool tunerReady;        /**< The tuner started up. */
  Tef668xError lastError; /**< What the tuner last complained about. */
  uint32_t updatedMs;     /**< When this was taken, ms since boot. */
  uint32_t sequence;      /**< Goes up every time. Spots a stalled task. */
} RadioSnapshot;

/**
 * Start the radio task.
 *
 * Brings the tuner up, then pins a task to core 0 that owns it. Returns once
 * the task is running, whether or not the tuner came up: a radio with a dead
 * tuner still has to be reachable, because that is how a fix gets installed.
 *
 * @param plan  The regional band choices. Copied, not kept by reference.
 * @return true when the task started. False means out of memory, and the
 *         radio has no business continuing.
 */
bool radioTaskStart(const BandPlanConfig *plan);

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

/**
 * Ask whether a command would be accepted, without posting it.
 *
 * For callers that have to tell someone whether it worked before the radio
 * has got round to it, such as the HTTP API answering a request.
 *
 * This does not ask the radio task. It runs the same pure state machine over
 * a copy of the last snapshot, which gives the same answer for the same
 * input. That is what lets a caller report a refusal straight away without
 * waiting for, or blocking, the task.
 *
 * @param command  What to ask about.
 * @param result   Receives why it would be refused. May be NULL.
 * @return true when it would be accepted.
 *
 * @note Strictly there is a gap between this and the post that follows it, in
 *       which something else could change the band and make the answer stale.
 *       Nothing else does: commands come from one person turning one knob or
 *       making one request. If that stops being true, this has to become a
 *       real round trip rather than being patched.
 */
bool radioWouldAccept(const RadioCommand *command, RadioError *result);

/**
 * Take a copy of the radio's state.
 *
 * @param out  Receives the snapshot.
 * @return false when the task is not running, in which case nothing is
 *         written.
 */
bool radioGetSnapshot(RadioSnapshot *out);

#endif /* RADIO_TASK_H */
