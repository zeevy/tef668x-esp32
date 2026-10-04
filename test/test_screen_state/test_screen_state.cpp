/*
 * Tests for the radio screen's state builder. Runs on a PC.
 *
 * The builder is glue: it turns the radio's snapshot and the other inputs
 * into the words and numbers the panel draws. These tests check what it holds
 * still between builds, what it reads only when it changes, and what it
 * leaves out.
 */
#include <unity.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "core/signal.h"
#include "screen_state.h"

#include "../test_meter/captures.h"

static RadioSnapshot snap;
static ScreenInputs in;
static ScreenBuild build;
static ScreenState state;

/* The fake channel store: what it returns, and how often it was asked. */
static int reads;
static int lastReadSlot;
static const char *storedName; /* NULL for an empty slot. */
static bool storedNameUnterminated;

static bool fakeRead(int slot, MemoryChannel *out) {
  reads++;
  lastReadSlot = slot;
  if (storedName == NULL && !storedNameUnterminated) {
    return false;
  }
  memset(out, 0, sizeof(*out));
  out->freqKHz = 106400;
  if (storedNameUnterminated) {
    /* Every byte of the field used, with no terminator. */
    memset(out->name, 'A', sizeof(out->name));
  } else {
    snprintf(out->name, sizeof(out->name), "%s", storedName);
  }
  return true;
}

/*
 * 106.40 as a seek sweep off the radio read it: 36.7 dBuV and 57
 * per cent modulation. On no stored channel, the squelch open, Wi-Fi off.
 */
void setUp(void) {
  memset(&snap, 0, sizeof(snap));
  memset(&in, 0, sizeof(in));
  memset(&state, 0, sizeof(state));
  bandPlanDefaults(&in.plan);
  in.planValid = true;
  radioDefaults(&snap.settings, &in.plan);
  snap.settings.band = BAND_FM;
  snap.settings.freqKHz = 106400;
  snap.quality.levelDbuVTenths = 367;
  snap.quality.modulationPercent = 57;
  snap.qualityValid = true;
  snap.qualityReads = 1;
  snap.levelSmoothedTenths = 367;
  snap.levelSmoothedValid = true;
  snap.tunerReady = true;
  snap.squelchMode = SQUELCH_AUTO;
  snap.squelchOpen = true;
  snap.memorySlot = MEMORY_NO_SLOT;
  in.snap = &snap;
  in.readChannel = fakeRead;
  reads = 0;
  lastReadSlot = MEMORY_NO_SLOT;
  storedName = NULL;
  storedNameUnterminated = false;
  screenStateReset(&build);
}

void tearDown(void) {}

static void buildNow(void) {
  screenStateBuild(&build, &in, &state);
}

/* One new reading from the tuner, at this time. */
static void readAgain(int16_t modulation, uint32_t nowMs) {
  snap.qualityReads++;
  snap.quality.modulationPercent = modulation;
  in.nowMs = nowMs;
  buildNow();
}

/* ------------------------------------------------------------- the reset */

static void a_reset_build_holds_nothing(void) {
  memset(&build, 0x5A, sizeof(build));
  screenStateReset(&build);
  TEST_ASSERT_FALSE(build.shownStationKnown);
  TEST_ASSERT_EQUAL_INT(BAND_FM, build.shownBand);
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, build.namedSlot);
  TEST_ASSERT_FALSE(build.modulationBar.valid);
  TEST_ASSERT_FALSE(build.modulationPeak.valid);
  TEST_ASSERT_EQUAL_STRING("", build.channelName);
}

/* -------------------------------------------------- frequency and scale */

static void an_fm_station_reads_in_megahertz_with_its_scale(void) {
  buildNow();
  TEST_ASSERT_TRUE(state.fm);
  TEST_ASSERT_EQUAL_STRING("FM", state.band);
  TEST_ASSERT_EQUAL_STRING("106.40", state.frequency);
  TEST_ASSERT_EQUAL_STRING("MHz", state.unit);
  TEST_ASSERT_NULL(state.meterBand);
  TEST_ASSERT_EQUAL_UINT32(87500, state.sweepLowKHz);
  TEST_ASSERT_EQUAL_UINT32(20500, state.sweepSpanKHz);
  TEST_ASSERT_EQUAL_UINT32(106400, state.sweepKHz);
}

