/* Tests for the sliding tuning scale. Runs on a PC. */
#include <unity.h>

#include <stdint.h>

#include "core/scale.h"

void setUp(void) {}
void tearDown(void) {}

/* A test row width of 300 pixels, so the pointer is at 150. */
#define ROW 300
#define CENTRE 150

/* FM is the band core/band_plan.c gives. MW is 522 to 1620 kHz on 9 kHz
 * spacing, a test range narrower than band_plan.c's 522 to 1791. */
#define FM_LOW 87500
#define FM_HIGH 108000
#define MW_LOW 522
#define MW_HIGH 1620

/* The first tick at this x, or NULL. */
static const ScaleTick *at(const ScaleTick *t, int n, int16_t x) {
  for (int i = 0; i < n; i++) {
    if (t[i].x == x) {
      return &t[i];
    }
  }
  return NULL;
}

static void assertEvenlySpaced(const ScaleTick *t, int n) {
  for (int i = 1; i < n; i++) {
    TEST_ASSERT_EQUAL_INT16(SCALE_TICK_PX, t[i].x - t[i - 1].x);
  }
}

/* -------------------------------------------------------------- the marks */

/* FM at 106.40 MHz: a mark every 100 kHz, 8 pixels apart, long and numbered
 * on the whole megahertz, medium on the half. */
static void fm_is_marked_every_100_kilohertz(void) {
  ScaleTick t[SCALE_MAX_TICKS];
  const int n =
      scaleTicks(FM_LOW, FM_HIGH, 106400, true, ROW, t, SCALE_MAX_TICKS);
  /* 43: the 37 on the row and three past each end, for the numbers. */
  TEST_ASSERT_EQUAL_INT(43, n);
  TEST_ASSERT_EQUAL_INT16(-18, t[0].x);
  TEST_ASSERT_EQUAL_INT16(318, t[n - 1].x);
  assertEvenlySpaced(t, n);
  TEST_ASSERT_EQUAL(SCALE_SHORT, at(t, n, CENTRE)->mark);
  const ScaleTick *m106 = at(t, n, CENTRE - 32);
  TEST_ASSERT_NOT_NULL(m106);
  TEST_ASSERT_EQUAL(SCALE_LONG, m106->mark);
  TEST_ASSERT_EQUAL_UINT32(106, m106->number);
  TEST_ASSERT_EQUAL(SCALE_MEDIUM, at(t, n, CENTRE + 8)->mark);
  TEST_ASSERT_EQUAL_UINT32(107, at(t, n, CENTRE + 48)->number);
}

/* MW at 738 kHz: a mark every 10 kHz, long and numbered in kHz on every
 * hundred. */
static void mw_is_marked_every_10_kilohertz(void) {
  ScaleTick t[SCALE_MAX_TICKS];
  const int n =
      scaleTicks(MW_LOW, MW_HIGH, 738, false, ROW, t, SCALE_MAX_TICKS);
  assertEvenlySpaced(t, n);
  const ScaleTick *m700 = at(t, n, CENTRE - 31);
  TEST_ASSERT_NOT_NULL(m700);
  TEST_ASSERT_EQUAL(SCALE_LONG, m700->mark);
  TEST_ASSERT_EQUAL_UINT32(700, m700->number);
  TEST_ASSERT_EQUAL(SCALE_MEDIUM, at(t, n, CENTRE - 31 + 40)->mark); /* 750 */
}

/* At the top of FM the scale goes on round to the bottom: four dummies, then
 * 87.5 five ticks right of 108, and 88 numbered ten ticks right, still 8
 * pixels apart. At the bottom, 108 is five ticks to the left. */
