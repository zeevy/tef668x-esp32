/*
 * Tests for the FM DX rules. Runs on a PC.
 *
 * Most of these replay recorded DX readings: a sweep of the whole band; 20
 * seconds on five channels, four beside strong stations and 91.1 itself; a
 * minute on 93.4 and 93.6; 93.4, 93.5 and 93.6 at each of four filter widths;
 * and a second sweep of the band at the 114 kHz DX width. One more uses the
 * recorded squelch readings, six stations read sixty times each.
 */
#include <unity.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../test_squelch/shoulder.h"
#include "captures.h"
#include "core/dx.h"
#include "core/rds.h"
#include "core/seek.h"

void setUp(void) {}
void tearDown(void) {}

/*
 * The stations the radio receives, and nothing else.
 *
 * The nine FM stations this radio is known to receive, plus 95.0 MHz, which
 * shows in the seek sweep and the recorded RDS groups. Taken from those rather
 * than from the DX readings, so the DX readings are judged against something
 * they did not decide.
 */
static const uint32_t kOnAir[] = {91100, 92700,  93500,  94300,  95000,
                                  98300, 101900, 102800, 104000, 106400};
#define ON_AIR_COUNT (sizeof(kOnAir) / sizeof(kOnAir[0]))

static bool onAir(uint32_t khz) {
  for (size_t i = 0; i < ON_AIR_COUNT; i++) {
    if (kOnAir[i] == khz) {
      return true;
    }
  }
  return false;
}

/* One channel either side of a station, where its sidebands reach. */
static bool besideAStation(uint32_t khz) {
  return onAir(khz - 100) || onAir(khz + 100);
}

static SeekReading reading(const DxReading *r) {
  SeekReading out;
  memset(&out, 0, sizeof(out));
  out.valid = true;
  out.levelTenths = r->levelTenths;
  out.noiseTenths = r->noiseTenths;
  out.multipathTenths = r->multipathTenths;
  out.offsetTenths = r->offsetTenths;
  return out;
}

static SeekConfig atSensitivity(uint8_t sensitivity) {
  SeekConfig cfg;
  seekDefaults(&cfg);
  cfg.fmSensitivity = sensitivity;
  return cfg;
}

static Rds rds;

/* Every group heard on the channel, fed to the decoder in order. */
static const RdsInfo *replay(const DxChannel *c) {
  rdsReset(&rds, c->khz);
  for (uint16_t i = 0; i < c->groupCount; i++) {
    RdsRead r;
    memset(&r, 0, sizeof(r));
    r.synchronised = true;
    r.haveGroup = true;
    for (int b = 0; b < 4; b++) {
      r.block[b] = c->groups[i].block[b];
      r.error[b] = (uint8_t)((c->groups[i].error >> (6 - b * 2)) & 0x03);
    }
    rdsFeed(&rds, &r);
  }
  return &rds.info;
}

static const DxChannel *find(const DxChannel *list, size_t count,
                             uint32_t khz) {
  for (size_t i = 0; i < count; i++) {
    if (list[i].khz == khz) {
      return &list[i];
    }
  }
  TEST_FAIL_MESSAGE("that channel is not in the capture");
  return NULL;
}

/* ------------------------------------------------- is there a signal here -- */

static void stopsOnlyOnStationsAt(uint8_t sensitivity) {
  SeekConfig cfg = atSensitivity(sensitivity);
  size_t stops = 0;
  for (size_t i = 0; i < DX_SWEEP_COUNT; i++) {
    SeekReading r = reading(&kDxSweep[i].tuned);
    if (seekShouldStop(&cfg, BAND_FM, &r)) {
      TEST_ASSERT_TRUE_MESSAGE(onAir(kDxSweep[i].khz),
                               "the seek stopped on something not on air");
      stops++;
    }
  }
  TEST_ASSERT_EQUAL_MESSAGE(ON_AIR_COUNT, stops,
                            "the seek missed a station that was on air");
}

static void the_seek_finds_every_station_and_nothing_else_by_default(void) {
  stopsOnlyOnStationsAt(SEEK_SENSITIVITY_DEFAULT);
}

static void the_loosest_seek_still_stops_on_nothing_but_stations(void) {
  stopsOnlyOnStationsAt(SEEK_SENSITIVITY_MAX);
}

/* The quietest of the 176 empty channels read a noise of 253, and the
 * loosest limit is 180, so even the loosest seek has room to spare. */
static void the_noise_floor_sits_above_the_loosest_limit(void) {
  uint16_t quietest = UINT16_MAX;
  for (size_t i = 0; i < DX_SWEEP_COUNT; i++) {
    const DxChannel *c = &kDxSweep[i];
    if (!onAir(c->khz) && !besideAStation(c->khz) &&
        c->tuned.noiseTenths < quietest) {
      quietest = c->tuned.noiseTenths;
    }
  }
  TEST_ASSERT_EQUAL(253, quietest);
  TEST_ASSERT_LESS_THAN(quietest, seekNoiseLimit(SEEK_SENSITIVITY_MAX));
}

/* ------------------------------------------------------ is this PI real -- */

/* The decoder's own rule, two clean receptions, on real stations. 91.1 sends
 * no RDS in the sweep, so it is taken from the 20 second readings. 94.3 and
 * 95.0 send 0000, which is not an identifier. */
