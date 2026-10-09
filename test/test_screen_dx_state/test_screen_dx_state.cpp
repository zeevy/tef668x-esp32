/*
 * Tests for the view builders of the four DX pages: DX, Catches, Scanner and
 * Scope. Runs on a PC.
 *
 * Where it helps, the inputs are real readings recorded on the radio: the band
 * sweep, the 20 seconds beside strong stations and the music recorded on 106.4
 * for the DX page, and both passes of a level sweep of the band for the Scope
 * page. The rest are small inputs built by hand, so each test shows one rule.
 */
#include <unity.h>

#include <stdio.h>
#include <string.h>

#include "../test_dx/captures.h"
#include "../test_dx_sweep/capture.h"
#include "../test_dx_sweep/capture_mw.h"
#include "../test_meter/captures.h"
#include "core/band_plan.h"
#include "core/rds.h"
#include "core/rds_country.h"
#include "core/strings.h"
#include "screen_dx_state.h"

void setUp(void) {}
void tearDown(void) {}

/* ------------------------------------------------------------ helpers -- */

static RadioSnapshot snap;
static Rds decoder;

/* The Catches builder with its inputs in the order the page reads them. */
static void catchesBuild(const DxCatches *list, uint8_t cursor, uint8_t page,
                         uint8_t pages, const char *clock,
                         int16_t offsetMinutes, const char *confirm,
                         ScreenCatchesKeep *keep, ScreenCatches *out) {
  ScreenCatchesInputs in;
  memset(&in, 0, sizeof(in));
  in.list = list;
  in.cursor = cursor;
  in.page = page;
  in.pages = pages;
  in.clock = clock;
  in.offsetMinutes = offsetMinutes;
  in.confirm = confirm;
  screenCatchesStateBuild(&in, keep, out);
}

/* The dial on `khz` of FM, with no reading and nothing heard yet. */
static void tuneTo(uint32_t khz) {
  memset(&snap, 0, sizeof(snap));
  snap.settings.band = BAND_FM;
  snap.settings.freqKHz = khz;
  snap.memorySlot = MEMORY_NO_SLOT;
}

/* A good reading on the channel, `offset` tenths of a kHz off centre. */
static void readAt(int16_t level, int16_t offset) {
  snap.qualityValid = true;
  snap.quality.levelDbuVTenths = level;
  snap.quality.offsetKHzTenths = offset;
}

static const DxChannel *channelOf(const DxChannel *list, size_t count,
                                  uint32_t khz) {
  for (size_t i = 0; i < count; i++) {
    if (list[i].khz == khz) {
      return &list[i];
    }
  }
  TEST_FAIL_MESSAGE("that channel is not in the capture");
  return NULL;
}

/* The dial on a captured channel: its settled reading, and the RDS decoder
 * fed every group heard there, in order. */
static void tuneToCapture(const DxChannel *c) {
  tuneTo(c->khz);
  snap.qualityValid = true;
  snap.quality.levelDbuVTenths = c->tuned.levelTenths;
  snap.quality.usnTenths = c->tuned.noiseTenths;
  snap.quality.multipathTenths = c->tuned.multipathTenths;
  snap.quality.offsetKHzTenths = c->tuned.offsetTenths;
  snap.quality.bandwidthKHz = c->bandwidthKHz;
  snap.quality.modulationPercent = c->modulation;
  snap.quality.stereo = c->stereo;
  rdsReset(&decoder, c->khz);
  for (uint16_t i = 0; i < c->groupCount; i++) {
    RdsRead r;
    memset(&r, 0, sizeof(r));
    r.synchronised = true;
    r.haveGroup = true;
    for (int b = 0; b < 4; b++) {
      r.block[b] = c->groups[i].block[b];
      r.error[b] = (uint8_t)((c->groups[i].error >> (6 - b * 2)) & 0x03);
    }
    rdsFeed(&decoder, &r);
  }
  snap.rds = decoder.info;
}

/* Fill a view with a pattern, so a test can see that a builder left it
 * alone. */
#define FILL(x) memset(&(x), 0xA5, sizeof(x))

static bool stillFilled(const void *p, size_t n) {
  const unsigned char *b = (const unsigned char *)p;
  for (size_t i = 0; i < n; i++) {
    if (b[i] != 0xA5) {
      return false;
    }
  }
  return true;
}

/* ------------------------------------------------------------ DX page -- */

static ScreenDxKeep dxKeep;
static ScreenDx dx;

static ScreenDxInputs dxInputs(void) {
  ScreenDxInputs in;
  memset(&in, 0, sizeof(in));
  in.snap = &snap;
  in.rdsEnabled = true;
  in.clock = "07:17";
  in.nowMs = 1000000;
  in.pages = 4;
  in.region = RDS_REGION_EUROPE;
  return in;
}

static void buildDx(const ScreenDxInputs *in) {
  screenDxStateReset(&dxKeep);
  screenDxStateBuild(in, &dxKeep, &dx);
}

/* 106.4 MHz from the band sweep: a local station, on its own channel. */
static void a_real_station_shows_its_readings_name_and_confirmed_pi(void) {
  tuneToCapture(channelOf(kDxSweep, DX_SWEEP_COUNT, 106400));
  const ScreenDxInputs in = dxInputs();
  buildDx(&in);

  TEST_ASSERT_EQUAL_STRING("1/4", dx.position);
  TEST_ASSERT_EQUAL_STRING("07:17", dx.clock);
  TEST_ASSERT_EQUAL_STRING("106.40", dx.frequency);
  TEST_ASSERT_EQUAL_STRING(txt(STR_COMMON_UNIT_MHZ), dx.unit);
  TEST_ASSERT_EQUAL_STRING("41.2", dx.level);
  TEST_ASSERT_EQUAL_STRING("3", dx.usn);
  TEST_ASSERT_EQUAL_STRING("11", dx.wam);
  TEST_ASSERT_EQUAL_STRING("+6", dx.offset);
  TEST_ASSERT_EQUAL_STRING("217", dx.bandwidth);
  TEST_ASSERT_EQUAL_STRING("46", dx.modulation);
  TEST_ASSERT_TRUE(dx.stereo);

  TEST_ASSERT_EQUAL(SCREEN_DX_PI_CONFIRMED, dx.pi);
  TEST_ASSERT_EQUAL_STRING("1064", dx.piDigits);
  TEST_ASSERT_FALSE(dx.piOther);
  /* No ECC came, so no country is named, and the help mark says so. */
  TEST_ASSERT_NULL(dx.country);
  TEST_ASSERT_TRUE(dx.countryUnsure);

  TEST_ASSERT_TRUE(dx.psShown);
  TEST_ASSERT_EQUAL_MEMORY(" MAGIC  ", dx.ps, SCREEN_DX_PS_LEN);
  for (int i = 0; i < SCREEN_DX_PS_LEN; i++) {
    TEST_ASSERT_TRUE(dx.psHave[i]);
  }
  /* Whole, it is one text without the station's padding. */
  TEST_ASSERT_EQUAL_STRING("MAGIC", dx.psWhole);
  TEST_ASSERT_EQUAL_STRING(rdsPtyNameIn(12, RDS_REGION_EUROPE), dx.pty);
  for (int b = 0; b < 4; b++) {
    TEST_ASSERT_EQUAL_INT8(0, dx.blockError[b]);
  }
}

/* 93.4 MHz, beside 93.5: the decoder confirms 93.5's PI through the
 * sidebands, 108 kHz off centre. The tile shows it only as heard. */
static void the_station_next_door_is_only_seen_never_confirmed(void) {
  tuneToCapture(channelOf(kDxTwenty, DX_TWENTY_COUNT, 93400));
  TEST_ASSERT_TRUE(snap.rds.hasPi);
  const ScreenDxInputs in = dxInputs();
  buildDx(&in);

  TEST_ASSERT_EQUAL(SCREEN_DX_PI_SEEN, dx.pi);
  TEST_ASSERT_EQUAL_STRING("0935", dx.piDigits);
  TEST_ASSERT_NULL(dx.country);
  TEST_ASSERT_FALSE(dx.countryUnsure);
  TEST_ASSERT_EQUAL_STRING("+108", dx.offset);
  TEST_ASSERT_FALSE(dx.stereo);
  TEST_ASSERT_EQUAL_INT8(3, dx.blockError[0]);
  TEST_ASSERT_EQUAL_INT8(3, dx.blockError[1]);
  TEST_ASSERT_EQUAL_INT8(3, dx.blockError[2]);
  TEST_ASSERT_EQUAL_INT8(2, dx.blockError[3]);
}

static void the_pi_tile_goes_from_none_to_seen_to_confirmed(void) {
  tuneTo(106400);
  readAt(412, 61);
  const ScreenDxInputs in = dxInputs();

  /* Nothing heard. */
  buildDx(&in);
  TEST_ASSERT_EQUAL(SCREEN_DX_PI_NONE, dx.pi);
  TEST_ASSERT_EQUAL_STRING("", dx.piDigits);

  /* Heard once. */
  snap.rds.hasPiHeard = true;
  snap.rds.piHeard = 0x63B2;
  buildDx(&in);
  TEST_ASSERT_EQUAL(SCREEN_DX_PI_SEEN, dx.pi);
  TEST_ASSERT_EQUAL_STRING("63B2", dx.piDigits);

  /* Heard again with one digit changed: that digit is in doubt. */
  snap.rds.piUnsureNibbles = 0x02;
  buildDx(&in);
  TEST_ASSERT_EQUAL(SCREEN_DX_PI_PARTIAL, dx.pi);
  TEST_ASSERT_EQUAL_STRING("63?2", dx.piDigits);

  /* Confirmed, but the carrier is 108 kHz off: still only seen. */
  snap.rds.piUnsureNibbles = 0;
  snap.rds.hasPi = true;
  snap.rds.pi = 0x63B2;
  readAt(412, 1082);
  buildDx(&in);
  TEST_ASSERT_EQUAL(SCREEN_DX_PI_SEEN, dx.pi);

  /* Confirmed, and this channel's own. */
  readAt(412, 61);
  buildDx(&in);
  TEST_ASSERT_EQUAL(SCREEN_DX_PI_CONFIRMED, dx.pi);
  TEST_ASSERT_EQUAL_STRING("63B2", dx.piDigits);

  /* A station that sends 0000. */
  tuneTo(94300);
  readAt(443, 32);
  snap.rds.piZero = true;
  snap.rds.hasPiHeard = true;
  snap.rds.piHeard = 0;
  buildDx(&in);
  TEST_ASSERT_EQUAL(SCREEN_DX_PI_ZERO, dx.pi);
  TEST_ASSERT_EQUAL_STRING("0000", dx.piDigits);
}

