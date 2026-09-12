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

/* ------------------------------------------- coming back to a band */

static void a_band_remembers_where_it_was_left(void) {
  apply((RadioCommand){.kind = RADIO_TUNE, .freqKHz = 102800});
  TEST_ASSERT_EQUAL_UINT32(102800, r.freqKHz);

  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_MW});
  apply((RadioCommand){.kind = RADIO_TUNE, .freqKHz = 738});
  TEST_ASSERT_EQUAL_UINT32(738, r.freqKHz);

  /* Back to FM, and back to the station that was playing. */
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_FM});
  TEST_ASSERT_EQUAL_UINT32(102800, r.freqKHz);

  /* And medium wave still remembers its own. */
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_MW});
  TEST_ASSERT_EQUAL_UINT32(738, r.freqKHz);
}

static void a_band_never_visited_starts_at_the_bottom(void) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  TEST_ASSERT_TRUE(bandLimits(BAND_SW, &plan, &lo, &hi));
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_SW});
  TEST_ASSERT_EQUAL_UINT32(lo, r.freqKHz);
}

static void choosing_the_band_already_in_use_changes_nothing(void) {
  apply((RadioCommand){.kind = RADIO_TUNE, .freqKHz = 102800});
  TEST_ASSERT_EQUAL_INT(
      RADIO_OK, apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_FM}));
  /* Not sent back to the bottom of the band, which is what made pressing
   * BAND twice lose the station. */
  TEST_ASSERT_EQUAL_UINT32(102800, r.freqKHz);
}

static void a_remembered_frequency_outside_the_band_is_not_used(void) {
  /* Japan only reaches 95 MHz. A frequency remembered from a wider region is
   * no longer in the band and must not be tuned. */
  apply((RadioCommand){.kind = RADIO_TUNE, .freqKHz = 102800});
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_MW});

  plan.fmRegion = FM_REGION_JAPAN;
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_FM});

  uint32_t lo = 0;
  uint32_t hi = 0;
  TEST_ASSERT_TRUE(bandLimits(BAND_FM, &plan, &lo, &hi));
  TEST_ASSERT_EQUAL_UINT32(lo, r.freqKHz);
}

static void meter_band_is_only_offered_on_shortwave(void) {
  for (int b = 0; b < BAND_COUNT; b++) {
    bool want = ((BandId)b == BAND_SW);
    TEST_ASSERT_EQUAL_INT(
        want, radioTuneModeAllowed(TUNE_MODE_METER_BAND, (BandId)b));
    /* The other three are available everywhere. */
    TEST_ASSERT_TRUE(radioTuneModeAllowed(TUNE_MODE_MANUAL, (BandId)b));
    TEST_ASSERT_TRUE(radioTuneModeAllowed(TUNE_MODE_AUTO, (BandId)b));
    TEST_ASSERT_TRUE(radioTuneModeAllowed(TUNE_MODE_MEMORY, (BandId)b));
  }
}

/* ------------------------------------------------ the next one, not a guess */

static void tuning_away_by_frequency_also_remembers_the_band(void) {
  /* The keypad leaves a band by tuning, not by pressing BAND, and that is the
   * usual way out. Without this the memory is lost on the common route. */
  apply((RadioCommand){.kind = RADIO_TUNE, .freqKHz = 90000});
  TEST_ASSERT_EQUAL_INT(BAND_FM, r.band);

  apply((RadioCommand){.kind = RADIO_TUNE, .freqKHz = 738});
  TEST_ASSERT_EQUAL_INT(BAND_MW, r.band);

  apply((RadioCommand){.kind = RADIO_CYCLE_BAND});
  while (r.band != BAND_FM) {
    apply((RadioCommand){.kind = RADIO_CYCLE_BAND});
  }
  TEST_ASSERT_EQUAL_UINT32(90000, r.freqKHz);
}

static void cycling_the_band_goes_round_every_band_and_back(void) {
  BandId first = r.band;
  for (int i = 0; i < BAND_COUNT; i++) {
    TEST_ASSERT_EQUAL_INT(RADIO_OK,
                          apply((RadioCommand){.kind = RADIO_CYCLE_BAND}));
  }
  TEST_ASSERT_EQUAL_INT(first, r.band);
}

