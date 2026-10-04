/* Implementation of the battery sense pin. */
#include "battery_adc.h"

#include <Arduino.h>

#include "board/board.h"
#include "core/battery.h"

#ifdef PIN_BATTERY_ADC

/*
 * How many samples one reading is made of.
 *
 * Eight, which is what the reference firmware uses on this board. The
 * converter moves by a few counts with nothing changing, and the cost of
 * eight reads is well under a millisecond on a task that is not the radio's.
 */
#define BATTERY_SAMPLES 8

/*
 * What the divider does, as a numerator and a denominator.
 *
 * A half divider, so the pin sees half the cell and the reading doubles. That
 * is what the reference firmware assumes on this board and it has not been
 * measured off this unit with a meter. It is checked rather than trusted:
 * core/battery.c refuses anything outside what a single lithium cell can be,
 * so a divider that is not this ratio gives no reading at all instead of a
 * confident wrong one. If the panel says nothing with a known good battery
 * fitted, this ratio is the first thing to doubt.
 */
#define BATTERY_DIVIDER_TOP 2
#define BATTERY_DIVIDER_BOTTOM 1

/*
 * Below this a sample is the converter saying no rather than a voltage.
 *
 * The pin sees half the cell, so even a deeply flat one puts about 1.4 V
 * here. A hundred millivolts is nothing any battery produces.
 */
#define BATTERY_SAMPLE_FLOOR_MV 100

/*
 * How long the pin needs after it is made an ADC input before it reads right.
 *
 * Measured on this radio over a power on boot, reading the pin every 5 ms.
 * The pin climbs to its final 2012 mV as a clean exponential:
 *
 *   +0 ms   771 mV   61.7% of the way still to go
 *   +5 ms  1577 mV   21.6%
 *   +10 ms 1870 mV    7.1%
 *   +15 ms 1964 mV    2.4%
 *   +20 ms 1996 mV    0.8%
 *   +25 ms 2007 mV    0.25%
 *
 * What is left of the gap falls by a factor of 0.334 every 5 ms on every
 * step, so this is a plain RC with a time constant of 4.6 ms. There is a
 * capacitor on the divider.
 *
 * The charging starts when the pin is made an ADC input, not at power on.
 * Fifty milliseconds is eleven time constants, which leaves under two
 * thousandths of one per cent of the gap, far below what the converter can
 * see.
 *
 * **This only bites on a power on reset.** A software reset leaves the pin
 * configured and the capacitor charged from the run before, so the first read
 * is already right. An over the air update ends in a software reset, so a
 * reading taken after one proves nothing about a cold start.
 */
#define BATTERY_SETTLE_MS 50

static bool sReady = false;
static uint16_t sBootMv = 0;
static bool sBootHave = false;
/* When the pin was primed, so the settle can be waited out only as far as it
 * still has to run. */
static uint32_t sPrimedMs = 0;

void batteryAdcBegin(void) {
  /* No pinMode. The pin is input only and the first read sets up what it
   * needs, which is the same thing analog.cpp does for the squelch pot. */
  analogReadResolution(12);
  /*
   * One read, thrown away, purely to make the pin an ADC input and start the
   * node charging. This does not wait for it. Call this as early in start up
   * as it can go and the settling happens under work that was going to happen
   * anyway, which is why it is split from the reading.
   */
  (void)analogReadMilliVolts(PIN_BATTERY_ADC);
  sPrimedMs = millis();
  sReady = true;
}

/*
 * Hold until the pin has settled, if it has not already.
 *
 * Costs nothing after the first fifty milliseconds of a boot, which is every
 * call except possibly the first. Without it the first read comes back at
 * about 38 per cent of the voltage, which on a full cell is 1.5 V, and
 * core/battery.c throws that away as impossible for a connected cell. The
 * panel then shows no battery at all for the whole session. It is here rather
 * than at the one call site that needs it, so that a reading taken early from
 * anywhere is right.
 */
static void waitForSettle(void) {
  const uint32_t since = millis() - sPrimedMs;
  if (since < BATTERY_SETTLE_MS) {
    delay(BATTERY_SETTLE_MS - since);
  }
}

bool batteryAdcFitted(void) {
  return true;
}

void batteryAdcSampleAtBoot(void) {
  uint16_t mv = 0;
  if (batteryAdcRead(&mv)) {
    sBootMv = mv;
    sBootHave = true;
  }
}

bool batteryAdcAtBoot(uint16_t *milliVolts) {
  if (!sBootHave || milliVolts == NULL) {
    return false;
  }
  *milliVolts = sBootMv;
  return true;
}

bool batteryAdcRead(uint16_t *milliVolts) {
  if (!sReady || milliVolts == NULL) {
    return false;
  }
  waitForSettle();
  uint32_t sum = 0;
  for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
    const uint32_t mv = analogReadMilliVolts(PIN_BATTERY_ADC);
    /*
     * Every sample has to be sane, not the average of them.
     *
     * While the converter is changing hands some samples come back as zero,
     * and averaging those in gives a voltage that is low and looks real,
     * which is how a full battery ends up showing a warning.
     */
    if (mv < BATTERY_SAMPLE_FLOOR_MV) {
      return false;
    }
    sum += mv;
  }
  const uint32_t at_pin = sum / BATTERY_SAMPLES;
  *milliVolts =
      (uint16_t)((at_pin * BATTERY_DIVIDER_TOP) / BATTERY_DIVIDER_BOTTOM);
  return true;
}

#else /* no battery sense pin on this board */

void batteryAdcBegin(void) {}
void batteryAdcSampleAtBoot(void) {}
bool batteryAdcAtBoot(uint16_t *milliVolts) {
  (void)milliVolts;
  return false;
}
bool batteryAdcFitted(void) {
  return false;
}
bool batteryAdcRead(uint16_t *milliVolts) {
  (void)milliVolts;
  return false;
}

#endif
