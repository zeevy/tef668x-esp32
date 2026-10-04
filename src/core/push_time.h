/*
 * How the pushes to the panel added up over the last whole second.
 *
 * Every rectangle LVGL finishes goes to the panel over SPI, and the task that
 * runs the screen and the web server waits for each one. This counts them in
 * windows of one second: how many there were, how many pixels they held, how
 * much of the second went on them and how long the longest took, and how long
 * the longest refresh took, LVGL drawing the dirty parts and pushing them,
 * which for a whole screen drawn again is the time it takes. Nothing here
 * reads a clock; the caller passes the times in.
 */
#ifndef CORE_PUSH_TIME_H
#define CORE_PUSH_TIME_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The window length, in milliseconds. */
#define PUSH_TIME_WINDOW_MS 1000u

/* One window's totals. */
typedef struct {
  uint32_t pushes;
  uint32_t pixels;
  uint32_t busyUs;
  uint32_t longestUs;
  uint32_t longestRefreshUs;
} PushSecond;

typedef struct {
  uint32_t startMs;    /* When the window being counted began. */
  PushSecond counting; /* The window being counted. */
  PushSecond last;     /* The last whole window. */
  bool haveLast;       /* A whole window has ended since the reset. */
} PushTime;

/* Start counting from `nowMs`, with no whole window yet. */
void pushTimeReset(PushTime *p, uint32_t nowMs);

/* One push of `pixels` pixels that took `us` microseconds and ended at
 * `nowMs`. It counts in the window `nowMs` falls in. */
void pushTimeAdd(PushTime *p, uint32_t nowMs, uint32_t pixels, uint32_t us);

/* One refresh that took `us` microseconds and ended at `nowMs`, counted the
 * same way. */
void pushTimeRefresh(PushTime *p, uint32_t nowMs, uint32_t us);

/*
 * The totals of the last whole window before `nowMs`.
 *
 * A window with no push in it reads as zeros, since that is what happened,
 * and so does the last window when more than one has passed without a call.
 * Returns false, and leaves `out` alone, before the first window has ended
 * and for a NULL: there is then nothing true to give.
 */
bool pushTimeLast(PushTime *p, uint32_t nowMs, PushSecond *out);

#ifdef __cplusplus
}
#endif

#endif /* CORE_PUSH_TIME_H */
