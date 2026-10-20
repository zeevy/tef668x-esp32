/*
 * Tests for the crash record. Runs on a PC.
 *
 * What matters is that only a record the panic handler sealed is believed,
 * that the JSON names what it holds, and that a text from the chip cannot
 * break the JSON around it.
 */
#include <string.h>
#include <unity.h>

#include "core/panic_record.h"

void setUp(void) {}
void tearDown(void) {}

static PanicRecord filled(void) {
  PanicRecord r;
  memset(&r, 0, sizeof(r));
  r.pc = 0x400d1234u;
  r.frames[0] = 0x400d1234u;
  r.frames[1] = 0x400d5678u;
  r.frameCount = 2;
  r.core = 1;
  r.uptimeS = 3600;
  strcpy(r.why, "LoadProhibited");
  strcpy(r.build, "0586e2f");
  return r;
}

static void a_sealed_record_is_believed(void) {
  PanicRecord r = filled();
  TEST_ASSERT_FALSE(panicRecordValid(&r));
  panicRecordSeal(&r);
  TEST_ASSERT_TRUE(panicRecordValid(&r));
}

/* RTC memory after a power on: noise, which must not read as a crash. */
static void noise_and_a_change_are_not(void) {
  PanicRecord r;
  memset(&r, 0xA5, sizeof(r));
  TEST_ASSERT_FALSE(panicRecordValid(&r));
  memset(&r, 0xFF, sizeof(r));
  TEST_ASSERT_FALSE(panicRecordValid(&r));
  r = filled();
  panicRecordSeal(&r);
  r.pc ^= 4;
  TEST_ASSERT_FALSE(panicRecordValid(&r));
  r = filled();
  panicRecordSeal(&r);
  panicRecordClear(&r);
  TEST_ASSERT_FALSE(panicRecordValid(&r));
  TEST_ASSERT_FALSE(panicRecordValid(NULL));
}

/* Texts with no end and too many frames are ended and capped by the seal. */
static void the_seal_ends_texts_and_caps_frames(void) {
  PanicRecord r = filled();
  memset(r.why, 'x', sizeof(r.why));
  memset(r.build, 'y', sizeof(r.build));
  r.frameCount = 200;
  panicRecordSeal(&r);
  TEST_ASSERT_TRUE(panicRecordValid(&r));
  TEST_ASSERT_EQUAL_UINT8(PANIC_RECORD_FRAMES, r.frameCount);
  TEST_ASSERT_EQUAL_size_t(PANIC_RECORD_WHY_LEN - 1, strlen(r.why));
  panicRecordSeal(NULL);
  panicRecordClear(NULL);
}

static void the_json_names_what_it_holds(void) {
  PanicRecord r = filled();
  panicRecordSeal(&r);
  char out[PANIC_RECORD_JSON_LEN];
  const size_t n = panicRecordJson(&r, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING(
      "{\"why\":\"LoadProhibited\",\"pc\":\"0x400d1234\","
      "\"bt\":[\"0x400d1234\",\"0x400d5678\"],"
      "\"core\":1,\"up\":3600,\"bld\":\"0586e2f\"}",
      out);
  TEST_ASSERT_EQUAL_size_t(strlen(out), n);
}

/* A quote, a backslash, a line break or a byte past ASCII in the chip's
 * words cannot break the JSON. */
static void the_json_keeps_a_strange_text_safe(void) {
  PanicRecord r = filled();
  strcpy(r.why, "abort() \"at\" \\x\n\xC3\xA9");
  r.frameCount = 0;
  panicRecordSeal(&r);
  char out[PANIC_RECORD_JSON_LEN];
  panicRecordJson(&r, out, sizeof(out));
  TEST_ASSERT_NOT_NULL(
      strstr(out, "\"why\":\"abort() \\\"at\\\" \\\\x\",\"pc\""));
  TEST_ASSERT_NOT_NULL(strstr(out, "\"bt\":[]"));
}

/* Every field full still fits the room the header names. */
static void the_longest_record_fits(void) {
  PanicRecord r = filled();
  memset(r.why, '"', sizeof(r.why) - 1);
  r.why[sizeof(r.why) - 1] = '\0';
  memset(r.build, 'f', sizeof(r.build) - 1);
  r.build[sizeof(r.build) - 1] = '\0';
  r.frameCount = PANIC_RECORD_FRAMES;
  r.uptimeS = 0xFFFFFFFFu;
  panicRecordSeal(&r);
  char out[PANIC_RECORD_JSON_LEN];
  const size_t n = panicRecordJson(&r, out, sizeof(out));
  TEST_ASSERT_TRUE(n > 0 && n < sizeof(out));
  TEST_ASSERT_EQUAL_CHAR('}', out[n - 1]);
}

static void no_record_or_no_room_says_so(void) {
  char out[PANIC_RECORD_JSON_LEN];
  TEST_ASSERT_EQUAL_size_t(4, panicRecordJson(NULL, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("null", out);
  char small[4];
  TEST_ASSERT_EQUAL_size_t(0, panicRecordJson(NULL, small, sizeof(small)));
  TEST_ASSERT_EQUAL_STRING("", small);
  PanicRecord r = filled();
  panicRecordSeal(&r);
  char short_[64];
  TEST_ASSERT_EQUAL_size_t(0, panicRecordJson(&r, short_, sizeof(short_)));
  TEST_ASSERT_EQUAL_STRING("", short_);
  TEST_ASSERT_EQUAL_size_t(0, panicRecordJson(&r, NULL, 10));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(a_sealed_record_is_believed);
  RUN_TEST(noise_and_a_change_are_not);
  RUN_TEST(the_seal_ends_texts_and_caps_frames);
  RUN_TEST(the_json_names_what_it_holds);
  RUN_TEST(the_json_keeps_a_strange_text_safe);
  RUN_TEST(the_longest_record_fits);
  RUN_TEST(no_record_or_no_room_says_so);
  return UNITY_END();
}