static void the_two_ends_of_fm_meet(void) {
  ScaleTick t[SCALE_MAX_TICKS];
  int n = scaleTicks(FM_LOW, FM_HIGH, 108000, true, ROW, t, SCALE_MAX_TICKS);
  assertEvenlySpaced(t, n);
  TEST_ASSERT_TRUE(at(t, n, CENTRE)->inBand);
  TEST_ASSERT_EQUAL_UINT32(108, at(t, n, CENTRE)->number);
  TEST_ASSERT_EQUAL(SCALE_LONG, at(t, n, CENTRE)->mark);
  for (int16_t i = 1; i <= SCALE_SEAM_TICKS; i++) {
    const ScaleTick *d = at(t, n, (int16_t)(CENTRE + 8 * i));
    TEST_ASSERT_NOT_NULL(d);
    TEST_ASSERT_FALSE(d->inBand);
    TEST_ASSERT_EQUAL(SCALE_SHORT, d->mark);
    TEST_ASSERT_EQUAL_UINT32(0, d->number);
  }
  TEST_ASSERT_TRUE(at(t, n, CENTRE + 40)->inBand);
  TEST_ASSERT_EQUAL(SCALE_MEDIUM, at(t, n, CENTRE + 40)->mark); /* 87.5 */
  TEST_ASSERT_EQUAL(SCALE_SHORT, at(t, n, CENTRE + 48)->mark);  /* 87.6 */
  TEST_ASSERT_EQUAL(SCALE_LONG, at(t, n, CENTRE + 80)->mark);
  TEST_ASSERT_EQUAL_UINT32(88, at(t, n, CENTRE + 80)->number);

  n = scaleTicks(FM_LOW, FM_HIGH, 87500, true, ROW, t, SCALE_MAX_TICKS);
  assertEvenlySpaced(t, n);
  TEST_ASSERT_EQUAL(SCALE_LONG, at(t, n, CENTRE - 40)->mark);
  TEST_ASSERT_EQUAL_UINT32(108, at(t, n, CENTRE - 40)->number);
  TEST_ASSERT_FALSE(at(t, n, CENTRE - 8)->inBand);
}

/* MW at the test range's top channel, 1620: four dummies, then the band's
 * first tick, 530, and 600 numbered. */
static void the_two_ends_of_mw_meet(void) {
  ScaleTick t[SCALE_MAX_TICKS];
  const int n =
      scaleTicks(MW_LOW, MW_HIGH, 1620, false, ROW, t, SCALE_MAX_TICKS);
  assertEvenlySpaced(t, n);
  TEST_ASSERT_EQUAL(SCALE_SHORT, at(t, n, CENTRE)->mark); /* 1620 */
  TEST_ASSERT_FALSE(at(t, n, CENTRE + 32)->inBand);
  TEST_ASSERT_TRUE(at(t, n, CENTRE + 40)->inBand); /* 530 */
  TEST_ASSERT_EQUAL_UINT32(600, at(t, n, CENTRE + 96)->number);
  TEST_ASSERT_EQUAL(SCALE_LONG, at(t, n, CENTRE - 16)->mark); /* 1600 */
}

/* On SW both ends are long marks, 27000 and 1700. The dummies keep them 40
 * pixels apart, so their numbers have room. */
static void the_two_ends_of_sw_are_40_pixels_apart(void) {
  ScaleTick t[SCALE_MAX_TICKS];
  const int n = scaleTicks(1700, 27000, 27000, false, ROW, t, SCALE_MAX_TICKS);
  TEST_ASSERT_EQUAL_UINT32(27000, at(t, n, CENTRE)->number);
  TEST_ASSERT_EQUAL(SCALE_LONG, at(t, n, CENTRE + 40)->mark);
  TEST_ASSERT_EQUAL_UINT32(1700, at(t, n, CENTRE + 40)->number);
}

/* The marks never reach further than SCALE_EDGE_PX past the row, and the
 * widest row, 320, fits in SCALE_MAX_TICKS wherever the dial is, the seam
 * included. */
static void every_mark_fits_on_the_row(void) {
  ScaleTick t[SCALE_MAX_TICKS + 8];
  int most = 0;
  for (uint32_t f = FM_LOW; f <= FM_HIGH; f += 37) {
    const int n =
        scaleTicks(FM_LOW, FM_HIGH, f, true, 320, t, SCALE_MAX_TICKS + 8);
    for (int i = 0; i < n; i++) {
      TEST_ASSERT_TRUE(t[i].x >= -SCALE_EDGE_PX &&
                       t[i].x <= 318 + SCALE_EDGE_PX);
    }
    most = n > most ? n : most;
  }
  TEST_ASSERT_EQUAL_INT(SCALE_MAX_TICKS, most);
}

/* -------------------------------------------------------------- the seam */

