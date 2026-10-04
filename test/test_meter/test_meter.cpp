/* Tests for the meter bar arithmetic. Runs on a PC. */
#include <unity.h>

#include <stdint.h>

#include "core/meter.h"

#include "captures.h"

void setUp(void) {}
void tearDown(void) {}

/* A bar of 38 blocks, the count the lit tests measure against. */
#define BAR_SEGS 38

/* ------------------------------------------------------------------- lit */

static void nothing_is_lit_at_zero(void) {
  TEST_ASSERT_EQUAL_UINT8(0, meterSegmentsLit(0, BAR_SEGS));
}

static void everything_is_lit_at_a_hundred(void) {
  TEST_ASSERT_EQUAL_UINT8(BAR_SEGS, meterSegmentsLit(100, BAR_SEGS));
}

static void a_reading_past_the_top_is_still_a_full_bar(void) {
  TEST_ASSERT_EQUAL_UINT8(BAR_SEGS, meterSegmentsLit(120, BAR_SEGS));
  TEST_ASSERT_EQUAL_UINT8(BAR_SEGS, meterSegmentsLit(255, BAR_SEGS));
}

static void half_a_bar_is_half_the_segments(void) {
  TEST_ASSERT_EQUAL_UINT8(19, meterSegmentsLit(50, BAR_SEGS));
}

/*
 * The case the minimum exists for. Without it a real reading rounds down to
 * an empty bar, and an empty bar on this panel means no reading at all.
 */
static void the_smallest_real_reading_still_lights_one(void) {
  TEST_ASSERT_EQUAL_UINT8(1, meterSegmentsLit(1, BAR_SEGS));
  TEST_ASSERT_EQUAL_UINT8(1, meterSegmentsLit(2, BAR_SEGS));
  TEST_ASSERT_EQUAL_UINT8(1, meterSegmentsLit(3, BAR_SEGS));
}

static void just_under_full_is_not_full(void) {
  TEST_ASSERT_EQUAL_UINT8(37, meterSegmentsLit(99, BAR_SEGS));
}

static void a_bar_with_no_segments_lights_none(void) {
  TEST_ASSERT_EQUAL_UINT8(0, meterSegmentsLit(50, 0));
  TEST_ASSERT_EQUAL_UINT8(0, meterSegmentsLit(100, 0));
}

static void lit_never_runs_past_the_count(void) {
  for (uint16_t pc = 0; pc <= 255; pc++) {
    TEST_ASSERT_TRUE(meterSegmentsLit((uint8_t)pc, BAR_SEGS) <= BAR_SEGS);
  }
}

/* ------------------------------------------------------------------ peak */

#define HOLD_MS 500u
#define FALL_MS 1700u /* IEC 60268-10 Type I, borrowed as a duration. */

static void a_fresh_peak_has_nothing_to_show(void) {
  MeterPeak p;
  meterPeakReset(&p);
  TEST_ASSERT_FALSE(p.valid);
  TEST_ASSERT_EQUAL_UINT8(0, p.percent);
}

static void the_first_reading_sets_the_mark(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 40, 1000, HOLD_MS, FALL_MS);
  TEST_ASSERT_TRUE(p.valid);
  TEST_ASSERT_EQUAL_UINT8(40, p.percent);
}

static void a_louder_reading_takes_the_mark_up_at_once(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 40, 1000, HOLD_MS, FALL_MS);
  meterPeakFeed(&p, 90, 1040, HOLD_MS, FALL_MS);
  TEST_ASSERT_EQUAL_UINT8(90, p.percent);
}

static void the_mark_does_not_move_during_the_hold(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 90, 1000, HOLD_MS, FALL_MS);
  meterPeakFeed(&p, 10, 1400, HOLD_MS, FALL_MS);
  TEST_ASSERT_EQUAL_UINT8(90, p.percent);
}

