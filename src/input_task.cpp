/* Implementation of the input layer. */
#include "input_task.h"

#include "core/squelch.h"
#include "drivers/analog.h"
#include "drivers/encoder.h"
#include "drivers/keypad.h"
#include "radio_task.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

/* The four buttons, in PanelButton order. */
static Button sButtons[PANEL_BUTTON_COUNT];

/* How fast the knob is being turned. */
static Acceleration sAcceleration;

/* Digits keyed and not yet entered. */
static char sTyped[INPUT_DIGITS_MAX + 1];
static uint8_t sTypedLen = 0;

/* When the last digit was keyed, so a half typed number does not sit
 *  there for ever waiting for an enter that is not coming. */
static uint32_t sTypedMs = 0;

/* A part typed number is dropped after this long with no new digit. */
#define TYPED_TIMEOUT_MS 5000

/*
 * How often the keypad is read, in milliseconds.
 *
 * The keypad sits on the tuner's I2C bus, so every read here is a read the
 * radio task has to wait for. Fifty times a second is far quicker than
 * anybody types and costs the bus almost nothing.
 */
#define KEYPAD_POLL_MS 20

/*
 * How long a button press waits for the radio, in milliseconds.
 *
 * Long enough for a retune, which is five I2C writes with a settle after
 * each. The wait is on the loop task, never on the radio task.
 */
#define BUTTON_SETTLE_MS 300

/*
 * How often the volume pot is read, in milliseconds.
 *
 * The same rate the working firmware uses. Faster buys nothing: a hand cannot
 * turn a knob faster than this and the converter needs averaging anyway.
 */
#define POT_POLL_MS 50

static InputStatus sStatus;

/* The last pot reading acted on, and whether there is one yet. */
static uint16_t sPot = 0;
static bool sPotKnown = false;

/*
 * How this unit's knob maps to volume and to a squelch threshold.
 *
 * The defaults are one radio's numbers, taken from the reference firmware:
 * the travel runs 120 to 4000 there. This unit reaches 0 and 4095, so they
 * are not wrong here, but a pot that read 200 to 3800 would lose travel at
 * both ends with nothing to say so. Calibration replaces them with what this
 * knob actually reaches.
 */
static PotConfig sPotCfg;

/* Calibration. While it runs the knob does nothing but record how far it
 * goes, so that sweeping to the loud end to find the end stop is not painful.
 * The state machine is in core/input.c, where it can be tested on a PC. */
static PotCalibration sCal;

/* Which presses make a sound. Off unless somebody asks. */
static BeepMode sBeepMode = BEEP_OFF;

/*
 * How long a beep lasts, in milliseconds.
 *
 * The reference firmware's figure for its band edge beep on this chip. Short
 * enough to be a tick rather than a tone.
 */
#define BEEP_MS 50

/*
 * And for a long press, which is deliberately different.
 *
 * A long press is the one you cannot tell has registered: there is nothing to
 * feel, and the moment it fires is decided by a timer rather than by letting
 * go. A tick of a different length says which of the two the radio took.
 */
#define BEEP_LONG_MS 200

/*
 * How many things a person has done to this radio, counted for inputActivity.
 *
 * Separate from the counts in InputStatus, which are per source and are there
 * for the diagnostic page. This one answers a different question, which is
 * whether the radio has been left alone.
 */
static uint32_t sActivity = 0;

/* What the knob was last doing, so a change of job can be acted on. */
static SquelchMode sJob = SQUELCH_OFF;
static bool sJobKnown = false;

static void note(const char *what) {
  snprintf(sStatus.lastEvent, sizeof(sStatus.lastEvent), "%s", what);
  sStatus.lastEventMs = millis();
  Serial.printf("[input] %s\n", what);
}

static void send(const RadioCommand *command) {
  radioPost(command);
}

/*
 * Send one command and wait for the radio to deal with it.
 *
 * For the buttons and the keypad, which are rare and whose log line has to
 * say what actually happened. Saying what was asked for instead is how MODE
 * came to announce a mode the radio had refused.
 */
static bool sendAndSettle(const RadioCommand *command) {
  RadioError why = RADIO_OK;
  return radioPostAndSettle(command, BUTTON_SETTLE_MS, &why) ==
             RADIO_POST_DONE &&
         why == RADIO_OK;
}

