/* Tests for the hex colour parser and formatter. Runs on a PC. */
#include <unity.h>

#include <string.h>

#include "core/theme_colour.h"

void setUp(void) {}
void tearDown(void) {}

static void a_hash_prefixed_colour_parses(void) {
  uint8_t r, g, b;
  TEST_ASSERT_TRUE(themeColourParse("#1a2b3c", &r, &g, &b));
  TEST_ASSERT_EQUAL_UINT8(0x1a, r);
  TEST_ASSERT_EQUAL_UINT8(0x2b, g);
  TEST_ASSERT_EQUAL_UINT8(0x3c, b);
}

static void a_bare_colour_parses_the_same_way(void) {
  uint8_t r, g, b;
  TEST_ASSERT_TRUE(themeColourParse("1a2b3c", &r, &g, &b));
  TEST_ASSERT_EQUAL_UINT8(0x1a, r);
  TEST_ASSERT_EQUAL_UINT8(0x2b, g);
  TEST_ASSERT_EQUAL_UINT8(0x3c, b);
}

static void upper_case_hex_parses(void) {
  uint8_t r, g, b;
  TEST_ASSERT_TRUE(themeColourParse("#FFB200", &r, &g, &b));
  TEST_ASSERT_EQUAL_UINT8(0xFF, r);
  TEST_ASSERT_EQUAL_UINT8(0xB2, g);
  TEST_ASSERT_EQUAL_UINT8(0x00, b);
}

static void the_extremes_parse(void) {
  uint8_t r, g, b;
  TEST_ASSERT_TRUE(themeColourParse("#000000", &r, &g, &b));
  TEST_ASSERT_EQUAL_UINT8(0, r);
  TEST_ASSERT_TRUE(themeColourParse("#ffffff", &r, &g, &b));
  TEST_ASSERT_EQUAL_UINT8(255, r);
  TEST_ASSERT_EQUAL_UINT8(255, g);
  TEST_ASSERT_EQUAL_UINT8(255, b);
}

static void a_short_string_is_refused(void) {
  uint8_t r, g, b;
  TEST_ASSERT_FALSE(themeColourParse("#1a2b3", &r, &g, &b));
  TEST_ASSERT_FALSE(themeColourParse("#12", &r, &g, &b));
  TEST_ASSERT_FALSE(themeColourParse("#", &r, &g, &b));
  TEST_ASSERT_FALSE(themeColourParse("", &r, &g, &b));
}

static void a_long_string_is_refused(void) {
  uint8_t r, g, b;
  TEST_ASSERT_FALSE(themeColourParse("#1a2b3c4", &r, &g, &b));
  TEST_ASSERT_FALSE(themeColourParse("#1a2b3c00", &r, &g, &b));
}

static void a_non_hex_character_is_refused(void) {
  uint8_t r, g, b;
  TEST_ASSERT_FALSE(themeColourParse("#1a2b3g", &r, &g, &b));
  TEST_ASSERT_FALSE(themeColourParse("#zzzzzz", &r, &g, &b));
  TEST_ASSERT_FALSE(themeColourParse("#1a2 3c", &r, &g, &b));
}

static void a_null_parse_call_does_not_crash(void) {
  uint8_t r, g, b;
  TEST_ASSERT_FALSE(themeColourParse(NULL, &r, &g, &b));
  TEST_ASSERT_FALSE(themeColourParse("#1a2b3c", NULL, &g, &b));
  TEST_ASSERT_FALSE(themeColourParse("#1a2b3c", &r, NULL, &b));
  TEST_ASSERT_FALSE(themeColourParse("#1a2b3c", &r, &g, NULL));
}

static void formatting_round_trips_through_parse(void) {
  char text[8];
  themeColourFormat(0x1a, 0x2b, 0x3c, text, sizeof(text));
  TEST_ASSERT_EQUAL_STRING("#1a2b3c", text);
  uint8_t r, g, b;
  TEST_ASSERT_TRUE(themeColourParse(text, &r, &g, &b));
  TEST_ASSERT_EQUAL_UINT8(0x1a, r);
  TEST_ASSERT_EQUAL_UINT8(0x2b, g);
  TEST_ASSERT_EQUAL_UINT8(0x3c, b);
}

static void a_too_small_format_buffer_does_nothing(void) {
  char text[4] = "xxx";
  themeColourFormat(0x1a, 0x2b, 0x3c, text, sizeof(text));
  TEST_ASSERT_EQUAL_STRING("xxx", text);
}

static void a_null_format_call_does_not_crash(void) {
  themeColourFormat(0, 0, 0, NULL, 8);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(a_hash_prefixed_colour_parses);
  RUN_TEST(a_bare_colour_parses_the_same_way);
  RUN_TEST(upper_case_hex_parses);
  RUN_TEST(the_extremes_parse);
  RUN_TEST(a_short_string_is_refused);
  RUN_TEST(a_long_string_is_refused);
  RUN_TEST(a_non_hex_character_is_refused);
  RUN_TEST(a_null_parse_call_does_not_crash);
  RUN_TEST(formatting_round_trips_through_parse);
  RUN_TEST(a_too_small_format_buffer_does_nothing);
  RUN_TEST(a_null_format_call_does_not_crash);

  return UNITY_END();
}
