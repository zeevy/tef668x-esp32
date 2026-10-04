/* Tests for the tuner's reply decode. Runs on a PC, with byte buffers. */
#include <unity.h>

#include <string.h>

#include "core/signal.h"
#include "drivers/tef668x_decode.h"

void setUp(void) {}
void tearDown(void) {}

/* One word into a reply, high byte first, as the chip sends it. */
static void put(uint8_t *buf, int word, uint16_t value) {
  buf[word * 2] = (uint8_t)(value >> 8);
  buf[word * 2 + 1] = (uint8_t)(value & 0xFF);
}

static void quality(uint8_t *buf, uint16_t status, int16_t level,
                    uint16_t noise, uint16_t fourth, int16_t offset,
                    uint16_t bandwidth, int16_t modulation) {
  put(buf, 0, status);
  put(buf, 1, (uint16_t)level);
  put(buf, 2, noise);
  put(buf, 3, fourth);
  put(buf, 4, (uint16_t)offset);
  put(buf, 5, bandwidth);
  put(buf, 6, (uint16_t)modulation);
}

/* -------------------------------------------------------------- quality */

/* A real reading at 104.0 MHz: 10.7 dBuV, noise 24.3, multipath
 * 25.7, 3.0 kHz low, 84 kHz wide and 71 % modulation, settled. */
static void a_real_fm_reading_decodes_as_it_was_read(void) {
  uint8_t buf[TEF668X_QUALITY_BYTES];
  quality(buf, 1000, 107, 243, 257, -30, 840, 710);
  Tef668xQuality q;
  memset(&q, 0, sizeof(q));
  q.stereo = true;
  tef668xDecodeQuality(buf, true, &q);
  TEST_ASSERT_EQUAL_UINT16(1000, q.status);
  TEST_ASSERT_EQUAL_INT16(107, q.levelDbuVTenths);
  TEST_ASSERT_EQUAL_UINT16(243, q.usnTenths);
  TEST_ASSERT_EQUAL_UINT16(257, q.multipathTenths);
  TEST_ASSERT_EQUAL_UINT16(0, q.coChannelTenths);
  TEST_ASSERT_EQUAL_INT16(-30, q.offsetKHzTenths);
  TEST_ASSERT_EQUAL_UINT16(84, q.bandwidthKHz);
  TEST_ASSERT_EQUAL_INT16(71, q.modulationPercent);
  TEST_ASSERT_EQUAL_INT8(signalSnrDb(107, 243, true), q.snrDb);
  /* The pilot comes from another command, so the decode leaves it. */
  TEST_ASSERT_TRUE(q.stereo);
}

/* Read as unsigned, -5.3 dBuV would be 6548.3 and -12.5 kHz 6541.1. */
static void negative_level_offset_and_modulation_keep_their_sign(void) {
  uint8_t buf[TEF668X_QUALITY_BYTES];
  quality(buf, 1000, -53, 0, 0, -125, 560, -35);
  Tef668xQuality q;
  tef668xDecodeQuality(buf, true, &q);
  TEST_ASSERT_EQUAL_INT16(-53, q.levelDbuVTenths);
  TEST_ASSERT_EQUAL_INT16(-125, q.offsetKHzTenths);
  /* Tenths of a percent to whole ones, towards zero. */
  TEST_ASSERT_EQUAL_INT16(-3, q.modulationPercent);
  TEST_ASSERT_EQUAL_UINT8(0xFF, buf[2]);
}

/* The widest and narrowest the chip reports, and the tenths dropped. */
static void the_bandwidth_is_read_in_whole_kilohertz(void) {
  uint8_t buf[TEF668X_QUALITY_BYTES];
  Tef668xQuality q;
  quality(buf, 1000, 0, 0, 0, 0, 3110, 0);
  tef668xDecodeQuality(buf, true, &q);
  TEST_ASSERT_EQUAL_UINT16(311, q.bandwidthKHz);
  quality(buf, 1000, 0, 0, 0, 0, 1849, 0);
  tef668xDecodeQuality(buf, true, &q);
  TEST_ASSERT_EQUAL_UINT16(184, q.bandwidthKHz);
  quality(buf, 1000, 0, 0, 0, 0, 30, 0);
  tef668xDecodeQuality(buf, false, &q);
  TEST_ASSERT_EQUAL_UINT16(3, q.bandwidthKHz);
}

/* On AM the fourth word is co-channel, and the noise is on its own scale,
 * so the same bytes give a different SNR. */
static void an_am_reading_reads_co_channel_and_its_own_noise_scale(void) {
  uint8_t buf[TEF668X_QUALITY_BYTES];
  quality(buf, 1000, 400, 1500, 120, 4, 60, 30);
  Tef668xQuality fm;
  Tef668xQuality am;
  tef668xDecodeQuality(buf, true, &fm);
  tef668xDecodeQuality(buf, false, &am);
  TEST_ASSERT_EQUAL_UINT16(0, am.multipathTenths);
  TEST_ASSERT_EQUAL_UINT16(120, am.coChannelTenths);
  TEST_ASSERT_EQUAL_UINT16(1500, am.usnTenths);
  TEST_ASSERT_EQUAL_INT8(signalSnrDb(400, 1500, false), am.snrDb);
  TEST_ASSERT_EQUAL_INT8(signalSnrDb(400, 1500, true), fm.snrDb);
  TEST_ASSERT_NOT_EQUAL(fm.snrDb, am.snrDb);
}

/* An AF_Update's reading carries the flag in the top bit, and the time stamp
 * below it, both kept whole for the caller to judge. */
static void the_status_word_is_kept_whole(void) {
  uint8_t buf[TEF668X_QUALITY_BYTES];
  quality(buf, 0x8000 | 1000, 0, 0, 0, 0, 0, 0);
  Tef668xQuality q;
  tef668xDecodeQuality(buf, true, &q);
  TEST_ASSERT_EQUAL_HEX16(0x83E8, q.status);
}