static void a_confirmed_pi_names_its_country_only_with_an_ecc(void) {
  tuneTo(106400);
  readAt(412, 61);
  snap.rds.hasPi = true;
  snap.rds.pi = 0x5064;
  const ScreenDxInputs in = dxInputs();
  buildDx(&in);
  TEST_ASSERT_NULL(dx.country);
  TEST_ASSERT_TRUE(dx.countryUnsure);

  snap.rds.hasEcc = true;
  snap.rds.ecc = 0xF2;
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("IN", dx.country);
  TEST_ASSERT_FALSE(dx.countryUnsure);
}

static void north_america_shows_call_letters_in_place_of_the_country(void) {
  tuneTo(98300);
  readAt(482, 61);
  snap.rds.hasPi = true;
  snap.rds.pi = 0x21C7;
  ScreenDxInputs in = dxInputs();
  in.region = RDS_REGION_NORTH_AMERICA;

  /* Worked out from the PI: a guess, so the help mark. */
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("KGTB", dx.country);
  TEST_ASSERT_TRUE(dx.countryUnsure);

  /* Sent by the station itself: no help mark. */
  snap.rds.hasStationShort = true;
  snprintf(snap.rds.stationShort, sizeof(snap.rds.stationShort), "HOT 97");
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("HOT 97", dx.country);
  TEST_ASSERT_FALSE(dx.countryUnsure);

  /* The same PI in Europe names no call letters. */
  in.region = RDS_REGION_EUROPE;
  buildDx(&in);
  TEST_ASSERT_NULL(dx.country);
}

static void a_preset_with_a_stored_pi_names_itself_under_the_pi(void) {
  tuneTo(106400);
  readAt(412, 61);
  snap.memorySlot = 2;
  ScreenDxInputs in = dxInputs();
  in.presetPi = 0x1064;

  /* One clean block A with the stored PI is enough: the tile goes amber
   * with the stored PI, and the line says P03. */
  snap.rds.hasPiHeard = true;
  snap.rds.piHeard = 0x1064;
  buildDx(&in);
  TEST_ASSERT_EQUAL(SCREEN_DX_PI_CONFIRMED, dx.pi);
  TEST_ASSERT_EQUAL_STRING("1064", dx.piDigits);
  TEST_ASSERT_EQUAL_STRING("P03", dx.country);
  TEST_ASSERT_FALSE(dx.countryUnsure);
  TEST_ASSERT_FALSE(dx.piOther);

  /* Another station, confirmed on the preset. */
  snap.rds.hasPi = true;
  snap.rds.pi = 0x26FF;
  snap.rds.piHeard = 0x26FF;
  buildDx(&in);
  TEST_ASSERT_EQUAL(SCREEN_DX_PI_CONFIRMED, dx.pi);
  TEST_ASSERT_EQUAL_STRING("26FF", dx.piDigits);
  TEST_ASSERT_EQUAL_STRING("not P03", dx.country);
  TEST_ASSERT_FALSE(dx.countryUnsure);
  TEST_ASSERT_TRUE(dx.piOther);

  /* Off every preset there is no line, whatever PI the caller passed. */
  snap.memorySlot = MEMORY_NO_SLOT;
  buildDx(&in);
  TEST_ASSERT_NULL(dx.country);
  TEST_ASSERT_TRUE(dx.countryUnsure);
  TEST_ASSERT_FALSE(dx.piOther);

  /* The stored PI heard off channel says nothing either way. */
  memset(&snap.rds, 0, sizeof(snap.rds));
  snap.memorySlot = 2;
  snap.rds.hasPiHeard = true;
  snap.rds.piHeard = 0x1064;
  readAt(412, 1082);
  buildDx(&in);
  TEST_ASSERT_EQUAL(SCREEN_DX_PI_SEEN, dx.pi);
  TEST_ASSERT_NULL(dx.country);
}

static void block_errors_are_minus_one_until_a_group_arrives(void) {
  tuneTo(106400);
  readAt(412, 61);
  const ScreenDxInputs in = dxInputs();
  buildDx(&in);
  for (int b = 0; b < 4; b++) {
    TEST_ASSERT_EQUAL_INT8(-1, dx.blockError[b]);
  }

  snap.rds.hasBlockErrors = true;
  snap.rds.blockError[0] = 0;
  snap.rds.blockError[1] = 1;
  snap.rds.blockError[2] = 2;
  snap.rds.blockError[3] = 3;
  buildDx(&in);
  TEST_ASSERT_EQUAL_INT8(0, dx.blockError[0]);
  TEST_ASSERT_EQUAL_INT8(1, dx.blockError[1]);
  TEST_ASSERT_EQUAL_INT8(2, dx.blockError[2]);
  TEST_ASSERT_EQUAL_INT8(3, dx.blockError[3]);
}

static void the_name_shows_each_character_as_it_arrives(void) {
  tuneTo(106400);
  readAt(412, 61);
  const ScreenDxInputs in = dxInputs();

  buildDx(&in);
  TEST_ASSERT_FALSE(dx.psShown);
  TEST_ASSERT_NULL(dx.pty);

  /* Two characters of the name so far. */
  snap.rds.psHeard[1] = 'M';
  snap.rds.psHeardHave[1] = true;
  snap.rds.psHeard[2] = 'A';
  snap.rds.psHeardHave[2] = true;
  buildDx(&in);
  TEST_ASSERT_TRUE(dx.psShown);
  TEST_ASSERT_FALSE(dx.psHave[0]);
  TEST_ASSERT_TRUE(dx.psHave[1]);
  TEST_ASSERT_TRUE(dx.psHave[2]);
  TEST_ASSERT_FALSE(dx.psHave[3]);
  TEST_ASSERT_EQUAL_CHAR('M', dx.ps[1]);
  TEST_ASSERT_EQUAL_CHAR('A', dx.ps[2]);
  /* Not whole yet, so it stays a character a cell. */
  TEST_ASSERT_EQUAL_STRING("", dx.psWhole);

  /* Every character heard, spaces among them: whole, the padding dropped,
   * the space inside kept. */
  const char heard[SCREEN_DX_PS_LEN + 1] = " MA  IC ";
  for (int i = 0; i < SCREEN_DX_PS_LEN; i++) {
    snap.rds.psHeard[i] = heard[i];
    snap.rds.psHeardHave[i] = true;
  }
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("MA  IC", dx.psWhole);

  /* A name of spaces alone is whole and empty, so nothing is drawn. */
  for (int i = 0; i < SCREEN_DX_PS_LEN; i++) {
    snap.rds.psHeard[i] = ' ';
  }
  buildDx(&in);
  TEST_ASSERT_TRUE(dx.psShown);
  TEST_ASSERT_EQUAL_STRING("", dx.psWhole);
  for (int i = 0; i < SCREEN_DX_PS_LEN; i++) {
    snap.rds.psHeard[i] = '\0';
    snap.rds.psHeardHave[i] = false;
  }

  /* The programme type is named in the region's own words. */
  snap.rds.hasPty = true;
  snap.rds.pty = 5;
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING(rdsPtyNameIn(5, RDS_REGION_EUROPE), dx.pty);
  ScreenDxInputs na = in;
  na.region = RDS_REGION_NORTH_AMERICA;
  buildDx(&na);
  TEST_ASSERT_EQUAL_STRING(rdsPtyNameIn(5, RDS_REGION_NORTH_AMERICA), dx.pty);
}

static void rds_switched_off_leaves_the_rds_fields_empty(void) {
  tuneToCapture(channelOf(kDxSweep, DX_SWEEP_COUNT, 106400));
  ScreenDxInputs in = dxInputs();
  in.rdsEnabled = false;
  buildDx(&in);
  TEST_ASSERT_EQUAL(SCREEN_DX_PI_NONE, dx.pi);
  TEST_ASSERT_EQUAL_STRING("", dx.piDigits);
  TEST_ASSERT_FALSE(dx.psShown);
  TEST_ASSERT_NULL(dx.pty);
  TEST_ASSERT_NULL(dx.country);
  for (int b = 0; b < 4; b++) {
    TEST_ASSERT_EQUAL_INT8(-1, dx.blockError[b]);
  }
  /* The readings do not need RDS. */
  TEST_ASSERT_EQUAL_STRING("41.2", dx.level);
}

static void readings_are_formatted_with_sign_rounding_and_offset(void) {
  tuneTo(106400);
  ScreenDxInputs in = dxInputs();

  readAt(-18, -61);
  snap.quality.usnTenths = 1000;
  snap.quality.multipathTenths = 4;
  snap.quality.modulationPercent = -1;
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("-1.8", dx.level);
  TEST_ASSERT_EQUAL_STRING("-6", dx.offset);
  TEST_ASSERT_EQUAL_STRING("100", dx.usn);
  TEST_ASSERT_EQUAL_STRING("0", dx.wam);
  TEST_ASSERT_EQUAL_STRING("-1", dx.modulation);

  /* Offsets round half away from zero, and a zero has no sign. */
  readAt(412, 4);
  snap.quality.multipathTenths = 5;
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("0", dx.offset);
  TEST_ASSERT_EQUAL_STRING("1", dx.wam);
  readAt(412, -4);
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("0", dx.offset);
  readAt(412, 5);
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("+1", dx.offset);
  readAt(412, -5);
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("-1", dx.offset);
}

