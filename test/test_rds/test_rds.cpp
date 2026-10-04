/* Tests for the RDS decoder. Runs on a PC. */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/rds.h"

#include "../test_dx/captures.h"
#include "captures.h"

/* A station on 101.9. */
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
 * Measured, not chosen. On both weak recordings, corrected blocks put
 * corrupt radio text on the panel, even at the smallest error the chip
 * reports.
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
 * the eight characters. 95.0 and 91.1 both do this, and the pattern is
 * theirs: three passes of one name, then three of the other.
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

/* Send one name for `holds` complete passes. */
static void sendName(const char *name, int holds) {
  for (int pass = 0; pass < holds; pass++) {
    for (uint8_t seg = 0; seg < 4; seg++) {
      RdsRead r = group(0x1234, blockB0A(10, false, false, true, seg), 0,
                        pair(name[seg * 2], name[seg * 2 + 1]));
      rdsFeed(&rds, &r);
    }
  }
}

/*
 * A name cut at the eighth character is stitched back together, once the
 * same cut has been seen twice.
 */
static void a_name_split_across_two_passes_is_stitched(void) {
  sendName("MIRCHI 9", 3);
  sendName("5       ", 3);
  /* One change is not enough. A single corrupt pass can make any two names
   * look like a split, and on a weak signal it does. */
  TEST_ASSERT_FALSE(rds.info.hasPsLong);

  sendName("MIRCHI 9", 3);
  sendName("5       ", 3);
  TEST_ASSERT_TRUE(rds.info.hasPsLong);
  TEST_ASSERT_EQUAL_STRING("MIRCHI 95", rds.info.psLong);
}

/* The pass itself is still there, exactly as the station sent it. */
static void stitching_leaves_the_pass_alone(void) {
  sendName("MIRCHI 9", 3);
  sendName("5       ", 3);
  sendName("MIRCHI 9", 3);
  sendName("5       ", 3);
  TEST_ASSERT_EQUAL_STRING("MIRCHI 95", rds.info.psLong);
  TEST_ASSERT_EQUAL_STRING("5       ", rds.info.ps);
}

/*
 * A padded name is a whole name and is never stitched to anything.
 *
 * This is what keeps Radio City 91.1 alone. It rotates nine names. Every one
 * of them but `Madhi Lo` is centred or trailing padded, and `Madhi Lo` is
 * followed by a centred name, so no pair of them qualifies.
 */
static void padded_names_are_never_stitched(void) {
  const char *names[4] = {"City FM ", "  ante  ", " Radio  ", "  City  "};
  for (int round = 0; round < 3; round++) {
    for (int i = 0; i < 4; i++) {
      sendName(names[i], 3);
    }
  }
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_FALSE(rds.info.hasPsLong);
}

/*
 * A name that runs to the eighth character but is followed by a padded one is
 * not a split either. Radio City sends `Madhi Lo` and then `  Moge  `.
 */
static void a_full_name_followed_by_a_padded_one_is_not_a_split(void) {
  for (int round = 0; round < 3; round++) {
    sendName("Madhi Lo", 3);
    sendName("  Moge  ", 3);
  }
  TEST_ASSERT_FALSE(rds.info.hasPsLong);
}

/* Three passes stitch, because a name can be longer than sixteen. */
static void a_name_split_across_three_passes_is_stitched(void) {
  for (int round = 0; round < 3; round++) {
    sendName("LONGNAME", 3);
    sendName("CONTINUE", 3);
    sendName("D       ", 3);
  }
  TEST_ASSERT_TRUE(rds.info.hasPsLong);
  TEST_ASSERT_EQUAL_STRING("LONGNAMECONTINUED", rds.info.psLong);
}

/*
 * A ticker of names that all fill the field is never stitched.
 *
 * Every pair of them qualifies, and each one carries on from the last, so a
 * chain of confirmed pairs builds a run. Publishing that run would hand the
 * panel `AAAAAAAABBBBBBBBCCCCCCCC`, and once the cap is reached it would start
 * again mid rotation and hand over `DDDDDDDDAAAAAAAA`. Neither is ever
 * transmitted as one thing, and the decoder must never show text that no
 * station sent.
 *
 * What refuses them is that a split name's last piece has room left in it,
 * because that is where the string ended. A ticker of full names has no last
 * piece, so no run of them is ever a whole name.
 */
static void a_ticker_of_full_names_is_never_stitched(void) {
  const char *names[4] = {"AAAAAAAA", "BBBBBBBB", "CCCCCCCC", "DDDDDDDD"};
  for (int round = 0; round < 4; round++) {
    for (int i = 0; i < 4; i++) {
      sendName(names[i], 3);
      TEST_ASSERT_FALSE(rds.info.hasPsLong);
    }
  }
}

/* Two real words run together is the same fabrication in a readable shape. */
static void two_full_words_are_not_run_together(void) {
  for (int round = 0; round < 4; round++) {
    sendName("NEWSDESK", 3);
    sendName("TOPHITS!", 3);
  }
  TEST_ASSERT_FALSE(rds.info.hasPsLong);
}

