/*
 * When the radio task's next round comes.
 *
 * The task sleeps on its command queue, so a command is taken the moment it
 * arrives, for at most the time worked out here: until the next quality
 * reading, or sooner when something running wants the loop back first. A
 * fade, a ramp down, a tone or a wake wants it back within one fade step, or
 * it arrives in as many steps as there are polls. A seek wants no wait at
 * all, since it judges one channel a round and waiting out the poll between
 * channels makes a pass of the FM band take a minute instead of ten seconds.
 * An AF check and the RDS read each want it back by the time they are due.
 *
 * Plain numbers in RTOS ticks, which wrap, so every comparison is of a
 * difference; the two deadline calls take milliseconds as well. Nothing here
 * reads a clock; the caller passes the times in.
 */
#ifndef CORE_RADIO_ROUND_H
#define CORE_RADIO_ROUND_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* What the round has running, in ticks. */
typedef struct {
  uint32_t now;
  uint32_t nextPoll; /* The next quality reading is due. */
  bool seeking;
  bool stepping; /* A fade, a ramp, a tone or a wake is running. */
  uint32_t step; /* One fade step. */
  bool afWaiting;
  uint32_t afIn; /* The AF check is due in this many, 0 when now or late. */
  bool rds;      /* The RDS decoder is being read. */
  uint32_t nextRds;
} RadioRoundTimes;

/* How long the round may sleep on the queue. */
uint32_t radioRoundWait(const RadioRoundTimes *t);

/* Whether `deadline` has come by `now`. */
bool radioRoundDue(uint32_t now, uint32_t deadline);

/*
 * The deadline after `deadline`, one `period` on. A slow round can leave that
 * already past by `now`, and then it is a period from `now`, so a late round
 * does not fire several in a row to catch up.
 */
uint32_t radioRoundNext(uint32_t deadline, uint32_t period, uint32_t now);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RADIO_ROUND_H */
