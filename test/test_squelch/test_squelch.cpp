/* Tests for the squelch. Runs on a PC. */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/squelch.h"

#include "shoulder.h"

static Squelch sq;
static SquelchConfig cfg;

void setUp(void) {
  squelchInit(&sq);
  squelchDefaults(&cfg);
}
void tearDown(void) {}

static SquelchReading good(void) {
  SquelchReading r;
  r.valid = true;
  r.levelTenths = 400;
  r.noiseTenths = 50;
  r.multipathTenths = 100;
  r.offsetTenths = 5;
  return r;
}

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

static void auto_opens_again_almost_as_soon_as_the_signal_comes_back(void) {
  /* Opening from shut takes SQUELCH_OPEN_READINGS readings and no more, so a
   * station arrives one poll later rather than being held. The one extra
   * reading is what throws away the isolated good reading the channel beside
   * a strong station produces. */
  uint32_t t = 1000;
  update(SQUELCH_AUTO, BAND_FM, good(), 0, t);
  t += 10;
  update(SQUELCH_AUTO, BAND_FM, bad(), 0, t);
  t += cfg.holdMs;
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_FM, bad(), 0, t));
  t += 50;
  TEST_ASSERT_FALSE(update(SQUELCH_AUTO, BAND_FM, good(), 0, t));
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

/* ------------------------------------------ replayed from the real capture */

/*
 * Judge every reading for one frequency the way the radio would.
 *
 * The capture was taken about three times a second and the radio judges the
 * squelch ten times a second. The hold is scaled to the capture, but the
 * smoothing cannot be: signalAverage forgets per reading, not per
 * millisecond, so replaying the raw column through it applies a far heavier
 * average than the radio does. The capture carries the radio's own smoothed
 * level instead, and that is what the floor is judged against here.
 */
static int replay(uint32_t khz, int *openOut, int *changes) {
  SquelchConfig conf;
  squelchDefaults(&conf);
  conf.holdMs = 1000;

  Squelch s;
  squelchInit(&s);
  int n = 0;
  int wasOpen = 1;
  *openOut = 0;
  *changes = 0;
  uint32_t now = 0;
  for (int i = 0; i < SHOULDER_COUNT; i++) {
    if (kShoulderCapture[i].khz != khz) {
      continue;
    }
    SquelchReading r;
    r.valid = true;
    /* The radio's own smoothed level, fed as the reading. The squelch
     * averages what it is given, and an average of an already smoothed
     * series settles on the same value, so the floor is judged against the
     * number the radio actually had. */
    r.levelTenths = kShoulderCapture[i].smoothedTenths;
    r.noiseTenths = kShoulderCapture[i].noiseTenths;
    r.multipathTenths = kShoulderCapture[i].multipathTenths;
    r.offsetTenths = kShoulderCapture[i].offsetTenths;
    bool open = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, now);
    if (open != (wasOpen != 0)) {
      (*changes)++;
      wasOpen = open ? 1 : 0;
    }
    if (open) {
      (*openOut)++;
    }
    n++;
    now += 330;
  }
  return n;
}

static void the_shoulder_of_a_strong_station_is_mostly_shut(void) {
  /* The defect this replaced. On 102.0, beside 101.9, the noise and the
   * multipath both read like a station, because the sidebands of the station
   * next door really are in the channel. The audio sat open on it. */
  int open = 0;
  int changes = 0;
  int n = replay(SHOULDER_KHZ, &open, &changes);
  TEST_ASSERT_TRUE(n >= 60);
  /* It was open on nearly every reading before the level floor. What is left
   * is the replay starting open and waiting out the hold. */
  TEST_ASSERT_TRUE(open * 100 / n < 10);
}

static void both_shoulders_in_the_capture_are_shut(void) {
  /* Two different strong stations, so one shoulder is not a special case. */
  const uint32_t shoulders[] = {102000, 91000};
  for (size_t i = 0; i < sizeof(shoulders) / sizeof(shoulders[0]); i++) {
    int open = 0;
    int changes = 0;
    int n = replay(shoulders[i], &open, &changes);
    TEST_ASSERT_TRUE(n >= 60);
    TEST_ASSERT_TRUE(open * 100 / n < 10);
    TEST_ASSERT_TRUE(changes <= 1);
  }
}

static void the_shoulder_no_longer_chatters(void) {
  /* Opening and shutting every couple of seconds is what made it worse than
   * simply being open: the squelch was calling attention to itself, which is
   * the one thing a squelch must not do. */
  int open = 0;
  int changes = 0;
  replay(SHOULDER_KHZ, &open, &changes);
  TEST_ASSERT_TRUE(changes <= 2);
}

