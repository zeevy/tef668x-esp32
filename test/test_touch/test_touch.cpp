/* Tests for the touch calibration, the half turn and hit testing. Runs on a
 * PC. */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/touch.h"

#include "captures.h"

void setUp(void) {}
void tearDown(void) {}

/* The ATS-125's panel in the board's own mount. */
#define W 320
#define H 240

/* Five marks, 20 pixels in from each corner and one in the middle, the way
 * a calibration screen would draw them. */
static const TouchPoint MARKS[5] = {
    {20, 20}, {299, 20}, {20, 219}, {299, 219}, {160, 120}};

static TouchPoint pt(int x, int y) {
  const TouchPoint p = {(int16_t)x, (int16_t)y};
  return p;
}

static void assertPoint(int x, int y, TouchPoint p) {
  TEST_ASSERT_EQUAL_INT(x, p.x);
  TEST_ASSERT_EQUAL_INT(y, p.y);
}

/*
 * The PE5PVB firmware's default calibration, its five stored numbers 300,
 * 3450, 300, 3450 and 3, worked out by hand. 3 sets two flags: the axes are
 * swapped, so raw y gives the screen's x and raw x its y, and the screen's
 * x runs against its reading. Each axis starts at reading 300 and spans
 * 3450 readings across the screen. Its pixel truncates where ours rounds.
 */
static TouchPoint pe5pvbPixel(TouchPoint raw) {
  return pt(320 - (raw.y - 300) * 320 / 3450, (raw.x - 300) * 240 / 3450);
}

/* The reading the same default gives at a pixel, rounded to a whole step. */
static TouchPoint pe5pvbRaw(TouchPoint px) {
  return pt(300 + (px.y * 3450 + 120) / 240,
            300 + ((320 - px.x) * 3450 + 160) / 320);
}

/*
 * A panel with every fault the fit has to cover, at once: raw x follows the
 * screen's y, so the axes are swapped; raw y falls as the screen's x grows,
 * so that axis is reversed; and each reading also moves a little with the
 * other screen axis, a skew. Every reading is a whole number, so the fit
 * of it is exact.
 */
static TouchPoint skewRaw(int x, int y) {
  return pt(200 + 14 * y + x, 3700 - 10 * x + y);
}

static bool fitMarks(TouchPoint (*panel)(TouchPoint), int n, TouchCal *cal) {
  TouchPoint raw[5];
  for (int i = 0; i < n; i++) {
    raw[i] = panel(MARKS[i]);
  }
  return touchCalFit(raw, MARKS, n, W, H, cal);
}

static TouchPoint skewAt(TouchPoint p) {
  return skewRaw(p.x, p.y);
}

/* A calibration with gain `g` on both axes and no swap: pixel = raw * g. */
static TouchCal straight(int32_t g) {
  const TouchCal c = {g, 0, 0, 0, g, 0};
  return c;
}

/* ------------------------------------------------------------ the fit */

/* The PE5PVB firmware's defaults, swapped and with x reversed, fit back to
 * the same mapping, within the one pixel its truncation can differ by. */
static void the_pe5pvb_defaults_fit_back(void) {
  TouchCal cal;
  TEST_ASSERT_TRUE(fitMarks(pe5pvbRaw, 5, &cal));
  /* Swapped and reversed: the screen's x comes from raw y with a falling
   * gain of 320 pixels over 3450 steps, -6079 in fixed point, and its y
   * from raw x at 240 over 3450, 4559. Rounding each mark to a whole step
   * moves a gain by well under 1 percent. */
  TEST_ASSERT_INT32_WITHIN(60, 0, cal.xx);
  TEST_ASSERT_INT32_WITHIN(60, -6079, cal.xy);
  TEST_ASSERT_INT32_WITHIN(45, 4559, cal.yx);
  TEST_ASSERT_INT32_WITHIN(45, 0, cal.yy);
  for (int rx = 300; rx < 3750; rx += 115) {
    for (int ry = 300; ry < 3750; ry += 115) {
      const TouchPoint want = pe5pvbPixel(pt(rx, ry));
      const TouchPoint got = touchCalMap(&cal, pt(rx, ry), W, H);
      TEST_ASSERT_INT_WITHIN(1, want.x, got.x);
      TEST_ASSERT_INT_WITHIN(1, want.y, got.y);
    }
  }
}

/* A swapped, reversed and skewed panel maps every pixel back to itself. */
static void a_swapped_reversed_skewed_panel_maps_every_pixel(void) {
  TouchCal cal;
  TEST_ASSERT_TRUE(fitMarks(skewAt, 5, &cal));
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      assertPoint(x, y, touchCalMap(&cal, skewRaw(x, y), W, H));
    }
  }
}

/* Three marks not in a line fix the fit on their own. */
static void three_marks_are_enough(void) {
  TouchCal cal;
  TEST_ASSERT_TRUE(fitMarks(skewAt, 3, &cal));
  for (int y = 0; y < H; y += 7) {
    for (int x = 0; x < W; x += 7) {
      assertPoint(x, y, touchCalMap(&cal, skewRaw(x, y), W, H));
    }
  }
}

/* The middle mark read 50 steps off on a panel with the PE5PVB defaults,
 * 3.5 pixels at 240 pixels over 3450 steps. The fit moves by a fifth of that,
 * so each corner mark still lands within a pixel, and the middle mark still
 * maps more than 2 pixels off. */
static void a_bad_tap_moves_the_fit_by_a_fifth(void) {
  TouchPoint raw[5];
  for (int i = 0; i < 5; i++) {
    raw[i] = pe5pvbRaw(MARKS[i]);
  }
  raw[4].x = (int16_t)(raw[4].x + 50);
  TouchCal cal;
  TEST_ASSERT_TRUE(touchCalFit(raw, MARKS, 5, W, H, &cal));
  for (int i = 0; i < 4; i++) {
    const TouchPoint got = touchCalMap(&cal, raw[i], W, H);
    TEST_ASSERT_INT_WITHIN(1, MARKS[i].x, got.x);
    TEST_ASSERT_INT_WITHIN(1, MARKS[i].y, got.y);
  }
  const TouchPoint middle = touchCalMap(&cal, raw[4], W, H);
  TEST_ASSERT_TRUE(middle.y - MARKS[4].y > 2);
}

/* Readings in one line, or all on one spot, fix no fit, and the
 * calibration passed in is left as it was. */
static void marks_read_in_a_line_are_refused(void) {
  TouchCal cal;
  memset(&cal, 0x5a, sizeof cal);
  TouchCal before = cal;
  const TouchPoint line[3] = {{500, 500}, {1500, 1500}, {2500, 2500}};
  TEST_ASSERT_FALSE(touchCalFit(line, MARKS, 3, W, H, &cal));
  const TouchPoint spot[5] = {
      {900, 900}, {900, 900}, {900, 900}, {900, 900}, {900, 900}};
  TEST_ASSERT_FALSE(touchCalFit(spot, MARKS, 5, W, H, &cal));
  TEST_ASSERT_EQUAL_MEMORY(&before, &cal, sizeof cal);
}

