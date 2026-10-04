/*
 * Tests for the string table. Runs on a PC.
 *
 * The enum and the table come from the same list in lang/en.h, so they cannot
 * drift. What is left to check is a hole in the list, an id past the end, and
 * that the table lines up with the enum at all.
 */
#include <string.h>
#include <unity.h>

#include "core/strings.h"

void setUp(void) {}
void tearDown(void) {}

static void every_id_has_text(void) {
  for (unsigned i = 0; i < STR_COUNT; i++) {
    TEST_ASSERT_NOT_NULL(txt((StrId)i));
    TEST_ASSERT_TRUE_MESSAGE(txt((StrId)i)[0] != '\0', "an id has empty text");
  }
}

static void an_id_past_the_end_gives_an_empty_string(void) {
  TEST_ASSERT_EQUAL_STRING("", txt(STR_COUNT));
  TEST_ASSERT_EQUAL_STRING("", txt((StrId)-1));
}

/* One known text, so a table shifted by one is caught. */
static void the_text_matches_the_id(void) {
  TEST_ASSERT_EQUAL_STRING("Volume AGC", txt(STR_MENU_VOLUME_AGC));
  TEST_ASSERT_EQUAL_STRING("Alarm", txt(STR_PTY_ALARM));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(every_id_has_text);
  RUN_TEST(an_id_past_the_end_gives_an_empty_string);
  RUN_TEST(the_text_matches_the_id);
  return UNITY_END();
}
