/* Tests for the XDR protocol's lines. Runs on a PC. */
#include <unity.h>

#include "core/xdr.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static XdrCommand parse(const char *line) {
  XdrCommand c;
  TEST_ASSERT_NULL_MESSAGE(xdrParse(line, &c), line);
  return c;
}

static void refused(const char *line) {
  XdrCommand c;
  c.kind = XDR_TUNE;
  TEST_ASSERT_NOT_NULL_MESSAGE(xdrParse(line, &c), line);
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, c.kind);
}

static void a_tune_is_100_to_200000_khz(void) {
  XdrCommand c = parse("T87500");
  TEST_ASSERT_EQUAL_INT(XDR_TUNE, c.kind);
  TEST_ASSERT_EQUAL_INT32(87500, c.value);
  TEST_ASSERT_EQUAL_INT32(100, parse("T100").value);
  TEST_ASSERT_EQUAL_INT32(200000, parse("T200000").value);
  refused("T99");
  refused("T200001");
  refused("T");
  refused("T-5");
  refused("T875a");
}

static void t0_is_taken_and_does_nothing(void) {
  /* FM-DX Webserver sends it to clear its own RDS. */
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, parse("T0").kind);
}

static void each_one_number_command_takes_its_range(void) {
  TEST_ASSERT_EQUAL_INT(XDR_MODE, parse("M1").kind);
  refused("M2");
  TEST_ASSERT_EQUAL_INT32(0, parse("W0").value);
  TEST_ASSERT_EQUAL_INT32(400000, parse("W400000").value);
  refused("W400001");
  refused("W-1");
  TEST_ASSERT_EQUAL_INT32(2, parse("D2").value);
  refused("D3");
  TEST_ASSERT_EQUAL_INT32(2, parse("B2").value);
  refused("B3");
  TEST_ASSERT_EQUAL_INT32(0, parse("Y0").value);
  TEST_ASSERT_EQUAL_INT32(100, parse("Y100").value);
  refused("Y101");
  TEST_ASSERT_EQUAL_INT32(-1, parse("Q-1").value);
  TEST_ASSERT_EQUAL_INT32(100, parse("Q100").value);
  refused("Q-2");
  TEST_ASSERT_EQUAL_INT32(3, parse("A3").value);
  refused("A4");
  TEST_ASSERT_EQUAL_INT32(3, parse("Z3").value);
  refused("Z4");
  TEST_ASSERT_EQUAL_INT32(127, parse("V127").value);
  refused("V128");
  TEST_ASSERT_EQUAL_INT(XDR_ROTATOR, parse("C2").kind);
  refused("C3");
}

static void g_is_the_equalizer_then_ims(void) {
  XdrCommand c = parse("G01");
  TEST_ASSERT_EQUAL_INT(XDR_EQ_IMS, c.kind);
  TEST_ASSERT_EQUAL_INT32(0, c.value);
  TEST_ASSERT_EQUAL_INT32(1, c.value2);
  c = parse("G10");
  TEST_ASSERT_EQUAL_INT32(1, c.value);
  TEST_ASSERT_EQUAL_INT32(0, c.value2);
  refused("G2");
  refused("G011");
  refused("G21");
  refused("G");
}

static void i_is_an_interval_and_an_optional_mode(void) {
  XdrCommand c = parse("I100");
  TEST_ASSERT_EQUAL_INT(XDR_INTERVAL, c.kind);
  TEST_ASSERT_EQUAL_INT32(100, c.value);
  c = parse("I250,1");
  TEST_ASSERT_EQUAL_INT32(250, c.value);
  TEST_ASSERT_EQUAL_INT32(1, c.value2);
  TEST_ASSERT_EQUAL_INT32(1000, parse("I1000").value);
  refused("I1001");
  refused("I-1");
  refused("I100,");
  refused("I100x");
}

static void a_session_starts_with_x_and_ends_with_capital_x(void) {
  TEST_ASSERT_EQUAL_INT(XDR_START, parse("x").kind);
  TEST_ASSERT_EQUAL_INT(XDR_END, parse("X").kind);
}

static void what_the_radio_does_not_offer_is_ignored(void) {
  /* F comes with its width as W after it; N, S and newer letters are not
   * offered. Ignored rather than refused, as xdrd does. */
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, parse("F").kind);
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, parse("F-1").kind);
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, parse("N").kind);
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, parse("Sa87500").kind);
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, parse("").kind);
}

static void a_carriage_return_is_dropped(void) {
  TEST_ASSERT_EQUAL_INT32(87500, parse("T87500\r").value);
}

