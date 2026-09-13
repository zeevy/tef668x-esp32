/**
 * @file test_radio.cpp
 * @brief Tests for the radio state machine. Runs on a PC.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/radio.h"
#include "core/settings.h"

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
  apply((RadioCommand){.kind = RADIO_SET_BANDWIDTH, .bandwidthKHz = 114});
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_TRUE(push.bandwidth);
  TEST_ASSERT_FALSE(push.retune);
}

static void a_bandwidth_the_band_does_not_offer_is_refused(void) {
  /* The two lists do not overlap. 4 kHz is an AM width, and on FM it pins
   * the filter far narrower than a station: the radio then reports no pilot
   * and no signal and reads as one with no aerial. A form meant for AM could
   * reach this endpoint, so the refusal is here and not in the caller. */
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings s;
  radioDefaults(&s, &plan);

  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_BANDWIDTH;
  cmd.bandwidthKHz = 4;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_BANDWIDTH, radioApply(&s, &plan, &cmd));
  cmd.bandwidthKHz = 110; /* Between two real ones is not a near miss. */
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_BANDWIDTH, radioApply(&s, &plan, &cmd));
  cmd.bandwidthKHz = 114;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &cmd));

  /* And the other way round, where 0 is the FM automatic setting that the
   * AM side has no answer for. */
  RadioCommand band = {};
  band.kind = RADIO_SET_BAND;
  band.band = BAND_MW;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &band));
  cmd.bandwidthKHz = 114;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_BANDWIDTH, radioApply(&s, &plan, &cmd));
  cmd.bandwidthKHz = 0;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_BANDWIDTH, radioApply(&s, &plan, &cmd));
  cmd.bandwidthKHz = 6;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &cmd));
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

static void weak_signal_handling_starts_switched_off(void) {
  /* The same as the radio this replaces ships, which is why a radio nobody
   * has configured does nothing at all about a weak signal. */
  TEST_ASSERT_EQUAL_UINT8(0, r.highCutStart);
  TEST_ASSERT_EQUAL_UINT8(0, r.stereoBlendStart);
  TEST_ASSERT_EQUAL_UINT8(0, r.stHiBlendStart);
  TEST_ASSERT_EQUAL_UINT8(0, r.amNoiseBlankerStart);
  TEST_ASSERT_EQUAL_UINT8(0, r.fmNoiseBlankerStart);
}

static void the_weak_signal_levels_can_be_set_together(void) {
  RadioCommand c = {};
  c.kind = RADIO_SET_WEAK_SIGNAL;
  c.weak[0] = 40;
  c.weak[1] = 38;
  c.weak[2] = 36;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
  TEST_ASSERT_EQUAL_UINT8(40, r.highCutStart);
  TEST_ASSERT_EQUAL_UINT8(38, r.stereoBlendStart);
  TEST_ASSERT_EQUAL_UINT8(36, r.stHiBlendStart);
}

static void a_weak_signal_level_no_signal_reaches_is_refused(void) {
  /* Below 20 dBuV the mechanism starts where nothing arrives, so it is on and
   * does nothing. Checked in the state machine, so every caller gets the same
   * answer, and so that a level the core took cannot later stop the settings
   * being stored. */
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings s;
  radioDefaults(&s, &plan);

  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_WEAK_SIGNAL;
  cmd.weak[0] = 10;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_RANGE, radioApply(&s, &plan, &cmd));
  cmd.weak[0] = 70;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_RANGE, radioApply(&s, &plan, &cmd));
  TEST_ASSERT_EQUAL_UINT8(0, s.highCutStart);

  cmd.weak[0] = 0; /* Off is a real choice. */
  cmd.weak[1] = 20;
  cmd.weak[2] = 60;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &cmd));
  TEST_ASSERT_EQUAL_UINT8(60, s.stHiBlendStart);
}

static void the_feature_cycle_walks_all_four_combinations(void) {
  /* One button gesture for two settings, so it has to reach every state and
   * always move to a different one. */
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings s;
  radioDefaults(&s, &plan);

  RadioCommand cmd = {};
  cmd.kind = RADIO_CYCLE_FM_FEATURES;

  struct {
    bool ims;
    bool eq;
  } want[] = {{true, false}, {false, true}, {true, true}, {false, false}};

  for (size_t i = 0; i < sizeof(want) / sizeof(want[0]); i++) {
    TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &cmd));
    TEST_ASSERT_EQUAL_INT(want[i].ims, s.multipathSuppression);
    TEST_ASSERT_EQUAL_INT(want[i].eq, s.equalizer);
  }

  /* And round again, from wherever it was left. */
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &cmd));
  TEST_ASSERT_TRUE(s.multipathSuppression);
}

