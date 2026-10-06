/* Implementation of the stepped band scan. */
#include "band_scan_task.h"

#include <string.h>

#include "core/band_scan.h"
#include "core/memory.h"
#include "core/radio.h"
#include "core/seek.h"
#include "memory_store.h"
#include "net/update_check.h"
#include "radio_task.h"

/*
 * How long a scan of another band waits for the band change, in ms. A round
 * of the radio task and the tuner's own change between FM and AM, with room
 * for the post to take its lock. A change still waiting after it refuses the
 * scan, and the band change follows in its turn.
 */
#define BAND_SCAN_SWITCH_WAIT_MS 1000

typedef struct {
  bool active;
  BandScanWalk walk;
  BandPlanConfig plan;
  uint32_t atKHz;
  BandScanFrom from;
  /* Where this scan last left the dial. Anywhere else means somebody tuned,
   * and the scan lets go. */
  uint32_t dialKHz;
  uint16_t done;
  uint16_t found;
  uint16_t added;
  uint16_t noRoom;
} BandScanState;

static BandScanState sScan;
static BandScanResult sLast;
static bool sLastRan;

bool bandScanActive(void) {
  return sScan.active;
}

void bandScanProgress(uint16_t *done, uint16_t *total) {
  if (done != NULL) {
    *done = sScan.active ? sScan.done : 0;
  }
  if (total != NULL) {
    *total = sScan.active ? sScan.walk.channels : 0;
  }
}

bool bandScanFrom(BandScanFrom *out) {
  if (!sScan.active || out == NULL) {
    return false;
  }
  *out = sScan.from;
  return true;
}

bool bandScanLastResult(BandScanResult *out) {
  if (out != NULL) {
    *out = sLast;
  }
  return sLastRan;
}

BandScanStartResult bandScanStart(BandId band) {
  if (sScan.active || updateCheckRunning()) {
    return BAND_SCAN_BUSY;
  }
  BandPlanConfig plan;
  BandScanWalk walk;
  if (!radioTaskPlan(&plan)) {
    return BAND_SCAN_BUSY;
  }
  if (!bandScanWalkFor(band, &plan, &walk)) {
    return BAND_SCAN_NOT_WALKED;
  }
  RadioSnapshot snap;
  if (!radioGetSnapshot(&snap)) {
    return BAND_SCAN_BUSY;
  }
  BandScanFrom from;
  from.band = snap.settings.band;
  from.khz = snap.settings.freqKHz;
  from.mode = snap.settings.tuneMode;
  from.scanned = band;
  /* Down before the band change, so the scanned band's dial is not heard
   * for the moment before the first probe. */
  radioSetScanning(true);
  if (from.band != band) {
    /* The probes tune only inside the band the radio is on, so the radio
     * goes to the band first, as the BAND key would take it, and comes back
     * when the scan ends. */
    RadioCommand go;
    memset(&go, 0, sizeof(go));
    go.kind = RADIO_SET_BAND;
    go.band = band;
    if (!radioPostOk(&go, BAND_SCAN_SWITCH_WAIT_MS) ||
        !radioGetSnapshot(&snap) || snap.settings.band != band) {
      /* A band change still waiting is carried out later all the same, so
       * the way back is queued behind it, and the radio ends on its own
       * band rather than on one with no scan running. */
      go.band = from.band;
      (void)radioPost(&go);
      radioSetScanning(false);
      return BAND_SCAN_BUSY;
    }
  }
  from.scannedKHz = snap.settings.freqKHz;

  memset(&sScan, 0, sizeof(sScan));
  sScan.active = true;
  sScan.walk = walk;
  sScan.plan = plan;
  sScan.atKHz = walk.firstKHz;
  sScan.from = from;
  sScan.dialKHz = snap.settings.freqKHz;
  return BAND_SCAN_STARTED;
}

/*
 * Publishes what the scan found, and puts the radio back where the scan
 * found it unless somebody has tuned since. Their choice stands over the
 * scan's.
 */
