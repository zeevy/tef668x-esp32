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
