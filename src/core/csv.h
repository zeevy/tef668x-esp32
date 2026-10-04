/*
 * One CSV field, the way the two exports that quote a name write it: the
 * preset list and the logbook, which quotes its radio text the same way.
 */
#ifndef CORE_CSV_H
#define CORE_CSV_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Write `text` into `out` as one CSV field: as it is, or inside quotes with
 * every quote doubled when it holds a comma or a quote. Reads at most
 * `maxLen` characters of it.
 *
 * A field too long for `cap` is cut short, but never between a quote and its
 * double, and a quoted field always gets both of its quotes: a quote written
 * alone closes the field early, or never, and the rest of the line lands in
 * the wrong columns. With no room for both quotes and the terminator the
 * field is left empty. Every caller sizes `out` for the worst case, so
 * nothing is cut today.
 */
static inline void csvQuote(const char *text, size_t maxLen, char *out,
                            size_t cap) {
  if (out == NULL || cap == 0) {
    return;
  }
  out[0] = '\0';
  bool quote = false;
  for (size_t i = 0; i < maxLen && text[i] != '\0'; i++) {
    if (text[i] == ',' || text[i] == '"') {
      quote = true;
      break;
    }
  }
  if (quote && cap < 3) {
    return;
  }
  size_t at = 0;
  if (quote) {
    out[at++] = '"';
  }
  /* Room kept for the closing quote, when there is one. */
  const size_t end = quote ? cap - 1 : cap;
  for (size_t i = 0; i < maxLen && text[i] != '\0'; i++) {
    const size_t need = text[i] == '"' ? 2 : 1;
    if (at + need >= end) {
      break;
    }
    if (text[i] == '"') {
      out[at++] = '"';
    }
    out[at++] = text[i];
  }
  if (quote) {
    out[at++] = '"';
  }
  out[at] = '\0';
}

#endif /* CORE_CSV_H */
