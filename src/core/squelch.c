/* Implementation of the squelch. */
#include "squelch.h"

#include <stddef.h>
#include <string.h>

/*
 * The level an FM signal has to reach, in tenths of a dBuV.
 *
 * Measured on this radio on 13 September 2026, and the working is in
 * test/fixtures/squelch/README.md.
 *
 * The shoulder of a strong station passes every other test the squelch
 * applies. There is real signal in the channel, because the sidebands of the
 * station next door reach into it, so the noise reads low and the multipath
 * reads low and everything says station. On 102.0 MHz, beside 101.9, the
 * reading passed the noise and multipath test on 10 of 12 samples.
 *
 * Level is what separates them, and only once it is smoothed. Raw, the
 * shoulder reaches 25.8 dBuV, which is above the 19.6 dBuV of the weakest
 * station in the seek sweep, so a floor on the reading either lets the
 * shoulder through or mutes a real station. Smoothed it collapses to 18.2,
 * because a station holds its level and a shoulder does not.
 *
 * 15.0 sits below every station ever measured here and above most of the
 * shoulder. It is deliberately not higher: 25 would shut the shoulder
 * completely but would have muted that 19.6 dBuV station, and silence on a
 * station you can hear is the worse fault. The cost is that the shoulder
 * still opens about one reading in ten, against nearly all of them before.
 *
 * The gap between 18.2 and 19.6 is 1.4 dB across two days, which is not a
 * comfortable margin. That is recorded in the fixture README rather than
 * smoothed over.
 */
#define SQUELCH_FM_LEVEL_FLOOR_TENTHS (SQUELCH_FM_LEVEL_FLOOR_DBUV * 10)

/* The lowest a manual threshold goes: always open. */
#define MANUAL_MIN_TENTHS (-100)

/* The highest it goes, which is above anything the chip reports. */
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
  out->fmLevelFloorTenths = SQUELCH_FM_LEVEL_FLOOR_TENTHS;
}

void squelchRetuned(Squelch *s) {
  if (s != NULL) {
    signalAverageReset(&s->level);
    s->levelSamples = 0;
    s->goodHistory = 0;
  }
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

static uint8_t goodInWindow(const Squelch *s) {
  uint8_t n = 0;
  for (uint8_t i = 0; i < SQUELCH_OPEN_WINDOW; i++) {
    if (s->goodHistory & (uint8_t)(1u << i)) {
      n++;
    }
  }
  return n;
}

static bool readingIsGood(const SquelchConfig *cfg, SquelchMode mode,
                          BandId band, const SquelchReading *reading,
                          int16_t thresholdTenths, int16_t smoothedTenths,
                          bool levelSettled) {
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
    /* The floor is FM only, for the same reason seek's is. It is a number
     * fitted to FM captures on an FM level scale, and there is no AM capture
     * to fit an AM one to. Applying it there would be a guessed threshold,
     * and a guessed threshold switches a feature off without saying so. */
    if (levelSettled && cfg->fmLevelFloorTenths != SQUELCH_LEVEL_FLOOR_OFF &&
        smoothedTenths < cfg->fmLevelFloorTenths) {
      return false;
    }
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

  /* Fed before anything else, including the Off shortcut below.
   *
   * The radio comes up with the squelch off, so an average that only ran in
   * Auto would be empty the moment somebody switched to it, and the first
   * reading after the switch would become the whole average with no history
   * behind it. Keeping it fed costs one add per reading and means Auto
   * starts knowing what the channel has been doing. */
  int16_t smoothed = reading->levelTenths;
  if (reading->valid) {
    smoothed = signalAverage(&s->level, reading->levelTenths);
    if (s->levelSamples < SQUELCH_LEVEL_SETTLE) {
      s->levelSamples++;
    }
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
    /* The history goes too, for the same reason the hold does. Readings
     * judged good under the FM rule say nothing about whether they would
     * pass the AM one, or the person's own manual threshold. */
    s->goodHistory = 0;
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

  bool good = readingIsGood(cfg, mode, band, reading, thresholdTenths, smoothed,
                            s->levelSamples >= SQUELCH_LEVEL_SETTLE);

  /* The newest reading goes in at the bottom and the rest shift up. Only the
   * lowest SQUELCH_OPEN_WINDOW bits are ever read, so the rest fall off the
   * top and nothing needs clearing. */
  s->goodHistory = (uint8_t)((s->goodHistory << 1) | (good ? 1u : 0u));

  if (good) {
    if (s->open) {
      /* Already open, so one good reading is enough to call off a shut that
       * is being held. A signal sitting exactly on the limit alternates good
       * and bad from one reading to the next, and it has to stay open: it is
       * a station somebody is listening to, and muting it is the fault this
       * squelch is least allowed to have. */
      s->waiting = false;
      return true;
    }
    if (goodInWindow(s) >= SQUELCH_OPEN_READINGS) {
      /* Shut, so it takes more than one. Waiting long to open would clip the
       * front of every station as the dial passes across it, so the wait is
       * one extra reading and no more: enough to throw away the isolated
       * good reading the channel beside a strong station produces, not
       * enough to be heard. */
      s->open = true;
      return true;
    }
    return false;
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
