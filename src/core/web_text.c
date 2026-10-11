#include "web_text.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t put(char *out, const char *text) {
  size_t n = strlen(text);
  memcpy(out, text, n + 1);
  return n;
}

size_t webJsonEscapeChar(char c, char *out) {
  const unsigned char u = (unsigned char)c;
  if (u == '"' || u == '\\') {
    out[0] = '\\';
    out[1] = c;
    out[2] = '\0';
    return 2;
  }
  if (u < 0x20) {
    snprintf(out, WEB_ESCAPE_MAX, "\\u%04x", u);
    return 6;
  }
  out[0] = c;
  out[1] = '\0';
  return 1;
}

size_t webHtmlEscapeChar(char c, char *out) {
  switch (c) {
    case '&':
      return put(out, "&amp;");
    case '<':
      return put(out, "&lt;");
    case '>':
      return put(out, "&gt;");
    case '"':
      return put(out, "&quot;");
    case '\'':
      return put(out, "&#39;");
    default:
      out[0] = c;
      out[1] = '\0';
      return 1;
  }
}

WebNumberResult webParseCount(const char *raw, uint32_t *out) {
  if (raw == NULL || raw[0] == '\0') {
    return WEB_NUMBER_NOT_WHOLE;
  }
  uint64_t value = 0;
  bool tooBig = false;
  for (const char *p = raw; *p != '\0'; p++) {
    if (*p < '0' || *p > '9') {
      return WEB_NUMBER_NOT_WHOLE;
    }
    /* Every character is still checked once it is too big, so "99999999999x"
     * is not a number rather than one out of range. */
    if (!tooBig) {
      value = value * 10u + (uint64_t)(*p - '0');
      tooBig = value > 0xFFFFFFFFull;
    }
  }
  if (tooBig) {
    return WEB_NUMBER_OUT_OF_RANGE;
  }
  if (out != NULL) {
    *out = (uint32_t)value;
  }
  return WEB_NUMBER_OK;
}

WebNumberResult webParseNumber(const char *raw, long low, long high,
                               long *out) {
  if (raw == NULL || raw[0] == '\0') {
    return WEB_NUMBER_NOT_WHOLE;
  }
  char *end = NULL;
  errno = 0;
  const long value = strtol(raw, &end, 10);
  if (end == raw || *end != '\0') {
    return WEB_NUMBER_NOT_WHOLE;
  }
  if (errno == ERANGE || value < low || value > high) {
    return WEB_NUMBER_OUT_OF_RANGE;
  }
  if (out != NULL) {
    *out = value;
  }
  return WEB_NUMBER_OK;
}
