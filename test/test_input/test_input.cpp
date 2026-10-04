/* Tests for the knob and button state machines. Runs on a PC. */
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

static int8_t turnUp(Encoder *e) {
  int8_t total = 0;
  total = (int8_t)(total + encoderFeed(e, true, false));
  total = (int8_t)(total + encoderFeed(e, true, true));
  total = (int8_t)(total + encoderFeed(e, false, true));
  total = (int8_t)(total + encoderFeed(e, false, false));
  return total;
}

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
   * every press wait to find out whether a second one is coming, or quick
   * presses would be lost. */
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

/* A press marked handled while held, as a turn of the held knob marks it,
 * reports nothing after: no long press however long it is held, and nothing
 * when it is let go. */
static void a_handled_press_reports_nothing_after(void) {
  buttonDefaults(&btnCfg);
  uint32_t t = 1000;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(true, &t, 200));
  btn.handled = true;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(true, &t, 2000));
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(false, &t, btnCfg.debounceMs + 10));
}

/* A press already down at start, as the knob press that woke the radio,
 * reports nothing however long it is held; the next press is a normal one. */
static void a_press_held_from_the_start_reports_nothing(void) {
  buttonDefaults(&btnCfg);
  memset(&btn, 0, sizeof(btn));
  uint32_t t = 1000;
  buttonStartHeld(&btn, t);
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(true, &t, 2000));
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(false, &t, btnCfg.debounceMs + 10));
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, hold(true, &t, 100));
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT, hold(false, &t, btnCfg.debounceMs + 10));
  buttonStartHeld(NULL, 0);
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

/* ----------------------------------------------------------- keypad hold */

static ButtonEvent keyHold(KeypadHold *h, const ButtonConfig *cfg, int8_t down,
                           uint32_t *t, uint32_t forMs, int8_t *keyOut) {
  ButtonEvent seen = BUTTON_NONE;
  int8_t seenKey = -1;
  for (uint32_t i = 0; i < forMs; i += 5) {
    int8_t k = -1;
    ButtonEvent e = keypadHoldFeed(h, cfg, down, *t, &k);
    if (e != BUTTON_NONE && seen == BUTTON_NONE) {
      seen = e;
      seenKey = k;
    }
    *t += 5;
  }
  if (keyOut != NULL) {
    *keyOut = seenKey;
  }
  return seen;
}

static void a_keypad_tap_is_a_short_press(void) {
  ButtonConfig cfg;
  buttonDefaults(&cfg);
  KeypadHold h;
  memset(&h, 0, sizeof(h));
  uint32_t t = 1000;
  int8_t key = -1;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keyHold(&h, &cfg, 5, &t, 100, &key));
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT, keyHold(&h, &cfg, -1, &t, 500, &key));
  TEST_ASSERT_EQUAL_INT8(5, key);
}

static void a_keypad_hold_is_a_long_press(void) {
  ButtonConfig cfg;
  buttonDefaults(&cfg);
  KeypadHold h;
  memset(&h, 0, sizeof(h));
  uint32_t t = 1000;
  int8_t key = -1;
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, keyHold(&h, &cfg, 9, &t, 800, &key));
  TEST_ASSERT_EQUAL_INT8(9, key);
}

static void a_keypad_long_press_is_reported_once_not_repeatedly(void) {
  ButtonConfig cfg;
  buttonDefaults(&cfg);
  KeypadHold h;
  memset(&h, 0, sizeof(h));
  uint32_t t = 1000;
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, keyHold(&h, &cfg, 9, &t, 800, NULL));
  /* Still held, for another two seconds. */
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keyHold(&h, &cfg, 9, &t, 2000, NULL));
  /* And letting go of a long press is not also a short press. */
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keyHold(&h, &cfg, -1, &t, 800, NULL));
}

