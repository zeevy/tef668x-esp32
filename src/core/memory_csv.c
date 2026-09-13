/* Implementation of the channel list as text. */
#include "memory_csv.h"

#include <string.h>

/* The header line this writes and accepts back. */
static const char kHeader[] = "slot,band,frequency,bandwidth,name\n";

const char *memoryCsvHeader(void) {
  return kHeader;
}

static void put(char *out, size_t cap, size_t *n, char c) {
  if (*n + 1 < cap) {
    out[*n] = c;
  }
  (*n)++;
}

static void putNumber(char *out, size_t cap, size_t *n, uint32_t value) {
  char digits[11];
  size_t count = 0;
  do {
    digits[count++] = (char)('0' + (value % 10u));
    value /= 10u;
  } while (value != 0 && count < sizeof(digits));
  while (count > 0) {
    put(out, cap, n, digits[--count]);
  }
}

static bool needsQuotes(const char *name) {
  for (size_t i = 0; name[i] != '\0'; i++) {
    if (name[i] == ',' || name[i] == '"') {
      return true;
    }
  }
  return false;
}

size_t memoryCsvLine(const MemoryStore *m, int slot, char *out, size_t cap) {
  const MemoryChannel *c = memoryGet(m, slot);
  if (c == NULL || out == NULL) {
    return 0;
  }
  size_t n = 0;
  /* Counted from 1, the way the slot is shown on the panel and typed on the
   * keypad. A file whose first channel is called 0 would be read back onto
   * the wrong slot by anybody editing it by hand. */
  putNumber(out, cap, &n, (uint32_t)slot + 1u);
  put(out, cap, &n, ',');
  const char *band = bandName((BandId)c->band);
  for (size_t i = 0; band[i] != '\0'; i++) {
    put(out, cap, &n, band[i]);
  }
  put(out, cap, &n, ',');
  putNumber(out, cap, &n, c->freqKHz);
  put(out, cap, &n, ',');
  putNumber(out, cap, &n, c->bandwidthKHz);
  put(out, cap, &n, ',');
  bool quoted = needsQuotes(c->name);
  if (quoted) {
    put(out, cap, &n, '"');
  }
  for (size_t i = 0; c->name[i] != '\0'; i++) {
    if (c->name[i] == '"') {
      put(out, cap, &n, '"');
    }
    put(out, cap, &n, c->name[i]);
  }
  if (quoted) {
    put(out, cap, &n, '"');
  }
  put(out, cap, &n, '\n');
  if (cap != 0) {
    out[n < cap ? n : cap - 1] = '\0';
  }
  return n;
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

static bool isSkippable(const char *line, size_t len) {
  if (len == 0) {
    return true;
  }
  if (line[0] == '#') {
    return true;
  }
  /* The header this writes, in any case, and nothing else that merely fails
   * to be a number. A line that is not a header and not a channel is an error
   * worth reporting, not something to pass over. */
  size_t headerLen = strlen(kHeader) - 1;
  if (len != headerLen) {
    return false;
  }
  for (size_t i = 0; i < headerLen; i++) {
    char a = line[i];
    if (a >= 'A' && a <= 'Z') {
      a = (char)(a - 'A' + 'a');
    }
    if (a != kHeader[i]) {
      return false;
    }
  }
  return true;
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
  /* Nothing may follow the name. A sixth field means the file has a shape
   * this does not understand, and reading the first five anyway would import
   * a channel from a line whose meaning is not known. */
  if (at != end) {
    return false;
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
