/*
 * Tests for the ECC country table and the North American call letters. Runs
 * on a PC.
 *
 * The cells tested are the ones the three lists behind the table disagreed
 * on, the ones this radio is most likely to meet, and the edges.
 */
#include <stdio.h>
#include <string.h>
#include <unity.h>

#include "core/rds_country.h"

void setUp(void) {}
void tearDown(void) {}

static void india_is_five_with_f2(void) {
  TEST_ASSERT_EQUAL_STRING("IN", rdsCountryCode(0x5123, 0xF2));
}

static void the_same_digit_is_a_different_country_under_each_ecc(void) {
  TEST_ASSERT_EQUAL_STRING("IT", rdsCountryCode(0x5000, 0xE0));
  TEST_ASSERT_EQUAL_STRING("JO", rdsCountryCode(0x5000, 0xE1));
  TEST_ASSERT_EQUAL_STRING("SK", rdsCountryCode(0x5000, 0xE2));
  TEST_ASSERT_EQUAL_STRING("TJ", rdsCountryCode(0x5000, 0xE3));
}

static void germany_has_two_digits(void) {
  TEST_ASSERT_EQUAL_STRING("DE", rdsCountryCode(0x1000, 0xE0));
  TEST_ASSERT_EQUAL_STRING("DE", rdsCountryCode(0xD000, 0xE0));
}

/* Where two of the three lists agreed against the third. */
static void the_disputed_cells_follow_two_lists(void) {
  TEST_ASSERT_EQUAL_STRING("NG", rdsCountryCode(0xF000, 0xD1));
  TEST_ASSERT_EQUAL_STRING("UZ", rdsCountryCode(0xB000, 0xE4));
  TEST_ASSERT_EQUAL_STRING("TM", rdsCountryCode(0xE000, 0xE4));
  TEST_ASSERT_EQUAL_STRING("PR", rdsCountryCode(0x8000, 0xA3));
  TEST_ASSERT_EQUAL_STRING("MK", rdsCountryCode(0x4000, 0xE3));
  TEST_ASSERT_EQUAL_STRING("LC", rdsCountryCode(0xB000, 0xA4));
  TEST_ASSERT_EQUAL_STRING("XK", rdsCountryCode(0x7000, 0xE4));
  TEST_ASSERT_EQUAL_STRING("CD", rdsCountryCode(0xB000, 0xD2));
}

static void the_first_and_last_rows_and_columns(void) {
  TEST_ASSERT_EQUAL_STRING("US", rdsCountryCode(0x1000, 0xA0));
  TEST_ASSERT_EQUAL_STRING("MN", rdsCountryCode(0xF000, 0xF3));
  TEST_ASSERT_EQUAL_STRING("CM", rdsCountryCode(0x1000, 0xD0));
  TEST_ASSERT_EQUAL_STRING("BA", rdsCountryCode(0xF000, 0xE4));
}

static void no_country_is_named_without_a_real_pair(void) {
  /* 0 is not a country identifier, as in a station that sends 0935. */
  TEST_ASSERT_NULL(rdsCountryCode(0x0935, 0xF2));
  /* An ECC outside the table. */
  TEST_ASSERT_NULL(rdsCountryCode(0x5000, 0x00));
  TEST_ASSERT_NULL(rdsCountryCode(0x5000, 0xE5));
  /* A cell the standard leaves empty. */
  TEST_ASSERT_NULL(rdsCountryCode(0xC000, 0xA0));
  TEST_ASSERT_NULL(rdsCountryCode(0xE000, 0xE0));
}

/* The call letters `pi` gives, or "" when it gives none. */
static const char *call(uint16_t pi) {
  static char out[5];
  if (!rdsCallSign(pi, out, sizeof(out))) {
    TEST_ASSERT_EQUAL_STRING("", out);
  }
  return out;
}

static void the_standards_own_examples_work_out(void) {
  /* NRSC-4-B D.7.2, examples 1 and 2. */
  TEST_ASSERT_EQUAL_STRING("KGTB", call(0x21C7));
  TEST_ASSERT_EQUAL_STRING("WKTI", call(0x7106));
}

static void the_k_and_w_blocks_end_where_the_standard_says(void) {
  TEST_ASSERT_EQUAL_STRING("", call(0x0FFF));
  /* KAAA and KAAB are 1000 and 1001, which the standard sends moved. */
  TEST_ASSERT_EQUAL_STRING("KAAA", call(0xAFA1));
  TEST_ASSERT_EQUAL_STRING("KAAB", call(0xA101));
  TEST_ASSERT_EQUAL_STRING("KZZY", call(0x54A6));
  TEST_ASSERT_EQUAL_STRING("KZZZ", call(0x54A7));
  TEST_ASSERT_EQUAL_STRING("WAAA", call(0x54A8));
  TEST_ASSERT_EQUAL_STRING("WAAB", call(0x54A9));
  TEST_ASSERT_EQUAL_STRING("WZZY", call(0x994E));
  TEST_ASSERT_EQUAL_STRING("WZZZ", call(0x994F));
}

