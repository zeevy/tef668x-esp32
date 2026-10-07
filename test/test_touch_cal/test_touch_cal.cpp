/* Tests for calibrating the touch screen. Runs on a PC. */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/touch_cal.h"

#define W 320
#define H 240

void setUp(void) {}
void tearDown(void) {}

static TouchPoint pt(int x, int y) {
  const TouchPoint p = {(int16_t)x, (int16_t)y};
  return p;
}

/* A panel with its axes swapped, one reversed and a little skew: the raw
 * reading for a pixel of the board's own mount. */
static TouchPoint panelRaw(TouchPoint px) {
  return pt(200 + 14 * px.y + px.x, 3700 - 10 * px.x + px.y);
}

/* The exact map of that panel, from four of its pixels. */
static TouchCal panelCal(void) {
  const TouchPoint px[4] = {{0, 0}, {319, 0}, {319, 239}, {0, 239}};
  TouchPoint raw[4];
  for (int i = 0; i < 4; i++) {
    raw[i] = panelRaw(px[i]);
  }
  TouchCal cal;
  TEST_ASSERT_TRUE(touchCalFit(raw, px, 4, W, H, &cal));
  return cal;
}

/* A rough guide: the panel's map moved 10 px, as a map from another radio
 * might be. */
static TouchCal roughGuide(void) {
  TouchCal cal = panelCal();
  cal.x0 += 10 * TOUCH_CAL_ONE;
  return cal;
}

static TouchCalFlow flow;
static uint32_t now;

/* One poll: contact with a new settled reading at `raw`, or none. */
static bool feed(bool contact, TouchPoint raw) {
  now += 10;
  return touchCalFlowFeed(&flow, contact, contact, raw, false, now);
}

/* No contact until a lift is a lift, then one poll more. */
static bool liftNow(void) {
  touchCalFlowFeed(&flow, false, false, pt(0, 0), false, now + 10);
  now += 10 + TOUCH_LIFT_BRIDGE_MS;
  return touchCalFlowFeed(&flow, false, false, pt(0, 0), false, now);
}

/* Hold `screenPx` with `n` settled readings, then lift. The finger is where
 * the screen shows the mark; the raw readings come from the place in the
 * board's own mount under it. */
static void holdAt(TouchPoint screenPx, int n, bool upsideDown) {
  const TouchPoint mount = touchTurn(screenPx, W, H, upsideDown);
  const TouchPoint raw = panelRaw(mount);
  for (int i = 0; i < n; i++) {
    feed(true, raw);
  }
  liftNow();
}

static void allMarks(bool upsideDown) {
  for (uint8_t i = 0; i < TOUCH_CAL_MARKS; i++) {
    holdAt(touchCalFlowMark(&flow, i), TOUCH_CAL_FILL, upsideDown);
  }
}

static void the_marks_sit_in_from_each_corner_and_in_the_middle(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  const TouchPoint want[5] = {
      {32, 32}, {287, 32}, {287, 207}, {32, 207}, {160, 120}};
  for (uint8_t i = 0; i < 5; i++) {
    TEST_ASSERT_EQUAL_INT(want[i].x, touchCalFlowMark(&flow, i).x);
    TEST_ASSERT_EQUAL_INT(want[i].y, touchCalFlowMark(&flow, i).y);
  }
  TEST_ASSERT_EQUAL_INT(224, touchCalFlowCheckDot(&flow).x);
  TEST_ASSERT_EQUAL_INT(90, touchCalFlowCheckDot(&flow).y);
}

/* Five marks held and a check tapped on the dot: saved, and the new map puts
 * every mark where it was drawn, though the guide was 10 px off. */