static void the_weakest_station_in_the_capture_is_never_muted(void) {
  /* 92.7 reads 31.3 to 34.7 dBuV here, which is 16 dB clear of the floor, so
   * this is a guard against the floor being set absurdly high rather than a
   * test of the margin. The station the threshold actually turns on is the
   * 19.6 dBuV one from the seek sweep, which is not in this capture at all.
   * a_station_just_above_the_floor_stays_open covers that, with the spread a
   * real signal has rather than a flat line. */
  int open = 0;
  int changes = 0;
  int n = replay(SHOULDER_WEAKEST_STATION_KHZ, &open, &changes);
  TEST_ASSERT_TRUE(n >= 60);
  TEST_ASSERT_EQUAL_INT(n, open);
  TEST_ASSERT_EQUAL_INT(0, changes);
}

static void every_station_in_the_capture_stays_open(void) {
  const uint32_t stations[] = {91100, 92700, 98300, 101900, 102800, 106400};
  for (size_t i = 0; i < sizeof(stations) / sizeof(stations[0]); i++) {
    int open = 0;
    int changes = 0;
    int n = replay(stations[i], &open, &changes);
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_EQUAL_INT(n, open);
  }
}

static void an_empty_channel_is_shut(void) {
  int open = 0;
  int changes = 0;
  int n = replay(SHOULDER_EMPTY_KHZ, &open, &changes);
  TEST_ASSERT_TRUE(n >= 60);
  /* Only the readings before the hold runs out. */
  TEST_ASSERT_TRUE(open <= 4);
}

/* ---------------------------------------------------- the floor itself */

static bool steadyLevel(int16_t levelTenths, int rounds) {
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = levelTenths;
  r.noiseTenths = 10;
  r.multipathTenths = 30;
  r.offsetTenths = 50;
  bool open = true;
  uint32_t now = 0;
  for (int i = 0; i < rounds; i++) {
    open = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, now);
    now += 100;
  }
  return open;
}

static void the_level_floor_on_the_boundary_and_either_side(void) {
  /* The value itself, one below and one above, which RULES.md asks for on
   * anything with a threshold in it. A clean signal held steady at each. */
  TEST_ASSERT_TRUE(steadyLevel(150, 40));
  TEST_ASSERT_TRUE(steadyLevel(151, 40));
  TEST_ASSERT_FALSE(steadyLevel(149, 40));
}

static void a_station_below_the_floor_with_a_clean_signal_is_still_shut(void) {
  /* The cost of the floor, stated rather than hidden. A genuine station this
   * weak is muted. No station measured on this radio has ever read below
   * 19.6 dBuV, which is why the floor sits at 15.0. */
  TEST_ASSERT_FALSE(steadyLevel(100, 40));
  TEST_ASSERT_TRUE(steadyLevel(196, 40));
}

static void a_station_just_above_the_floor_stays_open(void) {
  /* The case the threshold turns on: the weakest station ever measured on
   * this radio, 19.6 dBuV in the seek sweep, against a floor of 15.0. Given
   * the spread a real signal has rather than a flat line, because a flat
   * line cannot show what a wobble either side of the threshold does. */
  const int16_t station[] = {196, 188, 204, 191, 210, 185, 199, 193,
                             206, 190, 201, 187, 197, 208, 192, 195};
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.noiseTenths = 14;
  r.multipathTenths = 30;
  r.offsetTenths = 60;
  bool open = true;
  uint32_t now = 0;
  for (int round = 0; round < 6; round++) {
    for (size_t i = 0; i < sizeof(station) / sizeof(station[0]); i++) {
      r.levelTenths = station[i];
      open = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, now);
      TEST_ASSERT_TRUE(open);
      now += 100;
    }
  }
}

static void the_floor_is_fm_only(void) {
  /* Seek's floor is FM only for the same reason: it is fitted to FM captures
   * on an FM level scale and there is no AM capture to fit an AM one to. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = 0; /* Far below the FM floor. */
  r.noiseTenths = 10;
  r.multipathTenths = 30;
  r.offsetTenths = 5;
  bool open = true;
  for (int i = 0; i < 40; i++) {
    open = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_MW, &r, 0,
                         (uint32_t)i * 100);
  }
  TEST_ASSERT_TRUE(open);
}

