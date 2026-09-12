/**
 * @file test_input.cpp
 * @brief Tests for the knob and button state machines. Runs on a PC.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/input.h"

static Encoder enc;
static Button btn;
static ButtonConfig btnCfg;

void setUp(void) {
  encoderInit(&enc, ENCODER_STANDARD, ENCODER_NORMAL);
  memset(&btn, 0, sizeof(btn));
  buttonDefaults(&btnCfg);
  /* Most button tests are about the double press, so they ask for it. The
   * default is off, and there is a test below for that. */
  btnCfg.wantDouble = true;
}
void tearDown(void) {}

/* ---------------------------------------------------------------- encoder */

/** One full quadrature cycle clockwise, as the lines really move. */
static int8_t turnUp(Encoder *e) {
  int8_t total = 0;
  total = (int8_t)(total + encoderFeed(e, true, false));
  total = (int8_t)(total + encoderFeed(e, true, true));
  total = (int8_t)(total + encoderFeed(e, false, true));
  total = (int8_t)(total + encoderFeed(e, false, false));
  return total;
}

/** The same cycle the other way. */
static int8_t turnDown(Encoder *e) {
  int8_t total = 0;
  total = (int8_t)(total + encoderFeed(e, false, true));
  total = (int8_t)(total + encoderFeed(e, true, true));
  total = (int8_t)(total + encoderFeed(e, true, false));
  total = (int8_t)(total + encoderFeed(e, false, false));
  return total;
}

static void the_first_reading_never_moves_the_dial(void) {
  Encoder e;
  encoderInit(&e, ENCODER_STANDARD, ENCODER_NORMAL);
  /* Whatever the knob happens to be resting on at power on. */
  TEST_ASSERT_EQUAL_INT8(0, encoderFeed(&e, true, true));
}

static void one_click_clockwise_is_one_step_up(void) {
  encoderFeed(&enc, false, false); /* Settle where the knob is sitting. */
  TEST_ASSERT_EQUAL_INT8(1, turnUp(&enc));
}

static void one_click_anticlockwise_is_one_step_down(void) {
  encoderFeed(&enc, false, false);
  TEST_ASSERT_EQUAL_INT8(-1, turnDown(&enc));
}

static void a_reversed_encoder_counts_the_other_way(void) {
  Encoder e;
  encoderInit(&e, ENCODER_STANDARD, ENCODER_REVERSED);
  encoderFeed(&e, false, false);
  TEST_ASSERT_EQUAL_INT8(-1, turnUp(&e));
  TEST_ASSERT_EQUAL_INT8(1, turnDown(&e));
}

static void the_optical_encoder_needs_more_edges_for_one_click(void) {
  /* The same movement that is one click on the standard part must not be two
   * clicks on the optical one. This is the bug the setting exists to stop. */
  Encoder opt;
  encoderInit(&opt, ENCODER_OPTICAL, ENCODER_NORMAL);
  encoderFeed(&opt, false, false);
  TEST_ASSERT_EQUAL_INT8(0, turnUp(&opt));
  TEST_ASSERT_EQUAL_INT8(1, turnUp(&opt));
}

static void a_line_that_does_not_move_reports_nothing(void) {
  encoderFeed(&enc, false, false);
  for (int i = 0; i < 20; i++) {
    TEST_ASSERT_EQUAL_INT8(0, encoderFeed(&enc, false, false));
  }
}

static void both_lines_jumping_at_once_is_ignored(void) {
  /* An impossible transition. That is a bouncing contact, not a turn, and it
   * must not be counted as movement in either direction. */
  encoderFeed(&enc, false, false);
  for (int i = 0; i < 20; i++) {
    TEST_ASSERT_EQUAL_INT8(0, encoderFeed(&enc, true, true));
    TEST_ASSERT_EQUAL_INT8(0, encoderFeed(&enc, false, false));
  }
}

static void turning_back_and_forth_ends_where_it_started(void) {
  encoderFeed(&enc, false, false);
  int32_t net = 0;
  for (int i = 0; i < 10; i++) {
    net += turnUp(&enc);
    net += turnDown(&enc);
  }
  TEST_ASSERT_EQUAL_INT32(0, net);
}

static void ten_clicks_are_ten_steps_and_not_nine(void) {
  encoderFeed(&enc, false, false);
  int32_t net = 0;
  for (int i = 0; i < 10; i++) {
    net += turnUp(&enc);
  }
  TEST_ASSERT_EQUAL_INT32(10, net);
}

static void a_null_encoder_does_not_crash(void) {
  encoderInit(NULL, ENCODER_STANDARD, ENCODER_NORMAL);
  TEST_ASSERT_EQUAL_INT8(0, encoderFeed(NULL, true, true));
}

/* ----------------------------------------------------------- acceleration */