static void the_mark_starts_falling_once_the_hold_is_over(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 90, 1000, HOLD_MS, FALL_MS);
  meterPeakFeed(&p, 10, 1000 + HOLD_MS + 100, HOLD_MS, FALL_MS);
  TEST_ASSERT_TRUE(p.percent < 90);
}

/*
 * The whole point of timing the fall rather than counting calls. A meter fed
 * every 40 ms and one fed every 10 ms must end up in the same place, because
 * the standard specifies a time.
 */
static void the_fall_follows_the_clock_and_not_the_call_rate(void) {
  MeterPeak slow;
  MeterPeak fast;
  meterPeakReset(&slow);
  meterPeakReset(&fast);
  meterPeakFeed(&slow, 100, 0, 0, FALL_MS);
  meterPeakFeed(&fast, 100, 0, 0, FALL_MS);
  for (uint32_t t = 40; t <= 800; t += 40) {
    meterPeakFeed(&slow, 0, t, 0, FALL_MS);
  }
  for (uint32_t t = 10; t <= 800; t += 10) {
    meterPeakFeed(&fast, 0, t, 0, FALL_MS);
  }
  TEST_ASSERT_EQUAL_UINT8(slow.percent, fast.percent);
}

static void a_full_bar_takes_the_whole_fall_time_to_clear(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 100, 0, 0, FALL_MS);
  /* One step short of the full time still has something left. */
  for (uint32_t t = 10; t < FALL_MS - 20; t += 10) {
    meterPeakFeed(&p, 0, t, 0, FALL_MS);
  }
  TEST_ASSERT_TRUE(p.percent > 0);
  meterPeakFeed(&p, 0, FALL_MS + 10, 0, FALL_MS);
  TEST_ASSERT_EQUAL_UINT8(0, p.percent);
}

static void the_mark_stops_when_it_meets_the_bar(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 100, 0, 0, FALL_MS);
  for (uint32_t t = 10; t <= 4000; t += 10) {
    meterPeakFeed(&p, 30, t, 0, FALL_MS);
  }
  TEST_ASSERT_EQUAL_UINT8(30, p.percent);
}

static void a_zero_fall_time_drops_the_mark_as_soon_as_the_hold_ends(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 90, 1000, HOLD_MS, 0);
  meterPeakFeed(&p, 20, 1000 + HOLD_MS, HOLD_MS, 0);
  TEST_ASSERT_EQUAL_UINT8(20, p.percent);
}

static void a_fall_time_under_a_hundred_ms_still_falls(void) {
  /* Less than one millisecond a per cent, so the step arithmetic would be a
   * divide by zero if it were not guarded. */
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 90, 0, 0, 50);
  meterPeakFeed(&p, 20, 10, 0, 50);
  TEST_ASSERT_EQUAL_UINT8(20, p.percent);
}

static void a_reading_past_the_top_is_held_at_the_top(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 130, 0, HOLD_MS, FALL_MS);
  TEST_ASSERT_EQUAL_UINT8(100, p.percent);
}

static void the_clock_wrapping_does_not_strand_the_mark(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 100, 0xFFFFFF00u, 0, FALL_MS);
  /* Past the wrap, by the whole fall time. */
  meterPeakFeed(&p, 0, 0xFFFFFF00u + FALL_MS + 100u, 0, FALL_MS);
  TEST_ASSERT_EQUAL_UINT8(0, p.percent);
}

static void clearing_forgets_the_mark(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 80, 0, HOLD_MS, FALL_MS);
  meterPeakClear(&p);
  TEST_ASSERT_FALSE(p.valid);
  TEST_ASSERT_EQUAL_UINT8(0, p.percent);
}

/* Fed every 40 ms, a mark at 84 with a 750 ms hold and a 3750 ms fall never
 * drops more than 2 points a step, and nothing at all until the hold has
 * ended. The screen feeds it once per tuner read, which is less often. */
