/*
 * Tests for the DX level sweep. Runs on a PC.
 *
 * capture.h is two passes of the band read on the radio at the 114 kHz DX
 * width, with the results worked out ahead of time.
 */
#include <string.h>
#include <unity.h>

#include "capture.h"
#include "core/dx_sweep.h"

/* Big enough that the loop task's stack would feel them, so static here as
 * they are on the heap in the firmware. */
static DxSweep sweep;
static DxSweep other;
static DxSweep out;
static DxSweepHistory history;
static DxSweepHistory back;
static uint8_t file[DX_SWEEP_FILE_MAX];

void setUp(void) {
  memset(&sweep, 0, sizeof(sweep));
  memset(&other, 0, sizeof(other));
  memset(&out, 0, sizeof(out));
  memset(&history, 0, sizeof(history));
  memset(&back, 0, sizeof(back));
}
void tearDown(void) {}

/* A sweep of the recorded channels at 114 kHz, every level `level`. */
static void flat(DxSweep *s, int16_t level) {
  memset(s, 0, sizeof(*s));
  s->lowKHz = CAPTURE_LOW_KHZ;
  s->stepKHz = CAPTURE_STEP_KHZ;
  s->count = CAPTURE_COUNT;
  s->widthKHz = 114;
  for (uint16_t i = 0; i < s->count; i++) {
    s->level[i] = level;
  }
}

/* Recorded pass `p`, as the sweep would take it. */
static void fromCapture(DxSweep *s, const int16_t (*pass)[CAPTURE_READS]) {
  flat(s, 0);
  for (uint16_t i = 0; i < CAPTURE_COUNT; i++) {
    s->level[i] = dxSweepMean(pass[i], CAPTURE_READS);
  }
}

static void the_mean_rounds_to_the_nearest_tenth(void) {
  const int16_t up[2] = {10, 11};
  const int16_t down[2] = {-10, -11};
  const int16_t mixed[4] = {-101, -85, 30, 12};
  TEST_ASSERT_EQUAL_INT16(11, dxSweepMean(up, 2));
  TEST_ASSERT_EQUAL_INT16(-11, dxSweepMean(down, 2));
  TEST_ASSERT_EQUAL_INT16(-36, dxSweepMean(mixed, 4));
  TEST_ASSERT_EQUAL_INT16(DX_SWEEP_NO_READING, dxSweepMean(up, 0));
  TEST_ASSERT_EQUAL_INT16(DX_SWEEP_NO_READING, dxSweepMean(NULL, 2));
}

static void the_floor_of_a_real_band_is_its_lower_quartile(void) {
  fromCapture(&sweep, kPass0);
  TEST_ASSERT_EQUAL_INT16(CAPTURE_FLOOR0, dxSweepFloor(&sweep));
  fromCapture(&sweep, kPass1);
  TEST_ASSERT_EQUAL_INT16(CAPTURE_FLOOR1, dxSweepFloor(&sweep));
}

static void the_floor_leaves_out_channels_with_no_reading(void) {
  flat(&sweep, DX_SWEEP_NO_READING);
  TEST_ASSERT_EQUAL_INT16(DX_SWEEP_NO_READING, dxSweepFloor(&sweep));
  sweep.level[5] = -20;
  sweep.level[6] = 300;
  TEST_ASSERT_EQUAL_INT16(-20, dxSweepFloor(&sweep));
  sweep.count = 0;
  TEST_ASSERT_EQUAL_INT16(DX_SWEEP_NO_READING, dxSweepFloor(&sweep));
  TEST_ASSERT_EQUAL_INT16(DX_SWEEP_NO_READING, dxSweepFloor(NULL));
}

static void the_median_of_two_real_passes_matches_the_recorded_median(void) {
  fromCapture(&sweep, kPass0);
  fromCapture(&other, kPass1);
  dxSweepKeep(&history, &sweep);
  dxSweepKeep(&history, &other);
  TEST_ASSERT_EQUAL_UINT8(
      2, dxSweepMedian(history.item, history.count, &sweep, &out));
  TEST_ASSERT_EQUAL_INT16_ARRAY(kMedian, out.level, CAPTURE_COUNT);
  TEST_ASSERT_EQUAL_UINT16(CAPTURE_COUNT, out.count);
}

