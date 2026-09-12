/**
 * @file radio.c
 * @brief Implementation of the radio state machine.
 */
#include "radio.h"

#include <string.h>

static bool tuneModeAllowedForBand(TuneMode mode, BandId band);

/** Where each band parks when it is first selected, in kHz. */
static uint32_t bandHome(BandId band, const BandPlanConfig *plan) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (!bandLimits(band, plan, &lo, &hi)) {
    return 0;
  }
  /* The bottom of the band. Somewhere in the middle would be a guess about
   * what people listen to, and there is no measurement behind such a guess. */
  return lo;
}

/**
 * Put the settings in order after the band has changed.
 *
 * The step size, the tuning mode and the bandwidth all mean different things
 * on different bands, so all three have to move with it. Leaving the
 * bandwidth alone is the worst of the three: FM's automatic setting is a
 * bandwidth of zero, and zero on the AM side is not a width at all, so the
 * tuner refuses it and the radio goes quiet.
 */
static void settleAfterBandChange(RadioSettings *s,
                                  const BandPlanConfig *plan) {
  s->stepKHz = bandDefaultStep(s->band, plan);
  if (!tuneModeAllowedForBand(s->tuneMode, s->band)) {
    s->tuneMode = TUNE_MODE_MANUAL;
  }
  /* The bandwidth goes back to what the new band wants, in both directions.
   *
   * Carrying it across is wrong each way. An FM automatic setting of zero is
   * not a width at all on the AM side, so the tuner refuses it. An AM width of
   * 4 kHz on FM is a tenth of what the signal needs, so the audio is muffled
   * and stereo never locks. A bandwidth belongs to a band, not to the radio. */
  s->bandwidthKHz = bandModulation(s->band) == MODULATION_FM
                        ? 0
                        : RADIO_AM_DEFAULT_BANDWIDTH_KHZ;
}

const char *tuneModeName(TuneMode mode) {
  switch (mode) {
    case TUNE_MODE_MANUAL:
      return "Manual";
    case TUNE_MODE_AUTO:
      return "Auto";
    case TUNE_MODE_MEMORY:
      return "Memory";
    case TUNE_MODE_METER_BAND:
      return "Meter band";
    default:
      return "";
  }
}

const char *radioErrorText(RadioError error) {
  switch (error) {
    case RADIO_OK:
      return "ok";
    case RADIO_ERR_BAND:
      return "not a band this radio has";
    case RADIO_ERR_FREQUENCY:
      return "that frequency is in no band";
    case RADIO_ERR_STEP:
      return "that band does not offer that step";
    case RADIO_ERR_BANDWIDTH:
      return "that bandwidth is not allowed here";
    case RADIO_ERR_VOLUME:
      return "that volume is outside what the chip takes";
    case RADIO_ERR_TUNE_MODE:
      return "that tuning mode is not available here";
    default:
      return "not a command";
  }
}

void radioDefaults(RadioSettings *settings, const BandPlanConfig *plan) {
  if (settings == NULL) {
    return;
  }
  memset(settings, 0, sizeof(*settings));
  settings->band = BAND_FM;
  settings->freqKHz = bandHome(BAND_FM, plan);
  settings->stepKHz = bandDefaultStep(BAND_FM, plan);
  settings->bandwidthKHz = 0; /* Let the tuner choose. */
  settings->volumeDb = 0;
  settings->muted = false;
  settings->tuneMode = TUNE_MODE_MANUAL;
}

/** Meter band mode only makes sense where there are meter bands. */
static bool tuneModeAllowedForBand(TuneMode mode, BandId band) {
  if (mode >= TUNE_MODE_COUNT) {
    return false;
  }
  if (mode == TUNE_MODE_METER_BAND) {
    return band == BAND_SW;
  }
  return true;
}

