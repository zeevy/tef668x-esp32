/* Tests for who owns the panel during a firmware write. Runs on a PC. */
#include <unity.h>

#include <stdint.h>

#include "core/update_screen.h"

void setUp(void) {}
void tearDown(void) {}

/* Four seconds, the same figure `screen_task.cpp` passes in. */
#define HOLD_MS 4000UL

static void nothing_is_held_before_a_write_starts(void) {
  UpdateScreen u;
  updateScreenReset(&u);
  TEST_ASSERT_FALSE(updateScreenHolds(&u, 1000));

  /* And a percentage arriving with no write running draws nothing. Both
   * update routes can report progress after an abort. */
  int shown = -1;
  TEST_ASSERT_FALSE(updateScreenProgress(&u, 50, &shown));
  TEST_ASSERT_EQUAL_INT(-1, shown);
}

static void a_write_takes_the_panel_and_does_not_give_it_back(void) {
  UpdateScreen u;
  updateScreenReset(&u);
  updateScreenBegin(&u);

  TEST_ASSERT_TRUE(updateScreenHolds(&u, 1000));
  /* Days later, still held. The reboot is the only way out of this one. */
  TEST_ASSERT_TRUE(updateScreenHolds(&u, 1000 + 86400000UL));
}

static void each_whole_number_is_drawn_once(void) {
  UpdateScreen u;
  updateScreenReset(&u);
  updateScreenBegin(&u);

  int shown = -1;
  TEST_ASSERT_TRUE(updateScreenProgress(&u, 0, &shown));
  TEST_ASSERT_EQUAL_INT(0, shown);

  /* The same number again is the ordinary case: a chunk is a few kilobytes
   * and a percent of this image is about fifteen. */
  TEST_ASSERT_FALSE(updateScreenProgress(&u, 0, &shown));
  TEST_ASSERT_FALSE(updateScreenProgress(&u, 0, &shown));

  TEST_ASSERT_TRUE(updateScreenProgress(&u, 1, &shown));
  TEST_ASSERT_EQUAL_INT(1, shown);
  TEST_ASSERT_FALSE(updateScreenProgress(&u, 1, &shown));
}

static void every_step_of_a_whole_image_draws_a_hundred_and_one_times(void) {
  UpdateScreen u;
  updateScreenReset(&u);
  updateScreenBegin(&u);

  /* 1.4 MB in 1436 byte chunks, which is the shape of a real transfer. */
  const uint32_t total = 1436380;
  const uint32_t chunk = 1436;
  int drawn = 0;
  for (uint32_t done = 0; done < total; done += chunk) {
    /* The last chunk of a real transfer is a short one, so the count lands
     * exactly on the total and 100% is reached. Stepping past it in whole
     * chunks would stop at 99 and the panel would never say it finished. */
    uint32_t at = done + chunk > total ? total : done;
    int shown = -1;
    if (updateScreenProgress(&u, (int)((uint64_t)at * 100U / total), &shown)) {
      drawn++;
    }
  }
  int shown = -1;
  if (updateScreenProgress(&u, 100, &shown)) {
    drawn++;
  }
  /* 0 to 100 inclusive, and not the thousand times the chunks arrive. */
  TEST_ASSERT_EQUAL_INT(101, drawn);
}

static void a_percentage_past_the_end_is_clamped(void) {
  UpdateScreen u;
  updateScreenReset(&u);
  updateScreenBegin(&u);

  /* The browser route divides by the whole POST body, which includes the
   * multipart headers, so it cannot go over. The ArduinoOTA one divides by a
   * size the sender declared, so it can. */
  int shown = -1;
  TEST_ASSERT_TRUE(updateScreenProgress(&u, 130, &shown));
  TEST_ASSERT_EQUAL_INT(100, shown);
  TEST_ASSERT_FALSE(updateScreenProgress(&u, 140, &shown));
}

static void an_unknown_size_draws_no_number(void) {
  UpdateScreen u;
  updateScreenReset(&u);
  updateScreenBegin(&u);

  int shown = -1;
  TEST_ASSERT_FALSE(updateScreenProgress(&u, -1, &shown));
  TEST_ASSERT_EQUAL_INT(-1, shown);
  /* And the screen is still held, so it says UPDATING FIRMWARE with nothing
   * after it rather than giving the panel back. */
  TEST_ASSERT_TRUE(updateScreenHolds(&u, 1000));
}

static void a_write_that_worked_holds_until_the_reboot(void) {
  UpdateScreen u;
  updateScreenReset(&u);
  updateScreenBegin(&u);
  updateScreenFinished(&u, true, 1000, HOLD_MS);

  TEST_ASSERT_TRUE(updateScreenHolds(&u, 1000));
  TEST_ASSERT_TRUE(updateScreenHolds(&u, 1000 + HOLD_MS * 10));
}

static void a_write_that_failed_gives_the_panel_back(void) {
  UpdateScreen u;
  updateScreenReset(&u);
  updateScreenBegin(&u);
  updateScreenFinished(&u, false, 1000, HOLD_MS);

  TEST_ASSERT_TRUE(updateScreenHolds(&u, 1000));
  TEST_ASSERT_TRUE(updateScreenHolds(&u, 1000 + HOLD_MS - 1));
  TEST_ASSERT_FALSE(updateScreenHolds(&u, 1000 + HOLD_MS));

  /* And it is properly let go, not merely reported as free. */
  TEST_ASSERT_FALSE(updateScreenHolds(&u, 1000 + HOLD_MS + 1));
  int shown = -1;
  TEST_ASSERT_FALSE(updateScreenProgress(&u, 10, &shown));
}

