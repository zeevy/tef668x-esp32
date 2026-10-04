/*
 * Tests for the band scan's walk of each band, `bandScanWalkFor`, and its
 * decision for each channel, `bandScanJudge`, the same function the scan's
 * task calls: keep what `seekShouldStop` says is a station, skip it if a
 * memory channel is already near enough, otherwise add it to the lowest free
 * slot. Runs on a PC.
 *
 * The task itself tunes through the radio task, which does not build off the
 * ESP32, so its walk is replayed here with real readings from a seek sweep
 * standing in for the probe, the same sweep the seek tests use. A rule
 * checked only against invented numbers proves nothing about real signals.
 */
#include <unity.h>

#include <stdio.h>
#include <string.h>

#include "../test_seek/am_heard.h"
#include "../test_seek/fm_sweep.h"
#include "core/band_scan.h"
#include "core/memory.h"
#include "core/seek.h"

void setUp(void) {}
void tearDown(void) {}

/* The FM walk with the band plan's defaults, which the sweep was taken on. */
static BandScanWalk fmWalk(void) {
  BandScanWalk w;
  TEST_ASSERT_TRUE(bandScanWalkFor(BAND_FM, NULL, &w));
  return w;
}

static SeekReading readingAt(size_t i) {
  SeekReading r;
  memset(&r, 0, sizeof(r));
  r.valid = true;
  r.levelTenths = kFmSweep[i].levelTenths;
  r.noiseTenths = kFmSweep[i].noiseTenths;
  r.multipathTenths = kFmSweep[i].multipathTenths;
  r.offsetTenths = kFmSweep[i].offsetTenths;
  return r;
}

/* The stored channels the decision asks about, a store in memory here. */
static int findNearIn(void *ctx, uint8_t band, uint32_t freqKHz,
                      uint32_t toleranceKHz) {
  return memoryFindNear((const MemoryStore *)ctx, band, freqKHz, toleranceKHz);
}

/*
 * The scan's walk with the tuning and the settle time left out: for each
 * channel in the sweep, the verdict from `bandScanJudge`, and on an add a
 * write to the lowest free slot of `m`, as the store's save does.
 */
static void scanSweep(MemoryStore *m, int *found, int *added) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  const BandScanWalk walk = fmWalk();
  *found = 0;
  *added = 0;
  for (size_t i = 0; i < FM_SWEEP_COUNT; i++) {
    SeekReading r = readingAt(i);
    const BandScanVerdict verdict =
        bandScanJudge(&cfg, &walk, kFmSweep[i].khz, &r, findNearIn, m);
    if (verdict == BAND_SCAN_NOT_A_STATION) {
      continue;
    }
    (*found)++;
    if (verdict != BAND_SCAN_ADD) {
      continue;
    }
    const int slot = memoryFirstFree(m);
    if (slot == MEMORY_NO_SLOT) {
      continue;
    }
    MemoryChannel c;
    memset(&c, 0, sizeof(c));
    c.band = BAND_FM;
    c.freqKHz = kFmSweep[i].khz;
    TEST_ASSERT_TRUE(memorySet(m, slot, &c));
    (*added)++;
  }
}

static bool wasOnAir(uint32_t khz) {
  for (size_t i = 0; i < FM_SWEEP_STATION_COUNT; i++) {
    if (kFmSweepStations[i] == khz) {
      return true;
    }
  }
  return false;
}

static void a_scan_of_an_empty_store_adds_every_station(void) {
  MemoryStore m;
  memoryInit(&m);
  int found = 0;
  int added = 0;
  scanSweep(&m, &found, &added);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, found);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, added);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, memoryCount(&m));
  for (size_t i = 0; i < FM_SWEEP_STATION_COUNT; i++) {
    TEST_ASSERT_NOT_EQUAL(MEMORY_NO_SLOT,
                          memoryFind(&m, BAND_FM, kFmSweepStations[i]));
  }
}

