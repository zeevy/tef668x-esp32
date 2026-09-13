/*
 * Tests for the seek stop decision. Runs on a PC.
 *
 * Most of these replay a real sweep of the FM band taken off this radio, in
 * `test/fixtures/seek/`, rather than readings somebody invented. A stop
 * decision checked against invented numbers is a guessed threshold wearing a
 * test's clothing.
 */
#include <unity.h>

#include <stdio.h>

#include <stdint.h>
#include <string.h>

#include "core/seek.h"
#include "fm_sweep.h"

void setUp(void) {}
void tearDown(void) {}

static SeekReading readingAt(size_t i) {
  SeekReading r;
  memset(&r, 0, sizeof(r));
  r.valid = true;
  r.levelTenths = kFmSweep[i].levelTenths;
  r.noiseTenths = kFmSweep[i].noiseTenths;
  r.multipathTenths = kFmSweep[i].multipathTenths;
  r.offsetTenths = kFmSweep[i].offsetTenths;
  return r;
}

/*
 * A reading that passes every gate, so a test can spoil one of them and know
 * that is the only reason it failed.
 *
 * The numbers are 106.4 MHz out of the sweep, which is a real station.
 */
static SeekReading aStation(void) {
  SeekReading r;
  memset(&r, 0, sizeof(r));
  r.valid = true;
  r.levelTenths = 367;
  r.noiseTenths = 23;
  r.multipathTenths = 32;
  r.offsetTenths = 0;
  return r;
}

static bool wasOnAir(uint32_t khz) {
  for (size_t i = 0; i < FM_SWEEP_STATION_COUNT; i++) {
    if (kFmSweepStations[i] == khz) {
      return true;
    }
  }
  return false;
}

static void countStops(uint8_t sensitivity, int *stops, int *real) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  cfg.fmSensitivity = sensitivity;
  *stops = 0;
  *real = 0;
  for (size_t i = 0; i < FM_SWEEP_COUNT; i++) {
    SeekReading r = readingAt(i);
    if (!seekShouldStop(&cfg, BAND_FM, &r)) {
      continue;
    }
    (*stops)++;
    if (wasOnAir(kFmSweep[i].khz)) {
      (*real)++;
    }
  }
}

/* ------------------------------------------------------------ the sweep */

static void the_default_stops_on_every_station_that_was_on_air(void) {
  int stops = 0;
  int real = 0;
  countStops(SEEK_SENSITIVITY_DEFAULT, &stops, &real);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, real);
}

static void the_default_stops_on_nothing_else(void) {
  /* The other half, and the harder half. A seek that stops between stations
   * is not obviously broken, it just feels wrong to use. */
  int stops = 0;
  int real = 0;
  countStops(SEEK_SENSITIVITY_DEFAULT, &stops, &real);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, stops);
}

static void the_middle_sensitivities_are_all_clean(void) {
  for (uint8_t s = 2; s <= 4; s++) {
    int stops = 0;
    int real = 0;
    countStops(s, &stops, &real);
    TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, real);
    TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, stops);
  }
}

static void the_fussiest_setting_skips_a_real_station(void) {
  /* Written down rather than hidden. At sensitivity 1 the noise limit is 30
   * and 104.0 MHz read 31, so it is skipped. That is the cost of the fussiest
   * setting and it is why the radio does not ship on it. */
  int stops = 0;
  int real = 0;
  countStops(1, &stops, &real);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT - 1, real);
}

static void the_loosest_setting_stops_on_more(void) {
  /* Also written down. Sensitivity 6 is for finding something weak and far
   * away, and the price is stopping on things that are not stations. It keeps
   * every real one, which is the half that matters at that end. */
  int stops = 0;
  int real = 0;
  countStops(SEEK_SENSITIVITY_MAX, &stops, &real);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, real);
  TEST_ASSERT_TRUE(stops > FM_SWEEP_STATION_COUNT);
}

