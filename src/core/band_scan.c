#include "band_scan.h"

#include <stddef.h>
#include <string.h>

#include "memory.h"

bool bandScanWalkFor(BandId band, const BandPlanConfig *plan,
                     BandScanWalk *out) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (out == NULL || band == BAND_OIRT || !bandLimits(band, plan, &lo, &hi)) {
    return false;
  }
  out->band = band;
  out->stepKHz = bandDefaultStep(band, plan);
  if (out->stepKHz == 0) {
    return false;
  }
  const bool fm = bandModulation(band) == MODULATION_FM;
  out->toleranceKHz = fm ? out->stepKHz : out->stepKHz / 2u;
  out->reads = fm ? 1 : BAND_SCAN_MAX_READS;
  out->gapMs = fm ? 0 : 30;
  if (band == BAND_SW) {
    out->firstKHz = swMeterBandAt(0)->lowKHz;
    out->channels = (uint16_t)swMeterChannels(out->stepKHz);
  } else {
    out->firstKHz = lo;
    out->channels = (uint16_t)((hi - lo) / out->stepKHz + 1u);
  }
  return true;
}

SeekReading bandScanQuietest(const SeekReading *readings, uint8_t n) {
  SeekReading best;
  memset(&best, 0, sizeof(best));
  for (uint8_t i = 0; readings != NULL && i < n; i++) {
    if (readings[i].valid &&
        (!best.valid || readings[i].noiseTenths < best.noiseTenths)) {
      best = readings[i];
    }
  }
  return best;
}

uint32_t bandScanNext(const BandScanWalk *walk, const BandPlanConfig *plan,
                      uint32_t khz) {
  if (walk == NULL) {
    return khz;
  }
  return walk->band == BAND_SW
             ? swMeterStepUp(khz, walk->stepKHz)
             : bandStepUp(walk->band, plan, khz, walk->stepKHz);
}

BandScanVerdict bandScanJudge(const SeekConfig *cfg, const BandScanWalk *walk,
                              uint32_t freqKHz, const SeekReading *reading,
                              BandScanFindNear findNear, void *ctx) {
  if (cfg == NULL || walk == NULL || reading == NULL || findNear == NULL ||
      !reading->valid || !seekShouldStop(cfg, walk->band, reading)) {
    return BAND_SCAN_NOT_A_STATION;
  }
  return findNear(ctx, (uint8_t)walk->band, freqKHz, walk->toleranceKHz) ==
                 MEMORY_NO_SLOT
             ? BAND_SCAN_ADD
             : BAND_SCAN_STORED_NEAR;
}

void bandScanKeep(RadioSettings *s, const BandScanFrom *from) {
  if (s == NULL || from == NULL || s->band != from->scanned ||
      from->band >= BAND_COUNT || from->scanned >= BAND_COUNT) {
    return;
  }
  if (from->band != from->scanned) {
    /* The band change put away what the start band was set to, dial and
     * all, so this puts the scanned band away the same way and takes that
     * back, as the way back at the end of the scan does. */
    s->bandFreqKHz[s->band] = from->scannedKHz;
    s->bandBandwidthKHz[s->band] = s->bandwidthKHz;
    s->bandStepKHz[s->band] = s->stepKHz;
    s->band = from->band;
    s->freqKHz = s->bandFreqKHz[from->band];
    s->bandwidthKHz = s->bandBandwidthKHz[from->band];
    s->stepKHz = s->bandStepKHz[from->band];
    s->tuneMode = from->mode;
    return;
  }
  s->freqKHz = from->khz;
}
