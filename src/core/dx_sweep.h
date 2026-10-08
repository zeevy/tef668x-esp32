/*
 * The DX level sweep: the level of every channel of the band in about 4 s,
 * the sweeps kept, and a baseline to hold one against.
 *
 * Pure logic. The radio task tunes and reads; this averages the readings,
 * packs the kept sweeps for littlefs and unpacks them, and works out the
 * baseline, the noise floor and the peak hold.
 */
#ifndef CORE_DX_SWEEP_H
#define CORE_DX_SWEEP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "band_plan.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The most channels a sweep has: 65.0 to 108.0 MHz in 100 kHz, the widest
 * FM band the band plan gives. */
#define DX_SWEEP_MAX 431

/*
 * How a channel is read, from sweeps on this radio at the 114 kHz DX width with
 * each wait and number of readings, against a slow one that waits 30 ms. The
 * quality time stamp says settled straight after a tune, so it cannot be waited
 * for. With no wait, a channel just below a strong station reads up to 23 dB
 * high, its level not yet fallen; from a 5 ms wait a sweep is as near the slow
 * one as two slow ones are to each other, 1.7 to 1.8 dB against 2.2 on average.
 * Four readings, of about 2.7 ms each, halve the swing of one between two
 * sweeps at the 95th percentile, 15.8 to 7.1 dB. The 211 channels take about 4
 * s.
 */
#define DX_SWEEP_SETTLE_MS 5
#define DX_SWEEP_READS 4

/*
 * The wait on AM, from LW and MW sweeps on this radio at 3, 4 and 8 kHz, each
 * reading against the same channel's once long settled. Read at 5 to 15 ms an
 * AM level is 1.6 to 1.9 dB high on average and 1 reading in 20 is off by 6 to
 * 9 dB; a channel just above a strong station reads 5 dB low and climbs. From
 * 40 ms a reading is as near as one at 90 ms or later, within 0.4 dB at the
 * 95th percentile, at every width. The chip's own time stamp says settled
 * from about 32 ms. The four readings stay: AM noise comes in bursts of 12 to
 * 18 dB, and one alone would make a peak. MW's 142 channels take about 7.5 s.
 */
#define DX_SWEEP_SETTLE_AM_MS 40

/* How many sweeps are kept. */
#define DX_SWEEP_KEEP 8

/* A channel with no reading: its tune or every read failed. */
#define DX_SWEEP_NO_READING INT16_MIN

typedef struct {
  bool timeKnown;    /* `at` is UTC seconds; otherwise the clock was unset. */
  uint32_t at;       /* When it finished. */
  uint32_t lowKHz;   /* The first channel. */
  uint16_t stepKHz;  /* The distance between channels. */
  uint16_t count;    /* How many channels, at most DX_SWEEP_MAX. */
  uint16_t widthKHz; /* The tuner's width through the sweep. */
  uint16_t tookMs;   /* How long the radio took over it. */
  int16_t level[DX_SWEEP_MAX]; /* Tenths of a dBuV, or DX_SWEEP_NO_READING. */
} DxSweep;

/* The kept sweeps, newest first. */
typedef struct {
  uint8_t count;
  DxSweep item[DX_SWEEP_KEEP];
} DxSweepHistory;

/* The channels a sweep reads: the first, the distance between them, and how
 * many. */
typedef struct {
  uint32_t lowKHz;
  uint16_t stepKHz;
  uint16_t count;
} DxSweepRange;

/*
 * The channels a sweep of `band` reads, on the band's own channels from its
 * bottom edge in its default step. `spanKHz` 0 is the whole band. Otherwise it
 * is `spanKHz` wide, centred on the channel nearest `dialKHz`, and moved
 * inside the band where it would cross an edge; a span as wide as the band or
 * wider is the whole band. False when it would be more than DX_SWEEP_MAX
 * channels, as the whole of shortwave is, when the band has no edges or no
 * step, or `plan` or `out` is NULL.
 */
bool dxSweepRange(BandId band, const BandPlanConfig *plan, uint32_t dialKHz,
                  uint32_t spanKHz, DxSweepRange *out);

/* Whether a range can be swept on `band`: one channel or more, no more than
 * DX_SWEEP_MAX, every channel inside the band, and on FM on the tuner's 10
 * kHz grid. */
bool dxSweepRangeFits(BandId band, const BandPlanConfig *plan,
                      const DxSweepRange *r);

/* The average of `n` readings in tenths, rounded to the nearest, or
 * DX_SWEEP_NO_READING for none. */
int16_t dxSweepMean(const int16_t *reads, uint8_t n);

/* Whether two sweeps read the same channels at the same width, so one can
 * be held against the other. False for a sweep with no channels. */
bool dxSweepSameChannels(const DxSweep *a, const DxSweep *b);

/* The noise floor: the level a quarter of the channels read at or below,
 * the lower quartile, or DX_SWEEP_NO_READING for no reading at all. */
int16_t dxSweepFloor(const DxSweep *s);

/*
 * The baseline: channel by channel, the median of the `n` sweeps at `items`
 * that read the same channels as `like`, the mean of the middle two for an
 * even count. Returns how many sweeps it comes from, 0 when none match, and
 * then `out` is left alone. A channel no sweep read is DX_SWEEP_NO_READING.
 * At most DX_SWEEP_KEEP are looked at.
 */
uint8_t dxSweepMedian(const DxSweep *items, uint8_t n, const DxSweep *like,
                      DxSweep *out);

/* Hold the highest reading of each channel. A peak with no channels, or
 * with other channels than `s`, starts again from `s`. */
void dxSweepPeak(DxSweep *peak, const DxSweep *s);

/* The channel of a sweep `khz` is, or -1 when it is not one of them. */
int16_t dxSweepChannelOf(const DxSweep *s, uint32_t khz);

/* The frequency of channel `i`, or 0 past the last. */
uint32_t dxSweepKHzOf(const DxSweep *s, uint16_t i);

/* How long ago a sweep taken at `atUtc` was at `nowUtc`, for the Scope
 * page: "now" under a minute, then "12 min ago", "3 h ago" and "5 d ago",
 * whole units rounded down. False, and `out` empty, when `nowUtc` is
 * before `atUtc`, since the clock has moved and the age is not known. */
bool dxSweepAge(uint32_t atUtc, uint32_t nowUtc, char *out, size_t cap);

/* Put `s` at the front of the kept sweeps, dropping the oldest when full. */
void dxSweepKeep(DxSweepHistory *h, const DxSweep *s);

/* Bytes the packed history takes, and the most it can take. */
size_t dxSweepEncodedSize(const DxSweepHistory *h);
#define DX_SWEEP_FILE_MAX (6 + DX_SWEEP_KEEP * (17 + 2 * DX_SWEEP_MAX))

/* Pack the history for littlefs. Returns the bytes written, 0 when `cap`
 * is short. */
size_t dxSweepEncode(const DxSweepHistory *h, uint8_t *out, size_t cap);

/* Unpack a history. False, and `out` empty, for anything that is not a
 * whole, valid one. */
bool dxSweepDecode(const uint8_t *in, size_t len, DxSweepHistory *out);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DX_SWEEP_H */