static void the_median_of_an_odd_count_is_the_middle_one(void) {
  const int16_t levels[3] = {50, -30, 200};
  for (uint8_t i = 0; i < 3; i++) {
    flat(&sweep, levels[i]);
    dxSweepKeep(&history, &sweep);
  }
  TEST_ASSERT_EQUAL_UINT8(
      3, dxSweepMedian(history.item, history.count, &sweep, &out));
  TEST_ASSERT_EQUAL_INT16(50, out.level[0]);
}

/* Only sweeps of the same channels at the same width count: a wider filter
 * reads louder, and another region reads other channels. */
static void the_median_takes_only_the_same_channels(void) {
  flat(&sweep, 100);
  flat(&other, 900);
  other.widthKHz = 217;
  dxSweepKeep(&history, &sweep);
  dxSweepKeep(&history, &other);
  TEST_ASSERT_EQUAL_UINT8(
      1, dxSweepMedian(history.item, history.count, &sweep, &out));
  TEST_ASSERT_EQUAL_INT16(100, out.level[3]);
  /* Nothing matches: no baseline, and `out` untouched. */
  flat(&other, 0);
  other.lowKHz = 87500;
  flat(&out, 7);
  memset(&history, 0, sizeof(history));
  dxSweepKeep(&history, &sweep);
  TEST_ASSERT_EQUAL_UINT8(
      0, dxSweepMedian(history.item, history.count, &other, &out));
  TEST_ASSERT_EQUAL_INT16(7, out.level[0]);
  TEST_ASSERT_EQUAL_UINT8(0, dxSweepMedian(NULL, 1, &other, &out));
}

static void a_channel_no_sweep_read_has_no_baseline(void) {
  flat(&sweep, 100);
  sweep.level[4] = DX_SWEEP_NO_READING;
  dxSweepKeep(&history, &sweep);
  flat(&other, 300);
  other.level[4] = DX_SWEEP_NO_READING;
  other.level[9] = DX_SWEEP_NO_READING;
  dxSweepKeep(&history, &other);
  TEST_ASSERT_EQUAL_UINT8(
      2, dxSweepMedian(history.item, history.count, &sweep, &out));
  TEST_ASSERT_EQUAL_INT16(DX_SWEEP_NO_READING, out.level[4]);
  /* One sweep read it: its reading is the median. */
  TEST_ASSERT_EQUAL_INT16(100, out.level[9]);
  TEST_ASSERT_EQUAL_INT16(200, out.level[0]);
}

static void the_peak_holds_the_highest_and_restarts_on_other_channels(void) {
  DxSweep peak;
  memset(&peak, 0, sizeof(peak));
  flat(&sweep, 100);
  sweep.level[2] = DX_SWEEP_NO_READING;
  dxSweepPeak(&peak, &sweep);
  TEST_ASSERT_EQUAL_INT16(100, peak.level[0]);
  flat(&other, 50);
  other.level[1] = 400;
  other.level[2] = -10;
  other.at = 99;
  dxSweepPeak(&peak, &other);
  TEST_ASSERT_EQUAL_INT16(100, peak.level[0]);
  TEST_ASSERT_EQUAL_INT16(400, peak.level[1]);
  TEST_ASSERT_EQUAL_INT16(-10, peak.level[2]);
  TEST_ASSERT_EQUAL_UINT32(99, peak.at);
  /* A missing reading does not pull a peak down. */
  other.level[1] = DX_SWEEP_NO_READING;
  dxSweepPeak(&peak, &other);
  TEST_ASSERT_EQUAL_INT16(400, peak.level[1]);
  other.stepKHz = 50;
  dxSweepPeak(&peak, &other);
  TEST_ASSERT_EQUAL_INT16(50, peak.level[0]);
  dxSweepPeak(NULL, &other);
}

static void the_history_keeps_the_newest_eight(void) {
  for (int16_t i = 0; i < DX_SWEEP_KEEP + 3; i++) {
    flat(&sweep, i);
    dxSweepKeep(&history, &sweep);
  }
  TEST_ASSERT_EQUAL_UINT8(DX_SWEEP_KEEP, history.count);
  TEST_ASSERT_EQUAL_INT16(DX_SWEEP_KEEP + 2, history.item[0].level[0]);
  TEST_ASSERT_EQUAL_INT16(3, history.item[DX_SWEEP_KEEP - 1].level[0]);
  dxSweepKeep(NULL, &sweep);
}