static void every_station_sending_a_pi_has_it_confirmed(void) {
  const struct {
    const DxChannel *channel;
    uint16_t pi;
  } stations[] = {
      {find(kDxSweep, DX_SWEEP_COUNT, 93500), 0x0935},
      {find(kDxSweep, DX_SWEEP_COUNT, 98300), 0x26FF},
      {find(kDxSweep, DX_SWEEP_COUNT, 106400), 0x1064},
      {find(kDxTwenty, DX_TWENTY_COUNT, 91100), 0x3712},
  };
  for (size_t i = 0; i < sizeof(stations) / sizeof(stations[0]); i++) {
    const RdsInfo *info = replay(stations[i].channel);
    SeekReading r = reading(&stations[i].channel->tuned);
    TEST_ASSERT_TRUE(info->hasPi);
    TEST_ASSERT_EQUAL_HEX16(stations[i].pi, info->pi);
    TEST_ASSERT_TRUE(dxPiConfirmed(info, &r));
  }
}

/* Noise does produce a clean block A now and then, which is why one is not
 * enough. It never produced two alike. */
static void noise_gives_a_clean_block_a_but_never_a_pi(void) {
  size_t cleanOnNoise = 0;
  for (size_t i = 0; i < DX_SWEEP_COUNT; i++) {
    const DxChannel *c = &kDxSweep[i];
    if (onAir(c->khz) || besideAStation(c->khz)) {
      continue;
    }
    for (uint16_t g = 0; g < c->groupCount; g++) {
      if ((c->groups[g].error >> 6) == 0) {
        cleanOnNoise++;
      }
    }
    TEST_ASSERT_FALSE(replay(c)->hasPi);
  }
  TEST_ASSERT_EQUAL(1, cleanOnNoise);
}

/*
 * Why the decoder uses only a clean block A.
 *
 * Over every group heard on a station or beside one, a clean block A always
 * carried the PI that station sends. A corrected one carried a different PI
 * more often than the right one.
 */
static void only_a_clean_block_a_carries_the_right_pi(void) {
  const DxChannel *lists[] = {kDxSweep, kDxTwenty, kDxMinute};
  const size_t counts[] = {DX_SWEEP_COUNT, DX_TWENTY_COUNT, DX_MINUTE_COUNT};
  size_t cleanRight = 0;
  size_t cleanWrong = 0;
  size_t fixedRight = 0;
  size_t fixedWrong = 0;
  for (size_t l = 0; l < 3; l++) {
    for (size_t i = 0; i < counts[l]; i++) {
      const DxChannel *c = &lists[l][i];
      const RdsInfo *info = replay(c);
      if (!info->hasPi) {
        continue;
      }
      for (uint16_t g = 0; g < c->groupCount; g++) {
        bool right = c->groups[g].block[0] == info->pi;
        uint8_t level = (uint8_t)(c->groups[g].error >> 6);
        size_t *count = NULL;
        if (level == 0) {
          count = right ? &cleanRight : &cleanWrong;
        } else if (level < 3) {
          count = right ? &fixedRight : &fixedWrong;
        }
        if (count != NULL) {
          (*count)++;
        }
      }
    }
  }
  TEST_ASSERT_GREATER_THAN(300, cleanRight);
  TEST_ASSERT_EQUAL(0, cleanWrong);
  TEST_ASSERT_GREATER_THAN(fixedRight, fixedWrong);
}

/* ------------------------------------------ is this PI this channel's own -- */

static const struct {
  uint32_t khz;
  uint16_t neighbours;
} kShoulders[] = {
    {93400, 0x0935},
    {93600, 0x0935},
    {98200, 0x26FF},
    {98400, 0x26FF},
};
#define NEIGHBOUR_COUNT (sizeof(kShoulders) / sizeof(kShoulders[0]))

/* The fault this module exists for: the decoder is right that it heard the
 * PI, and the station is 100 kHz away. */
static void the_neighbours_pi_is_confirmed_beside_it(void) {
  for (size_t i = 0; i < NEIGHBOUR_COUNT; i++) {
    const RdsInfo *info =
        replay(find(kDxTwenty, DX_TWENTY_COUNT, kShoulders[i].khz));
    TEST_ASSERT_TRUE(info->hasPi);
    TEST_ASSERT_EQUAL_HEX16(kShoulders[i].neighbours, info->pi);
  }
}

static void the_neighbours_pi_is_refused_beside_it(void) {
  for (size_t i = 0; i < NEIGHBOUR_COUNT; i++) {
    const DxChannel *c = find(kDxTwenty, DX_TWENTY_COUNT, kShoulders[i].khz);
    SeekReading r = reading(&c->tuned);
    TEST_ASSERT_FALSE(dxPiConfirmed(replay(c), &r));
  }
}

/* Every reading over a minute, not only the first, and the offset kept the
 * neighbour out on all 69 while noise, multipath and level let it in on 27. */
static void the_offset_refused_the_neighbour_for_a_whole_minute(void) {
  SeekConfig cfg = atSensitivity(SEEK_SENSITIVITY_DEFAULT);
  size_t readings = 0;
  size_t passedTheRest = 0;
  for (size_t i = 0; i < DX_MINUTE_COUNT; i++) {
    const DxChannel *c = &kDxMinute[i];
    const RdsInfo *info = replay(c);
    TEST_ASSERT_TRUE(info->hasPi);
    TEST_ASSERT_EQUAL_HEX16(0x0935, info->pi);
    for (uint16_t q = 0; q <= c->readingCount; q++) {
      SeekReading r = reading(q == 0 ? &c->tuned : &c->readings[q - 1]);
      TEST_ASSERT_FALSE(dxPiConfirmed(info, &r));
      r.offsetTenths = 0;
      if (seekShouldStop(&cfg, BAND_FM, &r)) {
        passedTheRest++;
      }
      readings++;
    }
  }
  TEST_ASSERT_EQUAL(69, readings);
  TEST_ASSERT_EQUAL(27, passedTheRest);
}

/* The other side of the rule: a real station must not be refused as its
 * offset drifts. In the squelch capture, six stations read 60 times each
 * stayed between -5.3 and +12.3 kHz, inside the window on every reading. */
