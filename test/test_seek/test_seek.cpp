/*
 * Tests for the seek stop decision. Runs on a PC.
 *
 * Most of these replay a real sweep of the FM band, read on the radio, rather
 * than readings somebody invented. A stop decision checked against invented
 * numbers is still a guessed threshold.
 *
 * Some limits are compared with the seek in PE5PVB's TEF6686_ESP32 firmware,
 * called the PE5PVB firmware below.
 */
#include <unity.h>

#include <stdio.h>

#include <stdint.h>
#include <string.h>

#include "am_heard.h"
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
  /* Why level alone cannot be the gate. The shoulders either side of a
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
  /* 106.4 MHz, read at plus 84 tenths of a kHz in the sweep. The PE5PVB
   * firmware's seek allows plus or minus 80, so it would skip a station that is
   * plainly there. This radio reads every FM carrier 5 to 7 kHz high because
   * its crystal runs 50 ppm fast. */
  SeekReading r = aStation();
  r.offsetTenths = 84;
  TEST_ASSERT_TRUE(seekShouldStop(NULL, BAND_FM, &r));
}

static void the_shoulder_of_a_strong_station_is_refused(void) {
  /* 102.0 MHz, measured on the radio. It sits beside 101.9, the strongest
   * station on the band, so the receiver is hearing that station's sidebands:
   * the noise reads 25 to 87 and the multipath under 130, both well inside the
   * limits. Only the level gives it away, which is the case the level gate
   * exists for. */
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
  /* 95.0 MHz on a day with reflections: a real station, stereo, with a
   * multipath of 204 to 279 across nine samples. The PE5PVB firmware's
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
  /* The floor is fitted to FM readings on an FM level scale. On AM a channel
   * of noise alone reads a higher level than some stations heard clearly, so
   * the AM side keeps to noise and offset. */
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
  /* The noise step is the PE5PVB firmware's own, so a sensitivity set here
   * means the same thing as the same number on that firmware. */
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
 * Two separate sets of thresholds drift apart. A seek that stops at 10.0 dBuV
 * where the squelch opens at 15.0, or allows a multipath of 320 against its
 * 230, or a carrier 20 kHz off centre against its 10, makes the radio stop,
 * mute, and report a find. So the seek asks the squelch instead of keeping its
 * own copy.
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
  /* Above the seek's floor, below the squelch's. */
  SeekReading between = atLevel(120);
  TEST_ASSERT_FALSE(seekShouldStop(&cfg, BAND_FM, &between));

  SeekReading above = atLevel(200);
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_FM, &above));
}

/*
 * The multipath limits differ, 320 for the seek against 230 for the squelch.
 * With the squelch on, the seek must not stop on a channel in between, or it
 * would stop and then mute.
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
 * And the offset windows differ, 20 kHz for the seek against 10 for the
 * squelch. The seek's is wide on purpose, because this radio reads every FM
 * carrier 5 to 7 kHz high. With the squelch on, the seek must still not stop
 * where the audio shuts.
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

/* ------------------------------------------------------------- the walk */

/*
 * Walk the sweep from channel `start` the way the radio task does: the dial
 * moves one channel, wrapping at the band's ends, then the walk judges it.
 * Each stop begins the next walk from where it stopped, until the walks have
 * come back past `start`. The channels it stopped on go in `stops`.
 */
static int walkTheSweep(size_t start, bool up, uint32_t *stops, int room) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  SeekWalk w;
  memset(&w, 0, sizeof(w));
  size_t at = start;
  size_t moved = 0;
  int n = 0;
  seekWalkBegin(&w, kFmSweep[at].khz, up, FM_SWEEP_COUNT);
  while (w.walking && moved < FM_SWEEP_COUNT) {
    at = up ? (at + 1) % FM_SWEEP_COUNT
            : (at + FM_SWEEP_COUNT - 1) % FM_SWEEP_COUNT;
    moved++;
    SeekReading r = readingAt(at);
    if (seekWalkJudge(&w, &cfg, BAND_FM, &r) == SEEK_WALK_FOUND) {
      TEST_ASSERT_TRUE(w.found);
      if (n < room) {
        stops[n] = kFmSweep[at].khz;
      }
      n++;
      seekWalkEnd(&w, false);
      seekWalkBegin(&w, kFmSweep[at].khz, up, FM_SWEEP_COUNT);
    }
  }
  return n;
}

