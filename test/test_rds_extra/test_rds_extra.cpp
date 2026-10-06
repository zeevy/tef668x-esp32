/*
 * Tests for the last minute counts, RT+, EON and the programme language.
 * Runs on a PC.
 *
 * None of the recorded stations sends groups 3A, 11A, 14A or 1A variant 3:
 * they send 0A, 0B and 2A only. So the RT+, EON and language groups below are
 * written by hand from the standard's bit layout, the same known risk the
 * alternative frequencies and the clock carry. The minute counts are checked
 * against real groups from a weak signal.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/rds.h"

#include "../test_rds/captures.h"

#define TUNED_KHZ 106400u
#define OWN_PI 0x1064u

static Rds rds;

void setUp(void) {
  rdsReset(&rds, TUNED_KHZ);
}
void tearDown(void) {}

static RdsRead group(uint16_t a, uint16_t b, uint16_t c, uint16_t d,
                     uint32_t atMs) {
  RdsRead r;
  memset(&r, 0, sizeof(r));
  r.synchronised = true;
  r.haveGroup = true;
  r.block[0] = a;
  r.block[1] = b;
  r.block[2] = c;
  r.block[3] = d;
  r.atMs = atMs;
  return r;
}

static void feed(uint16_t b, uint16_t c, uint16_t d, uint32_t atMs) {
  RdsRead r = group(OWN_PI, b, c, d, atMs);
  rdsFeed(&rds, &r);
}

static void twice(uint16_t b, uint16_t c, uint16_t d) {
  feed(b, c, d, 0);
  feed(b, c, d, 0);
}

static uint16_t pair(char a, char b) {
  return (uint16_t)(((uint16_t)(uint8_t)a << 8) | (uint8_t)b);
}

/* ---------------------------------------------------------- the minute */

/* The error level of one block, out of the packed byte. */
static uint8_t levelOf(uint8_t packed, int block) {
  return (uint8_t)((packed >> (6 - 2 * block)) & 3u);
}

/* The weak 106.4 capture, Magic FM, fed four groups a second so the 400
 * groups span 100 s: the last minute holds the newest of them, and the block
 * levels are what the tuner said of them. */
static void the_minute_counts_the_weak_capture(void) {
  const CaptureGroup *groups = kfm_106400_weak_2026_09_13;
  const size_t n = sizeof(kfm_106400_weak_2026_09_13) /
                   sizeof(kfm_106400_weak_2026_09_13[0]);
  for (size_t i = 0; i < n; i++) {
    RdsRead r =
        group(groups[i].block[0], groups[i].block[1], groups[i].block[2],
              groups[i].block[3], (uint32_t)(i * 250u));
    for (int b = 0; b < 4; b++) {
      r.error[b] = levelOf(groups[i].error, b);
    }
    rdsFeed(&rds, &r);
  }
  /* What the counts should be: every group from the start of the oldest of
   * the twelve five second slots the decoder keeps. */
  const uint32_t lastMs = (uint32_t)((n - 1) * 250u);
  const uint32_t from = lastMs - lastMs % 5000u - 55000u;
  uint32_t groupsIn = 0;
  uint32_t fixed[4] = {0}, lost[4] = {0}, clean[4] = {0};
  for (size_t i = 0; i < n; i++) {
    if ((uint32_t)(i * 250u) < from) {
      continue;
    }
    groupsIn++;
    for (int b = 0; b < 4; b++) {
      const uint8_t e = levelOf(groups[i].error, b);
      if (e == 0) {
        clean[b]++;
      } else if (e >= 3) {
        lost[b]++;
      } else {
        fixed[b]++;
      }
    }
  }
  TEST_ASSERT_EQUAL_UINT16(groupsIn, rds.info.minute.groups);
  for (int b = 0; b < 4; b++) {
    TEST_ASSERT_EQUAL_UINT16(clean[b],
                             rds.info.minute.blocks[b][RDS_LEVEL_CLEAN]);
    TEST_ASSERT_EQUAL_UINT16(fixed[b],
                             rds.info.minute.blocks[b][RDS_LEVEL_CORRECTED]);
    TEST_ASSERT_EQUAL_UINT16(lost[b],
                             rds.info.minute.blocks[b][RDS_LEVEL_LOST]);
  }
  /* The damage is real: the tuner corrected some blocks. */
  TEST_ASSERT_TRUE(rds.info.minute.blocks[1][RDS_LEVEL_CORRECTED] > 0);
  /* Only 0A and 2A on air. */
  TEST_ASSERT_TRUE(rds.info.minute.types[0][0] > 0);
  TEST_ASSERT_TRUE(rds.info.minute.types[2][0] > 0);
  TEST_ASSERT_EQUAL_UINT16(0, rds.info.minute.types[0][1]);
  TEST_ASSERT_TRUE(rds.info.minute.spanMs >= 55000u &&
                   rds.info.minute.spanMs < 60000u);
}

