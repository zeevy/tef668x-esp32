/* Tests for the CPU busy share. Runs on a PC. */
#include <unity.h>

#include "core/cpu_load.h"

void setUp(void) {}
void tearDown(void) {}

static uint8_t busyOf(uint32_t idleBefore, uint32_t clockBefore,
                      uint32_t idleNow, uint32_t clockNow) {
  const CpuSample before = {idleBefore, clockBefore};
  const CpuSample now = {idleNow, clockNow};
  uint8_t pct = 222;
  TEST_ASSERT_TRUE(cpuBusyPercent(&before, &now, &pct));
  return pct;
}

static void a_core_idle_the_whole_second_is_zero(void) {
  TEST_ASSERT_EQUAL_UINT8(0, busyOf(5000, 10000, 1005000, 1010000));
}

static void a_core_never_idle_is_a_hundred(void) {
  TEST_ASSERT_EQUAL_UINT8(100, busyOf(5000, 10000, 5000, 1010000));
}

static void a_core_idle_for_a_quarter_is_seventy_five(void) {
  TEST_ASSERT_EQUAL_UINT8(75, busyOf(0, 0, 250000, 1000000));
}

static void the_half_per_cent_rounds_up_and_just_under_rounds_down(void) {
  /* 12.5 % busy rounds to 13, 12.4999 % to 12. */
  TEST_ASSERT_EQUAL_UINT8(13, busyOf(0, 0, 875000, 1000000));
  TEST_ASSERT_EQUAL_UINT8(12, busyOf(0, 0, 875001, 1000000));
}

static void a_counter_that_wrapped_still_gives_the_share(void) {
  /* Both counters pass 2^32 inside the window: 1 s elapsed, 0.6 s idle. */
  TEST_ASSERT_EQUAL_UINT8(
      40, busyOf(0xFFFF0000u, 0xFFFE0000u, 0xFFFF0000u + 600000u,
                 0xFFFE0000u + 1000000u));
}

static void idle_past_the_window_reads_as_zero_not_past_the_end(void) {
  TEST_ASSERT_EQUAL_UINT8(0, busyOf(0, 0, 1000001, 1000000));
  TEST_ASSERT_EQUAL_UINT8(0, busyOf(0, 0, 1000000, 1000000));
  TEST_ASSERT_EQUAL_UINT8(1, busyOf(0, 0, 990000, 1000000));
}

static void a_long_window_does_not_overflow(void) {
  /* An hour, all of it busy: busy times 100 is past 2^32. */
  TEST_ASSERT_EQUAL_UINT8(100, busyOf(0, 0, 0, 3600000000u));
  TEST_ASSERT_EQUAL_UINT8(50, busyOf(0, 0, 1800000000u, 3600000000u));
}

static void no_time_passed_gives_no_share(void) {
  const CpuSample s = {100, 200};
  uint8_t pct = 222;
  TEST_ASSERT_FALSE(cpuBusyPercent(&s, &s, &pct));
  TEST_ASSERT_EQUAL_UINT8(222, pct);
}

static void a_null_gives_no_share(void) {
  const CpuSample a = {0, 0};
  const CpuSample b = {0, 1000};
  uint8_t pct = 222;
  TEST_ASSERT_FALSE(cpuBusyPercent(NULL, &b, &pct));
  TEST_ASSERT_FALSE(cpuBusyPercent(&a, NULL, &pct));
  TEST_ASSERT_FALSE(cpuBusyPercent(&a, &b, NULL));
  TEST_ASSERT_EQUAL_UINT8(222, pct);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(a_core_idle_the_whole_second_is_zero);
  RUN_TEST(a_core_never_idle_is_a_hundred);
  RUN_TEST(a_core_idle_for_a_quarter_is_seventy_five);
  RUN_TEST(the_half_per_cent_rounds_up_and_just_under_rounds_down);
  RUN_TEST(a_counter_that_wrapped_still_gives_the_share);
  RUN_TEST(idle_past_the_window_reads_as_zero_not_past_the_end);
  RUN_TEST(a_long_window_does_not_overflow);
  RUN_TEST(no_time_passed_gives_no_share);
  RUN_TEST(a_null_gives_no_share);

  return UNITY_END();
}
