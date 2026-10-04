/*
 * Keeps the set of PIs ever caught in DX mode on the littlefs partition.
 * The byte layout is core/dx_catch.h's; this is only where the bytes sit.
 * littlefs is mounted by logbookFsBegin at start up.
 */
#ifndef DRIVERS_DX_SEEN_FS_H
#define DRIVERS_DX_SEEN_FS_H

#include "core/dx_catch.h"

/*
 * Read the set. A file that is not there, or that this format did not
 * write, reads as an empty set, so a first use and a damaged file both
 * start clean rather than from whatever the bytes held. False, with the set
 * empty, when it could not be read at all: no filesystem, or no heap to
 * read into. Then the set is not known, and saving it would write an empty
 * set over the real one.
 */
bool dxSeenFsLoad(DxSeen *out);

/* Write the set whole. False when it did not reach flash. Called when the
 * set has changed. */
bool dxSeenFsSave(const DxSeen *s);

#endif /* DRIVERS_DX_SEEN_FS_H */