static void a_station_stays_inside_the_window_as_it_drifts(void) {
  RdsInfo info;
  memset(&info, 0, sizeof(info));
  info.hasPi = true;
  size_t readings = 0;
  for (size_t i = 0; i < SHOULDER_COUNT; i++) {
    const ShoulderRow *row = &kShoulderCapture[i];
    if (!onAir(row->khz)) {
      continue;
    }
    SeekReading r;
    memset(&r, 0, sizeof(r));
    r.valid = true;
    r.offsetTenths = row->offsetTenths;
    TEST_ASSERT_TRUE(dxPiConfirmed(&info, &r));
    readings++;
  }
  TEST_ASSERT_EQUAL(360, readings);
}

/* ------------------------------------------------------------- the edges -- */

static RdsInfo withPi(void) {
  RdsInfo info;
  memset(&info, 0, sizeof(info));
  info.hasPi = true;
  info.pi = 0x0935;
  return info;
}

static SeekReading atOffset(int16_t tenths) {
  SeekReading r;
  memset(&r, 0, sizeof(r));
  r.valid = true;
  r.offsetTenths = tenths;
  return r;
}

static void the_window_is_the_seeks_own(void) {
  RdsInfo info = withPi();
  const int16_t w = SEEK_OFFSET_FM_TENTHS;
  SeekReading r = atOffset(w - 1);
  TEST_ASSERT_TRUE(dxPiConfirmed(&info, &r));
  r = atOffset((int16_t)(-w + 1));
  TEST_ASSERT_TRUE(dxPiConfirmed(&info, &r));
  r = atOffset(w);
  TEST_ASSERT_FALSE(dxPiConfirmed(&info, &r));
  r = atOffset((int16_t)-w);
  TEST_ASSERT_FALSE(dxPiConfirmed(&info, &r));
}

/* ------------------------------------------------- who the station is */

static RdsInfo withPiAndEcc(void) {
  RdsInfo info = withPi();
  info.pi = 0x5201; /* India's country nibble. */
  info.hasEcc = true;
  info.ecc = 0xF2;
  return info;
}

/* Off the channel's own carrier the DX page names nobody, where the RDS
 * screen names the country it heard. */
static void the_dx_page_rule_names_only_a_station_on_its_own_carrier(void) {
  RdsInfo info = withPiAndEcc();
  SeekReading off = atOffset(SEEK_OFFSET_FM_TENTHS);
  SeekReading on = atOffset(30);
  DxIdentity page;
  DxIdentity screen;
  dxStationIdentity(&info, &off, RDS_REGION_EUROPE, 0, true, &page);
  dxStationIdentity(&info, &off, RDS_REGION_EUROPE, 0, false, &screen);
  TEST_ASSERT_NULL(page.country);
  TEST_ASSERT_NOT_NULL(screen.country);
  TEST_ASSERT_EQUAL_STRING(rdsCountryCode(0x5201, 0xF2), screen.country);
  dxStationIdentity(&info, &on, RDS_REGION_EUROPE, 0, true, &page);
  TEST_ASSERT_EQUAL_STRING(rdsCountryCode(0x5201, 0xF2), page.country);
  TEST_ASSERT_EQUAL(RDS_CALL_NONE, page.call);
}

/* The country is the ECC and the PI together. */
static void no_country_without_both_the_ecc_and_the_pi(void) {
  RdsInfo info = withPiAndEcc();
  info.hasEcc = false;
  SeekReading on = atOffset(0);
  DxIdentity id;
  dxStationIdentity(&info, &on, RDS_REGION_EUROPE, 0, false, &id);
  TEST_ASSERT_NULL(id.country);
  info = withPiAndEcc();
  info.hasPi = false;
  dxStationIdentity(&info, &on, RDS_REGION_EUROPE, 0, false, &id);
  TEST_ASSERT_NULL(id.country);
}

/* The preset's own station, another one, or no preset to ask about. */
static void the_preset_match_comes_with_the_identity(void) {
  RdsInfo info = withPi();
  SeekReading on = atOffset(0);
  DxIdentity id;
  dxStationIdentity(&info, &on, RDS_REGION_EUROPE, 0x0935, true, &id);
  TEST_ASSERT_EQUAL(DX_PRESET_MATCH, id.preset);
  dxStationIdentity(&info, &on, RDS_REGION_EUROPE, 0x1064, true, &id);
  TEST_ASSERT_EQUAL(DX_PRESET_OTHER, id.preset);
  dxStationIdentity(&info, &on, RDS_REGION_EUROPE, 0, true, &id);
  TEST_ASSERT_EQUAL(DX_PRESET_NONE, id.preset);
}

static void no_rds_is_nobody(void) {
  SeekReading on = atOffset(0);
  DxIdentity id;
  memset(&id, 0xAA, sizeof(id));
  dxStationIdentity(NULL, &on, RDS_REGION_EUROPE, 0x0935, false, &id);
  TEST_ASSERT_NULL(id.country);
  TEST_ASSERT_EQUAL(RDS_CALL_NONE, id.call);
  TEST_ASSERT_EQUAL(DX_PRESET_NONE, id.preset);
  dxStationIdentity(NULL, &on, RDS_REGION_EUROPE, 0, false, NULL);
}

/* ------------------------------------------------ a log entry by hand */

static RdsInfo named(const char *ps, bool pi) {
  RdsInfo info = withPi();
  info.hasPi = pi;
  info.hasPs = true;
  snprintf(info.ps, sizeof(info.ps), "%s", ps);
  return info;
}