/* Readings 10 steps off one line: the gain across the line is far over a
 * pixel per step. */
static void marks_read_nearly_in_a_line_are_refused(void) {
  const TouchPoint raw[3] = {{500, 500}, {2500, 2500}, {1500, 1510}};
  const TouchPoint screen[3] = {{20, 20}, {299, 219}, {20, 219}};
  TouchCal cal;
  TEST_ASSERT_FALSE(touchCalFit(raw, screen, 3, W, H, &cal));
}

/* Marks across the whole screen read within 200 steps of each other: 279
 * pixels over 200 steps is more than a pixel a step, so pixels in between
 * could never be touched. */
static void marks_read_close_together_are_refused(void) {
  TouchPoint raw[5];
  for (int i = 0; i < 5; i++) {
    raw[i] = pt(1000 + MARKS[i].x * 200 / W, 1000 + MARKS[i].y * 200 / W);
  }
  TouchCal cal;
  TEST_ASSERT_FALSE(touchCalFit(raw, MARKS, 5, W, H, &cal));
}

static void a_fit_needs_three_to_nine_marks_on_the_screen(void) {
  TouchPoint raw[TOUCH_FIT_MAX_POINTS + 1];
  TouchPoint screen[TOUCH_FIT_MAX_POINTS + 1];
  for (int i = 0; i <= TOUCH_FIT_MAX_POINTS; i++) {
    screen[i] = pt(20 + i * 31, 20 + (i * 7 % 10) * 22);
    raw[i] = skewRaw(screen[i].x, screen[i].y);
  }
  TouchCal cal;
  TEST_ASSERT_FALSE(touchCalFit(raw, screen, 2, W, H, &cal));
  TEST_ASSERT_TRUE(touchCalFit(raw, screen, 3, W, H, &cal));
  TEST_ASSERT_TRUE(touchCalFit(raw, screen, TOUCH_FIT_MAX_POINTS, W, H, &cal));
  TEST_ASSERT_FALSE(
      touchCalFit(raw, screen, TOUCH_FIT_MAX_POINTS + 1, W, H, &cal));
}

/* A reading outside the converter's 0 to 4095, a mark off the screen, or a
 * screen of no size or wider than 4096 is refused before any sum. */
static void readings_and_marks_out_of_range_are_refused(void) {
  TouchPoint raw[5];
  TouchPoint screen[5];
  TouchCal cal;
  for (int i = 0; i < 5; i++) {
    raw[i] = skewAt(MARKS[i]);
    screen[i] = MARKS[i];
  }
  TEST_ASSERT_TRUE(touchCalFit(raw, screen, 5, W, H, &cal));

  const int16_t rawBad[] = {-1, TOUCH_RAW_STEPS};
  for (int k = 0; k < 2; k++) {
    raw[1].x = rawBad[k];
    TEST_ASSERT_FALSE(touchCalFit(raw, screen, 5, W, H, &cal));
    raw[1] = skewAt(MARKS[1]);
    raw[1].y = rawBad[k];
    TEST_ASSERT_FALSE(touchCalFit(raw, screen, 5, W, H, &cal));
    raw[1] = skewAt(MARKS[1]);
  }
  screen[2].x = -1;
  TEST_ASSERT_FALSE(touchCalFit(raw, screen, 5, W, H, &cal));
  screen[2].x = W;
  TEST_ASSERT_FALSE(touchCalFit(raw, screen, 5, W, H, &cal));
  screen[2] = MARKS[2];
  screen[2].y = -1;
  TEST_ASSERT_FALSE(touchCalFit(raw, screen, 5, W, H, &cal));
  screen[2].y = H;
  TEST_ASSERT_FALSE(touchCalFit(raw, screen, 5, W, H, &cal));
  screen[2] = MARKS[2];

  TEST_ASSERT_FALSE(touchCalFit(raw, screen, 5, 0, H, &cal));
  TEST_ASSERT_FALSE(touchCalFit(raw, screen, 5, W, 0, &cal));
  TEST_ASSERT_FALSE(touchCalFit(raw, screen, 5, TOUCH_RAW_STEPS + 1, H, &cal));
  TEST_ASSERT_FALSE(touchCalFit(raw, screen, 5, W, TOUCH_RAW_STEPS + 1, &cal));
}

/* ----------------------------------------------------------- the check */

/* One pixel a step is the most: below and on it pass, a step over fails,
 * for each of the four gains and either sign. */
static void a_gain_of_one_pixel_a_step_is_the_most(void) {
  const int32_t g[] = {65535, 65536, 65537};
  const bool ok[] = {true, true, false};
  for (int i = 0; i < 3; i++) {
    const TouchCal c = straight(g[i]);
    TEST_ASSERT_EQUAL(ok[i], touchCalValid(&c, W, H));
  }

  /* Each gain over the limit alone, on a calibration that is otherwise
   * fine. */
  const int32_t over[] = {65537, -65537};
  for (int k = 0; k < 2; k++) {
    TouchCal c = straight(32768);
    c.xx = over[k];
    TEST_ASSERT_FALSE(touchCalValid(&c, W, H));
    c = straight(32768);
    c.xy = over[k];
    TEST_ASSERT_FALSE(touchCalValid(&c, W, H));
    c = straight(32768);
    c.yx = over[k];
    TEST_ASSERT_FALSE(touchCalValid(&c, W, H));
    c = straight(32768);
    c.yy = over[k];
    TEST_ASSERT_FALSE(touchCalValid(&c, W, H));
  }

  /* Swapped axes at a falling gain of exactly one pixel a step: pixel x =
   * 319 - raw y, pixel y = raw x. */
  const TouchCal swapped = {0, -65536, 319 * 65536, 65536, 0, 0};
  TEST_ASSERT_TRUE(touchCalValid(&swapped, W, H));
}

/* Both pixel coordinates from raw x: the glass is folded onto a line. */
static void a_fit_folded_onto_a_line_is_refused(void) {
  const TouchCal folded = {65536, 0, 0, 65536, 0, 0};
  TEST_ASSERT_FALSE(touchCalValid(&folded, W, H));
}

/* pixel = raw - k, so the right edge, 319, needs reading 319 + k and the
 * bottom edge, 239, reading 239 + k. Needing 4094 or 4095 passes, needing
 * 4096 fails. */
static void the_far_corners_need_a_reading_up_to_4095(void) {
  const int need[] = {4094, 4095, 4096};
  const bool ok[] = {true, true, false};
  for (int i = 0; i < 3; i++) {
    TouchCal c = straight(65536);
    c.x0 = -(need[i] - (W - 1)) * 65536;
    TEST_ASSERT_EQUAL(ok[i], touchCalValid(&c, W, H));
    c = straight(65536);
    c.y0 = -(need[i] - (H - 1)) * 65536;
    TEST_ASSERT_EQUAL(ok[i], touchCalValid(&c, W, H));
  }
}

