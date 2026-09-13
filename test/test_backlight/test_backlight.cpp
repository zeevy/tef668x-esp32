/**
 * @file test_backlight.cpp
 * @brief Tests for the panel light. Runs on a PC.
 *
 * Everything here is about timing, which is what cannot be checked by looking
 * at a radio. A dim that never fires and a wake that never arrives both look
 * like a panel that is simply on.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/backlight.h"
#include "core/settings.h"

void setUp(void) {}
void tearDown(void) {}

/** A config with the fade, the dim and a known set of levels. */
static BacklightConfig cfg(uint8_t full, uint8_t dim, uint32_t afterMs,
                           uint16_t fadeUpMs) {
  BacklightConfig c;
  backlightDefaults(&c);
  c.fullPercent = full;
  c.dimPercent = dim;
  c.dimAfterMs = afterMs;
  c.fadeUpMs = fadeUpMs;
  return c;
}

/* ------------------------------------------------------------ the fade up */

static void it_starts_dark(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 0, 400);
  backlightInit(&b, &c, 1000);
  TEST_ASSERT_EQUAL_UINT8(0, backlightLevel(&b));
  TEST_ASSERT_TRUE(backlightFading(&b));
}

static void the_fade_climbs_and_arrives(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 0, 400);
  backlightInit(&b, &c, 1000);
  /* The fade length here is set by the test, not by BACKLIGHT_FADE_UP_MS, so
   * the shape is what is being checked and not the duration.
   *
   * A quarter of the way along is a quarter of the way up the scale an eye
   * reads, which is a sixteenth of the duty. A straight line in duty would
   * be at 25 here, already bright, with nothing left to see. */
  TEST_ASSERT_EQUAL_UINT8(6, backlightUpdate(&b, 1100));
  TEST_ASSERT_EQUAL_UINT8(25, backlightUpdate(&b, 1200));
  TEST_ASSERT_EQUAL_UINT8(56, backlightUpdate(&b, 1300));
  TEST_ASSERT_EQUAL_UINT8(100, backlightUpdate(&b, 1400));
  TEST_ASSERT_FALSE(backlightFading(&b));
}

static void the_fade_is_even_in_what_an_eye_sees(void) {
  /* Equal steps along the fade have to be equal steps in perceived
   * brightness, which is near enough the square root of the duty. Checking
   * the roots rather than the duties is checking the thing that matters. */
  Backlight b;
  BacklightConfig c = cfg(100, 20, 0, 400);
  backlightInit(&b, &c, 0);
  uint8_t q1 = backlightUpdate(&b, 100);
  uint8_t q2 = backlightUpdate(&b, 200);
  uint8_t q3 = backlightUpdate(&b, 300);

  /* Roots of 6, 25 and 56 are about 2.4, 5 and 7.5, evenly spaced. Compared
   * as squares to keep it in whole numbers: each step up is about the same
   * on the root scale, so the gaps between consecutive roots agree. */
  int r1 = (int)(q1 * 100);
  int r2 = (int)(q2 * 100);
  int r3 = (int)(q3 * 100);
  /* 25 is 6 times 4 and change, 56 is 25 times 2 and change: the duty grows
   * as the square of the time, which is what an even perceived fade is. */
  TEST_ASSERT_TRUE(r2 > r1 * 3 && r2 < r1 * 5);
  TEST_ASSERT_TRUE(r3 > r2 * 2 && r3 < r2 * 3);
}

static void it_never_goes_past_where_it_was_sent(void) {
  Backlight b;
  BacklightConfig c = cfg(60, 20, 0, 400);
  backlightInit(&b, &c, 0);
  TEST_ASSERT_EQUAL_UINT8(60, backlightUpdate(&b, 9999));
}

static void a_fade_of_nothing_snaps_on(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 0, 0);
  backlightInit(&b, &c, 1000);
  TEST_ASSERT_EQUAL_UINT8(100, backlightLevel(&b));
  TEST_ASSERT_FALSE(backlightFading(&b));
}

/* ---------------------------------------------------------------- the dim */

