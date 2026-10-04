/*
 * Tests for auto off. Runs on a PC.
 *
 * All of it is timing. A radio that sleeps a minute early, or never, looks
 * the same as one that works until somebody waits the whole time out.
 */
#include <unity.h>

#include <stdint.h>

#include "core/auto_off.h"

void setUp(void) {}
void tearDown(void) {}

#define MIN_MS 60000UL

static void any_minute_up_to_ten_hours_is_taken(void) {
  TEST_ASSERT_TRUE(autoOffMinutesOk(0));
  TEST_ASSERT_TRUE(autoOffMinutesOk(1));
  TEST_ASSERT_TRUE(autoOffMinutesOk(7));
  TEST_ASSERT_TRUE(autoOffMinutesOk(600));
  TEST_ASSERT_FALSE(autoOffMinutesOk(601));
  TEST_ASSERT_FALSE(autoOffMinutesOk(65535));
}

/* One minute, the shortest: amber from the start, since five minutes of
 * warning is longer than the whole time, and the fade for the last half. */
static void one_minute_is_amber_at_once_and_fades_for_half(void) {
  AutoOff a;
  autoOffInit(&a, 1, 0);
  TEST_ASSERT_EQUAL(AUTO_OFF_WARN, autoOffPhase(&a, 0));
  TEST_ASSERT_EQUAL(AUTO_OFF_WARN, autoOffPhase(&a, 29999));
  TEST_ASSERT_EQUAL(AUTO_OFF_FADE, autoOffPhase(&a, 30000));
  TEST_ASSERT_EQUAL(AUTO_OFF_SLEEP, autoOffPhase(&a, MIN_MS));
}

/* Ten hours, the longest, in milliseconds without running out of room. */
static void ten_hours_counts_all_the_way(void) {
  AutoOff a;
  autoOffInit(&a, 600, 0);
  uint32_t left = 0;
  TEST_ASSERT_TRUE(autoOffLeftMs(&a, 0, &left));
  TEST_ASSERT_EQUAL_UINT32(600 * MIN_MS, left);
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(&a, 594 * MIN_MS));
  TEST_ASSERT_EQUAL(AUTO_OFF_SLEEP, autoOffPhase(&a, 600 * MIN_MS));
}

static void off_never_sleeps_and_has_no_time_left(void) {
  AutoOff a;
  autoOffInit(&a, 0, 0);
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(&a, 0));
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(&a, 500 * MIN_MS));
  uint32_t left = 123;
  TEST_ASSERT_FALSE(autoOffLeftMs(&a, 10, &left));
  TEST_ASSERT_EQUAL_UINT32(123, left);
}

static void the_phases_come_in_order_at_the_right_times(void) {
  AutoOff a;
  autoOffInit(&a, 15, 1000);
  const uint32_t end = 1000 + 15 * MIN_MS;
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(&a, 1000));
  TEST_ASSERT_EQUAL_UINT32(5 * MIN_MS, AUTO_OFF_WARN_MS);
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(&a, end - 5 * MIN_MS - 1));
  TEST_ASSERT_EQUAL(AUTO_OFF_WARN, autoOffPhase(&a, end - 5 * MIN_MS));
  TEST_ASSERT_EQUAL(AUTO_OFF_WARN, autoOffPhase(&a, end - 30001));
  TEST_ASSERT_EQUAL(AUTO_OFF_FADE, autoOffPhase(&a, end - 30000));
  TEST_ASSERT_EQUAL(AUTO_OFF_FADE, autoOffPhase(&a, end - 1));
  TEST_ASSERT_EQUAL(AUTO_OFF_SLEEP, autoOffPhase(&a, end));
  TEST_ASSERT_EQUAL(AUTO_OFF_SLEEP, autoOffPhase(&a, end + 5 * MIN_MS));
}