static void a_medium_wave_station_reads_in_kilohertz(void) {
  snap.settings.band = BAND_MW;
  snap.settings.freqKHz = 1377;
  buildNow();
  TEST_ASSERT_FALSE(state.fm);
  TEST_ASSERT_EQUAL_STRING("MW", state.band);
  TEST_ASSERT_EQUAL_STRING("1377", state.frequency);
  TEST_ASSERT_EQUAL_STRING("kHz", state.unit);
  TEST_ASSERT_EQUAL_UINT32(522, state.sweepLowKHz);
  TEST_ASSERT_EQUAL_UINT32(1791 - 522, state.sweepSpanKHz);
  TEST_ASSERT_EQUAL_UINT32(1377, state.sweepKHz);
}

static void shortwave_names_the_metre_band_it_is_inside(void) {
  snap.settings.band = BAND_SW;
  snap.settings.freqKHz = 9420;
  buildNow();
  TEST_ASSERT_EQUAL_STRING("31 m", state.meterBand);
  /* Between two metre bands there is no name to give. */
  snap.settings.freqKHz = 8000;
  buildNow();
  TEST_ASSERT_NULL(state.meterBand);
}

static void a_frequency_off_the_band_sits_at_the_nearest_end_of_the_scale(
    void) {
  snap.settings.freqKHz = 80000;
  buildNow();
  TEST_ASSERT_EQUAL_UINT32(87500, state.sweepKHz);
  TEST_ASSERT_EQUAL_STRING("80.00", state.frequency);
  snap.settings.freqKHz = 110000;
  buildNow();
  TEST_ASSERT_EQUAL_UINT32(108000, state.sweepKHz);
}

static void no_band_plan_leaves_the_scale_out(void) {
  in.planValid = false;
  buildNow();
  TEST_ASSERT_EQUAL_UINT32(0, state.sweepSpanKHz);
  TEST_ASSERT_EQUAL_UINT32(0, state.sweepLowKHz);
  TEST_ASSERT_EQUAL_UINT32(0, state.sweepKHz);
  /* The frequency does not need the plan. */
  TEST_ASSERT_EQUAL_STRING("106.40", state.frequency);
}

static void a_band_that_does_not_exist_gives_empty_text_and_no_scale(void) {
  snap.settings.band = BAND_COUNT;
  snap.rds.hasPs = true;
  snprintf(snap.rds.ps, sizeof(snap.rds.ps), "%s", "RADIO 1 ");
  buildNow();
  TEST_ASSERT_EQUAL_STRING("", state.frequency);
  TEST_ASSERT_EQUAL_STRING("", state.band);
  TEST_ASSERT_NULL(state.meterBand);
  TEST_ASSERT_FALSE(state.fm);
  TEST_ASSERT_EQUAL_UINT32(0, state.sweepSpanKHz);
  TEST_ASSERT_NULL(state.stationName);
}

/* ------------------------------------------------------------ the level */

static void the_level_is_held_still_for_small_changes(void) {
  buildNow();
  TEST_ASSERT_EQUAL_INT16(37, state.signalDbuV);
  /* 37.5 would round to 38, but it is not a whole dB away from 37. */
  snap.levelSmoothedTenths = 375;
  buildNow();
  TEST_ASSERT_EQUAL_INT16(37, state.signalDbuV);
  snap.levelSmoothedTenths = 361;
  buildNow();
  TEST_ASSERT_EQUAL_INT16(37, state.signalDbuV);
  snap.levelSmoothedTenths = 380;
  buildNow();
  TEST_ASSERT_EQUAL_INT16(38, state.signalDbuV);
  TEST_ASSERT_TRUE(state.signalValid);
}

static void a_change_of_frequency_starts_the_hold_again(void) {
  buildNow();
  TEST_ASSERT_EQUAL_INT16(37, state.signalDbuV);
  /* The dial has moved and the level is still the old station's. */
  snap.settings.freqKHz = 101900;
  snap.levelSmoothedValid = false;
  snap.levelSmoothedTenths = 280;
  buildNow();
  TEST_ASSERT_EQUAL_INT16(37, state.signalDbuV);
  /* The first reading from the new station is taken as it stands. Held, it
   * would have stayed on 37. */
  snap.levelSmoothedValid = true;
  snap.levelSmoothedTenths = 375;
  buildNow();
  TEST_ASSERT_EQUAL_INT16(38, state.signalDbuV);
  TEST_ASSERT_EQUAL_UINT32(101900, build.shownFreqKHz);
}