/*
 * A name split across more than the cap is left alone, not trimmed.
 *
 * Publishing the last four pieces of a five piece name is publishing part of
 * it, and part of a name is not the name. So the radio shows the eight
 * character pass instead.
 */
static void a_split_longer_than_the_cap_is_left_alone(void) {
  const char *names[5] = {"AAAAAAAA", "BBBBBBBB", "CCCCCCCC", "DDDDDDDD",
                          "E       "};
  for (int round = 0; round < 5; round++) {
    for (int i = 0; i < 5; i++) {
      sendName(names[i], 3);
      TEST_ASSERT_FALSE(rds.info.hasPsLong);
    }
  }
}

/*
 * The flag and the string always agree.
 *
 * A station that rotates a split name among unrelated ones comes back to the
 * first piece of the split while the run is still held. The flag must not go
 * true while the string has been emptied on the way past the unrelated name.
 * An empty name draws as no name, so the panel would lose the station name for
 * a whole pass with nothing to say why.
 */
static void a_stitched_name_is_never_offered_empty(void) {
  for (int round = 0; round < 4; round++) {
    sendName("MIRCHI 9", 3);
    TEST_ASSERT_TRUE(rds.info.hasPsLong == (rds.info.psLong[0] != '\0'));
    sendName("5       ", 3);
    TEST_ASSERT_TRUE(rds.info.hasPsLong == (rds.info.psLong[0] != '\0'));
    sendName("  Moge  ", 3);
    TEST_ASSERT_TRUE(rds.info.hasPsLong == (rds.info.psLong[0] != '\0'));
    TEST_ASSERT_FALSE(rds.info.hasPsLong);
  }
  /* And the split itself still works alongside the unrelated name. */
  sendName("MIRCHI 9", 3);
  TEST_ASSERT_TRUE(rds.info.hasPsLong);
  TEST_ASSERT_EQUAL_STRING("MIRCHI 95", rds.info.psLong);
}

/* Padding at the front of the first piece is dropped with the rest. */
static void a_stitched_name_is_not_indented(void) {
  for (int round = 0; round < 3; round++) {
    sendName(" HALFWAY", 3);
    sendName("HOUSE   ", 3);
  }
  TEST_ASSERT_TRUE(rds.info.hasPsLong);
  TEST_ASSERT_EQUAL_STRING("HALFWAYHOUSE", rds.info.psLong);
}

/* Retuning forgets everything learned about the last station. */
static void retuning_forgets_a_stitched_name(void) {
  sendName("MIRCHI 9", 3);
  sendName("5       ", 3);
  sendName("MIRCHI 9", 3);
  sendName("5       ", 3);
  TEST_ASSERT_TRUE(rds.info.hasPsLong);

  rdsReset(&rds, 91100);
  TEST_ASSERT_FALSE(rds.info.hasPsLong);
  TEST_ASSERT_EQUAL_STRING("", rds.info.psLong);
}

/* A station that sends one name has nothing to stitch and says so. */
static void one_name_is_never_stitched(void) {
  sendName(" MAGIC  ", 8);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_FALSE(rds.info.hasPsLong);
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

static void a_character_outside_the_shown_range_becomes_an_underscore(void) {
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
  /* An underscore, not a question mark and not a space. A space would say the
   * station sent a space, and a question mark is a character stations send all
   * the time in radio text, so it would be ambiguous every time it appears. */
  TEST_ASSERT_EQUAL_STRING("_ESTFM  ", rds.info.ps);
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
 * On 93.5 at 20 dBuV, block D of segment 0 fails while block C survives. A
 * length worked out from the run of characters received gives "GA", the first
 * two characters of a song title. Two passes agreeing is not enough, because on
 * a steady bad signal the same blocks fail every pass and the two passes agree
 * on the wrong answer.
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
  /* Modified Julian day 60000 is 25 February 2023. An offset of +5:30 is 11
   * half hours, so 16:15 UTC is 21:45 local time. */
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
 * 21:45 UTC on 25 February is 03:15 on the 26th at an offset of +5:30. A
 * clock that applied the offset to the time and not to the date would show
 * the right hour on the wrong day, and only for five and a half hours out of
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

/* ---------------------------------------------- what the DX page reads -- */

static void one_clean_hearing_is_heard_but_not_confirmed(void) {
  RdsRead r = group(0x53A1, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  TEST_ASSERT_TRUE(rds.info.hasPiHeard);
  TEST_ASSERT_EQUAL_HEX16(0x53A1, rds.info.piHeard);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.piUnsureNibbles);
  char text[5];
  TEST_ASSERT_TRUE(rdsFormatPiHeard(&rds.info, text));
  TEST_ASSERT_EQUAL_STRING("53A1", text);

  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x53A1, rds.info.pi);
}

static void the_digits_that_changed_are_the_unsure_ones(void) {
  RdsRead first = group(0x53A1, blockB0A(10, false, false, true, 0), 0, 0);
  RdsRead second = group(0x5CA2, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &first);
  rdsFeed(&rds, &second);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  char text[5];
  rdsFormatPiHeard(&rds.info, text);
  TEST_ASSERT_EQUAL_STRING("5?A?", text);

  /* Two alike again, and nothing is unsure. */
  rdsFeed(&rds, &second);
  rdsFormatPiHeard(&rds.info, text);
  TEST_ASSERT_EQUAL_STRING("5CA2", text);
  TEST_ASSERT_TRUE(rds.info.hasPi);
}

static void a_corrected_block_a_is_not_heard(void) {
  RdsRead r = group(0x53A1, blockB0A(10, false, false, true, 0), 0, 0);
  r.error[0] = 1;
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasPiHeard);
  char text[5] = "x";
  TEST_ASSERT_FALSE(rdsFormatPiHeard(&rds.info, text));
  TEST_ASSERT_EQUAL_STRING("", text);
}