/** Move by whole steps, using whichever rule the tuning mode implies. */
static uint32_t stepBy(const RadioSettings *s, const BandPlanConfig *plan,
                       int16_t steps) {
  uint32_t freq = s->freqKHz;
  /* Widened on purpose: negating INT16_MIN overflows, and the loop then never
   * runs, so the command is silently ignored rather than refused. */
  int32_t remaining = steps < 0 ? -(int32_t)steps : (int32_t)steps;
  bool up = steps > 0;

  for (int32_t i = 0; i < remaining; i++) {
    if (s->tuneMode == TUNE_MODE_METER_BAND && s->band == BAND_SW) {
      freq = up ? swMeterBandNext(freq) : swMeterBandPrevious(freq);
    } else {
      freq = up ? bandStepUp(s->band, plan, freq, s->stepKHz)
                : bandStepDown(s->band, plan, freq, s->stepKHz);
    }
  }
  return freq;
}

RadioError radioApply(RadioSettings *settings, const BandPlanConfig *plan,
                      const RadioCommand *command) {
  if (settings == NULL || command == NULL) {
    return RADIO_ERR_UNKNOWN;
  }

  switch (command->kind) {
    case RADIO_TUNE: {
      BandId band;
      if (!bandForFrequency(plan, command->freqKHz, &band)) {
        return RADIO_ERR_FREQUENCY;
      }
      /* Tuning across a band edge changes band, and the step size has to
       * come with it or the next turn of the knob moves by something the new
       * band does not offer. */
      if (band != settings->band) {
        settings->band = band;
        settleAfterBandChange(settings, plan);
      }
      settings->freqKHz = command->freqKHz;
      return RADIO_OK;
    }

    case RADIO_STEP:
      if (command->steps == 0) {
        return RADIO_OK;
      }
      settings->freqKHz = stepBy(settings, plan, command->steps);
      return RADIO_OK;

    case RADIO_SET_BAND: {
      uint32_t lo = 0;
      uint32_t hi = 0;
      if (!bandLimits(command->band, plan, &lo, &hi)) {
        return RADIO_ERR_BAND;
      }
      settings->band = command->band;
      settings->freqKHz = bandHome(command->band, plan);
      settleAfterBandChange(settings, plan);
      return RADIO_OK;
    }

    case RADIO_SET_STEP:
      if (!bandStepAllowed(settings->band, plan, command->stepKHz)) {
        return RADIO_ERR_STEP;
      }
      settings->stepKHz = command->stepKHz;
      return RADIO_OK;

    case RADIO_SET_BANDWIDTH:
      /* Only FM has an automatic setting, so zero means something different
       * on the two sides and is refused where it means nothing. */
      if (command->bandwidthKHz == 0 &&
          bandModulation(settings->band) != MODULATION_FM) {
        return RADIO_ERR_BANDWIDTH;
      }
      if (command->bandwidthKHz > 6000) {
        return RADIO_ERR_BANDWIDTH;
      }
      settings->bandwidthKHz = command->bandwidthKHz;
      return RADIO_OK;

    case RADIO_SET_VOLUME:
      if (command->volumeDb < RADIO_VOLUME_MIN ||
          command->volumeDb > RADIO_VOLUME_MAX) {
        return RADIO_ERR_VOLUME;
      }
      settings->volumeDb = command->volumeDb;
      return RADIO_OK;

    case RADIO_SET_MUTE:
      settings->muted = command->muted;
      return RADIO_OK;

    case RADIO_SET_TUNE_MODE:
      if (!tuneModeAllowedForBand(command->tuneMode, settings->band)) {
        return RADIO_ERR_TUNE_MODE;
      }
      settings->tuneMode = command->tuneMode;
      return RADIO_OK;

    default:
      return RADIO_ERR_UNKNOWN;
  }
}

bool radioNeedsRetune(const RadioSettings *a, const RadioSettings *b) {
  if (a == NULL || b == NULL) {
    return true;
  }
  /* The step size and the tuning mode never reach the chip. They decide what
   * the next command will be, not what the tuner is doing now. */
  return a->band != b->band || a->freqKHz != b->freqKHz ||
         a->bandwidthKHz != b->bandwidthKHz || a->volumeDb != b->volumeDb ||
         a->muted != b->muted;
}
