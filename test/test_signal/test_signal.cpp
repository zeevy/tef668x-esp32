/* Tests for the derived signal figures. Runs on a PC. */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/signal.h"

#include "../test_meter/captures.h"

void setUp(void) {}
void tearDown(void) {}

/* ------------------------------------------------------------------- snr */

static void a_strong_clean_signal_reads_a_high_ratio(void) {
  TEST_ASSERT_TRUE(signalSnrDb(500, 10, true) > 15);
}

static void a_weak_noisy_signal_reads_a_low_ratio(void) {
  TEST_ASSERT_TRUE(signalSnrDb(100, 2000, true) < 0);
}

static void noise_pulls_the_ratio_down(void) {
  TEST_ASSERT_TRUE(signalSnrDb(400, 1500, true) < signalSnrDb(400, 50, true));
}

static void level_pushes_the_ratio_up(void) {
  TEST_ASSERT_TRUE(signalSnrDb(600, 200, true) > signalSnrDb(300, 200, true));
}

static void it_matches_the_reference_where_it_matters(void) {
  /* From the reference firmware's own line, 0.46222375 * level minus
   * 0.082495118 * noise plus 10, with level in dB and noise in per cent.
   * These sit around the point where the bandwidth extension switches on,
   * which is the only decision made on it. */
  struct {
    int16_t level;
    uint16_t usn;
    int8_t want;
  } cases[] = {
      {300, 100, 23},  {400, 100, 27}, {500, 500, 28},
      {200, 1000, 10}, {0, 0, 10},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    TEST_ASSERT_EQUAL_INT8(cases[i].want,
                           signalSnrDb(cases[i].level, cases[i].usn, true));
  }
}

static void the_level_is_clamped_at_both_ends(void) {
  TEST_ASSERT_EQUAL_INT8(signalSnrDb(1200, 0, true),
                         signalSnrDb(3000, 0, true));
  TEST_ASSERT_EQUAL_INT8(signalSnrDb(-200, 0, true),
                         signalSnrDb(-3000, 0, true));
}

static void the_answer_is_clamped_to_what_it_is_stored_in(void) {
  /* A level far past anything real, with no noise, must not wrap the signed
   * byte it comes back in. */
  TEST_ASSERT_TRUE(signalSnrDb(32767, 0, true) > 0);
  TEST_ASSERT_TRUE(signalSnrDb(-32768, 65535, true) < 0);
}

static void nothing_overflows_at_the_extremes(void) {
  /* Every corner, including a noise reading far larger than anything real,
   * which is what an unsigned wrap looks like. */
  (void)signalSnrDb(32767, 65535, true);
  (void)signalSnrDb(-32768, 65535, true);
  (void)signalSnrDb(32767, 0, true);
  (void)signalSnrDb(-32768, 0, true);
  TEST_ASSERT_TRUE(signalSnrDb(1200, 65535, true) < 0);
}

static void am_noise_is_on_a_different_scale(void) {
  /* The AM side puts its noise in the same field on a scale fifty times
   * larger, and the reference divides it before the same line. Read
   * unscaled, every AM reading would look far noisier than it is. */
  TEST_ASSERT_TRUE(signalSnrDb(400, 2500, false) >
                   signalSnrDb(400, 2500, true));
  /* Fifty times the AM figure gives the same answer as the FM one. */
  TEST_ASSERT_EQUAL_INT8(signalSnrDb(400, 50, true),
                         signalSnrDb(400, 2500, false));
}

static void it_matches_the_reference_on_the_am_side(void) {
  /* 0.46222375 * level minus 0.082495118 * (noise / 50), both in whole
   * units, plus 10. Worked from the reference's own AM variant. */
  TEST_ASSERT_EQUAL_INT8(23, signalSnrDb(300, 5000, false));
  TEST_ASSERT_EQUAL_INT8(27, signalSnrDb(400, 5000, false));
}

/* --------------------------------------------------------------- average */

static void the_first_sample_is_the_answer(void) {
  /* Not averaged up to from zero. A band change must not be followed by the
   * reading climbing to where it already is. */
  SignalAverage avg;
  memset(&avg, 0, sizeof(avg));
  TEST_ASSERT_EQUAL_INT16(400, signalAverage(&avg, 400));
}