/* pixel x = raw x + k, so the left edge, 0, needs reading -k. On 0 it
 * passes, on -1 it fails. */
static void the_near_corners_need_a_reading_down_to_0(void) {
  const int k[] = {-1, 0, 1};
  const bool ok[] = {true, true, false};
  for (int i = 0; i < 3; i++) {
    TouchCal c = straight(65536);
    c.x0 = k[i] * 65536;
    TEST_ASSERT_EQUAL(ok[i], touchCalValid(&c, W, H));
    c = straight(65536);
    c.y0 = k[i] * 65536;
    TEST_ASSERT_EQUAL(ok[i], touchCalValid(&c, W, H));
  }
}

/* A reversed axis turns the sign of the fit's determinant. pixel x = k -
 * raw x, so pixel 0 needs reading k: 4095 passes, 4096 fails. */
static void a_reversed_axis_is_checked_the_same_way(void) {
  const int k[] = {4094, 4095, 4096};
  const bool ok[] = {true, true, false};
  for (int i = 0; i < 3; i++) {
    const TouchCal c = {-65536, 0, k[i] * 65536, 0, 65536, 0};
    TEST_ASSERT_EQUAL(ok[i], touchCalValid(&c, W, H));
  }
}

/* A screen as wide as the converter has steps passes with every reading
 * used, one wider fails, and so does a screen with no pixels. */
static void the_screen_is_1_to_4096_pixels_each_way(void) {
  const TouchCal c = straight(65536);
  TEST_ASSERT_TRUE(touchCalValid(&c, 4095, 4095));
  TEST_ASSERT_TRUE(touchCalValid(&c, 4096, 4096));
  TEST_ASSERT_FALSE(touchCalValid(&c, 4097, H));
  TEST_ASSERT_FALSE(touchCalValid(&c, W, 4097));
  TEST_ASSERT_TRUE(touchCalValid(&c, 1, 1));
  TEST_ASSERT_FALSE(touchCalValid(&c, 0, H));
  TEST_ASSERT_FALSE(touchCalValid(&c, W, 0));
}

/* ------------------------------------------------------------- the map */

/* At half a pixel a step, a half pixel rounds up. */
static void a_reading_rounds_to_the_nearest_pixel(void) {
  const TouchCal c = straight(32768);
  assertPoint(0, 0, touchCalMap(&c, pt(0, 0), W, H));
  assertPoint(1, 1, touchCalMap(&c, pt(1, 1), W, H));
  assertPoint(1, 1, touchCalMap(&c, pt(2, 2), W, H));
  assertPoint(2, 2, touchCalMap(&c, pt(3, 3), W, H));
}

/* pixel = raw - 1: the reading one under the screen's edge is held on the
 * edge, at both ends. */
static void a_reading_past_the_screen_lands_on_its_edge(void) {
  TouchCal c = straight(65536);
  c.x0 = -65536;
  c.y0 = -65536;
  assertPoint(0, 0, touchCalMap(&c, pt(0, 0), W, H));
  assertPoint(0, 0, touchCalMap(&c, pt(1, 1), W, H));
  assertPoint(1, 1, touchCalMap(&c, pt(2, 2), W, H));
  assertPoint(318, 238, touchCalMap(&c, pt(319, 239), W, H));
  assertPoint(319, 239, touchCalMap(&c, pt(320, 240), W, H));
  assertPoint(319, 239, touchCalMap(&c, pt(321, 241), W, H));
  assertPoint(319, 239, touchCalMap(&c, pt(4095, 4095), W, H));
}

/* ------------------------------------------------------------ the turn */

static void the_normal_rotation_leaves_a_point_alone(void) {
  assertPoint(0, 0, touchTurn(pt(0, 0), W, H, false));
  assertPoint(319, 239, touchTurn(pt(319, 239), W, H, false));
  assertPoint(100, 50, touchTurn(pt(100, 50), W, H, false));
}

static void upside_down_turns_a_point_half_way_round(void) {
  assertPoint(319, 239, touchTurn(pt(0, 0), W, H, true));
  assertPoint(0, 0, touchTurn(pt(319, 239), W, H, true));
  assertPoint(0, 239, touchTurn(pt(319, 0), W, H, true));
  assertPoint(219, 189, touchTurn(pt(100, 50), W, H, true));
  /* Another panel size, so nothing in it is fixed to 320 by 240. */
  assertPoint(479, 319, touchTurn(pt(0, 0), 480, 320, true));
  assertPoint(379, 269, touchTurn(pt(100, 50), 480, 320, true));
}

/* One calibration serves both rotations. With the display upside down, the
 * pixel the screen shows at (x, y) sits at (319 - x, 239 - y) in the
 * board's mount, so a finger there gives that pixel's reading. Mapping it
 * and turning it gives (x, y) back. */
static void one_calibration_serves_both_rotations(void) {
  TouchCal cal;
  TEST_ASSERT_TRUE(fitMarks(skewAt, 5, &cal));
  for (int y = 0; y < H; y += 11) {
    for (int x = 0; x < W; x += 11) {
      const TouchPoint raw = skewRaw(W - 1 - x, H - 1 - y);
      const TouchPoint mount = touchCalMap(&cal, raw, W, H);
      assertPoint(x, y, touchTurn(mount, W, H, true));
    }
  }
}

/* -------------------------------------------------------- hit testing */

/* A zone 30 wide and 40 tall at (10, 20): its first and last pixel each
 * way are in, the next pixel out is not. */
static void a_zone_holds_its_edges_and_corners(void) {
  const TouchZone z[] = {{10, 20, 30, 40, 7}};
  TEST_ASSERT_EQUAL_INT(7, touchZoneAt(z, 1, pt(10, 20)));
  TEST_ASSERT_EQUAL_INT(7, touchZoneAt(z, 1, pt(39, 20)));
  TEST_ASSERT_EQUAL_INT(7, touchZoneAt(z, 1, pt(10, 59)));
  TEST_ASSERT_EQUAL_INT(7, touchZoneAt(z, 1, pt(39, 59)));
  TEST_ASSERT_EQUAL_INT(7, touchZoneAt(z, 1, pt(25, 40)));
  TEST_ASSERT_EQUAL_INT(TOUCH_NO_ZONE, touchZoneAt(z, 1, pt(9, 20)));
  TEST_ASSERT_EQUAL_INT(TOUCH_NO_ZONE, touchZoneAt(z, 1, pt(40, 20)));
  TEST_ASSERT_EQUAL_INT(TOUCH_NO_ZONE, touchZoneAt(z, 1, pt(10, 19)));
  TEST_ASSERT_EQUAL_INT(TOUCH_NO_ZONE, touchZoneAt(z, 1, pt(10, 60)));
  TEST_ASSERT_EQUAL_INT(TOUCH_NO_ZONE, touchZoneAt(z, 1, pt(9, 19)));
  TEST_ASSERT_EQUAL_INT(TOUCH_NO_ZONE, touchZoneAt(z, 1, pt(40, 60)));
}

