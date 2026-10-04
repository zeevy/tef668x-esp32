/* Tests for the POST body size check. Runs on a PC. */
#include <unity.h>

#include "core/web_limits.h"

void setUp(void) {}
void tearDown(void) {}

static void a_body_under_the_cap_is_not_too_large(void) {
  TEST_ASSERT_FALSE(webPostBodyTooLarge(0, 8192));
  TEST_ASSERT_FALSE(webPostBodyTooLarge(1, 8192));
  TEST_ASSERT_FALSE(webPostBodyTooLarge(8191, 8192));
}

static void a_body_exactly_the_cap_is_not_too_large(void) {
  TEST_ASSERT_FALSE(webPostBodyTooLarge(8192, 8192));
}

static void a_body_one_over_the_cap_is_too_large(void) {
  TEST_ASSERT_TRUE(webPostBodyTooLarge(8193, 8192));
}

static void a_body_well_over_the_cap_is_too_large(void) {
  TEST_ASSERT_TRUE(webPostBodyTooLarge(500000, 8192));
}

static void a_negative_header_fails_closed(void) {
  /* String::toInt() returns this for a header that opens with a minus
   * sign. Passed through as a size_t further down, it would underflow
   * into a huge value instead of the small one it looks like here. */
  TEST_ASSERT_TRUE(webPostBodyTooLarge(-1, 8192));
  TEST_ASSERT_TRUE(webPostBodyTooLarge(-500000, 8192));
}

static void the_default_cap_matches_what_a_full_channel_list_needs(void) {
  /* 99 lines of 58 bytes plus the 38 byte header, with headroom over it. */
  TEST_ASSERT_FALSE(webPostBodyTooLarge(5780, WEB_MAX_POST_BODY_BYTES));
  TEST_ASSERT_EQUAL_UINT32(8192u, WEB_MAX_POST_BODY_BYTES);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(a_body_under_the_cap_is_not_too_large);
  RUN_TEST(a_body_exactly_the_cap_is_not_too_large);
  RUN_TEST(a_body_one_over_the_cap_is_too_large);
  RUN_TEST(a_body_well_over_the_cap_is_too_large);
  RUN_TEST(a_negative_header_fails_closed);
  RUN_TEST(the_default_cap_matches_what_a_full_channel_list_needs);
  return UNITY_END();
}
