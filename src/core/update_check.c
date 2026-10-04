#include "update_check.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------- versions */

/* One part of x.y.z from `*p`, up to `stop` or the end of the string. */
static bool parsePart(const char **p, char stop, uint16_t *out) {
  const char *s = *p;
  uint32_t value = 0;
  int digits = 0;
  while (*s >= '0' && *s <= '9') {
    if (digits == 5) {
      return false;
    }
    value = value * 10u + (uint32_t)(*s - '0');
    digits++;
    s++;
  }
  if (digits == 0 || value > 65535u || *s != stop) {
    return false;
  }
  /* "007" is not a version anybody writes, and two spellings of one number
   * would compare equal while reading differently. */
  if (digits > 1 && (*p)[0] == '0') {
    return false;
  }
  *out = (uint16_t)value;
  *p = s;
  return true;
}

bool updateParseVersion(const char *text, UpdateVersion *out) {
  if (text == NULL || out == NULL) {
    return false;
  }
  UpdateVersion v;
  const char *p = text;
  if (!parsePart(&p, '.', &v.major)) {
    return false;
  }
  p++;
  if (!parsePart(&p, '.', &v.minor)) {
    return false;
  }
  p++;
  if (!parsePart(&p, '\0', &v.patch)) {
    return false;
  }
  *out = v;
  return true;
}

int updateCompareVersions(const UpdateVersion *a, const UpdateVersion *b) {
  if (a->major != b->major) {
    return a->major < b->major ? -1 : 1;
  }
  if (a->minor != b->minor) {
    return a->minor < b->minor ? -1 : 1;
  }
  if (a->patch != b->patch) {
    return a->patch < b->patch ? -1 : 1;
  }
  return 0;
}

/* ------------------------------------------------------------- the JSON */

/*
 * A reader for the one shape a manifest has: a flat object whose values are
 * strings with no escapes, or whole numbers. Anything else is refused rather
 * than half understood, since the manifest decides what gets written to
 * flash.
 */
typedef struct {
  const char *p;
  const char *end;
} Cursor;

static void skipSpace(Cursor *c) {
  while (c->p < c->end &&
         (*c->p == ' ' || *c->p == '\t' || *c->p == '\n' || *c->p == '\r')) {
    c->p++;
  }
}

static bool take(Cursor *c, char ch) {
  skipSpace(c);
  if (c->p < c->end && *c->p == ch) {
    c->p++;
    return true;
  }
  return false;
}

/*
 * A string into `out`, which always ends up terminated. `*fits` says whether
 * all of it went in. With `out` NULL the string is only passed over. False
 * for a string that does not end, or that holds an escape or a control
 * character.
 */
static bool readString(Cursor *c, char *out, size_t cap, bool *fits) {
  if (!take(c, '"')) {
    return false;
  }
  size_t n = 0;
  *fits = true;
  while (c->p < c->end && *c->p != '"') {
    const unsigned char ch = (unsigned char)*c->p;
    if (ch < 0x20 || ch == '\\') {
      return false;
    }
    if (out != NULL && n + 1 < cap) {
      out[n++] = (char)ch;
    } else if (out != NULL) {
      *fits = false;
    }
    c->p++;
  }
  if (c->p >= c->end) {
    return false;
  }
  c->p++; /* the closing quote */
  if (out != NULL) {
    out[n] = '\0';
  }
  return true;
}

/* A whole number, 0 to 4294967295, with no sign, fraction or leading zero. */
static bool readNumber(Cursor *c, uint32_t *out) {
  skipSpace(c);
  const char *start = c->p;
  uint64_t value = 0;
  while (c->p < c->end && *c->p >= '0' && *c->p <= '9') {
    value = value * 10u + (uint64_t)(*c->p - '0');
    if (value > 0xFFFFFFFFu) {
      return false;
    }
    c->p++;
  }
  const size_t digits = (size_t)(c->p - start);
  if (digits == 0 || (digits > 1 && start[0] == '0')) {
    return false;
  }
  *out = (uint32_t)value;
  return true;
}

/* The five fields, before they are judged. */
typedef struct {
  char version[24];
  char board[24];
  char url[sizeof(((UpdateManifest *)0)->url) + 1];
  char sha256[66];
  uint32_t size;
  bool haveVersion, haveBoard, haveUrl, haveSize, haveSha256;
  bool versionFits, boardFits, urlFits, shaFits;
} Fields;

/* Reads one value into its field, or passes over it when the key is not
 * one of the five. */
