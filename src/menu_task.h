/*
 * The menu, put together: the cursor, the table of what is in it, and the
 * screen.
 *
 * Another composition root, like input_task.h and screen_task.h. `core/menu.c`
 * decides where the cursor goes and knows nothing about radios; `ui/` draws
 * rows and knows nothing about settings; this is the one place that knows
 * both, plus what every row is called and what it is allowed to be.
 *
 * **Every row writes through `settingsApplyLive`**, the same call the HTTP
 * settings endpoint uses. That is the rule the whole file exists to keep: two
 * ways into a setting that apply it differently is how a radio ends up
 * behaving one way from the panel and another from a browser.
 *
 * Flash is written once per accepted edit, not once per click of the knob. A
 * brightness swept from 5 to 100 in steps of five is twenty values on the
 * panel and one write to NVS.
 */
#ifndef MENU_TASK_H
#define MENU_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/band_plan.h"
#include "core/settings.h"

/* Give it the live settings. They are read for every row and written back
 * when an edit is accepted. */
void menuTaskBegin(Settings *live);

/* Whether the menu owns the panel. The input task asks before it does
 * anything with the knob, because while this is true the knob belongs to the
 * menu and not to the dial. */
bool menuTaskIsOpen(void);

/* Open it. A press of the knob or a hold of MODE. */
void menuTaskOpen(void);

/*
 * Open the choice of bands for a typed number: the band in use does not hold
 * it, and more than one other does. `typed` is the digits as keyed, for the
 * title, and `readings` the bands with their frequencies, lowest first. It
 * borrows the menu's screen and controls: a turn moves, a press or ENTER
 * tunes and shuts it, and back shuts it. False when it could not open, the
 * menu or another screen being up, or fewer than two readings.
 */
bool menuTaskOpenChoice(const char *typed, const BandTypedReading *readings,
                        uint8_t count);

/*
 * Close it from outside, for anything that needs the panel.
 *
 * An edit in flight is cancelled and the old value put back, because it was
 * never accepted. Safe to call when the menu is already shut.
 */
void menuTaskClose(void);

/* The knob, in clicks rather than accelerated steps. One click is one row. */
void menuTaskTurn(int32_t clicks);

/* A short press of the knob: open the group, start the edit, keep the value. */
void menuTaskPress(void);

/* A long press of the knob: cancel, back, close. Always means "not this". */
void menuTaskBack(void);

/*
 * A digit key, 0 to 9. Taken only while the Web PIN is being set, where it
 * sets the digit being set and moves on, the sixth one saving the PIN as a
 * press would. False when the menu has no use for it, which is everywhere
 * else.
 */
bool menuTaskDigit(uint8_t digit);

/*
 * Called from loop(). Takes the CPU sample once a second, and redraws the
 * rows whose values change on their own rather than only when the knob does:
 * a running scan and the Squelch rows four times a second, and Diagnostics,
 * Network Info, Audio and the AGC edit once a second. Draws nothing the rest
 * of the time.
 */
void menuTaskPoll(void);

#endif /* MENU_TASK_H */
