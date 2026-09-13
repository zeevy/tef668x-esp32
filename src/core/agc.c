/**
 * @file agc.c
 * @brief Implementation of the volume AGC.
 */
#include "agc.h"

#include <stddef.h>

/**
 * Where each whole dB of gain starts, as a ratio scaled by ten thousand.
 *
 * The gain the average asks for is 20 log10(target / average), rounded to a
 * whole dB. That is worked out here without floating point, because the
 * answer only ever spans the 21 values from AGC_MAX_CUT to AGC_BOOST_MAX and
 * a table of boundaries settles it exactly.
 *
 * Entry i is for a gain of AGC_MAX_CUT + i, and holds 10^(-(g - 0.5) / 20)
 * times ten thousand. The wanted gain is the largest g whose average is at or
 * below target times that entry, which is the same rounding the logarithm
 * would do.
 *
 * Checked against the capture rather than trusted: replaying the 168 readings
 * in test/fixtures/agc/05-target50-boost6-final.log that have a settled
 * average, this reproduces the gain the prototype worked out with log10f on
 * every one of them.
 */
static const uint16_t kGainBoundary[] = {
    42170, 37584, 33497, 29854, 26607, 23714, 21135, 18836, 16788, 14962, 13335,
    11885, 10593, 9441,  8414,  7499,  6683,  5957,  5309,  4732,  4217};

/** How many entries that table has, which is the span it covers. */
#define AGC_GAIN_SPAN ((int)(sizeof(kGainBoundary) / sizeof(kGainBoundary[0])))

/** Whether the AGC has anything to do at all. */
static bool configOn(const AgcConfig *cfg) {
  if (cfg == NULL || cfg->targetPercent == 0) {
    return false;
  }
  /* A target outside the range this was set up for is not acted on. It can
   * only arrive from a settings blob that has gone wrong, and aiming at it
   * would quietly drive the volume somewhere nothing asked for. Off is the
   * safe reading of a value nobody can have chosen. */
  return cfg->targetPercent >= AGC_TARGET_MIN &&
         cfg->targetPercent <= AGC_TARGET_MAX;
}

/** The most the AGC may add. */
static int8_t boostCeiling(const AgcConfig *cfg) {
  if (cfg == NULL || cfg->boostDb > AGC_BOOST_MAX) {
    /* A boost nobody could have asked for means no boost, the same way a
     * target nobody could have asked for means the AGC is off. Holding it at
     * the maximum instead would turn a corrupt settings blob into the loudest
     * setting the radio has, which is the wrong direction to fail in. */
    return 0;
  }
  return (int8_t)cfg->boostDb;
}

/** Whether this reading is worth putting into the average. */
static bool readingIsUsable(const AgcReading *r) {
  if (r == NULL || !r->valid || !r->listening || !r->fresh) {
    return false;
  }
  if (r->levelTenths < AGC_MIN_SIGNAL * 10) {
    return false;
  }
  /* Noise reads as heavy modulation, so an empty channel has to be rejected
   * or the AGC turns the volume down on it. The two sides report different
   * things in that field, so the test is FM only; the level gate above is
   * what carries the AM side. */
  if (r->fm && r->noiseTenths > AGC_MAX_NOISE_TENTHS) {
    return false;
  }
  return r->modulationPercent >= AGC_MOD_MIN &&
         r->modulationPercent <= AGC_MOD_MAX;
}

void agcInit(Agc *a) {
  if (a == NULL) {
    return;
  }
  a->averageMilli = 0;
  a->ticks = 0;
  a->idle = 0;
  a->gainDb = 0;
}

void agcRetuned(Agc *a) {
  if (a == NULL) {
    return;
  }
  a->averageMilli = 0;
  a->ticks = 0;
  a->idle = 0;
}

bool agcSettled(const Agc *a) {
  return a != NULL && a->ticks >= AGC_MIN_TICKS;
}

int16_t agcAverageTenths(const Agc *a) {
  return a != NULL ? (int16_t)(a->averageMilli / 100) : (int16_t)0;
}

int8_t agcGain(const Agc *a) {
  return a != NULL ? a->gainDb : (int8_t)0;
}