static void a_line_one_over_the_limit_is_refused(void) {
  char line[XDR_LINE_MAX + 2];
  memset(line, 'W', sizeof(line));
  line[XDR_LINE_MAX] = '\0';
  XdrCommand c;
  /* At the limit: understood, and refused only for what it says. */
  TEST_ASSERT_EQUAL_STRING("W is a width in Hz, 0 for automatic.",
                           xdrParse(line, &c));
  line[XDR_LINE_MAX] = 'W';
  line[XDR_LINE_MAX + 1] = '\0';
  TEST_ASSERT_EQUAL_STRING("The line is too long.", xdrParse(line, &c));
}

static void a_ten_digit_number_is_refused(void) {
  refused("T1000000000");
}

static void a_null_is_refused(void) {
  XdrCommand c;
  TEST_ASSERT_NOT_NULL(xdrParse(NULL, &c));
  TEST_ASSERT_NOT_NULL(xdrParse("T87500", NULL));
}

static void dbf_is_dbuv_plus_11_25_rounded_away_from_zero(void) {
  TEST_ASSERT_EQUAL_INT32(113, xdrDbfTenths(0));
  TEST_ASSERT_EQUAL_INT32(373, xdrDbfTenths(260));
  /* Below 0 dBf the sign stays and the rounding goes away from zero. */
  TEST_ASSERT_EQUAL_INT32(-38, xdrDbfTenths(-150));
  TEST_ASSERT_EQUAL_INT32(-1, xdrDbfTenths(-113));
  TEST_ASSERT_EQUAL_INT32(4, xdrDbfTenths(-109));
}

static void the_signal_line_has_its_flag_and_one_decimal(void) {
  char line[24];
  xdrSignal(line, sizeof(line), 260, true, false, false);
  TEST_ASSERT_EQUAL_STRING("Ss37.3", line);
  xdrSignal(line, sizeof(line), 260, true, true, false);
  TEST_ASSERT_EQUAL_STRING("SS37.3", line);
  xdrSignal(line, sizeof(line), 260, false, false, false);
  TEST_ASSERT_EQUAL_STRING("Sm37.3", line);
  xdrSignal(line, sizeof(line), 260, false, true, false);
  TEST_ASSERT_EQUAL_STRING("SM37.3", line);
  /* AM has no pilot and no forced mono. */
  xdrSignal(line, sizeof(line), 260, true, true, true);
  TEST_ASSERT_EQUAL_STRING("Sm37.3", line);
}

static void a_level_below_zero_dbf_keeps_its_sign(void) {
  char line[24];
  xdrSignal(line, sizeof(line), -150, false, false, false);
  TEST_ASSERT_EQUAL_STRING("Sm-3.8", line);
  xdrSignal(line, sizeof(line), -113, false, false, false);
  TEST_ASSERT_EQUAL_STRING("Sm-0.1", line);
}

static void the_signal_line_is_cut_to_its_room(void) {
  char line[6];
  TEST_ASSERT_EQUAL_UINT32(
      5, xdrSignal(line, sizeof(line), 260, true, false, false));
  TEST_ASSERT_EQUAL_STRING("Ss37.", line);
  TEST_ASSERT_EQUAL_UINT32(0, xdrSignal(line, 3, 260, true, false, false));
}

static void the_pi_line_carries_its_doubt(void) {
  char line[12];
  xdrPi(line, sizeof(line), 0x1064, 0);
  TEST_ASSERT_EQUAL_STRING("P1064", line);
  xdrPi(line, sizeof(line), 0xC20A, 2);
  TEST_ASSERT_EQUAL_STRING("PC20A??", line);
  xdrPi(line, sizeof(line), 0x0001, 9);
  TEST_ASSERT_EQUAL_STRING("P0001???", line);
}

static void the_rds_line_is_18_hex_characters(void) {
  const uint16_t block[4] = {0x1064, 0x0548, 0xA8BB, 0x4631};
  char line[24];
  TEST_ASSERT_EQUAL_UINT32(19, xdrRds(line, sizeof(line), block, 0x5A));
  TEST_ASSERT_EQUAL_STRING("R10640548A8BB46315A", line);
}

static void echoes_and_the_users_line(void) {
  char line[16];
  xdrValue(line, sizeof(line), 'Q', -1);
  TEST_ASSERT_EQUAL_STRING("Q-1", line);
  xdrValue(line, sizeof(line), 'T', 87500);
  TEST_ASSERT_EQUAL_STRING("T87500", line);
  xdrEqIms(line, sizeof(line), true, false);
  TEST_ASSERT_EQUAL_STRING("G10", line);
  xdrUsers(line, sizeof(line), 2);
  TEST_ASSERT_EQUAL_STRING("o2,0", line);
}

