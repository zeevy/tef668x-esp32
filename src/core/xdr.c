/* Implementation of the XDR protocol's lines. */
#include "xdr.h"

#include <stdio.h>
#include <string.h>

/* A whole number with an optional minus, at most 9 digits so it cannot
 * overflow. `*end` is left on the first character after it. */
static bool readNumber(const char *s, const char **end, int32_t *out) {
  bool negative = false;
  if (*s == '-') {
    negative = true;
    s++;
  }
  int32_t value = 0;
  int digits = 0;
  while (*s >= '0' && *s <= '9') {
    if (++digits > 9) {
      return false;
    }
    value = value * 10 + (*s - '0');
    s++;
  }
  if (digits == 0) {
    return false;
  }
  *out = negative ? -value : value;
  *end = s;
  return true;
}

/* The whole rest of the line as one number from `min` to `max`. */
static bool oneNumber(const char *s, int32_t min, int32_t max, int32_t *out) {
  const char *end = NULL;
  return readNumber(s, &end, out) && *end == '\0' && *out >= min && *out <= max;
}

typedef struct {
  char letter;
  XdrKind kind;
  int32_t min;
  int32_t max;
  const char *why;
} XdrRange;

/* The commands that carry one number, and what they may be. */
static const XdrRange kRanges[] = {
    {'M', XDR_MODE, 0, 1, "M is 0 for FM or 1 for AM."},
    {'W', XDR_WIDTH, 0, 400000, "W is a width in Hz, 0 for automatic."},
    {'D', XDR_DEEMPHASIS, 0, 2, "D is 0, 1 or 2."},
    {'B', XDR_MONO, 0, 2, "B is 0, 1 or 2."},
    {'Y', XDR_VOLUME, 0, 100, "Y is 0 to 100."},
    {'Q', XDR_SQUELCH, -1, 100, "Q is -1 to 100."},
    {'A', XDR_AGC, 0, 3, "A is 0 to 3."},
    {'Z', XDR_ANTENNA, 0, 3, "Z is 0 to 3."},
    {'V', XDR_ATTENUATION, 0, 127, "V is 0 to 127."},
    {'C', XDR_ROTATOR, 0, 2, "C is 0, 1 or 2."},
};

const char *xdrParse(const char *line, XdrCommand *out) {
  if (out == NULL) {
    return "Nothing to put it in.";
  }
  out->kind = XDR_IGNORE;
  out->value = 0;
  out->value2 = 0;
  if (line == NULL) {
    return "No line.";
  }
  char text[XDR_LINE_MAX + 2];
  size_t len = strlen(line);
  if (len > 0 && line[len - 1] == '\r') {
    len--;
  }
  if (len > XDR_LINE_MAX) {
    return "The line is too long.";
  }
  memcpy(text, line, len);
  text[len] = '\0';
  if (len == 0) {
    return NULL; /* An empty line ends a scan; there is none to end. */
  }
  const char letter = text[0];
  const char *rest = text + 1;
  int32_t value = 0;
  switch (letter) {
    case 'x':
      out->kind = XDR_START;
      return NULL;
    case 'X':
      out->kind = XDR_END;
      return NULL;
    case 'T':
      if (!oneNumber(rest, 0, 200000, &value) || (value != 0 && value < 100)) {
        return "T is a frequency in kHz, 100 to 200000.";
      }
      /* FM-DX Webserver sends T0 to clear its own RDS; there is nothing to
       * tune. */
      if (value != 0) {
        out->kind = XDR_TUNE;
        out->value = value;
      }
      return NULL;
    case 'G':
      if (strlen(rest) != 2 || (rest[0] != '0' && rest[0] != '1') ||
          (rest[1] != '0' && rest[1] != '1')) {
        return "G is two digits, each 0 or 1.";
      }
      out->kind = XDR_EQ_IMS;
      out->value = rest[0] - '0';
      out->value2 = rest[1] - '0';
      return NULL;
    case 'I': {
      const char *end = NULL;
      int32_t mode = 0;
      if (!readNumber(rest, &end, &value) || value < 0 ||
          value > XDR_INTERVAL_MAX_MS ||
          (*end == ',' && (!readNumber(end + 1, &end, &mode))) ||
          *end != '\0') {
        return "I is a time in ms, 0 to 1000, then an optional comma and a "
               "number.";
      }
      out->kind = XDR_INTERVAL;
      out->value = value;
      out->value2 = mode;
      return NULL;
    }
    default:
      break;
  }
  for (size_t i = 0; i < sizeof(kRanges) / sizeof(kRanges[0]); i++) {
    if (kRanges[i].letter != letter) {
      continue;
    }
    if (!oneNumber(rest, kRanges[i].min, kRanges[i].max, &value)) {
      return kRanges[i].why;
    }
    out->kind = kRanges[i].kind;
    out->value = value;
    return NULL;
  }
  /* F, the old filter index, comes with the width it means as W after it;
   * N, S and anything newer are not offered. */
  return NULL;
}

int32_t xdrDbfTenths(int16_t levelTenthsDbuV) {
  const int32_t hundredths = (int32_t)levelTenthsDbuV * 10 + 1125;
  return hundredths >= 0 ? (hundredths + 5) / 10 : -((-hundredths + 5) / 10);
}

/* A number in tenths with one decimal, the sign kept below 1. */
static int tenths(char *out, size_t cap, int32_t t) {
  const int32_t whole = (t < 0 ? -t : t);
  return snprintf(out, cap, "%s%ld.%ld", t < 0 ? "-" : "", (long)(whole / 10),
                  (long)(whole % 10));
}