/* Two zones with a 10 pixel gap: a point in the gap is in neither. */
static void the_gap_between_two_zones_is_in_neither(void) {
  const TouchZone z[] = {{0, 0, 30, 30, 1}, {40, 0, 30, 30, 2}};
  TEST_ASSERT_TRUE(touchZonesValid(z, 2));
  TEST_ASSERT_EQUAL_INT(1, touchZoneAt(z, 2, pt(29, 10)));
  TEST_ASSERT_EQUAL_INT(TOUCH_NO_ZONE, touchZoneAt(z, 2, pt(30, 10)));
  TEST_ASSERT_EQUAL_INT(TOUCH_NO_ZONE, touchZoneAt(z, 2, pt(39, 10)));
  TEST_ASSERT_EQUAL_INT(2, touchZoneAt(z, 2, pt(40, 10)));
}

/* Two zones side by side: the shared edge belongs to the right one, and a
 * zone numbered 0 is found as 0, not as no zone. */
static void a_shared_edge_belongs_to_one_zone(void) {
  const TouchZone z[] = {{0, 0, 30, 30, 0}, {30, 0, 30, 30, 2}};
  TEST_ASSERT_TRUE(touchZonesValid(z, 2));
  TEST_ASSERT_EQUAL_INT(0, touchZoneAt(z, 2, pt(29, 29)));
  TEST_ASSERT_EQUAL_INT(2, touchZoneAt(z, 2, pt(30, 29)));
  TEST_ASSERT_EQUAL_INT(TOUCH_NO_ZONE, touchZoneAt(z, 2, pt(30, 30)));
}

static void an_empty_list_holds_no_point(void) {
  TEST_ASSERT_TRUE(touchZonesValid(NULL, 0));
  TEST_ASSERT_EQUAL_INT(TOUCH_NO_ZONE, touchZoneAt(NULL, 0, pt(10, 10)));
}

/* A second zone starting one pixel inside the first, on its edge, and one
 * pixel past it, each way. */
static void overlapping_zones_are_refused(void) {
  const int start[] = {29, 30, 31};
  const bool ok[] = {false, true, true};
  for (int i = 0; i < 3; i++) {
    const TouchZone across[] = {{0, 0, 30, 30, 1},
                                {(int16_t)start[i], 0, 30, 30, 2}};
    TEST_ASSERT_EQUAL(ok[i], touchZonesValid(across, 2));
    const TouchZone down[] = {{0, 0, 30, 30, 1},
                              {0, (int16_t)start[i], 30, 30, 2}};
    TEST_ASSERT_EQUAL(ok[i], touchZonesValid(down, 2));
  }
  /* One inside another, and a later pair after a good one. */
  const TouchZone inside[] = {{0, 0, 100, 100, 1}, {40, 40, 10, 10, 2}};
  TEST_ASSERT_FALSE(touchZonesValid(inside, 2));
  const TouchZone later[] = {
      {0, 0, 30, 30, 1}, {100, 0, 30, 30, 2}, {120, 20, 30, 30, 3}};
  TEST_ASSERT_FALSE(touchZonesValid(later, 3));
}

/* A zone with no width or no height can never be touched, so the list is
 * wrong. */
static void a_zone_with_no_area_is_refused(void) {
  const TouchZone flat[] = {{0, 0, 30, 0, 1}};
  const TouchZone thin[] = {{0, 0, 0, 30, 1}};
  const TouchZone one[] = {{0, 0, 1, 1, 1}};
  TEST_ASSERT_FALSE(touchZonesValid(flat, 1));
  TEST_ASSERT_FALSE(touchZonesValid(thin, 1));
  TEST_ASSERT_TRUE(touchZonesValid(one, 1));
}

/* ------------------------------------------------------------- gestures */

/* Figures for the tests only, set far enough apart to test each boundary
 * alone. The radio's own come from taps measured on its glass. */
static const TouchGestureConfig GESTURE = {10, 40, 300, TOUCH_HOLD_MS};

static TouchGesture gesture;

static void gestureSetUp(void) {
  memset(&gesture, 0, sizeof(gesture));
}

static TouchGestureEvent feedAt(bool down, int x, int y, bool settled, int zone,
                                bool drags, uint32_t screen, uint32_t ms) {
  TouchSample s;
  s.down = down;
  s.at = pt(x, y);
  s.unsettled = !settled;
  s.zone = zone;
  s.zoneDrags = drags;
  s.screen = screen;
  return touchGestureFeed(&gesture, &GESTURE, &s, ms);
}

/* A settled sample, the usual case. */
static TouchGestureEvent feed(bool down, int x, int y, int zone, bool drags,
                              uint32_t screen, uint32_t ms) {
  return feedAt(down, x, y, true, zone, drags, screen, ms);
}

/* Every touch below goes down at 1000. */
#define START_MS 1000u

/* The finger off at `ms`; what the lift reports once it has lasted the
 * bridge. Nothing before that. */
static TouchGestureEvent lift(uint32_t ms) {
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(false, 0, 0, 0, false, 1, ms));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(false, 0, 0, 0, false, 1,
                                            ms + TOUCH_LIFT_BRIDGE_MS - 1));
  return feed(false, 0, 0, 0, false, 1, ms + TOUCH_LIFT_BRIDGE_MS);
}

static void a_tap_is_reported_once_the_lift_has_lasted_the_bridge(void) {
  gestureSetUp();
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 100, 100, 1, false, 1, START_MS));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 100, 100, 1, false, 1, START_MS + 100));
  TEST_ASSERT_EQUAL_INT(TOUCH_TAP, lift(START_MS + 200));
  TEST_ASSERT_EQUAL_INT(1, gesture.zone);
}

/* A lift one under the hold time is a tap; still down at the hold time is a
 * hold, reported once, and nothing after it. */
static void the_hold_time_splits_a_tap_from_a_hold(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  TEST_ASSERT_EQUAL_INT(TOUCH_TAP, lift(START_MS + TOUCH_HOLD_MS - 1));

  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(true, 100, 100, 1, false, 1,
                                            START_MS + TOUCH_HOLD_MS - 1));
  TEST_ASSERT_EQUAL_INT(
      TOUCH_HOLD, feed(true, 100, 100, 1, false, 1, START_MS + TOUCH_HOLD_MS));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(true, 100, 100, 1, false, 1,
                                            START_MS + TOUCH_HOLD_MS + 1));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, lift(START_MS + TOUCH_HOLD_MS + 500));
}