static void no_reading_leaves_the_six_readings_empty(void) {
  tuneTo(106400);
  snap.quality.stereo = true;
  const ScreenDxInputs in = dxInputs();
  buildDx(&in);
  TEST_ASSERT_NULL(dx.level);
  TEST_ASSERT_NULL(dx.usn);
  TEST_ASSERT_NULL(dx.wam);
  TEST_ASSERT_NULL(dx.offset);
  TEST_ASSERT_NULL(dx.bandwidth);
  TEST_ASSERT_NULL(dx.modulation);
  /* A pilot from a reading that failed is not drawn. */
  TEST_ASSERT_FALSE(dx.stereo);
  /* The frequency is known all the same. */
  TEST_ASSERT_EQUAL_STRING("106.40", dx.frequency);
}

static void the_history_keeps_the_peak_of_each_second_and_gaps_stay_empty(
    void) {
  tuneTo(106400);
  screenDxStateReset(&dxKeep);
  readAt(300, 61);
  screenDxStateFeed(&dxKeep, &snap, 10000);
  readAt(350, 61);
  screenDxStateFeed(&dxKeep, &snap, 10400);
  readAt(320, 61);
  screenDxStateFeed(&dxKeep, &snap, 10900);
  /* No reading in second 11. */
  readAt(200, 61);
  screenDxStateFeed(&dxKeep, &snap, 12100);

  /* The build adds its own reading, in second 13. */
  readAt(250, 61);
  ScreenDxInputs in = dxInputs();
  in.nowMs = 13000;
  screenDxStateBuild(&in, &dxKeep, &dx);

  TEST_ASSERT_TRUE(dx.historyHave[59]);
  TEST_ASSERT_EQUAL_INT16(250, dx.historyTenths[59]);
  TEST_ASSERT_TRUE(dx.historyHave[58]);
  TEST_ASSERT_EQUAL_INT16(200, dx.historyTenths[58]);
  TEST_ASSERT_FALSE(dx.historyHave[57]);
  TEST_ASSERT_TRUE(dx.historyHave[56]);
  TEST_ASSERT_EQUAL_INT16(350, dx.historyTenths[56]);
  for (int i = 0; i < 56; i++) {
    TEST_ASSERT_FALSE(dx.historyHave[i]);
  }

  /* A failed reading adds no bar, and the bars before it stay. */
  snap.qualityValid = false;
  in.nowMs = 14000;
  screenDxStateBuild(&in, &dxKeep, &dx);
  TEST_ASSERT_FALSE(dx.historyHave[59]);
  TEST_ASSERT_TRUE(dx.historyHave[58]);
  TEST_ASSERT_EQUAL_INT16(250, dx.historyTenths[58]);
  TEST_ASSERT_TRUE(dx.historyHave[55]);
}

static void a_retune_starts_the_history_again(void) {
  tuneTo(106400);
  screenDxStateReset(&dxKeep);
  readAt(300, 61);
  screenDxStateFeed(&dxKeep, &snap, 10000);
  screenDxStateFeed(&dxKeep, &snap, 11000);

  tuneTo(98300);
  readAt(400, 61);
  ScreenDxInputs in = dxInputs();
  in.nowMs = 12000;
  screenDxStateBuild(&in, &dxKeep, &dx);
  TEST_ASSERT_TRUE(dx.historyHave[59]);
  TEST_ASSERT_EQUAL_INT16(400, dx.historyTenths[59]);
  for (int i = 0; i < 59; i++) {
    TEST_ASSERT_FALSE(dx.historyHave[i]);
  }
  TEST_ASSERT_EQUAL_UINT32(98300, dxKeep.historyKHz);

  /* A failed reading on a new channel still starts its history again. */
  tuneTo(106400);
  screenDxStateFeed(&dxKeep, &snap, 13000);
  TEST_ASSERT_EQUAL_UINT32(106400, dxKeep.historyKHz);
  TEST_ASSERT_FALSE(dxKeep.history.started);
}

static void the_header_follows_the_inputs(void) {
  tuneTo(106400);
  ScreenDxInputs in = dxInputs();
  in.pages = 0;
  in.clock = NULL;
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("1/1", dx.position);
  TEST_ASSERT_NULL(dx.clock);
}

static void a_band_that_is_not_one_has_no_frequency(void) {
  tuneTo(106400);
  snap.settings.band = BAND_COUNT;
  const ScreenDxInputs in = dxInputs();
  buildDx(&in);
  TEST_ASSERT_NULL(dx.frequency);
  TEST_ASSERT_NULL(dx.unit);

  /* An AM band reads in kHz. */
  tuneTo(1377);
  snap.settings.band = BAND_MW;
  buildDx(&in);
  TEST_ASSERT_EQUAL_STRING("1377", dx.frequency);
  TEST_ASSERT_EQUAL_STRING(txt(STR_COMMON_UNIT_KHZ), dx.unit);
}

static void dx_builders_leave_everything_alone_on_null_inputs(void) {
  tuneTo(106400);
  readAt(412, 61);
  screenDxStateReset(NULL);
  screenDxStateFeed(NULL, &snap, 1000);
  FILL(dxKeep);
  screenDxStateFeed(&dxKeep, NULL, 1000);
  TEST_ASSERT_TRUE(stillFilled(&dxKeep, sizeof(dxKeep)));

  ScreenDxInputs in = dxInputs();
  FILL(dx);
  screenDxStateBuild(NULL, &dxKeep, &dx);
  screenDxStateBuild(&in, NULL, &dx);
  in.snap = NULL;
  screenDxStateBuild(&in, &dxKeep, &dx);
  TEST_ASSERT_TRUE(stillFilled(&dx, sizeof(dx)));
  TEST_ASSERT_TRUE(stillFilled(&dxKeep, sizeof(dxKeep)));
  in.snap = &snap;
  screenDxStateBuild(&in, &dxKeep, NULL);
  TEST_ASSERT_TRUE(stillFilled(&dxKeep, sizeof(dxKeep)));
}

/* ------------------------------------------------------- Catches page -- */

static DxCatches catches;
static ScreenCatchesKeep catchKeep;
static ScreenCatches cv;
static uint32_t visit;

/* 07:16 on a UTC+5:30 clock, 01:46 UTC. */
static const uint32_t kMorningUtc = 1790473560u;
static const int16_t kIstMinutes = 330;

/* One confirmation of `pi` on `khz`. A time of 0 means the clock was not
 * set. */
static void hear(uint32_t khz, uint16_t pi, const char *ps, const char *country,
                 int16_t level, uint32_t utc, bool isNew) {
  DxHearing h;
  memset(&h, 0, sizeof(h));
  h.khz = khz;
  h.band = (uint8_t)BAND_FM;
  h.pi = pi;
  h.ps = ps;
  h.country = country;
  h.readings.levelDbuVTenths = level;
  h.at.known = utc != 0;
  h.at.value = utc != 0 ? utc : 5000;
  h.visit = ++visit;
  TEST_ASSERT_NOT_NULL(dxCatchesAdd(&catches, &h, isNew));
}

/* `n` catches on channels 200 kHz apart, each with its own PI from 0x1000
 * up, the last one added the newest. */
static void hearMany(uint8_t n) {
  dxCatchesReset(&catches);
  for (uint8_t i = 0; i < n; i++) {
    hear(87500 + (uint32_t)i * 200, (uint16_t)(0x1000 + i), NULL, NULL, 300,
         kMorningUtc + i * 60u, false);
  }
}

static void buildCatches(uint8_t cursor, const char *confirm) {
  catchesBuild(&catches, cursor, 3, 4, "07:39", kIstMinutes, confirm,
               &catchKeep, &cv);
}

static void an_empty_catches_list_shows_no_rows(void) {
  dxCatchesReset(&catches);
  catchesBuild(&catches, 0, 0, 0, NULL, kIstMinutes, NULL, &catchKeep, &cv);
  TEST_ASSERT_EQUAL_UINT8(0, cv.rows);
  TEST_ASSERT_NULL(cv.range);
  TEST_ASSERT_EQUAL_STRING("1/1", cv.position);
  TEST_ASSERT_NULL(cv.clock);

  /* No list at all looks the same, and a message still shows in the
   * header. */
  catchesBuild(NULL, 0, 3, 4, "07:39", kIstMinutes, "Logged", &catchKeep, &cv);
  TEST_ASSERT_EQUAL_UINT8(0, cv.rows);
  TEST_ASSERT_NULL(cv.range);
  TEST_ASSERT_EQUAL_STRING("Logged", cv.position);
  TEST_ASSERT_NULL(cv.clock);
}

static void a_long_list_shows_the_six_rows_that_hold_the_cursor(void) {
  hearMany(14);

  buildCatches(0, NULL);
  TEST_ASSERT_EQUAL_STRING("1-6 of 14", cv.range);
  TEST_ASSERT_EQUAL_UINT8(6, cv.rows);
  TEST_ASSERT_EQUAL_UINT8(0, cv.cursor);
  TEST_ASSERT_EQUAL_STRING("100D", cv.row[0].pi);
  TEST_ASSERT_EQUAL_STRING("1008", cv.row[5].pi);

  buildCatches(7, NULL);
  TEST_ASSERT_EQUAL_STRING("7-12 of 14", cv.range);
  TEST_ASSERT_EQUAL_UINT8(6, cv.rows);
  TEST_ASSERT_EQUAL_UINT8(1, cv.cursor);
  TEST_ASSERT_EQUAL_STRING("1007", cv.row[0].pi);
  TEST_ASSERT_EQUAL_STRING("1006", cv.row[1].pi);

  buildCatches(13, NULL);
  TEST_ASSERT_EQUAL_STRING("13-14 of 14", cv.range);
  TEST_ASSERT_EQUAL_UINT8(2, cv.rows);
  TEST_ASSERT_EQUAL_UINT8(1, cv.cursor);
  TEST_ASSERT_EQUAL_STRING("1000", cv.row[1].pi);

  TEST_ASSERT_EQUAL_STRING("4/4", cv.position);
  TEST_ASSERT_EQUAL_STRING("07:39", cv.clock);
}

