/* Implementation of the channel list as text. */
#include "memory_csv.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "csv.h"

/* The header line this writes and accepts back, and the two written before,
 * without the programme type and the time and without the PI, accepted too
 * so an older file still reads. `ptn`, the programme type's name, and
 * `country`, from the PI and the ECC, are for reading the file: an import
 * passes over them, since `pty` and `ecc` say the same. */
static const char kHeader[] =
    "slot,band,frequency,bandwidth,name,pi,pty,ptn,ecc,country,saved\n";
static const char kHeaderV2[] = "slot,band,frequency,bandwidth,name,pi\n";
static const char kHeaderNoPi[] = "slot,band,frequency,bandwidth,name\n";

const char *memoryCsvHeader(void) {
  return kHeader;
}

size_t memoryCsvLine(const MemoryStore *m, int slot, RdsRegion region,
                     char *out, size_t cap) {
  const MemoryChannel *c = memoryGet(m, slot);
  if (c == NULL || out == NULL) {
    return 0;
  }
  /* At most 16 characters as well as up to the terminator, because that is
   * the longest name the field can hold and still end inside itself.
   * memoryChannelValid is what guarantees the terminator is there, run on
   * every write and over everything read back from flash, so this bound is
   * the second answer rather than the first. Sized for every character a
   * doubled quote, inside the two that wrap the field. */
  char name[2 + (MEMORY_NAME_LEN - 1) * 2 + 1];
  csvQuote(c->name, MEMORY_NAME_LEN - 1, name, sizeof(name));
  /* The PI as four hex digits, or nothing when none is known. */
  char pi[5] = "";
  if (c->pi != 0) {
    snprintf(pi, sizeof(pi), "%04X", (unsigned)c->pi);
  }
  /* The programme type as its number and its name, the ECC as two hex
   * digits and the country they name with the PI, each empty for none. */
  char pty[4] = "";
  const char *ptn = "";
  if (c->pty != 0) {
    snprintf(pty, sizeof(pty), "%u", (unsigned)c->pty);
    ptn = rdsPtyNameEnglish(c->pty, region);
  }
  char ecc[3] = "";
  const char *country = NULL;
  if (c->ecc != 0) {
    snprintf(ecc, sizeof(ecc), "%02X", (unsigned)c->ecc);
    country = c->pi != 0 ? rdsCountryCode(c->pi, c->ecc) : NULL;
  }
  /* When it was saved, in UTC, as the station log writes a time. */
  char saved[20] = "";
  if (c->savedUtc != 0) {
    const time_t t = (time_t)c->savedUtc;
    struct tm tm;
    if (gmtime_r(&t, &tm) == NULL ||
        strftime(saved, sizeof(saved), "%Y-%m-%d %H:%M", &tm) == 0) {
      saved[0] = '\0';
    }
  }
  /* The slot counted from 1, the way it is shown on the panel and typed on
   * the keypad. A file whose first channel is called 0 would be read back
   * onto the wrong slot by anybody editing it by hand. */
  const int n =
      snprintf(out, cap, "%d,%s,%lu,%u,%s,%s,%s,%s,%s,%s,%s\n", slot + 1,
               bandName((BandId)c->band), (unsigned long)c->freqKHz,
               (unsigned)c->bandwidthKHz, name, pi, pty, ptn, ecc,
               country != NULL ? country : "", saved);
  return n < 0 ? 0 : (size_t)n;
}

static bool readField(const char **at, const char *end, char *out, size_t cap,
                      bool *cut) {
  const char *p = *at;
  size_t n = 0;
  if (p < end && *p == '"') {
    p++;
    for (;;) {
      if (p >= end) {
        return false;
      }
      if (*p == '"') {
        if (p + 1 < end && p[1] == '"') {
          p += 2;
        } else {
          p++;
          break;
        }
      } else {
        p++;
      }
      if (n + 1 < cap) {
        out[n++] = p[-1];
      } else {
        *cut = true;
      }
    }
    if (p < end && *p != ',') {
      return false;
    }
  } else {
    while (p < end && *p != ',') {
      if (n + 1 < cap) {
        out[n++] = *p;
      } else {
        *cut = true;
      }
      p++;
    }
  }
  out[n] = '\0';
  if (p < end && *p == ',') {
    p++;
  }
  *at = p;
  return true;
}

static bool wholeNumber(const char *text, uint32_t *out) {
  if (text[0] == '\0') {
    return false;
  }
  uint32_t value = 0;
  for (size_t i = 0; text[i] != '\0'; i++) {
    if (text[i] < '0' || text[i] > '9') {
      return false;
    }
    /* Refused rather than wrapped. A frequency that wrapped would land on a
     * real channel somewhere else in the band with nothing to say it had. */
    if (value > (UINT32_MAX - (uint32_t)(text[i] - '0')) / 10u) {
      return false;
    }
    value = value * 10u + (uint32_t)(text[i] - '0');
  }
  *out = value;
  return true;
}