static void a_keypad_press_on_the_long_boundary_is_long(void) {
  ButtonConfig cfg;
  buttonDefaults(&cfg);
  KeypadHold h;
  memset(&h, 0, sizeof(h));
  uint32_t t = 1000;
  int8_t key = -1;
  /* The first call only takes up the watch on key 7; a Button starts
   * already watching its one physical switch, but this has to learn which
   * key it is watching first. The actual press begins on the call after. */
  keypadHoldFeed(&h, &cfg, 7, t, &key);
  keypadHoldFeed(&h, &cfg, 7, t, &key);
  t += cfg.debounceMs; /* The press is only seen once it has settled. */
  keypadHoldFeed(&h, &cfg, 7, t, &key);
  uint32_t pressedAt = t;
  /* Every millisecond up to but not including the boundary is not yet a
   * long press. */
  while (t + 1 < pressedAt + cfg.longMs) {
    t++;
    TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keypadHoldFeed(&h, &cfg, 7, t, &key));
  }
  t = pressedAt + cfg.longMs;
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, keypadHoldFeed(&h, &cfg, 7, t, &key));
  TEST_ASSERT_EQUAL_INT8(7, key);
}

static void a_second_key_ends_the_watch_as_a_release(void) {
  /* A different key appearing while the first is still watched is the same
   * ambiguity the keypad driver already refuses to answer, so it is not
   * folded into a guess at which key was meant. It ends the watch as a
   * release, reporting the first key's tap once buttonFeed's own debounce
   * says it has actually let go, and the second key starts its own watch
   * only from there. */
  ButtonConfig cfg;
  buttonDefaults(&cfg);
  KeypadHold h;
  memset(&h, 0, sizeof(h));
  uint32_t t = 1000;
  int8_t key = -1;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keyHold(&h, &cfg, 5, &t, 100, &key));
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT, keyHold(&h, &cfg, 9, &t, 200, &key));
  TEST_ASSERT_EQUAL_INT8(5, key);

  /* The new key's watch continues from wherever the swap left it. */
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, keyHold(&h, &cfg, 9, &t, 800, &key));
  TEST_ASSERT_EQUAL_INT8(9, key);
}

static void two_keypad_taps_are_a_double_press(void) {
  ButtonConfig cfg;
  buttonDefaults(&cfg);
  cfg.wantDouble = true;
  KeypadHold h;
  memset(&h, 0, sizeof(h));
  uint32_t t = 1000;
  int8_t key = -1;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keyHold(&h, &cfg, 5, &t, 60, &key));
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keyHold(&h, &cfg, -1, &t, 100, &key));
  TEST_ASSERT_EQUAL_INT(BUTTON_DOUBLE, keyHold(&h, &cfg, 5, &t, 60, &key));
  TEST_ASSERT_EQUAL_INT8(5, key);
  /* And the second tap of the pair does not then also report a short. */
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keyHold(&h, &cfg, -1, &t, 600, &key));
}

static void a_keypad_tap_with_no_second_one_is_still_a_short_press(void) {
  /* wantDouble holds a single tap back to see if a second one follows.
   * With none coming, it still has to resolve to a short press rather than
   * being lost while the watch is held open for it. */
  ButtonConfig cfg;
  buttonDefaults(&cfg);
  cfg.wantDouble = true;
  KeypadHold h;
  memset(&h, 0, sizeof(h));
  uint32_t t = 1000;
  int8_t key = -1;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keyHold(&h, &cfg, 5, &t, 60, &key));
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT,
                        keyHold(&h, &cfg, -1, &t, cfg.doubleMs + 50, &key));
  TEST_ASSERT_EQUAL_INT8(5, key);
}

static void nothing_down_never_reports_anything(void) {
  ButtonConfig cfg;
  buttonDefaults(&cfg);
  KeypadHold h;
  memset(&h, 0, sizeof(h));
  uint32_t t = 1000;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keyHold(&h, &cfg, -1, &t, 5000, NULL));
}

