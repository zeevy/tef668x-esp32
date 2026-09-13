/**
 * @file screen_task.h
 * @brief Fills the screen in from the radio's state.
 *
 * Another composition root, like input_task.h and for the same reason. The
 * screen in `ui/` takes plain strings and knows nothing about tuners, tasks or
 * snapshots. This is the one place that knows both.
 *
 * It runs on the loop task. Drawing is SPI, so it never touches the tuner and
 * never holds the radio task up.
 *
 * The panel light is here too. It is a property of the screen rather than of
 * anything the radio receives, and the logic that decides how bright it
 * should be is in core/backlight.h where it can be tested.
 */
#ifndef SCREEN_TASK_H
#define SCREEN_TASK_H

#include <stdbool.h>

#include "core/backlight.h"

/**
 * Start the panel, show the boot message and bring the light up.
 *
 * The fade runs inside this call rather than from the poll. Start up carries
 * on for seconds after this returns, through the tuner patch and the Wi-Fi
 * join, and a fade driven from the loop would leave the panel dark for all of
 * it.
 *
 * @param cfg  How bright the panel should be and when it dims. NULL takes the
 *             defaults.
 * @return false when the panel did not come up. The radio carries on, because
 *         a radio with a dead screen is still a radio and is still reachable
 *         over Wi-Fi.
 */
bool screenTaskBegin(const BacklightConfig *cfg);

/**
 * Redraw whatever changed, and move the panel light along.
 *
 * Call this from loop(). It reads a snapshot and compares against what is
 * already on the panel, so calling it often costs nothing when nothing moved.
 */
void screenTaskPoll(void);

/**
 * Change how bright the panel is and when it dims, with effect now.
 *
 * @param cfg  The new settings. NULL takes the defaults.
 */
void screenTaskSetBacklight(const BacklightConfig *cfg);

/**
 * What the panel light is doing, for the web page and the checklist.
 *
 * @param percent  Receives the brightness, 0 to 100. May be NULL.
 * @return true when the radio has been left alone long enough to have dimmed.
 */
bool screenTaskBacklightState(uint8_t *percent);

/**
 * The signal number the panel is actually showing, in whole dBuV.
 *
 * Not the same as the smoothed level in the snapshot. The screen holds its
 * number until the level moves a whole dB from it, so the two differ most of
 * the time, and the held one is what a person is reading.
 *
 * Published because otherwise the only way to check it is to stand at the
 * radio and look, and the checks that matter here are about a number sitting
 * still over half a minute.
 *
 * @return The level on the screen. 0 before the first reading.
 */
int16_t screenTaskSignalShown(void);

#endif /* SCREEN_TASK_H */