static void a_cursor_past_the_end_goes_to_the_oldest(void) {
  hearMany(12);
  buildCatches(200, NULL);
  TEST_ASSERT_EQUAL_STRING("7-12 of 12", cv.range);
  TEST_ASSERT_EQUAL_UINT8(6, cv.rows);
  TEST_ASSERT_EQUAL_UINT8(5, cv.cursor);
}

static void a_catch_row_shows_time_frequency_pi_name_country_and_level(void) {
  dxCatchesReset(&catches);
  hear(98300, 0x26FF, "MIRCHI", "IN", 482, kMorningUtc, true);
  hear(106400, 0x1064, NULL, NULL, -18, 0, false);
  buildCatches(1, NULL);
  TEST_ASSERT_EQUAL_STRING("1-2 of 2", cv.range);
  TEST_ASSERT_EQUAL_UINT8(2, cv.rows);
  TEST_ASSERT_EQUAL_UINT8(1, cv.cursor);

  /* The newest first: heard before the clock was set, no name, no ECC. */
  const ScreenCatchRow *a = &cv.row[0];
  TEST_ASSERT_NULL(a->time);
  TEST_ASSERT_EQUAL_STRING("106.40", a->frequency);
  TEST_ASSERT_EQUAL_STRING("1064", a->pi);
  TEST_ASSERT_NULL(a->ps);
  TEST_ASSERT_NULL(a->country);
  TEST_ASSERT_TRUE(a->countryUnsure);
  TEST_ASSERT_FALSE(a->isNew);
  TEST_ASSERT_EQUAL_STRING("-1.8", a->level);
  TEST_ASSERT_EQUAL_STRING(
      "\xC3\x97"
      "1",
      a->count);

  /* A NEW catch with its name and country, at the local time. */
  const ScreenCatchRow *b = &cv.row[1];
  TEST_ASSERT_EQUAL_STRING("07:16", b->time);
  TEST_ASSERT_EQUAL_STRING("98.30", b->frequency);
  TEST_ASSERT_EQUAL_STRING("26FF", b->pi);
  TEST_ASSERT_EQUAL_STRING("MIRCHI", b->ps);
  TEST_ASSERT_EQUAL_STRING("IN", b->country);
  TEST_ASSERT_FALSE(b->countryUnsure);
  TEST_ASSERT_TRUE(b->isNew);
  TEST_ASSERT_EQUAL_STRING("48.2", b->level);

  /* The clock offset is the caller's. */
  catchesBuild(&catches, 1, 3, 4, "07:39", 0, "Logged", &catchKeep, &cv);
  TEST_ASSERT_EQUAL_STRING("01:46", cv.row[1].time);
  TEST_ASSERT_EQUAL_STRING("48.2", cv.row[1].level);
  TEST_ASSERT_EQUAL_STRING("-1.8", cv.row[0].level);
  TEST_ASSERT_EQUAL_STRING("Logged", cv.position);
}

static void a_catch_count_past_99_says_99_plus(void) {
  dxCatchesReset(&catches);
  hear(98300, 0x26FF, NULL, NULL, 482, kMorningUtc, false);
  catches.item[0].count = 99;
  buildCatches(0, NULL);
  TEST_ASSERT_EQUAL_STRING(
      "\xC3\x97"
      "99",
      cv.row[0].count);
  catches.item[0].count = 100;
  buildCatches(0, NULL);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_COUNT_99_PLUS), cv.row[0].count);
}

static void a_full_list_keeps_the_newest_and_shows_the_oldest_kept_last(void) {
  hearMany(DX_CATCHES_MAX + 1);
  TEST_ASSERT_EQUAL_UINT8(DX_CATCHES_MAX, catches.count);

  buildCatches(0, NULL);
  TEST_ASSERT_EQUAL_STRING("1-6 of 32", cv.range);
  TEST_ASSERT_EQUAL_STRING("1020", cv.row[0].pi);

  /* The first one heard went to make room. The second is the oldest kept,
   * on the last row. */
  buildCatches(DX_CATCHES_MAX - 1, NULL);
  TEST_ASSERT_EQUAL_STRING("31-32 of 32", cv.range);
  TEST_ASSERT_EQUAL_UINT8(2, cv.rows);
  TEST_ASSERT_EQUAL_UINT8(1, cv.cursor);
  TEST_ASSERT_EQUAL_STRING("1002", cv.row[0].pi);
  TEST_ASSERT_EQUAL_STRING("1001", cv.row[1].pi);
  TEST_ASSERT_EQUAL_STRING("87.70", cv.row[1].frequency);
}

static void a_catch_with_a_bad_band_or_clock_offset_leaves_those_empty(void) {
  dxCatchesReset(&catches);
  hear(98300, 0x26FF, NULL, NULL, 482, kMorningUtc, false);
  catches.item[0].band = (uint8_t)BAND_COUNT;
  catchesBuild(&catches, 0, 3, 4, NULL, 2000, NULL, &catchKeep, &cv);
  TEST_ASSERT_NULL(cv.row[0].frequency);
  TEST_ASSERT_NULL(cv.row[0].time);
  TEST_ASSERT_EQUAL_STRING("26FF", cv.row[0].pi);
}

static void catches_builder_leaves_everything_alone_on_null_keep_or_out(void) {
  hearMany(3);
  FILL(cv);
  catchesBuild(&catches, 0, 3, 4, NULL, 0, NULL, NULL, &cv);
  TEST_ASSERT_TRUE(stillFilled(&cv, sizeof(cv)));
  FILL(catchKeep);
  catchesBuild(&catches, 0, 3, 4, NULL, 0, NULL, &catchKeep, NULL);
  screenCatchesStateBuild(NULL, &catchKeep, &cv); /* Must not crash. */
  TEST_ASSERT_TRUE(stillFilled(&catchKeep, sizeof(catchKeep)));
}

/* ------------------------------------------------------- Scanner page -- */

static DxScan scan;
static DxScanBand walk = {87500, 100, NULL, NULL};
static ScreenScanKeep scanKeep;
static ScreenScan sv;

static ScreenScanInputs scanInputs(void) {
  ScreenScanInputs in;
  memset(&in, 0, sizeof(in));
  in.scan = &scan;
  in.snap = &snap;
  in.catches = &catches;
  in.lowKHz = 87500;
  in.highKHz = 108000;
  in.range = DX_RANGE_BAND_LESS_MEMORY;
  in.memFirst = 1;
  in.memLast = MEMORY_SLOT_COUNT;
  in.stop = DX_STOP_NEW;
  in.dwellMs = DX_SCAN_DWELL_MS;
  in.page = 2;
  in.pages = 4;
  in.clock = "07:17";
  return in;
}

/* A scan of the FM band started from the dial on 106.4. */
static void startScan(void) {
  DxScanPlan plan;
  memset(&plan, 0, sizeof(plan));
  plan.count = dxScanBandCount(87500, 108000, 100);
  plan.channel = dxScanBandChannel;
  plan.ctx = &walk;
  plan.dwellMs = DX_SCAN_DWELL_MS;
  dxScanReset(&scan);
  (void)dxScanStart(&scan, &plan, 106400);
  TEST_ASSERT_EQUAL(DX_SCAN_RUNNING, scan.state);
}

/* Walk on until the scan is sent to `khz`, a PI heard on 93.5 and 94.3 on
 * the way. Returns the time at the end. */
static uint32_t walkTo(uint32_t khz, uint32_t now) {
  while (scan.state == DX_SCAN_RUNNING && scan.atKHz != khz) {
    const uint32_t at = scan.atKHz;
    (void)dxScanPoll(&scan, now, at, false, false, false);
    if (at == 93500 || at == 94300) {
      (void)dxScanPoll(&scan, now, at, true, false, false);
    } else {
      now += DX_SCAN_DWELL_MS;
      (void)dxScanPoll(&scan, now, at, false, false, false);
    }
  }
  TEST_ASSERT_EQUAL_UINT32(khz, scan.atKHz);
  return now;
}

static void buildScan(const ScreenScanInputs *in) {
  screenScanStateBuild(in, &scanKeep, &sv);
}

static void an_idle_scanner_shows_ready_with_no_progress(void) {
  dxScanReset(&scan);
  dxCatchesReset(&catches);
  tuneTo(106400);
  readAt(412, 61);
  const ScreenScanInputs in = scanInputs();
  buildScan(&in);
  TEST_ASSERT_EQUAL(SCREEN_SCAN_IDLE, sv.state);
  TEST_ASSERT_EQUAL_STRING("0 found", sv.found);
  TEST_ASSERT_EQUAL_STRING("3/4", sv.position);
  TEST_ASSERT_EQUAL_STRING("07:17", sv.clock);
  TEST_ASSERT_NULL(sv.frequency);
  TEST_ASSERT_NULL(sv.left);
  TEST_ASSERT_FALSE(sv.hasProgress);
  TEST_ASSERT_NULL(sv.step);
  TEST_ASSERT_NULL(sv.pi);
  TEST_ASSERT_NULL(sv.level);
  TEST_ASSERT_FALSE(sv.stationOn);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_NO_STATION_YET), sv.note);
  TEST_ASSERT_EQUAL_STRING("2.5 s", sv.dwell);

  ScreenScanInputs none = in;
  none.pages = 0;
  none.page = 0;
  buildScan(&none);
  TEST_ASSERT_EQUAL_STRING("1/1", sv.position);
}