static void a_change_of_band_starts_the_hold_again(void) {
  buildNow();
  TEST_ASSERT_EQUAL_INT16(37, state.signalDbuV);
  /* The same number of kHz on another band is another station. */
  snap.settings.band = BAND_OIRT;
  snap.levelSmoothedTenths = 375;
  buildNow();
  TEST_ASSERT_EQUAL_INT16(38, state.signalDbuV);
  TEST_ASSERT_EQUAL_INT(BAND_OIRT, build.shownBand);
}

static void the_level_offset_moves_the_number_and_the_scale_peak(void) {
  in.levelOffsetDb = -5;
  buildNow();
  TEST_ASSERT_EQUAL_INT16(32, state.signalDbuV);
  TEST_ASSERT_EQUAL_UINT8(signalBarPercent(320, SIGNAL_FULL_FM_DBUV),
                          state.signalPercent);
  in.levelOffsetDb = 10;
  buildNow();
  TEST_ASSERT_EQUAL_INT16(47, state.signalDbuV);
  TEST_ASSERT_EQUAL_UINT8(78, state.signalPercent);
}

/* The strongest medium wave channel in a night sweep off the radio,
 * against the AM top of the scale peak. */
static void am_bands_use_their_own_top_of_the_scale_peak(void) {
  snap.settings.band = BAND_MW;
  snap.settings.freqKHz = 1377;
  snap.levelSmoothedTenths = 354;
  buildNow();
  TEST_ASSERT_EQUAL_INT16(35, state.signalDbuV);
  TEST_ASSERT_EQUAL_UINT8(signalBarPercent(350, SIGNAL_FULL_AM_DBUV),
                          state.signalPercent);
}

static void a_failed_reading_leaves_the_peak_and_the_meter_empty(void) {
  snap.qualityValid = false;
  buildNow();
  TEST_ASSERT_FALSE(state.signalValid);
  TEST_ASSERT_EQUAL_UINT8(0, state.signalPercent);
  TEST_ASSERT_FALSE(state.modulationValid);
  TEST_ASSERT_FALSE(state.modulationPeakValid);
  TEST_ASSERT_EQUAL_UINT8(0, state.modulationPercent);
}

/* ------------------------------------------------------------- typing */

static void typed_digits_show_with_a_hyphen_while_more_can_follow(void) {
  in.typed = "104";
  buildNow();
  TEST_ASSERT_EQUAL_STRING("104-", state.typing);
  /* The frequency is still built, for when the typing ends. */
  TEST_ASSERT_EQUAL_STRING("106.40", state.frequency);
}

static void a_full_buffer_of_digits_shows_no_hyphen(void) {
  in.typed = "1064000";
  TEST_ASSERT_EQUAL_size_t(INPUT_DIGITS_MAX, strlen(in.typed));
  buildNow();
  TEST_ASSERT_EQUAL_STRING("1064000", state.typing);
}

static void no_digits_or_null_digits_mean_nobody_is_typing(void) {
  in.typed = "";
  buildNow();
  TEST_ASSERT_NULL(state.typing);
  in.typed = NULL;
  buildNow();
  TEST_ASSERT_NULL(state.typing);
}

static void digits_longer_than_the_buffer_are_cut_and_not_overrun(void) {
  in.typed = "123456789012";
  buildNow();
  TEST_ASSERT_EQUAL_size_t(sizeof(build.typing) - 1, strlen(state.typing));
  TEST_ASSERT_EQUAL_STRING("12345678", state.typing);
}

/* ------------------------------------------------- the stored channel */

static void the_channel_name_is_read_only_when_the_slot_or_the_store_changes(
    void) {
  storedName = "MIRCHI";
  snap.memorySlot = 4;
  in.memoryGeneration = 1;
  buildNow();
  TEST_ASSERT_EQUAL_INT(1, reads);
  TEST_ASSERT_EQUAL_INT(4, lastReadSlot);
  TEST_ASSERT_EQUAL_STRING("MIRCHI", state.memoryName);
  TEST_ASSERT_EQUAL_STRING("P05", state.memory);

  /* The screen builds many times a second. The store is not asked again. */
  for (int i = 0; i < 5; i++) {
    buildNow();
  }
  TEST_ASSERT_EQUAL_INT(1, reads);
  TEST_ASSERT_EQUAL_STRING("MIRCHI", state.memoryName);

  /* A change the generation does not show is not seen. */
  storedName = "RED FM";
  buildNow();
  TEST_ASSERT_EQUAL_INT(1, reads);
  TEST_ASSERT_EQUAL_STRING("MIRCHI", state.memoryName);

  /* An edit moves the generation, and the name is read again. */
  in.memoryGeneration = 2;
  buildNow();
  TEST_ASSERT_EQUAL_INT(2, reads);
  TEST_ASSERT_EQUAL_STRING("RED FM", state.memoryName);

  /* Another slot is read at once. */
  snap.memorySlot = 5;
  buildNow();
  TEST_ASSERT_EQUAL_INT(3, reads);
  TEST_ASSERT_EQUAL_INT(5, lastReadSlot);
  TEST_ASSERT_EQUAL_STRING("P06", state.memory);
}