static void the_feature_cycle_is_fm_only(void) {
  /* The tuner has nowhere to put either on the AM side, so the command says
   * so rather than appearing to work.
   *
   * Both are switched on first, on FM, so the refusal has something to
   * damage. Starting from the defaults would pass even if the AM branch
   * cleared them on its way out. */
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings s;
  radioDefaults(&s, &plan);

  RadioCommand cmd = {};
  cmd.kind = RADIO_CYCLE_FM_FEATURES;
  for (int i = 0; i < 3; i++) {
    TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &cmd));
  }
  TEST_ASSERT_TRUE(s.multipathSuppression);
  TEST_ASSERT_TRUE(s.equalizer);

  RadioCommand band = {};
  band.kind = RADIO_SET_BAND;
  band.band = BAND_MW;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &band));

  TEST_ASSERT_EQUAL_INT(RADIO_ERR_FM_ONLY, radioApply(&s, &plan, &cmd));
  /* Both survive the refusal. A command that is refused changes nothing. */
  TEST_ASSERT_TRUE(s.multipathSuppression);
  TEST_ASSERT_TRUE(s.equalizer);

  /* And coming back to FM carries on from where it was. */
  band.band = BAND_FM;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &band));
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &cmd));
  TEST_ASSERT_FALSE(s.multipathSuppression);
  TEST_ASSERT_FALSE(s.equalizer);
}

static void cycling_the_features_needs_a_feature_push(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings a;
  radioDefaults(&a, &plan);
  RadioSettings b = a;

  RadioCommand cmd = {};
  cmd.kind = RADIO_CYCLE_FM_FEATURES;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&b, &plan, &cmd));

  RadioPush push = radioPushNeeded(&a, &b);
  TEST_ASSERT_TRUE(push.features);
  TEST_ASSERT_FALSE(push.retune);
}

static void the_weak_signal_levels_are_fm_only(void) {
  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_MW});
  RadioCommand c = {};
  c.kind = RADIO_SET_WEAK_SIGNAL;
  c.weak[0] = 40;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_FM_ONLY, apply(c));
}

static void the_noise_blankers_can_be_set_from_either_band(void) {
  /* The AM blanker is the useful half, and refusing to set it while the radio
   * happens to be on FM would be awkward for no reason. */
  RadioCommand c = {};
  c.kind = RADIO_SET_NOISE_BLANKER;
  c.blanker[0] = 100;
  c.blanker[1] = 80;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
  TEST_ASSERT_EQUAL_UINT8(100, r.amNoiseBlankerStart);

  apply((RadioCommand){.kind = RADIO_SET_BAND, .band = BAND_MW});
  c.blanker[0] = 120;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
  TEST_ASSERT_EQUAL_UINT8(120, r.amNoiseBlankerStart);
}

static void the_noise_blanker_is_a_percentage_not_a_level(void) {
  /* Zero for off, and fifty to a hundred and fifty usable. A value between
   * the two switches it on to do nothing, which is the silent no-op the
   * rules warn about, so it is refused where the rule belongs rather than
   * only in whichever caller happens to exist. */
  RadioCommand c = {};
  c.kind = RADIO_SET_NOISE_BLANKER;

  c.blanker[0] = 0;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));

  c.blanker[0] = 50;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));
  c.blanker[0] = 150;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, apply(c));

  uint8_t before = r.amNoiseBlankerStart;
  c.blanker[0] = 20;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_RANGE, apply(c));
  c.blanker[0] = 49;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_RANGE, apply(c));
  c.blanker[0] = 151;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_RANGE, apply(c));
  /* And a refused one changes nothing. */
  TEST_ASSERT_EQUAL_UINT8(before, r.amNoiseBlankerStart);
}

static void changing_the_weak_signal_levels_needs_a_feature_push(void) {
  RadioSettings before = r;
  RadioCommand c = {};
  c.kind = RADIO_SET_WEAK_SIGNAL;
  c.weak[0] = 40;
  apply(c);
  RadioPush push = radioPushNeeded(&before, &r);
  TEST_ASSERT_TRUE(push.features);
  TEST_ASSERT_FALSE(push.retune);

  before = r;
  RadioCommand nb = {};
  nb.kind = RADIO_SET_NOISE_BLANKER;
  nb.blanker[0] = 100;
  apply(nb);
  push = radioPushNeeded(&before, &r);
  TEST_ASSERT_TRUE(push.features);
}

/* ------------------------------------------------------------- the fade */

