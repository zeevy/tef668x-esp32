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

/*
 * Read the settings out of NVS.
 *
 * A radio that has never been written to, or whose blob is corrupt or was
 * written by a newer firmware, comes back with defaults rather than an error,
 * because there is nothing useful the caller could do about it.
 */
bool settingsNvsLoad(Settings *out);

bool settingsNvsSave(const Settings *s);

bool settingsNvsClear(void);

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
