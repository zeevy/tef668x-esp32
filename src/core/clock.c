/* What time it is. No hardware, no network. */
#include "clock.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "core/strings.h"

#define MINUTES_PER_DAY (24 * 60)

ClockTime clockLocal(int32_t utcMinutes, int16_t offsetMinutes) {
  ClockTime t;
  t.hour = 0;
  t.minute = 0;
  t.known = false;

  if (utcMinutes < 0 || utcMinutes >= MINUTES_PER_DAY) {
    return t;
  }
  if (offsetMinutes < CLOCK_OFFSET_MIN_MINUTES ||
      offsetMinutes > CLOCK_OFFSET_MAX_MINUTES) {
    return t;
  }

  /* The offset can carry the time past either end of the day. Adding a whole
   * day before the modulo keeps the operand positive, because C's % on a
   * negative left hand side gives a negative answer and that would land on an
   * hour of -3. The offset cannot exceed a day, so one is enough. */
  int32_t local =
      (utcMinutes + offsetMinutes + MINUTES_PER_DAY) % MINUTES_PER_DAY;

  t.hour = (uint8_t)(local / 60);
  t.minute = (uint8_t)(local % 60);
  t.known = true;
  return t;
}

ClockTime clockFromEpoch(uint32_t epochUtc, int16_t offsetMinutes) {
  return clockLocal((int32_t)((epochUtc / 60u) % MINUTES_PER_DAY),
                    offsetMinutes);
}

bool clockFormat(ClockTime t, char *out, size_t outLen) {
  if (out == NULL || outLen < CLOCK_TEXT_LEN) {
    return false;
  }
  if (!t.known || t.hour > 23 || t.minute > 59) {
    out[0] = '\0';
    return false;
  }
  snprintf(out, outLen, txt(STR_COMMON_FMT_CLOCK), (unsigned)t.hour,
           (unsigned)t.minute);
  return true;
}

bool clockFormatDate(uint32_t epochUtc, int16_t offsetMinutes, char *out,
                     size_t outLen) {
  static const StrId kDays[7] = {
      STR_DAY_SUNDAY,   STR_DAY_MONDAY, STR_DAY_TUESDAY, STR_DAY_WEDNESDAY,
      STR_DAY_THURSDAY, STR_DAY_FRIDAY, STR_DAY_SATURDAY};
  static const StrId kMonths[12] = {
      STR_MONTH_JANUARY, STR_MONTH_FEBRUARY, STR_MONTH_MARCH,
      STR_MONTH_APRIL,   STR_MONTH_MAY,      STR_MONTH_JUNE,
      STR_MONTH_JULY,    STR_MONTH_AUGUST,   STR_MONTH_SEPTEMBER,
      STR_MONTH_OCTOBER, STR_MONTH_NOVEMBER, STR_MONTH_DECEMBER};
  if (out == NULL) {
    return false;
  }
  if (outLen < CLOCK_DATE_LEN || offsetMinutes < CLOCK_OFFSET_MIN_MINUTES ||
      offsetMinutes > CLOCK_OFFSET_MAX_MINUTES) {
    if (outLen > 0) {
      out[0] = '\0';
    }
    return false;
  }
  /* Local time is UTC moved by the offset, so gmtime_r on the moved count
   * gives the local date. */
  const time_t local = (time_t)epochUtc + (time_t)offsetMinutes * 60;
  struct tm tm;
  if (gmtime_r(&local, &tm) == NULL) {
    out[0] = '\0';
    return false;
  }
  const int day = tm.tm_mday;

  /* 11th, 12th and 13th, not 11st, 12nd and 13rd. */
  StrId suffix = STR_DATE_SUFFIX_TH;
  if (day < 11 || day > 13) {
    suffix = day % 10 == 1   ? STR_DATE_SUFFIX_ST
             : day % 10 == 2 ? STR_DATE_SUFFIX_ND
             : day % 10 == 3 ? STR_DATE_SUFFIX_RD
                             : STR_DATE_SUFFIX_TH;
  }
  snprintf(out, outLen, txt(STR_DATE_FMT_LINE), txt(kDays[tm.tm_wday]), day,
           txt(suffix), txt(kMonths[tm.tm_mon]), tm.tm_year + 1900);
  return true;
}

bool clockFormatOffset(int16_t offsetMinutes, char *out, size_t outLen) {
  if (out == NULL || outLen < 7) {
    return false;
  }
  if (offsetMinutes < CLOCK_OFFSET_MIN_MINUTES ||
      offsetMinutes > CLOCK_OFFSET_MAX_MINUTES) {
    out[0] = '\0';
    return false;
  }
  const char sign = offsetMinutes < 0 ? '-' : '+';
  int32_t magnitude =
      offsetMinutes < 0 ? -(int32_t)offsetMinutes : offsetMinutes;
  snprintf(out, outLen, "%c%02u:%02u", sign, (unsigned)(magnitude / 60),
           (unsigned)(magnitude % 60));
  return true;
}

/* One digit, or -1. Written out rather than using isdigit, which is locale
 * aware and would accept whatever the C library thinks a digit is. */
static int digit(char c) {
  return (c >= '0' && c <= '9') ? (c - '0') : -1;
}

bool clockParseOffset(const char *text, int16_t *out) {
  if (text == NULL || out == NULL) {
    return false;
  }

  int sign = 1;
  const char *p = text;
  if (*p == '+') {
    p++;
  } else if (*p == '-') {
    sign = -1;
    p++;
  }

  /* A bare "0" is how a person writes UTC, and refusing it in favour of
   * "+00:00" would be pedantry in a text box. */
  if (p[0] == '0' && p[1] == '\0') {
    *out = 0;
    return true;
  }

  const int h1 = digit(p[0]);
  const int h2 = digit(p[1]);
  if (h1 < 0 || h2 < 0) {
    return false;
  }
  p += 2;
  if (*p == ':') {
    p++;
  }
  /* One at a time, and each checked before the next is read. Reading both
   * first walks off the end of the string on a half typed offset like "+05",
   * where p[0] is the terminator and p[1] is past the buffer. */
  const int m1 = digit(p[0]);
  if (m1 < 0) {
    return false;
  }
  const int m2 = digit(p[1]);
  if (m2 < 0 || p[2] != '\0') {
    return false;
  }

  const int32_t minutes = (h1 * 10 + h2) * 60 + (m1 * 10 + m2);
  /* A minutes field of 60 or more is a typo, not an offset. It would
   * otherwise pass as a legitimate hour and a bit. */
  if (m1 * 10 + m2 > 59) {
    return false;
  }
  const int32_t signed_minutes = sign * minutes;
  if (signed_minutes < CLOCK_OFFSET_MIN_MINUTES ||
      signed_minutes > CLOCK_OFFSET_MAX_MINUTES) {
    return false;
  }
  *out = (int16_t)signed_minutes;
  return true;
}

bool clockIsDay(ClockTime t) {
  if (!t.known || t.hour > 23) {
    return true;
  }
  return t.hour >= CLOCK_DAY_FROM_HOUR && t.hour < CLOCK_NIGHT_FROM_HOUR;
}
