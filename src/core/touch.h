/*
 * Touch points: from the touch controller's raw readings to a steady point,
 * from a raw point to a pixel, from a pixel to the zone of the screen under
 * it, and from a run of pixels to a gesture: a tap, a hold, a drag or a
 * swipe.
 *
 * The XPT2046 under the glass gives two 12 bit readings, 0 to 4095, that
 * grow across the glass. Which reading runs along the screen's width, and
 * which way each one runs, depends on how the glass is mounted, and the
 * glass is never quite square to the pixels. A calibration from marks tapped
 * on the screen covers all of that. It is taken and kept in the board's own
 * mount, and the turn for a display shown upside down is made on the pixel
 * after it, so turning the display never needs a new calibration.
 *
 * The screen size is always an argument, so another panel needs no change
 * here. Integer arithmetic only, so it is tested on a PC.
 *
 * Touch input is planned. No firmware code calls this module yet, and the
 * unit tests cover it.
 */
#ifndef CORE_TOUCH_H
#define CORE_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* How many steps the XPT2046's 12 bit converter has. A reading is 0 to
 * TOUCH_RAW_STEPS - 1. */
#define TOUCH_RAW_STEPS 4096

/* A raw reading pair or a pixel, by what is passed. */
typedef struct {
  int16_t x;
  int16_t y;
} TouchPoint;

/*
 * The calibration, from a raw reading to a pixel:
 *
 *   pixel x = (xx * raw x + xy * raw y + x0) / TOUCH_CAL_ONE
 *   pixel y = (yx * raw x + yy * raw y + y0) / TOUCH_CAL_ONE
 *
 * This covers glass whose readings run along the other axis, xy and yx
 * carry the gain and xx and yy are near 0, glass whose reading runs the
 * other way, a negative gain, and glass a little skewed to the pixels, a
 * small cross term. Fixed point with 16 bits after the point, since a gain
 * is a fraction of a pixel per reading step, near 0.09 when 320 pixels
 * span 3450 steps: the rounding of two gains then moves a point less than
 * a tenth of a pixel over the whole 4096 steps.
 */
#define TOUCH_CAL_ONE 65536
typedef struct {
  int32_t xx, xy, x0;
  int32_t yx, yy, y0;
} TouchCal;

/*
 * The most marks touchCalFit takes, a three by three grid. With up to nine
 * marks, and every reading and every pixel under 4096, each sum the fit
 * makes stays under 2^31 and each product of two under 2^62, inside 64 bits.
 */
#define TOUCH_FIT_MAX_POINTS 9

/*
 * The calibration from `n` marks drawn at `screen[i]` and read at `raw[i]`
 * on a screen `width` by `height`. A least squares fit, from 3 to
 * TOUCH_FIT_MAX_POINTS marks. Each mark is one tap, and a tap on resistive
 * glass lands a few steps off the mark. Three marks are fitted exactly, so
 * one bad tap bends the whole fit to pass through it. With more marks the
 * errors partly cancel. With a mark near each corner and one in the
 * middle, a middle mark read `e` steps off moves the whole fit by only
 * e / 5 and leaves the other 4e / 5 at that mark, where a later check can
 * see it. So five marks or more are the ones to ask for.
 *
 * False, and `out` untouched, for a count out of that range, a reading
 * outside 0 to 4095, a mark off the screen, marks all in one line, and any
 * fit touchCalValid refuses.
 */
bool touchCalFit(const TouchPoint *raw, const TouchPoint *screen, int n,
                 int16_t width, int16_t height, TouchCal *out);

/*
 * Whether `cal` can be kept for a screen `width` by `height`. Each rule
 * comes from the converter's 4096 steps or from the shape of the fit:
 *
 * - The screen is 1 to 4096 pixels each way. A screen wider than the
 *   converter has steps has pixels no reading can land on.
 * - One step of either reading moves the point by at most one pixel each
 *   way, each gain at most TOUCH_CAL_ONE. A bigger gain also leaves pixels
 *   no reading lands on. So the fit uses at least as many steps across the
 *   screen as the screen has pixels, 320 of the 4096 on a 320 pixel width.
 *   Marks tapped nearly in a line give a huge gain across that line, so
 *   this refuses them as well.
 * - The fit does not fold the glass onto a line.
 * - Each corner of the screen comes from a reading the converter can give,
 *   0 to 4095 on both axes, so every pixel can be touched.
 *
 * No margin is kept inside 0 and 4095. How near those ends the glass reads
 * at its edges is a property of each glass, so it is not fixed here.
 */