static void the_fall_starts_when_the_hold_ends(void) {
  MeterPeak p;
  meterPeakReset(&p);
  meterPeakFeed(&p, 84, 0, 750, 3750);
  uint8_t last = p.percent;
  for (uint32_t t = 40; t <= 3000; t += 40) {
    meterPeakFeed(&p, 10, t, 750, 3750);
    if (t < 750) {
      TEST_ASSERT_EQUAL_UINT8(84, p.percent);
    }
    TEST_ASSERT_TRUE_MESSAGE(last - p.percent <= 2, "one frame fell too far");
    last = p.percent;
  }
  TEST_ASSERT_TRUE(p.percent < 84);
}

static void a_null_peak_is_safe(void) {
  meterPeakReset(NULL);
  meterPeakClear(NULL);
  meterPeakFeed(NULL, 50, 0, HOLD_MS, FALL_MS);
}

/* The bar: up at once, back slowly. MODULATION_FALL_MS in screen_state.cpp
 * is 1700, so the tests below use that. */
#define BAR_FALL_MS 1700u

static void a_fresh_bar_has_nothing_to_show(void) {
  MeterBar b;
  meterBarReset(&b);
  TEST_ASSERT_FALSE(b.valid);
  TEST_ASSERT_EQUAL_UINT16(0, b.percent);
}

static void the_first_reading_sets_the_bar(void) {
  MeterBar b;
  meterBarReset(&b);
  TEST_ASSERT_EQUAL_UINT16(40, meterBarFeed(&b, 40, 1000, BAR_FALL_MS));
  TEST_ASSERT_TRUE(b.valid);
}

static void a_louder_reading_takes_the_bar_up_at_once(void) {
  MeterBar b;
  meterBarReset(&b);
  meterBarFeed(&b, 20, 1000, BAR_FALL_MS);
  TEST_ASSERT_EQUAL_UINT16(90, meterBarFeed(&b, 90, 1100, BAR_FALL_MS));
}

static void the_bar_falls_at_the_rate_it_was_given(void) {
  MeterBar b;
  meterBarReset(&b);
  meterBarFeed(&b, 100, 0, BAR_FALL_MS);
  /* One per cent every 17 ms at this fall time. */
  TEST_ASSERT_EQUAL_UINT16(100, meterBarFeed(&b, 0, 16, BAR_FALL_MS));
  TEST_ASSERT_EQUAL_UINT16(99, meterBarFeed(&b, 0, 17, BAR_FALL_MS));
  TEST_ASSERT_EQUAL_UINT16(90, meterBarFeed(&b, 0, 170, BAR_FALL_MS));
}

static void the_bar_takes_the_whole_fall_time_to_clear(void) {
  MeterBar b;
  meterBarReset(&b);
  meterBarFeed(&b, 100, 0, BAR_FALL_MS);
  TEST_ASSERT_EQUAL_UINT16(0, meterBarFeed(&b, 0, BAR_FALL_MS, BAR_FALL_MS));
}

/* The fall is worked out from the clock, so a caller polling faster than one
 * per cent of the bar must not stall it. */
static void the_bar_fall_follows_the_clock_and_not_the_call_rate(void) {
  MeterBar slow;
  MeterBar fast;
  meterBarReset(&slow);
  meterBarReset(&fast);
  meterBarFeed(&slow, 100, 0, BAR_FALL_MS);
  meterBarFeed(&fast, 100, 0, BAR_FALL_MS);
  meterBarFeed(&slow, 0, 850, BAR_FALL_MS);
  for (uint32_t t = 10; t <= 850; t += 10) {
    meterBarFeed(&fast, 0, t, BAR_FALL_MS);
  }
  TEST_ASSERT_EQUAL_UINT16(slow.percent, fast.percent);
}

static void the_bar_stops_when_it_meets_the_reading(void) {
  MeterBar b;
  meterBarReset(&b);
  meterBarFeed(&b, 80, 0, BAR_FALL_MS);
  TEST_ASSERT_EQUAL_UINT16(60, meterBarFeed(&b, 60, 100000, BAR_FALL_MS));
}