static void nothing_to_format_without_somewhere_to_put_it(void) {
  TEST_ASSERT_FALSE(rdsFormatPiHeard(&rds.info, NULL));
  char text[5] = "x";
  TEST_ASSERT_FALSE(rdsFormatPiHeard(NULL, text));
  TEST_ASSERT_EQUAL_STRING("", text);
}

static void a_station_sending_zero_says_so_after_two_hearings(void) {
  RdsRead r = group(0x0000, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.piZero);
  TEST_ASSERT_TRUE(rds.info.hasPiHeard);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.piZero);
  TEST_ASSERT_FALSE(rds.info.hasPi);
}

static void a_zero_between_two_hearings_does_not_stop_them_agreeing(void) {
  RdsRead real = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  RdsRead none = group(0x0000, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &real);
  rdsFeed(&rds, &none);
  rdsFeed(&rds, &real);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_FALSE(rds.info.piZero);
}

static void a_real_identifier_ends_the_zero(void) {
  RdsRead none = group(0x0000, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &none);
  rdsFeed(&rds, &none);
  TEST_ASSERT_TRUE(rds.info.piZero);

  RdsRead real = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &real);
  TEST_ASSERT_TRUE(rds.info.piZero);
  rdsFeed(&rds, &real);
  TEST_ASSERT_FALSE(rds.info.piZero);
  TEST_ASSERT_TRUE(rds.info.hasPi);
}

static void the_block_errors_are_the_last_groups(void) {
  TEST_ASSERT_FALSE(rds.info.hasBlockErrors);
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  r.error[1] = 1;
  r.error[2] = 2;
  r.error[3] = 3;
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.hasBlockErrors);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.blockError[0]);
  TEST_ASSERT_EQUAL_UINT8(1, rds.info.blockError[1]);
  TEST_ASSERT_EQUAL_UINT8(2, rds.info.blockError[2]);
  TEST_ASSERT_EQUAL_UINT8(3, rds.info.blockError[3]);

  /* A read with no group in it leaves them as they were. */
  RdsRead locked = r;
  locked.haveGroup = false;
  rdsFeed(&rds, &locked);
  TEST_ASSERT_TRUE(rds.info.hasBlockErrors);
  TEST_ASSERT_EQUAL_UINT8(3, rds.info.blockError[3]);
}

static void the_block_errors_go_with_the_lock(void) {
  RdsRead r = group(0x1234, blockB0A(10, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &r);
  RdsRead lost;
  memset(&lost, 0, sizeof(lost));
  for (int i = 0; i < RDS_SYNC_LOSS_READS - 1; i++) {
    rdsFeed(&rds, &lost);
    TEST_ASSERT_TRUE(rds.info.hasBlockErrors);
  }
  rdsFeed(&rds, &lost);
  TEST_ASSERT_FALSE(rds.info.synchronised);
  TEST_ASSERT_FALSE(rds.info.hasBlockErrors);
}

static void the_name_is_heard_a_pair_at_a_time(void) {
  RdsRead r =
      group(0x1234, blockB0A(10, false, false, true, 1), 0, pair('D', 'X'));
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasPs);
  TEST_ASSERT_FALSE(rds.info.psHeardHave[0]);
  TEST_ASSERT_TRUE(rds.info.psHeardHave[2]);
  TEST_ASSERT_TRUE(rds.info.psHeardHave[3]);
  TEST_ASSERT_EQUAL_CHAR('D', rds.info.psHeard[2]);
  TEST_ASSERT_EQUAL_CHAR('X', rds.info.psHeard[3]);

  /* A damaged block D gives nothing to hear. */
  RdsRead damaged =
      group(0x1234, blockB0A(10, false, false, true, 2), 0, pair('Q', 'Q'));
  damaged.error[3] = 1;
  rdsFeed(&rds, &damaged);
  TEST_ASSERT_FALSE(rds.info.psHeardHave[4]);
}