static void old_groups_leave_the_minute(void) {
  for (int i = 0; i < 10; i++) {
    feed(0x0000, 0, 0, 1000);
  }
  for (int i = 0; i < 7; i++) {
    feed(0x0000, 0, 0, 30000);
  }
  TEST_ASSERT_EQUAL_UINT16(17, rds.info.minute.groups);
  /* The first slot began at the first read, 1000 ms. 60 s on it is the
   * one being filled again, and what it held is gone. */
  RdsRead none;
  memset(&none, 0, sizeof(none));
  none.synchronised = true;
  none.atMs = 60999;
  rdsFeed(&rds, &none);
  TEST_ASSERT_EQUAL_UINT16(17, rds.info.minute.groups);
  none.atMs = 61000;
  rdsFeed(&rds, &none);
  TEST_ASSERT_EQUAL_UINT16(7, rds.info.minute.groups);
  TEST_ASSERT_EQUAL_UINT16(7, rds.info.minute.types[0][0]);
  TEST_ASSERT_EQUAL_UINT16(7, rds.info.minute.blocks[1][RDS_LEVEL_CLEAN]);
}

static void a_minute_with_no_reads_empties_it(void) {
  feed(0x0000, 0, 0, 1000);
  feed(0x0000, 0, 0, 2000);
  RdsRead none;
  memset(&none, 0, sizeof(none));
  none.atMs = 200000;
  rdsFeed(&rds, &none);
  TEST_ASSERT_EQUAL_UINT16(0, rds.info.minute.groups);
  /* The time with no read is not time the counts cover. */
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.minute.spanMs);
  none.atMs = 212000;
  rdsFeed(&rds, &none);
  TEST_ASSERT_EQUAL_UINT32(12000u, rds.info.minute.spanMs);
}

static void the_span_grows_from_the_first_read(void) {
  feed(0x0000, 0, 0, 5000);
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.minute.spanMs);
  feed(0x0000, 0, 0, 17000);
  TEST_ASSERT_EQUAL_UINT32(12000u, rds.info.minute.spanMs);
  for (uint32_t t = 20000; t <= 125000; t += 5000) {
    feed(0x0000, 0, 0, t);
  }
  TEST_ASSERT_EQUAL_UINT32(55000u, rds.info.minute.spanMs);
}

/* 49.7 days on one station and the millisecond clock wraps; the span must
 * not fall to nothing when it does. */
static void the_span_survives_the_clock_wrapping(void) {
  for (uint32_t i = 0; i < 40; i++) {
    feed(0x0000, 0, 0, 0xFFFE0000u + i * 5000u);
  }
  TEST_ASSERT_EQUAL_UINT32(55000u, rds.info.minute.spanMs);
  TEST_ASSERT_TRUE(rds.info.minute.groups > 0);
}

static void a_damaged_block_b_counts_no_type(void) {
  RdsRead r = group(OWN_PI, 0x2000, 0, 0, 0);
  r.error[1] = 3;
  r.error[2] = 1;
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT16(1, rds.info.minute.groups);
  TEST_ASSERT_EQUAL_UINT16(0, rds.info.minute.types[2][0]);
  TEST_ASSERT_EQUAL_UINT16(1, rds.info.minute.blocks[1][RDS_LEVEL_LOST]);
  TEST_ASSERT_EQUAL_UINT16(1, rds.info.minute.blocks[2][RDS_LEVEL_CORRECTED]);
  TEST_ASSERT_EQUAL_UINT16(1, rds.info.minute.blocks[0][RDS_LEVEL_CLEAN]);
}

/* A slot is a byte. Past 255 in five seconds, which no station sends, the
 * count stops rather than wraps to a small number. */
static void a_full_slot_stops_counting(void) {
  for (int i = 0; i < 300; i++) {
    feed(0x0000, 0, 0, 0);
  }
  TEST_ASSERT_EQUAL_UINT16(255, rds.info.minute.groups);
  TEST_ASSERT_EQUAL_UINT16(255, rds.info.minute.types[0][0]);
}

static void a_new_station_starts_the_minute_again(void) {
  twice(0x0000, 0, 0);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_UINT16(2, rds.info.minute.groups);
  RdsRead r = group(0x2345, 0x0000, 0, 0, 0);
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT16(0x2345, rds.info.pi);
  TEST_ASSERT_EQUAL_UINT16(1, rds.info.minute.groups);
}

/* -------------------------------------------------------------- language */

static uint16_t blockC1AVariant(uint8_t variant, uint16_t low12) {
  return (uint16_t)(((uint16_t)(variant & 7u) << 12) | (low12 & 0x0FFFu));
}

static void the_language_is_taken_on_its_second_hearing(void) {
  feed(0x1000, blockC1AVariant(3, 0x09), 0, 0);
  TEST_ASSERT_FALSE(rds.info.hasLanguage);
  feed(0x1000, blockC1AVariant(3, 0x09), 0, 0);
  TEST_ASSERT_TRUE(rds.info.hasLanguage);
  TEST_ASSERT_EQUAL_UINT8(0x09, rds.info.language);
  TEST_ASSERT_EQUAL_STRING("English", rdsLanguageName(rds.info.language));
  /* Variant 3 is not the ECC. */
  TEST_ASSERT_FALSE(rds.info.hasEcc);
}

static void a_language_that_is_not_one_is_not_taken(void) {
  twice(0x1000, blockC1AVariant(3, 0x00), 0);
  TEST_ASSERT_FALSE(rds.info.hasLanguage);
  twice(0x1000, blockC1AVariant(3, 0x80), 0);
  TEST_ASSERT_FALSE(rds.info.hasLanguage);
  twice(0x1000, blockC1AVariant(3, 0x109), 0);
  TEST_ASSERT_FALSE(rds.info.hasLanguage);
  /* The B version has the PI in block C. */
  twice(0x1800, blockC1AVariant(3, 0x09), 0);
  TEST_ASSERT_FALSE(rds.info.hasLanguage);
}

