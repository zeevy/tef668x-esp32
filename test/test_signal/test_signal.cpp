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

  return UNITY_END();
}