static void a_station_already_stored_is_skipped_not_duplicated(void) {
  MemoryStore m;
  memoryInit(&m);
  MemoryChannel mine;
  memset(&mine, 0, sizeof(mine));
  mine.band = BAND_FM;
  mine.freqKHz = kFmSweepStations[0]; /* Stored by hand, ahead of the scan. */
  strcpy(mine.name, "My station");
  TEST_ASSERT_TRUE(memorySet(&m, 0, &mine));

  int found = 0;
  int added = 0;
  scanSweep(&m, &found, &added);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, found);
  /* One fewer added than found: the one already there was skipped. */
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT - 1, added);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, memoryCount(&m));

  /* And the slot it already held is untouched: still slot 0, still its
   * own name, not overwritten by the scan. */
  const MemoryChannel *still = memoryGet(&m, 0);
  TEST_ASSERT_NOT_NULL(still);
  TEST_ASSERT_EQUAL_UINT32(kFmSweepStations[0], still->freqKHz);
  TEST_ASSERT_EQUAL_STRING("My station", still->name);
}

static void a_dedupe_tolerance_of_zero_only_catches_the_exact_channel(void) {
  MemoryStore m;
  memoryInit(&m);
  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  c.band = BAND_FM;
  c.freqKHz = 106400;
  TEST_ASSERT_TRUE(memorySet(&m, 0, &c));

  TEST_ASSERT_NOT_EQUAL(MEMORY_NO_SLOT, memoryFindNear(&m, BAND_FM, 106400, 0));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryFindNear(&m, BAND_FM, 106500, 0));
}

static void a_full_store_stops_adding_but_keeps_scanning(void) {
  MemoryStore m;
  memoryInit(&m);
  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  c.band = BAND_MW; /* A band the sweep never touches, so nothing dedupes. */
  c.freqKHz = 1000;
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    TEST_ASSERT_TRUE(memorySet(&m, i, &c));
  }

  int found = 0;
  int added = 0;
  scanSweep(&m, &found, &added);
  TEST_ASSERT_EQUAL_INT(FM_SWEEP_STATION_COUNT, found);
  TEST_ASSERT_EQUAL_INT(0, added);
  TEST_ASSERT_EQUAL_INT(MEMORY_SLOT_COUNT, memoryCount(&m));
}

static void nothing_that_was_not_on_air_gets_added(void) {
  MemoryStore m;
  memoryInit(&m);
  int found = 0;
  int added = 0;
  scanSweep(&m, &found, &added);
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    const MemoryChannel *c = memoryGet(&m, i);
    if (c == NULL) {
      continue;
    }
    TEST_ASSERT_TRUE(wasOnAir(c->freqKHz));
  }
}

/* Counts the times the store is asked, to show it is asked only about a
 * station. */
static int sAsked = 0;
static int nothingStored(void *ctx, uint8_t band, uint32_t freqKHz,
                         uint32_t toleranceKHz) {
  (void)ctx;
  (void)band;
  (void)freqKHz;
  TEST_ASSERT_EQUAL_UINT32(100, toleranceKHz);
  sAsked++;
  return MEMORY_NO_SLOT;
}

static void the_store_is_asked_only_about_a_station(void) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  const BandScanWalk walk = fmWalk();
  sAsked = 0;
  size_t station = FM_SWEEP_COUNT;
  size_t quiet = FM_SWEEP_COUNT;
  for (size_t i = 0; i < FM_SWEEP_COUNT; i++) {
    SeekReading r = readingAt(i);
    if (seekShouldStop(&cfg, BAND_FM, &r)) {
      station = i;
    } else {
      quiet = i;
    }
  }
  TEST_ASSERT_TRUE(station < FM_SWEEP_COUNT && quiet < FM_SWEEP_COUNT);
  SeekReading r = readingAt(quiet);
  TEST_ASSERT_EQUAL(
      BAND_SCAN_NOT_A_STATION,
      bandScanJudge(&cfg, &walk, kFmSweep[quiet].khz, &r, nothingStored, NULL));
  TEST_ASSERT_EQUAL_INT(0, sAsked);
  r = readingAt(station);
  TEST_ASSERT_EQUAL(BAND_SCAN_ADD,
                    bandScanJudge(&cfg, &walk, kFmSweep[station].khz, &r,
                                  nothingStored, NULL));
  TEST_ASSERT_EQUAL_INT(1, sAsked);
}

/* A reading that did not arrive says nothing about the channel, and a
 * missing argument is never taken as a station. */