static void a_history_survives_the_file(void) {
  fromCapture(&sweep, kPass0);
  sweep.timeKnown = true;
  sweep.at = 1790000000u;
  sweep.tookMs = 2345;
  sweep.level[0] = DX_SWEEP_NO_READING;
  dxSweepKeep(&history, &sweep);
  flat(&other, -120);
  other.count = DX_SWEEP_MAX;
  other.lowKHz = 65000;
  dxSweepKeep(&history, &other);
  const size_t n = dxSweepEncode(&history, file, sizeof(file));
  TEST_ASSERT_EQUAL_size_t(dxSweepEncodedSize(&history), n);
  TEST_ASSERT_TRUE(dxSweepDecode(file, n, &back));
  TEST_ASSERT_EQUAL_UINT8(2, back.count);
  TEST_ASSERT_EQUAL_MEMORY(&history.item[0], &back.item[0], sizeof(DxSweep));
  TEST_ASSERT_EQUAL_UINT16(DX_SWEEP_MAX, back.item[0].count);
  TEST_ASSERT_TRUE(back.item[1].timeKnown);
  TEST_ASSERT_EQUAL_UINT32(1790000000u, back.item[1].at);
  TEST_ASSERT_EQUAL_UINT16(2345, back.item[1].tookMs);
  TEST_ASSERT_EQUAL_INT16(DX_SWEEP_NO_READING, back.item[1].level[0]);
  TEST_ASSERT_EQUAL_INT16_ARRAY(sweep.level, back.item[1].level, CAPTURE_COUNT);
  /* The largest history fits the largest file. */
  for (uint8_t i = 0; i < DX_SWEEP_KEEP; i++) {
    dxSweepKeep(&history, &other);
  }
  TEST_ASSERT_EQUAL_size_t(DX_SWEEP_FILE_MAX, dxSweepEncodedSize(&history));
  TEST_ASSERT_EQUAL_size_t(DX_SWEEP_FILE_MAX,
                           dxSweepEncode(&history, file, sizeof(file)));
  TEST_ASSERT_EQUAL_size_t(0, dxSweepEncode(&history, file, 100));
  /* An empty history is a file too. */
  memset(&history, 0, sizeof(history));
  TEST_ASSERT_EQUAL_size_t(6, dxSweepEncode(&history, file, sizeof(file)));
  TEST_ASSERT_TRUE(dxSweepDecode(file, 6, &back));
  TEST_ASSERT_EQUAL_UINT8(0, back.count);
}

/* Anything short, long or out of range reads as no history at all, never
 * as part of one. */
static void a_damaged_file_is_refused_whole(void) {
  flat(&sweep, 10);
  dxSweepKeep(&history, &sweep);
  dxSweepKeep(&history, &sweep);
  const size_t n = dxSweepEncode(&history, file, sizeof(file));
  TEST_ASSERT_FALSE(dxSweepDecode(file, n - 1, &back));
  TEST_ASSERT_EQUAL_UINT8(0, back.count);
  TEST_ASSERT_FALSE(dxSweepDecode(file, n + 1, &back));
  TEST_ASSERT_FALSE(dxSweepDecode(file, 20, &back));
  TEST_ASSERT_FALSE(dxSweepDecode(file, 3, &back));
  TEST_ASSERT_FALSE(dxSweepDecode(NULL, n, &back));
  TEST_ASSERT_FALSE(dxSweepDecode(file, n, NULL));
  uint8_t bad[DX_SWEEP_FILE_MAX];
  const size_t at[] = {0, 4, 5, 6, 15, 16, 23 + 2 * CAPTURE_COUNT};
  const uint8_t to[] = {'X', 2, DX_SWEEP_KEEP + 1, 2, 0, 0, 2};
  for (size_t i = 0; i < sizeof(at) / sizeof(at[0]); i++) {
    memcpy(bad, file, n);
    bad[at[i]] = to[i];
    if (at[i] == 15) {
      bad[16] = 0; /* A step of 0. */
    }
    if (at[i] == 16) {
      bad[18] = 0xFF; /* A count past DX_SWEEP_MAX. */
    }
    TEST_ASSERT_FALSE_MESSAGE(dxSweepDecode(bad, n, &back),
                              "a damaged file was taken");
  }
  TEST_ASSERT_TRUE(dxSweepDecode(file, n, &back));
  TEST_ASSERT_EQUAL_size_t(0, dxSweepEncode(NULL, file, sizeof(file)));
  TEST_ASSERT_EQUAL_size_t(0, dxSweepEncodedSize(NULL));
}