static void clearTyped(void) {
  sTypedLen = 0;
  sTyped[0] = '\0';
  memcpy(sStatus.typed, sTyped, sizeof(sTyped));
}

bool inputBegin(EncoderKind kind, EncoderDirection direction) {
  memset(sButtons, 0, sizeof(sButtons));
  memset(&sAcceleration, 0, sizeof(sAcceleration));
  memset(&sStatus, 0, sizeof(sStatus));
  clearTyped();

  /* Nothing is known about the knob or about what it is for until the first
   * poll, so both are forgotten here rather than carried over. */
  sPotKnown = false;
  sJobKnown = false;
  potCalibrateCancel(&sCal);
  potDefaults(&sPotCfg);

  encoderBegin(kind, direction);
  analogBegin();
  sStatus.keypadPresent = keypadBegin();

  /* Read the pot for the status document, and send nothing.
   *
   * Where the radio starts is main.cpp's job, because only it knows what the
   * knob is for: in manual squelch the knob is the squelch control and the
   * volume comes from the stored one instead. Sending a volume here would
   * overwrite that a few milliseconds after the task was started with it.
   *
   * The first pollPot takes over. Its jobChanged is true on the first poll,
   * so it acts at once rather than waiting for the knob to move past the
   * deadband. */
  sPot = potRead();
  sStatus.pot = sPot;
  sStatus.potDb = potVolumeDb(sPot, &sPotCfg);

  return sStatus.keypadPresent;
}

