/* How a meter bar is divided up, and how its peak mark falls back. */
#include "meter.h"

uint8_t meterSegmentsLit(uint8_t percent, uint8_t count) {
  if (count == 0 || percent == 0) {
    return 0;
  }
  if (percent >= 100) {
    return count;
  }
  const uint8_t lit = (uint8_t)(((uint32_t)percent * count) / 100u);
  return lit == 0 ? 1 : lit;
}

void meterPeakReset(MeterPeak *p) {
  if (p == NULL) {
    return;
  }
  p->percent = 0;
  p->valid = false;
  p->heldMs = 0;
  p->fellMs = 0;
}

void meterPeakClear(MeterPeak *p) {
  meterPeakReset(p);
}

void meterPeakFeed(MeterPeak *p, uint8_t percent, uint32_t nowMs,
                   uint32_t holdMs, uint32_t fallFullMs) {
  if (p == NULL) {
    return;
  }
  if (percent > 100) {
    percent = 100;
  }
  if (!p->valid || percent >= p->percent) {
    /* At or above the mark, so the mark is the reading and the hold starts
     * again. This is the only place it ever goes up. */
    p->percent = percent;
    p->valid = true;
    p->heldMs = nowMs;
    p->fellMs = nowMs;
    return;
  }
  if ((uint32_t)(nowMs - p->heldMs) < holdMs) {
    return;
  }
  /* The fall starts when the hold ends. Counted from the push, its first step
   * would take back the whole hold at once, about three segments of a
   * fourteen segment meter in one frame. */
  const uint32_t holdEnd = p->heldMs + holdMs;
  if ((int32_t)(p->fellMs - holdEnd) < 0) {
    p->fellMs = holdEnd;
  }
  if (fallFullMs == 0) {
    p->percent = percent;
    p->fellMs = nowMs;
    return;
  }
  /*
   * How far it should have fallen by now, worked out from the clock rather
   * than from how many times this was called. `fellMs` moves by exactly the
   * time that was used, so the remainder is carried to the next call and a
   * slow fall does not stall on a caller that polls faster than one per cent
   * of the bar.
   */
  const uint32_t since = (uint32_t)(nowMs - p->fellMs);
  const uint32_t msPerPercent = fallFullMs / 100u;
  if (msPerPercent == 0) {
    p->percent = percent;
    p->fellMs = nowMs;
    return;
  }
  const uint32_t steps = since / msPerPercent;
  if (steps == 0) {
    return;
  }
  p->fellMs += steps * msPerPercent;
  /* It stops when it meets the bar rather than running on to zero. The moment
   * the reading is at or above it, the mark belongs there. */
  p->percent =
      steps >= p->percent ? percent : (uint8_t)(p->percent - (uint8_t)steps);
  if (p->percent < percent) {
    p->percent = percent;
  }
}

void meterBarReset(MeterBar *b) {
  if (b != NULL) {
    b->percent = 0;
    b->valid = false;
    b->fellMs = 0;
  }
}

void meterBarClear(MeterBar *b) {
  meterBarReset(b);
}

uint16_t meterBarFeed(MeterBar *b, uint16_t percent, uint32_t nowMs,
                      uint32_t fallFullMs) {
  if (b == NULL) {
    return percent;
  }
  if (!b->valid || percent >= b->percent) {
    /* Up at once. A peak that is smoothed on the way up is not a peak, and
     * the whole bar exists to show one. */
    b->percent = percent;
    b->valid = true;
    b->fellMs = nowMs;
    return b->percent;
  }
  if (fallFullMs == 0) {
    b->percent = percent;
    b->fellMs = nowMs;
    return b->percent;
  }
  /*
   * How far it should have fallen by now, from the clock rather than from how
   * many times this was called. `fellMs` moves by exactly the time that was
   * used, so the remainder is carried and a caller polling faster than one
   * per cent of the bar does not stall the fall.
   */
  const uint32_t msPerPercent = fallFullMs / 100u;
  if (msPerPercent == 0) {
    b->percent = percent;
    b->fellMs = nowMs;
    return b->percent;
  }
  const uint32_t steps = (uint32_t)(nowMs - b->fellMs) / msPerPercent;
  if (steps == 0) {
    return b->percent;
  }
  b->fellMs += steps * msPerPercent;
  b->percent =
      steps >= b->percent ? percent : (uint16_t)(b->percent - (uint16_t)steps);
  /* The fall stops at the reading. Below it the bar would be showing less
   * than the radio is measuring. */
  if (b->percent < percent) {
    b->percent = percent;
  }
  return b->percent;
}
