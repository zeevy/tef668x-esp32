#include "radio_round.h"

#include <stddef.h>

/* What is left until `deadline`, 0 once it has come. */
static uint32_t until(uint32_t now, uint32_t deadline) {
  return (int32_t)(deadline - now) > 0 ? deadline - now : 0;
}

static uint32_t sooner(uint32_t a, uint32_t b) {
  return a < b ? a : b;
}

uint32_t radioRoundWait(const RadioRoundTimes *t) {
  if (t == NULL || t->seeking) {
    return 0;
  }
  uint32_t wait = until(t->now, t->nextPoll);
  if (t->stepping) {
    wait = sooner(wait, t->step);
  }
  if (t->afWaiting) {
    wait = sooner(wait, t->afIn);
  }
  if (t->rds) {
    wait = sooner(wait, until(t->now, t->nextRds));
  }
  return wait;
}

bool radioRoundDue(uint32_t now, uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}

uint32_t radioRoundNext(uint32_t deadline, uint32_t period, uint32_t now) {
  const uint32_t next = deadline + period;
  return radioRoundDue(now, next) ? now + period : next;
}