static void a_floor_of_zero_switches_it_off(void) {
  SquelchConfig conf;
  squelchDefaults(&conf);
  conf.fmLevelFloorTenths = SQUELCH_LEVEL_FLOOR_OFF;
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = -50;
  r.noiseTenths = 10;
  r.multipathTenths = 30;
  r.offsetTenths = 50;
  bool open = true;
  for (int i = 0; i < 40; i++) {
    open = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0,
                         (uint32_t)i * 100);
  }
  TEST_ASSERT_TRUE(open);
}

static void a_floor_of_exactly_zero_is_a_usable_value(void) {
  /* Zero is not the way to switch the floor off, so it stays available as a
   * floor. It is a value somebody might want: a dead channel on this radio
   * reads a little below zero, so 0.0 dBuV is the line between nothing at
   * all and something. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  conf.fmLevelFloorTenths = 0;
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.noiseTenths = 10;
  r.multipathTenths = 30;
  r.offsetTenths = 50;

  r.levelTenths = -50;
  bool open = true;
  for (int i = 0; i < 40; i++) {
    open = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0,
                         (uint32_t)i * 100);
  }
  TEST_ASSERT_FALSE(open);

  squelchInit(&s);
  r.levelTenths = 50;
  for (int i = 0; i < 40; i++) {
    open = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0,
                         (uint32_t)i * 100);
  }
  TEST_ASSERT_TRUE(open);
}

static void the_floor_waits_for_the_average_to_settle(void) {
  /* The average takes its first reading as the answer outright, so before a
   * few have gone in the floor would be judged on one raw sample. A station's
   * first reading after a retune can be taken before the tuner has settled,
   * and muting one because of that is the front of the station gone. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = -50; /* Far below the floor. */
  r.noiseTenths = 10;
  r.multipathTenths = 30;
  r.offsetTenths = 50;

  /* The first few readings are judged on everything but the floor, so a
   * clean signal stays open however low the level reads. */
  for (int i = 0; i < SQUELCH_LEVEL_SETTLE - 1; i++) {
    TEST_ASSERT_TRUE(squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0,
                                   (uint32_t)i * 100));
  }
  /* Once it has settled the floor applies, and the hold runs before it
   * shuts. */
  bool open = true;
  for (int i = SQUELCH_LEVEL_SETTLE - 1; i < 40; i++) {
    open = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0,
                         (uint32_t)i * 100);
  }
  TEST_ASSERT_FALSE(open);
}

static void the_average_runs_while_the_squelch_is_off(void) {
  /* The radio comes up with the squelch off, so an average that only ran in
   * Auto would be empty the moment somebody switched to it, and the first
   * reading after the switch would become the whole average. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = -50;
  r.noiseTenths = 10;
  r.multipathTenths = 30;
  r.offsetTenths = 50;
  for (int i = 0; i < 20; i++) {
    TEST_ASSERT_TRUE(squelchUpdate(&s, &conf, SQUELCH_OFF, BAND_FM, &r, 0,
                                   (uint32_t)i * 100));
  }
  TEST_ASSERT_EQUAL_UINT8(SQUELCH_LEVEL_SETTLE, s.levelSamples);

  /* So switching to Auto judges on history rather than on one sample. */
  bool open = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, 2000);
  TEST_ASSERT_TRUE(open); /* The hold has to run first. */
  for (int i = 0; i < 20; i++) {
    open = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0,
                         2000 + (uint32_t)i * 100);
  }
  TEST_ASSERT_FALSE(open);
}

static void a_station_on_the_limit_opens_from_shut(void) {
  /* The half that two in a row got wrong. A signal alternating either side of
   * a limit never produces two good readings together, so a rule wanting two
   * in a row leaves it muted for ever. Two of the last three reaches it on
   * the second good reading. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = 350;
  r.multipathTenths = 30;
  r.offsetTenths = 50;
  uint32_t now = 0;

  /* Start it shut on a run of bad readings. */
  r.noiseTenths = 900;
  for (int i = 0; i < 30; i++) {
    squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, now);
    now += 100;
  }
  TEST_ASSERT_FALSE(s.open);

  /* Now alternate. It has to come back. */
  bool opened = false;
  for (int i = 0; i < 20 && !opened; i++) {
    r.noiseTenths = (i % 2) ? 900 : 10;
    opened = squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, now);
    now += 100;
  }
  TEST_ASSERT_TRUE(opened);
}