/* After a hold, a move does nothing, even in a zone that takes drags. */
static void nothing_follows_a_hold(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, true, 1, START_MS);
  TEST_ASSERT_EQUAL_INT(
      TOUCH_HOLD, feed(true, 100, 100, 1, true, 1, START_MS + TOUCH_HOLD_MS));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(true, 160, 100, 1, true, 1,
                                            START_MS + TOUCH_HOLD_MS + 50));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, lift(START_MS + TOUCH_HOLD_MS + 100));
}

/* A move of exactly the slop along either axis is still a tap; one pixel
 * more is not. */
static void the_slop_is_the_furthest_a_tap_can_move(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(true, 110, 100, 1, false, 1, START_MS + 50);
  feed(true, 100, 90, 1, false, 1, START_MS + 60);
  TEST_ASSERT_EQUAL_INT(TOUCH_TAP, lift(START_MS + 100));

  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(true, 100, 111, 1, false, 1, START_MS + 50);
  feed(true, 100, 100, 1, false, 1, START_MS + 60);
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, lift(START_MS + 100));
}

/* A finger that slips past the slop and stays still to the hold time is
 * not a hold either. */
static void a_moved_finger_never_holds(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(true, 120, 100, 1, false, 1, START_MS + 50);
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(true, 120, 100, 1, false, 1,
                                            START_MS + TOUCH_HOLD_MS));
}

/* Sliding off the zone cancels a tap, even when the finger comes back and
 * lifts where it started. */
static void leaving_the_zone_cancels_the_tap(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(true, 104, 100, 2, false, 1, START_MS + 50);
  feed(true, 100, 100, 1, false, 1, START_MS + 60);
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, lift(START_MS + 100));
}

/* In a zone that takes drags, each new point past the slop is a drag, the
 * same point twice is one, the hold time passing changes nothing, and the
 * lift ends it. */
static void a_drag_reports_each_new_point_and_its_end(void) {
  gestureSetUp();
  feed(true, 100, 150, 3, true, 1, START_MS);
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 110, 150, 3, true, 1, START_MS + 20));
  TEST_ASSERT_EQUAL_INT(TOUCH_DRAG,
                        feed(true, 111, 150, 3, true, 1, START_MS + 40));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 111, 150, 3, true, 1, START_MS + 60));
  TEST_ASSERT_EQUAL_INT(
      TOUCH_DRAG, feed(true, 90, 150, 4, true, 1, START_MS + TOUCH_HOLD_MS));
  TEST_ASSERT_EQUAL_INT(90, gesture.last.x);
  TEST_ASSERT_EQUAL_INT(3, gesture.zone);
  TEST_ASSERT_EQUAL_INT(TOUCH_DRAG_END, lift(START_MS + TOUCH_HOLD_MS + 100));
}

/* A swipe needs the swipe distance from its first point: one pixel short is
 * nothing, along either axis. */
static void a_swipe_needs_its_distance(void) {
  static const struct {
    int x, y;
    TouchGestureEvent want;
  } kCases[] = {{139, 100, TOUCH_NOTHING},
                {140, 100, TOUCH_SWIPE_RIGHT},
                {100, 139, TOUCH_NOTHING},
                {100, 140, TOUCH_SWIPE_DOWN}};
  for (const auto &c : kCases) {
    gestureSetUp();
    feed(true, 100, 100, 1, false, 1, START_MS);
    feed(true, c.x, c.y, 1, false, 1, START_MS + 100);
    TEST_ASSERT_EQUAL_INT(c.want, lift(START_MS + 150));
  }
}

/* And the swipe time, first contact to lift: 300 ms is a swipe, 301 is
 * not. The bridge after the lift does not count. */
static void a_swipe_has_to_be_quick(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(true, 160, 100, 1, false, 1, START_MS + 100);
  TEST_ASSERT_EQUAL_INT(TOUCH_SWIPE_RIGHT, lift(START_MS + 300));

  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(true, 160, 100, 1, false, 1, START_MS + 100);
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, lift(START_MS + 301));
}

/* The axis it moved most on gives the direction, the way the finger went. */
static void a_swipe_goes_the_way_the_finger_went(void) {
  static const struct {
    int x, y;
    TouchGestureEvent want;
  } kCases[] = {{50, 110, TOUCH_SWIPE_LEFT},
                {150, 90, TOUCH_SWIPE_RIGHT},
                {110, 50, TOUCH_SWIPE_UP},
                {90, 150, TOUCH_SWIPE_DOWN}};
  for (const auto &c : kCases) {
    gestureSetUp();
    feed(true, 100, 100, 1, false, 1, START_MS);
    feed(true, c.x, c.y, 1, false, 1, START_MS + 100);
    TEST_ASSERT_EQUAL_INT(c.want, lift(START_MS + 150));
  }
}

/* A touch belongs to the screen it started on. When the screen changes
 * under it, nothing more is reported, the lift included; the next touch is
 * read on the new screen. */
static void a_new_screen_ignores_the_rest_of_the_touch(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, true, 1, START_MS);
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 100, 100, 1, true, 2, START_MS + 50));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 160, 100, 1, true, 2, START_MS + 60));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(true, 160, 100, 1, true, 2,
                                            START_MS + TOUCH_HOLD_MS));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(false, 0, 0, 0, true, 2, 3000));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(false, 0, 0, 0, true, 2, 3200));

  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(true, 50, 50, 5, false, 2, 4000));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(false, 0, 0, 0, false, 2, 4100));
  TEST_ASSERT_EQUAL_INT(
      TOUCH_TAP, feed(false, 0, 0, 0, false, 2, 4100 + TOUCH_LIFT_BRIDGE_MS));
}

/* A lift one under the bridge is the same touch; at the bridge, it ends. */
static void a_break_under_the_bridge_is_the_same_touch(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(false, 0, 0, 0, false, 1, START_MS + 100);
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 100, 100, 1, false, 1,
                             START_MS + 100 + TOUCH_LIFT_BRIDGE_MS - 1));
  TEST_ASSERT_EQUAL_INT(TOUCH_TAP, lift(START_MS + 400));

  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(false, 0, 0, 0, false, 1, START_MS + 100);
  TEST_ASSERT_EQUAL_INT(TOUCH_TAP, feed(false, 0, 0, 0, false, 1,
                                        START_MS + 100 + TOUCH_LIFT_BRIDGE_MS));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 100, 100, 1, false, 1, START_MS + 300));
  TEST_ASSERT_EQUAL_INT(TOUCH_TAP, lift(START_MS + 400));
}

/* A swipe that breaks contact is one swipe, measured from its very first
 * point, settled or not. */
