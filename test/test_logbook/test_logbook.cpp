/* Tests for the logbook ring, record format and CSV lines. Runs on a PC. */
#include <unity.h>

#include <stdio.h>
#include <string.h>

#include "core/band_plan.h"
#include "core/logbook.h"

void setUp(void) {}
void tearDown(void) {}

/* ------------------------------------------------------------------- ring */

static void a_fresh_ring_is_empty(void) {
  LogbookRing ring;
  logbookRingInit(&ring);
  TEST_ASSERT_EQUAL_UINT16(0, ring.count);
  TEST_ASSERT_EQUAL_UINT16(LOGBOOK_MAX_ENTRIES,
                           logbookRingSlotAt(&ring, 0)); /* nothing yet */
}

static void appending_below_capacity_just_grows(void) {
  LogbookRing ring;
  logbookRingInit(&ring);
  for (uint16_t i = 0; i < 5; i++) {
    TEST_ASSERT_EQUAL_UINT16(i, logbookRingAppend(&ring));
  }
  TEST_ASSERT_EQUAL_UINT16(5, ring.count);
  TEST_ASSERT_EQUAL_UINT16(0, ring.head);
  for (uint16_t i = 0; i < 5; i++) {
    TEST_ASSERT_EQUAL_UINT16(i, logbookRingSlotAt(&ring, i));
  }
}

static void filling_it_exactly_never_drops_anything(void) {
  LogbookRing ring;
  logbookRingInit(&ring);
  for (uint16_t i = 0; i < LOGBOOK_MAX_ENTRIES; i++) {
    logbookRingAppend(&ring);
  }
  TEST_ASSERT_EQUAL_UINT16(LOGBOOK_MAX_ENTRIES, ring.count);
  TEST_ASSERT_EQUAL_UINT16(0, ring.head);
  TEST_ASSERT_EQUAL_UINT16(0, logbookRingSlotAt(&ring, 0));
  TEST_ASSERT_EQUAL_UINT16(LOGBOOK_MAX_ENTRIES - 1,
                           logbookRingSlotAt(&ring, LOGBOOK_MAX_ENTRIES - 1));
}

static void one_past_capacity_drops_the_oldest(void) {
  LogbookRing ring;
  logbookRingInit(&ring);
  for (uint16_t i = 0; i < LOGBOOK_MAX_ENTRIES; i++) {
    logbookRingAppend(&ring);
  }
  /* The 251st entry. Count stays at the cap, and the oldest slot (0) is no
   * longer where the oldest entry lives. */
  uint16_t slot = logbookRingAppend(&ring);
  TEST_ASSERT_EQUAL_UINT16(0, slot); /* it lands where entry 0 was */
  TEST_ASSERT_EQUAL_UINT16(LOGBOOK_MAX_ENTRIES, ring.count);
  TEST_ASSERT_EQUAL_UINT16(1, ring.head);
  /* The oldest entry left standing is now what used to be index 1. */
  TEST_ASSERT_EQUAL_UINT16(1, logbookRingSlotAt(&ring, 0));
}

static void the_ring_wraps_more_than_once(void) {
  LogbookRing ring;
  logbookRingInit(&ring);
  for (uint16_t i = 0; i < LOGBOOK_MAX_ENTRIES * 3 + 7; i++) {
    logbookRingAppend(&ring);
  }
  TEST_ASSERT_EQUAL_UINT16(LOGBOOK_MAX_ENTRIES, ring.count);
  TEST_ASSERT_EQUAL_UINT16(7, ring.head);
}

static void asking_past_count_is_refused(void) {
  LogbookRing ring;
  logbookRingInit(&ring);
  logbookRingAppend(&ring);
  logbookRingAppend(&ring);
  TEST_ASSERT_EQUAL_UINT16(LOGBOOK_MAX_ENTRIES, logbookRingSlotAt(&ring, 2));
}

static void a_null_ring_does_not_crash(void) {
  logbookRingInit(NULL);
  TEST_ASSERT_EQUAL_UINT16(0, logbookRingAppend(NULL));
  TEST_ASSERT_EQUAL_UINT16(LOGBOOK_MAX_ENTRIES, logbookRingSlotAt(NULL, 0));
}

/* --------------------------------------------------------------- record */

