/* Implementation of the panel brightness. */
#include "backlight.h"

#include <stddef.h>

/* The brightness a panel comes up at when nobody has said otherwise. */
#define BACKLIGHT_DEFAULT_FULL 100

/*
 * The dim level a panel drops to, as a percentage.
 *
 * Measured on this radio rather than chosen. Stepping the panel from 70 per
 * cent down to 5 in daylight, the frequency stayed comfortable to read down
 * to 18 per cent, could still be identified at 8, and was legible with a
 * little effort at 5. See the backlight section of HARDWARE.md.
 *
 * 20 sits just above that comfortable floor. It is the dimmest the panel can
 * go while a glance still reads it, which is what a dim is for: save what
 * there is to save, and stop where the screen stops being useful.
 */
#define BACKLIGHT_DEFAULT_DIM 20

/*
 * How long the radio is left alone before it dims, in seconds. 0 never.
 *
 * Off, for the same reason the key beeps are off and decision 27 says the
 * chime is not. A panel that goes dark by itself is the one thing on this
 * list that reads as a fault, and a person who has not asked for it has no
 * reason to think the radio is working.
 */
#define BACKLIGHT_DEFAULT_DIM_AFTER_S 0

static uint8_t clampPercent(uint8_t percent) {
  return percent > 100 ? 100 : percent;
}

/*
 * Whole number square root, by the usual bit at a time method.
 *
 * Here so the fade can work in a perceptual scale without floating point on a
 * path that runs every time round the loop.
 */
static uint32_t isqrt32(uint32_t v) {
  uint32_t root = 0;
  uint32_t rem = v;
  uint32_t place = 1UL << 30;
  while (place > rem) {
    place >>= 2;
  }
  while (place != 0) {
    if (rem >= root + place) {
      rem -= root + place;
      root += place << 1;
    }
    root >>= 1;
    place >>= 2;
  }
  return root;
}

/*
 * A brightness on the scale an eye reads it on, 0 to 1000.
 *
 * The square root of the duty, times a hundred to keep the precision in whole
 * numbers. An eye's response to light is near enough the other way round, so
 * moving evenly along this scale looks like an even change.
 */
static uint32_t perceived(uint8_t percent) {
  return isqrt32((uint32_t)clampPercent(percent) * 10000UL);
}

/*
 * Where a fade should be by now, given where it started and where it ends.
 *
 * Even in what an eye sees, not in what the panel is driven with. That is not
 * a refinement, it is what makes the fade visible at all.
 *
 * Measured on this radio on 13 September 2026: the frequency is readable at 5
 * per cent duty. So a straight line from 0 to 100 over four tenths of a
 * second has the panel clearly lit about twenty milliseconds in, and spends
 * the remaining three hundred and eighty going from lit to slightly brighter,
 * which is a change the eye compresses to almost nothing. Tried that way
 * first and it could not be told from a snap in a blind comparison.
 *
 * Interpolating the square root instead and squaring it back puts the slow
 * part of the travel down at the bottom, where a change of duty is a change
 * you can see. Same start, same end, same length.
 */
static uint8_t fadeLevel(const Backlight *b, uint32_t nowMs) {
  uint32_t gone = nowMs - b->fadeStartMs;
  if (b->fadeMs == 0 || gone >= b->fadeMs) {
    return b->fadeTo;
  }
  int32_t from = (int32_t)perceived(b->fadeFrom);
  int32_t to = (int32_t)perceived(b->fadeTo);
  int32_t at = from + (to - from) * (int32_t)gone / (int32_t)b->fadeMs;
  /* At most 1000, so the square is at most a million and stays in 32 bits. */
  return (uint8_t)(((uint32_t)at * (uint32_t)at) / 10000UL);
}

static void startFade(Backlight *b, uint8_t to, uint16_t ms, uint32_t nowMs) {
  to = clampPercent(to);
  if (ms == 0 || to == b->level) {
    b->level = to;
    b->fadeMs = 0;
    return;
  }
  b->fadeFrom = b->level;
  b->fadeTo = to;
  b->fadeMs = ms;
  b->fadeStartMs = nowMs;
}

static void jumpTo(Backlight *b, uint8_t to) {
  b->level = clampPercent(to);
  b->fadeMs = 0;
}

static uint8_t restingLevel(const Backlight *b) {
  if (!b->dimmed) {
    return clampPercent(b->cfg.fullPercent);
  }
  /* Never brighter when dimmed than when awake. A dim level above the full
   * one would otherwise make the panel come up when it was left alone. */
  uint8_t dim = clampPercent(b->cfg.dimPercent);
  uint8_t full = clampPercent(b->cfg.fullPercent);
  return dim < full ? dim : full;
}

