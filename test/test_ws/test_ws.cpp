/* Tests for the WebSocket handshake and frames. Runs on a PC. */
#include <unity.h>

#include "core/ws.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* The worked example in RFC 6455, section 1.3. */
static const char kRfcKey[] = "dGhlIHNhbXBsZSBub25jZQ==";
static const uint8_t kRfcDigest[WS_DIGEST_LEN] = {
    0xb3, 0x7a, 0x4f, 0x2c, 0xc0, 0x62, 0x4f, 0x16, 0x90, 0xf6,
    0x46, 0x06, 0xcf, 0x38, 0x59, 0x45, 0xb2, 0xbe, 0xc4, 0xea};

static void the_rfc_example_hashes_the_key_and_the_guid(void) {
  char in[WS_HASH_INPUT_LEN + 1];
  TEST_ASSERT_TRUE(wsHashInput(kRfcKey, in, sizeof(in)));
  TEST_ASSERT_EQUAL_STRING(
      "dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11", in);
}

static void the_rfc_example_accepts_with_its_own_key(void) {
  char accept[WS_ACCEPT_LEN + 1];
  wsAcceptKey(kRfcDigest, accept);
  TEST_ASSERT_EQUAL_STRING("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", accept);
}

static void the_accept_key_uses_both_ends_of_the_alphabet(void) {
  uint8_t digest[WS_DIGEST_LEN];
  char accept[WS_ACCEPT_LEN + 1];
  memset(digest, 0x00, sizeof(digest));
  wsAcceptKey(digest, accept);
  TEST_ASSERT_EQUAL_STRING("AAAAAAAAAAAAAAAAAAAAAAAAAAA=", accept);
  memset(digest, 0xFF, sizeof(digest));
  wsAcceptKey(digest, accept);
  TEST_ASSERT_EQUAL_STRING("//////////////////////////8=", accept);
}

static void a_key_that_is_not_16_bytes_of_base64_is_refused(void) {
  char in[WS_HASH_INPUT_LEN + 1];
  TEST_ASSERT_FALSE(wsHashInput(NULL, in, sizeof(in)));
  TEST_ASSERT_FALSE(wsHashInput("", in, sizeof(in)));
  TEST_ASSERT_FALSE(wsHashInput("dGhlIHNhbXBsZSBub25jZQ=", in, sizeof(in)));
  TEST_ASSERT_FALSE(wsHashInput("dGhlIHNhbXBsZSBub25jZQ===", in, sizeof(in)));
  TEST_ASSERT_FALSE(wsHashInput("dGhlIHNhbXBsZSBub25jZQAA", in, sizeof(in)));
  TEST_ASSERT_FALSE(wsHashInput("dGhlIHNhbXBsZSBub25jZ!==", in, sizeof(in)));
  TEST_ASSERT_FALSE(wsHashInput("dGhlIHNhbXBsZSBub25j Q==", in, sizeof(in)));
}

static void the_text_must_fit_with_its_end(void) {
  char in[WS_HASH_INPUT_LEN + 1];
  TEST_ASSERT_FALSE(wsHashInput(kRfcKey, in, WS_HASH_INPUT_LEN));
  TEST_ASSERT_FALSE(wsHashInput(kRfcKey, NULL, sizeof(in)));
  TEST_ASSERT_TRUE(wsHashInput(kRfcKey, in, WS_HASH_INPUT_LEN + 1));
}

static void a_short_text_has_a_two_byte_header(void) {
  uint8_t h[WS_HEADER_MAX];
  TEST_ASSERT_EQUAL_UINT(2, wsTextHeader(0, h));
  TEST_ASSERT_EQUAL_HEX8(0x81, h[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, h[1]);
  TEST_ASSERT_EQUAL_UINT(2, wsTextHeader(125, h));
  TEST_ASSERT_EQUAL_HEX8(125, h[1]);
}

static void from_126_bytes_the_length_takes_two_more(void) {
  uint8_t h[WS_HEADER_MAX];
  TEST_ASSERT_EQUAL_UINT(4, wsTextHeader(126, h));
  const uint8_t at126[] = {0x81, 126, 0x00, 126};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(at126, h, 4);
  TEST_ASSERT_EQUAL_UINT(4, wsTextHeader(1826, h));
  const uint8_t state[] = {0x81, 126, 0x07, 0x22};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(state, h, 4);
  TEST_ASSERT_EQUAL_UINT(4, wsTextHeader(WS_TEXT_MAX, h));
  const uint8_t most[] = {0x81, 126, 0xFF, 0xFF};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(most, h, 4);
}

static void a_text_over_65535_bytes_is_not_framed(void) {
  uint8_t h[WS_HEADER_MAX];
  TEST_ASSERT_EQUAL_UINT(0, wsTextHeader(WS_TEXT_MAX + 1, h));
}

static void a_close_from_the_browser_is_told_from_the_rest(void) {
  const uint8_t close[] = {0x88, 0x82, 1, 2, 3, 4, 5, 6};
  const uint8_t text[] = {0x81, 0x80, 1, 2, 3, 4};
  const uint8_t ping[] = {0x89, 0x80, 1, 2, 3, 4};
  const uint8_t unfinished[] = {0x08};
  TEST_ASSERT_TRUE(wsIsClose(close, sizeof(close)));
  TEST_ASSERT_TRUE(wsIsClose(unfinished, sizeof(unfinished)));
  TEST_ASSERT_FALSE(wsIsClose(text, sizeof(text)));
  TEST_ASSERT_FALSE(wsIsClose(ping, sizeof(ping)));
  TEST_ASSERT_FALSE(wsIsClose(close, 0));
  TEST_ASSERT_FALSE(wsIsClose(NULL, 4));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(the_rfc_example_hashes_the_key_and_the_guid);
  RUN_TEST(the_rfc_example_accepts_with_its_own_key);
  RUN_TEST(the_accept_key_uses_both_ends_of_the_alphabet);
  RUN_TEST(a_key_that_is_not_16_bytes_of_base64_is_refused);
  RUN_TEST(the_text_must_fit_with_its_end);
  RUN_TEST(a_short_text_has_a_two_byte_header);
  RUN_TEST(from_126_bytes_the_length_takes_two_more);
  RUN_TEST(a_text_over_65535_bytes_is_not_framed);
  RUN_TEST(a_close_from_the_browser_is_told_from_the_rest);
  return UNITY_END();
}
