/* What the battery voltage means. No hardware. */
#include "battery.h"

#include <stdio.h>
#include <string.h>

#include "core/strings.h"

/*
 * How slow the smoothing is.
 *
 * One sixteenth of the gap per reading. At one reading a second, which the
 * screen task takes while Wi-Fi is off, a step change settles in about half
 * a minute. That is far slower than anything a battery does and far faster
 * than anything a person waits for. With Wi-Fi on there is only the reading
 * taken at start up.
 */
#define BATTERY_SMOOTHING 16

void batteryReset(Battery *b) {
  if (b == NULL) {
    return;
  }
  b->filteredMv = 0;
  b->have = false;
  b->strikes = 0;
}

bool batteryFeed(Battery *b, uint16_t milliVolts, bool ok) {
  if (b == NULL) {
    return false;
  }

  const bool plausible = ok && milliVolts >= BATTERY_PLAUSIBLE_MV &&
                         milliVolts <= BATTERY_IMPLAUSIBLE_MV;
  if (!plausible) {
    if (b->strikes < BATTERY_STRIKES) {
      b->strikes++;
    }
    if (b->strikes >= BATTERY_STRIKES) {
      /* Given up. The panel goes back to showing nothing rather than holding
       * the last number, because a battery reading nobody is updating is
       * worse than no battery reading. */
      b->have = false;
      b->filteredMv = 0;
    }
    return b->have;
  }

  b->strikes = 0;
  if (!b->have) {
    b->filteredMv = (int32_t)milliVolts;
    b->have = true;
  } else {
    b->filteredMv += ((int32_t)milliVolts - b->filteredMv) / BATTERY_SMOOTHING;
  }
  return true;
}

uint16_t batteryMilliVolts(const Battery *b) {
  if (b == NULL || !b->have) {
    return 0;
  }
  return (uint16_t)b->filteredMv;
}

uint8_t batteryPercent(const Battery *b) {
  if (b == NULL || !b->have) {
    return 0;
  }
  int32_t mv = b->filteredMv;
  if (mv <= BATTERY_EMPTY_MV) {
    return 0;
  }
  if (mv >= BATTERY_FULL_MV) {
    return 100;
  }
  return (uint8_t)(((mv - BATTERY_EMPTY_MV) * 100) /
                   (BATTERY_FULL_MV - BATTERY_EMPTY_MV));
}

bool batteryFormat(const Battery *b, BatteryShow show, char *out,
                   size_t outLen) {
  if (out == NULL || outLen < BATTERY_TEXT_LEN) {
    return false;
  }
  out[0] = '\0';
  if (b == NULL || !b->have || show == BATTERY_SHOW_OFF ||
      show >= BATTERY_SHOW_COUNT) {
    return false;
  }
  if (show == BATTERY_SHOW_PERCENT) {
    snprintf(out, outLen, txt(STR_COMMON_FMT_PERCENT),
             (unsigned)batteryPercent(b));
  } else {
    const uint16_t mv = batteryMilliVolts(b);
    /* One decimal, rounded: the battery is read once at start up, so a
     * second decimal never moves while the radio is on. No
     * unit: the header draws a small upright battery just before it. */
    const unsigned tenths = (unsigned)((mv + 50) / 100);
    snprintf(out, outLen, txt(STR_COMMON_FMT_VOLTS_TENTHS), tenths / 10,
             tenths % 10);
  }
  return true;
}