static void a_slow_turn_moves_one_step_at_a_time(void) {
  Acceleration a;
  memset(&a, 0, sizeof(a));
  uint32_t t = 1000;
  TEST_ASSERT_EQUAL_UINT8(1, accelerationSteps(&a, NULL, t));
  for (int i = 0; i < 5; i++) {
    t += 500;
    TEST_ASSERT_EQUAL_UINT8(1, accelerationSteps(&a, NULL, t));
  }
}

static void a_spin_moves_further_per_click(void) {
  Acceleration a;
  memset(&a, 0, sizeof(a));
  uint32_t t = 1000;
  accelerationSteps(&a, NULL, t);
  t += 5;
  TEST_ASSERT_EQUAL_UINT8(6, accelerationSteps(&a, NULL, t));
}

static void every_threshold_is_tested_on_both_sides(void) {
  AccelerationConfig cfg;
  accelerationDefaults(&cfg);
  Acceleration a;
  uint32_t t;

  /* One below each boundary is the faster band, the boundary itself is not.
   * A threshold with no test on its exact value is how a band silently
   * becomes one wide. */
  struct {
    uint16_t gap;
    uint8_t want;
  } cases[] = {
      {(uint16_t)(cfg.spinMs - 1), cfg.spinSteps},
      {cfg.spinMs, cfg.fasterSteps},
      {(uint16_t)(cfg.fasterMs - 1), cfg.fasterSteps},
      {cfg.fasterMs, cfg.fastSteps},
      {(uint16_t)(cfg.fastMs - 1), cfg.fastSteps},
      {cfg.fastMs, 1},
      {(uint16_t)(cfg.fastMs + 1), 1},
  };

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    memset(&a, 0, sizeof(a));
    t = 100000;
    accelerationSteps(&a, &cfg, t);
    t += cases[i].gap;
    TEST_ASSERT_EQUAL_UINT8(cases[i].want, accelerationSteps(&a, &cfg, t));
  }
}

static void acceleration_survives_the_millisecond_wrap(void) {
  Acceleration a;
  memset(&a, 0, sizeof(a));
  uint32_t t = 0xFFFFFFF0UL;
  accelerationSteps(&a, NULL, t);
  /* Ten milliseconds later, across the wrap. That is a spin, not a pause of
   * forty nine days. */
  t += 10;
  TEST_ASSERT_EQUAL_UINT8(6, accelerationSteps(&a, NULL, t));
}

static void a_null_accelerator_still_returns_one_step(void) {
  TEST_ASSERT_EQUAL_UINT8(1, accelerationSteps(NULL, NULL, 0));
}

/* ----------------------------------------------------------------- button */

/** Hold a level for a while, returning the first event that comes out. */
static ButtonEvent hold(bool pressed, uint32_t *t, uint32_t forMs) {
  ButtonEvent seen = BUTTON_NONE;
  for (uint32_t i = 0; i < forMs; i += 5) {
    ButtonEvent e = buttonFeed(&btn, &btnCfg, pressed, *t);
    if (e != BUTTON_NONE && seen == BUTTON_NONE) {
      seen = e;
    }
    *t += 5;
  }
  return seen;
}

static void without_double_watching_a_press_reports_as_soon_as_it_ends(void) {
  /* The default. A button nothing binds a double press to must not make
   * every press wait to find out whether a second one is coming, which is
   * what swallowed quick presses of MODE. */
  buttonDefaults(&btnCfg);
  uint32_t t = 1000;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(true, &t, 100));
  /* Reported within one debounce time of letting go, not one doubleMs. */
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT, hold(false, &t, btnCfg.debounceMs + 10));
}

static void without_double_watching_two_quick_taps_are_two_presses(void) {
  buttonDefaults(&btnCfg);
  uint32_t t = 1000;
  int shorts = 0;
  for (int i = 0; i < 4; i++) {
    if (hold(true, &t, 60) == BUTTON_SHORT) {
      shorts++;
    }
    if (hold(false, &t, 60) == BUTTON_SHORT) {
      shorts++;
    }
  }
  TEST_ASSERT_EQUAL_INT(4, shorts);
}

static void without_double_watching_a_long_press_is_still_long(void) {
  buttonDefaults(&btnCfg);
  uint32_t t = 1000;
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, hold(true, &t, 800));
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(false, &t, 400));
}

static void a_tap_is_a_short_press(void) {
  uint32_t t = 1000;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(true, &t, 100));
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT, hold(false, &t, 500));
}

static void a_hold_is_a_long_press(void) {
  uint32_t t = 1000;
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, hold(true, &t, 800));
}

static void a_long_press_is_reported_once_not_repeatedly(void) {
  uint32_t t = 1000;
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, hold(true, &t, 800));
  /* Still held, for another two seconds. */
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(true, &t, 2000));
  /* And letting go of a long press is not also a short press. */
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(false, &t, 800));
}

static void two_taps_are_a_double_press(void) {
  uint32_t t = 1000;
  hold(true, &t, 60);
  hold(false, &t, 100);
  TEST_ASSERT_EQUAL_INT(BUTTON_DOUBLE, hold(true, &t, 60));
  /* And the second tap of the pair does not then also report a short. */
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(false, &t, 600));
}

