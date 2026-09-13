/* Tests for the RDS decoder. Runs on a PC. */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/rds.h"

#include "captures.h"

/* A station on 101.9, which is where the captures were taken. */
#define TUNED_KHZ 101900u

static Rds rds;

void setUp(void) {
  rdsReset(&rds, TUNED_KHZ);
}
void tearDown(void) {}

static RdsRead group(uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  RdsRead r;
  memset(&r, 0, sizeof(r));
  r.synchronised = true;
  r.haveGroup = true;
  r.block[0] = a;
  r.block[1] = b;
  r.block[2] = c;
  r.block[3] = d;
  return r;
}

/* Block B for a group 0A: the flags a station sends with its name. */
static uint16_t blockB0A(uint8_t pty, bool tp, bool ta, bool music,
                         uint8_t segment) {
  return (uint16_t)((0u << 12) | (tp ? 0x0400u : 0u) |
                    ((uint16_t)(pty & 0x1F) << 5) | (ta ? 0x0010u : 0u) |
                    (music ? 0x0008u : 0u) | (segment & 0x03u));
}

static uint16_t blockB0B(uint8_t pty, uint8_t segment) {
  return (uint16_t)((0u << 12) | 0x0800u | ((uint16_t)(pty & 0x1F) << 5) |
                    (segment & 0x03u));
}

static uint16_t blockB2A(bool flag, uint8_t segment) {
  return (uint16_t)((2u << 12) | (flag ? 0x0010u : 0u) | (segment & 0x0Fu));
}

static uint16_t blockB2B(bool flag, uint8_t segment) {
  return (uint16_t)((2u << 12) | 0x0800u | (flag ? 0x0010u : 0u) |
                    (segment & 0x0Fu));
}

static uint16_t pair(char a, char b) {
  return (uint16_t)(((uint16_t)(uint8_t)a << 8) | (uint8_t)b);
}

/* Send one segment of a station name, twice, so it settles. */
static void sendPs(const char *name, uint8_t pty, bool tp, bool ta,
                   bool music) {
  for (int round = 0; round < 2; round++) {
    for (uint8_t seg = 0; seg < 4; seg++) {
      RdsRead r = group(0x1234, blockB0A(pty, tp, ta, music, seg), 0,
                        pair(name[seg * 2], name[seg * 2 + 1]));
      rdsFeed(&rds, &r);
    }
  }
}

/* ------------------------------------------------------------- the start -- */

static void a_fresh_decoder_claims_nothing(void) {
  TEST_ASSERT_FALSE(rds.info.synchronised);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  TEST_ASSERT_FALSE(rds.info.hasPs);
  TEST_ASSERT_FALSE(rds.info.hasRt);
  TEST_ASSERT_FALSE(rds.info.hasPty);
  TEST_ASSERT_FALSE(rds.info.hasFlags);
  TEST_ASSERT_FALSE(rds.info.clock.valid);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.afCount);
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.groupsSeen);
}

static void nulls_do_not_crash(void) {
  RdsRead r = group(0x1234, 0, 0, 0);
  rdsReset(NULL, 0);
  rdsFeed(NULL, &r);
  rdsFeed(&rds, NULL);
  TEST_ASSERT_FALSE(rds.info.synchronised);
}

/* --------------------------------------------------------------- the lock -- */

static void a_read_with_a_lock_and_no_group_is_still_a_lock(void) {
  RdsRead r;
  memset(&r, 0, sizeof(r));
  r.synchronised = true;
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.synchronised);
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.groupsSeen);
}

static void the_lock_survives_one_read_without_it(void) {
  RdsRead locked;
  memset(&locked, 0, sizeof(locked));
  locked.synchronised = true;
  rdsFeed(&rds, &locked);

  RdsRead lost;
  memset(&lost, 0, sizeof(lost));
  rdsFeed(&rds, &lost);
  TEST_ASSERT_TRUE(rds.info.synchronised);

  rdsFeed(&rds, &lost);
  TEST_ASSERT_FALSE(rds.info.synchronised);
}

static void one_locked_read_brings_the_lock_straight_back(void) {
  RdsRead lost;
  memset(&lost, 0, sizeof(lost));
  rdsFeed(&rds, &lost);
  rdsFeed(&rds, &lost);
  TEST_ASSERT_FALSE(rds.info.synchronised);

  RdsRead locked = lost;
  locked.synchronised = true;
  rdsFeed(&rds, &locked);
  TEST_ASSERT_TRUE(rds.info.synchronised);
}

/* ------------------------------------------------------- the identifier -- */

static void the_identifier_waits_for_a_second_reception(void) {
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x1234, rds.info.pi);
}

static void two_different_identifiers_settle_neither(void) {
  RdsRead a = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  RdsRead b = group(0x5678, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &a);
  rdsFeed(&rds, &b);
  rdsFeed(&rds, &a);
  TEST_ASSERT_FALSE(rds.info.hasPi);
}

static void a_block_the_tuner_could_not_correct_is_not_an_identifier(void) {
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  r.error[0] = 3;
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_UINT32(2, rds.info.blocksBad);
}

/*
 * A block the tuner says it corrected is thrown away too.
 *
 * Measured, not chosen. Both weak captures in test/fixtures/rds/ put corrupt
 * radio text on the panel through blocks reported as corrected at the
 * smallest error the chip reports.
 */
static void a_block_the_tuner_corrected_is_not_taken(void) {
  RdsRead one = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  one.error[0] = 1;
  rdsFeed(&rds, &one);
  rdsFeed(&rds, &one);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_UINT32(2, rds.info.blocksCorrected);
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.blocksBad);

  RdsRead two = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  two.error[0] = 2;
  rdsFeed(&rds, &two);
  rdsFeed(&rds, &two);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_UINT32(4, rds.info.blocksCorrected);
}

static void an_identifier_of_zero_is_no_identifier(void) {
  RdsRead r = group(0x0000, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasPi);
}

static void an_identifier_of_zero_does_not_disturb_a_real_one(void) {
  RdsRead real = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &real);
  rdsFeed(&rds, &real);
  TEST_ASSERT_TRUE(rds.info.hasPi);

  RdsRead none = group(0x0000, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &none);
  rdsFeed(&rds, &none);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x1234, rds.info.pi);
}

static void the_group_that_changes_the_station_counts_as_its_first(void) {
  RdsRead first = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &first);
  rdsFeed(&rds, &first);
  rdsFeed(&rds, &first);
  TEST_ASSERT_EQUAL_UINT32(3, rds.info.groupsSeen);

  RdsRead other = group(0x9999, blockB0A(1, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &other);
  rdsFeed(&rds, &other);
  /* The second one confirmed the change and cleared the counters, so it is
   * group one of the new station. Never one used out of none seen. */
  TEST_ASSERT_EQUAL_UINT32(1, rds.info.groupsSeen);
  TEST_ASSERT_EQUAL_UINT32(1, rds.info.groupsUsed);
  TEST_ASSERT_TRUE(rds.info.groupsUsed <= rds.info.groupsSeen);
}