bool touchCalValid(const TouchCal *cal, int16_t width, int16_t height);

/*
 * The pixel `cal` gives for `raw`, rounded to the nearest. A reading past
 * the screen gives the nearest pixel on its edge, since a finger at the
 * rim of the glass means the edge of the screen.
 */
TouchPoint touchCalMap(const TouchCal *cal, TouchPoint raw, int16_t width,
                       int16_t height);

/*
 * The pixel `p` of the board's own mount as the screen shows it, turned
 * half way round when the display is shown `upsideDown`: (width - 1 - x,
 * height - 1 - y), the same turn the panel makes when it mirrors both axes.
 */
TouchPoint touchTurn(TouchPoint p, int16_t width, int16_t height,
                     bool upsideDown);

/*
 * A part of the screen a touch can act on. It covers x to x + w - 1 and y
 * to y + h - 1, the way a panel rectangle is given: the pixel at x + w is
 * outside it, so two zones side by side share no pixel.
 */
typedef struct {
  int16_t x, y, w, h;
  uint8_t id;
} TouchZone;

/* What touchZoneAt gives for a point in no zone. Not a uint8_t, so no
 * zone's id can be mistaken for it. */
#define TOUCH_NO_ZONE (-1)

/*
 * Whether `n` zones can be hit tested: each has some width and height, and
 * no two share a pixel, so a point is in one zone at most. An empty list is
 * fine.
 */
bool touchZonesValid(const TouchZone *zones, int n);

/* The id of the zone holding `p`, or TOUCH_NO_ZONE. */
int touchZoneAt(const TouchZone *zones, int n, TouchPoint p);

/*
 * The weakest reading that is contact. With nothing on the glass the chip's
 * first contact reading, Z1, reads 0 to 5, a reading taken as a finger lifts
 * among them; the weakest real touch measured, a fingertip at a corner that
 * reads poorly, read 86. A reading under this is no contact, however the pen
 * line looked.
 */
#define TOUCH_Z1_MIN 40

/* One poll of the touch chip. */
typedef struct {
  bool pen;       /* The chip's pen line says something is down. */
  bool read;      /* A reading was taken this poll. Never without `pen`. */
  TouchPoint raw; /* The reading's raw x and y, when `read`. */
  uint16_t z1;    /* Its first contact reading, when `read`. */
} TouchReading;

/*
 * Raw readings made steady, one contact at a time. Zero it before first
 * use.
 *
 * The first readings of a contact, while the finger lands, and the last,
 * while it lifts, can sit up to 18 px off where it is; from the third last
 * inwards they sat within 10. So the point is the median of the latest three
 * good readings, x and y each on its own, which leaves out any one reading
 * far from the other two. Before three have come, the point is the middle of
 * two or the one, and is not yet settled: about one tap in fifty gives fewer
 * than three readings in all.
 */
typedef struct {
  TouchPoint keep[3]; /* The latest good readings, oldest first. */
  uint8_t kept;       /* How many of `keep` are filled, up to 3. */
  bool lastGood;      /* The latest reading was contact. */
} TouchFilter;

/*
 * Feed one poll. True while there is contact, with `*at` and `*unsettled`,
 * if not NULL, set to its steady raw point and whether fewer than three
 * readings stand behind it. Contact starts with a good reading. A poll with
 * the pen line down and no reading taken keeps the latest reading's verdict,
 * so how often the loop polls between readings changes nothing. False for a
 * NULL filter or reading.
 */
bool touchFilterFeed(TouchFilter *f, const TouchReading *r, TouchPoint *at,
                     bool *unsettled);

/*
 * How long contact can break and still be the same touch. A finger swiping
 * lifted for 134 ms mid-swipe in one swipe of ten measured, and the pen
 * line chatters for under 5 ms inside a tap. So a lift is only a lift once
 * this long has passed with no contact, and a tap is reported that long
 * after the finger lifts.
 */
#define TOUCH_LIFT_BRIDGE_MS 150