static void two_taps_far_apart_are_two_short_presses(void) {
  uint32_t t = 1000;
  hold(true, &t, 60);
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT, hold(false, &t, 600));
  hold(true, &t, 60);
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT, hold(false, &t, 600));
}

static void a_press_on_the_long_boundary_is_long(void) {
  uint32_t t = 1000;
  /* Pressed, then held to exactly the long press time from when it settled. */
  buttonFeed(&btn, &btnCfg, true, t);
  t += btnCfg.debounceMs; /* The press is only seen once it has settled. */
  buttonFeed(&btn, &btnCfg, true, t);
  uint32_t pressedAt = t;
  /* Every millisecond up to but not including the boundary is not yet a long
   * press. */
  while (t + 1 < pressedAt + btnCfg.longMs) {
    t++;
    TEST_ASSERT_EQUAL_INT(BUTTON_NONE, buttonFeed(&btn, &btnCfg, true, t));
  }
  t = pressedAt + btnCfg.longMs;
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, buttonFeed(&btn, &btnCfg, true, t));
}

static void a_bouncing_contact_is_still_one_press(void) {
  uint32_t t = 1000;
  int shorts = 0;
  /* Five milliseconds of chatter as the contact closes, which is shorter
   * than the debounce time. */
  for (int i = 0; i < 10; i++) {
    if (buttonFeed(&btn, &btnCfg, (i % 2) == 0, t) == BUTTON_SHORT) {
      shorts++;
    }
    t += 1;
  }
  /* Then held properly and let go. */
  if (hold(true, &t, 100) == BUTTON_SHORT) {
    shorts++;
  }
  if (hold(false, &t, 600) == BUTTON_SHORT) {
    shorts++;
  }
  TEST_ASSERT_EQUAL_INT(1, shorts);
}

static void a_button_survives_the_millisecond_wrap(void) {
  uint32_t t = 0xFFFFFF00UL;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(true, &t, 100));
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT, hold(false, &t, 500));
  /* t has now wrapped past zero. A long press still works on the far side. */
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, hold(true, &t, 800));
}

static void event_names_are_never_null(void) {
  TEST_ASSERT_EQUAL_STRING("none", buttonEventName(BUTTON_NONE));
  TEST_ASSERT_EQUAL_STRING("short", buttonEventName(BUTTON_SHORT));
  TEST_ASSERT_EQUAL_STRING("long", buttonEventName(BUTTON_LONG));
  TEST_ASSERT_EQUAL_STRING("double", buttonEventName(BUTTON_DOUBLE));
  TEST_ASSERT_NOT_NULL(buttonEventName((ButtonEvent)99));
}

static void a_null_button_does_not_crash(void) {
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, buttonFeed(NULL, NULL, true, 0));
  buttonDefaults(NULL);
  accelerationDefaults(NULL);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(the_first_reading_never_moves_the_dial);
  RUN_TEST(one_click_clockwise_is_one_step_up);
  RUN_TEST(one_click_anticlockwise_is_one_step_down);
  RUN_TEST(a_reversed_encoder_counts_the_other_way);
  RUN_TEST(the_optical_encoder_needs_more_edges_for_one_click);
  RUN_TEST(a_line_that_does_not_move_reports_nothing);
  RUN_TEST(both_lines_jumping_at_once_is_ignored);
  RUN_TEST(turning_back_and_forth_ends_where_it_started);
  RUN_TEST(ten_clicks_are_ten_steps_and_not_nine);
  RUN_TEST(a_null_encoder_does_not_crash);

  RUN_TEST(a_slow_turn_moves_one_step_at_a_time);
  RUN_TEST(a_spin_moves_further_per_click);
  RUN_TEST(every_threshold_is_tested_on_both_sides);
  RUN_TEST(acceleration_survives_the_millisecond_wrap);
  RUN_TEST(a_null_accelerator_still_returns_one_step);

  RUN_TEST(without_double_watching_a_press_reports_as_soon_as_it_ends);
  RUN_TEST(without_double_watching_two_quick_taps_are_two_presses);
  RUN_TEST(without_double_watching_a_long_press_is_still_long);
  RUN_TEST(a_tap_is_a_short_press);
  RUN_TEST(a_hold_is_a_long_press);
  RUN_TEST(a_long_press_is_reported_once_not_repeatedly);
  RUN_TEST(two_taps_are_a_double_press);
  RUN_TEST(two_taps_far_apart_are_two_short_presses);
  RUN_TEST(a_press_on_the_long_boundary_is_long);
  RUN_TEST(a_bouncing_contact_is_still_one_press);
  RUN_TEST(a_button_survives_the_millisecond_wrap);
  RUN_TEST(event_names_are_never_null);
  RUN_TEST(a_null_button_does_not_crash);

  return UNITY_END();
}
