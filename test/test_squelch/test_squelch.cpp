/**
 * @file test_squelch.cpp
 * @brief Tests for the squelch. Runs on a PC.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/squelch.h"

static Squelch sq;
static SquelchConfig cfg;

void setUp(void) {
  squelchInit(&sq);
  squelchDefaults(&cfg);
}
void tearDown(void) {}

/** A reading good enough to listen to on either band. */
static SquelchReading good(void) {
  SquelchReading r;
  r.valid = true;
  r.levelTenths = 400;
  r.noiseTenths = 50;
  r.multipathTenths = 100;
  r.offsetTenths = 5;
  return r;
}

/** A reading that is nothing but noise. */
static SquelchReading bad(void) {
  SquelchReading r;
  r.valid = true;
  r.levelTenths = 20;
  r.noiseTenths = 900;
  r.multipathTenths = 800;
  r.offsetTenths = 300;
  return r;
}

static bool update(SquelchMode mode, BandId band, SquelchReading r,
                   int16_t threshold, uint32_t nowMs) {
  return squelchUpdate(&sq, &cfg, mode, band, &r, threshold, nowMs);
}

/* -------------------------------------------------------------------- off */

static void off_is_always_open(void) {
  TEST_ASSERT_TRUE(update(SQUELCH_OFF, BAND_FM, bad(), 0, 1000));
  TEST_ASSERT_TRUE(update(SQUELCH_OFF, BAND_MW, bad(), 900, 2000));
  /* Even a reading that never arrived. */
  SquelchReading none = bad();
  none.valid = false;
  TEST_ASSERT_TRUE(update(SQUELCH_OFF, BAND_FM, none, 0, 3000));
}

/* ------------------------------------------------------------------- auto */

static void auto_opens_on_a_clean_signal(void) {
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, good(), 0, 1000));
}

static void auto_shuts_on_noise_after_the_hold(void) {
  uint32_t t = 1000;
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, good(), 0, t));

  /* It does not shut the instant the reading goes bad. */
  t += 10;
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, bad(), 0, t));
  t += cfg.holdMs - 20;
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, bad(), 0, t));

  /* It shuts once the bad reading has lasted the whole hold. */
  t += 30;
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_FM, bad(), 0, t));
}

static void a_signal_on_the_edge_does_not_chatter(void) {
  /* The case the hold exists for. A reading flapping either side of the
   * limit must not switch the audio on and off several times a second. */
  uint32_t t = 1000;
  update(SQUELCH_AUTO, BAND_FM, good(), 0, t);

  int opens = 0;
  bool last = true;
  for (int i = 0; i < 40; i++) {
    t += 50;
    bool open = update(SQUELCH_AUTO, BAND_FM, (i % 2) ? bad() : good(), 0, t);
    if (open != last) {
      opens++;
      last = open;
    }
  }
  /* Never shuts at all, because a good reading keeps arriving inside the
   * hold and opening is immediate. */
  TEST_ASSERT_EQUAL_INT(0, opens);
}

static void auto_opens_again_as_soon_as_the_signal_comes_back(void) {
  /* Opening is not held. Waiting to open would clip the front of every
   * station as the dial crosses it. */
  uint32_t t = 1000;
  update(SQUELCH_AUTO, BAND_FM, good(), 0, t);
  t += 10;
  update(SQUELCH_AUTO, BAND_FM, bad(), 0, t);
  t += cfg.holdMs;
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_FM, bad(), 0, t));
  t += 50;
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, good(), 0, t));
}

static void am_is_not_judged_on_multipath(void) {
  /* The tuner does not report multipath on the AM side, so that field holds
   * something else there and must not be tested against. This reading would
   * fail the FM rule on multipath alone. */
  SquelchReading r = good();
  r.multipathTenths = 9999;
  /* Through the hold, or the shut answer would be the hold speaking and not
   * the rule. This test passed for that wrong reason once. */
  update(SQUELCH_AUTO, BAND_FM, good(), 0, 1000);
  update(SQUELCH_AUTO, BAND_FM, r, 0, 1010);
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_FM, r, 0, 1010 + cfg.holdMs));

  /* The same reading on the AM side stays open however long it is fed in,
   * because multipath is not part of the AM rule. */
  squelchInit(&sq);
  update(SQUELCH_AUTO, BAND_MW, r, 0, 1000);
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_MW, r, 0, 1000 + 2 * cfg.holdMs));
}