static bool bandFromName(const char *text, uint8_t *out) {
  for (int b = 0; b < BAND_COUNT; b++) {
    const char *name = bandName((BandId)b);
    size_t i = 0;
    for (; name[i] != '\0' && text[i] != '\0'; i++) {
      char a = text[i];
      if (a >= 'a' && a <= 'z') {
        a = (char)(a - 'a' + 'A');
      }
      if (a != name[i]) {
        break;
      }
    }
    if (name[i] == '\0' && text[i] == '\0') {
      *out = (uint8_t)b;
      return true;
    }
  }
  return false;
}

/* Whether `line` is `header`, its newline left off, in any case. */
static bool isHeader(const char *line, size_t len, const char *header) {
  size_t headerLen = strlen(header) - 1;
  if (len != headerLen) {
    return false;
  }
  for (size_t i = 0; i < headerLen; i++) {
    char a = line[i];
    if (a >= 'A' && a <= 'Z') {
      a = (char)(a - 'A' + 'a');
    }
    if (a != header[i]) {
      return false;
    }
  }
  return true;
}

/* Exactly `digits` hex digits in either case, or an empty field for none.
 * False for anything else, and for all zeros: PI 0000 names no station and
 * ECC 00 no country. */
static bool hexField(const char *text, int digits, uint16_t *out) {
  if (text[0] == '\0') {
    *out = 0;
    return true;
  }
  uint16_t value = 0;
  for (int i = 0; i < digits; i++) {
    char a = text[i];
    uint16_t digit;
    if (a >= '0' && a <= '9') {
      digit = (uint16_t)(a - '0');
    } else if (a >= 'A' && a <= 'F') {
      digit = (uint16_t)(a - 'A' + 10);
    } else if (a >= 'a' && a <= 'f') {
      digit = (uint16_t)(a - 'a' + 10);
    } else {
      return false;
    }
    value = (uint16_t)(value * 16u + digit);
  }
  if (text[digits] != '\0' || value == 0) {
    return false;
  }
  *out = value;
  return true;
}

static bool isSkippable(const char *line, size_t len) {
  if (len == 0) {
    return true;
  }
  if (line[0] == '#') {
    return true;
  }
  /* The header this writes, or the one written before the PI column, in
   * any case, and nothing else that merely fails to be a number. A line
   * that is not a header and not a channel is an error worth reporting, not
   * something to pass over. */
  return isHeader(line, len, kHeader) || isHeader(line, len, kHeaderV2) ||
         isHeader(line, len, kHeaderNoPi);
}

/* A whole number of exactly `digits` digits from `text`. */
static bool digitsAt(const char *text, int digits, int *out) {
  int v = 0;
  for (int i = 0; i < digits; i++) {
    if (text[i] < '0' || text[i] > '9') {
      return false;
    }
    v = v * 10 + (text[i] - '0');
  }
  *out = v;
  return true;
}

/* Days from 1970-01-01 to the date, by the proleptic Gregorian calendar. */
static int64_t daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const int64_t yoe = y - era * 400;
  const int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

/* "2026-10-12 21:10" in UTC, as memoryCsvLine writes it, or an empty field
 * for none. False for anything else, a day the month does not have, and a
 * date before 1971 or after 2105, which no clock set from the network
 * gives. */
static bool savedField(const char *text, uint32_t *out) {
  if (text[0] == '\0') {
    *out = 0;
    return true;
  }
  int y, mo, d, h, mi;
  if (strlen(text) != 16 || text[4] != '-' || text[7] != '-' ||
      text[10] != ' ' || text[13] != ':' || !digitsAt(text, 4, &y) ||
      !digitsAt(text + 5, 2, &mo) || !digitsAt(text + 8, 2, &d) ||
      !digitsAt(text + 11, 2, &h) || !digitsAt(text + 14, 2, &mi) || y < 1971 ||
      y > 2105 || mo < 1 || mo > 12 || d < 1 || d > 31 || h > 23 || mi > 59) {
    return false;
  }
  /* A day past the end of its month, 31 June, would land in the next. */
  const time_t t =
      (time_t)(daysFromCivil(y, mo, d) * 86400 + h * 3600 + mi * 60);
  struct tm tm;
  if (gmtime_r(&t, &tm) == NULL || tm.tm_mday != d) {
    return false;
  }
  *out = (uint32_t)t;
  return true;
}

/* One of the columns after the PI, whole: each but the last ends in its
 * comma and the last ends the line, so a line short of a column, or with one
 * more, is not read as if it had the right shape. */
static bool nextField(const char **at, const char *end, char *out, size_t cap,
                      bool last) {
  bool wide = false;
  const char *start = *at;
  if (!readField(at, end, out, cap, &wide) || wide) {
    return false;
  }
  /* readField steps over the comma after a field: a field that took one is
   * followed by another column, and one that took none ends the line. */
  const bool tookComma = *at > start && (*at)[-1] == ',';
  return last ? *at == end && !tookComma : tookComma;
}