static void a_keypad_hold_survives_the_millisecond_wrap(void) {
  ButtonConfig cfg;
  buttonDefaults(&cfg);
  KeypadHold h;
  memset(&h, 0, sizeof(h));
  uint32_t t = 0xFFFFFF00UL;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keyHold(&h, &cfg, 3, &t, 100, NULL));
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT, keyHold(&h, &cfg, -1, &t, 500, NULL));
  /* t has now wrapped past zero. A long press still works on the far
   * side. */
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, keyHold(&h, &cfg, 3, &t, 800, NULL));
}

static void a_null_keypad_hold_does_not_crash(void) {
  int8_t key = 99;
  TEST_ASSERT_EQUAL_INT(BUTTON_NONE, keypadHoldFeed(NULL, NULL, 5, 0, &key));
  TEST_ASSERT_EQUAL_INT8(-1, key);
}

/* -------------------------------------------------------------------- pot */

static void the_bottom_of_the_travel_is_silent(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  TEST_ASSERT_EQUAL_INT8(cfg.dbMute, potVolumeDb(0, NULL));
  TEST_ASSERT_EQUAL_INT8(cfg.dbMute, potVolumeDb(cfg.rawMute, NULL));
}

static void the_top_of_the_travel_is_the_loudest(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  TEST_ASSERT_EQUAL_INT8(cfg.dbMax, potVolumeDb(cfg.rawMax, NULL));
  /* Past the end of the travel, which a real pot reaches, stays at the top
   * rather than running past it. */
  TEST_ASSERT_EQUAL_INT8(cfg.dbMax, potVolumeDb(4095, NULL));
}

static void leaving_the_mute_zone_does_not_jump(void) {
  /* The quiet end of the line is the mute level itself, so the first count
   * above the mute zone is still -60 dB, not a step up to -30 dB. */
  PotConfig cfg;
  potDefaults(&cfg);
  TEST_ASSERT_EQUAL_INT8(-60, cfg.dbMute);
  TEST_ASSERT_EQUAL_INT8(cfg.dbMute, cfg.dbMin);
  TEST_ASSERT_EQUAL_INT8(cfg.dbMute,
                         potVolumeDb((uint16_t)(cfg.rawMute + 1), NULL));
  TEST_ASSERT_EQUAL_INT8(cfg.dbMin, potVolumeDb(cfg.rawMin, NULL));
}

/* The first 10% of the travel covers -60 to -30 dB, the other 90% -30 to
 * 0 dB. */
static void the_first_tenth_of_the_travel_reaches_minus_30(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  const int32_t width = cfg.rawMax - cfg.rawMin;
  const int32_t knee = width * 10 / 100;
  TEST_ASSERT_EQUAL_INT8(10, cfg.kneePercent);
  TEST_ASSERT_EQUAL_INT8(-45,
                         potVolumeDb((uint16_t)(cfg.rawMin + knee / 2), NULL));
  TEST_ASSERT_EQUAL_INT8(-30, potVolumeDb((uint16_t)(cfg.rawMin + knee), NULL));
  TEST_ASSERT_EQUAL_INT8(
      -15,
      potVolumeDb((uint16_t)(cfg.rawMin + knee + (width - knee) / 2), NULL));
}

/* No jump where the two lines meet: one count either side of the knee is one
 * dB apart at most. */
static void the_knee_has_no_step(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  const uint16_t knee = (uint16_t)(cfg.rawMin + (cfg.rawMax - cfg.rawMin) *
                                                    cfg.kneePercent / 100);
  const int8_t below = potVolumeDb((uint16_t)(knee - 1), NULL);
  const int8_t at = potVolumeDb(knee, NULL);
  TEST_ASSERT_TRUE(at - below >= 0 && at - below <= 1);
}

/* A knee at 0% is one straight line from the knee's level, and a knee that
 * fills the travel still reaches the loud end at the top. */