/* In the middle of the dummies, two and a half ticks past 108, wherever that
 * is on the row, and not drawn when it is out of view. */
static void the_seam_is_in_the_middle_of_the_dummies(void) {
  int16_t x[SCALE_MAX_SEAMS];
  TEST_ASSERT_EQUAL_INT(
      1, scaleSeams(FM_LOW, FM_HIGH, 108000, true, ROW, x, SCALE_MAX_SEAMS));
  TEST_ASSERT_EQUAL_INT16(CENTRE + 20, x[0]);
  TEST_ASSERT_EQUAL_INT(
      1, scaleSeams(FM_LOW, FM_HIGH, 87500, true, ROW, x, SCALE_MAX_SEAMS));
  TEST_ASSERT_EQUAL_INT16(CENTRE - 20, x[0]);
  /* 1.85 MHz right of 106.40 is 148 pixels, the row's last column. */
  TEST_ASSERT_EQUAL_INT(
      1, scaleSeams(FM_LOW, FM_HIGH, 106400, true, ROW, x, SCALE_MAX_SEAMS));
  TEST_ASSERT_EQUAL_INT16(ROW - 2, x[0]);
  TEST_ASSERT_EQUAL_INT(
      0, scaleSeams(FM_LOW, FM_HIGH, 100000, true, ROW, x, SCALE_MAX_SEAMS));
  TEST_ASSERT_EQUAL_INT(
      1, scaleSeams(MW_LOW, MW_HIGH, 1620, false, ROW, x, SCALE_MAX_SEAMS));
  TEST_ASSERT_EQUAL_INT16(CENTRE + 20, x[0]);
  /* Never on a mark, real or dummy. */
  ScaleTick t[SCALE_MAX_TICKS];
  const int n =
      scaleTicks(MW_LOW, MW_HIGH, 1620, false, ROW, t, SCALE_MAX_TICKS);
  TEST_ASSERT_NULL(at(t, n, x[0]));
}

/* A band whose loop is shorter than the row, 1000 to 1200 kHz, 21 ticks and
 * 4 dummies, 200 pixels round: both sides of the pointer reach the seam. */
static void a_band_shorter_than_the_row_shows_its_seam_twice(void) {
  int16_t x[SCALE_MAX_SEAMS];
  TEST_ASSERT_EQUAL_INT(
      2, scaleSeams(1000, 1200, 1100, false, ROW, x, SCALE_MAX_SEAMS));
  TEST_ASSERT_EQUAL_INT16(CENTRE - 100, x[0]);
  TEST_ASSERT_EQUAL_INT16(CENTRE + 100, x[1]);
  ScaleTick t[SCALE_MAX_TICKS];
  const int n = scaleTicks(1000, 1200, 1100, false, ROW, t, SCALE_MAX_TICKS);
  assertEvenlySpaced(t, n);
  TEST_ASSERT_NULL(at(t, n, x[0]));
  TEST_ASSERT_NULL(at(t, n, x[1]));
  /* Room for no more than asked. */
  TEST_ASSERT_EQUAL_INT(1, scaleSeams(1000, 1200, 1100, false, ROW, x, 1));
}

/* -------------------------------------------------------------- the peak */

/* The two pixels either side of the middle are under the middle mark. */
static void marks_under_the_middle_are_hidden(void) {
  TEST_ASSERT_TRUE(scaleUnderMiddle(0));
  TEST_ASSERT_TRUE(scaleUnderMiddle(1));
  TEST_ASSERT_TRUE(scaleUnderMiddle(-1));
  TEST_ASSERT_FALSE(scaleUnderMiddle(2));
  TEST_ASSERT_FALSE(scaleUnderMiddle(-2));
  TEST_ASSERT_EQUAL_INT(-1, scalePeakRank(0));
  TEST_ASSERT_EQUAL_INT(-1, scalePeakRank(1));
}

