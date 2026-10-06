/*
 * The knob, the buttons and the keypad, turned into radio commands.
 *
 * This sits outside the five layers for the same reason radio_task.h does. It
 * is a composition root: the one place allowed to know about `core/` and
 * `drivers/` at once. Nothing in `core/` may include it.
 *
 * It runs on the loop task, not on a task of its own. Reading a few pins and
 * one I2C register takes microseconds, and no third task gets added because
 * something feels slow: every extra task brings concurrency bugs, which are
 * hard to find.
 *
 * Commands go on the same queue the HTTP API uses. There is no second path
 * into the tuner, which is what stops the knob and the browser disagreeing
 * about what the radio is set to.
 */
#ifndef INPUT_TASK_H
#define INPUT_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/input.h"
#include "core/touch.h"
#include "drivers/touch.h"

/*
 * How many typed digits are kept before the rest are ignored.
 *
 * The highest literal value the band table holds is 108000, FM's own top
 * edge, which is six digits. Seven leaves one spare, since most entries
 * land in far fewer digits anyway: the value is tried at every power of ten
 * before it is refused, so typing 1028 finds 102.80 MHz without needing all
 * six.
 */
#define INPUT_DIGITS_MAX 7

/* How long the description of the last event can be, with its terminator. */
#define INPUT_EVENT_MAX 40

/* What the input layer has seen, for a diagnostic page. */
typedef struct {
  bool keypadPresent;              /* The expander answered at start up. */
  uint32_t clicks;                 /* Knob clicks since boot. */
  uint32_t presses;                /* Button and key events since boot. */
  char lastEvent[INPUT_EVENT_MAX]; /* The last one in words, such as
                                    *   "BAND long". */
  uint32_t lastEventMs; /* When that was, ms since boot. 0 for never. */
  char typed[INPUT_DIGITS_MAX + 1]; /* Digits keyed and not yet entered. */
  uint16_t pot;                     /* The volume pot, 0 to 4095. */
  int8_t potDb;                     /* What that was turned into, in dB. */
  uint16_t lines;      /* The keypad's sixteen lines. A 0 bit is a key held. */
  uint16_t linesOk;    /* Non zero once the lines have been read at all. */
  bool touchOn;        /* The Touch setting lets the chip be read. */
  bool touchPen;       /* The touch chip's pen line says a finger is down. */
  uint32_t touchDowns; /* Times the pen line went down since boot. */
  uint32_t touchReads; /* Touch readings taken since boot. */
  TouchRaw touch;      /* The last of them. Only good once touchReads is
                        * above 0. */
  bool touchMapped;    /* touchAt holds a point. */
  TouchPoint touchAt;  /* The last steady point, in screen pixels. */
  bool touchCalStored; /* The map in use is one a person made, not the
                        * board's own. */
  uint32_t touchGestures; /* Taps, holds, drags and swipes since boot. */
} InputStatus;

/*
 * Set the knob, the buttons and the keypad up.
 *
 * A missing keypad is not a failure. The expander is on the same I2C bus as
 * the tuner, and a radio whose keypad is not fitted still has to work.
 */
bool inputBegin(EncoderKind kind, EncoderDirection direction);

/*
 * Read everything once and act on what changed.
 *
 * Call this from loop(). It never blocks and it never talks to the tuner.
 */
void inputPoll(void);

/*
 * A press, or a turn of the knob by `clicks`, sent over POST /api/key. It
 * waits for the next inputPoll and is then handled exactly as the real one
 * would be, but is not counted in InputStatus, and its `lastEvent` starts
 * "api ". False, with nothing queued, while an earlier one still waits, for
 * a key that is not one, for a long press on a key that has none, and for a
 * turn of 0.
 */
bool inputPressFromApi(InputKey key, ButtonEvent event);
bool inputTurnFromApi(int32_t clicks);

