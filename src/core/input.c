/* Implementation of the knob and button state machines. */
#include "input.h"

#include <stddef.h>
#include <string.h>

/* ---------------------------------------------------------------- encoder */

/*
 * What each pair of readings means.
 *
 * The index is the old two bits followed by the new two bits. A valid step
 * either way gives +1 or -1. A repeat gives 0, and so does an impossible jump
 * of both lines at once, which is what a bouncing contact looks like. Reading
 * it off a table rather than working it out with ifs is what keeps this
 * short enough to be obviously right.
 */
static const int8_t kTransitions[16] = {0,  -1, 1, 0, 1, 0, 0,  -1,
                                        -1, 0,  0, 1, 0, 1, -1, 0};

/*
 * How many transitions make one click.
 *
 * The optical variant puts out more edges for the same movement, so it needs
 * a higher count or the dial moves twice as far as the hand did.
 */
static int8_t detentThreshold(EncoderKind kind) {
  return kind == ENCODER_OPTICAL ? 4 : 3;
}

void encoderInit(Encoder *e, EncoderKind kind, EncoderDirection direction) {
  if (e == NULL) {
    return;
  }
  e->history = 0;
  e->count = 0;
  e->started = false;
  e->kind = kind;
  e->direction = direction;
}

int8_t encoderFeed(Encoder *e, bool a, bool b) {
  if (e == NULL) {
    return 0;
  }

  uint8_t now = (uint8_t)((a ? 0x02 : 0x00) | (b ? 0x01 : 0x00));

  if (!e->started) {
    /* Nothing to compare with yet. Remember where the knob is sitting and
     * report nothing, or the radio moves itself at power on. */
    e->history = now;
    e->started = true;
    return 0;
  }

  e->history = (uint8_t)(((e->history << 2) | now) & 0x0F);
  e->count = (int8_t)(e->count + kTransitions[e->history]);

  int8_t threshold = detentThreshold(e->kind);
  int8_t detent = 0;
  if (e->count > threshold) {
    detent = 1;
  } else if (e->count < -threshold) {
    detent = -1;
  }
  if (detent != 0) {
    e->count = 0;
    if (e->direction == ENCODER_REVERSED) {
      detent = (int8_t)-detent;
    }
  }
  return detent;
}

/* ----------------------------------------------------------- acceleration */

int32_t inputKnobSteps(int32_t clicks, uint8_t factor, bool list) {
  int32_t steps = list ? clicks : clicks * (int32_t)factor;
  if (steps > INPUT_KNOB_MAX_STEPS) {
    steps = INPUT_KNOB_MAX_STEPS;
  } else if (steps < -INPUT_KNOB_MAX_STEPS) {
    steps = -INPUT_KNOB_MAX_STEPS;
  }
  return steps;
}

uint8_t accelerationSteps(Acceleration *a, uint32_t nowMs) {
  if (a == NULL) {
    return 1;
  }

  if (!a->started) {
    a->started = true;
    a->lastMs = nowMs;
    return 1;
  }

  /* By subtraction, so a knob turned across the millis() wrap does not
   * suddenly accelerate. */
  uint32_t gap = nowMs - a->lastMs;
  a->lastMs = nowMs;

  if (gap < ACCEL_SPIN_MS) {
    return ACCEL_SPIN_STEPS;
  }
  if (gap < ACCEL_FASTER_MS) {
    return ACCEL_FASTER_STEPS;
  }
  if (gap < ACCEL_FAST_MS) {
    return ACCEL_FAST_STEPS;
  }
  return 1;
}

/* -------------------------------------------------------------------- pot */

void potDefaults(PotConfig *out) {
  if (out == NULL) {
    return;
  }
  out->rawMute = 100;
  out->rawMin = 120;
  out->rawMax = 4000;
  out->dbMute = -60;
  out->dbMin = -60;
  out->dbKnee = -30;
  out->kneePercent = 10;
  out->dbMax = 0;
  out->deadband = 25;
}

void potCalibrateStart(PotCalibration *c, uint16_t raw, uint32_t nowMs) {
  if (c == NULL) {
    return;
  }
  c->active = true;
  c->rawMin = raw;
  c->rawMax = raw;
  c->startedMs = nowMs;
}

