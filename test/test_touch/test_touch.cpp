/* Tests for the touch calibration, the half turn and hit testing. Runs on a
 * PC. */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/touch.h"

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
  return UNITY_END();
}
