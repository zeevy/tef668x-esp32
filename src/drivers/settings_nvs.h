/*
 * Reads and writes the settings struct in NVS.
 *
 * This lives in drivers/ and not in core/ because it includes Preferences.h.
 * The rule that core/ builds on a PC with no hardware is what makes the
 * settings struct, its defaults and its migration testable, so the one file
 * that needs NVS sits on this side of the line.
 */
#ifndef DRIVERS_SETTINGS_NVS_H
#define DRIVERS_SETTINGS_NVS_H

#include "core/settings.h"
#include "core/touch_cal.h"

/*
 * Read the settings out of NVS.
 *
 * A radio that has never been written to, or whose blob is corrupt or was
 * written by a newer firmware, comes back with defaults rather than an error,
 * because there is nothing useful the caller could do about it.
 */
bool settingsNvsLoad(Settings *out);

/*
 * Whether anything has ever been stored.
 *
 * `settingsNvsLoad` answers false for two different radios: one that has
 * never been written to, which is every radio on its first boot after a
 * flash, and one whose stored blob cannot be read, which has lost the PIN,
 * the station and the calibration. The first is healthy and the second is
 * not, and the boot screen puts a mark on the glass for it, so it has to be
 * able to tell them apart.
 */
bool settingsNvsStored(void);

bool settingsNvsSave(const Settings *s);

/*
 * The touch calibration a person made, kept apart from the settings. Erase
 * is what Erase Settings does to it too.
 */
bool touchCalNvsSave(const TouchCal *cal);
bool touchCalNvsErase(void);

/* The calibration to use on a screen `width` by `height`: the one kept, or
 * else the board's own when none is kept or the one kept does not fit this
 * screen. `stored` says which. False when neither makes one. */
bool touchCalNvsLoadOrBoard(int16_t width, int16_t height, TouchCal *out,
                            bool *stored);

/*
 * Whether the stored settings were read back at start up.
 *
 * Defined by the application rather than by this driver, because the driver
 * does not know when start up happened. It is declared here so the banner and
 * the web page can say so without reaching into main.
 *
 * False means the blob was missing or was refused, so the radio is running on
 * the defaults: the access PIN is 000000 again and the stored station, band
 * plan and knob calibration are gone. Without saying this anywhere, the only
 * symptom is that everything went back to how it was.
 */
bool settingsWereLoaded(void);

#endif /* DRIVERS_SETTINGS_NVS_H */
