/*
 * Tests for the web server's reply times. Runs on a PC.
 *
 * The ring is one module's own and has no reset, so these run in order and
 * each starts from where the one before left it.
 */
#include <unity.h>

#include <stdint.h>

#include "reply_times.h"

void setUp(void) {}
void tearDown(void) {}

static void with_no_reply_the_gap_is_unknown(void) {
  TEST_ASSERT_EQUAL_INT32(INT32_MAX, replyTimesGapMs(1000, 1010));
}

static void the_gap_is_to_the_nearer_edge_of_the_reply(void) {
  replyTimesNote(1000, 1010);
  /* After the reply, before it, and over it. */
  TEST_ASSERT_EQUAL_INT32(10, replyTimesGapMs(1020, 1030));
  TEST_ASSERT_EQUAL_INT32(5, replyTimesGapMs(990, 995));
  TEST_ASSERT_EQUAL_INT32(0, replyTimesGapMs(1005, 1006));
  TEST_ASSERT_EQUAL_INT32(0, replyTimesGapMs(990, 1000));
}

static void the_nearest_of_several_replies_counts(void) {
  replyTimesNote(2000, 2010);
  TEST_ASSERT_EQUAL_INT32(30, replyTimesGapMs(1040, 1050));
  TEST_ASSERT_EQUAL_INT32(40, replyTimesGapMs(1950, 1960));
}

static void a_span_older_than_a_full_ring_has_no_known_gap(void) {
  /* Thirty two more, so the first two are gone and the oldest kept begins
   * at 3000. */
  for (uint32_t i = 0; i < 32; i++) {
    replyTimesNote(3000 + i * 100, 3000 + i * 100 + 5);
  }
  TEST_ASSERT_EQUAL_INT32(INT32_MAX, replyTimesGapMs(1000, 1010));
  /* One that ends inside what is kept is still answered. */
  TEST_ASSERT_EQUAL_INT32(0, replyTimesGapMs(2990, 3002));
  TEST_ASSERT_EQUAL_INT32(15, replyTimesGapMs(3020, 3030));
}

static void the_gap_holds_across_the_clock_wrap(void) {
  for (uint32_t i = 0; i < 32; i++) {
    replyTimesNote(0xFFFFF000u + i * 100, 0xFFFFF000u + i * 100 + 5);
  }
  /* The last reply is from 0xFFFFFC1C to 0xFFFFFC21 and then the clock
   * wraps; a span just past zero is 1007 ms after it. */
  TEST_ASSERT_EQUAL_INT32(1007, replyTimesGapMs(0x10u, 0x20u));
  TEST_ASSERT_EQUAL_INT32(0, replyTimesGapMs(0xFFFFFBFEu, 0x2u));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(with_no_reply_the_gap_is_unknown);
  RUN_TEST(the_gap_is_to_the_nearer_edge_of_the_reply);
  RUN_TEST(the_nearest_of_several_replies_counts);
  RUN_TEST(a_span_older_than_a_full_ring_has_no_known_gap);
  RUN_TEST(the_gap_holds_across_the_clock_wrap);
  return UNITY_END();
}