static void the_two_bands_allow_different_offsets(void) {
  /* Five kHz off centre is fine on FM and far too much on medium wave, where
   * the channels are 9 kHz apart. */
  SquelchReading r = good();
  r.offsetTenths = 50;
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, r, 0, 1000));

  squelchInit(&sq);
  update(SQUELCH_AUTO, BAND_MW, good(), 0, 1000);
  /* Two bad readings: the first starts the hold, the second comes after it
   * has run out. One is not enough, by design. */
  update(SQUELCH_AUTO, BAND_MW, r, 0, 1010);
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_MW, r, 0, 1010 + cfg.holdMs));
}

static void the_offset_is_judged_either_side_of_centre(void) {
  SquelchReading r = good();
  r.offsetTenths = -300;
  update(SQUELCH_AUTO, BAND_FM, good(), 0, 1000);
  update(SQUELCH_AUTO, BAND_FM, r, 0, 1010);
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_FM, r, 0, 1010 + cfg.holdMs));
}

static void every_auto_threshold_is_tested_on_its_boundary(void) {
  struct {
    const char *what;
    BandId band;
    uint16_t noise;
    uint16_t multipath;
    int16_t offset;
    bool want;
  } cases[] = {
      {"fm noise just under", BAND_FM, 119, 100, 0, true},
      {"fm noise exactly on", BAND_FM, 120, 100, 0, false},
      {"fm multipath just under", BAND_FM, 50, 229, 0, true},
      {"fm multipath exactly on", BAND_FM, 50, 230, 0, false},
      {"fm offset just under", BAND_FM, 50, 100, 99, true},
      {"fm offset exactly on", BAND_FM, 50, 100, 100, false},
      {"am noise just under", BAND_MW, 119, 0, 0, true},
      {"am noise exactly on", BAND_MW, 120, 0, 0, false},
      {"am offset just under", BAND_MW, 50, 0, 19, true},
      {"am offset exactly on", BAND_MW, 50, 0, 20, false},
  };

  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    squelchInit(&sq);
    SquelchReading r = good();
    r.noiseTenths = cases[i].noise;
    r.multipathTenths = cases[i].multipath;
    r.offsetTenths = cases[i].offset;

    /* Open first, then feed the reading past the hold, so a shut answer is
     * the rule speaking and not the hold. */
    update(SQUELCH_AUTO, cases[i].band, good(), 0, 1000);
    update(SQUELCH_AUTO, cases[i].band, r, 0, 1010);
    bool open = update(SQUELCH_AUTO, cases[i].band, r, 0, 1010 + cfg.holdMs);
    TEST_ASSERT_EQUAL_INT_MESSAGE(cases[i].want, open, cases[i].what);
  }
}

/* ----------------------------------------------------------------- manual */

static void manual_opens_when_the_signal_beats_the_threshold(void) {
  SquelchReading r = good();
  r.levelTenths = 400;
  TEST_ASSERT_TRUE(update(SQUELCH_MANUAL, BAND_FM, r, 300, 1000));
}

static void manual_shuts_when_it_does_not(void) {
  SquelchReading r = good();
  r.levelTenths = 200;
  update(SQUELCH_MANUAL, BAND_FM, good(), 0, 1000);
  update(SQUELCH_MANUAL, BAND_FM, r, 300, 1010);
  TEST_ASSERT_FALSE(update(SQUELCH_MANUAL, BAND_FM, r, 300, 1010 + cfg.holdMs));
}

static void manual_at_the_bottom_of_the_travel_is_always_open(void) {
  /* Without this there is no way to listen to a weak station on purpose. */
  SquelchReading r = bad();
  TEST_ASSERT_TRUE(update(SQUELCH_MANUAL, BAND_FM, r,
                          squelchThresholdFromPot(0, 0, 0), 1000));
}