static void languages_are_named_by_the_table(void) {
  TEST_ASSERT_NULL(rdsLanguageName(0x00));
  TEST_ASSERT_EQUAL_STRING("Albanian", rdsLanguageName(0x01));
  TEST_ASSERT_EQUAL_STRING("Walloon", rdsLanguageName(0x2B));
  TEST_ASSERT_NULL(rdsLanguageName(0x2C));
  TEST_ASSERT_NULL(rdsLanguageName(0x3F));
  TEST_ASSERT_EQUAL_STRING("Background", rdsLanguageName(0x40));
  TEST_ASSERT_NULL(rdsLanguageName(0x44));
  TEST_ASSERT_EQUAL_STRING("Zulu", rdsLanguageName(0x45));
  TEST_ASSERT_EQUAL_STRING("Telugu", rdsLanguageName(0x4B));
  TEST_ASSERT_EQUAL_STRING("Hindi", rdsLanguageName(0x6B));
  TEST_ASSERT_EQUAL_STRING("Amharic", rdsLanguageName(0x7F));
  TEST_ASSERT_NULL(rdsLanguageName(0x80));
}

/* ------------------------------------------------------------------- RT+ */

/* 3A: block B's low five bits name the group, 11A is 10110. */
#define GROUP_11A 22u
#define AID_RTPLUS 0x4BD7u

static uint16_t blockB3A(uint8_t code) {
  return (uint16_t)((3u << 12) | (code & 0x1Fu));
}

/* An 11A RT+ group from its two tags, IEC 62106-6 annex B. */
static void rtPlusGroup(bool toggle, bool running, uint8_t t1, uint8_t s1,
                        uint8_t l1, uint8_t t2, uint8_t s2, uint8_t l2,
                        uint16_t *b, uint16_t *c, uint16_t *d) {
  *b = (uint16_t)((11u << 12) | (toggle ? 0x10u : 0u) | (running ? 0x08u : 0u) |
                  ((t1 >> 3) & 0x07u));
  *c = (uint16_t)(((uint16_t)(t1 & 0x07u) << 13) |
                  ((uint16_t)(s1 & 0x3Fu) << 7) |
                  ((uint16_t)((l1 - 1u) & 0x3Fu) << 1) | ((t2 >> 5) & 0x01u));
  *d = (uint16_t)(((uint16_t)(t2 & 0x1Fu) << 11) |
                  ((uint16_t)(s2 & 0x3Fu) << 5) | ((l2 - 1u) & 0x1Fu));
}

static void announce(void) {
  twice(blockB3A(GROUP_11A), 0x0000, AID_RTPLUS);
}

/* Send a radio text with the given A/B flag, all of it, with its terminator. */
static void sendText(const char *text, bool flag) {
  char buf[64];
  memset(buf, ' ', sizeof(buf));
  const size_t n = strlen(text);
  memcpy(buf, text, n);
  if (n < 64) {
    buf[n] = 0x0D;
  }
  const int segments = (int)((n + 1 + 3) / 4);
  for (int s = 0; s < segments && s < 16; s++) {
    const uint16_t b =
        (uint16_t)((2u << 12) | (flag ? 0x10u : 0u) | (uint16_t)s);
    feed(b, pair(buf[s * 4], buf[s * 4 + 1]),
         pair(buf[s * 4 + 2], buf[s * 4 + 3]), 0);
  }
}

static void rt_plus_is_taken_once_announced_twice(void) {
  feed(blockB3A(GROUP_11A), 0, AID_RTPLUS, 0);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  feed(blockB3A(GROUP_11A), 0, AID_RTPLUS, 0);
  TEST_ASSERT_TRUE(rds.info.rtPlus);
}

/* TMC, AID CD46, taken on its second hearing like RT+'s announcement, and
 * gone with the station. North America's call letters need it. */
static void tmc_is_taken_once_announced_twice(void) {
  feed(blockB3A(16), 0, 0xCD46, 0);
  TEST_ASSERT_FALSE(rds.info.tmc);
  feed(blockB3A(16), 0, 0xCD46, 0);
  TEST_ASSERT_TRUE(rds.info.tmc);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  rdsReset(&rds, TUNED_KHZ);
  TEST_ASSERT_FALSE(rds.info.tmc);
}

static void another_application_is_not_rt_plus(void) {
  twice(blockB3A(GROUP_11A), 0, 0xCD46);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  /* 2A carries radio text, so it cannot carry RT+. */
  twice(blockB3A(4), 0, AID_RTPLUS);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  /* 4A, 10A, 3A and 14A carry data of their own too. */
  twice(blockB3A(8), 0, AID_RTPLUS);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  twice(blockB3A(20), 0, AID_RTPLUS);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  twice(blockB3A(6), 0, AID_RTPLUS);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  twice(blockB3A(28), 0, AID_RTPLUS);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  /* A B version has the PI in block C, so it cannot carry the tags. */
  twice(blockB3A(7), 0, AID_RTPLUS);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  twice(blockB3A(23), 0, AID_RTPLUS);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  twice(blockB3A(10), 0, AID_RTPLUS);
  TEST_ASSERT_TRUE(rds.info.rtPlus);
}

