/*
 * Whether a POST body's declared size is one the radio will accept.
 *
 * WebServer reads a plain or url encoded POST body into a fresh malloc
 * before any handler of ours runs, sized to whatever the client's own
 * Content-Length header says, with no hook anywhere in the library to
 * refuse that first. This is the check that has to run before it, from a
 * patch to the vendored library itself, tools/patch_webserver.py. Kept
 * here instead of written inline in the patch so the boundary can be
 * tested from a PC.
 */
#ifndef CORE_WEB_LIMITS_H
#define CORE_WEB_LIMITS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The largest POST body the radio ever makes on its own.
 *
 * The biggest one is the memory channel CSV export: MEMORY_SLOT_COUNT
 * (core/memory.h) lines of at most a two digit slot, the longest band
 * name, "OIRT", with its top frequency, 74000, a three digit bandwidth in
 * kHz, a name of MEMORY_NAME_LEN - 1 characters, all of them quotes, so
 * each is doubled inside the two quotes that wrap the field, and a four
 * digit PI. That line is 58 bytes with its newline, the header line
 * (memoryCsvHeader) 38, and 99 lines and the header come to 5780 bytes.
 * This limit is about 40% above that, not a round number picked without
 * working it out.
 */
#define WEB_MAX_POST_BODY_BYTES 8192u

/*
 * WebServer hands the header value over as a signed long, parsed by
 * String::toInt(), which returns 0 for anything it cannot read and a
 * negative number for a header that starts with a minus sign. Both have
 * to be refused rather than let through as "small enough".
 */
bool webPostBodyTooLarge(long contentLength, uint32_t capBytes);

#ifdef __cplusplus
}
#endif

#endif /* CORE_WEB_LIMITS_H */