static void no_reading_or_no_argument_is_not_a_station(void) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  const BandScanWalk walk = fmWalk();
  size_t station = 0;
  for (size_t i = 0; i < FM_SWEEP_COUNT; i++) {
    SeekReading r = readingAt(i);
    if (seekShouldStop(&cfg, BAND_FM, &r)) {
      station = i;
      break;
    }
  }
  SeekReading r = readingAt(station);
  const uint32_t khz = kFmSweep[station].khz;
  sAsked = 0;
  TEST_ASSERT_EQUAL(BAND_SCAN_NOT_A_STATION,
                    bandScanJudge(NULL, &walk, khz, &r, nothingStored, NULL));
  TEST_ASSERT_EQUAL(BAND_SCAN_NOT_A_STATION,
                    bandScanJudge(&cfg, &walk, khz, NULL, nothingStored, NULL));
  TEST_ASSERT_EQUAL(BAND_SCAN_NOT_A_STATION,
                    bandScanJudge(&cfg, &walk, khz, &r, NULL, NULL));
  TEST_ASSERT_EQUAL(BAND_SCAN_NOT_A_STATION,
                    bandScanJudge(&cfg, NULL, khz, &r, nothingStored, NULL));
  r.valid = false;
  TEST_ASSERT_EQUAL(BAND_SCAN_NOT_A_STATION,
                    bandScanJudge(&cfg, &walk, khz, &r, nothingStored, NULL));
  TEST_ASSERT_EQUAL_INT(0, sAsked);
}

/* ----------------------------------------------------------------- walks */

/* The store with nothing in it, for the AM judge below. */
static int nothingStoredAm(void *ctx, uint8_t band, uint32_t freqKHz,
                           uint32_t toleranceKHz) {
  (void)ctx;
  (void)freqKHz;
  TEST_ASSERT_EQUAL_UINT8(BAND_MW, band);
  TEST_ASSERT_EQUAL_UINT32(4, toleranceKHz);
  return MEMORY_NO_SLOT;
}

static void each_band_walks_its_own_channel_spacing(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  plan.fmRegion = FM_REGION_87_108;
  BandScanWalk w;

  TEST_ASSERT_TRUE(bandScanWalkFor(BAND_FM, &plan, &w));
  TEST_ASSERT_EQUAL_UINT16(100, w.stepKHz);
  TEST_ASSERT_EQUAL_UINT32(100, w.toleranceKHz);
  TEST_ASSERT_EQUAL_UINT32(87000, w.firstKHz);
  TEST_ASSERT_EQUAL_UINT16(211, w.channels);
  TEST_ASSERT_EQUAL_UINT8(1, w.reads);

  TEST_ASSERT_TRUE(bandScanWalkFor(BAND_MW, &plan, &w));
  TEST_ASSERT_EQUAL_UINT16(9, w.stepKHz);
  TEST_ASSERT_EQUAL_UINT32(4, w.toleranceKHz);
  TEST_ASSERT_EQUAL_UINT32(522, w.firstKHz);
  TEST_ASSERT_EQUAL_UINT16(142, w.channels);
  TEST_ASSERT_EQUAL_UINT8(2, w.reads);
  TEST_ASSERT_EQUAL_UINT16(30, w.gapMs);

  plan.mwSpacing = MW_SPACING_10K;
  TEST_ASSERT_TRUE(bandScanWalkFor(BAND_MW, &plan, &w));
  TEST_ASSERT_EQUAL_UINT16(10, w.stepKHz);
  TEST_ASSERT_EQUAL_UINT32(5, w.toleranceKHz);
  TEST_ASSERT_EQUAL_UINT32(520, w.firstKHz);
  TEST_ASSERT_EQUAL_UINT16(121, w.channels);

  TEST_ASSERT_TRUE(bandScanWalkFor(BAND_LW, &plan, &w));
  TEST_ASSERT_EQUAL_UINT16(9, w.stepKHz);
  TEST_ASSERT_EQUAL_UINT32(4, w.toleranceKHz);
  TEST_ASSERT_EQUAL_UINT32(144, w.firstKHz);
  TEST_ASSERT_EQUAL_UINT16(42, w.channels);
}

/* SW walks the metre bands only, from the bottom of 160 m, and the gaps
 * between them are passed over. */
