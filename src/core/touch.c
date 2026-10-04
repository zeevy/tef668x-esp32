/* Implementation of the touch calibration, the half turn and hit testing. */
#include "touch.h"

/* `num / den` rounded to the nearest, halves away from 0. `den` is
 * positive. */
static int64_t divRound(int64_t num, int64_t den) {
  return num >= 0 ? (num + den / 2) / den : -((-num + den / 2) / den);
}

/*
 * `num / den` as a gain in fixed point, for `den` positive. False for a
 * gain over one pixel per step, which touchCalValid refuses anyway; turning
 * it away here keeps `num` times TOUCH_CAL_ONE inside 64 bits once `den` is
 * at most 2^46. Each halving on the way there moves the ratio by at most
 * 2^-45, and there are at most 15 of them, far below the 1 / 65536 step of
 * the result.
 */
static bool gain(int64_t num, int64_t den, int32_t *out) {
  if (num > den || num < -den) {
    return false;
  }
  while (den > ((int64_t)1 << 46)) {
    num /= 2;
    den /= 2;
  }
  *out = (int32_t)divRound(num * TOUCH_CAL_ONE, den);
  return true;
}

/*
 * One axis of the least squares fit: the gains on raw x and raw y, and the
 * offset, that best give the screen's x, or its y when `alongY`. The sums
 * are taken about the marks' middle and multiplied by `n`, so they stay
 * whole numbers. False when the readings all lie on one line, which leaves
 * the gains with no answer, or a gain is over one pixel per step.
 */
static bool fitAxis(const TouchPoint *raw, const TouchPoint *screen, int n,
                    bool alongY, int32_t *gx, int32_t *gy, int32_t *offset) {
  int64_t sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
  int64_t ss = 0, sxs = 0, sys = 0;
  for (int i = 0; i < n; i++) {
    const int64_t rx = raw[i].x;
    const int64_t ry = raw[i].y;
    const int64_t s = alongY ? screen[i].y : screen[i].x;
    sx += rx;
    sy += ry;
    sxx += rx * rx;
    syy += ry * ry;
    sxy += rx * ry;
    ss += s;
    sxs += rx * s;
    sys += ry * s;
  }
  const int64_t cxx = n * sxx - sx * sx;
  const int64_t cyy = n * syy - sy * sy;
  const int64_t cxy = n * sxy - sx * sy;
  const int64_t cxs = n * sxs - sx * ss;
  const int64_t cys = n * sys - sy * ss;
  /* Never negative, and 0 only when the readings lie on one line. */
  const int64_t det = cxx * cyy - cxy * cxy;
  if (det == 0 || !gain(cxs * cyy - cxy * cys, det, gx) ||
      !gain(cxx * cys - cxy * cxs, det, gy)) {
    return false;
  }
  *offset = (int32_t)divRound(
      ss * TOUCH_CAL_ONE - (int64_t)*gx * sx - (int64_t)*gy * sy, n);
  return true;
}

static bool sizeOk(int16_t width, int16_t height) {
  return width >= 1 && height >= 1 && width <= TOUCH_RAW_STEPS &&
         height <= TOUCH_RAW_STEPS;
}

static bool inRange(int16_t v, int16_t size) {
  return v >= 0 && v < size;
}

bool touchCalFit(const TouchPoint *raw, const TouchPoint *screen, int n,
                 int16_t width, int16_t height, TouchCal *out) {
  if (n < 3 || n > TOUCH_FIT_MAX_POINTS || !sizeOk(width, height)) {
    return false;
  }
  for (int i = 0; i < n; i++) {
    if (!inRange(raw[i].x, TOUCH_RAW_STEPS) ||
        !inRange(raw[i].y, TOUCH_RAW_STEPS) || !inRange(screen[i].x, width) ||
        !inRange(screen[i].y, height)) {
      return false;
    }
  }
  TouchCal c;
  if (!fitAxis(raw, screen, n, false, &c.xx, &c.xy, &c.x0) ||
      !fitAxis(raw, screen, n, true, &c.yx, &c.yy, &c.y0) ||
      !touchCalValid(&c, width, height)) {
    return false;
  }
  *out = c;
  return true;
}

