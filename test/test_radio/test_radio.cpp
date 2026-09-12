/**
 * @file test_radio.cpp
 * @brief Tests for the radio state machine. Runs on a PC.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/radio.h"

static BandPlanConfig plan;
static RadioSettings r;

void setUp(void) {
  bandPlanDefaults(&plan);
  radioDefaults(&r, &plan);
}
void tearDown(void) {}

/** Shorthand for applying one command. */
static RadioError apply(RadioCommand c) {
  return radioApply(&r, &plan, &c);
}

static void a_new_radio_comes_up_on_fm_at_the_bottom_of_the_band(void) {
  TEST_ASSERT_EQUAL_INT(BAND_FM, r.band);
  TEST_ASSERT_EQUAL_UINT32(87500, r.freqKHz);
  TEST_ASSERT_EQUAL_UINT16(100, r.stepKHz);
  TEST_ASSERT_EQUAL_UINT16(0, r.bandwidthKHz);
  TEST_ASSERT_EQUAL_INT8(0, r.volumeDb);
  TEST_ASSERT_FALSE(r.muted);
  TEST_ASSERT_EQUAL_INT(TUNE_MODE_MANUAL, r.tuneMode);
}

static void tuning_to_a_frequency_works(void) {
  RadioCommand c = {};
  c.kind = RADIO_TUNE;
  c.freqKHz = 104000;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
  TEST_ASSERT_EQUAL_UINT32(104000, r.freqKHz);
  TEST_ASSERT_EQUAL_INT(BAND_FM, r.band);
}

static void tuning_somewhere_that_is_in_no_band_is_refused(void) {
  RadioCommand c = {};
  c.kind = RADIO_TUNE;
  c.freqKHz = 50000;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_FREQUENCY, apply(c));
  /* Refused means nothing moved, not half moved. */
  TEST_ASSERT_EQUAL_UINT32(87500, r.freqKHz);
  TEST_ASSERT_EQUAL_INT(BAND_FM, r.band);
}

static void tuning_across_a_band_edge_brings_the_step_size_with_it(void) {
  RadioCommand c = {};
  c.kind = RADIO_TUNE;
  c.freqKHz = 738; /* Medium wave. */
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
  TEST_ASSERT_EQUAL_INT(BAND_MW, r.band);
  TEST_ASSERT_EQUAL_UINT32(738, r.freqKHz);
  /* The FM step of 100 kHz would be meaningless here. */
  TEST_ASSERT_EQUAL_UINT16(9, r.stepKHz);
  TEST_ASSERT_TRUE(bandStepAllowed(r.band, &plan, r.stepKHz));
}

static void changing_band_lands_somewhere_inside_it(void) {
  for (int b = 0; b < BAND_COUNT; b++) {
    radioDefaults(&r, &plan);
    RadioCommand c = {};
    c.kind = RADIO_SET_BAND;
    c.band = (BandId)b;
    TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
    TEST_ASSERT_EQUAL_INT(b, r.band);
    TEST_ASSERT_TRUE(bandContains(r.band, &plan, r.freqKHz));
    TEST_ASSERT_TRUE(bandStepAllowed(r.band, &plan, r.stepKHz));
  }
}

static void changing_to_something_that_is_not_a_band_is_refused(void) {
  RadioCommand c = {};
  c.kind = RADIO_SET_BAND;
  c.band = (BandId)BAND_COUNT;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_BAND, apply(c));
  TEST_ASSERT_EQUAL_INT(BAND_FM, r.band);
}

static void stepping_moves_by_the_step_size(void) {
  RadioCommand c = {};
  c.kind = RADIO_STEP;
  c.steps = 1;
  apply(c);
  TEST_ASSERT_EQUAL_UINT32(87600, r.freqKHz);
  c.steps = -1;
  apply(c);
  TEST_ASSERT_EQUAL_UINT32(87500, r.freqKHz);
}

static void stepping_several_at_once_is_the_same_as_one_at_a_time(void) {
  RadioSettings once = r;
  RadioCommand many = {};
  many.kind = RADIO_STEP;
  many.steps = 7;
  radioApply(&once, &plan, &many);

  RadioCommand one = {};
  one.kind = RADIO_STEP;
  one.steps = 1;
  for (int i = 0; i < 7; i++) {
    apply(one);
  }
  TEST_ASSERT_EQUAL_UINT32(once.freqKHz, r.freqKHz);
}