static void sw_walks_the_metre_bands_only(void) {
  BandScanWalk w;
  TEST_ASSERT_TRUE(bandScanWalkFor(BAND_SW, NULL, &w));
  TEST_ASSERT_EQUAL_UINT16(5, w.stepKHz);
  TEST_ASSERT_EQUAL_UINT32(2, w.toleranceKHz);
  TEST_ASSERT_EQUAL_UINT32(1800, w.firstKHz);
  TEST_ASSERT_EQUAL_UINT16(1000, w.channels);
  TEST_ASSERT_EQUAL_UINT32(1805, bandScanNext(&w, NULL, 1800));
  TEST_ASSERT_EQUAL_UINT32(2300, bandScanNext(&w, NULL, 2000));
  TEST_ASSERT_EQUAL_UINT32(9400, bandScanNext(&w, NULL, 7450));
}

/* Every walk visits each of its channels once and is back at the first
 * after the last, so a count of `channels` is the whole band. */
static void a_walk_covers_its_channels_once(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  static const BandId kBands[] = {BAND_LW, BAND_MW, BAND_SW, BAND_FM};
  for (size_t b = 0; b < sizeof(kBands) / sizeof(kBands[0]); b++) {
    BandScanWalk w;
    TEST_ASSERT_TRUE(bandScanWalkFor(kBands[b], &plan, &w));
    uint32_t khz = w.firstKHz;
    uint32_t last = 0;
    for (uint16_t i = 0; i < w.channels; i++) {
      TEST_ASSERT_TRUE(bandContains(kBands[b], &plan, khz));
      TEST_ASSERT_TRUE(i == 0 || khz > last);
      last = khz;
      khz = bandScanNext(&w, &plan, khz);
    }
    TEST_ASSERT_EQUAL_UINT32(w.firstKHz, khz);
  }
}

static void oirt_and_a_null_walk_are_refused(void) {
  BandScanWalk w;
  TEST_ASSERT_FALSE(bandScanWalkFor(BAND_OIRT, NULL, &w));
  TEST_ASSERT_FALSE(bandScanWalkFor(BAND_COUNT, NULL, &w));
  TEST_ASSERT_FALSE(bandScanWalkFor(BAND_FM, NULL, NULL));
  TEST_ASSERT_EQUAL_UINT32(1234, bandScanNext(NULL, NULL, 1234));
}

/* On AM a memory channel one channel away is another station, and one on
 * the same channel, off it by a kHz, is the same. */
static void am_counts_only_the_nearest_channel_as_stored(void) {
  MemoryStore m;
  memoryInit(&m);
  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  c.band = BAND_MW;
  c.freqKHz = 738;
  TEST_ASSERT_TRUE(memorySet(&m, 0, &c));
  BandScanWalk w;
  TEST_ASSERT_TRUE(bandScanWalkFor(BAND_MW, NULL, &w));
  SeekConfig cfg;
  seekDefaults(&cfg);
  /* A clean carrier on its channel: what the AM seek stops on. */
  SeekReading r;
  memset(&r, 0, sizeof(r));
  r.valid = true;
  r.levelTenths = 400;
  TEST_ASSERT_TRUE(seekShouldStop(&cfg, BAND_MW, &r));
  TEST_ASSERT_EQUAL(BAND_SCAN_STORED_NEAR,
                    bandScanJudge(&cfg, &w, 738, &r, findNearIn, &m));
  c.freqKHz = 739;
  TEST_ASSERT_TRUE(memorySet(&m, 0, &c));
  TEST_ASSERT_EQUAL(BAND_SCAN_STORED_NEAR,
                    bandScanJudge(&cfg, &w, 738, &r, findNearIn, &m));
  TEST_ASSERT_EQUAL(BAND_SCAN_ADD,
                    bandScanJudge(&cfg, &w, 747, &r, findNearIn, &m));
  TEST_ASSERT_EQUAL(BAND_SCAN_ADD,
                    bandScanJudge(&cfg, &w, 729, &r, findNearIn, &m));
}

/* Of the readings of one channel, the quietest valid one is judged; one
 * that did not arrive is never picked over one that did. */