static LogbookEntry sampleWithNameAndPi(void) {
  LogbookEntry e;
  memset(&e, 0, sizeof(e));
  e.timeKnown = true;
  e.timeValue = 1758109999UL;
  e.band = 4; /* BAND_FM. Kept as a plain number, see logbook.h. */
  e.freqKHz = 106400;
  e.levelDbuVTenths = -345;
  e.usnTenths = 12;
  e.multipathTenths = 8;
  e.coChannelTenths = 0;
  e.snrDb = -5;
  e.stereo = true;
  e.bandwidthKHz = 184;
  e.hasName = true;
  snprintf(e.name, sizeof(e.name), "MAGIC");
  e.hasPi = true;
  e.pi = 0x1064;
  return e;
}

/* --------------------------------------------------------- same station */

static void the_same_channel_and_pi_is_the_same_station(void) {
  LogbookEntry a = sampleWithNameAndPi();
  LogbookEntry b = sampleWithNameAndPi();
  /* What else was heard does not matter: a later hearing reads differently
   * and still names the same station. */
  b.timeValue += 3600;
  b.levelDbuVTenths = 120;
  b.hasName = false;
  b.hasRt = true;
  TEST_ASSERT_TRUE(logbookSameStation(&a, &b));
  TEST_ASSERT_TRUE(logbookSameStation(&b, &a));
}

static void another_frequency_or_band_is_another_station(void) {
  LogbookEntry a = sampleWithNameAndPi();
  LogbookEntry b = sampleWithNameAndPi();
  b.freqKHz = 106450;
  TEST_ASSERT_FALSE(logbookSameStation(&a, &b));
  b.freqKHz = 106350;
  TEST_ASSERT_FALSE(logbookSameStation(&a, &b));
  b = sampleWithNameAndPi();
  b.band = 3; /* BAND_OIRT, on the same number. */
  TEST_ASSERT_FALSE(logbookSameStation(&a, &b));
}

static void two_pis_on_one_channel_are_two_stations(void) {
  LogbookEntry a = sampleWithNameAndPi();
  LogbookEntry b = sampleWithNameAndPi();
  b.pi = 0x1065;
  TEST_ASSERT_FALSE(logbookSameStation(&a, &b));
}

static void a_pi_on_one_side_only_leaves_the_channel_to_decide(void) {
  LogbookEntry a = sampleWithNameAndPi();
  LogbookEntry b = sampleWithNameAndPi();
  b.hasPi = false;
  b.pi = 0;
  TEST_ASSERT_TRUE(logbookSameStation(&a, &b));
  TEST_ASSERT_TRUE(logbookSameStation(&b, &a));
  a.hasPi = false;
  TEST_ASSERT_TRUE(logbookSameStation(&a, &b));
}

static void a_missing_entry_is_never_the_same_station(void) {
  LogbookEntry a = sampleWithNameAndPi();
  TEST_ASSERT_FALSE(logbookSameStation(&a, NULL));
  TEST_ASSERT_FALSE(logbookSameStation(NULL, &a));
  TEST_ASSERT_FALSE(logbookSameStation(NULL, NULL));
}

static void the_ring_says_which_entry_a_slot_holds(void) {
  LogbookRing ring;
  logbookRingInit(&ring);
  uint16_t i = 99;
  TEST_ASSERT_FALSE(logbookRingIndexOf(&ring, 0, &i));
  /* Full, the oldest in slot 240: slot 240 is entry 0, slot 239 the newest. */
  ring.head = 240;
  ring.count = LOGBOOK_MAX_ENTRIES;
  TEST_ASSERT_TRUE(logbookRingIndexOf(&ring, 240, &i));
  TEST_ASSERT_EQUAL_UINT16(0, i);
  TEST_ASSERT_TRUE(logbookRingIndexOf(&ring, 0, &i));
  TEST_ASSERT_EQUAL_UINT16(10, i);
  TEST_ASSERT_TRUE(logbookRingIndexOf(&ring, 239, &i));
  TEST_ASSERT_EQUAL_UINT16(LOGBOOK_MAX_ENTRIES - 1, i);
  for (uint16_t k = 0; k < LOGBOOK_MAX_ENTRIES; k++) {
    TEST_ASSERT_TRUE(
        logbookRingIndexOf(&ring, logbookRingSlotAt(&ring, k), &i));
    TEST_ASSERT_EQUAL_UINT16(k, i);
  }
  /* Three entries from slot 248: 248, 249, 0; slot 1 is not held. */
  ring.head = 248;
  ring.count = 3;
  TEST_ASSERT_TRUE(logbookRingIndexOf(&ring, 0, &i));
  TEST_ASSERT_EQUAL_UINT16(2, i);
  TEST_ASSERT_FALSE(logbookRingIndexOf(&ring, 1, &i));
  TEST_ASSERT_FALSE(logbookRingIndexOf(&ring, 247, &i));
  TEST_ASSERT_FALSE(logbookRingIndexOf(&ring, LOGBOOK_MAX_ENTRIES, &i));
  TEST_ASSERT_FALSE(logbookRingIndexOf(NULL, 0, &i));
  TEST_ASSERT_FALSE(logbookRingIndexOf(&ring, 0, NULL));
}