static void rt_plus_tags_cut_the_title_and_artist_out_of_the_text(void) {
  announce();
  sendText("DREAMS BY FLEETWOOD MAC", false);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  uint16_t b, c, d;
  rtPlusGroup(false, true, 1, 0, 6, 4, 10, 13, &b, &c, &d);
  feed(b, c, d, 0);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.rtPlusCount);
  feed(b, c, d, 0);
  TEST_ASSERT_EQUAL_UINT8(2, rds.info.rtPlusCount);
  TEST_ASSERT_TRUE(rds.info.rtPlusRunning);
  char out[64];
  TEST_ASSERT_TRUE(rdsRtPlusText(&rds.info, 0, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("DREAMS", out);
  TEST_ASSERT_EQUAL_STRING("TITLE", rdsRtPlusLabel(rds.info.rtPlusTag[0].type));
  TEST_ASSERT_TRUE(rdsRtPlusText(&rds.info, 1, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("FLEETWOOD MAC", out);
  TEST_ASSERT_EQUAL_STRING("ARTIST",
                           rdsRtPlusLabel(rds.info.rtPlusTag[1].type));
}

static void a_tag_group_before_the_announcement_is_nothing(void) {
  uint16_t b, c, d;
  rtPlusGroup(false, true, 1, 0, 6, 4, 10, 13, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.rtPlusCount);
}

static void a_damaged_tag_group_is_not_read(void) {
  announce();
  uint16_t b, c, d;
  rtPlusGroup(false, true, 1, 0, 6, 0, 0, 1, &b, &c, &d);
  RdsRead r = group(OWN_PI, b, c, d, 0);
  r.error[3] = 1;
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.rtPlusCount);
}

static void a_new_item_empties_the_tags(void) {
  announce();
  sendText("DREAMS BY FLEETWOOD MAC", false);
  uint16_t b, c, d;
  rtPlusGroup(false, true, 1, 0, 6, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.rtPlusCount);
  rtPlusGroup(true, false, 4, 10, 13, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.rtPlusCount);
  TEST_ASSERT_EQUAL_UINT8(4, rds.info.rtPlusTag[0].type);
  TEST_ASSERT_FALSE(rds.info.rtPlusRunning);
}

static void a_new_radio_text_empties_the_tags(void) {
  announce();
  sendText("DREAMS BY FLEETWOOD MAC", false);
  uint16_t b, c, d;
  rtPlusGroup(false, true, 1, 0, 6, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.rtPlusCount);
  sendText("NEWS AT NINE", true);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.rtPlusCount);
}

/* The station's own short name, content type 31, is kept when the radio text
 * changes and its tags are emptied. */
static void the_short_name_outlives_a_new_radio_text(void) {
  announce();
  sendText("NOW ON WXYZ 101.1", false);
  uint16_t b, c, d;
  rtPlusGroup(false, false, 31, 7, 4, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_TRUE(rds.info.hasStationShort);
  TEST_ASSERT_EQUAL_STRING("WXYZ", rds.info.stationShort);
  sendText("NEWS AT NINE", true);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.rtPlusCount);
  TEST_ASSERT_TRUE(rds.info.hasStationShort);
  TEST_ASSERT_EQUAL_STRING("WXYZ", rds.info.stationShort);
}

/* A newer one replaces it, and it goes with the station. */
static void the_short_name_goes_with_the_station(void) {
  announce();
  sendText("NOW ON WXYZ 101.1", false);
  uint16_t b, c, d;
  rtPlusGroup(false, false, 31, 7, 4, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  sendText("HOT 97 PLAYS HITS", true);
  rtPlusGroup(false, false, 31, 0, 6, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_STRING("HOT 97", rds.info.stationShort);
  rdsReset(&rds, TUNED_KHZ + 100);
  TEST_ASSERT_FALSE(rds.info.hasStationShort);
  TEST_ASSERT_EQUAL_STRING("", rds.info.stationShort);
}

/* Tags heard before the text is whole name nothing yet; the name is kept
 * once the text arrives. */
static void a_short_name_is_kept_once_its_text_arrives(void) {
  announce();
  uint16_t b, c, d;
  rtPlusGroup(false, false, 31, 7, 4, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_FALSE(rds.info.hasStationShort);
  sendText("NOW ON WXYZ 101.1", false);
  TEST_ASSERT_TRUE(rds.info.hasStationShort);
  TEST_ASSERT_EQUAL_STRING("WXYZ", rds.info.stationShort);
}

/* Longer than sixteen characters it is never kept cut short. */
static void a_short_name_too_long_is_not_kept(void) {
  announce();
  sendText("ON THIS STATION CALLED WXYZ-FM ALL DAY", false);
  uint16_t b, c, d;
  rtPlusGroup(false, false, 31, 0, 17, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_FALSE(rds.info.hasStationShort);
  rtPlusGroup(false, false, 31, 0, 16, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_TRUE(rds.info.hasStationShort);
  TEST_ASSERT_EQUAL_STRING("ON THIS STATION", rds.info.stationShort);
}

static void a_tag_is_kept_inside_the_text(void) {
  announce();
  sendText("HI THERE", false);
  uint16_t b, c, d;
  /* Title from 3, eight long, reaches past the end of an eight character
   * text: cut at the end. Artist from 40 starts past it: no words. */
  rtPlusGroup(false, false, 1, 3, 8, 4, 40, 5, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_UINT8(2, rds.info.rtPlusCount);
  char out[64];
  TEST_ASSERT_TRUE(rdsRtPlusText(&rds.info, 0, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("THERE", out);
  TEST_ASSERT_FALSE(rdsRtPlusText(&rds.info, 1, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
  /* Just a space. */
  rtPlusGroup(false, false, 2, 2, 1, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_FALSE(rdsRtPlusText(&rds.info, 2, out, sizeof(out)));
  /* No such tag, a NULL, and room for one character. */
  TEST_ASSERT_FALSE(rdsRtPlusText(&rds.info, 3, out, sizeof(out)));
  TEST_ASSERT_FALSE(rdsRtPlusText(NULL, 0, out, sizeof(out)));
  TEST_ASSERT_FALSE(rdsRtPlusText(&rds.info, 0, NULL, 4));
  TEST_ASSERT_TRUE(rdsRtPlusText(&rds.info, 0, out, 3));
  TEST_ASSERT_EQUAL_STRING("TH", out);
}

static void no_radio_text_gives_no_tag_words(void) {
  announce();
  uint16_t b, c, d;
  rtPlusGroup(false, false, 1, 0, 6, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.rtPlusCount);
  char out[16];
  TEST_ASSERT_FALSE(rdsRtPlusText(&rds.info, 0, out, sizeof(out)));
}

/* A station may send two tag groups in turn; each is still heard twice. */
static void two_tag_groups_in_turn_are_both_taken(void) {
  announce();
  sendText("DREAMS BY FLEETWOOD MAC", false);
  uint16_t b1, c1, d1, b2, c2, d2;
  rtPlusGroup(false, true, 1, 0, 6, 0, 0, 1, &b1, &c1, &d1);
  rtPlusGroup(false, true, 4, 10, 13, 0, 0, 1, &b2, &c2, &d2);
  feed(b1, c1, d1, 0);
  feed(b2, c2, d2, 0);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.rtPlusCount);
  feed(b1, c1, d1, 0);
  feed(b2, c2, d2, 0);
  TEST_ASSERT_EQUAL_UINT8(2, rds.info.rtPlusCount);
}

/* Three tag groups in turn are all taken, each on its second hearing. */
static void three_tag_groups_in_turn_are_all_taken(void) {
  announce();
  sendText("DREAMS BY FLEETWOOD MAC ON RUMOURS", false);
  uint16_t b[3], c[3], d[3];
  rtPlusGroup(false, true, 1, 0, 6, 0, 0, 1, &b[0], &c[0], &d[0]);
  rtPlusGroup(false, true, 4, 10, 13, 0, 0, 1, &b[1], &c[1], &d[1]);
  rtPlusGroup(false, true, 2, 27, 7, 0, 0, 1, &b[2], &c[2], &d[2]);
  for (int round = 0; round < 2; round++) {
    for (int g = 0; g < 3; g++) {
      feed(b[g], c[g], d[g], 0);
    }
  }
  TEST_ASSERT_EQUAL_UINT8(3, rds.info.rtPlusCount);
}

/* A blank text between songs empties the tags as well. */
static void a_blank_text_empties_the_tags(void) {
  announce();
  sendText("DREAMS BY FLEETWOOD MAC", false);
  uint16_t b, c, d;
  rtPlusGroup(false, true, 1, 0, 6, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.rtPlusCount);
  sendText("", false);
  TEST_ASSERT_FALSE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.rtPlusCount);
  feed(b, c, d, 0);
  sendText("NEWS AT NINE", false);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.rtPlusCount);
}

/* A new text under the same flag empties the tags, and a tag group heard
 * once before the change is not taken on one hearing after it. */
static void a_new_text_under_the_same_flag_empties_the_tags(void) {
  announce();
  sendText("DREAMS BY FLEETWOOD MAC", false);
  uint16_t b, c, d;
  rtPlusGroup(false, true, 1, 0, 6, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.rtPlusCount);
  feed(b, c, d, 0);
  sendText("MAGIC 106.4 YOUR HITS", false);
  TEST_ASSERT_EQUAL_STRING("MAGIC 106.4 YOUR HITS", rds.info.rt);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.rtPlusCount);
  feed(b, c, d, 0);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.rtPlusCount);
}

static void the_tag_list_keeps_the_latest_four_types(void) {
  announce();
  uint16_t b, c, d;
  const uint8_t types[5] = {1, 2, 4, 33, 36};
  for (int i = 0; i < 5; i++) {
    rtPlusGroup(false, true, types[i], (uint8_t)i, 1, 0, 0, 1, &b, &c, &d);
    twice(b, c, d);
  }
  TEST_ASSERT_EQUAL_UINT8(RDS_RTPLUS_MAX, rds.info.rtPlusCount);
  TEST_ASSERT_EQUAL_UINT8(2, rds.info.rtPlusTag[0].type);
  TEST_ASSERT_EQUAL_UINT8(36, rds.info.rtPlusTag[3].type);
  /* The same type again moves nothing, only where it points. */
  rtPlusGroup(false, true, 4, 20, 3, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_UINT8(RDS_RTPLUS_MAX, rds.info.rtPlusCount);
  TEST_ASSERT_EQUAL_UINT8(4, rds.info.rtPlusTag[1].type);
  TEST_ASSERT_EQUAL_UINT8(20, rds.info.rtPlusTag[1].start);
  /* A tag past the 64th character is not one. */
  rtPlusGroup(false, true, 5, 60, 8, 0, 0, 1, &b, &c, &d);
  twice(b, c, d);
  TEST_ASSERT_EQUAL_UINT8(2, rds.info.rtPlusTag[0].type);
}

static void rt_plus_labels_cover_every_type(void) {
  TEST_ASSERT_EQUAL_STRING("OTHER", rdsRtPlusLabel(0));
  TEST_ASSERT_EQUAL_STRING("TITLE", rdsRtPlusLabel(1));
  TEST_ASSERT_EQUAL_STRING("ARTIST", rdsRtPlusLabel(4));
  TEST_ASSERT_EQUAL_STRING("PROGRAM", rdsRtPlusLabel(33));
  TEST_ASSERT_EQUAL_STRING("HOST", rdsRtPlusLabel(36));
  TEST_ASSERT_EQUAL_STRING("OTHER", rdsRtPlusLabel(54));
  TEST_ASSERT_EQUAL_STRING("PLACE", rdsRtPlusLabel(59));
  TEST_ASSERT_EQUAL_STRING("APPOINTMENT", rdsRtPlusLabel(60));
  TEST_ASSERT_EQUAL_STRING("OTHER", rdsRtPlusLabel(63));
  TEST_ASSERT_EQUAL_STRING("TITLE", rdsRtPlusLabel(65));
}

/* ------------------------------------------------------------------- EON */

static uint16_t blockB14A(bool tp, uint8_t variant) {
  return (uint16_t)((14u << 12) | (tp ? 0x10u : 0u) | (variant & 0x0Fu));
}

static void eonName(uint16_t pi, const char *name) {
  for (uint8_t v = 0; v < 4; v++) {
    feed(blockB14A(true, v), pair(name[v * 2], name[v * 2 + 1]), pi, 0);
  }
}

/* A name is taken on two whole passes that agree, as the station's own. */
static void eon_builds_a_name_from_its_four_variants(void) {
  eonName(0x5242, "RADIO 2 ");
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.eonCount);
  TEST_ASSERT_FALSE(rds.info.eon[0].hasPs);
  eonName(0x5242, "RADIO 2 ");
  TEST_ASSERT_TRUE(rds.info.eon[0].hasPs);
  TEST_ASSERT_EQUAL_STRING("RADIO 2 ", rds.info.eon[0].ps);
  TEST_ASSERT_EQUAL_UINT16(0x5242, rds.info.eon[0].pi);
  TEST_ASSERT_EQUAL_UINT8(8, rds.info.eon[0].heard);
  TEST_ASSERT_TRUE(rds.info.eon[0].hasTp);
  TEST_ASSERT_TRUE(rds.info.eon[0].tp);
}

/* One damaged segment makes a pass that does not match, so the mixed name
 * is never shown; the old one stays until a new name is heard twice. */
static void eon_does_not_show_a_mixed_name(void) {
  eonName(0x5242, "RADIO 2 ");
  eonName(0x5242, "RADIO 2 ");
  feed(blockB14A(true, 0), pair('X', 'X'), 0x5242, 0);
  feed(blockB14A(true, 1), pair('D', 'I'), 0x5242, 0);
  feed(blockB14A(true, 2), pair('O', ' '), 0x5242, 0);
  feed(blockB14A(true, 3), pair('2', ' '), 0x5242, 0);
  TEST_ASSERT_EQUAL_STRING("RADIO 2 ", rds.info.eon[0].ps);
  eonName(0x5242, "RADIO 9 ");
  TEST_ASSERT_EQUAL_STRING("RADIO 2 ", rds.info.eon[0].ps);
  eonName(0x5242, "RADIO 9 ");
  TEST_ASSERT_EQUAL_STRING("RADIO 9 ", rds.info.eon[0].ps);
  /* A segment again before the pass is whole starts the pass again. */
  feed(blockB14A(true, 0), pair('R', 'A'), 0x5242, 0);
  feed(blockB14A(true, 0), pair('R', 'A'), 0x5242, 0);
  TEST_ASSERT_EQUAL_UINT8(0x03, rds.eonPsHave[0]);
}

/* 250 in the high byte says the low byte is long or medium wave. */
static void eon_leaves_out_a_long_wave_pair(void) {
  feed(blockB14A(false, 4), (uint16_t)((250u << 8) | 16u), 0x5242, 0);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.eon[0].afCount);
}

static void eon_frequencies_come_from_method_a_and_the_mapped_ones(void) {
  /* 89.9 is code 24, 90.6 code 31; 225 is a count, not a frequency. */
  feed(blockB14A(false, 4), (uint16_t)((225u << 8) | 24u), 0x5242, 0);
  feed(blockB14A(false, 4), (uint16_t)((31u << 8) | 24u), 0x5242, 0);
  /* Mapped: this station's 106.4 in the high byte, the other's 91.3. */
  feed(blockB14A(false, 5), (uint16_t)((189u << 8) | 38u), 0x5242, 0);
  const RdsEon *e = &rds.info.eon[0];
  TEST_ASSERT_EQUAL_UINT8(3, e->afCount);
  TEST_ASSERT_EQUAL_UINT32(89900, rdsAfCodeKHz(e->afCode[0]));
  TEST_ASSERT_EQUAL_UINT32(90600, rdsAfCodeKHz(e->afCode[1]));
  TEST_ASSERT_EQUAL_UINT32(91300, rdsAfCodeKHz(e->afCode[2]));
  TEST_ASSERT_EQUAL_UINT32(87600, rdsAfCodeKHz(1));
  TEST_ASSERT_EQUAL_UINT32(107900, rdsAfCodeKHz(204));
  TEST_ASSERT_EQUAL_UINT32(0, rdsAfCodeKHz(0));
  TEST_ASSERT_EQUAL_UINT32(0, rdsAfCodeKHz(205));
  TEST_ASSERT_FALSE(e->afMore);
  for (uint8_t code = 40; code < 45; code++) {
    feed(blockB14A(false, 6), code, 0x5242, 0);
  }
  TEST_ASSERT_EQUAL_UINT8(RDS_EON_AF_MAX, e->afCount);
  TEST_ASSERT_TRUE(e->afMore);
}

static void eon_ta_comes_from_variant_13_and_from_14b(void) {
  feed(blockB14A(true, 13), 0x0001, 0xC204, 0);
  TEST_ASSERT_TRUE(rds.info.eon[0].hasTa);
  TEST_ASSERT_TRUE(rds.info.eon[0].ta);
  const uint16_t b14b = (uint16_t)((14u << 12) | 0x0800u | 0x10u);
  feed(b14b, OWN_PI, 0xC204, 0);
  TEST_ASSERT_FALSE(rds.info.eon[0].ta);
  feed((uint16_t)(b14b | 0x08u), OWN_PI, 0xC204, 0);
  TEST_ASSERT_TRUE(rds.info.eon[0].ta);
  TEST_ASSERT_EQUAL_UINT8(3, rds.info.eon[0].heard);
}

static void eon_ignores_no_station_and_this_station(void) {
  twice(0x0000, 0, 0);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  feed(blockB14A(false, 0), pair('A', 'B'), 0x0000, 0);
  feed(blockB14A(false, 0), pair('A', 'B'), OWN_PI, 0);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.eonCount);
}

static void eon_needs_block_c_for_its_data(void) {
  RdsRead r = group(OWN_PI, blockB14A(true, 0), pair('X', 'Y'), 0x5242, 0);
  r.error[2] = 3;
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.eonCount);
  TEST_ASSERT_EQUAL_UINT8(0, rds.eonPsHave[0]);
  TEST_ASSERT_FALSE(rds.info.eon[0].hasPs);
  TEST_ASSERT_TRUE(rds.info.eon[0].tp);
  r.error[2] = 0;
  r.error[3] = 3;
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.eon[0].heard);
}

/* The first four stay, and a fifth, sent in turn with them, is said to
 * exist rather than swapped in. */
static void eon_keeps_the_first_four_stations(void) {
  for (int round = 0; round < 3; round++) {
    for (uint16_t pi = 0x5241; pi < 0x5246; pi++) {
      feed(blockB14A(false, 13), 0, pi, 0);
    }
  }
  TEST_ASSERT_EQUAL_UINT8(RDS_EON_MAX, rds.info.eonCount);
  TEST_ASSERT_EQUAL_UINT16(0x5241, rds.info.eon[0].pi);
  TEST_ASSERT_EQUAL_UINT16(0x5244, rds.info.eon[3].pi);
  TEST_ASSERT_EQUAL_UINT8(3, rds.info.eon[3].heard);
  TEST_ASSERT_TRUE(rds.info.eonMore);
}

static void a_new_station_forgets_eon_rt_plus_and_language(void) {
  twice(0x0000, 0, 0);
  announce();
  twice(0x1000, blockC1AVariant(3, 0x09), 0);
  feed(blockB14A(false, 13), 0, 0x5242, 0);
  TEST_ASSERT_TRUE(rds.info.rtPlus);
  TEST_ASSERT_TRUE(rds.info.hasLanguage);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.eonCount);
  RdsRead r = group(0x2345, 0x0000, 0, 0, 0);
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.rtPlus);
  TEST_ASSERT_FALSE(rds.info.hasLanguage);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.eonCount);
}