static void the_same_channels_needs_channels(void) {
  flat(&sweep, 0);
  flat(&other, 0);
  TEST_ASSERT_TRUE(dxSweepSameChannels(&sweep, &other));
  sweep.count = 0;
  other.count = 0;
  TEST_ASSERT_FALSE(dxSweepSameChannels(&sweep, &other));
  TEST_ASSERT_FALSE(dxSweepSameChannels(NULL, &other));
}

static void a_channel_is_found_only_on_the_sweep(void) {
  flat(&sweep, 0);
  TEST_ASSERT_EQUAL_INT16(0, dxSweepChannelOf(&sweep, 87000));
  TEST_ASSERT_EQUAL_INT16(114, dxSweepChannelOf(&sweep, 98400));
  TEST_ASSERT_EQUAL_INT16(CAPTURE_COUNT - 1, dxSweepChannelOf(&sweep, 108000));
  TEST_ASSERT_EQUAL_INT16(-1, dxSweepChannelOf(&sweep, 108100));
  TEST_ASSERT_EQUAL_INT16(-1, dxSweepChannelOf(&sweep, 86900));
  TEST_ASSERT_EQUAL_INT16(-1, dxSweepChannelOf(&sweep, 98450));
  TEST_ASSERT_EQUAL_INT16(-1, dxSweepChannelOf(NULL, 98400));
  TEST_ASSERT_EQUAL_UINT32(98400, dxSweepKHzOf(&sweep, 114));
  TEST_ASSERT_EQUAL_UINT32(0, dxSweepKHzOf(&sweep, CAPTURE_COUNT));
  TEST_ASSERT_EQUAL_UINT32(0, dxSweepKHzOf(NULL, 0));
  sweep.stepKHz = 0;
  TEST_ASSERT_EQUAL_INT16(-1, dxSweepChannelOf(&sweep, 87000));
}

static void the_age_is_in_whole_units(void) {
  char t[16];
  TEST_ASSERT_TRUE(dxSweepAge(1000, 1059, t, sizeof(t)));
  TEST_ASSERT_EQUAL_STRING("now", t);
  TEST_ASSERT_TRUE(dxSweepAge(1000, 1060, t, sizeof(t)));
  TEST_ASSERT_EQUAL_STRING("1 min ago", t);
  TEST_ASSERT_TRUE(dxSweepAge(1000, 1000 + 3599, t, sizeof(t)));
  TEST_ASSERT_EQUAL_STRING("59 min ago", t);
  TEST_ASSERT_TRUE(dxSweepAge(1000, 1000 + 3600, t, sizeof(t)));
  TEST_ASSERT_EQUAL_STRING("1 h ago", t);
  TEST_ASSERT_TRUE(dxSweepAge(1000, 1000 + 86399, t, sizeof(t)));
  TEST_ASSERT_EQUAL_STRING("23 h ago", t);
  TEST_ASSERT_TRUE(dxSweepAge(1000, 1000 + 3 * 86400, t, sizeof(t)));
  TEST_ASSERT_EQUAL_STRING("3 d ago", t);
  /* A clock that went back since says nothing about the age. */
  TEST_ASSERT_FALSE(dxSweepAge(1000, 999, t, sizeof(t)));
  TEST_ASSERT_EQUAL_STRING("", t);
  TEST_ASSERT_FALSE(dxSweepAge(1000, 2000, NULL, 4));
  TEST_ASSERT_FALSE(dxSweepAge(1000, 2000, t, 0));
}

/* The whole band in its default step, and a span round the dial on the same
 * channels, kept inside the band's edges. */