int8_t agcWantedGain(const Agc *a, const AgcConfig *cfg) {
  if (a == NULL || !configOn(cfg) || !agcSettled(a) || a->averageMilli <= 0) {
    return 0;
  }
  int8_t top = boostCeiling(cfg);
  /* From the most gain downwards, so the first entry that fits is the
   * rounded answer. */
  for (int i = AGC_GAIN_SPAN - 1; i >= 0; i--) {
    int8_t g = (int8_t)(AGC_MAX_CUT + i);
    if (g > top) {
      continue;
    }
    /* average <= target * boundary, with both sides scaled the same way.
     * The average is in thousandths and the boundary by ten thousand, so the
     * target is multiplied by a thousand to match. Both sides reach about
     * two thousand million at the extremes, so the comparison is done in 64
     * bits rather than relying on that staying inside an int32. */
    int64_t left = (int64_t)a->averageMilli * 10000;
    int64_t right =
        (int64_t)cfg->targetPercent * 1000 * (int64_t)kGainBoundary[i];
    if (left <= right) {
      return g;
    }
  }
  return AGC_MAX_CUT;
}

int8_t agcUpdate(Agc *a, const AgcConfig *cfg, const AgcReading *r) {
  if (a == NULL) {
    return 0;
  }
  if (!configOn(cfg)) {
    /* Switched off, or asked for a target nobody could have chosen. The gain
     * goes with it rather than being left applied, because nothing would move
     * it again, and it goes at once rather than a step at a time: whoever
     * applies the volume already ramps it, and stepping here would leave the
     * AGC quietly still working for twelve seconds after being switched off.
     *
     * The average goes too. Switching straight back on would otherwise take
     * the settled path on its first call and jump towards a gain worked out
     * from whatever was playing before. */
    agcRetuned(a);
    a->gainDb = 0;
    return 0;
  }

  bool usable = readingIsUsable(r);

  if (usable) {
    a->idle = 0;
    int32_t mod = (int32_t)r->modulationPercent * 1000;
    if (a->ticks == 0) {
      a->averageMilli = mod;
      a->ticks = 1;
    } else {
      int32_t div = a->ticks < AGC_SETTLE_TICKS ? AGC_FAST_DIV : AGC_SLOW_DIV;
      a->averageMilli += (mod - a->averageMilli) / div;
      if (a->ticks < AGC_SETTLE_TICKS) {
        a->ticks++;
      }
    }
  } else if (a->idle < AGC_IDLE_TICKS) {
    a->idle++;
  }

  if (!agcSettled(a)) {
    /* No usable average yet, so there is nothing to aim at.
     *
     * Once nothing has been measurable here for a long time the gain is
     * walked back to zero instead, which is the case of tuning onto a station
     * the AGC cannot read: without it the last station's cut sits on the new
     * one and it plays quiet for as long as it is tuned in, with nothing to
     * say why.
     *
     * This release only applies before the average has settled, and that is
     * deliberate rather than an oversight. Once there is an average, a
     * station that dips into the noise or is muted for a while keeps the gain
     * its own average asked for, because that average is still the right
     * answer for the station being listened to. Releasing there would walk
     * the volume up during every quiet passage and back down afterwards. */
    if (a->idle >= AGC_IDLE_TICKS && a->gainDb != 0) {
      a->gainDb =
          (int8_t)(a->gainDb + (a->gainDb < 0 ? AGC_STEP_DB : -AGC_STEP_DB));
    }
    return a->gainDb;
  }

  /* Past here the gain moves on every call, measurable or not, so a station
   * that dips into the noise carries on towards the level its average already
   * asked for instead of stopping part way there. */
  int8_t wanted = agcWantedGain(a, cfg);
  int16_t error = (int16_t)(wanted - a->gainDb);
  if (error > -AGC_DEADBAND_DB && error < AGC_DEADBAND_DB) {
    return a->gainDb;
  }
  /* One step per call, so the change is heard as a fade and not a jump. */
  a->gainDb = (int8_t)(a->gainDb + (error > 0 ? AGC_STEP_DB : -AGC_STEP_DB));
  return a->gainDb;
}
