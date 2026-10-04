/*
 * The RDS screen's view: what it says, built from the radio's snapshot.
 *
 * A part of `screen_task.cpp` kept in its own file. Which screen owns the
 * panel, and when the RDS screen opens, closes or turns a page, stays there;
 * this file only fills the page in and keeps the two timers the page shows,
 * which nothing else needs. Glue like the task files, not a layer of its own.
 */
#ifndef SCREEN_TASK_RDS_H
#define SCREEN_TASK_RDS_H

#include <stdint.h>

/* Fill the RDS screen in from the radio, on page `page`, 0 to
 * SCREEN_RDS_PAGES - 1, and draw it. Does nothing when the snapshot cannot
 * be read. */
void screenTaskRdsDraw(uint8_t page);

/* Start the lock timer again, for a screen that has just been opened: a
 * station looked at, left, and looked at again should not report a lock
 * held since the first visit. */
void screenTaskRdsRestart(void);

#endif /* SCREEN_TASK_RDS_H */
