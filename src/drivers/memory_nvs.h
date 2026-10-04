/*
 * Reads and writes the channel list in NVS.
 *
 * This lives in drivers/ and not in core/ because it includes Preferences.h.
 * It stores the blob memoryToBlob makes: an 8 byte header with a version,
 * then the list. A load also reads version 1 and the bare list with no
 * header. A blob in any other form is ignored.
 */
#ifndef DRIVERS_MEMORY_NVS_H
#define DRIVERS_MEMORY_NVS_H

#include "core/memory.h"

/*
 * Read the list back, and empty any slot this firmware could not have
 * written. clearedOut takes how many channels that lost, and may be NULL.
 *
 * The count matters because the list comes back one channel shorter with
 * nothing else saying why, and a shorter list looks exactly like a list that
 * was always that length.
 */
bool memoryNvsLoad(MemoryStore *out, int *clearedOut);

/* Write a blob memoryToBlob already made, `memoryBlobSize()` bytes, for a
 * caller that makes it under its own lock. */
bool memoryNvsSaveBlob(const uint8_t *blob, size_t size);

#endif /* DRIVERS_MEMORY_NVS_H */
