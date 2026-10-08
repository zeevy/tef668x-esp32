/* Implementation of the band scope's sweeps. */
#include "scope_task.h"

#include <stdlib.h>

#include "net/ntp.h"
#include "radio_task.h"

typedef struct {
  DxSweep latest;
  DxSweep next;
} Scope;

static const Settings *sSettings = NULL;
/* On the heap, taken the first time a sweep is asked for: static RAM is for
 * what every radio needs. */
static Scope *sScope = NULL;
static bool sRunning = false;
static bool sNextWhole = false;
static bool sWhole = false;
static BandId sNextBand = BAND_FM;
static BandId sBand = BAND_FM;
static bool sAbandoned = false;
static uint16_t sRevision = 0;

void scopeTaskBegin(const Settings *settings) {
  sSettings = settings;
}

/* Start `sweep` on the band `now` is on. */
static ScopeStart start(const RadioSettings *now, const BandPlanConfig *plan,
                        RadioSweepPlan *sweep) {
  if (sScope == NULL) {
    sScope = static_cast<Scope *>(calloc(1, sizeof(Scope)));
    if (sScope == NULL) {
      return SCOPE_NO_MEMORY;
    }
  }
  if (bandModulation(now->band) != MODULATION_FM) {
    sweep->widthKHz = 0;
  }
  if (!radioSweepStart(&sScope->next, sweep)) {
    return SCOPE_REFUSED;
  }
  DxSweepRange whole;
  sNextWhole = dxSweepRange(now->band, plan, now->freqKHz, 0, &whole) &&
               whole.lowKHz == sweep->range.lowKHz &&
               whole.stepKHz == sweep->range.stepKHz &&
               whole.count == sweep->range.count;
  sNextBand = now->band;
  sRunning = true;
  sAbandoned = false;
  return SCOPE_STARTED;
}

ScopeStart scopeTaskSweep(uint32_t spanKHz) {
  scopeTaskPoll();
  RadioSettings now;
  BandPlanConfig plan;
  RadioSweepPlan sweep = {};
  if (sRunning || sSettings == NULL || !radioGetSettings(&now) ||
      !radioTaskPlan(&plan)) {
    return SCOPE_REFUSED;
  }
  if (!dxSweepRange(now.band, &plan, now.freqKHz, spanKHz, &sweep.range)) {
    return SCOPE_TOO_WIDE;
  }
  sweep.widthKHz = sSettings->dxWidthKHz;
  return start(&now, &plan, &sweep);
}

ScopeStart scopeTaskSweepRange(const DxSweepRange *range, uint16_t widthKHz) {
  scopeTaskPoll();
  RadioSettings now;
  BandPlanConfig plan;
  if (range == NULL || sRunning || !radioGetSettings(&now) ||
      !radioTaskPlan(&plan)) {
    return SCOPE_REFUSED;
  }
  RadioSweepPlan sweep = {*range, widthKHz};
  return start(&now, &plan, &sweep);
}

void scopeTaskPoll(void) {
  if (!sRunning || radioSweepBusy()) {
    return;
  }
  sRunning = false;
  if (sScope->next.count == 0) {
    sAbandoned = true;
    return;
  }
  uint32_t utc = 0;
  sScope->next.timeKnown = ntpEpochUtc(&utc);
  sScope->next.at = sScope->next.timeKnown ? utc : 0;
  sScope->latest = sScope->next;
  sWhole = sNextWhole;
  sBand = sNextBand;
  sRevision++;
}

void scopeTaskView(ScopeView *out) {
  scopeTaskPoll();
  out->latest =
      sScope != NULL && sScope->latest.count > 0 ? &sScope->latest : NULL;
  out->whole = sWhole;
  out->running = sRunning;
  out->abandoned = sAbandoned;
  out->revision = sRevision;
  out->band = sBand;
}
