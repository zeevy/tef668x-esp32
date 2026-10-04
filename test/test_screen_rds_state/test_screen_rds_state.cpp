/*
 * Tests for the RDS screen's view, built from the decoder's state. Runs on a
 * PC.
 *
 * The station, its text and the last minute come from real groups taken off
 * air from this radio, fed through the decoder. None of the captures holds
 * an ECC, RT+, alternative frequencies or other networks, so those stations
 * are written by hand.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/memory.h"
#include "core/rds.h"
#include "core/rds_country.h"
#include "core/strings.h"
#include "screen_rds_state.h"

#include "../test_rds/captures.h"

#define STRONG_106400 "fm-106400-2026-09-13.log"
#define WEAK_106400 "fm-106400-weak-2026-09-13.log"
#define STRONG_94300 "fm-94300-2026-09-13.log"
#define STRONG_95000 "fm-95000-2026-09-13.log"

/* The separator the page puts between two values. */
#define DOT " \xC2\xB7 "

static Rds rds;
static RdsInfo info;
static ScreenRds view;

void setUp(void) {
  rdsReset(&rds, 106400);
  memset(&info, 0, sizeof(info));
  memset(&view, 0, sizeof(view));
}
void tearDown(void) {}

/* Feed the first `upTo` groups of the named recording, at the standard's
 * 11.4 groups a second, so the last minute covers what it would on air. */
static void replay(const char *name, uint16_t upTo) {
  const Capture *cap = NULL;
  for (int i = 0; i < CAPTURE_COUNT; i++) {
    if (strcmp(kCaptures[i].name, name) == 0) {
      cap = &kCaptures[i];
    }
  }
  TEST_ASSERT_NOT_NULL_MESSAGE(cap, "no capture by that name");
  rdsReset(&rds, cap->khz);
  for (uint16_t i = 0; i < upTo && i < cap->count; i++) {
    RdsRead r;
    memset(&r, 0, sizeof(r));
    r.atMs = (uint32_t)(i * 1000u / 11.4);
    r.synchronised = true;
    r.haveGroup = true;
    for (int b = 0; b < 4; b++) {
      r.block[b] = cap->groups[i].block[b];
      r.error[b] = (uint8_t)((cap->groups[i].error >> (6 - b * 2)) & 0x03);
    }
    rdsFeed(&rds, &r);
  }
}

/* What the screen task hands over: page 1, Europe, on no preset. */
static ScreenRdsInputs inputs(const RdsInfo *r) {
  ScreenRdsInputs in;
  memset(&in, 0, sizeof(in));
  in.rds = r;
  in.frequency = "106.40 MHz";
  in.clock = "06:31";
  in.sync = "41m10s";
  in.syncGood = true;
  in.region = RDS_REGION_EUROPE;
  in.presetSlot = MEMORY_NO_SLOT;
  return in;
}

static void build(const RdsInfo *r) {
  const ScreenRdsInputs in = inputs(r);
  screenRdsStateBuild(&in, &view);
}

/* A station with a confirmed PI and the rest of page 1 sent. */
static void station(RdsInfo *r) {
  memset(r, 0, sizeof(*r));
  r->synchronised = true;
  r->hasPi = true;
  r->pi = 0xC241;
  r->hasPs = true;
  strcpy(r->ps, "RADIO 1 ");
  r->hasPty = true;
  r->pty = 10;
}

/* A radio text with three RT+ tags, sent in the wrong order. */
static void withRtPlus(RdsInfo *r) {
  r->hasRt = true;
  strcpy(r->rt,
         "NOW PLAYING: DREAMS BY FLEETWOOD MAC ON RADIO 1, YOUR HIT MUSIC");
  r->rtPlus = true;
  r->rtPlusRunning = true;
  r->rtPlusCount = 3;
  r->rtPlusTag[0] = {33, 54, 9}; /* HIT MUSIC. */
  r->rtPlusTag[1] = {4, 23, 13}; /* FLEETWOOD MAC. */
  r->rtPlusTag[2] = {1, 13, 6};  /* DREAMS. */
}

/* Another network, named often enough to be listed. */
static RdsEon *addEon(RdsInfo *r, uint16_t pi) {
  RdsEon *e = &r->eon[r->eonCount++];
  memset(e, 0, sizeof(*e));
  e->pi = pi;
  e->heard = 9;
  return e;
}

/* ------------------------------------------------------- guarded inputs */

static void no_view_to_fill_does_not_crash(void) {
  const ScreenRdsInputs in = inputs(&info);
  screenRdsStateBuild(&in, NULL);
  screenRdsStateBuild(NULL, NULL);
}