static void a_broken_swipe_counts_from_its_first_point(void) {
  gestureSetUp();
  feedAt(true, 250, 120, false, 1, false, 1, START_MS);
  feed(false, 0, 0, 0, false, 1, START_MS + 10);
  feedAt(true, 120, 125, false, 1, false, 1, START_MS + 134);
  feed(true, 115, 125, 1, false, 1, START_MS + 160);
  feed(true, 80, 126, 1, false, 1, START_MS + 200);
  TEST_ASSERT_EQUAL_INT(TOUCH_SWIPE_LEFT, lift(START_MS + 260));
}

/* A landing point far off, in another zone, does not make the tap move or
 * change its zone: the start waits for a settled point. */
static void the_start_waits_for_a_settled_point(void) {
  gestureSetUp();
  feedAt(true, 130, 100, false, 2, false, 1, START_MS);
  feedAt(true, 116, 100, false, 2, false, 1, START_MS + 10);
  feed(true, 100, 100, 1, false, 1, START_MS + 20);
  feed(true, 102, 101, 1, false, 1, START_MS + 30);
  TEST_ASSERT_EQUAL_INT(TOUCH_TAP, lift(START_MS + 100));
  TEST_ASSERT_EQUAL_INT(1, gesture.zone);
}

/* A tap too short to settle taps where its point last was. */
static void a_tap_that_never_settles_taps_where_it_was(void) {
  gestureSetUp();
  feedAt(true, 60, 60, false, 4, false, 1, START_MS);
  TEST_ASSERT_EQUAL_INT(TOUCH_TAP, lift(START_MS + 20));
  TEST_ASSERT_EQUAL_INT(4, gesture.zone);
}

/* After a break, the points not settled again are left out, so a landing
 * point far off does not make the touch move. */
static void a_landing_after_a_break_is_left_out(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(false, 0, 0, 0, false, 1, START_MS + 100);
  feedAt(true, 140, 100, false, 1, false, 1, START_MS + 150);
  feed(true, 101, 100, 1, false, 1, START_MS + 180);
  TEST_ASSERT_EQUAL_INT(TOUCH_TAP, lift(START_MS + 220));
}

/* Times that wrap past 2^32 ms, about 49.7 days of uptime. */
static void a_hold_across_the_clock_wrap(void) {
  gestureSetUp();
  const uint32_t t = 0xFFFFFF00u;
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(true, 10, 10, 1, false, 1, t));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 10, 10, 1, false, 1, t + TOUCH_HOLD_MS - 1));
  TEST_ASSERT_EQUAL_INT(TOUCH_HOLD,
                        feed(true, 10, 10, 1, false, 1, t + TOUCH_HOLD_MS));
}

static void a_missing_argument_reports_nothing(void) {
  gestureSetUp();
  TouchSample s;
  memset(&s, 0, sizeof(s));
  s.down = true;
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, touchGestureFeed(NULL, &GESTURE, &s, 0));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, touchGestureFeed(&gesture, NULL, &s, 0));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        touchGestureFeed(&gesture, &GESTURE, NULL, 0));
  TEST_ASSERT_FALSE(gesture.on);
}

/* A break in contact during a hold does not start its time again: the
 * hold is counted from the first contact. */
static void a_break_does_not_restart_the_hold(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(false, 0, 0, 0, false, 1, START_MS + 1000);
  feed(true, 100, 100, 1, false, 1, START_MS + 1020);
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, feed(true, 100, 100, 1, false, 1,
                                            START_MS + TOUCH_HOLD_MS - 1));
  TEST_ASSERT_EQUAL_INT(
      TOUCH_HOLD, feed(true, 100, 100, 1, false, 1, START_MS + TOUCH_HOLD_MS));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, lift(START_MS + 2300));
}

/* A blip in contact after a hold is still the held touch: no tap, no second
 * hold. */
static void a_blip_after_a_hold_is_not_a_new_touch(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  TEST_ASSERT_EQUAL_INT(
      TOUCH_HOLD, feed(true, 100, 100, 1, false, 1, START_MS + TOUCH_HOLD_MS));
  feed(false, 0, 0, 0, false, 1, START_MS + 1600);
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 100, 100, 1, false, 1, START_MS + 1610));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING,
                        feed(true, 100, 100, 1, false, 1, START_MS + 3200));
  TEST_ASSERT_EQUAL_INT(TOUCH_NOTHING, lift(START_MS + 3300));
}

/* A screen change while the lift waits out the bridge ends the touch with
 * nothing. */
static void a_new_screen_during_the_bridge_ends_with_nothing(void) {
  gestureSetUp();
  feed(true, 100, 100, 1, false, 1, START_MS);
  feed(false, 0, 0, 0, false, 1, START_MS + 200);
  feed(false, 0, 0, 0, false, 2, START_MS + 250);
  TEST_ASSERT_EQUAL_INT(
      TOUCH_NOTHING,
      feed(false, 0, 0, 0, false, 2, START_MS + 200 + TOUCH_LIFT_BRIDGE_MS));
  TEST_ASSERT_FALSE(gesture.on);
}

/* Before the point settles, a move past the swipe distance from the first
 * point is a move, so a swipe broken before it settles is never a tap. */
static void a_long_move_before_settling_is_a_move(void) {
  gestureSetUp();
  feedAt(true, 250, 120, false, 1, false, 1, START_MS);
  feed(false, 0, 0, 0, false, 1, START_MS + 10);
  feedAt(true, 100, 122, false, 2, false, 1, START_MS + 134);
  feed(true, 98, 122, 2, false, 1, START_MS + 160);
  feed(true, 88, 122, 2, false, 1, START_MS + 200);
  TEST_ASSERT_EQUAL_INT(TOUCH_SWIPE_LEFT, lift(START_MS + 260));
}

/* A sample nobody marked unsettled is a settled one. */
static void a_plain_sample_counts_as_settled(void) {
  gestureSetUp();
  TouchSample s;
  memset(&s, 0, sizeof(s));
  s.down = true;
  s.at = pt(100, 100);
  s.zone = 1;
  s.zoneDrags = true;
  touchGestureFeed(&gesture, &GESTURE, &s, START_MS);
  s.at = pt(150, 100);
  TEST_ASSERT_EQUAL_INT(
      TOUCH_DRAG, touchGestureFeed(&gesture, &GESTURE, &s, START_MS + 50));
}

/* -------------------------------------------------------------- filter */

static TouchFilter filter;
static TouchPoint filtered;
static bool filterUnsettled;

static bool feedReading(int x, int y, unsigned z1) {
  TouchReading r;
  r.pen = true;
  r.read = true;
  r.raw = pt(x, y);
  r.z1 = (uint16_t)z1;
  return touchFilterFeed(&filter, &r, &filtered, &filterUnsettled);
}

static bool feedPen(bool pen) {
  TouchReading r;
  memset(&r, 0, sizeof(r));
  r.pen = pen;
  return touchFilterFeed(&filter, &r, &filtered, &filterUnsettled);
}