static UpdateManifestResult readValue(Cursor *c, const char *key, Fields *f) {
  bool fits = true;
  struct {
    const char *name;
    char *buf;
    size_t cap;
    bool *have;
    bool *fitsOut;
  } strings[] = {
      {"version", f->version, sizeof(f->version), &f->haveVersion,
       &f->versionFits},
      {"board", f->board, sizeof(f->board), &f->haveBoard, &f->boardFits},
      {"url", f->url, sizeof(f->url), &f->haveUrl, &f->urlFits},
      {"sha256", f->sha256, sizeof(f->sha256), &f->haveSha256, &f->shaFits},
  };
  for (size_t i = 0; i < sizeof(strings) / sizeof(strings[0]); i++) {
    if (strcmp(key, strings[i].name) == 0) {
      if (*strings[i].have) {
        return UPDATE_MANIFEST_REPEATED_FIELD;
      }
      if (!readString(c, strings[i].buf, strings[i].cap, &fits)) {
        return UPDATE_MANIFEST_NOT_JSON;
      }
      *strings[i].have = true;
      *strings[i].fitsOut = fits;
      return UPDATE_MANIFEST_OK;
    }
  }
  if (strcmp(key, "size") == 0) {
    if (f->haveSize) {
      return UPDATE_MANIFEST_REPEATED_FIELD;
    }
    if (!readNumber(c, &f->size)) {
      return UPDATE_MANIFEST_NOT_JSON;
    }
    f->haveSize = true;
    return UPDATE_MANIFEST_OK;
  }
  /* A field this firmware does not know: a string or a whole number, read
   * and dropped. */
  skipSpace(c);
  if (c->p < c->end && *c->p == '"') {
    return readString(c, NULL, 0, &fits) ? UPDATE_MANIFEST_OK
                                         : UPDATE_MANIFEST_NOT_JSON;
  }
  uint32_t ignored = 0;
  return readNumber(c, &ignored) ? UPDATE_MANIFEST_OK
                                 : UPDATE_MANIFEST_NOT_JSON;
}

static UpdateManifestResult readFields(const char *json, size_t len,
                                       Fields *f) {
  Cursor c = {json, json + len};
  if (!take(&c, '{')) {
    return UPDATE_MANIFEST_NOT_JSON;
  }
  if (!take(&c, '}')) {
    for (;;) {
      char key[16];
      bool fits = true;
      if (!readString(&c, key, sizeof(key), &fits) || !take(&c, ':')) {
        return UPDATE_MANIFEST_NOT_JSON;
      }
      /* A key too long for the buffer is none of the five. */
      const UpdateManifestResult r = readValue(&c, fits ? key : "", f);
      if (r != UPDATE_MANIFEST_OK) {
        return r;
      }
      if (take(&c, ',')) {
        continue;
      }
      if (take(&c, '}')) {
        break;
      }
      return UPDATE_MANIFEST_NOT_JSON;
    }
  }
  skipSpace(&c);
  return c.p == c.end ? UPDATE_MANIFEST_OK : UPDATE_MANIFEST_NOT_JSON;
}

/* ------------------------------------------------------------- the fields */

static bool isHex(char ch) {
  return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
         (ch >= 'A' && ch <= 'F');
}

static bool tagChar(char ch) {
  return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') ||
         (ch >= 'A' && ch <= 'Z') || ch == '.' || ch == '_' || ch == '-';
}

/*
 * The release file of this board under this repository's releases:
 * UPDATE_IMAGE_URL_PREFIX, a tag, "/firmware-", the board and ".bin". The tag
 * is 1 to 40 of the characters a git tag here uses, and cannot step out of
 * the path with dots.
 */
static bool imageUrlOk(const char *url, const char *board) {
  const size_t prefixLen = strlen(UPDATE_IMAGE_URL_PREFIX);
  if (strncmp(url, UPDATE_IMAGE_URL_PREFIX, prefixLen) != 0) {
    return false;
  }
  const char *tag = url + prefixLen;
  size_t tagLen = 0;
  while (tagChar(tag[tagLen])) {
    tagLen++;
  }
  if (tagLen == 0 || tagLen > 40 || tag[0] == '.' ||
      strstr(tag, "..") != NULL) {
    return false;
  }
  char tail[48];
  if ((size_t)snprintf(tail, sizeof(tail), "/firmware-%s.bin", board) >=
      sizeof(tail)) {
    return false;
  }
  return strcmp(tag + tagLen, tail) == 0;
}