/* A caller that draws it anyway draws dashes, not what was there before. */
static void no_inputs_or_no_decoder_leaves_the_view_cleared(void) {
  ScreenRds zero;
  memset(&zero, 0, sizeof(zero));

  memset(&view, 0xA5, sizeof(view));
  screenRdsStateBuild(NULL, &view);
  TEST_ASSERT_EQUAL_MEMORY(&zero, &view, sizeof(view));

  ScreenRdsInputs in = inputs(NULL);
  memset(&view, 0xA5, sizeof(view));
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_MEMORY(&zero, &view, sizeof(view));
}

static void the_header_fields_are_passed_on(void) {
  ScreenRdsInputs in = inputs(&info);
  in.page = 3;
  in.syncGood = false;
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_UINT8(3, view.page);
  TEST_ASSERT_EQUAL_STRING("106.40 MHz", view.frequency);
  TEST_ASSERT_EQUAL_STRING("06:31", view.clock);
  TEST_ASSERT_EQUAL_STRING("41m10s", view.sync);
  TEST_ASSERT_FALSE(view.syncGood);
}

/* --------------------------------------------------- nothing heard yet */

static void an_empty_decoder_shows_nothing_as_heard(void) {
  build(&rds.info);
  /* Page 1. */
  TEST_ASSERT_NULL(view.ps);
  TEST_ASSERT_NULL(view.pi);
  TEST_ASSERT_FALSE(view.piSure);
  TEST_ASSERT_NULL(view.area);
  TEST_ASSERT_NULL(view.ptyNumber);
  TEST_ASSERT_NULL(view.ptyName);
  /* No reason either, since there is no PI to have a country. */
  TEST_ASSERT_NULL(view.country);
  TEST_ASSERT_FALSE(view.countryNamed);
  TEST_ASSERT_NULL(view.ecc);
  TEST_ASSERT_NULL(view.ptyn);
  TEST_ASSERT_NULL(view.language);
  TEST_ASSERT_NULL(view.ct);
  TEST_ASSERT_EQUAL(SCREEN_RDS_UNKNOWN, view.tp);
  TEST_ASSERT_EQUAL(SCREEN_RDS_UNKNOWN, view.ta);
  TEST_ASSERT_EQUAL(SCREEN_RDS_UNKNOWN, view.speech);
  TEST_ASSERT_EQUAL(SCREEN_RDS_UNKNOWN, view.stereo);
  /* Page 2. */
  TEST_ASSERT_NULL(view.text);
  TEST_ASSERT_EQUAL_UINT8(0, view.tagCount);
  TEST_ASSERT_FALSE(view.rtPlusRunning);
  TEST_ASSERT_EQUAL_STRING("No RT+ heard from this station", view.rtPlusNote);
  /* Page 3. */
  TEST_ASSERT_EQUAL_UINT8(0, view.afCount);
  TEST_ASSERT_EQUAL_UINT8(0, view.afMore);
  TEST_ASSERT_EQUAL_UINT8(0, view.eonCount);
  TEST_ASSERT_EQUAL_UINT8(0, view.eonMore);
  TEST_ASSERT_FALSE(view.eonMoreHeard);
  /* Page 4. */
  TEST_ASSERT_EQUAL_STRING("106.40 MHz" DOT "last 0 s", view.span);
  TEST_ASSERT_NULL(view.rate);
  TEST_ASSERT_NULL(view.bler);
  TEST_ASSERT_NULL(view.lost);
  TEST_ASSERT_FALSE(view.blocksKnown);
  for (int b = 0; b < 4; b++) {
    TEST_ASSERT_NULL(view.block[b].text);
  }
  TEST_ASSERT_EQUAL_UINT8(0, view.groupCount);
  TEST_ASSERT_EQUAL_STRING("No groups in the last minute", view.groupNote);
}

/* ----------------------------------------------------- page 1, on air */

static void magic_fm_shows_its_name_pi_and_programme_type(void) {
  replay(STRONG_106400, 160);
  build(&rds.info);
  /* Without the spaces the station centres it with. */
  TEST_ASSERT_EQUAL_STRING("MAGIC", view.ps);
  TEST_ASSERT_EQUAL_STRING("1064", view.pi);
  TEST_ASSERT_TRUE(view.piSure);
  TEST_ASSERT_EQUAL_STRING("Local", view.area);
  TEST_ASSERT_EQUAL_STRING("PTY 12", view.ptyNumber);
  TEST_ASSERT_EQUAL_STRING("Easy Listening", view.ptyName);
  /* No ECC, so no country, and the reason in its place. */
  TEST_ASSERT_NULL(view.ecc);
  TEST_ASSERT_EQUAL_STRING("no ECC heard", view.country);
  TEST_ASSERT_FALSE(view.countryNamed);
  TEST_ASSERT_FALSE(view.countryGuess);
  TEST_ASSERT_FALSE(view.countryFault);
  TEST_ASSERT_EQUAL(SCREEN_RDS_NO, view.tp);
  TEST_ASSERT_EQUAL(SCREEN_RDS_NO, view.ta);
  TEST_ASSERT_EQUAL(SCREEN_RDS_YES, view.speech);
  TEST_ASSERT_EQUAL(SCREEN_RDS_YES, view.stereo);
}