static void a_range_is_the_band_or_a_span_inside_it(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  DxSweepRange r;
  TEST_ASSERT_TRUE(dxSweepRange(BAND_FM, &plan, 106400, 0, &r));
  TEST_ASSERT_EQUAL_UINT32(87500, r.lowKHz);
  TEST_ASSERT_EQUAL_UINT16(100, r.stepKHz);
  TEST_ASSERT_EQUAL_UINT16(206, r.count);
  /* 3.6 MHz round 106.4: 37 channels, 104.6 to 108.2 would cross the top,
   * so it ends on 108.0. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_FM, &plan, 106400, 3600, &r));
  TEST_ASSERT_EQUAL_UINT16(37, r.count);
  TEST_ASSERT_EQUAL_UINT32(104400, r.lowKHz);
  TEST_ASSERT_EQUAL_UINT32(108000, r.lowKHz + (r.count - 1u) * r.stepKHz);
  /* In the middle of the band it is centred on the dial. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_FM, &plan, 98300, 3600, &r));
  TEST_ASSERT_EQUAL_UINT32(96500, r.lowKHz);
  TEST_ASSERT_EQUAL_UINT32(100100, r.lowKHz + (r.count - 1u) * r.stepKHz);
  /* At the bottom edge it starts on the band's first channel. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_FM, &plan, 87500, 3600, &r));
  TEST_ASSERT_EQUAL_UINT32(87500, r.lowKHz);
  TEST_ASSERT_EQUAL_UINT16(37, r.count);
  /* A dial between channels goes to the nearest: 98.36 is 98.4. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_FM, &plan, 98360, 3600, &r));
  TEST_ASSERT_EQUAL_UINT32(96600, r.lowKHz);
  /* As wide as the band or wider, it is the band. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_FM, &plan, 98300, 20500, &r));
  TEST_ASSERT_EQUAL_UINT16(206, r.count);
  TEST_ASSERT_EQUAL_UINT32(87500, r.lowKHz);
  /* Medium wave in its 9 kHz channels. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_MW, &plan, 738, 0, &r));
  TEST_ASSERT_EQUAL_UINT32(522, r.lowKHz);
  TEST_ASSERT_EQUAL_UINT16(9, r.stepKHz);
  TEST_ASSERT_EQUAL_UINT16(142, r.count);
  /* A dial outside the band, after a band change, keeps it inside. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_FM, &plan, 80000, 3600, &r));
  TEST_ASSERT_EQUAL_UINT32(87500, r.lowKHz);
  TEST_ASSERT_TRUE(dxSweepRange(BAND_FM, &plan, 110000, 3600, &r));
  TEST_ASSERT_EQUAL_UINT32(108000, r.lowKHz + (r.count - 1u) * r.stepKHz);
  /* OIRT in its 30 kHz channels. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_OIRT, &plan, 70000, 900, &r));
  TEST_ASSERT_EQUAL_UINT16(30, r.stepKHz);
  TEST_ASSERT_EQUAL_UINT16(31, r.count);
  /* The nearest OIRT channel to 70.00 MHz is 70.01, the span's middle. */
  TEST_ASSERT_EQUAL_UINT32(69560, r.lowKHz);
  TEST_ASSERT_EQUAL_UINT32(70010, r.lowKHz + 15u * r.stepKHz);
  /* A span narrower than a step is the dial's own channel. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_FM, &plan, 98300, 50, &r));
  TEST_ASSERT_EQUAL_UINT16(1, r.count);
  TEST_ASSERT_EQUAL_UINT32(98300, r.lowKHz);
  /* Shortwave is 5061 channels: no whole band, and a span round the dial
   * wherever it is, 73 channels of 5 kHz round 9500. */
  TEST_ASSERT_FALSE(dxSweepRange(BAND_SW, &plan, 9500, 0, &r));
  TEST_ASSERT_TRUE(dxSweepRange(BAND_SW, &plan, 9500, 360, &r));
  TEST_ASSERT_EQUAL_UINT16(5, r.stepKHz);
  TEST_ASSERT_EQUAL_UINT16(73, r.count);
  TEST_ASSERT_EQUAL_UINT32(9320, r.lowKHz);
  TEST_ASSERT_EQUAL_UINT32(9680, r.lowKHz + (r.count - 1u) * r.stepKHz);
  /* At the top of shortwave it ends on the band's last channel. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_SW, &plan, 26990, 360, &r));
  TEST_ASSERT_EQUAL_UINT32(27000, r.lowKHz + (r.count - 1u) * r.stepKHz);
  /* A span of more channels than a sweep holds is refused, and one of
   * exactly that many taken. */
  TEST_ASSERT_FALSE(dxSweepRange(BAND_SW, &plan, 9500, DX_SWEEP_MAX * 5, &r));
  TEST_ASSERT_TRUE(
      dxSweepRange(BAND_SW, &plan, 9500, (DX_SWEEP_MAX - 1) * 5, &r));
  TEST_ASSERT_EQUAL_UINT16(DX_SWEEP_MAX, r.count);
  /* Long wave in its 9 kHz channels. */
  TEST_ASSERT_TRUE(dxSweepRange(BAND_LW, &plan, 225, 0, &r));
  TEST_ASSERT_EQUAL_UINT32(144, r.lowKHz);
  TEST_ASSERT_EQUAL_UINT16(42, r.count);
  /* Nothing to fill in, or no plan. */
  TEST_ASSERT_FALSE(dxSweepRange(BAND_FM, NULL, 98300, 0, &r));
  TEST_ASSERT_FALSE(dxSweepRange(BAND_FM, &plan, 98300, 0, NULL));
}

