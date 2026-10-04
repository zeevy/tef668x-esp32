/*
 * Tests for the DX scan's RDS times. Runs on a PC.
 */
#include <unity.h>

#include <string.h>

#include "core/dx_timing.h"

static DxTiming t;
static RdsInfo rds;

void setUp(void) {
  memset(&t, 0, sizeof(t));
  memset(&rds, 0, sizeof(rds));
}
void tearDown(void) {}

/* A station as the decoder builds it up: the lock, a group, a clean block
 * A, then the PI twice. Each time is the first look that saw it. */
static void each_event_is_timed_from_the_first_look_that_saw_it(void) {
  dxTimingStart(&t, 93500, 1000);
  dxTimingFeed(&t, 93500, &rds, 120, 114, 1040);
  rds.synchronised = true;
  dxTimingFeed(&t, 93500, &rds, 150, 114, 1080);
  rds.hasBlockErrors = true;
  dxTimingFeed(&t, 93500, &rds, 140, 114, 1160);
  rds.hasPiHeard = true;
  rds.piHeard = 0x0935;
  dxTimingFeed(&t, 93500, &rds, 130, 114, 1240);
  rds.hasPi = true;
  rds.pi = 0x0935;
  dxTimingFeed(&t, 93500, &rds, 130, 114, 1400);
  dxTimingFeed(&t, 93500, &rds, 130, 114, 1600);
  TEST_ASSERT_TRUE(dxTimingEnd(&t, 3500));
  TEST_ASSERT_EQUAL_UINT16(80, t.syncMs);
  TEST_ASSERT_EQUAL_UINT16(160, t.groupMs);
  TEST_ASSERT_EQUAL_UINT16(240, t.cleanAMs);
  TEST_ASSERT_EQUAL_UINT16(400, t.piMs);
  TEST_ASSERT_EQUAL_HEX16(0x0935, t.pi);
  TEST_ASSERT_EQUAL_INT16(150, t.levelTenths);
  TEST_ASSERT_EQUAL_UINT16(2500, t.dwellMs);
}

/* A look taken while the dial is still on the channel before, or already
 * on the next, does not count for this one. */
static void a_look_off_the_channel_is_ignored(void) {
  dxTimingStart(&t, 93500, 0);
  rds.synchronised = true;
  dxTimingFeed(&t, 93400, &rds, 500, 114, 40);
  TEST_ASSERT_EQUAL_UINT16(DX_TIMING_NEVER, t.syncMs);
  TEST_ASSERT_EQUAL_INT16(INT16_MIN, t.levelTenths);
  dxTimingFeed(&t, 93500, &rds, 100, 114, 80);
  TEST_ASSERT_EQUAL_UINT16(80, t.syncMs);
}

/* An empty channel, never locked, is not kept. */
static void a_channel_that_never_locked_is_not_kept(void) {
  dxTimingStart(&t, 95300, 0);
  dxTimingFeed(&t, 95300, &rds, -90, 114, 40);
  TEST_ASSERT_FALSE(dxTimingEnd(&t, 2500));
  TEST_ASSERT_FALSE(t.active);
  TEST_ASSERT_FALSE(dxTimingEnd(&t, 2600));
}

/* A station sending 0000 confirms with no PI to name. */
static void a_station_sending_zero_confirms_with_no_pi(void) {
  dxTimingStart(&t, 94300, 0);
  rds.synchronised = true;
  rds.piZero = true;
  dxTimingFeed(&t, 94300, &rds, 300, 114, 200);
  TEST_ASSERT_EQUAL_UINT16(200, t.piMs);
  TEST_ASSERT_EQUAL_HEX16(0, t.pi);
}

/* A dwell longer than the field holds is held just under "never". */
static void a_very_long_dwell_is_held_under_never(void) {
  dxTimingStart(&t, 93500, 0);
  rds.synchronised = true;
  dxTimingFeed(&t, 93500, &rds, 0, 114, 100000);
  TEST_ASSERT_EQUAL_UINT16(DX_TIMING_NEVER - 1u, t.syncMs);
  TEST_ASSERT_TRUE(dxTimingEnd(&t, 200000));
  TEST_ASSERT_EQUAL_UINT16(DX_TIMING_NEVER - 1u, t.dwellMs);
}