/* One clean block A is heard but not yet confirmed: drawn dimmed, and no
 * area or country is named for it. */
static void a_pi_heard_once_is_shown_unsure(void) {
  replay(STRONG_106400, 1);
  build(&rds.info);
  TEST_ASSERT_EQUAL_STRING("1064", view.pi);
  TEST_ASSERT_FALSE(view.piSure);
  TEST_ASSERT_NULL(view.area);
  TEST_ASSERT_NULL(view.country);
  TEST_ASSERT_NULL(view.ps);
}

static void a_pi_whose_digits_changed_shows_question_marks(void) {
  info.hasPiHeard = true;
  info.piHeard = 0x1064;
  info.piUnsureNibbles = 0x04;
  build(&info);
  TEST_ASSERT_EQUAL_STRING("1?64", view.pi);
  TEST_ASSERT_FALSE(view.piSure);
}

/* Fever FM sends 0000 in every group. That is what it sends, not a dash,
 * and it has no area since it is not an identifier. */
static void a_station_sending_zero_shows_it_with_no_area(void) {
  replay(STRONG_94300, 160);
  build(&rds.info);
  TEST_ASSERT_EQUAL_STRING("FEVER FM", view.ps);
  TEST_ASSERT_EQUAL_STRING("0000", view.pi);
  TEST_ASSERT_TRUE(view.piSure);
  TEST_ASSERT_NULL(view.area);
  TEST_ASSERT_EQUAL_STRING("no ECC heard", view.country);
  TEST_ASSERT_EQUAL(SCREEN_RDS_YES, view.tp);
}

/* Mirchi 95 splits its name over two passes, and the page shows both. */
static void a_joined_name_is_shown_in_place_of_one_pass(void) {
  replay(STRONG_95000, 533);
  TEST_ASSERT_TRUE(rds.info.hasPsLong);
  build(&rds.info);
  TEST_ASSERT_EQUAL_STRING(rds.info.psLong, view.ps);
  TEST_ASSERT_NOT_EQUAL(0, strcmp(rds.info.ps, view.ps));
}

static void the_other_station_fields_are_shown_when_sent(void) {
  station(&info);
  info.hasPtyn = true;
  strcpy(info.ptyn, "POP HITS");
  info.hasLanguage = true;
  info.language = 0x09;
  info.clock.valid = true;
  info.clock.hour = 7;
  info.clock.minute = 4;
  info.hasFlags = true;
  info.tp = true;
  info.ta = true;
  info.speech = false;
  info.hasDiStereo = true;
  info.diStereo = false;
  build(&info);
  TEST_ASSERT_EQUAL_STRING("RADIO 1", view.ps);
  TEST_ASSERT_EQUAL_STRING("C241", view.pi);
  TEST_ASSERT_EQUAL_STRING("PTY 10", view.ptyNumber);
  TEST_ASSERT_EQUAL_STRING(txt(STR_PTY_POP_MUSIC), view.ptyName);
  TEST_ASSERT_EQUAL_STRING("POP HITS", view.ptyn);
  TEST_ASSERT_EQUAL_STRING("English", view.language);
  TEST_ASSERT_EQUAL_STRING("07:04", view.ct);
  TEST_ASSERT_EQUAL(SCREEN_RDS_YES, view.tp);
  TEST_ASSERT_EQUAL(SCREEN_RDS_YES, view.ta);
  TEST_ASSERT_EQUAL(SCREEN_RDS_NO, view.speech);
  TEST_ASSERT_EQUAL(SCREEN_RDS_NO, view.stereo);
}

/* ------------------------------------------------------------ country */

static void the_ecc_and_the_pi_name_the_country(void) {
  station(&info);
  info.hasEcc = true;
  info.ecc = 0xE1;
  build(&info);
  TEST_ASSERT_EQUAL_STRING("E1", view.ecc);
  TEST_ASSERT_EQUAL_STRING("GB", view.country);
  TEST_ASSERT_TRUE(view.countryNamed);
  TEST_ASSERT_FALSE(view.countryGuess);
}

/* An ECC whose cell for this PI is empty says so, not "no ECC". */
static void an_ecc_with_no_country_for_it_says_not_listed(void) {
  station(&info);
  info.pi = 0xE123;
  info.hasEcc = true;
  info.ecc = 0xE0;
  build(&info);
  TEST_ASSERT_EQUAL_STRING("E0", view.ecc);
  TEST_ASSERT_EQUAL_STRING("not listed", view.country);
  TEST_ASSERT_FALSE(view.countryNamed);
}

