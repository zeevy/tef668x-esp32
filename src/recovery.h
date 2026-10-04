/*
 * The one screen with no theme. The setting that broke the panel may be the
 * theme itself, so this screen never draws by the saved one.
 *
 * Checked once, right after the settings load in `setup` and before
 * anything else claims the panel, the tuner or the network: holding the
 * rotary button at power on takes the radio to a plain list instead of the
 * one it would otherwise start. Nothing here builds or tests on a PC; it
 * is a raw pin read at the one moment it can decide which of two ways
 * `setup` continues, the same reason `main.cpp` itself is not tested.
 */
#ifndef RECOVERY_H
#define RECOVERY_H

#include "core/settings.h"

/*
 * Read the knob, and take the radio to recovery if it is held.
 *
 * Returns having done nothing else when it is not: `setup` carries on
 * exactly as it always has. Never returns when it is: recovery is a closed
 * loop of its own, ending in `ESP.restart()` on every way out, "Exit and
 * start radio" included, so the normal sequence this call sits inside is
 * never resumed half run.
 */
void recoveryCheckAndRun(Settings *settings);

#endif /* RECOVERY_H */
