/* Tests for when the battery is read again with Wi-Fi paused. Runs on a PC. */
#include <unity.h>

#include <stdint.h>

#include "core/battery_check.h"

static BatteryCheckInput in;

/* Started at 1000 ms; everything that allows a reading, one interval on. */
void setUp(void) {
  in = BatteryCheckInput{};
  in.lastTryMs = 1000;
  in.nowMs = 1000 + BATTERY_CHECK_EVERY_MS;
  in.fitted = true;
  in.wifiRunning = true;
  in.lastInputMs = 1000;
}
void tearDown(void) {}

static void due_one_interval_after_start_and_not_a_moment_before(void) {
  TEST_ASSERT_TRUE(batteryCheckDue(&in));
  in.nowMs--;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
}

static void after_a_try_the_next_is_a_full_interval_away(void) {
  in.lastTryMs = in.nowMs;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
  in.nowMs += BATTERY_CHECK_EVERY_MS - 1;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
  in.nowMs++;
  TEST_ASSERT_TRUE(batteryCheckDue(&in));
}

static void unfitted_or_without_wifi_it_never_pauses(void) {
  in.fitted = false;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
  in.fitted = true;
  /* With Wi-Fi off the battery is read every second already. */
  in.wifiRunning = false;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
  TEST_ASSERT_FALSE(batteryCheckDue(NULL));
}

static void a_key_or_knob_in_the_last_five_minutes_holds_it(void) {
  in.lastInputMs = in.nowMs - BATTERY_CHECK_IDLE_MS + 1;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
  in.lastInputMs = in.nowMs - BATTERY_CHECK_IDLE_MS;
  TEST_ASSERT_TRUE(batteryCheckDue(&in));
}

static void the_panel_dimmed_to_nothing_counts_as_idle(void) {
  in.lastInputMs = in.nowMs;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
  in.screenOff = true;
  TEST_ASSERT_TRUE(batteryCheckDue(&in));
}

static void anyone_on_the_network_holds_it(void) {
  in.livePages = 1;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
  in.livePages = 0;
  /* A PC on the PC Link, or a phone on the radio's own hotspot. */
  in.clients = 1;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
  in.clients = 0;
  /* A page polling every 5 s sends a request inside the minute. */
  in.anyRequest = true;
  in.lastRequestMs = in.nowMs - BATTERY_CHECK_QUIET_MS + 1;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
  in.lastRequestMs = in.nowMs - BATTERY_CHECK_QUIET_MS;
  TEST_ASSERT_TRUE(batteryCheckDue(&in));
}

static void a_running_job_or_a_firmware_on_trial_holds_it(void) {
  in.busy = true;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
  in.busy = false;
  in.onTrial = true;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
}

static void the_clock_wrapping_round_changes_nothing(void) {
  /* Started just before the 32 bit clock wraps, read just after it. */
  in.lastTryMs = 0xFFFFFF00UL;
  in.nowMs = 0xFFFFFF00UL + BATTERY_CHECK_EVERY_MS;
  in.lastInputMs = 0xFFFFFF00UL;
  TEST_ASSERT_TRUE(batteryCheckDue(&in));
  in.nowMs--;
  TEST_ASSERT_FALSE(batteryCheckDue(&in));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(due_one_interval_after_start_and_not_a_moment_before);
  RUN_TEST(after_a_try_the_next_is_a_full_interval_away);
  RUN_TEST(unfitted_or_without_wifi_it_never_pauses);
  RUN_TEST(a_key_or_knob_in_the_last_five_minutes_holds_it);
  RUN_TEST(the_panel_dimmed_to_nothing_counts_as_idle);
  RUN_TEST(anyone_on_the_network_holds_it);
  RUN_TEST(a_running_job_or_a_firmware_on_trial_holds_it);
  RUN_TEST(the_clock_wrapping_round_changes_nothing);
  return UNITY_END();
}