/* The country is the PI and the ECC together, so an unsure PI names none. */
static void an_ecc_beside_an_unsure_pi_names_no_country(void) {
  info.hasPiHeard = true;
  info.piHeard = 0xC241;
  info.hasEcc = true;
  info.ecc = 0xE1;
  build(&info);
  TEST_ASSERT_EQUAL_STRING("E1", view.ecc);
  TEST_ASSERT_FALSE(view.piSure);
  TEST_ASSERT_NULL(view.country);
  TEST_ASSERT_FALSE(view.countryNamed);
}

/* --------------------------------------------------------- the region */

/* The same PI read on Europe names an area and the European programme type,
 * and on North America gives call letters, dimmed as a guess, and RBDS's
 * programme type. */
static void north_america_reads_call_letters_from_the_pi(void) {
  station(&info);
  info.pi = 0x21C7;
  build(&info);
  TEST_ASSERT_EQUAL_STRING(rdsPiOriginName(0x21C7), view.area);
  TEST_ASSERT_EQUAL_STRING("no ECC heard", view.country);
  TEST_ASSERT_EQUAL_STRING(txt(STR_PTY_POP_MUSIC), view.ptyName);

  ScreenRdsInputs in = inputs(&info);
  in.region = RDS_REGION_NORTH_AMERICA;
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_NULL(view.area);
  TEST_ASSERT_EQUAL_STRING("KGTB", view.country);
  TEST_ASSERT_TRUE(view.countryNamed);
  TEST_ASSERT_TRUE(view.countryGuess);
  TEST_ASSERT_EQUAL_STRING(txt(STR_PTY_NA_COUNTRY), view.ptyName);
}

/* The station's own short name is its words, not a guess. */
static void north_america_prefers_the_name_the_station_sends(void) {
  station(&info);
  info.pi = 0x21C7;
  info.hasStationShort = true;
  strcpy(info.stationShort, "HOT 97");
  ScreenRdsInputs in = inputs(&info);
  in.region = RDS_REGION_NORTH_AMERICA;
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_STRING("HOT 97", view.country);
  TEST_ASSERT_TRUE(view.countryNamed);
  TEST_ASSERT_FALSE(view.countryGuess);
}

/* 1064 is a PI the standard sends moved, so it gives no call letters, and
 * on North America a PI starting 1 has no area either. */
static void north_america_gives_magic_fm_no_call_letters(void) {
  replay(STRONG_106400, 160);
  ScreenRdsInputs in = inputs(&rds.info);
  in.region = RDS_REGION_NORTH_AMERICA;
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_STRING("1064", view.pi);
  TEST_ASSERT_NULL(view.area);
  TEST_ASSERT_EQUAL_STRING("no ECC heard", view.country);
  TEST_ASSERT_FALSE(view.countryNamed);
  TEST_ASSERT_FALSE(view.countryGuess);
}

/* ----------------------------------------------------------- presets */

static ScreenRdsInputs onPreset(const RdsInfo *r, uint16_t storedPi) {
  ScreenRdsInputs in = inputs(r);
  in.reading.valid = true;
  in.reading.offsetTenths = 30;
  in.presetSlot = 2;
  in.presetPi = storedPi;
  return in;
}

static void the_preset_s_own_station_names_the_preset(void) {
  replay(STRONG_106400, 160);
  const ScreenRdsInputs in = onPreset(&rds.info, 0x1064);
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_STRING("P03", view.country);
  TEST_ASSERT_TRUE(view.countryNamed);
  TEST_ASSERT_FALSE(view.countryGuess);
  TEST_ASSERT_FALSE(view.countryFault);
  TEST_ASSERT_EQUAL_STRING("1064", view.pi);
  TEST_ASSERT_TRUE(view.piSure);
}

/* The stored PI is another station's, and the PI here is confirmed. */
static void another_station_on_the_preset_is_a_fault(void) {
  replay(STRONG_106400, 160);
  const ScreenRdsInputs in = onPreset(&rds.info, 0x26FF);
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_STRING("not P03", view.country);
  TEST_ASSERT_TRUE(view.countryNamed);
  TEST_ASSERT_TRUE(view.countryFault);
  /* The PI shown is the one heard, not the stored one. */
  TEST_ASSERT_EQUAL_STRING("1064", view.pi);
}

/* One clean block A with the stored PI is that station, so the PI is shown
 * as sure before the decoder has confirmed it. */
static void one_block_with_the_stored_pi_shows_it_as_sure(void) {
  replay(STRONG_106400, 1);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  const ScreenRdsInputs in = onPreset(&rds.info, 0x1064);
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_STRING("1064", view.pi);
  TEST_ASSERT_TRUE(view.piSure);
  TEST_ASSERT_EQUAL_STRING("P03", view.country);
}