bool potCalibrateSample(PotCalibration *c, uint16_t raw, uint32_t nowMs) {
  if (c == NULL || !c->active) {
    return false;
  }
  if ((uint32_t)(nowMs - c->startedMs) >= POT_CALIBRATE_TIMEOUT_MS) {
    /* Given up on. The knob has been doing nothing all this time, and a
     * radio with no working volume control and no explanation is worse than
     * an uncalibrated one. */
    c->active = false;
    return false;
  }
  if (raw < c->rawMin) {
    c->rawMin = raw;
  }
  if (raw > c->rawMax) {
    c->rawMax = raw;
  }
  return true;
}

void potApplyCalibration(PotConfig *cfg, uint16_t rawMin, uint16_t rawMax) {
  if (cfg == NULL || rawMax <= rawMin) {
    return;
  }
  uint16_t span = (uint16_t)(rawMax - rawMin);
  /* The bottom 2.5 per cent mutes, and the audible travel starts just above
   * it. On a knob reading 0 to 4095 that works out at 102 and 122, which is
   * where the built in 100 and 120 sit. */
  cfg->rawMute = (uint16_t)(rawMin + span / 40);
  cfg->rawMin = (uint16_t)(cfg->rawMute + span / 200);
  cfg->rawMax = rawMax;
}

bool potCalibrateFinish(PotCalibration *c, PotConfig *cfg) {
  if (c == NULL) {
    return false;
  }
  /* Only a running calibration can be finished. Without this a finish after
   * a cancel would apply the extremes the cancelled sweep recorded, so
   * cancelling would not cancel anything. */
  bool wasActive = c->active;
  c->active = false;
  if (!wasActive || cfg == NULL) {
    return false;
  }
  if (c->rawMax <= c->rawMin ||
      (uint16_t)(c->rawMax - c->rawMin) < POT_CALIBRATE_MIN_SPAN) {
    return false;
  }
  potApplyCalibration(cfg, c->rawMin, c->rawMax);
  return true;
}

void potCalibrateCancel(PotCalibration *c) {
  if (c == NULL) {
    return;
  }
  c->active = false;
}

int8_t potVolumeDb(uint16_t raw, const PotConfig *cfg) {
  PotConfig defaults;
  if (cfg == NULL) {
    potDefaults(&defaults);
    cfg = &defaults;
  }

  if (raw <= cfg->rawMute) {
    return cfg->dbMute;
  }
  if (raw < cfg->rawMin) {
    raw = cfg->rawMin;
  }
  if (raw > cfg->rawMax) {
    raw = cfg->rawMax;
  }
  if (cfg->rawMax <= cfg->rawMin) {
    return cfg->dbMax;
  }

  /* Two straight lines meeting at the knee. Widened to 32 bits before the
   * multiplication: the travel is nearly four thousand counts and the span is
   * tens of dB, so the product does not fit in sixteen bits. */
  const int32_t along = (int32_t)raw - cfg->rawMin;
  const int32_t width = (int32_t)cfg->rawMax - cfg->rawMin;
  const int32_t knee = width * cfg->kneePercent / 100;
  if (along < knee) {
    return (int8_t)(cfg->dbMin +
                    (along * ((int32_t)cfg->dbKnee - cfg->dbMin)) / knee);
  }
  if (knee >= width) {
    return cfg->dbMax;
  }
  return (int8_t)(cfg->dbKnee +
                  ((along - knee) * ((int32_t)cfg->dbMax - cfg->dbKnee)) /
                      (width - knee));
}

bool potMoved(uint16_t previous, uint16_t now, const PotConfig *cfg) {
  PotConfig defaults;
  if (cfg == NULL) {
    potDefaults(&defaults);
    cfg = &defaults;
  }
  uint16_t apart =
      now > previous ? (uint16_t)(now - previous) : (uint16_t)(previous - now);
  return apart >= cfg->deadband;
}

/* ----------------------------------------------------------------- button */

const char *buttonEventName(ButtonEvent event) {
  switch (event) {
    case BUTTON_SHORT:
      return "short";
    case BUTTON_LONG:
      return "long";
    case BUTTON_NONE:
    default:
      return "none";
  }
}