static void a_record_encodes_to_the_fixed_size(void) {
  LogbookEntry e = sampleWithNameAndPi();
  uint8_t buf[LOGBOOK_RECORD_SIZE];
  TEST_ASSERT_EQUAL_UINT32(LOGBOOK_RECORD_SIZE,
                           logbookEncode(&e, buf, sizeof(buf)));
}

static void a_record_round_trips_every_field(void) {
  LogbookEntry e = sampleWithNameAndPi();
  uint8_t buf[LOGBOOK_RECORD_SIZE];
  TEST_ASSERT_EQUAL_UINT32(LOGBOOK_RECORD_SIZE,
                           logbookEncode(&e, buf, sizeof(buf)));

  LogbookEntry back;
  /* Filled with junk, so the decode must write every field. */
  memset(&back, 0xAA, sizeof(back));
  TEST_ASSERT_TRUE(logbookDecode(buf, sizeof(buf), &back));

  TEST_ASSERT_TRUE(back.timeKnown);
  TEST_ASSERT_EQUAL_UINT32(e.timeValue, back.timeValue);
  TEST_ASSERT_EQUAL_UINT8(e.band, back.band);
  TEST_ASSERT_EQUAL_UINT32(e.freqKHz, back.freqKHz);
  TEST_ASSERT_EQUAL_INT16(e.levelDbuVTenths, back.levelDbuVTenths);
  TEST_ASSERT_EQUAL_UINT16(e.usnTenths, back.usnTenths);
  TEST_ASSERT_EQUAL_UINT16(e.multipathTenths, back.multipathTenths);
  TEST_ASSERT_EQUAL_UINT16(e.coChannelTenths, back.coChannelTenths);
  TEST_ASSERT_EQUAL_INT8(e.snrDb, back.snrDb);
  TEST_ASSERT_TRUE(back.stereo);
  TEST_ASSERT_EQUAL_UINT16(e.bandwidthKHz, back.bandwidthKHz);
  TEST_ASSERT_TRUE(back.hasName);
  TEST_ASSERT_EQUAL_STRING("MAGIC", back.name);
  TEST_ASSERT_TRUE(back.hasPi);
  TEST_ASSERT_EQUAL_UINT16(e.pi, back.pi);
  TEST_ASSERT_FALSE(back.hasRt);
  TEST_ASSERT_EQUAL_STRING("", back.rt);
}

/* A text of the full 64 characters, the most RDS sends. */
static void a_radio_text_round_trips_whole(void) {
  LogbookEntry e = sampleWithNameAndPi();
  e.hasRt = true;
  snprintf(e.rt, sizeof(e.rt), "%s",
           "0123456789012345678901234567890123456789012345678901234567890123");
  TEST_ASSERT_EQUAL_size_t(64, strlen(e.rt));
  uint8_t buf[LOGBOOK_RECORD_SIZE];
  TEST_ASSERT_EQUAL_UINT32(LOGBOOK_RECORD_SIZE,
                           logbookEncode(&e, buf, sizeof(buf)));
  LogbookEntry back;
  memset(&back, 0xAA, sizeof(back));
  TEST_ASSERT_TRUE(logbookDecode(buf, sizeof(buf), &back));
  TEST_ASSERT_TRUE(back.hasRt);
  TEST_ASSERT_EQUAL_STRING(e.rt, back.rt);
  TEST_ASSERT_EQUAL_UINT16(e.pi, back.pi);
}

/* A record whose text has no terminator, which only a damaged file holds,
 * still decodes to a string that ends inside the field. */
static void a_radio_text_with_no_terminator_is_cut_at_the_field(void) {
  LogbookEntry e = sampleWithNameAndPi();
  uint8_t buf[LOGBOOK_RECORD_SIZE];
  logbookEncode(&e, buf, sizeof(buf));
  /* The flag and the text end format 2's record. */
  memset(buf + LOGBOOK_RECORD_SIZE_V2 - LOGBOOK_RT_LEN - 1, 1,
         LOGBOOK_RT_LEN + 1);
  LogbookEntry back;
  TEST_ASSERT_TRUE(logbookDecode(buf, sizeof(buf), &back));
  TEST_ASSERT_TRUE(back.hasRt);
  TEST_ASSERT_EQUAL_size_t(LOGBOOK_RT_LEN - 1, strlen(back.rt));
}

