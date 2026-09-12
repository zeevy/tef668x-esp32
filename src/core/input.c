/**
 * @file input.c
 * @brief Implementation of the knob and button state machines.
 */
#include "input.h"

#include <stddef.h>

/* ---------------------------------------------------------------- encoder */

/**
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

/**
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

void accelerationDefaults(AccelerationConfig *out) {
  if (out == NULL) {
    return;
  }
  out->spinMs = 15;
  out->fasterMs = 30;
  out->fastMs = 45;
  out->spinSteps = 6;
  out->fasterSteps = 4;
  out->fastSteps = 2;
}

uint8_t accelerationSteps(Acceleration *a, const AccelerationConfig *cfg,
                          uint32_t nowMs) {
  AccelerationConfig defaults;
  if (cfg == NULL) {
    accelerationDefaults(&defaults);
    cfg = &defaults;
  }
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

  if (gap < cfg->spinMs) {
    return cfg->spinSteps;
  }
  if (gap < cfg->fasterMs) {
    return cfg->fasterSteps;
  }
  if (gap < cfg->fastMs) {
    return cfg->fastSteps;
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
  out->dbMin = -30;
  out->dbMax = 0;
  out->deadband = 25;
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

  /* Widened to 32 bits before the multiplication. The travel is nearly four
   * thousand counts and the span is tens of dB, so the product does not fit
   * in sixteen bits. */
  int32_t span = (int32_t)cfg->dbMax - cfg->dbMin;
  int32_t along = (int32_t)raw - cfg->rawMin;
  int32_t width = (int32_t)cfg->rawMax - cfg->rawMin;
  return (int8_t)(cfg->dbMin + (along * span) / width);
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

void buttonDefaults(ButtonConfig *out) {
  if (out == NULL) {
    return;
  }
  out->debounceMs = 25;
  out->longMs = 600;
  out->doubleMs = 350;
  out->wantDouble = false;
}

const char *buttonEventName(ButtonEvent event) {
  switch (event) {
    case BUTTON_SHORT:
      return "short";
    case BUTTON_LONG:
      return "long";
    case BUTTON_DOUBLE:
      return "double";
    case BUTTON_NONE:
    default:
      return "none";
  }
}

ButtonEvent buttonFeed(Button *b, const ButtonConfig *cfg, bool pressed,
                       uint32_t nowMs) {
  ButtonConfig defaults;
  if (cfg == NULL) {
    buttonDefaults(&defaults);
    cfg = &defaults;
  }
  if (b == NULL) {
    return BUTTON_NONE;
  }

  /* Debounce first. A change only counts once the level has held still for
   * long enough, so one press is one press and not five. */
  if (pressed != b->raw) {
    b->raw = pressed;
    b->changedMs = nowMs;
  }
  bool settled = (uint32_t)(nowMs - b->changedMs) >= cfg->debounceMs;

  if (settled && b->raw != b->level) {
    b->level = b->raw;
    if (b->level) {
      b->pressedMs = nowMs;
      b->handled = false;
      if (b->waitingDouble) {
        b->waitingDouble = false;
        /* Only a pair that really is close together. The check for a single
         * press expiring only runs when this is called, so a caller that was
         * held up elsewhere can arrive with a press that should already have
         * been reported as a single, and calling that a double would be
         * wrong by however long the caller was away. */
        if ((uint32_t)(nowMs - b->releasedMs) < cfg->doubleMs) {
          /* The second press of a pair. Reported as soon as the finger goes
           * down, rather than making the person wait for it to come up. */
          b->handled = true;
          return BUTTON_DOUBLE;
        }
        /* Too late to be a double. The first press was a single after all,
         * and this one starts again. */
        return BUTTON_SHORT;
      }
    } else {
      b->releasedMs = nowMs;
      if (!b->handled) {
        if (!cfg->wantDouble) {
          /* Nothing is bound to a double press here, so there is nothing to
           * wait for. Report it now rather than a third of a second late. */
          return BUTTON_SHORT;
        }
        /* Hold it back to see whether a second press follows. */
        b->waitingDouble = true;
      }
    }
  }

  /* A long press happens while nothing is happening, so it is checked every
   * time round rather than only when the level changes. */
  if (b->level && !b->handled &&
      (uint32_t)(nowMs - b->pressedMs) >= cfg->longMs) {
    b->handled = true;
    b->waitingDouble = false;
    return BUTTON_LONG;
  }

  /* No second press came, so the first one was a single after all. */
  if (b->waitingDouble && !b->level &&
      (uint32_t)(nowMs - b->releasedMs) >= cfg->doubleMs) {
    b->waitingDouble = false;
    return BUTTON_SHORT;
  }

  return BUTTON_NONE;
}