static void a_knee_at_either_end_still_works(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  cfg.kneePercent = 0;
  cfg.dbKnee = cfg.dbMin;
  TEST_ASSERT_EQUAL_INT8(cfg.dbMin, potVolumeDb(cfg.rawMin, &cfg));
  TEST_ASSERT_EQUAL_INT8(
      -30, potVolumeDb((uint16_t)((cfg.rawMin + cfg.rawMax) / 2), &cfg));
  potDefaults(&cfg);
  cfg.kneePercent = 100;
  TEST_ASSERT_EQUAL_INT8(cfg.dbMax, potVolumeDb(cfg.rawMax, &cfg));
}

static void the_volume_only_ever_goes_up_as_the_knob_does(void) {
  int8_t last = -127;
  for (uint16_t raw = 0; raw < 4095; raw = (uint16_t)(raw + 37)) {
    int8_t db = potVolumeDb(raw, NULL);
    TEST_ASSERT_TRUE(db >= last);
    last = db;
  }
}

static void a_reading_that_only_jitters_is_ignored(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  TEST_ASSERT_FALSE(potMoved(2000, 2000, NULL));
  TEST_ASSERT_FALSE(potMoved(2000, (uint16_t)(2000 + cfg.deadband - 1), NULL));
  TEST_ASSERT_FALSE(potMoved(2000, (uint16_t)(2000 - cfg.deadband + 1), NULL));
  /* Exactly the deadband counts as movement, in both directions. */
  TEST_ASSERT_TRUE(potMoved(2000, (uint16_t)(2000 + cfg.deadband), NULL));
  TEST_ASSERT_TRUE(potMoved(2000, (uint16_t)(2000 - cfg.deadband), NULL));
}

static void a_pot_wired_the_other_way_does_not_divide_by_zero(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  cfg.rawMax = cfg.rawMin;
  TEST_ASSERT_EQUAL_INT8(cfg.dbMax, potVolumeDb(2000, &cfg));
}

static void a_null_pot_config_uses_the_defaults(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  potDefaults(NULL); /* Must not crash. */
  /* Compared against the defaults, not against itself. */
  for (uint16_t raw = 0; raw < 4095; raw = (uint16_t)(raw + 401)) {
    TEST_ASSERT_EQUAL_INT8(potVolumeDb(raw, &cfg), potVolumeDb(raw, NULL));
  }
  TEST_ASSERT_EQUAL_INT(potMoved(1000, 1030, &cfg), potMoved(1000, 1030, NULL));
}

/* --------------------------------------------------- pot calibration */

static void a_sweep_is_kept_and_moves_the_ends(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  PotCalibration c;
  memset(&c, 0, sizeof(c));

  potCalibrateStart(&c, 2000, 1000);
  TEST_ASSERT_TRUE(potCalibrateSample(&c, 250, 1100));
  TEST_ASSERT_TRUE(potCalibrateSample(&c, 3800, 1200));
  TEST_ASSERT_TRUE(potCalibrateSample(&c, 2000, 1300));
  TEST_ASSERT_TRUE(potCalibrateFinish(&c, &cfg));

  TEST_ASSERT_EQUAL_UINT16(3800, cfg.rawMax);
  /* The mute zone moves with the quiet end, or a knob that starts at 900
   * would never reach the built in 100 and could not switch the radio off. */
  TEST_ASSERT_TRUE(cfg.rawMute > 250);
  TEST_ASSERT_TRUE(cfg.rawMin > cfg.rawMute);

  /* And the whole travel is usable afterwards. */
  TEST_ASSERT_EQUAL_INT8(cfg.dbMute, potVolumeDb(250, &cfg));
  TEST_ASSERT_EQUAL_INT8(cfg.dbMax, potVolumeDb(3800, &cfg));
}

