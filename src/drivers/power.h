/*
 * The ESP32's deep sleep, and waking from it with a press of the knob.
 *
 * Not a power switch. Deep sleep keeps the chip's RTC part running so it can
 * watch the knob, and everything else on the board that has no switch of its
 * own stays powered, so a small current still flows. A wake is a full start
 * up, the same as a restart, on whatever was saved before sleeping.
 */
#ifndef DRIVERS_POWER_H
#define DRIVERS_POWER_H

#include <stdbool.h>

/*
 * Let go of the backlight pin, which sleep held low. Before the display
 * claims it, or the panel never lights again until a power cycle.
 */
void powerBegin(void);

/* This start up is the knob waking the radio from sleep. */
bool powerWokeFromSleep(void);

/*
 * Turn the panel light off, hold it off, wait for the knob to be let go and
 * sleep until it is pressed. Never returns.
 *
 * Saving, the sound and the tuner are the caller's, before this.
 */
void powerDeepSleep(void);

#endif /* DRIVERS_POWER_H */
