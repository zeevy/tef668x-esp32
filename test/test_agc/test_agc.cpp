/*
 * Tests for the volume AGC. Runs on a PC.
 *
 * The captures in test/fixtures/agc/ were printed once a second by a
 * prototype that ran ten times a second, so they cannot drive the running
 * average one row at a time. What they can do is check the pairs that sit on
 * one row together, above all the average and the gain that average asked
 * for, and supply the real readings the guards have to cope with.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/agc.h"

#include "captures.h"

void setUp(void) {}
void tearDown(void) {}

static AgcConfig conf(uint8_t target, uint8_t boost) {
  AgcConfig c;
  c.targetPercent = target;
  c.boostDb = boost;
  return c;
}

static AgcReading good(int16_t mod) {
  AgcReading r;
  r.valid = true;
  r.modulationPercent = mod;
  r.levelTenths = 300;
  r.noiseTenths = 100;
  r.fm = true;
  r.listening = true;
  r.fresh = true;
  return r;
}

/*
 * Feed the same reading until the average has settled.
 *
 * Held steady on purpose. The AGC takes the caller's word for whether a
 * reading is fresh and never compares one value against the last, so a
 * station sitting at one modulation depth is measured like any other.
 */
static void settle(Agc *a, const AgcConfig *c, int16_t mod, int rounds) {
  for (int i = 0; i < rounds; i++) {
    AgcReading r = good(mod);
    agcUpdate(a, c, &r);
  }
}

/* ------------------------------------------- the gain the average asks for */

static void the_wanted_gain_matches_every_capture_row(void) {
  /* The heart of it, checked against real output rather than against my
   * reading of the prototype. Every row of the final capture that had a
   * settled average logged both the average and the gain that average asked
   * for, worked out there with log10f. This works it out with a table of
   * boundaries and no floating point, and has to agree on all of them. */
  int checked = 0;
  for (int i = 0; i < AGC_CAPTURE_COUNT; i++) {
    const AgcRow *row = &kAgcCapture[i];
    if (row->file != AGC_FINAL_FILE || row->ticks < AGC_MIN_TICKS ||
        row->averageTenths <= 0) {
      continue;
    }
    Agc a;
    agcInit(&a);
    a.averageMilli = (int32_t)row->averageTenths * 100;
    a.ticks = AGC_MIN_TICKS;
    AgcConfig c = conf(row->target, AGC_FINAL_BOOST);
    TEST_ASSERT_EQUAL_INT8((int8_t)row->wantedDb, agcWantedGain(&a, &c));
    checked++;
  }
  /* The capture has 168 such rows. Far fewer would mean the header stopped
   * carrying what this test thinks it does. */
  TEST_ASSERT_TRUE(checked > 150);
}

static void the_gain_is_zero_when_the_station_is_at_the_target(void) {
  Agc a;
  agcInit(&a);
  a.averageMilli = 50000;
  a.ticks = AGC_MIN_TICKS;
  AgcConfig c = conf(50, 6);
  TEST_ASSERT_EQUAL_INT8(0, agcWantedGain(&a, &c));
}

static void a_loud_station_is_cut_and_a_quiet_one_is_lifted(void) {
  Agc a;
  agcInit(&a);
  a.ticks = AGC_MIN_TICKS;
  AgcConfig c = conf(50, 6);

  a.averageMilli = 100000; /* Twice the target. */
  TEST_ASSERT_EQUAL_INT8(-6, agcWantedGain(&a, &c));
  a.averageMilli = 25000; /* Half the target. */
  TEST_ASSERT_EQUAL_INT8(6, agcWantedGain(&a, &c));
}

static void the_cut_stops_at_its_limit(void) {
  Agc a;
  agcInit(&a);
  a.ticks = AGC_MIN_TICKS;
  AgcConfig c = conf(30, 6);
  a.averageMilli = 200000; /* Far past anything the cut can reach. */
  TEST_ASSERT_EQUAL_INT8(AGC_MAX_CUT, agcWantedGain(&a, &c));
}