static void the_mute_zone_stays_a_zone_not_a_point(void) {
  /* An ADC at rest wanders by a few counts. A mute that needed one exact
   * reading would almost never fire, and the knob could not switch the radio
   * off. The zone keeps the same share of the travel the built in figures
   * use, the bottom 2.5 per cent. */
  PotConfig cfg;
  potDefaults(&cfg);
  potApplyCalibration(&cfg, 0, 4095);

  TEST_ASSERT_TRUE(cfg.rawMute >= 80);
  /* Every reading across the bottom of the travel mutes, not just one. */
  for (uint16_t raw = 0; raw <= 80; raw = (uint16_t)(raw + 8)) {
    TEST_ASSERT_EQUAL_INT8(cfg.dbMute, potVolumeDb(raw, &cfg));
  }
  /* And the top of the travel is still the loud end. */
  TEST_ASSERT_EQUAL_INT8(cfg.dbMax, potVolumeDb(4095, &cfg));

  /* A knob that does not start at zero keeps a zone of its own. */
  potDefaults(&cfg);
  potApplyCalibration(&cfg, 900, 3900);
  TEST_ASSERT_EQUAL_INT8(cfg.dbMute, potVolumeDb(900, &cfg));
  TEST_ASSERT_EQUAL_INT8(cfg.dbMute, potVolumeDb(940, &cfg));
  TEST_ASSERT_EQUAL_INT8(cfg.dbMax, potVolumeDb(3900, &cfg));

  /* Ends that say nothing change nothing. */
  PotConfig before;
  potDefaults(&before);
  PotConfig same = before;
  potApplyCalibration(&same, 3000, 100);
  TEST_ASSERT_EQUAL_UINT16(before.rawMute, same.rawMute);
  potApplyCalibration(NULL, 0, 4095);
}

static void cancelling_really_cancels(void) {
  /* Cancelling has to forget the sweep, not just stop it. A finish afterwards
   * must not apply the extremes the cancelled sweep recorded. */
  PotConfig cfg;
  potDefaults(&cfg);
  PotConfig before = cfg;
  PotCalibration c;
  memset(&c, 0, sizeof(c));

  potCalibrateStart(&c, 2000, 1000);
  potCalibrateSample(&c, 250, 1100);
  potCalibrateSample(&c, 3800, 1200);
  potCalibrateCancel(&c);

  TEST_ASSERT_FALSE(potCalibrateFinish(&c, &cfg));
  TEST_ASSERT_EQUAL_UINT16(before.rawMin, cfg.rawMin);
  TEST_ASSERT_EQUAL_UINT16(before.rawMax, cfg.rawMax);
  TEST_ASSERT_EQUAL_UINT16(before.rawMute, cfg.rawMute);
}

static void finishing_without_starting_changes_nothing(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  PotConfig before = cfg;
  PotCalibration c;
  memset(&c, 0, sizeof(c));
  TEST_ASSERT_FALSE(potCalibrateFinish(&c, &cfg));
  TEST_ASSERT_EQUAL_UINT16(before.rawMin, cfg.rawMin);
}

static void a_knob_that_barely_moved_is_refused(void) {
  /* Storing it would leave the radio with almost no usable travel and
   * nothing to say why. */
  PotConfig cfg;
  potDefaults(&cfg);
  PotConfig before = cfg;
  PotCalibration c;
  memset(&c, 0, sizeof(c));

  potCalibrateStart(&c, 2000, 1000);
  potCalibrateSample(&c, 1990, 1100);
  potCalibrateSample(&c, 2010, 1200);
  TEST_ASSERT_FALSE(potCalibrateFinish(&c, &cfg));
  TEST_ASSERT_EQUAL_UINT16(before.rawMin, cfg.rawMin);
}