static void stepping_zero_changes_nothing(void) {
  RadioCommand c = {};
  c.kind = RADIO_STEP;
  c.steps = 0;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
  TEST_ASSERT_EQUAL_UINT32(87500, r.freqKHz);
}

static void stepping_off_the_end_wraps_and_stays_in_the_band(void) {
  RadioCommand down = {};
  down.kind = RADIO_STEP;
  down.steps = -1;
  apply(down); /* From the bottom edge. */
  TEST_ASSERT_TRUE(bandContains(r.band, &plan, r.freqKHz));
  TEST_ASSERT_EQUAL_UINT32(bandTopChannel(BAND_FM, &plan, 100), r.freqKHz);

  RadioCommand up = {};
  up.kind = RADIO_STEP;
  up.steps = 1;
  apply(up);
  TEST_ASSERT_EQUAL_UINT32(87500, r.freqKHz);
}

static void a_step_size_the_band_refuses_is_refused(void) {
  RadioCommand c = {};
  c.kind = RADIO_SET_STEP;
  c.stepKHz = 9; /* A medium wave step, on FM. */
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_STEP, apply(c));
  TEST_ASSERT_EQUAL_UINT16(100, r.stepKHz);

  c.stepKHz = 50;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
  TEST_ASSERT_EQUAL_UINT16(50, r.stepKHz);
}

static void volume_outside_what_the_chip_takes_is_refused(void) {
  RadioCommand c = {};
  c.kind = RADIO_SET_VOLUME;
  c.volumeDb = RADIO_VOLUME_MAX;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
  c.volumeDb = RADIO_VOLUME_MIN;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));

  c.volumeDb = RADIO_VOLUME_MAX + 1;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_VOLUME, apply(c));
  c.volumeDb = RADIO_VOLUME_MIN - 1;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_VOLUME, apply(c));
  /* The last accepted value survives a refusal. */
  TEST_ASSERT_EQUAL_INT8(RADIO_VOLUME_MIN, r.volumeDb);
}

static void muting_and_unmuting_works(void) {
  RadioCommand c = {};
  c.kind = RADIO_SET_MUTE;
  c.muted = true;
  apply(c);
  TEST_ASSERT_TRUE(r.muted);
  c.muted = false;
  apply(c);
  TEST_ASSERT_FALSE(r.muted);
}

static void automatic_bandwidth_is_only_offered_on_fm(void) {
  RadioCommand bw = {};
  bw.kind = RADIO_SET_BANDWIDTH;
  bw.bandwidthKHz = 0;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(bw));

  RadioCommand band = {};
  band.kind = RADIO_SET_BAND;
  band.band = BAND_MW;
  apply(band);
  /* Nothing on the AM side has an automatic setting to copy, so zero means
   * nothing there and is refused rather than guessed at. */
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_BANDWIDTH, apply(bw));

  bw.bandwidthKHz = 4;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(bw));
  TEST_ASSERT_EQUAL_UINT16(4, r.bandwidthKHz);

  bw.bandwidthKHz = 6001;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_BANDWIDTH, apply(bw));
  TEST_ASSERT_EQUAL_UINT16(4, r.bandwidthKHz);
}

static void meter_band_mode_is_only_available_on_shortwave(void) {
  RadioCommand mode = {};
  mode.kind = RADIO_SET_TUNE_MODE;
  mode.tuneMode = TUNE_MODE_METER_BAND;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_TUNE_MODE, apply(mode));

  RadioCommand band = {};
  band.kind = RADIO_SET_BAND;
  band.band = BAND_SW;
  apply(band);
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(mode));
  TEST_ASSERT_EQUAL_INT(TUNE_MODE_METER_BAND, r.tuneMode);
}

static void leaving_shortwave_drops_meter_band_mode(void) {
  RadioCommand band = {};
  band.kind = RADIO_SET_BAND;
  band.band = BAND_SW;
  apply(band);
  RadioCommand mode = {};
  mode.kind = RADIO_SET_TUNE_MODE;
  mode.tuneMode = TUNE_MODE_METER_BAND;
  apply(mode);

  band.band = BAND_FM;
  apply(band);
  /* Otherwise the encoder would be in a mode that means nothing here. */
  TEST_ASSERT_EQUAL_INT(TUNE_MODE_MANUAL, r.tuneMode);
}