static void a_calibration_held_and_checked_passes(void) {
  const TouchCal guide = roughGuide();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  allMarks(false);
  TEST_ASSERT_EQUAL_INT(TOUCH_CAL_CHECK, flow.step);
  holdAt(touchCalFlowCheckDot(&flow), 3, false);
  TEST_ASSERT_EQUAL_INT(TOUCH_CAL_PASSED, flow.step);
  TEST_ASSERT_EQUAL_UINT16(0, flow.checkOffPx);
  for (uint8_t i = 0; i < 5; i++) {
    const TouchPoint m = touchCalFlowMark(&flow, i);
    const TouchPoint got = touchCalMap(&flow.result, panelRaw(m), W, H);
    TEST_ASSERT_INT_WITHIN(1, m.x, got.x);
    TEST_ASSERT_INT_WITHIN(1, m.y, got.y);
  }
}

/* A mark takes exactly TOUCH_CAL_FILL settled readings. */
static void a_mark_takes_its_fill_of_readings(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  const TouchPoint raw = panelRaw(touchCalFlowMark(&flow, 0));
  for (int i = 0; i < TOUCH_CAL_FILL - 1; i++) {
    feed(true, raw);
  }
  TEST_ASSERT_EQUAL_INT(0, flow.mark);
  TEST_ASSERT_EQUAL_INT(TOUCH_CAL_FILL - 1, flow.filled);
  TEST_ASSERT_TRUE(feed(true, raw));
  TEST_ASSERT_EQUAL_INT(1, flow.mark);
  TEST_ASSERT_EQUAL_INT(0, flow.filled);
}

/* Readings not yet settled do not count. */
static void unsettled_readings_do_not_fill(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  const TouchPoint raw = panelRaw(touchCalFlowMark(&flow, 0));
  TEST_ASSERT_FALSE(touchCalFlowFeed(&flow, true, true, raw, true, 10));
  TEST_ASSERT_EQUAL_INT(0, flow.filled);
  /* Nor does a lift with nothing filled change what is shown. */
  TEST_ASSERT_FALSE(liftNow());
}

/* A lift before the mark is full starts it again. */
static void a_lift_before_full_starts_the_mark_again(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  holdAt(touchCalFlowMark(&flow, 0), 10, false);
  TEST_ASSERT_EQUAL_INT(0, flow.mark);
  TEST_ASSERT_EQUAL_INT(0, flow.filled);
}

/* A finger may wander up to the still distance; one pixel more and the mark
 * fills again from there. */
static void a_finger_that_slides_fills_again(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  const TouchPoint m = touchCalFlowMark(&flow, 0);
  feed(true, panelRaw(m));
  feed(true, panelRaw(pt(m.x + TOUCH_CAL_STILL_PX, m.y)));
  TEST_ASSERT_EQUAL_INT(2, flow.filled);
  feed(true, panelRaw(pt(m.x, m.y + TOUCH_CAL_STILL_PX + 1)));
  TEST_ASSERT_EQUAL_INT(1, flow.filled);
}

/* After a mark, holding on fills nothing until the finger lifts. */
static void the_next_mark_waits_for_a_lift(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  const TouchPoint raw = panelRaw(touchCalFlowMark(&flow, 0));
  for (int i = 0; i < TOUCH_CAL_FILL + 5; i++) {
    feed(true, raw);
  }
  TEST_ASSERT_EQUAL_INT(1, flow.mark);
  TEST_ASSERT_EQUAL_INT(0, flow.filled);
  liftNow();
  feed(true, raw);
  TEST_ASSERT_EQUAL_INT(1, flow.filled);
}

/* The check: TOUCH_CAL_CHECK_PX off the dot is saved, one more is not. */
static void the_check_has_to_land_near_the_dot(void) {
  const TouchCal guide = panelCal();
  for (int off = TOUCH_CAL_CHECK_PX; off <= TOUCH_CAL_CHECK_PX + 1; off++) {
    touchCalFlowBegin(&flow, W, H, false, &guide);
    allMarks(false);
    const TouchPoint dot = touchCalFlowCheckDot(&flow);
    holdAt(pt(dot.x + off, dot.y), 3, false);
    TEST_ASSERT_EQUAL_INT(
        off <= TOUCH_CAL_CHECK_PX ? TOUCH_CAL_PASSED : TOUCH_CAL_MISSED,
        flow.step);
    TEST_ASSERT_EQUAL(off > TOUCH_CAL_CHECK_PX, touchCalFlowFailed(&flow));
    TEST_ASSERT_EQUAL_UINT16(off, flow.checkOffPx);
  }
}

