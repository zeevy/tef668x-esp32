/**
 * @file signal.c
 * @brief Implementation of the derived signal figures.
 */
#include "signal.h"

#include <stdio.h>

#include <stddef.h>

/** The lowest level the reference firmware will consider, in tenths. */
#define LEVEL_FLOOR (-200)

/** The highest, in tenths. */
#define LEVEL_CEILING 1200

int8_t signalSnrDb(int16_t levelTenths, uint16_t noiseTenths, bool fm) {
  if (levelTenths < LEVEL_FLOOR) {
    levelTenths = LEVEL_FLOOR;
  }
  if (levelTenths > LEVEL_CEILING) {
    levelTenths = LEVEL_CEILING;
  }

  /* The AM noise field is on a different scale. The reference divides it by
   * fifty, as integers, before the line. */
  int32_t noise = fm ? (int32_t)noiseTenths : (int32_t)(noiseTenths / 50);

  /* The same line as the reference, in fixed point rather than floating.
   *
   * Scaled by a hundred million, which needs 64 bits: the largest term is
   * 1200 times 46222375, about 55 thousand million. Scaling by a million
   * instead keeps it in 32 bits, but rounding the noise constant from
   * 0.0082495118 to 0.00825 moves the answer by a whole dB in seven thousand
   * of six million combinations, six of them straddling the test that opens
   * the filter. A threshold that quietly switches a feature off fails without
   * saying so, so the precision is worth the wider arithmetic on something
   * that runs ten times a second.
   *
   * Checked against the floating point original over six million
   * combinations of level and noise on each band: they agree everywhere on
   * AM, and everywhere on FM but one, at 49.4 dBuV with 337 per cent noise,
   * where the last fraction falls the other way. */
  int64_t scaled = (int64_t)levelTenths * 46222375 - (int64_t)noise * 8249512;
  int32_t snr = (int32_t)(scaled / 1000000000) + 10;

  /* Only the bottom needs clamping. With the level already held at 1200
   * tenths the highest this can reach is 65, so there is nothing to do at the
   * top and a clamp there would be a branch no test could ever take. The
   * bottom reaches -539 when a floor level meets a noise reading that has
   * wrapped, which is a real thing this tuner does. */
  if (snr < -128) {
    snr = -128;
  }
  return (int8_t)snr;
}

/** Nearest whole dB, taking the sign before the division. */
static int16_t wholeDb(int16_t tenths) {
  return (int16_t)(tenths >= 0 ? (tenths + 5) / 10 : (tenths - 5) / 10);
}

int16_t signalDisplayLevel(SignalDisplay *d, int16_t smoothedTenths,
                           bool fresh) {
  if (d == NULL) {
    return wholeDb(smoothedTenths);
  }
  if (d->waitingForStation) {
    if (!fresh) {
      /* Hold what is on the screen. The wait is about a tenth of a second,
       * and blanking or jumping for that long on every turn of the knob
       * would be worse than one stale number. */
      return d->started ? d->shownDb : wholeDb(smoothedTenths);
    }
    /* The reading is from where the dial is now, so take it as it stands
     * instead of making it climb out of the old station's band. */
    d->waitingForStation = false;
    d->started = false;
  }
  if (!d->started) {
    d->started = true;
    d->shownDb = wholeDb(smoothedTenths);
    return d->shownDb;
  }
  int32_t away = (int32_t)smoothedTenths - (int32_t)d->shownDb * 10;
  if (away < 0) {
    away = -away;
  }
  /* Half a dB of rounding plus the hysteresis, so the level has to move a
   * whole dB from what is shown before the digit follows it. */
  if (away >= 5 + SIGNAL_DISPLAY_HYSTERESIS_TENTHS) {
    d->shownDb = wholeDb(smoothedTenths);
  }
  return d->shownDb;
}

void signalDisplayStationChanged(SignalDisplay *d) {
  if (d != NULL) {
    d->waitingForStation = true;
  }
}

void signalDisplayReset(SignalDisplay *d) {
  if (d != NULL) {
    d->shownDb = 0;
    d->started = false;
    d->waitingForStation = false;
  }
}

void signalAverageReset(SignalAverage *avg) {
  if (avg != NULL) {
    avg->accumulator = 0;
    avg->started = false;
  }
}

void signalFormatLevel(int16_t tenths, char *out, size_t outLen) {
  if (out == NULL || outLen == 0) {
    return;
  }
  /* The sign is taken before the value is split. Dividing minus five tenths
   * by ten gives zero and the minus would be lost. */
  const char *sign = tenths < 0 ? "-" : "";
  uint16_t magnitude = (uint16_t)(tenths < 0 ? -tenths : tenths);
  snprintf(out, outLen, "%s%u.%u", sign, (unsigned)(magnitude / 10),
           (unsigned)(magnitude % 10));
}

int16_t signalAverage(SignalAverage *avg, int16_t sample) {
  if (avg == NULL) {
    return sample;
  }
  if (!avg->started) {
    /* Taken as the answer, not averaged up to from zero. Otherwise every band
     * change is followed by a second of the reading climbing to where it
     * already is, and anything deciding on it flaps meanwhile. */
    avg->started = true;
    avg->accumulator = (int32_t)sample * 10;
    return sample;
  }

  /* The reference firmware's smoothing, exactly: keep nine tenths and add the
   * new sample, then read back a tenth of it. */
  avg->accumulator = ((avg->accumulator * 9) + 5) / 10 + sample;
  return (int16_t)(avg->accumulator / 10);
}