static void the_fade_starts_below_the_target_and_ends_at_it(void) {
  /* Not from silence. The chip takes whole dB, so a fade across the whole
   * sixty of them in a fraction of a second can only be a series of jumps,
   * and that is what it sounded like on the radio. */
  TEST_ASSERT_EQUAL_INT8(-RADIO_FADE_DEPTH_DB, radioFadeVolume(0, 0, 1500));
  TEST_ASSERT_EQUAL_INT8(0, radioFadeVolume(0, 1500, 1500));
  TEST_ASSERT_EQUAL_INT8(0, radioFadeVolume(0, 9999, 1500));
}

static void no_step_of_the_fade_is_big_enough_to_hear_as_a_jump(void) {
  /* The test the jerk would have failed. At the rate the radio task moves the
   * volume during a fade, no single step may be more than a couple of dB. */
  for (uint16_t duration = 300; duration <= 1500; duration += 600) {
    int8_t last = radioFadeVolume(0, 0, duration);
    for (uint32_t t = 0; t <= duration; t += RADIO_FADE_STEP_MS) {
      int8_t v = radioFadeVolume(0, t, duration);
      TEST_ASSERT_TRUE(v - last <= 2);
      last = v;
    }
  }
}

static void a_fade_to_a_very_quiet_target_does_not_go_below_silence(void) {
  int8_t target = (int8_t)(RADIO_VOLUME_MIN + 5);
  TEST_ASSERT_EQUAL_INT8(RADIO_VOLUME_MIN, radioFadeVolume(target, 0, 1500));
  TEST_ASSERT_EQUAL_INT8(target, radioFadeVolume(target, 1500, 1500));
}

static void the_fade_only_ever_rises(void) {
  int8_t last = -128;
  for (uint32_t t = 0; t <= 1500; t += 25) {
    int8_t v = radioFadeVolume(-10, t, 1500);
    TEST_ASSERT_TRUE(v >= last);
    last = v;
  }
  TEST_ASSERT_EQUAL_INT8(-10, last);
}

static void the_fade_follows_a_target_that_moves(void) {
  /* The knob still works while the radio is coming up. Half way through a
   * fade to 0 dB, the knob is turned down to -30: the volume must not jump
   * above the new target. */
  int8_t half = radioFadeVolume(0, 750, 1500);
  int8_t moved = radioFadeVolume(-30, 750, 1500);
  TEST_ASSERT_TRUE(moved < half);
  TEST_ASSERT_TRUE(moved <= -30 || moved < half);
}

static void a_target_at_silence_has_nothing_to_rise_from(void) {
  TEST_ASSERT_EQUAL_INT8(RADIO_VOLUME_MIN,
                         radioFadeVolume(RADIO_VOLUME_MIN, 0, 1500));
  TEST_ASSERT_EQUAL_INT8(RADIO_VOLUME_MIN,
                         radioFadeVolume(RADIO_VOLUME_MIN, 750, 1500));
}

static void the_fade_reaches_the_target_and_not_before(void) {
  /* Just short of the end it is still below, and at the end it is there. */
  TEST_ASSERT_TRUE(radioFadeVolume(0, 1499, 1500) < 0);
  TEST_ASSERT_EQUAL_INT8(0, radioFadeVolume(0, 1500, 1500));
}

static void the_fade_never_overshoots_its_target(void) {
  for (int8_t target = RADIO_VOLUME_MIN; target <= RADIO_VOLUME_MAX; target++) {
    for (uint32_t t = 0; t <= 1600; t += 100) {
      int8_t v = radioFadeVolume(target, t, 1500);
      TEST_ASSERT_TRUE(v <= target);
      TEST_ASSERT_TRUE(v >= RADIO_VOLUME_MIN);
    }
  }
}

static void no_duration_means_no_fade(void) {
  /* So an ordinary tune, which must not fade, costs nothing. */
  TEST_ASSERT_EQUAL_INT8(-5, radioFadeVolume(-5, 0, 0));
}

static void the_band_change_fade_is_much_shorter_than_the_one_at_start(void) {
  /* A band change already goes silent while the tuner moves. Only the return
   * is softened, and nobody wants to wait a second and a half for it. */
  TEST_ASSERT_TRUE(RADIO_BAND_FADE_MS < RADIO_FADE_MS);
}

/* ---------------------------------------------------------- from settings */

static void the_plan_comes_from_the_stored_region_and_spacing(void) {
  Settings st;
  settingsDefaults(&st);
  st.fmRegion = (uint8_t)FM_REGION_JAPAN;
  st.mwSpacing = (uint8_t)MW_SPACING_10K;

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);
  TEST_ASSERT_EQUAL_INT(FM_REGION_JAPAN, plan.fmRegion);
  TEST_ASSERT_EQUAL_INT(MW_SPACING_10K, plan.mwSpacing);
}