/* ------------------------------------------------------------- stats */

static void the_stats_of_a_minute(void) {
  RdsMinute m;
  memset(&m, 0, sizeof(m));
  m.groups = 684;
  m.spanMs = 60000;
  m.blocks[0][RDS_LEVEL_CLEAN] = 684;
  m.blocks[1][RDS_LEVEL_CLEAN] = 605;
  m.blocks[1][RDS_LEVEL_CORRECTED] = 75;
  m.blocks[1][RDS_LEVEL_LOST] = 4;
  m.blocks[2][RDS_LEVEL_CLEAN] = 680;
  m.blocks[2][RDS_LEVEL_LOST] = 4;
  m.blocks[3][RDS_LEVEL_CLEAN] = 684;
  m.types[2][0] = 342;
  m.types[0][0] = 338;
  m.types[0][1] = 2;
  m.types[14][0] = 2;
  RdsMinuteStats s;
  rdsMinuteStats(&m, 3, &s);
  TEST_ASSERT_TRUE(s.known);
  TEST_ASSERT_TRUE(s.rateKnown);
  TEST_ASSERT_EQUAL_UINT16(114, s.rateTenths);
  TEST_ASSERT_EQUAL_UINT16(8, s.lostBlocks);
  TEST_ASSERT_EQUAL_UINT16(2736, s.totalBlocks);
  TEST_ASSERT_EQUAL_UINT16(3, s.blerTenths);
  TEST_ASSERT_EQUAL_UINT8(100, s.clean[0]);
  TEST_ASSERT_EQUAL_UINT8(88, s.clean[1]);
  TEST_ASSERT_EQUAL_UINT8(11, s.fixed[1]);
  TEST_ASSERT_EQUAL_UINT8(1, s.lost[1]);
  TEST_ASSERT_EQUAL_UINT16(6, s.lostTenths[1]);
  TEST_ASSERT_EQUAL_UINT16(110, s.fixedTenths[1]);
  TEST_ASSERT_EQUAL_UINT16(0, s.lostTenths[3]);
  /* Most first, a tie in type order, 0B before 14A. */
  TEST_ASSERT_EQUAL_UINT8(3, s.groupCount);
  TEST_ASSERT_EQUAL_UINT8(2, s.groupType[0]);
  TEST_ASSERT_FALSE(s.groupIsB[0]);
  TEST_ASSERT_EQUAL_UINT8(50, s.groupPercent[0]);
  TEST_ASSERT_EQUAL_UINT8(0, s.groupType[1]);
  TEST_ASSERT_EQUAL_UINT8(49, s.groupPercent[1]);
  TEST_ASSERT_EQUAL_UINT8(0, s.groupType[2]);
  TEST_ASSERT_TRUE(s.groupIsB[2]);
  TEST_ASSERT_EQUAL_UINT8(0, s.groupPercent[2]);
}

