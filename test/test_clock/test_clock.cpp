/* Tests for the clock. Runs on a PC. */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/clock.h"

void setUp(void) {}
void tearDown(void) {}

/* ------------------------------------------------------- local from utc */

static void an_epoch_gives_its_time_of_day(void) {
  /* 27 September 2026, 01:46:00 UTC. The date has to be dropped: minutes
   * since 1970 are not a time of day. */
  ClockTime t = clockFromEpoch(1790473560u, 0);
  TEST_ASSERT_TRUE(t.known);
  TEST_ASSERT_EQUAL_UINT8(1, t.hour);
  TEST_ASSERT_EQUAL_UINT8(46, t.minute);

  t = clockFromEpoch(1790473560u, 330);
  TEST_ASSERT_TRUE(t.known);
  TEST_ASSERT_EQUAL_UINT8(7, t.hour);
  TEST_ASSERT_EQUAL_UINT8(16, t.minute);
}

static void an_epoch_on_either_side_of_midnight(void) {
  /* 28 September 2026, 00:00:00 UTC, and the second before it. */
  ClockTime t = clockFromEpoch(1790553600u, 0);
  TEST_ASSERT_TRUE(t.known);
  TEST_ASSERT_EQUAL_UINT8(0, t.hour);
  TEST_ASSERT_EQUAL_UINT8(0, t.minute);

  t = clockFromEpoch(1790553599u, 0);
  TEST_ASSERT_TRUE(t.known);
  TEST_ASSERT_EQUAL_UINT8(23, t.hour);
  TEST_ASSERT_EQUAL_UINT8(59, t.minute);
}

static void an_epoch_with_a_bad_offset_is_unknown(void) {
  TEST_ASSERT_FALSE(
      clockFromEpoch(1790473560u, CLOCK_OFFSET_MAX_MINUTES + 1).known);
}

static void utc_with_no_offset_is_utc(void) {
  ClockTime t = clockLocal(13 * 60 + 45, 0);
  TEST_ASSERT_TRUE(t.known);
  TEST_ASSERT_EQUAL_UINT8(13, t.hour);
  TEST_ASSERT_EQUAL_UINT8(45, t.minute);
}

static void a_half_hour_offset_lands_on_the_half_hour(void) {
  /* +05:30. 08:20 UTC is 13:50 there. */
  ClockTime t = clockLocal(8 * 60 + 20, 330);
  TEST_ASSERT_TRUE(t.known);
  TEST_ASSERT_EQUAL_UINT8(13, t.hour);
  TEST_ASSERT_EQUAL_UINT8(50, t.minute);
}

static void a_quarter_hour_offset_works_too(void) {
  /* Nepal, +05:45. */
  ClockTime t = clockLocal(0, 345);
  TEST_ASSERT_TRUE(t.known);
  TEST_ASSERT_EQUAL_UINT8(5, t.hour);
  TEST_ASSERT_EQUAL_UINT8(45, t.minute);
}

static void a_positive_offset_rolls_into_the_next_day(void) {
  /* 21:00 UTC plus 05:30 is 02:30, tomorrow. The hour is what matters here:
   * an implementation that does not wrap gives 26:30. */
  ClockTime t = clockLocal(21 * 60, 330);
  TEST_ASSERT_TRUE(t.known);
  TEST_ASSERT_EQUAL_UINT8(2, t.hour);
  TEST_ASSERT_EQUAL_UINT8(30, t.minute);
}

static void a_negative_offset_rolls_into_the_previous_day(void) {
  /* 03:00 UTC minus 08:00 is 19:00, yesterday. Without the wrap this is a
   * negative number of minutes and the hour comes out nonsense. */
  ClockTime t = clockLocal(3 * 60, -480);
  TEST_ASSERT_TRUE(t.known);
  TEST_ASSERT_EQUAL_UINT8(19, t.hour);
  TEST_ASSERT_EQUAL_UINT8(0, t.minute);
}

static void the_widest_offsets_either_way_still_wrap(void) {
  ClockTime east = clockLocal(23 * 60 + 59, CLOCK_OFFSET_MAX_MINUTES);
  TEST_ASSERT_TRUE(east.known);
  TEST_ASSERT_EQUAL_UINT8(13, east.hour);
  TEST_ASSERT_EQUAL_UINT8(59, east.minute);

  ClockTime west = clockLocal(0, CLOCK_OFFSET_MIN_MINUTES);
  TEST_ASSERT_TRUE(west.known);
  TEST_ASSERT_EQUAL_UINT8(12, west.hour);
  TEST_ASSERT_EQUAL_UINT8(0, west.minute);
}