static void a_new_identifier_throws_the_old_station_away(void) {
  sendPs("TESTFM  ", 10, true, false, true);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_TRUE(rds.info.synchronised);

  RdsRead other = group(0x9999, blockB0A(1, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &other);
  rdsFeed(&rds, &other);

  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x9999, rds.info.pi);
  TEST_ASSERT_FALSE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING("", rds.info.ps);
  /* The dial has not moved and the tuner is still locked, so neither of
   * those is thrown away with the station. */
  TEST_ASSERT_TRUE(rds.info.synchronised);
}

/* ---------------------------------------------------- the programme type -- */

static void the_programme_type_waits_for_a_second_reception(void) {
  RdsRead r = group(0x1234, blockB0A(24, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasPty);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.hasPty);
  TEST_ASSERT_EQUAL_UINT8(24, rds.info.pty);
}

static void the_programme_type_comes_from_every_group(void) {
  /* Group 8A is a traffic message group, which this radio does not decode,
   * but its block B carries the programme type like every other group. */
  RdsRead r = group(0x1234, (uint16_t)((8u << 12) | (7u << 5)), 0, 0);
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.hasPty);
  TEST_ASSERT_EQUAL_UINT8(7, rds.info.pty);
}

static void every_programme_type_has_a_name(void) {
  for (uint8_t i = 0; i < RDS_PTY_COUNT; i++) {
    TEST_ASSERT_NOT_NULL(rdsPtyName(i));
    TEST_ASSERT_TRUE(rdsPtyName(i)[0] != '\0');
  }
  TEST_ASSERT_EQUAL_STRING("None", rdsPtyName(RDS_PTY_COUNT));
  TEST_ASSERT_EQUAL_STRING("None", rdsPtyName(255));
  TEST_ASSERT_EQUAL_STRING("Alarm", rdsPtyName(31));
}

/* --------------------------------------------------------------- the flags -- */

static void the_flags_arrive_with_the_first_group_zero(void) {
  RdsRead r = group(0x1234, blockB0A(10, true, true, false, 0), 0, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.hasFlags);
  TEST_ASSERT_TRUE(rds.info.tp);
  TEST_ASSERT_TRUE(rds.info.ta);
  TEST_ASSERT_TRUE(rds.info.speech);
}

static void the_music_bit_reads_as_music(void) {
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.speech);
}

static void a_group_that_is_not_group_zero_leaves_the_flags_alone(void) {
  RdsRead r = group(0x1234, (uint16_t)((4u << 12) | (10u << 5)), 0, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasFlags);
}

/* -------------------------------------------------------- the station name -- */

static void the_name_waits_for_two_passes_that_agree(void) {
  /* One whole pass is not enough. Every position has arrived, but nothing
   * has confirmed it. */
  for (uint8_t seg = 0; seg < 4; seg++) {
    RdsRead r = group(0x1234, blockB0A(10, false, false, true, seg), 0,
                      pair("TESTFM  "[seg * 2], "TESTFM  "[seg * 2 + 1]));
    rdsFeed(&rds, &r);
    TEST_ASSERT_FALSE(rds.info.hasPs);
  }
  for (uint8_t seg = 0; seg < 3; seg++) {
    RdsRead r = group(0x1234, blockB0A(10, false, false, true, seg), 0,
                      pair("TESTFM  "[seg * 2], "TESTFM  "[seg * 2 + 1]));
    rdsFeed(&rds, &r);
    TEST_ASSERT_FALSE(rds.info.hasPs);
  }
  RdsRead last =
      group(0x1234, blockB0A(10, false, false, true, 3), 0, pair(' ', ' '));
  rdsFeed(&rds, &last);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING("TESTFM  ", rds.info.ps);
}

static void the_segments_of_a_pass_may_arrive_in_any_order(void) {
  const uint8_t order[4] = {2, 0, 3, 1};
  for (int round = 0; round < 2; round++) {
    for (int i = 0; i < 4; i++) {
      uint8_t seg = order[i];
      RdsRead r = group(0x1234, blockB0A(10, false, false, true, seg), 0,
                        pair("TESTFM  "[seg * 2], "TESTFM  "[seg * 2 + 1]));
      rdsFeed(&rds, &r);
    }
  }
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING("TESTFM  ", rds.info.ps);
}

static void a_name_from_a_zero_b_group_decodes_the_same(void) {
  for (int round = 0; round < 2; round++) {
    for (uint8_t seg = 0; seg < 4; seg++) {
      RdsRead r = group(0x1234, blockB0B(10, seg), 0x1234,
                        pair("RADIO ONE"[seg * 2], "RADIO ONE"[seg * 2 + 1]));
      rdsFeed(&rds, &r);
    }
  }
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING("RADIO ON", rds.info.ps);
}

static void one_position_that_keeps_changing_holds_the_name_back(void) {
  const char *good = "TESTFM  ";
  for (int round = 0; round < 4; round++) {
    for (uint8_t seg = 0; seg < 4; seg++) {
      /* Segment 1 alternates, which is what interference on one group looks
       * like. Every other position is steady. */
      char first = good[seg * 2];
      if (seg == 1) {
        first = (round % 2) ? 'X' : 'S';
      }
      RdsRead r = group(0x1234, blockB0A(10, false, false, true, seg), 0,
                        pair(first, good[seg * 2 + 1]));
      rdsFeed(&rds, &r);
    }
  }
  TEST_ASSERT_FALSE(rds.info.hasPs);
}

static void a_damaged_last_block_holds_the_whole_pass_open(void) {
  for (int round = 0; round < 3; round++) {
    for (uint8_t seg = 0; seg < 4; seg++) {
      RdsRead r = group(0x1234, blockB0A(10, false, false, true, seg), 0,
                        pair("TESTFM  "[seg * 2], "TESTFM  "[seg * 2 + 1]));
      if (seg == 2) {
        r.error[3] = 3;
      }
      rdsFeed(&rds, &r);
    }
    TEST_ASSERT_FALSE(rds.info.hasPs);
  }

  /* Two positions of every pass never arrived, so no pass ever closed. Two
   * clean passes now are what it takes, not one segment. */
  sendPs("TESTFM  ", 10, false, false, true);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING("TESTFM  ", rds.info.ps);
}

/*
 * Two names sent in turn, which is how a station scrolls a longer one through
 * the eight characters. 95.0 and 91.1 both do this, and the capture in
 * test/fixtures/rds/ is where the pattern comes from: three passes of one
 * name, then three of the other.
 */
static void a_scrolling_name_never_shows_a_mixture(void) {
  const char *frames[2] = {"MIRCHI 9", "5       "};
  for (int cycle = 0; cycle < 3; cycle++) {
    for (int which = 0; which < 2; which++) {
      for (int pass = 0; pass < 3; pass++) {
        for (uint8_t seg = 0; seg < 4; seg++) {
          RdsRead r =
              group(0x1234, blockB0A(10, false, false, true, seg), 0,
                    pair(frames[which][seg * 2], frames[which][seg * 2 + 1]));
          rdsFeed(&rds, &r);
          if (rds.info.hasPs) {
            /* Whatever is shown is one of the two names the station sent,
             * never a mixture of both. */
            TEST_ASSERT_TRUE(strcmp(rds.info.ps, frames[0]) == 0 ||
                             strcmp(rds.info.ps, frames[1]) == 0);
          }
        }
      }
    }
  }
  TEST_ASSERT_TRUE(rds.info.hasPs);
}

