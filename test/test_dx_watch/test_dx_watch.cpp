/*
 * Tests for the preset watch. Runs on a PC. The data is recorded AF_Update
 * checks, replayed.
 */
#include <unity.h>

#include <string.h>

#include "captures.h"
#include "core/dx_watch.h"

static DxWatch w;

void setUp(void) {
  memset(&w, 0, sizeof(w));
}
void tearDown(void) {}

static void watchOne(uint32_t khz) {
  dxWatchSetChannels(&w, &khz, 1);
}

/* Feeds the readings and counts what they meant. */
static void replay(const int16_t *level, uint16_t n, uint16_t *suspect,
                   uint16_t *up) {
  *suspect = 0;
  *up = 0;
  for (uint16_t i = 0; i < n; i++) {
    const DxWatchResult r = dxWatchFeed(&w, 95300, level[i], true, NULL);
    *suspect += r == DX_WATCH_SUSPECT;
    *up += r == DX_WATCH_UP;
  }
}

/* No quiet channel in the recorded checks is flagged, and the counts match
 * the expected ones kept with each capture. */
static void no_captured_quiet_channel_is_flagged(void) {
  for (size_t i = 0; i < sizeof(kCaptures) / sizeof(kCaptures[0]); i++) {
    setUp();
    watchOne(95300);
    uint16_t suspect = 0;
    uint16_t up = 0;
    replay(kCaptures[i].level, kCaptures[i].count, &suspect, &up);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(kCaptures[i].suspect, suspect,
                                     kCaptures[i].file);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(kCaptures[i].up, up, kCaptures[i].file);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, up, kCaptures[i].file);
  }
}

/* A real empty channel with a station arriving 15 dB up half way through:
 * flagged once, on the second reading up, and not again while it stays. */
static void a_station_arriving_on_a_quiet_channel_is_flagged_once(void) {
  const int16_t *quiet = kWatch95600114k20260928;
  int16_t level[200];
  for (int i = 0; i < 200; i++) {
    level[i] = (int16_t)(quiet[i] + (i >= 100 ? 150 : 0));
  }
  watchOne(95300);
  uint16_t flaggedAt = 0;
  uint16_t ups = 0;
  for (uint16_t i = 0; i < 200; i++) {
    int16_t rise = 0;
    const DxWatchResult r = dxWatchFeed(&w, 95300, level[i], true, &rise);
    if (i == 100) {
      TEST_ASSERT_EQUAL(DX_WATCH_SUSPECT, r);
      TEST_ASSERT_TRUE(rise > DX_WATCH_RISE_TENTHS);
    }
    if (r == DX_WATCH_UP) {
      ups++;
      flaggedAt = i;
    }
  }
  TEST_ASSERT_EQUAL_UINT16(1, ups);
  TEST_ASSERT_EQUAL_UINT16(101, flaggedAt);
}

/* Nothing is judged until the floor has its five readings. */
static void the_first_readings_only_fill_the_floor(void) {
  watchOne(95300);
  for (int i = 0; i < DX_WATCH_HISTORY; i++) {
    int16_t rise = 7;
    TEST_ASSERT_EQUAL(DX_WATCH_QUIET,
                      dxWatchFeed(&w, 95300, (int16_t)(i * 200), true, &rise));
    TEST_ASSERT_EQUAL_INT16(0, rise);
  }
  TEST_ASSERT_EQUAL(DX_WATCH_SUSPECT, dxWatchFeed(&w, 95300, 2000, true, NULL));
}

/* On the edge: exactly 8 dB over the floor is not a rise, a tenth more is. */
static void the_rise_is_measured_from_the_median(void) {
  watchOne(95300);
  const int16_t floor5[] = {10, -40, 30, 20, 0}; /* Median 10. */
  for (int i = 0; i < 5; i++) {
    dxWatchFeed(&w, 95300, floor5[i], true, NULL);
  }
  int16_t rise = 0;
  TEST_ASSERT_EQUAL(
      DX_WATCH_QUIET,
      dxWatchFeed(&w, 95300, 10 + DX_WATCH_RISE_TENTHS, true, &rise));
  TEST_ASSERT_EQUAL_INT16(DX_WATCH_RISE_TENTHS, rise);
  /* That reading was not a rise, so it went into the floor in place of the
   * oldest, 10. The median is now 20, so the next must clear 20 more. */
  TEST_ASSERT_EQUAL(
      DX_WATCH_SUSPECT,
      dxWatchFeed(&w, 95300, 10 + DX_WATCH_RISE_TENTHS + 1 + 20, true, NULL));
}