static void it_dims_once_it_is_left_alone(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 5000, 0);
  backlightInit(&b, &c, 0);

  /* One below the delay, on it, and past it. */
  TEST_ASSERT_EQUAL_UINT8(100, backlightUpdate(&b, 4999));
  TEST_ASSERT_FALSE(backlightDimmed(&b));
  backlightUpdate(&b, 5000);
  TEST_ASSERT_TRUE(backlightDimmed(&b));
  TEST_ASSERT_EQUAL_UINT8(20, backlightUpdate(&b, 5000 + 1000));
}

static void the_dim_is_a_fade_and_not_a_step(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 0, 5000, 0);
  backlightInit(&b, &c, 0);
  backlightUpdate(&b, 5000);
  /* Halfway through BACKLIGHT_FADE_DOWN_MS, so halfway down. */
  uint8_t half = backlightUpdate(&b, 5000 + BACKLIGHT_FADE_DOWN_MS / 2);
  TEST_ASSERT_TRUE(half > 20 && half < 80);
}

static void a_delay_of_zero_never_dims(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 0, 0);
  backlightInit(&b, &c, 0);
  TEST_ASSERT_EQUAL_UINT8(100, backlightUpdate(&b, 3600000UL));
  TEST_ASSERT_FALSE(backlightDimmed(&b));
}

static void a_dim_level_at_or_above_full_changes_nothing(void) {
  Backlight b;
  BacklightConfig c = cfg(50, 80, 1000, 0);
  backlightInit(&b, &c, 0);
  backlightUpdate(&b, 2000);
  TEST_ASSERT_EQUAL_UINT8(50, backlightUpdate(&b, 9000));
}

/* --------------------------------------------------------------- the wake */

static void any_input_brings_it_straight_back(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 5000, 0);
  backlightInit(&b, &c, 0);
  backlightUpdate(&b, 6000);
  backlightUpdate(&b, 7000);
  TEST_ASSERT_EQUAL_UINT8(20, backlightLevel(&b));

  backlightWake(&b, 7500);
  /* No fade. A person who has just pressed a key is already looking at the
   * panel, so anything but full brightness now is a delay. */
  TEST_ASSERT_EQUAL_UINT8(100, backlightLevel(&b));
  TEST_ASSERT_FALSE(backlightDimmed(&b));
  TEST_ASSERT_FALSE(backlightFading(&b));
}

static void a_wake_partway_through_the_drop_stops_it(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 10, 5000, 0);
  backlightInit(&b, &c, 0);
  backlightUpdate(&b, 5000);
  uint8_t partway = backlightUpdate(&b, 5000 + BACKLIGHT_FADE_DOWN_MS / 2);
  TEST_ASSERT_TRUE(partway < 100);

  backlightWake(&b, 5000 + BACKLIGHT_FADE_DOWN_MS / 2);
  TEST_ASSERT_EQUAL_UINT8(100, backlightLevel(&b));
  TEST_ASSERT_EQUAL_UINT8(100, backlightUpdate(&b, 5000 + 900));
}

static void a_wake_does_not_cut_the_fade_up_short(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 5000, 400);
  backlightInit(&b, &c, 0);
  backlightUpdate(&b, 100);
  backlightWake(&b, 100);
  TEST_ASSERT_TRUE(backlightFading(&b));
  TEST_ASSERT_EQUAL_UINT8(100, backlightUpdate(&b, 400));
}

static void the_clock_starts_again_on_every_input(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 5000, 0);
  backlightInit(&b, &c, 0);
  for (uint32_t t = 4000; t < 40000; t += 4000) {
    backlightUpdate(&b, t);
    backlightWake(&b, t);
  }
  TEST_ASSERT_FALSE(backlightDimmed(&b));
  TEST_ASSERT_EQUAL_UINT8(100, backlightLevel(&b));
}

/* -------------------------------------------------------- changed settings */

static void a_new_brightness_is_seen_at_once(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 0, 0);
  backlightInit(&b, &c, 0);
  c = cfg(40, 20, 0, 0);
  backlightSetConfig(&b, &c, 1000);
  TEST_ASSERT_EQUAL_UINT8(40, backlightLevel(&b));
}

static void a_new_dim_level_is_seen_while_it_is_dimmed(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 5000, 0);
  backlightInit(&b, &c, 0);
  backlightUpdate(&b, 6000);
  backlightUpdate(&b, 7000);

  c = cfg(100, 60, 5000, 0);
  backlightSetConfig(&b, &c, 7000);
  TEST_ASSERT_EQUAL_UINT8(60, backlightLevel(&b));
  /* Still dimmed. Changing a setting from a browser is not somebody touching
   * the radio. */
  TEST_ASSERT_TRUE(backlightDimmed(&b));
}