static void signal_level_alone_would_not_do_this(void) {
  /* Why the decision does not look at level. The shoulders either side of a
   * strong station read higher than a real but quieter station, so any level
   * that keeps every station also keeps a pile of noise. */
  int16_t weakest = 32767;
  for (size_t i = 0; i < FM_SWEEP_COUNT; i++) {
    if (wasOnAir(kFmSweep[i].khz) && kFmSweep[i].levelTenths < weakest) {
      weakest = kFmSweep[i].levelTenths;
    }
  }
  int noiseAbove = 0;
  for (size_t i = 0; i < FM_SWEEP_COUNT; i++) {
    if (!wasOnAir(kFmSweep[i].khz) && kFmSweep[i].levelTenths >= weakest / 2) {
      noiseAbove++;
    }
  }
  TEST_ASSERT_TRUE(noiseAbove > 0);
}

/* ------------------------------------------------------------ the gates */

static void a_reading_that_did_not_arrive_never_stops(void) {
  /* Stopping on a failed read would park the radio wherever the bus happened
   * to fail and report it as a find. */
  SeekReading r;
  memset(&r, 0, sizeof(r));
  r.valid = false;
  r.noiseTenths = 0;
  r.multipathTenths = 0;
  r.offsetTenths = 0;
  TEST_ASSERT_FALSE(seekShouldStop(NULL, BAND_FM, &r));
  TEST_ASSERT_FALSE(seekShouldStop(NULL, BAND_FM, NULL));
}

static void a_station_the_reference_window_would_skip_is_kept(void) {
  /* 106.4 MHz, read at plus 84 tenths of a kHz in the sweep. The reference
   * firmware's seek allows plus or minus 80, so it would skip a station that
   * is plainly there. This radio reads every FM carrier 5 to 7 kHz high
   * because its crystal runs 50 ppm fast, which is in HARDWARE.md. */
  SeekReading r = aStation();
  r.offsetTenths = 84;
  TEST_ASSERT_TRUE(seekShouldStop(NULL, BAND_FM, &r));
}

static void the_shoulder_of_a_strong_station_is_refused(void) {
  /* 102.0 MHz, measured on the radio on 13 September 2026. It sits beside
   * 101.9, the strongest station on the band, so the receiver is hearing that
   * station's sidebands: the noise reads 25 to 87 and the multipath under
   * 130, both well inside the limits. Only the level gives it away, which is
   * the case the level gate exists for. */
  SeekReading r;
  memset(&r, 0, sizeof(r));
  r.valid = true;
  r.levelTenths = -18;
  r.noiseTenths = 27;
  r.multipathTenths = 83;
  r.offsetTenths = 0;
  TEST_ASSERT_FALSE(seekShouldStop(NULL, BAND_FM, &r));

  /* And the same channel with a real station's level on it would be kept, so
   * the gate is refusing it for the level and nothing else. */
  r.levelTenths = 367;
  TEST_ASSERT_TRUE(seekShouldStop(NULL, BAND_FM, &r));
}

static void the_level_floor_falls_as_sensitivity_rises(void) {
  /* The one gate that runs the other way: a looser setting settles for a
   * weaker signal, which is the whole point of having the control. */
  TEST_ASSERT_TRUE(seekLevelFloor(1) > seekLevelFloor(6));
  TEST_ASSERT_EQUAL_INT16(100, seekLevelFloor(SEEK_SENSITIVITY_DEFAULT));
  TEST_ASSERT_EQUAL_INT16(seekLevelFloor(SEEK_SENSITIVITY_MIN),
                          seekLevelFloor(0));
  TEST_ASSERT_EQUAL_INT16(seekLevelFloor(SEEK_SENSITIVITY_MAX),
                          seekLevelFloor(99));

  SeekConfig cfg;
  seekDefaults(&cfg);
  SeekReading r = aStation();
  r.levelTenths = 80; /* Under the default floor of 100, over the one at 6. */
  TEST_ASSERT_FALSE(seekShouldStop(&cfg, BAND_FM, &r));
  cfg.fmSensitivity = SEEK_SENSITIVITY_MAX;
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_FM, &r));
}