static void a_moved_pi_is_moved_back_first(void) {
  /* Exception 1, P1 0 P3 P4 sent as A P1 P3 P4: the standard's own three. */
  TEST_ASSERT_EQUAL_STRING("KACR", call(0xA145)); /* 1045 */
  TEST_ASSERT_EQUAL_STRING("KMMK", call(0xA3F2)); /* 30F2 */
  TEST_ASSERT_EQUAL_STRING("WQQZ", call(0xA8A1)); /* 80A1 */
  /* Exception 2, P1 P2 0 0 sent as A F P1 P2. */
  TEST_ASSERT_EQUAL_STRING("KEOE", call(0xAF1C)); /* 1C00 */
  TEST_ASSERT_EQUAL_STRING("KMWU", call(0xAF32)); /* 3200 */
  /* 1000 took both: 1000 to A100 to AFA1. */
  TEST_ASSERT_EQUAL_STRING("KAAA", call(0xAFA1));
  TEST_ASSERT_EQUAL_STRING("KADW", call(0xA164)); /* 1064 */
  TEST_ASSERT_EQUAL_STRING("WWMI", call(0xAFA9)); /* 9000 */
}

/* The standard sends each of these moved, so as they are they belong to no
 * station: a station that sends 1064 is not KADW, which would send A164. */
static void a_pi_the_standard_always_moves_gives_no_call_as_it_is(void) {
  TEST_ASSERT_EQUAL_STRING("", call(0x1000));
  TEST_ASSERT_EQUAL_STRING("", call(0x1001));
  TEST_ASSERT_EQUAL_STRING("", call(0x1045));
  TEST_ASSERT_EQUAL_STRING("", call(0x1064));
  TEST_ASSERT_EQUAL_STRING("", call(0x1C00));
  TEST_ASSERT_EQUAL_STRING("", call(0x80A1));
  TEST_ASSERT_EQUAL_STRING("", call(0x9000));
  /* Moved only half way: 1000 is sent as AFA1, never as A100. */
  TEST_ASSERT_EQUAL_STRING("", call(0xA100));
}

static void the_three_letter_calls_come_from_the_table(void) {
  TEST_ASSERT_EQUAL_STRING("KEX", call(0x9950)); /* The first. */
  TEST_ASSERT_EQUAL_STRING("KYW", call(0x996B));
  TEST_ASSERT_EQUAL_STRING("WWL", call(0x9989));
  TEST_ASSERT_EQUAL_STRING("WRC", call(0x99B9)); /* The last. */
  /* Inside 9950 to 9EFF but not in the table. */
  TEST_ASSERT_EQUAL_STRING("", call(0x9961));
  TEST_ASSERT_EQUAL_STRING("", call(0x99BA));
  TEST_ASSERT_EQUAL_STRING("", call(0x9EFF));
  TEST_ASSERT_EQUAL_STRING("", call(0x9F00));
}

static void a_pi_that_names_no_station_gives_no_call(void) {
  TEST_ASSERT_EQUAL_STRING("", call(0x0000));
  /* Networks linked across stations, NPR-1 among them. */
  TEST_ASSERT_EQUAL_STRING("", call(0xB101));
  TEST_ASSERT_EQUAL_STRING("", call(0xD201));
  TEST_ASSERT_EQUAL_STRING("", call(0xE301));
  /* Canada and Mexico. */
  TEST_ASSERT_EQUAL_STRING("", call(0xC241));
  TEST_ASSERT_EQUAL_STRING("", call(0xF123));
  /* Moved back to nothing: A0 and AF00 hold no call letters. */
  TEST_ASSERT_EQUAL_STRING("", call(0xA012));
  TEST_ASSERT_EQUAL_STRING("", call(0xAF00));

  char small[4];
  TEST_ASSERT_FALSE(rdsCallSign(0x21C7, small, sizeof(small)));
  TEST_ASSERT_FALSE(rdsCallSign(0x21C7, NULL, 5));
}

static void a_call_shows_only_on_north_america(void) {
  RdsInfo info;
  memset(&info, 0, sizeof(info));
  info.hasPi = true;
  info.pi = 0x21C7;
  char out[RDS_CALL_TEXT_LEN];
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_GUESS,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("KGTB", out);
  /* In Europe the same PI is a country and an area, never call letters. */
  TEST_ASSERT_EQUAL_INT(RDS_CALL_NONE, rdsStationCall(&info, RDS_REGION_EUROPE,
                                                      out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
  /* A PI heard once is not confirmed, and gives nothing. */
  info.hasPi = false;
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_NONE,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, out, sizeof(out)));
  /* Canada names no call letters. */
  info.hasPi = true;
  info.pi = 0xC241;
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_NONE,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, out, sizeof(out)));
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_NONE,
      rdsStationCall(NULL, RDS_REGION_NORTH_AMERICA, out, sizeof(out)));
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_NONE,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, NULL, sizeof(out)));
}