static void midnight_and_the_last_minute_are_both_real_times(void) {
  ClockTime midnight = clockLocal(0, 0);
  TEST_ASSERT_TRUE(midnight.known);
  TEST_ASSERT_EQUAL_UINT8(0, midnight.hour);
  TEST_ASSERT_EQUAL_UINT8(0, midnight.minute);

  ClockTime last = clockLocal(24 * 60 - 1, 0);
  TEST_ASSERT_TRUE(last.known);
  TEST_ASSERT_EQUAL_UINT8(23, last.hour);
  TEST_ASSERT_EQUAL_UINT8(59, last.minute);
}

static void a_time_outside_the_day_is_not_a_time(void) {
  TEST_ASSERT_FALSE(clockLocal(-1, 0).known);
  TEST_ASSERT_FALSE(clockLocal(24 * 60, 0).known);
}

static void an_offset_no_place_uses_is_refused(void) {
  TEST_ASSERT_FALSE(clockLocal(0, CLOCK_OFFSET_MIN_MINUTES - 1).known);
  TEST_ASSERT_FALSE(clockLocal(0, CLOCK_OFFSET_MAX_MINUTES + 1).known);
}

/* ----------------------------------------------------------- formatting */

static void a_time_writes_as_hours_and_minutes(void) {
  char out[CLOCK_TEXT_LEN];
  TEST_ASSERT_TRUE(clockFormat(clockLocal(9 * 60 + 5, 0), out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("09:05", out);
}

static void an_unknown_time_writes_nothing_at_all(void) {
  /* The whole point. A blank is not "00:00", because a person reading
   * "00:00" believes the radio knows the time. */
  char out[CLOCK_TEXT_LEN];
  ClockTime nothing;
  memset(&nothing, 0, sizeof(nothing));
  TEST_ASSERT_FALSE(clockFormat(nothing, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
}

static void a_time_claiming_to_be_known_but_impossible_writes_nothing(void) {
  char out[CLOCK_TEXT_LEN];
  ClockTime bad;
  bad.hour = 24;
  bad.minute = 0;
  bad.known = true;
  TEST_ASSERT_FALSE(clockFormat(bad, out, sizeof(out)));
  bad.hour = 0;
  bad.minute = 60;
  TEST_ASSERT_FALSE(clockFormat(bad, out, sizeof(out)));
}

static void formatting_refuses_a_buffer_it_would_overrun(void) {
  char small[CLOCK_TEXT_LEN - 1];
  TEST_ASSERT_FALSE(clockFormat(clockLocal(0, 0), small, sizeof(small)));
  TEST_ASSERT_FALSE(clockFormat(clockLocal(0, 0), NULL, CLOCK_TEXT_LEN));
}

/* ---------------------------------------------------- offset formatting */

static void an_offset_always_carries_its_sign(void) {
  char out[8];
  TEST_ASSERT_TRUE(clockFormatOffset(330, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("+05:30", out);
  TEST_ASSERT_TRUE(clockFormatOffset(-480, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("-08:00", out);
}

static void utc_is_written_as_a_signed_zero(void) {
  /* "+00:00" rather than "00:00", so it reads as a choice somebody made
   * rather than as a field nobody filled in. */
  char out[8];
  TEST_ASSERT_TRUE(clockFormatOffset(0, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("+00:00", out);
}

static void an_offset_outside_the_world_writes_nothing(void) {
  char out[8];
  TEST_ASSERT_FALSE(
      clockFormatOffset(CLOCK_OFFSET_MAX_MINUTES + 1, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
  TEST_ASSERT_FALSE(
      clockFormatOffset(CLOCK_OFFSET_MIN_MINUTES - 1, out, sizeof(out)));
}

static void offset_formatting_refuses_a_short_buffer(void) {
  char small[6];
  TEST_ASSERT_FALSE(clockFormatOffset(0, small, sizeof(small)));
  TEST_ASSERT_FALSE(clockFormatOffset(0, NULL, 8));
}

/* ------------------------------------------------------- offset parsing */

static void it_reads_back_what_it_writes(void) {
  const int16_t cases[] = {0,
                           330,
                           345,
                           -480,
                           -210,
                           765,
                           600,
                           CLOCK_OFFSET_MIN_MINUTES,
                           CLOCK_OFFSET_MAX_MINUTES};
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    char text[8];
    TEST_ASSERT_TRUE(clockFormatOffset(cases[i], text, sizeof(text)));
    int16_t back = 1;
    TEST_ASSERT_TRUE(clockParseOffset(text, &back));
    TEST_ASSERT_EQUAL_INT16(cases[i], back);
  }
}

static void the_colon_is_optional(void) {
  int16_t out = 0;
  TEST_ASSERT_TRUE(clockParseOffset("+0530", &out));
  TEST_ASSERT_EQUAL_INT16(330, out);
}

static void a_missing_sign_means_east(void) {
  int16_t out = 0;
  TEST_ASSERT_TRUE(clockParseOffset("05:30", &out));
  TEST_ASSERT_EQUAL_INT16(330, out);
}

static void a_bare_zero_is_utc(void) {
  int16_t out = 99;
  TEST_ASSERT_TRUE(clockParseOffset("0", &out));
  TEST_ASSERT_EQUAL_INT16(0, out);
  TEST_ASSERT_TRUE(clockParseOffset("-0", &out));
  TEST_ASSERT_EQUAL_INT16(0, out);
}

static void a_minutes_field_of_sixty_or_more_is_a_typo(void) {
  /* 05:60 is not five hours and an hour. Accepting it would turn a slip into
   * a legitimate looking offset one hour out, which is the hardest kind of
   * wrong to notice on a clock. */
  int16_t out = 0;
  TEST_ASSERT_FALSE(clockParseOffset("+05:60", &out));
  TEST_ASSERT_FALSE(clockParseOffset("+05:99", &out));
}

static void an_offset_past_the_ends_of_the_world_is_refused(void) {
  int16_t out = 0;
  TEST_ASSERT_FALSE(clockParseOffset("+14:01", &out));
  TEST_ASSERT_FALSE(clockParseOffset("-12:01", &out));
  TEST_ASSERT_TRUE(clockParseOffset("+14:00", &out));
  TEST_ASSERT_EQUAL_INT16(840, out);
  TEST_ASSERT_TRUE(clockParseOffset("-12:00", &out));
  TEST_ASSERT_EQUAL_INT16(-720, out);
}

static void rubbish_is_refused_and_leaves_the_answer_alone(void) {
  int16_t out = 123;
  TEST_ASSERT_FALSE(clockParseOffset("", &out));
  TEST_ASSERT_FALSE(clockParseOffset("+", &out));
  TEST_ASSERT_FALSE(clockParseOffset("abc", &out));
  TEST_ASSERT_FALSE(clockParseOffset("+5:30", &out));
  TEST_ASSERT_FALSE(clockParseOffset("+05:3", &out));
  /* Half typed. */
  TEST_ASSERT_FALSE(clockParseOffset("+05", &out));
  TEST_ASSERT_FALSE(clockParseOffset("05", &out));
  TEST_ASSERT_FALSE(clockParseOffset("+05:", &out));
  TEST_ASSERT_FALSE(clockParseOffset("+05:300", &out));
  TEST_ASSERT_FALSE(clockParseOffset("+0a:30", &out));
  TEST_ASSERT_FALSE(clockParseOffset("+05:3a", &out));
  TEST_ASSERT_FALSE(clockParseOffset(NULL, &out));
  TEST_ASSERT_EQUAL_INT16(123, out);
  TEST_ASSERT_FALSE(clockParseOffset("+05:30", NULL));
}

/* ---------------------------------------------------------------- date */

static void assertDate(uint32_t epoch, int16_t offset, const char *want) {
  char out[CLOCK_DATE_LEN];
  TEST_ASSERT_TRUE(clockFormatDate(epoch, offset, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING(want, out);
}

static void a_date_is_written_in_full(void) {
  assertDate(1790402400u, 0, "SATURDAY, 26th September 2026");
}

static void the_offset_can_move_the_date_to_the_next_day(void) {
  /* 20:00 UTC on the 25th is 01:30 on the 26th at +05:30. */
  assertDate(1790366400u, 330, "SATURDAY, 26th September 2026");
  assertDate(1790366400u, 0, "FRIDAY, 25th September 2026");
}

/* Back a day from the first second of 1970, and forward from 23:00 UTC on
 * 31 December 2026 into the new year. */
static void the_offset_can_move_the_date_back_or_into_a_new_year(void) {
  assertDate(0u, -480, "WEDNESDAY, 31st December 1969");
  assertDate(1798758000u, 60, "FRIDAY, 1st January 2027");
}

static void a_leap_day_is_a_real_day(void) {
  assertDate(1709208000u, 0, "THURSDAY, 29th February 2024");
}

static void eleven_twelve_and_thirteen_take_th(void) {
  assertDate(1768132800u, 0, "SUNDAY, 11th January 2026");
  assertDate(1774180800u, 0, "SUNDAY, 22nd March 2026");
  assertDate(1774180800u + 86400u, 0, "MONDAY, 23rd March 2026");
  assertDate(1768132800u + 86400u, 0, "MONDAY, 12th January 2026");
  assertDate(1768132800u + 2u * 86400u, 0, "TUESDAY, 13th January 2026");
}

static void the_longest_date_fits_the_buffer(void) {
  /* 30 September 2026 is a Wednesday: the longest weekday and the longest
   * month, with a two digit day. */
  assertDate(1790769600u, 0, "WEDNESDAY, 30th September 2026");
  TEST_ASSERT_TRUE(strlen("WEDNESDAY, 30th September 2026") < CLOCK_DATE_LEN);
}

static void a_date_with_a_bad_offset_or_short_buffer_is_refused(void) {
  char out[CLOCK_DATE_LEN] = "x";
  TEST_ASSERT_FALSE(clockFormatDate(0u, 900, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
  out[0] = 'x';
  TEST_ASSERT_FALSE(clockFormatDate(0u, 0, out, 8));
  TEST_ASSERT_EQUAL_STRING("", out);
  TEST_ASSERT_FALSE(clockFormatDate(0u, 0, NULL, sizeof(out)));
}

/* 17:43 at +05:30 on Saturday 10 October 2026, the "now" of these tests. */
static const uint32_t kNow = 1791634380u;

static void assertWhen(uint32_t epoch, uint32_t now, int16_t offset,
                       ClockWhenStyle style, const char *want) {
  char out[CLOCK_WHEN_LEN];
  TEST_ASSERT_TRUE(
      clockFormatWhen(epoch, now, offset, style, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING(want, out);
}

static void a_moment_today_is_its_time(void) {
  assertWhen(1791632280u, kNow, 330, CLOCK_WHEN_TAG, "17:08");
  assertWhen(1791632280u, kNow, 330, CLOCK_WHEN_ROW, "Today 17:08");
}

static void a_moment_yesterday_says_so(void) {
  assertWhen(1791560400u, kNow, 330, CLOCK_WHEN_TAG, "YDAY 21:10");
  assertWhen(1791560400u, kNow, 330, CLOCK_WHEN_ROW, "Yesterday 21:10");
}

static void an_older_moment_gives_the_date(void) {
  /* 21:10 on the 9th, seen at 09:00 on the 11th. */
  assertWhen(1791560400u, 1791689400u, 330, CLOCK_WHEN_TAG, "9 OCT 21:10");
  assertWhen(1791560400u, 1791689400u, 330, CLOCK_WHEN_ROW, "9 Oct 21:10");
}

static void midnight_is_where_the_day_turns(void) {
  /* 23:59 on the 9th and 00:00 on the 10th, one minute apart. */
  assertWhen(1791570540u, kNow, 330, CLOCK_WHEN_TAG, "YDAY 23:59");
  assertWhen(1791570600u, kNow, 330, CLOCK_WHEN_TAG, "00:00");
}

static void the_offset_decides_the_day(void) {
  /* 20:00 UTC on the 9th is 01:30 on the 10th at +05:30, but still the 9th
   * in UTC, where "now" is 12:13 on the 10th. */
  assertWhen(1791576000u, kNow, 330, CLOCK_WHEN_TAG, "01:30");
  assertWhen(1791576000u, kNow, 0, CLOCK_WHEN_TAG, "YDAY 20:00");
}

static void a_moment_after_now_gives_the_date(void) {
  /* Only a clock that moved gives one: 08:00 on the 12th. */
  assertWhen(1791772200u, kNow, 330, CLOCK_WHEN_ROW, "12 Oct 08:00");
}

static void when_with_a_bad_offset_or_buffer_is_empty(void) {
  char out[CLOCK_WHEN_LEN];
  out[0] = 'x';
  TEST_ASSERT_FALSE(
      clockFormatWhen(kNow, kNow, 900, CLOCK_WHEN_TAG, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
  out[0] = 'x';
  TEST_ASSERT_FALSE(clockFormatWhen(kNow, kNow, 0, CLOCK_WHEN_TAG, out, 8));
  TEST_ASSERT_EQUAL_STRING("", out);
  TEST_ASSERT_FALSE(
      clockFormatWhen(kNow, kNow, 0, CLOCK_WHEN_TAG, NULL, sizeof(out)));
}

static ClockTime at(uint8_t hour, uint8_t minute) {
  ClockTime t = {hour, minute, true};
  return t;
}

/* Day runs from 06:00 up to 17:59, and night from 18:00 up to 05:59. */
static void day_is_six_in_the_morning_to_six_in_the_evening(void) {
  TEST_ASSERT_FALSE(clockIsDay(at(5, 59)));
  TEST_ASSERT_TRUE(clockIsDay(at(6, 0)));
  TEST_ASSERT_TRUE(clockIsDay(at(12, 30)));
  TEST_ASSERT_TRUE(clockIsDay(at(17, 59)));
  TEST_ASSERT_FALSE(clockIsDay(at(18, 0)));
  TEST_ASSERT_FALSE(clockIsDay(at(23, 59)));
  TEST_ASSERT_FALSE(clockIsDay(at(0, 0)));
}

/* With no time, or an impossible one, the radio keeps its day theme. */
static void a_time_not_known_counts_as_day(void) {
  ClockTime unknown = {20, 0, false};
  TEST_ASSERT_TRUE(clockIsDay(unknown));
  TEST_ASSERT_TRUE(clockIsDay(at(24, 0)));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(an_epoch_gives_its_time_of_day);
  RUN_TEST(an_epoch_on_either_side_of_midnight);
  RUN_TEST(a_moment_today_is_its_time);
  RUN_TEST(a_moment_yesterday_says_so);
  RUN_TEST(an_older_moment_gives_the_date);
  RUN_TEST(midnight_is_where_the_day_turns);
  RUN_TEST(the_offset_decides_the_day);
  RUN_TEST(a_moment_after_now_gives_the_date);
  RUN_TEST(when_with_a_bad_offset_or_buffer_is_empty);
  RUN_TEST(an_epoch_with_a_bad_offset_is_unknown);
  RUN_TEST(utc_with_no_offset_is_utc);
  RUN_TEST(a_half_hour_offset_lands_on_the_half_hour);
  RUN_TEST(a_quarter_hour_offset_works_too);
  RUN_TEST(a_positive_offset_rolls_into_the_next_day);
  RUN_TEST(a_negative_offset_rolls_into_the_previous_day);
  RUN_TEST(the_widest_offsets_either_way_still_wrap);
  RUN_TEST(midnight_and_the_last_minute_are_both_real_times);
  RUN_TEST(a_time_outside_the_day_is_not_a_time);
  RUN_TEST(an_offset_no_place_uses_is_refused);

  RUN_TEST(a_time_writes_as_hours_and_minutes);
  RUN_TEST(an_unknown_time_writes_nothing_at_all);
  RUN_TEST(a_time_claiming_to_be_known_but_impossible_writes_nothing);
  RUN_TEST(formatting_refuses_a_buffer_it_would_overrun);

  RUN_TEST(an_offset_always_carries_its_sign);
  RUN_TEST(utc_is_written_as_a_signed_zero);
  RUN_TEST(an_offset_outside_the_world_writes_nothing);
  RUN_TEST(offset_formatting_refuses_a_short_buffer);

  RUN_TEST(it_reads_back_what_it_writes);
  RUN_TEST(the_colon_is_optional);
  RUN_TEST(a_missing_sign_means_east);
  RUN_TEST(a_bare_zero_is_utc);
  RUN_TEST(a_minutes_field_of_sixty_or_more_is_a_typo);
  RUN_TEST(an_offset_past_the_ends_of_the_world_is_refused);
  RUN_TEST(rubbish_is_refused_and_leaves_the_answer_alone);
  RUN_TEST(a_date_is_written_in_full);
  RUN_TEST(the_offset_can_move_the_date_to_the_next_day);
  RUN_TEST(the_offset_can_move_the_date_back_or_into_a_new_year);
  RUN_TEST(a_leap_day_is_a_real_day);
  RUN_TEST(eleven_twelve_and_thirteen_take_th);
  RUN_TEST(the_longest_date_fits_the_buffer);
  RUN_TEST(a_date_with_a_bad_offset_or_short_buffer_is_refused);
  RUN_TEST(day_is_six_in_the_morning_to_six_in_the_evening);
  RUN_TEST(a_time_not_known_counts_as_day);

  return UNITY_END();
}
