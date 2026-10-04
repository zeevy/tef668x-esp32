/*
 * Walking a whole band, channel by channel, to see what is on it, and
 * keeping what it finds as ordinary memory channels.
 *
 * A seek stops on the first station it finds. This does not stop: it
 * visits every channel of the band's walk, `bandScanWalkFor`, and judges
 * each one with the seek's stop decision from `core/seek.c` at the default
 * seek sensitivity, `seekDefaults`. The Seek Sensitivity setting does not
 * apply to the scan. A channel that passes is kept as a memory channel,
 * the same 99 slots `core/memory.h` already owns. It is skipped rather
 * than stored again when a memory channel already sits within the walk's
 * tolerance of it on the same band, `memoryFindNear`. A slot that already
 * holds something is never touched: the scan only ever writes into a slot
 * nothing else was using.
 *
 * One channel is looked at per `bandScanTaskPoll` call, called from
 * `loop()` the same as every other composition root's own poll, rather
 * than blocking for however long the whole scan takes. FM's 211 channels
 * take about a quarter of a second each, close to a minute in all, and
 * `loop()` is the one place that also drives the screen and the web
 * server; a single blocking call there would freeze the panel and every
 * HTTP request for that whole stretch. `radioSettleProbe` itself still
 * blocks the caller for one channel: the settle time and the rest of the
 * radio task's round. Every other caller that reads the tuner through
 * `radioSettleProbe` waits the same way.
 */
#ifndef BAND_SCAN_TASK_H
#define BAND_SCAN_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/band_plan.h"
#include "core/band_scan.h"

/* Whether a scan started, and if not, why. */
typedef enum {
  BAND_SCAN_STARTED = 0,
  BAND_SCAN_BUSY, /* One is running, the radio could not be read, or it did
                     not take the band change in time. */
  BAND_SCAN_NOT_WALKED, /* A band the scan does not walk: OIRT. */
} BandScanStartResult;

/*
 * Start scanning `band`.
 *
 * Nothing starts while a scan is already running, or while the radio could
 * not be read to know where to go back to once this one ends. On another
 * band, the radio changes to `band` first, as the BAND key would, and the
 * scan starts only once it is there; the call waits up to a second for
 * that, on the caller's task.
 *
 * Tuning, seeking or changing band while it runs stops it, and the dial
 * stays where it was put. Only a scan that reaches the end of its walk, or
 * is refused by the radio, goes back: to the band, station and tuning mode
 * it started from, with the scanned band's own dial left where it was.
 */
BandScanStartResult bandScanStart(BandId band);

/* Called from loop(). Does one channel's worth of work if a scan is
 * running, nothing at all otherwise. */
void bandScanTaskPoll(void);

bool bandScanActive(void);

/* Ends a running scan the way a scan the radio refused does: the radio goes
 * back where the scan found it, what was added is kept, and the result says
 * the band was not covered. Nothing when no scan is running. Loop task
 * only. */
void bandScanStop(void);

/* Where the running scan started, for a save to keep while it runs,
 * bandScanKeep. False when no scan is running. Loop task only, where the
 * scan runs. */
bool bandScanFrom(BandScanFrom *out);

/*
 * How far the running scan has got. Both 0 when none is running: `done`
 * counts channels looked at so far and `total` the channels of the walk,
 * so a caller can show "done of total" without knowing the band plan
 * itself.
 */
void bandScanProgress(uint16_t *done, uint16_t *total);

/* What the last scan to finish did. */
typedef struct {
  BandId band;     /* The band it walked. */
  uint16_t found;  /* Channels that passed the seek's own stop decision. */
  uint16_t added;  /* Of those, how many were written into a free slot. */
  uint16_t noRoom; /* New ones with no free slot left to take them. */
  /* False when it stopped before the end of its walk, because somebody
   * tuned or the radio would not take a probe, so a short count is not read
   * as a quiet band. */
  bool complete;
} BandScanResult;

/*
 * The last scan to finish, with every count 0 until one has. A found station
 * is not added when a memory channel already holds it, or when every slot is
 * full, and `noRoom` counts the second, so a full store is not read as a band
 * with nothing new on it.
 *
 * Returns false until a scan has finished, so "none has run" is not read
 * as "one was cut short".
 */
bool bandScanLastResult(BandScanResult *out);

#endif /* BAND_SCAN_TASK_H */
