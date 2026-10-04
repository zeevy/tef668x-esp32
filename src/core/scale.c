/* Implementation of the sliding tuning scale. */
#include "scale.h"

#include <stddef.h>

static int64_t tickKHz(bool fm) {
  return fm ? SCALE_FM_TICK_KHZ : SCALE_AM_TICK_KHZ;
}

/* `a / b` rounded down, where C rounds towards zero. `b` is positive. */
static int64_t floorDiv(int64_t a, int64_t b) {
  return a >= 0 ? a / b : -((-a + b - 1) / b);
}

/* `a` brought into 0 to `b - 1`. `b` is positive. */
static int64_t floorMod(int64_t a, int64_t b) {
  return a - floorDiv(a, b) * b;
}

/*
 * The band's ticks: the first multiple of `tick` at or above `low`, and how
 * many there are up to `high`. False when no tick falls in the band, which
 * leaves nothing to draw.
 */
static bool bandTicks(uint32_t low, uint32_t high, int64_t tick, int64_t *first,
                      int64_t *count) {
  if (high <= low) {
    return false;
  }
  *first = floorDiv((int64_t)low + tick - 1, tick) * tick;
  if (*first > (int64_t)high) {
    return false;
  }
  *count = ((int64_t)high - *first) / tick + 1;
  return true;
}

/* How far round the loop is, in kHz: the band's ticks and the dummies after
 * them, so the first tick comes SCALE_SEAM_TICKS + 1 ticks after the last. */
static int64_t loopKHz(int64_t count, int64_t tick) {
  return (count + SCALE_SEAM_TICKS) * tick;
}

bool scaleUnderMiddle(int32_t dxPx) {
  return dxPx > -2 && dxPx < 2;
}

/* Where the peak's ranks start, in pixels from the middle: past the middle
 * mark, which is 2 wide. */
#define PEAK_FIRST_PX 2

int scalePeakRank(int32_t dxPx) {
  const int32_t d = dxPx < 0 ? -dxPx : dxPx;
  if (d < PEAK_FIRST_PX) {
    return -1;
  }
  /* The ranks are a tick wide, so each holds exactly one mark on each
   * side. */
  const int32_t rank = (d - PEAK_FIRST_PX) / SCALE_TICK_PX;
  return rank < SCALE_PEAK_RANKS ? (int)rank : -1;
}

int32_t scalePeakReachPx(void) {
  return PEAK_FIRST_PX + SCALE_PEAK_RANKS * SCALE_TICK_PX - 1;
}

int16_t scalePeakPx(int rank, uint8_t levelPercent) {
  if (rank < 0 || rank >= SCALE_PEAK_RANKS) {
    return 0;
  }
  const int32_t level = levelPercent > 100 ? 100 : levelPercent;
  const int32_t top = (SCALE_PEAK_MAX_PX * level + 50) / 100;
  return (int16_t)((top * (100 - SCALE_PEAK_STEP_PERCENT * rank) + 50) / 100);
}

int scaleTicks(uint32_t lowKHz, uint32_t highKHz, uint32_t tunedKHz, bool fm,
               int16_t widthPx, ScaleTick *out, int max) {
  const int64_t tick = tickKHz(fm);
  int64_t first = 0;
  int64_t count = 0;
  if (widthPx < 2 || out == NULL || max <= 0 ||
      !bandTicks(lowKHz, highKHz, tick, &first, &count)) {
    return 0;
  }
  const int32_t centre = widthPx / 2;
  /* Tick `k` counts from the band's first tick and runs on past either end
   * without wrapping, so the ticks stay a tick apart. Which tick of the band
   * it is, and so its mark and number, is `k` taken round the loop; past the
   * band's last tick come the dummies. Starts one tick left of the first
   * place a tick is given. */
  int64_t k = floorDiv((int64_t)tunedKHz - first, tick) -
              (int64_t)((centre + SCALE_EDGE_PX) / SCALE_TICK_PX) - 1;
  int n = 0;
  for (; n < max; k++) {
    const int32_t x =
        centre +
        (int32_t)floorDiv(
            (first + k * tick - (int64_t)tunedKHz) * SCALE_TICK_PX, tick);
    if (x > widthPx - 2 + SCALE_EDGE_PX) {
      break;
    }
    if (x < -SCALE_EDGE_PX) {
      continue;
    }
    const int64_t j = floorMod(k, count + SCALE_SEAM_TICKS);
    out[n].x = (int16_t)x;
    out[n].inBand = j < count;
    if (out[n].inBand) {
      const int64_t f = first + j * tick;
      const int64_t place = (f / tick) % 10;
      out[n].mark = (uint8_t)(place == 0   ? SCALE_LONG
                              : place == 5 ? SCALE_MEDIUM
                                           : SCALE_SHORT);
      /* A number past what 16 bits hold is no band this radio has; it is
       * left blank rather than shown wrapped. */
      const int64_t number = fm ? f / 1000 : f;
      out[n].number = number <= UINT16_MAX ? (uint16_t)number : 0;
    } else {
      out[n].mark = (uint8_t)SCALE_SHORT;
      out[n].number = 0;
    }
    n++;
  }
  return n;
}

int scaleSeams(uint32_t lowKHz, uint32_t highKHz, uint32_t tunedKHz, bool fm,
               int16_t widthPx, int16_t *out, int max) {
  const int64_t tick = tickKHz(fm);
  int64_t first = 0;
  int64_t count = 0;
  if (widthPx < 2 || out == NULL || max <= 0 ||
      !bandTicks(lowKHz, highKHz, tick, &first, &count)) {
    return 0;
  }
  const int64_t loop = loopKHz(count, tick);
  const int32_t centre = widthPx / 2;
  /* The middle of the dummies after the band's last tick, the nearest way
   * round, then one loop either side of that for a band shorter than the
   * row. */
  const int64_t seam =
      first + (count - 1) * tick + (SCALE_SEAM_TICKS + 1) * tick / 2;
  const int64_t d =
      floorMod(seam - (int64_t)tunedKHz + loop / 2, loop) - loop / 2;
  int n = 0;
  for (int64_t m = -1; m <= 1 && n < max && n < SCALE_MAX_SEAMS; m++) {
    const int32_t x =
        centre + (int32_t)floorDiv((d + m * loop) * SCALE_TICK_PX, tick);
    if (x >= 0 && x <= widthPx - 1) {
      out[n++] = (int16_t)x;
    }
  }
  return n;
}
