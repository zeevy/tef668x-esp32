/*
 * Tests for when the radio task's next round comes. Runs on a PC.
 */
#include <unity.h>

#include <string.h>

#include "core/radio_round.h"

void setUp(void) {}
void tearDown(void) {}

/* A round with nothing running, the next poll 100 ticks away. */
static RadioRoundTimes quiet(void) {
  RadioRoundTimes t;
  memset(&t, 0, sizeof(t));
  t.now = 1000;
  t.nextPoll = 1100;
  t.step = 20;
  t.nextRds = 1031;
  return t;
}

static void a_quiet_round_waits_for_the_poll(void) {
  RadioRoundTimes t = quiet();
  TEST_ASSERT_EQUAL_UINT32(100, radioRoundWait(&t));
}

static void a_late_poll_waits_for_nothing(void) {
  RadioRoundTimes t = quiet();
  t.nextPoll = 990;
  TEST_ASSERT_EQUAL_UINT32(0, radioRoundWait(&t));
}

static void a_fade_comes_back_within_a_step(void) {
  RadioRoundTimes t = quiet();
  t.stepping = true;
  TEST_ASSERT_EQUAL_UINT32(20, radioRoundWait(&t));
  /* And a poll sooner than the step still wins. */
  t.nextPoll = 1005;
  TEST_ASSERT_EQUAL_UINT32(5, radioRoundWait(&t));
}

static void a_seek_does_not_wait(void) {
  RadioRoundTimes t = quiet();
  t.seeking = true;
  TEST_ASSERT_EQUAL_UINT32(0, radioRoundWait(&t));
  TEST_ASSERT_EQUAL_UINT32(0, radioRoundWait(NULL));
}

static void an_af_check_due_sooner_wins(void) {
  RadioRoundTimes t = quiet();
  t.afWaiting = true;
  t.afIn = 40;
  TEST_ASSERT_EQUAL_UINT32(40, radioRoundWait(&t));
  t.afIn = 0;
  TEST_ASSERT_EQUAL_UINT32(0, radioRoundWait(&t));
}

static void the_rds_read_due_sooner_wins(void) {
  RadioRoundTimes t = quiet();
  t.rds = true;
  TEST_ASSERT_EQUAL_UINT32(31, radioRoundWait(&t));
  t.nextRds = 999;
  TEST_ASSERT_EQUAL_UINT32(0, radioRoundWait(&t));
}

static void rds_not_read_does_not_shorten_the_wait(void) {
  RadioRoundTimes t = quiet();
  t.nextRds = 999;
  TEST_ASSERT_EQUAL_UINT32(100, radioRoundWait(&t));
}

static void the_wait_holds_across_the_tick_wrap(void) {
  RadioRoundTimes t = quiet();
  t.now = 0xFFFFFFF0u;
  t.nextPoll = 0x10u;
  t.rds = true;
  t.nextRds = 0x40u;
  TEST_ASSERT_EQUAL_UINT32(0x20, radioRoundWait(&t));
  t.nextPoll = 0xFFFFFFE0u;
  TEST_ASSERT_EQUAL_UINT32(0, radioRoundWait(&t));
}

static void a_deadline_comes_at_its_tick_and_after(void) {
  TEST_ASSERT_FALSE(radioRoundDue(999, 1000));
  TEST_ASSERT_TRUE(radioRoundDue(1000, 1000));
  TEST_ASSERT_TRUE(radioRoundDue(1001, 1000));
  TEST_ASSERT_TRUE(radioRoundDue(0x5u, 0xFFFFFFF0u));
  TEST_ASSERT_FALSE(radioRoundDue(0xFFFFFFF0u, 0x5u));
}

static void the_next_deadline_is_a_period_on(void) {
  TEST_ASSERT_EQUAL_UINT32(1100, radioRoundNext(1000, 100, 1010));
}

static void a_late_round_starts_again_from_now(void) {
  TEST_ASSERT_EQUAL_UINT32(1350, radioRoundNext(1000, 100, 1250));
  /* Exactly due counts as late, so it does not fire again at once. */
  TEST_ASSERT_EQUAL_UINT32(1200, radioRoundNext(1000, 100, 1100));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(a_quiet_round_waits_for_the_poll);
  RUN_TEST(a_late_poll_waits_for_nothing);
  RUN_TEST(a_fade_comes_back_within_a_step);
  RUN_TEST(a_seek_does_not_wait);
  RUN_TEST(an_af_check_due_sooner_wins);
  RUN_TEST(the_rds_read_due_sooner_wins);
  RUN_TEST(rds_not_read_does_not_shorten_the_wait);
  RUN_TEST(the_wait_holds_across_the_tick_wrap);
  RUN_TEST(a_deadline_comes_at_its_tick_and_after);
  RUN_TEST(the_next_deadline_is_a_period_on);
  RUN_TEST(a_late_round_starts_again_from_now);
  return UNITY_END();
}