static void a_walk_up_stops_on_each_station_in_turn(void) {
  uint32_t stops[FM_SWEEP_STATION_COUNT + 4];
  int n = walkTheSweep(0, true, stops, FM_SWEEP_STATION_COUNT + 4);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, n);
  for (int i = 0; i < FM_SWEEP_STATION_COUNT; i++) {
    TEST_ASSERT_EQUAL_UINT32(kFmSweepStations[i], stops[i]);
  }
}

static void a_walk_down_finds_them_the_other_way_round(void) {
  uint32_t stops[FM_SWEEP_STATION_COUNT + 4];
  int n = walkTheSweep(FM_SWEEP_COUNT - 1, false, stops,
                       FM_SWEEP_STATION_COUNT + 4);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, n);
  for (int i = 0; i < FM_SWEEP_STATION_COUNT; i++) {
    TEST_ASSERT_EQUAL_UINT32(kFmSweepStations[FM_SWEEP_STATION_COUNT - 1 - i],
                             stops[i]);
  }
}

static void a_walk_with_nothing_to_find_ends_after_one_lap(void) {
  SeekWalk w;
  memset(&w, 0, sizeof(w));
  seekWalkBegin(&w, 87500, true, FM_SWEEP_COUNT);
  SeekReading none;
  memset(&none, 0, sizeof(none));
  for (uint32_t i = 1; i < FM_SWEEP_COUNT; i++) {
    TEST_ASSERT_EQUAL(SEEK_WALK_ON, seekWalkJudge(&w, NULL, BAND_FM, &none));
  }
  TEST_ASSERT_EQUAL(SEEK_WALK_EMPTY, seekWalkJudge(&w, NULL, BAND_FM, &none));
  TEST_ASSERT_FALSE(w.walking);
  TEST_ASSERT_FALSE(w.found);
}

static void turning_round_keeps_the_first_start(void) {
  SeekWalk w;
  memset(&w, 0, sizeof(w));
  seekWalkBegin(&w, 100000, true, FM_SWEEP_COUNT);
  SeekReading none;
  memset(&none, 0, sizeof(none));
  seekWalkJudge(&w, NULL, BAND_FM, &none);
  seekWalkBegin(&w, 100100, false, FM_SWEEP_COUNT);
  TEST_ASSERT_EQUAL_UINT32(100000, w.fromKHz);
  TEST_ASSERT_FALSE(w.up);
  TEST_ASSERT_EQUAL_UINT32(0, w.visited);
  seekWalkEnd(&w, false);
  seekWalkBegin(&w, 100100, true, FM_SWEEP_COUNT);
  TEST_ASSERT_EQUAL_UINT32(100100, w.fromKHz);
}

static void a_band_with_no_channel_walks_nothing(void) {
  SeekWalk w;
  memset(&w, 0, sizeof(w));
  seekWalkBegin(&w, 100000, true, 0);
  TEST_ASSERT_FALSE(w.walking);
}

static void a_station_on_the_last_channel_of_a_lap_is_found(void) {
  SeekWalk w;
  memset(&w, 0, sizeof(w));
  seekWalkBegin(&w, 87500, true, FM_SWEEP_COUNT);
  SeekReading none;
  memset(&none, 0, sizeof(none));
  for (uint32_t i = 1; i < FM_SWEEP_COUNT; i++) {
    seekWalkJudge(&w, NULL, BAND_FM, &none);
  }
  SeekReading r = aStation();
  TEST_ASSERT_EQUAL(SEEK_WALK_FOUND, seekWalkJudge(&w, NULL, BAND_FM, &r));
  TEST_ASSERT_TRUE(w.found);
}

