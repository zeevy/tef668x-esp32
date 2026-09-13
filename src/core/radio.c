/**
 * @file radio.c
 * @brief Implementation of the radio state machine.
 */
#include "radio.h"

#include <string.h>

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
  if (!radioTuneModeAllowed(s->tuneMode, s->band)) {
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
    case RADIO_ERR_FM_ONLY:
      return "that only works on FM";
    case RADIO_ERR_FREQUENCY:
      return "that frequency is in no band";
    case RADIO_ERR_STEP:
      return "that band does not offer that step";
    case RADIO_ERR_BANDWIDTH:
      return "that bandwidth is not allowed here";
    case RADIO_ERR_VOLUME:
      return "that volume is outside what the chip takes";
    case RADIO_ERR_RANGE:
      return "that value is outside what the setting takes";
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
  /* 50 us, which is right everywhere except the Americas. The driver writes
   * the same figure in its start up defaults, so a radio that is never told
   * otherwise sounds right rather than dull. */
  settings->deemphasisUs = 50;
}

/*
 * Whether a frequency lands on a channel of any step the band offers.
 *
 * A plain comment rather than a doc block, because this is static. The
 * public functions are documented on their declarations in radio.h, and a
 * second doc comment beside the definition is a second place to keep right.
 */
static bool onAnyChannel(BandId band, const BandPlanConfig *plan,
                         uint32_t freqKHz) {
  size_t count = bandStepCount(band, plan);
  for (size_t i = 0; i < count; i++) {
    uint16_t step = bandStepAt(band, plan, i);
    if (step != 0 && bandNearestChannel(band, plan, freqKHz, step) == freqKHz) {
      return true;
    }
  }
  return false;
}

void radioPlanFromSettings(const Settings *settings, BandPlanConfig *out) {
  if (out == NULL) {
    return;
  }
  bandPlanDefaults(out);
  if (settings == NULL) {
    return;
  }
  if (settings->fmRegion < FM_REGION_COUNT) {
    out->fmRegion = (FmRegion)settings->fmRegion;
  }
  if (settings->mwSpacing <= (uint8_t)MW_SPACING_10K) {
    out->mwSpacing = (MwSpacing)settings->mwSpacing;
  }
}

void radioFromSettings(const Settings *settings, const BandPlanConfig *plan,
                       RadioSettings *out) {
  if (out == NULL || plan == NULL) {
    return;
  }
  radioDefaults(out, plan);
  if (settings == NULL) {
    return;
  }

  /* The band first, then the frequency inside it, so the frequency is judged
   * against the band it belongs to. */
  if (settings->startBand < BAND_COUNT) {
    RadioCommand band = {};
    band.kind = RADIO_SET_BAND;
    band.band = (BandId)settings->startBand;
    radioApply(out, plan, &band);
  }
  if (settings->startFreqKHz != 0) {
    RadioCommand tune = {};
    tune.kind = RADIO_TUNE;
    /* Onto a channel, not merely inside the band. The two come apart when the
     * grid moves under a frequency stored earlier: 738 kHz is a real medium
     * wave channel at 9 kHz spacing and is not one at 10 kHz, and it is
     * inside the band either way. Coming up between channels is slightly off
     * every station until somebody moves it.
     *
     * Only when it is off every step the band offers. The small steps exist
     * so a station can be tuned off centre on purpose, against selective
     * fading on AM or a crowded FM band, and the step in use is not stored.
     * Snapping to the default step alone would quietly undo that: a medium
     * wave station left on 737 would come back on 738. */
    tune.freqKHz = settings->startFreqKHz;
    BandId band = out->band;
    if (bandForFrequency(plan, settings->startFreqKHz, &band) &&
        !onAnyChannel(band, plan, settings->startFreqKHz)) {
      tune.freqKHz = bandNearestChannel(band, plan, settings->startFreqKHz,
                                        bandDefaultStep(band, plan));
    }
    /* The frequency decides the band when the two disagree, because
     * radioApply moves to whichever band holds it. They are stored together
     * so they normally agree, and when a region change breaks that the
     * frequency is the more exact of the two. A frequency in no band at all
     * is refused and the band's own starting point is what is left. */
    radioApply(out, plan, &tune);
  }

  out->multipathSuppression = settings->fmMultipathSuppression != 0;
  out->equalizer = settings->fmEqualizer != 0;
  out->forcedMono = settings->fmForcedMono != 0;
  out->highCutStart = settings->fmHighCutStart;
  out->stereoBlendStart = settings->fmStereoBlendStart;
  out->stHiBlendStart = settings->fmStHiBlendStart;
  out->fmNoiseBlankerStart = settings->fmNoiseBlankerStart;
  out->amNoiseBlankerStart = settings->amNoiseBlankerStart;
  out->deemphasisUs = settings->fmDeemphasisUs;

  /* The AM width only applies where AM applies. On FM the automatic setting
   * is what the band wants and settleAfterBandChange has already chosen it. */
  if (bandModulation(out->band) != MODULATION_FM) {
    out->bandwidthKHz = settings->amBandwidthKHz;
  }
}