/* ------------------------------------------------------------------ RDS */

static void rds(uint8_t *buf, uint16_t status, uint16_t a, uint16_t b,
                uint16_t c, uint16_t d, uint16_t errors) {
  put(buf, 0, status);
  put(buf, 1, a);
  put(buf, 2, b);
  put(buf, 3, c);
  put(buf, 4, d);
  put(buf, 5, errors);
}

#define DATA (1u << 15)
#define PI_ONLY (1u << 13)
#define SYNC (1u << 9)

/* Block A in the top pair of the error word, bits 15 and 14, down to block D
 * in bits 9 and 8: clean, small, large, lost. */
static void a_group_decodes_its_blocks_and_their_errors(void) {
  uint8_t buf[TEF668X_RDS_BYTES];
  rds(buf, DATA | SYNC, 0x1064, 0x0408, 0x2047, 0x4D41, 0x1B00);
  Tef668xRdsRead r;
  tef668xDecodeRds(buf, &r);
  TEST_ASSERT_TRUE(r.read);
  TEST_ASSERT_TRUE(r.synchronised);
  TEST_ASSERT_TRUE(r.haveGroup);
  TEST_ASSERT_EQUAL_HEX16(DATA | SYNC, r.status);
  TEST_ASSERT_EQUAL_HEX16(0x1064, r.block[0]);
  TEST_ASSERT_EQUAL_HEX16(0x0408, r.block[1]);
  TEST_ASSERT_EQUAL_HEX16(0x2047, r.block[2]);
  TEST_ASSERT_EQUAL_HEX16(0x4D41, r.block[3]);
  TEST_ASSERT_EQUAL_UINT8(0, r.error[0]);
  TEST_ASSERT_EQUAL_UINT8(1, r.error[1]);
  TEST_ASSERT_EQUAL_UINT8(2, r.error[2]);
  TEST_ASSERT_EQUAL_UINT8(3, r.error[3]);
}

static void every_error_pair_is_read_from_its_own_bits(void) {
  uint8_t buf[TEF668X_RDS_BYTES];
  rds(buf, DATA, 1, 2, 3, 4, 0xC000);
  Tef668xRdsRead r;
  tef668xDecodeRds(buf, &r);
  TEST_ASSERT_EQUAL_UINT8(3, r.error[0]);
  TEST_ASSERT_EQUAL_UINT8(0, r.error[1]);
  TEST_ASSERT_EQUAL_UINT8(0, r.error[3]);
  rds(buf, DATA, 1, 2, 3, 4, 0x0300);
  tef668xDecodeRds(buf, &r);
  TEST_ASSERT_EQUAL_UINT8(0, r.error[0]);
  TEST_ASSERT_EQUAL_UINT8(3, r.error[3]);
  /* The low byte carries no block. */
  rds(buf, DATA, 1, 2, 3, 4, 0x00FF);
  tef668xDecodeRds(buf, &r);
  for (int i = 0; i < 4; i++) {
    TEST_ASSERT_EQUAL_UINT8(0, r.error[i]);
  }
}

/* Nothing waiting: the decoder can still be locked, and the blocks are not
 * read, since the registers hold whatever came before. */
static void no_group_waiting_reads_no_blocks(void) {
  uint8_t buf[TEF668X_RDS_BYTES];
  rds(buf, SYNC, 0x1064, 0x0408, 0x2047, 0x4D41, 0xFFFF);
  Tef668xRdsRead r;
  memset(&r, 0xAA, sizeof(r));
  tef668xDecodeRds(buf, &r);
  TEST_ASSERT_TRUE(r.read);
  TEST_ASSERT_TRUE(r.synchronised);
  TEST_ASSERT_FALSE(r.haveGroup);
  TEST_ASSERT_EQUAL_HEX16(0, r.block[0]);
  TEST_ASSERT_EQUAL_UINT8(0, r.error[3]);
}

/* A PI only event carries block A and nothing else, so it is not a group. */
static void a_pi_only_event_is_not_a_group(void) {
  uint8_t buf[TEF668X_RDS_BYTES];
  rds(buf, DATA | PI_ONLY | SYNC, 0x1064, 0x1234, 0x5678, 0x9ABC, 0);
  Tef668xRdsRead r;
  tef668xDecodeRds(buf, &r);
  TEST_ASSERT_FALSE(r.haveGroup);
  TEST_ASSERT_EQUAL_HEX16(0, r.block[1]);
}

static void an_unlocked_decoder_says_so(void) {
  uint8_t buf[TEF668X_RDS_BYTES];
  rds(buf, 0, 0, 0, 0, 0, 0);
  Tef668xRdsRead r;
  tef668xDecodeRds(buf, &r);
  TEST_ASSERT_TRUE(r.read);
  TEST_ASSERT_FALSE(r.synchronised);
  TEST_ASSERT_FALSE(r.haveGroup);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(a_real_fm_reading_decodes_as_it_was_read);
  RUN_TEST(negative_level_offset_and_modulation_keep_their_sign);
  RUN_TEST(the_bandwidth_is_read_in_whole_kilohertz);
  RUN_TEST(an_am_reading_reads_co_channel_and_its_own_noise_scale);
  RUN_TEST(the_status_word_is_kept_whole);
  RUN_TEST(a_group_decodes_its_blocks_and_their_errors);
  RUN_TEST(every_error_pair_is_read_from_its_own_bits);
  RUN_TEST(no_group_waiting_reads_no_blocks);
  RUN_TEST(a_pi_only_event_is_not_a_group);
  RUN_TEST(an_unlocked_decoder_says_so);
  return UNITY_END();
}
