/*
 * Updates from GitHub releases: what is in a release's manifest, whether it
 * is newer than this firmware, which addresses a download may go to, and when
 * the radio should look.
 *
 * All of the deciding and none of the doing. The manifest comes from the
 * network, so every field is checked here before anything acts on it, and
 * none of that needs a radio to test.
 *
 * A manifest is one small JSON object written by the release workflow:
 *
 *   {"version": "0.2.0", "board": "ats125",
 *    "url": "https://github.com/zeevy/tef668x-esp32/releases/download/v0.2.0/firmware-ats125.bin",
 *    "size": 1714848, "sha256": "64 hex digits"}
 */
#ifndef CORE_UPDATE_CHECK_H
#define CORE_UPDATE_CHECK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A manifest is about 250 bytes. Anything over this is not one, and is not
 * read any further. */
#define UPDATE_MANIFEST_MAX_BYTES 512

/* Where the manifest of the latest release is. GitHub answers with a
 * redirect to the release's own copy, and that one with a redirect to its
 * file host. The board name and ".json" follow. */
#define UPDATE_MANIFEST_URL_PREFIX \
  "https://github.com/zeevy/tef668x-esp32/releases/latest/download/manifest-"

/* The only place a firmware image may come from: this repository's releases.
 * The tag, "/firmware-", the board name and ".bin" follow. */
#define UPDATE_IMAGE_URL_PREFIX \
  "https://github.com/zeevy/tef668x-esp32/releases/download/"

/* A version, x.y.z, each part 0 to 65535. */
typedef struct {
  uint16_t major;
  uint16_t minor;
  uint16_t patch;
} UpdateVersion;

typedef struct {
  UpdateVersion version;
  char versionText[24]; /* "65535.65535.65535" at most, in a field the
                          size the reader takes. */
  char url[160];
  uint32_t size;
  char sha256[65]; /* 64 lower case hex digits. */
} UpdateManifest;

/* Why a manifest was refused, or that it was not. */
typedef enum {
  UPDATE_MANIFEST_OK = 0,
  UPDATE_MANIFEST_TOO_LONG,      /* Over UPDATE_MANIFEST_MAX_BYTES, or empty. */
  UPDATE_MANIFEST_NOT_JSON,      /* Not one flat object of strings and
                                    whole numbers. */
  UPDATE_MANIFEST_MISSING_FIELD, /* One of the five is not there. */
  UPDATE_MANIFEST_REPEATED_FIELD, /* One of the five is there twice. */
  UPDATE_MANIFEST_BAD_VERSION,    /* Not x.y.z. */
  UPDATE_MANIFEST_WRONG_BOARD,    /* Made for another radio. */
  UPDATE_MANIFEST_BAD_SIZE,       /* 0, or bigger than the free slot. */
  UPDATE_MANIFEST_BAD_SHA256,     /* Not 64 hex digits. */
  UPDATE_MANIFEST_BAD_URL         /* Not this repository's release file for
                                     this board. */
} UpdateManifestResult;

/*
 * Read "x.y.z" into `out`. Each part is 1 to 5 digits, no more than 65535,
 * with no leading zero unless the part is 0. False for anything else, and
 * `out` is then left alone.
 */
bool updateParseVersion(const char *text, UpdateVersion *out);

/* Negative, zero or positive as `a` is older than, the same as, or newer than
 * `b`. Each part is compared as a number, so 0.10.0 is newer than 0.9.0. */
int updateCompareVersions(const UpdateVersion *a, const UpdateVersion *b);

/*
 * Check a manifest of `len` bytes, made for `board`, against an app slot of
 * `slotBytes`, and fill in `out` when it passes. A field the radio does not
 * know is passed over, so a later manifest can carry more.
 */
UpdateManifestResult updateParseManifest(const char *json, size_t len,
                                         const char *board, uint32_t slotBytes,
                                         UpdateManifest *out);

/*
 * Whether a download may go to `url`, the first address or the target of a
 * redirect: this repository's releases on github.com, or GitHub's file hosts
 * for release files. Only https, and no space or control character.
 */
bool updateUrlAllowed(const char *url);

/* What the caller can see about the radio right now. */
typedef struct {
  bool enabled; /* The Check for Updates setting is on. */
  bool online;  /* Joined to the network, not serving its own hotspot. */
  bool onTrial; /* A new firmware is still proving itself. */
  bool busy;    /* A scan, a sweep, the menu or a message screen is up. */
  uint32_t nowMs;
} UpdateDueInputs;

/*
 * A failed check is tried again this long after, as long as the radio is
 * quiet then, and this many times in one start. A check fails when the
 * network drops, or when heavy use leaves too little memory for HTTPS to
 * check GitHub's certificate. Ten minutes lets a burst of sweeps or a scan
 * end; three tries cover half an hour.
 */
#define UPDATE_RETRY_MS (10UL * 60UL * 1000UL)
#define UPDATE_RETRIES 3

/* Remembers whether the check of this start has been made, and its tries. */
typedef struct {
  bool done;
  uint8_t retries;  /* Failed checks tried again so far. */
  uint32_t retryMs; /* When the next try may start, while one is waiting. */
} UpdateDue;

void updateDueReset(UpdateDue *due);

/*
 * Whether to check now. True once per start, the first time the setting is
 * on, the radio is online, no update is on trial and nothing is busy, and
 * again after a failed check, under the same rule, UPDATE_RETRY_MS after
 * it, up to UPDATE_RETRIES times.
 */
bool updateDueStep(UpdateDue *due, const UpdateDueInputs *in);

/* A check that failed, at `nowMs`: a try is due after UPDATE_RETRY_MS while
 * any are left. */
void updateDueFailed(UpdateDue *due, uint32_t nowMs);

/* "1.7" for 1,714,848 bytes: megabytes of a million bytes, one decimal,
 * rounded. Writes "" into an `out` too small or NULL. */
void updateFormatMegabytes(uint32_t bytes, char *out, size_t outLen);

#ifdef __cplusplus
}
#endif

#endif /* CORE_UPDATE_CHECK_H */
