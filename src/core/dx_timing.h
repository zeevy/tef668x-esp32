/*
 * How long RDS takes on each channel a DX scan stops on.
 *
 * For each channel the scanner dwells on: how long from the dial reaching
 * it to the RDS lock, the first group, the first clean block A and the PI
 * confirmed. Kept on flash by the caller, so a scan in a real opening
 * collects them by itself, and served as a CSV file, so the dwell can be
 * set from times measured on distant stations rather than local ones.
 * Pure logic, no hardware.
 */
#ifndef CORE_DX_TIMING_H
#define CORE_DX_TIMING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rds.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A time that never came, in the fields below. */
#define DX_TIMING_NEVER 0xFFFFu

/* One channel's dwell. Zeroed is none under way. */
typedef struct {
  bool active;
  uint32_t khz;
  uint32_t startMs; /* When the dial was first seen on it. */
  /* Milliseconds from `startMs`, or DX_TIMING_NEVER. */
  uint16_t syncMs;     /* The tuner locked to an RDS bit stream. */
  uint16_t groupMs;    /* The first group arrived. */
  uint16_t cleanAMs;   /* The first clean block A. */
  uint16_t piMs;       /* The PI confirmed, or 0000 twice. */
  uint16_t pi;         /* The PI confirmed, 0 with none or with 0000. */
  int16_t levelTenths; /* The strongest level read while on it. */
  uint16_t widthKHz;   /* The filter it was read through, the last seen. */
  uint16_t dwellMs;    /* How long it was on it, once ended. */
} DxTiming;

/* A dwell on `khz` from `nowMs`, ending any under way without keeping it. */
void dxTimingStart(DxTiming *t, uint32_t khz, uint32_t nowMs);

/*
 * One look while on the channel: each event is taken the first time it is
 * seen. Ignored when no dwell is under way or the dial is not on its
 * channel, so a look taken as the dial moves never counts for the wrong
 * one. A NULL `rds` counts only the level and the width, and INT16_MIN
 * for a level that was not read leaves the strongest as it is.
 */
void dxTimingFeed(DxTiming *t, uint32_t dialKHz, const RdsInfo *rds,
                  int16_t levelTenths, uint16_t widthKHz, uint32_t nowMs);

/*
 * End the dwell. True when it is worth keeping, which is when the tuner
 * locked to RDS at least once: an empty channel says nothing about RDS
 * times, and there are two hundred of them a scan. Nothing under way is
 * false.
 */
bool dxTimingEnd(DxTiming *t, uint32_t nowMs);

const char *dxTimingCsvHeader(void);

/*
 * One ended dwell as a CSV line, with `utc`, UTC epoch seconds, or 0 when
 * the clock was not set, which writes an empty field. A time that never
 * came, and a PI not confirmed, are empty fields. Returns the length
 * snprintf would, so a short buffer can be told.
 */
size_t dxTimingCsvLine(const DxTiming *t, uint32_t utc, char *out, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DX_TIMING_H */