static void an_isolated_good_reading_never_opens_it(void) {
  /* The shoulder of a strong station produced two good readings in sixty,
   * three apart. Neither may unmute the radio for the length of the hold. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = 350;
  r.multipathTenths = 30;
  r.offsetTenths = 50;
  uint32_t now = 0;
  r.noiseTenths = 900;
  for (int i = 0; i < 30; i++) {
    squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, now);
    now += 100;
  }
  TEST_ASSERT_FALSE(s.open);

  /* One good reading every fourth, which is wider than the window. */
  for (int i = 0; i < 40; i++) {
    r.noiseTenths = (i % 4 == 0) ? 10 : 900;
    TEST_ASSERT_FALSE(
        squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, now));
    now += 100;
  }
}

static void manual_mode_opens_on_a_signal_that_wobbles(void) {
  /* Manual judges the raw level against the person's own threshold, and the
   * raw level of this tuner jumps several dB between readings. A threshold
   * near the signal therefore alternates, and it must not leave the radio
   * silent: the person set that threshold to hear this station. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.noiseTenths = 10;
  r.multipathTenths = 30;
  r.offsetTenths = 50;
  uint32_t now = 0;

  r.levelTenths = 100;
  for (int i = 0; i < 30; i++) {
    squelchUpdate(&s, &conf, SQUELCH_MANUAL, BAND_FM, &r, 200, now);
    now += 100;
  }
  TEST_ASSERT_FALSE(s.open);

  bool opened = false;
  for (int i = 0; i < 20 && !opened; i++) {
    r.levelTenths = (i % 2) ? 190 : 210; /* Either side of 200. */
    opened = squelchUpdate(&s, &conf, SQUELCH_MANUAL, BAND_FM, &r, 200, now);
    now += 100;
  }
  TEST_ASSERT_TRUE(opened);
}

static void the_bottom_of_the_manual_travel_opens_quickly(void) {
  /* Turning the knob right down means always open, whatever the signal. It
   * still goes through the window, so it takes two readings rather than one,
   * and that is all it may take. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = -100;
  r.noiseTenths = 900;
  r.multipathTenths = 900;
  r.offsetTenths = 900;
  uint32_t now = 0;
  for (int i = 0; i < 30; i++) {
    squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, now);
    now += 100;
  }
  TEST_ASSERT_FALSE(s.open);

  squelchUpdate(&s, &conf, SQUELCH_MANUAL, BAND_FM, &r, -1000, now);
  now += 100;
  TEST_ASSERT_TRUE(
      squelchUpdate(&s, &conf, SQUELCH_MANUAL, BAND_FM, &r, -1000, now));
}

static void manual_mode_ignores_the_floor(void) {
  /* Manual is the person's own threshold against the raw level. The floor is
   * part of what Auto means by a station and has no business there. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = 100; /* Below the Auto floor, above the manual threshold. */
  r.noiseTenths = 900;
  r.multipathTenths = 900;
  r.offsetTenths = 900;
  bool open = true;
  for (int i = 0; i < 40; i++) {
    open = squelchUpdate(&s, &conf, SQUELCH_MANUAL, BAND_FM, &r, 50,
                         (uint32_t)i * 100);
  }
  TEST_ASSERT_TRUE(open);
}

/* ------------------------------------------------- the average and retuning */

static void a_retune_starts_the_level_average_again(void) {
  /* Landing on a station from the shoulder of another one must not be held
   * shut while the average climbs. That would be the front of the station
   * gone, which is what the immediate open exists to prevent. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);

  SquelchReading low;
  low.valid = true;
  low.levelTenths = -20;
  low.noiseTenths = 10;
  low.multipathTenths = 30;
  low.offsetTenths = 50;
  uint32_t now = 0;
  for (int i = 0; i < 40; i++) {
    squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &low, 0, now);
    now += 100;
  }

  SquelchReading station = low;
  station.levelTenths = 350;
  /* Without the reset the average would take about a second to climb. With
   * it, the station is heard after SQUELCH_OPEN_READINGS readings, which is
   * one extra poll and not a second. */
  squelchRetuned(&s);
  for (int i = 0; i < SQUELCH_OPEN_READINGS - 1; i++) {
    squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &station, 0, now);
    now += 100;
  }
  TEST_ASSERT_TRUE(
      squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &station, 0, now));
}

