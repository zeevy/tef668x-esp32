/* Implementation of when to write the settings down. */
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
  /* Busy holds the clock as moving does. While a seek or a scan walks the
   * dial, what would be stored can hold still for longer than the wait: a
   * seek's start is kept, and a DX scan can dwell on one channel for thirty
   * seconds. The full wait then starts when it ends. */
  if (moved || busy) {
    a->lastChangeMs = nowMs;
  }
  if (!differs || busy) {
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