static void two_names_alternating_every_pass_show_neither(void) {
  const char *frames[2] = {"MIRCHI 9", "5       "};
  for (int pass = 0; pass < 8; pass++) {
    for (uint8_t seg = 0; seg < 4; seg++) {
      const char *name = frames[pass % 2];
      RdsRead r = group(0x1234, blockB0A(10, false, false, true, seg), 0,
                        pair(name[seg * 2], name[seg * 2 + 1]));
      rdsFeed(&rds, &r);
    }
  }
  /* No two passes running ever agreed, so there is nothing the station can
   * be said to have called itself. */
  TEST_ASSERT_FALSE(rds.info.hasPs);
}

static void a_damaged_block_b_decodes_nothing_but_the_identifier(void) {
  RdsRead r =
      group(0x1234, blockB0A(10, false, false, true, 0), 0, pair('A', 'B'));
  r.error[1] = 3;
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_FALSE(rds.info.hasPty);
  TEST_ASSERT_FALSE(rds.info.hasFlags);
  TEST_ASSERT_FALSE(rds.info.hasPs);
}

static void a_character_outside_the_shown_range_becomes_a_question_mark(void) {
  for (int round = 0; round < 2; round++) {
    for (uint8_t seg = 0; seg < 4; seg++) {
      uint16_t d = pair("TESTFM  "[seg * 2], "TESTFM  "[seg * 2 + 1]);
      if (seg == 0) {
        d = (uint16_t)(0xC5 << 8 | (uint8_t)'E'); /* An accented letter. */
      }
      RdsRead r = group(0x1234, blockB0A(10, false, false, true, seg), 0, d);
      rdsFeed(&rds, &r);
    }
  }
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING("?ESTFM  ", rds.info.ps);
}

/* ---------------------------------------------------------- the radio text -- */

static void a_full_sixty_four_character_text_is_published(void) {
  const char *text =
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  for (uint8_t seg = 0; seg < 16; seg++) {
    RdsRead r = group(0x1234, blockB2A(false, seg),
                      pair(text[seg * 4], text[seg * 4 + 1]),
                      pair(text[seg * 4 + 2], text[seg * 4 + 3]));
    rdsFeed(&rds, &r);
  }
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING(text, rds.info.rt);
}

static void a_text_ends_at_its_terminator(void) {
  RdsRead a =
      group(0x1234, blockB2A(false, 0), pair('H', 'i'), pair('!', 0x0D));
  rdsFeed(&rds, &a);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("Hi!", rds.info.rt);
}

static void a_text_with_a_hole_in_it_is_not_published(void) {
  RdsRead a = group(0x1234, blockB2A(false, 0), pair('H', 'i'), pair(' ', ' '));
  RdsRead c =
      group(0x1234, blockB2A(false, 2), pair('t', 'h'), pair('e', 0x0D));
  rdsFeed(&rds, &a);
  rdsFeed(&rds, &c);
  TEST_ASSERT_FALSE(rds.info.hasRt);

  RdsRead b = group(0x1234, blockB2A(false, 1), pair('t', 'o'), pair(' ', ' '));
  rdsFeed(&rds, &b);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("Hi  to  the", rds.info.rt);
}

static void trailing_padding_is_not_part_of_the_text(void) {
  RdsRead a = group(0x1234, blockB2A(false, 0), pair('O', 'K'), pair(' ', ' '));
  RdsRead b =
      group(0x1234, blockB2A(false, 1), pair(' ', ' '), pair(' ', 0x0D));
  rdsFeed(&rds, &a);
  rdsFeed(&rds, &b);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("OK", rds.info.rt);
}

static void the_short_text_of_a_two_b_group_decodes(void) {
  const char *text = "Now playing.";
  for (uint8_t seg = 0; seg < 6; seg++) {
    RdsRead r = group(0x1234, blockB2B(false, seg), 0x1234,
                      pair(text[seg * 2], text[seg * 2 + 1]));
    rdsFeed(&rds, &r);
  }
  TEST_ASSERT_FALSE(rds.info.hasRt);
  RdsRead last = group(0x1234, blockB2B(false, 6), 0x1234, pair(0x0D, ' '));
  rdsFeed(&rds, &last);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("Now playing.", rds.info.rt);
}

static void the_flag_toggling_starts_a_new_text(void) {
  RdsRead a = group(0x1234, blockB2A(false, 0), pair('L', 'o'), pair('n', 'g'));
  RdsRead b =
      group(0x1234, blockB2A(false, 1), pair('e', 'r'), pair(' ', 0x0D));
  rdsFeed(&rds, &a);
  rdsFeed(&rds, &b);
  TEST_ASSERT_EQUAL_STRING("Longer", rds.info.rt);

  RdsRead c = group(0x1234, blockB2A(true, 0), pair('N', 'e'), pair('w', 0x0D));
  rdsFeed(&rds, &c);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("New", rds.info.rt);
}

static void the_flag_toggling_drops_a_text_that_was_being_built(void) {
  RdsRead a = group(0x1234, blockB2A(false, 0), pair('L', 'o'), pair('n', 'g'));
  rdsFeed(&rds, &a);
  TEST_ASSERT_FALSE(rds.info.hasRt);

  RdsRead b = group(0x1234, blockB2A(true, 1), pair('X', 'X'), pair('X', 0x0D));
  rdsFeed(&rds, &b);
  /* Segment 0 of the new text has not arrived, so there is nothing to show
   * even though segment 1 carried a terminator. */
  TEST_ASSERT_FALSE(rds.info.hasRt);
}

static void a_damaged_text_block_leaves_its_characters_out(void) {
  RdsRead a = group(0x1234, blockB2A(false, 0), pair('A', 'B'), pair('C', 'D'));
  a.error[2] = 3;
  rdsFeed(&rds, &a);
  RdsRead b =
      group(0x1234, blockB2A(false, 1), pair(0x0D, ' '), pair(' ', ' '));
  rdsFeed(&rds, &b);
  TEST_ASSERT_FALSE(rds.info.hasRt);

  RdsRead again =
      group(0x1234, blockB2A(false, 0), pair('A', 'B'), pair('C', 'D'));
  rdsFeed(&rds, &again);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("ABCD", rds.info.rt);
}

/* Send segments 0 to `segments`-1 of a 2A text, once. */
static void sendRtPass(const char *text, int segments) {
  for (int seg = 0; seg < segments; seg++) {
    RdsRead r = group(0x1234, blockB2A(false, (uint8_t)seg),
                      pair(text[seg * 4], text[seg * 4 + 1]),
                      pair(text[seg * 4 + 2], text[seg * 4 + 3]));
    rdsFeed(&rds, &r);
  }
}

/*
 * A station that sends a short text and never a terminator, which is what
 * 94.3 does: four segments, "FEVER 94.3 FM   ", round and round.
 */
