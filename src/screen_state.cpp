/* Implementation of the radio screen's state. */
#include "screen_state.h"

#include "core/radio.h"
#include "core/rds.h"
#include "core/strings.h"
#include "core/wifi_signal.h"

#include <stdio.h>
#include <string.h>

/*
 * The modulation meter's timings. With the bar rising to a reading at once,
 * a 1700 ms fall takes the movement from 17.3 points of a hundred a frame to
 * 7.7, and a longer one stops being worth the wait. The peak holds three
 * quarters of a second and falls the whole meter in 3.75 s, times that were
 * checked on the radio.
 */
#define MODULATION_FALL_MS 1700
#define MODULATION_PEAK_HOLD_MS 750
#define MODULATION_PEAK_FALL_MS 3750

void screenStateReset(ScreenBuild *b) {
  memset(b, 0, sizeof(*b));
  meterBarReset(&b->modulationBar);
  meterPeakReset(&b->modulationPeak);
  b->shownBand = BAND_FM;
  b->namedSlot = MEMORY_NO_SLOT;
}

void screenStateBuild(ScreenBuild *b, const ScreenInputs *in,
                      ScreenState *out) {
  const RadioSnapshot &snap = *in->snap;
  ScreenState &state = *out;

  if (!bandFormatFrequency(snap.settings.band, snap.settings.freqKHz,
                           b->frequency, sizeof(b->frequency))) {
    b->frequency[0] = '\0';
  }

  /* The band's two ends and the tuned frequency, for the scale. A span of 0,
   * when the plan cannot be read, tells the panel not to draw a scale it has
   * no numbers for. */
  uint32_t sweepKHz = 0;
  uint32_t sweepSpanKHz = 0;
  uint32_t sweepLowKHz = 0;
  {
    uint32_t lowKHz = 0;
    uint32_t highKHz = 0;
    if (in->planValid &&
        bandLimits(snap.settings.band, &in->plan, &lowKHz, &highKHz) &&
        highKHz > lowKHz) {
      uint32_t f = snap.settings.freqKHz;
      if (f < lowKHz) {
        f = lowKHz;
      } else if (f > highKHz) {
        f = highKHz;
      }
      sweepKHz = f;
      sweepSpanKHz = highKHz - lowKHz;
      sweepLowKHz = lowKHz;
    }
  }
  const bool fm = bandModulation(snap.settings.band) == MODULATION_FM;

  memset(&state, 0, sizeof(state));
  state.fm = fm;
  state.band = bandName(snap.settings.band);
  if (swMeterBandFormat(snap.settings.band, snap.settings.freqKHz, b->meterBand,
                        sizeof(b->meterBand))) {
    state.meterBand = b->meterBand;
  }
  state.frequency = b->frequency;
  state.unit = bandFrequencyUnit(snap.settings.band);
  state.sweepKHz = sweepKHz;
  state.sweepSpanKHz = sweepSpanKHz;
  state.sweepLowKHz = sweepLowKHz;
  /*
   * The trailing hyphen is added here rather than kept on the digits
   * themselves, because the input layer's own copy of them, in `/api/state`
   * and the diagnostics page, is the frequency somebody is typing and not a
   * piece of the panel's own presentation.
   *
   * Left off once the buffer is full. A hyphen invites another digit, and at
   * INPUT_DIGITS_MAX there is nowhere left for one to go: the next key is
   * refused rather than kept, so a mark that says more can follow would be
   * wrong the moment it is drawn.
   */
  {
    const char *typed = in->typed != NULL ? in->typed : "";
    if (typed[0] != '\0') {
      screenTypedText(typed, b->typing, sizeof(b->typing));
      state.typing = b->typing;
    }
  }
  /* The smoothed level, and then held still on top of that.
   *
   * The smoothing alone is not enough. It takes the swing on a steady FM
   * station from about 3 dB to about 1.6, so a decimal place would still
   * move ten times a second and read as flicker. Whole dB with hysteresis
   * takes that to one change in twenty four seconds, measured.
   *
   * A change of station starts it again, or the old number would be held
   * until the new one had moved a whole dB from it. */
  if (!b->shownStationKnown || snap.settings.band != b->shownBand ||
      snap.settings.freqKHz != b->shownFreqKHz) {
    b->shownBand = snap.settings.band;
    b->shownFreqKHz = snap.settings.freqKHz;
    b->shownStationKnown = true;
    signalDisplayStationChanged(&b->signalDisplay);
    /* The old station's modulation says nothing about the new one, and the
     * reading the snapshot still carries is the old station's until the
     * radio's next read. */
    meterBarReset(&b->modulationBar);
    meterPeakReset(&b->modulationPeak);
    b->modulationReads = snap.qualityReads;
  }
  /* The offset goes on after the hold, so the number still moves a whole dB at
   * a time, and the scale's peak below is drawn from the level as shown. */
  state.signalDbuV =
      (int16_t)(signalDisplayLevel(&b->signalDisplay, snap.levelSmoothedTenths,
                                   snap.levelSmoothedValid) +
                in->levelOffsetDb);
  state.signalValid = snap.qualityValid;
  /*
   * Why there is no sound, rather than only whether there is.
   *
   * A person muting the radio and the squelch holding the audio down both
   * mean silence, and the V: tile says which. Without that a squelched radio
   * looks like a broken one.
   */
  state.audio = snap.settings.muted ? SCREEN_AUDIO_MUTED
                : !snap.squelchOpen ? SCREEN_AUDIO_SQUELCHED
                                    : SCREEN_AUDIO_ON;
  state.tunerReady = snap.tunerReady;
  /* The short names, not tuneModeName. Those are mixed case for the browser,
   * and the tile holds three or four capitals. */
  state.tuneMode = tuneModeShort(snap.settings.tuneMode);
  /* What the network is doing, and how strong the link is. The bars are only
   * meaningful when the state is SCREEN_WIFI_JOINED, so left at 0 otherwise;
   * the panel does not read them then. */
  state.wifi = in->wifi;
  if (in->rssiValid) {
    state.wifiBars = wifiSignalBars(in->rssiDbm);
  }

  snprintf(b->volumeText, sizeof(b->volumeText), txt(STR_RADIO_FMT_VOL_DB),
           (int)snap.settings.volumeDb);
  state.volume = b->volumeText;
  /* From the snapshot rather than from radioSquelchMode, so the mode and
   * squelchOpen above come from the same instant. Capitals, because a tile
   * value that is a word is written in capitals. In Manual the volume knob
   * sets the level, so the tile shows the level, rounded to a whole dB, and
   * it moves as the knob turns. */
  const SquelchMode squelchMode = snap.squelchMode;
  if (squelchMode == SQUELCH_MANUAL) {
    const int t = snap.squelchThresholdTenths;
    snprintf(b->squelchText, sizeof(b->squelchText), txt(STR_RADIO_FMT_SQL_DB),
             (t < 0 ? t - 5 : t + 5) / 10);
    state.squelchMode = b->squelchText;
  } else {
    state.squelchMode =
        txt(squelchMode == SQUELCH_OFF ? STR_RADIO_SQL_OFF : STR_COMMON_AUTO);
  }
  /*
   * The filter: DYN while the tuner chooses the width for itself, which a
   * setting of 0 hands over on every band, and the width a person set
   * otherwise. The width is a number with its unit, so it keeps its case.
   */
  if (snap.settings.bandwidthKHz == 0) {
    state.filter = txt(STR_RADIO_BW_DYN);
  } else {
    snprintf(b->filterText, sizeof(b->filterText), txt(STR_RADIO_FMT_BW_K),
             (unsigned)snap.settings.bandwidthKHz);
    state.filter = b->filterText;
  }

  /*
   * The battery, when there is a reading and the setting is not off: the
   * shape alone for a per cent, or the voltage and a small upright shape.
   */
  if (in->battery != NULL && in->batteryShow != BATTERY_SHOW_OFF &&
      batteryMilliVolts(in->battery) > 0) {
    state.batteryValid = true;
    state.batteryPercent = batteryPercent(in->battery);
    /* As a per cent the shape alone says it, so there is no text. */
    if (in->batteryShow == BATTERY_SHOW_PERCENT ||
        !batteryFormat(in->battery, in->batteryShow, b->batteryText,
                       sizeof(b->batteryText))) {
      b->batteryText[0] = '\0';
    }
    state.batteryText = b->batteryText[0] != '\0' ? b->batteryText : NULL;
  }

  /*
   * How tall the tuning scale's peak stands, against the top for the band
   * group, core/signal.h. Driven from the same held number printed on the
   * panel, so the two cannot disagree.
   */
  if (snap.qualityValid) {
    const uint8_t full = fm ? SIGNAL_FULL_FM_DBUV : SIGNAL_FULL_AM_DBUV;
    state.signalPercent =
        signalBarPercent((int16_t)((int32_t)state.signalDbuV * 10), full);
  }

  /* The modulation meter, on every band, since the tuner reports it on AM as
   * well. Fed once for each read of the tuner, not once for each build: the
   * screen builds more often than the radio reads, and feeding the same
   * reading again would start the peak's hold again. Past 100 is real and
   * routine, a station driving past reference deviation, and is drawn full:
   * no figure is printed beside it, so it is capped before the bar, which
   * would otherwise stay full while it fell back from the excess. A reading
   * that stopped arriving leaves no level on screen that nothing measures. */
  if (snap.qualityValid) {
    if (snap.qualityReads != b->modulationReads) {
      b->modulationReads = snap.qualityReads;
      const uint8_t reading = snap.quality.modulationPercent <= 0 ? 0
                              : snap.quality.modulationPercent > 100
                                  ? 100
                                  : (uint8_t)snap.quality.modulationPercent;
      (void)meterBarFeed(&b->modulationBar, reading, in->nowMs,
                         MODULATION_FALL_MS);
      meterPeakFeed(&b->modulationPeak, reading, in->nowMs,
                    MODULATION_PEAK_HOLD_MS, MODULATION_PEAK_FALL_MS);
    }
    state.modulationValid = b->modulationBar.valid;
    state.modulationPercent = (uint8_t)b->modulationBar.percent;
    state.modulationPeakValid = b->modulationPeak.bar.valid;
    state.modulationPeakPercent = (uint8_t)b->modulationPeak.bar.percent;
  } else {
    meterBarReset(&b->modulationBar);
    meterPeakReset(&b->modulationPeak);
  }

  /* What the RDS decoder has made of the station. FM only, and every field
   * is left out rather than blanked when it cannot be answered. */
  if (fm) {
    /* The stitched name when the station splits one across two passes, and
     * the pass itself otherwise. A station that sends `RADIO 9` then `5`
     * means `RADIO 95`; showing those in turn is truthful and unreadable. */
    const char *sent = snap.rds.hasPsLong ? snap.rds.psLong
                       : snap.rds.hasPs   ? snap.rds.ps
                                          : NULL;
    state.stationName =
        sent != NULL && rdsNameTrim(sent, sizeof(b->stationName),
                                    b->stationName, sizeof(b->stationName))
            ? b->stationName
            : NULL;
    state.radioText = snap.rds.hasRt ? snap.rds.rt : NULL;
  }

  if (snap.memorySlot != MEMORY_NO_SLOT) {
    snprintf(b->memory, sizeof(b->memory), txt(STR_RADIO_FMT_MEMORY_SLOT),
             (int)snap.memorySlot + 1);
    state.memory = b->memory;
  }
  /*
   * What a person called this channel, for the slot the station name would
   * have used when the station does not name itself.
   *
   * Held between passes rather than read on each one. memoryStoreRead takes
   * the store's lock, and the radio task needs that same lock to walk the
   * list in memory mode, so asking twenty five times a second for a string
   * that changes when somebody edits it is contention bought for nothing. The
   * generation counter is a plain read with no lock, so the two together say
   * exactly when the answer can have changed: a different slot, or an edit.
   */
  const uint32_t memoryGeneration = in->memoryGeneration;
  if (snap.memorySlot == MEMORY_NO_SLOT) {
    b->channelName[0] = '\0';
    b->namedSlot = MEMORY_NO_SLOT;
  } else if (snap.memorySlot != b->namedSlot ||
             memoryGeneration != b->namedGeneration) {
    MemoryChannel channel;
    b->channelName[0] = '\0';
    if (in->readChannel(snap.memorySlot, &channel)) {
      /* Bounded rather than trusting the terminator. memoryChannelValid
       * checks that a name ends inside its field and memorySet refuses one
       * that does not, but memoryNvsLoad reads the whole store back as a blob
       * and checks only its length, so a name that reached flash by any other
       * route is not covered. %s would then read past the field. */
      snprintf(b->channelName, sizeof(b->channelName), "%.*s",
               (int)(MEMORY_NAME_LEN - 1), channel.name);
    }
    b->namedSlot = snap.memorySlot;
    b->namedGeneration = memoryGeneration;
  }
  /* A channel may have no name at all, and an empty one is not a name. */
  if (b->channelName[0] != '\0') {
    state.memoryName = b->channelName;
  }

  /* The clock and the date, only once a server has answered: see where
   * screen_task.cpp reads them. */
  state.clock = in->clock;
  state.date = in->date;
  state.menuMark = in->touchOn;
  state.pcMark = in->pcLink;

  /* What the tuner last refused. Without it the screen can show a station the
   * radio is not actually on, with nothing to say so. */
  state.fault = in->fault;

  /* The logbook confirmation, which takes the name line while it holds: see
   * screenTaskLogConfirm. */
  state.logConfirm = in->logConfirm;

  /* What the radio is doing for a few seconds, on the line under the panel
   * in place of the radio text. */
  state.notice = in->notice;
}

void screenTypedText(const char *typed, char *out, size_t len) {
  snprintf(out, len,
           strlen(typed) >= INPUT_DIGITS_MAX ? "%s" : txt(STR_RADIO_FMT_TYPED),
           typed);
}