static bool gainOk(int32_t g) {
  return g >= -TOUCH_CAL_ONE && g <= TOUCH_CAL_ONE;
}

/*
 * Whether the pixel (x, y) comes from a reading of 0 to 4095 on both axes.
 * The fit is turned round with the inverse of its 2 by 2 gains, whose
 * determinant is `det`, and the reading is compared still multiplied by
 * `det`, so nothing is divided. With each gain at most 2^16, every product
 * here is under 2^49.
 */
static bool reachable(const TouchCal *c, int64_t det, int32_t x, int32_t y) {
  const int64_t u = (int64_t)x * TOUCH_CAL_ONE - c->x0;
  const int64_t v = (int64_t)y * TOUCH_CAL_ONE - c->y0;
  int64_t rx = c->yy * u - c->xy * v;
  int64_t ry = c->xx * v - c->yx * u;
  if (det < 0) {
    rx = -rx;
    ry = -ry;
    det = -det;
  }
  const int64_t top = (TOUCH_RAW_STEPS - 1) * det;
  return rx >= 0 && rx <= top && ry >= 0 && ry <= top;
}

bool touchCalValid(const TouchCal *cal, int16_t width, int16_t height) {
  if (!sizeOk(width, height) || !gainOk(cal->xx) || !gainOk(cal->xy) ||
      !gainOk(cal->yx) || !gainOk(cal->yy)) {
    return false;
  }
  const int64_t det = (int64_t)cal->xx * cal->yy - (int64_t)cal->xy * cal->yx;
  /* The screen is a parallelogram of readings, so it lies inside the
   * converter's square when its four corners do. */
  return det != 0 && reachable(cal, det, 0, 0) &&
         reachable(cal, det, width - 1, 0) &&
         reachable(cal, det, 0, height - 1) &&
         reachable(cal, det, width - 1, height - 1);
}

/* One coordinate of the pixel for `raw`, rounded, and held to 0 to
 * `size - 1`. */
static int16_t pixel(int32_t gx, int32_t gy, int32_t offset, TouchPoint raw,
                     int16_t size) {
  const int64_t v =
      (int64_t)gx * raw.x + (int64_t)gy * raw.y + offset + TOUCH_CAL_ONE / 2;
  if (v < 0) {
    return 0;
  }
  const int64_t p = v / TOUCH_CAL_ONE;
  return p >= size ? (int16_t)(size - 1) : (int16_t)p;
}

TouchPoint touchCalMap(const TouchCal *cal, TouchPoint raw, int16_t width,
                       int16_t height) {
  const TouchPoint p = {pixel(cal->xx, cal->xy, cal->x0, raw, width),
                        pixel(cal->yx, cal->yy, cal->y0, raw, height)};
  return p;
}

TouchPoint touchTurn(TouchPoint p, int16_t width, int16_t height,
                     bool upsideDown) {
  if (upsideDown) {
    p.x = (int16_t)(width - 1 - p.x);
    p.y = (int16_t)(height - 1 - p.y);
  }
  return p;
}

static bool overlap(const TouchZone *a, const TouchZone *b) {
  return a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h &&
         b->y < a->y + a->h;
}

bool touchZonesValid(const TouchZone *zones, int n) {
  for (int i = 0; i < n; i++) {
    if (zones[i].w < 1 || zones[i].h < 1) {
      return false;
    }
    for (int j = 0; j < i; j++) {
      if (overlap(&zones[i], &zones[j])) {
        return false;
      }
    }
  }
  return true;
}

int touchZoneAt(const TouchZone *zones, int n, TouchPoint p) {
  for (int i = 0; i < n; i++) {
    const TouchZone *z = &zones[i];
    if (p.x >= z->x && p.x < z->x + z->w && p.y >= z->y && p.y < z->y + z->h) {
      return z->id;
    }
  }
  return TOUCH_NO_ZONE;
}