static void the_time_left_counts_down_to_zero(void) {
  AutoOff a;
  autoOffInit(&a, 120, 0);
  uint32_t left = 0;
  TEST_ASSERT_TRUE(autoOffLeftMs(&a, 0, &left));
  TEST_ASSERT_EQUAL_UINT32(120 * MIN_MS, left);
  TEST_ASSERT_TRUE(autoOffLeftMs(&a, 90 * MIN_MS, &left));
  TEST_ASSERT_EQUAL_UINT32(30 * MIN_MS, left);
  TEST_ASSERT_TRUE(autoOffLeftMs(&a, 200 * MIN_MS, &left));
  TEST_ASSERT_EQUAL_UINT32(0, left);
}

static void use_starts_the_count_again_even_while_fading(void) {
  AutoOff a;
  autoOffInit(&a, 30, 0);
  const uint32_t fading = 30 * MIN_MS - 10000;
  TEST_ASSERT_EQUAL(AUTO_OFF_FADE, autoOffPhase(&a, fading));
  autoOffUsed(&a, fading);
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(&a, fading));
  TEST_ASSERT_EQUAL(AUTO_OFF_SLEEP, autoOffPhase(&a, fading + 30 * MIN_MS));
}

static void a_new_time_starts_the_count_again(void) {
  AutoOff a;
  autoOffInit(&a, 60, 0);
  /* Forty minutes alone, then 15 chosen: not asleep at once, 15 from now. */
  autoOffSetMinutes(&a, 15, 40 * MIN_MS);
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(&a, 40 * MIN_MS));
  TEST_ASSERT_EQUAL(AUTO_OFF_SLEEP, autoOffPhase(&a, 55 * MIN_MS));
  autoOffSetMinutes(&a, 0, 55 * MIN_MS);
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(&a, 500 * MIN_MS));
}

static void a_time_nobody_can_choose_is_off(void) {
  AutoOff a;
  autoOffInit(&a, 601, 0);
  TEST_ASSERT_EQUAL_UINT16(0, a.minutes);
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(&a, 500 * MIN_MS));
}

static void the_millisecond_counter_may_wrap(void) {
  AutoOff a;
  const uint32_t start = 0xFFFFFFFFUL - 5 * MIN_MS;
  autoOffInit(&a, 15, start);
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(&a, start + 9 * MIN_MS));
  TEST_ASSERT_EQUAL(AUTO_OFF_WARN, autoOffPhase(&a, start + 10 * MIN_MS));
  TEST_ASSERT_EQUAL(AUTO_OFF_SLEEP, autoOffPhase(&a, start + 15 * MIN_MS));
}

static void null_is_safe_everywhere(void) {
  uint32_t left = 7;
  autoOffInit(NULL, 15, 0);
  autoOffSetMinutes(NULL, 15, 0);
  autoOffUsed(NULL, 0);
  TEST_ASSERT_EQUAL(AUTO_OFF_AWAKE, autoOffPhase(NULL, 0));
  TEST_ASSERT_FALSE(autoOffLeftMs(NULL, 0, &left));
  AutoOff a;
  autoOffInit(&a, 15, 0);
  TEST_ASSERT_TRUE(autoOffLeftMs(&a, 0, NULL));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(any_minute_up_to_ten_hours_is_taken);
  RUN_TEST(one_minute_is_amber_at_once_and_fades_for_half);
  RUN_TEST(ten_hours_counts_all_the_way);
  RUN_TEST(off_never_sleeps_and_has_no_time_left);
  RUN_TEST(the_phases_come_in_order_at_the_right_times);
  RUN_TEST(the_time_left_counts_down_to_zero);
  RUN_TEST(use_starts_the_count_again_even_while_fading);
  RUN_TEST(a_new_time_starts_the_count_again);
  RUN_TEST(a_time_nobody_can_choose_is_off);
  RUN_TEST(the_millisecond_counter_may_wrap);
  RUN_TEST(null_is_safe_everywhere);
  return UNITY_END();
}