bool buttonEventFromName(const char *name, ButtonEvent *out) {
  if (name == NULL || out == NULL) {
    return false;
  }
  if (strcmp(name, "short") == 0) {
    *out = BUTTON_SHORT;
    return true;
  }
  if (strcmp(name, "long") == 0) {
    *out = BUTTON_LONG;
    return true;
  }
  return false;
}

/* In the order of InputKey, up to the digits. */
static const char *const kKeyNames[INPUT_KEY_DIGIT_0] = {
    "BAND", "BW", "MODE", "PUSH", "ENTER", "DX"};
static const char *const kDigitNames[10] = {"0", "1", "2", "3", "4",
                                            "5", "6", "7", "8", "9"};

const char *inputKeyName(InputKey key) {
  if (key < INPUT_KEY_DIGIT_0) {
    return kKeyNames[key];
  }
  return key < INPUT_KEY_COUNT ? kDigitNames[key - INPUT_KEY_DIGIT_0] : "?";
}

bool inputKeyFromName(const char *name, InputKey *out) {
  if (name == NULL || out == NULL) {
    return false;
  }
  for (int k = 0; k < INPUT_KEY_COUNT; k++) {
    if (strcmp(name, inputKeyName((InputKey)k)) == 0) {
      *out = (InputKey)k;
      return true;
    }
  }
  return false;
}

bool inputKeyTakesLong(InputKey key) {
  return key <= INPUT_KEY_ENTER;
}

bool inputAfterMenuQuiet(bool shut, uint32_t shutMs, uint32_t nowMs) {
  return shut && (uint32_t)(nowMs - shutMs) < INPUT_AFTER_MENU_QUIET_MS;
}

bool inputPageIdle(uint32_t lastMs, uint32_t nowMs) {
  return (uint32_t)(nowMs - lastMs) >= INPUT_PAGE_IDLE_MS;
}

void buttonStartHeld(Button *b, uint32_t nowMs) {
  if (b == NULL) {
    return;
  }
  b->level = true;
  b->raw = true;
  b->handled = true;
  b->changedMs = nowMs;
  b->pressedMs = nowMs;
}

ButtonEvent buttonFeed(Button *b, bool pressed, uint32_t nowMs) {
  if (b == NULL) {
    return BUTTON_NONE;
  }

  /* Debounce first. A change only counts once the level has held still for
   * long enough, so one press is one press and not five. */
  if (pressed != b->raw) {
    b->raw = pressed;
    b->changedMs = nowMs;
  }
  bool settled = (uint32_t)(nowMs - b->changedMs) >= BUTTON_DEBOUNCE_MS;

  if (settled && b->raw != b->level) {
    b->level = b->raw;
    if (b->level) {
      b->pressedMs = nowMs;
      b->handled = false;
    } else if (!b->handled) {
      return BUTTON_SHORT;
    }
  }

  /* A long press happens while nothing is happening, so it is checked every
   * time round rather than only when the level changes. */
  if (b->level && !b->handled &&
      (uint32_t)(nowMs - b->pressedMs) >= BUTTON_LONG_MS) {
    b->handled = true;
    return BUTTON_LONG;
  }

  return BUTTON_NONE;
}

/* ------------------------------------------------------------ keypad hold */

ButtonEvent keypadHoldFeed(KeypadHold *h, int8_t down, uint32_t nowMs,
                           int8_t *key) {
  if (h == NULL) {
    if (key != NULL) {
      *key = -1;
    }
    return BUTTON_NONE;
  }

  bool pressed = h->active && down == h->key;
  ButtonEvent event = buttonFeed(&h->button, pressed, nowMs);
  if (key != NULL) {
    *key = event != BUTTON_NONE ? h->key : -1;
  }

  /*
   * Only once the watched key is fully at rest does this take up whatever
   * is down now, or go idle waiting for something to be. A second key
   * appearing while the first is still down is left alone rather than
   * guessed at: it gets no watch of its own until the first one lets go and
   * buttonFeed's own debounce says so, the same ambiguity the keypad driver
   * refuses to turn into an answer.
   */
  if (!h->button.level && (!h->active || down != h->key)) {
    h->active = down >= 0;
    if (h->active) {
      h->key = down;
      memset(&h->button, 0, sizeof(h->button));
    }
  }

  return event;
}