static void leaving_the_stored_channels_forgets_the_name(void) {
  storedName = "MIRCHI";
  snap.memorySlot = 4;
  buildNow();
  TEST_ASSERT_EQUAL_INT(1, reads);

  snap.memorySlot = MEMORY_NO_SLOT;
  buildNow();
  TEST_ASSERT_EQUAL_INT(1, reads);
  TEST_ASSERT_NULL(state.memoryName);
  TEST_ASSERT_NULL(state.memory);
  TEST_ASSERT_EQUAL_STRING("", build.channelName);

  /* Coming back to the same slot reads it again, since nothing was kept. */
  snap.memorySlot = 4;
  buildNow();
  TEST_ASSERT_EQUAL_INT(2, reads);
  TEST_ASSERT_EQUAL_STRING("MIRCHI", state.memoryName);
}

static void an_empty_or_unreadable_channel_has_no_name(void) {
  snap.memorySlot = 4;
  storedName = NULL;
  buildNow();
  TEST_ASSERT_EQUAL_INT(1, reads);
  TEST_ASSERT_NULL(state.memoryName);
  /* The slot mark does not depend on the store. */
  TEST_ASSERT_EQUAL_STRING("P05", state.memory);

  storedName = "";
  in.memoryGeneration++;
  buildNow();
  TEST_ASSERT_EQUAL_INT(2, reads);
  TEST_ASSERT_NULL(state.memoryName);
}

static void a_failed_read_does_not_keep_the_name_before_it(void) {
  storedName = "MIRCHI";
  snap.memorySlot = 4;
  buildNow();
  TEST_ASSERT_EQUAL_STRING("MIRCHI", state.memoryName);
  storedName = NULL;
  in.memoryGeneration++;
  buildNow();
  TEST_ASSERT_NULL(state.memoryName);
}

static void a_name_with_no_terminator_is_cut_to_its_field(void) {
  storedNameUnterminated = true;
  snap.memorySlot = 0;
  buildNow();
  TEST_ASSERT_NOT_NULL(state.memoryName);
  TEST_ASSERT_EQUAL_size_t(MEMORY_NAME_LEN - 1, strlen(state.memoryName));
  TEST_ASSERT_EQUAL_STRING("P01", state.memory);
}

/* --------------------------------------------------------------- RDS */

static void the_station_name_and_text_come_from_rds_on_fm(void) {
  buildNow();
  TEST_ASSERT_NULL(state.stationName);
  TEST_ASSERT_NULL(state.radioText);

  snap.rds.hasPs = true;
  snprintf(snap.rds.ps, sizeof(snap.rds.ps), "%s", "MIRCHI 9");
  snap.rds.hasRt = true;
  snprintf(snap.rds.rt, sizeof(snap.rds.rt), "%s", "NOW PLAYING");
  buildNow();
  TEST_ASSERT_EQUAL_STRING("MIRCHI 9", state.stationName);
  TEST_ASSERT_EQUAL_STRING("NOW PLAYING", state.radioText);

  /* A name split over two passes is shown stitched. */
  snap.rds.hasPsLong = true;
  snprintf(snap.rds.psLong, sizeof(snap.rds.psLong), "%s", "MIRCHI 95");
  buildNow();
  TEST_ASSERT_EQUAL_STRING("MIRCHI 95", state.stationName);
}

static void rds_is_not_read_on_am(void) {
  snap.settings.band = BAND_MW;
  snap.settings.freqKHz = 1377;
  snap.rds.hasPs = true;
  snprintf(snap.rds.ps, sizeof(snap.rds.ps), "%s", "RADIO 1 ");
  snap.rds.hasRt = true;
  snprintf(snap.rds.rt, sizeof(snap.rds.rt), "%s", "NOW PLAYING");
  buildNow();
  TEST_ASSERT_NULL(state.stationName);
  TEST_ASSERT_NULL(state.radioText);
}

/* ------------------------------------------------- the passed through */