/* Calibrated with the screen upside down, the map is still the board's own
 * mount: the marks map back to where they sit in the mount. */
static void a_calibration_upside_down_passes_in_the_mount(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, true, &guide);
  allMarks(true);
  holdAt(touchCalFlowCheckDot(&flow), 3, true);
  TEST_ASSERT_EQUAL_INT(TOUCH_CAL_PASSED, flow.step);
  const TouchPoint mount = pt(50, 60);
  const TouchPoint got = touchCalMap(&flow.result, panelRaw(mount), W, H);
  TEST_ASSERT_INT_WITHIN(1, 50, got.x);
  TEST_ASSERT_INT_WITHIN(1, 60, got.y);
}

/* Every mark held at one place makes no calibration. */
static void marks_in_one_place_make_no_calibration(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  for (int i = 0; i < TOUCH_CAL_MARKS; i++) {
    holdAt(pt(160, 120), TOUCH_CAL_FILL, false);
  }
  TEST_ASSERT_EQUAL_INT(TOUCH_CAL_NO_FIT, flow.step);
  TEST_ASSERT_TRUE(touchCalFlowFailed(&flow));
  TEST_ASSERT_FALSE(feed(true, panelRaw(pt(1, 1))));
  /* What the caller sets when it cannot save: a failure too. */
  flow.step = TOUCH_CAL_NOT_SAVED;
  TEST_ASSERT_TRUE(touchCalFlowFailed(&flow));
}

/* A check lift with no reading before it does nothing. */
static void a_check_lift_with_no_reading_does_nothing(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  allMarks(false);
  TEST_ASSERT_FALSE(liftNow());
  TEST_ASSERT_EQUAL_INT(TOUCH_CAL_CHECK, flow.step);
}

/* A break in contact under the bridge is no lift: the mark keeps filling. */
static void a_short_break_does_not_empty_the_mark(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  const TouchPoint raw = panelRaw(touchCalFlowMark(&flow, 0));
  for (int i = 0; i < 5; i++) {
    feed(true, raw);
  }
  touchCalFlowFeed(&flow, false, false, raw, false, now + 10);
  touchCalFlowFeed(&flow, false, false, raw, false,
                   now + TOUCH_LIFT_BRIDGE_MS - 1);
  now += TOUCH_LIFT_BRIDGE_MS - 1;
  TEST_ASSERT_EQUAL_INT(5, flow.filled);
  feed(true, raw);
  TEST_ASSERT_EQUAL_INT(6, flow.filled);
}

/* A poll with contact but no new reading, a loop faster than the chip is
 * read, does not count. */
static void a_poll_without_a_new_reading_does_not_fill(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  const TouchPoint raw = panelRaw(touchCalFlowMark(&flow, 0));
  feed(true, raw);
  for (int i = 0; i < 20; i++) {
    TEST_ASSERT_FALSE(touchCalFlowFeed(&flow, true, false, raw, false, now));
  }
  TEST_ASSERT_EQUAL_INT(1, flow.filled);
}

/* The check takes a settled reading over an unsettled one: a lift reading
 * far off, not yet settled, does not move it. */
static void the_check_prefers_a_settled_reading(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(&flow, W, H, false, &guide);
  allMarks(false);
  const TouchPoint dot = touchCalFlowCheckDot(&flow);
  feed(true, panelRaw(dot));
  now += 10;
  touchCalFlowFeed(&flow, true, true, panelRaw(pt(dot.x + 60, dot.y)), true,
                   now);
  liftNow();
  TEST_ASSERT_EQUAL_INT(TOUCH_CAL_PASSED, flow.step);
}

/* The corners a pen read on this radio make a map that puts them back on
 * the corners. */
