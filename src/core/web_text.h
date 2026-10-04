/*
 * Text the web server writes and reads: a character made safe for a JSON
 * string or for HTML, and a whole number read from a request.
 *
 * Kept here, apart from the web server, so the rules can be tested on a PC.
 * The station's name, its radio text and the panel's texts all reach the
 * browser and the API through them, and one wrong escape breaks the whole
 * state document, which a script reading it cannot tell from a hung radio.
 */
#ifndef CORE_WEB_TEXT_H
#define CORE_WEB_TEXT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The longest one character can come out as, "\u001f" or "&quot;", with
 * the terminator. */
#define WEB_ESCAPE_MAX 7

/*
 * Write `c` the way it goes inside a JSON string into `out`, which holds
 * WEB_ESCAPE_MAX bytes, and return how many bytes that is, not counting the
 * terminator.
 *
 * A quote and a backslash get a backslash before them. A control character
 * comes out as "\u00XX": none belongs in a name, but a corrupt stored name
 * can still hold one and the document has to stay valid JSON. Everything
 * else is written as it is, so the bytes of a UTF-8 character pass through
 * whole.
 */
size_t webJsonEscapeChar(char c, char *out);

/*
 * Write `c` the way it goes into HTML text or a quoted attribute into
 * `out`, which holds WEB_ESCAPE_MAX bytes, and return how many bytes that
 * is, not counting the terminator. `&`, `<`, `>`, `"` and `'` become their
 * entities; everything else is written as it is.
 */
size_t webHtmlEscapeChar(char c, char *out);

typedef enum {
  WEB_NUMBER_OK = 0,
  WEB_NUMBER_NOT_WHOLE,    /* Empty, or not a base 10 whole number. */
  WEB_NUMBER_OUT_OF_RANGE, /* A whole number, but outside low to high. */
} WebNumberResult;

/*
 * Read `raw` as a base 10 whole number from `low` to `high` into `out`.
 *
 * What strtol takes, all of it: leading spaces and a sign are allowed, and
 * anything after the digits is refused. A number too big for a long is out
 * of range rather than read as the largest long. `out` is written only on
 * WEB_NUMBER_OK.
 */
WebNumberResult webParseNumber(const char *raw, long low, long high, long *out);

#ifdef __cplusplus
}
#endif

#endif /* CORE_WEB_TEXT_H */