static void a_zero_fall_time_follows_the_reading_down(void) {
  MeterBar b;
  meterBarReset(&b);
  meterBarFeed(&b, 80, 0, 0);
  TEST_ASSERT_EQUAL_UINT16(10, meterBarFeed(&b, 10, 1, 0));
}

static void a_bar_fall_time_under_a_hundred_ms_still_falls(void) {
  MeterBar b;
  meterBarReset(&b);
  meterBarFeed(&b, 80, 0, 50);
  TEST_ASSERT_EQUAL_UINT16(10, meterBarFeed(&b, 10, 1, 50));
}

/* Modulation past reference deviation is real, so the core carries values
 * over 100 and the caller caps them. The caller draws at most a full bar. */
static void a_reading_past_a_hundred_is_carried_not_capped(void) {
  MeterBar b;
  meterBarReset(&b);
  TEST_ASSERT_EQUAL_UINT16(153, meterBarFeed(&b, 153, 0, BAR_FALL_MS));
}

/* It falls at the same rate above a hundred as below it. */
static void a_reading_past_a_hundred_falls_at_the_same_rate(void) {
  MeterBar b;
  meterBarReset(&b);
  meterBarFeed(&b, 150, 0, BAR_FALL_MS);
  TEST_ASSERT_EQUAL_UINT16(140, meterBarFeed(&b, 0, 170, BAR_FALL_MS));
}

static void the_clock_wrapping_does_not_strand_the_bar(void) {
  MeterBar b;
  meterBarReset(&b);
  meterBarFeed(&b, 100, 0xFFFFFF00u, BAR_FALL_MS);
  /* 511 ms later, through the wrap, which is 30 steps of 17 ms. */
  TEST_ASSERT_EQUAL_UINT16(70, meterBarFeed(&b, 0, 0x000000FFu, BAR_FALL_MS));
}

static void clearing_forgets_the_bar(void) {
  MeterBar b;
  meterBarReset(&b);
  meterBarFeed(&b, 70, 0, BAR_FALL_MS);
  meterBarClear(&b);
  TEST_ASSERT_FALSE(b.valid);
  TEST_ASSERT_EQUAL_UINT16(0, b.percent);
}

static void a_null_bar_is_safe(void) {
  TEST_ASSERT_EQUAL_UINT16(50, meterBarFeed(NULL, 50, 0, BAR_FALL_MS));
  TEST_ASSERT_EQUAL_UINT16(200, meterBarFeed(NULL, 200, 0, BAR_FALL_MS));
  meterBarReset(NULL);
  meterBarClear(NULL);
}

/*
 * On every recording, the damped bar moves less in total than the raw
 * reading does.
 *
 * Replayed at the timestamps the radio recorded, not at an assumed spacing.
 * The reading moves a mean of 17.0 and 17.6 points a frame on the two
 * programme recordings.
 */
static void the_bar_is_calmer_than_the_reading_on_every_capture(void) {
  for (int c = 0; c < METER_CAPTURE_SETS; c++) {
    const MeterCaptureSet *set = &kMeterCaptures[c];
    MeterBar b;
    meterBarReset(&b);
    uint32_t rawMove = 0;
    uint32_t barMove = 0;
    int last = -1;
    int lastBar = -1;
    for (uint16_t i = 0; i < set->count; i++) {
      int mod = set->rows[i].mod;
      if (mod < 0) {
        mod = 0;
      }
      if (mod > 100) {
        mod = 100;
      }
      uint8_t bar =
          meterBarFeed(&b, (uint16_t)mod, set->rows[i].ms, BAR_FALL_MS);
      if (last >= 0) {
        rawMove += (uint32_t)(mod > last ? mod - last : last - mod);
        barMove += (uint32_t)(bar > lastBar ? bar - lastBar : lastBar - bar);
      }
      last = mod;
      lastBar = (int)bar;
    }
    TEST_ASSERT_TRUE(barMove <= rawMove);
  }
}