static void tuning_out_of_shortwave_also_drops_meter_band_mode(void) {
  RadioCommand band = {};
  band.kind = RADIO_SET_BAND;
  band.band = BAND_SW;
  apply(band);
  RadioCommand mode = {};
  mode.kind = RADIO_SET_TUNE_MODE;
  mode.tuneMode = TUNE_MODE_METER_BAND;
  apply(mode);

  RadioCommand tune = {};
  tune.kind = RADIO_TUNE;
  tune.freqKHz = 104000;
  apply(tune);
  TEST_ASSERT_EQUAL_INT(TUNE_MODE_MANUAL, r.tuneMode);
}

static void meter_band_mode_steps_between_meter_bands(void) {
  RadioCommand band = {};
  band.kind = RADIO_SET_BAND;
  band.band = BAND_SW;
  apply(band);
  RadioCommand mode = {};
  mode.kind = RADIO_SET_TUNE_MODE;
  mode.tuneMode = TUNE_MODE_METER_BAND;
  apply(mode);
  RadioCommand tune = {};
  tune.kind = RADIO_TUNE;
  tune.freqKHz = 9420;
  apply(tune);

  RadioCommand step = {};
  step.kind = RADIO_STEP;
  step.steps = 1;
  apply(step);
  TEST_ASSERT_EQUAL_UINT32(11600, r.freqKHz); /* 25 metres. */
  step.steps = -1;
  apply(step);
  TEST_ASSERT_EQUAL_UINT32(9400, r.freqKHz); /* Back into 31 metres. */
}

static void moving_to_an_am_band_never_leaves_the_fm_automatic_bandwidth(void) {
  /* FM's automatic setting is a bandwidth of zero. Zero is not a width at
   * all on the AM side, so the tuner refuses it, and the code that pushes to
   * the tuner mutes before it tunes. Carrying the zero across a band change
   * is therefore how the radio goes silent with no way back. */
  TEST_ASSERT_EQUAL_UINT16(0, r.bandwidthKHz);

  RadioCommand band = {};
  band.kind = RADIO_SET_BAND;
  for (int b = 0; b < BAND_COUNT; b++) {
    radioDefaults(&r, &plan);
    band.band = (BandId)b;
    TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(band));
    if (bandModulation(r.band) != MODULATION_FM) {
      TEST_ASSERT_NOT_EQUAL_UINT16(0, r.bandwidthKHz);
    }
  }

  /* Tuning across the edge has to do the same, since that changes band too. */
  radioDefaults(&r, &plan);
  RadioCommand tune = {};
  tune.kind = RADIO_TUNE;
  tune.freqKHz = 738;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(tune));
  TEST_ASSERT_EQUAL_INT(BAND_MW, r.band);
  TEST_ASSERT_NOT_EQUAL_UINT16(0, r.bandwidthKHz);

  /* And the other direction matters just as much. An AM width of 4 kHz left
   * on FM is a tenth of what the signal needs: the audio is muffled and
   * stereo never locks. */
  band.band = BAND_FM;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(band));
  TEST_ASSERT_EQUAL_UINT16(0, r.bandwidthKHz);

  /* Round tripping through every band always leaves a width that band can
   * actually use. */
  for (int b = 0; b < BAND_COUNT; b++) {
    band.band = (BandId)b;
    apply(band);
    if (bandModulation(r.band) == MODULATION_FM) {
      TEST_ASSERT_EQUAL_UINT16(0, r.bandwidthKHz);
    } else {
      TEST_ASSERT_NOT_EQUAL_UINT16(0, r.bandwidthKHz);
    }
    band.band = BAND_FM;
    apply(band);
    TEST_ASSERT_EQUAL_UINT16(0, r.bandwidthKHz);
  }
}

static void the_most_negative_step_count_is_not_silently_ignored(void) {
  /* Negating INT16_MIN overflows. The loop then never runs and the command
   * vanishes while still reporting success. */
  RadioCommand c = {};
  c.kind = RADIO_STEP;
  c.steps = INT16_MIN;
  uint32_t before = r.freqKHz;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
  TEST_ASSERT_NOT_EQUAL_UINT32(before, r.freqKHz);
  TEST_ASSERT_TRUE(bandContains(r.band, &plan, r.freqKHz));
}