static void the_clock_date_fault_and_log_confirm_are_null_until_set(void) {
  buildNow();
  TEST_ASSERT_NULL(state.clock);
  TEST_ASSERT_NULL(state.date);
  TEST_ASSERT_NULL(state.fault);
  TEST_ASSERT_NULL(state.logConfirm);
  TEST_ASSERT_NULL(state.notice);

  in.clock = "05:09";
  in.date = "Wednesday, 30th September 2026";
  in.fault = "TUNE REFUSED";
  in.logConfirm = "LOGGED";
  in.notice = "CHECKING";
  buildNow();
  TEST_ASSERT_EQUAL_PTR(in.clock, state.clock);
  TEST_ASSERT_EQUAL_PTR(in.date, state.date);
  TEST_ASSERT_EQUAL_PTR(in.fault, state.fault);
  TEST_ASSERT_EQUAL_PTR(in.logConfirm, state.logConfirm);
  TEST_ASSERT_EQUAL_PTR(in.notice, state.notice);

  /* And back to NULL when the caller has nothing again. */
  in.clock = NULL;
  in.fault = NULL;
  buildNow();
  TEST_ASSERT_NULL(state.clock);
  TEST_ASSERT_NULL(state.fault);
}

/* ------------------------------------------------------------ the tiles */

static void the_audio_tile_says_why_there_is_no_sound(void) {
  buildNow();
  TEST_ASSERT_EQUAL_INT(SCREEN_AUDIO_ON, state.audio);
  snap.squelchOpen = false;
  buildNow();
  TEST_ASSERT_EQUAL_INT(SCREEN_AUDIO_SQUELCHED, state.audio);
  /* A person muting it wins over the squelch. */
  snap.settings.muted = true;
  buildNow();
  TEST_ASSERT_EQUAL_INT(SCREEN_AUDIO_MUTED, state.audio);
  snap.squelchOpen = true;
  buildNow();
  TEST_ASSERT_EQUAL_INT(SCREEN_AUDIO_MUTED, state.audio);
}

static void the_tiles_read_the_settings(void) {
  snap.settings.volumeDb = -6;
  snap.settings.tuneMode = TUNE_MODE_AUTO;
  snap.squelchMode = SQUELCH_OFF;
  snap.settings.bandwidthKHz = 0;
  buildNow();
  TEST_ASSERT_EQUAL_STRING("-6dB", state.volume);
  TEST_ASSERT_EQUAL_STRING("AUTO", state.tuneMode);
  TEST_ASSERT_EQUAL_STRING("OFF", state.squelchMode);
  TEST_ASSERT_EQUAL_STRING("DYN", state.filter);
  TEST_ASSERT_TRUE(state.tunerReady);

  snap.settings.tuneMode = TUNE_MODE_METER_BAND;
  snap.squelchMode = SQUELCH_AUTO;
  snap.settings.bandwidthKHz = 114;
  snap.tunerReady = false;
  buildNow();
  TEST_ASSERT_EQUAL_STRING("MTR", state.tuneMode);
  TEST_ASSERT_EQUAL_STRING("AUTO", state.squelchMode);
  TEST_ASSERT_EQUAL_STRING("114k", state.filter);
  TEST_ASSERT_FALSE(state.tunerReady);

  /* Manual shows the level the knob sets, to the nearest whole dB. */
  snap.squelchMode = SQUELCH_MANUAL;
  snap.squelchThresholdTenths = 345;
  buildNow();
  TEST_ASSERT_EQUAL_STRING("35dB", state.squelchMode);
  snap.squelchThresholdTenths = -100;
  buildNow();
  TEST_ASSERT_EQUAL_STRING("-10dB", state.squelchMode);
  snap.squelchThresholdTenths = -4;
  buildNow();
  TEST_ASSERT_EQUAL_STRING("0dB", state.squelchMode);
  snap.squelchThresholdTenths = 920;
  buildNow();
  TEST_ASSERT_EQUAL_STRING("92dB", state.squelchMode);
}

static void a_volume_and_a_width_at_their_limits_fit_their_text(void) {
  snap.settings.volumeDb = INT8_MIN;
  snap.settings.bandwidthKHz = UINT16_MAX;
  buildNow();
  TEST_ASSERT_EQUAL_STRING("-128dB", state.volume);
  TEST_ASSERT_EQUAL_STRING("65535k", state.filter);
}

/* ------------------------------------------------- battery and Wi-Fi */