static void the_heard_name_outlasts_a_pass(void) {
  sendPs("TESTFM  ", 10, true, false, true);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  for (int i = 0; i < RDS_PS_LEN; i++) {
    TEST_ASSERT_TRUE(rds.info.psHeardHave[i]);
  }
  TEST_ASSERT_EQUAL_MEMORY("TESTFM  ", rds.info.psHeard, RDS_PS_LEN);
}

static void a_new_station_forgets_the_heard_name(void) {
  sendPs("TESTFM  ", 10, true, false, true);
  RdsRead other = group(0x9999, blockB0A(1, false, false, true, 0), 0, 0);
  other.error[3] = 3;
  rdsFeed(&rds, &other);
  rdsFeed(&rds, &other);
  TEST_ASSERT_EQUAL_HEX16(0x9999, rds.info.pi);
  for (int i = 0; i < RDS_PS_LEN; i++) {
    TEST_ASSERT_FALSE(rds.info.psHeardHave[i]);
  }
}

static void a_retune_forgets_what_was_heard(void) {
  sendPs("TESTFM  ", 10, true, false, true);
  rdsReset(&rds, 98300);
  TEST_ASSERT_FALSE(rds.info.hasPiHeard);
  TEST_ASSERT_FALSE(rds.info.piZero);
  TEST_ASSERT_FALSE(rds.info.hasBlockErrors);
  TEST_ASSERT_FALSE(rds.info.psHeardHave[0]);
}

/* -------------------------------------------------------------- the ECC -- */

/* Block B of group 1A with a programme type, and block C carrying variant 0
 * with an ECC. */
static uint16_t blockB1A(uint8_t pty) {
  return (uint16_t)((1u << 12) | ((uint16_t)(pty & 0x1F) << 5));
}
static uint16_t eccBlock(uint8_t ecc) {
  return ecc;
}

static void the_ecc_arrives_on_its_second_hearing(void) {
  RdsRead r = group(0x5123, blockB1A(0), eccBlock(0xF2), 0);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasEcc);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.hasEcc);
  TEST_ASSERT_EQUAL_HEX8(0xF2, rds.info.ecc);
}

static void a_corrected_block_c_gives_no_ecc(void) {
  RdsRead r = group(0x5123, blockB1A(0), eccBlock(0xF2), 0);
  r.error[2] = 1;
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasEcc);
}

static void two_different_eccs_do_not_agree(void) {
  RdsRead a = group(0x5123, blockB1A(0), eccBlock(0xF2), 0);
  RdsRead b = group(0x5123, blockB1A(0), eccBlock(0xE0), 0);
  rdsFeed(&rds, &a);
  rdsFeed(&rds, &b);
  TEST_ASSERT_FALSE(rds.info.hasEcc);
}

/* Only variant 0 carries the ECC. Variant 3 is the language, in the same
 * bits. */
static void another_variant_is_not_an_ecc(void) {
  RdsRead r = group(0x5123, blockB1A(0), (uint16_t)(3u << 12 | 0x0F2u), 0);
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasEcc);
}

/* In 1B block C is the PI again. */
static void a_b_version_group_has_no_ecc(void) {
  RdsRead r = group(0x5123, (uint16_t)(1u << 12 | 0x0800u), 0x5123, 0);
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasEcc);
}

static void a_new_station_clears_the_ecc(void) {
  RdsRead r = group(0x5123, blockB1A(0), eccBlock(0xF2), 0);
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.hasEcc);
  RdsRead other = group(0x9999, blockB0A(1, false, false, true, 0), 0, 0);
  rdsFeed(&rds, &other);
  rdsFeed(&rds, &other);
  TEST_ASSERT_FALSE(rds.info.hasEcc);
}

/* ------------------------------------------------- the real broadcasts -- */

/*
 * Everything below replays real groups recorded from six FM stations.
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

/* By capture name, because two stations were captured twice. */
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

/* True for the weak captures. */
static bool isWeak(const Capture *cap) {
  return strstr(cap->name, "weak") != NULL;
}

/*
 * The name shown is always one the station actually sent.
 *
 * This is the property the eight character pass rule exists for, checked
 * against real broadcasts rather than against made up ones. 91.1 rotates nine
 * different names through the field and 95.0 rotates two, so on those two a
 * decoder that confirms each position on its own puts a name on the panel
 * that is half of one and half of another.
 */
