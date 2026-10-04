/*
 * What the battery voltage means.
 *
 * No hardware here. The driver hands over millivolts from the divider and
 * this decides whether that is a battery at all, smooths it, and turns it
 * into something to show.
 *
 * A battery reading is easy to get wrong: the sense pin is on ADC2, Wi-Fi
 * owns ADC2, and the Arduino core's analogRead returns 0 when it cannot
 * have it rather than saying so. A 0 that means "cannot read" and a 0 that
 * means "flat" are the same number. So nothing here ever returns a level it
 * is not sure of.
 */
#ifndef CORE_BATTERY_H
#define CORE_BATTERY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The cell, in millivolts.
 *
 * A single lithium polymer cell, 3.7 V nominal and 2500 mAh. Full is 4.2 V
 * by the chemistry. Empty is taken as 3.0 V, which is where the protection
 * circuit in a pack like this cuts off, and it is the number the reference
 * firmware uses on this same radio.
 */
#define BATTERY_FULL_MV 4200
#define BATTERY_EMPTY_MV 3000

/*
 * Below this, it is not a battery.
 *
 * A cell that is connected at all sits above 2.5 V, because the protection
 * board disconnects before it gets there. So anything under this is one of
 * three things: no battery fitted, the ADC handing back a zero because Wi-Fi
 * had the converter, or a divider that is not wired the way this assumes.
 * None of them is a reading, and all three have to look the same from here
 * because nothing can tell them apart.
 */
#define BATTERY_PLAUSIBLE_MV 2000

/*
 * Above this, the divider is not what this thinks it is.
 *
 * A single cell cannot exceed about 4.25 V. A reading well past that means
 * the assumed ratio is wrong, and reporting it would put a confident number
 * on screen that is arithmetic rather than measurement.
 */
#define BATTERY_IMPLAUSIBLE_MV 5000

/* How many characters batteryFormat writes, including the terminator. */
#define BATTERY_TEXT_LEN 8

/* How the reading is shown, if at all. */
typedef enum {
  BATTERY_SHOW_OFF = 0, /* Nothing on the panel. */
  BATTERY_SHOW_PERCENT, /* "78%" */
  BATTERY_SHOW_VOLTS,   /* "3.9", with the upright battery before it */
  BATTERY_SHOW_COUNT
} BatteryShow;

/* The running state. Zero it, or call batteryReset. */
typedef struct {
  int32_t filteredMv; /* Where the smoothing has got to. */
  bool have;          /* False until the first plausible reading. */
  uint8_t strikes;    /* Unreadable attempts in a row. */
} Battery;

void batteryReset(Battery *b);

/*
 * Take one reading from the driver.
 *
 * `milliVolts` is the cell voltage the driver worked out, and `ok` is whether
 * the driver could read at all. Returns true when there is now something
 * worth showing.
 *
 * Smoothed, because a few millivolts of converter noise is a whole per cent
 * of the 3.0 to 4.2 V span and the number would otherwise walk between 49 and
 * 50 with nothing happening. A battery does not move quickly, so the filter
 * can be slow.
 *
 * One bad reading does not throw the answer away, because blanking the panel
 * on each one would flicker. It takes BATTERY_STRIKES in a row, each either
 * not read or out of the plausible range. The firmware feeds in only the
 * readings that arrived, so it always passes `ok` as true, and a strike comes
 * only from a voltage out of range.
 */
#define BATTERY_STRIKES 3
bool batteryFeed(Battery *b, uint16_t milliVolts, bool ok);

/* The smoothed voltage in millivolts, or 0 when there is nothing to say. */
uint16_t batteryMilliVolts(const Battery *b);

/*
 * Where the cell sits between empty and full, 0 to 100.
 *
 * This is a voltage, mapped. It is not a charge gauge and does not claim to
 * be: a lithium cell holds most of its charge across a narrow part of its
 * voltage range, so the number falls slowly and then quickly. Doing better
 * needs the current as well, and this radio does not measure it.
 */
uint8_t batteryPercent(const Battery *b);

/*
 * Write the reading the way the setting asks for.
 *
 * Returns false and writes an empty string when there is no reading or the
 * setting is off, so a caller that ignores the result shows nothing rather
 * than a zero.
 */
bool batteryFormat(const Battery *b, BatteryShow show, char *out,
                   size_t outLen);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BATTERY_H */