/* On the channel's own carrier, the entry keeps both the name and the PI. */
static void a_hand_entry_on_channel_keeps_the_name_and_the_pi(void) {
  RdsInfo info = named(" MAGIC  ", true);
  SeekReading r = atOffset(60);
  LogbookEntry e;
  memset(&e, 0, sizeof(e));
  dxLogIdentity(&e, &info, &r);
  TEST_ASSERT_TRUE(e.hasName);
  TEST_ASSERT_EQUAL_STRING(" MAGIC  ", e.name);
  TEST_ASSERT_TRUE(e.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x0935, e.pi);
}

/* Beside a strong station the decoder spells its name and confirms its PI;
 * off the window, the entry keeps neither, so the station is not logged
 * 100 kHz from where it is. */
static void a_hand_entry_beside_a_strong_station_keeps_neither(void) {
  RdsInfo info = named(" MAGIC  ", true);
  SeekReading r = atOffset(SEEK_OFFSET_FM_TENTHS);
  LogbookEntry e;
  memset(&e, 0xAA, sizeof(e));
  dxLogIdentity(&e, &info, &r);
  TEST_ASSERT_FALSE(e.hasName);
  TEST_ASSERT_EQUAL_STRING("", e.name);
  TEST_ASSERT_FALSE(e.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0, e.pi);
}

/* The name needs no PI, so a station that sends PI 0000, or whose PI has
 * not been made sure of yet, still keeps its name. */
static void a_hand_entry_keeps_the_name_without_a_pi(void) {
  RdsInfo info = named("FEVER   ", false);
  SeekReading r = atOffset(-40);
  LogbookEntry e;
  memset(&e, 0, sizeof(e));
  dxLogIdentity(&e, &info, &r);
  TEST_ASSERT_TRUE(e.hasName);
  TEST_ASSERT_EQUAL_STRING("FEVER   ", e.name);
  TEST_ASSERT_FALSE(e.hasPi);
}

static void a_hand_entry_with_nothing_to_go_on_keeps_nothing(void) {
  RdsInfo info = named(" MAGIC  ", true);
  SeekReading r = atOffset(0);
  LogbookEntry e;
  memset(&e, 0xAA, sizeof(e));
  dxLogIdentity(&e, NULL, &r);
  TEST_ASSERT_FALSE(e.hasName);
  TEST_ASSERT_FALSE(e.hasPi);
  dxLogIdentity(&e, &info, NULL);
  TEST_ASSERT_FALSE(e.hasName);
  TEST_ASSERT_FALSE(e.hasPi);
  r.valid = false;
  dxLogIdentity(&e, &info, &r);
  TEST_ASSERT_FALSE(e.hasName);
  info.hasPs = false;
  r.valid = true;
  dxLogIdentity(&e, &info, &r);
  TEST_ASSERT_FALSE(e.hasName);
  TEST_ASSERT_TRUE(e.hasPi);
  dxLogIdentity(NULL, &info, &r); /* Must not crash. */
}

/* With no stored PI there is nothing to say, whatever is heard. */
static void a_preset_with_no_pi_says_nothing(void) {
  RdsInfo info = withPi();
  SeekReading r = atOffset(0);
  TEST_ASSERT_EQUAL_INT(DX_PRESET_NONE, dxPresetPi(&info, &r, 0));
  TEST_ASSERT_EQUAL_INT(DX_PRESET_NONE, dxPresetPi(NULL, &r, 0x0935));
}

/* One clean block A with the stored PI, on the channel, is the preset's
 * station before the PI is confirmed; off the channel it is not. */
static void one_matching_block_is_the_presets_station(void) {
  RdsInfo info;
  memset(&info, 0, sizeof(info));
  info.hasPiHeard = true;
  info.piHeard = 0x0935;
  SeekReading r = atOffset(0);
  TEST_ASSERT_EQUAL_INT(DX_PRESET_MATCH, dxPresetPi(&info, &r, 0x0935));
  TEST_ASSERT_EQUAL_INT(DX_PRESET_NONE, dxPresetPi(&info, &r, 0x26FF));
  r = atOffset(SEEK_OFFSET_FM_TENTHS);
  TEST_ASSERT_EQUAL_INT(DX_PRESET_NONE, dxPresetPi(&info, &r, 0x0935));
}

/* A confirmed PI on the channel is the preset's station or another one;
 * an unconfirmed other PI says nothing, so one misread block cannot name
 * another station. */
static void only_a_confirmed_other_pi_is_another_station(void) {
  RdsInfo info = withPi();
  SeekReading r = atOffset(0);
  TEST_ASSERT_EQUAL_INT(DX_PRESET_MATCH, dxPresetPi(&info, &r, 0x0935));
  TEST_ASSERT_EQUAL_INT(DX_PRESET_OTHER, dxPresetPi(&info, &r, 0x26FF));
  info.hasPi = false;
  info.hasPiHeard = true;
  info.piHeard = 0x1111;
  TEST_ASSERT_EQUAL_INT(DX_PRESET_NONE, dxPresetPi(&info, &r, 0x26FF));
}

/* A preset with no PI learns a confirmed one, on the channel only; a
 * stored PI is never replaced. */
static void a_preset_learns_only_when_it_has_none(void) {
  RdsInfo info = withPi();
  SeekReading r = atOffset(0);
  uint16_t pi = 0;
  TEST_ASSERT_TRUE(dxPresetLearn(&info, &r, 0, &pi));
  TEST_ASSERT_EQUAL_HEX16(0x0935, pi);
  pi = 0;
  TEST_ASSERT_FALSE(dxPresetLearn(&info, &r, 0x26FF, &pi));
  TEST_ASSERT_EQUAL_HEX16(0, pi);
  r = atOffset(SEEK_OFFSET_FM_TENTHS);
  TEST_ASSERT_FALSE(dxPresetLearn(&info, &r, 0, &pi));
  info.hasPi = false;
  r = atOffset(0);
  TEST_ASSERT_FALSE(dxPresetLearn(&info, &r, 0, &pi));
  TEST_ASSERT_FALSE(dxPresetLearn(&info, &r, 0, NULL));
}

