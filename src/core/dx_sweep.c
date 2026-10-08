/* Implementation of the DX level sweep. */
#include "dx_sweep.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/bytes.h"
#include "core/strings.h"

int16_t dxSweepMean(const int16_t *reads, uint8_t n) {
  if (reads == NULL || n == 0) {
    return DX_SWEEP_NO_READING;
  }
  int32_t sum = 0;
  for (uint8_t i = 0; i < n; i++) {
    sum += reads[i];
  }
  /* Rounded half away from zero, so -0.5 is -1 as 0.5 is 1. */
  const int32_t half = n / 2;
  return (int16_t)(sum >= 0 ? (sum + half) / n : (sum - half) / n);
}

bool dxSweepSameChannels(const DxSweep *a, const DxSweep *b) {
  return a != NULL && b != NULL && a->count > 0 && a->count == b->count &&
         a->lowKHz == b->lowKHz && a->stepKHz == b->stepKHz &&
         a->widthKHz == b->widthKHz;
}

static int compareLevel(const void *x, const void *y) {
  const int16_t a = *(const int16_t *)x;
  const int16_t b = *(const int16_t *)y;
  return (a > b) - (a < b);
}

int16_t dxSweepFloor(const DxSweep *s) {
  if (s == NULL || s->count == 0 || s->count > DX_SWEEP_MAX) {
    return DX_SWEEP_NO_READING;
  }
  int16_t sorted[DX_SWEEP_MAX];
  uint16_t n = 0;
  for (uint16_t i = 0; i < s->count; i++) {
    if (s->level[i] != DX_SWEEP_NO_READING) {
      sorted[n++] = s->level[i];
    }
  }
  if (n == 0) {
    return DX_SWEEP_NO_READING;
  }
  qsort(sorted, n, sizeof(sorted[0]), compareLevel);
  /* The reading a quarter of the way up: at or above it are three
   * quarters of the channels, at or below it a quarter. */
  return sorted[(n - 1) / 4];
}

uint8_t dxSweepMedian(const DxSweep *items, uint8_t count, const DxSweep *like,
                      DxSweep *out) {
  if (items == NULL || like == NULL || out == NULL) {
    return 0;
  }
  const DxSweep *use[DX_SWEEP_KEEP];
  uint8_t n = 0;
  for (uint8_t i = 0; i < count && i < DX_SWEEP_KEEP; i++) {
    if (dxSweepSameChannels(&items[i], like)) {
      use[n++] = &items[i];
    }
  }
  if (n == 0) {
    return 0;
  }
  /* Written straight into `out`, since a whole sweep is too big to copy on
   * the loop task's stack. */
  out->timeKnown = like->timeKnown;
  out->at = like->at;
  out->lowKHz = like->lowKHz;
  out->stepKHz = like->stepKHz;
  out->count = like->count;
  out->widthKHz = like->widthKHz;
  for (uint16_t c = 0; c < out->count; c++) {
    int16_t v[DX_SWEEP_KEEP];
    uint8_t m = 0;
    for (uint8_t i = 0; i < n; i++) {
      if (use[i]->level[c] != DX_SWEEP_NO_READING) {
        v[m++] = use[i]->level[c];
      }
    }
    if (m == 0) {
      out->level[c] = DX_SWEEP_NO_READING;
      continue;
    }
    qsort(v, m, sizeof(v[0]), compareLevel);
    if (m % 2 == 1) {
      out->level[c] = v[m / 2];
    } else {
      const int16_t pair[2] = {v[m / 2 - 1], v[m / 2]};
      out->level[c] = dxSweepMean(pair, 2);
    }
  }
  return n;
}

void dxSweepPeak(DxSweep *peak, const DxSweep *s) {
  if (peak == NULL || s == NULL) {
    return;
  }
  if (!dxSweepSameChannels(peak, s)) {
    *peak = *s;
    return;
  }
  for (uint16_t c = 0; c < s->count; c++) {
    if (s->level[c] != DX_SWEEP_NO_READING &&
        (peak->level[c] == DX_SWEEP_NO_READING ||
         s->level[c] > peak->level[c])) {
      peak->level[c] = s->level[c];
    }
  }
  peak->timeKnown = s->timeKnown;
  peak->at = s->at;
}

int16_t dxSweepChannelOf(const DxSweep *s, uint32_t khz) {
  if (s == NULL || s->count == 0 || s->stepKHz == 0 || khz < s->lowKHz ||
      (khz - s->lowKHz) % s->stepKHz != 0) {
    return -1;
  }
  const uint32_t i = (khz - s->lowKHz) / s->stepKHz;
  return i < s->count ? (int16_t)i : (int16_t)-1;
}

uint32_t dxSweepKHzOf(const DxSweep *s, uint16_t i) {
  if (s == NULL || i >= s->count) {
    return 0;
  }
  return s->lowKHz + (uint32_t)i * s->stepKHz;
}

bool dxSweepAge(uint32_t atUtc, uint32_t nowUtc, char *out, size_t cap) {
  if (out == NULL || cap == 0) {
    return false;
  }
  out[0] = '\0';
  if (nowUtc < atUtc) {
    return false;
  }
  const uint32_t s = nowUtc - atUtc;
  if (s < 60u) {
    snprintf(out, cap, "%s", txt(STR_DX_NOW));
  } else if (s < 3600u) {
    snprintf(out, cap, txt(STR_DX_FMT_MIN_AGO), (unsigned)(s / 60u));
  } else if (s < 86400u) {
    snprintf(out, cap, txt(STR_DX_FMT_H_AGO), (unsigned)(s / 3600u));
  } else {
    snprintf(out, cap, txt(STR_DX_FMT_D_AGO), (unsigned)(s / 86400u));
  }
  return true;
}