void radioToSettings(const RadioSettings *radio, Settings *settings) {
  if (radio == NULL || settings == NULL) {
    return;
  }
  settings->startBand = (uint8_t)radio->band;
  settings->startFreqKHz = radio->freqKHz;
  /* Kept for the one mode where the knob is not the volume. Clamped to what
   * the knob itself can ask for, because that is the range it is compared
   * against when the mode changes back. */
  settings->startVolumeDb = radio->volumeDb > 0 ? 0
                            : radio->volumeDb < RADIO_VOLUME_MIN
                                ? RADIO_VOLUME_MIN
                                : radio->volumeDb;
  settings->fmMultipathSuppression = radio->multipathSuppression ? 1 : 0;
  settings->fmEqualizer = radio->equalizer ? 1 : 0;
  settings->fmForcedMono = radio->forcedMono ? 1 : 0;
  settings->fmHighCutStart = radio->highCutStart;
  settings->fmStereoBlendStart = radio->stereoBlendStart;
  settings->fmStHiBlendStart = radio->stHiBlendStart;
  settings->fmNoiseBlankerStart = radio->fmNoiseBlankerStart;
  settings->amNoiseBlankerStart = radio->amNoiseBlankerStart;
  settings->fmDeemphasisUs = radio->deemphasisUs;
  /* The AM width, only when it was read off an AM band. On FM the width is
   * the tuner's own choice and 0 means adaptive, which is not a width any AM
   * band would take. */
  if (bandModulation(radio->band) != MODULATION_FM &&
      radio->bandwidthKHz != 0) {
    settings->amBandwidthKHz = (uint8_t)radio->bandwidthKHz;
  }
}