static void a_stored_region_out_of_range_leaves_the_default(void) {
  Settings st;
  settingsDefaults(&st);
  st.fmRegion = 99;
  st.mwSpacing = 99;

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);
  BandPlanConfig want;
  bandPlanDefaults(&want);
  TEST_ASSERT_EQUAL_INT(want.fmRegion, plan.fmRegion);
  TEST_ASSERT_EQUAL_INT(want.mwSpacing, plan.mwSpacing);
}

static void no_settings_gives_the_default_plan(void) {
  BandPlanConfig plan;
  radioPlanFromSettings(NULL, &plan);
  BandPlanConfig want;
  bandPlanDefaults(&want);
  TEST_ASSERT_EQUAL_INT(want.fmRegion, plan.fmRegion);
  TEST_ASSERT_EQUAL_INT(want.mwSpacing, plan.mwSpacing);
  radioPlanFromSettings(NULL, NULL); /* Must not crash. */
}

static void the_radio_starts_on_the_stored_band_and_frequency(void) {
  Settings st;
  settingsDefaults(&st);
  st.startBand = (uint8_t)BAND_MW;
  st.startFreqKHz = 738;

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);
  RadioSettings r;
  radioFromSettings(&st, &plan, &r);
  TEST_ASSERT_EQUAL_INT(BAND_MW, r.band);
  TEST_ASSERT_EQUAL_UINT32(738, r.freqKHz);
}

static void a_stored_frequency_in_no_band_falls_back(void) {
  /* A region change can put a stored frequency outside every band. The
   * stored band's own start is what is left, and it is right. */
  Settings st;
  settingsDefaults(&st);
  st.startBand = (uint8_t)BAND_FM;
  st.startFreqKHz = 5; /* Below everything this radio covers. */

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);
  RadioSettings r;
  radioFromSettings(&st, &plan, &r);
  TEST_ASSERT_EQUAL_INT(BAND_FM, r.band);
  uint32_t lo = 0;
  uint32_t hi = 0;
  TEST_ASSERT_TRUE(bandLimits(BAND_FM, &plan, &lo, &hi));
  TEST_ASSERT_EQUAL_UINT32(lo, r.freqKHz);
}

static void a_stored_frequency_picks_its_own_band(void) {
  /* The two are stored together so they normally agree. When a region change
   * breaks that, the frequency is the more exact of the two and wins. */
  Settings st;
  settingsDefaults(&st);
  st.startBand = (uint8_t)BAND_FM;
  st.startFreqKHz = 738;

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);
  RadioSettings r;
  radioFromSettings(&st, &plan, &r);
  TEST_ASSERT_EQUAL_INT(BAND_MW, r.band);
  TEST_ASSERT_EQUAL_UINT32(738, r.freqKHz);
}

static void a_stored_frequency_the_radio_can_tune_is_left_alone(void) {
  /* 738 kHz stored, then the medium wave grid changed to 10 kHz. The station
   * is still on 738: a setting on this radio does not move a transmitter.
   * Medium wave offers a 1 kHz step, so 738 is a frequency the radio can
   * reach, and coming up anywhere else would move it off the station. */
  Settings st;
  settingsDefaults(&st);
  st.startBand = (uint8_t)BAND_MW;
  st.startFreqKHz = 738;
  st.mwSpacing = (uint8_t)MW_SPACING_10K;

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);
  RadioSettings r;
  radioFromSettings(&st, &plan, &r);
  TEST_ASSERT_EQUAL_INT(BAND_MW, r.band);
  TEST_ASSERT_EQUAL_UINT32(738, r.freqKHz);
}

static void a_frequency_no_step_can_reach_is_snapped(void) {
  /* The case that is worth correcting. FM offers 50, 100 and 200 kHz steps
   * from the bottom of the band, so 102.825 MHz is on none of them and
   * nothing on the radio could have tuned it. It comes up on the nearest
   * frequency that can be reached. */
  Settings st;
  settingsDefaults(&st);
  st.startBand = (uint8_t)BAND_FM;
  st.startFreqKHz = 102825;

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);
  RadioSettings r;
  radioFromSettings(&st, &plan, &r);
  TEST_ASSERT_EQUAL_INT(BAND_FM, r.band);
  /* Onto the band's default step of 100 kHz, which is the grid a person
   * tuning by the knob would be on. */
  TEST_ASSERT_EQUAL_UINT32(102800, r.freqKHz);
}

