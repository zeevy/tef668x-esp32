/* Tests for the XDR protocol's lines. Runs on a PC. */
#include <unity.h>

#include "../test_dx_sweep/capture.h"
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
  /* F comes with its width as W after it; N and newer letters are not
   * offered. Ignored rather than refused, as xdrd does. */
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, parse("F").kind);
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, parse("F-1").kind);
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, parse("N").kind);
  TEST_ASSERT_EQUAL_INT(XDR_IGNORE, parse("Sz0").kind);
}

/* The scan lines as XDR-GTK and the Spectrum Graph plugin send them. */
static void the_scan_lines_are_read(void) {
  XdrCommand c = parse("Sa87500");
  TEST_ASSERT_EQUAL_INT(XDR_SCAN_FROM, c.kind);
  TEST_ASSERT_EQUAL_INT32(87500, c.value);
  TEST_ASSERT_EQUAL_INT(XDR_SCAN_TO, parse("Sb108000").kind);
  c = parse("Sc100");
  TEST_ASSERT_EQUAL_INT(XDR_SCAN_STEP, c.kind);
  TEST_ASSERT_EQUAL_INT32(100, c.value);
  c = parse("Sw114000");
  TEST_ASSERT_EQUAL_INT(XDR_SCAN_WIDTH, c.kind);
  TEST_ASSERT_EQUAL_INT32(114000, c.value);
  TEST_ASSERT_EQUAL_INT32(0, parse("Sw0").value);
  /* PE5PVB's filter numbers: 3 is 114 kHz, 26 is 64, 0 is 56. */
  c = parse("Sf3");
  TEST_ASSERT_EQUAL_INT(XDR_SCAN_WIDTH, c.kind);
  TEST_ASSERT_EQUAL_INT32(114000, c.value);
  TEST_ASSERT_EQUAL_INT32(64000, parse("Sf26").value);
  TEST_ASSERT_EQUAL_INT32(56000, parse("Sf0").value);
  TEST_ASSERT_EQUAL_INT32(311000, parse("Sf15").value);
  c = parse("S");
  TEST_ASSERT_EQUAL_INT(XDR_SCAN_RUN, c.kind);
  TEST_ASSERT_EQUAL_INT32(0, c.value);
  c = parse("Sm");
  TEST_ASSERT_EQUAL_INT(XDR_SCAN_RUN, c.kind);
  TEST_ASSERT_EQUAL_INT32(1, c.value);
  TEST_ASSERT_EQUAL_INT(XDR_SCAN_STOP, parse("").kind);
  TEST_ASSERT_EQUAL_INT(XDR_SCAN_STOP, parse("\r").kind);
  refused("Sa0");
  refused("Sa");
  refused("Sc-100");
  refused("Sb200001");
  refused("Sw-1");
  refused("Sw400001");
  refused("Sf2");
  refused("Sf");
  refused("Smm");
}

/* A PC's range as the sweep's channels: cut to the band, on the PC's grid,
 * and refused with a reason when it cannot be swept. */
