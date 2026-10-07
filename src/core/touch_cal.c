/* Implementation of calibrating the touch screen. See touch_cal.h. */
#include "touch_cal.h"

#include <string.h>

/* The larger of the two distances from `a` to `b`, along x or along y. */
static int32_t apart(TouchPoint a, TouchPoint b) {
  const int32_t dx = b.x > a.x ? b.x - a.x : a.x - b.x;
  const int32_t dy = b.y > a.y ? b.y - a.y : a.y - b.y;
  return dx > dy ? dx : dy;
}

void touchCalFlowBegin(TouchCalFlow *f, int16_t width, int16_t height,
                       bool upsideDown, const TouchCal *guide) {
  if (f == NULL || guide == NULL) {
    return;
  }
  memset(f, 0, sizeof(*f));
  f->width = width;
  f->height = height;
  f->upsideDown = upsideDown;
  f->guide = *guide;
  f->step = TOUCH_CAL_MARK;
}

TouchPoint touchCalFlowMark(const TouchCalFlow *f, uint8_t i) {
  if (f == NULL) {
    const TouchPoint none = {0, 0};
    return none;
  }
  const int16_t in = TOUCH_CAL_INSET_PX;
  const int16_t r = (int16_t)(f->width - 1 - in);
  const int16_t b = (int16_t)(f->height - 1 - in);
  const TouchPoint m[TOUCH_CAL_MARKS] = {
      {in, in},
      {r, in},
      {r, b},
      {in, b},
      {(int16_t)(f->width / 2), (int16_t)(f->height / 2)}};
  return m[i < TOUCH_CAL_MARKS ? i : TOUCH_CAL_MARKS - 1];
}

TouchPoint touchCalFlowCheckDot(const TouchCalFlow *f) {
  TouchPoint p = {0, 0};
  if (f != NULL) {
    p.x = (int16_t)(f->width * 7 / 10);
    p.y = (int16_t)(f->height * 3 / 8);
  }
  return p;
}

/* The marks are drawn on the screen as it is shown; the fit is made in the
 * board's own mount, so a mark of a screen shown upside down is turned back
 * first. */
static bool fit(TouchCalFlow *f) {
  TouchPoint screen[TOUCH_CAL_MARKS];
  for (uint8_t i = 0; i < TOUCH_CAL_MARKS; i++) {
    screen[i] =
        touchTurn(touchCalFlowMark(f, i), f->width, f->height, f->upsideDown);
  }
  return touchCalFit(f->raw, screen, TOUCH_CAL_MARKS, f->width, f->height,
                     &f->result);
}

/* Whether a lift ends with this poll: no contact for the bridge after there
 * was some. Contact keeps the time. */
static bool lifted(TouchCalFlow *f, bool contact, uint32_t nowMs) {
  if (contact) {
    f->touching = true;
    f->contactMs = nowMs;
    return false;
  }
  if (!f->touching || (uint32_t)(nowMs - f->contactMs) < TOUCH_LIFT_BRIDGE_MS) {
    return false;
  }
  f->touching = false;
  return true;
}

static bool feedMark(TouchCalFlow *f, bool contact, bool fresh, TouchPoint raw,
                     bool unsettled, uint32_t nowMs) {
  const bool lift = lifted(f, contact, nowMs);
  if (f->needLift) {
    if (lift) {
      f->needLift = false;
    }
    return false;
  }
  if (lift) {
    if (f->filled == 0) {
      return false;
    }
    /* Lifted before the mark was full: it starts again. */
    f->filled = 0;
    return true;
  }
  if (!contact || !fresh || unsettled) {
    return false;
  }
  const TouchPoint at = touchCalMap(&f->guide, raw, f->width, f->height);
  if (f->filled == 0 || apart(f->anchor, at) > TOUCH_CAL_STILL_PX) {
    /* A new fill, or the finger slid: fill from here. */
    f->anchor = at;
    f->sumX = raw.x;
    f->sumY = raw.y;
    f->filled = 1;
    return true;
  }
  f->sumX += raw.x;
  f->sumY += raw.y;
  f->filled++;
  if (f->filled < TOUCH_CAL_FILL) {
    return true;
  }
  f->raw[f->mark].x = (int16_t)touchDivRound(f->sumX, TOUCH_CAL_FILL);
  f->raw[f->mark].y = (int16_t)touchDivRound(f->sumY, TOUCH_CAL_FILL);
  f->filled = 0;
  f->needLift = true;
  f->mark++;
  if (f->mark == TOUCH_CAL_MARKS) {
    f->step = fit(f) ? TOUCH_CAL_CHECK : TOUCH_CAL_NO_FIT;
  }
  return true;
}

static bool feedCheck(TouchCalFlow *f, bool contact, bool fresh, TouchPoint raw,
                      bool unsettled, uint32_t nowMs) {
  const bool lift = lifted(f, contact, nowMs);
  if (f->needLift) {
    if (lift) {
      f->needLift = false;
    }
    return false;
  }
  if (contact && fresh) {
    /* A settled reading stands over an unsettled one; the latest of the
     * kind stands. */
    if (!unsettled || !f->checkSettled) {
      f->checkRaw = raw;
      f->checkSettled = !unsettled;
    }
    f->checkSeen = true;
    return false;
  }
  if (!lift || !f->checkSeen) {
    return false;
  }
  const TouchPoint at =
      touchTurn(touchCalMap(&f->result, f->checkRaw, f->width, f->height),
                f->width, f->height, f->upsideDown);
  const int32_t off = apart(at, touchCalFlowCheckDot(f));
  f->checkOffPx = (uint16_t)(off > 0xFFFF ? 0xFFFF : off);
  f->step = off <= TOUCH_CAL_CHECK_PX ? TOUCH_CAL_PASSED : TOUCH_CAL_MISSED;
  return true;
}

bool touchCalFlowFeed(TouchCalFlow *f, bool contact, bool fresh, TouchPoint raw,
                      bool unsettled, uint32_t nowMs) {
  if (f == NULL) {
    return false;
  }
  switch (f->step) {
    case TOUCH_CAL_MARK:
      return feedMark(f, contact, fresh, raw, unsettled, nowMs);
    case TOUCH_CAL_CHECK:
      return feedCheck(f, contact, fresh, raw, unsettled, nowMs);
    default:
      return false;
  }
}

bool touchCalFlowFailed(const TouchCalFlow *f) {
  return f != NULL &&
         (f->step == TOUCH_CAL_NO_FIT || f->step == TOUCH_CAL_MISSED ||
          f->step == TOUCH_CAL_NOT_SAVED);
}

bool touchCalFromCorners(const TouchPoint corners[4], int16_t width,
                         int16_t height, TouchCal *out) {
  if (corners == NULL || out == NULL) {
    return false;
  }
  const TouchPoint screen[4] = {{0, 0},
                                {(int16_t)(width - 1), 0},
                                {(int16_t)(width - 1), (int16_t)(height - 1)},
                                {0, (int16_t)(height - 1)}};
  return touchCalFit(corners, screen, 4, width, height, out);
}

bool touchCalBlobRead(const TouchCalBlob *blob, int16_t width, int16_t height,
                      TouchCal *out) {
  if (blob == NULL || out == NULL || blob->version != TOUCH_CAL_BLOB_VERSION ||
      !touchCalValid(&blob->cal, width, height)) {
    return false;
  }
  *out = blob->cal;
  return true;
}
