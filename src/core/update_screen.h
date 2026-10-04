/*
 * Who owns the panel while firmware is being written, and for how long.
 *
 * All of the deciding and none of the drawing, for the same reason as
 * `core/wifi_join.h`: it is a hold with a deadline and a redraw that has to
 * be skipped most of the time, and both fail quietly. A hold that is never
 * released leaves the radio showing Update Failed for ever. A redraw that
 * fires on every chunk repaints the panel hundreds of times on the same task
 * that is receiving the image. Neither shows up in a build and both can be
 * tested on a PC.
 *
 * The caller owns the screen. It asks whether to hold, asks whether this
 * percentage is worth drawing, and draws.
 */
#ifndef CORE_UPDATE_SCREEN_H
#define CORE_UPDATE_SCREEN_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Everything the hold needs to remember between calls. */
typedef struct {
  bool held;          /* The write owns the panel. */
  bool forever;       /* Until the reboot, so there is no deadline. */
  uint32_t releaseMs; /* When to give it back, if not forever. */
  int percentShown;   /* The last whole number drawn, or -1 for none. */
} UpdateScreen;

/* Nothing held, nothing shown. */
void updateScreenReset(UpdateScreen *u);

/*
 * A write has started. Take the panel and hold it with no deadline.
 *
 * No deadline because a write that finishes ends in a reboot, and one that
 * fails says so through `updateScreenFinished`, which is what sets one.
 */
void updateScreenBegin(UpdateScreen *u);

/*
 * Whether this percentage is worth putting on the panel.
 *
 * True at most once per whole number, and never unless a write is running.
 * `shown` is filled in with the number to draw, clamped to 0 to 100, so a
 * caller that trusts a byte count cannot print 103%.
 */
bool updateScreenProgress(UpdateScreen *u, int percent, int *shown);

/*
 * The write is over.
 *
 * `ok` true holds the panel with no deadline: the reboot is what takes the
 * screen away, and giving the radio back for the half second before it would
 * be a flicker of a station that is about to disappear.
 *
 * `ok` false sets a deadline `holdMs` from now, because the radio carries on
 * running the image it already had and the panel has to come back.
 */
void updateScreenFinished(UpdateScreen *u, bool ok, uint32_t nowMs,
                          uint32_t holdMs);

/*
 * Whether the panel still belongs to the write, this millisecond.
 *
 * Call it from the poll. It releases the hold itself once the deadline has
 * passed, so the caller has one question to ask and not two.
 */
bool updateScreenHolds(UpdateScreen *u, uint32_t nowMs);

/*
 * A key, knob or touch while the panel may be held.
 *
 * True when the write owns the panel, and then the press does nothing else:
 * acting on it would change the radio behind a screen that hides it. A
 * failed write's message is given up at the next poll, so a person who has
 * read it does not have to wait out the rest of its time. A write that is
 * running, or one that worked and is waiting for the reboot, keeps the panel.
 */
bool updateScreenPress(UpdateScreen *u, uint32_t nowMs);

#ifdef __cplusplus
}
#endif

#endif /* CORE_UPDATE_SCREEN_H */