static void every_name_shown_is_one_the_station_sent(void) {
  for (int c = 0; c < CAPTURE_COUNT; c++) {
    const Capture *cap = &kCaptures[c];

    /* Every complete eight character pass in these groups. */
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
 * Not one block in any of the six strong recordings came back uncorrected.
 *
 * Every recorded station that carries RDS is strong, so these groups say
 * nothing about a damaged stream. That path is covered by the tests above
 * this section, which set the error bits by hand. This test is here so that groups recorded
 * in worse conditions are noticed rather than quietly changing what the other
 * tests mean.
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
 * The two weak captures do have damage in them, and the decoder still gets the
 * right answer out of them.
 *
 * 91.1 and 106.4 read about 10 dBuV in these instead of the usual 45.
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
    /* Block A survives every single group of both recordings while B, C and D
     * do not. The chip protects the identifier hardest, which is why the
     * identifier is the one thing that never comes out wrong here. */
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
 * This is the test the clean block rule exists for. Accepting blocks the tuner
 * says it corrected, which is what PE5PVB's TEF6686_ESP32 firmware does, puts
 * "MAwQCFM VINTOO..." and a text whose terminator was corrupted, so it runs
 * on into padding, onto the panel from these two recordings. With the rule,
 * every text they publish is one of the handful the two stations actually
 * sent.
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
 * No capture holds a list of alternative frequencies.
 *
 * Four of the six send group 0A, but every code in it is 224 or 205, which
 * mean "none follow" and "filler". So the alternative frequency decoding is
 * proved only by the made up groups above, and this test records that rather
 * than leaving the empty list looking like a decoder that failed.
 */
static void no_capture_lists_an_alternative(void) {
  for (int c = 0; c < CAPTURE_COUNT; c++) {
    replay(&kCaptures[c], kCaptures[c].count);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, rds.info.afCount, kCaptures[c].name);
  }
}

/* No capture holds the time either, so nothing claims to know it. */
static void no_capture_sends_the_time(void) {
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

/*
 * Magic FM 106.4's own decoder identification bits: stereo, not an artificial
 * head, compressed, not dynamic PTY. The tuner's own pilot reading is not in
 * the recorded groups, so this is a fact about what the broadcaster sent, not a
 * comparison against it.
 */
static void magic_fm_1064_decoder_identification(void) {
  replay(capture(STRONG_106400), 160);
  TEST_ASSERT_TRUE(rds.info.hasDiStereo);
  TEST_ASSERT_TRUE(rds.info.diStereo);
  TEST_ASSERT_TRUE(rds.info.hasDiArtificialHead);
  TEST_ASSERT_FALSE(rds.info.diArtificialHead);
  TEST_ASSERT_TRUE(rds.info.hasDiCompressed);
  TEST_ASSERT_TRUE(rds.info.diCompressed);
  TEST_ASSERT_TRUE(rds.info.hasDiDynamicPty);
  TEST_ASSERT_FALSE(rds.info.diDynamicPty);
}

/* 106.4 sends 0A and 2A only: in its first 160 recorded groups, 80 of
 * each. */
static void magic_fm_1064_group_types(void) {
  replay(capture(STRONG_106400), 160);
  TEST_ASSERT_EQUAL_UINT32(80, rds.info.groupTypeCount[0][0]);
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.groupTypeCount[0][1]);
  TEST_ASSERT_EQUAL_UINT32(80, rds.info.groupTypeCount[2][0]);
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.groupTypeCount[2][1]);
  /* Nothing else has ever been sent, group 1 and group 10 included. */
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.groupTypeCount[1][0]);
  TEST_ASSERT_EQUAL_UINT32(0, rds.info.groupTypeCount[10][0]);
  TEST_ASSERT_FALSE(rds.info.hasPin);
  TEST_ASSERT_FALSE(rds.info.hasPtyn);
}

/*
 * None of the recorded stations sends group 1 or group 10. So, like the AF
 * and clock tests, the PIN and PTYN tests use hand-made groups.
 */
static uint16_t pinBlockD(uint8_t day, uint8_t hour, uint8_t minute) {
  return (uint16_t)(((uint16_t)(day & 0x1F) << 11) |
                    ((uint16_t)(hour & 0x1F) << 6) | (minute & 0x3Fu));
}

static void a_pin_agreeing_twice_is_published(void) {
  RdsRead r = group(0x1234, blockB1A(10), 0, pinBlockD(15, 21, 0));
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasPin);
  rdsFeed(&rds, &r);
  TEST_ASSERT_TRUE(rds.info.hasPin);
  TEST_ASSERT_EQUAL_UINT8(15, rds.info.pinDay);
  TEST_ASSERT_EQUAL_UINT8(21, rds.info.pinHour);
  TEST_ASSERT_EQUAL_UINT8(0, rds.info.pinMinute);
}

static void a_pin_with_day_zero_is_not_a_date(void) {
  /* RDS: The Radio Data System (Kopitz and Marks), Section 4.6: day 1 to 31.
   * Zero is what "no PIN sent" looks like. */
  RdsRead r = group(0x1234, blockB1A(10), 0, pinBlockD(0, 21, 0));
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasPin);
}

static void an_hour_or_minute_out_of_range_is_not_a_pin(void) {
  RdsRead hourBad = group(0x1234, blockB1A(10), 0, pinBlockD(15, 24, 0));
  rdsFeed(&rds, &hourBad);
  rdsFeed(&rds, &hourBad);
  TEST_ASSERT_FALSE(rds.info.hasPin);
  RdsRead minuteBad = group(0x1234, blockB1A(10), 0, pinBlockD(15, 21, 60));
  rdsFeed(&rds, &minuteBad);
  rdsFeed(&rds, &minuteBad);
  TEST_ASSERT_FALSE(rds.info.hasPin);
}

