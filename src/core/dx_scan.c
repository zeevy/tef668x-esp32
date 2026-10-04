/* Implementation of the DX scanner. */
#include "dx_scan.h"

#include <string.h>

void dxScanReset(DxScan *s) {
  if (s != NULL) {
    memset(s, 0, sizeof(*s));
  }
}

static const DxScanAction kNone = {false, 0};

/* The first position from `pos` on that is not passed over, with its
 * channel, or false past the end. */
static bool firstFrom(const DxScan *s, uint16_t pos, uint16_t *at,
                      uint32_t *khz) {
  for (; pos < s->plan.count; pos++) {
    if (s->plan.channel(s->plan.ctx, pos, khz)) {
      *at = pos;
      return true;
    }
  }
  return false;
}

static DxScanAction goTo(DxScan *s, uint16_t pos, uint32_t khz,
                         uint32_t dialKHz) {
  s->state = DX_SCAN_RUNNING;
  s->pos = pos;
  s->atKHz = khz;
  s->fromKHz = dialKHz;
  s->landed = false;
  s->waiting = false;
  s->counted = false;
  s->onCatch = false;
  DxScanAction a = {true, khz};
  return a;
}

/* The end of the walk: done, and the dial back where the scan found it. */
static DxScanAction finish(DxScan *s) {
  s->state = DX_SCAN_IDLE;
  s->finished = true;
  s->onCatch = false;
  DxScanAction a = {true, s->startKHz};
  return a;
}

DxScanAction dxScanStart(DxScan *s, const DxScanPlan *plan, uint32_t dialKHz) {
  if (s == NULL || plan == NULL || plan->count == 0 || plan->dwellMs == 0 ||
      plan->channel == NULL) {
    return kNone;
  }
  dxScanReset(s);
  s->plan = *plan;
  s->startKHz = dialKHz;
  uint16_t pos = 0;
  uint32_t khz = 0;
  if (!firstFrom(s, 0, &pos, &khz)) {
    return kNone;
  }
  return goTo(s, pos, khz, dialKHz);
}

/* The position after the one listened to, round again from the first at
 * the end of a looping walk; false at the end of one that does not loop. */
static bool peekNext(const DxScan *s, uint16_t *pos, uint32_t *khz,
                     bool *lapped) {
  *lapped = false;
  if (firstFrom(s, (uint16_t)(s->pos + 1), pos, khz)) {
    return true;
  }
  if (s->plan.loop && firstFrom(s, 0, pos, khz)) {
    *lapped = true;
    return true;
  }
  return false;
}

/*
 * Whether the next position is the frequency the dial is on already: a
 * looping walk of one channel, or two memory slots on one. The chip does not
 * retune to where it is, so its RDS, and the PI just heard, stay; a heard PI
 * there must not send the scan on, or it would go round every look.
 */
static bool nextIsHere(const DxScan *s) {
  uint16_t pos = 0;
  uint32_t khz = 0;
  bool lapped = false;
  return s->landed && peekNext(s, &pos, &khz, &lapped) && khz == s->atKHz;
}

/*
 * On to the next position, or the finish. A new lap starts the found count
 * again, so the page counts one lap's stations, not each station once a
 * lap. The next position on the frequency the dial is on is not tuned; the
 * dwell starts again there.
 */
static DxScanAction next(DxScan *s, uint32_t nowMs, uint32_t dialKHz) {
  uint16_t pos = 0;
  uint32_t khz = 0;
  bool lapped = false;
  if (!peekNext(s, &pos, &khz, &lapped)) {
    return finish(s);
  }
  if (khz == s->atKHz && s->landed) {
    if (lapped) {
      s->found = 0;
      s->counted = false;
    }
    s->state = DX_SCAN_RUNNING;
    s->pos = pos;
    s->landedMs = nowMs;
    s->onCatch = false;
    return kNone;
  }
  if (lapped) {
    s->found = 0;
  }
  return goTo(s, pos, khz, dialKHz);
}