static void the_boost_stops_where_it_was_told(void) {
  Agc a;
  agcInit(&a);
  a.ticks = AGC_MIN_TICKS;
  a.averageMilli = 10000; /* Far below any target. */

  AgcConfig none = conf(50, 0);
  TEST_ASSERT_EQUAL_INT8(0, agcWantedGain(&a, &none));
  AgcConfig two = conf(50, 2);
  TEST_ASSERT_EQUAL_INT8(2, agcWantedGain(&a, &two));
  AgcConfig eight = conf(50, 8);
  TEST_ASSERT_EQUAL_INT8(8, agcWantedGain(&a, &eight));
}

static void a_boost_past_the_maximum_means_no_boost(void) {
  /* It can only come from a settings blob that has gone wrong. Holding it at
   * the maximum would turn that into the loudest setting the radio has, which
   * is the wrong direction to fail in, and it would not match the target,
   * where a value nobody could have chosen switches the AGC off. */
  Agc a;
  agcInit(&a);
  a.ticks = AGC_MIN_TICKS;
  a.averageMilli = 10000;
  AgcConfig c = conf(50, AGC_BOOST_MAX + 1);
  TEST_ASSERT_EQUAL_INT8(0, agcWantedGain(&a, &c));
  AgcConfig edge = conf(50, AGC_BOOST_MAX);
  TEST_ASSERT_EQUAL_INT8(AGC_BOOST_MAX, agcWantedGain(&a, &edge));
}

/* ------------------------------------------------------- what gets measured */

static void a_reading_below_the_signal_gate_is_not_measured(void) {
  /* The first capture was taken with this gate at 20 dBuV. On 104.0 the
   * signal sat at 10 to 18, so the average froze and the gain stayed at 0
   * for twenty four seconds: a threshold that switched the feature off
   * without failing. It is 8 dBuV now. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  for (int i = 0; i < 40; i++) {
    AgcReading r = good((int16_t)(60 + (i % 2)));
    r.levelTenths = AGC_MIN_SIGNAL * 10 - 1;
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_FALSE(agcSettled(&a));

  agcInit(&a);
  for (int i = 0; i < 40; i++) {
    AgcReading r = good((int16_t)(60 + (i % 2)));
    r.levelTenths = AGC_MIN_SIGNAL * 10;
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_TRUE(agcSettled(&a));
}

static void noise_is_not_measured_on_fm(void) {
  /* Noise reads as heavy modulation, so an empty channel would have the AGC
   * turn the volume down on it. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  for (int i = 0; i < 40; i++) {
    AgcReading r = good((int16_t)(150 + (i % 2)));
    r.noiseTenths = AGC_MAX_NOISE_TENTHS + 1;
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_FALSE(agcSettled(&a));
}

static void the_noise_limit_rejects_no_station_in_the_capture(void) {
  /* The limit is set from the squelch capture, which is the only one here
   * holding stations, the channel beside a strong station and an empty
   * channel all sampled the same way. It has to let every station reading
   * through, or the AGC would be throwing away the signal it measures. */
  int stations = 0;
  for (int i = 0; i < AGC_STATION_NOISE_COUNT; i++) {
    TEST_ASSERT_TRUE(kStationNoiseTenths[i] <= AGC_MAX_NOISE_TENTHS);
    stations++;
  }
  TEST_ASSERT_TRUE(stations > 300);
}

