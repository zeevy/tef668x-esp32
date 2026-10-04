/* Implementation of the panel push totals. */
#include "push_time.h"

#include <stddef.h>
#include <string.h>

void pushTimeReset(PushTime *p, uint32_t nowMs) {
  if (p == NULL) {
    return;
  }
  memset(p, 0, sizeof(*p));
  p->startMs = nowMs;
}

/* Close every window that has ended by `nowMs`. The start moves on by whole
 * windows, so each one is exactly a second long however late the call. */
static void roll(PushTime *p, uint32_t nowMs) {
  const uint32_t gone = nowMs - p->startMs;
  if (gone < PUSH_TIME_WINDOW_MS) {
    return;
  }
  if (gone < 2u * PUSH_TIME_WINDOW_MS) {
    p->last = p->counting;
  } else {
    /* The window that ended last had no push in it. */
    memset(&p->last, 0, sizeof(p->last));
  }
  memset(&p->counting, 0, sizeof(p->counting));
  p->haveLast = true;
  p->startMs += (gone / PUSH_TIME_WINDOW_MS) * PUSH_TIME_WINDOW_MS;
}

void pushTimeAdd(PushTime *p, uint32_t nowMs, uint32_t pixels, uint32_t us) {
  if (p == NULL) {
    return;
  }
  roll(p, nowMs);
  p->counting.pushes++;
  p->counting.pixels += pixels;
  p->counting.busyUs += us;
  if (us > p->counting.longestUs) {
    p->counting.longestUs = us;
  }
}

void pushTimeRefresh(PushTime *p, uint32_t nowMs, uint32_t us) {
  if (p == NULL) {
    return;
  }
  roll(p, nowMs);
  if (us > p->counting.longestRefreshUs) {
    p->counting.longestRefreshUs = us;
  }
}

bool pushTimeLast(PushTime *p, uint32_t nowMs, PushSecond *out) {
  if (p == NULL || out == NULL) {
    return false;
  }
  roll(p, nowMs);
  if (!p->haveLast) {
    return false;
  }
  *out = p->last;
  return true;
}