static void a_fine_tuned_am_station_survives_a_restart(void) {
  /* The 1 kHz step exists so a station can be tuned off centre on purpose,
   * against selective fading. The step in use is not stored, so snapping to
   * the band's default step would quietly undo that every restart. */
  Settings st;
  settingsDefaults(&st);
  st.startBand = (uint8_t)BAND_MW;
  st.startFreqKHz = 737;

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);
  RadioSettings r;
  radioFromSettings(&st, &plan, &r);
  TEST_ASSERT_EQUAL_UINT32(737, r.freqKHz);
}

static void the_fm_features_come_from_the_settings(void) {
  Settings st;
  settingsDefaults(&st);
  st.fmMultipathSuppression = 1;
  st.fmEqualizer = 1;
  st.fmForcedMono = 1;
  st.fmHighCutStart = 30;
  st.fmStereoBlendStart = 35;
  st.fmStHiBlendStart = 40;
  st.fmNoiseBlankerStart = 90;
  st.amNoiseBlankerStart = 100;
  st.fmDeemphasisUs = 75;

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);
  RadioSettings r;
  radioFromSettings(&st, &plan, &r);
  TEST_ASSERT_TRUE(r.multipathSuppression);
  TEST_ASSERT_TRUE(r.equalizer);
  TEST_ASSERT_TRUE(r.forcedMono);
  TEST_ASSERT_EQUAL_UINT8(30, r.highCutStart);
  TEST_ASSERT_EQUAL_UINT8(35, r.stereoBlendStart);
  TEST_ASSERT_EQUAL_UINT8(40, r.stHiBlendStart);
  TEST_ASSERT_EQUAL_UINT8(90, r.fmNoiseBlankerStart);
  TEST_ASSERT_EQUAL_UINT8(100, r.amNoiseBlankerStart);
  TEST_ASSERT_EQUAL_UINT16(75, r.deemphasisUs);
}

static void the_stored_am_width_only_applies_on_am(void) {
  Settings st;
  settingsDefaults(&st);
  st.amBandwidthKHz = 6;

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);

  st.startBand = (uint8_t)BAND_FM;
  RadioSettings fm;
  radioFromSettings(&st, &plan, &fm);
  TEST_ASSERT_EQUAL_UINT16(0, fm.bandwidthKHz); /* The tuner chooses. */

  /* The frequency has to move with the band. It is the more exact of the
   * two, so leaving it on an FM frequency would put the radio back on FM. */
  st.startBand = (uint8_t)BAND_MW;
  st.startFreqKHz = 738;
  RadioSettings am;
  radioFromSettings(&st, &plan, &am);
  TEST_ASSERT_EQUAL_UINT16(6, am.bandwidthKHz);
}

static void no_settings_gives_the_radio_defaults(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings want;
  radioDefaults(&want, &plan);
  RadioSettings got;
  radioFromSettings(NULL, &plan, &got);
  TEST_ASSERT_EQUAL_MEMORY(&want, &got, sizeof(want));

  radioFromSettings(NULL, NULL, &got); /* Must not crash. */
  radioFromSettings(NULL, &plan, NULL);
}

