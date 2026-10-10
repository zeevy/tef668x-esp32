/*
 * What time it is, and how to write it down.
 *
 * The radio has no battery backed clock. This board has an RX8010SJ footprint
 * but no cell, so NTP over Wi-Fi is the only source of time, and it is not
 * guaranteed. The RDS clock time a station sends is only shown, and never
 * sets this clock. That is why every function here can answer "I do not
 * know": a clock that is wrong is worse than a clock that is
 * absent, because a wrong one still looks like a clock.
 *
 * No hardware and no network in here. The network side asks an NTP server and
 * hands the answer in; this decides what to do with it and what to print.
 */
#ifndef CORE_CLOCK_H
#define CORE_CLOCK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * How far from UTC a setting is allowed to be, in minutes.
 *
 * The real world runs from -12:00 (Baker Island) to +14:00 (Kiritimati), and
 * these are those two ends and nothing wider. A range wider than the world
 * lets a typo through as a legitimate setting, and an hour out is exactly the
 * kind of wrong that looks right.
 */
#define CLOCK_OFFSET_MIN_MINUTES (-720)
#define CLOCK_OFFSET_MAX_MINUTES (840)

/* Longest string clockFormat writes, including the terminator: "23:59". */
#define CLOCK_TEXT_LEN 6

/* Longest string clockFormatDate writes, with room to spare:
 * "WEDNESDAY, 30th September 2026" is 30 characters. */
#define CLOCK_DATE_LEN 32

/* A time of day, already in local time. */
typedef struct {
  uint8_t hour;   /* 0 to 23. */
  uint8_t minute; /* 0 to 59. */
  bool known;     /* False means no source has answered yet. */
} ClockTime;

/*
 * Turn a UTC time of day into a local one.
 *
 * `utcMinutes` is minutes since UTC midnight, which is what an NTP answer
 * reduces to once the date is dropped. `offsetMinutes` is the setting.
 *
 * Wraps at both ends, because applying +5:30 to 21:00 UTC lands on the next
 * day and applying -8:00 to 03:00 lands on the previous one. Getting this
 * wrong shows a time that is exactly right for a different day, which nobody
 * would spot.
 *
 * Returns a time with `known` false if `utcMinutes` is not a real time of day
 * or the offset is outside the range above.
 */
ClockTime clockLocal(int32_t utcMinutes, int16_t offsetMinutes);

/*
 * The local time of day at `epochUtc`, seconds since 1970 in UTC. The date
 * is dropped, which is all clockLocal needs. An offset of 0 gives UTC.
 * Unknown for an offset outside the range above.
 */
ClockTime clockFromEpoch(uint32_t epochUtc, int16_t offsetMinutes);

/* Where the day theme starts and stops, in local hours: 6 AM to 6 PM. */
#define CLOCK_DAY_FROM_HOUR 6
#define CLOCK_NIGHT_FROM_HOUR 18

/*
 * Whether `t` is day, 06:00 to 17:59, for the day and night themes. A time
 * that is not known, or not a real time, counts as day, so a radio that never
 * learns the time keeps its day theme rather than switching on a guess.
 */
bool clockIsDay(ClockTime t);

/*
 * Write a time as "HH:MM", zero padded, into a buffer of CLOCK_TEXT_LEN.
 *
 * Returns false and writes an empty string when the time is not known, so a
 * caller that ignores the result prints nothing rather than "00:00". Midnight
 * and no answer are different things and must not look the same.
 */
bool clockFormat(ClockTime t, char *out, size_t outLen);

/*
 * Write a date as "SATURDAY, 26th September 2026", in local time: the full
 * weekday in capitals, a comma, the day with its ordinal, the full month and
 * the year.
 *
 * `epochUtc` is seconds since 1970 in UTC, `offsetMinutes` the setting.
 * Returns false and writes an empty string when the offset is outside the
 * range above or the buffer is shorter than CLOCK_DATE_LEN.
 */
bool clockFormatDate(uint32_t epochUtc, int16_t offsetMinutes, char *out,
                     size_t outLen);

/* The two ways clockFormatWhen names a moment. */
typedef enum {
  CLOCK_WHEN_TAG, /* "17:08", "YDAY 21:10", "9 OCT 21:10": a label. */
  CLOCK_WHEN_ROW, /* "Today 17:08", "Yesterday 21:10", "9 Oct 21:10": a list. */
} ClockWhenStyle;

/* Room for any of them: "Yesterday 23:59" is 15 characters. */
#define CLOCK_WHEN_LEN 20

/*
 * Write when `epochUtc` was, seen from `nowUtc`, in local time: the time on
 * the same local day, yesterday's word and the time on the day before, and
 * otherwise the day, the month's short name and the time. A moment on a later
 * day than `nowUtc`, which only a clock that moved gives, is written as a
 * date. Returns false and writes an empty string when the offset is outside
 * the range above or the buffer is shorter than CLOCK_WHEN_LEN.
 */
bool clockFormatWhen(uint32_t epochUtc, uint32_t nowUtc, int16_t offsetMinutes,
                     ClockWhenStyle style, char *out, size_t outLen);

/*
 * Write an offset as "+05:30" or "-08:00", into a buffer of at least 7 bytes.
 *
 * Always signed, including "+00:00" for UTC, because an unsigned zero reads
 * as "not set" rather than as a choice.
 */
bool clockFormatOffset(int16_t offsetMinutes, char *out, size_t outLen);

/*
 * Read an offset written the way clockFormatOffset writes it.
 *
 * Accepts "+05:30", "-08:00" and "+0530", and a bare "0". Returns false and
 * leaves `out` alone on anything else, including an offset inside the string
 * format but outside the range a real place uses.
 */
bool clockParseOffset(const char *text, int16_t *out);

#ifdef __cplusplus
}
#endif

#endif /* CORE_CLOCK_H */