/* One reading up and the next back down is not flagged, and the one that
 * rose is left out of the floor. */
static void a_single_rise_is_not_flagged(void) {
  watchOne(95300);
  for (int i = 0; i < 5; i++) {
    dxWatchFeed(&w, 95300, 0, true, NULL);
  }
  TEST_ASSERT_EQUAL(DX_WATCH_SUSPECT, dxWatchFeed(&w, 95300, 300, true, NULL));
  TEST_ASSERT_EQUAL(DX_WATCH_QUIET, dxWatchFeed(&w, 95300, 0, true, NULL));
  int16_t rise = 0;
  dxWatchFeed(&w, 95300, 0, true, &rise);
  TEST_ASSERT_EQUAL_INT16(0, rise);
  TEST_ASSERT_FALSE(w.confirming);
}

/* The watch goes round the list, and a rise holds it on that channel for
 * the confirming reading. */
static void it_goes_round_and_holds_on_a_rise(void) {
  const uint32_t khz[] = {88000, 90000, 92000};
  dxWatchSetChannels(&w, khz, 3);
  uint32_t next = 0;
  for (int round = 0; round < 5; round++) {
    for (int i = 0; i < 3; i++) {
      TEST_ASSERT_TRUE(dxWatchNext(&w, &next));
      TEST_ASSERT_EQUAL_UINT32(khz[i], next);
      dxWatchFeed(&w, next, 0, true, NULL);
    }
  }
  TEST_ASSERT_TRUE(dxWatchNext(&w, &next));
  TEST_ASSERT_EQUAL(DX_WATCH_SUSPECT, dxWatchFeed(&w, next, 400, true, NULL));
  uint32_t again = 0;
  TEST_ASSERT_TRUE(dxWatchNext(&w, &again));
  TEST_ASSERT_EQUAL_UINT32(88000, again);
  TEST_ASSERT_EQUAL(DX_WATCH_UP, dxWatchFeed(&w, again, 400, true, NULL));
  TEST_ASSERT_TRUE(dxWatchNext(&w, &next));
  TEST_ASSERT_EQUAL_UINT32(90000, next);
}

/* A check that gave no reading moves on and drops a confirmation. */
static void a_check_with_no_reading_moves_on(void) {
  const uint32_t khz[] = {88000, 90000};
  dxWatchSetChannels(&w, khz, 2);
  for (int i = 0; i < 10; i++) {
    uint32_t next = 0;
    dxWatchNext(&w, &next);
    dxWatchFeed(&w, next, 0, true, NULL);
  }
  TEST_ASSERT_EQUAL(DX_WATCH_SUSPECT, dxWatchFeed(&w, 88000, 400, true, NULL));
  TEST_ASSERT_EQUAL(DX_WATCH_QUIET, dxWatchFeed(&w, 88000, 400, false, NULL));
  uint32_t next = 0;
  dxWatchNext(&w, &next);
  TEST_ASSERT_EQUAL_UINT32(90000, next);
  TEST_ASSERT_FALSE(w.confirming);
}

/* A reading of a channel that is not the one due changes nothing. */
static void a_reading_of_another_channel_is_ignored(void) {
  const uint32_t khz[] = {88000, 90000};
  dxWatchSetChannels(&w, khz, 2);
  TEST_ASSERT_EQUAL(DX_WATCH_QUIET, dxWatchFeed(&w, 90000, 500, true, NULL));
  TEST_ASSERT_EQUAL_UINT8(0, w.ch[1].held);
  uint32_t next = 0;
  dxWatchNext(&w, &next);
  TEST_ASSERT_EQUAL_UINT32(88000, next);
}