static void a_text_with_no_terminator_ends_at_its_highest_segment(void) {
  /* One pass says the station has sent segments 0 to 3 but not that it has
   * finished, so there is nothing to publish yet. */
  sendRtPass("FEVER 94.3 FM   ", 4);
  TEST_ASSERT_FALSE(rds.info.hasRt);
  /* Segment 0 coming round again says the pass finished at 3, so the text is
   * sixteen characters and all sixteen have arrived. */
  sendRtPass("FEVER 94.3 FM   ", 4);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("FEVER 94.3 FM", rds.info.rt);
}

/*
 * A steady bad signal must never shorten a text.
 *
 * Taken from the radio on 13 September 2026. On 93.5 at 20 dBuV, with block D
 * of segment 0 failing while block C survived, the length was worked out from
 * the run of characters received and the radio published "GA", which was the
 * first two characters of a song title. Two passes agreeing was not enough,
 * because on a steady bad signal the same blocks fail every pass and the two
 * passes agree on the wrong answer.
 */
static void a_steady_bad_signal_never_shortens_a_text(void) {
  const char *text = "REDFM VINANDI VINANDI ULLASANGA UTHSAAHANGAA\r";
  for (int pass = 0; pass < 8; pass++) {
    for (int seg = 0; seg < 12; seg++) {
      RdsRead r = group(0x0935, blockB2A(false, (uint8_t)seg),
                        pair(text[seg * 4], text[seg * 4 + 1]),
                        pair(text[seg * 4 + 2], text[seg * 4 + 3]));
      if (seg == 0) {
        r.error[3] = 1; /* Corrected, so thrown away. Positions 2 and 3 lost. */
      } else {
        r.error[2] = 1;
        r.error[3] = 1;
      }
      rdsFeed(&rds, &r);
      /* Nothing is ever published, because positions 2 and 3 never arrive.
       * Delayed is the right answer. Truncated is not. */
      TEST_ASSERT_FALSE(rds.info.hasRt);
    }
  }
}

/* And once those blocks come through, the whole text goes out. */
static void the_text_arrives_whole_once_the_damage_stops(void) {
  const char *text = "REDFM VINANDI VINANDI ULLASANGA UTHSAAHANGAA\r";
  for (int pass = 0; pass < 4; pass++) {
    for (int seg = 0; seg < 12; seg++) {
      RdsRead r = group(0x0935, blockB2A(false, (uint8_t)seg),
                        pair(text[seg * 4], text[seg * 4 + 1]),
                        pair(text[seg * 4 + 2], text[seg * 4 + 3]));
      if (pass < 2 && seg == 0) {
        r.error[3] = 1;
      }
      rdsFeed(&rds, &r);
    }
  }
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("REDFM VINANDI VINANDI ULLASANGA UTHSAAHANGAA",
                           rds.info.rt);
}

static void a_pass_cut_short_does_not_shorten_the_text(void) {
  const char *text =
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  sendRtPass(text, 16);
  sendRtPass(text, 16);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING(text, rds.info.rt);

  /* A pass that loses its last segments to interference must not turn a 64
   * character text into a 48 character one. The highest segment the station
   * has been seen to send only ever grows within one text. */
  sendRtPass(text, 12);
  sendRtPass(text, 12);
  TEST_ASSERT_EQUAL_STRING(text, rds.info.rt);
}

/*
 * A station clearing its radio text says so with a terminator in the first
 * position, which happens between songs.
 */
static void a_text_cleared_by_the_station_is_published_as_nothing(void) {
  RdsRead first =
      group(0x1234, blockB2A(false, 0), pair('H', 'i'), pair('!', 0x0D));
  rdsFeed(&rds, &first);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("Hi!", rds.info.rt);

  RdsRead cleared =
      group(0x1234, blockB2A(false, 0), pair(0x0D, ' '), pair(' ', ' '));
  rdsFeed(&rds, &cleared);
  /* Not an empty string, which a reader cannot tell from a text that has not
   * arrived, and not the old one either. */
  TEST_ASSERT_FALSE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("", rds.info.rt);
}

static void a_text_that_is_all_padding_is_published_as_nothing(void) {
  RdsRead a = group(0x1234, blockB2A(false, 0), pair(' ', ' '), pair(' ', ' '));
  RdsRead b =
      group(0x1234, blockB2A(false, 1), pair(' ', ' '), pair(' ', 0x0D));
  rdsFeed(&rds, &a);
  rdsFeed(&rds, &b);
  TEST_ASSERT_FALSE(rds.info.hasRt);
}

static void a_pass_joined_halfway_publishes_nothing(void) {
  /* Tuning in mid pass means the run from the start is empty, and an empty
   * text is not a text. */
  for (int round = 0; round < 3; round++) {
    for (int seg = 8; seg < 16; seg++) {
      RdsRead r = group(0x1234, blockB2A(false, (uint8_t)seg), pair('a', 'b'),
                        pair('c', 'd'));
      rdsFeed(&rds, &r);
    }
  }
  TEST_ASSERT_FALSE(rds.info.hasRt);
}

/* ------------------------------------------- the alternative frequencies -- */

static void two_alternatives_arrive_in_one_group(void) {
  /* Code 1 is 87.6 MHz and code 204 is 107.9 MHz, the two ends of the scale. */
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0),
                    (uint16_t)(1 << 8 | 204), 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT8(2, rds.info.afCount);
  TEST_ASSERT_EQUAL_UINT32(87600, rds.info.afKHz[0]);
  TEST_ASSERT_EQUAL_UINT32(107900, rds.info.afKHz[1]);
}

static void the_same_alternative_is_only_listed_once(void) {
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0),
                    (uint16_t)(30 << 8 | 30), 0);
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.afCount);
  TEST_ASSERT_EQUAL_UINT32(90500, rds.info.afKHz[0]);
}

static void the_station_own_frequency_is_not_an_alternative(void) {
  /* 101.9 is code 144, and it is what this decoder was reset on. */
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0),
                    (uint16_t)(144 << 8 | 30), 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.afCount);
  TEST_ASSERT_EQUAL_UINT32(90500, rds.info.afKHz[0]);
}

static void the_codes_that_are_not_a_frequency_are_left_out(void) {
  /* 224 says how many follow, 0 is unused, and 205 is reserved. */
  RdsRead a = group(0x1234, blockB0A(10, false, false, true, 0),
                    (uint16_t)(224 << 8 | 0), 0);
  RdsRead b = group(0x1234, blockB0A(10, false, false, true, 0),
                    (uint16_t)(205 << 8 | 255), 0);
  rdsFeed(&rds, &a);
  rdsFeed(&rds, &b);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.afCount);
}

static void a_long_or_medium_wave_pair_is_read_past(void) {
  /* Code 250 says the other byte names a frequency below the FM band. Taken
   * as an FM code, 100 would put 97.5 MHz in the list. */
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0),
                    (uint16_t)(250 << 8 | 100), 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.afCount);
}

static void a_zero_b_group_does_not_list_its_identifier_as_a_frequency(void) {
  /* Block C of a 0B group repeats the identifier. Read as two frequency
   * codes, 0x1234 would add 89.3 and 92.7. */
  RdsRead r = group(0x1234, blockB0B(10, 0), 0x1234, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.afCount);
}

static void a_damaged_block_c_lists_no_frequency(void) {
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0),
                    (uint16_t)(30 << 8 | 31), 0);
  r.error[2] = 3;
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.afCount);
}

