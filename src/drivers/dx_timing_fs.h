/*
 * Keeps the DX scan's RDS times on the littlefs partition: one CSV
 * line a channel, core/dx_timing.h's, appended as each dwell ends. The file
 * is capped: past DX_TIMING_FILE_MAX it becomes the older file and a new
 * one starts, so the two hold the latest few hundred lines. littlefs is
 * mounted by logbookFsBegin at start up.
 */
#ifndef DRIVERS_DX_TIMING_FS_H
#define DRIVERS_DX_TIMING_FS_H

#include <stddef.h>

#define DX_TIMING_PATH "/dxtiming.csv"
#define DX_TIMING_OLD_PATH "/dxtiming.old"
/* About 500 lines a file. */
#define DX_TIMING_FILE_MAX 32768

/* Add one line. False when it did not reach flash. */
bool dxTimingFsAppend(const char *line);

/* Hand every line kept to `send`, oldest first, a piece at a time. */
void dxTimingFsRead(void (*send)(const char *text, size_t len, void *ctx),
                    void *ctx);

#endif /* DRIVERS_DX_TIMING_FS_H */
