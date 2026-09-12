/**
 * @file input_task.h
 * @brief The knob, the buttons and the keypad, turned into radio commands.
 *
 * This sits outside the five layers for the same reason radio_task.h does. It
 * is a composition root: the one place allowed to know about `core/` and
 * `drivers/` at once. Nothing in `core/` may include it.
 *
 * It runs on the loop task, not on a task of its own. Reading a few pins and
 * one I2C register takes microseconds, and decision 7 says no third task gets
 * added because something feels slow.
 *
 * Commands go on the same queue the HTTP API uses. There is no second path
 * into the tuner, which is what stops the knob and the browser disagreeing
 * about what the radio is set to.
 */
#ifndef INPUT_TASK_H
#define INPUT_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/input.h"

/** How many typed digits are kept before the rest are ignored. */
#define INPUT_DIGITS_MAX 8

/** How long the description of the last event can be, with its terminator. */
#define INPUT_EVENT_MAX 40

/** What the input layer has seen, for a diagnostic page. */
typedef struct {
  bool keypadPresent;              /**< The expander answered at start up. */
  uint32_t clicks;                 /**< Knob clicks since boot. */
  uint32_t presses;                /**< Button and key events since boot. */
  char lastEvent[INPUT_EVENT_MAX]; /**< The last one in words, such as
                                    *   "BAND long". */
  uint32_t lastEventMs; /**< When that was, ms since boot. 0 for never. */
  char typed[INPUT_DIGITS_MAX + 1]; /**< Digits keyed and not yet entered. */
  uint16_t lines;   /**< The keypad's sixteen lines. A 0 bit is a key held. */
  uint16_t linesOk; /**< Non zero once the lines have been read at all. */
} InputStatus;

/**
 * Set the knob, the buttons and the keypad up.
 *
 * A missing keypad is not a failure. The expander is on the same I2C bus as
 * the tuner, and a radio whose keypad is not fitted still has to work.
 *
 * @param kind       Which encoder is fitted.
 * @param direction  Which way round it is wired.
 * @return true when the keypad answered as well.
 */
bool inputBegin(EncoderKind kind, EncoderDirection direction);

/**
 * Read everything once and act on what changed.
 *
 * Call this from loop(). It never blocks and it never talks to the tuner.
 */
void inputPoll(void);

/**
 * What the input layer has seen.
 *
 * Without a screen this is the only way to tell a dead switch from a wrong
 * pin number, so it exists before the display does rather than after.
 *
 * @param out  Receives the status.
 */
void inputStatusGet(InputStatus *out);

#endif /* INPUT_TASK_H */
