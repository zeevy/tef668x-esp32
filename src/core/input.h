/**
 * @file input.h
 * @brief Turning knobs and pressing keys, with no pins in sight.
 *
 * Three small state machines live here. A quadrature decoder that turns two
 * changing levels into detents. A button that tells a short press from a long
 * one and from a double. And an acceleration rule that says how far one
 * detent should move the dial when the knob is being spun.
 *
 * None of them read a pin or know what a pin is. They are fed levels and a
 * millisecond count, which is what makes them testable on a PC: a double
 * press that arrives one millisecond too late, a press held exactly on the
 * long press boundary, a knob turned back and forth across a detent. Those
 * are miserable to check by hand on a radio and easy to get wrong.
 *
 * Every millisecond count is compared by subtraction, so the machines keep
 * working across the 49 day wrap of millis().
 */
#ifndef CORE_INPUT_H
#define CORE_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------- encoder */

/**
 * Which encoder is fitted.
 *
 * Both exist on ATS-125 units, and they differ in how many quadrature steps
 * make one click of the knob. Getting it wrong does not fail, it just makes
 * the dial move twice as far as the hand did, or half as far, which is why
 * this is a setting and not a guess. Taken from the PE5PVB firmware, which
 * carries the same switch for the same reason.
 */
typedef enum {
  ENCODER_STANDARD, /**< Four transitions per click. The common part. */
  ENCODER_OPTICAL   /**< More transitions per click. The optical variant. */
} EncoderKind;

/** Which way round the knob is wired. */
typedef enum {
  ENCODER_NORMAL,  /**< Clockwise counts up. */
  ENCODER_REVERSED /**< Clockwise counts down. Some units are wired this way. */
} EncoderDirection;

/** A quadrature decoder. Zero it before first use. */
typedef struct {
  uint8_t history;            /**< The last two AB readings, four bits. */
  int8_t count;               /**< Transitions since the last detent. */
  bool started;               /**< A first reading has been taken. */
  EncoderKind kind;           /**< How many transitions make a detent. */
  EncoderDirection direction; /**< Which way round it is wired. */
} Encoder;

/**
 * Set an encoder up.
 *
 * @param e          The decoder. Cleared.
 * @param kind       Which part is fitted.
 * @param direction  Which way round it is wired.
 */
void encoderInit(Encoder *e, EncoderKind kind, EncoderDirection direction);

/**
 * Feed the decoder one reading of both lines.
 *
 * Call this every time either line changes, which on the radio means from an
 * interrupt on both pins.
 *
 * The first reading after encoderInit only establishes where the knob is
 * sitting. It never reports a detent, because there is nothing to compare it
 * with, and a decoder that invented one would move the dial at power on.
 *
 * @param e  The decoder.
 * @param a  The level on line A.
 * @param b  The level on line B.
 * @return +1 or -1 when a whole detent has been turned, otherwise 0.
 */
int8_t encoderFeed(Encoder *e, bool a, bool b);

/* ----------------------------------------------------------- acceleration */

/** How fast the knob has to be turning before the dial moves further. */
typedef struct {
  uint16_t fastMs;     /**< Detents closer together than this are fast. */
  uint16_t fasterMs;   /**< Closer than this is faster. */
  uint16_t spinMs;     /**< Closer than this is a flick of the wrist. */
  uint8_t fastSteps;   /**< Steps per detent when fast. */
  uint8_t fasterSteps; /**< Steps per detent when faster. */
  uint8_t spinSteps;   /**< Steps per detent when spinning. */
} AccelerationConfig;

/**
 * The acceleration the ATS-125 ships with.
 *
 * The thresholds are the ones in the PE5PVB firmware, which has been turned
 * by a lot of hands on this exact knob. They are not a guess, and they are
 * not arbitrary either: 15, 30 and 45 milliseconds between detents is roughly
 * 65, 33 and 22 clicks per second, which is the range a person can actually
 * produce on a knob this size.
 *
 * @param out  Receives the defaults.
 */
void accelerationDefaults(AccelerationConfig *out);

/** Works out how far one detent should move the dial. Zero before use. */
typedef struct {
  uint32_t lastMs; /**< When the previous detent arrived. */
  bool started;    /**< A first detent has been seen. */
} Acceleration;