static void the_quietest_reading_is_the_one_judged(void) {
  SeekReading r[BAND_SCAN_MAX_READS];
  memset(r, 0, sizeof(r));
  r[0].valid = true;
  r[0].noiseTenths = 156;
  r[1].valid = true;
  r[1].noiseTenths = 56;
  TEST_ASSERT_EQUAL_UINT16(56, bandScanQuietest(r, 2).noiseTenths);
  TEST_ASSERT_EQUAL_UINT16(156, bandScanQuietest(r, 1).noiseTenths);
  r[1].valid = false;
  r[1].noiseTenths = 0;
  TEST_ASSERT_EQUAL_UINT16(156, bandScanQuietest(r, 2).noiseTenths);
  r[0].valid = false;
  TEST_ASSERT_FALSE(bandScanQuietest(r, 2).valid);
  TEST_ASSERT_FALSE(bandScanQuietest(r, 0).valid);
  TEST_ASSERT_FALSE(bandScanQuietest(NULL, 2).valid);
}

/* 738 kHz read 156 once in 36 readings, over the limit of 120: that
 * reading alone turns the station away, and the quieter of two keeps it. */
static void a_loud_reading_of_an_am_station_does_not_lose_it(void) {
  BandScanWalk w;
  TEST_ASSERT_TRUE(bandScanWalkFor(BAND_MW, NULL, &w));
  SeekConfig cfg;
  seekDefaults(&cfg);
  SeekReading r[BAND_SCAN_MAX_READS];
  memset(r, 0, sizeof(r));
  r[0].valid = true;
  r[0].levelTenths = 486;
  r[0].noiseTenths = 156;
  r[1] = r[0];
  r[1].noiseTenths = 63;
  sAsked = 0;
  TEST_ASSERT_EQUAL(BAND_SCAN_NOT_A_STATION,
                    bandScanJudge(&cfg, &w, 738, &r[0], nothingStoredAm, NULL));
  const SeekReading quiet = bandScanQuietest(r, w.reads);
  TEST_ASSERT_EQUAL(BAND_SCAN_ADD, bandScanJudge(&cfg, &w, 738, &quiet,
                                                 nothingStoredAm, NULL));
}

/* An empty store, for the scan over the readings labelled by ear. */
static int noneStored(void *ctx, uint8_t band, uint32_t freqKHz,
                      uint32_t toleranceKHz) {
  (void)ctx;
  (void)band;
  (void)freqKHz;
  (void)toleranceKHz;
  return MEMORY_NO_SLOT;
}

/* The scan on AM judges the quieter of its first two readings: on real
 * channels labelled by ear it adds every one heard clearly and none heard as
 * noise alone. */
static void an_am_scan_adds_what_was_heard_and_not_noise(void) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  for (size_t i = 0; i < AM_HEARD_COUNT; i++) {
    const AmChannel *c = &kAmHeard[i];
    if (c->heard == AM_HEARD_WEAK) {
      continue;
    }
    BandId band = BAND_COUNT;
    TEST_ASSERT_TRUE(bandForFrequency(NULL, c->khz, &band));
    BandScanWalk w;
    TEST_ASSERT_TRUE(bandScanWalkFor(band, NULL, &w));
    SeekReading r[BAND_SCAN_MAX_READS];
    memset(r, 0, sizeof(r));
    for (uint8_t k = 0; k < w.reads; k++) {
      r[k].valid = true;
      r[k].levelTenths = c->levelTenths[k];
      r[k].noiseTenths = c->noiseTenths[k];
      r[k].offsetTenths = c->offsetTenths[k];
    }
    const SeekReading quiet = bandScanQuietest(r, w.reads);
    char why[32];
    snprintf(why, sizeof(why), "%u kHz", (unsigned)c->khz);
    TEST_ASSERT_EQUAL_MESSAGE(
        c->heard == AM_HEARD_CLEAR ? BAND_SCAN_ADD : BAND_SCAN_NOT_A_STATION,
        bandScanJudge(&cfg, &w, c->khz, &quiet, noneStored, NULL), why);
  }
}

/* ------------------------------------------------------------ the save */

static RadioSettings scanning(BandId band, uint32_t khz) {
  RadioSettings s;
  memset(&s, 0, sizeof(s));
  s.band = band;
  s.freqKHz = khz;
  return s;
}

/* A scan of the band the radio was on keeps its station, the rest as it
 * is. */
