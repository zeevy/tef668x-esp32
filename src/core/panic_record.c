#include "panic_record.h"

#include <stdio.h>
#include <string.h>

/* A word that neither zeros nor ones, the usual noise, can be. */
#define PANIC_RECORD_MARKER 0x50EC1C00u

static uint32_t sum(const PanicRecord *r) {
  const uint8_t *p = (const uint8_t *)r;
  uint32_t s = 0x811C9DC5u;
  for (size_t i = 0; i < offsetof(PanicRecord, check); i++) {
    s = (s ^ p[i]) * 0x01000193u; /* FNV-1a, so a swapped pair changes it. */
  }
  return s;
}

void panicRecordSeal(PanicRecord *r) {
  if (r == NULL) {
    return;
  }
  r->why[PANIC_RECORD_WHY_LEN - 1] = '\0';
  r->build[PANIC_RECORD_BUILD_LEN - 1] = '\0';
  if (r->frameCount > PANIC_RECORD_FRAMES) {
    r->frameCount = PANIC_RECORD_FRAMES;
  }
  r->marker = PANIC_RECORD_MARKER;
  r->check = sum(r);
}

bool panicRecordValid(const PanicRecord *r) {
  return r != NULL && r->marker == PANIC_RECORD_MARKER && r->check == sum(r) &&
         r->frameCount <= PANIC_RECORD_FRAMES &&
         memchr(r->why, '\0', sizeof(r->why)) != NULL &&
         memchr(r->build, '\0', sizeof(r->build)) != NULL;
}

void panicRecordClear(PanicRecord *r) {
  if (r != NULL) {
    memset(r, 0, sizeof(*r));
  }
}

/* A text for JSON: quotes and backslashes escaped, anything below a space
 * or past ASCII dropped, since the words come from the chip and could be
 * cut anywhere. */
static size_t putText(char *out, size_t cap, size_t at, const char *text) {
  for (const char *c = text; *c != '\0' && at + 3 < cap; c++) {
    const unsigned char u = (unsigned char)*c;
    if (u < 0x20 || u > 0x7E) {
      continue;
    }
    if (u == '"' || u == '\\') {
      out[at++] = '\\';
    }
    out[at++] = (char)u;
  }
  out[at] = '\0';
  return at;
}

size_t panicRecordJson(const PanicRecord *r, char *out, size_t cap) {
  if (out == NULL || cap == 0) {
    return 0;
  }
  if (!panicRecordValid(r)) {
    if (cap < 5) {
      out[0] = '\0';
      return 0;
    }
    memcpy(out, "null", 5);
    return 4;
  }
  if (cap < PANIC_RECORD_JSON_LEN) {
    out[0] = '\0';
    return 0;
  }
  size_t at = (size_t)snprintf(out, cap, "{\"why\":\"");
  at = putText(out, cap, at, r->why);
  at += (size_t)snprintf(out + at, cap - at, "\",\"pc\":\"0x%08lx\",\"bt\":[",
                         (unsigned long)r->pc);
  for (uint8_t i = 0; i < r->frameCount; i++) {
    at += (size_t)snprintf(out + at, cap - at, "%s\"0x%08lx\"", i ? "," : "",
                           (unsigned long)r->frames[i]);
  }
  at += (size_t)snprintf(out + at, cap - at,
                         "],\"core\":%u,\"up\":%lu,\"bld\":\"",
                         (unsigned)r->core, (unsigned long)r->uptimeS);
  at = putText(out, cap, at, r->build);
  at += (size_t)snprintf(out + at, cap - at, "\"}");
  return at;
}
