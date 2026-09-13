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
 * **Nothing here writes to flash.** A write to NVS takes tens of milliseconds
 * and the radio task owns a hundred millisecond cadence, so a change is only
 * marked and memoryStorePoll writes it from loop().
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
 * inputs measured for decision 29, and much shorter than the ten seconds the
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

bool memoryStoreRead(int slot, MemoryChannel *out);

bool memoryStoreWrite(int slot, const MemoryChannel *c);

bool memoryStoreClearSlot(int slot);

bool memoryStoreWipe(void);

int memoryStoreCount(void);

size_t memoryStoreLine(int slot, char *out, size_t cap);

bool memoryStoreImport(MemoryImportMode mode, const char *text, size_t len,
                       MemoryImportResult *out);

int memoryStoreStep(const BandPlanConfig *plan, int from, bool up, int moves);

int memoryStoreFind(uint8_t band, uint32_t freqKHz);

int memoryStoreFirstFree(void);

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

#endif /* MEMORY_STORE_H */