static void finish(bool restore, bool complete) {
  if (restore) {
    /* Each waited for, so in the usual case the scan is over only once the
     * snapshot is back on the station it started from, and anything that
     * asks whether the dial is walking never finds the last channel probed.
     * A command still waiting after its wait ends the scan anyway and
     * follows in its turn. */
    const BandScanFrom *from = &sScan.from;
    RadioCommand back;
    memset(&back, 0, sizeof(back));
    if (from->band != from->scanned) {
      /* The scanned band is left where its dial was, and the band change
       * back brings the start band's dial, width and step with it. Only a
       * Meter band mode, which no band but SW keeps, has to be put back. */
      back.kind = RADIO_TUNE;
      back.freqKHz = from->scannedKHz;
      (void)radioPostOk(&back, RADIO_TUNE_BACK_WAIT_MS);
      memset(&back, 0, sizeof(back));
      back.kind = RADIO_SET_BAND;
      back.band = from->band;
      (void)radioPostOk(&back, BAND_SCAN_SWITCH_WAIT_MS);
      RadioSnapshot now;
      if (radioGetSnapshot(&now) && now.settings.tuneMode != from->mode) {
        memset(&back, 0, sizeof(back));
        back.kind = RADIO_SET_TUNE_MODE;
        back.tuneMode = from->mode;
        (void)radioPostOk(&back, RADIO_TUNE_BACK_WAIT_MS);
      }
    } else {
      back.kind = RADIO_TUNE;
      back.freqKHz = from->khz;
      (void)radioPostOk(&back, RADIO_TUNE_BACK_WAIT_MS);
    }
  }
  /* After the way back, so the audio comes up on the station the scan
   * started from rather than on the last channel probed. */
  radioSetScanning(false);

  sLast.band = sScan.walk.band;
  sLast.found = sScan.found;
  sLast.added = sScan.added;
  sLast.noRoom = sScan.noRoom;
  sLast.complete = complete;
  sLastRan = true;
  sScan.active = false;
}

void bandScanStop(void) {
  if (sScan.active) {
    finish(true, false);
  }
}

/* The stored channels, for the scan's decision. */
static int findNearInStore(void *ctx, uint8_t band, uint32_t freqKHz,
                           uint32_t toleranceKHz) {
  (void)ctx;
  return memoryStoreFindNear(band, freqKHz, toleranceKHz);
}

void bandScanTaskPoll(void) {
  if (!sScan.active) {
    return;
  }

  Tef668xQuality q[BAND_SCAN_MAX_READS];
  memset(q, 0, sizeof(q));
  bool moved = false;
  RadioProbeResult probed =
      radioSettleProbe(sScan.atKHz, SEEK_SETTLE_MS, sScan.walk.reads,
                       sScan.walk.gapMs, sScan.dialKHz, q, &moved);
  if (probed == RADIO_PROBE_MOVED) {
    /* Somebody tuned, seeked or changed band. The knob means stop, the same
     * as it does for a seek, and the dial stays where they put it. What was
     * already added is kept, and `complete` says the band was not covered. */
    finish(false, false);
    return;
  }
  if (probed == RADIO_PROBE_REFUSED) {
    /* The radio would not take it: on its way down for a restart, or the
     * lock was busy. Stop rather than fight it, keeping what was added. */
    finish(true, false);
    return;
  }
  if (probed == RADIO_PROBE_OK || probed == RADIO_PROBE_NO_READ) {
    /* Both ran, so the dial is on this channel now. After NO_ANSWER the
     * probe was withdrawn and the dial is where it was, unless the radio
     * task had already picked the probe up. Then the next probe reads the
     * late retune as a tune and the scan stops early, which is the safe way
     * to be wrong. */
    sScan.dialKHz = sScan.atKHz;
  }

  /* The same sensitivity the seek uses, so one setting decides what counts
   * as a station for both. */
  SeekConfig cfg;
  radioSeekConfig(&cfg);
  SeekReading readings[BAND_SCAN_MAX_READS];
  for (uint8_t i = 0; i < sScan.walk.reads; i++) {
    readings[i] = radioSeekReading(&q[i], probed == RADIO_PROBE_OK);
  }
  const SeekReading reading = bandScanQuietest(readings, sScan.walk.reads);
  const BandScanVerdict verdict = bandScanJudge(
      &cfg, &sScan.walk, sScan.atKHz, &reading, findNearInStore, NULL);
  if (verdict != BAND_SCAN_NOT_A_STATION) {
    sScan.found++;
  }
  if (verdict == BAND_SCAN_ADD) {
    /* Counted only once written. A store already full is not a failure
     * worth stopping the scan for: the rest of the band is still worth
     * looking at and reporting on, even though nothing more can be kept, so
     * each station that found no room is counted instead. */
    int slot = MEMORY_NO_SLOT;
    const MemorySaveResult saved =
        memoryStoreSave((uint8_t)sScan.walk.band, sScan.atKHz, 0, NULL, &slot);
    if (saved == MEMORY_SAVE_OK) {
      sScan.added++;
    } else if (saved == MEMORY_SAVE_FULL) {
      sScan.noRoom++;
    }
  }

  sScan.done++;
  if (sScan.done >= sScan.walk.channels) {
    finish(true, true);
    return;
  }
  sScan.atKHz = bandScanNext(&sScan.walk, &sScan.plan, sScan.atKHz);
}