/* The preset's line takes the place of North America's call letters. */
static void the_preset_line_replaces_the_call_letters(void) {
  station(&info);
  info.pi = 0x21C7;
  ScreenRdsInputs in = onPreset(&info, 0x21C7);
  in.region = RDS_REGION_NORTH_AMERICA;
  in.presetSlot = 41;
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_STRING("P42", view.country);
  TEST_ASSERT_FALSE(view.countryGuess);
  TEST_ASSERT_FALSE(view.countryFault);
}

static void no_preset_or_nothing_to_say_leaves_the_country_alone(void) {
  replay(STRONG_106400, 160);
  /* Not on a preset, whatever PI is passed. */
  ScreenRdsInputs in = onPreset(&rds.info, 0x26FF);
  in.presetSlot = MEMORY_NO_SLOT;
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_STRING("no ECC heard", view.country);
  TEST_ASSERT_FALSE(view.countryFault);
  /* A preset with no stored PI. */
  in = onPreset(&rds.info, 0);
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_STRING("no ECC heard", view.country);
  /* Off the channel, one block is not enough to say. */
  replay(STRONG_106400, 1);
  in = onPreset(&rds.info, 0x1064);
  in.reading.offsetTenths = 500;
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_NULL(view.country);
  TEST_ASSERT_FALSE(view.piSure);
}

/* --------------------------------------------------------- page 2, text */

static void magic_fm_s_radio_text_is_on_page_two(void) {
  replay(STRONG_106400, 160);
  build(&rds.info);
  TEST_ASSERT_EQUAL_STRING(
      "KINNERASANI - SITARA - S.P. BALASUBRAHMANYAM + S.P. SAILAJA - MA",
      view.text);
  TEST_ASSERT_EQUAL_UINT8(0, view.tagCount);
  TEST_ASSERT_FALSE(view.rtPlusRunning);
  TEST_ASSERT_EQUAL_STRING("No RT+ heard from this station", view.rtPlusNote);
}

/* A title comes before an artist, whatever order the station sent them. */
static void rt_plus_tags_come_in_content_type_order(void) {
  station(&info);
  withRtPlus(&info);
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(3, view.tagCount);
  TEST_ASSERT_EQUAL_STRING(rdsRtPlusLabel(1), view.tag[0].label);
  TEST_ASSERT_EQUAL_STRING("DREAMS", view.tag[0].text);
  TEST_ASSERT_EQUAL_STRING(rdsRtPlusLabel(4), view.tag[1].label);
  TEST_ASSERT_EQUAL_STRING("FLEETWOOD MAC", view.tag[1].text);
  TEST_ASSERT_EQUAL_STRING(rdsRtPlusLabel(33), view.tag[2].label);
  TEST_ASSERT_EQUAL_STRING("HIT MUSIC", view.tag[2].text);
  TEST_ASSERT_TRUE(view.rtPlusRunning);
  TEST_ASSERT_EQUAL_STRING("No RT+ tags for this text yet", view.rtPlusNote);
}

/* Four tags and room for three: the three lowest content types. */
static void only_three_rt_plus_tags_are_shown(void) {
  station(&info);
  withRtPlus(&info);
  info.rtPlusCount = 4;
  info.rtPlusTag[3] = {2, 0, 11}; /* NOW PLAYING. */
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(3, view.tagCount);
  TEST_ASSERT_EQUAL_STRING("DREAMS", view.tag[0].text);
  TEST_ASSERT_EQUAL_STRING("NOW PLAYING", view.tag[1].text);
  TEST_ASSERT_EQUAL_STRING("FLEETWOOD MAC", view.tag[2].text);
}

/* A tag past the end of the text, or over spaces only, has no words. It is
 * left out, and the tag after it still gets its place. */
static void a_tag_with_no_words_is_left_out(void) {
  station(&info);
  withRtPlus(&info);
  info.rtPlusCount = 3;
  info.rtPlusTag[0] = {1, 70, 5};
  info.rtPlusTag[1] = {4, 22, 1};
  info.rtPlusTag[2] = {33, 54, 9};
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(1, view.tagCount);
  TEST_ASSERT_EQUAL_STRING(rdsRtPlusLabel(33), view.tag[0].label);
  TEST_ASSERT_EQUAL_STRING("HIT MUSIC", view.tag[0].text);
}

/* RT+ is announced but the text has not arrived: no tag, and the note says
 * the tags are still to come. */
static void rt_plus_with_no_text_yet_waits(void) {
  station(&info);
  withRtPlus(&info);
  info.hasRt = false;
  build(&info);
  TEST_ASSERT_NULL(view.text);
  TEST_ASSERT_EQUAL_UINT8(0, view.tagCount);
  TEST_ASSERT_EQUAL_STRING("No RT+ tags for this text yet", view.rtPlusNote);
}

/* ----------------------------------------------------- page 3, networks */