static void the_list_stops_at_its_limit(void) {
  /* 26 different codes offered, which is one more than the standard allows a
   * station to name, so the last is dropped rather than written past the end. */
  for (uint8_t i = 0; i < 26; i++) {
    RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0),
                      (uint16_t)((1 + i) << 8 | (1 + i)), 0);
    rdsFeed(&rds, &r);
  }
  TEST_ASSERT_EQUAL_UINT8(RDS_AF_MAX, rds.info.afCount);
  TEST_ASSERT_EQUAL_UINT32(87600, rds.info.afKHz[0]);
  TEST_ASSERT_EQUAL_UINT32(90000, rds.info.afKHz[RDS_AF_MAX - 1]);
}

/* ------------------------------------------------------------- the clock -- */

/* Build a 4A group. The hour and minute in one are UTC, as the standard says. */
static RdsRead clockGroup(uint32_t mjd, uint8_t hour, uint8_t minute,
                          int8_t halfHours) {
  uint16_t b = (uint16_t)((4u << 12) | (uint16_t)((mjd >> 15) & 0x03u));
  uint16_t c = (uint16_t)(((mjd & 0x7FFFu) << 1) | ((hour >> 4) & 0x01u));
  uint8_t sign = halfHours < 0 ? 1 : 0;
  uint8_t magnitude = (uint8_t)(halfHours < 0 ? -halfHours : halfHours);
  uint16_t d = (uint16_t)(((uint16_t)(hour & 0x0F) << 12) |
                          ((uint16_t)(minute & 0x3F) << 6) |
                          ((uint16_t)sign << 5) | (magnitude & 0x1Fu));
  return group(0x1234, b, c, d);
}

static void a_date_and_time_decodes(void) {
  /* Modified Julian day 60000 is 25 February 2023. India is 5.5 hours ahead
   * of UTC, which is 11 half hours, so 16:15 UTC is 21:45 here. */
  RdsRead r = clockGroup(60000, 16, 15, 11);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.clock.valid);
  TEST_ASSERT_EQUAL_UINT16(2023, rds.info.clock.year);
  TEST_ASSERT_EQUAL_UINT8(2, rds.info.clock.month);
  TEST_ASSERT_EQUAL_UINT8(25, rds.info.clock.day);
  TEST_ASSERT_EQUAL_UINT8(21, rds.info.clock.hour);
  TEST_ASSERT_EQUAL_UINT8(45, rds.info.clock.minute);
  TEST_ASSERT_EQUAL_INT8(11, rds.info.clock.offsetHalfHours);
}

/*
 * The offset carries the time past midnight, so the date goes with it.
 *
 * 21:45 UTC on 25 February is 03:15 on the 26th in India. A clock that
 * applied the offset to the time and not to the date would show the right
 * hour on the wrong day, and only for five and a half hours out of
 * twenty four, which is the kind of wrong nobody notices.
 */
static void the_offset_can_move_the_date_forward(void) {
  RdsRead r = clockGroup(60000, 21, 45, 11);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.clock.valid);
  TEST_ASSERT_EQUAL_UINT16(2023, rds.info.clock.year);
  TEST_ASSERT_EQUAL_UINT8(2, rds.info.clock.month);
  TEST_ASSERT_EQUAL_UINT8(26, rds.info.clock.day);
  TEST_ASSERT_EQUAL_UINT8(3, rds.info.clock.hour);
  TEST_ASSERT_EQUAL_UINT8(15, rds.info.clock.minute);
}

static void the_offset_can_move_the_date_back(void) {
  /* 01:00 UTC on 1 March, five hours behind, is 20:00 on 28 February. */
  RdsRead r = clockGroup(60004, 1, 0, -10);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.clock.valid);
  TEST_ASSERT_EQUAL_UINT8(2, rds.info.clock.month);
  TEST_ASSERT_EQUAL_UINT8(28, rds.info.clock.day);
  TEST_ASSERT_EQUAL_UINT8(20, rds.info.clock.hour);
}

/* The offset is five bits of half hours, so the day never moves twice. */
static void the_largest_offsets_move_the_day_only_once(void) {
  RdsRead ahead = clockGroup(60000, 23, 59, 31);
  rdsFeed(&rds, &ahead);
  TEST_ASSERT_TRUE(rds.info.clock.valid);
  TEST_ASSERT_EQUAL_UINT8(26, rds.info.clock.day);
  TEST_ASSERT_EQUAL_UINT8(15, rds.info.clock.hour);
  TEST_ASSERT_EQUAL_UINT8(29, rds.info.clock.minute);

  rdsReset(&rds, TUNED_KHZ);
  RdsRead behind = clockGroup(60000, 0, 0, -31);
  rdsFeed(&rds, &behind);
  TEST_ASSERT_TRUE(rds.info.clock.valid);
  TEST_ASSERT_EQUAL_UINT8(24, rds.info.clock.day);
  TEST_ASSERT_EQUAL_UINT8(8, rds.info.clock.hour);
  TEST_ASSERT_EQUAL_UINT8(30, rds.info.clock.minute);
}

/* Stepping back off the first day the formula covers has no answer. */
static void an_offset_that_steps_before_the_formula_is_refused(void) {
  RdsRead r = clockGroup(15079, 0, 0, -2);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.clock.valid);
}

static void the_hour_bit_that_lives_in_block_c_is_read(void) {
  /* Any hour of 16 or more has its top bit in the other block. */
  RdsRead r = clockGroup(60000, 23, 59, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.clock.valid);
  TEST_ASSERT_EQUAL_UINT8(23, rds.info.clock.hour);
  TEST_ASSERT_EQUAL_UINT8(59, rds.info.clock.minute);
}

static void a_negative_offset_decodes(void) {
  /* 08:00 UTC, five hours behind, is 03:00 the same day. */
  RdsRead r = clockGroup(60000, 8, 0, -10);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.clock.valid);
  TEST_ASSERT_EQUAL_INT8(-10, rds.info.clock.offsetHalfHours);
  TEST_ASSERT_EQUAL_UINT8(3, rds.info.clock.hour);
  TEST_ASSERT_EQUAL_UINT8(25, rds.info.clock.day);
}

static void an_hour_that_cannot_exist_is_refused(void) {
  RdsRead r = clockGroup(60000, 24, 0, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.clock.valid);
}

static void a_minute_that_cannot_exist_is_refused(void) {
  RdsRead r = clockGroup(60000, 12, 60, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.clock.valid);
}

static void a_day_before_the_formula_starts_is_refused(void) {
  RdsRead zero = clockGroup(0, 12, 0, 0);
  rdsFeed(&rds, &zero);
  TEST_ASSERT_FALSE(rds.info.clock.valid);

  RdsRead early = clockGroup(15078, 12, 0, 0);
  rdsFeed(&rds, &early);
  TEST_ASSERT_FALSE(rds.info.clock.valid);
}

static void the_first_day_the_formula_covers_decodes(void) {
  /* Modified Julian day 15079 is 1 March 1900. */
  RdsRead r = clockGroup(15079, 0, 0, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.clock.valid);
  TEST_ASSERT_EQUAL_UINT16(1900, rds.info.clock.year);
  TEST_ASSERT_EQUAL_UINT8(3, rds.info.clock.month);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.clock.day);
}

