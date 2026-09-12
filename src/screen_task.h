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
 */
#ifndef SCREEN_TASK_H
#define SCREEN_TASK_H

#include <stdbool.h>

/**
 * Start the panel and show the boot message.
 *
 * @return false when the panel did not come up. The radio carries on, because
 *         a radio with a dead screen is still a radio and is still reachable
 *         over Wi-Fi.
 */
bool screenTaskBegin(void);

/**
 * Redraw whatever changed.
 *
 * Call this from loop(). It reads a snapshot and compares against what is
 * already on the panel, so calling it often costs nothing when nothing moved.
 */
void screenTaskPoll(void);

#endif /* SCREEN_TASK_H */