/* The decoder keeping the station's own RT+ StationName.Short is tested in
 * test_rds_extra. Here only that it wins over the PI, and that the PI is used
 * without it. */
static void the_stations_own_short_name_wins_over_the_pi(void) {
  RdsInfo info;
  memset(&info, 0, sizeof(info));
  info.hasPi = true;
  info.pi = 0x21C7;
  info.hasStationShort = true;
  snprintf(info.stationShort, sizeof(info.stationShort), "WXYZ");
  char out[RDS_CALL_TEXT_LEN];
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_SENT,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("WXYZ", out);
  /* Sixteen characters fit whole. */
  snprintf(info.stationShort, sizeof(info.stationShort), "ON THIS STATION");
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_SENT,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("ON THIS STATION", out);
  /* A slot too small for it is not given a cut name: the PI is used. */
  char small[8];
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_GUESS,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, small, sizeof(small)));
  TEST_ASSERT_EQUAL_STRING("KGTB", small);
  /* With none kept, the PI. */
  info.hasStationShort = false;
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_GUESS,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("KGTB", out);
}

/* NRSC-4-B D.7.4: a TMC station may send 1 in place of its PI's first
 * digit, so a PI starting 1 from one gives no guess. Any other digit does. */
static void a_tmc_station_starting_1_gives_no_guess(void) {
  RdsInfo info;
  memset(&info, 0, sizeof(info));
  info.hasPi = true;
  info.pi = 0x1106;
  info.tmc = true;
  char out[RDS_CALL_TEXT_LEN];
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_NONE,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, out, sizeof(out)));
  info.tmc = false;
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_GUESS,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, out, sizeof(out)));
  info.pi = 0x21C7;
  info.tmc = true;
  TEST_ASSERT_EQUAL_INT(
      RDS_CALL_GUESS,
      rdsStationCall(&info, RDS_REGION_NORTH_AMERICA, out, sizeof(out)));
}

static void only_the_b_d_and_e_blocks_carry_an_area_in_north_america(void) {
  TEST_ASSERT_TRUE(rdsPiHasArea(0x21C7, RDS_REGION_EUROPE));
  TEST_ASSERT_TRUE(rdsPiHasArea(0x5123, RDS_REGION_EUROPE));
  TEST_ASSERT_TRUE(rdsPiHasArea(0xB101, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_TRUE(rdsPiHasArea(0xD201, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_TRUE(rdsPiHasArea(0xE301, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_FALSE(rdsPiHasArea(0xA164, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_FALSE(rdsPiHasArea(0xC241, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_FALSE(rdsPiHasArea(0xF123, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_FALSE(rdsPiHasArea(0x21C7, RDS_REGION_NORTH_AMERICA));
}

static void north_america_names_a_programme_type_its_own_way(void) {
  TEST_ASSERT_EQUAL_STRING("Education", rdsPtyNameIn(5, RDS_REGION_EUROPE));
  TEST_ASSERT_EQUAL_STRING("Rock", rdsPtyNameIn(5, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_EQUAL_STRING("None", rdsPtyNameIn(0, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_EQUAL_STRING("Top 40", rdsPtyNameIn(9, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_EQUAL_STRING("Unassigned",
                           rdsPtyNameIn(27, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_EQUAL_STRING("Emergency",
                           rdsPtyNameIn(31, RDS_REGION_NORTH_AMERICA));
  TEST_ASSERT_EQUAL_STRING("None", rdsPtyNameIn(32, RDS_REGION_NORTH_AMERICA));
  for (uint8_t i = 0; i < 32; i++) {
    TEST_ASSERT_TRUE(rdsPtyNameIn(i, RDS_REGION_NORTH_AMERICA)[0] != '\0');
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(india_is_five_with_f2);
  RUN_TEST(the_same_digit_is_a_different_country_under_each_ecc);
  RUN_TEST(germany_has_two_digits);
  RUN_TEST(the_disputed_cells_follow_two_lists);
  RUN_TEST(the_first_and_last_rows_and_columns);
  RUN_TEST(no_country_is_named_without_a_real_pair);
  RUN_TEST(the_standards_own_examples_work_out);
  RUN_TEST(the_k_and_w_blocks_end_where_the_standard_says);
  RUN_TEST(a_moved_pi_is_moved_back_first);
  RUN_TEST(a_pi_the_standard_always_moves_gives_no_call_as_it_is);
  RUN_TEST(the_three_letter_calls_come_from_the_table);
  RUN_TEST(a_pi_that_names_no_station_gives_no_call);
  RUN_TEST(a_call_shows_only_on_north_america);
  RUN_TEST(the_stations_own_short_name_wins_over_the_pi);
  RUN_TEST(a_tmc_station_starting_1_gives_no_guess);
  RUN_TEST(only_the_b_d_and_e_blocks_carry_an_area_in_north_america);
  RUN_TEST(north_america_names_a_programme_type_its_own_way);
  return UNITY_END();
}