static void the_stats_of_an_empty_or_short_minute(void) {
  RdsMinute m;
  memset(&m, 0, sizeof(m));
  RdsMinuteStats s;
  rdsMinuteStats(&m, 6, &s);
  TEST_ASSERT_FALSE(s.known);
  TEST_ASSERT_EQUAL_UINT8(0, s.groupCount);
  rdsMinuteStats(NULL, 6, &s);
  TEST_ASSERT_FALSE(s.known);
  rdsMinuteStats(&m, 6, NULL);
  m.groups = 5;
  m.spanMs = 400;
  m.types[0][0] = 5;
  rdsMinuteStats(&m, 200, &s);
  TEST_ASSERT_TRUE(s.known);
  TEST_ASSERT_FALSE(s.rateKnown);
  TEST_ASSERT_EQUAL_UINT8(1, s.groupCount);
  TEST_ASSERT_EQUAL_UINT8(100, s.groupPercent[0]);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(the_minute_counts_the_weak_capture);
  RUN_TEST(old_groups_leave_the_minute);
  RUN_TEST(a_minute_with_no_reads_empties_it);
  RUN_TEST(the_span_grows_from_the_first_read);
  RUN_TEST(the_span_survives_the_clock_wrapping);
  RUN_TEST(a_damaged_block_b_counts_no_type);
  RUN_TEST(a_full_slot_stops_counting);
  RUN_TEST(a_new_station_starts_the_minute_again);
  RUN_TEST(the_language_is_taken_on_its_second_hearing);
  RUN_TEST(a_language_that_is_not_one_is_not_taken);
  RUN_TEST(languages_are_named_by_the_table);
  RUN_TEST(rt_plus_is_taken_once_announced_twice);
  RUN_TEST(tmc_is_taken_once_announced_twice);
  RUN_TEST(another_application_is_not_rt_plus);
  RUN_TEST(rt_plus_tags_cut_the_title_and_artist_out_of_the_text);
  RUN_TEST(a_tag_group_before_the_announcement_is_nothing);
  RUN_TEST(a_damaged_tag_group_is_not_read);
  RUN_TEST(a_new_item_empties_the_tags);
  RUN_TEST(a_new_radio_text_empties_the_tags);
  RUN_TEST(the_short_name_outlives_a_new_radio_text);
  RUN_TEST(the_short_name_goes_with_the_station);
  RUN_TEST(a_short_name_is_kept_once_its_text_arrives);
  RUN_TEST(a_short_name_too_long_is_not_kept);
  RUN_TEST(a_tag_is_kept_inside_the_text);
  RUN_TEST(no_radio_text_gives_no_tag_words);
  RUN_TEST(two_tag_groups_in_turn_are_both_taken);
  RUN_TEST(a_new_text_under_the_same_flag_empties_the_tags);
  RUN_TEST(three_tag_groups_in_turn_are_all_taken);
  RUN_TEST(a_blank_text_empties_the_tags);
  RUN_TEST(the_tag_list_keeps_the_latest_four_types);
  RUN_TEST(rt_plus_labels_cover_every_type);
  RUN_TEST(eon_builds_a_name_from_its_four_variants);
  RUN_TEST(eon_does_not_show_a_mixed_name);
  RUN_TEST(eon_leaves_out_a_long_wave_pair);
  RUN_TEST(eon_frequencies_come_from_method_a_and_the_mapped_ones);
  RUN_TEST(eon_ta_comes_from_variant_13_and_from_14b);
  RUN_TEST(eon_ignores_no_station_and_this_station);
  RUN_TEST(eon_needs_block_c_for_its_data);
  RUN_TEST(eon_keeps_the_first_four_stations);
  RUN_TEST(a_new_station_forgets_eon_rt_plus_and_language);
  RUN_TEST(the_stats_of_a_minute);
  RUN_TEST(the_stats_of_an_empty_or_short_minute);
  return UNITY_END();
}