static void a_command_that_is_not_a_command_is_refused(void) {
  RadioCommand c = {};
  c.kind = (RadioCommandKind)99;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_UNKNOWN, apply(c));
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_UNKNOWN, radioApply(NULL, &plan, &c));
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_UNKNOWN, radioApply(&r, &plan, NULL));
}

static void every_error_and_mode_has_words(void) {
  for (int e = 0; e <= RADIO_ERR_UNKNOWN; e++) {
    TEST_ASSERT_NOT_NULL(radioErrorText((RadioError)e));
    TEST_ASSERT_NOT_EQUAL(0, strlen(radioErrorText((RadioError)e)));
  }
  for (int m = 0; m < TUNE_MODE_COUNT; m++) {
    TEST_ASSERT_NOT_NULL(tuneModeName((TuneMode)m));
    TEST_ASSERT_NOT_EQUAL(0, strlen(tuneModeName((TuneMode)m)));
  }
  TEST_ASSERT_EQUAL_STRING("", tuneModeName((TuneMode)TUNE_MODE_COUNT));
}

static void only_changes_the_tuner_cares_about_ask_for_a_retune(void) {
  RadioSettings before = r;
  TEST_ASSERT_FALSE(radioNeedsRetune(&before, &r));

  /* The step size and the tuning mode decide what the next command will be.
   * Neither ever reaches the chip. */
  RadioCommand step = {};
  step.kind = RADIO_SET_STEP;
  step.stepKHz = 50;
  apply(step);
  TEST_ASSERT_FALSE(radioNeedsRetune(&before, &r));

  RadioCommand mode = {};
  mode.kind = RADIO_SET_TUNE_MODE;
  mode.tuneMode = TUNE_MODE_AUTO;
  apply(mode);
  TEST_ASSERT_FALSE(radioNeedsRetune(&before, &r));

  RadioCommand tune = {};
  tune.kind = RADIO_TUNE;
  tune.freqKHz = 104000;
  apply(tune);
  TEST_ASSERT_TRUE(radioNeedsRetune(&before, &r));

  TEST_ASSERT_TRUE(radioNeedsRetune(NULL, &r));
  TEST_ASSERT_TRUE(radioNeedsRetune(&r, NULL));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(a_new_radio_comes_up_on_fm_at_the_bottom_of_the_band);
  RUN_TEST(tuning_to_a_frequency_works);
  RUN_TEST(tuning_somewhere_that_is_in_no_band_is_refused);
  RUN_TEST(tuning_across_a_band_edge_brings_the_step_size_with_it);
  RUN_TEST(changing_band_lands_somewhere_inside_it);
  RUN_TEST(changing_to_something_that_is_not_a_band_is_refused);
  RUN_TEST(stepping_moves_by_the_step_size);
  RUN_TEST(stepping_several_at_once_is_the_same_as_one_at_a_time);
  RUN_TEST(stepping_zero_changes_nothing);
  RUN_TEST(stepping_off_the_end_wraps_and_stays_in_the_band);
  RUN_TEST(a_step_size_the_band_refuses_is_refused);
  RUN_TEST(volume_outside_what_the_chip_takes_is_refused);
  RUN_TEST(muting_and_unmuting_works);
  RUN_TEST(automatic_bandwidth_is_only_offered_on_fm);
  RUN_TEST(meter_band_mode_is_only_available_on_shortwave);
  RUN_TEST(leaving_shortwave_drops_meter_band_mode);
  RUN_TEST(tuning_out_of_shortwave_also_drops_meter_band_mode);
  RUN_TEST(meter_band_mode_steps_between_meter_bands);
  RUN_TEST(moving_to_an_am_band_never_leaves_the_fm_automatic_bandwidth);
  RUN_TEST(the_most_negative_step_count_is_not_silently_ignored);
  RUN_TEST(a_command_that_is_not_a_command_is_refused);
  RUN_TEST(every_error_and_mode_has_words);
  RUN_TEST(only_changes_the_tuner_cares_about_ask_for_a_retune);
  return UNITY_END();
}