/* Z1 at the limit is contact, one under is not. */
static void a_z1_under_the_limit_is_no_contact(void) {
  memset(&filter, 0, sizeof(filter));
  TEST_ASSERT_FALSE(feedReading(2000, 2000, TOUCH_Z1_MIN - 1));
  TEST_ASSERT_TRUE(feedReading(2000, 2000, TOUCH_Z1_MIN));
  TEST_ASSERT_TRUE(feedReading(2001, 2001, TOUCH_Z1_MIN + 1));
}

/* A poll with the pen line down and no reading keeps the last reading's
 * verdict, contact or not. */
static void a_poll_without_a_reading_keeps_the_last_verdict(void) {
  memset(&filter, 0, sizeof(filter));
  TEST_ASSERT_TRUE(feedReading(2000, 2000, 600));
  TEST_ASSERT_TRUE(feedPen(true));
  assertPoint(2000, 2000, filtered);
  TEST_ASSERT_FALSE(feedReading(4095, 1206, 4));
  TEST_ASSERT_FALSE(feedPen(true));
  TEST_ASSERT_TRUE(feedReading(2010, 2010, 600));
}

/* The pen line up ends contact, and the next contact starts afresh. */
static void a_lift_ends_contact_and_the_next_starts_afresh(void) {
  memset(&filter, 0, sizeof(filter));
  feedReading(1000, 1000, 600);
  feedReading(1000, 1000, 600);
  feedReading(1000, 1000, 600);
  TEST_ASSERT_FALSE(filterUnsettled);
  TEST_ASSERT_FALSE(feedPen(false));
  TEST_ASSERT_FALSE(feedPen(true));
  TEST_ASSERT_TRUE(feedReading(3000, 3000, 600));
  assertPoint(3000, 3000, filtered);
  TEST_ASSERT_TRUE(filterUnsettled);
}

/* One reading stands; two give their middle; from three on, the median of
 * the latest three, settled, so one reading far from the other two is left
 * out. */
static void the_point_is_the_median_of_the_latest_three(void) {
  memset(&filter, 0, sizeof(filter));
  feedReading(1000, 2000, 600);
  assertPoint(1000, 2000, filtered);
  TEST_ASSERT_TRUE(filterUnsettled);
  feedReading(1010, 2020, 600);
  assertPoint(1005, 2010, filtered);
  TEST_ASSERT_TRUE(filterUnsettled);
  feedReading(3900, 100, 600);
  assertPoint(1010, 2000, filtered);
  TEST_ASSERT_FALSE(filterUnsettled);
  feedReading(1020, 2010, 600);
  assertPoint(1020, 2010, filtered);
  feedReading(1030, 2030, 600);
  assertPoint(1030, 2010, filtered);
}

static void a_missing_filter_or_reading_is_no_contact(void) {
  TouchReading r;
  memset(&r, 0, sizeof(r));
  r.pen = true;
  r.read = true;
  r.z1 = 600;
  TEST_ASSERT_FALSE(touchFilterFeed(NULL, &r, &filtered, &filterUnsettled));
  memset(&filter, 0, sizeof(filter));
  TEST_ASSERT_FALSE(
      touchFilterFeed(&filter, NULL, &filtered, &filterUnsettled));
  TEST_ASSERT_TRUE(touchFilterFeed(&filter, &r, NULL, NULL));
}

/* -------------------------------------------- the taps, played back */

/* The corners of the picture as a pen read them on this radio, a rough map
 * from raw readings to pixels for the play back. */
static bool roughCal(TouchCal *cal) {
  static const TouchPoint raw[4] = {
      {223, 3886}, {243, 481}, {3901, 484}, {3894, 3900}};
  static const TouchPoint screen[4] = {{0, 0}, {319, 0}, {319, 239}, {0, 239}};
  return touchCalFit(raw, screen, 4, W, H, cal);
}

/* The gestures a capture gives, and the median and the furthest of its taps'
 * points. Polls every 10 ms, as the driver reads: a reading when one falls
 * in the poll, else the pen line down if the readings either side are
 * within 100 ms of each other, else up. */
typedef struct {
  int taps, holds, left, right, nothing, other;
  int spread; /* The furthest a tap's point sat from the taps' median, px. */
} Played;

static Played playBack(const TouchCapSet &set, const TouchCal *cal) {
  static const TouchGestureConfig kRadio = {16, 64, 1000, TOUCH_HOLD_MS};
  static TouchPoint tapAt[64];
  Played p;
  memset(&p, 0, sizeof(p));
  int nTaps = 0;
  memset(&filter, 0, sizeof(filter));
  gestureSetUp();
  size_t i = 0;
  const uint32_t end = set.readings[set.count - 1].ms + 400;
  for (uint32_t t = set.readings[0].ms; t <= end; t += 10) {
    TouchReading r;
    memset(&r, 0, sizeof(r));
    const TouchCapReading *got = NULL;
    while (i < set.count && set.readings[i].ms <= t) {
      got = &set.readings[i++];
    }
    if (got != NULL) {
      r.pen = true;
      r.read = true;
      r.raw = pt(got->x, got->y);
      r.z1 = got->z1;
    } else if (i > 0 && i < set.count &&
               set.readings[i].ms - set.readings[i - 1].ms <= 100) {
      r.pen = true;
    }
    TouchPoint raw = pt(0, 0);
    bool unsettled = true;
    TouchSample s;
    memset(&s, 0, sizeof(s));
    s.down = touchFilterFeed(&filter, &r, &raw, &unsettled);
    s.at = touchCalMap(cal, raw, W, H);
    s.unsettled = unsettled;
    s.zone = 1;
    s.screen = 1;
    const bool wasWaiting = gesture.on && !gesture.touching && !gesture.done;
    switch (touchGestureFeed(&gesture, &kRadio, &s, t)) {
      case TOUCH_NOTHING:
        /* A touch, not a finished hold, that waited out its bridge and
         * made nothing. */
        if (wasWaiting && !gesture.on) {
          p.nothing++;
        }
        break;
      case TOUCH_TAP:
        p.taps++;
        if (nTaps < 64) {
          tapAt[nTaps++] = gesture.last;
        }
        break;
      case TOUCH_HOLD:
        p.holds++;
        break;
      case TOUCH_SWIPE_LEFT:
        p.left++;
        break;
      case TOUCH_SWIPE_RIGHT:
        p.right++;
        break;
      default:
        p.other++;
        break;
    }
  }
  /* The spread of the taps around their median, x and y each on its own. */
  int xs[64], ys[64];
  for (int k = 0; k < nTaps; k++) {
    xs[k] = tapAt[k].x;
    ys[k] = tapAt[k].y;
  }
  for (int a = 0; a < nTaps; a++) {
    for (int b = a + 1; b < nTaps; b++) {
      if (xs[b] < xs[a]) {
        const int v = xs[a];
        xs[a] = xs[b];
        xs[b] = v;
      }
      if (ys[b] < ys[a]) {
        const int v = ys[a];
        ys[a] = ys[b];
        ys[b] = v;
      }
    }
  }
  if (nTaps > 0) {
    const int mx = xs[nTaps / 2], my = ys[nTaps / 2];
    for (int k = 0; k < nTaps; k++) {
      const int dx = tapAt[k].x > mx ? tapAt[k].x - mx : mx - tapAt[k].x;
      const int dy = tapAt[k].y > my ? tapAt[k].y - my : my - tapAt[k].y;
      p.spread = dx > p.spread ? dx : p.spread;
      p.spread = dy > p.spread ? dy : p.spread;
    }
  }
  return p;
}