static void the_mode_and_rule_follow_the_dx_scanner_menu(void) {
  dxScanReset(&scan);
  tuneTo(106400);
  ScreenScanInputs in = scanInputs();

  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_MODE_BAND_MEMORY), sv.mode);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_RULE_STOP_ON_NEW), sv.rule);

  in.range = DX_RANGE_BAND;
  in.stop = DX_STOP_ANY_PI;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_MODE_WHOLE_BAND), sv.mode);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_RULE_STOP_ON_PI), sv.rule);

  in.range = DX_RANGE_MEMORY;
  in.memFirst = 3;
  in.memLast = 40;
  in.stop = DX_STOP_NEVER;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("PRESETS 3-40", sv.mode);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_RULE_NO_STOP), sv.rule);

  /* Learning the locals never stops, whatever the rule says. */
  in.learning = true;
  in.stop = DX_STOP_ANY_PI;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_MODE_LEARN_LOCALS), sv.mode);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_RULE_NO_STOP), sv.rule);

  /* The sweep a fresh scan starts with comes before all of them. */
  in.sweeping = true;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_MODE_SWEEPING), sv.mode);
}

static void the_dwell_is_cut_to_one_decimal_not_rounded(void) {
  dxScanReset(&scan);
  tuneTo(106400);
  ScreenScanInputs in = scanInputs();
  in.dwellMs = 500;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("0.5 s", sv.dwell);
  in.dwellMs = 2599;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("2.5 s", sv.dwell);
  in.dwellMs = 30000;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("30.0 s", sv.dwell);
}

static void the_range_is_mhz_for_a_band_walk_and_channels_for_presets(void) {
  dxScanReset(&scan);
  tuneTo(106400);
  ScreenScanInputs in = scanInputs();
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("87.5", sv.from);
  TEST_ASSERT_EQUAL_STRING("108.0", sv.to);

  in.lowKHz = 65900;
  in.highKHz = 74000;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("65.9", sv.from);
  TEST_ASSERT_EQUAL_STRING("74.0", sv.to);

  in.range = DX_RANGE_MEMORY;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("CH 1", sv.from);
  TEST_ASSERT_EQUAL_STRING("CH 99", sv.to);

  /* Learning walks the band, so the band's edges again. */
  in.learning = true;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("65.9", sv.from);
}

static void a_running_scan_shows_the_channel_the_dwell_left_and_progress(void) {
  dxCatchesReset(&catches);
  startScan();
  tuneTo(106400);
  ScreenScanInputs in = scanInputs();

  /* Just started: sent to 87.5, not there yet. */
  buildScan(&in);
  TEST_ASSERT_EQUAL(SCREEN_SCAN_RUNNING, sv.state);
  TEST_ASSERT_EQUAL_STRING("87.50", sv.frequency);
  TEST_ASSERT_EQUAL_STRING("2.5", sv.left);
  TEST_ASSERT_TRUE(sv.hasProgress);
  TEST_ASSERT_EQUAL_UINT16(0, sv.progressPermille);
  TEST_ASSERT_EQUAL_STRING("0 / 206", sv.step);

  /* On to 96.4, past a PI on 93.5 and on 94.3. */
  uint32_t now = walkTo(96400, 1000);
  tuneTo(96400);
  (void)dxScanPoll(&scan, now, 96400, false, false, false);
  in.nowMs = now + 800;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("2 found", sv.found);
  TEST_ASSERT_EQUAL_STRING("96.40", sv.frequency);
  TEST_ASSERT_EQUAL_STRING("1.7", sv.left);
  TEST_ASSERT_EQUAL_STRING("89 / 206", sv.step);
  TEST_ASSERT_EQUAL_UINT16(432, sv.progressPermille);

  /* A message takes the header while running, the count with it. */
  in.confirm = "Scanning";
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("Scanning", sv.position);
  TEST_ASSERT_NULL(sv.found);
  TEST_ASSERT_NULL(sv.clock);
  TEST_ASSERT_EQUAL_STRING("96.40", sv.frequency);

  /* A band that is not one has no frequency to show. */
  in.confirm = NULL;
  snap.settings.band = BAND_COUNT;
  buildScan(&in);
  TEST_ASSERT_NULL(sv.frequency);
}

static void a_running_scan_shows_the_tile_only_once_the_dial_has_landed(void) {
  startScan();
  tuneTo(106400);
  readAt(127, 40);
  snap.rds.hasPiHeard = true;
  snap.rds.piHeard = 0x63B2;
  snap.rds.piUnsureNibbles = 0x02;
  ScreenScanInputs in = scanInputs();

  /* What is heard before the dial lands is the last channel's. */
  buildScan(&in);
  TEST_ASSERT_NULL(sv.pi);
  TEST_ASSERT_NULL(sv.level);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_NO_RDS_YET), sv.note);

  snap.settings.freqKHz = 87500;
  (void)dxScanPoll(&scan, 0, 87500, false, false, false);
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("63?2", sv.pi);
  TEST_ASSERT_FALSE(sv.piSure);
  TEST_ASSERT_EQUAL_STRING("12.7", sv.level);
  TEST_ASSERT_FALSE(sv.stationOn);
  TEST_ASSERT_NULL(sv.note);

  /* Confirmed as the channel's own. */
  snap.rds.hasPi = true;
  snap.rds.pi = 0x63B2;
  snap.rds.piUnsureNibbles = 0;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("63B2", sv.pi);
  TEST_ASSERT_TRUE(sv.piSure);
  TEST_ASSERT_EQUAL_STRING("12.7", sv.level);
  TEST_ASSERT_FALSE(sv.stationOn);

  /* Nothing heard and no reading: no station, and while the scan runs it
   * says so. */
  memset(&snap.rds, 0, sizeof(snap.rds));
  snap.qualityValid = false;
  buildScan(&in);
  TEST_ASSERT_NULL(sv.pi);
  TEST_ASSERT_NULL(sv.level);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_NO_RDS_YET), sv.note);
}

/* 98.3 from the band sweep, caught for the first time. */
static void a_scan_stopped_on_a_catch_shows_the_station_and_new_pill(void) {
  const DxChannel *c = channelOf(kDxSweep, DX_SWEEP_COUNT, 98300);
  dxCatchesReset(&catches);
  startScan();
  const uint32_t now = walkTo(98300, 1000);
  (void)dxScanPoll(&scan, now, 98300, true, false, true);
  TEST_ASSERT_EQUAL(DX_SCAN_STOPPED, scan.state);
  tuneToCapture(c);
  hear(98300, snap.rds.pi, NULL, NULL, c->tuned.levelTenths, kMorningUtc, true);
  ScreenScanInputs in = scanInputs();
  in.nowMs = now;
  buildScan(&in);

  TEST_ASSERT_EQUAL(SCREEN_SCAN_STOPPED, sv.state);
  TEST_ASSERT_EQUAL_STRING("3 found", sv.found);
  TEST_ASSERT_EQUAL_STRING("98.30", sv.frequency);
  TEST_ASSERT_NULL(sv.left);
  TEST_ASSERT_TRUE(sv.hasProgress);
  TEST_ASSERT_EQUAL_STRING("108 / 206", sv.step);
  TEST_ASSERT_TRUE(sv.stationOn);
  TEST_ASSERT_EQUAL_STRING("26FF", sv.pi);
  TEST_ASSERT_TRUE(sv.piSure);
  TEST_ASSERT_EQUAL_STRING("MIRCHI", sv.ps);
  TEST_ASSERT_EQUAL_STRING("48.2", sv.level);
  TEST_ASSERT_TRUE(sv.isNew);

  /* Caught before: no pill. With no list at all: no pill either. */
  catches.item[0].isNew = false;
  buildScan(&in);
  TEST_ASSERT_TRUE(sv.stationOn);
  TEST_ASSERT_FALSE(sv.isNew);
  in.catches = NULL;
  catches.item[0].isNew = true;
  buildScan(&in);
  TEST_ASSERT_TRUE(sv.stationOn);
  TEST_ASSERT_FALSE(sv.isNew);

  /* No name yet. */
  snap.rds.hasPs = false;
  buildScan(&in);
  TEST_ASSERT_NULL(sv.ps);

  /* A message takes the header and leaves the station as it is. */
  in.confirm = "Logged";
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("Logged", sv.position);
  TEST_ASSERT_TRUE(sv.stationOn);
  TEST_ASSERT_EQUAL_STRING("26FF", sv.pi);
}

static void a_scan_stopped_by_a_key_or_turned_away_shows_no_station(void) {
  dxCatchesReset(&catches);
  startScan();
  tuneTo(87500);
  readAt(482, 61);
  snap.rds.hasPi = true;
  snap.rds.pi = 0x26FF;
  (void)dxScanPoll(&scan, 0, 87500, false, false, false);
  dxScanStop(&scan);
  ScreenScanInputs in = scanInputs();

  /* Stopped by a key on the channel: the tile, but not amber. */
  buildScan(&in);
  TEST_ASSERT_EQUAL(SCREEN_SCAN_STOPPED, sv.state);
  TEST_ASSERT_EQUAL_STRING("87.50", sv.frequency);
  TEST_ASSERT_EQUAL_STRING("26FF", sv.pi);
  TEST_ASSERT_EQUAL_STRING("48.2", sv.level);
  TEST_ASSERT_FALSE(sv.stationOn);
  TEST_ASSERT_NULL(sv.ps);

  /* The dial turned away: the frequency is the dial's, and the tile is
   * empty since what is heard there is not what the scan stopped on. */
  snap.settings.freqKHz = 87600;
  buildScan(&in);
  TEST_ASSERT_EQUAL_STRING("87.60", sv.frequency);
  TEST_ASSERT_NULL(sv.pi);
  TEST_ASSERT_NULL(sv.level);
  TEST_ASSERT_FALSE(sv.stationOn);
}

