/*
 * The battery read again with Wi-Fi paused, while nobody is using the radio.
 *
 * Gathers what core/battery_check.c needs to decide, and when it says so,
 * stops Wi-Fi, reads the battery and starts Wi-Fi again, on the loop task.
 */
#ifndef BATTERY_CHECK_TASK_H
#define BATTERY_CHECK_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/settings.h"

/* Call once, after the reading at start up. */
void batteryCheckTaskBegin(const Settings *settings);

/* Call on every pass of the loop. `busy` answers whether something a pause
 * would spoil is open or running: the menu, DX mode, the band scope, a scan
 * or a sweep; it is asked only when a reading is otherwise due. Costs a
 * comparison or two on most passes; a reading holds the loop for the stop
 * of Wi-Fi, the read and its start again. */
void batteryCheckTaskPoll(bool (*busy)(void));

/* The pauses so far, how long the last one held the loop, and how long Wi-Fi
 * took to be back on the network it was on before it, in ms; 0 while there
 * is none to report, and while Wi-Fi is not back from the last pause. */
uint16_t batteryCheckTaskCount(void);
uint32_t batteryCheckTaskPauseMs(void);
uint32_t batteryCheckTaskBackMs(void);

#endif /* BATTERY_CHECK_TASK_H */
