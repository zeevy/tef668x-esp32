/* Implementation of the FM DX rules. */
#include "dx.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

bool dxOnChannel(const SeekReading *reading) {
  if (reading == NULL || !reading->valid) {
    return false;
  }
  /* The same window, and the same edges, as the seek's own offset test. */
  return reading->offsetTenths > -SEEK_OFFSET_FM_TENTHS &&
         reading->offsetTenths < SEEK_OFFSET_FM_TENTHS;
}

void dxStationIdentity(const RdsInfo *rds, const SeekReading *reading,
                       RdsRegion region, uint16_t presetPi, bool pageRule,
                       DxIdentity *out) {
  if (out == NULL) {
    return;
  }
  memset(out, 0, sizeof(*out));
  out->call = RDS_CALL_NONE;
  out->preset = DX_PRESET_NONE;
  if (rds == NULL) {
    return;
  }
  if (!pageRule || dxPiTile(rds, reading) == DX_PI_CONFIRMED) {
    /* Named only with the PI confirmed too, since the country is the two
     * together. */
    out->country =
        rds->hasEcc && rds->hasPi ? rdsCountryCode(rds->pi, rds->ecc) : NULL;
    out->call =
        rdsStationCall(rds, region, out->callText, sizeof(out->callText));
  }
  out->preset = dxPresetPi(rds, reading, presetPi);
}

void dxLogIdentity(LogbookEntry *e, const RdsInfo *rds,
                   const SeekReading *reading) {
  if (e == NULL) {
    return;
  }
  e->hasName = rds != NULL && rds->hasPs && dxOnChannel(reading);
  if (e->hasName) {
    snprintf(e->name, sizeof(e->name), "%s", rds->ps);
  } else {
    e->name[0] = '\0';
  }
  e->hasPi = dxPiConfirmed(rds, reading);
  e->pi = e->hasPi ? rds->pi : 0;
}

bool dxPiConfirmed(const RdsInfo *rds, const SeekReading *reading) {
  return rds != NULL && rds->hasPi && dxOnChannel(reading);
}

void dxLogRadioText(LogbookEntry *e, const RdsInfo *rds,
                    const SeekReading *reading, uint32_t dialKHz) {
  if (e == NULL) {
    return;
  }
  /* Zeroed whole, so the record's bytes do not depend on what was there. */
  e->hasRt = false;
  memset(e->rt, 0, sizeof(e->rt));
  if (rds == NULL || !rds->hasRt || e->freqKHz != dialKHz ||
      !dxOnChannel(reading) ||
      (e->hasPi && !(rds->hasPi && rds->pi == e->pi))) {
    return;
  }
  e->hasRt = true;
  snprintf(e->rt, sizeof(e->rt), "%s", rds->rt);
}

bool dxPiHeardNow(const RdsInfo *rds, const SeekReading *reading) {
  return dxPiConfirmed(rds, reading) && rds->hasBlockErrors &&
         rds->blockError[0] == RDS_BLOCK_CLEAN && rds->hasPiHeard &&
         rds->piHeard == rds->pi;
}

DxPresetPi dxPresetPi(const RdsInfo *rds, const SeekReading *reading,
                      uint16_t storedPi) {
  if (rds == NULL || storedPi == 0) {
    return DX_PRESET_NONE;
  }
  if (dxPiConfirmed(rds, reading)) {
    return rds->pi == storedPi ? DX_PRESET_MATCH : DX_PRESET_OTHER;
  }
  if (dxOnChannel(reading) && rds->hasPiHeard && rds->piHeard == storedPi) {
    return DX_PRESET_MATCH;
  }
  return DX_PRESET_NONE;
}

bool dxPresetLearn(const RdsInfo *rds, const SeekReading *reading,
                   uint16_t storedPi, uint16_t *out) {
  if (out == NULL || storedPi != 0 || !dxPiConfirmed(rds, reading)) {
    return false;
  }
  *out = rds->pi;
  return true;
}