static void alternative_frequencies_fill_the_tiles(void) {
  station(&info);
  info.afCount = 4;
  info.afKHz[0] = 88100;
  info.afKHz[1] = 97700;
  info.afKHz[2] = 98800;
  info.afKHz[3] = 104900;
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(4, view.afCount);
  TEST_ASSERT_EQUAL_UINT8(0, view.afMore);
  TEST_ASSERT_EQUAL_STRING("88.10", view.af[0]);
  TEST_ASSERT_EQUAL_STRING("97.70", view.af[1]);
  TEST_ASSERT_EQUAL_STRING("98.80", view.af[2]);
  TEST_ASSERT_EQUAL_STRING("104.90", view.af[3]);
}

static void eight_alternative_frequencies_all_fit(void) {
  station(&info);
  info.afCount = SCREEN_RDS_AF;
  for (int i = 0; i < SCREEN_RDS_AF; i++) {
    info.afKHz[i] = 100000u + (uint32_t)i * 100u;
  }
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(SCREEN_RDS_AF, view.afCount);
  TEST_ASSERT_EQUAL_UINT8(0, view.afMore);
  TEST_ASSERT_EQUAL_STRING("100.00", view.af[0]);
  TEST_ASSERT_EQUAL_STRING("100.70", view.af[7]);
}

/* What does not fit is counted on the last tile, in place of one. */
static void more_frequencies_than_tiles_count_the_rest(void) {
  station(&info);
  info.afCount = 12;
  for (int i = 0; i < 12; i++) {
    info.afKHz[i] = 100000u + (uint32_t)i * 100u;
  }
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(SCREEN_RDS_AF, view.afCount);
  TEST_ASSERT_EQUAL_UINT8(5, view.afMore);
  TEST_ASSERT_EQUAL_STRING("100.60", view.af[6]);
  TEST_ASSERT_EQUAL_STRING("+5", view.af[7]);
}

static void other_networks_show_their_pi_name_and_frequencies(void) {
  station(&info);
  RdsEon *e = addEon(&info, 0xC242);
  e->hasPs = true;
  strcpy(e->ps, "RADIO 2 ");
  e->hasTa = true;
  e->ta = false;
  e->afCount = 2;
  e->afCode[0] = 24; /* 89.9 MHz. */
  e->afCode[1] = 31; /* 90.6 MHz. */
  e = addEon(&info, 0xC204);
  e->hasPs = true;
  strcpy(e->ps, "TRAFFIC ");
  e->hasTa = true;
  e->ta = true;
  e->afCount = 1;
  e->afCode[0] = 146; /* 102.1 MHz. */
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(2, view.eonCount);
  TEST_ASSERT_EQUAL_UINT8(0, view.eonMore);
  TEST_ASSERT_EQUAL_STRING("C242", view.eon[0].pi);
  TEST_ASSERT_EQUAL_STRING("RADIO 2", view.eon[0].ps);
  TEST_ASSERT_EQUAL(SCREEN_RDS_NO, view.eon[0].ta);
  TEST_ASSERT_EQUAL_STRING("89.90" DOT "90.60", view.eon[0].freqs);
  TEST_ASSERT_EQUAL_STRING("C204", view.eon[1].pi);
  TEST_ASSERT_EQUAL(SCREEN_RDS_YES, view.eon[1].ta);
  TEST_ASSERT_EQUAL_STRING("102.10", view.eon[1].freqs);
}

/* One group naming it could be a block D corrected wrongly. */
static void a_network_named_once_is_not_listed(void) {
  station(&info);
  addEon(&info, 0xC242)->heard = 1;
  addEon(&info, 0xC243);
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(1, view.eonCount);
  TEST_ASSERT_EQUAL_STRING("C243", view.eon[0].pi);
}

static void a_network_with_nothing_more_heard_shows_dashes(void) {
  station(&info);
  addEon(&info, 0xC242);
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(1, view.eonCount);
  TEST_ASSERT_EQUAL_STRING("C242", view.eon[0].pi);
  TEST_ASSERT_NULL(view.eon[0].ps);
  TEST_ASSERT_NULL(view.eon[0].freqs);
  TEST_ASSERT_EQUAL(SCREEN_RDS_UNKNOWN, view.eon[0].ta);
}

/* Two whole frequencies, then a count of the rest when the radio kept them
 * all, or "more" when it did not. */
static void a_network_s_other_frequencies_are_counted(void) {
  station(&info);
  RdsEon *e = addEon(&info, 0xC242);
  e->afCount = 4;
  e->afCode[0] = 24;
  e->afCode[1] = 31;
  e->afCode[2] = 38;
  e->afCode[3] = 45;
  e = addEon(&info, 0xC243);
  e->afCount = 4;
  e->afMore = true;
  e->afCode[0] = 24;
  e->afCode[1] = 31;
  e->afCode[2] = 38;
  e->afCode[3] = 45;
  build(&info);
  TEST_ASSERT_EQUAL_STRING("89.90" DOT "90.60" DOT "+2", view.eon[0].freqs);
  TEST_ASSERT_EQUAL_STRING("89.90" DOT "90.60" DOT "more", view.eon[1].freqs);
}

