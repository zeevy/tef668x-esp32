/* Tests for the web server's escapes and its number reader. Runs on a PC. */
#include <unity.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "core/web_text.h"

void setUp(void) {}
void tearDown(void) {}

/* A whole string through one of the escapes, the way the web server uses
 * them, one character after another. */
static void escapeAll(size_t (*escape)(char, char *), const char *raw,
                      char *out, size_t cap) {
  size_t used = 0;
  out[0] = '\0';
  for (const char *p = raw; *p != '\0'; p++) {
    char piece[WEB_ESCAPE_MAX];
    const size_t n = escape(*p, piece);
    TEST_ASSERT_EQUAL_size_t(strlen(piece), n);
    TEST_ASSERT_TRUE(used + n < cap);
    memcpy(out + used, piece, n + 1);
    used += n;
  }
}

/* ------------------------------------------------------------------ JSON */

static void plain_text_passes_through_json_as_it_is(void) {
  char out[64];
  escapeAll(webJsonEscapeChar, "MAGIC 106.4 FM", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("MAGIC 106.4 FM", out);
}

static void a_quote_and_a_backslash_get_a_backslash_in_json(void) {
  char out[64];
  escapeAll(webJsonEscapeChar, "say \"hi\" \\o/", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("say \\\"hi\\\" \\\\o/", out);
}

static void control_characters_come_out_as_unicode_escapes(void) {
  char out[64];
  escapeAll(webJsonEscapeChar, "a\nb\tc\x01\x1f", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("a\\u000ab\\u0009c\\u0001\\u001f", out);
}

/* The panel's own texts carry UTF-8, its icons and "dBuV" with a micro
 * sign, and the bytes have to reach the API whole for the text to read. */
static void utf8_bytes_pass_through_json_whole(void) {
  char out[64];
  escapeAll(webJsonEscapeChar, "dB\xc2\xb5V \xee\xa0\xb7", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("dB\xc2\xb5V \xee\xa0\xb7", out);
}

/* The space and the delete character are not control characters JSON
 * needs escaped. */
static void the_edges_of_the_control_range_in_json(void) {
  char piece[WEB_ESCAPE_MAX];
  TEST_ASSERT_EQUAL_size_t(1, webJsonEscapeChar(' ', piece));
  TEST_ASSERT_EQUAL_STRING(" ", piece);
  TEST_ASSERT_EQUAL_size_t(1, webJsonEscapeChar('\x7f', piece));
  TEST_ASSERT_EQUAL_size_t(6, webJsonEscapeChar('\x1f', piece));
  TEST_ASSERT_EQUAL_STRING("\\u001f", piece);
}

/* ------------------------------------------------------------------ HTML */

static void plain_text_passes_through_html_as_it_is(void) {
  char out[64];
  escapeAll(webHtmlEscapeChar, "Radio Mirchi 98.3", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("Radio Mirchi 98.3", out);
}

static void the_five_html_characters_become_entities(void) {
  char out[96];
  escapeAll(webHtmlEscapeChar, "<b>R&B</b> \"Tom's\"", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("&lt;b&gt;R&amp;B&lt;/b&gt; &quot;Tom&#39;s&quot;",
                           out);
}

/* A station name that tries to close the attribute it sits in stays text. */
static void a_name_cannot_break_out_of_an_attribute(void) {
  char out[96];
  escapeAll(webHtmlEscapeChar, "x' onmouseover='alert(1)", out, sizeof(out));
  TEST_ASSERT_NULL(strchr(out, '\''));
  TEST_ASSERT_NULL(strchr(out, '<'));
}

static void the_longest_escapes_fit_the_buffer(void) {
  char piece[WEB_ESCAPE_MAX];
  TEST_ASSERT_EQUAL_size_t(6, webHtmlEscapeChar('"', piece));
  TEST_ASSERT_EQUAL_size_t(WEB_ESCAPE_MAX - 1, strlen(piece));
  TEST_ASSERT_EQUAL_size_t(6, webJsonEscapeChar('\x01', piece));
  TEST_ASSERT_EQUAL_size_t(WEB_ESCAPE_MAX - 1, strlen(piece));
}

/* --------------------------------------------------------------- numbers */

static void a_whole_number_in_range_is_read(void) {
  long v = 0;
  TEST_ASSERT_EQUAL(WEB_NUMBER_OK, webParseNumber("106400", 1, 200000, &v));
  TEST_ASSERT_EQUAL(106400, v);
  TEST_ASSERT_EQUAL(WEB_NUMBER_OK, webParseNumber("-20", -20, 20, &v));
  TEST_ASSERT_EQUAL(-20, v);
  TEST_ASSERT_EQUAL(WEB_NUMBER_OK, webParseNumber("20", -20, 20, &v));
  TEST_ASSERT_EQUAL(20, v);
}

/* What strtol takes, so a request that worked before still works. */
static void a_sign_and_leading_spaces_are_taken(void) {
  long v = 0;
  TEST_ASSERT_EQUAL(WEB_NUMBER_OK, webParseNumber("+5", 0, 9, &v));
  TEST_ASSERT_EQUAL(5, v);
  TEST_ASSERT_EQUAL(WEB_NUMBER_OK, webParseNumber(" 7", 0, 9, &v));
  TEST_ASSERT_EQUAL(7, v);
}

static void anything_that_is_not_a_whole_number_is_refused(void) {
  long v = 99;
  const char *bad[] = {"", " ", "-", "abc", "12a", "1.5", "5 ", "0x10", NULL};
  for (int i = 0; bad[i] != NULL; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(WEB_NUMBER_NOT_WHOLE,
                              webParseNumber(bad[i], 0, 100, &v), bad[i]);
  }
  TEST_ASSERT_EQUAL(WEB_NUMBER_NOT_WHOLE, webParseNumber(NULL, 0, 100, &v));
  /* Nothing is written on a refusal. */
  TEST_ASSERT_EQUAL(99, v);
}

static void a_number_outside_the_range_is_refused(void) {
  long v = 99;
  TEST_ASSERT_EQUAL(WEB_NUMBER_OUT_OF_RANGE, webParseNumber("21", -20, 20, &v));
  TEST_ASSERT_EQUAL(WEB_NUMBER_OUT_OF_RANGE,
                    webParseNumber("-21", -20, 20, &v));
  TEST_ASSERT_EQUAL(99, v);
}

/* Read as the largest long, a number this big would pass a range that
 * ends at the largest long. */
static void a_number_too_big_for_a_long_is_out_of_range(void) {
  long v = 99;
  TEST_ASSERT_EQUAL(
      WEB_NUMBER_OUT_OF_RANGE,
      webParseNumber("99999999999999999999999", LONG_MIN, LONG_MAX, &v));
  TEST_ASSERT_EQUAL(
      WEB_NUMBER_OUT_OF_RANGE,
      webParseNumber("-99999999999999999999999", LONG_MIN, LONG_MAX, &v));
  TEST_ASSERT_EQUAL(99, v);
  char max[32];
  snprintf(max, sizeof(max), "%ld", LONG_MAX);
  TEST_ASSERT_EQUAL(WEB_NUMBER_OK, webParseNumber(max, LONG_MIN, LONG_MAX, &v));
  TEST_ASSERT_EQUAL(LONG_MAX, v);
}

static void no_place_for_the_answer_still_says_what_it_read(void) {
  TEST_ASSERT_EQUAL(WEB_NUMBER_OK, webParseNumber("3", 0, 9, NULL));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(plain_text_passes_through_json_as_it_is);
  RUN_TEST(a_quote_and_a_backslash_get_a_backslash_in_json);
  RUN_TEST(control_characters_come_out_as_unicode_escapes);
  RUN_TEST(utf8_bytes_pass_through_json_whole);
  RUN_TEST(the_edges_of_the_control_range_in_json);
  RUN_TEST(plain_text_passes_through_html_as_it_is);
  RUN_TEST(the_five_html_characters_become_entities);
  RUN_TEST(a_name_cannot_break_out_of_an_attribute);
  RUN_TEST(the_longest_escapes_fit_the_buffer);
  RUN_TEST(a_whole_number_in_range_is_read);
  RUN_TEST(a_sign_and_leading_spaces_are_taken);
  RUN_TEST(anything_that_is_not_a_whole_number_is_refused);
  RUN_TEST(a_number_outside_the_range_is_refused);
  RUN_TEST(a_number_too_big_for_a_long_is_out_of_range);
  RUN_TEST(no_place_for_the_answer_still_says_what_it_read);
  return UNITY_END();
}
