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
 * Open the offer of the newer release the update check found: a box with the
 * versions and the size, and the buttons Update and Later. The loop calls it
 * when the check found one and the radio screen is up on its own. False when
 * there is nothing to offer or no screen for it.
 */
bool menuTaskOpenUpdateOffer(void);

/*
 * A tap on a row of the list or picker on show, `by` rows from the cursor:
 * turned to and pressed, the knob's own calls.
 */
void menuTaskTapRow(int32_t by);

/*
 * The value being changed moved to `value`, from a finger on its bar: in
 * the bar's own units, an index on a row that walks a list, and turned there
 * in whole steps as the knob turns it, applied as it moves.
 */
void menuTaskBarTo(int32_t value);

/*
 * A page of the list or picker on show, for a swipe: the window and the
 * cursor move a whole window on, `dir` 1, or back, -1, stopping at the ends.
 * Nothing on a value with a bar, the PIN, the Restart question or a dialog.
 */
void menuTaskPage(int dir);

/*
 * Where the menu is: its level, group and sub-group, or a dialog, as one
 * number under 2^13 that changes whenever the screen under a finger does. 0
 * while it is shut.
 */
uint32_t menuTaskPlace(void);

/*
 * Open the menu straight on Audio > Squelch > Squelch Mode with its edit
 * started, for the SQL tile of the radio screen. Keeping the value or going
 * back shuts the menu again, back on the radio screen. False when it could
 * not open, the menu or another screen being up.
 */
bool menuTaskOpenSquelchMode(void);

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
