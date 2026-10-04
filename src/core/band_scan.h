/*
 * The band scan's walk of each band, its decision for one channel: not a
 * station, a station already stored nearby, or a station to add, and what a
 * save keeps while a scan has the dial.
 *
 * Kept apart from the scan's task, which tunes and waits through the radio
 * task, so the same function the radio calls can be tested on a PC with
 * real readings.
 */
#ifndef CORE_BAND_SCAN_H
#define CORE_BAND_SCAN_H

#include <stdbool.h>
#include <stdint.h>

#include "band_plan.h"
#include "radio.h"
#include "seek.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The most readings a scan takes of one channel. */
#define BAND_SCAN_MAX_READS 2

/*
 * How a scan walks one band: the channels it looks at, how many readings it
 * takes of each, and how close a memory channel has to be to count as the
 * station it found.
 */
typedef struct {
  BandId band;
  uint16_t stepKHz;      /* The band's own channel spacing. */
  uint32_t toleranceKHz; /* A memory channel this close is the station. */
  uint32_t firstKHz;     /* The first channel looked at. */
  uint16_t channels;     /* How many channels the walk looks at. */
  uint8_t reads;         /* Readings of each channel, 1 to the most. */
  uint16_t gapMs;        /* Between two readings. */
} BandScanWalk;

/*
 * The walk for `band`, with the band plan's regional choices, NULL for the
 * defaults. False for OIRT, which the scan does not walk, and for a NULL
 * `out`.
 *
 * The step is the band's own channel spacing, the band plan's default step:
 * FM 100 kHz, MW 9 or 10 kHz by region, LW 9 kHz and SW 5 kHz, whatever
 * step the radio is set to, which is a listening choice and not where
 * stations sit.
 *
 * On FM a memory channel within one step is the station, since a person can
 * store a station 50 kHz off the raster. On AM the next channel is another
 * station, and the seek's offset gate already turns away a carrier 9 kHz
 * off, so a memory channel is the station only within half a step: the
 * channel it is nearest.
 *
 * An AM channel is read twice, 30 ms apart, and judged on the quieter
 * reading. The AM noise reading of a strong station jumps: 36 readings of
 * 738 kHz at 48 dBuV, 50 to 200 ms after a retune, read from 25 to 156,
 * against the seek's limit of 120, so one reading in about 36 turns a
 * station away and a scan that walks past it once never comes back. Two
 * readings 30 ms apart differ, so a miss needs two high ones running. On FM
 * the gates pass every station on every reading of a band sweep, so FM
 * reads once.
 *
 * SW walks only the metre bands, the circle the Meter band tuning mode
 * keeps to, about 1000 channels: the whole band at 5 kHz is 5061 channels,
 * over 20 minutes, and the gaps between the metre bands hold no broadcast
 * stations.
 */
bool bandScanWalkFor(BandId band, const BandPlanConfig *plan,
                     BandScanWalk *out);

/*
 * The reading a scan judges a channel on: of `n` taken one after another,
 * the valid one with the least noise. Not valid when none is, or for a NULL
 * list.
 */
SeekReading bandScanQuietest(const SeekReading *readings, uint8_t n);

/* The channel after `khz` on the walk, round to the first after the last. */
uint32_t bandScanNext(const BandScanWalk *walk, const BandPlanConfig *plan,
                      uint32_t khz);

typedef enum {
  BAND_SCAN_NOT_A_STATION = 0,
  BAND_SCAN_STORED_NEAR, /* A station, and a memory channel is near it. */
  BAND_SCAN_ADD,         /* A station with nothing stored near it. */
} BandScanVerdict;

/*
 * The slot of a memory channel on `band` within `toleranceKHz` of
 * `freqKHz`, or MEMORY_NO_SLOT.
 */
typedef int (*BandScanFindNear)(void *ctx, uint8_t band, uint32_t freqKHz,
                                uint32_t toleranceKHz);

/*
 * Judge one channel of the scan from its reading.
 *
 * A station is what the seek would stop on, on the walk's band, so an AM
 * band takes the AM seek's rule. The store is asked about it,
 * through `findNear`, only once it is a station, since that reads every
 * slot. A reading that did not arrive, or a missing argument, is not a
 * station.
 */
BandScanVerdict bandScanJudge(const SeekConfig *cfg, const BandScanWalk *walk,
                              uint32_t freqKHz, const SeekReading *reading,
                              BandScanFindNear findNear, void *ctx);

/*
 * Where a running scan started: the band, its dial and the tuning mode, and
 * the band it scans with that band's own dial before the scan moved it.
 * When the scan changed band, a save keeps the start band's dial as the
 * band change put it away rather than `khz`, so a turn of the knob in the
 * moment before the change is kept.
 */
typedef struct {
  BandId band;
  uint32_t khz;
  TuneMode mode;
  BandId scanned;
  uint32_t scannedKHz;
} BandScanFrom;

/*
 * The radio's settings as a save keeps them while a scan has the dial: on
 * the band and station the scan started from, with the scanned band left
 * where its dial was, so a save, or a restart, during the scan never keeps
 * a channel the scan was only passing. Nothing changes once the radio is on
 * another band than the scanned one, since then a person has moved it, or
 * for a NULL argument.
 */
void bandScanKeep(RadioSettings *s, const BandScanFrom *from);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BAND_SCAN_H */