static void one_left_running_gives_up_on_its_own(void) {
  /* While a calibration runs the knob sets neither the volume nor the
   * squelch, so one started and walked away from would leave the radio with
   * no working volume control and nothing on it to say why. */
  PotCalibration c;
  memset(&c, 0, sizeof(c));
  potCalibrateStart(&c, 2000, 1000);
  TEST_ASSERT_TRUE(potCalibrateSample(&c, 2100, 1000 + 1000));
  TEST_ASSERT_TRUE(
      potCalibrateSample(&c, 2100, 1000 + POT_CALIBRATE_TIMEOUT_MS - 1));
  TEST_ASSERT_FALSE(
      potCalibrateSample(&c, 2100, 1000 + POT_CALIBRATE_TIMEOUT_MS));
  TEST_ASSERT_FALSE(c.active);

  /* And a timed out one cannot then be finished. */
  PotConfig cfg;
  potDefaults(&cfg);
  PotConfig before = cfg;
  TEST_ASSERT_FALSE(potCalibrateFinish(&c, &cfg));
  TEST_ASSERT_EQUAL_UINT16(before.rawMax, cfg.rawMax);
}

static void the_timeout_survives_the_millisecond_wrap(void) {
  PotCalibration c;
  memset(&c, 0, sizeof(c));
  uint32_t start = 0xFFFFFF00UL;
  potCalibrateStart(&c, 2000, start);
  TEST_ASSERT_TRUE(potCalibrateSample(&c, 2100, start + 1000));
  TEST_ASSERT_FALSE(
      potCalibrateSample(&c, 2100, start + POT_CALIBRATE_TIMEOUT_MS));
}

static void nothing_crashes_on_a_null_calibration(void) {
  PotConfig cfg;
  potDefaults(&cfg);
  potCalibrateStart(NULL, 0, 0);
  TEST_ASSERT_FALSE(potCalibrateSample(NULL, 0, 0));
  TEST_ASSERT_FALSE(potCalibrateFinish(NULL, &cfg));
  potCalibrateCancel(NULL);

  PotCalibration c;
  memset(&c, 0, sizeof(c));
  potCalibrateStart(&c, 100, 0);
  TEST_ASSERT_FALSE(potCalibrateFinish(&c, NULL));
}

/* ------------------------------------------------------------- knob steps */

static void a_list_takes_one_click_as_one_step_however_fast(void) {
  TEST_ASSERT_EQUAL_INT32(3, inputKnobSteps(3, 25, true));
  TEST_ASSERT_EQUAL_INT32(-3, inputKnobSteps(-3, 25, true));
  TEST_ASSERT_EQUAL_INT32(1, inputKnobSteps(1, 1, true));
}

static void the_dial_multiplies_the_clicks_by_the_speed(void) {
  TEST_ASSERT_EQUAL_INT32(75, inputKnobSteps(3, 25, false));
  TEST_ASSERT_EQUAL_INT32(-75, inputKnobSteps(-3, 25, false));
  TEST_ASSERT_EQUAL_INT32(2, inputKnobSteps(2, 1, false));
}

static void knob_steps_are_held_to_what_the_queue_carries(void) {
  TEST_ASSERT_EQUAL_INT32(INPUT_KNOB_MAX_STEPS, inputKnobSteps(100, 25, false));
  TEST_ASSERT_EQUAL_INT32(-INPUT_KNOB_MAX_STEPS,
                          inputKnobSteps(-100, 25, false));
  TEST_ASSERT_EQUAL_INT32(INPUT_KNOB_MAX_STEPS, inputKnobSteps(5000, 1, true));
  TEST_ASSERT_EQUAL_INT32(0, inputKnobSteps(0, 25, false));
}

/* --------------------------------------------------- POST /api/key names */

static void every_key_name_reads_back_as_its_key(void) {
  for (int k = 0; k < INPUT_KEY_COUNT; k++) {
    InputKey back = INPUT_KEY_COUNT;
    TEST_ASSERT_TRUE(inputKeyFromName(inputKeyName((InputKey)k), &back));
    TEST_ASSERT_EQUAL_INT(k, back);
  }
  InputKey key = INPUT_KEY_COUNT;
  TEST_ASSERT_TRUE(inputKeyFromName("7", &key));
  TEST_ASSERT_EQUAL_INT(INPUT_KEY_DIGIT_0 + 7, key);
  TEST_ASSERT_EQUAL_STRING("MODE", inputKeyName(INPUT_KEY_MODE));
  TEST_ASSERT_EQUAL_STRING("?", inputKeyName(INPUT_KEY_COUNT));
}