static void a_steady_reading_stays_where_it_is(void) {
  SignalAverage avg;
  memset(&avg, 0, sizeof(avg));
  signalAverage(&avg, 400);
  for (int i = 0; i < 50; i++) {
    TEST_ASSERT_INT16_WITHIN(1, 400, signalAverage(&avg, 400));
  }
}

static void one_odd_reading_barely_moves_it(void) {
  /* The whole point. A single jump must not switch a feature on, or it
   * switches on and off several times a second. */
  SignalAverage avg;
  memset(&avg, 0, sizeof(avg));
  signalAverage(&avg, 400);
  TEST_ASSERT_TRUE(signalAverage(&avg, 0) > 340);
}

static void it_follows_a_real_change(void) {
  SignalAverage avg;
  memset(&avg, 0, sizeof(avg));
  signalAverage(&avg, 400);
  int16_t last = 400;
  for (int i = 0; i < 60; i++) {
    last = signalAverage(&avg, 100);
  }
  TEST_ASSERT_INT16_WITHIN(5, 100, last);
}

static void resetting_makes_the_next_sample_the_answer_again(void) {
  /* A band change makes every reading before it meaningless. Without this the
   * smoothing carries the old band's numbers for about two seconds, and
   * anything deciding on them decides on the wrong band. */
  SignalAverage avg;
  memset(&avg, 0, sizeof(avg));
  signalAverage(&avg, 600);
  for (int i = 0; i < 5; i++) {
    signalAverage(&avg, 600);
  }
  signalAverageReset(&avg);
  TEST_ASSERT_EQUAL_INT16(100, signalAverage(&avg, 100));

  signalAverageReset(NULL); /* Must not crash. */
}

static void a_null_average_gives_the_sample_back(void) {
  TEST_ASSERT_EQUAL_INT16(123, signalAverage(NULL, 123));
}

/* ------------------------------------------------- the number on the screen */

static void the_first_reading_is_shown_as_it_stands(void) {
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  TEST_ASSERT_EQUAL_INT16(24, signalDisplayLevel(&d, 243, true));
}

static void a_steady_level_never_changes_the_number(void) {
  /* The swing measured on FM 106.40 is about 1.6 dB on the smoothed level.
   * Nothing inside that may move the digit. */
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  TEST_ASSERT_EQUAL_INT16(24, signalDisplayLevel(&d, 240, true));
  const int16_t swing[] = {233, 249, 236, 245, 238, 247, 241, 235, 244};
  for (size_t i = 0; i < sizeof(swing) / sizeof(swing[0]); i++) {
    TEST_ASSERT_EQUAL_INT16(24, signalDisplayLevel(&d, swing[i], true));
  }
}

static void it_takes_a_whole_db_to_move_the_number(void) {
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  signalDisplayLevel(&d, 300, true);
  /* Nine tenths away is not enough. */
  TEST_ASSERT_EQUAL_INT16(30, signalDisplayLevel(&d, 309, true));
  TEST_ASSERT_EQUAL_INT16(30, signalDisplayLevel(&d, 291, true));
  /* A whole dB away is. */
  TEST_ASSERT_EQUAL_INT16(31, signalDisplayLevel(&d, 310, true));
  TEST_ASSERT_EQUAL_INT16(30, signalDisplayLevel(&d, 300, true));
  TEST_ASSERT_EQUAL_INT16(29, signalDisplayLevel(&d, 290, true));
}

static void the_boundary_and_either_side_of_it(void) {
  /* The value on the boundary, one below and one above, as for anything with a
   * threshold in it. */
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  signalDisplayLevel(&d, 400, true);
  TEST_ASSERT_EQUAL_INT16(40, signalDisplayLevel(&d, 409, true));
  memset(&d, 0, sizeof(d));
  signalDisplayLevel(&d, 400, true);
  TEST_ASSERT_EQUAL_INT16(41, signalDisplayLevel(&d, 410, true));
  memset(&d, 0, sizeof(d));
  signalDisplayLevel(&d, 400, true);
  TEST_ASSERT_EQUAL_INT16(41, signalDisplayLevel(&d, 411, true));
}