static void the_noise_test_is_fm_only(void) {
  /* The chip puts a different measurement in that field on the AM side, so
   * judging it against an FM limit would be reading the wrong number. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  for (int i = 0; i < 40; i++) {
    AgcReading r = good((int16_t)(60 + (i % 2)));
    r.fm = false;
    r.noiseTenths = AGC_MAX_NOISE_TENTHS + 1;
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_TRUE(agcSettled(&a));
}

static void silence_is_not_measured(void) {
  /* A speech pause is not a quiet station. Averaging it in would drag the
   * gain up and leave it there when the talking starts again. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  for (int i = 0; i < 40; i++) {
    AgcReading r = good((int16_t)(AGC_MOD_MIN - 1 - (i % 2)));
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_FALSE(agcSettled(&a));
}

static void a_reading_past_believable_is_dropped(void) {
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  for (int i = 0; i < 40; i++) {
    AgcReading r = good((int16_t)(AGC_MOD_MAX + 1 + (i % 2)));
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_FALSE(agcSettled(&a));
}

static void a_negative_reading_is_dropped(void) {
  /* The chip reports modulation in an unsigned word and the driver turns a
   * negative reading back into the small negative it was. Taken as a large
   * positive it would read as full modulation and pull the gain right down. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  for (int i = 0; i < 40; i++) {
    AgcReading r = good((int16_t)(-20 - (i % 2)));
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_FALSE(agcSettled(&a));
}

static void over_modulation_in_the_captures_is_measured_not_dropped(void) {
  /* 104.0 reports 134, 146, 150 and 153 in the final capture. These are real
   * over modulation, and a limit of 120 threw away the loudest samples and
   * under corrected the loudest station, which is why files 01 and 02 stop at
   * exactly 120 and the later ones do not. */
  int over120 = 0;
  int highest = 0;
  for (int i = 0; i < AGC_CAPTURE_COUNT; i++) {
    int16_t mod = kAgcCapture[i].modPercent;
    if (mod > 120) {
      over120++;
    }
    if (mod > highest) {
      highest = mod;
    }
  }
  TEST_ASSERT_TRUE(over120 > 20);
  TEST_ASSERT_EQUAL_INT(153, highest);
  /* And every one of them is inside what the AGC will measure. */
  TEST_ASSERT_TRUE(highest <= AGC_MOD_MAX);
}