static void cycling_the_bandwidth_uses_the_band_it_is_actually_on(void) {
  /* The bug this replaces: the caller read FM, the radio moved to medium
   * wave, and the caller then sent 56 kHz to a band whose widest filter is
   * 8 kHz. Worked out here, there is no gap for the state to move in. */
  apply((RadioCommand){.kind = RADIO_TUNE, .freqKHz = 738});
  TEST_ASSERT_EQUAL_INT(BAND_MW, r.band);

  for (int i = 0; i < 12; i++) {
    TEST_ASSERT_EQUAL_INT(RADIO_OK,
                          apply((RadioCommand){.kind = RADIO_CYCLE_BANDWIDTH}));
    /* Never anything the AM side does not have. */
    TEST_ASSERT_TRUE(r.bandwidthKHz >= 3 && r.bandwidthKHz <= 8);
  }
}

static void cycling_the_bandwidth_on_fm_walks_the_whole_list(void) {
  size_t count = bandBandwidthCount(BAND_FM);
  for (size_t i = 1; i <= count; i++) {
    apply((RadioCommand){.kind = RADIO_CYCLE_BANDWIDTH});
    TEST_ASSERT_EQUAL_UINT16(bandBandwidthAt(BAND_FM, i % count),
                             r.bandwidthKHz);
  }
}

static void cycling_the_mode_skips_what_the_band_does_not_offer(void) {
  /* On FM, meter band never appears however many times the button is
   * pressed. */
  for (int i = 0; i < 12; i++) {
    apply((RadioCommand){.kind = RADIO_CYCLE_TUNE_MODE});
    TEST_ASSERT_NOT_EQUAL_INT(TUNE_MODE_METER_BAND, r.tuneMode);
  }

  /* On shortwave it does. */
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_SW});
  bool seen = false;
  for (int i = 0; i < 12; i++) {
    apply((RadioCommand){.kind = RADIO_CYCLE_TUNE_MODE});
    if (r.tuneMode == TUNE_MODE_METER_BAND) {
      seen = true;
    }
  }
  TEST_ASSERT_TRUE(seen);
}

static void cycling_the_mode_always_moves(void) {
  for (int i = 0; i < 8; i++) {
    TuneMode before = r.tuneMode;
    apply((RadioCommand){.kind = RADIO_CYCLE_TUNE_MODE});
    TEST_ASSERT_NOT_EQUAL_INT(before, r.tuneMode);
  }
}

static void toggling_mute_turns_it_over_each_time(void) {
  bool before = r.muted;
  apply((RadioCommand){.kind = RADIO_TOGGLE_MUTE});
  TEST_ASSERT_EQUAL_INT(!before, r.muted);
  apply((RadioCommand){.kind = RADIO_TOGGLE_MUTE});
  TEST_ASSERT_EQUAL_INT(before, r.muted);
}

static void changing_band_never_leaves_a_bandwidth_the_band_refuses(void) {
  /* Walk FM to a wide filter, then go round every band cycling the bandwidth
   * at each stop. Nothing may end up outside the list for the band it is on. */
  for (int i = 0; i < 6; i++) {
    apply((RadioCommand){.kind = RADIO_CYCLE_BANDWIDTH});
  }
  for (int b = 0; b < BAND_COUNT * 2; b++) {
    apply((RadioCommand){.kind = RADIO_CYCLE_BAND});
    apply((RadioCommand){.kind = RADIO_CYCLE_BANDWIDTH});
    bool found = false;
    for (size_t i = 0; i < bandBandwidthCount(r.band); i++) {
      if (bandBandwidthAt(r.band, i) == r.bandwidthKHz) {
        found = true;
      }
    }
    TEST_ASSERT_TRUE(found);
  }
}

/* ------------------------------------------- only send what actually moved */

static void turning_the_volume_does_not_move_the_dial(void) {
  /* The one that matters. A volume change that counts as a retune makes the
   * task mute and unmute around it, and the knob sends one of those every
   * fiftieth of a second while it is turned. That is audible as the sound
   * breaking up. */
  RadioSettings before = r;
  r.volumeDb = -12;
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_FALSE(push.retune);
  TEST_ASSERT_FALSE(push.bandwidth);
  TEST_ASSERT_FALSE(push.mute);
  TEST_ASSERT_TRUE(push.volume);
}

