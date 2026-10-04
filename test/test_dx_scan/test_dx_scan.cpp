/*
 * Tests for the DX scanner. Runs on a PC.
 */
#include <unity.h>

#include "core/dx_scan.h"

static DxScan scan;

void setUp(void) {
  dxScanReset(&scan);
}
void tearDown(void) {}

/* The FM band as this radio has it: 87.5 to 108.0 in 100 kHz, 206
 * channels. */
#define LOW 87500u
#define HIGH 108000u
#define STEP 100u

/* 87.6 and 87.7 are stored channels, passed over. */
static bool stored(void *ctx, uint32_t khz) {
  (void)ctx;
  return khz == 87600u || khz == 87700u;
}

static DxScanBand band;

/* A band walk from `low` to `high`, with the stored channels passed over
 * when `skip` is set. */
static DxScanPlan bandPlan(uint32_t low, uint32_t high, bool skip) {
  band.lowKHz = low;
  band.stepKHz = STEP;
  band.skip = skip ? stored : NULL;
  band.skipCtx = NULL;
  DxScanPlan p;
  p.count = dxScanBandCount(low, high, STEP);
  p.channel = dxScanBandChannel;
  p.ctx = &band;
  p.dwellMs = DX_SCAN_DWELL_MS;
  p.loop = false;
  return p;
}

static DxScanAction startAt(uint32_t dial) {
  const DxScanPlan p = bandPlan(LOW, HIGH, true);
  return dxScanStart(&scan, &p, dial);
}

static void a_scan_starts_at_the_bottom_of_the_band(void) {
  DxScanAction a = startAt(106400);
  TEST_ASSERT_TRUE(a.tune);
  TEST_ASSERT_EQUAL_UINT32(LOW, a.khz);
  TEST_ASSERT_EQUAL(DX_SCAN_RUNNING, scan.state);
  TEST_ASSERT_EQUAL_UINT16(206, dxScanTotal(&scan));
  TEST_ASSERT_EQUAL_UINT16(0, dxScanPassed(&scan));
  TEST_ASSERT_EQUAL_UINT32(DX_SCAN_DWELL_MS, dxScanLeftMs(&scan, 0));
}

/* The dwell counts from when the dial is on the channel, to the
 * millisecond, and the next channel passes over the stored ones. */
static void the_dwell_runs_from_the_landing_and_skips_stored_channels(void) {
  startAt(106400);
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 1000, 106400, false, false, false).tune);
  TEST_ASSERT_FALSE(scan.landed);
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 1100, LOW, false, false, false).tune);
  TEST_ASSERT_TRUE(scan.landed);
  TEST_ASSERT_EQUAL_UINT32(1200, dxScanLeftMs(&scan, 2400));
  TEST_ASSERT_FALSE(
      dxScanPoll(&scan, 1100 + DX_SCAN_DWELL_MS - 1, LOW, false, false, false)
          .tune);
  DxScanAction a =
      dxScanPoll(&scan, 1100 + DX_SCAN_DWELL_MS, LOW, false, false, false);
  TEST_ASSERT_TRUE(a.tune);
  TEST_ASSERT_EQUAL_UINT32(87800, a.khz);
  TEST_ASSERT_EQUAL_UINT16(3, dxScanPassed(&scan));
}

/* A PI heard before the dial lands is the last channel's. A NEW one heard
 * after stops the scan there. */
static void a_pi_stops_it_only_once_the_dial_is_there(void) {
  startAt(106400);
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 1000, 106400, true, false, true).tune);
  TEST_ASSERT_EQUAL(DX_SCAN_RUNNING, scan.state);
  dxScanPoll(&scan, 1100, LOW, true, false, true);
  TEST_ASSERT_EQUAL(DX_SCAN_STOPPED, scan.state);
  TEST_ASSERT_TRUE(scan.onCatch);
  TEST_ASSERT_EQUAL_UINT16(1, scan.found);
  TEST_ASSERT_EQUAL_UINT32(0, dxScanLeftMs(&scan, 1200));
  /* Stopped, it looks at nothing. */
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 9000, LOW, false, false, false).tune);
}

/* A known PI is caught, and found, but nothing is left to wait for on its
 * channel: the scan goes on at once, dwell or not. */