UpdateManifestResult updateParseManifest(const char *json, size_t len,
                                         const char *board, uint32_t slotBytes,
                                         UpdateManifest *out) {
  if (json == NULL || board == NULL || out == NULL || len == 0 ||
      len > UPDATE_MANIFEST_MAX_BYTES) {
    return UPDATE_MANIFEST_TOO_LONG;
  }
  Fields f;
  memset(&f, 0, sizeof(f));
  const UpdateManifestResult read = readFields(json, len, &f);
  if (read != UPDATE_MANIFEST_OK) {
    return read;
  }
  if (!f.haveVersion || !f.haveBoard || !f.haveUrl || !f.haveSize ||
      !f.haveSha256) {
    return UPDATE_MANIFEST_MISSING_FIELD;
  }

  UpdateVersion version;
  if (!f.versionFits || !updateParseVersion(f.version, &version)) {
    return UPDATE_MANIFEST_BAD_VERSION;
  }
  if (!f.boardFits || strcmp(f.board, board) != 0) {
    return UPDATE_MANIFEST_WRONG_BOARD;
  }
  if (f.size == 0 || f.size > slotBytes) {
    return UPDATE_MANIFEST_BAD_SIZE;
  }
  if (!f.shaFits || strlen(f.sha256) != 64) {
    return UPDATE_MANIFEST_BAD_SHA256;
  }
  for (size_t i = 0; i < 64; i++) {
    if (!isHex(f.sha256[i])) {
      return UPDATE_MANIFEST_BAD_SHA256;
    }
  }
  if (!f.urlFits || strlen(f.url) >= sizeof(out->url) ||
      !imageUrlOk(f.url, board)) {
    return UPDATE_MANIFEST_BAD_URL;
  }

  out->version = version;
  /* Both lengths were checked above: the version parsed, so it fits, and
   * the address was refused unless it was shorter than `out->url`. */
  memcpy(out->versionText, f.version, strlen(f.version) + 1);
  memcpy(out->url, f.url, strlen(f.url) + 1);
  out->size = f.size;
  for (size_t i = 0; i < 64; i++) {
    const char ch = f.sha256[i];
    out->sha256[i] = (ch >= 'A' && ch <= 'F') ? (char)(ch - 'A' + 'a') : ch;
  }
  out->sha256[64] = '\0';
  return UPDATE_MANIFEST_OK;
}

/* ------------------------------------------------------------- addresses */

bool updateUrlAllowed(const char *url) {
  if (url == NULL) {
    return false;
  }
  const size_t len = strlen(url);
  if (len == 0 || len > 2048) {
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    const unsigned char ch = (unsigned char)url[i];
    if (ch <= 0x20 || ch >= 0x7F || ch == '\\') {
      return false;
    }
  }
  static const char kRepo[] =
      "https://github.com/zeevy/tef668x-esp32/releases/";
  if (strncmp(url, kRepo, sizeof(kRepo) - 1) == 0) {
    /* The path must stay under this repository's releases. */
    return strstr(url, "..") == NULL && strchr(url, '%') == NULL;
  }
  /* GitHub's file hosts for release files. Their addresses carry a signed,
   * percent encoded query, and the host alone is what matters: the
   * certificate check proves it. */
  static const char *const kHosts[] = {
      "https://release-assets.githubusercontent.com/",
      "https://objects.githubusercontent.com/",
  };
  for (size_t i = 0; i < sizeof(kHosts) / sizeof(kHosts[0]); i++) {
    if (strncmp(url, kHosts[i], strlen(kHosts[i])) == 0) {
      return true;
    }
  }
  return false;
}

/* ------------------------------------------------------------- when */

void updateDueReset(UpdateDue *due) {
  if (due != NULL) {
    due->done = false;
  }
}

bool updateDueStep(UpdateDue *due, const UpdateDueInputs *in) {
  if (due == NULL || in == NULL || due->done) {
    return false;
  }
  if (!in->enabled || !in->online || in->onTrial || in->busy) {
    return false;
  }
  due->done = true;
  return true;
}

void updateFormatMegabytes(uint32_t bytes, char *out, size_t outLen) {
  if (out == NULL || outLen == 0) {
    return;
  }
  const uint32_t tenths = (uint32_t)(((uint64_t)bytes + 50000u) / 100000u);
  if ((size_t)snprintf(out, outLen, "%u.%u", (unsigned)(tenths / 10u),
                       (unsigned)(tenths % 10u)) >= outLen) {
    out[0] = '\0';
  }
}