static void moving_the_dial_is_a_retune(void) {
  RadioSettings before = r;
  apply((RadioCommand){.kind = RADIO_TUNE, .freqKHz = 102800});
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_TRUE(push.retune);
  /* And the width goes with it, because a band can change under a tune. */
  TEST_ASSERT_TRUE(push.bandwidth);
}

static void crossing_between_fm_and_am_sends_the_volume_again(void) {
  /* Not because the number moved. Crossing sides makes the driver put the
   * chip back into its active mode, and whether that resets the gain is not
   * settled. Sending it again is one write and removes the question. */
  RadioSettings before = r;
  TEST_ASSERT_EQUAL_INT(BAND_FM, before.band);
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_MW});
  TEST_ASSERT_EQUAL_INT8(before.volumeDb, r.volumeDb);
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_TRUE(push.volume);

  /* Moving between two AM bands does not cross sides, so it does not. */
  RadioSettings onMw = r;
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_SW});
  push = radioPushNeeded(&onMw, &r);
  TEST_ASSERT_TRUE(push.retune);
  TEST_ASSERT_FALSE(push.volume);
}

static void changing_band_carries_the_bandwidth_with_it(void) {
  RadioSettings before = r;
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_MW});
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_TRUE(push.retune);
  TEST_ASSERT_TRUE(push.bandwidth);
}

static void muting_only_sets_the_mute(void) {
  RadioSettings before = r;
  apply((RadioCommand){.kind = RADIO_TOGGLE_MUTE});
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_TRUE(push.mute);
  TEST_ASSERT_FALSE(push.retune);
  TEST_ASSERT_FALSE(push.volume);
}

static void changing_the_bandwidth_alone_does_not_retune(void) {
  RadioSettings before = r;
  apply((RadioCommand){.kind = RADIO_SET_BANDWIDTH, .bandwidthKHz = 110});
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_TRUE(push.bandwidth);
  TEST_ASSERT_FALSE(push.retune);
}

static void the_step_size_and_the_mode_never_reach_the_tuner(void) {
  RadioSettings before = r;
  apply((RadioCommand){.kind = RADIO_SET_STEP, .stepKHz = 50});
  apply(
      (RadioCommand){.kind = RADIO_SET_TUNE_MODE, .tuneMode = TUNE_MODE_AUTO});
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_FALSE(push.retune);
  TEST_ASSERT_FALSE(push.bandwidth);
  TEST_ASSERT_FALSE(push.volume);
  TEST_ASSERT_FALSE(push.mute);
  TEST_ASSERT_FALSE(radioNeedsRetune(&before, &r));
}

static void nothing_known_means_send_everything(void) {
  /* The first push after start up, and any push after a failure, when what
   * the tuner is set to is not known. */
  RadioPush push = radioPushNeeded(NULL, &r);
  TEST_ASSERT_TRUE(push.retune);
  TEST_ASSERT_TRUE(push.bandwidth);
  TEST_ASSERT_TRUE(push.volume);
  TEST_ASSERT_TRUE(push.mute);
}

/* ------------------------------------------------------- the FM features */

static void the_fm_features_start_off(void) {
  /* The same as the radio this replaces ships. Its setting for these is
   * stored inverted, so a 1 there means off, which is worth not copying. */
  TEST_ASSERT_FALSE(r.multipathSuppression);
  TEST_ASSERT_FALSE(r.equalizer);
  TEST_ASSERT_FALSE(r.forcedMono);
}

static void each_fm_feature_can_be_turned_on_and_off(void) {
  TEST_ASSERT_EQUAL_INT(
      RADIO_OK,
      apply((RadioCommand){.kind = RADIO_SET_MPH_SUPPRESSION, .on = true}));
  TEST_ASSERT_TRUE(r.multipathSuppression);
  TEST_ASSERT_EQUAL_INT(
      RADIO_OK, apply((RadioCommand){.kind = RADIO_SET_EQUALIZER, .on = true}));
  TEST_ASSERT_TRUE(r.equalizer);
  TEST_ASSERT_EQUAL_INT(
      RADIO_OK, apply((RadioCommand){.kind = RADIO_SET_MONO, .on = true}));
  TEST_ASSERT_TRUE(r.forcedMono);

  apply((RadioCommand){.kind = RADIO_SET_MPH_SUPPRESSION, .on = false});
  apply((RadioCommand){.kind = RADIO_SET_EQUALIZER, .on = false});
  apply((RadioCommand){.kind = RADIO_SET_MONO, .on = false});
  TEST_ASSERT_FALSE(r.multipathSuppression);
  TEST_ASSERT_FALSE(r.equalizer);
  TEST_ASSERT_FALSE(r.forcedMono);
}

