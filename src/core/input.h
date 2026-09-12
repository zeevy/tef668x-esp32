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
