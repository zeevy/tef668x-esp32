/* Implementation of the channel list as text. */
#include "memory_csv.h"

#include <stdio.h>
#include <string.h>

#include "csv.h"

/* The header line this writes and accepts back, and the one written before
 * the PI column, accepted too so an older file still reads. */
static const char kHeader[] = "slot,band,frequency,bandwidth,name,pi\n";
static const char kHeaderNoPi[] = "slot,band,frequency,bandwidth,name\n";

const char *memoryCsvHeader(void) {
  return kHeader;
}

size_t memoryCsvLine(const MemoryStore *m, int slot, char *out, size_t cap) {
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
  /* The slot counted from 1, the way it is shown on the panel and typed on
   * the keypad. A file whose first channel is called 0 would be read back
   * onto the wrong slot by anybody editing it by hand. */
  const int n = snprintf(out, cap, "%d,%s,%lu,%u,%s,%s\n", slot + 1,
                         bandName((BandId)c->band), (unsigned long)c->freqKHz,
                         (unsigned)c->bandwidthKHz, name, pi);
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

/* Four hex digits in either case, or an empty field for none. False for
 * anything else, and for 0000, which names no station. */
static bool piField(const char *text, uint16_t *out) {
  if (text[0] == '\0') {
    *out = 0;
    return true;
  }
  uint16_t value = 0;
  for (int i = 0; i < 4; i++) {
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
  if (text[4] != '\0' || value == 0) {
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
  return isHeader(line, len, kHeader) || isHeader(line, len, kHeaderNoPi);
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
  /* The PI, or nothing in a file written before the column. Nothing may
   * follow it: a seventh field means a shape this does not understand, and
   * reading the first six anyway would import a channel from a line whose
   * meaning is not known. */
  if (at != end) {
    if (!readField(&at, end, field, sizeof(field), &wide) || wide ||
        !piField(field, &out->pi) || at != end) {
      return false;
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
