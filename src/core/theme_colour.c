/* Implementation of the hex colour parser and formatter. */
#include "theme_colour.h"

#include <stdio.h>

static bool hexNibble(char c, uint8_t *out) {
  if (c >= '0' && c <= '9') {
    *out = (uint8_t)(c - '0');
    return true;
  }
  if (c >= 'a' && c <= 'f') {
    *out = (uint8_t)(c - 'a' + 10);
    return true;
  }
  if (c >= 'A' && c <= 'F') {
    *out = (uint8_t)(c - 'A' + 10);
    return true;
  }
  return false;
}

static bool hexByte(const char *p, uint8_t *out) {
  uint8_t hi, lo;
  if (!hexNibble(p[0], &hi) || !hexNibble(p[1], &lo)) {
    return false;
  }
  *out = (uint8_t)((hi << 4) | lo);
  return true;
}

bool themeColourParse(const char *text, uint8_t *r, uint8_t *g, uint8_t *b) {
  if (text == NULL || r == NULL || g == NULL || b == NULL) {
    return false;
  }
  if (text[0] == '#') {
    text++;
  }
  size_t len = 0;
  while (text[len] != '\0') {
    len++;
  }
  if (len != 6) {
    return false;
  }
  return hexByte(text, r) && hexByte(text + 2, g) && hexByte(text + 4, b);
}

void themeColourFormat(uint8_t r, uint8_t g, uint8_t b, char *out, size_t cap) {
  if (out == NULL || cap < 8) {
    return;
  }
  snprintf(out, cap, "#%02x%02x%02x", r, g, b);
}