static void a_known_pi_moves_it_on_at_once(void) {
  startAt(106400);
  dxScanPoll(&scan, 1000, LOW, false, false, false);
  DxScanAction a = dxScanPoll(&scan, 1200, LOW, true, false, false);
  TEST_ASSERT_TRUE(a.tune);
  TEST_ASSERT_EQUAL_UINT32(87800, a.khz);
  TEST_ASSERT_EQUAL(DX_SCAN_RUNNING, scan.state);
  TEST_ASSERT_EQUAL_UINT16(1, scan.found);
}

/* A tune the radio would not take never lands: after a dwell's wait the
 * scan gives the channel up and goes on, rather than sitting there muted. */
static void a_tune_that_never_lands_is_given_up(void) {
  startAt(106400);
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 1000, 106400, false, false, false).tune);
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 1000 + DX_SCAN_DWELL_MS - 1, 106400,
                               false, false, false)
                        .tune);
  DxScanAction a =
      dxScanPoll(&scan, 1000 + DX_SCAN_DWELL_MS, 106400, false, false, false);
  TEST_ASSERT_TRUE(a.tune);
  TEST_ASSERT_EQUAL_UINT32(87800, a.khz);
  TEST_ASSERT_EQUAL(DX_SCAN_RUNNING, scan.state);
}

/* The dial anywhere but where it was or where it was sent means somebody
 * tuned: stop, and leave the dial. */
static void somebody_tuning_stops_it(void) {
  startAt(106400);
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 1000, 98300, false, false, false).tune);
  TEST_ASSERT_EQUAL(DX_SCAN_STOPPED, scan.state);
  TEST_ASSERT_FALSE(scan.onCatch);

  startAt(106400);
  dxScanPoll(&scan, 1000, LOW, false, false, false);
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 1500, 87600, false, false, false).tune);
  TEST_ASSERT_EQUAL(DX_SCAN_STOPPED, scan.state);
}

/* A resume goes on from the channel after the stop, wherever the dial was
 * turned to meanwhile. */
static void a_resume_goes_on_after_the_stop(void) {
  startAt(106400);
  dxScanPoll(&scan, 1000, LOW, false, false, false);
  dxScanStop(&scan);
  TEST_ASSERT_EQUAL(DX_SCAN_STOPPED, scan.state);
  TEST_ASSERT_FALSE(scan.onCatch);
  DxScanAction a = dxScanResume(&scan, 98300);
  TEST_ASSERT_TRUE(a.tune);
  TEST_ASSERT_EQUAL_UINT32(87800, a.khz);
  TEST_ASSERT_EQUAL(DX_SCAN_RUNNING, scan.state);
  /* The dial is still on 98.3. The tune has not landed yet, and this is not
   * a retune. */
  dxScanPoll(&scan, 1100, 98300, false, false, false);
  TEST_ASSERT_EQUAL(DX_SCAN_RUNNING, scan.state);
  /* Only from a stop. */
  TEST_ASSERT_FALSE(dxScanResume(&scan, 98300).tune);
  dxScanStop(&scan);
  dxScanStop(&scan);
  TEST_ASSERT_EQUAL(DX_SCAN_STOPPED, scan.state);
}

/* At the top of the band it finishes, found count kept, and sends the dial
 * back to where the scan started. A resume from the top does the same. */
static void the_top_of_the_band_finishes_and_goes_back(void) {
  const DxScanPlan p = bandPlan(107800, HIGH, false);
  dxScanStart(&scan, &p, 106400);
  uint32_t now = 0;
  for (uint32_t khz = 107800; khz <= 107900; khz += STEP) {
    dxScanPoll(&scan, now, khz, false, false, false);
    now += DX_SCAN_DWELL_MS;
    dxScanPoll(&scan, now, khz, false, false, false);
  }
  dxScanPoll(&scan, now, HIGH, true, false, true);
  TEST_ASSERT_EQUAL(DX_SCAN_STOPPED, scan.state);
  DxScanAction a = dxScanResume(&scan, HIGH);
  TEST_ASSERT_TRUE(a.tune);
  TEST_ASSERT_EQUAL_UINT32(106400, a.khz);
  TEST_ASSERT_EQUAL(DX_SCAN_IDLE, scan.state);
  TEST_ASSERT_TRUE(scan.finished);
  TEST_ASSERT_EQUAL_UINT16(1, scan.found);
  TEST_ASSERT_EQUAL_UINT16(3, dxScanPassed(&scan));
  TEST_ASSERT_EQUAL_UINT16(3, dxScanTotal(&scan));
}

