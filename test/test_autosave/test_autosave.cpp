/**
 * @file test_autosave.cpp
 * @brief Tests for when the settings get written down. Runs on a PC.
 *
 * All of this is about time, which is the part that cannot be checked by
 * looking at a radio. A save that never fires and a save that fires forty
 * times a minute both look like a radio working normally, and the second one
 * only shows up years later as a worn out sector.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/autosave.h"

void setUp(void) {}
void tearDown(void) {}

/** A state set up with the shipping wait. */
static AutoSave fresh(uint32_t nowMs) {
  AutoSave a;
  memset(&a, 0, sizeof(a));
  autoSaveInit(&a, AUTOSAVE_IDLE_MS, nowMs);
  return a;
}

static void nothing_is_written_when_nothing_differs(void) {
  AutoSave a = fresh(0);
  TEST_ASSERT_FALSE(autoSaveDue(&a, false, false, false, 60000));
}

static void a_change_waits_out_the_whole_idle_time(void) {
  AutoSave a = fresh(0);
  autoSaveDue(&a, true, true, false, 1000); /* Something moved at 1 s. */

  /* One below the wait, on it, and one above. */
  TEST_ASSERT_FALSE(autoSaveDue(&a, true, false, false, 1000 + 9999));
  TEST_ASSERT_TRUE(autoSaveDue(&a, true, false, false, 1000 + 10000));
  TEST_ASSERT_TRUE(autoSaveDue(&a, true, false, false, 1000 + 10001));
}

static void every_further_change_starts_the_wait_again(void) {
  /* The case the measurement is about. Hunting across a band is a burst of
   * inputs a fraction of a second apart, and it has to come out as one
   * write, not one per channel. */
  AutoSave a = fresh(0);
  for (uint32_t t = 500; t < 30000; t += 500) {
    TEST_ASSERT_FALSE(autoSaveDue(&a, true, true, false, t));
  }
  /* Only once the hand comes off. */
  TEST_ASSERT_FALSE(autoSaveDue(&a, true, false, false, 39400));
  TEST_ASSERT_TRUE(autoSaveDue(&a, true, false, false, 39500));
}

static void a_seek_never_writes(void) {
  AutoSave a = fresh(0);
  autoSaveDue(&a, true, true, true, 1000);
  /* Long past the wait, but the seek is still walking the band. */
  TEST_ASSERT_FALSE(autoSaveDue(&a, true, false, true, 60000));
  /* And the moment it stops, the value it stopped on is worth keeping. */
  TEST_ASSERT_TRUE(autoSaveDue(&a, true, false, false, 60000));
}

static void writing_starts_the_wait_again(void) {
  AutoSave a = fresh(0);
  autoSaveDue(&a, true, true, false, 1000);
  TEST_ASSERT_TRUE(autoSaveDue(&a, true, false, false, 11000));

  autoSaveDone(&a, 11000);
  /* Still different, because the caller has not caught up yet, but it must
   * not write again on the next tick. */
  TEST_ASSERT_FALSE(autoSaveDue(&a, true, false, false, 11001));
  TEST_ASSERT_TRUE(autoSaveDue(&a, true, false, false, 21000));
}

static void settling_back_to_what_is_stored_cancels_the_write(void) {
  /* Tune away and come back to where you started, and there is nothing to
   * write. */
  AutoSave a = fresh(0);
  autoSaveDue(&a, true, true, false, 1000);
  TEST_ASSERT_FALSE(autoSaveDue(&a, false, true, false, 2000));
  TEST_ASSERT_FALSE(autoSaveDue(&a, false, false, false, 60000));
}

static void a_wait_of_zero_never_writes(void) {
  /* The way to switch it off. */
  AutoSave a;
  memset(&a, 0, sizeof(a));
  autoSaveInit(&a, 0, 0);
  TEST_ASSERT_FALSE(autoSaveDue(&a, true, true, false, 3600000UL));
}

static void the_time_left_counts_down_and_stops_at_nothing(void) {
  AutoSave a = fresh(0);
  autoSaveDue(&a, true, true, false, 1000);
  TEST_ASSERT_EQUAL_UINT32(10000, autoSaveWaitMs(&a, true, 1000));
  TEST_ASSERT_EQUAL_UINT32(6000, autoSaveWaitMs(&a, true, 5000));
  TEST_ASSERT_EQUAL_UINT32(0, autoSaveWaitMs(&a, true, 11000));
  TEST_ASSERT_EQUAL_UINT32(0, autoSaveWaitMs(&a, true, 99000));
  /* Nothing waiting means no countdown to show. */
  TEST_ASSERT_EQUAL_UINT32(0, autoSaveWaitMs(&a, false, 5000));
}

static void a_state_that_was_never_set_up_starts_from_its_first_tick(void) {
  /* Zeroed rather than initialised. Without starting the clock here, a first
   * tick minutes after boot would look like a long idle and write at once. */
  AutoSave a;
  memset(&a, 0, sizeof(a));
  a.idleMs = AUTOSAVE_IDLE_MS;
  TEST_ASSERT_FALSE(autoSaveDue(&a, true, false, false, 500000));
  TEST_ASSERT_TRUE(autoSaveDue(&a, true, false, false, 510000));
}

static void the_millisecond_counter_may_wrap(void) {
  uint32_t start = 0xFFFFFF00UL;
  AutoSave a = fresh(start);
  autoSaveDue(&a, true, true, false, start);
  /* Straight over the wrap. */
  TEST_ASSERT_FALSE(autoSaveDue(&a, true, false, false, start + 9999));
  TEST_ASSERT_TRUE(autoSaveDue(&a, true, false, false, start + 10000));
  TEST_ASSERT_EQUAL_UINT32(1, autoSaveWaitMs(&a, true, start + 9999));
}

static void null_is_safe(void) {
  TEST_ASSERT_FALSE(autoSaveDue(NULL, true, true, false, 0));
  TEST_ASSERT_EQUAL_UINT32(0, autoSaveWaitMs(NULL, true, 0));
  autoSaveInit(NULL, 1000, 0);
  autoSaveDone(NULL, 0);
}

/* The volume is deliberately not part of what drives a write. The rule lives
 * in settings_task.cpp because it needs the squelch mode, but the reason is
 * worth stating where the wait is tested: the volume follows the knob, so
 * letting it count would mean a write to flash after every turn of the
 * volume control, for a value decision 26 says is read back in one mode
 * only. Checklist rows 339 and 340 are the check on the radio. */

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(nothing_is_written_when_nothing_differs);
  RUN_TEST(a_change_waits_out_the_whole_idle_time);
  RUN_TEST(every_further_change_starts_the_wait_again);
  RUN_TEST(a_seek_never_writes);
  RUN_TEST(writing_starts_the_wait_again);
  RUN_TEST(settling_back_to_what_is_stored_cancels_the_write);
  RUN_TEST(a_wait_of_zero_never_writes);
  RUN_TEST(the_time_left_counts_down_and_stops_at_nothing);
  RUN_TEST(a_state_that_was_never_set_up_starts_from_its_first_tick);
  RUN_TEST(the_millisecond_counter_may_wrap);
  RUN_TEST(null_is_safe);

  return UNITY_END();
}
