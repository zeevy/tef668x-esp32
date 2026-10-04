/* Implementation of auto off. */
#include "auto_off.h"

#include <stddef.h>

bool autoOffMinutesOk(uint16_t minutes) {
  return minutes <= AUTO_OFF_MAX_MINUTES;
}

void autoOffInit(AutoOff *a, uint16_t minutes, uint32_t nowMs) {
  if (a == NULL) {
    return;
  }
  a->minutes = 0;
  a->lastUseMs = nowMs;
  autoOffSetMinutes(a, minutes, nowMs);
}

void autoOffSetMinutes(AutoOff *a, uint16_t minutes, uint32_t nowMs) {
  if (a == NULL) {
    return;
  }
  /* A time nobody can have chosen is off, not the nearest one: it can only
   * come from a settings blob gone wrong. */
  a->minutes = autoOffMinutesOk(minutes) ? minutes : 0;
  a->lastUseMs = nowMs;
}

void autoOffUsed(AutoOff *a, uint32_t nowMs) {
  if (a != NULL) {
    a->lastUseMs = nowMs;
  }
}

bool autoOffLeftMs(const AutoOff *a, uint32_t nowMs, uint32_t *out) {
  if (a == NULL || a->minutes == 0) {
    return false;
  }
  const uint32_t total = (uint32_t)a->minutes * 60000UL;
  /* Unsigned, so the count carries on across the 49 day wrap of the clock. */
  const uint32_t idle = nowMs - a->lastUseMs;
  if (out != NULL) {
    *out = idle >= total ? 0 : total - idle;
  }
  return true;
}

AutoOffPhase autoOffPhase(const AutoOff *a, uint32_t nowMs) {
  uint32_t left = 0;
  if (!autoOffLeftMs(a, nowMs, &left)) {
    return AUTO_OFF_AWAKE;
  }
  if (left == 0) {
    return AUTO_OFF_SLEEP;
  }
  if (left <= AUTO_OFF_FADE_MS) {
    return AUTO_OFF_FADE;
  }
  if (left <= AUTO_OFF_WARN_MS) {
    return AUTO_OFF_WARN;
  }
  return AUTO_OFF_AWAKE;
}