static void a_band_with_nothing_to_scan_does_not_start(void) {
  DxScanPlan p = bandPlan(87600, 87700, true);
  TEST_ASSERT_FALSE(dxScanStart(&scan, &p, 106400).tune);
  TEST_ASSERT_EQUAL(DX_SCAN_IDLE, scan.state);
  p = bandPlan(HIGH, LOW, false);
  TEST_ASSERT_EQUAL_UINT16(0, p.count);
  TEST_ASSERT_FALSE(dxScanStart(&scan, &p, 1).tune);
  TEST_ASSERT_EQUAL_UINT16(0, dxScanBandCount(LOW, HIGH, 0));
  p = bandPlan(LOW, HIGH, false);
  p.dwellMs = 0;
  TEST_ASSERT_FALSE(dxScanStart(&scan, &p, 1).tune);
  p.dwellMs = DX_SCAN_DWELL_MS;
  p.channel = NULL;
  TEST_ASSERT_FALSE(dxScanStart(&scan, &p, 1).tune);
  TEST_ASSERT_FALSE(dxScanStart(&scan, NULL, 1).tune);
  TEST_ASSERT_FALSE(dxScanStart(NULL, &p, 1).tune);
  uint32_t khz = 0;
  TEST_ASSERT_FALSE(dxScanBandChannel(NULL, 0, &khz));
  TEST_ASSERT_FALSE(dxScanBandChannel(&band, 0, NULL));
}

/* A looping scan goes round from the first position instead of finishing,
 * and keeps its found count; its dwell is the plan's. */
static void a_looping_scan_goes_round_with_its_own_dwell(void) {
  DxScanPlan p = bandPlan(107900, HIGH, false);
  p.loop = true;
  p.dwellMs = 500;
  dxScanStart(&scan, &p, 106400);
  dxScanPoll(&scan, 0, 107900, false, false, false);
  TEST_ASSERT_EQUAL_UINT32(500, dxScanLeftMs(&scan, 0));
  DxScanAction a = dxScanPoll(&scan, 500, 107900, false, false, false);
  TEST_ASSERT_EQUAL_UINT32(HIGH, a.khz);
  dxScanPoll(&scan, 600, HIGH, false, false, false);
  a = dxScanPoll(&scan, 1100, HIGH, false, false, false);
  TEST_ASSERT_TRUE(a.tune);
  TEST_ASSERT_EQUAL_UINT32(107900, a.khz);
  TEST_ASSERT_EQUAL(DX_SCAN_RUNNING, scan.state);
  TEST_ASSERT_FALSE(scan.finished);
}

/* Any list of channels, such as the memory channels in slot order, walks
 * the same way, passing over the empty ones. */
static const uint32_t kSlots[] = {98300, 0, 93500, 0};

static bool slotChannel(void *ctx, uint16_t pos, uint32_t *khz) {
  (void)ctx;
  if (kSlots[pos] == 0) {
    return false;
  }
  *khz = kSlots[pos];
  return true;
}

static void a_list_of_channels_walks_in_its_own_order(void) {
  DxScanPlan p;
  p.count = 4;
  p.channel = slotChannel;
  p.ctx = NULL;
  p.dwellMs = DX_SCAN_DWELL_MS;
  p.loop = false;
  DxScanAction a = dxScanStart(&scan, &p, 106400);
  TEST_ASSERT_EQUAL_UINT32(98300, a.khz);
  dxScanPoll(&scan, 0, 98300, false, false, false);
  a = dxScanPoll(&scan, DX_SCAN_DWELL_MS, 98300, false, false, false);
  TEST_ASSERT_EQUAL_UINT32(93500, a.khz);
  TEST_ASSERT_EQUAL_UINT16(2, dxScanPassed(&scan));
  /* A PI its rule stops on stops it; the end of the list finishes. */
  dxScanPoll(&scan, 3000, 93500, false, false, false);
  dxScanPoll(&scan, 3100, 93500, true, false, true);
  TEST_ASSERT_EQUAL(DX_SCAN_STOPPED, scan.state);
  a = dxScanResume(&scan, 93500);
  TEST_ASSERT_EQUAL_UINT32(106400, a.khz);
  TEST_ASSERT_TRUE(scan.finished);
  TEST_ASSERT_EQUAL_UINT16(4, dxScanPassed(&scan));
}