static bool parseLine(const char *line, size_t len, int *slot,
                      MemoryChannel *out, bool *cut) {
  char field[MEMORY_CSV_LINE_MAX];
  const char *at = line;
  const char *end = line + len;
  uint32_t number = 0;

  bool wide = false;
  if (!readField(&at, end, field, sizeof(field), &wide) || wide ||
      !wholeNumber(field, &number) || number == 0 ||
      number > MEMORY_SLOT_COUNT) {
    return false;
  }
  *slot = (int)number - 1;

  memset(out, 0, sizeof(*out));
  if (!readField(&at, end, field, sizeof(field), &wide) || wide ||
      !bandFromName(field, &out->band)) {
    return false;
  }
  if (!readField(&at, end, field, sizeof(field), &wide) || wide ||
      !wholeNumber(field, &number) || number == 0) {
    return false;
  }
  out->freqKHz = number;
  if (!readField(&at, end, field, sizeof(field), &wide) || wide ||
      !wholeNumber(field, &number) || number > UINT16_MAX) {
    return false;
  }
  out->bandwidthKHz = (uint16_t)number;

  if (!readField(&at, end, out->name, MEMORY_NAME_LEN, cut)) {
    return false;
  }
  /* The PI, or nothing in a file written before the column. */
  if (at != end) {
    if (!readField(&at, end, field, sizeof(field), &wide) || wide ||
        !hexField(field, 4, &out->pi)) {
      return false;
    }
  }
  /* Then nothing, in a file written before the programme type, or all five
   * of its columns. Any other shape is one this does not understand, and
   * reading part of it would import a channel from a line whose meaning is
   * not known. */
  if (at != end) {
    if (!nextField(&at, end, field, sizeof(field), false)) {
      return false;
    }
    if (field[0] != '\0') {
      if (!wholeNumber(field, &number) || number == 0 || number > 31) {
        return false;
      }
      out->pty = (uint8_t)number;
    }
    /* The type's name and the country are for a person reading the file:
     * the type and the ECC say the same. */
    uint16_t ecc = 0;
    if (!nextField(&at, end, field, sizeof(field), false) ||
        !nextField(&at, end, field, sizeof(field), false) ||
        !hexField(field, 2, &ecc) ||
        !nextField(&at, end, field, sizeof(field), false) ||
        !nextField(&at, end, field, sizeof(field), true)) {
      return false;
    }
    out->ecc = (uint8_t)ecc;
    /* The time only says when the preset was saved, and a spreadsheet that
     * saves the file again writes dates in its own way, so one this cannot
     * read is taken as not known rather than costing the preset. */
    if (!savedField(field, &out->savedUtc)) {
      out->savedUtc = 0;
    }
  }
  return memoryChannelValid(out);
}

static size_t lineLength(const char *text, size_t len) {
  size_t n = 0;
  while (n < len && text[n] != '\n') {
    n++;
  }
  return n;
}

bool memoryImportCsv(MemoryStore *m, MemoryImportMode mode, const char *text,
                     size_t len, MemoryImportResult *out) {
  MemoryImportResult result;
  memset(&result, 0, sizeof(result));
  if (out != NULL) {
    *out = result;
  }
  if (m == NULL || text == NULL ||
      (mode != MEMORY_IMPORT_MERGE && mode != MEMORY_IMPORT_REPLACE)) {
    return false;
  }

  /* Replace reads the whole file before it writes anything, so a file with a
   * line it cannot read leaves the stored list alone. Merge writes as it
   * goes, because a merge can only add. */
  int passes = mode == MEMORY_IMPORT_REPLACE ? 2 : 1;
  for (int pass = 0; pass < passes; pass++) {
    bool writing = pass == passes - 1;
    memset(&result, 0, sizeof(result));
    if (writing && mode == MEMORY_IMPORT_REPLACE) {
      memoryInit(m);
    }
    size_t at = 0;
    uint16_t lineNumber = 0;
    while (at < len) {
      size_t n = lineLength(text + at, len - at);
      const char *line = text + at;
      size_t length = n;
      if (length > 0 && line[length - 1] == '\r') {
        length--;
      }
      at += n < len - at ? n + 1 : n;
      lineNumber++;
      if (isSkippable(line, length)) {
        continue;
      }
      result.lines++;

      int slot = 0;
      MemoryChannel channel;
      bool cut = false;
      if (!parseLine(line, length, &slot, &channel, &cut)) {
        result.skipped++;
        if (result.firstBadLine == 0) {
          result.firstBadLine = lineNumber;
        }
        if (mode == MEMORY_IMPORT_REPLACE) {
          /* Nothing has been written and nothing will be, so the counts from
           * the lines read so far would say channels were imported when none
           * were. Only the line to look at is worth reporting. */
          uint16_t bad = result.firstBadLine;
          memset(&result, 0, sizeof(result));
          result.firstBadLine = bad;
          if (out != NULL) {
            *out = result;
          }
          return false;
        }
        continue;
      }
      if (cut) {
        result.truncated++;
      }
      if (mode == MEMORY_IMPORT_MERGE && memorySlotUsed(m, slot)) {
        result.kept++;
        continue;
      }
      if (writing) {
        memorySet(m, slot, &channel);
      }
      result.imported++;
    }
  }

  if (out != NULL) {
    *out = result;
  }
  return true;
}