DxScanAction dxScanResume(DxScan *s, uint32_t dialKHz) {
  if (s == NULL || s->state != DX_SCAN_STOPPED) {
    return kNone;
  }
  /* No time is known here; a stay on the same frequency starts its dwell at
   * 0, which the next look ends at once, as a dwell run out would. */
  return next(s, 0, dialKHz);
}

void dxScanStop(DxScan *s) {
  if (s != NULL && s->state == DX_SCAN_RUNNING) {
    s->state = DX_SCAN_STOPPED;
    s->onCatch = false;
  }
}

DxScanAction dxScanPoll(DxScan *s, uint32_t nowMs, uint32_t dialKHz,
                        bool piHeard, bool holdOn, bool stopOnIt) {
  if (s == NULL || s->state != DX_SCAN_RUNNING) {
    return kNone;
  }
  if (!s->landed) {
    if (dialKHz == s->atKHz) {
      s->landed = true;
      s->landedMs = nowMs;
    } else if (dialKHz != s->fromKHz) {
      /* Neither where it was nor where it was sent: somebody tuned. */
      dxScanStop(s);
      return kNone;
    }
    /* Otherwise the tune has not landed yet. A PI now is the last
     * channel's, so it is not asked until it has. A tune the radio would
     * not take never lands, so a dwell's wait is given up on. */
    if (!s->landed) {
      if (!s->waiting) {
        s->waiting = true;
        s->waitMs = nowMs;
      } else if ((uint32_t)(nowMs - s->waitMs) >= s->plan.dwellMs) {
        return next(s, nowMs, dialKHz);
      }
      return kNone;
    }
  } else if (dialKHz != s->atKHz) {
    dxScanStop(s);
    return kNone;
  }
  /* Counted once a channel, the moment its PI is confirmed, whatever is
   * done about it. */
  if (piHeard && !s->counted) {
    s->found++;
    s->counted = true;
  }
  if (piHeard && stopOnIt) {
    s->state = DX_SCAN_STOPPED;
    s->onCatch = true;
    return kNone;
  }
  if (piHeard && !holdOn && !nextIsHere(s)) {
    return next(s, nowMs, dialKHz);
  }
  if ((uint32_t)(nowMs - s->landedMs) >= s->plan.dwellMs) {
    return next(s, nowMs, dialKHz);
  }
  return kNone;
}

uint32_t dxScanLeftMs(const DxScan *s, uint32_t nowMs) {
  if (s == NULL || s->state != DX_SCAN_RUNNING) {
    return 0;
  }
  if (!s->landed) {
    return s->plan.dwellMs;
  }
  const uint32_t gone = (uint32_t)(nowMs - s->landedMs);
  return gone >= s->plan.dwellMs ? 0 : s->plan.dwellMs - gone;
}

uint16_t dxScanPassed(const DxScan *s) {
  if (s == NULL) {
    return 0;
  }
  if (s->finished && s->state == DX_SCAN_IDLE) {
    return s->plan.count;
  }
  return s->pos;
}

uint16_t dxScanTotal(const DxScan *s) {
  return s != NULL ? s->plan.count : 0;
}

bool dxScanBandChannel(void *ctx, uint16_t pos, uint32_t *khz) {
  const DxScanBand *b = (const DxScanBand *)ctx;
  if (b == NULL || khz == NULL) {
    return false;
  }
  const uint32_t at = b->lowKHz + (uint32_t)pos * b->stepKHz;
  if (b->skip != NULL && b->skip(b->skipCtx, at)) {
    return false;
  }
  *khz = at;
  return true;
}

uint16_t dxScanBandCount(uint32_t lowKHz, uint32_t highKHz, uint16_t stepKHz) {
  if (stepKHz == 0 || highKHz < lowKHz) {
    return 0;
  }
  return (uint16_t)((highKHz - lowKHz) / stepKHz + 1);
}