static void nothing_happens_without_a_scan(void) {
  TEST_ASSERT_FALSE(dxScanPoll(NULL, 0, LOW, true, false, true).tune);
  TEST_ASSERT_FALSE(dxScanResume(NULL, LOW).tune);
  dxScanStop(NULL);
  dxScanReset(NULL);
  TEST_ASSERT_EQUAL_UINT32(0, dxScanLeftMs(NULL, 0));
  TEST_ASSERT_EQUAL_UINT16(0, dxScanPassed(NULL));
  TEST_ASSERT_EQUAL_UINT16(0, dxScanTotal(NULL));
  TEST_ASSERT_EQUAL_UINT16(0, dxScanPassed(&scan));
  TEST_ASSERT_EQUAL_UINT16(0, dxScanTotal(&scan));
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 0, LOW, true, false, true).tune);
}

/* The millisecond clock wraps after 49 days; the dwell still ends. */
static void the_dwell_ends_across_a_clock_wrap(void) {
  startAt(106400);
  dxScanPoll(&scan, 0xFFFFFF00u, LOW, false, false, false);
  TEST_ASSERT_FALSE(
      dxScanPoll(&scan, 0x00000100u, LOW, false, false, false).tune);
  TEST_ASSERT_TRUE(dxScanPoll(&scan, 0xFFFFFF00u + DX_SCAN_DWELL_MS, LOW, false,
                              false, false)
                       .tune);
}

/* Held on a channel, a PI is counted once and the dwell runs out before
 * the scan moves: the rule for waiting on a NEW station's name. */
static void a_held_channel_counts_once_and_waits_the_dwell(void) {
  startAt(106400);
  dxScanPoll(&scan, 1000, LOW, false, false, false);
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 1200, LOW, true, true, false).tune);
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 1300, LOW, true, true, false).tune);
  TEST_ASSERT_EQUAL_UINT16(1, scan.found);
  TEST_ASSERT_TRUE(
      dxScanPoll(&scan, 1000 + DX_SCAN_DWELL_MS, LOW, true, true, false).tune);
  TEST_ASSERT_EQUAL_UINT16(1, scan.found);
}

static const uint32_t kOne[] = {98300};

static bool oneChannel(void *ctx, uint16_t pos, uint32_t *khz) {
  (void)ctx;
  *khz = kOne[pos];
  return true;
}

/* A looping walk of one channel: nothing is tuned again, a heard PI does
 * not spin it round, and each lap counts its station once. */
static void a_loop_of_one_channel_neither_spins_nor_overcounts(void) {
  DxScanPlan p;
  p.count = 1;
  p.channel = oneChannel;
  p.ctx = NULL;
  p.dwellMs = 500;
  p.loop = true;
  dxScanStart(&scan, &p, 106400);
  dxScanPoll(&scan, 0, 98300, false, false, false);
  for (uint32_t t = 40; t < 480; t += 40) {
    TEST_ASSERT_FALSE(dxScanPoll(&scan, t, 98300, true, false, false).tune);
  }
  TEST_ASSERT_EQUAL_UINT16(1, scan.found);
  TEST_ASSERT_FALSE(dxScanPoll(&scan, 500, 98300, true, false, false).tune);
  TEST_ASSERT_EQUAL(DX_SCAN_RUNNING, scan.state);
  TEST_ASSERT_EQUAL_UINT16(0, scan.found);
  dxScanPoll(&scan, 540, 98300, true, false, false);
  TEST_ASSERT_EQUAL_UINT16(1, scan.found);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(a_scan_starts_at_the_bottom_of_the_band);
  RUN_TEST(the_dwell_runs_from_the_landing_and_skips_stored_channels);
  RUN_TEST(a_pi_stops_it_only_once_the_dial_is_there);
  RUN_TEST(a_known_pi_moves_it_on_at_once);
  RUN_TEST(a_held_channel_counts_once_and_waits_the_dwell);
  RUN_TEST(a_loop_of_one_channel_neither_spins_nor_overcounts);
  RUN_TEST(a_tune_that_never_lands_is_given_up);
  RUN_TEST(somebody_tuning_stops_it);
  RUN_TEST(a_resume_goes_on_after_the_stop);
  RUN_TEST(the_top_of_the_band_finishes_and_goes_back);
  RUN_TEST(a_band_with_nothing_to_scan_does_not_start);
  RUN_TEST(a_looping_scan_goes_round_with_its_own_dwell);
  RUN_TEST(a_list_of_channels_walks_in_its_own_order);
  RUN_TEST(nothing_happens_without_a_scan);
  RUN_TEST(the_dwell_ends_across_a_clock_wrap);
  return UNITY_END();
}