/*
 * How long a touch is held still before it is a hold. Taps measured on this
 * glass lasted up to 1.2 s, the slow ones at the corners, and natural holds
 * 2.3 s or more; the keys' 0.6 s would have made 18 taps of 137 into holds.
 * Passed to the gestures as their hold time.
 */
#define TOUCH_HOLD_MS 1500

/*
 * What a finger did, worked out from the steady points of one touch.
 *
 * Every gesture belongs to the zone it started in and to the screen it
 * started on. Where it started is its first settled point: the first
 * readings of a contact can be off, so until then the start and its zone
 * follow the point, though a move past the swipe distance from the very first
 * point is a move already. If the screen changes before the touch is over,
 * the bridge after its lift included, the rest of it is ignored, so the
 * finger that opened a screen cannot also act on the new one.
 */
typedef enum {
  TOUCH_NOTHING = 0,
  /* Down and up again before the hold time, never moved further than the
   * slop from its start, and never off its zone. Reported once the lift has
   * lasted the bridge. */
  TOUCH_TAP,
  /* Held still to the hold time, counted from its first contact whatever
   * breaks it had. Reported once, while still down; nothing after it, a
   * move, a break or the lift, reports anything. */
  TOUCH_HOLD,
  /* Moved past the slop, in a zone that takes drags. Reported on every
   * sample that moved, so at most once a poll. */
  TOUCH_DRAG,
  /* Lifted after a drag. */
  TOUCH_DRAG_END,
  /* Moved past the slop in a zone that does not take drags, and lifted
   * within the swipe time with at least the swipe distance along the axis
   * it moved most on, counted from its very first point. The direction is
   * the finger's. */
  TOUCH_SWIPE_LEFT,
  TOUCH_SWIPE_RIGHT,
  TOUCH_SWIPE_UP,
  TOUCH_SWIPE_DOWN,
} TouchGestureEvent;

/*
 * How far and how fast, in pixels and milliseconds. None of these has a
 * default here: each comes from taps measured on the glass, and the caller
 * passes it.
 */
typedef struct {
  /* A finger moved more than this along either axis from its start is
   * moving, not tapping or holding. */
  uint16_t slopPx;
  /* The least travel along its main axis a swipe needs. */
  uint16_t swipePx;
  /* The longest a swipe takes, from the first contact to the lift. */
  uint16_t swipeMs;
  /* How long a touch is held still before it is a hold, TOUCH_HOLD_MS. */
  uint16_t holdMs;
} TouchGestureConfig;

/* One sample, given every poll whether or not anything changed. */
typedef struct {
  bool down;       /* There is contact now, as the filter says. */
  TouchPoint at;   /* Where, in pixels. Read only while `down`. */
  bool unsettled;  /* Fewer than three readings stand behind `at`. */
  int zone;        /* touchZoneAt for `at`. Read only while `down`. */
  bool zoneDrags;  /* That zone takes drags. Read only while `down`. */
  uint32_t screen; /* Changes whenever another screen is shown. */
} TouchSample;

/*
 * One touch being followed. Zero it before first use. While a touch is on,
 * `zone` is the zone it started in, `start` where, and `last` the latest
 * point, which is what a drag or a swipe's caller reads.
 */
typedef struct {
  bool on;         /* A touch is being followed, down or within the bridge. */
  bool touching;   /* It had contact at the latest sample. */
  bool done;       /* A hold was reported, or the screen changed: nothing
                    * more until the lift has lasted the bridge. */
  bool settled;    /* `start` and `zone` are fixed. */
  bool moved;      /* Past the slop. */
  bool offZone;    /* Left its zone at some point. */
  bool drags;      /* Its zone takes drags. */
  int zone;        /* The zone it started in. */
  uint32_t screen; /* The screen it started on. */
  uint32_t startMs;
  uint32_t liftMs;
  TouchPoint origin; /* Its very first point, where a swipe is measured from. */
  TouchPoint start;
  TouchPoint last;
} TouchGesture;

/*
 * Feed one sample and get what it completed, or TOUCH_NOTHING. NULL for any
 * argument gives TOUCH_NOTHING and changes nothing.
 */
TouchGestureEvent touchGestureFeed(TouchGesture *g,
                                   const TouchGestureConfig *cfg,
                                   const TouchSample *s, uint32_t nowMs);

#ifdef __cplusplus
}
#endif

#endif /* CORE_TOUCH_H */