void backlightDefaults(BacklightConfig *cfg) {
  if (cfg == NULL) {
    return;
  }
  cfg->fullPercent = BACKLIGHT_DEFAULT_FULL;
  cfg->dimPercent = BACKLIGHT_DEFAULT_DIM;
  cfg->dimAfterMs = (uint32_t)BACKLIGHT_DEFAULT_DIM_AFTER_S * 1000;
  cfg->fadeUpMs = BACKLIGHT_FADE_UP_MS;
}

void backlightFromSettings(const Settings *s, BacklightConfig *out) {
  if (out == NULL) {
    return;
  }
  backlightDefaults(out);
  if (s == NULL) {
    return;
  }
  out->fullPercent = s->backlightPercent;
  out->dimPercent = s->backlightDimPercent;
  out->dimAfterMs = (uint32_t)s->backlightDimAfterS * 1000;
  out->fadeUpMs = s->backlightFade != 0 ? BACKLIGHT_FADE_UP_MS : 0;
}

void backlightInit(Backlight *b, const BacklightConfig *cfg, uint32_t nowMs) {
  if (b == NULL) {
    return;
  }
  if (cfg != NULL) {
    b->cfg = *cfg;
  } else {
    backlightDefaults(&b->cfg);
  }
  b->level = 0;
  b->dimmed = false;
  b->lastInputMs = nowMs;
  b->fadeMs = 0;
  b->fadeFrom = 0;
  b->fadeTo = 0;
  b->fadeStartMs = nowMs;
  startFade(b, b->cfg.fullPercent, b->cfg.fadeUpMs, nowMs);
}

void backlightSetConfig(Backlight *b, const BacklightConfig *cfg,
                        uint32_t nowMs) {
  if (b == NULL) {
    return;
  }
  uint32_t wasAfterMs = b->cfg.dimAfterMs;
  if (cfg != NULL) {
    b->cfg = *cfg;
  } else {
    backlightDefaults(&b->cfg);
  }
  /* A change to the delay starts it again from now.
   *
   * Both directions need it. Measuring a new delay against a radio that was
   * already idle means somebody who has been at the browser for a minute and
   * then switches the dim on watches the panel drop within milliseconds
   * rather than after the ten seconds they asked for. And a dim switched off
   * leaves a panel that had already dropped sitting at the old level with
   * nothing to bring it back, because the only other thing that lifts one is
   * an input.
   *
   * Changing the levels alone is not a change to the delay, so a panel that
   * is already dimmed stays dimmed and moves to the new dim level. That is
   * what makes a dim level choosable: it can be seen while it applies. */
  if (b->cfg.dimAfterMs != wasAfterMs) {
    b->dimmed = false;
    b->lastInputMs = nowMs;
  }
  jumpTo(b, restingLevel(b));
}

void backlightWake(Backlight *b, uint32_t nowMs) {
  if (b == NULL) {
    return;
  }
  b->lastInputMs = nowMs;
  if (b->dimmed) {
    /* Straight there, which also cancels the drop if it is still running.
     * The flag goes up on the same call that starts the fade down, so a
     * press halfway through the drop comes through here and the panel does
     * not carry on darkening under the hand of the person pressing. */
    b->dimmed = false;
    jumpTo(b, restingLevel(b));
  }
}

uint8_t backlightUpdate(Backlight *b, uint32_t nowMs) {
  if (b == NULL) {
    return 0;
  }
  if (b->fadeMs != 0) {
    b->level = fadeLevel(b, nowMs);
    if ((uint32_t)(nowMs - b->fadeStartMs) >= b->fadeMs) {
      b->fadeMs = 0;
    }
    return b->level;
  }

  if (!b->dimmed && b->cfg.dimAfterMs != 0 &&
      (uint32_t)(nowMs - b->lastInputMs) >= b->cfg.dimAfterMs) {
    b->dimmed = true;
    startFade(b, restingLevel(b), BACKLIGHT_FADE_DOWN_MS, nowMs);
    if (b->fadeMs != 0) {
      /* The first step of the fade goes out on this call rather than the
       * next, so a caller that only writes the panel when the level changes
       * does not sit a whole poll interval at the old brightness. Guarded,
       * because a dim to the level it is already at starts no fade and
       * fadeTo then still holds whatever the last fade aimed at. */
      b->level = fadeLevel(b, nowMs);
    }
  }
  return b->level;
}

uint8_t backlightLevel(const Backlight *b) {
  return b != NULL ? b->level : 0;
}

bool backlightDimmed(const Backlight *b) {
  return b != NULL && b->dimmed;
}

bool backlightFading(const Backlight *b) {
  return b != NULL && b->fadeMs != 0;
}
