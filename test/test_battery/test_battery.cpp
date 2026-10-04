/* Tests for what a battery voltage means. Runs on a PC. */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/battery.h"

static Battery b;

void setUp(void) {
  batteryReset(&b);
}
void tearDown(void) {}

/* Settle the filter on one voltage with 200 readings. */
static void settle(uint16_t mv) {
  for (int i = 0; i < 200; i++) {
    batteryFeed(&b, mv, true);
  }
}

/* ------------------------------------------------------ what is a reading */

static void nothing_is_known_before_the_first_reading(void) {
  TEST_ASSERT_EQUAL_UINT16(0, batteryMilliVolts(&b));
  TEST_ASSERT_EQUAL_UINT8(0, batteryPercent(&b));
}

static void the_first_plausible_reading_is_taken_as_it_stands(void) {
  TEST_ASSERT_TRUE(batteryFeed(&b, 3800, true));
  TEST_ASSERT_EQUAL_UINT16(3800, batteryMilliVolts(&b));
}

static void a_driver_that_could_not_read_is_not_a_reading(void) {
  TEST_ASSERT_FALSE(batteryFeed(&b, 3800, false));
  TEST_ASSERT_EQUAL_UINT16(0, batteryMilliVolts(&b));
}

static void a_zero_is_never_taken_as_a_flat_battery(void) {
  /* The whole reason this file exists. The Arduino core returns 0 from
   * analogRead when Wi-Fi has the converter, and a flat cell would read 0
   * too, so the two cannot be told apart and neither is allowed through. */
  TEST_ASSERT_FALSE(batteryFeed(&b, 0, true));
  TEST_ASSERT_EQUAL_UINT16(0, batteryMilliVolts(&b));
  TEST_ASSERT_EQUAL_UINT8(0, batteryPercent(&b));
}

static void the_plausible_boundary_and_either_side_of_it(void) {
  TEST_ASSERT_FALSE(batteryFeed(&b, BATTERY_PLAUSIBLE_MV - 1, true));
  batteryReset(&b);
  TEST_ASSERT_TRUE(batteryFeed(&b, BATTERY_PLAUSIBLE_MV, true));
  batteryReset(&b);
  TEST_ASSERT_TRUE(batteryFeed(&b, BATTERY_IMPLAUSIBLE_MV, true));
  batteryReset(&b);
  TEST_ASSERT_FALSE(batteryFeed(&b, BATTERY_IMPLAUSIBLE_MV + 1, true));
}

static void a_reading_far_too_high_means_the_divider_is_wrong(void) {
  /* Twice the assumed ratio would put a 3.9 V cell at 7.8 V. Printing that
   * is arithmetic presented as measurement. */
  TEST_ASSERT_FALSE(batteryFeed(&b, 7800, true));
}

/* ------------------------------------------------------------- the strikes */

static void one_failed_read_does_not_blank_the_panel(void) {
  settle(3900);
  TEST_ASSERT_TRUE(batteryFeed(&b, 0, false));
  TEST_ASSERT_TRUE(batteryMilliVolts(&b) > 0);
}

static void enough_failed_reads_in_a_row_do(void) {
  settle(3900);
  for (int i = 0; i < BATTERY_STRIKES - 1; i++) {
    TEST_ASSERT_TRUE(batteryFeed(&b, 0, false));
  }
  TEST_ASSERT_FALSE(batteryFeed(&b, 0, false));
  TEST_ASSERT_EQUAL_UINT16(0, batteryMilliVolts(&b));
}

static void a_good_read_clears_the_strikes(void) {
  settle(3900);
  batteryFeed(&b, 0, false);
  batteryFeed(&b, 0, false);
  TEST_ASSERT_TRUE(batteryFeed(&b, 3900, true));
  /* Back to none, so the next two failures are not the last two of a run. */
  batteryFeed(&b, 0, false);
  batteryFeed(&b, 0, false);
  TEST_ASSERT_TRUE(batteryMilliVolts(&b) > 0);
}

/* -------------------------------------------------------------- smoothing */

static void a_steady_voltage_stays_where_it_is(void) {
  settle(3750);
  TEST_ASSERT_EQUAL_UINT16(3750, batteryMilliVolts(&b));
}

static void converter_noise_barely_moves_it(void) {
  settle(3750);
  batteryFeed(&b, 3790, true);
  /* Forty millivolts of noise on one reading moves the answer by under
   * three, which is a fifth of a per cent and invisible. */
  TEST_ASSERT_TRUE(batteryMilliVolts(&b) - 3750 < 4);
}

static void it_follows_a_real_change(void) {
  settle(4100);
  settle(3400);
  TEST_ASSERT_TRUE(batteryMilliVolts(&b) < 3500);
}

/* ------------------------------------------------------------- percentage */

static void full_and_empty_are_the_ends(void) {
  settle(BATTERY_FULL_MV);
  TEST_ASSERT_EQUAL_UINT8(100, batteryPercent(&b));
  batteryReset(&b);
  settle(BATTERY_EMPTY_MV);
  TEST_ASSERT_EQUAL_UINT8(0, batteryPercent(&b));
}