/* Exact names only: a lower case one or one with more after it is refused
 * rather than guessed at. */
static void a_name_that_is_no_key_is_refused(void) {
  InputKey key = INPUT_KEY_BAND;
  TEST_ASSERT_FALSE(inputKeyFromName("mode", &key));
  TEST_ASSERT_FALSE(inputKeyFromName("MODE ", &key));
  TEST_ASSERT_FALSE(inputKeyFromName("10", &key));
  TEST_ASSERT_FALSE(inputKeyFromName("", &key));
  TEST_ASSERT_FALSE(inputKeyFromName(NULL, &key));
  TEST_ASSERT_FALSE(inputKeyFromName("BAND", NULL));
  TEST_ASSERT_EQUAL_INT(INPUT_KEY_BAND, key);
}

static void only_short_and_long_are_events_to_send(void) {
  ButtonEvent e = BUTTON_NONE;
  TEST_ASSERT_TRUE(buttonEventFromName("short", &e));
  TEST_ASSERT_EQUAL_INT(BUTTON_SHORT, e);
  TEST_ASSERT_TRUE(buttonEventFromName("long", &e));
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, e);
  TEST_ASSERT_FALSE(buttonEventFromName("double", &e));
  TEST_ASSERT_FALSE(buttonEventFromName("none", &e));
  TEST_ASSERT_FALSE(buttonEventFromName(NULL, &e));
  TEST_ASSERT_FALSE(buttonEventFromName("short", NULL));
  TEST_ASSERT_EQUAL_INT(BUTTON_LONG, e);
}

/* The buttons and ENTER have a hold; a digit and DX act as they go down. */
static void only_the_buttons_and_enter_have_a_long_press(void) {
  TEST_ASSERT_TRUE(inputKeyTakesLong(INPUT_KEY_BAND));
  TEST_ASSERT_TRUE(inputKeyTakesLong(INPUT_KEY_BW));
  TEST_ASSERT_TRUE(inputKeyTakesLong(INPUT_KEY_MODE));
  TEST_ASSERT_TRUE(inputKeyTakesLong(INPUT_KEY_PUSH));
  TEST_ASSERT_TRUE(inputKeyTakesLong(INPUT_KEY_ENTER));
  TEST_ASSERT_FALSE(inputKeyTakesLong(INPUT_KEY_DX));
  TEST_ASSERT_FALSE(inputKeyTakesLong(INPUT_KEY_DIGIT_0));
  TEST_ASSERT_FALSE(inputKeyTakesLong((InputKey)(INPUT_KEY_DIGIT_0 + 9)));
}

/* The back gestures go quiet for a moment after the menu shuts, and only
 * then; the time is read the right way across the counter's wrap. */
static void back_gestures_rest_just_after_the_menu_shuts(void) {
  TEST_ASSERT_FALSE(inputAfterMenuQuiet(false, 0, 0));
  TEST_ASSERT_TRUE(inputAfterMenuQuiet(true, 1000, 1000));
  TEST_ASSERT_TRUE(
      inputAfterMenuQuiet(true, 1000, 1000 + INPUT_AFTER_MENU_QUIET_MS - 1));
  TEST_ASSERT_FALSE(
      inputAfterMenuQuiet(true, 1000, 1000 + INPUT_AFTER_MENU_QUIET_MS));
  TEST_ASSERT_TRUE(inputAfterMenuQuiet(true, UINT32_MAX - 100, 200));
  TEST_ASSERT_FALSE(inputAfterMenuQuiet(true, UINT32_MAX - 100, 2000));
}

