/* Tests for the RSSI to bars mapping. Runs on a PC. */
#include <unity.h>

#include "core/wifi_signal.h"

void setUp(void) {}
void tearDown(void) {}

static void a_strong_reading_is_three_bars(void) {
  TEST_ASSERT_EQUAL_UINT8(3, wifiSignalBars(-30));
  TEST_ASSERT_EQUAL_UINT8(3, wifiSignalBars(-49));
}

static void the_top_boundary_is_tested_on_both_sides(void) {
  TEST_ASSERT_EQUAL_UINT8(3, wifiSignalBars(-49));
  TEST_ASSERT_EQUAL_UINT8(2, wifiSignalBars(-50));
}

static void the_middle_boundary_is_tested_on_both_sides(void) {
  TEST_ASSERT_EQUAL_UINT8(2, wifiSignalBars(-59));
  TEST_ASSERT_EQUAL_UINT8(1, wifiSignalBars(-60));
}

static void the_bottom_boundary_is_tested_on_both_sides(void) {
  TEST_ASSERT_EQUAL_UINT8(1, wifiSignalBars(-69));
  TEST_ASSERT_EQUAL_UINT8(0, wifiSignalBars(-70));
}

static void a_very_weak_reading_is_no_bars(void) {
  TEST_ASSERT_EQUAL_UINT8(0, wifiSignalBars(-90));
  TEST_ASSERT_EQUAL_UINT8(0, wifiSignalBars(-128));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(a_strong_reading_is_three_bars);
  RUN_TEST(the_top_boundary_is_tested_on_both_sides);
  RUN_TEST(the_middle_boundary_is_tested_on_both_sides);
  RUN_TEST(the_bottom_boundary_is_tested_on_both_sides);
  RUN_TEST(a_very_weak_reading_is_no_bars);

  return UNITY_END();
}