/* Given again, the list keeps each channel's readings and the one due. */
static void a_list_given_again_keeps_the_readings(void) {
  const uint32_t first[] = {88000, 90000, 92000};
  dxWatchSetChannels(&w, first, 3);
  dxWatchFeed(&w, 88000, 11, true, NULL);
  dxWatchFeed(&w, 90000, 22, true, NULL);
  const uint32_t second[] = {92000, 90000, 95000};
  dxWatchSetChannels(&w, second, 3);
  TEST_ASSERT_EQUAL_UINT8(3, w.count);
  TEST_ASSERT_EQUAL_UINT32(92000, w.ch[0].khz);
  TEST_ASSERT_EQUAL_UINT8(1, w.ch[1].held);
  TEST_ASSERT_EQUAL_INT16(22, w.ch[1].recent[0]);
  TEST_ASSERT_EQUAL_UINT8(0, w.ch[2].held);
  uint32_t next = 0;
  dxWatchNext(&w, &next);
  TEST_ASSERT_EQUAL_UINT32(92000, next);
}

/* The channel being confirmed dropped from the list drops the
 * confirmation too. */
static void dropping_the_channel_confirmed_drops_the_confirmation(void) {
  watchOne(88000);
  for (int i = 0; i < 5; i++) {
    dxWatchFeed(&w, 88000, 0, true, NULL);
  }
  dxWatchFeed(&w, 88000, 400, true, NULL);
  TEST_ASSERT_TRUE(w.confirming);
  const uint32_t other = 90000;
  dxWatchSetChannels(&w, &other, 1);
  TEST_ASSERT_FALSE(w.confirming);
}

static void nothing_to_watch_is_safe(void) {
  uint32_t next = 1;
  TEST_ASSERT_FALSE(dxWatchNext(&w, &next));
  TEST_ASSERT_EQUAL(DX_WATCH_QUIET, dxWatchFeed(&w, 88000, 0, true, NULL));
  dxWatchSetChannels(&w, NULL, 5);
  TEST_ASSERT_EQUAL_UINT8(0, w.count);
  TEST_ASSERT_FALSE(dxWatchNext(NULL, &next));
  TEST_ASSERT_FALSE(dxWatchNext(&w, NULL));
  TEST_ASSERT_EQUAL(DX_WATCH_QUIET, dxWatchFeed(NULL, 88000, 0, true, NULL));
  dxWatchSetChannels(NULL, &next, 1);
}

/* More channels than presets exist are cut to DX_WATCH_MAX. */
static void a_list_longer_than_the_presets_is_cut(void) {
  uint32_t khz[DX_WATCH_MAX + 5];
  for (int i = 0; i < DX_WATCH_MAX + 5; i++) {
    khz[i] = (uint32_t)(87500 + 100 * i);
  }
  dxWatchSetChannels(&w, khz, DX_WATCH_MAX + 5);
  TEST_ASSERT_EQUAL_UINT8(DX_WATCH_MAX, w.count);
  /* A full list given again all new starts every channel fresh. */
  dxWatchFeed(&w, 87500, 33, true, NULL);
  for (int i = 0; i < DX_WATCH_MAX; i++) {
    khz[i] = (uint32_t)(65800 + 10 * i);
  }
  dxWatchSetChannels(&w, khz, DX_WATCH_MAX);
  TEST_ASSERT_EQUAL_UINT8(DX_WATCH_MAX, w.count);
  for (int i = 0; i < DX_WATCH_MAX; i++) {
    TEST_ASSERT_EQUAL_UINT32(khz[i], w.ch[i].khz);
    TEST_ASSERT_EQUAL_UINT8(0, w.ch[i].held);
  }
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(no_captured_quiet_channel_is_flagged);
  RUN_TEST(a_station_arriving_on_a_quiet_channel_is_flagged_once);
  RUN_TEST(the_first_readings_only_fill_the_floor);
  RUN_TEST(the_rise_is_measured_from_the_median);
  RUN_TEST(a_single_rise_is_not_flagged);
  RUN_TEST(it_goes_round_and_holds_on_a_rise);
  RUN_TEST(a_check_with_no_reading_moves_on);
  RUN_TEST(a_reading_of_another_channel_is_ignored);
  RUN_TEST(a_list_given_again_keeps_the_readings);
  RUN_TEST(dropping_the_channel_confirmed_drops_the_confirmation);
  RUN_TEST(nothing_to_watch_is_safe);
  RUN_TEST(a_list_longer_than_the_presets_is_cut);
  return UNITY_END();
}
