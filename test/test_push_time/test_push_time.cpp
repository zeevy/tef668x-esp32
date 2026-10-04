/* Tests for the panel push totals. Runs on a PC. */
#include <unity.h>

#include "core/push_time.h"

void setUp(void) {}
void tearDown(void) {}

static PushSecond lastAt(PushTime *p, uint32_t nowMs) {
  PushSecond s = {111, 111, 111, 111, 111};
  TEST_ASSERT_TRUE(pushTimeLast(p, nowMs, &s));
  return s;
}

static void nothing_to_give_before_the_first_second_ends(void) {
  PushTime p;
  pushTimeReset(&p, 5000);
  pushTimeAdd(&p, 5100, 5328, 11400);
  PushSecond s = {111, 111, 111, 111, 111};
  TEST_ASSERT_FALSE(pushTimeLast(&p, 5999, &s));
  TEST_ASSERT_EQUAL_UINT32(111, s.pushes);
}

static void a_second_of_pushes_adds_up(void) {
  PushTime p;
  pushTimeReset(&p, 0);
  pushTimeAdd(&p, 10, 5328, 11400);
  pushTimeAdd(&p, 33, 5328, 11700);
  pushTimeAdd(&p, 500, 320, 900);
  const PushSecond s = lastAt(&p, 1000);
  TEST_ASSERT_EQUAL_UINT32(3, s.pushes);
  TEST_ASSERT_EQUAL_UINT32(10976, s.pixels);
  TEST_ASSERT_EQUAL_UINT32(24000, s.busyUs);
  TEST_ASSERT_EQUAL_UINT32(11700, s.longestUs);
}

static void a_push_on_the_boundary_counts_in_the_next_second(void) {
  PushTime p;
  pushTimeReset(&p, 0);
  pushTimeAdd(&p, 999, 100, 10);
  pushTimeAdd(&p, 1000, 200, 20);
  TEST_ASSERT_EQUAL_UINT32(100, lastAt(&p, 1999).pixels);
  TEST_ASSERT_EQUAL_UINT32(200, lastAt(&p, 2000).pixels);
}

static void a_quiet_second_reads_as_zero(void) {
  PushTime p;
  pushTimeReset(&p, 0);
  pushTimeAdd(&p, 500, 5328, 11400);
  TEST_ASSERT_EQUAL_UINT32(1, lastAt(&p, 1500).pushes);
  const PushSecond s = lastAt(&p, 2000);
  TEST_ASSERT_EQUAL_UINT32(0, s.pushes);
  TEST_ASSERT_EQUAL_UINT32(0, s.busyUs);
  TEST_ASSERT_EQUAL_UINT32(0, s.longestUs);
}

static void seconds_with_no_call_do_not_keep_an_old_busy_one(void) {
  PushTime p;
  pushTimeReset(&p, 0);
  pushTimeAdd(&p, 500, 5328, 11400);
  /* Nothing asked until long after: the busy second is not the last one. */
  TEST_ASSERT_EQUAL_UINT32(0, lastAt(&p, 7300).pushes);
}

static void the_windows_stay_on_whole_seconds_after_a_late_call(void) {
  PushTime p;
  pushTimeReset(&p, 0);
  pushTimeAdd(&p, 3700, 1, 1); /* closes 0 to 3000, counts in 3000 to 4000 */
  pushTimeAdd(&p, 3999, 1, 1);
  pushTimeAdd(&p, 4000, 1, 1);
  TEST_ASSERT_EQUAL_UINT32(2, lastAt(&p, 4999).pushes);
}

static void the_millisecond_clock_wrapping_does_not_break_the_window(void) {
  PushTime p;
  pushTimeReset(&p, 0xFFFFFE00u);
  pushTimeAdd(&p, 0xFFFFFF00u, 10, 10);
  pushTimeAdd(&p, 0x00000100u, 10, 10); /* 0x300 = 768 ms after the start */
  PushSecond s = {111, 111, 111, 111, 111};
  TEST_ASSERT_FALSE(pushTimeLast(&p, 0x00000100u, &s));
  TEST_ASSERT_EQUAL_UINT32(2, lastAt(&p, 0x000001E8u).pushes);
}

static void a_reset_forgets_what_was_counted(void) {
  PushTime p;
  pushTimeReset(&p, 0);
  pushTimeAdd(&p, 10, 5328, 11400);
  TEST_ASSERT_EQUAL_UINT32(1, lastAt(&p, 1000).pushes);
  pushTimeReset(&p, 1000);
  PushSecond s = {111, 111, 111, 111, 111};
  TEST_ASSERT_FALSE(pushTimeLast(&p, 1999, &s));
}

static void a_null_does_nothing(void) {
  PushTime p;
  PushSecond s = {111, 111, 111, 111, 111};
  pushTimeReset(NULL, 0);
  pushTimeAdd(NULL, 0, 1, 1);
  pushTimeReset(&p, 0);
  TEST_ASSERT_FALSE(pushTimeLast(NULL, 1000, &s));
  TEST_ASSERT_FALSE(pushTimeLast(&p, 1000, NULL));
  TEST_ASSERT_EQUAL_UINT32(111, s.pushes);
}

/* The longest refresh of a second, kept apart from the pushes, and zero in
 * a second with none. */
static void the_longest_refresh_of_a_second_is_kept(void) {
  PushTime p;
  pushTimeReset(&p, 0);
  pushTimeRefresh(&p, 10, 900);
  pushTimeRefresh(&p, 20, 41000);
  pushTimeRefresh(&p, 30, 1200);
  pushTimeAdd(&p, 40, 5328, 2900);
  PushSecond s = lastAt(&p, 1000);
  TEST_ASSERT_EQUAL_UINT32(41000, s.longestRefreshUs);
  TEST_ASSERT_EQUAL_UINT32(1, s.pushes);
  TEST_ASSERT_EQUAL_UINT32(2900, s.longestUs);
  s = lastAt(&p, 2000);
  TEST_ASSERT_EQUAL_UINT32(0, s.longestRefreshUs);
  pushTimeRefresh(NULL, 0, 1);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(nothing_to_give_before_the_first_second_ends);
  RUN_TEST(a_second_of_pushes_adds_up);
  RUN_TEST(a_push_on_the_boundary_counts_in_the_next_second);
  RUN_TEST(a_quiet_second_reads_as_zero);
  RUN_TEST(seconds_with_no_call_do_not_keep_an_old_busy_one);
  RUN_TEST(the_windows_stay_on_whole_seconds_after_a_late_call);
  RUN_TEST(the_millisecond_clock_wrapping_does_not_break_the_window);
  RUN_TEST(a_reset_forgets_what_was_counted);
  RUN_TEST(a_null_does_nothing);
  RUN_TEST(the_longest_refresh_of_a_second_is_kept);
  return UNITY_END();
}