/* --------------------------------------------------- formats 1 and 2 */

/* Format 1's record is this format's first 35 bytes, so an entry written
 * with no radio text, note or mark and cut to that length is exactly what
 * format 1 wrote. */
static void a_format_1_record_upgrades_to_the_same_entry(void) {
  LogbookEntry e = sampleWithNameAndPi();
  uint8_t now[LOGBOOK_RECORD_SIZE];
  logbookEncode(&e, now, sizeof(now));
  uint8_t old[LOGBOOK_RECORD_SIZE_V1];
  memcpy(old, now, sizeof(old));

  uint8_t up[LOGBOOK_RECORD_SIZE];
  memset(up, 0xAA, sizeof(up));
  TEST_ASSERT_TRUE(logbookUpgrade(old, sizeof(old), up, sizeof(up)));
  TEST_ASSERT_EQUAL_MEMORY(now, up, LOGBOOK_RECORD_SIZE);

  LogbookEntry back;
  TEST_ASSERT_TRUE(logbookDecode(up, sizeof(up), &back));
  TEST_ASSERT_EQUAL_STRING("MAGIC", back.name);
  TEST_ASSERT_EQUAL_UINT16(0x1064, back.pi);
  TEST_ASSERT_EQUAL_UINT32(106400, back.freqKHz);
  TEST_ASSERT_FALSE(back.hasRt);
}

/* Format 2 is the first 101 bytes: the radio text comes through, the note
 * is empty and the name is not marked renamed. */
static void a_format_2_record_keeps_its_radio_text_and_gains_no_note(void) {
  LogbookEntry e = sampleWithNameAndPi();
  e.hasRt = true;
  snprintf(e.rt, sizeof(e.rt), "%s", "Now playing");
  uint8_t now[LOGBOOK_RECORD_SIZE];
  logbookEncode(&e, now, sizeof(now));
  uint8_t old[LOGBOOK_RECORD_SIZE_V2];
  memcpy(old, now, sizeof(old));
  uint8_t up[LOGBOOK_RECORD_SIZE];
  memset(up, 0xAA, sizeof(up));
  TEST_ASSERT_TRUE(logbookUpgrade(old, sizeof(old), up, sizeof(up)));
  TEST_ASSERT_EQUAL_MEMORY(now, up, LOGBOOK_RECORD_SIZE);
  LogbookEntry back;
  TEST_ASSERT_TRUE(logbookDecode(up, sizeof(up), &back));
  TEST_ASSERT_EQUAL_STRING("Now playing", back.rt);
  TEST_ASSERT_EQUAL_STRING("", back.note);
  TEST_ASSERT_FALSE(back.renamed);
}

static void an_upgrade_with_no_room_is_refused(void) {
  uint8_t old[LOGBOOK_RECORD_SIZE_V1] = {0};
  uint8_t up[LOGBOOK_RECORD_SIZE + 1];
  TEST_ASSERT_FALSE(
      logbookUpgrade(old, sizeof(old), up, LOGBOOK_RECORD_SIZE - 1));
  TEST_ASSERT_FALSE(logbookUpgrade(NULL, sizeof(old), up, sizeof(up)));
  TEST_ASSERT_FALSE(logbookUpgrade(old, sizeof(old), NULL, sizeof(up)));
  TEST_ASSERT_FALSE(
      logbookUpgrade(up, LOGBOOK_RECORD_SIZE + 1, up, sizeof(up)));
}

/* --------------------------------------------------------------- format 3 */

static void a_note_and_the_renamed_mark_round_trip(void) {
  LogbookEntry e = sampleWithNameAndPi();
  snprintf(e.note, sizeof(e.note), "%s", "Heard from the roof, 40 km");
  e.renamed = true;
  uint8_t buf[LOGBOOK_RECORD_SIZE];
  TEST_ASSERT_EQUAL_UINT32(LOGBOOK_RECORD_SIZE,
                           logbookEncode(&e, buf, sizeof(buf)));
  LogbookEntry back;
  memset(&back, 0xAA, sizeof(back));
  TEST_ASSERT_TRUE(logbookDecode(buf, sizeof(buf), &back));
  TEST_ASSERT_EQUAL_STRING("Heard from the roof, 40 km", back.note);
  TEST_ASSERT_TRUE(back.renamed);
  /* Only bit 0 is the mark: a flags byte with others set is not read as
   * more than it says. */
  buf[LOGBOOK_RECORD_SIZE - 1] = 0xFE;
  TEST_ASSERT_TRUE(logbookDecode(buf, sizeof(buf), &back));
  TEST_ASSERT_FALSE(back.renamed);
  TEST_ASSERT_EQUAL_STRING("MAGIC", back.name);
}