static void a_year_past_the_range_is_refused(void) {
  /* Modified Julian day 88070 is in the year 2100. */
  RdsRead r = clockGroup(88070, 12, 0, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.clock.valid);
}

static void a_damaged_clock_block_is_refused(void) {
  RdsRead r = clockGroup(60000, 21, 45, 11);
  r.error[3] = 3;
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.clock.valid);

  RdsRead c = clockGroup(60000, 21, 45, 11);
  c.error[2] = 3;
  rdsFeed(&rds, &c);
  TEST_ASSERT_FALSE(rds.info.clock.valid);
}

static void a_four_b_group_is_not_a_clock(void) {
  RdsRead r = clockGroup(60000, 21, 45, 11);
  r.block[1] |= 0x0800u;
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.clock.valid);
}

/* ------------------------------------------------------------ the counters -- */

static void a_group_with_no_identifier_in_it_is_not_counted_as_used(void) {
  /* Fever FM sends 0000 in every group. A group whose only clean block is
   * block A, carrying no identifier, gave this decoder nothing. */
  RdsRead r = group(0x0000, blockB0A(10, false, false, true, 0), 0, 0);
  r.error[1] = 3;
  r.error[2] = 3;
  r.error[3] = 3;
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT32(1, rds.info.groupsSeen);
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.groupsUsed);
}

static void the_counters_follow_what_arrived(void) {
  RdsRead good = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &good);
  rdsFeed(&rds, &good);
  TEST_ASSERT_EQUAL_UINT32(2, rds.info.groupsSeen);
  TEST_ASSERT_EQUAL_UINT32(2, rds.info.groupsUsed);
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.blocksBad);

  RdsRead ruined = good;
  ruined.error[0] = 3;
  ruined.error[1] = 3;
  ruined.error[2] = 1;
  rdsFeed(&rds, &ruined);
  TEST_ASSERT_EQUAL_UINT32(3, rds.info.groupsSeen);
  TEST_ASSERT_EQUAL_UINT32(2, rds.info.groupsUsed);
  TEST_ASSERT_EQUAL_UINT32(2, rds.info.blocksBad);
  TEST_ASSERT_EQUAL_UINT32(1, rds.info.blocksCorrected);
}

static void a_retune_forgets_everything(void) {
  sendPs("TESTFM  ", 10, true, false, true);
  TEST_ASSERT_TRUE(rds.info.hasPs);

  rdsReset(&rds, 98300);
  TEST_ASSERT_FALSE(rds.info.hasPs);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  TEST_ASSERT_FALSE(rds.info.synchronised);
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.groupsSeen);

  /* 98.3 is code 108, so the new station's own frequency is the one dropped. */
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0),
                    (uint16_t)(108 << 8 | 144), 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.afCount);
  TEST_ASSERT_EQUAL_UINT32(101900, rds.info.afKHz[0]);
}

/* ------------------------------------------------- the real broadcasts -- */

/*
 * Everything below replays groups taken off air in Hyderabad on 13 September
 * 2026. See test/fixtures/rds/README.md for what each station sends and what
 * the captures showed.
 */

static void replay(const Capture *cap, uint16_t upTo) {
  rdsReset(&rds, cap->khz);
  for (uint16_t i = 0; i < upTo && i < cap->count; i++) {
    RdsRead r;
    memset(&r, 0, sizeof(r));
    r.synchronised = true;
    r.haveGroup = true;
    for (int b = 0; b < 4; b++) {
      r.block[b] = cap->groups[i].block[b];
      r.error[b] = (uint8_t)((cap->groups[i].error >> (6 - b * 2)) & 0x03);
    }
    rdsFeed(&rds, &r);
  }
}

/* By file name, because two stations were captured twice. */
static const Capture *capture(const char *name) {
  for (int i = 0; i < CAPTURE_COUNT; i++) {
    if (strcmp(kCaptures[i].name, name) == 0) {
      return &kCaptures[i];
    }
  }
  TEST_FAIL_MESSAGE("no capture by that name");
  return NULL;
}

#define STRONG_91100 "fm-91100-2026-09-13.log"
#define STRONG_93500 "fm-93500-2026-09-13.log"
#define STRONG_94300 "fm-94300-2026-09-13.log"
#define STRONG_95000 "fm-95000-2026-09-13.log"
#define STRONG_98300 "fm-98300-2026-09-13.log"
#define STRONG_106400 "fm-106400-2026-09-13.log"
#define WEAK_91100 "fm-91100-weak-2026-09-13.log"
#define WEAK_106400 "fm-106400-weak-2026-09-13.log"

/* True for a capture taken with the aerial collapsed. */
static bool isWeak(const Capture *cap) {
  return strstr(cap->name, "weak") != NULL;
}

/*
 * The name shown is always one the station actually sent.
 *
 * This is the property the eight character pass rule exists for, checked
 * against real broadcasts rather than against made up ones. 91.1 rotates five
 * different names through the field and 95.0 rotates two, so on those two a
 * decoder that confirms each position on its own puts a name on the panel
 * that is half of one and half of another.
 */
static void every_name_shown_is_one_the_station_sent(void) {
  for (int c = 0; c < CAPTURE_COUNT; c++) {
    const Capture *cap = &kCaptures[c];

    /* Every complete eight character pass in this capture. */
    char sent[64][RDS_PS_LEN + 1];
    int sentCount = 0;
    char frame[RDS_PS_LEN];
    bool have[RDS_PS_LEN];
    memset(have, 0, sizeof(have));

    rdsReset(&rds, cap->khz);
    for (uint16_t i = 0; i < cap->count; i++) {
      uint16_t b = cap->groups[i].block[1];
      uint16_t d = cap->groups[i].block[3];
      if ((b >> 12) == 0) {
        int seg = b & 0x0003;
        frame[seg * 2] = (char)(d >> 8);
        frame[seg * 2 + 1] = (char)(d & 0xFF);
        have[seg * 2] = true;
        have[seg * 2 + 1] = true;
        bool full = true;
        for (int k = 0; k < RDS_PS_LEN; k++) {
          full = full && have[k];
        }
        if (full) {
          bool known = false;
          for (int k = 0; k < sentCount; k++) {
            known = known || memcmp(sent[k], frame, RDS_PS_LEN) == 0;
          }
          if (!known && sentCount < 64) {
            memcpy(sent[sentCount], frame, RDS_PS_LEN);
            sent[sentCount][RDS_PS_LEN] = '\0';
            sentCount++;
          }
          memset(have, 0, sizeof(have));
        }
      }

      RdsRead r;
      memset(&r, 0, sizeof(r));
      r.synchronised = true;
      r.haveGroup = true;
      for (int k = 0; k < 4; k++) {
        r.block[k] = cap->groups[i].block[k];
        r.error[k] = (uint8_t)((cap->groups[i].error >> (6 - k * 2)) & 0x03);
      }
      rdsFeed(&rds, &r);

      if (rds.info.hasPs) {
        bool matched = false;
        for (int k = 0; k < sentCount; k++) {
          matched = matched || strcmp(sent[k], rds.info.ps) == 0;
        }
        TEST_ASSERT_TRUE_MESSAGE(matched, cap->name);
      }
    }
  }
}