static void pollEncoder(uint32_t nowMs) {
  int32_t clicks = encoderTake();
  if (clicks == 0) {
    return;
  }
  sStatus.clicks += (uint32_t)(clicks < 0 ? -clicks : clicks);
  sActivity++;

  /* One acceleration decision for the batch, then multiplied by how many
   * clicks were in it.
   *
   * Not one call per click. They all carry the same timestamp, because the
   * interrupt counts clicks and does not time them, so every call after the
   * first would see no gap at all and read as a full speed spin. A slow turn
   * that happened while the loop was busy elsewhere would then jump the dial
   * by twenty five steps. */
  int32_t direction = clicks < 0 ? -1 : 1;
  int32_t count = clicks < 0 ? -clicks : clicks;
  int32_t steps = direction * count *
                  (int32_t)accelerationSteps(&sAcceleration, NULL, nowMs);

  /* The queue carries whole steps in an int16_t, and the state machine caps
   * the move at a band's width anyway. */
  if (steps > 1000) {
    steps = 1000;
  } else if (steps < -1000) {
    steps = -1000;
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_STEP;
  cmd.steps = (int16_t)steps;
  send(&cmd);
}

/*
 * Send one command, wait for the radio, and say where it ended up.
 *
 * The buttons all mean "the next one", and the radio works that out from its
 * own state. Reading the state here, working out the next value and sending
 * that would leave a gap for the state to move in, which is how BW came to
 * send an FM bandwidth to a radio that had just landed on medium wave.
 */
static void cycleAndNote(RadioCommandKind kind, const char *what) {
  RadioCommand cmd = {};
  cmd.kind = kind;

  RadioError why = RADIO_OK;
  if (radioPostAndSettle(&cmd, BUTTON_SETTLE_MS, &why) != RADIO_POST_DONE ||
      why != RADIO_OK) {
    note(why != RADIO_OK ? radioErrorText(why) : "the radio is busy");
    return;
  }

  /* What it actually reached, read back after it settled. */
  RadioSnapshot now;
  if (!radioGetSnapshot(&now)) {
    note(what != NULL ? what : "done");
    return;
  }

  char text[INPUT_EVENT_MAX];
  switch (kind) {
    case RADIO_CYCLE_BAND:
      snprintf(text, sizeof(text), "%s", bandName(now.settings.band));
      break;
    case RADIO_CYCLE_BANDWIDTH:
      if (now.settings.bandwidthKHz == 0) {
        snprintf(text, sizeof(text), "BW automatic");
      } else {
        snprintf(text, sizeof(text), "BW %u kHz",
                 (unsigned)now.settings.bandwidthKHz);
      }
      break;
    case RADIO_CYCLE_TUNE_MODE:
      snprintf(text, sizeof(text), "%s", tuneModeName(now.settings.tuneMode));
      break;
    case RADIO_TOGGLE_MUTE:
      snprintf(text, sizeof(text), "%s",
               now.settings.muted ? "muted" : "unmuted");
      break;
    case RADIO_CYCLE_FM_FEATURES:
      /* Both named every time, on or off, so the button says which of the
       * four states it landed in rather than only what changed. */
      snprintf(text, sizeof(text), "iMS %s, EQ %s",
               now.settings.multipathSuppression ? "on" : "off",
               now.settings.equalizer ? "on" : "off");
      break;
    default:
      snprintf(text, sizeof(text), "%s", what != NULL ? what : "done");
      break;
  }
  note(text);
}

static void onBand(ButtonEvent event) {
  if (event == BUTTON_SHORT) {
    cycleAndNote(RADIO_CYCLE_BAND, NULL);
    return;
  }
  if (event == BUTTON_LONG) {
    /* The full RDS screen goes here. It needs a display, which is phase 4, so
     * for now the press is recorded and nothing else happens. Recording it is
     * what proves the wiring works before there is anything to show. */
    note("BAND long, RDS screen is phase 4");
  }
}

static void onBandwidth(ButtonEvent event) {
  if (event == BUTTON_SHORT) {
    cycleAndNote(RADIO_CYCLE_BANDWIDTH, NULL);
  }
}

static void onMode(ButtonEvent event) {
  if (event == BUTTON_SHORT) {
    cycleAndNote(RADIO_CYCLE_TUNE_MODE, NULL);
    return;
  }
  if (event == BUTTON_LONG) {
    /* iMS and the channel equalizer, the two features the radio this
     * replaces gives their own badge. One gesture for two settings, and the
     * radio says which of the four it reached, because a cycle with no
     * feedback leaves a person counting presses. */
    cycleAndNote(RADIO_CYCLE_FM_FEATURES, NULL);
  }
}

static void onPush(ButtonEvent event) {
  if (event == BUTTON_SHORT) {
    cycleAndNote(RADIO_TOGGLE_MUTE, NULL);
    return;
  }
  if (event == BUTTON_LONG) {
    /* The menu goes here, in phase 4. */
    note("PUSH long, menu is phase 4");
  }
}

static void pollButtons(uint32_t nowMs) {
  for (int i = 0; i < PANEL_BUTTON_COUNT; i++) {
    PanelButton which = (PanelButton)i;
    ButtonEvent event =
        buttonFeed(&sButtons[i], NULL, encoderButtonDown(which), nowMs);
    if (event == BUTTON_NONE) {
      continue;
    }
    sStatus.presses++;
    sActivity++;
    /* A long press only, and longer than a keypad tick so the two are told
     * apart by ear. A short press needs nothing: the band changes, the filter
     * changes, the sound stops, and the result is the feedback. A long press
     * has none of that, and the moment it fires is decided by a timer rather
     * than by letting go. */
    if (sBeepMode >= BEEP_EVERY_PRESS ||
        (sBeepMode >= BEEP_KEYS_AND_LONG && event == BUTTON_LONG)) {
      radioBeep(event == BUTTON_LONG ? BEEP_LONG_MS : BEEP_MS);
    }

    char seen[INPUT_EVENT_MAX];
    snprintf(seen, sizeof(seen), "%s %s", panelButtonName(which),
             buttonEventName(event));
    note(seen);

    switch (which) {
      case PANEL_BUTTON_BAND:
        onBand(event);
        break;
      case PANEL_BUTTON_BW:
        onBandwidth(event);
        break;
      case PANEL_BUTTON_MODE:
        onMode(event);
        break;
      case PANEL_BUTTON_ENCODER:
        onPush(event);
        break;
      default:
        break;
    }
  }
}

static void enterTyped(void) {
  if (sTypedLen == 0) {
    note("enter with nothing typed");
    return;
  }

  uint32_t typed = (uint32_t)strtoul(sTyped, NULL, 10);
  BandPlanConfig plan;
  RadioSnapshot now;
  BandId prefer = BAND_COUNT;
  if (radioGetSnapshot(&now)) {
    prefer = now.settings.band;
  }

  char text[INPUT_EVENT_MAX];
  uint32_t khz = 0;
  BandId band = BAND_COUNT;
  if (!radioTaskPlan(&plan) ||
      !bandFromTypedNumber(&plan, typed, prefer, &khz, &band)) {
    snprintf(text, sizeof(text), "%s is in no band", sTyped);
    note(text);
    clearTyped();
    return;
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_TUNE;
  cmd.freqKHz = khz;
  if (!sendAndSettle(&cmd)) {
    snprintf(text, sizeof(text), "%s was not tuned", sTyped);
    note(text);
    clearTyped();
    return;
  }

  char freq[16];
  bandFormatFrequency(band, khz, freq, sizeof(freq));
  snprintf(text, sizeof(text), "%s %s", bandName(band), freq);
  note(text);
  clearTyped();
}

static void pollKeypad(uint32_t nowMs) {
  static uint32_t lastPollMs = 0;
  if (!sStatus.keypadPresent) {
    return;
  }
  if ((uint32_t)(nowMs - lastPollMs) < KEYPAD_POLL_MS) {
    return;
  }
  lastPollMs = nowMs;

  /* A number left half typed is dropped, so the next person to press a key
   * is not silently continuing somebody else's. */
  if (sTypedLen > 0 && (uint32_t)(nowMs - sTypedMs) >= TYPED_TIMEOUT_MS) {
    note("typed number timed out");
    clearTyped();
  }

  /* One read of the expander, giving both the key and the raw lines. Reading
   * it twice doubles the traffic on the tuner's bus and lets the two answers
   * disagree, so the diagnostic could show a key the radio never acted on.
   *
   * The raw lines go out whether or not they mean a key. A button that is
   * fitted but not in the map reads as a line going low and nothing else
   * happening, and that is the only way to tell it from a dead switch. */
  int8_t key = KEYPAD_NONE;
  uint16_t lines = 0;
  if (!keypadRead(&key, &lines)) {
    return;
  }
  sStatus.lines = lines;
  sStatus.linesOk = 1;
  if (key == KEYPAD_NONE) {
    return;
  }
  sStatus.presses++;
  sActivity++;
  if (sBeepMode >= BEEP_KEYS) {
    /* Every key, including the ones that go on to be refused. The beep says
     * the press was seen, which is the question a person is asking when they
     * press a key and nothing happens. */
    radioBeep(BEEP_MS);
  }

  if (key == KEYPAD_ENTER) {
    enterTyped();
    return;
  }

  if (key == KEYPAD_DX) {
    /* The DX menu goes here. It sets the scan sensitivity, which needs the
     * settings work in issue 14 and a screen to show it on. The press is
     * recorded so the key can be seen to work before either exists. */
    note("DX, its menu is phase 4");
    return;
  }

  if (sTypedLen >= INPUT_DIGITS_MAX) {
    /* Past what any frequency needs. Ignore the digit rather than losing the
     * front of the number, which is the part that says which band it is.
     *
     * The timeout is deliberately not refreshed here. A digit that was thrown
     * away must not keep a full and unusable buffer alive. */
    note("too many digits");
    return;
  }
  /* Only a digit that was actually kept restarts the clock. */
  sTypedMs = nowMs;
  sTyped[sTypedLen++] = (char)('0' + key);
  sTyped[sTypedLen] = '\0';
  memcpy(sStatus.typed, sTyped, sizeof(sTyped));

  char text[INPUT_EVENT_MAX];
  snprintf(text, sizeof(text), "key %d, typed %s", (int)key, sTyped);
  note(text);
}

/*
 * The knob.
 *
 * One knob, one job at a time, and the squelch mode decides which. Off and
 * Auto leave it as the volume; Manual takes it for the squelch threshold.
 *
 * The job changing is the awkward part. Whichever it becomes has to be
 * applied at once from where the knob is now, or the setting it took over
 * keeps a value from a knob position that is long gone: coming back from
 * Manual with the volume stuck where it was before, or entering Manual with
 * a threshold from an old position that silences everything. Neither
 * recovers until the knob is moved past the deadband.
 */
static void pollPot(uint32_t nowMs) {
  static uint32_t lastPollMs = 0;
  if ((uint32_t)(nowMs - lastPollMs) < POT_POLL_MS) {
    return;
  }
  lastPollMs = nowMs;

  /* Asked for directly, not read out of the snapshot. The snapshot is
   * republished ten times a second, so for up to that long after a mode
   * change the knob would still be doing its old job. */
  SquelchMode mode = radioSquelchMode(NULL);
  bool jobChanged = !sJobKnown || mode != sJob;
  sJob = mode;
  sJobKnown = true;

  uint16_t raw = potRead();
  sStatus.pot = raw;

  /* While calibrating the knob only records how far it reaches. It does not
   * set the volume or the squelch, because finding the loud end stop should
   * not mean sweeping the volume to full on the way.
   *
   * potCalibrateSample gives up on its own after a couple of minutes, so a
   * calibration somebody walked away from cannot leave the knob dead. */
  if (sCal.active) {
    /* Somebody is standing at the radio sweeping the knob end to end, so
     * every poll counts as them using it. Without this the return below
     * skips the activity count and the panel dims under their hand halfway
     * through the calibration they are doing. potCalibrateSample gives up on
     * its own after a couple of minutes, so this cannot hold the panel lit
     * for a radio somebody walked away from. */
    sActivity++;
    if (potCalibrateSample(&sCal, raw, nowMs)) {
      return;
    }
    /* It just timed out. Fall through, so the knob takes its job back on
     * this same poll rather than on the next movement. */
    sPotKnown = false;
  }

  bool moved = potMoved(sPot, raw, &sPotCfg);
  if (!jobChanged && sPotKnown && !moved) {
    return;
  }
  /* Only a real turn counts as somebody using the radio. The reading is taken
   * twenty times a second and never sits perfectly still, so counting every
   * one of them would mean the radio was never left alone. */
  if (sPotKnown && moved) {
    sActivity++;
  }
  sPot = raw;
  sPotKnown = true;

  if (mode == SQUELCH_MANUAL) {
    /* The knob is the squelch now, so there is no volume to report from it.
     * Leaving the old number there would read as the volume the knob is
     * pointing at, which it is not. */
    sStatus.potDb = 0;
    int16_t tenths =
        squelchThresholdFromPot(raw, sPotCfg.rawMin, sPotCfg.rawMax);
    radioSetSquelchThreshold(tenths);
    char text[INPUT_EVENT_MAX];
    snprintf(text, sizeof(text), "squelch %s%d.%d dBuV",
             (tenths < 0 && tenths > -10) ? "-" : "", tenths / 10,
             (tenths < 0 ? -tenths : tenths) % 10);
    note(text);
    return;
  }

  int8_t db = potVolumeDb(raw, &sPotCfg);
  if (!jobChanged && db == sStatus.potDb) {
    /* The reading moved but not far enough to be a different volume. */
    return;
  }
  sStatus.potDb = db;

  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_VOLUME;
  cmd.volumeDb = db;
  /* Not settled. The knob can be turned faster than the radio can answer, and
   * waiting for each step would make it feel stiff. The last one sent wins. */
  send(&cmd);
}

void inputSetBeeps(BeepMode mode) {
  sBeepMode = mode < BEEP_MODE_COUNT ? mode : BEEP_OFF;
}

void inputSetPotConfig(const PotConfig *cfg) {
  if (cfg == NULL) {
    potDefaults(&sPotCfg);
    return;
  }
  sPotCfg = *cfg;
}

void inputPotCalibrateStart(void) {
  potCalibrateStart(&sCal, potRead(), millis());
}

bool inputPotCalibrateFinish(uint16_t *rawMin, uint16_t *rawMax) {
  /* Nothing to report unless a calibration was actually running. Reading the
   * extremes first would quote the previous sweep back as though this one had
   * happened. */
  bool running = sCal.active;
  if (rawMin != NULL) {
    *rawMin = running ? sCal.rawMin : 0;
  }
  if (rawMax != NULL) {
    *rawMax = running ? sCal.rawMax : 0;
  }
  if (!potCalibrateFinish(&sCal, &sPotCfg)) {
    return false;
  }
  /* The knob has moved a long way during the sweep, so the next poll acts on
   * where it has been left rather than comparing against a stale reading. */
  sPotKnown = false;
  return true;
}

void inputPotCalibrateCancel(void) {
  potCalibrateCancel(&sCal);
}

bool inputPotCalibrating(uint16_t *rawMin, uint16_t *rawMax) {
  if (rawMin != NULL) {
    *rawMin = sCal.rawMin;
  }
  if (rawMax != NULL) {
    *rawMax = sCal.rawMax;
  }
  return sCal.active;
}

uint32_t inputActivity(void) {
  return sActivity;
}

void inputPoll(void) {
  uint32_t nowMs = millis();
  pollEncoder(nowMs);
  pollButtons(nowMs);
  pollPot(nowMs);
  pollKeypad(nowMs);
}

void inputStatusGet(InputStatus *out) {
  if (out == NULL) {
    return;
  }
  *out = sStatus;
}