static void switching_the_dim_on_does_not_drop_the_panel_at_once(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 0, 0);
  backlightInit(&b, &c, 0);
  /* A minute at the browser with nobody touching the radio. */
  backlightUpdate(&b, 60000);
  TEST_ASSERT_FALSE(backlightDimmed(&b));

  /* Now the dim is switched on. The ten seconds has to be measured from
   * here, not against a radio that was already idle. */
  c = cfg(100, 20, 10000, 0);
  backlightSetConfig(&b, &c, 60000);
  TEST_ASSERT_EQUAL_UINT8(100, backlightUpdate(&b, 60000));
  TEST_ASSERT_EQUAL_UINT8(100, backlightUpdate(&b, 69999));
  TEST_ASSERT_FALSE(backlightDimmed(&b));
  backlightUpdate(&b, 70000);
  TEST_ASSERT_TRUE(backlightDimmed(&b));
}

static void changing_the_delay_starts_it_again(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 10000, 0);
  backlightInit(&b, &c, 0);
  backlightUpdate(&b, 9000);

  /* Lengthened with one second left to run. The new delay runs from now. */
  c = cfg(100, 20, 30000, 0);
  backlightSetConfig(&b, &c, 9000);
  TEST_ASSERT_EQUAL_UINT8(100, backlightUpdate(&b, 38999));
  backlightUpdate(&b, 39000);
  TEST_ASSERT_TRUE(backlightDimmed(&b));
}

static void switching_the_dim_off_brings_a_dimmed_panel_back(void) {
  Backlight b;
  BacklightConfig c = cfg(100, 20, 5000, 0);
  backlightInit(&b, &c, 0);
  backlightUpdate(&b, 6000);
  backlightUpdate(&b, 7000);
  TEST_ASSERT_EQUAL_UINT8(20, backlightLevel(&b));

  /* Without this the only thing that lifts a dimmed panel is an input, so a
   * dim switched off from a browser would leave the panel dark. */
  c = cfg(100, 20, 0, 0);
  backlightSetConfig(&b, &c, 7000);
  TEST_ASSERT_EQUAL_UINT8(100, backlightLevel(&b));
  TEST_ASSERT_FALSE(backlightDimmed(&b));
}

/* ------------------------------------------------------------ the settings */

static void the_settings_come_through_unchanged(void) {
  Settings s;
  settingsDefaults(&s);
  s.backlightPercent = 70;
  s.backlightDimPercent = 15;
  s.backlightDimAfterS = 30;
  s.backlightFade = 1;

  BacklightConfig c;
  backlightFromSettings(&s, &c);
  TEST_ASSERT_EQUAL_UINT8(70, c.fullPercent);
  TEST_ASSERT_EQUAL_UINT8(15, c.dimPercent);
  TEST_ASSERT_EQUAL_UINT32(30000, c.dimAfterMs);
  TEST_ASSERT_EQUAL_UINT16(BACKLIGHT_FADE_UP_MS, c.fadeUpMs);
}

static void the_fade_switched_off_leaves_no_fade(void) {
  Settings s;
  settingsDefaults(&s);
  s.backlightFade = 0;
  BacklightConfig c;
  backlightFromSettings(&s, &c);
  TEST_ASSERT_EQUAL_UINT16(0, c.fadeUpMs);
}

static void the_radio_ships_with_the_dim_off(void) {
  Settings s;
  settingsDefaults(&s);
  /* Decision 28. A panel that goes dark on its own is the one thing on this
   * list that reads as a fault to somebody who did not ask for it. */
  TEST_ASSERT_EQUAL_UINT8(0, s.backlightDimAfterS);
  TEST_ASSERT_EQUAL_UINT8(100, s.backlightPercent);
  TEST_ASSERT_EQUAL_UINT8(1, s.backlightFade);
}

/* ------------------------------------------------------------- the corners */