static void the_salt_uses_xdrds_64_characters(void) {
  uint8_t random[XDR_SALT_LEN];
  for (int i = 0; i < XDR_SALT_LEN; i++) {
    random[i] = (uint8_t)i;
  }
  char salt[XDR_SALT_LEN + 1];
  xdrSalt(random, salt);
  TEST_ASSERT_EQUAL_STRING("QWERTYUIOPASDFGH", salt);
  random[0] = 63;
  random[1] = 64;
  random[2] = 255;
  xdrSalt(random, salt);
  TEST_ASSERT_EQUAL_CHAR('-', salt[0]);
  TEST_ASSERT_EQUAL_CHAR('Q', salt[1]);
  TEST_ASSERT_EQUAL_CHAR('-', salt[2]);
}

static const uint8_t kAbc[XDR_DIGEST_LEN] = {
    0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e,
    0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d};

static void a_digest_is_40_lower_case_hex(void) {
  char hex[XDR_DIGEST_HEX + 1];
  xdrHex(kAbc, hex);
  TEST_ASSERT_EQUAL_STRING("a9993e364706816aba3e25717850c26c9cd0d89d", hex);
}

static void a_login_matches_in_either_case_and_nothing_else(void) {
  char hex[XDR_DIGEST_HEX + 1];
  xdrHex(kAbc, hex);
  TEST_ASSERT_TRUE(xdrDigestMatches(hex, hex));
  TEST_ASSERT_TRUE(
      xdrDigestMatches("A9993E364706816ABA3E25717850C26C9CD0D89D", hex));
  TEST_ASSERT_TRUE(
      xdrDigestMatches("a9993e364706816aba3e25717850c26c9cd0d89d\r", hex));
  TEST_ASSERT_FALSE(
      xdrDigestMatches("a9993e364706816aba3e25717850c26c9cd0d89e", hex));
  TEST_ASSERT_FALSE(
      xdrDigestMatches("a9993e364706816aba3e25717850c26c9cd0d89", hex));
  TEST_ASSERT_FALSE(
      xdrDigestMatches("a9993e364706816aba3e25717850c26c9cd0d89d0", hex));
  TEST_ASSERT_FALSE(xdrDigestMatches("", hex));
  TEST_ASSERT_FALSE(xdrDigestMatches(NULL, hex));
  TEST_ASSERT_FALSE(xdrDigestMatches(hex, NULL));
}

static void volume_100_is_0_db_and_each_step_down_is_06_db(void) {
  TEST_ASSERT_EQUAL_INT8(0, xdrVolumeDb(100));
  TEST_ASSERT_EQUAL_INT8(-25, xdrVolumeDb(58));
  TEST_ASSERT_EQUAL_INT8(-59, xdrVolumeDb(1));
  /* 0 is the caller's mute; out of range is held to the scale. */
  TEST_ASSERT_EQUAL_INT8(-59, xdrVolumeDb(0));
  TEST_ASSERT_EQUAL_INT8(0, xdrVolumeDb(101));
}

static void a_volume_reads_back_as_the_db_in_force(void) {
  TEST_ASSERT_EQUAL_INT32(0, xdrVolumeFromDb(-25, true));
  TEST_ASSERT_EQUAL_INT32(58, xdrVolumeFromDb(-25, false));
  TEST_ASSERT_EQUAL_INT32(100, xdrVolumeFromDb(0, false));
  /* Above the top of the scale it says the top, not a number past it. */
  TEST_ASSERT_EQUAL_INT32(100, xdrVolumeFromDb(6, false));
  TEST_ASSERT_EQUAL_INT32(1, xdrVolumeFromDb(-60, false));
  /* Every whole dB from -59 to 0 comes back as itself, so the echo of a
   * volume set on the panel never moves the panel when a PC sends it back. */
  for (int db = -59; db <= 0; db++) {
    TEST_ASSERT_EQUAL_INT8(db, xdrVolumeDb(xdrVolumeFromDb((int8_t)db, false)));
  }
}

