/*
 * Calibrating the touch screen: the marks a person holds, the check after
 * them, and how a calibration is saved in a blob of its own.
 *
 * A calibration maps the touch chip's raw readings to pixels in the board's
 * own mount (core/touch.h). Each glass is mounted a little differently, so a
 * map from one radio can be some pixels off on another; this is how a person
 * makes one for their own.
 *
 * Plain C with no hardware: the glue feeds it what the touch filter gives
 * each poll and draws what it says. The screen size is an argument.
 */
#ifndef CORE_TOUCH_CAL_H
#define CORE_TOUCH_CAL_H

#include <stdbool.h>
#include <stdint.h>

#include "touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* How many marks a calibration takes: one near each corner and one in the
 * middle, the shape touchCalFit does best with. */
#define TOUCH_CAL_MARKS 5

/* How far in from each edge the corner marks sit. A fingertip's centre got
 * no nearer the edge than 26 to 29 px on this glass, so a mark further in
 * can be held with a finger as well as a pen. */
#define TOUCH_CAL_INSET_PX 32

/* How many steady readings a mark takes before it is done: twice the eight
 * a corner took in the firmware this one replaces, so one reading off moves
 * the mark half as far. */
#define TOUCH_CAL_FILL 16

/* How far a finger may wander while a mark fills. A finger held still in the
 * middle wandered up to 14.2 px, but at the corner that reads weakest a
 * fingertip's readings wandered 25 px; the mark's readings are averaged, so
 * the wander costs nothing. Past this, the mark starts filling again from
 * where the finger is. */
#define TOUCH_CAL_STILL_PX 32

/* How near the check dot a tap must land for the calibration to be saved,
 * along either axis. Light fingertip taps landed within 15.4 px of where
 * they clustered along either axis; the fit's own few pixels at the dot
 * come on top. */
#define TOUCH_CAL_CHECK_PX 24

typedef enum {
  TOUCH_CAL_MARK = 0,  /* Holding the marks, one at a time. */
  TOUCH_CAL_CHECK,     /* Tapping the check dot. */
  TOUCH_CAL_PASSED,    /* The new calibration passed its check. */
  TOUCH_CAL_NO_FIT,    /* The marks did not make a calibration that can be
                       * saved, touchCalValid's rules. */
  TOUCH_CAL_MISSED,    /* The check landed too far from the dot. */
  TOUCH_CAL_NOT_SAVED, /* It passed, but could not be saved. Set by the
                        * caller that keeps it; the flow never sets it. */
} TouchCalStep;

/* One calibration being made. Start it with touchCalFlowBegin. */
typedef struct {
  int16_t width, height;
  bool upsideDown; /* The screen is shown turned half way round. */
  TouchCal guide;  /* The map in use, to judge a finger held still. */
  TouchCalStep step;
  bool touching;      /* Contact within the bridge of now. */
  uint32_t contactMs; /* The last poll with contact. */
  uint8_t mark;       /* The mark being held, while marking. */
  uint8_t filled;     /* Steady readings the mark has, 0 to TOUCH_CAL_FILL. */
  bool needLift;      /* A mark was just taken: nothing until the lift. */
  TouchPoint anchor;  /* Where this fill started, in pixels. */
  int32_t sumX, sumY; /* The fill's raw readings, added up. */
  TouchPoint raw[TOUCH_CAL_MARKS];
  bool checkSeen;      /* The check tap has a reading. */
  bool checkSettled;   /* And a settled one. */
  TouchPoint checkRaw; /* Its latest settled reading, or its latest. */
  uint16_t checkOffPx; /* How far from the dot it landed. */
  TouchCal result;     /* The new calibration, once the marks are done. */
} TouchCalFlow;

/*
 * Start a calibration of a screen `width` by `height`, shown turned half
 * way round when `upsideDown`. `guide` is the calibration in use now; it
 * only judges whether a finger is still, so a rough one does.
 */
void touchCalFlowBegin(TouchCalFlow *f, int16_t width, int16_t height,
                       bool upsideDown, const TouchCal *guide);

/* Where mark `i` is drawn, in screen pixels. */
TouchPoint touchCalFlowMark(const TouchCalFlow *f, uint8_t i);

/* Where the check dot is drawn, in screen pixels: a place no mark took. */
TouchPoint touchCalFlowCheckDot(const TouchCalFlow *f);

/*
 * Feed one poll: whether there is contact, whether a new reading was taken
 * this poll, the filter's steady raw point and whether it is unsettled, and
 * the time. True when what the screen shows changed. A lift is only a lift
 * once TOUCH_LIFT_BRIDGE_MS pass with no contact, as for a gesture. A mark
 * fills with new settled readings while the finger stays within
 * TOUCH_CAL_STILL_PX of where it started; a lift before it is full starts
 * it again. After each mark the finger must lift before the next. The check
 * is a tap: its last settled reading, or its last when none settled, mapped
 * by the new calibration, must land within TOUCH_CAL_CHECK_PX of the dot.
 */
bool touchCalFlowFeed(TouchCalFlow *f, bool contact, bool fresh, TouchPoint raw,
                      bool unsettled, uint32_t nowMs);

/*
 * The map the firmware uses before anybody calibrates: from the raw
 * readings at the four corners of the picture, top left, top right, bottom
 * right, bottom left. False if they do not make one.
 */
bool touchCalFromCorners(const TouchPoint corners[4], int16_t width,
                         int16_t height, TouchCal *out);

/* Whether the flow ended with a calibration that was not saved, which a
 * press of the knob starts again. */
bool touchCalFlowFailed(const TouchCalFlow *f);

/* The version a saved calibration is written with. A new layout gets a new
 * number, and a blob of any other number is not read. */
#define TOUCH_CAL_BLOB_VERSION 1

/* A calibration as it is saved: a version in front of it. */
typedef struct {
  uint16_t version;
  uint16_t spare; /* Always 0. */
  TouchCal cal;
} TouchCalBlob;

/* The calibration in a saved blob. False, and `out` untouched, for a blob of
 * another version, or one touchCalValid refuses for this screen. */
bool touchCalBlobRead(const TouchCalBlob *blob, int16_t width, int16_t height,
                      TouchCal *out);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TOUCH_CAL_H */