static void the_csv_line_has_every_column(void) {
  TEST_ASSERT_EQUAL_STRING(
      "utc,khz,pi,level_dbuv,width_khz,sync_ms,group_ms,clean_a_ms,pi_ms,"
      "dwell_ms\n",
      dxTimingCsvHeader());
  dxTimingStart(&t, 93500, 1000);
  rds.synchronised = true;
  rds.hasBlockErrors = true;
  rds.hasPiHeard = true;
  rds.hasPi = true;
  rds.pi = 0x0935;
  dxTimingFeed(&t, 93500, &rds, 455, 114, 1120);
  dxTimingEnd(&t, 3500);
  char line[128];
  /* 1790611201 is 2026-09-28 16:00:01 UTC. */
  dxTimingCsvLine(&t, 1790611201UL, line, sizeof(line));
  TEST_ASSERT_EQUAL_STRING(
      "2026-09-28T16:00:01Z,93500,0935,45.5,114,120,120,120,120,2500\n", line);
}

/* No clock, no PI, and times that never came are empty fields; a level
 * just below zero keeps its minus. */
static void what_is_not_known_is_an_empty_field(void) {
  dxTimingStart(&t, 88000, 0);
  rds.synchronised = true;
  dxTimingFeed(&t, 88000, &rds, -5, 114, 80);
  dxTimingEnd(&t, 2500);
  char line[128];
  dxTimingCsvLine(&t, 0, line, sizeof(line));
  TEST_ASSERT_EQUAL_STRING(",88000,,-0.5,114,80,,,,2500\n", line);
}

/* Every field at its widest. The glue writes lines from a 96 byte
 * buffer. */
static void the_longest_line_fits_the_buffer(void) {
  memset(&t, 0, sizeof(t));
  t.khz = 0xFFFFFFFFUL;
  t.pi = 0xFFFF;
  t.levelTenths = -32767;
  t.widthKHz = 65535;
  t.syncMs = t.groupMs = t.cleanAMs = t.piMs = DX_TIMING_NEVER - 1u;
  t.dwellMs = DX_TIMING_NEVER - 1u;
  char line[128];
  TEST_ASSERT_EQUAL_size_t(
      81, dxTimingCsvLine(&t, 0xFFFFFFFFUL, line, sizeof(line)));
}

static void a_null_or_short_call_is_safe(void) {
  dxTimingStart(NULL, 1, 0);
  dxTimingFeed(NULL, 1, &rds, 0, 114, 0);
  TEST_ASSERT_FALSE(dxTimingEnd(NULL, 0));
  dxTimingFeed(&t, 1, &rds, 0, 114, 0); /* Nothing under way. */
  TEST_ASSERT_FALSE(t.active);
  char line[128];
  TEST_ASSERT_EQUAL_size_t(0, dxTimingCsvLine(NULL, 0, line, sizeof(line)));
  dxTimingStart(&t, 93500, 0);
  dxTimingFeed(&t, 93500, NULL, 42, 114, 10);
  TEST_ASSERT_EQUAL_INT16(42, t.levelTenths);
  dxTimingEnd(&t, 100);
  char tiny[8];
  TEST_ASSERT_TRUE(dxTimingCsvLine(&t, 0, tiny, sizeof(tiny)) >= sizeof(tiny));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(each_event_is_timed_from_the_first_look_that_saw_it);
  RUN_TEST(a_look_off_the_channel_is_ignored);
  RUN_TEST(a_channel_that_never_locked_is_not_kept);
  RUN_TEST(a_station_sending_zero_confirms_with_no_pi);
  RUN_TEST(a_very_long_dwell_is_held_under_never);
  RUN_TEST(the_csv_line_has_every_column);
  RUN_TEST(what_is_not_known_is_an_empty_field);
  RUN_TEST(the_longest_line_fits_the_buffer);
  RUN_TEST(a_null_or_short_call_is_safe);
  return UNITY_END();
}