static void a_carrier_far_off_centre_is_still_refused(void) {
  /* The window is wide, not absent. The shoulders of a strong station read
   * two and three hundred. */
  SeekReading r = aStation();
  r.offsetTenths = 306;
  TEST_ASSERT_FALSE(seekShouldStop(NULL, BAND_FM, &r));
  r.offsetTenths = -306;
  TEST_ASSERT_FALSE(seekShouldStop(NULL, BAND_FM, &r));
}

static void the_am_side_keeps_the_tight_window(void) {
  /* There is no bias to allow for on AM. The same 50 ppm at 738 kHz is 0.04
   * kHz, far too small to see. */
  SeekReading r = aStation();
  r.offsetTenths = 84;
  TEST_ASSERT_FALSE(seekShouldStop(NULL, BAND_MW, &r));
  r.offsetTenths = 15;
  TEST_ASSERT_TRUE(seekShouldStop(NULL, BAND_MW, &r));
}

static void a_weak_station_with_reflections_is_kept(void) {
  /* 95.0 MHz as it read on 14 September 2026: a real station, stereo, with a
   * multipath of 204 to 279 across nine samples. The reference firmware's
   * fixed limit of 230 would skip it about half the time and a limit fitted
   * to the sweep alone would skip it always.
   *
   * A seek that skips your station is useless. A seek that stops once too
   * often costs one more press. */
  SeekReading r;
  memset(&r, 0, sizeof(r));
  r.valid = true;
  r.levelTenths = 184;
  r.noiseTenths = 96;
  r.multipathTenths = 279;
  r.offsetTenths = 60;
  TEST_ASSERT_TRUE(seekShouldStop(NULL, BAND_FM, &r));
}

static void the_level_floor_is_fm_only(void) {
  /* The floor is fitted to an FM sweep on an FM level scale. There is no AM
   * sweep to fit an AM one to, so the AM side keeps to the rule the reference
   * firmware uses: noise and offset. */
  SeekReading r = aStation();
  r.levelTenths = -200; /* Far under the FM floor. */
  TEST_ASSERT_FALSE(seekShouldStop(NULL, BAND_FM, &r));
  TEST_ASSERT_TRUE(seekShouldStop(NULL, BAND_MW, &r));

  /* Noise still decides on AM. */
  r.noiseTenths = 500;
  TEST_ASSERT_FALSE(seekShouldStop(NULL, BAND_MW, &r));
}

static void multipath_is_ignored_on_the_am_bands(void) {
  /* The chip puts a co-channel figure in the same field on the AM side. It is
   * a different measurement on a different scale, so judging it against a
   * multipath limit would refuse stations for no reason. */
  SeekReading r = aStation();
  r.multipathTenths = 5000;
  TEST_ASSERT_TRUE(seekShouldStop(NULL, BAND_MW, &r));
  TEST_ASSERT_FALSE(seekShouldStop(NULL, BAND_FM, &r));
}

static void the_two_bands_have_their_own_sensitivity(void) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  cfg.fmSensitivity = 1;
  cfg.amSensitivity = 6;

  SeekReading r = aStation();
  r.noiseTenths = 100;
  TEST_ASSERT_FALSE(seekShouldStop(&cfg, BAND_FM, &r));
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_MW, &r));
}

/* ----------------------------------------------------------- the limits */

static void the_limits_follow_the_reference_scale(void) {
  /* The noise step is the reference firmware's own, so a sensitivity set here
   * means the same thing as the same number on the radio this replaces. */
  TEST_ASSERT_EQUAL_UINT16(30, seekNoiseLimit(1));
  TEST_ASSERT_EQUAL_UINT16(120, seekNoiseLimit(4));
  TEST_ASSERT_EQUAL_UINT16(180, seekNoiseLimit(6));
  TEST_ASSERT_EQUAL_UINT16(320, seekMultipathLimit(4));
}

static void a_sensitivity_out_of_range_is_held_at_the_edge(void) {
  TEST_ASSERT_EQUAL_UINT16(seekNoiseLimit(SEEK_SENSITIVITY_MIN),
                           seekNoiseLimit(0));
  TEST_ASSERT_EQUAL_UINT16(seekNoiseLimit(SEEK_SENSITIVITY_MAX),
                           seekNoiseLimit(200));
  TEST_ASSERT_EQUAL_UINT16(seekMultipathLimit(SEEK_SENSITIVITY_MAX),
                           seekMultipathLimit(200));
}