/* A note of 32 characters, the most, and one with no terminator, which only
 * a damaged file holds, cut at the field. */
static void a_full_note_round_trips_and_a_broken_one_is_cut(void) {
  LogbookEntry e = sampleWithNameAndPi();
  snprintf(e.note, sizeof(e.note), "%s", "01234567890123456789012345678901");
  TEST_ASSERT_EQUAL_size_t(32, strlen(e.note));
  uint8_t buf[LOGBOOK_RECORD_SIZE];
  logbookEncode(&e, buf, sizeof(buf));
  LogbookEntry back;
  TEST_ASSERT_TRUE(logbookDecode(buf, sizeof(buf), &back));
  TEST_ASSERT_EQUAL_STRING(e.note, back.note);
  memset(buf + LOGBOOK_RECORD_SIZE_V2, 'x', LOGBOOK_NOTE_LEN);
  TEST_ASSERT_TRUE(logbookDecode(buf, sizeof(buf), &back));
  TEST_ASSERT_EQUAL_size_t(LOGBOOK_NOTE_LEN - 1, strlen(back.note));
}

static void a_text_fits_when_short_and_plain(void) {
  TEST_ASSERT_TRUE(logbookTextFits("", 8));
  TEST_ASSERT_TRUE(logbookTextFits("MAGIC FM", 8));
  TEST_ASSERT_FALSE(logbookTextFits("MAGIC FM!", 8));
  TEST_ASSERT_TRUE(logbookTextFits("~ and space ok", 32));
  TEST_ASSERT_FALSE(logbookTextFits("line\nbreak", 32));
  TEST_ASSERT_FALSE(logbookTextFits("tab\there", 32));
  TEST_ASSERT_FALSE(logbookTextFits("caf\xC3\xA9", 32));
  TEST_ASSERT_FALSE(logbookTextFits("\x7F", 32));
  TEST_ASSERT_FALSE(logbookTextFits(NULL, 32));
}

static void a_note_is_the_last_csv_field_quoted_when_it_must_be(void) {
  LogbookEntry e = sampleWithNameAndPi();
  snprintf(e.note, sizeof(e.note), "%s", "roof, \"good\"");
  char line[240];
  logbookCsvLine(&e, 330, line, sizeof(line));
  TEST_ASSERT_NOT_NULL(strstr(line, ",MAGIC,1064,,\"roof, \"\"good\"\"\",0\n"));
}

static void an_entry_with_nothing_extra_round_trips_too(void) {
  /* No name, no PI, no real time. The boot-relative and off-air case. */
  LogbookEntry e;
  memset(&e, 0, sizeof(e));
  e.timeKnown = false;
  e.timeValue = 42;
  e.band = 3;
  e.freqKHz = 738;
  e.hasName = false;
  e.hasPi = false;

  uint8_t buf[LOGBOOK_RECORD_SIZE];
  TEST_ASSERT_EQUAL_UINT32(LOGBOOK_RECORD_SIZE,
                           logbookEncode(&e, buf, sizeof(buf)));
  LogbookEntry back;
  TEST_ASSERT_TRUE(logbookDecode(buf, sizeof(buf), &back));
  TEST_ASSERT_FALSE(back.timeKnown);
  TEST_ASSERT_EQUAL_UINT32(42, back.timeValue);
  TEST_ASSERT_FALSE(back.hasName);
  TEST_ASSERT_FALSE(back.hasPi);
}

static void a_short_buffer_is_refused_both_ways(void) {
  LogbookEntry e = sampleWithNameAndPi();
  uint8_t buf[LOGBOOK_RECORD_SIZE];
  TEST_ASSERT_EQUAL_UINT32(0, logbookEncode(&e, buf, LOGBOOK_RECORD_SIZE - 1));

  memset(buf, 0, sizeof(buf));
  logbookEncode(&e, buf, sizeof(buf));
  LogbookEntry back;
  TEST_ASSERT_FALSE(logbookDecode(buf, LOGBOOK_RECORD_SIZE - 1, &back));
}

