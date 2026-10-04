/*
 * Auto off and Sleep Now.
 *
 * What counts as using the radio, the sleep mark and its last five minutes,
 * the fade in the last thirty seconds and the sleep at the end. The timing
 * itself is core/auto_off.c. Loop task only, where the input, the menu and
 * the web server run.
 */
#ifndef SLEEP_TASK_H
#define SLEEP_TASK_H

#include <stdbool.h>
#include <stdint.h>

/* From the settings: 0 for off, or one `autoOffMinutesOk` takes. Choosing a
 * time starts the count again. */
void sleepTaskSetMinutes(uint16_t minutes);

/* A write from the browser or the API, which is using the radio as much as a
 * key is. Reads are not, so a page left open does not keep it awake. */
void sleepTaskUsed(void);

void sleepTaskPoll(void);

/*
 * Go to sleep now, a moment from now so a reply to the browser can go out
 * first. False, and nothing happens, while an update is being written or is
 * on trial: the wake is a restart, and a restart then rolls the update back.
 */
bool sleepTaskNow(void);

/* Seconds until auto off, into `out`. False when it is off. */
bool sleepTaskLeftS(uint32_t *out);

#endif /* SLEEP_TASK_H */