static void the_fm_features_are_refused_on_am(void) {
  /* The chip has nowhere to put them on the AM side. Refusing says so,
   * where a write that quietly goes nowhere would not. */
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_MW});
  TEST_ASSERT_EQUAL_INT(
      RADIO_ERR_FM_ONLY,
      apply((RadioCommand){.kind = RADIO_SET_MPH_SUPPRESSION, .on = true}));
  TEST_ASSERT_EQUAL_INT(
      RADIO_ERR_FM_ONLY,
      apply((RadioCommand){.kind = RADIO_SET_EQUALIZER, .on = true}));
  TEST_ASSERT_EQUAL_INT(
      RADIO_ERR_FM_ONLY,
      apply((RadioCommand){.kind = RADIO_SET_MONO, .on = true}));
  /* And the reason is its own, not the one for a band that does not exist. */
  TEST_ASSERT_EQUAL_STRING("that only works on FM",
                           radioErrorText(RADIO_ERR_FM_ONLY));
  TEST_ASSERT_FALSE(r.multipathSuppression);
}

static void changing_a_feature_does_not_move_the_dial(void) {
  RadioSettings before = r;
  apply((RadioCommand){.kind = RADIO_SET_MPH_SUPPRESSION, .on = true});
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_TRUE(push.features);
  TEST_ASSERT_FALSE(push.retune);
  TEST_ASSERT_FALSE(push.volume);
  TEST_ASSERT_FALSE(push.mute);
}

static void a_retune_sends_the_features_again(void) {
  /* Crossing to the AM side and back puts the chip through its active mode,
   * and what it keeps across that is not documented. */
  apply((RadioCommand){.kind = RADIO_SET_MPH_SUPPRESSION, .on = true});
  RadioSettings before = r;
  apply((RadioCommand){.kind = RADIO_TUNE, .freqKHz = 98300});
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_TRUE(push.retune);
  TEST_ASSERT_TRUE(push.features);
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
  RUN_TEST(a_band_remembers_where_it_was_left);
  RUN_TEST(a_band_never_visited_starts_at_the_bottom);
  RUN_TEST(choosing_the_band_already_in_use_changes_nothing);
  RUN_TEST(a_remembered_frequency_outside_the_band_is_not_used);
  RUN_TEST(meter_band_is_only_offered_on_shortwave);

  RUN_TEST(tuning_away_by_frequency_also_remembers_the_band);
  RUN_TEST(cycling_the_band_goes_round_every_band_and_back);
  RUN_TEST(cycling_the_bandwidth_uses_the_band_it_is_actually_on);
  RUN_TEST(cycling_the_bandwidth_on_fm_walks_the_whole_list);
  RUN_TEST(cycling_the_mode_skips_what_the_band_does_not_offer);
  RUN_TEST(cycling_the_mode_always_moves);
  RUN_TEST(toggling_mute_turns_it_over_each_time);
  RUN_TEST(changing_band_never_leaves_a_bandwidth_the_band_refuses);

  RUN_TEST(turning_the_volume_does_not_move_the_dial);
  RUN_TEST(moving_the_dial_is_a_retune);
  RUN_TEST(crossing_between_fm_and_am_sends_the_volume_again);
  RUN_TEST(changing_band_carries_the_bandwidth_with_it);
  RUN_TEST(muting_only_sets_the_mute);
  RUN_TEST(changing_the_bandwidth_alone_does_not_retune);
  RUN_TEST(the_step_size_and_the_mode_never_reach_the_tuner);
  RUN_TEST(nothing_known_means_send_everything);

  RUN_TEST(the_fm_features_start_off);
  RUN_TEST(each_fm_feature_can_be_turned_on_and_off);
  RUN_TEST(the_fm_features_are_refused_on_am);
  RUN_TEST(changing_a_feature_does_not_move_the_dial);
  RUN_TEST(a_retune_sends_the_features_again);

  return UNITY_END();
}