void dxSweepKeep(DxSweepHistory *h, const DxSweep *s) {
  if (h == NULL || s == NULL) {
    return;
  }
  const uint8_t keep =
      h->count < DX_SWEEP_KEEP ? h->count : (uint8_t)(DX_SWEEP_KEEP - 1);
  memmove(&h->item[1], &h->item[0], (size_t)keep * sizeof(DxSweep));
  h->item[0] = *s;
  h->count = (uint8_t)(keep + 1);
}

/*
 * The file: "DXSW", a version byte and the number of sweeps, then each
 * sweep newest first: whether its time is known, the time, the first
 * channel, the step, the count, the width and how long it took, then its
 * levels. Little endian throughout.
 */
static const uint8_t kMagic[4] = {'D', 'X', 'S', 'W'};
#define FILE_VERSION 1
#define FILE_HEAD 6
#define SWEEP_HEAD 17

size_t dxSweepEncodedSize(const DxSweepHistory *h) {
  if (h == NULL) {
    return 0;
  }
  size_t size = FILE_HEAD;
  for (uint8_t i = 0; i < h->count && i < DX_SWEEP_KEEP; i++) {
    size += SWEEP_HEAD + 2u * h->item[i].count;
  }
  return size;
}

size_t dxSweepEncode(const DxSweepHistory *h, uint8_t *out, size_t cap) {
  if (h == NULL || out == NULL || h->count > DX_SWEEP_KEEP) {
    return 0;
  }
  const size_t size = dxSweepEncodedSize(h);
  if (size > cap) {
    return 0;
  }
  memcpy(out, kMagic, sizeof(kMagic));
  out[4] = FILE_VERSION;
  out[5] = h->count;
  uint8_t *p = out + FILE_HEAD;
  for (uint8_t i = 0; i < h->count; i++) {
    const DxSweep *s = &h->item[i];
    p[0] = s->timeKnown ? 1 : 0;
    putU32(p + 1, s->at);
    putU32(p + 5, s->lowKHz);
    putU16(p + 9, s->stepKHz);
    putU16(p + 11, s->count);
    putU16(p + 13, s->widthKHz);
    putU16(p + 15, s->tookMs);
    p += SWEEP_HEAD;
    for (uint16_t c = 0; c < s->count; c++) {
      putU16(p, (uint16_t)s->level[c]);
      p += 2;
    }
  }
  return size;
}

bool dxSweepDecode(const uint8_t *in, size_t len, DxSweepHistory *out) {
  if (out == NULL) {
    return false;
  }
  memset(out, 0, sizeof(*out));
  if (in == NULL || len < FILE_HEAD || memcmp(in, kMagic, sizeof(kMagic)) ||
      in[4] != FILE_VERSION || in[5] > DX_SWEEP_KEEP) {
    return false;
  }
  const uint8_t n = in[5];
  size_t at = FILE_HEAD;
  for (uint8_t i = 0; i < n; i++) {
    if (len - at < SWEEP_HEAD) {
      memset(out, 0, sizeof(*out));
      return false;
    }
    const uint8_t *p = in + at;
    DxSweep *s = &out->item[i];
    s->timeKnown = p[0] == 1;
    s->at = getU32(p + 1);
    s->lowKHz = getU32(p + 5);
    s->stepKHz = getU16(p + 9);
    s->count = getU16(p + 11);
    s->widthKHz = getU16(p + 13);
    s->tookMs = getU16(p + 15);
    at += SWEEP_HEAD;
    if (p[0] > 1 || s->stepKHz == 0 || s->count == 0 ||
        s->count > DX_SWEEP_MAX || len - at < 2u * s->count) {
      memset(out, 0, sizeof(*out));
      return false;
    }
    for (uint16_t c = 0; c < s->count; c++) {
      s->level[c] = (int16_t)getU16(in + at);
      at += 2;
    }
  }
  if (at != len) {
    memset(out, 0, sizeof(*out));
    return false;
  }
  out->count = n;
  return true;
}

bool dxSweepRange(BandId band, const BandPlanConfig *plan, uint32_t dialKHz,
                  uint32_t spanKHz, DxSweepRange *out) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (plan == NULL || out == NULL || !bandLimits(band, plan, &lo, &hi) ||
      hi < lo) {
    return false;
  }
  const uint16_t step = bandDefaultStep(band, plan);
  if (step == 0) {
    return false;
  }
  const uint32_t all = (hi - lo) / step + 1;
  const uint32_t want =
      spanKHz == 0 || spanKHz / step + 1 >= all ? all : spanKHz / step + 1;
  if (want > DX_SWEEP_MAX) {
    return false;
  }
  const uint32_t dial = dialKHz < lo ? 0 : (dialKHz - lo + step / 2) / step;
  uint32_t first = dial > want / 2 ? dial - want / 2 : 0;
  if (first + want > all) {
    first = all - want;
  }
  out->lowKHz = lo + first * step;
  out->stepKHz = step;
  out->count = (uint16_t)want;
  return true;
}

bool dxSweepRangeFits(BandId band, const BandPlanConfig *plan,
                      const DxSweepRange *r) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (plan == NULL || r == NULL || r->count == 0 || r->count > DX_SWEEP_MAX ||
      r->stepKHz == 0 || !bandLimits(band, plan, &lo, &hi)) {
    return false;
  }
  const uint32_t last = r->lowKHz + (uint32_t)(r->count - 1u) * r->stepKHz;
  if (r->lowKHz < lo || last > hi) {
    return false;
  }
  return bandModulation(band) != MODULATION_FM ||
         ((r->lowKHz % 10) == 0 && (r->stepKHz % 10) == 0);
}