/* A real battery reading from this radio. */
static void the_battery_shows_only_with_a_reading_and_the_setting_on(void) {
  static Battery battery;
  batteryReset(&battery);
  in.batteryShow = BATTERY_SHOW_PERCENT;

  in.battery = NULL;
  buildNow();
  TEST_ASSERT_FALSE(state.batteryValid);

  /* No reading yet. */
  in.battery = &battery;
  buildNow();
  TEST_ASSERT_FALSE(state.batteryValid);

  batteryFeed(&battery, 3754, true);
  buildNow();
  TEST_ASSERT_TRUE(state.batteryValid);
  TEST_ASSERT_EQUAL_UINT8(batteryPercent(&battery), state.batteryPercent);
  /* As a per cent the shape says it, so there is no text. */
  TEST_ASSERT_NULL(state.batteryText);

  in.batteryShow = BATTERY_SHOW_VOLTS;
  buildNow();
  TEST_ASSERT_TRUE(state.batteryValid);
  TEST_ASSERT_EQUAL_STRING("3.8", state.batteryText);

  in.batteryShow = BATTERY_SHOW_OFF;
  buildNow();
  TEST_ASSERT_FALSE(state.batteryValid);
  TEST_ASSERT_NULL(state.batteryText);
}

static void a_battery_setting_past_the_list_shows_the_shape_alone(void) {
  static Battery battery;
  batteryReset(&battery);
  batteryFeed(&battery, 3754, true);
  in.battery = &battery;
  in.batteryShow = BATTERY_SHOW_VOLTS;
  buildNow();
  TEST_ASSERT_EQUAL_STRING("3.8", state.batteryText);
  /* The old text is not left behind. */
  in.batteryShow = BATTERY_SHOW_COUNT;
  buildNow();
  TEST_ASSERT_TRUE(state.batteryValid);
  TEST_ASSERT_NULL(state.batteryText);
}

/* -52 dBm is a real reading from this radio. */
static void wifi_bars_are_given_only_when_the_strength_is_known(void) {
  in.wifi = SCREEN_WIFI_JOINED;
  in.rssiValid = true;
  in.rssiDbm = -52;
  buildNow();
  TEST_ASSERT_EQUAL_INT(SCREEN_WIFI_JOINED, state.wifi);
  TEST_ASSERT_EQUAL_UINT8(2, state.wifiBars);

  in.rssiValid = false;
  buildNow();
  TEST_ASSERT_EQUAL_UINT8(0, state.wifiBars);

  in.wifi = SCREEN_WIFI_AP;
  buildNow();
  TEST_ASSERT_EQUAL_INT(SCREEN_WIFI_AP, state.wifi);
  in.wifi = SCREEN_WIFI_TRYING;
  buildNow();
  TEST_ASSERT_EQUAL_INT(SCREEN_WIFI_TRYING, state.wifi);
  in.wifi = SCREEN_WIFI_NONE;
  buildNow();
  TEST_ASSERT_EQUAL_INT(SCREEN_WIFI_NONE, state.wifi);
}

/* ------------------------------------------------ the modulation meter */

static void the_first_build_on_a_station_waits_for_its_own_reading(void) {
  /* The reading the snapshot carries may be the station before. */
  buildNow();
  TEST_ASSERT_FALSE(state.modulationValid);
  TEST_ASSERT_FALSE(state.modulationPeakValid);
  readAgain(57, 100);
  TEST_ASSERT_TRUE(state.modulationValid);
  TEST_ASSERT_EQUAL_UINT8(57, state.modulationPercent);
}

static void the_modulation_bar_rises_at_once_and_falls_over_time(void) {
  buildNow();
  readAgain(80, 100);
  TEST_ASSERT_EQUAL_UINT8(80, state.modulationPercent);
  TEST_ASSERT_TRUE(state.modulationPeakValid);
  TEST_ASSERT_EQUAL_UINT8(80, state.modulationPeakPercent);

  /* 400 ms on: the bar has fallen 23, the peak is still held. */
  readAgain(20, 500);
  TEST_ASSERT_EQUAL_UINT8(57, state.modulationPercent);
  TEST_ASSERT_EQUAL_UINT8(80, state.modulationPeakPercent);

  /* 850 ms on from the peak: the bar has fallen half the meter, and the peak
   * has started to fall 100 ms after its hold ended. */
  readAgain(20, 950);
  TEST_ASSERT_EQUAL_UINT8(30, state.modulationPercent);
  TEST_ASSERT_EQUAL_UINT8(78, state.modulationPeakPercent);

  /* It stops at the reading. */
  readAgain(20, 5000);
  TEST_ASSERT_EQUAL_UINT8(20, state.modulationPercent);
  TEST_ASSERT_EQUAL_UINT8(20, state.modulationPeakPercent);
}