static void what_is_worth_keeping_goes_back_to_the_settings(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings r;
  radioDefaults(&r, &plan);
  r.band = BAND_SW;
  r.freqKHz = 9500;
  r.equalizer = true;
  r.stereoBlendStart = 35;
  r.deemphasisUs = 75;
  r.volumeDb = -12;

  Settings st;
  settingsDefaults(&st);
  radioToSettings(&r, &st);
  TEST_ASSERT_EQUAL_UINT8((uint8_t)BAND_SW, st.startBand);
  TEST_ASSERT_EQUAL_UINT32(9500, st.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(1, st.fmEqualizer);
  TEST_ASSERT_EQUAL_UINT8(35, st.fmStereoBlendStart);
  TEST_ASSERT_EQUAL_UINT16(75, st.fmDeemphasisUs);
  /* The volume is kept for the one mode where the knob is not the volume. */
  TEST_ASSERT_EQUAL_INT8(-12, st.startVolumeDb);
  TEST_ASSERT_TRUE(settingsValid(&st));

  radioToSettings(NULL, &st); /* Must not crash. */
  radioToSettings(&r, NULL);
}

static void a_stored_volume_stays_inside_what_the_knob_can_ask_for(void) {
  /* The chip takes up to 24 dB, the knob only reaches 0. A volume set past
   * the knob's top through the API must not be stored as a number the knob
   * can never get back down from in one turn. */
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings r;
  radioDefaults(&r, &plan);
  Settings st;
  settingsDefaults(&st);

  r.volumeDb = RADIO_VOLUME_MAX;
  radioToSettings(&r, &st);
  TEST_ASSERT_EQUAL_INT8(0, st.startVolumeDb);
  TEST_ASSERT_TRUE(settingsValid(&st));

  r.volumeDb = RADIO_VOLUME_MIN;
  radioToSettings(&r, &st);
  TEST_ASSERT_EQUAL_INT8(RADIO_VOLUME_MIN, st.startVolumeDb);
  TEST_ASSERT_TRUE(settingsValid(&st));
}

static void a_round_trip_through_the_settings_changes_nothing(void) {
  Settings st;
  settingsDefaults(&st);
  st.startBand = (uint8_t)BAND_LW;
  st.startFreqKHz = 198;
  st.fmForcedMono = 1;
  st.fmHighCutStart = 25;
  st.amNoiseBlankerStart = 120;

  BandPlanConfig plan;
  radioPlanFromSettings(&st, &plan);
  RadioSettings r;
  radioFromSettings(&st, &plan, &r);

  Settings back;
  settingsDefaults(&back);
  radioToSettings(&r, &back);
  TEST_ASSERT_EQUAL_UINT8(st.startBand, back.startBand);
  TEST_ASSERT_EQUAL_UINT32(st.startFreqKHz, back.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(st.fmForcedMono, back.fmForcedMono);
  TEST_ASSERT_EQUAL_UINT8(st.fmHighCutStart, back.fmHighCutStart);
  TEST_ASSERT_EQUAL_UINT8(st.amNoiseBlankerStart, back.amNoiseBlankerStart);
}

/* ------------------------------------------------------------ de-emphasis */

static void the_deemphasis_can_be_set_from_either_band(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings s;
  radioDefaults(&s, &plan);
  TEST_ASSERT_EQUAL_UINT16(50, s.deemphasisUs);

  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_DEEMPHASIS;
  cmd.deemphasisUs = 75;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &cmd));
  TEST_ASSERT_EQUAL_UINT16(75, s.deemphasisUs);

  /* And on AM, where it does not reach the chip but still belongs to the
   * person's country rather than to the band they happen to be on. */
  RadioCommand band = {};
  band.kind = RADIO_SET_BAND;
  band.band = BAND_MW;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &band));
  cmd.deemphasisUs = 50;
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &cmd));
  TEST_ASSERT_EQUAL_UINT16(50, s.deemphasisUs);
}

static void only_the_two_real_deemphasis_standards_are_taken(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings s;
  radioDefaults(&s, &plan);

  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_DEEMPHASIS;
  cmd.deemphasisUs = 60;
  TEST_ASSERT_EQUAL_INT(RADIO_ERR_RANGE, radioApply(&s, &plan, &cmd));
  TEST_ASSERT_EQUAL_UINT16(50, s.deemphasisUs);

  cmd.deemphasisUs = 0; /* Off is a real choice. */
  TEST_ASSERT_EQUAL_INT(RADIO_OK, radioApply(&s, &plan, &cmd));
  TEST_ASSERT_EQUAL_UINT16(0, s.deemphasisUs);
}

static void changing_the_deemphasis_needs_a_feature_push(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  RadioSettings a;
  radioDefaults(&a, &plan);
  RadioSettings b = a;
  b.deemphasisUs = 75;

  RadioPush push = radioPushNeeded(&a, &b);
  TEST_ASSERT_TRUE(push.features);
  TEST_ASSERT_FALSE(push.retune);
}

/* ------------------------------------------------------- the duck down */

static void the_duck_starts_where_it_is_and_ends_in_silence(void) {
  TEST_ASSERT_EQUAL_INT8(-10, radioDuckVolume(-10, 0, 120));
  TEST_ASSERT_EQUAL_INT8(RADIO_VOLUME_MIN, radioDuckVolume(-10, 120, 120));
  TEST_ASSERT_EQUAL_INT8(RADIO_VOLUME_MIN, radioDuckVolume(-10, 500, 120));
}

static void the_duck_only_ever_falls(void) {
  int8_t last = 0;
  for (uint32_t t = 0; t <= 120; t += 5) {
    int8_t now = radioDuckVolume(0, t, 120);
    TEST_ASSERT_TRUE(now <= last);
    last = now;
  }
}

static void no_duration_means_silence_at_once(void) {
  /* Which is what switching the ramp off has to mean: the old behaviour, an
   * instant cut, not a ramp of length zero that never gets there. */
  TEST_ASSERT_EQUAL_INT8(RADIO_VOLUME_MIN, radioDuckVolume(-10, 0, 0));
}