static void the_defaults_are_the_reference_defaults(void) {
  SeekConfig cfg;
  memset(&cfg, 0xAA, sizeof(cfg));
  seekDefaults(&cfg);
  TEST_ASSERT_EQUAL_UINT8(SEEK_SENSITIVITY_DEFAULT, cfg.fmSensitivity);
  TEST_ASSERT_EQUAL_UINT8(SEEK_SENSITIVITY_DEFAULT, cfg.amSensitivity);
  seekDefaults(NULL); /* Must not crash. */
}

static void no_config_means_the_defaults(void) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  for (size_t i = 0; i < FM_SWEEP_COUNT; i++) {
    SeekReading r = readingAt(i);
    TEST_ASSERT_EQUAL_INT(seekShouldStop(&cfg, BAND_FM, &r),
                          seekShouldStop(NULL, BAND_FM, &r));
  }
}

/* ------------------------ the seek and the squelch cannot disagree -- */

/*
 * A seek must never stop where the audio will not open.
 *
 * The two used to be separate sets of thresholds and differed three ways: the
 * seek stopped at 10.0 dBuV where the squelch opens at 15.0, allowed a
 * multipath of 320 against its 230, and a carrier 20 kHz off centre against
 * its 10. So the radio could stop, mute, and report a find. The seek now asks
 * the squelch instead of keeping its own copy.
 */
static SeekReading atLevel(int16_t tenths) {
  SeekReading r;
  r.valid = true;
  r.levelTenths = tenths;
  r.noiseTenths = 20;
  r.multipathTenths = 40;
  r.offsetTenths = 50;
  return r;
}

/* A config that asks the squelch, set up the way the radio task sets it. */
static SeekConfig withSquelch(SquelchMode mode, int16_t thresholdTenths) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  cfg.checkAudible = true;
  cfg.squelchMode = mode;
  cfg.squelchThresholdTenths = thresholdTenths;
  return cfg;
}

static void by_default_nothing_is_asked_of_the_squelch(void) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  TEST_ASSERT_FALSE(cfg.checkAudible);
  /* Just above the seek's own floor of 10.0 and below the squelch's 15.0. */
  SeekReading r = atLevel(120);
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_FM, &r));
}

static void the_squelch_level_floor_is_respected(void) {
  SeekConfig cfg = withSquelch(SQUELCH_AUTO, 0);
  /* The gap that produced the defect. */
  SeekReading between = atLevel(120);
  TEST_ASSERT_FALSE(seekShouldStop(&cfg, BAND_FM, &between));

  SeekReading above = atLevel(200);
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_FM, &above));
}

/*
 * The multipath limits differed, 320 for the seek against 230 for the
 * squelch, so a channel in between was stopped on and then muted.
 */
static void the_squelch_multipath_limit_is_respected(void) {
  SeekConfig cfg = withSquelch(SQUELCH_AUTO, 0);
  SeekReading r = atLevel(400);
  r.multipathTenths = 280; /* Inside the seek's 320, outside the squelch's. */
  TEST_ASSERT_FALSE(seekShouldStop(&cfg, BAND_FM, &r));

  r.multipathTenths = 200;
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_FM, &r));
}

/*
 * And the offset windows differed, 20 kHz for the seek against 10 for the
 * squelch. The seek's is wide on purpose, because this radio reads every FM
 * carrier 5 to 7 kHz high, but wide enough to stop where the audio shuts is
 * not what anybody wanted.
 */
static void the_squelch_offset_window_is_respected(void) {
  SeekConfig cfg = withSquelch(SQUELCH_AUTO, 0);
  SeekReading r = atLevel(400);
  r.offsetTenths = 150; /* Inside the seek's 200, outside the squelch's 100. */
  TEST_ASSERT_FALSE(seekShouldStop(&cfg, BAND_FM, &r));

  /* The crystal bias itself, plus 84 tenths, still gets through. */
  r.offsetTenths = 84;
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_FM, &r));
}

