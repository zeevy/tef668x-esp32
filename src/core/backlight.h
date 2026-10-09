/*
 * How bright the panel should be right now.
 *
 * Three behaviours, all decided here and none of them touching a pin: the
 * panel comes up with a fade rather than a snap, it drops to a lower level
 * after a while with nobody touching the radio, and it goes back to full the
 * moment somebody does.
 *
 * It is in `core/` because it is a decision with timing in it, and timing is
 * what cannot be checked by looking at a radio. A dim that never fires and a
 * wake that never arrives both look like a panel that is simply on.
 *
 * The caller gives it the time in milliseconds since boot, the same
 * `millis()` the rest of the firmware uses, and gets back a percentage to
 * hand to the panel driver. Every comparison is done on a difference, so the
 * wrap after about forty nine days passes without a jump.
 */
#ifndef CORE_BACKLIGHT_H
#define CORE_BACKLIGHT_H

#include <stdbool.h>
#include <stdint.h>

#include "settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * How long the panel takes to come up at boot, in milliseconds.
 *
 * Measured on this radio in a blind comparison of a boot with the fade against
 * one without: four tenths of a second cannot be told from the light simply
 * switching on, two and a half seconds reads as obviously slow, and one second
 * reads as a fade.
 *
 * 750 is a choice inside that range, not a measurement. The same ramp is also
 * used for the swap from the boot screen to the radio. 750 is comfortably above
 * the four tenths that reads as a switch, and against one second it takes a
 * quarter of a second off both ends of start up.
 *
 * The fade runs inside start up and blocks it, so this is time added to how
 * long the radio takes to come on.
 */
#define BACKLIGHT_FADE_UP_MS 750

/*
 * How long the drop to the dim level takes, in milliseconds.
 *
 * The same as the fade up, and it moves with it. A radio whose light goes out
 * faster than it comes on reads as two mechanisms rather than one, and this
 * panel has one ramp speed wherever the light moves: start up, the dim, and
 * the swap from the boot screen to the radio.
 *
 * The measurement behind it is the one written above: a second reads as a
 * fade rather than a step, and a drop from full to a fifth over that time is
 * seen to move, not jump. 750 is the chosen value inside the same range.
 */
#define BACKLIGHT_FADE_DOWN_MS 750

/*
 * The dimmest the panel may be set to when it is awake, as a percentage.
 *
 * Measured, not a round number. 5 per cent is the lowest setting at which the
 * frequency could still be read off this panel in daylight, and below it the
 * only control for a panel nobody can read is the web page that has just gone
 * dark.
 */
#define BACKLIGHT_MIN_AWAKE 5

/* The longest the radio may be left before it dims, in seconds. */
#define BACKLIGHT_DIM_AFTER_MAX_S 240

/* What the panel is asked to do. Milliseconds and percentages throughout. */
typedef struct {
  uint8_t fullPercent; /* Brightness while somebody is using the radio. */
  /*
   * Brightness once it has been left alone.
   *
   * A value at or above `fullPercent` means the dim changes nothing, which
   * is a way of switching it off without a second setting saying so.
   */
  uint8_t dimPercent;
  uint32_t dimAfterMs; /* How long with no input before it dims. 0 never. */
  /* How long the fade at boot takes: BACKLIGHT_FADE_UP_MS on the radio.
   * 0 snaps on, which only the tests use. */
  uint16_t fadeUpMs;
} BacklightConfig;

/* Where the fade has got to. Zero it before first use. */
typedef struct {
  BacklightConfig cfg;  /* What it was asked to do. */
  uint32_t fadeStartMs; /* When the fade running now began. */
  uint32_t lastInputMs; /* When somebody last touched the radio. */
  uint16_t fadeMs;      /* How long that fade lasts. 0 means none is on. */
  uint8_t fadeFrom;     /* The level it started from. */
  uint8_t fadeTo;       /* The level it is going to. */
  uint8_t level;        /* What the panel should be at now. */
  bool dimmed;          /* It has been left alone and has dropped. */
} Backlight;

void backlightDefaults(BacklightConfig *cfg);

/*
 * Fill a config in from the stored settings.
 *
 * The one place that conversion happens, for the same reason
 * radioPlanFromSettings is the only place the band plan is built: a second
 * copy assembled somewhere else drifts, and then the page and the panel
 * disagree about what a setting means.
 */
void backlightFromSettings(const Settings *s, BacklightConfig *out);

void backlightInit(Backlight *b, const BacklightConfig *cfg, uint32_t nowMs);

/*
 * Change what it should do, and act on it at once.
 *
 * At once rather than at the next fade, because the only way to choose a dim
 * level is to look at the panel set to it. A setting that could not be seen
 * until the radio had been left alone for a minute could not be chosen at
 * all.
 *
 * Changing the delay starts the idle clock again from now, in both
 * directions. Switching the dim on after a spell at the browser must not drop
 * the panel at once, and switching it off has to lift a panel that had
 * already dropped, because the only other thing that lifts one is an input.
 *
 * Changing only the levels leaves the clock alone. That is not somebody
 * touching the radio, so a panel that had already dimmed stays dimmed and
 * moves to the new dim level, which is what makes a dim level choosable.
 */
void backlightSetConfig(Backlight *b, const BacklightConfig *cfg,
                        uint32_t nowMs);

/*
 * Somebody touched the radio.
 *
 * Full brightness comes back at once with no fade. A person who has just
 * pressed a key is looking at the panel already, so a fade would only be a
 * delay between the press and being able to read the answer.
 */
void backlightWake(Backlight *b, uint32_t nowMs);

/*
 * Move the fade along and say what the panel should be at.
 *
 * Call it as often as convenient. It does no work of its own beyond a
 * subtraction when nothing is moving.
 */
uint8_t backlightUpdate(Backlight *b, uint32_t nowMs);

uint8_t backlightLevel(const Backlight *b);

bool backlightDimmed(const Backlight *b);

/*
 * Whether a fade is running.
 *
 * For a caller that wants to drive the fade out at boot and stop when it is
 * over, rather than count milliseconds of its own.
 */
bool backlightFading(const Backlight *b);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BACKLIGHT_H */