static size_t done(int n, size_t cap) {
  return n < 0 ? 0 : (size_t)n >= cap ? cap - 1 : (size_t)n;
}

size_t xdrSignal(char *out, size_t cap, int16_t levelTenthsDbuV, bool pilot,
                 bool forcedMono, bool am) {
  if (out == NULL || cap < 4) {
    return 0;
  }
  char flag = 'm';
  if (!am) {
    flag = pilot ? (forcedMono ? 'S' : 's') : (forcedMono ? 'M' : 'm');
  }
  out[0] = 'S';
  out[1] = flag;
  const int n = tenths(out + 2, cap - 2, xdrDbfTenths(levelTenthsDbuV));
  return n < 0 ? 0 : 2 + done(n, cap - 2);
}

size_t xdrPi(char *out, size_t cap, uint16_t pi, uint8_t doubt) {
  if (doubt > 3) {
    doubt = 3;
  }
  return done(snprintf(out, cap, "P%04X%.*s", (unsigned)pi, (int)doubt, "???"),
              cap);
}

size_t xdrRds(char *out, size_t cap, const uint16_t block[4], uint8_t error) {
  return done(snprintf(out, cap, "R%04X%04X%04X%04X%02X", (unsigned)block[0],
                       (unsigned)block[1], (unsigned)block[2],
                       (unsigned)block[3], (unsigned)error),
              cap);
}

size_t xdrValue(char *out, size_t cap, char letter, long value) {
  return done(snprintf(out, cap, "%c%ld", letter, value), cap);
}

size_t xdrEqIms(char *out, size_t cap, bool equalizer, bool ims) {
  return done(snprintf(out, cap, "G%d%d", equalizer ? 1 : 0, ims ? 1 : 0), cap);
}

size_t xdrUsers(char *out, size_t cap, unsigned users) {
  return done(snprintf(out, cap, "o%u,0", users), cap);
}

void xdrSalt(const uint8_t random[XDR_SALT_LEN], char out[XDR_SALT_LEN + 1]) {
  static const char kChars[] =
      "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm0123456789_-";
  for (int i = 0; i < XDR_SALT_LEN; i++) {
    out[i] = kChars[random[i] & 63u];
  }
  out[XDR_SALT_LEN] = '\0';
}

void xdrHex(const uint8_t digest[XDR_DIGEST_LEN],
            char out[XDR_DIGEST_HEX + 1]) {
  static const char kHex[] = "0123456789abcdef";
  for (int i = 0; i < XDR_DIGEST_LEN; i++) {
    out[2 * i] = kHex[digest[i] >> 4];
    out[2 * i + 1] = kHex[digest[i] & 15u];
  }
  out[XDR_DIGEST_HEX] = '\0';
}

bool xdrDigestMatches(const char *line,
                      const char expected[XDR_DIGEST_HEX + 1]) {
  if (line == NULL || expected == NULL) {
    return false;
  }
  size_t len = strlen(line);
  if (len > 0 && line[len - 1] == '\r') {
    len--;
  }
  if (len != XDR_DIGEST_HEX) {
    return false;
  }
  unsigned differ = 0;
  for (size_t i = 0; i < XDR_DIGEST_HEX; i++) {
    char c = line[i];
    if (c >= 'A' && c <= 'F') {
      c = (char)(c - 'A' + 'a');
    }
    differ |= (unsigned)(c ^ expected[i]);
  }
  return differ == 0;
}

int8_t xdrVolumeDb(int32_t volume) {
  if (volume < 1) {
    volume = 1;
  }
  if (volume > 100) {
    volume = 100;
  }
  return (int8_t)(-60 + (volume * 60 + 50) / 100);
}

int32_t xdrVolumeFromDb(int8_t db, bool muted) {
  if (muted) {
    return 0;
  }
  if (db >= 0) {
    return 100;
  }
  const int32_t v = ((int32_t)(db + 60) * 100 + 30) / 60;
  return v < 1 ? 1 : v;
}

uint16_t xdrWidthKHz(BandId band, int32_t hz, uint16_t current) {
  const size_t count = bandBandwidthCount(band);
  if (hz <= 0) {
    return count > 0 && bandBandwidthAt(band, 0) == 0 ? 0 : current;
  }
  const int32_t want = (hz + 500) / 1000;
  uint16_t best = current;
  int32_t bestOff = INT32_MAX;
  for (size_t i = 0; i < count; i++) {
    const uint16_t w = bandBandwidthAt(band, i);
    if (w == 0) {
      continue;
    }
    const int32_t off = want > w ? want - w : w - want;
    if (off < bestOff) {
      bestOff = off;
      best = w;
    }
  }
  return best;
}

int32_t xdrSquelchValue(SquelchMode mode, bool am, int16_t manualTenths,
                        uint8_t fmFloorDbuV) {
  if (mode == SQUELCH_OFF) {
    return 0;
  }
  if (mode != SQUELCH_MANUAL && (am || fmFloorDbuV == 0)) {
    return 1;
  }
  const int32_t tenths = xdrDbfTenths(
      mode == SQUELCH_MANUAL ? manualTenths : (int16_t)(fmFloorDbuV * 10));
  const int32_t whole = tenths >= 0 ? (tenths + 5) / 10 : -((-tenths + 5) / 10);
  return whole < 1 ? 1 : whole > 100 ? 100 : whole;
}

uint16_t xdrDeemphasisUs(int32_t code) {
  return code == 0 ? 50 : code == 1 ? 75 : 0;
}

int32_t xdrDeemphasisCode(uint16_t us) {
  return us == 50 ? 0 : us == 75 ? 1 : 2;
}
