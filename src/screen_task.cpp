/**
 * @file screen_task.cpp
 * @brief Implementation of the screen glue.
 */
#include "screen_task.h"

#include "board/board.h"
#include "core/version.h"
#include "drivers/analog.h"
#include "radio_task.h"
#include "ui/screen.h"

#include <Arduino.h>
#include <stdio.h>

/**
 * How often the screen is looked at, in milliseconds.
 *
 * The radio publishes ten times a second, so anything faster than this only
 * compares strings and finds them the same. Twenty five times a second is
 * quicker than a hand can turn the knob.
 */
#define SCREEN_POLL_MS 40

static bool sReady = false;
static uint32_t sLastPollMs = 0;

bool screenTaskBegin(void) {
  sReady = screenBegin();
  if (!sReady) {
    Serial.println(F("[screen] the panel did not come up"));
    return false;
  }
  screenMessage("TEF668X", BOARD_NAME_DISPLAY " " FIRMWARE_VERSION);
  return true;
}

void screenTaskPoll(void) {
  if (!sReady) {
    return;
  }
  uint32_t nowMs = millis();
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
  state.signalTenths = snap.quality.levelDbuVTenths;
  state.signalValid = snap.qualityValid;
  state.stereo = snap.qualityValid && snap.quality.stereo;
  state.muted = snap.settings.muted;
  state.tunerReady = snap.tunerReady;
  /* What the tuner last refused. Without it the screen can show a station the
   * radio is not actually on, with nothing to say so. */
  state.fault =
      snap.lastError != TEF668X_OK ? tef668xErrorText(snap.lastError) : NULL;

  screenShow(&state);

  /* The needle shows what the screen shows. Driven from here rather than from
   * its own place because it is the same job: telling the person holding the
   * radio what the radio is receiving. */
  smeterShow(snap.qualityValid ? snap.quality.levelDbuVTenths : 0);
}
