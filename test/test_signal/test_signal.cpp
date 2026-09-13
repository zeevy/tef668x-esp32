/**
 * @file test_signal.cpp
 * @brief Tests for the derived signal figures. Runs on a PC.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/signal.h"

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
   * which is the only thing that reads this. */
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

/* --------------------------------------------------------------- average */

static void am_noise_is_on_a_different_scale(void) {
  /* The AM side puts its noise in the same field on a scale fifty times
   * larger, and the reference divides it before the same line. Reading it
   * unscaled made every AM reading look far noisier than it is. */
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

/* ------------------------------------------------------- formatting */

/* ------------------------------------------------- the number on the screen */

static void the_first_reading_is_shown_as_it_stands(void) {
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  TEST_ASSERT_EQUAL_INT16(24, signalDisplayLevel(&d, 243, true));
}

static void a_steady_level_never_changes_the_number(void) {
  /* The swing measured on FM 106.40 on 13 September 2026 was about 1.6 dB on
   * the smoothed level. Nothing inside that may move the digit. */
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
  /* The value on the boundary, one below and one above, which is the rule in
   * RULES.md for anything with a threshold in it. */
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

static void resetting_takes_the_next_reading_as_it_stands(void) {
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  signalDisplayLevel(&d, 500, true);
  /* Half a dB away, so without the reset it would still show 50. */
  TEST_ASSERT_EQUAL_INT16(50, signalDisplayLevel(&d, 505, true));
  signalDisplayReset(&d);
  TEST_ASSERT_EQUAL_INT16(51, signalDisplayLevel(&d, 505, true));
  signalDisplayReset(NULL); /* Must not crash. */
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

static void resetting_clears_a_pending_station_change(void) {
  SignalDisplay d;
  memset(&d, 0, sizeof(d));
  signalDisplayLevel(&d, 300, true);
  signalDisplayStationChanged(&d);
  signalDisplayReset(&d);
  /* No longer waiting, so a reading marked stale is taken as it stands. */
  TEST_ASSERT_EQUAL_INT16(10, signalDisplayLevel(&d, 100, false));
}

static void a_null_display_still_gives_a_number(void) {
  TEST_ASSERT_EQUAL_INT16(24, signalDisplayLevel(NULL, 243, true));
  TEST_ASSERT_EQUAL_INT16(25, signalDisplayLevel(NULL, 245, true));
}

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
  RUN_TEST(resetting_takes_the_next_reading_as_it_stands);
  RUN_TEST(a_station_change_waits_for_the_reading_to_catch_up);
  RUN_TEST(a_station_change_holds_the_old_number_meanwhile);
  RUN_TEST(a_station_change_before_any_reading_is_safe);
  RUN_TEST(resetting_clears_a_pending_station_change);
  RUN_TEST(a_null_display_still_gives_a_number);

  RUN_TEST(a_level_just_below_zero_keeps_its_sign);
  RUN_TEST(an_ordinary_level_reads_as_it_should);
  RUN_TEST(formatting_never_writes_past_the_buffer);

  return UNITY_END();
}