DxPiTile dxPiTile(const RdsInfo *rds, const SeekReading *reading) {
  if (rds == NULL) {
    return DX_PI_NONE;
  }
  if (dxPiConfirmed(rds, reading)) {
    return DX_PI_CONFIRMED;
  }
  if (rds->piZero) {
    return DX_PI_ZERO;
  }
  if (!rds->hasPiHeard) {
    return DX_PI_NONE;
  }
  return rds->piUnsureNibbles != 0 ? DX_PI_PARTIAL : DX_PI_SEEN;
}

void dxPiDigits(const RdsInfo *rds, DxPiTile tile, char *out) {
  if (out == NULL) {
    return;
  }
  out[0] = '\0';
  if (rds == NULL) {
    return;
  }
  switch (tile) {
    case DX_PI_CONFIRMED:
      snprintf(out, 5, "%04X", (unsigned)rds->pi);
      break;
    case DX_PI_ZERO:
      memcpy(out, "0000", 5);
      break;
    case DX_PI_SEEN:
    case DX_PI_PARTIAL:
      rdsFormatPiHeard(rds, out);
      break;
    case DX_PI_NONE:
    default:
      break;
  }
}

uint8_t dxBlockSegments(int8_t level) {
  return level < 0 || level > 3 ? 0 : (uint8_t)(4 - level);
}

void dxHistoryReset(DxHistory *h) {
  if (h != NULL) {
    memset(h, 0, sizeof(*h));
  }
}

/*
 * Move the newest slot on to `second`, emptying every second passed over on
 * the way, so a gap in the readings shows as a gap and not as the last bar
 * repeated.
 */
static void advanceTo(DxHistory *h, uint32_t second) {
  if (!h->started) {
    h->started = true;
    h->newestSecond = second;
    h->newest = 0;
    h->have[0] = false;
    return;
  }
  uint32_t passed = second - h->newestSecond;
  if (passed == 0) {
    return;
  }
  if (passed >= DX_HISTORY_SECONDS) {
    memset(h->have, 0, sizeof(h->have));
    passed = DX_HISTORY_SECONDS;
  }
  for (uint32_t i = 0; i < passed; i++) {
    h->newest = (uint8_t)((h->newest + 1) % DX_HISTORY_SECONDS);
    h->have[h->newest] = false;
  }
  h->newestSecond = second;
}

void dxHistoryAdd(DxHistory *h, uint32_t nowMs, int16_t levelTenths) {
  if (h == NULL) {
    return;
  }
  advanceTo(h, nowMs / 1000u);
  if (!h->have[h->newest] || levelTenths > h->peakTenths[h->newest]) {
    h->peakTenths[h->newest] = levelTenths;
  }
  h->have[h->newest] = true;
}

bool dxHistoryBar(const DxHistory *h, uint32_t nowMs, uint8_t bar,
                  int16_t *peakTenths) {
  if (h == NULL || !h->started || bar >= DX_HISTORY_SECONDS) {
    return false;
  }
  /* How many seconds before now this bar is, and how far that is behind the
   * newest reading. Seconds after the newest one have not had a reading. */
  uint32_t age = (uint32_t)(DX_HISTORY_SECONDS - 1 - bar);
  uint32_t sinceNewest = nowMs / 1000u - h->newestSecond;
  if (age < sinceNewest) {
    return false;
  }
  uint32_t back = age - sinceNewest;
  if (back >= DX_HISTORY_SECONDS) {
    return false;
  }
  uint8_t slot =
      (uint8_t)((h->newest + DX_HISTORY_SECONDS - back) % DX_HISTORY_SECONDS);
  if (!h->have[slot]) {
    return false;
  }
  if (peakTenths != NULL) {
    *peakTenths = h->peakTenths[slot];
  }
  return true;
}

/* The top of the signal history bars' scale, in tenths of a dBuV. */
#define DX_HISTORY_TOP_TENTHS 700

uint8_t dxHistoryBarHeight(int16_t levelTenths, uint8_t innerHeight) {
  if (innerHeight == 0) {
    return 0;
  }
  int32_t level = levelTenths < 0 ? 0 : levelTenths;
  if (level > DX_HISTORY_TOP_TENTHS) {
    level = DX_HISTORY_TOP_TENTHS;
  }
  int32_t height =
      (level * innerHeight + DX_HISTORY_TOP_TENTHS / 2) / DX_HISTORY_TOP_TENTHS;
  return (uint8_t)(height < 1 ? 1 : height);
}
