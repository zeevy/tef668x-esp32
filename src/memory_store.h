/*
 * Owns the channel list, and is the only way to reach it.
 *
 * A composition root, like settings_task.h. It is the one place that knows
 * core/memory.h and NVS at once.
 *
 * The list is read from two cores: the radio task asks for the next channel
 * when the knob is turned in memory mode, and the web server reads and writes
 * it while serving a request. So every call takes a lock, and nothing hands
 * out a pointer into the list.
 *
 * **Only two calls write to flash, both on the loop task.** A write to NVS
 * takes tens of milliseconds and the radio task owns a hundred millisecond
 * cadence, so a change is only marked. memoryStorePoll writes it from loop()
 * once it settles, and memoryStoreSaveNow at once, before a restart.
 */
#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/memory.h"
#include "core/memory_csv.h"

/*
 * How quiet the list has to go before it is written, milliseconds.
 *
 * A run of edits then costs one write of the whole list rather than one each.
 * Two seconds is about four and a half times the 0.43 s median gap between
 * inputs measured on the radio, and much shorter than the ten seconds the
 * settings wait, because storing a channel is a deliberate act and the window
 * in which a power cycle loses it should be small.
 */
#define MEMORY_SETTLE_MS 2000

bool memoryStoreBegin(void);

/*
 * Write the list down if it has changed and has stopped changing.
 *
 * Call this from loop(). It costs nothing when nothing has changed.
 */
void memoryStorePoll(void);

/*
 * Write the list down now if it has changed, without waiting for it to go
 * quiet. For a restart somebody asked for, the same routes that call
 * settingsTaskSaveNow, so a channel stored or imported in the last two
 * seconds, or found by a scan still running, is not lost. Loop task only.
 */
void memoryStoreSaveNow(void);

bool memoryStoreRead(int slot, MemoryChannel *out);

/* The PI stored with `slot`, or 0 when the slot is empty, out of range or
 * has learnt none. MEMORY_NO_SLOT gives 0. */
uint16_t memoryStorePi(int slot);

bool memoryStoreWrite(int slot, const MemoryChannel *c);

int memoryStoreCount(void);
/* The n-th used slot, n from 0 in slot order, read into `out` under one lock,
 * or MEMORY_NO_SLOT. */
int memoryStoreNthUsed(int n, MemoryChannel *out);

size_t memoryStoreLine(int slot, char *out, size_t cap);

bool memoryStoreImport(MemoryImportMode mode, const char *text, size_t len,
                       MemoryImportResult *out);

int memoryStoreStep(const BandPlanConfig *plan, int from, bool up, int moves);

int memoryStoreFind(uint8_t band, uint32_t freqKHz);

int memoryStoreFindNear(uint8_t band, uint32_t freqKHz, uint32_t toleranceKHz);

/*
 * memorySaveChannel on the list, under one take of its lock.
 *
 * The call a station scan saves what it finds through.
 * MEMORY_SAVE_UNREACHABLE when the store has no lock, which means it did not
 * start.
 */
MemorySaveResult memoryStoreSave(uint8_t band, uint32_t freqKHz,
                                 uint16_t bandwidthKHz, const char *name,
                                 int *slot);

/* memoryPick on the list, under one take of its lock. MEMORY_NO_SLOT when the
 * lock could not be taken. */
int memoryStoreSlotFor(int first, int second, uint8_t band, uint32_t freqKHz);

/*
 * A number that changes whenever the list does.
 *
 * For a caller holding something worked out from the list. The radio holds
 * which slot it is sitting on, and without this a channel stored for the
 * station already playing would not show its slot until the dial was moved
 * off it and back.
 */
uint32_t memoryStoreGeneration(void);

/*
 * Whether the last write to NVS failed.
 *
 * Worth publishing because a list that cannot be written looks like a working
 * radio until the next power cycle, when every channel is gone.
 */
bool memoryStoreFailed(void);

/*
 * How many stored channels the last start up threw away.
 *
 * Not zero means the list in flash held a channel this firmware could not
 * have written, so the list is shorter than the person left it.
 */
int memoryStoreCleared(void);

#endif /* MEMORY_STORE_H */