static void scanner_builder_leaves_everything_alone_on_null_inputs(void) {
  dxScanReset(&scan);
  tuneTo(106400);
  ScreenScanInputs in = scanInputs();
  FILL(sv);
  FILL(scanKeep);
  screenScanStateBuild(NULL, &scanKeep, &sv);
  screenScanStateBuild(&in, NULL, &sv);
  screenScanStateBuild(&in, &scanKeep, NULL);
  in.scan = NULL;
  screenScanStateBuild(&in, &scanKeep, &sv);
  in.scan = &scan;
  in.snap = NULL;
  screenScanStateBuild(&in, &scanKeep, &sv);
  TEST_ASSERT_TRUE(stillFilled(&sv, sizeof(sv)));
  TEST_ASSERT_TRUE(stillFilled(&scanKeep, sizeof(scanKeep)));
}

/* --------------------------------------------------------- Scope page -- */

static DxSweep live;
static DxSweep earlier;
static DxSweep base;
static DxSweep peak;
static ScreenScopeKeep scopeKeep;
static ScreenScope pv;
static const uint32_t kScopeNow = 1790511689u;

/* A recorded pass of the band as the level sweep takes it: the mean of
 * the four readings on each channel. */
static void sweepFromCapture(DxSweep *s, const int16_t (*pass)[CAPTURE_READS],
                             uint32_t at) {
  memset(s, 0, sizeof(*s));
  s->timeKnown = true;
  s->at = at;
  s->lowKHz = CAPTURE_LOW_KHZ;
  s->stepKHz = CAPTURE_STEP_KHZ;
  s->count = CAPTURE_COUNT;
  s->widthKHz = 114;
  for (uint16_t i = 0; i < CAPTURE_COUNT; i++) {
    s->level[i] = dxSweepMean(pass[i], CAPTURE_READS);
  }
}

/* The second pass is the latest sweep, three minutes old. It is held against
 * the first pass, taken five minutes before it. The dial is on 106.4. */
static ScreenScopeInputs realScope(void) {
  sweepFromCapture(&live, kPass1, kScopeNow - 180);
  sweepFromCapture(&earlier, kPass0, kScopeNow - 480);
  ScreenScopeInputs in;
  memset(&in, 0, sizeof(in));
  in.live = &live;
  in.baseN = dxSweepMedian(&earlier, 1, &live, &base);
  in.base = &base;
  memset(&peak, 0, sizeof(peak));
  dxSweepPeak(&peak, &earlier);
  dxSweepPeak(&peak, &live);
  in.peak = &peak;
  in.revision = 7;
  in.cursor = (uint16_t)dxSweepChannelOf(&live, 98300);
  in.dialKHz = 106400;
  in.nowKnown = true;
  in.nowUtc = kScopeNow;
  in.page = 1;
  in.pages = 4;
  in.clock = "17:43";
  return in;
}

/* Five channels from 87.5, a gap in the second, and a baseline with a gap
 * in the third. */
static ScreenScopeInputs smallScope(void) {
  static const int16_t kLive[5] = {100, DX_SWEEP_NO_READING, 300, 50, 200};
  static const int16_t kBase[5] = {96, 100, DX_SWEEP_NO_READING, 50, 204};
  memset(&live, 0, sizeof(live));
  live.lowKHz = 87500;
  live.stepKHz = 100;
  live.count = 5;
  live.widthKHz = 114;
  memcpy(live.level, kLive, sizeof(kLive));
  base = live;
  memcpy(base.level, kBase, sizeof(kBase));
  ScreenScopeInputs in;
  memset(&in, 0, sizeof(in));
  in.live = &live;
  in.base = &base;
  in.baseN = 3;
  in.dialKHz = 87500;
  return in;
}

static void buildScope(const ScreenScopeInputs *in) {
  screenScopeStateBuild(in, &scopeKeep, &pv);
}

static void no_sweep_asks_for_one(void) {
  ScreenScopeInputs in;
  memset(&in, 0, sizeof(in));
  buildScope(&in);
  TEST_ASSERT_EQUAL_UINT16(0, pv.count);
  TEST_ASSERT_NULL(pv.level);
  TEST_ASSERT_NULL(pv.context);
  TEST_ASSERT_EQUAL_STRING("1/1", pv.position);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_PRESS_TO_SWEEP), pv.empty);
  TEST_ASSERT_EQUAL_INT16(SCREEN_SCOPE_NONE, pv.floor);
  TEST_ASSERT_NULL(pv.floorText);
  TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, pv.dial);
  TEST_ASSERT_NULL(pv.cursorFreq);
  TEST_ASSERT_NULL(pv.from);

  /* A sweep with no channels is no sweep. */
  memset(&live, 0, sizeof(live));
  in.live = &live;
  buildScope(&in);
  TEST_ASSERT_EQUAL_UINT16(0, pv.count);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_PRESS_TO_SWEEP), pv.empty);

  /* The first sweep running: no box, and the header says it is running. */
  in.sweeping = true;
  buildScope(&in);
  TEST_ASSERT_NULL(pv.empty);
  TEST_ASSERT_TRUE(pv.sweeping);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_CONTEXT_SWEEPING), pv.context);
}

static void a_real_sweep_shows_bars_baseline_peak_and_floor(void) {
  const ScreenScopeInputs in = realScope();
  TEST_ASSERT_EQUAL_UINT8(1, in.baseN);
  buildScope(&in);

  TEST_ASSERT_EQUAL_STRING("3 min ago", pv.context);
  TEST_ASSERT_EQUAL_STRING("2/4", pv.position);
  TEST_ASSERT_EQUAL_STRING("17:43", pv.clock);
  TEST_ASSERT_EQUAL_UINT16(7, pv.revision);
  TEST_ASSERT_FALSE(pv.sweeping);
  TEST_ASSERT_NULL(pv.empty);
  TEST_ASSERT_EQUAL_UINT16(CAPTURE_COUNT, pv.count);
  TEST_ASSERT_EQUAL_PTR(live.level, pv.level);
  TEST_ASSERT_EQUAL_PTR(base.level, pv.base);
  TEST_ASSERT_EQUAL_PTR(peak.level, pv.peak);
  TEST_ASSERT_EQUAL_STRING("MEDIAN OF 1", pv.baseText);
  TEST_ASSERT_EQUAL_INT16(CAPTURE_FLOOR1, pv.floor);
  TEST_ASSERT_EQUAL_STRING("FLOOR -2.1", pv.floorText);
  TEST_ASSERT_EQUAL_UINT16(194, pv.dial);
  TEST_ASSERT_EQUAL_UINT16(113, pv.cursor);
  TEST_ASSERT_EQUAL_STRING("87.0", pv.from);
  TEST_ASSERT_EQUAL_STRING("97.5", pv.mid);
  TEST_ASSERT_EQUAL_STRING("108.0", pv.to);

  /* The cursor on 98.3, a station, 0.3 dB over its level before. */
  TEST_ASSERT_EQUAL_STRING("98.30", pv.cursorFreq);
  TEST_ASSERT_EQUAL_STRING("52.0", pv.cursorLevel);
  TEST_ASSERT_EQUAL_STRING("+0.3", pv.cursorRise);
  TEST_ASSERT_TRUE(pv.riseUp);

  /* On 99.5, an empty channel, 0.4 dB under. */
  ScreenScopeInputs empty = in;
  empty.cursor = (uint16_t)dxSweepChannelOf(&live, 99500);
  buildScope(&empty);
  TEST_ASSERT_EQUAL_STRING("99.50", pv.cursorFreq);
  TEST_ASSERT_EQUAL_STRING("-1.6", pv.cursorLevel);
  TEST_ASSERT_EQUAL_STRING("-0.4", pv.cursorRise);
  TEST_ASSERT_FALSE(pv.riseUp);
}

static void the_cursor_frequency_reads_in_mhz_with_two_decimals(void) {
  ScreenScopeInputs in = realScope();
  in.cursor = (uint16_t)dxSweepChannelOf(&live, 106400);
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING("106.40", pv.cursorFreq);
  TEST_ASSERT_EQUAL_STRING("50.3", pv.cursorLevel);
  TEST_ASSERT_EQUAL_STRING("+1.1", pv.cursorRise);

  /* Past the last channel the cursor stays on the last. */
  in.cursor = 500;
  buildScope(&in);
  TEST_ASSERT_EQUAL_UINT16(CAPTURE_COUNT - 1, pv.cursor);
  TEST_ASSERT_EQUAL_STRING("108.00", pv.cursorFreq);

  /* A dial between channels or below the first is on none of them. */
  in.dialKHz = 106450;
  buildScope(&in);
  TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, pv.dial);
  in.dialKHz = 86900;
  buildScope(&in);
  TEST_ASSERT_EQUAL_UINT16(UINT16_MAX, pv.dial);
}

