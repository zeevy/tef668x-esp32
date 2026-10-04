/* Tests for the note left behind before a restart. Runs on a PC. */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/restart_why.h"

void setUp(void) {}
void tearDown(void) {}

static void every_reason_survives_the_round_trip(void) {
  const RestartWhy all[] = {RESTART_WHY_ASKED, RESTART_WHY_BOOT_WATCHDOG,
                            RESTART_WHY_ROLLBACK, RESTART_WHY_DISPLAY,
                            RESTART_WHY_UPDATE};
  for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); i++) {
    TEST_ASSERT_EQUAL_INT(all[i], restartWhyUnpack(restartWhyPack(all[i])));
  }
}

static void an_uninitialised_word_is_not_a_reason(void) {
  /*
   * The one that matters. RTC memory holds nothing in particular across a
   * power cycle, and a 3 sitting there would otherwise be read as a rollback
   * that never happened.
   */
  TEST_ASSERT_EQUAL_INT(RESTART_WHY_NONE, restartWhyUnpack(0));
  TEST_ASSERT_EQUAL_INT(RESTART_WHY_NONE, restartWhyUnpack(3));
  TEST_ASSERT_EQUAL_INT(RESTART_WHY_NONE, restartWhyUnpack(0xFFFFFFFFUL));
  TEST_ASSERT_EQUAL_INT(RESTART_WHY_NONE, restartWhyUnpack(0xDEADBEEFUL));
  TEST_ASSERT_EQUAL_INT(RESTART_WHY_NONE, restartWhyUnpack(0x5257310AUL));
}

static void a_marked_word_with_no_reason_in_it_is_not_a_reason(void) {
  /* The marker alone, which is what a half written word looks like. */
  TEST_ASSERT_EQUAL_INT(RESTART_WHY_NONE, restartWhyUnpack(0x52573100UL));
}

static void nothing_worth_noting_packs_to_nothing(void) {
  TEST_ASSERT_EQUAL_UINT32(0, restartWhyPack(RESTART_WHY_NONE));
  TEST_ASSERT_EQUAL_UINT32(0, restartWhyPack((RestartWhy)99));
  /* One past the last real reason, which is what a new one added to the enum
   * without touching the range check would look like. */
  TEST_ASSERT_EQUAL_UINT32(
      0, restartWhyPack((RestartWhy)(RESTART_WHY_UPDATE + 1)));
  TEST_ASSERT_EQUAL_INT(RESTART_WHY_NONE, restartWhyUnpack(0));
}

static void each_reason_has_its_own_words(void) {
  TEST_ASSERT_EQUAL_STRING("asked for", restartWhyText(RESTART_WHY_ASKED));
  TEST_ASSERT_EQUAL_STRING("boot watchdog",
                           restartWhyText(RESTART_WHY_BOOT_WATCHDOG));
  TEST_ASSERT_EQUAL_STRING("rollback", restartWhyText(RESTART_WHY_ROLLBACK));
  TEST_ASSERT_EQUAL_STRING("display assertion",
                           restartWhyText(RESTART_WHY_DISPLAY));
  TEST_ASSERT_EQUAL_STRING("update", restartWhyText(RESTART_WHY_UPDATE));

  /* And no words at all for nothing, so it cannot be printed as if it were a
   * reason. */
  TEST_ASSERT_NULL(restartWhyText(RESTART_WHY_NONE));
  TEST_ASSERT_NULL(restartWhyText((RestartWhy)99));
}

static void the_reasons_are_told_apart(void) {
  /* These all read as ESP_RST_SW from the chip, so only this note tells them
   * apart. */
  TEST_ASSERT_NOT_EQUAL(restartWhyPack(RESTART_WHY_ROLLBACK),
                        restartWhyPack(RESTART_WHY_ASKED));
  TEST_ASSERT_NOT_EQUAL(restartWhyPack(RESTART_WHY_BOOT_WATCHDOG),
                        restartWhyPack(RESTART_WHY_DISPLAY));
  /* The pair that share a code path: the reboot at the end of an update and
   * the reboot button both set the same flag in web_update.cpp, and the note
   * is what separates them. */
  TEST_ASSERT_NOT_EQUAL(restartWhyPack(RESTART_WHY_UPDATE),
                        restartWhyPack(RESTART_WHY_ASKED));
}

/* The heap's low point comes back as it went in, at the edges too. */
static void a_heap_low_point_survives_the_round_trip(void) {
  const uint32_t values[] = {0, 1, 74212, 0x7FFFFFFFUL, 0xFFFFFFFFUL};
  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
    uint32_t words[2];
    restartHeapPack(values[i], words);
    uint32_t back = 111;
    TEST_ASSERT_TRUE(restartHeapUnpack(words, &back));
    TEST_ASSERT_EQUAL_UINT32(values[i], back);
  }
}

/* What RTC memory may hold after a power cycle reads as nothing: zeros,
 * ones, and a packed pair with one word changed. */
static void words_that_were_not_packed_read_as_nothing(void) {
  uint32_t back = 111;
  const uint32_t zeros[2] = {0, 0};
  TEST_ASSERT_FALSE(restartHeapUnpack(zeros, &back));
  const uint32_t ones[2] = {0xFFFFFFFFUL, 0xFFFFFFFFUL};
  TEST_ASSERT_FALSE(restartHeapUnpack(ones, &back));
  uint32_t words[2];
  restartHeapPack(74212, words);
  const uint32_t changed[2] = {words[0] + 1, words[1]};
  TEST_ASSERT_FALSE(restartHeapUnpack(changed, &back));
  TEST_ASSERT_EQUAL_UINT32(111, back);
}

static void a_null_packs_and_reads_nothing(void) {
  const uint32_t words[2] = {5, 6};
  uint32_t back = 111;
  restartHeapPack(1, NULL);
  TEST_ASSERT_FALSE(restartHeapUnpack(NULL, &back));
  TEST_ASSERT_FALSE(restartHeapUnpack(words, NULL));
  TEST_ASSERT_EQUAL_UINT32(111, back);
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(every_reason_survives_the_round_trip);
  RUN_TEST(an_uninitialised_word_is_not_a_reason);
  RUN_TEST(a_marked_word_with_no_reason_in_it_is_not_a_reason);
  RUN_TEST(nothing_worth_noting_packs_to_nothing);
  RUN_TEST(each_reason_has_its_own_words);
  RUN_TEST(the_reasons_are_told_apart);
  RUN_TEST(a_heap_low_point_survives_the_round_trip);
  RUN_TEST(words_that_were_not_packed_read_as_nothing);
  RUN_TEST(a_null_packs_and_reads_nothing);

  return UNITY_END();
}