static void null_is_safe_everywhere(void) {
  TEST_ASSERT_EQUAL_UINT8(0, backlightLevel(NULL));
  TEST_ASSERT_FALSE(backlightDimmed(NULL));
  TEST_ASSERT_FALSE(backlightFading(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, backlightUpdate(NULL, 0));
  backlightWake(NULL, 0);
  backlightInit(NULL, NULL, 0);
  backlightSetConfig(NULL, NULL, 0);
  backlightDefaults(NULL);
  backlightFromSettings(NULL, NULL);

  /* And with somewhere to write to but nothing to read from, which is the
   * other half: the config has to come back usable rather than untouched. */
  BacklightConfig c;
  memset(&c, 0xFF, sizeof(c));
  backlightFromSettings(NULL, &c);
  BacklightConfig want;
  backlightDefaults(&want);
  TEST_ASSERT_EQUAL_UINT8(want.fullPercent, c.fullPercent);
  TEST_ASSERT_EQUAL_UINT32(want.dimAfterMs, c.dimAfterMs);
}

static void a_null_config_takes_the_defaults(void) {
  Backlight b;
  BacklightConfig want;
  backlightDefaults(&want);
  backlightInit(&b, NULL, 0);
  TEST_ASSERT_EQUAL_UINT8(want.fullPercent, backlightUpdate(&b, 10000));

  /* And on the way in again, not only at the start. */
  BacklightConfig c = cfg(30, 10, 1000, 0);
  backlightSetConfig(&b, &c, 10000);
  TEST_ASSERT_EQUAL_UINT8(30, backlightLevel(&b));
  backlightSetConfig(&b, NULL, 10000);
  TEST_ASSERT_EQUAL_UINT8(want.fullPercent, backlightLevel(&b));
}

static void a_level_past_a_hundred_is_held_there(void) {
  Backlight b;
  BacklightConfig c = cfg(200, 20, 0, 0);
  backlightInit(&b, &c, 0);
  TEST_ASSERT_EQUAL_UINT8(100, backlightLevel(&b));
}

static void the_millisecond_counter_may_wrap(void) {
  /* millis() wraps after about forty nine days. Every comparison is on a
   * difference, so the wrap has to pass without the panel jumping. */
  Backlight b;
  BacklightConfig c = cfg(100, 20, 5000, 400);
  uint32_t start = 0xFFFFFF00UL;
  backlightInit(&b, &c, start);
  TEST_ASSERT_EQUAL_UINT8(100, backlightUpdate(&b, start + 400));

  backlightWake(&b, start + 400);
  TEST_ASSERT_FALSE(backlightDimmed(&b));
  /* Past the wrap, and past the delay. */
  backlightUpdate(&b, start + 400 + 5000);
  TEST_ASSERT_TRUE(backlightDimmed(&b));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(it_starts_dark);
  RUN_TEST(the_fade_climbs_and_arrives);
  RUN_TEST(the_fade_is_even_in_what_an_eye_sees);
  RUN_TEST(it_never_goes_past_where_it_was_sent);
  RUN_TEST(a_fade_of_nothing_snaps_on);

  RUN_TEST(it_dims_once_it_is_left_alone);
  RUN_TEST(the_dim_is_a_fade_and_not_a_step);
  RUN_TEST(a_delay_of_zero_never_dims);
  RUN_TEST(a_dim_level_at_or_above_full_changes_nothing);

  RUN_TEST(any_input_brings_it_straight_back);
  RUN_TEST(a_wake_partway_through_the_drop_stops_it);
  RUN_TEST(a_wake_does_not_cut_the_fade_up_short);
  RUN_TEST(the_clock_starts_again_on_every_input);

  RUN_TEST(a_new_brightness_is_seen_at_once);
  RUN_TEST(a_new_dim_level_is_seen_while_it_is_dimmed);
  RUN_TEST(switching_the_dim_on_does_not_drop_the_panel_at_once);
  RUN_TEST(changing_the_delay_starts_it_again);
  RUN_TEST(switching_the_dim_off_brings_a_dimmed_panel_back);

  RUN_TEST(the_settings_come_through_unchanged);
  RUN_TEST(the_fade_switched_off_leaves_no_fade);
  RUN_TEST(the_radio_ships_with_the_dim_off);

  RUN_TEST(null_is_safe_everywhere);
  RUN_TEST(a_null_config_takes_the_defaults);
  RUN_TEST(a_level_past_a_hundred_is_held_there);
  RUN_TEST(the_millisecond_counter_may_wrap);

  return UNITY_END();
}