/* Ranks run outwards a tick at a time, and stop after four. */
static void the_peak_ranks_run_outwards(void) {
  TEST_ASSERT_EQUAL_INT(0, scalePeakRank(2));
  TEST_ASSERT_EQUAL_INT(0, scalePeakRank(8));
  TEST_ASSERT_EQUAL_INT(0, scalePeakRank(-9));
  TEST_ASSERT_EQUAL_INT(1, scalePeakRank(10));
  TEST_ASSERT_EQUAL_INT(1, scalePeakRank(-16));
  TEST_ASSERT_EQUAL_INT(3, scalePeakRank(33));
  TEST_ASSERT_EQUAL_INT(-1, scalePeakRank(34));
  TEST_ASSERT_EQUAL_INT(-1, scalePeakRank(-200));
}

/* Wherever the dial sits between two marks, each side has one mark of each
 * rank. */
static void each_side_has_one_mark_of_each_rank(void) {
  for (int32_t first = -SCALE_TICK_PX + 1; first <= 0; first++) {
    int left[SCALE_PEAK_RANKS] = {0};
    int right[SCALE_PEAK_RANKS] = {0};
    for (int32_t dx = first - 6 * SCALE_TICK_PX;
         dx <= first + 6 * SCALE_TICK_PX; dx += SCALE_TICK_PX) {
      const int r = scalePeakRank(dx);
      if (r >= 0) {
        (dx < 0 ? left : right)[r]++;
      }
    }
    for (int r = 0; r < SCALE_PEAK_RANKS; r++) {
      TEST_ASSERT_EQUAL_INT(1, left[r]);
      TEST_ASSERT_EQUAL_INT(1, right[r]);
    }
  }
}

/* At a signal of 75 per cent the peak is 24 pixels high at the nearest mark,
 * then 19, 13 and 8 at the next three. Full signal is the full height, none
 * is flat, and over 100 counts as 100. */
static void the_peak_follows_the_signal(void) {
  TEST_ASSERT_EQUAL_INT(24, scalePeakPx(0, 75));
  TEST_ASSERT_EQUAL_INT(19, scalePeakPx(1, 75));
  TEST_ASSERT_EQUAL_INT(13, scalePeakPx(2, 75));
  TEST_ASSERT_EQUAL_INT(8, scalePeakPx(3, 75));
  TEST_ASSERT_EQUAL_INT(SCALE_PEAK_MAX_PX, scalePeakPx(0, 100));
  TEST_ASSERT_EQUAL_INT(SCALE_PEAK_MAX_PX, scalePeakPx(0, 250));
  TEST_ASSERT_EQUAL_INT(0, scalePeakPx(0, 0));
  TEST_ASSERT_EQUAL_INT(0, scalePeakPx(-1, 100));
  TEST_ASSERT_EQUAL_INT(0, scalePeakPx(SCALE_PEAK_RANKS, 100));
}

/* --------------------------------------------------------- no scale at all */

static void the_ticks_stop_at_the_room_given(void) {
  ScaleTick t[6];
  t[5].x = 1234;
  TEST_ASSERT_EQUAL_INT(5,
                        scaleTicks(FM_LOW, FM_HIGH, 100000, true, ROW, t, 5));
  TEST_ASSERT_EQUAL_INT16(1234, t[5].x);
}

static void no_band_or_no_row_has_no_scale(void) {
  ScaleTick t[SCALE_MAX_TICKS];
  int16_t x[SCALE_MAX_SEAMS];
  TEST_ASSERT_EQUAL_INT(
      0, scaleTicks(100000, 100000, 100000, true, ROW, t, SCALE_MAX_TICKS));
  TEST_ASSERT_EQUAL_INT(
      0, scaleTicks(FM_HIGH, FM_LOW, 100000, true, ROW, t, SCALE_MAX_TICKS));
  TEST_ASSERT_EQUAL_INT(
      0, scaleTicks(FM_LOW, FM_HIGH, 100000, true, 1, t, SCALE_MAX_TICKS));
  TEST_ASSERT_EQUAL_INT(
      0, scaleTicks(FM_LOW, FM_HIGH, 100000, true, ROW, NULL, 9));
  TEST_ASSERT_EQUAL_INT(0,
                        scaleTicks(FM_LOW, FM_HIGH, 100000, true, ROW, t, 0));
  TEST_ASSERT_EQUAL_INT(
      0, scaleSeams(100000, 100000, 100000, true, ROW, x, SCALE_MAX_SEAMS));
  TEST_ASSERT_EQUAL_INT(
      0, scaleSeams(FM_LOW, FM_HIGH, 108000, true, 1, x, SCALE_MAX_SEAMS));
  TEST_ASSERT_EQUAL_INT(
      0, scaleSeams(FM_LOW, FM_HIGH, 108000, true, ROW, NULL, 2));
  TEST_ASSERT_EQUAL_INT(0,
                        scaleSeams(FM_LOW, FM_HIGH, 108000, true, ROW, x, 0));
}