static void a_save_during_a_scan_keeps_the_station_it_started_from(void) {
  RadioSettings s = scanning(BAND_FM, 93100);
  s.stepKHz = 100;
  BandScanFrom from = {BAND_FM, 106400, TUNE_MODE_MEMORY, BAND_FM, 106400};
  bandScanKeep(&s, &from);
  TEST_ASSERT_EQUAL(BAND_FM, s.band);
  TEST_ASSERT_EQUAL_UINT32(106400, s.freqKHz);
  TEST_ASSERT_EQUAL_UINT16(100, s.stepKHz);
}

/* A scan of MW started on FM: a save keeps FM, its station, width, step
 * and tuning mode, and MW where its own dial was. */
static void a_save_during_a_scan_of_another_band_keeps_the_start_band(void) {
  RadioSettings s = scanning(BAND_MW, 1503);
  s.bandwidthKHz = 4;
  s.stepKHz = 9;
  s.tuneMode = TUNE_MODE_MANUAL;
  s.bandFreqKHz[BAND_FM] = 106400;
  s.bandBandwidthKHz[BAND_FM] = 0;
  s.bandStepKHz[BAND_FM] = 100;
  /* Turned to 106.4 in the moment before the band change, after the scan
   * read 106.2: the band change put 106.4 away, and that is kept. */
  BandScanFrom from = {BAND_FM, 106200, TUNE_MODE_MEMORY, BAND_MW, 738};
  bandScanKeep(&s, &from);
  TEST_ASSERT_EQUAL(BAND_FM, s.band);
  TEST_ASSERT_EQUAL_UINT32(106400, s.freqKHz);
  TEST_ASSERT_EQUAL_UINT16(0, s.bandwidthKHz);
  TEST_ASSERT_EQUAL_UINT16(100, s.stepKHz);
  TEST_ASSERT_EQUAL(TUNE_MODE_MEMORY, s.tuneMode);
  TEST_ASSERT_EQUAL_UINT32(738, s.bandFreqKHz[BAND_MW]);
  TEST_ASSERT_EQUAL_UINT16(4, s.bandBandwidthKHz[BAND_MW]);
  TEST_ASSERT_EQUAL_UINT16(9, s.bandStepKHz[BAND_MW]);
}

/* Moved off the scanned band, by a person, the dial is theirs. */
static void a_save_off_the_scanned_band_keeps_the_dial(void) {
  RadioSettings s = scanning(BAND_SW, 9420);
  BandScanFrom from = {BAND_FM, 106400, TUNE_MODE_MEMORY, BAND_MW, 738};
  bandScanKeep(&s, &from);
  TEST_ASSERT_EQUAL(BAND_SW, s.band);
  TEST_ASSERT_EQUAL_UINT32(9420, s.freqKHz);
  bandScanKeep(NULL, &from);
  bandScanKeep(&s, NULL);
  TEST_ASSERT_EQUAL_UINT32(9420, s.freqKHz);
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(a_scan_of_an_empty_store_adds_every_station);
  RUN_TEST(a_station_already_stored_is_skipped_not_duplicated);
  RUN_TEST(a_dedupe_tolerance_of_zero_only_catches_the_exact_channel);
  RUN_TEST(a_full_store_stops_adding_but_keeps_scanning);
  RUN_TEST(nothing_that_was_not_on_air_gets_added);
  RUN_TEST(the_store_is_asked_only_about_a_station);
  RUN_TEST(no_reading_or_no_argument_is_not_a_station);
  RUN_TEST(each_band_walks_its_own_channel_spacing);
  RUN_TEST(sw_walks_the_metre_bands_only);
  RUN_TEST(a_walk_covers_its_channels_once);
  RUN_TEST(oirt_and_a_null_walk_are_refused);
  RUN_TEST(am_counts_only_the_nearest_channel_as_stored);
  RUN_TEST(the_quietest_reading_is_the_one_judged);
  RUN_TEST(a_loud_reading_of_an_am_station_does_not_lose_it);
  RUN_TEST(an_am_scan_adds_what_was_heard_and_not_noise);
  RUN_TEST(a_save_during_a_scan_keeps_the_station_it_started_from);
  RUN_TEST(a_save_during_a_scan_of_another_band_keeps_the_start_band);
  RUN_TEST(a_save_off_the_scanned_band_keeps_the_dial);

  return UNITY_END();
}