static void a_failure_hold_survives_the_millis_wrap(void) {
  UpdateScreen u;
  updateScreenReset(&u);
  updateScreenBegin(&u);

  /* Two seconds before the counter wraps, with a four second hold. */
  const uint32_t nowMs = 0xFFFFFFFFUL - 2000UL;
  updateScreenFinished(&u, false, nowMs, HOLD_MS);

  TEST_ASSERT_TRUE(updateScreenHolds(&u, nowMs));
  TEST_ASSERT_TRUE(updateScreenHolds(&u, nowMs + 1999));
  /* Past the wrap, and the deadline still lands where it should. */
  TEST_ASSERT_TRUE(updateScreenHolds(&u, (uint32_t)(nowMs + 3999)));
  TEST_ASSERT_FALSE(updateScreenHolds(&u, (uint32_t)(nowMs + HOLD_MS)));
}

static void a_second_write_starts_the_count_again(void) {
  UpdateScreen u;
  updateScreenReset(&u);
  updateScreenBegin(&u);

  int shown = -1;
  TEST_ASSERT_TRUE(updateScreenProgress(&u, 40, &shown));
  updateScreenFinished(&u, false, 1000, HOLD_MS);
  TEST_ASSERT_FALSE(updateScreenHolds(&u, 1000 + HOLD_MS));

  /* Somebody tries again after a failed upload. The panel must not skip 40%
   * because the last attempt reached it. */
  updateScreenBegin(&u);
  TEST_ASSERT_TRUE(updateScreenProgress(&u, 40, &shown));
  TEST_ASSERT_EQUAL_INT(40, shown);
}

static void a_press_closes_only_the_failure_message(void) {
  UpdateScreen u;
  updateScreenReset(&u);

  /* No write: the press is the radio's. */
  TEST_ASSERT_FALSE(updateScreenPress(&u, 100));

  /* While the write runs, the press is swallowed and the panel stays. */
  updateScreenBegin(&u);
  TEST_ASSERT_TRUE(updateScreenPress(&u, 200));
  TEST_ASSERT_TRUE(updateScreenHolds(&u, 300));

  /* The write failed: the press is swallowed and the message goes at once,
   * not when its time runs out. */
  updateScreenFinished(&u, false, 1000, HOLD_MS);
  TEST_ASSERT_TRUE(updateScreenPress(&u, 1500));
  TEST_ASSERT_FALSE(updateScreenHolds(&u, 1500));
  TEST_ASSERT_FALSE(updateScreenPress(&u, 1600));

  /* A write that worked keeps the panel until the reboot. */
  updateScreenBegin(&u);
  updateScreenFinished(&u, true, 2000, HOLD_MS);
  TEST_ASSERT_TRUE(updateScreenPress(&u, 2100));
  TEST_ASSERT_TRUE(updateScreenHolds(&u, 2100 + HOLD_MS));
}

static void a_null_argument_asks_for_nothing(void) {
  /* Same rule as core/wifi_join: a caller that has not been started must
   * not be told to take the panel away from the radio. */
  int shown = -1;
  TEST_ASSERT_FALSE(updateScreenHolds(NULL, 1000));
  TEST_ASSERT_FALSE(updateScreenProgress(NULL, 50, &shown));
  TEST_ASSERT_FALSE(updateScreenPress(NULL, 1000));
  updateScreenBegin(NULL);
  updateScreenFinished(NULL, false, 1000, HOLD_MS);
  updateScreenReset(NULL);
}

static void the_whole_browser_upload_runs_through(void) {
  UpdateScreen u;
  updateScreenReset(&u);

  /* Nothing on the panel but the radio. */
  TEST_ASSERT_FALSE(updateScreenHolds(&u, 100));

  /* The POST arrives and the panel goes over to it. */
  updateScreenBegin(&u);
  TEST_ASSERT_TRUE(updateScreenHolds(&u, 200));

  int shown = -1;
  TEST_ASSERT_TRUE(updateScreenProgress(&u, 0, &shown));
  TEST_ASSERT_TRUE(updateScreenProgress(&u, 55, &shown));
  TEST_ASSERT_EQUAL_INT(55, shown);
  TEST_ASSERT_TRUE(updateScreenProgress(&u, 100, &shown));

  /* Written, so it stays up while the reply goes out and the radio reboots. */
  updateScreenFinished(&u, true, 16000, HOLD_MS);
  TEST_ASSERT_TRUE(updateScreenHolds(&u, 16500));
  TEST_ASSERT_TRUE(updateScreenHolds(&u, 20000));
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(nothing_is_held_before_a_write_starts);
  RUN_TEST(a_write_takes_the_panel_and_does_not_give_it_back);

  RUN_TEST(each_whole_number_is_drawn_once);
  RUN_TEST(every_step_of_a_whole_image_draws_a_hundred_and_one_times);
  RUN_TEST(a_percentage_past_the_end_is_clamped);
  RUN_TEST(an_unknown_size_draws_no_number);

  RUN_TEST(a_write_that_worked_holds_until_the_reboot);
  RUN_TEST(a_write_that_failed_gives_the_panel_back);
  RUN_TEST(a_failure_hold_survives_the_millis_wrap);
  RUN_TEST(a_second_write_starts_the_count_again);

  RUN_TEST(a_press_closes_only_the_failure_message);
  RUN_TEST(a_null_argument_asks_for_nothing);
  RUN_TEST(the_whole_browser_upload_runs_through);

  return UNITY_END();
}