/**
 * How many steps this detent is worth.
 *
 * @param a    The accelerator.
 * @param cfg  The thresholds. NULL means the defaults.
 * @param nowMs  The millisecond count now.
 * @return At least 1. More when the knob is being turned quickly.
 */
uint8_t accelerationSteps(Acceleration *a, const AccelerationConfig *cfg,
                          uint32_t nowMs);

/* -------------------------------------------------------------------- pot */

/** How the pot's travel maps to volume. */
typedef struct {
  uint16_t rawMute;  /**< At or below this the pot is off. */
  uint16_t rawMin;   /**< Start of the usable travel. */
  uint16_t rawMax;   /**< End of it. */
  int8_t dbMute;     /**< What "off" means, in dB. */
  int8_t dbMin;      /**< The quiet end. */
  int8_t dbMax;      /**< The loud end. */
  uint16_t deadband; /**< Raw counts it must move before anything changes. */
} PotConfig;

/**
 * The pot settings this radio ships with.
 *
 * From the working PE5PVB firmware, which drives the same 10k linear pot on
 * the same ADC pin. The narrow span is the part worth understanding: the
 * usable travel covers -30 dB to 0 dB, not the -60 dB the chip can do.
 * Spreading the full range across the knob puts everything below the halfway
 * point under what can be heard, so half the travel would do nothing.
 *
 * The bottom of the travel is a separate mute zone rather than the quiet end
 * of the span, so the knob can actually turn the radio off.
 *
 * @param out  Receives the defaults.
 */
void potDefaults(PotConfig *out);

/**
 * How long a calibration may sit unfinished, in milliseconds.
 *
 * While one runs the knob sets neither the volume nor the squelch, so one
 * that is started and forgotten leaves the radio with no working volume
 * control and nothing on it to say why. Two minutes is far longer than
 * turning a knob to both ends takes.
 */
#define POT_CALIBRATE_TIMEOUT_MS 120000

/**
 * The narrowest sweep worth keeping, in raw counts.
 *
 * A knob that moved less than this was not swept end to end, and storing what
 * it saw would leave almost no usable travel with nothing to say why. The
 * converter is 4096 counts, so this is a quarter of it.
 */
#define POT_CALIBRATE_MIN_SPAN 1000

/** Learning how far this unit's knob actually turns. */
typedef struct {
  bool active;        /**< A calibration is running. */
  uint16_t rawMin;    /**< The lowest seen so far. */
  uint16_t rawMax;    /**< The highest. */
  uint32_t startedMs; /**< When it began, for the timeout. */
} PotCalibration;

/**
 * Begin learning, from where the knob is now.
 *
 * Started from the current reading rather than from the ends of the
 * converter, or the first comparison would never beat them.
 *
 * @param c      The calibration. Cleared and started.
 * @param raw    Where the knob is now.
 * @param nowMs  The millisecond count now.
 */
void potCalibrateStart(PotCalibration *c, uint16_t raw, uint32_t nowMs);

/**
 * Offer a reading, and give up if it has been running too long.
 *
 * @param c      The calibration.
 * @param raw    What the knob reads.
 * @param nowMs  The millisecond count now.
 * @return true while it is still running. False once it has timed out, at
 *         which point the knob goes back to its usual job.
 */
bool potCalibrateSample(PotCalibration *c, uint16_t raw, uint32_t nowMs);

/**
 * Set the ends of travel from a measured sweep.
 *
 * Not a plain copy. The knob has to keep a mute zone at the bottom, and that
 * zone is a stretch of travel rather than a single reading: an ADC at rest
 * wanders by a few counts, so a mute that needed one exact value would almost
 * never fire and the radio could not be switched off by the knob. The same
 * goes for the always open end of a manual squelch.
 *
 * The zone is kept at the same share of the travel the built in figures use,
 * which is the bottom 2.5 per cent.
 *
 * @param cfg     The mapping to change.
 * @param rawMin  What the knob reads at the quiet end.
 * @param rawMax  What it reads at the loud end.
 */
void potApplyCalibration(PotConfig *cfg, uint16_t rawMin, uint16_t rawMax);

/**
 * Stop, and keep what was learned if the knob was swept far enough.
 *
 * Refuses when no calibration is running, which is what makes cancelling
 * mean something: without that check a finish after a cancel would apply the
 * extremes the cancelled sweep had recorded.
 *
 * @param c    The calibration. Stopped either way.
 * @param cfg  Receives the new ends of travel, only when this returns true.
 * @return true when it was applied.
 */
