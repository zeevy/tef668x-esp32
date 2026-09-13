/**
 * @file autosave.c
 * @brief Implementation of when to write the settings down.
 */
#include "autosave.h"

#include <stddef.h>

void autoSaveInit(AutoSave *a, uint32_t idleMs, uint32_t nowMs) {
  if (a == NULL) {
    return;
  }
  a->idleMs = idleMs;
  a->lastChangeMs = nowMs;
  a->started = true;
}

bool autoSaveDue(AutoSave *a, bool differs, bool moved, bool busy,
                 uint32_t nowMs) {
  if (a == NULL || a->idleMs == 0) {
    return false;
  }
  if (!a->started) {
    /* Never set up. Start the clock from here rather than from zero, or a
     * radio whose first tick arrives minutes after boot saves at once. */
    a->lastChangeMs = nowMs;
    a->started = true;
  }
  if (moved) {
    a->lastChangeMs = nowMs;
  }
  if (!differs) {
    return false;
  }
  /* Not during a seek. The dial moves every fifty milliseconds while one
   * runs, so `moved` would hold the clock open anyway, but the dial also
   * stops moving between channels while the reading settles and that is long
   * enough to slip through. */
  if (busy) {
    return false;
  }
  return (uint32_t)(nowMs - a->lastChangeMs) >= a->idleMs;
}

void autoSaveDone(AutoSave *a, uint32_t nowMs) {
  if (a != NULL) {
    a->lastChangeMs = nowMs;
    a->started = true;
  }
}

uint32_t autoSaveWaitMs(const AutoSave *a, bool differs, uint32_t nowMs) {
  if (a == NULL || a->idleMs == 0 || !differs || !a->started) {
    return 0;
  }
  uint32_t gone = nowMs - a->lastChangeMs;
  return gone >= a->idleMs ? 0 : a->idleMs - gone;
}