static void a_width_goes_to_the_nearest_the_band_offers(void) {
  TEST_ASSERT_EQUAL_UINT16(114, xdrWidthKHz(BAND_FM, 114000, 236));
  TEST_ASSERT_EQUAL_UINT16(114, xdrWidthKHz(BAND_FM, 110000, 236));
  TEST_ASSERT_EQUAL_UINT16(311, xdrWidthKHz(BAND_FM, 309000, 236));
  TEST_ASSERT_EQUAL_UINT16(56, xdrWidthKHz(BAND_FM, 9000, 236));
  TEST_ASSERT_EQUAL_UINT16(0, xdrWidthKHz(BAND_FM, 0, 236));
  TEST_ASSERT_EQUAL_UINT16(8, xdrWidthKHz(BAND_MW, 8000, 4));
  TEST_ASSERT_EQUAL_UINT16(3, xdrWidthKHz(BAND_MW, 1000, 4));
  /* No automatic width on AM: 0 keeps what it has. */
  TEST_ASSERT_EQUAL_UINT16(4, xdrWidthKHz(BAND_MW, 0, 4));
}

static void the_squelch_number_is_where_it_opens_in_dbf(void) {
  TEST_ASSERT_EQUAL_INT32(0, xdrSquelchValue(SQUELCH_OFF, false, 300, 15));
  /* Auto's FM floor: 15 dBuV is 26.25 dBf. */
  TEST_ASSERT_EQUAL_INT32(26, xdrSquelchValue(SQUELCH_AUTO, false, 300, 15));
  /* Auto with no level in it says on, and nothing more. */
  TEST_ASSERT_EQUAL_INT32(1, xdrSquelchValue(SQUELCH_AUTO, false, 300, 0));
  TEST_ASSERT_EQUAL_INT32(1, xdrSquelchValue(SQUELCH_AUTO, true, 300, 15));
  /* Manual's threshold, on either band, held to 1 to 100. */
  TEST_ASSERT_EQUAL_INT32(41, xdrSquelchValue(SQUELCH_MANUAL, false, 300, 15));
  TEST_ASSERT_EQUAL_INT32(41, xdrSquelchValue(SQUELCH_MANUAL, true, 300, 0));
  TEST_ASSERT_EQUAL_INT32(1, xdrSquelchValue(SQUELCH_MANUAL, false, -200, 15));
  TEST_ASSERT_EQUAL_INT32(100,
                          xdrSquelchValue(SQUELCH_MANUAL, false, 1200, 15));
}

static void de_emphasis_codes_go_both_ways(void) {
  TEST_ASSERT_EQUAL_UINT16(50, xdrDeemphasisUs(0));
  TEST_ASSERT_EQUAL_UINT16(75, xdrDeemphasisUs(1));
  TEST_ASSERT_EQUAL_UINT16(0, xdrDeemphasisUs(2));
  TEST_ASSERT_EQUAL_INT32(0, xdrDeemphasisCode(50));
  TEST_ASSERT_EQUAL_INT32(1, xdrDeemphasisCode(75));
  TEST_ASSERT_EQUAL_INT32(2, xdrDeemphasisCode(0));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(a_tune_is_100_to_200000_khz);
  RUN_TEST(t0_is_taken_and_does_nothing);
  RUN_TEST(each_one_number_command_takes_its_range);
  RUN_TEST(g_is_the_equalizer_then_ims);
  RUN_TEST(i_is_an_interval_and_an_optional_mode);
  RUN_TEST(a_session_starts_with_x_and_ends_with_capital_x);
  RUN_TEST(what_the_radio_does_not_offer_is_ignored);
  RUN_TEST(a_carriage_return_is_dropped);
  RUN_TEST(a_line_one_over_the_limit_is_refused);
  RUN_TEST(a_ten_digit_number_is_refused);
  RUN_TEST(a_null_is_refused);
  RUN_TEST(dbf_is_dbuv_plus_11_25_rounded_away_from_zero);
  RUN_TEST(the_signal_line_has_its_flag_and_one_decimal);
  RUN_TEST(a_level_below_zero_dbf_keeps_its_sign);
  RUN_TEST(the_signal_line_is_cut_to_its_room);
  RUN_TEST(the_pi_line_carries_its_doubt);
  RUN_TEST(the_rds_line_is_18_hex_characters);
  RUN_TEST(echoes_and_the_users_line);
  RUN_TEST(the_salt_uses_xdrds_64_characters);
  RUN_TEST(a_digest_is_40_lower_case_hex);
  RUN_TEST(a_login_matches_in_either_case_and_nothing_else);
  RUN_TEST(volume_100_is_0_db_and_each_step_down_is_06_db);
  RUN_TEST(a_volume_reads_back_as_the_db_in_force);
  RUN_TEST(a_width_goes_to_the_nearest_the_band_offers);
  RUN_TEST(the_squelch_number_is_where_it_opens_in_dbf);
  RUN_TEST(de_emphasis_codes_go_both_ways);
  return UNITY_END();
}