static void a_calibrated_knob_uses_its_own_ends(void) {
  /* A pot that only reaches 200 to 3800 would otherwise sit at "always open"
   * for the first stretch of its travel and hit the ceiling before the end,
   * with nothing to say so. */
  TEST_ASSERT_EQUAL_INT16(-100, squelchThresholdFromPot(200, 200, 3800));
  TEST_ASSERT_EQUAL_INT16(920, squelchThresholdFromPot(3800, 200, 3800));

  /* Past either end is held at that end rather than running off. */
  TEST_ASSERT_EQUAL_INT16(-100, squelchThresholdFromPot(0, 200, 3800));
  TEST_ASSERT_EQUAL_INT16(920, squelchThresholdFromPot(4095, 200, 3800));

  /* Ends that say nothing fall back to the whole converter range, which is
   * what an uncalibrated radio uses. */
  TEST_ASSERT_EQUAL_INT16(squelchThresholdFromPot(2048, 0, 0),
                          squelchThresholdFromPot(2048, 3800, 200));

  /* The middle of a calibrated knob is the middle of the range. */
  int16_t mid = squelchThresholdFromPot(2000, 200, 3800);
  TEST_ASSERT_INT16_WITHIN(30, (920 - 100) / 2, mid);
}

static void the_pot_covers_the_whole_useful_range(void) {
  TEST_ASSERT_EQUAL_INT16(-100, squelchThresholdFromPot(0, 0, 0));
  TEST_ASSERT_EQUAL_INT16(920, squelchThresholdFromPot(4095, 0, 0));
  /* Past the end of the converter, which a real pot reaches. */
  TEST_ASSERT_EQUAL_INT16(920, squelchThresholdFromPot(60000, 0, 0));
  /* And it only ever goes up. */
  int16_t last = -1000;
  for (uint16_t raw = 0; raw < 4095; raw = (uint16_t)(raw + 53)) {
    int16_t t = squelchThresholdFromPot(raw, 0, 0);
    TEST_ASSERT_TRUE(t >= last);
    last = t;
  }
}

/* ---------------------------------------------------------------- general */

static void a_reading_that_failed_never_shuts_the_audio(void) {
  /* An I2C read failing says nothing about the signal. Going silent on it
   * makes a fault somewhere else into the listener's problem. */
  uint32_t t = 1000;
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, good(), 0, t));

  SquelchReading none = bad();
  none.valid = false;
  for (int i = 0; i < 20; i++) {
    t += 100;
    TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, none, 0, t));
  }
}

static void it_starts_open_rather_than_silent(void) {
  Squelch fresh;
  memset(&fresh, 0xFF, sizeof(fresh)); /* Whatever was in that memory. */
  squelchInit(&fresh);
  SquelchReading none = good();
  none.valid = false;
  TEST_ASSERT_TRUE(
      squelchUpdate(&fresh, &cfg, SQUELCH_AUTO, BAND_FM, &none, 0, 1000));
}

static void readings_that_keep_failing_open_the_audio_again(void) {
  /* The permanent silence this guards against. The squelch shuts on noise,
   * then the tuner stops answering. Staying shut for ever would make a fault
   * in one place into a radio that cannot be recovered without a reboot, and
   * there is no squelch control on the front of this radio at all. */
  uint32_t t = 1000;
  update(SQUELCH_AUTO, BAND_FM, good(), 0, t);
  t += 10;
  update(SQUELCH_AUTO, BAND_FM, bad(), 0, t);
  t += cfg.holdMs;
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_FM, bad(), 0, t));

  SquelchReading none = good();
  none.valid = false;
  /* Still shut while the failures are recent. */
  t += 100;
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_FM, none, 0, t));
  /* Open once they have gone on as long as the hold. */
  t += cfg.holdMs;
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, none, 0, t));
}

