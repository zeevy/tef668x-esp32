/*
 * Reads and writes the channel list in NVS.
 *
 * This lives in drivers/ and not in core/ because it includes Preferences.h.
 * It stores the list as it sits in memory. A blob of any other length is from
 * a firmware whose channel struct was a different shape, and is ignored.
 */
#ifndef DRIVERS_MEMORY_NVS_H
#define DRIVERS_MEMORY_NVS_H

#include "core/memory.h"

bool memoryNvsLoad(MemoryStore *out);

bool memoryNvsSave(const MemoryStore *m);

#endif /* DRIVERS_MEMORY_NVS_H */