/*
 * Not one block in any of the six captures came back uncorrected.
 *
 * Every station that carries RDS here is strong, so the captures say nothing
 * about a damaged stream. That path is covered by the tests above this
 * section, which set the error bits by hand. This test is here so that a
 * capture taken in worse conditions is noticed rather than quietly changing
 * what the other tests mean.
 */
static void the_strong_captures_have_nothing_damaged_in_them(void) {
  for (int c = 0; c < CAPTURE_COUNT; c++) {
    if (isWeak(&kCaptures[c])) {
      continue;
    }
    replay(&kCaptures[c], kCaptures[c].count);
    TEST_ASSERT_TRUE(rds.info.synchronised);
    TEST_ASSERT_EQUAL_UINT32(kCaptures[c].count, rds.info.groupsSeen);
    TEST_ASSERT_EQUAL_UINT32(kCaptures[c].count, rds.info.groupsUsed);
    TEST_ASSERT_EQUAL_UINT32(0, rds.info.blocksCorrected);
    TEST_ASSERT_EQUAL_UINT32(0, rds.info.blocksBad);
  }
}

/*
 * The two captures taken with the aerial collapsed do have damage in them,
 * and the decoder still gets the right answer out of them.
 *
 * 91.1 and 106.4 read about 10 dBuV in these instead of the usual 45. This
 * is the only real damaged data there is: nothing receivable from here is
 * weak enough on its own, and detuning does not work because the tuner's
 * decoder either locks cleanly or does not lock.
 */
static void the_weak_captures_are_damaged(void) {
  bool any = false;
  for (int c = 0; c < CAPTURE_COUNT; c++) {
    if (!isWeak(&kCaptures[c])) {
      continue;
    }
    any = true;
    replay(&kCaptures[c], kCaptures[c].count);
    TEST_ASSERT_TRUE(rds.info.synchronised);
    TEST_ASSERT_TRUE_MESSAGE(rds.info.blocksCorrected > 0, kCaptures[c].name);
    TEST_ASSERT_TRUE_MESSAGE(rds.info.blocksBad > 0, kCaptures[c].name);
    /* Block A survived every single group of both captures while B, C and D
     * did not. The chip protects the identifier hardest, which is why the
     * identifier is the one thing that never came out wrong here. */
    TEST_ASSERT_EQUAL_UINT32(rds.info.groupsSeen, rds.info.groupsUsed);
  }
  TEST_ASSERT_TRUE(any);
}

static void radio_city_911_decodes_through_the_damage(void) {
  const Capture *cap = capture(WEAK_91100);
  replay(cap, cap->count);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x3712, rds.info.pi);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("RADIO CITY 91.1", rds.info.rt);
}

static void magic_fm_1064_decodes_through_the_damage(void) {
  const Capture *cap = capture(WEAK_106400);
  replay(cap, cap->count);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x1064, rds.info.pi);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING(" MAGIC  ", rds.info.ps);
}

/*
 * Not one character of a published text is ever a corrupted one.
 *
 * This is the test the clean block rule exists for, and it is what caught the
 * rule being too loose. Accepting blocks the tuner said it had corrected,
 * which is what the reference firmware does, put "MAwQCFM VINTOO..." and a
 * text whose terminator had been corrupted so it ran on into padding onto the
 * panel from these two captures. Every text they publish now is one of the
 * handful the two stations actually sent.
 */
static void a_damaged_stream_never_publishes_a_corrupt_text(void) {
  static const char *kSent[] = {
      "RADIO CITY 91.1",
      "NAATHORA THAMASHALALO - NINNE PELLADATHA - SANJEEV WADHWANI + SU",
      "MAGICFM VINTOO MAIMARACHIPODAAM!",
      "YE CHIKITHA - BADRI - RAMA GOGULA + SUNITHA - MAGICFM",
  };
  const int sentCount = (int)(sizeof(kSent) / sizeof(kSent[0]));

  for (int c = 0; c < CAPTURE_COUNT; c++) {
    const Capture *cap = &kCaptures[c];
    if (!isWeak(cap)) {
      continue;
    }
    rdsReset(&rds, cap->khz);
    for (uint16_t i = 0; i < cap->count; i++) {
      RdsRead r;
      memset(&r, 0, sizeof(r));
      r.synchronised = true;
      r.haveGroup = true;
      for (int b = 0; b < 4; b++) {
        r.block[b] = cap->groups[i].block[b];
        r.error[b] = (uint8_t)((cap->groups[i].error >> (6 - b * 2)) & 0x03);
      }
      rdsFeed(&rds, &r);
      if (!rds.info.hasRt) {
        continue;
      }
      bool known = false;
      for (int k = 0; k < sentCount; k++) {
        known = known || strcmp(kSent[k], rds.info.rt) == 0;
      }
      TEST_ASSERT_TRUE_MESSAGE(known, rds.info.rt);
    }
  }
}

/*
 * No station here sends a list of alternative frequencies.
 *
 * Four of the six send group 0A, but every code in it is 224 or 205, which
 * mean "none follow" and "filler". So the alternative frequency decoding is
 * proved only by the made up groups above, and this test records that rather
 * than leaving the empty list looking like a decoder that failed.
 */
static void no_station_here_lists_an_alternative(void) {
  for (int c = 0; c < CAPTURE_COUNT; c++) {
    replay(&kCaptures[c], kCaptures[c].count);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, rds.info.afCount, kCaptures[c].name);
  }
}

/* No station here sends the time either, so nothing claims to know it. */
static void no_station_here_sends_the_time(void) {
  for (int c = 0; c < CAPTURE_COUNT; c++) {
    replay(&kCaptures[c], kCaptures[c].count);
    TEST_ASSERT_FALSE_MESSAGE(rds.info.clock.valid, kCaptures[c].name);
  }
}

static void red_fm_935_decodes(void) {
  replay(capture(STRONG_93500), 160);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x0935, rds.info.pi);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING("  RED   ", rds.info.ps);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("REDFM VINANDI VINANDI ULLASANGA UTHSAAHANGAA",
                           rds.info.rt);
  TEST_ASSERT_TRUE(rds.info.hasPty);
  TEST_ASSERT_EQUAL_UINT8(12, rds.info.pty);
}

static void magic_fm_1064_decodes(void) {
  replay(capture(STRONG_106400), 160);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x1064, rds.info.pi);
  TEST_ASSERT_EQUAL_STRING(" MAGIC  ", rds.info.ps);
  /* A song title, and the longest text of the six. It fills all 64
   * characters and is cut off there, which is the standard's limit and not
   * this decoder's. */
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING(
      "KINNERASANI - SITARA - S.P. BALASUBRAHMANYAM + S.P. SAILAJA - MA",
      rds.info.rt);
}

static void mirchi_983_decodes(void) {
  replay(capture(STRONG_98300), 160);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x26FF, rds.info.pi);
  TEST_ASSERT_EQUAL_STRING("MIRCHI  ", rds.info.ps);
  TEST_ASSERT_EQUAL_STRING("MIRCHI  98.3MHz", rds.info.rt);
}

