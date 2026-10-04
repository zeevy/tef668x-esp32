/* Who owns the panel during a firmware write. No drawing, no waiting. */
#include "update_screen.h"

#include <stddef.h>

void updateScreenReset(UpdateScreen *u) {
  if (u == NULL) {
    return;
  }
  u->held = false;
  u->forever = false;
  u->releaseMs = 0;
  u->percentShown = -1;
}

void updateScreenBegin(UpdateScreen *u) {
  if (u == NULL) {
    return;
  }
  u->held = true;
  u->forever = true;
  u->releaseMs = 0;
  u->percentShown = -1;
}

bool updateScreenProgress(UpdateScreen *u, int percent, int *shown) {
  if (u == NULL || !u->held) {
    return false;
  }
  /*
   * A negative percentage means the size was not known. Nothing is drawn for
   * it, so the panel keeps its progress screen with no number rather than
   * putting up a figure that was invented.
   */
  if (percent < 0) {
    return false;
  }
  if (percent > 100) {
    percent = 100;
  }
  if (percent == u->percentShown) {
    return false;
  }
  u->percentShown = percent;
  if (shown != NULL) {
    *shown = percent;
  }
  return true;
}

void updateScreenFinished(UpdateScreen *u, bool ok, uint32_t nowMs,
                          uint32_t holdMs) {
  if (u == NULL) {
    return;
  }
  u->held = true;
  if (ok) {
    u->forever = true;
    u->releaseMs = 0;
    return;
  }
  u->forever = false;
  u->releaseMs = nowMs + holdMs;
}

bool updateScreenHolds(UpdateScreen *u, uint32_t nowMs) {
  if (u == NULL || !u->held) {
    return false;
  }
  if (u->forever) {
    return true;
  }
  /* Signed subtraction, so the wrap of millis every forty nine days is one
   * more pass round rather than a panel held for another forty nine days. */
  if ((int32_t)(nowMs - u->releaseMs) < 0) {
    return true;
  }
  updateScreenReset(u);
  return false;
}

bool updateScreenPress(UpdateScreen *u, uint32_t nowMs) {
  if (!updateScreenHolds(u, nowMs)) {
    return false;
  }
  if (!u->forever) {
    u->releaseMs = nowMs;
  }
  return true;
}