static void a_find_is_forgotten_when_the_dial_moves(void) {
  SeekWalk w;
  memset(&w, 0, sizeof(w));
  seekWalkBegin(&w, 106300, true, FM_SWEEP_COUNT);
  SeekReading r = aStation();
  TEST_ASSERT_EQUAL(SEEK_WALK_FOUND, seekWalkJudge(&w, NULL, BAND_FM, &r));
  TEST_ASSERT_TRUE(w.found);
  seekWalkEnd(&w, false);
  TEST_ASSERT_FALSE(w.found);
}

/* ------------------------------------------------- AM, heard by ear -- */

/* Reading `i` of an AM channel from the readings labelled by ear. */
static SeekReading amReading(const AmChannel *c, int i) {
  SeekReading r;
  memset(&r, 0, sizeof(r));
  r.valid = true;
  r.levelTenths = c->levelTenths[i];
  r.noiseTenths = c->noiseTenths[i];
  r.offsetTenths = c->offsetTenths[i];
  return r;
}

static BandId amBand(const AmChannel *c) {
  BandId band = BAND_COUNT;
  TEST_ASSERT_TRUE(bandForFrequency(NULL, c->khz, &band));
  TEST_ASSERT_TRUE(bandModulation(band) == MODULATION_AM);
  return band;
}

/* The AM rule, noise and offset, on the first reading, which is the one the
 * seek decides on: it stops on every channel heard clearly and on no channel
 * heard as noise alone, on MW, LW and SW. */
static void the_am_seek_stops_on_what_was_heard_and_not_on_noise(void) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  int clear = 0;
  int noise = 0;
  for (size_t i = 0; i < AM_HEARD_COUNT; i++) {
    const AmChannel *c = &kAmHeard[i];
    const SeekReading r = amReading(c, 0);
    char why[32];
    snprintf(why, sizeof(why), "%u kHz", (unsigned)c->khz);
    if (c->heard == AM_HEARD_CLEAR) {
      TEST_ASSERT_TRUE_MESSAGE(seekShouldStop(&cfg, amBand(c), &r), why);
      clear++;
    } else if (c->heard == AM_HEARD_NOISE) {
      TEST_ASSERT_FALSE_MESSAGE(seekShouldStop(&cfg, amBand(c), &r), why);
      noise++;
    }
  }
  TEST_ASSERT_TRUE(clear >= 5);
  TEST_ASSERT_TRUE(noise >= 10);
}

/* A station heard only weakly under the noise reads a noise of 313 to 775,
 * far over the limit of 120, so the AM seek passes over it: it stops where
 * a programme can be followed, not wherever one can just be made out. */
static void the_am_seek_passes_over_a_weak_station_under_the_noise(void) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  int weak = 0;
  for (size_t i = 0; i < AM_HEARD_COUNT; i++) {
    const AmChannel *c = &kAmHeard[i];
    if (c->heard != AM_HEARD_WEAK) {
      continue;
    }
    for (int k = 0; k < AM_HEARD_READS; k++) {
      const SeekReading r = amReading(c, k);
      TEST_ASSERT_FALSE(seekShouldStop(&cfg, amBand(c), &r));
    }
    weak++;
  }
  TEST_ASSERT_EQUAL_INT(2, weak);
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

  RUN_TEST(the_am_seek_stops_on_what_was_heard_and_not_on_noise);
  RUN_TEST(the_am_seek_passes_over_a_weak_station_under_the_noise);

  RUN_TEST(a_walk_up_stops_on_each_station_in_turn);
  RUN_TEST(a_walk_down_finds_them_the_other_way_round);
  RUN_TEST(a_walk_with_nothing_to_find_ends_after_one_lap);
  RUN_TEST(turning_round_keeps_the_first_start);
  RUN_TEST(a_band_with_no_channel_walks_nothing);
  RUN_TEST(a_station_on_the_last_channel_of_a_lap_is_found);
  RUN_TEST(a_find_is_forgotten_when_the_dial_moves);

  return UNITY_END();
}