/* What became of a touch sent over POST /api/touch. */
typedef enum {
  INPUT_TOUCH_TAKEN = 0,
  INPUT_TOUCH_BUSY,        /* An earlier key or touch still waits. */
  INPUT_TOUCH_OFF,         /* The Touch setting is Off. */
  INPUT_TOUCH_NOT_NOW,     /* The panel is dark, or the boot, going to sleep
                            * or update screen holds it: a touch does
                            * nothing then. */
  INPUT_TOUCH_CALIBRATING, /* The calibration screen takes only the glass. */
} InputTouchResult;

/*
 * A whole gesture, as if made on the glass: a tap, a hold or a swipe,
 * waiting for the next inputPoll in the same one slot as a key. It makes
 * the same checks as a touch on the glass, is not counted in InputStatus,
 * and its `lastEvent` starts "api ". Refused, with nothing queued, wherever
 * a touch on the glass would do nothing. No screen has zones yet, so where
 * it is does not change what it does.
 */
InputTouchResult inputTouchFromApi(TouchGestureEvent event);

/*
 * How many times a person has touched the radio.
 *
 * Counts a knob click, a button press, a key press, and a turn of the volume
 * pot far enough to be a real turn rather than converter noise. Nothing the
 * web, the tuner or a seek did on its own is in here.
 *
 * A caller that wants to know whether the radio has been left alone watches
 * this one number instead of each source. It only goes up, and it wraps after
 * about four thousand million events, which a caller comparing it against
 * what it last saw does not care about.
 */
uint32_t inputActivity(void);

/*
 * What the input layer has seen.
 *
 * Without a screen this is the only way to tell a dead switch from a wrong
 * pin number.
 */
void inputStatusGet(InputStatus *out);

void inputSetPotConfig(const PotConfig *cfg);

/*
 * Which presses make a sound.
 *
 * Off by default. A beep is noticed every time it happens, so it is not
 * something to switch on for somebody.
 *
 * The band edge beep is not here. Only the radio knows the dial wrapped, so
 * that one is radioSetEdgeBeep.
 */
void inputSetBeeps(BeepMode mode);

/*
 * Whether the touch screen is read at all, the Touch setting. Off, the chip
 * is not read and the touch counts stand still, for a panel that sends
 * touches nobody made. Its pen line is still armed once at start up, before
 * the setting is known.
 */
void inputSetTouch(bool on);

/* Whether the screen is shown upside down, so a touch is turned with it. */
void inputSetTouchUpsideDown(bool upsideDown);

/*
 * The map from touch readings to pixels in the board's own mount: one a
 * person made and kept (`stored`), or the board's own. Get gives it and
 * whether the screen is shown upside down; false on a board without touch.
 */
void inputTouchCalSet(const TouchCal *cal, bool stored);
bool inputTouchCalGet(TouchCal *cal, bool *upsideDown);

/*
 * Start learning how far the knob actually turns.
 *
 * While this is running the knob sets neither the volume nor the squelch. It
 * only records the lowest and highest it reads, so that sweeping to the loud
 * end stop does not mean sweeping the volume to full on the way.
 */
void inputPotCalibrateStart(void);

bool inputPotCalibrateFinish(uint16_t *rawMin, uint16_t *rawMax);

void inputPotCalibrateCancel(void);

bool inputPotCalibrating(uint16_t *rawMin, uint16_t *rawMax);

/* What came of a logbook write. */
typedef enum {
  INPUT_LOG_WRITTEN, /* The entry is in the logbook. */
  INPUT_LOG_WALKING, /* Refused: a seek or a scan is walking the dial. */
  INPUT_LOG_ALREADY, /* Not written: the log holds this station already. */
  INPUT_LOG_FAILED   /* Not written: no state to read, or nowhere to write. */
} InputLogResult;

/*
 * Write a logbook entry for whatever the radio is on right now, exactly as
 * holding ENTER does.
 *
 * For `POST /api/log`, so the API can write an entry too. There is no second
 * way into the log, just a second way to ask for the one there is. A caller
 * that needs to know more about a failure reads the note this leaves in the
 * status document.
 */
InputLogResult inputWriteLogEntry(void);

/*
 * Step to the next band, as a press of BAND does on the radio screen. For the
 * menu's Next Band row, the knob's only way to change band. Loop task only.
 */
void inputNextBand(void);

#endif /* INPUT_TASK_H */