bool radioTuneModeAllowed(TuneMode mode, BandId band) {
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
        /* And the band being left has to remember where it was, exactly as
         * it does when the BAND button moves off it. Typing a frequency on
         * the keypad is the usual way to leave a band, so without this the
         * memory is lost on the most common route out. */
        if (settings->band < BAND_COUNT) {
          settings->bandFreqKHz[settings->band] = settings->freqKHz;
        }
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
      if (command->band == settings->band) {
        /* Already there. Doing the work anyway would throw away where the
         * band was left and go back to the bottom of it. */
        return RADIO_OK;
      }

      /* Remember where this band was before leaving it. */
      if (settings->band < BAND_COUNT) {
        settings->bandFreqKHz[settings->band] = settings->freqKHz;
      }

      settings->band = command->band;
      uint32_t back = settings->bandFreqKHz[command->band];
      /* Clamped, because the band edges can move when the region or the
       * medium wave spacing changes, and a remembered frequency from before
       * that change can now be outside the band. */
      settings->freqKHz = (back != 0 && bandContains(command->band, plan, back))
                              ? back
                              : bandHome(command->band, plan);
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
      /* It has to be one the band actually offers. A width from the other
       * side's list is not a near miss: 4 kHz on FM pins the filter far
       * narrower than a station, and the radio then reports no pilot and no
       * signal and reads as one with no aerial.
       *
       * Zero is the FM automatic setting, and it is in the FM list and not
       * in the AM one, so it is refused on AM by the same check. */
      if (!bandBandwidthAllowed(settings->band, command->bandwidthKHz)) {
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

    case RADIO_CYCLE_BAND: {
      RadioCommand next = *command;
      next.kind = RADIO_SET_BAND;
      next.band = (BandId)((settings->band + 1) % BAND_COUNT);
      return radioApply(settings, plan, &next);
    }

    case RADIO_CYCLE_BANDWIDTH: {
      RadioCommand next = *command;
      next.kind = RADIO_SET_BANDWIDTH;
      next.bandwidthKHz =
          bandBandwidthNext(settings->band, settings->bandwidthKHz);
      return radioApply(settings, plan, &next);
    }

    case RADIO_CYCLE_TUNE_MODE: {
      /* Skip the modes this band does not offer, so the button always does
       * something. Meter band is shortwave only, and without this the cycle
       * sticks on the mode before it everywhere else. */
      TuneMode next = settings->tuneMode;
      for (int i = 0; i < TUNE_MODE_COUNT; i++) {
        next = (TuneMode)((next + 1) % TUNE_MODE_COUNT);
        if (radioTuneModeAllowed(next, settings->band)) {
          break;
        }
      }
      settings->tuneMode = next;
      return RADIO_OK;
    }

    case RADIO_TOGGLE_MUTE:
      settings->muted = !settings->muted;
      return RADIO_OK;

    case RADIO_CYCLE_FM_FEATURES: {
      if (bandModulation(settings->band) != MODULATION_FM) {
        return RADIO_ERR_FM_ONLY;
      }
      /* Counted as a two bit number, iMS in the low bit and EQ in the high
       * one, so the four combinations come round in a fixed order and the
       * button always moves to a different one. */
      uint8_t state = (uint8_t)((settings->multipathSuppression ? 1 : 0) |
                                (settings->equalizer ? 2 : 0));
      state = (uint8_t)((state + 1) & 3);
      settings->multipathSuppression = (state & 1) != 0;
      settings->equalizer = (state & 2) != 0;
      return RADIO_OK;
    }

    case RADIO_SET_WEAK_SIGNAL:
      if (bandModulation(settings->band) != MODULATION_FM) {
        return RADIO_ERR_FM_ONLY;
      }
      /* Checked here rather than only in the caller that happens to exist
       * today, the same as the blankers below. Each is a level in dBuV: 0 to
       * switch it off, or 20 to 60. Below 20 the mechanism starts at a level
       * no signal reaches, so it is on and does nothing. */
      for (int i = 0; i < 3; i++) {
        if (command->weak[i] != 0 &&
            (command->weak[i] < 20 || command->weak[i] > 60)) {
          return RADIO_ERR_RANGE;
        }
      }
      settings->highCutStart = command->weak[0];
      settings->stereoBlendStart = command->weak[1];
      settings->stHiBlendStart = command->weak[2];
      return RADIO_OK;

    case RADIO_SET_NOISE_BLANKER:
      /* Both bands, and settable from either, because the AM one is the
       * useful half and refusing it while on FM would be awkward for no
       * reason.
       *
       * The range is checked here rather than only in the caller that
       * happens to exist today. These are percentages: 0 for off, and 50 to
       * 150 usable. Between the two is neither, and a value there switches
       * the blanker on to do nothing. */
      for (int i = 0; i < 2; i++) {
        if (command->blanker[i] != 0 &&
            (command->blanker[i] < 50 || command->blanker[i] > 150)) {
          return RADIO_ERR_RANGE;
        }
      }
      settings->amNoiseBlankerStart = command->blanker[0];
      settings->fmNoiseBlankerStart = command->blanker[1];
      return RADIO_OK;

    case RADIO_BEEP:
      /* Nothing to apply, the same as a seek. A tone is something the radio
       * does for a moment, not a state this struct can hold. */
      return RADIO_OK;

    case RADIO_SEEK:
      /* Nothing to apply. Seeking is not a state this struct can hold: it is
       * something the radio does over the next few seconds, so the task owns
       * it. Accepted here so that a caller posting it is told the radio took
       * the command, which it did. */
      return RADIO_OK;

    case RADIO_SET_DEEMPHASIS:
      /* Settable from either side, like the blankers. It only reaches the
       * chip on FM, but refusing it on AM would mean a person has to change
       * band before they can set a thing that belongs to their country.
       *
       * Only the two real standards and off. Anything else is a guess, and
       * the chip would take it and quietly sound wrong. */
      if (command->deemphasisUs != 0 && command->deemphasisUs != 50 &&
          command->deemphasisUs != 75) {
        return RADIO_ERR_RANGE;
      }
      settings->deemphasisUs = command->deemphasisUs;
      return RADIO_OK;

    case RADIO_SET_MPH_SUPPRESSION:
    case RADIO_SET_EQUALIZER:
    case RADIO_SET_MONO:
      /* All three are FM ideas. The chip has nowhere to put them on the AM
       * side, so asking there is a mistake worth reporting rather than a
       * write that quietly goes nowhere. */
      if (bandModulation(settings->band) != MODULATION_FM) {
        return RADIO_ERR_FM_ONLY;
      }
      if (command->kind == RADIO_SET_MPH_SUPPRESSION) {
        settings->multipathSuppression = command->on;
      } else if (command->kind == RADIO_SET_EQUALIZER) {
        settings->equalizer = command->on;
      } else {
        settings->forcedMono = command->on;
      }
      return RADIO_OK;

    case RADIO_SET_TUNE_MODE:
      if (!radioTuneModeAllowed(command->tuneMode, settings->band)) {
        return RADIO_ERR_TUNE_MODE;
      }
      settings->tuneMode = command->tuneMode;
      return RADIO_OK;

    default:
      return RADIO_ERR_UNKNOWN;
  }
}