static void nothing_confirmed_means_no(void) {
  RdsInfo info = withPi();
  SeekReading r = atOffset(0);
  TEST_ASSERT_FALSE(dxPiConfirmed(NULL, &r));
  TEST_ASSERT_FALSE(dxPiConfirmed(&info, NULL));
  r.valid = false;
  TEST_ASSERT_FALSE(dxPiConfirmed(&info, &r));
  r.valid = true;
  info.hasPi = false;
  TEST_ASSERT_FALSE(dxPiConfirmed(&info, &r));
}

/* The name has no PI to test, so it takes the offset half alone: beside a
 * strong station the decoder spells that station's name too. */
static void the_channel_test_is_the_window_alone(void) {
  SeekReading r = atOffset((int16_t)(SEEK_OFFSET_FM_TENTHS - 1));
  TEST_ASSERT_TRUE(dxOnChannel(&r));
  r = atOffset(SEEK_OFFSET_FM_TENTHS);
  TEST_ASSERT_FALSE(dxOnChannel(&r));
  r = atOffset(0);
  r.valid = false;
  TEST_ASSERT_FALSE(dxOnChannel(&r));
  TEST_ASSERT_FALSE(dxOnChannel(NULL));
}

static RdsInfo hearingNow(void) {
  RdsInfo info = withPi();
  info.hasBlockErrors = true;
  info.hasPiHeard = true;
  info.piHeard = info.pi;
  return info;
}

/* A confirmed PI stays until the dial moves. It is heard now only while a
 * clean block A carrying it is still arriving. */
static void a_pi_is_heard_now_only_on_a_clean_block_a(void) {
  SeekReading r = atOffset(50);
  RdsInfo info = hearingNow();
  TEST_ASSERT_TRUE(dxPiHeardNow(&info, &r));
  info.blockError[0] = 1;
  TEST_ASSERT_FALSE(dxPiHeardNow(&info, &r));
  info = hearingNow();
  info.hasBlockErrors = false; /* The lock is lost. */
  TEST_ASSERT_FALSE(dxPiHeardNow(&info, &r));
  info = hearingNow();
  info.piHeard = 0x1064; /* Another station's block A. */
  TEST_ASSERT_FALSE(dxPiHeardNow(&info, &r));
  info = hearingNow();
  info.hasPiHeard = false;
  TEST_ASSERT_FALSE(dxPiHeardNow(&info, &r));
  info = hearingNow();
  r = atOffset(SEEK_OFFSET_FM_TENTHS);
  TEST_ASSERT_FALSE(dxPiHeardNow(&info, &r));
  TEST_ASSERT_FALSE(dxPiHeardNow(NULL, &r));
}

/* ------------------------------------------------- the log's radio text */

static RdsInfo withText(void) {
  RdsInfo info = withPi();
  info.hasRt = true;
  snprintf(info.rt, sizeof(info.rt), "Now playing");
  return info;
}

static LogbookEntry entryAt(uint32_t khz, bool hasPi, uint16_t pi) {
  LogbookEntry e;
  memset(&e, 0, sizeof(e));
  e.freqKHz = khz;
  e.hasPi = hasPi;
  e.pi = pi;
  return e;
}

static void the_text_goes_in_when_it_is_the_entrys_station(void) {
  RdsInfo info = withText();
  SeekReading r = atOffset(0);
  LogbookEntry e = entryAt(93500, true, 0x0935);
  dxLogRadioText(&e, &info, &r, 93500);
  TEST_ASSERT_TRUE(e.hasRt);
  TEST_ASSERT_EQUAL_STRING("Now playing", e.rt);
}

/* A station that sends PI 0000 has a name logged on the offset alone, and
 * its text goes the same way. */
static void an_entry_with_no_pi_takes_the_offset_alone(void) {
  RdsInfo info = withText();
  info.hasPi = false;
  SeekReading r = atOffset(0);
  LogbookEntry e = entryAt(93500, false, 0);
  dxLogRadioText(&e, &info, &r, 93500);
  TEST_ASSERT_TRUE(e.hasRt);
}

static void no_whole_text_means_none(void) {
  RdsInfo info = withText();
  info.hasRt = false;
  SeekReading r = atOffset(0);
  LogbookEntry e = entryAt(93500, true, 0x0935);
  dxLogRadioText(&e, &info, &r, 93500);
  TEST_ASSERT_FALSE(e.hasRt);
  TEST_ASSERT_EQUAL_STRING("", e.rt);
}

/* A catch logged from the list while the dial is on another channel: the
 * text on the air is that channel's station's. */
static void another_channel_on_the_dial_gives_none(void) {
  RdsInfo info = withText();
  SeekReading r = atOffset(0);
  LogbookEntry e = entryAt(93500, true, 0x0935);
  dxLogRadioText(&e, &info, &r, 93600);
  TEST_ASSERT_FALSE(e.hasRt);
}

static void another_pi_gives_none(void) {
  RdsInfo info = withText();
  SeekReading r = atOffset(0);
  LogbookEntry e = entryAt(93500, true, 0x1064);
  dxLogRadioText(&e, &info, &r, 93500);
  TEST_ASSERT_FALSE(e.hasRt);
  info.hasPi = false;
  e = entryAt(93500, true, 0x0935);
  dxLogRadioText(&e, &info, &r, 93500);
  TEST_ASSERT_FALSE(e.hasRt);
}

/* The readings beside a strong station: the decoder there hears the strong
 * one, and its offset says so. */