/* A page goes back after a minute with no input, not before; the time is
 * read the right way across the counter's wrap. */
static void a_page_left_alone_for_a_minute_is_idle(void) {
  TEST_ASSERT_FALSE(inputPageIdle(1000, 1000));
  TEST_ASSERT_FALSE(inputPageIdle(1000, 1000 + INPUT_PAGE_IDLE_MS - 1));
  TEST_ASSERT_TRUE(inputPageIdle(1000, 1000 + INPUT_PAGE_IDLE_MS));
  TEST_ASSERT_FALSE(inputPageIdle(UINT32_MAX - 100, 200));
  TEST_ASSERT_TRUE(inputPageIdle(UINT32_MAX - 100, INPUT_PAGE_IDLE_MS));
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
  RUN_TEST(a_handled_press_reports_nothing_after);
  RUN_TEST(a_press_held_from_the_start_reports_nothing);
  RUN_TEST(a_bouncing_contact_is_still_one_press);
  RUN_TEST(a_button_survives_the_millisecond_wrap);
  RUN_TEST(event_names_are_never_null);
  RUN_TEST(a_null_button_does_not_crash);

  RUN_TEST(a_keypad_tap_is_a_short_press);
  RUN_TEST(a_keypad_hold_is_a_long_press);
  RUN_TEST(a_keypad_long_press_is_reported_once_not_repeatedly);
  RUN_TEST(a_keypad_press_on_the_long_boundary_is_long);
  RUN_TEST(a_second_key_ends_the_watch_as_a_release);
  RUN_TEST(two_keypad_taps_are_a_double_press);
  RUN_TEST(a_keypad_tap_with_no_second_one_is_still_a_short_press);
  RUN_TEST(nothing_down_never_reports_anything);
  RUN_TEST(a_keypad_hold_survives_the_millisecond_wrap);
  RUN_TEST(a_null_keypad_hold_does_not_crash);

  RUN_TEST(the_bottom_of_the_travel_is_silent);
  RUN_TEST(the_top_of_the_travel_is_the_loudest);
  RUN_TEST(leaving_the_mute_zone_does_not_jump);
  RUN_TEST(the_first_tenth_of_the_travel_reaches_minus_30);
  RUN_TEST(the_knee_has_no_step);
  RUN_TEST(a_knee_at_either_end_still_works);
  RUN_TEST(the_volume_only_ever_goes_up_as_the_knob_does);
  RUN_TEST(a_reading_that_only_jitters_is_ignored);
  RUN_TEST(a_pot_wired_the_other_way_does_not_divide_by_zero);
  RUN_TEST(a_null_pot_config_uses_the_defaults);

  RUN_TEST(a_sweep_is_kept_and_moves_the_ends);
  RUN_TEST(the_mute_zone_stays_a_zone_not_a_point);
  RUN_TEST(cancelling_really_cancels);
  RUN_TEST(finishing_without_starting_changes_nothing);
  RUN_TEST(a_knob_that_barely_moved_is_refused);
  RUN_TEST(one_left_running_gives_up_on_its_own);
  RUN_TEST(the_timeout_survives_the_millisecond_wrap);
  RUN_TEST(nothing_crashes_on_a_null_calibration);
  RUN_TEST(a_list_takes_one_click_as_one_step_however_fast);
  RUN_TEST(the_dial_multiplies_the_clicks_by_the_speed);
  RUN_TEST(knob_steps_are_held_to_what_the_queue_carries);
  RUN_TEST(every_key_name_reads_back_as_its_key);
  RUN_TEST(a_name_that_is_no_key_is_refused);
  RUN_TEST(only_short_and_long_are_events_to_send);
  RUN_TEST(only_the_buttons_and_enter_have_a_long_press);
  RUN_TEST(back_gestures_rest_just_after_the_menu_shuts);
  RUN_TEST(a_page_left_alone_for_a_minute_is_idle);

  return UNITY_END();
}