static void networks_past_three_are_counted(void) {
  station(&info);
  for (uint16_t i = 0; i < RDS_EON_MAX; i++) {
    addEon(&info, (uint16_t)(0xC242 + i));
  }
  info.eonMore = true;
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(SCREEN_RDS_EON, view.eonCount);
  TEST_ASSERT_EQUAL_UINT8(1, view.eonMore);
  TEST_ASSERT_TRUE(view.eonMoreHeard);
  TEST_ASSERT_EQUAL_STRING("C244", view.eon[2].pi);
}

/* ------------------------------------------------- page 4, the decoder */

static void magic_fm_s_last_minute_is_clean(void) {
  replay(STRONG_106400, 160);
  build(&rds.info);
  TEST_ASSERT_EQUAL_STRING("106.40 MHz" DOT "last 13 s", view.span);
  TEST_ASSERT_EQUAL_STRING("11.5", view.rate);
  TEST_ASSERT_EQUAL_STRING("0.0 %", view.bler);
  TEST_ASSERT_EQUAL_STRING("0/640", view.lost);
  TEST_ASSERT_TRUE(view.blocksKnown);
  for (int b = 0; b < 4; b++) {
    TEST_ASSERT_EQUAL_UINT16(0, view.block[b].fixedTenths);
    TEST_ASSERT_EQUAL_UINT16(0, view.block[b].lostTenths);
    TEST_ASSERT_NULL(view.block[b].text);
  }
  TEST_ASSERT_EQUAL_UINT8(2, view.groupCount);
  TEST_ASSERT_EQUAL_STRING("0A", view.group[0].label);
  TEST_ASSERT_EQUAL_UINT8(50, view.group[0].percent);
  TEST_ASSERT_EQUAL_STRING("2A", view.group[1].label);
  TEST_ASSERT_EQUAL_UINT8(50, view.group[1].percent);
  TEST_ASSERT_NULL(view.groupNote);
}

/* In the weak capture: block A stays clean, the rest show what was corrected
 * and what was lost. */
static void the_weak_capture_shows_its_damaged_blocks(void) {
  replay(WEAK_106400, 400);
  build(&rds.info);
  TEST_ASSERT_EQUAL_STRING("106.40 MHz" DOT "last 35 s", view.span);
  TEST_ASSERT_EQUAL_STRING("11.4", view.rate);
  TEST_ASSERT_EQUAL_STRING("0.3 %", view.bler);
  TEST_ASSERT_EQUAL_STRING("4/1600", view.lost);
  TEST_ASSERT_NULL(view.block[0].text);
  TEST_ASSERT_EQUAL_UINT16(110, view.block[1].fixedTenths);
  TEST_ASSERT_EQUAL_UINT16(3, view.block[1].lostTenths);
  TEST_ASSERT_EQUAL_STRING("11 %" DOT "0.3 %", view.block[1].text);
  TEST_ASSERT_EQUAL_STRING("11 %" DOT "0.5 %", view.block[2].text);
  TEST_ASSERT_EQUAL_STRING("10 %" DOT "0.3 %", view.block[3].text);
  TEST_ASSERT_EQUAL_STRING("2A", view.group[0].label);
  TEST_ASSERT_EQUAL_UINT8(46, view.group[0].percent);
  TEST_ASSERT_EQUAL_STRING("0A", view.group[1].label);
}

/* Fever FM sends its name in the B version. */
static void a_b_version_group_is_labelled_b(void) {
  replay(STRONG_94300, 160);
  build(&rds.info);
  TEST_ASSERT_EQUAL_STRING("0B", view.group[0].label);
  TEST_ASSERT_EQUAL_STRING("2A", view.group[1].label);
}

/* Under a second there is no rate yet, but the counts are known. */
static void one_group_has_counts_but_no_rate(void) {
  replay(STRONG_106400, 1);
  build(&rds.info);
  TEST_ASSERT_NULL(view.rate);
  TEST_ASSERT_EQUAL_STRING("0.0 %", view.bler);
  TEST_ASSERT_EQUAL_STRING("0/4", view.lost);
  TEST_ASSERT_TRUE(view.blocksKnown);
}

static void groups_with_no_type_read_say_so(void) {
  info.minute.groups = 10;
  info.minute.spanMs = 1000;
  for (int b = 0; b < 4; b++) {
    info.minute.blocks[b][RDS_LEVEL_CLEAN] = 10;
  }
  build(&info);
  TEST_ASSERT_TRUE(view.blocksKnown);
  TEST_ASSERT_EQUAL_UINT8(0, view.groupCount);
  TEST_ASSERT_EQUAL_STRING("No block B read clean in the last minute",
                           view.groupNote);
}