static uint16_t blockB10A(uint8_t pty, uint8_t segment) {
  return (uint16_t)((10u << 12) | ((uint16_t)(pty & 0x1F) << 5) |
                    (segment & 0x01u));
}

static void sendPtyn(const char *name) {
  for (int round = 0; round < 2; round++) {
    for (uint8_t seg = 0; seg < 2; seg++) {
      RdsRead r = group(0x1234, blockB10A(10, seg),
                        pair(name[seg * 4], name[seg * 4 + 1]),
                        pair(name[seg * 4 + 2], name[seg * 4 + 3]));
      rdsFeed(&rds, &r);
    }
  }
}

static void a_ptyn_agreeing_twice_is_published(void) {
  sendPtyn("CRICKET ");
  TEST_ASSERT_TRUE(rds.info.hasPtyn);
  TEST_ASSERT_EQUAL_STRING("CRICKET ", rds.info.ptyn);
}

static void a_ptyn_10b_group_is_not_read(void) {
  /* 10B carries something this radio does not use, the same reasoning
   * group 4B already gets: decoding it as PTYN would show text a station
   * never sent as its programme type name. */
  uint16_t b = (uint16_t)(blockB10A(10, 0) | 0x0800u);
  RdsRead r = group(0x1234, b, pair('X', 'X'), pair('X', 'X'));
  rdsFeed(&rds, &r);
  rdsFeed(&rds, &r);
  TEST_ASSERT_FALSE(rds.info.hasPtyn);
}

