/**
 * @file squelch.c
 * @brief Implementation of the squelch.
 */
#include "squelch.h"

#include <stddef.h>
#include <string.h>

/** The lowest a manual threshold goes: always open. */
#define MANUAL_MIN_TENTHS (-100)

/** The highest it goes, which is above anything the chip reports. */
#define MANUAL_MAX_TENTHS 920

const char *squelchModeName(SquelchMode mode) {
  switch (mode) {
    case SQUELCH_OFF:
      return "Off";
    case SQUELCH_AUTO:
      return "Auto";
    case SQUELCH_MANUAL:
      return "Manual";
    case SQUELCH_MODE_COUNT:
    default:
      return "";
  }
}

void squelchDefaults(SquelchConfig *out) {
  if (out == NULL) {
    return;
  }
  out->fmNoiseTenths = 120;
  out->fmMultipathTenths = 230;
  out->fmOffsetTenths = 100;
  out->amNoiseTenths = 120;
  out->amOffsetTenths = 20;
  out->holdMs = 1000;
}

void squelchInit(Squelch *s) {
  if (s == NULL) {
    return;
  }
  memset(s, 0, sizeof(*s));
  s->open = true;
}

int16_t squelchThresholdFromPot(uint16_t raw, uint16_t rawMin,
                                uint16_t rawMax) {
  /* An uncalibrated radio uses the whole converter range, which is what this
   * did before the ends could be given at all. */
  if (rawMax <= rawMin) {
    rawMin = 0;
    rawMax = 4095;
  }
  if (raw < rawMin) {
    raw = rawMin;
  }
  if (raw > rawMax) {
    raw = rawMax;
  }
  int32_t span = MANUAL_MAX_TENTHS - MANUAL_MIN_TENTHS;
  int32_t along = (int32_t)raw - rawMin;
  int32_t width = (int32_t)rawMax - rawMin;
  return (int16_t)(MANUAL_MIN_TENTHS + (along * span) / width);
}

/** Whether one reading is good enough to listen to. */
static bool readingIsGood(const SquelchConfig *cfg, SquelchMode mode,
                          BandId band, const SquelchReading *reading,
                          int16_t thresholdTenths) {
  if (mode == SQUELCH_MANUAL) {
    /* At the very bottom of the travel the squelch is off, whatever the
     * signal is doing. Without that there is no way to hear a weak station
     * on purpose. */
    if (thresholdTenths <= MANUAL_MIN_TENTHS) {
      return true;
    }
    return reading->levelTenths > thresholdTenths;
  }

  /* Auto. The two sides report different things, so they need different
   * rules. Multipath is an FM idea and the tuner does not report it on AM. */
  int16_t offset = reading->offsetTenths < 0 ? (int16_t)-reading->offsetTenths
                                             : reading->offsetTenths;
  if (bandModulation(band) == MODULATION_FM) {
    return reading->noiseTenths < cfg->fmNoiseTenths &&
           reading->multipathTenths < cfg->fmMultipathTenths &&
           offset < (int16_t)cfg->fmOffsetTenths;
  }
  return reading->noiseTenths < cfg->amNoiseTenths &&
         offset < (int16_t)cfg->amOffsetTenths;
}

bool squelchUpdate(Squelch *s, const SquelchConfig *cfg, SquelchMode mode,
                   BandId band, const SquelchReading *reading,
                   int16_t thresholdTenths, uint32_t nowMs) {
  SquelchConfig defaults;
  if (cfg == NULL) {
    squelchDefaults(&defaults);
    cfg = &defaults;
  }
  if (s == NULL || reading == NULL) {
    return true;
  }

  if (mode == SQUELCH_OFF) {
    s->open = true;
    s->waiting = false;
    s->lost = false;
    s->mode = mode;
    s->band = band;
    return true;
  }

  /* A hold started under one rule must not be counted towards a decision made
   * under another. Changing band swaps the rule, and changing mode swaps it
   * entirely, so either one starts the wait again. */
  if (s->mode != mode || s->band != band) {
    s->mode = mode;
    s->band = band;
    s->waiting = false;
  }

  if (!reading->valid) {
    /* A reading that did not come back says nothing about the signal, so it
     * neither shuts the audio nor cancels a hold that is already running.
     * Cancelling it let one failed read in the middle of a bad patch push the
     * shut back indefinitely.
     *
     * But it cannot leave the audio shut for ever either. If the reads keep
     * failing there is no evidence for silence any more, and a radio that
     * stays silent because its tuner stopped answering has turned a fault in
     * one place into the listener's problem. So after as long as the hold, it
     * opens. */
    if (!s->lost) {
      s->lost = true;
      s->lostMs = nowMs;
    } else if ((uint32_t)(nowMs - s->lostMs) >= cfg->holdMs) {
      s->open = true;
      s->waiting = false;
    }
    return s->open;
  }
  s->lost = false;

  bool good = readingIsGood(cfg, mode, band, reading, thresholdTenths);

  if (good) {
    /* Opening is immediate. Waiting to open would clip the front of every
     * station as the dial passes across it. */
    s->waiting = false;
    s->open = true;
    return true;
  }

  /* Shutting waits, so a signal sitting on the threshold does not chatter. */
  if (!s->open) {
    return false;
  }
  if (!s->waiting) {
    s->waiting = true;
    s->badMs = nowMs;
    return true;
  }
  if ((uint32_t)(nowMs - s->badMs) >= cfg->holdMs) {
    s->waiting = false;
    s->open = false;
    return false;
  }
  return true;
}
