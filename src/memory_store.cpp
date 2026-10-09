/* Implementation of the channel list owner. */
#include "memory_store.h"
#include "debug_log.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stdlib.h>
#include <string.h>

#include <atomic>

#include "drivers/memory_nvs.h"

/* The one copy of the list. */
static MemoryStore sStore;

/* Taken by every call here, so the two cores never see it half changed. */
static SemaphoreHandle_t sLock = NULL;

/* A change is waiting to be written, and when it arrived. */
static bool sDirty = false;
static uint32_t sDirtyAtMs = 0;

/*
 * Goes up on every change, so a caller can tell the list has moved.
 *
 * Atomic, because it is changed under the lock and read without it, from
 * either core.
 */
static std::atomic<uint32_t> sGeneration{0};

static bool sLastFailed = false;

/* Channels the load threw away because they were not valid. Kept because it
 * is the only thing that tells a shorter list from a list that was always
 * this length. */
static int sCleared = 0;

/*
 * Take the lock.
 *
 * The wait has no limit. Every section below is arithmetic on the list with
 * no waiting in it, so nobody is kept out for as much as a millisecond, and a
 * limit would only add a way to fail that the caller could not tell from an
 * empty list.
 */
static bool lock(void) {
  return sLock != NULL && xSemaphoreTake(sLock, portMAX_DELAY) == pdTRUE;
}

static void unlock(void) {
  xSemaphoreGive(sLock);
}

static void changed(void) {
  sDirty = true;
  sDirtyAtMs = millis();
  sGeneration++;
}

bool memoryStoreBegin(void) {
  if (sLock == NULL) {
    sLock = xSemaphoreCreateMutex();
  }
  bool loaded = memoryNvsLoad(&sStore, &sCleared);
  sDirty = false;
  sGeneration++;
  return loaded;
}

static void writeNow(void) {
  /* The blob is made under the lock and written outside it, because the write
   * blocks for tens of milliseconds. It is taken from the heap for the one save
   * rather than kept as a static, since static RAM is the scarcer and a save is
   * rare, and it is the only copy. */
  const size_t size = memoryBlobSize();
  uint8_t *blob = (uint8_t *)malloc(size);
  if (blob != NULL && !lock()) {
    free(blob);
    return;
  }
  bool made = false;
  if (blob != NULL) {
    made = memoryToBlob(&sStore, blob, size) == size;
    sDirty = false;
    unlock();
  }
  /* No heap for the blob is a failed save like any other: said, and tried
   * again once things go quiet. */
  sLastFailed = !made || !memoryNvsSaveBlob(blob, size);
  free(blob);
  if (sLastFailed) {
    /* Left marked so it is tried again once things go quiet, rather than
     * dropped. A list that failed to write and was forgotten disappears at
     * the next power cycle with nothing having said so. */
    sDirty = true;
    sDirtyAtMs = millis();
    DebugLog.println(F("[memory] the channel list could not be written"));
  }
}

void memoryStorePoll(void) {
  if (!sDirty || (uint32_t)(millis() - sDirtyAtMs) < MEMORY_SETTLE_MS) {
    return;
  }
  writeNow();
}

void memoryStoreSaveNow(void) {
  if (sDirty) {
    writeNow();
  }
}

bool memoryStoreRead(int slot, MemoryChannel *out) {
  if (out == NULL || !lock()) {
    return false;
  }
  const MemoryChannel *c = memoryGet(&sStore, slot);
  if (c != NULL) {
    *out = *c;
  }
  unlock();
  return c != NULL;
}

uint16_t memoryStorePi(int slot) {
  MemoryChannel c;
  return memoryStoreRead(slot, &c) ? c.pi : 0;
}

bool memoryStoreWrite(int slot, const MemoryChannel *c) {
  if (!lock()) {
    return false;
  }
  bool ok = memorySet(&sStore, slot, c);
  if (ok) {
    changed();
  }
  unlock();
  return ok;
}

int memoryStoreNthUsed(int n, MemoryChannel *out) {
  if (out == NULL || !lock()) {
    return MEMORY_NO_SLOT;
  }
  const int slot = memoryNthUsed(&sStore, n);
  if (slot != MEMORY_NO_SLOT) {
    *out = sStore.slot[slot];
  }
  unlock();
  return slot;
}

int memoryStoreCount(void) {
  if (!lock()) {
    return 0;
  }
  int count = memoryCount(&sStore);
  unlock();
  return count;
}

size_t memoryStoreLine(int slot, char *out, size_t cap) {
  if (!lock()) {
    return 0;
  }
  size_t n = memoryCsvLine(&sStore, slot, out, cap);
  unlock();
  return n;
}

bool memoryStoreImport(MemoryImportMode mode, const char *text, size_t len,
                       MemoryImportResult *out) {
  MemoryImportResult counts;
  memset(&counts, 0, sizeof(counts));
  if (out != NULL) {
    *out = counts;
  }
  if (!lock()) {
    return false;
  }
  bool ok = memoryImportCsv(&sStore, mode, text, len, &counts);
  /* A merge whose every line named a taken slot changed nothing, and so did
   * an empty file. Marking those would spend a flash write on a request that
   * did nothing. A replace always counts, because wiping the list is a
   * change even when the file that follows is empty. */
  if (ok && (counts.imported > 0 || mode == MEMORY_IMPORT_REPLACE)) {
    changed();
  }
  unlock();
  if (out != NULL) {
    *out = counts;
  }
  return ok;
}

int memoryStoreStep(const BandPlanConfig *plan, int from, bool up, int moves) {
  if (moves <= 0 || !lock()) {
    return MEMORY_NO_SLOT;
  }
  /* The whole walk under one lock. A knob spun fast sends many steps at once,
   * and taking the lock for each would be dozens of round trips inside the
   * radio task's own cadence for an answer that is one walk. */
  int slot = from;
  for (int i = 0; i < moves; i++) {
    slot = memoryStepTunable(&sStore, plan, slot, up);
    if (slot == MEMORY_NO_SLOT) {
      break;
    }
  }
  unlock();
  return slot;
}

int memoryStoreFind(uint8_t band, uint32_t freqKHz) {
  if (!lock()) {
    return MEMORY_NO_SLOT;
  }
  int slot = memoryFind(&sStore, band, freqKHz);
  unlock();
  return slot;
}

int memoryStoreFindNear(uint8_t band, uint32_t freqKHz, uint32_t toleranceKHz) {
  if (!lock()) {
    return MEMORY_NO_SLOT;
  }
  int slot = memoryFindNear(&sStore, band, freqKHz, toleranceKHz);
  unlock();
  return slot;
}

MemorySaveResult memoryStoreSave(uint8_t band, uint32_t freqKHz,
                                 uint16_t bandwidthKHz, const char *name,
                                 int *slot) {
  if (!lock()) {
    return MEMORY_SAVE_UNREACHABLE;
  }
  MemorySaveResult r =
      memorySaveChannel(&sStore, band, freqKHz, bandwidthKHz, name, slot);
  if (r == MEMORY_SAVE_OK) {
    changed();
  }
  unlock();
  return r;
}

int memoryStoreSlotFor(int first, int second, uint8_t band, uint32_t freqKHz) {
  if (!lock()) {
    return MEMORY_NO_SLOT;
  }
  int slot = memoryPick(&sStore, first, second, band, freqKHz);
  unlock();
  return slot;
}

uint32_t memoryStoreGeneration(void) {
  return sGeneration;
}

bool memoryStoreFailed(void) {
  return sLastFailed;
}

int memoryStoreCleared(void) {
  return sCleared;
}