static void past_the_ends_is_held_rather_than_wrapped(void) {
  settle(4400);
  TEST_ASSERT_EQUAL_UINT8(100, batteryPercent(&b));
  batteryReset(&b);
  settle(2600);
  TEST_ASSERT_EQUAL_UINT8(0, batteryPercent(&b));
}

static void the_middle_is_the_middle(void) {
  settle((BATTERY_FULL_MV + BATTERY_EMPTY_MV) / 2);
  TEST_ASSERT_EQUAL_UINT8(50, batteryPercent(&b));
}

/* ------------------------------------------------------------- formatting */

static void percent_reads_as_percent(void) {
  char out[BATTERY_TEXT_LEN];
  settle(3900);
  TEST_ASSERT_TRUE(batteryFormat(&b, BATTERY_SHOW_PERCENT, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("75%", out);
}

/* One decimal, rounded to the nearest tenth. */
static void volts_read_as_volts(void) {
  char out[BATTERY_TEXT_LEN];
  settle(3920);
  TEST_ASSERT_TRUE(batteryFormat(&b, BATTERY_SHOW_VOLTS, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("3.9", out);
  batteryReset(&b);
  settle(3960);
  TEST_ASSERT_TRUE(batteryFormat(&b, BATTERY_SHOW_VOLTS, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("4.0", out);
  batteryReset(&b);
  settle(3949);
  TEST_ASSERT_TRUE(batteryFormat(&b, BATTERY_SHOW_VOLTS, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("3.9", out);
}

static void a_voltage_with_a_zero_tenth_keeps_it(void) {
  char out[BATTERY_TEXT_LEN];
  settle(3020);
  TEST_ASSERT_TRUE(batteryFormat(&b, BATTERY_SHOW_VOLTS, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("3.0", out);
}

static void off_writes_nothing(void) {
  char out[BATTERY_TEXT_LEN];
  settle(3900);
  TEST_ASSERT_FALSE(batteryFormat(&b, BATTERY_SHOW_OFF, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
}

static void no_reading_writes_nothing(void) {
  char out[BATTERY_TEXT_LEN];
  TEST_ASSERT_FALSE(batteryFormat(&b, BATTERY_SHOW_PERCENT, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
  TEST_ASSERT_FALSE(batteryFormat(&b, BATTERY_SHOW_VOLTS, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
}

static void formatting_refuses_what_it_cannot_write_into(void) {
  char small[BATTERY_TEXT_LEN - 1];
  settle(3900);
  TEST_ASSERT_FALSE(
      batteryFormat(&b, BATTERY_SHOW_VOLTS, small, sizeof(small)));
  TEST_ASSERT_FALSE(
      batteryFormat(&b, BATTERY_SHOW_VOLTS, NULL, BATTERY_TEXT_LEN));
  char out[BATTERY_TEXT_LEN];
  TEST_ASSERT_FALSE(batteryFormat(NULL, BATTERY_SHOW_VOLTS, out, sizeof(out)));
  TEST_ASSERT_FALSE(batteryFormat(&b, BATTERY_SHOW_COUNT, out, sizeof(out)));
}

static void a_null_battery_is_safe_everywhere(void) {
  batteryReset(NULL);
  TEST_ASSERT_FALSE(batteryFeed(NULL, 3900, true));
  TEST_ASSERT_EQUAL_UINT16(0, batteryMilliVolts(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, batteryPercent(NULL));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(nothing_is_known_before_the_first_reading);
  RUN_TEST(the_first_plausible_reading_is_taken_as_it_stands);
  RUN_TEST(a_driver_that_could_not_read_is_not_a_reading);
  RUN_TEST(a_zero_is_never_taken_as_a_flat_battery);
  RUN_TEST(the_plausible_boundary_and_either_side_of_it);
  RUN_TEST(a_reading_far_too_high_means_the_divider_is_wrong);

  RUN_TEST(one_failed_read_does_not_blank_the_panel);
  RUN_TEST(enough_failed_reads_in_a_row_do);
  RUN_TEST(a_good_read_clears_the_strikes);

  RUN_TEST(a_steady_voltage_stays_where_it_is);
  RUN_TEST(converter_noise_barely_moves_it);
  RUN_TEST(it_follows_a_real_change);

  RUN_TEST(full_and_empty_are_the_ends);
  RUN_TEST(past_the_ends_is_held_rather_than_wrapped);
  RUN_TEST(the_middle_is_the_middle);

  RUN_TEST(percent_reads_as_percent);
  RUN_TEST(volts_read_as_volts);
  RUN_TEST(a_voltage_with_a_zero_tenth_keeps_it);
  RUN_TEST(off_writes_nothing);
  RUN_TEST(no_reading_writes_nothing);
  RUN_TEST(formatting_refuses_what_it_cannot_write_into);
  RUN_TEST(a_null_battery_is_safe_everywhere);
  return UNITY_END();
}