/* The bar must never show more than the radio measured, at any moment of any
 * recording. A bar reading high is a bar claiming a level nothing sent. */
static void the_bar_never_shows_more_than_the_reading_peak(void) {
  for (int c = 0; c < METER_CAPTURE_SETS; c++) {
    const MeterCaptureSet *set = &kMeterCaptures[c];
    MeterBar b;
    meterBarReset(&b);
    uint16_t seen = 0;
    for (uint16_t i = 0; i < set->count; i++) {
      int mod = set->rows[i].mod;
      if (mod < 0) {
        mod = 0;
      }
      if (mod > 100) {
        mod = 100;
      }
      if ((uint16_t)mod > seen) {
        seen = (uint16_t)mod;
      }
      uint8_t bar =
          meterBarFeed(&b, (uint16_t)mod, set->rows[i].ms, BAR_FALL_MS);
      TEST_ASSERT_LESS_OR_EQUAL_UINT16(seen, bar);
    }
  }
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(nothing_is_lit_at_zero);
  RUN_TEST(everything_is_lit_at_a_hundred);
  RUN_TEST(a_reading_past_the_top_is_still_a_full_bar);
  RUN_TEST(half_a_bar_is_half_the_segments);
  RUN_TEST(the_smallest_real_reading_still_lights_one);
  RUN_TEST(just_under_full_is_not_full);
  RUN_TEST(a_bar_with_no_segments_lights_none);
  RUN_TEST(lit_never_runs_past_the_count);

  RUN_TEST(a_fresh_peak_has_nothing_to_show);
  RUN_TEST(the_first_reading_sets_the_mark);
  RUN_TEST(a_louder_reading_takes_the_mark_up_at_once);
  RUN_TEST(the_mark_does_not_move_during_the_hold);
  RUN_TEST(the_mark_starts_falling_once_the_hold_is_over);
  RUN_TEST(the_fall_follows_the_clock_and_not_the_call_rate);
  RUN_TEST(a_full_bar_takes_the_whole_fall_time_to_clear);
  RUN_TEST(the_mark_stops_when_it_meets_the_bar);
  RUN_TEST(a_zero_fall_time_drops_the_mark_as_soon_as_the_hold_ends);
  RUN_TEST(a_fall_time_under_a_hundred_ms_still_falls);
  RUN_TEST(a_reading_past_the_top_is_held_at_the_top);
  RUN_TEST(the_clock_wrapping_does_not_strand_the_mark);
  RUN_TEST(clearing_forgets_the_mark);
  RUN_TEST(the_fall_starts_when_the_hold_ends);
  RUN_TEST(a_null_peak_is_safe);

  RUN_TEST(a_fresh_bar_has_nothing_to_show);
  RUN_TEST(the_first_reading_sets_the_bar);
  RUN_TEST(a_louder_reading_takes_the_bar_up_at_once);
  RUN_TEST(the_bar_falls_at_the_rate_it_was_given);
  RUN_TEST(the_bar_takes_the_whole_fall_time_to_clear);
  RUN_TEST(the_bar_fall_follows_the_clock_and_not_the_call_rate);
  RUN_TEST(the_bar_stops_when_it_meets_the_reading);
  RUN_TEST(a_zero_fall_time_follows_the_reading_down);
  RUN_TEST(a_bar_fall_time_under_a_hundred_ms_still_falls);
  RUN_TEST(a_reading_past_a_hundred_is_carried_not_capped);
  RUN_TEST(a_reading_past_a_hundred_falls_at_the_same_rate);
  RUN_TEST(the_clock_wrapping_does_not_strand_the_bar);
  RUN_TEST(clearing_forgets_the_bar);
  RUN_TEST(a_null_bar_is_safe);
  RUN_TEST(the_bar_is_calmer_than_the_reading_on_every_capture);
  RUN_TEST(the_bar_never_shows_more_than_the_reading_peak);

  return UNITY_END();
}