static void beside_a_strong_station_gives_none(void) {
  RdsInfo info = withText();
  for (size_t i = 0; i < NEIGHBOUR_COUNT; i++) {
    const DxChannel *c = find(kDxTwenty, DX_TWENTY_COUNT, kShoulders[i].khz);
    SeekReading r = reading(&c->tuned);
    info.pi = kShoulders[i].neighbours;
    LogbookEntry e = entryAt(kShoulders[i].khz, false, 0);
    dxLogRadioText(&e, &info, &r, kShoulders[i].khz);
    TEST_ASSERT_FALSE(e.hasRt);
  }
}

/* What was in the entry before is not left behind when this one has none. */
static void a_refused_text_clears_the_entrys(void) {
  RdsInfo info = withText();
  SeekReading r = atOffset(SEEK_OFFSET_FM_TENTHS);
  LogbookEntry e = entryAt(93500, true, 0x0935);
  e.hasRt = true;
  snprintf(e.rt, sizeof(e.rt), "old");
  dxLogRadioText(&e, &info, &r, 93500);
  TEST_ASSERT_FALSE(e.hasRt);
  TEST_ASSERT_EQUAL_STRING("", e.rt);
  dxLogRadioText(&e, NULL, &r, 93500);
  TEST_ASSERT_FALSE(e.hasRt);
  dxLogRadioText(NULL, &info, &r, 93500);
}

/* ----------------------------------------------------------- the page -- */

static SeekReading centred(void) {
  return atOffset(50);
}

static void the_tile_is_empty_before_anything_is_heard(void) {
  RdsInfo info;
  memset(&info, 0, sizeof(info));
  SeekReading r = centred();
  TEST_ASSERT_EQUAL(DX_PI_NONE, dxPiTile(&info, &r));
  TEST_ASSERT_EQUAL(DX_PI_NONE, dxPiTile(NULL, &r));
  char digits[5] = "x";
  dxPiDigits(&info, DX_PI_NONE, digits);
  TEST_ASSERT_EQUAL_STRING("", digits);
}

static void the_tile_follows_the_decoder(void) {
  RdsInfo info;
  memset(&info, 0, sizeof(info));
  SeekReading r = centred();
  char digits[5];

  info.hasPiHeard = true;
  info.piHeard = 0x53A1;
  TEST_ASSERT_EQUAL(DX_PI_SEEN, dxPiTile(&info, &r));
  dxPiDigits(&info, DX_PI_SEEN, digits);
  TEST_ASSERT_EQUAL_STRING("53A1", digits);

  info.piUnsureNibbles = 0x5;
  TEST_ASSERT_EQUAL(DX_PI_PARTIAL, dxPiTile(&info, &r));
  dxPiDigits(&info, DX_PI_PARTIAL, digits);
  TEST_ASSERT_EQUAL_STRING("5?A?", digits);

  info.hasPi = true;
  info.pi = 0x53A1;
  TEST_ASSERT_EQUAL(DX_PI_CONFIRMED, dxPiTile(&info, &r));
  dxPiDigits(&info, DX_PI_CONFIRMED, digits);
  TEST_ASSERT_EQUAL_STRING("53A1", digits);
}

static void a_station_sending_zero_gets_its_own_tile(void) {
  RdsInfo info;
  memset(&info, 0, sizeof(info));
  info.piZero = true;
  info.hasPiHeard = true;
  SeekReading r = centred();
  TEST_ASSERT_EQUAL(DX_PI_ZERO, dxPiTile(&info, &r));
  char digits[5];
  dxPiDigits(&info, DX_PI_ZERO, digits);
  TEST_ASSERT_EQUAL_STRING("0000", digits);
}

/* The shoulder's confirmed PI never turns the tile amber. */
static void the_neighbours_pi_is_only_seen_on_the_tile(void) {
  const DxChannel *c = find(kDxTwenty, DX_TWENTY_COUNT, 93600);
  const RdsInfo *info = replay(c);
  SeekReading r = reading(&c->tuned);
  TEST_ASSERT_TRUE(info->hasPi);
  TEST_ASSERT_EQUAL(DX_PI_SEEN, dxPiTile(info, &r));
}

static void nowhere_to_put_the_digits_is_safe(void) {
  RdsInfo info = withPi();
  dxPiDigits(&info, DX_PI_CONFIRMED, NULL);
  char digits[5] = "x";
  dxPiDigits(NULL, DX_PI_CONFIRMED, digits);
  TEST_ASSERT_EQUAL_STRING("", digits);
}

static void the_block_meter_counts_down_with_the_errors(void) {
  TEST_ASSERT_EQUAL_UINT8(4, dxBlockSegments(0));
  TEST_ASSERT_EQUAL_UINT8(3, dxBlockSegments(1));
  TEST_ASSERT_EQUAL_UINT8(2, dxBlockSegments(2));
  TEST_ASSERT_EQUAL_UINT8(1, dxBlockSegments(3));
  TEST_ASSERT_EQUAL_UINT8(0, dxBlockSegments(4));
  TEST_ASSERT_EQUAL_UINT8(0, dxBlockSegments(7));
  /* No group has arrived yet. */
  TEST_ASSERT_EQUAL_UINT8(0, dxBlockSegments(-1));
}

static DxHistory history;

static void an_empty_history_has_no_bars(void) {
  dxHistoryReset(&history);
  int16_t level = 0;
  TEST_ASSERT_FALSE(
      dxHistoryBar(&history, 5000, DX_HISTORY_SECONDS - 1, &level));
  TEST_ASSERT_FALSE(dxHistoryBar(NULL, 5000, 0, &level));
}

