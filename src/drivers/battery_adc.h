/*
 * The battery sense pin.
 *
 * A divider on GPIO 13, as traced on the board, which the reference
 * firmware reads the same way. This only reads the pin. What a
 * voltage means is core/battery.h, where it can be tested.
 *
 * **GPIO 13 is ADC2, and the Wi-Fi driver owns ADC2.** Wi-Fi is usually on, so
 * the converter is often unavailable, and the Arduino core's analogRead does
 * not say so: it ignores the error and returns 0. A zero from a busy converter
 * and a zero from a flat cell are the same number.
 *
 * That is why this returns a flag rather than a number, and why core/battery.c
 * throws away anything below a voltage a connected cell can reach. The three
 * ways of getting nothing, no battery fitted, the converter busy, and a
 * divider that is not what this assumes, cannot be told apart here and all
 * three mean the same thing on screen: nothing.
 */
#ifndef DRIVERS_BATTERY_ADC_H
#define DRIVERS_BATTERY_ADC_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Set the pin up. Safe to call before Wi-Fi has joined. Does not block.
 *
 * Call it as early in start up as it will go. Making the pin an ADC input is
 * what starts the divider node charging, and it needs 50 ms after that before
 * it reads the real voltage. This starts the clock and returns, so the
 * settling runs under whatever start up was doing anyway, and only
 * batteryAdcRead waits for what is left. The 50 ms is measured, not chosen:
 * see BATTERY_SETTLE_MS in battery_adc.cpp for the readings behind it.
 */
void batteryAdcBegin(void);

/*
 * Read the cell, in millivolts, averaged over a few samples.
 *
 * Returns false when the board has no battery sense pin, or when any sample
 * came back at or near zero, which is what a converter that could not be had
 * looks like. `out` is untouched in that case.
 *
 * Blocks for whatever is left of the 50 ms settle since batteryAdcBegin. In
 * practice that is nothing, because start up takes longer than 50 ms to reach
 * the first call. A read before the pin has settled comes back at about 38
 * per cent of the real voltage, which on a full cell core/battery.c refuses
 * as impossible, so the wait is not optional.
 */
bool batteryAdcRead(uint16_t *milliVolts);

/*
 * Whether this board has the pin at all.
 *
 * Comes from the board header, so a board without a battery never shows an
 * empty space where a reading would be.
 */
bool batteryAdcFitted(void);

/*
 * What the pin read once at start up, before Wi-Fi was brought up.
 *
 * The one reading on this board that the Wi-Fi driver cannot have interfered
 * with, because it is taken before there is a Wi-Fi driver. It exists to tell
 * two failures apart that look identical afterwards: a converter that was
 * busy, and a pin with nothing on it.
 *
 * Returns false if it was never taken or read as nothing. Not used for the
 * panel: it is one sample from one moment and a battery moves.
 */
bool batteryAdcAtBoot(uint16_t *milliVolts);

/* Take that reading. Call once, before Wi-Fi starts. */
void batteryAdcSampleAtBoot(void);

#endif /* DRIVERS_BATTERY_ADC_H */