static void the_built_in_map_comes_from_the_corners(void) {
  const TouchPoint corners[4] = {
      {223, 3886}, {243, 481}, {3901, 484}, {3894, 3900}};
  TouchCal cal;
  TEST_ASSERT_TRUE(touchCalFromCorners(corners, W, H, &cal));
  const TouchPoint want[4] = {{0, 0}, {319, 0}, {319, 239}, {0, 239}};
  for (int i = 0; i < 4; i++) {
    const TouchPoint got = touchCalMap(&cal, corners[i], W, H);
    TEST_ASSERT_INT_WITHIN(3, want[i].x, got.x);
    TEST_ASSERT_INT_WITHIN(3, want[i].y, got.y);
  }
  const TouchPoint flat[4] = {{1, 1}, {1, 1}, {1, 1}, {1, 1}};
  TEST_ASSERT_FALSE(touchCalFromCorners(flat, W, H, &cal));
  TEST_ASSERT_FALSE(touchCalFromCorners(NULL, W, H, &cal));
}

/* A saved calibration comes back from its blob whole; a blob of another
 * version or a map that cannot be saved does not. */
static void a_calibration_round_trips_through_its_blob(void) {
  TouchCalBlob blob;
  memset(&blob, 0, sizeof(blob));
  blob.version = TOUCH_CAL_BLOB_VERSION;
  blob.cal = panelCal();
  TouchCal back;
  memset(&back, 0, sizeof(back));
  TEST_ASSERT_TRUE(touchCalBlobRead(&blob, W, H, &back));
  TEST_ASSERT_EQUAL_MEMORY(&blob.cal, &back, sizeof(back));

  TouchCalBlob other = blob;
  other.version = TOUCH_CAL_BLOB_VERSION + 1;
  TEST_ASSERT_FALSE(touchCalBlobRead(&other, W, H, &back));
  other = blob;
  memset(&other.cal, 0, sizeof(other.cal));
  TEST_ASSERT_FALSE(touchCalBlobRead(&other, W, H, &back));
}

static void a_missing_argument_is_refused(void) {
  const TouchCal guide = panelCal();
  touchCalFlowBegin(NULL, W, H, false, &guide);
  TEST_ASSERT_FALSE(touchCalFlowFeed(NULL, true, true, pt(1, 1), false, 0));
  TEST_ASSERT_EQUAL_INT(0, touchCalFlowMark(NULL, 0).x);
  TEST_ASSERT_EQUAL_INT(0, touchCalFlowCheckDot(NULL).x);
  TouchCal cal;
  TEST_ASSERT_FALSE(touchCalBlobRead(NULL, W, H, &cal));
  TEST_ASSERT_FALSE(touchCalFlowFailed(NULL));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(the_marks_sit_in_from_each_corner_and_in_the_middle);
  RUN_TEST(a_calibration_held_and_checked_passes);
  RUN_TEST(a_mark_takes_its_fill_of_readings);
  RUN_TEST(unsettled_readings_do_not_fill);
  RUN_TEST(a_lift_before_full_starts_the_mark_again);
  RUN_TEST(a_finger_that_slides_fills_again);
  RUN_TEST(the_next_mark_waits_for_a_lift);
  RUN_TEST(the_check_has_to_land_near_the_dot);
  RUN_TEST(a_calibration_upside_down_passes_in_the_mount);
  RUN_TEST(marks_in_one_place_make_no_calibration);
  RUN_TEST(a_check_lift_with_no_reading_does_nothing);
  RUN_TEST(a_short_break_does_not_empty_the_mark);
  RUN_TEST(a_poll_without_a_new_reading_does_not_fill);
  RUN_TEST(the_check_prefers_a_settled_reading);
  RUN_TEST(the_built_in_map_comes_from_the_corners);
  RUN_TEST(a_calibration_round_trips_through_its_blob);
  RUN_TEST(a_missing_argument_is_refused);
  return UNITY_END();
}