static void each_second_keeps_its_highest_reading(void) {
  dxHistoryReset(&history);
  dxHistoryAdd(&history, 10100, 120);
  dxHistoryAdd(&history, 10500, 310);
  dxHistoryAdd(&history, 10900, 150);
  int16_t level = 0;
  TEST_ASSERT_TRUE(
      dxHistoryBar(&history, 10950, DX_HISTORY_SECONDS - 1, &level));
  TEST_ASSERT_EQUAL_INT16(310, level);
}

static void a_second_with_no_reading_has_no_bar(void) {
  dxHistoryReset(&history);
  dxHistoryAdd(&history, 10000, 200);
  dxHistoryAdd(&history, 12000, 250);
  int16_t level = 0;
  const uint8_t now = DX_HISTORY_SECONDS - 1;
  TEST_ASSERT_TRUE(dxHistoryBar(&history, 12000, now, &level));
  TEST_ASSERT_EQUAL_INT16(250, level);
  TEST_ASSERT_FALSE(dxHistoryBar(&history, 12000, now - 1, &level));
  TEST_ASSERT_TRUE(dxHistoryBar(&history, 12000, now - 2, &level));
  TEST_ASSERT_EQUAL_INT16(200, level);

  /* Later, with nothing added, the bars move left and the new seconds on
   * the right have none. */
  TEST_ASSERT_FALSE(dxHistoryBar(&history, 14000, now, &level));
  TEST_ASSERT_TRUE(dxHistoryBar(&history, 14000, now - 2, &level));
  TEST_ASSERT_EQUAL_INT16(250, level);
}

static void a_minute_later_the_history_has_rolled_off(void) {
  dxHistoryReset(&history);
  dxHistoryAdd(&history, 10000, 200);
  int16_t level = 0;
  TEST_ASSERT_TRUE(dxHistoryBar(&history, 69000, 0, &level));
  TEST_ASSERT_FALSE(dxHistoryBar(&history, 70000, 0, &level));
  dxHistoryAdd(&history, 200000, 300);
  for (uint8_t bar = 0; bar < DX_HISTORY_SECONDS - 1; bar++) {
    TEST_ASSERT_FALSE(dxHistoryBar(&history, 200000, bar, &level));
  }
}

static void a_bar_out_of_range_is_refused(void) {
  dxHistoryReset(&history);
  dxHistoryAdd(&history, 10000, 200);
  TEST_ASSERT_FALSE(dxHistoryBar(&history, 10000, DX_HISTORY_SECONDS, NULL));
  TEST_ASSERT_TRUE(dxHistoryBar(&history, 10000, DX_HISTORY_SECONDS - 1, NULL));
  dxHistoryAdd(NULL, 10000, 200);
  dxHistoryReset(NULL);
}

/* The minute on 93.6, a reading every second and a half or so: every
 * reading lands in its own second, and the gaps between them stay empty. */
static void the_minute_capture_fills_the_history_with_gaps(void) {
  const DxChannel *c = &kDxMinute[1];
  dxHistoryReset(&history);
  uint32_t ms = 100000;
  for (uint16_t i = 0; i < c->readingCount; i++) {
    dxHistoryAdd(&history, ms, c->readings[i].levelTenths);
    ms += 1500;
  }
  int bars = 0;
  for (uint8_t bar = 0; bar < DX_HISTORY_SECONDS; bar++) {
    int16_t level = 0;
    if (dxHistoryBar(&history, ms - 1500, bar, &level)) {
      bars++;
    }
  }
  /* 30 readings over 45 seconds: 30 bars, and 15 seconds with none. */
  TEST_ASSERT_EQUAL(30, c->readingCount);
  TEST_ASSERT_EQUAL(c->readingCount, bars);
}

static void the_bar_height_follows_the_0_to_70_dbuv_scale(void) {
  TEST_ASSERT_EQUAL_UINT8(32, dxHistoryBarHeight(700, 32));
  TEST_ASSERT_EQUAL_UINT8(32, dxHistoryBarHeight(900, 32));
  TEST_ASSERT_EQUAL_UINT8(16, dxHistoryBarHeight(350, 32));
  TEST_ASSERT_EQUAL_UINT8(1, dxHistoryBarHeight(0, 32));
  TEST_ASSERT_EQUAL_UINT8(1, dxHistoryBarHeight(-40, 32));
  TEST_ASSERT_EQUAL_UINT8(0, dxHistoryBarHeight(350, 0));
}

/* ------------------------------------------------------ the DX width -- */

/* 93.4, 93.5 and 93.6 over 20 seconds at each of four DX widths. */
static void the_dx_default_keeps_rds_whole(void) {
  for (size_t i = 0; i < DX_WIDTHS_COUNT; i++) {
    const DxChannel *c = &kDxWidths[i];
    if (c->khz != 93500) {
      continue;
    }
    uint16_t clean = 0;
    for (uint16_t g = 0; g < c->groupCount; g++) {
      if ((c->groups[g].error >> 6) == 0) {
        clean++;
      }
    }
    if (c->bandwidthKHz >= DX_BANDWIDTH_DEFAULT_KHZ) {
      TEST_ASSERT_GREATER_OR_EQUAL(c->groupCount * 99 / 100, clean);
    } else {
      /* Narrower than the default, under seven in ten came clean. */
      TEST_ASSERT_LESS_THAN(c->groupCount * 7 / 10, clean);
    }
  }
}

static void no_dx_width_lets_the_neighbours_rds_through(void) {
  size_t shoulders = 0;
  for (size_t i = 0; i < DX_WIDTHS_COUNT; i++) {
    const DxChannel *c = &kDxWidths[i];
    if (c->khz == 93500) {
      continue;
    }
    TEST_ASSERT_EQUAL_UINT16(0, c->groupCount);
    TEST_ASSERT_FALSE(replay(c)->hasPi);
    shoulders++;
  }
  TEST_ASSERT_EQUAL(8, shoulders);
}