static void the_same_reading_is_not_fed_twice(void) {
  buildNow();
  readAgain(80, 100);
  /* The screen builds more often than the tuner is read. A build with no new
   * reading neither moves the bar nor starts the peak's hold again. */
  snap.quality.modulationPercent = 0;
  in.nowMs = 2000;
  buildNow();
  TEST_ASSERT_EQUAL_UINT8(80, state.modulationPercent);
  TEST_ASSERT_EQUAL_UINT8(80, state.modulationPeakPercent);
  TEST_ASSERT_EQUAL_UINT32(100, build.modulationPeak.heldMs);
}

/* 141 is the first recorded meter reading on 104.00. */
static void a_reading_past_full_is_drawn_full(void) {
  buildNow();
  readAgain(k_mid_104000_2026_09_15[0].mod, 100);
  TEST_ASSERT_EQUAL_INT16(141, k_mid_104000_2026_09_15[0].mod);
  TEST_ASSERT_EQUAL_UINT8(100, state.modulationPercent);
  TEST_ASSERT_EQUAL_UINT8(100, state.modulationPeakPercent);
}

static void a_reading_below_zero_is_drawn_empty(void) {
  buildNow();
  readAgain(-3, 100);
  TEST_ASSERT_TRUE(state.modulationValid);
  TEST_ASSERT_EQUAL_UINT8(0, state.modulationPercent);
}

static void a_failed_read_forgets_the_modulation(void) {
  buildNow();
  readAgain(80, 100);
  TEST_ASSERT_TRUE(state.modulationValid);

  snap.qualityValid = false;
  buildNow();
  TEST_ASSERT_FALSE(state.modulationValid);
  TEST_ASSERT_FALSE(build.modulationBar.valid);

  /* Good again but no new read yet: still nothing to show. */
  snap.qualityValid = true;
  buildNow();
  TEST_ASSERT_FALSE(state.modulationValid);

  /* The next read starts it from the new reading, not from 80. */
  readAgain(30, 300);
  TEST_ASSERT_TRUE(state.modulationValid);
  TEST_ASSERT_EQUAL_UINT8(30, state.modulationPercent);
  TEST_ASSERT_EQUAL_UINT8(30, state.modulationPeakPercent);
}

static void a_change_of_station_forgets_the_modulation(void) {
  buildNow();
  readAgain(80, 100);
  TEST_ASSERT_EQUAL_UINT8(80, state.modulationPercent);

  /* Retuned. This snapshot's reading is new but from the station before. */
  snap.settings.freqKHz = 101900;
  readAgain(80, 200);
  TEST_ASSERT_FALSE(state.modulationValid);
  TEST_ASSERT_FALSE(state.modulationPeakValid);

  readAgain(10, 300);
  TEST_ASSERT_EQUAL_UINT8(10, state.modulationPercent);
  TEST_ASSERT_EQUAL_UINT8(10, state.modulationPeakPercent);
}

/*
 * Recorded readings of music on 106.40, replayed at their own times, with the
 * level smoothed the way the radio task does. The screen builds twice for every
 * read, as it does on the radio. Through all of it the bar is never below the
 * reading, the peak is never below the bar, and the number shown is never a
 * whole dB from the level under it.
 */
