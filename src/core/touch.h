/*
 * Touch points: from the touch controller's raw readings to a pixel, and
 * from a pixel to the zone of the screen under it.
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

#ifdef __cplusplus
}
#endif

#endif /* CORE_TOUCH_H */