/* Why the knob steps rather than seeks in DX mode: at 114 kHz the seek's
 * own rule, fitted at the radio's own width, stops on empty channels and
 * misses a station on air. */
static void the_seek_rule_does_not_hold_at_the_dx_width(void) {
  SeekConfig cfg = atSensitivity(SEEK_SENSITIVITY_DEFAULT);
  size_t stopsOffAir = 0;
  size_t missed = 0;
  for (size_t i = 0; i < DX_AT114_COUNT; i++) {
    const DxChannel *c = &kDxAt114[i];
    TEST_ASSERT_EQUAL_UINT16(DX_BANDWIDTH_DEFAULT_KHZ, c->bandwidthKHz);
    SeekReading r = reading(&c->tuned);
    const bool stop = seekShouldStop(&cfg, BAND_FM, &r);
    if (stop && !onAir(c->khz)) {
      stopsOffAir++;
    }
    if (!stop && onAir(c->khz)) {
      missed++;
    }
  }
  TEST_ASSERT_GREATER_THAN(0, stopsOffAir);
  TEST_ASSERT_GREATER_THAN(0, missed);
}

/* Why the page's stereo mark is the pilot alone: no empty channel showed
 * one at either width. */
static void no_empty_channel_shows_a_stereo_pilot(void) {
  const DxChannel *lists[] = {kDxSweep, kDxAt114};
  const size_t counts[] = {DX_SWEEP_COUNT, DX_AT114_COUNT};
  size_t onStations = 0;
  for (size_t l = 0; l < 2; l++) {
    for (size_t i = 0; i < counts[l]; i++) {
      const DxChannel *c = &lists[l][i];
      if (onAir(c->khz)) {
        onStations += c->stereo ? 1 : 0;
      } else if (!besideAStation(c->khz)) {
        TEST_ASSERT_FALSE(c->stereo);
      }
    }
  }
  TEST_ASSERT_GREATER_THAN(0, onStations);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();

  RUN_TEST(the_seek_finds_every_station_and_nothing_else_by_default);
  RUN_TEST(the_loosest_seek_still_stops_on_nothing_but_stations);
  RUN_TEST(the_noise_floor_sits_above_the_loosest_limit);

  RUN_TEST(every_station_sending_a_pi_has_it_confirmed);
  RUN_TEST(noise_gives_a_clean_block_a_but_never_a_pi);
  RUN_TEST(only_a_clean_block_a_carries_the_right_pi);

  RUN_TEST(the_neighbours_pi_is_confirmed_beside_it);
  RUN_TEST(the_neighbours_pi_is_refused_beside_it);
  RUN_TEST(the_offset_refused_the_neighbour_for_a_whole_minute);
  RUN_TEST(a_station_stays_inside_the_window_as_it_drifts);

  RUN_TEST(the_window_is_the_seeks_own);
  RUN_TEST(the_dx_page_rule_names_only_a_station_on_its_own_carrier);
  RUN_TEST(no_country_without_both_the_ecc_and_the_pi);
  RUN_TEST(the_preset_match_comes_with_the_identity);
  RUN_TEST(no_rds_is_nobody);
  RUN_TEST(a_hand_entry_on_channel_keeps_the_name_and_the_pi);
  RUN_TEST(a_hand_entry_beside_a_strong_station_keeps_neither);
  RUN_TEST(a_hand_entry_keeps_the_name_without_a_pi);
  RUN_TEST(a_hand_entry_with_nothing_to_go_on_keeps_nothing);
  RUN_TEST(a_preset_with_no_pi_says_nothing);
  RUN_TEST(one_matching_block_is_the_presets_station);
  RUN_TEST(only_a_confirmed_other_pi_is_another_station);
  RUN_TEST(a_preset_learns_only_when_it_has_none);
  RUN_TEST(nothing_confirmed_means_no);
  RUN_TEST(the_channel_test_is_the_window_alone);
  RUN_TEST(a_pi_is_heard_now_only_on_a_clean_block_a);
  RUN_TEST(the_text_goes_in_when_it_is_the_entrys_station);
  RUN_TEST(an_entry_with_no_pi_takes_the_offset_alone);
  RUN_TEST(no_whole_text_means_none);
  RUN_TEST(another_channel_on_the_dial_gives_none);
  RUN_TEST(another_pi_gives_none);
  RUN_TEST(beside_a_strong_station_gives_none);
  RUN_TEST(a_refused_text_clears_the_entrys);

  RUN_TEST(the_tile_is_empty_before_anything_is_heard);
  RUN_TEST(the_tile_follows_the_decoder);
  RUN_TEST(a_station_sending_zero_gets_its_own_tile);
  RUN_TEST(the_neighbours_pi_is_only_seen_on_the_tile);
  RUN_TEST(nowhere_to_put_the_digits_is_safe);
  RUN_TEST(the_block_meter_counts_down_with_the_errors);
  RUN_TEST(an_empty_history_has_no_bars);
  RUN_TEST(each_second_keeps_its_highest_reading);
  RUN_TEST(a_second_with_no_reading_has_no_bar);
  RUN_TEST(a_minute_later_the_history_has_rolled_off);
  RUN_TEST(a_bar_out_of_range_is_refused);
  RUN_TEST(the_minute_capture_fills_the_history_with_gaps);
  RUN_TEST(the_bar_height_follows_the_0_to_70_dbuv_scale);

  RUN_TEST(the_dx_default_keeps_rds_whole);
  RUN_TEST(no_dx_width_lets_the_neighbours_rds_through);
  RUN_TEST(the_seek_rule_does_not_hold_at_the_dx_width);
  RUN_TEST(no_empty_channel_shows_a_stereo_pilot);

  return UNITY_END();
}