static void a_station_on_the_limit_is_never_muted(void) {
  /* A signal alternating either side of the limit is a station somebody is
   * listening to. Once the audio is open, one good reading calls off a shut
   * that is being held, so it stays open. Requiring a run to open must not
   * turn into requiring a run to stay open. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = 350;
  r.multipathTenths = 30;
  r.offsetTenths = 50;
  uint32_t now = 0;
  r.noiseTenths = 10;
  for (int i = 0; i < 10; i++) {
    squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, now);
    now += 100;
  }
  for (int i = 0; i < 60; i++) {
    r.noiseTenths = (i % 2) ? 900 : 10;
    TEST_ASSERT_TRUE(
        squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, now));
    now += 100;
  }
}

static void retuning_does_not_reopen_a_shut_squelch(void) {
  /* Only the average starts again. Whether the audio is open carries across
   * a retune, or every step of the dial across an empty band would blip. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading low;
  low.valid = true;
  low.levelTenths = -20;
  low.noiseTenths = 900;
  low.multipathTenths = 30;
  low.offsetTenths = 50;
  uint32_t now = 0;
  for (int i = 0; i < 40; i++) {
    squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &low, 0, now);
    now += 100;
  }
  TEST_ASSERT_FALSE(s.open);
  squelchRetuned(&s);
  TEST_ASSERT_FALSE(s.open);
  squelchRetuned(NULL); /* Must not crash. */
}

static void the_corners_of_the_small_calls(void) {
  /* A mode that is not a mode has to give a name rather than run off the end
   * of the table, and setting a null squelch up has to do nothing rather
   * than write through it. */
  TEST_ASSERT_EQUAL_STRING("", squelchModeName(SQUELCH_MODE_COUNT));
  TEST_ASSERT_EQUAL_STRING("", squelchModeName((SquelchMode)99));
  squelchInit(NULL); /* Must not crash. */
}

static void a_failed_reading_does_not_enter_the_average(void) {
  /* A failed read leaves the previous values in the struct, so feeding it
   * again would count the same sample twice and drag the average towards a
   * number nothing measured. */
  SquelchConfig conf;
  squelchDefaults(&conf);
  Squelch s;
  squelchInit(&s);
  SquelchReading r;
  r.valid = true;
  r.levelTenths = 350;
  r.noiseTenths = 10;
  r.multipathTenths = 30;
  r.offsetTenths = 50;
  squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, 0);
  int32_t before = s.level.accumulator;

  r.valid = false;
  squelchUpdate(&s, &conf, SQUELCH_AUTO, BAND_FM, &r, 0, 100);
  TEST_ASSERT_EQUAL_INT32(before, s.level.accumulator);
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(off_is_always_open);

  RUN_TEST(auto_opens_on_a_clean_signal);
  RUN_TEST(auto_shuts_on_noise_after_the_hold);
  RUN_TEST(a_signal_on_the_edge_does_not_chatter);
  RUN_TEST(auto_opens_again_almost_as_soon_as_the_signal_comes_back);
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

  RUN_TEST(the_shoulder_of_a_strong_station_is_mostly_shut);
  RUN_TEST(both_shoulders_in_the_capture_are_shut);
  RUN_TEST(the_shoulder_no_longer_chatters);
  RUN_TEST(the_weakest_station_in_the_capture_is_never_muted);
  RUN_TEST(a_station_just_above_the_floor_stays_open);
  RUN_TEST(every_station_in_the_capture_stays_open);
  RUN_TEST(an_empty_channel_is_shut);
  RUN_TEST(the_level_floor_on_the_boundary_and_either_side);
  RUN_TEST(a_station_below_the_floor_with_a_clean_signal_is_still_shut);
  RUN_TEST(the_floor_is_fm_only);
  RUN_TEST(a_floor_of_zero_switches_it_off);
  RUN_TEST(a_floor_of_exactly_zero_is_a_usable_value);
  RUN_TEST(the_floor_waits_for_the_average_to_settle);
  RUN_TEST(the_average_runs_while_the_squelch_is_off);
  RUN_TEST(a_station_on_the_limit_opens_from_shut);
  RUN_TEST(an_isolated_good_reading_never_opens_it);
  RUN_TEST(manual_mode_opens_on_a_signal_that_wobbles);
  RUN_TEST(the_bottom_of_the_manual_travel_opens_quickly);
  RUN_TEST(manual_mode_ignores_the_floor);
  RUN_TEST(a_retune_starts_the_level_average_again);
  RUN_TEST(a_station_on_the_limit_is_never_muted);
  RUN_TEST(retuning_does_not_reopen_a_shut_squelch);
  RUN_TEST(the_corners_of_the_small_calls);
  RUN_TEST(a_failed_reading_does_not_enter_the_average);

  return UNITY_END();
}