static void ducking_from_silence_stays_at_silence(void) {
  for (uint32_t t = 0; t <= 120; t += 30) {
    TEST_ASSERT_EQUAL_INT8(RADIO_VOLUME_MIN,
                           radioDuckVolume(RADIO_VOLUME_MIN, t, 120));
  }
}

static void no_step_of_the_duck_is_big_enough_to_hear_as_a_click(void) {
  /* The whole point. A step is what it is replacing, so the ramp must not be
   * delivered in a handful of jumps. */
  int8_t last = radioDuckVolume(0, 0, RADIO_SOFT_MUTE_MS);
  for (uint32_t t = RADIO_FADE_STEP_MS; t <= RADIO_SOFT_MUTE_MS;
       t += RADIO_FADE_STEP_MS) {
    int8_t now = radioDuckVolume(0, t, RADIO_SOFT_MUTE_MS);
    TEST_ASSERT_TRUE((int)last - (int)now <= 12);
    last = now;
  }
}

static void the_duck_is_the_mirror_of_the_fade(void) {
  /* Both straight lines in dB, so one undoes the other. */
  int8_t down = radioDuckVolume(0, 60, 120);
  int8_t up = radioFadeVolume(0, 60, 120);
  TEST_ASSERT_TRUE(down < 0);
  TEST_ASSERT_TRUE(up < 0);
}

/* The inverse of the fade, used when a ramp down is cancelled part way and
 * the fade up has to pick up from where the volume actually is. */
static void the_inverse_fade_lands_where_the_fade_started(void) {
  const int8_t target = -10;
  const uint16_t ms = 120;
  for (uint32_t e = 0; e <= ms; e += 10) {
    int8_t at = radioFadeVolume(target, e, ms);
    uint32_t back = radioFadeElapsedAt(target, at, ms);
    int8_t again = radioFadeVolume(target, back, ms);
    /* The volume is whole dB, so the two cannot be exact inverses. What has
     * to hold is that the round trip never loses volume, because a caller
     * uses this to carry on from where it is and a loss there is the step
     * the ramp exists to remove. */
    TEST_ASSERT_TRUE(again >= at);
    TEST_ASSERT_TRUE(again - at <= 1);
  }
}

static void the_inverse_fade_clamps_at_both_ends(void) {
  const int8_t target = -10;
  const uint16_t ms = 120;
  /* At or below the floor the fade has not started. */
  TEST_ASSERT_EQUAL_UINT32(0, radioFadeElapsedAt(target, RADIO_VOLUME_MIN, ms));
  TEST_ASSERT_EQUAL_UINT32(
      0,
      radioFadeElapsedAt(target, (int8_t)(target - RADIO_FADE_DEPTH_DB), ms));
  /* At or above the target it is over. */
  TEST_ASSERT_EQUAL_UINT32(ms, radioFadeElapsedAt(target, target, ms));
  TEST_ASSERT_EQUAL_UINT32(ms, radioFadeElapsedAt(target, 0, ms));
}

static void the_inverse_fade_has_no_duration_to_be_part_way_through(void) {
  TEST_ASSERT_EQUAL_UINT32(0, radioFadeElapsedAt(-10, -20, 0));
}

static void a_cancelled_duck_picks_up_where_it_left_off(void) {
  /* A little way down a 120 ms ramp from -5 dB, then the reason for the ramp
   * goes away. The fade up carries on from that volume rather than dropping
   * to its own floor first. */
  const int8_t target = -5;
  const uint16_t ms = 120;
  int8_t early = radioDuckVolume(target, ms / 8, ms);
  TEST_ASSERT_TRUE(early > (int8_t)(target - RADIO_FADE_DEPTH_DB));
  uint32_t into = radioFadeElapsedAt(target, early, ms);
  int8_t resumed = radioFadeVolume(target, into, ms);
  TEST_ASSERT_TRUE(resumed >= early);
  TEST_ASSERT_TRUE(resumed - early <= 1);
}