/* A manual threshold applies on every band, so the seek must respect it on
 * AM too, where the seek has no level gate of its own. */
static void a_manual_threshold_is_respected_on_am(void) {
  SeekConfig cfg = withSquelch(SQUELCH_MANUAL, 400);
  SeekReading weak = atLevel(50);
  weak.multipathTenths = 0;
  weak.offsetTenths = 5;
  TEST_ASSERT_FALSE(seekShouldStop(&cfg, BAND_MW, &weak));

  SeekReading strong = weak;
  strong.levelTenths = 500;
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_MW, &strong));
}

/*
 * At exactly a manual threshold the squelch is shut, because it opens above
 * the threshold and not at it. Sharing the test means the seek gets that for
 * free rather than having to add one to a number.
 */
static void the_seek_matches_the_squelch_at_the_threshold(void) {
  SeekConfig cfg = withSquelch(SQUELCH_MANUAL, 200);
  SeekReading on = atLevel(200);
  SeekReading above = atLevel(201);
  TEST_ASSERT_FALSE(seekShouldStop(&cfg, BAND_FM, &on));
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_FM, &above));
}

/* A squelch that is off mutes nothing, so no stop can be silent. */
static void a_squelch_that_is_off_constrains_nothing(void) {
  SeekConfig cfg = withSquelch(SQUELCH_OFF, 0);
  SeekReading r = atLevel(120);
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_FM, &r));
}

/* The seek's own gates still apply on top. Sharing must not loosen it. */
static void the_seek_gates_still_apply(void) {
  SeekConfig cfg = withSquelch(SQUELCH_OFF, 0);
  SeekReading noisy = atLevel(400);
  noisy.noiseTenths = 900;
  TEST_ASSERT_FALSE(seekShouldStop(&cfg, BAND_FM, &noisy));

  SeekReading quiet = atLevel(50);
  TEST_ASSERT_FALSE(seekShouldStop(&cfg, BAND_FM, &quiet));
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(the_default_stops_on_every_station_that_was_on_air);
  RUN_TEST(the_default_stops_on_nothing_else);
  RUN_TEST(the_middle_sensitivities_are_all_clean);
  RUN_TEST(the_fussiest_setting_skips_a_real_station);
  RUN_TEST(the_loosest_setting_stops_on_more);
  RUN_TEST(signal_level_alone_would_not_do_this);

  RUN_TEST(a_reading_that_did_not_arrive_never_stops);
  RUN_TEST(a_station_the_reference_window_would_skip_is_kept);
  RUN_TEST(the_shoulder_of_a_strong_station_is_refused);
  RUN_TEST(the_level_floor_falls_as_sensitivity_rises);
  RUN_TEST(a_carrier_far_off_centre_is_still_refused);
  RUN_TEST(the_am_side_keeps_the_tight_window);
  RUN_TEST(a_weak_station_with_reflections_is_kept);
  RUN_TEST(the_level_floor_is_fm_only);
  RUN_TEST(multipath_is_ignored_on_the_am_bands);
  RUN_TEST(the_two_bands_have_their_own_sensitivity);

  RUN_TEST(the_limits_follow_the_reference_scale);
  RUN_TEST(a_sensitivity_out_of_range_is_held_at_the_edge);
  RUN_TEST(the_defaults_are_the_reference_defaults);
  RUN_TEST(no_config_means_the_defaults);

  RUN_TEST(by_default_nothing_is_asked_of_the_squelch);
  RUN_TEST(the_squelch_level_floor_is_respected);
  RUN_TEST(the_squelch_multipath_limit_is_respected);
  RUN_TEST(the_squelch_offset_window_is_respected);
  RUN_TEST(a_manual_threshold_is_respected_on_am);
  RUN_TEST(the_seek_matches_the_squelch_at_the_threshold);
  RUN_TEST(a_squelch_that_is_off_constrains_nothing);
  RUN_TEST(the_seek_gates_still_apply);

  return UNITY_END();
}