static void the_music_capture_replays_the_way_the_radio_shows_it(void) {
  const MeterCapture *rows = k_music_106400_2026_09_15;
  const uint16_t count =
      sizeof(k_music_106400_2026_09_15) / sizeof(k_music_106400_2026_09_15[0]);
  SignalAverage avg;
  memset(&avg, 0, sizeof(avg));
  snap.settings.freqKHz = rows[0].khz;
  snap.levelSmoothedTenths = signalAverage(&avg, rows[0].sig);
  buildNow();

  int shownChanges = 0;
  int roundedChanges = 0;
  int falling = 0;
  int16_t lastShown = state.signalDbuV;
  int16_t lastRounded = state.signalDbuV;
  for (uint16_t i = 1; i < count; i++) {
    snap.levelSmoothedTenths = signalAverage(&avg, rows[i].sig);
    readAgain(rows[i].mod, rows[i].ms);
    const uint8_t reading = rows[i].mod > 100 ? 100 : (uint8_t)rows[i].mod;
    TEST_ASSERT_TRUE(state.modulationValid);
    TEST_ASSERT_TRUE(state.modulationPercent >= reading);
    TEST_ASSERT_TRUE(state.modulationPeakPercent >= state.modulationPercent);
    TEST_ASSERT_INT_WITHIN(9, snap.levelSmoothedTenths,
                           (int)state.signalDbuV * 10);
    if (state.modulationPercent > reading) {
      falling++;
    }
    if (state.signalDbuV != lastShown) {
      shownChanges++;
      lastShown = state.signalDbuV;
    }
    const int16_t rounded = (int16_t)((snap.levelSmoothedTenths + 5) / 10);
    if (rounded != lastRounded) {
      roundedChanges++;
      lastRounded = rounded;
    }

    /* A build between two reads changes nothing. */
    const ScreenState before = state;
    in.nowMs = rows[i].ms + 40;
    buildNow();
    TEST_ASSERT_EQUAL_UINT8(before.modulationPercent, state.modulationPercent);
    TEST_ASSERT_EQUAL_UINT8(before.modulationPeakPercent,
                            state.modulationPeakPercent);
    TEST_ASSERT_EQUAL_INT16(before.signalDbuV, state.signalDbuV);
  }
  /* The bar did fall at times, rather than follow every reading. */
  TEST_ASSERT_TRUE(falling > 0);
  /* And the hold kept the number stiller than rounding alone would. */
  TEST_ASSERT_TRUE(shownChanges < roundedChanges);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(a_reset_build_holds_nothing);

  RUN_TEST(an_fm_station_reads_in_megahertz_with_its_scale);
  RUN_TEST(a_medium_wave_station_reads_in_kilohertz);
  RUN_TEST(shortwave_names_the_metre_band_it_is_inside);
  RUN_TEST(a_frequency_off_the_band_sits_at_the_nearest_end_of_the_scale);
  RUN_TEST(no_band_plan_leaves_the_scale_out);
  RUN_TEST(a_band_that_does_not_exist_gives_empty_text_and_no_scale);

  RUN_TEST(the_level_is_held_still_for_small_changes);
  RUN_TEST(a_change_of_frequency_starts_the_hold_again);
  RUN_TEST(a_change_of_band_starts_the_hold_again);
  RUN_TEST(the_level_offset_moves_the_number_and_the_scale_peak);
  RUN_TEST(am_bands_use_their_own_top_of_the_scale_peak);
  RUN_TEST(a_failed_reading_leaves_the_peak_and_the_meter_empty);

  RUN_TEST(typed_digits_show_with_a_hyphen_while_more_can_follow);
  RUN_TEST(a_full_buffer_of_digits_shows_no_hyphen);
  RUN_TEST(no_digits_or_null_digits_mean_nobody_is_typing);
  RUN_TEST(digits_longer_than_the_buffer_are_cut_and_not_overrun);

  RUN_TEST(the_channel_name_is_read_only_when_the_slot_or_the_store_changes);
  RUN_TEST(leaving_the_stored_channels_forgets_the_name);
  RUN_TEST(an_empty_or_unreadable_channel_has_no_name);
  RUN_TEST(a_failed_read_does_not_keep_the_name_before_it);
  RUN_TEST(a_name_with_no_terminator_is_cut_to_its_field);

  RUN_TEST(the_station_name_and_text_come_from_rds_on_fm);
  RUN_TEST(rds_is_not_read_on_am);

  RUN_TEST(the_clock_date_fault_and_log_confirm_are_null_until_set);

  RUN_TEST(the_audio_tile_says_why_there_is_no_sound);
  RUN_TEST(the_tiles_read_the_settings);
  RUN_TEST(a_volume_and_a_width_at_their_limits_fit_their_text);

  RUN_TEST(the_battery_shows_only_with_a_reading_and_the_setting_on);
  RUN_TEST(a_battery_setting_past_the_list_shows_the_shape_alone);
  RUN_TEST(wifi_bars_are_given_only_when_the_strength_is_known);

  RUN_TEST(the_first_build_on_a_station_waits_for_its_own_reading);
  RUN_TEST(the_modulation_bar_rises_at_once_and_falls_over_time);
  RUN_TEST(the_same_reading_is_not_fed_twice);
  RUN_TEST(a_reading_past_full_is_drawn_full);
  RUN_TEST(a_reading_below_zero_is_drawn_empty);
  RUN_TEST(a_failed_read_forgets_the_modulation);
  RUN_TEST(a_change_of_station_forgets_the_modulation);
  RUN_TEST(the_music_capture_replays_the_way_the_radio_shows_it);

  return UNITY_END();
}
