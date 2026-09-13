/* Implementation of the screen glue. */
#include "screen_task.h"

#include "board/board.h"
#include "core/signal.h"
#include "core/version.h"
#include "drivers/analog.h"
#include "drivers/display.h"
#include "input_task.h"
#include "radio_task.h"
#include "ui/screen.h"

#include <Arduino.h>
#include <stdio.h>

/*
 * How often the screen is looked at, in milliseconds.
 *
 * The radio publishes ten times a second, so anything faster than this only
 * compares strings and finds them the same. Twenty five times a second is
 * quicker than a hand can turn the knob.
 */
#define SCREEN_POLL_MS 40

/*
 * How often the panel light is stepped during the fade at boot, in ms.
 *
 * The fade runs inside screenTaskBegin, so this is a plain delay and nothing
 * else is waiting on it. About sixty steps over the one second the fade takes
 * is finer than the eye follows.
 */
#define BACKLIGHT_STEP_MS 16

static bool sReady = false;
static uint32_t sLastPollMs = 0;

/* The panel light, and what it was last set to, so it is not rewritten. */
static Backlight sBacklight;
static uint8_t sLastWritten = 0;
static bool sLastWrittenKnown = false;

/*
 * The signal number on the screen, and what it was last shown for.
 *
 * Held still rather than redrawn from every reading. The band and frequency
 * are kept beside it so a change of station starts it again, instead of the
 * old station's number being held until the new one has moved a whole dB
 * away from it.
 */
static SignalDisplay sSignalDisplay;
static uint32_t sShownFreqKHz = 0;
static BandId sShownBand = BAND_FM;
static bool sShownStationKnown = false;

/* The activity count last seen, so a change of it counts as one wake. */
static uint32_t sLastActivity = 0;

/*
 * Whether the loop has run once yet.
 *
 * Start up carries on for seconds after the panel is lit, through the tuner
 * patch and the Wi-Fi join, and nothing moves the backlight along during it.
 * Without this, a radio set to dim after ten seconds and taking twelve to
 * come up would dim the moment it finished booting, in front of the person
 * who has just switched it on. The idle clock starts when the radio is ready,
 * not when the panel was lit.
 */
static bool sFirstPoll = true;

static void writeBacklight(uint8_t percent) {
  if (sLastWrittenKnown && percent == sLastWritten) {
    return;
  }
  displayBacklight(percent);
  sLastWritten = percent;
  sLastWrittenKnown = true;
}

bool screenTaskBegin(const BacklightConfig *cfg) {
  sReady = screenBegin();
  if (!sReady) {
    Serial.println(F("[screen] the panel did not come up"));
    return false;
  }
  screenMessage("TEF668X", BOARD_NAME_DISPLAY " " FIRMWARE_VERSION);

  /* The message goes on the glass before any light does, so the fade reveals
   * something rather than coming up on a blank panel and filling in after. */
  backlightInit(&sBacklight, cfg, millis());
  writeBacklight(backlightLevel(&sBacklight));
  while (backlightFading(&sBacklight)) {
    delay(BACKLIGHT_STEP_MS);
    writeBacklight(backlightUpdate(&sBacklight, millis()));
  }
  writeBacklight(backlightUpdate(&sBacklight, millis()));

  sLastActivity = inputActivity();
  return true;
}

void screenTaskSetBacklight(const BacklightConfig *cfg) {
  backlightSetConfig(&sBacklight, cfg, millis());
  if (sReady) {
    writeBacklight(backlightLevel(&sBacklight));
  }
}

bool screenTaskBacklightState(uint8_t *percent) {
  if (percent != NULL) {
    *percent = backlightLevel(&sBacklight);
  }
  return backlightDimmed(&sBacklight);
}

int16_t screenTaskSignalShown(void) {
  return sSignalDisplay.shownDb;
}

void screenTaskPoll(void) {
  if (!sReady) {
    return;
  }
  uint32_t nowMs = millis();

  if (sFirstPoll) {
    sFirstPoll = false;
    backlightWake(&sBacklight, nowMs);
  }

  /* Checked every time round the loop, not on the poll interval below. A
   * press that had to wait up to forty milliseconds for the light to come
   * back would be a press that looked ignored. */
  uint32_t activity = inputActivity();
  if (activity != sLastActivity) {
    sLastActivity = activity;
    backlightWake(&sBacklight, nowMs);
  }
  writeBacklight(backlightUpdate(&sBacklight, nowMs));

  if ((uint32_t)(nowMs - sLastPollMs) < SCREEN_POLL_MS) {
    return;
  }
  sLastPollMs = nowMs;

  RadioSnapshot snap;
  if (!radioGetSnapshot(&snap)) {
    return;
  }

  char frequency[16];
  if (!bandFormatFrequency(snap.settings.band, snap.settings.freqKHz, frequency,
                           sizeof(frequency))) {
    frequency[0] = '\0';
  }

  ScreenState state;
  state.band = bandName(snap.settings.band);
  state.frequency = frequency;
  state.unit = bandFrequencyUnit(snap.settings.band);
  state.mode = tuneModeName(snap.settings.tuneMode);
  /* The smoothed level, and then held still on top of that.
   *
   * The smoothing alone was not enough. It takes the swing on a steady FM
   * station from about 3 dB to about 1.6, but the screen was printing a
   * decimal place, so the last digit still moved ten times a second and read
   * as flicker whatever the number underneath was doing. Whole dB with
   * hysteresis takes that to one change in twenty four seconds, measured.
   *
   * A change of station starts it again, or the old number would be held
   * until the new one had moved a whole dB from it. */
  if (!sShownStationKnown || snap.settings.band != sShownBand ||
      snap.settings.freqKHz != sShownFreqKHz) {
    sShownBand = snap.settings.band;
    sShownFreqKHz = snap.settings.freqKHz;
    sShownStationKnown = true;
    signalDisplayStationChanged(&sSignalDisplay);
  }
  state.signalDbuV = signalDisplayLevel(
      &sSignalDisplay, snap.levelSmoothedTenths, snap.levelSmoothedValid);
  state.signalValid = snap.qualityValid;
  /* A pilot on its own is not stereo coming out of the speaker. Forced mono
   * leaves the pilot exactly where it was, so the flag stays true while the
   * audio is mono, and the screen then tells the person something untrue
   * about what they are listening to.
   *
   * The automatic blend is a matter of degree rather than a switch, so it is
   * not folded in here. Forced mono is the unambiguous half. */
  state.stereo =
      snap.qualityValid && snap.quality.stereo && !snap.settings.forcedMono;
  state.muted = snap.settings.muted;
  /* Only on FM, where they do something. Showing them on medium wave would
   * say the radio is applying something the tuner has nowhere to put. */
  bool fm = bandModulation(snap.settings.band) == MODULATION_FM;
  state.ims = fm && snap.settings.multipathSuppression;
  state.eq = fm && snap.settings.equalizer;
  state.tunerReady = snap.tunerReady;
  /* What the tuner last refused. Without it the screen can show a station the
   * radio is not actually on, with nothing to say so. */
  state.fault =
      snap.lastError != TEF668X_OK ? tef668xErrorText(snap.lastError) : NULL;

  screenShow(&state);

  /* The needle shows what the screen shows. Driven from here rather than from
   * its own place because it is the same job: telling the person holding the
   * radio what the radio is receiving. Smoothed for the same reason, and more
   * so: a needle has weight and a jumping one reads as a loose pointer. */
  smeterShow(snap.qualityValid ? snap.levelSmoothedTenths : 0);
}
