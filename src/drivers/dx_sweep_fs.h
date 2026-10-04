/*
 * Keeps the DX level sweeps on the littlefs partition: the
 * last few in one file, and the baseline fixed by hand in another, as a
 * history of one. The byte layout is core/dx_sweep.h's; this is only where
 * the bytes sit. littlefs is mounted by logbookFsBegin at start up.
 */
#ifndef DRIVERS_DX_SWEEP_FS_H
#define DRIVERS_DX_SWEEP_FS_H

#include "core/dx_sweep.h"

#define DX_SWEEP_PATH "/dxsweep.bin"
#define DX_SWEEP_BASE_PATH "/dxbase.bin"

/* Read the file at `path`. A file that is not there, or that this format
 * did not write, reads as an empty history, and so does one that could not
 * be read: the sweeps are a record of the band, not data a person entered,
 * and the next sweep starts a new one. */
void dxSweepFsLoad(const char *path, DxSweepHistory *out);

/* Write the history whole. False when it did not reach flash. */
bool dxSweepFsSave(const char *path, const DxSweepHistory *h);

/* Remove the file. True when it is gone, or never was. */
bool dxSweepFsRemove(const char *path);

#endif /* DRIVERS_DX_SWEEP_FS_H */