static void a_null_record_call_does_not_crash(void) {
  LogbookEntry e = sampleWithNameAndPi();
  uint8_t buf[LOGBOOK_RECORD_SIZE];
  TEST_ASSERT_EQUAL_UINT32(0, logbookEncode(NULL, buf, sizeof(buf)));
  TEST_ASSERT_EQUAL_UINT32(0, logbookEncode(&e, NULL, sizeof(buf)));
  TEST_ASSERT_FALSE(logbookDecode(NULL, sizeof(buf), &e));
  TEST_ASSERT_FALSE(logbookDecode(buf, sizeof(buf), NULL));
}

/* ------------------------------------------------------------------- csv */

static void the_csv_header_names_every_column(void) {
  TEST_ASSERT_EQUAL_STRING(
      "time,real,band,khz,level_dbuv,usn,multipath,cochannel,snr,stereo,"
      "bandwidth_khz,name,pi,rt,note,renamed\n",
      logbookCsvHeader());
}

static void a_known_time_writes_a_calendar_date(void) {
  LogbookEntry e = sampleWithNameAndPi();
  /* 1758109999 UTC is 2025-09-17 11:53:19. UTC+5:30 lands it on
   * 17 September 2025, 17:23. */
  char line[160];
  TEST_ASSERT_TRUE(logbookCsvLine(&e, 330, line, sizeof(line)) > 0);
  TEST_ASSERT_EQUAL_STRING(
      "2025-09-17 17:23,1,FM,106400,-345,12,8,0,-5,1,184,MAGIC,1064,,,0\n",
      line);
}

static void an_offset_that_crosses_midnight_moves_the_date(void) {
  LogbookEntry e = sampleWithNameAndPi();
  e.timeValue = 1758067200UL; /* 2025-09-17 00:00:00 UTC exactly. */
  char line[160];
  logbookCsvLine(&e, -60, line, sizeof(line)); /* UTC-1: the previous day. */
  TEST_ASSERT_TRUE(strncmp(line, "2025-09-16 23:00", 16) == 0);
}

static void an_unknown_time_writes_how_long_since_boot(void) {
  LogbookEntry e = sampleWithNameAndPi();
  e.timeKnown = false;
  e.timeValue = 754321;
  char line[160];
  logbookCsvLine(&e, 330, line, sizeof(line));
  TEST_ASSERT_TRUE(strncmp(line, "+754321ms,0,", 12) == 0);
}

static void a_missing_name_and_identifier_are_empty_fields(void) {
  LogbookEntry e = sampleWithNameAndPi();
  e.hasName = false;
  e.hasPi = false;
  char line[160];
  logbookCsvLine(&e, 330, line, sizeof(line));
  /* Name, PI, radio text and note all empty, then the renamed mark. */
  TEST_ASSERT_NOT_NULL(strstr(line, ",184,,,,,0\n"));
}

static void a_name_holding_a_comma_is_quoted(void) {
  LogbookEntry e = sampleWithNameAndPi();
  snprintf(e.name, sizeof(e.name), "A,B");
  char line[160];
  logbookCsvLine(&e, 330, line, sizeof(line));
  TEST_ASSERT_TRUE(strstr(line, "\"A,B\"") != NULL);
}

static void a_name_holding_a_quote_doubles_it(void) {
  LogbookEntry e = sampleWithNameAndPi();
  snprintf(e.name, sizeof(e.name), "A\"B");
  char line[160];
  logbookCsvLine(&e, 330, line, sizeof(line));
  TEST_ASSERT_TRUE(strstr(line, "\"A\"\"B\"") != NULL);
}

/* The longest a quoted name gets: eight characters, each one a quote that
 * has to be doubled, inside the two quotes that wrap the field. */
static void a_quote_in_the_last_place_keeps_its_escape(void) {
  LogbookEntry e = sampleWithNameAndPi();
  snprintf(e.name, sizeof(e.name), "ABCDEFG\"");
  char line[160];
  logbookCsvLine(&e, 330, line, sizeof(line));
  TEST_ASSERT_NOT_NULL(strstr(line, ",\"ABCDEFG\"\"\",1064,,,0\n"));
}