/* Every set of taps, holds and swipes the owner made on this radio, played
 * back through the filter and the gestures with the radio's own figures:
 * taps come out as taps, near each other, holds as holds and swipes as
 * swipes. The weak top right corner loses one tap, whose readings wandered
 * past the slop. */
static void the_owners_taps_holds_and_swipes_come_out_as_made(void) {
  TouchCal cal;
  TEST_ASSERT_TRUE(roughCal(&cal));
  static const struct {
    const char *name;
    int taps, holds, left, right, nothing;
  } kWant[] = {
      {"finger-mark0", 18, 0, 0, 0, 0}, {"finger-mark1", 19, 0, 0, 0, 0},
      {"finger-mark2", 11, 0, 0, 0, 1}, {"finger-mark3", 19, 0, 0, 0, 0},
      {"finger-mark4", 19, 0, 0, 0, 0}, {"pen-mark0", 20, 0, 0, 0, 0},
      {"finger-busy", 10, 0, 0, 0, 0},  {"finger-repaint", 10, 0, 0, 0, 0},
      {"finger-holds", 0, 3, 0, 0, 0},  {"finger-swipes", 0, 0, 5, 5, 0},
      {"finger-light", 10, 0, 0, 0, 0},
  };
  for (const auto &w : kWant) {
    const TouchCapSet *set = NULL;
    for (const TouchCapSet &s : kTouchCaptures) {
      if (strcmp(s.name, w.name) == 0) {
        set = &s;
      }
    }
    TEST_ASSERT_NOT_NULL_MESSAGE(set, w.name);
    const Played p = playBack(*set, &cal);
    TEST_ASSERT_EQUAL_INT_MESSAGE(w.taps, p.taps, w.name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(w.holds, p.holds, w.name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(w.left, p.left, w.name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(w.right, p.right, w.name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(w.nothing, p.nothing, w.name);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, p.other, w.name);
    /* Light taps landed within 16.4 px of each other's middle at the 95th
     * percentile, normal ones within 9.4. */
    TEST_ASSERT_LESS_OR_EQUAL_INT_MESSAGE(18, p.spread, w.name);
  }
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(the_pe5pvb_defaults_fit_back);
  RUN_TEST(a_swapped_reversed_skewed_panel_maps_every_pixel);
  RUN_TEST(three_marks_are_enough);
  RUN_TEST(a_bad_tap_moves_the_fit_by_a_fifth);
  RUN_TEST(marks_read_in_a_line_are_refused);
  RUN_TEST(marks_read_nearly_in_a_line_are_refused);
  RUN_TEST(marks_read_close_together_are_refused);
  RUN_TEST(a_fit_needs_three_to_nine_marks_on_the_screen);
  RUN_TEST(readings_and_marks_out_of_range_are_refused);
  RUN_TEST(a_gain_of_one_pixel_a_step_is_the_most);
  RUN_TEST(a_fit_folded_onto_a_line_is_refused);
  RUN_TEST(the_far_corners_need_a_reading_up_to_4095);
  RUN_TEST(the_near_corners_need_a_reading_down_to_0);
  RUN_TEST(a_reversed_axis_is_checked_the_same_way);
  RUN_TEST(the_screen_is_1_to_4096_pixels_each_way);
  RUN_TEST(a_reading_rounds_to_the_nearest_pixel);
  RUN_TEST(a_reading_past_the_screen_lands_on_its_edge);
  RUN_TEST(the_normal_rotation_leaves_a_point_alone);
  RUN_TEST(upside_down_turns_a_point_half_way_round);
  RUN_TEST(one_calibration_serves_both_rotations);
  RUN_TEST(a_zone_holds_its_edges_and_corners);
  RUN_TEST(the_gap_between_two_zones_is_in_neither);
  RUN_TEST(a_shared_edge_belongs_to_one_zone);
  RUN_TEST(an_empty_list_holds_no_point);
  RUN_TEST(overlapping_zones_are_refused);
  RUN_TEST(a_zone_with_no_area_is_refused);
  RUN_TEST(a_tap_is_reported_once_the_lift_has_lasted_the_bridge);
  RUN_TEST(the_hold_time_splits_a_tap_from_a_hold);
  RUN_TEST(nothing_follows_a_hold);
  RUN_TEST(the_slop_is_the_furthest_a_tap_can_move);
  RUN_TEST(a_moved_finger_never_holds);
  RUN_TEST(leaving_the_zone_cancels_the_tap);
  RUN_TEST(a_drag_reports_each_new_point_and_its_end);
  RUN_TEST(a_swipe_needs_its_distance);
  RUN_TEST(a_swipe_has_to_be_quick);
  RUN_TEST(a_swipe_goes_the_way_the_finger_went);
  RUN_TEST(a_new_screen_ignores_the_rest_of_the_touch);
  RUN_TEST(a_break_under_the_bridge_is_the_same_touch);
  RUN_TEST(a_broken_swipe_counts_from_its_first_point);
  RUN_TEST(the_start_waits_for_a_settled_point);
  RUN_TEST(a_tap_that_never_settles_taps_where_it_was);
  RUN_TEST(a_landing_after_a_break_is_left_out);
  RUN_TEST(a_hold_across_the_clock_wrap);
  RUN_TEST(a_break_does_not_restart_the_hold);
  RUN_TEST(a_blip_after_a_hold_is_not_a_new_touch);
  RUN_TEST(a_new_screen_during_the_bridge_ends_with_nothing);
  RUN_TEST(a_long_move_before_settling_is_a_move);
  RUN_TEST(a_plain_sample_counts_as_settled);
  RUN_TEST(a_missing_argument_reports_nothing);
  RUN_TEST(a_z1_under_the_limit_is_no_contact);
  RUN_TEST(a_poll_without_a_reading_keeps_the_last_verdict);
  RUN_TEST(a_lift_ends_contact_and_the_next_starts_afresh);
  RUN_TEST(the_point_is_the_median_of_the_latest_three);
  RUN_TEST(a_missing_filter_or_reading_is_no_contact);
  RUN_TEST(the_owners_taps_holds_and_swipes_come_out_as_made);
  return UNITY_END();
}
