/**
 * @file seek.c
 * @brief Implementation of the seek stop decision.
 */
#include "seek.h"

/**
 * Noise and multipath limits, per step of sensitivity.
 *
 * The noise step is the reference firmware's own: its seek compares against
 * `fmscansens * 30`, so a sensitivity set here means the same thing as the
 * same number on the radio this replaces. The multipath step is the same
 * shape fitted to the sweep in `test/fixtures/seek/`, which is where the
 * reference uses a fixed 230 instead.
 *
 * A fixed 230 is too loose to seek with on this radio. At the default
 * sensitivity it would stop on 104.9 MHz, which reads 1.1 dBuV with a
 * multipath of 228 and is nothing at all. Scaling it keeps every real station
 * without that stop, and gives the sensitivity control something useful to do
 * at its loose end.
 */
#define SEEK_NOISE_PER_STEP 30
/**
 * The multipath limit per step of sensitivity, tenths of a per cent.
 *
 * 320 at the default, which is looser than the reference firmware's fixed
 * 230, and the reason is measurement. The sweep in `test/fixtures/seek/` was
 * taken on an afternoon when nothing on the band read above 36, so a limit
 * fitted to it alone looked safe at 200. Measured again the next day, 95.0
 * MHz read 204 to 279 across nine samples and is plainly a real station: a
 * weaker one with reflections, which is exactly the case a seek must not
 * skip. A limit fitted to one quiet afternoon would have skipped it.
 *
 * This is affordable because the level floor below does the work of
 * rejecting what is not a station, which a tight multipath limit cannot do
 * on its own: the shoulder of a strong station has very little of it.
 */
#define SEEK_MULTIPATH_PER_STEP 80

/*
 * The level floor, in tenths of a dBuV, as a starting point less a step per
 * sensitivity. It runs the other way to the other two: a higher sensitivity
 * settles for a weaker signal.
 *
 * At the default of 4 that is 10.0 dBuV. Every real station in the sweep read
 * 26.7 dBuV or better, and the two nearest things that are not stations read
 * -1.8 and -5.0. The gap is wide and the floor sits in the middle of it.
 */
/** Where the level floor starts before sensitivity is taken off it. */
#define SEEK_LEVEL_BASE_TENTHS 220
/** How much of the floor each step of sensitivity gives away. */
#define SEEK_LEVEL_PER_STEP 30

/** Sensitivity, held inside the range the radio offers. */
static uint8_t clampSensitivity(uint8_t sensitivity) {
  if (sensitivity < SEEK_SENSITIVITY_MIN) {
    return SEEK_SENSITIVITY_MIN;
  }
  if (sensitivity > SEEK_SENSITIVITY_MAX) {
    return SEEK_SENSITIVITY_MAX;
  }
  return sensitivity;
}

void seekDefaults(SeekConfig *out) {
  if (out == NULL) {
    return;
  }
  out->fmSensitivity = SEEK_SENSITIVITY_DEFAULT;
  out->amSensitivity = SEEK_SENSITIVITY_DEFAULT;
}

uint16_t seekNoiseLimit(uint8_t sensitivity) {
  return (uint16_t)(clampSensitivity(sensitivity) * SEEK_NOISE_PER_STEP);
}

uint16_t seekMultipathLimit(uint8_t sensitivity) {
  return (uint16_t)(clampSensitivity(sensitivity) * SEEK_MULTIPATH_PER_STEP);
}

int16_t seekLevelFloor(uint8_t sensitivity) {
  return (int16_t)(SEEK_LEVEL_BASE_TENTHS -
                   clampSensitivity(sensitivity) * SEEK_LEVEL_PER_STEP);
}

bool seekShouldStop(const SeekConfig *cfg, BandId band,
                    const SeekReading *reading) {
  if (reading == NULL || !reading->valid) {
    /* A reading that did not arrive is not a station. Stopping on one would
     * park the radio wherever the bus happened to fail, and report it as a
     * find. */
    return false;
  }

  SeekConfig defaults;
  if (cfg == NULL) {
    seekDefaults(&defaults);
    cfg = &defaults;
  }

  bool fm = bandModulation(band) == MODULATION_FM;
  uint8_t sensitivity = fm ? cfg->fmSensitivity : cfg->amSensitivity;

  if (reading->noiseTenths >= seekNoiseLimit(sensitivity)) {
    return false;
  }

  /* The shoulder of a strong station is quiet and clean and is not a station.
   * Only level tells it apart: everything else about it looks right, because
   * what the receiver is hearing really is a transmitter, just the one next
   * door.
   *
   * FM only. The floor is a number fitted to an FM sweep on an FM level
   * scale, and there is no AM sweep yet to fit an AM one to. Applying it
   * there would be a guessed threshold, and a guessed threshold switches a
   * feature off without saying so. The AM side therefore keeps to the rule
   * the reference firmware uses, which is noise and offset, and which runs on
   * this board today. See test/fixtures/seek/README.md. */
  if (fm && reading->levelTenths < seekLevelFloor(sensitivity)) {
    return false;
  }

  /* Multipath is an FM idea. The chip puts a co-channel figure in the same
   * field on the AM side, which is a different measurement on a different
   * scale, so it is not judged against a multipath limit. */
  if (fm && reading->multipathTenths >= seekMultipathLimit(sensitivity)) {
    return false;
  }

  int16_t window = fm ? SEEK_OFFSET_FM_TENTHS : SEEK_OFFSET_AM_TENTHS;
  if (reading->offsetTenths <= -window || reading->offsetTenths >= window) {
    return false;
  }

  return true;
}