static void a_negative_level_rounds_the_right_way(void) {
  /* A dead band really does read a little below zero, so the sign has to be
   * taken before the division or minus five tenths becomes zero. */
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  TEST_ASSERT_EQUAL_INT16(-1, signalDisplayLevel(&d, -5, true));
  memset(&d, 0, sizeof(d));
  TEST_ASSERT_EQUAL_INT16(-2, signalDisplayLevel(&d, -21, true));
  memset(&d, 0, sizeof(d));
  signalDisplayLevel(&d, -100, true);
  TEST_ASSERT_EQUAL_INT16(-11, signalDisplayLevel(&d, -110, true));
}

static void a_station_change_waits_for_the_reading_to_catch_up(void) {
  /* The dial and the reading do not move together. The frequency changes as
   * soon as the command is worked through and the reading follows about a
   * tenth of a second later, so there is a moment carrying the new station
   * and the old level. Starting again on that moment latches the station
   * just left, and two stations a dB apart then leave the old number on the
   * screen for good. */
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  signalDisplayLevel(&d, 244, true); /* On a station reading 24.4. */
  TEST_ASSERT_EQUAL_INT16(24, d.shownDb);

  signalDisplayStationChanged(&d);
  /* The old station's level arrives once more, marked as not caught up. */
  TEST_ASSERT_EQUAL_INT16(24, signalDisplayLevel(&d, 244, false));
  /* Now the new station's reading, only half a dB away from the old one. */
  TEST_ASSERT_EQUAL_INT16(25, signalDisplayLevel(&d, 249, true));
}

static void a_station_change_holds_the_old_number_meanwhile(void) {
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  signalDisplayLevel(&d, 300, true);
  signalDisplayStationChanged(&d);
  /* Several rounds before the reading catches up. The screen must not blink
   * or jump about while it waits. */
  for (int i = 0; i < 5; i++) {
    TEST_ASSERT_EQUAL_INT16(30, signalDisplayLevel(&d, 100, false));
  }
  TEST_ASSERT_EQUAL_INT16(10, signalDisplayLevel(&d, 100, true));
}

static void a_station_change_before_any_reading_is_safe(void) {
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  signalDisplayStationChanged(&d);
  TEST_ASSERT_EQUAL_INT16(24, signalDisplayLevel(&d, 243, false));
  TEST_ASSERT_EQUAL_INT16(24, signalDisplayLevel(&d, 243, true));
  signalDisplayStationChanged(NULL); /* Must not crash. */
}

static void a_null_display_still_gives_a_number(void) {
  TEST_ASSERT_EQUAL_INT16(24, signalDisplayLevel(NULL, 243, true));
  TEST_ASSERT_EQUAL_INT16(25, signalDisplayLevel(NULL, 245, true));
}

/* ------------------------------------------------------- formatting */