bool potCalibrateFinish(PotCalibration *c, PotConfig *cfg);

/**
 * Give up, keeping whatever was in use before.
 *
 * @param c  The calibration.
 */
void potCalibrateCancel(PotCalibration *c);

/**
 * Turn a pot reading into a volume.
 *
 * @param raw  The averaged ADC reading.
 * @param cfg  The mapping. NULL means the defaults.
 * @return The volume in dB.
 */
int8_t potVolumeDb(uint16_t raw, const PotConfig *cfg);

/**
 * Whether the pot has moved far enough to act on.
 *
 * An ADC reading jitters by a few counts with nothing touching it, and acting
 * on that would send a volume command several times a second for ever.
 *
 * @param previous  The last reading acted on.
 * @param now       The reading now.
 * @param cfg       The mapping. NULL means the defaults.
 * @return true when the change is real.
 */
bool potMoved(uint16_t previous, uint16_t now, const PotConfig *cfg);

/* ----------------------------------------------------------------- button */

/** What a button did. */
typedef enum {
  BUTTON_NONE = 0, /**< Nothing happened this time round. */
  BUTTON_SHORT,    /**< Pressed and let go. */
  BUTTON_LONG,     /**< Held past the long press time, reported once. */
  BUTTON_DOUBLE    /**< Two short presses close together. */
} ButtonEvent;

/** How long a press has to be, and how close two have to be. */
typedef struct {
  uint16_t debounceMs; /**< Ignore changes closer together than this. */
  uint16_t longMs;     /**< Held this long is a long press. */
  uint16_t doubleMs;   /**< A second press within this is a double. */
  /**
   * Watch for a double press on this button.
   *
   * Off by default, and that is not laziness. Telling a single press from a
   * double means holding the single back until it is certain no second press
   * is coming, so every press on the button arrives `doubleMs` late. On a
   * button pressed repeatedly, such as MODE, that also swallows the second of
   * two quick presses into one double that the caller then ignores.
   *
   * So a button only pays that price if something actually uses its double
   * press.
   */
  bool wantDouble;
} ButtonConfig;

/**
 * The press timings the radio ships with.
 *
 * 25 ms of debounce is longer than any switch bounce on this board and
 * shorter than a person can press twice. 600 ms for a long press is the
 * usual feel: long enough not to fire while someone is tapping, short enough
 * not to feel stuck. 350 ms for a double is inside what a hand can do twice.
 *
 * Double press watching is off. Turn it on for a button that has something
 * bound to its double press, and not for the others.
 *
 * @param out  Receives the defaults.
 */
void buttonDefaults(ButtonConfig *out);

/** One button. Zero it before first use. */
typedef struct {
  bool level;          /**< The debounced level. True means pressed. */
  bool raw;            /**< The last level seen, before debouncing. */
  bool handled;        /**< This press has already produced its event, so
                      *   letting go must not produce another one. */
  bool waitingDouble;  /**< A short press is being held back to see if a
                       *   second one follows. */
  uint32_t changedMs;  /**< When the raw level last changed. */
  uint32_t pressedMs;  /**< When the current press started. */
  uint32_t releasedMs; /**< When the last press ended. */
} Button;

/**
 * Feed a button its current level.
 *
 * Call this often, whether or not the level changed. It needs to be called
 * with the level unchanged as well, because a long press is a thing that
 * happens while nothing is happening.
 *
 * With `wantDouble` off, a short press is reported the moment the finger comes
 * off. With it on, the short press is held back `doubleMs` to see whether a
 * second one follows, which is the price of telling the two apart.
 *
 * @param b        The button.
 * @param cfg      The timings. NULL means the defaults.
 * @param pressed  True when the button is down now.
 * @param nowMs    The millisecond count now.
 * @return What happened, or BUTTON_NONE.
 */
ButtonEvent buttonFeed(Button *b, const ButtonConfig *cfg, bool pressed,
                       uint32_t nowMs);

/**
 * The event in words, for a log line or a diagnostic page.
 *
 * @param event  The event.
 * @return A short lower case word. Never NULL.
 */
const char *buttonEventName(ButtonEvent event);

#ifdef __cplusplus
}
#endif

#endif /* CORE_INPUT_H */