int8_t radioFadeVolume(int8_t targetDb, uint32_t elapsedMs,
                       uint16_t durationMs) {
  if (durationMs == 0 || elapsedMs >= durationMs) {
    return targetDb;
  }

  /* From a fixed depth below the target up to it, in a straight line.
   * Straight is right here: the volume is already in dB, so a straight line
   * in dB is a curve to the ear, which is the shape a fade wants. */
  int32_t from = (int32_t)targetDb - RADIO_FADE_DEPTH_DB;
  if (from < RADIO_VOLUME_MIN) {
    from = RADIO_VOLUME_MIN;
  }

  int32_t span = (int32_t)targetDb - from;
  int32_t along = (span * (int32_t)elapsedMs) / durationMs;
  int32_t now = from + along;

  if (now > targetDb) {
    now = targetDb;
  }
  if (now < RADIO_VOLUME_MIN) {
    now = RADIO_VOLUME_MIN;
  }
  return (int8_t)now;
}

uint32_t radioFadeElapsedAt(int8_t targetDb, int8_t nowDb,
                            uint16_t durationMs) {
  if (durationMs == 0) {
    return 0;
  }
  /* The same floor and the same straight line radioFadeVolume uses, read the
   * other way round. */
  int32_t from = (int32_t)targetDb - RADIO_FADE_DEPTH_DB;
  if (from < RADIO_VOLUME_MIN) {
    from = RADIO_VOLUME_MIN;
  }
  int32_t span = (int32_t)targetDb - from;
  if (span <= 0) {
    return durationMs;
  }
  int32_t along = (int32_t)nowDb - from;
  if (along <= 0) {
    return 0;
  }
  if (along >= span) {
    return durationMs;
  }
  /* Rounded up, not down. radioFadeVolume truncates on the way out, so
   * truncating here as well would answer with a moment slightly earlier than
   * the one asked about, and a caller starting a fade there would step the
   * volume down a dB before walking it up. Rounding up guarantees the fade
   * at this moment is at least the volume given. */
  return (uint32_t)((along * (int32_t)durationMs + span - 1) / span);
}

int8_t radioDuckVolume(int8_t fromDb, uint32_t elapsedMs, uint16_t durationMs) {
  if (durationMs == 0 || elapsedMs >= durationMs) {
    return RADIO_VOLUME_MIN;
  }

  /* Straight down in dB, for the same reason the fade up is straight: the
   * scale is already logarithmic, so a straight line here is a curve to the
   * ear. */
  int32_t span = (int32_t)fromDb - RADIO_VOLUME_MIN;
  int32_t along = (span * (int32_t)elapsedMs) / durationMs;
  int32_t now = (int32_t)fromDb - along;

  if (now > fromDb) {
    now = fromDb;
  }
  if (now < RADIO_VOLUME_MIN) {
    now = RADIO_VOLUME_MIN;
  }
  return (int8_t)now;
}

RadioPush radioPushNeeded(const RadioSettings *from, const RadioSettings *to) {
  RadioPush push;
  push.retune = true;
  push.bandwidth = true;
  push.volume = true;
  push.mute = true;
  push.features = true;
  if (from == NULL || to == NULL) {
    return push;
  }

  push.retune = from->band != to->band || from->freqKHz != to->freqKHz;
  /* A band change chooses a new bandwidth for the new band, so the width goes
   * with a retune whether or not the number happens to differ. */
  push.bandwidth = push.retune || from->bandwidthKHz != to->bandwidthKHz;
  /* The volume goes again when the band changes from FM to AM or back.
   *
   * Not because the number moved. Crossing between the two sides makes the
   * driver put the chip into its active mode again, and whether that resets
   * the output gain is not something the datasheet settles. Re-sending it
   * costs one write on a band change and removes the question. */
  bool sideChanged = bandModulation(from->band) != bandModulation(to->band);
  push.volume = sideChanged || from->volumeDb != to->volumeDb;
  push.mute = from->muted != to->muted;
  /* The three FM features go together. They are three writes either way, and
   * a band change loses them, so a retune re-sends them. */
  push.features =
      push.retune || from->multipathSuppression != to->multipathSuppression ||
      from->equalizer != to->equalizer || from->forcedMono != to->forcedMono ||
      from->highCutStart != to->highCutStart ||
      from->stereoBlendStart != to->stereoBlendStart ||
      from->stHiBlendStart != to->stHiBlendStart ||
      from->amNoiseBlankerStart != to->amNoiseBlankerStart ||
      from->fmNoiseBlankerStart != to->fmNoiseBlankerStart ||
      from->deemphasisUs != to->deemphasisUs;
  return push;
}

bool radioNeedsRetune(const RadioSettings *a, const RadioSettings *b) {
  /* The step size and the tuning mode never reach the chip. They decide what
   * the next command will be, not what the tuner is doing now. */
  RadioPush push = radioPushNeeded(a, b);
  return push.retune || push.bandwidth || push.volume || push.mute ||
         push.features;
}