/* A range fits when every channel is in the band, and on FM on its 10 kHz
 * grid. */
static void a_range_fits_only_inside_the_band(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  DxSweepRange r = {87500, 100, 206};
  TEST_ASSERT_TRUE(dxSweepRangeFits(BAND_FM, &plan, &r));
  r.count = 207; /* One past 108.0. */
  TEST_ASSERT_FALSE(dxSweepRangeFits(BAND_FM, &plan, &r));
  r = (DxSweepRange){87400, 100, 10}; /* Starts below the band. */
  TEST_ASSERT_FALSE(dxSweepRangeFits(BAND_FM, &plan, &r));
  r = (DxSweepRange){98305, 100, 10}; /* Off the 10 kHz grid. */
  TEST_ASSERT_FALSE(dxSweepRangeFits(BAND_FM, &plan, &r));
  r = (DxSweepRange){98300, 55, 10};
  TEST_ASSERT_FALSE(dxSweepRangeFits(BAND_FM, &plan, &r));
  r = (DxSweepRange){98300, 100, 0};
  TEST_ASSERT_FALSE(dxSweepRangeFits(BAND_FM, &plan, &r));
  r = (DxSweepRange){87500, 0, 10};
  TEST_ASSERT_FALSE(dxSweepRangeFits(BAND_FM, &plan, &r));
  r = (DxSweepRange){65000, 10, DX_SWEEP_MAX + 1};
  TEST_ASSERT_FALSE(dxSweepRangeFits(BAND_OIRT, &plan, &r));
  /* Medium wave's 9 kHz channels need no 10 kHz grid. */
  r = (DxSweepRange){522, 9, 142};
  TEST_ASSERT_TRUE(dxSweepRangeFits(BAND_MW, &plan, &r));
  TEST_ASSERT_FALSE(dxSweepRangeFits(BAND_MW, NULL, &r));
  TEST_ASSERT_FALSE(dxSweepRangeFits(BAND_MW, &plan, NULL));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(the_mean_rounds_to_the_nearest_tenth);
  RUN_TEST(the_floor_of_a_real_band_is_its_lower_quartile);
  RUN_TEST(the_floor_leaves_out_channels_with_no_reading);
  RUN_TEST(the_median_of_two_real_passes_matches_the_recorded_median);
  RUN_TEST(the_median_of_an_odd_count_is_the_middle_one);
  RUN_TEST(the_median_takes_only_the_same_channels);
  RUN_TEST(a_channel_no_sweep_read_has_no_baseline);
  RUN_TEST(the_peak_holds_the_highest_and_restarts_on_other_channels);
  RUN_TEST(the_history_keeps_the_newest_eight);
  RUN_TEST(a_history_survives_the_file);
  RUN_TEST(a_damaged_file_is_refused_whole);
  RUN_TEST(the_same_channels_needs_channels);
  RUN_TEST(a_channel_is_found_only_on_the_sweep);
  RUN_TEST(the_age_is_in_whole_units);
  RUN_TEST(a_range_is_the_band_or_a_span_inside_it);
  RUN_TEST(a_range_fits_only_inside_the_band);
  return UNITY_END();
}