/*
 * Fever FM sends no identifier and no terminator on its text.
 *
 * Its block A is 0000 on every group, which the standard keeps for a station
 * that has not been given an identifier, so the radio must not report one.
 * Its text is four segments repeated for ever, which is what the pass rule
 * is for.
 */
static void fever_fm_943_decodes(void) {
  replay(capture(STRONG_94300), 160);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING("FEVER FM", rds.info.ps);
  TEST_ASSERT_TRUE(rds.info.hasRt);
  TEST_ASSERT_EQUAL_STRING("FEVER 94.3 FM", rds.info.rt);
}

/* Mirchi 95 also sends no identifier, and scrolls its name in two passes. */
static void mirchi_950_decodes(void) {
  replay(capture(STRONG_95000), 160);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_TRUE(strcmp(rds.info.ps, "MIRCHI 9") == 0 ||
                   strcmp(rds.info.ps, "5       ") == 0);
  TEST_ASSERT_EQUAL_STRING("MIRCHI 95.0MHZ", rds.info.rt);
}

/* Radio City rotates five different names through the eight characters. */
static void radio_city_911_decodes(void) {
  replay(capture(STRONG_91100), 160);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x3712, rds.info.pi);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING("RADIO CITY 91.1", rds.info.rt);
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(a_fresh_decoder_claims_nothing);
  RUN_TEST(nulls_do_not_crash);

  RUN_TEST(a_read_with_a_lock_and_no_group_is_still_a_lock);
  RUN_TEST(the_lock_survives_one_read_without_it);
  RUN_TEST(one_locked_read_brings_the_lock_straight_back);

  RUN_TEST(the_identifier_waits_for_a_second_reception);
  RUN_TEST(two_different_identifiers_settle_neither);
  RUN_TEST(a_block_the_tuner_could_not_correct_is_not_an_identifier);
  RUN_TEST(a_block_the_tuner_corrected_is_not_taken);
  RUN_TEST(an_identifier_of_zero_is_no_identifier);
  RUN_TEST(an_identifier_of_zero_does_not_disturb_a_real_one);
  RUN_TEST(the_group_that_changes_the_station_counts_as_its_first);
  RUN_TEST(a_new_identifier_throws_the_old_station_away);

  RUN_TEST(the_programme_type_waits_for_a_second_reception);
  RUN_TEST(the_programme_type_comes_from_every_group);
  RUN_TEST(every_programme_type_has_a_name);

  RUN_TEST(the_flags_arrive_with_the_first_group_zero);
  RUN_TEST(the_music_bit_reads_as_music);
  RUN_TEST(a_group_that_is_not_group_zero_leaves_the_flags_alone);

  RUN_TEST(the_name_waits_for_two_passes_that_agree);
  RUN_TEST(the_segments_of_a_pass_may_arrive_in_any_order);
  RUN_TEST(a_name_from_a_zero_b_group_decodes_the_same);
  RUN_TEST(one_position_that_keeps_changing_holds_the_name_back);
  RUN_TEST(a_damaged_last_block_holds_the_whole_pass_open);
  RUN_TEST(a_scrolling_name_never_shows_a_mixture);
  RUN_TEST(two_names_alternating_every_pass_show_neither);
  RUN_TEST(a_damaged_block_b_decodes_nothing_but_the_identifier);
  RUN_TEST(a_character_outside_the_shown_range_becomes_a_question_mark);

  RUN_TEST(a_full_sixty_four_character_text_is_published);
  RUN_TEST(a_text_ends_at_its_terminator);
  RUN_TEST(a_text_with_a_hole_in_it_is_not_published);
  RUN_TEST(trailing_padding_is_not_part_of_the_text);
  RUN_TEST(the_short_text_of_a_two_b_group_decodes);
  RUN_TEST(the_flag_toggling_starts_a_new_text);
  RUN_TEST(the_flag_toggling_drops_a_text_that_was_being_built);
  RUN_TEST(a_damaged_text_block_leaves_its_characters_out);
  RUN_TEST(a_text_with_no_terminator_ends_at_its_highest_segment);
  RUN_TEST(a_steady_bad_signal_never_shortens_a_text);
  RUN_TEST(the_text_arrives_whole_once_the_damage_stops);
  RUN_TEST(a_pass_cut_short_does_not_shorten_the_text);
  RUN_TEST(a_text_cleared_by_the_station_is_published_as_nothing);
  RUN_TEST(a_text_that_is_all_padding_is_published_as_nothing);
  RUN_TEST(a_pass_joined_halfway_publishes_nothing);

  RUN_TEST(two_alternatives_arrive_in_one_group);
  RUN_TEST(the_same_alternative_is_only_listed_once);
  RUN_TEST(the_station_own_frequency_is_not_an_alternative);
  RUN_TEST(the_codes_that_are_not_a_frequency_are_left_out);
  RUN_TEST(a_long_or_medium_wave_pair_is_read_past);
  RUN_TEST(a_zero_b_group_does_not_list_its_identifier_as_a_frequency);
  RUN_TEST(a_damaged_block_c_lists_no_frequency);
  RUN_TEST(the_list_stops_at_its_limit);

  RUN_TEST(a_date_and_time_decodes);
  RUN_TEST(the_offset_can_move_the_date_forward);
  RUN_TEST(the_offset_can_move_the_date_back);
  RUN_TEST(the_largest_offsets_move_the_day_only_once);
  RUN_TEST(an_offset_that_steps_before_the_formula_is_refused);
  RUN_TEST(the_hour_bit_that_lives_in_block_c_is_read);
  RUN_TEST(a_negative_offset_decodes);
  RUN_TEST(an_hour_that_cannot_exist_is_refused);
  RUN_TEST(a_minute_that_cannot_exist_is_refused);
  RUN_TEST(a_day_before_the_formula_starts_is_refused);
  RUN_TEST(the_first_day_the_formula_covers_decodes);
  RUN_TEST(a_year_past_the_range_is_refused);
  RUN_TEST(a_damaged_clock_block_is_refused);
  RUN_TEST(a_four_b_group_is_not_a_clock);

  RUN_TEST(a_group_with_no_identifier_in_it_is_not_counted_as_used);
  RUN_TEST(the_counters_follow_what_arrived);
  RUN_TEST(a_retune_forgets_everything);

  RUN_TEST(every_name_shown_is_one_the_station_sent);
  RUN_TEST(the_strong_captures_have_nothing_damaged_in_them);
  RUN_TEST(the_weak_captures_are_damaged);
  RUN_TEST(radio_city_911_decodes_through_the_damage);
  RUN_TEST(magic_fm_1064_decodes_through_the_damage);
  RUN_TEST(a_damaged_stream_never_publishes_a_corrupt_text);
  RUN_TEST(no_station_here_lists_an_alternative);
  RUN_TEST(no_station_here_sends_the_time);
  RUN_TEST(red_fm_935_decodes);
  RUN_TEST(magic_fm_1064_decodes);
  RUN_TEST(mirchi_983_decodes);
  RUN_TEST(fever_fm_943_decodes);
  RUN_TEST(mirchi_950_decodes);
  RUN_TEST(radio_city_911_decodes);

  return UNITY_END();
}