static void a_scan_range_is_cut_to_the_band(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  DxSweepRange r;
  TEST_ASSERT_NULL(xdrScanRange(BAND_FM, &plan, 87500, 108000, 100, &r));
  TEST_ASSERT_EQUAL_UINT32(87500, r.lowKHz);
  TEST_ASSERT_EQUAL_UINT16(100, r.stepKHz);
  TEST_ASSERT_EQUAL_UINT16(206, r.count);
  /* Either way round. */
  TEST_ASSERT_NULL(xdrScanRange(BAND_FM, &plan, 108000, 87500, 100, &r));
  TEST_ASSERT_EQUAL_UINT16(206, r.count);
  /* 87.0 is under this band plan's 87.5: it starts on the PC's first
   * channel inside, and stops at the top. */
  TEST_ASSERT_NULL(xdrScanRange(BAND_FM, &plan, 87050, 110000, 100, &r));
  TEST_ASSERT_EQUAL_UINT32(87550, r.lowKHz);
  TEST_ASSERT_EQUAL_UINT32(107950, r.lowKHz + (r.count - 1u) * r.stepKHz);
  /* 431 points fit, and more are cut to the first 431, as the Spectrum
   * Graph plugin's 86 to 108 MHz in 50 kHz would be on a band from 76. */
  TEST_ASSERT_NULL(xdrScanRange(BAND_FM, &plan, 87500, 91800, 10, &r));
  TEST_ASSERT_EQUAL_UINT16(DX_SWEEP_MAX, r.count);
  TEST_ASSERT_NULL(xdrScanRange(BAND_FM, &plan, 87500, 108000, 10, &r));
  TEST_ASSERT_EQUAL_UINT16(DX_SWEEP_MAX, r.count);
  TEST_ASSERT_EQUAL_UINT32(87500, r.lowKHz);
  /* Off FM's 10 kHz grid, the start and the step go up to it. */
  TEST_ASSERT_NULL(xdrScanRange(BAND_FM, &plan, 87500, 88000, 5, &r));
  TEST_ASSERT_EQUAL_UINT16(10, r.stepKHz);
  TEST_ASSERT_EQUAL_UINT16(51, r.count);
  TEST_ASSERT_NULL(xdrScanRange(BAND_FM, &plan, 87505, 88005, 100, &r));
  TEST_ASSERT_EQUAL_UINT32(87510, r.lowKHz);
  /* Outside the band, no step, too wide a step. */
  TEST_ASSERT_NOT_NULL(xdrScanRange(BAND_FM, &plan, 65750, 74000, 30, &r));
  TEST_ASSERT_NOT_NULL(xdrScanRange(BAND_FM, &plan, 87500, 108000, 0, &r));
  TEST_ASSERT_NOT_NULL(xdrScanRange(BAND_FM, &plan, 87500, 108000, 70000, &r));
  /* 65535 is a step on AM, but on FM it goes up past the most there is. */
  TEST_ASSERT_NOT_NULL(xdrScanRange(BAND_FM, &plan, 87500, 108000, 65535, &r));
  /* Inside the band, but none of the PC's own channels is. */
  TEST_ASSERT_NOT_NULL(xdrScanRange(BAND_FM, &plan, 87000, 108000, 65000, &r));
  /* Medium wave in 9 kHz. */
  TEST_ASSERT_NULL(xdrScanRange(BAND_MW, &plan, 522, 1710, 9, &r));
  TEST_ASSERT_EQUAL_UINT16(133, r.count);
  TEST_ASSERT_NOT_NULL(xdrScanRange(BAND_MW, &plan, 87500, 108000, 100, &r));
  TEST_ASSERT_NOT_NULL(xdrScanRange(BAND_MW, &plan, 522, 1710, 9, NULL));
}

/* The U line: a pair a channel with a comma after each, the level in dBf, a
 * channel with no reading left out, and a space before the line end. */
static void the_scan_answer_is_one_line_of_pairs(void) {
  DxSweep s;
  memset(&s, 0, sizeof(s));
  s.lowKHz = 87500;
  s.stepKHz = 100;
  s.count = 3;
  s.level[0] = 100;
  s.level[1] = DX_SWEEP_NO_READING;
  s.level[2] = -150;
  char out[64];
  uint16_t next = 0;
  size_t len = xdrScanPart(out, sizeof(out), &s, &next);
  out[len] = '\0';
  TEST_ASSERT_EQUAL_STRING("U87500=21.3,87700=-3.8, \n", out);
  TEST_ASSERT_EQUAL_UINT16(4, next);
  TEST_ASSERT_EQUAL_size_t(0, xdrScanPart(out, sizeof(out), &s, &next));
  /* In parts too small for the whole line, whole pairs only. */
  next = 0;
  len = xdrScanPart(out, 14, &s, &next);
  out[len] = '\0';
  TEST_ASSERT_EQUAL_STRING("U87500=21.3,", out);
  len = xdrScanPart(out, 14, &s, &next);
  out[len] = '\0';
  TEST_ASSERT_EQUAL_STRING("87700=-3.8, \n", out);
  TEST_ASSERT_EQUAL_size_t(0, xdrScanPart(out, 14, &s, &next));
  /* The line end in a part of its own when the last pair fills one. */
  next = 0;
  (void)xdrScanPart(out, 12, &s, &next);
  len = xdrScanPart(out, 12, &s, &next);
  out[len] = '\0';
  TEST_ASSERT_EQUAL_STRING("87700=-3.8,", out);
  len = xdrScanPart(out, 12, &s, &next);
  out[len] = '\0';
  TEST_ASSERT_EQUAL_STRING(" \n", out);
  next = 0;
  TEST_ASSERT_EQUAL_size_t(0, xdrScanPart(out, 1, &s, &next));
  TEST_ASSERT_EQUAL_size_t(0, xdrScanPart(out, sizeof(out), NULL, &next));
}

/* A whole band off the radio, 211 channels, in the parts the server sends:
 * as many commas as pairs, and the ending both PC programs look for. */