static void a_level_just_below_zero_keeps_its_sign(void) {
  /* Dividing minus five tenths by ten gives zero, so a level a little below
   * zero reads as 0.0 unless the sign is taken before the value is split. A
   * dead band really does read a little below zero. */
  char out[12];
  signalFormatLevel(-5, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("-0.5", out);
  signalFormatLevel(-9, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("-0.9", out);
  signalFormatLevel(-101, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("-10.1", out);
}

static void an_ordinary_level_reads_as_it_should(void) {
  char out[12];
  signalFormatLevel(0, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("0.0", out);
  signalFormatLevel(475, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("47.5", out);
  signalFormatLevel(1200, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("120.0", out);
}

static void formatting_never_writes_past_the_buffer(void) {
  char out[4];
  memset(out, 'x', sizeof(out));
  signalFormatLevel(-1234, out, sizeof(out));
  TEST_ASSERT_EQUAL_CHAR('\0', out[3]);
  signalFormatLevel(0, NULL, 10); /* Must not crash. */
  signalFormatLevel(0, out, 0);
}

/* The held readings beside the bars. */

static void the_first_reading_is_taken_as_it_stands(void) {
  ReadingHold h;
  readingHoldReset(&h);
  TEST_ASSERT_EQUAL_INT16(50, readingHoldFeed(&h, 54, 10, 15));
}

static void a_move_under_the_band_does_not_change_what_is_shown(void) {
  ReadingHold h;
  readingHoldReset(&h);
  readingHoldFeed(&h, 100, 10, 15);
  /* The average takes a while to reach a new level, so the reading is held
   * long enough for it to get there before the band is judged. */
  int16_t shown = 0;
  for (int i = 0; i < 40; i++) {
    shown = readingHoldFeed(&h, 110, 10, 15);
  }
  TEST_ASSERT_EQUAL_INT16(100, shown);
}

static void a_move_past_the_band_changes_what_is_shown(void) {
  ReadingHold h;
  readingHoldReset(&h);
  readingHoldFeed(&h, 100, 10, 15);
  int16_t shown = 0;
  for (int i = 0; i < 40; i++) {
    shown = readingHoldFeed(&h, 140, 10, 15);
  }
  TEST_ASSERT_EQUAL_INT16(140, shown);
}

/* Rounding a negative towards zero would put the tuning offset a whole
 * kilohertz nearer centre than it is, on every reading below zero. */
static void a_negative_reading_rounds_away_from_zero_at_the_half(void) {
  ReadingHold h;
  readingHoldReset(&h);
  TEST_ASSERT_EQUAL_INT16(-20, readingHoldFeed(&h, -15, 10, 15));
  readingHoldReset(&h);
  TEST_ASSERT_EQUAL_INT16(-10, readingHoldFeed(&h, -14, 10, 15));
  readingHoldReset(&h);
  TEST_ASSERT_EQUAL_INT16(20, readingHoldFeed(&h, 15, 10, 15));
}

static void a_quantum_of_one_leaves_the_value_alone(void) {
  ReadingHold h;
  readingHoldReset(&h);
  TEST_ASSERT_EQUAL_INT16(287, readingHoldFeed(&h, 287, 1, 20));
}

static void resetting_a_hold_takes_the_next_reading_as_it_stands(void) {
  ReadingHold h;
  readingHoldReset(&h);
  for (int i = 0; i < 40; i++) {
    readingHoldFeed(&h, 500, 10, 15);
  }
  readingHoldReset(&h);
  TEST_ASSERT_EQUAL_INT16(100, readingHoldFeed(&h, 100, 10, 15));
}

static void a_null_hold_gives_the_reading_back(void) {
  TEST_ASSERT_EQUAL_INT16(77, readingHoldFeed(NULL, 77, 10, 15));
  readingHoldReset(NULL);
}

/*
 * The recorded readings are why this exists, so they are what it is tested on.
 *
 * The counts are what this code produces replaying the real readings. They
 * are pinned so a change to the average, the rounding or a threshold has to
 * be looked at rather than absorbed. The two cleanly received stations are
 * 101.90 and 106.40; 104.00 and 93.60 are weak and off tune, and their offset
 * genuinely wanders, which is why they are allowed to change more.
 */
typedef struct {
  const char *capture;
  int offset;
  int usn;
} HoldChanges;

static int countChanges(const MeterCaptureSet *set, int which) {
  ReadingHold h;
  readingHoldReset(&h);
  int16_t quantum =
      which == 0 ? READING_OFFSET_QUANTUM_TENTHS : READING_USN_QUANTUM_TENTHS;
  int16_t hyst = which == 0 ? READING_OFFSET_HYSTERESIS_TENTHS
                            : READING_USN_HYSTERESIS_TENTHS;
  int changes = 0;
  int16_t prev = 0;
  for (uint16_t i = 0; i < set->count; i++) {
    int16_t raw = which == 0 ? set->rows[i].off : (int16_t)set->rows[i].usn;
    int16_t shown = readingHoldFeed(&h, raw, quantum, hyst);
    if (i != 0 && shown != prev) {
      changes++;
    }
    prev = shown;
  }
  return changes;
}

static void the_captures_replay_to_the_counts_that_were_measured(void) {
  static const HoldChanges kExpected[] = {
      {"mid-104000-2026-09-15.log", 21, 10},
      {"music-106400-2026-09-15.log", 0, 3},
      {"speech-101900-2026-09-15.log", 5, 2},
      {"weak-93600-2026-09-15.log", 20, 16},
  };
  TEST_ASSERT_EQUAL_INT(METER_CAPTURE_SETS,
                        (int)(sizeof(kExpected) / sizeof(kExpected[0])));
  for (int c = 0; c < METER_CAPTURE_SETS; c++) {
    const MeterCaptureSet *set = &kMeterCaptures[c];
    TEST_ASSERT_EQUAL_STRING(kExpected[c].capture, set->name);
    TEST_ASSERT_EQUAL_INT(kExpected[c].offset, countChanges(set, 0));
    TEST_ASSERT_EQUAL_INT(kExpected[c].usn, countChanges(set, 1));
  }
}

/* Without the average the same thresholds are defeated by a reading that
 * crosses the band again and again, which is why both parts are there. */
static void the_average_is_what_makes_the_hysteresis_work(void) {
  int worstHeld = 0;
  int worstBare = 0;
  for (int c = 0; c < METER_CAPTURE_SETS; c++) {
    const MeterCaptureSet *set = &kMeterCaptures[c];
    int held = countChanges(set, 0);
    if (held > worstHeld) {
      worstHeld = held;
    }
    /* The same rounding and band, fed the raw reading. */
    int16_t shown = 0;
    int bare = 0;
    int16_t prev = 0;
    for (uint16_t i = 0; i < set->count; i++) {
      int16_t raw = set->rows[i].off;
      int32_t away = (int32_t)raw - (int32_t)shown;
      if (away < 0) {
        away = -away;
      }
      if (i == 0 || away >= READING_OFFSET_HYSTERESIS_TENTHS) {
        int32_t half = READING_OFFSET_QUANTUM_TENTHS / 2;
        int32_t adj = raw >= 0 ? raw + half : raw - half;
        shown = (int16_t)((adj / READING_OFFSET_QUANTUM_TENTHS) *
                          READING_OFFSET_QUANTUM_TENTHS);
      }
      if (i != 0 && shown != prev) {
        bare++;
      }
      prev = shown;
    }
    if (bare > worstBare) {
      worstBare = bare;
    }
  }
  TEST_ASSERT_EQUAL_INT(21, worstHeld);
  TEST_ASSERT_EQUAL_INT(240, worstBare);
}

/* Where a level sits on the tuning scale peak and the browser meter. */

static void the_bottom_of_the_scale_is_nothing(void) {
  TEST_ASSERT_EQUAL_UINT8(0, signalBarPercent(0, SIGNAL_FULL_FM_DBUV));
}

/* A dead band reads a little below zero. It is not a signal pointing the
 * other way, so it is the empty bar and not a wrapped percentage. */
static void a_level_below_zero_is_nothing(void) {
  TEST_ASSERT_EQUAL_UINT8(0, signalBarPercent(-1, SIGNAL_FULL_FM_DBUV));
  TEST_ASSERT_EQUAL_UINT8(0, signalBarPercent(-539, SIGNAL_FULL_FM_DBUV));
  TEST_ASSERT_EQUAL_UINT8(0, signalBarPercent(INT16_MIN, SIGNAL_FULL_FM_DBUV));
}

static void the_top_of_the_scale_is_full(void) {
  TEST_ASSERT_EQUAL_UINT8(100, signalBarPercent(600, 60));
}

/* The bar stops at the end of its track rather than running past it. */
static void a_level_past_the_top_is_still_full(void) {
  TEST_ASSERT_EQUAL_UINT8(100, signalBarPercent(1200, 60));
  TEST_ASSERT_EQUAL_UINT8(100, signalBarPercent(INT16_MAX, 60));
}

static void the_value_on_the_boundary_and_either_side(void) {
  TEST_ASSERT_EQUAL_UINT8(99, signalBarPercent(599, 60));
  TEST_ASSERT_EQUAL_UINT8(100, signalBarPercent(600, 60));
  TEST_ASSERT_EQUAL_UINT8(100, signalBarPercent(601, 60));
}

/* A scale of no length answers nothing rather than dividing by it, so a
 * caller that has not been given one draws an empty bar. */
static void a_scale_of_zero_draws_nothing(void) {
  TEST_ASSERT_EQUAL_UINT8(0, signalBarPercent(300, 0));
}

static void the_scale_ends_where_it_is_told(void) {
  /* 30 dBuV against a top of 60 is half the bar, and against 30 it is all of
   * it. The same level reads differently because the scale is what moved. */
  TEST_ASSERT_EQUAL_UINT8(50, signalBarPercent(300, 60));
  TEST_ASSERT_EQUAL_UINT8(100, signalBarPercent(300, 30));
  TEST_ASSERT_EQUAL_UINT8(33, signalBarPercent(300, 90));
}

/*
 * The fixed scale tops against the sweeps they were set from.
 *
 * FM: the six real stations in a seek sweep run 26.7 to 47.5 dBuV.
 * AM: the strongest channel in a night sweep of MW reads 35.4 and the
 * median 14.3. The two tops put the strongest station on each band within one
 * point of the same place, which is what makes one bar readable on two bands
 * that are 12 dB apart.
 */
static void the_scale_tops_put_the_strongest_station_in_the_same_place(void) {
  TEST_ASSERT_EQUAL_UINT8(79, signalBarPercent(475, SIGNAL_FULL_FM_DBUV));
  TEST_ASSERT_EQUAL_UINT8(78, signalBarPercent(354, SIGNAL_FULL_AM_DBUV));
}

/* Against the fixed FM scale top, the weakest real FM station still lights a
 * readable length of bar, and the strongest empty channel in the sweep, a
 * shoulder at 21.8, does not reach where the weakest station sits. */
static void the_fm_scale_top_separates_a_station_from_an_empty_channel(void) {
  TEST_ASSERT_EQUAL_UINT8(44, signalBarPercent(267, SIGNAL_FULL_FM_DBUV));
  TEST_ASSERT_EQUAL_UINT8(36, signalBarPercent(218, SIGNAL_FULL_FM_DBUV));
}

/* ------------------------------------------------------- the wider filter */

/* Both limits are strict, so each is tested on the value, one past it, and
 * with the other limit failing. */
static void the_wide_filter_needs_both_limits_passed(void) {
  TEST_ASSERT_TRUE(signalWantsWideBandwidth(301, 16));
  TEST_ASSERT_FALSE(signalWantsWideBandwidth(300, 16));
  TEST_ASSERT_FALSE(signalWantsWideBandwidth(301, 15));
  TEST_ASSERT_FALSE(signalWantsWideBandwidth(300, 15));
  TEST_ASSERT_TRUE(signalWantsWideBandwidth(700, 40));
  TEST_ASSERT_FALSE(signalWantsWideBandwidth(700, 0));
  TEST_ASSERT_FALSE(signalWantsWideBandwidth(-200, 40));
}

/* A strong clean station opens it, and a weak noisy channel does not. The
 * two readings are made up for the test, but they go through signalSnrDb,
 * so the limits are checked against the ratio they are really compared
 * with rather than against a number typed in. */
static void a_strong_station_opens_the_wide_filter_and_noise_does_not(void) {
  TEST_ASSERT_TRUE(signalWantsWideBandwidth(600, signalSnrDb(600, 20, true)));
  TEST_ASSERT_FALSE(signalWantsWideBandwidth(150, signalSnrDb(150, 400, true)));
}

/* The offset moves what is shown, a tenth of a dB per step of ten, and never
 * wraps. */
static void the_shown_level_is_the_reading_plus_the_offset(void) {
  TEST_ASSERT_EQUAL_INT16(355, signalShownTenths(355, 0));
  TEST_ASSERT_EQUAL_INT16(505,
                          signalShownTenths(355, SIGNAL_LEVEL_OFFSET_MAX_DB));
  TEST_ASSERT_EQUAL_INT16(105,
                          signalShownTenths(355, SIGNAL_LEVEL_OFFSET_MIN_DB));
  TEST_ASSERT_EQUAL_INT16(-255,
                          signalShownTenths(-5, SIGNAL_LEVEL_OFFSET_MIN_DB));
  TEST_ASSERT_EQUAL_INT16(INT16_MAX, signalShownTenths(INT16_MAX - 10, 15));
  TEST_ASSERT_EQUAL_INT16(INT16_MIN, signalShownTenths(INT16_MIN + 10, -25));
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(a_strong_clean_signal_reads_a_high_ratio);
  RUN_TEST(a_weak_noisy_signal_reads_a_low_ratio);
  RUN_TEST(noise_pulls_the_ratio_down);
  RUN_TEST(level_pushes_the_ratio_up);
  RUN_TEST(it_matches_the_reference_where_it_matters);
  RUN_TEST(the_level_is_clamped_at_both_ends);
  RUN_TEST(the_answer_is_clamped_to_what_it_is_stored_in);
  RUN_TEST(nothing_overflows_at_the_extremes);

  RUN_TEST(am_noise_is_on_a_different_scale);
  RUN_TEST(it_matches_the_reference_on_the_am_side);
  RUN_TEST(the_first_sample_is_the_answer);
  RUN_TEST(a_steady_reading_stays_where_it_is);
  RUN_TEST(one_odd_reading_barely_moves_it);
  RUN_TEST(it_follows_a_real_change);
  RUN_TEST(resetting_makes_the_next_sample_the_answer_again);
  RUN_TEST(a_null_average_gives_the_sample_back);

  RUN_TEST(the_first_reading_is_shown_as_it_stands);
  RUN_TEST(a_steady_level_never_changes_the_number);
  RUN_TEST(it_takes_a_whole_db_to_move_the_number);
  RUN_TEST(the_boundary_and_either_side_of_it);
  RUN_TEST(a_negative_level_rounds_the_right_way);
  RUN_TEST(a_station_change_waits_for_the_reading_to_catch_up);
  RUN_TEST(a_station_change_holds_the_old_number_meanwhile);
  RUN_TEST(a_station_change_before_any_reading_is_safe);
  RUN_TEST(a_null_display_still_gives_a_number);

  RUN_TEST(a_level_just_below_zero_keeps_its_sign);
  RUN_TEST(an_ordinary_level_reads_as_it_should);
  RUN_TEST(formatting_never_writes_past_the_buffer);

  RUN_TEST(the_first_reading_is_taken_as_it_stands);
  RUN_TEST(a_move_under_the_band_does_not_change_what_is_shown);
  RUN_TEST(a_move_past_the_band_changes_what_is_shown);
  RUN_TEST(a_negative_reading_rounds_away_from_zero_at_the_half);
  RUN_TEST(a_quantum_of_one_leaves_the_value_alone);
  RUN_TEST(resetting_a_hold_takes_the_next_reading_as_it_stands);
  RUN_TEST(a_null_hold_gives_the_reading_back);
  RUN_TEST(the_captures_replay_to_the_counts_that_were_measured);
  RUN_TEST(the_average_is_what_makes_the_hysteresis_work);

  RUN_TEST(the_bottom_of_the_scale_is_nothing);
  RUN_TEST(a_level_below_zero_is_nothing);
  RUN_TEST(the_top_of_the_scale_is_full);
  RUN_TEST(a_level_past_the_top_is_still_full);
  RUN_TEST(the_value_on_the_boundary_and_either_side);
  RUN_TEST(a_scale_of_zero_draws_nothing);
  RUN_TEST(the_scale_ends_where_it_is_told);
  RUN_TEST(the_scale_tops_put_the_strongest_station_in_the_same_place);
  RUN_TEST(the_fm_scale_top_separates_a_station_from_an_empty_channel);
  RUN_TEST(the_wide_filter_needs_both_limits_passed);
  RUN_TEST(a_strong_station_opens_the_wide_filter_and_noise_does_not);
  RUN_TEST(the_shown_level_is_the_reading_plus_the_offset);

  return UNITY_END();
}