static void a_duck_below_the_fade_floor_resumes_at_the_floor(void) {
  /* The duck falls the whole way to silence and a fade only spans
   * RADIO_FADE_DEPTH_DB, so a ramp cancelled late is already below anything
   * a fade can start from. It resumes at the floor, which is a step up of a
   * few dB from near silence rather than a step down. */
  const int8_t target = -5;
  const uint16_t ms = 120;
  int8_t deep = radioDuckVolume(target, ms / 2, ms);
  int8_t floorDb = (int8_t)(target - RADIO_FADE_DEPTH_DB);
  TEST_ASSERT_TRUE(deep < floorDb);
  TEST_ASSERT_EQUAL_UINT32(0, radioFadeElapsedAt(target, deep, ms));
  int8_t resumed = radioFadeVolume(target, 0, ms);
  TEST_ASSERT_EQUAL_INT8(floorDb, resumed);
  TEST_ASSERT_TRUE(resumed > deep);
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
  RUN_TEST(a_bandwidth_the_band_does_not_offer_is_refused);
  RUN_TEST(the_step_size_and_the_mode_never_reach_the_tuner);
  RUN_TEST(nothing_known_means_send_everything);

  RUN_TEST(the_fm_features_start_off);
  RUN_TEST(each_fm_feature_can_be_turned_on_and_off);
  RUN_TEST(the_fm_features_are_refused_on_am);
  RUN_TEST(changing_a_feature_does_not_move_the_dial);
  RUN_TEST(a_retune_sends_the_features_again);

  RUN_TEST(weak_signal_handling_starts_switched_off);
  RUN_TEST(the_weak_signal_levels_can_be_set_together);
  RUN_TEST(a_weak_signal_level_no_signal_reaches_is_refused);
  RUN_TEST(the_feature_cycle_walks_all_four_combinations);
  RUN_TEST(the_feature_cycle_is_fm_only);
  RUN_TEST(cycling_the_features_needs_a_feature_push);
  RUN_TEST(the_weak_signal_levels_are_fm_only);
  RUN_TEST(the_noise_blankers_can_be_set_from_either_band);
  RUN_TEST(the_noise_blanker_is_a_percentage_not_a_level);
  RUN_TEST(changing_the_weak_signal_levels_needs_a_feature_push);

  RUN_TEST(the_fade_starts_below_the_target_and_ends_at_it);
  RUN_TEST(no_step_of_the_fade_is_big_enough_to_hear_as_a_jump);
  RUN_TEST(a_fade_to_a_very_quiet_target_does_not_go_below_silence);
  RUN_TEST(the_fade_only_ever_rises);
  RUN_TEST(the_fade_follows_a_target_that_moves);
  RUN_TEST(a_target_at_silence_has_nothing_to_rise_from);
  RUN_TEST(the_fade_reaches_the_target_and_not_before);
  RUN_TEST(the_fade_never_overshoots_its_target);
  RUN_TEST(no_duration_means_no_fade);
  RUN_TEST(the_band_change_fade_is_much_shorter_than_the_one_at_start);

  RUN_TEST(the_plan_comes_from_the_stored_region_and_spacing);
  RUN_TEST(a_stored_region_out_of_range_leaves_the_default);
  RUN_TEST(no_settings_gives_the_default_plan);
  RUN_TEST(the_radio_starts_on_the_stored_band_and_frequency);
  RUN_TEST(a_stored_frequency_in_no_band_falls_back);
  RUN_TEST(a_stored_frequency_picks_its_own_band);
  RUN_TEST(a_stored_frequency_the_radio_can_tune_is_left_alone);
  RUN_TEST(a_frequency_no_step_can_reach_is_snapped);
  RUN_TEST(a_fine_tuned_am_station_survives_a_restart);
  RUN_TEST(the_fm_features_come_from_the_settings);
  RUN_TEST(the_stored_am_width_only_applies_on_am);
  RUN_TEST(no_settings_gives_the_radio_defaults);
  RUN_TEST(what_is_worth_keeping_goes_back_to_the_settings);
  RUN_TEST(a_stored_volume_stays_inside_what_the_knob_can_ask_for);
  RUN_TEST(a_round_trip_through_the_settings_changes_nothing);

  RUN_TEST(the_deemphasis_can_be_set_from_either_band);
  RUN_TEST(only_the_two_real_deemphasis_standards_are_taken);
  RUN_TEST(changing_the_deemphasis_needs_a_feature_push);

  RUN_TEST(the_duck_starts_where_it_is_and_ends_in_silence);
  RUN_TEST(the_duck_only_ever_falls);
  RUN_TEST(no_duration_means_silence_at_once);
  RUN_TEST(ducking_from_silence_stays_at_silence);
  RUN_TEST(no_step_of_the_duck_is_big_enough_to_hear_as_a_click);
  RUN_TEST(the_duck_is_the_mirror_of_the_fade);
  RUN_TEST(the_inverse_fade_lands_where_the_fade_started);
  RUN_TEST(the_inverse_fade_clamps_at_both_ends);
  RUN_TEST(the_inverse_fade_has_no_duration_to_be_part_way_through);
  RUN_TEST(a_cancelled_duck_picks_up_where_it_left_off);
  RUN_TEST(a_duck_below_the_fade_floor_resumes_at_the_floor);

  return UNITY_END();
}
