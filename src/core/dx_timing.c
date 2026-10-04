/* How long RDS takes on each channel a DX scan stops on. */
#include "dx_timing.h"

#include "signal.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

void dxTimingStart(DxTiming *t, uint32_t khz, uint32_t nowMs) {
  if (t == NULL) {
    return;
  }
  memset(t, 0, sizeof(*t));
  t->active = true;
  t->khz = khz;
  t->startMs = nowMs;
  t->syncMs = DX_TIMING_NEVER;
  t->groupMs = DX_TIMING_NEVER;
  t->cleanAMs = DX_TIMING_NEVER;
  t->piMs = DX_TIMING_NEVER;
  t->levelTenths = INT16_MIN;
}

/* Milliseconds since the dwell began, held under DX_TIMING_NEVER. */
static uint16_t since(const DxTiming *t, uint32_t nowMs) {
  const uint32_t ms = nowMs - t->startMs;
  return (uint16_t)(ms < DX_TIMING_NEVER ? ms : DX_TIMING_NEVER - 1u);
}

static void first(uint16_t *at, bool happened, uint16_t ms) {
  if (happened && *at == DX_TIMING_NEVER) {
    *at = ms;
  }
}

void dxTimingFeed(DxTiming *t, uint32_t dialKHz, const RdsInfo *rds,
                  int16_t levelTenths, uint16_t widthKHz, uint32_t nowMs) {
  if (t == NULL || !t->active || dialKHz != t->khz) {
    return;
  }
  if (levelTenths > t->levelTenths) {
    t->levelTenths = levelTenths;
  }
  t->widthKHz = widthKHz;
  if (rds == NULL) {
    return;
  }
  const uint16_t ms = since(t, nowMs);
  first(&t->syncMs, rds->synchronised, ms);
  first(&t->groupMs, rds->hasBlockErrors, ms);
  first(&t->cleanAMs, rds->hasPiHeard, ms);
  if (t->piMs == DX_TIMING_NEVER && (rds->hasPi || rds->piZero)) {
    t->piMs = ms;
    t->pi = rds->hasPi ? rds->pi : 0;
  }
}

bool dxTimingEnd(DxTiming *t, uint32_t nowMs) {
  if (t == NULL || !t->active) {
    return false;
  }
  t->active = false;
  t->dwellMs = since(t, nowMs);
  return t->syncMs != DX_TIMING_NEVER;
}

const char *dxTimingCsvHeader(void) {
  return "utc,khz,pi,level_dbuv,width_khz,sync_ms,group_ms,clean_a_ms,pi_ms,"
         "dwell_ms\n";
}

/* A time as a field: empty when it never came. */
static void msField(uint16_t ms, char *out, size_t cap) {
  if (ms == DX_TIMING_NEVER) {
    out[0] = '\0';
  } else {
    snprintf(out, cap, "%u", (unsigned)ms);
  }
}

size_t dxTimingCsvLine(const DxTiming *t, uint32_t utc, char *out, size_t cap) {
  if (t == NULL || out == NULL) {
    return 0;
  }
  /* Wide enough for any year a 32 bit epoch reaches. */
  char when[40] = "";
  if (utc != 0) {
    const time_t at = (time_t)utc;
    struct tm tmUtc;
    if (gmtime_r(&at, &tmUtc) != NULL) {
      snprintf(when, sizeof(when), "%04d-%02d-%02dT%02d:%02d:%02dZ",
               tmUtc.tm_year + 1900, tmUtc.tm_mon + 1, tmUtc.tm_mday,
               tmUtc.tm_hour, tmUtc.tm_min, tmUtc.tm_sec);
    }
  }
  char pi[8] = "";
  if (t->pi != 0) {
    snprintf(pi, sizeof(pi), "%04X", t->pi);
  }
  char level[12] = "";
  if (t->levelTenths != INT16_MIN) {
    signalFormatLevel(t->levelTenths, level, sizeof(level));
  }
  char sync[8];
  char group[8];
  char cleanA[8];
  char piAt[8];
  msField(t->syncMs, sync, sizeof(sync));
  msField(t->groupMs, group, sizeof(group));
  msField(t->cleanAMs, cleanA, sizeof(cleanA));
  msField(t->piMs, piAt, sizeof(piAt));
  return (size_t)snprintf(out, cap, "%s,%u,%s,%s,%u,%s,%s,%s,%s,%u\n", when,
                          (unsigned)t->khz, pi, level, (unsigned)t->widthKHz,
                          sync, group, cleanA, piAt, (unsigned)t->dwellMs);
}
