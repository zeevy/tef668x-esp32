/**
 * @file settings_nvs.h
 * @brief Reads and writes the settings struct in NVS.
 *
 * This lives in drivers/ and not in core/ because it includes Preferences.h.
 * The rule that core/ builds on a PC with no hardware is what makes the
 * settings struct, its defaults and its migration testable, so the one file
 * that needs NVS sits on this side of the line.
 */
#ifndef DRIVERS_SETTINGS_NVS_H
#define DRIVERS_SETTINGS_NVS_H

#include "core/settings.h"

/**
 * Read the settings out of NVS.
 *
 * A radio that has never been written to, or whose blob is corrupt or was
 * written by a newer firmware, comes back with defaults rather than an error,
 * because there is nothing useful the caller could do about it.
 *
 * @param out  Receives the settings. Always left in a valid state.
 * @return true when a stored blob was read, false when defaults were used.
 */
bool settingsNvsLoad(Settings *out);

/**
 * Write the settings to NVS.
 *
 * @param s  The settings to store. Must pass settingsValid.
 * @return true when the write went through.
 */
bool settingsNvsSave(const Settings *s);

/**
 * Throw away the stored settings, so the next load returns defaults.
 *
 * @return true when the key was cleared.
 */
bool settingsNvsClear(void);

#endif /* DRIVERS_SETTINGS_NVS_H */