static void a_real_sweep_makes_a_line_both_programs_read(void) {
  static DxSweep s;
  memset(&s, 0, sizeof(s));
  s.lowKHz = CAPTURE_LOW_KHZ;
  s.stepKHz = CAPTURE_STEP_KHZ;
  s.count = CAPTURE_COUNT;
  for (uint16_t i = 0; i < CAPTURE_COUNT; i++) {
    s.level[i] = dxSweepMean(kPass1[i], CAPTURE_READS);
  }
  static char line[8192];
  size_t len = 0;
  uint16_t next = 0;
  for (size_t n; (n = xdrScanPart(line + len, 512, &s, &next)) > 0;) {
    len += n;
  }
  line[len] = '\0';
  size_t commas = 0;
  for (size_t i = 0; i < len; i++) {
    commas += line[i] == ',';
  }
  TEST_ASSERT_EQUAL_size_t(CAPTURE_COUNT, commas);
  TEST_ASSERT_EQUAL_INT(0, strncmp(line, "U87000=", 7));
  TEST_ASSERT_EQUAL_INT(0, strcmp(line + len - 3, ", \n"));
  TEST_ASSERT_NOT_NULL(strstr(line, ",108000="));
  /* The longest line, 431 six digit frequencies at a level of four digits,
   * is under the 5744 bytes a socket takes at once. */
  s.lowKHz = 103700;
  s.stepKHz = 10;
  s.count = DX_SWEEP_MAX;
  for (uint16_t i = 0; i < DX_SWEEP_MAX; i++) {
    s.level[i] = 1200;
  }
  len = 0;
  next = 0;
  for (size_t n; (n = xdrScanPart(line + len, 512, &s, &next)) > 0;) {
    len += n;
  }
  TEST_ASSERT_TRUE(len > 5000 && len < 5744);
}

static bool feedCable(XdrCableLine *line, const char *bytes, size_t n) {
  bool whole = false;
  for (size_t i = 0; i < n; i++) {
    whole = xdrCableFeed(line, (unsigned char)bytes[i]);
  }
  return whole;
}

static void a_cable_line_ends_at_the_line_feed(void) {
  XdrCableLine line = {};
  TEST_ASSERT_FALSE(xdrCableFeed(&line, 'x'));
  TEST_ASSERT_TRUE(xdrCableFeed(&line, '\n'));
  TEST_ASSERT_EQUAL_STRING("x", line.text);
  TEST_ASSERT_TRUE(feedCable(&line, "T87500\r\n", 8));
  TEST_ASSERT_EQUAL_STRING("T87500\r", line.text);
  TEST_ASSERT_TRUE(feedCable(&line, "\n", 1));
  TEST_ASSERT_EQUAL_STRING("", line.text);
}

static void a_glitch_before_a_cable_line_is_dropped(void) {
  XdrCableLine line = {};
  TEST_ASSERT_TRUE(feedCable(&line, "\x00\xffx\n", 4));
  TEST_ASSERT_EQUAL_STRING("x", line.text);
}

static void a_bad_byte_inside_a_cable_line_refuses_it(void) {
  XdrCableLine line = {};
  TEST_ASSERT_FALSE(feedCable(&line,
                              "T87\xb7"
                              "00\n",
                              7));
  TEST_ASSERT_TRUE(feedCable(&line, "T87500\n", 7));
  TEST_ASSERT_EQUAL_STRING("T87500", line.text);
}

static void a_cable_line_of_glitches_is_not_an_empty_line(void) {
  XdrCableLine line = {};
  TEST_ASSERT_FALSE(feedCable(&line, "\x00\n", 2));
  TEST_ASSERT_TRUE(feedCable(&line, "\n", 1));
}

static void a_cable_line_too_long_is_refused(void) {
  XdrCableLine line = {};
  char longer[XDR_LINE_MAX + 3];
  memset(longer, 'A', sizeof(longer) - 1);
  longer[sizeof(longer) - 1] = '\n';
  TEST_ASSERT_FALSE(feedCable(&line, longer, sizeof(longer)));
  TEST_ASSERT_TRUE(feedCable(&line, "x\n", 2));
  TEST_ASSERT_EQUAL_STRING("x", line.text);
  TEST_ASSERT_FALSE(xdrCableFeed(NULL, 'x'));
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
  RUN_TEST(the_scan_lines_are_read);
  RUN_TEST(a_scan_range_is_cut_to_the_band);
  RUN_TEST(the_scan_answer_is_one_line_of_pairs);
  RUN_TEST(a_real_sweep_makes_a_line_both_programs_read);
  RUN_TEST(a_cable_line_ends_at_the_line_feed);
  RUN_TEST(a_glitch_before_a_cable_line_is_dropped);
  RUN_TEST(a_bad_byte_inside_a_cable_line_refuses_it);
  RUN_TEST(a_cable_line_of_glitches_is_not_an_empty_line);
  RUN_TEST(a_cable_line_too_long_is_refused);
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