static void a_gap_in_the_sweep_shows_no_level_and_no_rise(void) {
  ScreenScopeInputs in = smallScope();
  in.cursor = 1;
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING("87.60", pv.cursorFreq);
  TEST_ASSERT_NULL(pv.cursorLevel);
  TEST_ASSERT_NULL(pv.cursorRise);
  TEST_ASSERT_FALSE(pv.riseUp);

  /* A gap in the baseline: the level, but no rise. */
  in.cursor = 2;
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING("30.0", pv.cursorLevel);
  TEST_ASSERT_NULL(pv.cursorRise);

  /* The floor is taken over the channels that had a reading. */
  TEST_ASSERT_EQUAL_INT16(50, pv.floor);
  TEST_ASSERT_EQUAL_STRING("FLOOR 5.0", pv.floorText);
  TEST_ASSERT_EQUAL_STRING("87.5", pv.from);
  TEST_ASSERT_EQUAL_STRING("87.7", pv.mid);
  TEST_ASSERT_EQUAL_STRING("87.9", pv.to);
  TEST_ASSERT_EQUAL_UINT16(0, pv.dial);

  /* A sweep with no reading at all has no floor. */
  for (int i = 0; i < 5; i++) {
    live.level[i] = DX_SWEEP_NO_READING;
  }
  buildScope(&in);
  TEST_ASSERT_EQUAL_INT16(SCREEN_SCOPE_NONE, pv.floor);
  TEST_ASSERT_NULL(pv.floorText);
  TEST_ASSERT_NULL(pv.cursorLevel);
}

static void the_rise_has_a_sign_only_when_it_is_above_the_baseline(void) {
  ScreenScopeInputs in = smallScope();
  in.cursor = 0;
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING("10.0", pv.cursorLevel);
  TEST_ASSERT_EQUAL_STRING("+0.4", pv.cursorRise);
  TEST_ASSERT_TRUE(pv.riseUp);

  in.cursor = 3;
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING("0.0", pv.cursorRise);
  TEST_ASSERT_FALSE(pv.riseUp);

  in.cursor = 4;
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING("-0.4", pv.cursorRise);
  TEST_ASSERT_FALSE(pv.riseUp);
}

static void a_baseline_or_peak_on_other_channels_is_left_out(void) {
  ScreenScopeInputs in = smallScope();
  in.cursor = 0;
  peak = live;
  in.peak = &peak;
  in.baseFixed = true;
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING("FIXED", pv.baseText);
  TEST_ASSERT_EQUAL_PTR(base.level, pv.base);
  TEST_ASSERT_EQUAL_PTR(peak.level, pv.peak);

  /* Read at another width, or over fewer channels. */
  base.widthKHz = 217;
  peak.count = 4;
  buildScope(&in);
  TEST_ASSERT_NULL(pv.base);
  TEST_ASSERT_NULL(pv.baseText);
  TEST_ASSERT_NULL(pv.cursorRise);
  TEST_ASSERT_NULL(pv.peak);
  TEST_ASSERT_EQUAL_STRING("10.0", pv.cursorLevel);

  /* A baseline from no sweeps is none. */
  base.widthKHz = 114;
  in.baseN = 0;
  in.baseFixed = false;
  buildScope(&in);
  TEST_ASSERT_NULL(pv.base);
  TEST_ASSERT_NULL(pv.baseText);
}

static void the_age_shows_only_when_both_times_are_known(void) {
  ScreenScopeInputs in = realScope();
  in.nowUtc = live.at + 30;
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_NOW), pv.context);

  in.nowKnown = false;
  buildScope(&in);
  TEST_ASSERT_NULL(pv.context);

  in.nowKnown = true;
  live.timeKnown = false;
  buildScope(&in);
  TEST_ASSERT_NULL(pv.context);

  /* The clock moved back past the sweep: its age is not known. */
  live.timeKnown = true;
  in.nowUtc = live.at - 1;
  buildScope(&in);
  TEST_ASSERT_NULL(pv.context);

  /* Sweeping says so over the old sweep, which stays drawn. */
  in.sweeping = true;
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING(txt(STR_DX_CONTEXT_SWEEPING), pv.context);
  TEST_ASSERT_EQUAL_UINT16(CAPTURE_COUNT, pv.count);
  TEST_ASSERT_NULL(pv.empty);

  /* A message takes the whole of the header's run, so "Sweeping" does not
   * stand beside "sweeping". */
  in.confirm = "Sweeping";
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING("Sweeping", pv.position);
  TEST_ASSERT_NULL(pv.context);
  TEST_ASSERT_NULL(pv.clock);
}

/* The touch buttons are drawn with Touch On only. */
static void the_scope_buttons_follow_the_touch_setting(void) {
  ScreenScopeInputs in;
  memset(&in, 0, sizeof(in));
  buildScope(&in);
  TEST_ASSERT_FALSE(pv.buttons);
  in.touchOn = true;
  buildScope(&in);
  TEST_ASSERT_TRUE(pv.buttons);
}

/* The band scope's page: its own title and span in the header, the marks
 * passed through, and no noise floor for a span, which is not the band. */
static void the_band_scope_has_its_title_span_and_marks(void) {
  static const uint16_t kMarks[2] = {3, 194};
  static const uint16_t kCatch[1] = {50};
  ScreenScopeInputs in = realScope();
  buildScope(&in);
  TEST_ASSERT_NULL(pv.title);
  TEST_ASSERT_EQUAL_STRING("2/4", pv.position);
  TEST_ASSERT_NOT_EQUAL(SCREEN_SCOPE_NONE, pv.floor);
  in.title = "FM Scope";
  in.position = "Full";
  in.marks = kMarks;
  in.markCount = 2;
  in.catches = kCatch;
  in.catchCount = 1;
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING("FM Scope", pv.title);
  TEST_ASSERT_EQUAL_STRING("Full", pv.position);
  TEST_ASSERT_EQUAL_PTR(kMarks, pv.marks);
  TEST_ASSERT_EQUAL_UINT8(2, pv.markCount);
  TEST_ASSERT_EQUAL_PTR(kCatch, pv.catches);
  TEST_ASSERT_EQUAL_UINT8(1, pv.catchCount);
  TEST_ASSERT_NOT_EQUAL(SCREEN_SCOPE_NONE, pv.floor);
  in.span = true;
  in.position = "3.6 MHz";
  buildScope(&in);
  TEST_ASSERT_EQUAL_STRING("3.6 MHz", pv.position);
  TEST_ASSERT_EQUAL_INT16(SCREEN_SCOPE_NONE, pv.floor);
  TEST_ASSERT_NULL(pv.floorText);
}

/* A medium wave sweep off the radio: the axis and the cursor in whole kHz,
 * with the noise floor of the whole band. */
static void an_am_scope_reads_in_khz(void) {
  memset(&live, 0, sizeof(live));
  live.lowKHz = CAPTURE_MW_LOW_KHZ;
  live.stepKHz = CAPTURE_MW_STEP_KHZ;
  live.count = CAPTURE_MW_COUNT;
  memcpy(live.level, kMw, sizeof(kMw));
  ScreenScopeInputs in;
  memset(&in, 0, sizeof(in));
  in.live = &live;
  in.am = true;
  in.title = "MW Scope";
  in.position = "Full";
  in.dialKHz = 738;
  in.cursor = (uint16_t)dxSweepChannelOf(&live, 738);
  buildScope(&in);
  TEST_ASSERT_TRUE(pv.am);
  TEST_ASSERT_EQUAL_STRING("522", pv.from);
  TEST_ASSERT_EQUAL_STRING("1156", pv.mid);
  TEST_ASSERT_EQUAL_STRING("1791", pv.to);
  TEST_ASSERT_EQUAL_STRING("738", pv.cursorFreq);
  TEST_ASSERT_EQUAL_STRING("32.0", pv.cursorLevel);
  TEST_ASSERT_EQUAL_UINT16(24, pv.dial);
  TEST_ASSERT_NOT_EQUAL(SCREEN_SCOPE_NONE, pv.floor);
  /* The same sweep on FM's terms is not what the page shows. */
  in.am = false;
  buildScope(&in);
  TEST_ASSERT_FALSE(pv.am);
  TEST_ASSERT_EQUAL_STRING("0.5", pv.from);
}

static void scope_builder_leaves_everything_alone_on_null_inputs(void) {
  const ScreenScopeInputs in = realScope();
  FILL(pv);
  FILL(scopeKeep);
  screenScopeStateBuild(NULL, &scopeKeep, &pv);
  screenScopeStateBuild(&in, NULL, &pv);
  screenScopeStateBuild(&in, &scopeKeep, NULL);
  TEST_ASSERT_TRUE(stillFilled(&pv, sizeof(pv)));
  TEST_ASSERT_TRUE(stillFilled(&scopeKeep, sizeof(scopeKeep)));
}

/* ------------------------------------------------ the header's message */

/* No DX page has a line of hints, so a moment's message, such as what a log
 * hold did, is handed to the header: in place of the page, with the clock
 * and the context left out, on all four pages. When it goes, they come
 * back. */
static void a_message_takes_the_header_while_it_shows(void) {
  const char *const logged = "Logged 106.40";

  tuneTo(106400);
  ScreenDxInputs dxIn = dxInputs();
  dxIn.confirm = logged;
  buildDx(&dxIn);
  TEST_ASSERT_EQUAL_PTR(logged, dx.position);
  TEST_ASSERT_NULL(dx.clock);
  TEST_ASSERT_EQUAL_STRING("106.40", dx.frequency);
  dxIn.confirm = NULL;
  buildDx(&dxIn);
  TEST_ASSERT_EQUAL_STRING("1/4", dx.position);
  TEST_ASSERT_EQUAL_STRING("07:17", dx.clock);

  ScreenScopeInputs scopeIn = realScope();
  scopeIn.confirm = logged;
  buildScope(&scopeIn);
  TEST_ASSERT_EQUAL_PTR(logged, pv.position);
  TEST_ASSERT_NULL(pv.context);
  TEST_ASSERT_NULL(pv.clock);
  TEST_ASSERT_EQUAL_STRING("98.30", pv.cursorFreq);
  scopeIn.confirm = NULL;
  buildScope(&scopeIn);
  TEST_ASSERT_EQUAL_STRING("2/4", pv.position);
  TEST_ASSERT_EQUAL_STRING("3 min ago", pv.context);
  TEST_ASSERT_EQUAL_STRING("17:43", pv.clock);

  dxScanReset(&scan);
  dxCatchesReset(&catches);
  ScreenScanInputs scanIn = scanInputs();
  scanIn.confirm = logged;
  buildScan(&scanIn);
  TEST_ASSERT_EQUAL_PTR(logged, sv.position);
  TEST_ASSERT_NULL(sv.found);
  TEST_ASSERT_NULL(sv.clock);
  scanIn.confirm = NULL;
  buildScan(&scanIn);
  TEST_ASSERT_EQUAL_STRING("3/4", sv.position);
  TEST_ASSERT_EQUAL_STRING("0 found", sv.found);
  TEST_ASSERT_EQUAL_STRING("07:17", sv.clock);

  hearMany(3);
  buildCatches(0, logged);
  TEST_ASSERT_EQUAL_PTR(logged, cv.position);
  TEST_ASSERT_NULL(cv.range);
  TEST_ASSERT_NULL(cv.clock);
  TEST_ASSERT_EQUAL_UINT8(3, cv.rows);
  buildCatches(0, NULL);
  TEST_ASSERT_EQUAL_STRING("4/4", cv.position);
  TEST_ASSERT_EQUAL_STRING("1-3 of 3", cv.range);
  TEST_ASSERT_EQUAL_STRING("07:39", cv.clock);
}