static void a_name_of_only_quotes_is_doubled_in_full(void) {
  LogbookEntry e = sampleWithNameAndPi();
  snprintf(e.name, sizeof(e.name), "\"\"\"\"\"\"\"\"");
  char line[160];
  logbookCsvLine(&e, 330, line, sizeof(line));
  /* The wrapping quote, sixteen for the eight doubled, the closing quote. */
  TEST_ASSERT_NOT_NULL(
      strstr(line, ",\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\",1064,,,0\n"));
}

static void a_radio_text_is_the_last_column(void) {
  LogbookEntry e = sampleWithNameAndPi();
  e.hasRt = true;
  snprintf(e.rt, sizeof(e.rt), "Now playing");
  char line[320];
  logbookCsvLine(&e, 330, line, sizeof(line));
  TEST_ASSERT_NOT_NULL(strstr(line, ",MAGIC,1064,Now playing,,0\n"));
}

static void a_radio_text_holding_a_comma_is_quoted(void) {
  LogbookEntry e = sampleWithNameAndPi();
  e.hasRt = true;
  snprintf(e.rt, sizeof(e.rt), "Artist, \"Title\"");
  char line[320];
  logbookCsvLine(&e, 330, line, sizeof(line));
  TEST_ASSERT_NOT_NULL(strstr(line, ",1064,\"Artist, \"\"Title\"\"\",,0\n"));
}

/* Every field at its widest: the latest time a 32 bit epoch reaches, the
 * longest band name, the widest numbers, and a name and a text of nothing
 * but quotes, each doubled. GET /api/log.csv sends lines from a 320 byte
 * buffer. */
static void the_longest_line_fits_the_export_buffer(void) {
  LogbookEntry e;
  memset(&e, 0, sizeof(e));
  e.timeKnown = true;
  e.timeValue = 0xFFFFFFFFUL;
  e.band = BAND_OIRT; /* The longest band name. */
  e.freqKHz = 0xFFFFFFFFUL;
  e.levelDbuVTenths = -32768;
  e.usnTenths = 65535;
  e.multipathTenths = 65535;
  e.coChannelTenths = 65535;
  e.snrDb = -128;
  e.stereo = true;
  e.bandwidthKHz = 65535;
  e.hasName = true;
  memset(e.name, '"', LOGBOOK_NAME_LEN - 1);
  e.hasPi = true;
  e.pi = 0xFFFF;
  e.hasRt = true;
  memset(e.rt, '"', LOGBOOK_RT_LEN - 1);
  memset(e.note, '"', LOGBOOK_NOTE_LEN - 1);
  char line[320];
  const size_t n = logbookCsvLine(&e, 14 * 60, line, sizeof(line));
  /* Under the 320 bytes the export writes each line into. */
  TEST_ASSERT_EQUAL_size_t(297, n);
}

static void a_too_small_buffer_reports_the_length_it_needed(void) {
  LogbookEntry e = sampleWithNameAndPi();
  char tiny[8];
  size_t need = logbookCsvLine(&e, 330, tiny, sizeof(tiny));
  TEST_ASSERT_TRUE(need >= sizeof(tiny));
}

static void a_null_csv_call_does_not_crash(void) {
  char line[160];
  TEST_ASSERT_EQUAL_UINT32(0, logbookCsvLine(NULL, 0, line, sizeof(line)));
}

static LogbookEntry labelled(uint8_t band, uint32_t khz, const char *name,
                             int pi) {
  LogbookEntry e;
  memset(&e, 0, sizeof(e));
  e.band = band;
  e.freqKHz = khz;
  if (name != NULL) {
    e.hasName = true;
    snprintf(e.name, sizeof(e.name), "%s", name);
  }
  if (pi >= 0) {
    e.hasPi = true;
    e.pi = (uint16_t)pi;
  }
  return e;
}