static void the_coverage_area_names_are_the_standard_table(void) {
  /* The second nibble from the left, so 0x0N64 puts the coverage code
   * under test at N regardless of what the other three nibbles hold. */
  TEST_ASSERT_EQUAL_STRING("Local", rdsPiOriginName(0x0064));
  TEST_ASSERT_EQUAL_STRING("International", rdsPiOriginName(0x0164));
  TEST_ASSERT_EQUAL_STRING("National", rdsPiOriginName(0x0264));
  TEST_ASSERT_EQUAL_STRING("Supra-Regional", rdsPiOriginName(0x0364));
  TEST_ASSERT_EQUAL_STRING("Regional 1", rdsPiOriginName(0x0464));
  TEST_ASSERT_EQUAL_STRING("Regional 12", rdsPiOriginName(0x0F64));
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

/*
 * The real broadcast, replayed whole.
 *
 * The hand-written test above works from names typed into it. This one replays
 * every group Mirchi 95 actually transmitted, so the stitching is checked
 * against the timing and the repetition the station really uses rather than
 * against a pattern chosen to suit it.
 */
static void mirchi_950_name_is_stitched_from_the_real_capture(void) {
  const Capture *cap = capture(STRONG_95000);
  replay(cap, cap->count);
  TEST_ASSERT_TRUE(rds.info.hasPsLong);
  TEST_ASSERT_EQUAL_STRING("MIRCHI 95", rds.info.psLong);
}

/*
 * Radio City rotates nine names and none of them is stitched to another.
 *
 * Eight are centred or trailing padded. The one that fills the field,
 * `Madhi Lo`, is followed by the centred `  Moge  `. So no pair qualifies.
 * This is the station the rule has to leave alone, and it is checked on
 * every recorded group rather than the first 160, because four of the nine
 * names first come round after group 160.
 */
static void radio_city_911_is_left_alone_on_the_real_capture(void) {
  const Capture *cap = capture(STRONG_91100);
  replay(cap, cap->count);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_FALSE(rds.info.hasPsLong);
}

/*
 * Damage does not invent a split.
 *
 * The weak Radio City groups are the one recorded place where a pair
 * qualifies by accident: corruption turns `Madhi Lo` into `Maadi Lo` and
 * ` MdhhiLo`, and each of those runs into the real `Madhi Lo`. Both happen
 * once in forty three name changes, against Mirchi's five in ten, so needing
 * two sightings keeps it out.
 */
static void damage_does_not_invent_a_split(void) {
  const Capture *cap = capture(WEAK_91100);
  replay(cap, cap->count);
  TEST_ASSERT_FALSE(rds.info.hasPsLong);
}

/* No station that sends one name is ever given a stitched one. */
static void the_single_name_stations_are_never_stitched(void) {
  const char *single[4] = {STRONG_93500, STRONG_94300, STRONG_98300,
                           STRONG_106400};
  for (int i = 0; i < 4; i++) {
    const Capture *cap = capture(single[i]);
    replay(cap, cap->count);
    TEST_ASSERT_TRUE(rds.info.hasPs);
    TEST_ASSERT_FALSE(rds.info.hasPsLong);
  }
}

/* Radio City rotates five different names through the eight characters. */
static void radio_city_911_decodes(void) {
  replay(capture(STRONG_91100), 160);
  TEST_ASSERT_TRUE(rds.info.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x3712, rds.info.pi);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING("RADIO CITY 91.1", rds.info.rt);
}

/* ------------------------------------------------- recorded DX groups -- */

/*
 * Recorded DX groups, from a band sweep and from channels beside strong
 * stations, fed through the decoder.
 */

static void replayDx(const DxChannel *c, uint16_t upTo) {
  rdsReset(&rds, c->khz);
  for (uint16_t i = 0; i < upTo && i < c->groupCount; i++) {
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
}

static const DxChannel *dxAt(uint32_t khz) {
  for (size_t i = 0; i < DX_SWEEP_COUNT; i++) {
    if (kDxSweep[i].khz == khz) {
      return &kDxSweep[i];
    }
  }
  TEST_FAIL_MESSAGE("that channel is not in the sweep");
  return NULL;
}

/* 94.3 and 95.0 send 0000 in every group. */
static void the_two_stations_sending_zero_say_so(void) {
  const uint32_t zero[] = {94300, 95000};
  for (size_t i = 0; i < 2; i++) {
    replayDx(dxAt(zero[i]), UINT16_MAX);
    TEST_ASSERT_TRUE(rds.info.piZero);
    TEST_ASSERT_FALSE(rds.info.hasPi);
  }
}

/* Unsure from the first clean block A to the second, and confirmed from
 * then on, on every station that sends an identifier. 106.4 is the one
 * whose first block A came back corrected, so it has to wait a group. */
static void a_station_is_unsure_until_its_second_clean_block_a(void) {
  const uint32_t stations[] = {93500, 98300, 106400};
  for (size_t i = 0; i < 3; i++) {
    const DxChannel *c = dxAt(stations[i]);
    int clean = 0;
    for (uint16_t n = 1; n <= c->groupCount && clean < 2; n++) {
      if ((c->groups[n - 1].error >> 6) == 0) {
        clean++;
      }
      replayDx(c, n);
      TEST_ASSERT_EQUAL(clean > 0, rds.info.hasPiHeard);
      TEST_ASSERT_EQUAL(clean > 1, rds.info.hasPi);
    }
    TEST_ASSERT_EQUAL(2, clean);
    TEST_ASSERT_EQUAL_HEX16(rds.info.piHeard, rds.info.pi);
  }
}

/* The one clean block A that noise produced in the whole sweep is heard,
 * and stays unsure, which is what the page is to show. */
static void noise_can_be_heard_but_is_never_confirmed(void) {
  replayDx(dxAt(90700), UINT16_MAX);
  TEST_ASSERT_TRUE(rds.info.hasPiHeard);
  TEST_ASSERT_EQUAL_HEX16(0xE688, rds.info.piHeard);
  TEST_ASSERT_FALSE(rds.info.hasPi);
  TEST_ASSERT_FALSE(rds.info.piZero);
}

/* 93.6, beside Red FM, over a minute: the name builds before it is
 * published, and every position heard is a letter the station sent. */
static void a_name_builds_before_it_is_published(void) {
  const DxChannel *c = &kDxMinute[1];
  TEST_ASSERT_EQUAL_UINT32(93600, c->khz);
  const char sent[] = "  RED   ";
  bool sawPartial = false;
  for (uint16_t n = 1; n <= c->groupCount; n++) {
    replayDx(c, n);
    int heard = 0;
    for (int i = 0; i < RDS_PS_LEN; i++) {
      if (rds.info.psHeardHave[i]) {
        TEST_ASSERT_EQUAL_CHAR(sent[i], rds.info.psHeard[i]);
        heard++;
      }
    }
    if (heard > 0 && !rds.info.hasPs) {
      sawPartial = true;
    }
  }
  TEST_ASSERT_TRUE(sawPartial);
  TEST_ASSERT_TRUE(rds.info.hasPs);
  TEST_ASSERT_EQUAL_STRING(sent, rds.info.ps);
}

/* The last recorded group sets the meters, block for block. */
static void the_block_errors_follow_the_capture(void) {
  const DxChannel *c = &kDxMinute[0];
  replayDx(c, c->groupCount);
  uint8_t last = c->groups[c->groupCount - 1].error;
  TEST_ASSERT_TRUE(rds.info.hasBlockErrors);
  for (int b = 0; b < 4; b++) {
    TEST_ASSERT_EQUAL_UINT8((last >> (6 - b * 2)) & 0x03,
                            rds.info.blockError[b]);
  }
}

static void a_name_is_shown_without_its_padding(void) {
  char out[RDS_PS_LONG_LEN + 1];
  /* The spaces a station centres its name with go; one inside stays. */
  TEST_ASSERT_TRUE(rdsNameTrim(" MAGIC  ", 8, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("MAGIC", out);
  TEST_ASSERT_TRUE(rdsNameTrim("RADIO 1 ", 8, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("RADIO 1", out);
  /* Eight characters with no terminator, as the DX page keeps them. */
  const char cells[8] = {'W', 'W', 'W', 'W', 'W', 'W', 'W', 'W'};
  TEST_ASSERT_TRUE(rdsNameTrim(cells, sizeof(cells), out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("WWWWWWWW", out);
  /* Spaces alone leave nothing to show. */
  TEST_ASSERT_FALSE(rdsNameTrim("        ", 8, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
  TEST_ASSERT_FALSE(rdsNameTrim(NULL, 8, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
  /* Cut to what `out` holds. */
  char small[4];
  TEST_ASSERT_TRUE(rdsNameTrim(" MAGIC  ", 8, small, sizeof(small)));
  TEST_ASSERT_EQUAL_STRING("MAG", small);
  TEST_ASSERT_FALSE(rdsNameTrim("MAGIC", 5, small, 0));
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
  RUN_TEST(a_name_split_across_two_passes_is_stitched);
  RUN_TEST(stitching_leaves_the_pass_alone);
  RUN_TEST(padded_names_are_never_stitched);
  RUN_TEST(a_full_name_followed_by_a_padded_one_is_not_a_split);
  RUN_TEST(a_name_split_across_three_passes_is_stitched);
  RUN_TEST(a_ticker_of_full_names_is_never_stitched);
  RUN_TEST(two_full_words_are_not_run_together);
  RUN_TEST(a_split_longer_than_the_cap_is_left_alone);
  RUN_TEST(a_stitched_name_is_never_offered_empty);
  RUN_TEST(a_stitched_name_is_not_indented);
  RUN_TEST(retuning_forgets_a_stitched_name);
  RUN_TEST(one_name_is_never_stitched);
  RUN_TEST(two_names_alternating_every_pass_show_neither);
  RUN_TEST(a_damaged_block_b_decodes_nothing_but_the_identifier);
  RUN_TEST(a_character_outside_the_shown_range_becomes_an_underscore);

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
  RUN_TEST(no_capture_lists_an_alternative);
  RUN_TEST(no_capture_sends_the_time);
  RUN_TEST(red_fm_935_decodes);
  RUN_TEST(magic_fm_1064_decodes);
  RUN_TEST(magic_fm_1064_decoder_identification);
  RUN_TEST(magic_fm_1064_group_types);
  RUN_TEST(a_pin_agreeing_twice_is_published);
  RUN_TEST(a_pin_with_day_zero_is_not_a_date);
  RUN_TEST(an_hour_or_minute_out_of_range_is_not_a_pin);
  RUN_TEST(a_ptyn_agreeing_twice_is_published);
  RUN_TEST(a_ptyn_10b_group_is_not_read);
  RUN_TEST(the_coverage_area_names_are_the_standard_table);
  RUN_TEST(mirchi_983_decodes);
  RUN_TEST(fever_fm_943_decodes);
  RUN_TEST(mirchi_950_decodes);
  RUN_TEST(mirchi_950_name_is_stitched_from_the_real_capture);
  RUN_TEST(radio_city_911_is_left_alone_on_the_real_capture);
  RUN_TEST(damage_does_not_invent_a_split);
  RUN_TEST(the_single_name_stations_are_never_stitched);
  RUN_TEST(radio_city_911_decodes);

  RUN_TEST(one_clean_hearing_is_heard_but_not_confirmed);
  RUN_TEST(the_digits_that_changed_are_the_unsure_ones);
  RUN_TEST(a_corrected_block_a_is_not_heard);
  RUN_TEST(nothing_to_format_without_somewhere_to_put_it);
  RUN_TEST(a_station_sending_zero_says_so_after_two_hearings);
  RUN_TEST(a_zero_between_two_hearings_does_not_stop_them_agreeing);
  RUN_TEST(a_real_identifier_ends_the_zero);
  RUN_TEST(the_block_errors_are_the_last_groups);
  RUN_TEST(the_block_errors_go_with_the_lock);
  RUN_TEST(the_name_is_heard_a_pair_at_a_time);
  RUN_TEST(the_heard_name_outlasts_a_pass);
  RUN_TEST(a_new_station_forgets_the_heard_name);
  RUN_TEST(a_retune_forgets_what_was_heard);
  RUN_TEST(the_two_stations_sending_zero_say_so);
  RUN_TEST(a_station_is_unsure_until_its_second_clean_block_a);
  RUN_TEST(noise_can_be_heard_but_is_never_confirmed);
  RUN_TEST(a_name_builds_before_it_is_published);
  RUN_TEST(the_block_errors_follow_the_capture);

  RUN_TEST(the_ecc_arrives_on_its_second_hearing);
  RUN_TEST(a_corrected_block_c_gives_no_ecc);
  RUN_TEST(two_different_eccs_do_not_agree);
  RUN_TEST(another_variant_is_not_an_ecc);
  RUN_TEST(a_b_version_group_has_no_ecc);
  RUN_TEST(a_new_station_clears_the_ecc);
  RUN_TEST(a_name_is_shown_without_its_padding);

  return UNITY_END();
}