static void only_six_group_types_are_shown(void) {
  info.minute.groups = 70;
  info.minute.spanMs = 6000;
  for (int t = 0; t < 7; t++) {
    info.minute.types[t][0] = 10;
  }
  build(&info);
  TEST_ASSERT_EQUAL_UINT8(SCREEN_RDS_GROUPS, view.groupCount);
  TEST_ASSERT_EQUAL_STRING("5A", view.group[5].label);
  TEST_ASSERT_NULL(view.groupNote);
}

/* The header never says more than the minute it is about. */
static void the_span_stops_at_sixty_seconds(void) {
  info.minute.spanMs = 125000;
  build(&info);
  TEST_ASSERT_EQUAL_STRING("106.40 MHz" DOT "last 60 s", view.span);
}

static void no_frequency_gives_no_page_four_header(void) {
  ScreenRdsInputs in = inputs(&info);
  in.frequency = NULL;
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_NULL(view.frequency);
  TEST_ASSERT_NULL(view.span);
}

/* A moment's message for the header is handed on as it came, and none means
 * the header keeps its page and clock. */
static void the_header_message_is_handed_on(void) {
  RdsInfo r;
  station(&r);
  ScreenRdsInputs in = inputs(&r);
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_NULL(view.message);
  in.message = "Tune 104-";
  screenRdsStateBuild(&in, &view);
  TEST_ASSERT_EQUAL_STRING("Tune 104-", view.message);
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(no_view_to_fill_does_not_crash);
  RUN_TEST(no_inputs_or_no_decoder_leaves_the_view_cleared);
  RUN_TEST(the_header_fields_are_passed_on);
  RUN_TEST(an_empty_decoder_shows_nothing_as_heard);

  RUN_TEST(magic_fm_shows_its_name_pi_and_programme_type);
  RUN_TEST(a_pi_heard_once_is_shown_unsure);
  RUN_TEST(a_pi_whose_digits_changed_shows_question_marks);
  RUN_TEST(a_station_sending_zero_shows_it_with_no_area);
  RUN_TEST(a_joined_name_is_shown_in_place_of_one_pass);
  RUN_TEST(the_other_station_fields_are_shown_when_sent);

  RUN_TEST(the_ecc_and_the_pi_name_the_country);
  RUN_TEST(an_ecc_with_no_country_for_it_says_not_listed);
  RUN_TEST(an_ecc_beside_an_unsure_pi_names_no_country);

  RUN_TEST(north_america_reads_call_letters_from_the_pi);
  RUN_TEST(north_america_prefers_the_name_the_station_sends);
  RUN_TEST(north_america_gives_magic_fm_no_call_letters);

  RUN_TEST(the_preset_s_own_station_names_the_preset);
  RUN_TEST(another_station_on_the_preset_is_a_fault);
  RUN_TEST(one_block_with_the_stored_pi_shows_it_as_sure);
  RUN_TEST(the_preset_line_replaces_the_call_letters);
  RUN_TEST(no_preset_or_nothing_to_say_leaves_the_country_alone);

  RUN_TEST(magic_fm_s_radio_text_is_on_page_two);
  RUN_TEST(rt_plus_tags_come_in_content_type_order);
  RUN_TEST(only_three_rt_plus_tags_are_shown);
  RUN_TEST(a_tag_with_no_words_is_left_out);
  RUN_TEST(rt_plus_with_no_text_yet_waits);

  RUN_TEST(alternative_frequencies_fill_the_tiles);
  RUN_TEST(eight_alternative_frequencies_all_fit);
  RUN_TEST(more_frequencies_than_tiles_count_the_rest);
  RUN_TEST(other_networks_show_their_pi_name_and_frequencies);
  RUN_TEST(a_network_named_once_is_not_listed);
  RUN_TEST(a_network_with_nothing_more_heard_shows_dashes);
  RUN_TEST(a_network_s_other_frequencies_are_counted);
  RUN_TEST(networks_past_three_are_counted);

  RUN_TEST(magic_fm_s_last_minute_is_clean);
  RUN_TEST(the_weak_capture_shows_its_damaged_blocks);
  RUN_TEST(a_b_version_group_is_labelled_b);
  RUN_TEST(one_group_has_counts_but_no_rate);
  RUN_TEST(groups_with_no_type_read_say_so);
  RUN_TEST(only_six_group_types_are_shown);
  RUN_TEST(the_span_stops_at_sixty_seconds);
  RUN_TEST(no_frequency_gives_no_page_four_header);
  RUN_TEST(the_header_message_is_handed_on);

  return UNITY_END();
}