static void a_reading_the_caller_calls_stale_is_not_counted(void) {
  /* On a weak FM signal the tuner is read every 300 ms while the AGC runs
   * every 100 ms, so the same value arrives three times over. Counting it
   * three times makes the settling guard pass on a third of the evidence. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  for (int i = 0; i < 40; i++) {
    AgcReading r = good((int16_t)(70 + (i % 2)));
    r.fresh = false;
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_FALSE(agcSettled(&a));
}

static void a_steady_station_is_not_mistaken_for_a_repeat(void) {
  /* The caller says whether a reading is fresh, rather than the AGC guessing
   * from the value. A station that genuinely holds the same whole per cent
   * looks exactly like the same sample arriving twice, and a guess would
   * stall the AGC on the steadiest signal it will ever see. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  for (int i = 0; i < 60; i++) {
    AgcReading r = good(70);
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_TRUE(agcSettled(&a));
  TEST_ASSERT_EQUAL_INT16(700, agcAverageTenths(&a));
}

static void the_slow_average_follows_a_small_change(void) {
  /* The average moves by a divided difference, and the slow divisor is 64.
   * Held in tenths, any difference under 6.4 per cent divides to nothing and
   * the average freezes for good, which is the whole feature stopped. This is
   * a five per cent move, comfortably inside that. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  settle(&a, &c, 40, 60);
  int16_t before = agcAverageTenths(&a);
  TEST_ASSERT_INT16_WITHIN(20, 405, before);

  settle(&a, &c, 45, 2000);
  int16_t after = agcAverageTenths(&a);
  TEST_ASSERT_TRUE(after > before);
  TEST_ASSERT_INT16_WITHIN(20, 455, after);
}

static void a_reading_that_did_not_arrive_changes_nothing(void) {
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  settle(&a, &c, 100, 30);
  int32_t before = a.averageMilli;
  for (int i = 0; i < 5; i++) {
    AgcReading r = good(60);
    r.valid = false;
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_EQUAL_INT32(before, a.averageMilli);
  agcUpdate(&a, &c, NULL);
  TEST_ASSERT_EQUAL_INT32(before, a.averageMilli);
}

static void nothing_is_measured_while_the_audio_is_not_playing(void) {
  /* A seek walking the band, a shut squelch and a deliberate mute all mean
   * the reading describes nothing anybody is listening to. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  for (int i = 0; i < 40; i++) {
    AgcReading r = good((int16_t)(60 + (i % 2)));
    r.listening = false;
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_FALSE(agcSettled(&a));
}

/* --------------------------------------------------- settling and releasing */

static void the_gain_does_not_move_before_the_average_means_anything(void) {
  /* A couple of samples are not a loudness measurement. Acting on them means
   * a loud or quiet moment at the instant of tuning sends the gain the wrong
   * way, which is heard as a swoop. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  for (int i = 0; i < AGC_MIN_TICKS - 1; i++) {
    AgcReading r = good((int16_t)(150 + i));
    TEST_ASSERT_EQUAL_INT8(0, agcUpdate(&a, &c, &r));
  }
}

static void the_gain_moves_one_step_at_a_time(void) {
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  settle(&a, &c, 150, AGC_MIN_TICKS);

  int8_t last = agcGain(&a);
  for (int i = 0; i < 20; i++) {
    AgcReading r = good((int16_t)(150 + (i % 2)));
    int8_t now = agcUpdate(&a, &c, &r);
    int diff = now - last;
    TEST_ASSERT_TRUE(diff >= -AGC_STEP_DB && diff <= AGC_STEP_DB);
    last = now;
  }
}

static void the_gain_settles_and_then_holds(void) {
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  settle(&a, &c, 100, 200);
  int8_t settled = agcGain(&a);
  /* Twice the target is about six dB down. */
  TEST_ASSERT_INT8_WITHIN(1, -6, settled);
  settle(&a, &c, 100, 60);
  TEST_ASSERT_INT8_WITHIN(1, settled, agcGain(&a));
}

static void the_deadband_stops_it_hunting(void) {
  /* Without it the gain toggles by a dB every update whenever the average
   * sits on a rounding boundary, heard as tremolo. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  settle(&a, &c, 100, 200);

  int changes = 0;
  int8_t last = agcGain(&a);
  for (int i = 0; i < 100; i++) {
    AgcReading r = good((int16_t)(100 + (i % 2)));
    int8_t now = agcUpdate(&a, &c, &r);
    if (now != last) {
      changes++;
      last = now;
    }
  }
  TEST_ASSERT_TRUE(changes <= 1);
}

static void the_gain_is_released_when_nothing_can_be_measured(void) {
  /* Otherwise the last station's cut sits on a station the AGC cannot read
   * and it plays quiet for as long as it is tuned in, with nothing to say
   * why. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  settle(&a, &c, 150, 200);
  TEST_ASSERT_TRUE(agcGain(&a) < 0);

  agcRetuned(&a);
  for (int i = 0; i < AGC_IDLE_TICKS + 40; i++) {
    AgcReading r = good(60);
    r.listening = false;
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_EQUAL_INT8(0, agcGain(&a));
}

static void a_retune_starts_the_average_again_but_keeps_the_gain(void) {
  /* Walking the gain to zero on every retune would be heard as the volume
   * jumping about as the dial crosses the band. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  settle(&a, &c, 150, 200);
  int8_t held = agcGain(&a);
  TEST_ASSERT_TRUE(held < 0);

  agcRetuned(&a);
  TEST_ASSERT_FALSE(agcSettled(&a));
  TEST_ASSERT_EQUAL_INT32(0, a.averageMilli);
  TEST_ASSERT_EQUAL_INT8(held, agcGain(&a));
}

static void a_new_station_settles_quickly_then_slowly(void) {
  /* The fast average for the first stretch after a tune is what makes a new
   * station reach its level in about two seconds rather than twenty. */
  Agc fast;
  agcInit(&fast);
  AgcConfig c = conf(50, 6);
  settle(&fast, &c, 40, AGC_SETTLE_TICKS);
  int32_t afterFast = fast.averageMilli;

  /* Now it is on the slow average, so the same number of readings at a very
   * different level moves it far less. */
  settle(&fast, &c, 160, AGC_SETTLE_TICKS);
  int32_t moved = fast.averageMilli - afterFast;
  TEST_ASSERT_TRUE(moved > 0);
  TEST_ASSERT_TRUE(moved < (160000 - afterFast) / 2);
}

/* ---------------------------------------------------------------- switched off */

static void the_gain_holds_when_a_settled_station_goes_quiet(void) {
  /* The release only applies before the average has settled, and this pins
   * that as deliberate. Once there is an average, a station that dips into
   * the noise or is muted for a while keeps the gain its own average asked
   * for, because that average is still the right answer for the station being
   * listened to. Releasing here would walk the volume up through every quiet
   * passage and back down after it. */
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(50, 6);
  settle(&a, &c, 150, 200);
  int8_t held = agcGain(&a);
  TEST_ASSERT_TRUE(held < 0);

  for (int i = 0; i < AGC_IDLE_TICKS * 20; i++) {
    AgcReading r = good(60);
    r.listening = false;
    agcUpdate(&a, &c, &r);
  }
  TEST_ASSERT_EQUAL_INT8(held, agcGain(&a));
}

static void a_target_outside_the_range_is_treated_as_off(void) {
  /* It can only arrive from a settings blob that has gone wrong, and aiming
   * at it would drive the volume somewhere nothing asked for. */
  Agc a;
  agcInit(&a);
  AgcConfig low = conf(AGC_TARGET_MIN - 1, 6);
  AgcConfig high = conf(AGC_TARGET_MAX + 1, 6);
  for (int i = 0; i < 60; i++) {
    AgcReading r = good((int16_t)(150 + (i % 2)));
    TEST_ASSERT_EQUAL_INT8(0, agcUpdate(&a, &low, &r));
    TEST_ASSERT_EQUAL_INT8(0, agcUpdate(&a, &high, &r));
  }
  AgcConfig edgeLow = conf(AGC_TARGET_MIN, 6);
  AgcConfig edgeHigh = conf(AGC_TARGET_MAX, 6);
  agcInit(&a);
  settle(&a, &edgeLow, 150, 60);
  TEST_ASSERT_TRUE(agcGain(&a) < 0);
  agcInit(&a);
  settle(&a, &edgeHigh, 150, 60);
  TEST_ASSERT_TRUE(agcGain(&a) < 0);
}

static void switching_it_off_forgets_the_average_too(void) {
  /* Switching straight back on would otherwise take the settled path on its
   * first call and jump towards a gain worked out from whatever was playing
   * before. */
  Agc a;
  agcInit(&a);
  AgcConfig on = conf(50, 6);
  settle(&a, &on, 150, 200);
  TEST_ASSERT_TRUE(agcSettled(&a));

  AgcConfig off = conf(0, 6);
  AgcReading r = good(150);
  agcUpdate(&a, &off, &r);
  TEST_ASSERT_FALSE(agcSettled(&a));
  TEST_ASSERT_EQUAL_INT32(0, a.averageMilli);
}

static void a_target_of_zero_is_off(void) {
  Agc a;
  agcInit(&a);
  AgcConfig c = conf(0, 6);
  for (int i = 0; i < 60; i++) {
    AgcReading r = good((int16_t)(150 + (i % 2)));
    TEST_ASSERT_EQUAL_INT8(0, agcUpdate(&a, &c, &r));
  }
}

static void switching_it_off_gives_the_gain_back(void) {
  /* A gain left applied with nothing to move it again would be a station
   * playing quiet for ever with no control that explains it. */
  Agc a;
  agcInit(&a);
  AgcConfig on = conf(50, 6);
  settle(&a, &on, 150, 200);
  TEST_ASSERT_TRUE(agcGain(&a) < 0);

  AgcConfig off = conf(0, 6);
  AgcReading r = good(150);
  TEST_ASSERT_EQUAL_INT8(0, agcUpdate(&a, &off, &r));
  TEST_ASSERT_EQUAL_INT8(0, agcGain(&a));
}

static void a_null_config_is_off(void) {
  Agc a;
  agcInit(&a);
  AgcReading r = good(150);
  TEST_ASSERT_EQUAL_INT8(0, agcUpdate(&a, NULL, &r));
}

static void null_is_safe(void) {
  AgcConfig c = conf(50, 6);
  AgcReading r = good(80);
  TEST_ASSERT_EQUAL_INT8(0, agcUpdate(NULL, &c, &r));
  TEST_ASSERT_EQUAL_INT8(0, agcGain(NULL));
  TEST_ASSERT_EQUAL_INT8(0, agcWantedGain(NULL, &c));
  agcInit(NULL);
  agcRetuned(NULL);
}

/* ------------------------------------------------- what the captures showed */

static void the_captures_carry_the_awkward_readings(void) {
  /* The fixtures are only worth keeping if they still hold the cases they
   * were kept for. If a capture is retaken and these stop being true, the
   * tests above are being run against easier data than they claim. */
  int silence = 0;
  int weak = 0;
  for (int i = 0; i < AGC_CAPTURE_COUNT; i++) {
    if (kAgcCapture[i].modPercent < AGC_MOD_MIN) {
      silence++;
    }
    if (kAgcCapture[i].levelDbuV < 20) {
      weak++;
    }
  }
  TEST_ASSERT_TRUE(silence > 10);
  TEST_ASSERT_TRUE(weak > 10);
}

static void no_capture_holds_a_reading_the_guard_would_have_to_catch(void) {
  /* The fixture README says a negative reading arrives as a number in the
   * thousands and has to be dropped. No capture contains one, so that guard
   * stands on what the chip does and on the driver's own note about reading
   * the word as signed, not on this evidence. Stated here so nobody later
   * believes the fixtures prove it. */
  for (int i = 0; i < AGC_CAPTURE_COUNT; i++) {
    TEST_ASSERT_TRUE(kAgcCapture[i].modPercent <= AGC_MOD_MAX);
  }
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(the_wanted_gain_matches_every_capture_row);
  RUN_TEST(the_gain_is_zero_when_the_station_is_at_the_target);
  RUN_TEST(a_loud_station_is_cut_and_a_quiet_one_is_lifted);
  RUN_TEST(the_cut_stops_at_its_limit);
  RUN_TEST(the_boost_stops_where_it_was_told);
  RUN_TEST(a_boost_past_the_maximum_means_no_boost);

  RUN_TEST(a_reading_below_the_signal_gate_is_not_measured);
  RUN_TEST(noise_is_not_measured_on_fm);
  RUN_TEST(the_noise_limit_rejects_no_station_in_the_capture);
  RUN_TEST(the_noise_test_is_fm_only);
  RUN_TEST(silence_is_not_measured);
  RUN_TEST(a_reading_past_believable_is_dropped);
  RUN_TEST(a_negative_reading_is_dropped);
  RUN_TEST(over_modulation_in_the_captures_is_measured_not_dropped);
  RUN_TEST(a_reading_the_caller_calls_stale_is_not_counted);
  RUN_TEST(a_steady_station_is_not_mistaken_for_a_repeat);
  RUN_TEST(the_slow_average_follows_a_small_change);
  RUN_TEST(a_reading_that_did_not_arrive_changes_nothing);
  RUN_TEST(nothing_is_measured_while_the_audio_is_not_playing);

  RUN_TEST(the_gain_does_not_move_before_the_average_means_anything);
  RUN_TEST(the_gain_moves_one_step_at_a_time);
  RUN_TEST(the_gain_settles_and_then_holds);
  RUN_TEST(the_deadband_stops_it_hunting);
  RUN_TEST(the_gain_is_released_when_nothing_can_be_measured);
  RUN_TEST(a_retune_starts_the_average_again_but_keeps_the_gain);
  RUN_TEST(a_new_station_settles_quickly_then_slowly);

  RUN_TEST(the_gain_holds_when_a_settled_station_goes_quiet);
  RUN_TEST(a_target_outside_the_range_is_treated_as_off);
  RUN_TEST(switching_it_off_forgets_the_average_too);
  RUN_TEST(a_target_of_zero_is_off);
  RUN_TEST(switching_it_off_gives_the_gain_back);
  RUN_TEST(a_null_config_is_off);
  RUN_TEST(null_is_safe);

  RUN_TEST(the_captures_carry_the_awkward_readings);
  RUN_TEST(no_capture_holds_a_reading_the_guard_would_have_to_catch);

  return UNITY_END();
}