/* ------------------------------------------- the offset and noise held */

/* One build per tuner reading, the read count moving each time, as the
 * screen task sees it. */
static void readingIn(const ScreenDxInputs *in, int16_t offset, uint16_t usn) {
  snap.qualityValid = true;
  snap.quality.offsetKHzTenths = offset;
  snap.quality.usnTenths = usn;
  snap.qualityReads++;
  screenDxStateBuild(in, &dxKeep, &dx);
}

/* Small moves around the reading shown leave it where it is; a move past the
 * hysteresis is followed. */
static void the_offset_holds_through_small_moves_and_follows_a_real_one(void) {
  tuneTo(106400);
  ScreenDxInputs in = dxInputs();
  screenDxStateReset(&dxKeep);
  /* The first reading on a newly opened page is shown as it is, and the hold
   * starts from the read after it. */
  readingIn(&in, 60, 20);
  readingIn(&in, 60, 20);
  TEST_ASSERT_EQUAL_STRING("+6", dx.offset);
  const int16_t wobble[] = {72, 48, 66, 55, 70, 51, 64};
  for (size_t i = 0; i < sizeof(wobble) / sizeof(wobble[0]); i++) {
    readingIn(&in, wobble[i], 20);
    TEST_ASSERT_EQUAL_STRING("+6", dx.offset);
  }
  for (int i = 0; i < 30; i++) {
    readingIn(&in, 120, 20);
  }
  TEST_ASSERT_EQUAL_STRING("+12", dx.offset);
}

/* The page is built on every poll; the same reading built again is not fed
 * again, so a reading that sits still is not held twice as hard. */
static void the_same_reading_is_fed_once(void) {
  tuneTo(106400);
  ScreenDxInputs in = dxInputs();
  screenDxStateReset(&dxKeep);
  readingIn(&in, 60, 20);
  for (int i = 0; i < 30; i++) {
    readingIn(&in, 120, 20);
  }
  char after[8];
  snprintf(after, sizeof(after), "%s", dx.offset);
  /* Many builds with no new reading change nothing. */
  snap.quality.offsetKHzTenths = -200;
  for (int i = 0; i < 50; i++) {
    screenDxStateBuild(&in, &dxKeep, &dx);
  }
  TEST_ASSERT_EQUAL_STRING(after, dx.offset);
}

/* A new channel's first reading is shown as it is, not pulled towards the
 * last station's. */
static void a_retune_starts_the_holds_again(void) {
  tuneTo(106400);
  ScreenDxInputs in = dxInputs();
  screenDxStateReset(&dxKeep);
  for (int i = 0; i < 30; i++) {
    readingIn(&in, 60, 20);
  }
  /* The dial moves first; the snapshot still carries the old station's
   * reading, which is shown as it is and does not start the holds. */
  snap.settings.freqKHz = 98300;
  screenDxStateBuild(&in, &dxKeep, &dx);
  TEST_ASSERT_EQUAL_STRING("+6", dx.offset);
  /* The radio's next read is the new station's, and the holds start there. */
  readingIn(&in, -40, 300);
  TEST_ASSERT_EQUAL_STRING("-4", dx.offset);
  TEST_ASSERT_EQUAL_STRING("30", dx.usn);
  for (int i = 0; i < 5; i++) {
    readingIn(&in, -45, 310);
  }
  TEST_ASSERT_EQUAL_STRING("-4", dx.offset);
}

/* Recorded music on 106.4, ten readings a second: raw, the offset shown
 * changes on most readings; held, it settles. */
static void the_music_capture_settles_the_offset_and_the_noise(void) {
  tuneTo(106400);
  ScreenDxInputs in = dxInputs();
  screenDxStateReset(&dxKeep);
  const MeterCapture *c = k_music_106400_2026_09_15;
  char last[8] = "";
  char lastUsn[8] = "";
  int offsetChanges = 0;
  int usnChanges = 0;
  for (int i = 0; i < 300; i++) {
    in.nowMs = c[i].ms;
    readingIn(&in, c[i].off, c[i].usn);
    if (i > 0 && strcmp(last, dx.offset) != 0) {
      offsetChanges++;
    }
    if (i > 0 && strcmp(lastUsn, dx.usn) != 0) {
      usnChanges++;
    }
    snprintf(last, sizeof(last), "%s", dx.offset);
    snprintf(lastUsn, sizeof(lastUsn), "%s", dx.usn);
  }
  /* About five changes in thirty seconds or fewer for a clean station, the
   * rate the limits were chosen for. */
  TEST_ASSERT_TRUE(offsetChanges <= 5);
  TEST_ASSERT_TRUE(usnChanges <= 5);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(a_real_station_shows_its_readings_name_and_confirmed_pi);
  RUN_TEST(the_station_next_door_is_only_seen_never_confirmed);
  RUN_TEST(the_pi_tile_goes_from_none_to_seen_to_confirmed);
  RUN_TEST(a_confirmed_pi_names_its_country_only_with_an_ecc);
  RUN_TEST(north_america_shows_call_letters_in_place_of_the_country);
  RUN_TEST(a_preset_with_a_stored_pi_names_itself_under_the_pi);
  RUN_TEST(block_errors_are_minus_one_until_a_group_arrives);
  RUN_TEST(the_name_shows_each_character_as_it_arrives);
  RUN_TEST(rds_switched_off_leaves_the_rds_fields_empty);
  RUN_TEST(readings_are_formatted_with_sign_rounding_and_offset);
  RUN_TEST(no_reading_leaves_the_six_readings_empty);
  RUN_TEST(the_history_keeps_the_peak_of_each_second_and_gaps_stay_empty);
  RUN_TEST(a_retune_starts_the_history_again);
  RUN_TEST(the_header_follows_the_inputs);
  RUN_TEST(a_band_that_is_not_one_has_no_frequency);
  RUN_TEST(dx_builders_leave_everything_alone_on_null_inputs);
  RUN_TEST(an_empty_catches_list_shows_no_rows);
  RUN_TEST(a_long_list_shows_the_six_rows_that_hold_the_cursor);
  RUN_TEST(a_cursor_past_the_end_goes_to_the_oldest);
  RUN_TEST(a_catch_row_shows_time_frequency_pi_name_country_and_level);
  RUN_TEST(a_catch_count_past_99_says_99_plus);
  RUN_TEST(a_full_list_keeps_the_newest_and_shows_the_oldest_kept_last);
  RUN_TEST(a_catch_with_a_bad_band_or_clock_offset_leaves_those_empty);
  RUN_TEST(catches_builder_leaves_everything_alone_on_null_keep_or_out);
  RUN_TEST(an_idle_scanner_shows_ready_with_no_progress);
  RUN_TEST(the_mode_and_rule_follow_the_dx_scanner_menu);
  RUN_TEST(the_dwell_is_cut_to_one_decimal_not_rounded);
  RUN_TEST(the_range_is_mhz_for_a_band_walk_and_channels_for_presets);
  RUN_TEST(a_running_scan_shows_the_channel_the_dwell_left_and_progress);
  RUN_TEST(a_running_scan_shows_the_tile_only_once_the_dial_has_landed);
  RUN_TEST(a_scan_stopped_on_a_catch_shows_the_station_and_new_pill);
  RUN_TEST(a_scan_stopped_by_a_key_or_turned_away_shows_no_station);
  RUN_TEST(scanner_builder_leaves_everything_alone_on_null_inputs);
  RUN_TEST(no_sweep_asks_for_one);
  RUN_TEST(a_real_sweep_shows_bars_baseline_peak_and_floor);
  RUN_TEST(the_cursor_frequency_reads_in_mhz_with_two_decimals);
  RUN_TEST(a_gap_in_the_sweep_shows_no_level_and_no_rise);
  RUN_TEST(the_rise_has_a_sign_only_when_it_is_above_the_baseline);
  RUN_TEST(a_baseline_or_peak_on_other_channels_is_left_out);
  RUN_TEST(the_age_shows_only_when_both_times_are_known);
  RUN_TEST(the_scope_buttons_follow_the_touch_setting);
  RUN_TEST(the_band_scope_has_its_title_span_and_marks);
  RUN_TEST(an_am_scope_reads_in_khz);
  RUN_TEST(scope_builder_leaves_everything_alone_on_null_inputs);
  RUN_TEST(a_message_takes_the_header_while_it_shows);
  RUN_TEST(the_offset_holds_through_small_moves_and_follows_a_real_one);
  RUN_TEST(the_same_reading_is_fed_once);
  RUN_TEST(a_retune_starts_the_holds_again);
  RUN_TEST(the_music_capture_settles_the_offset_and_the_noise);
  return UNITY_END();
}