static void an_entry_is_called_by_its_name_without_the_padding(void) {
  char out[16];
  LogbookEntry e = labelled(BAND_FM, 106400, " MAGIC  ", 0x1064);
  logbookEntryLabel(&e, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("MAGIC", out);
  e = labelled(BAND_FM, 94300, "FEVER FM", -1);
  logbookEntryLabel(&e, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("FEVER FM", out);
}

static void an_entry_with_no_name_is_called_by_its_pi_then_its_band(void) {
  char out[16];
  /* A name of nothing but spaces is no name. */
  LogbookEntry e = labelled(BAND_FM, 98300, "        ", 0x26FF);
  logbookEntryLabel(&e, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("PI 26FF", out);
  e = labelled(BAND_FM, 93500, NULL, 0x0935);
  logbookEntryLabel(&e, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("PI 0935", out);
  e = labelled(BAND_SW, 11990, NULL, -1);
  logbookEntryLabel(&e, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("SW", out);
}

static void a_frequency_comes_with_its_unit(void) {
  char out[16];
  LogbookEntry e = labelled(BAND_FM, 98300, NULL, -1);
  TEST_ASSERT_TRUE(logbookEntryFrequency(&e, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("98.30 MHz", out);
  e = labelled(BAND_SW, 11990, NULL, -1);
  TEST_ASSERT_TRUE(logbookEntryFrequency(&e, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("11990 kHz", out);
  e = labelled(BAND_COUNT, 11990, NULL, -1);
  TEST_ASSERT_FALSE(logbookEntryFrequency(&e, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
}

static void a_label_fits_a_small_buffer_and_survives_null(void) {
  char out[4];
  LogbookEntry e = labelled(BAND_FM, 94300, "FEVER FM", -1);
  logbookEntryLabel(&e, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("FEV", out);
  logbookEntryLabel(NULL, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("", out);
  logbookEntryLabel(&e, NULL, 4);
  TEST_ASSERT_FALSE(logbookEntryFrequency(NULL, out, sizeof(out)));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(a_fresh_ring_is_empty);
  RUN_TEST(appending_below_capacity_just_grows);
  RUN_TEST(filling_it_exactly_never_drops_anything);
  RUN_TEST(one_past_capacity_drops_the_oldest);
  RUN_TEST(the_ring_wraps_more_than_once);
  RUN_TEST(asking_past_count_is_refused);
  RUN_TEST(a_null_ring_does_not_crash);

  RUN_TEST(the_same_channel_and_pi_is_the_same_station);
  RUN_TEST(another_frequency_or_band_is_another_station);
  RUN_TEST(two_pis_on_one_channel_are_two_stations);
  RUN_TEST(a_pi_on_one_side_only_leaves_the_channel_to_decide);
  RUN_TEST(a_missing_entry_is_never_the_same_station);

  RUN_TEST(a_record_encodes_to_the_fixed_size);
  RUN_TEST(a_record_round_trips_every_field);
  RUN_TEST(a_radio_text_round_trips_whole);
  RUN_TEST(a_radio_text_with_no_terminator_is_cut_at_the_field);
  RUN_TEST(a_format_1_record_upgrades_to_the_same_entry);
  RUN_TEST(a_format_2_record_keeps_its_radio_text_and_gains_no_note);
  RUN_TEST(an_upgrade_with_no_room_is_refused);
  RUN_TEST(a_note_and_the_renamed_mark_round_trip);
  RUN_TEST(the_ring_says_which_entry_a_slot_holds);
  RUN_TEST(a_full_note_round_trips_and_a_broken_one_is_cut);
  RUN_TEST(a_text_fits_when_short_and_plain);
  RUN_TEST(a_note_is_the_last_csv_field_quoted_when_it_must_be);
  RUN_TEST(an_entry_with_nothing_extra_round_trips_too);
  RUN_TEST(a_short_buffer_is_refused_both_ways);
  RUN_TEST(a_null_record_call_does_not_crash);

  RUN_TEST(the_csv_header_names_every_column);
  RUN_TEST(a_known_time_writes_a_calendar_date);
  RUN_TEST(an_offset_that_crosses_midnight_moves_the_date);
  RUN_TEST(an_unknown_time_writes_how_long_since_boot);
  RUN_TEST(a_missing_name_and_identifier_are_empty_fields);
  RUN_TEST(a_name_holding_a_comma_is_quoted);
  RUN_TEST(a_name_holding_a_quote_doubles_it);
  RUN_TEST(a_quote_in_the_last_place_keeps_its_escape);
  RUN_TEST(a_name_of_only_quotes_is_doubled_in_full);
  RUN_TEST(a_radio_text_is_the_last_column);
  RUN_TEST(a_radio_text_holding_a_comma_is_quoted);
  RUN_TEST(the_longest_line_fits_the_export_buffer);
  RUN_TEST(a_too_small_buffer_reports_the_length_it_needed);
  RUN_TEST(a_null_csv_call_does_not_crash);
  RUN_TEST(an_entry_is_called_by_its_name_without_the_padding);
  RUN_TEST(an_entry_with_no_name_is_called_by_its_pi_then_its_band);
  RUN_TEST(a_frequency_comes_with_its_unit);
  RUN_TEST(a_label_fits_a_small_buffer_and_survives_null);

  return UNITY_END();
}