/* A number past 16 bits is no band this radio has: blank, not wrapped.
 * The ticks are still there and still long every tenth, only unlabelled. */
static void a_number_too_big_for_a_tick_is_left_blank(void) {
  ScaleTick t[SCALE_MAX_TICKS];
  int n = scaleTicks(70000, 70500, 70200, false, ROW, t, SCALE_MAX_TICKS);
  int longInBand = 0;
  for (int i = 0; i < n; i++) {
    TEST_ASSERT_EQUAL_UINT16(0, t[i].number);
    if (t[i].inBand && t[i].mark == SCALE_LONG) {
      longInBand++;
    }
  }
  TEST_ASSERT_TRUE(longInBand > 0);
}

/* Six bytes a tick: the panel keeps a row of them in static RAM. */
static void a_tick_is_six_bytes(void) {
  TEST_ASSERT_EQUAL(6, (int)sizeof(ScaleTick));
}

/* A new level redraws only the marks within the peak's reach, so that reach
 * must hold every mark with a rank, and nothing past it may have one. */
static void the_peak_reach_holds_every_peak_mark(void) {
  const int32_t reach = scalePeakReachPx();
  TEST_ASSERT_EQUAL_INT(SCALE_PEAK_RANKS - 1, scalePeakRank(reach));
  TEST_ASSERT_EQUAL_INT(SCALE_PEAK_RANKS - 1, scalePeakRank(-reach));
  TEST_ASSERT_EQUAL_INT(-1, scalePeakRank(reach + 1));
  TEST_ASSERT_EQUAL_INT(-1, scalePeakRank(-reach - 1));
  for (int32_t dx = -reach - SCALE_TICK_PX; dx <= reach + SCALE_TICK_PX; dx++) {
    if (scalePeakRank(dx) >= 0) {
      TEST_ASSERT_TRUE(dx >= -reach && dx <= reach);
    }
  }
}

/* A band too narrow to hold a tick, 101 to 109 kHz on the 10 kHz grid, has
 * no scale and no loop. */
static void a_band_with_no_tick_has_no_scale(void) {
  ScaleTick t[SCALE_MAX_TICKS];
  int16_t x[SCALE_MAX_SEAMS];
  TEST_ASSERT_EQUAL_INT(
      0, scaleTicks(101, 109, 105, false, ROW, t, SCALE_MAX_TICKS));
  TEST_ASSERT_EQUAL_INT(
      0, scaleSeams(101, 109, 105, false, ROW, x, SCALE_MAX_SEAMS));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(a_number_too_big_for_a_tick_is_left_blank);
  RUN_TEST(a_tick_is_six_bytes);
  RUN_TEST(fm_is_marked_every_100_kilohertz);
  RUN_TEST(mw_is_marked_every_10_kilohertz);
  RUN_TEST(the_two_ends_of_fm_meet);
  RUN_TEST(the_two_ends_of_mw_meet);
  RUN_TEST(the_two_ends_of_sw_are_40_pixels_apart);
  RUN_TEST(every_mark_fits_on_the_row);
  RUN_TEST(the_seam_is_in_the_middle_of_the_dummies);
  RUN_TEST(a_band_shorter_than_the_row_shows_its_seam_twice);
  RUN_TEST(marks_under_the_middle_are_hidden);
  RUN_TEST(the_peak_ranks_run_outwards);
  RUN_TEST(each_side_has_one_mark_of_each_rank);
  RUN_TEST(the_peak_follows_the_signal);
  RUN_TEST(the_ticks_stop_at_the_room_given);
  RUN_TEST(no_band_or_no_row_has_no_scale);
  RUN_TEST(the_peak_reach_holds_every_peak_mark);
  RUN_TEST(a_band_with_no_tick_has_no_scale);
  return UNITY_END();
}