static void one_failed_reading_does_not_postpone_the_shut(void) {
  /* A read that fails in the middle of a bad patch must not cancel the hold.
   * If it did, a tuner failing one read every few hundred milliseconds would
   * keep the audio open on noise for ever. */
  uint32_t t = 1000;
  update(SQUELCH_AUTO, BAND_FM, good(), 0, t);
  t += 10;
  update(SQUELCH_AUTO, BAND_FM, bad(), 0, t);

  SquelchReading none = good();
  none.valid = false;
  t += 300;
  update(SQUELCH_AUTO, BAND_FM, none, 0, t);
  t += 300;
  update(SQUELCH_AUTO, BAND_FM, bad(), 0, t);

  /* The hold is measured from the first bad reading, not from the last
   * failure. */
  t = 1010 + cfg.holdMs;
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_FM, bad(), 0, t));
}

static void changing_band_starts_the_hold_again(void) {
  uint32_t t = 1000;
  update(SQUELCH_AUTO, BAND_FM, good(), 0, t);
  t += 10;
  update(SQUELCH_AUTO, BAND_FM, bad(), 0, t);

  /* Most of the way through the hold, then the band changes. */
  t += cfg.holdMs - 50;
  update(SQUELCH_AUTO, BAND_MW, bad(), 0, t);
  /* What was left of the old hold must not count towards the new band. */
  t += 60;
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_MW, bad(), 0, t));
  t += cfg.holdMs;
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_MW, bad(), 0, t));
}

static void it_survives_the_millisecond_wrap(void) {
  uint32_t t = 0xFFFFFF00UL;
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, good(), 0, t));
  t += 10;
  TEST_ASSERT_TRUE(update(SQUELCH_AUTO, BAND_FM, bad(), 0, t));
  /* Past the wrap, and the hold still measures the right length. */
  t += cfg.holdMs;
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_FM, bad(), 0, t));
}

static void mode_names_are_never_null(void) {
  TEST_ASSERT_EQUAL_STRING("Off", squelchModeName(SQUELCH_OFF));
  TEST_ASSERT_EQUAL_STRING("Auto", squelchModeName(SQUELCH_AUTO));
  TEST_ASSERT_EQUAL_STRING("Manual", squelchModeName(SQUELCH_MANUAL));
  TEST_ASSERT_NOT_NULL(squelchModeName((SquelchMode)99));
}

static void nulls_do_not_crash(void) {
  squelchDefaults(NULL);
  SquelchReading r = good();
  TEST_ASSERT_TRUE(squelchUpdate(NULL, NULL, SQUELCH_AUTO, BAND_FM, &r, 0, 0));
  TEST_ASSERT_TRUE(squelchUpdate(&sq, NULL, SQUELCH_AUTO, BAND_FM, NULL, 0, 0));
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(off_is_always_open);

  RUN_TEST(auto_opens_on_a_clean_signal);
  RUN_TEST(auto_shuts_on_noise_after_the_hold);
  RUN_TEST(a_signal_on_the_edge_does_not_chatter);
  RUN_TEST(auto_opens_again_as_soon_as_the_signal_comes_back);
  RUN_TEST(am_is_not_judged_on_multipath);
  RUN_TEST(the_two_bands_allow_different_offsets);
  RUN_TEST(the_offset_is_judged_either_side_of_centre);
  RUN_TEST(every_auto_threshold_is_tested_on_its_boundary);

  RUN_TEST(manual_opens_when_the_signal_beats_the_threshold);
  RUN_TEST(manual_shuts_when_it_does_not);
  RUN_TEST(manual_at_the_bottom_of_the_travel_is_always_open);
  RUN_TEST(the_pot_covers_the_whole_useful_range);
  RUN_TEST(a_calibrated_knob_uses_its_own_ends);

  RUN_TEST(a_reading_that_failed_never_shuts_the_audio);
  RUN_TEST(it_starts_open_rather_than_silent);
  RUN_TEST(readings_that_keep_failing_open_the_audio_again);
  RUN_TEST(one_failed_reading_does_not_postpone_the_shut);
  RUN_TEST(changing_band_starts_the_hold_again);
  RUN_TEST(it_survives_the_millisecond_wrap);
  RUN_TEST(mode_names_are_never_null);
  RUN_TEST(nulls_do_not_crash);

  return UNITY_END();
}
