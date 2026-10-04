/* Implementation of the littlefs-backed logbook. */
#include "logbook_fs.h"

#include <LittleFS.h>

#include "core/bytes.h"

#define LOGBOOK_PATH "/logbook.bin"
/* Where a format 1 log is copied before the copy takes its place. */
#define LOGBOOK_NEW_PATH "/logbook.new"

/*
 * Changes if the header or record layout ever does. A format 1 file is
 * copied into this format, entry for entry; any other is started fresh
 * rather than misread.
 */
#define LOGBOOK_MAGIC 0x4C4F4731UL /* "LOG1" */
#define LOGBOOK_VERSION 2
#define LOGBOOK_VERSION_1 1 /* No radio text. */

#define LOGBOOK_HEADER_SIZE 10 /* magic(4) + version(2) + head(2) + count(2) */

static bool sPresent = false;
static LogbookRing sRing;

/* Returns whether the header actually made it to flash, so a caller can
 * tell a write that stuck from one that only ran. */
static bool writeHeader(File &f, const LogbookRing &ring) {
  uint8_t buf[LOGBOOK_HEADER_SIZE];
  putU32(buf, LOGBOOK_MAGIC);
  putU16(buf + 4, LOGBOOK_VERSION);
  putU16(buf + 6, ring.head);
  putU16(buf + 8, ring.count);
  f.seek(0);
  return f.write(buf, sizeof(buf)) == sizeof(buf);
}

/* A freshly created file: an empty ring and every slot zeroed, so a slot
 * that has never been written decodes to a harmless all-zero entry rather
 * than whatever the flash happened to hold before this file existed. */
static bool createEmpty(void) {
  File f = LittleFS.open(LOGBOOK_PATH, "w");
  if (!f) {
    return false;
  }
  LogbookRing empty;
  logbookRingInit(&empty);
  bool ok = writeHeader(f, empty);
  uint8_t zero[LOGBOOK_RECORD_SIZE] = {0};
  for (uint16_t i = 0; ok && i < LOGBOOK_MAX_ENTRIES; i++) {
    ok = f.write(zero, sizeof(zero)) == sizeof(zero);
  }
  f.close();
  if (ok) {
    sRing = empty;
  }
  return ok;
}

/* The format of the file on flash, with its ring in `ring`, or 0 when there
 * is no file this code can read. */
static uint16_t readHeader(LogbookRing *ring) {
  File f = LittleFS.open(LOGBOOK_PATH, "r");
  if (!f) {
    return 0;
  }
  uint8_t buf[LOGBOOK_HEADER_SIZE];
  bool ok = f.read(buf, sizeof(buf)) == (int)sizeof(buf) &&
            getU32(buf) == LOGBOOK_MAGIC;
  const uint16_t version = ok ? getU16(buf + 4) : 0;
  const long record = version == LOGBOOK_VERSION     ? LOGBOOK_RECORD_SIZE
                      : version == LOGBOOK_VERSION_1 ? LOGBOOK_RECORD_SIZE_V1
                                                     : 0;
  ok = ok && record != 0 &&
       f.size() >= LOGBOOK_HEADER_SIZE + (long)LOGBOOK_MAX_ENTRIES * record;
  f.close();
  if (!ok) {
    return 0;
  }
  ring->head = getU16(buf + 6);
  ring->count = getU16(buf + 8);
  if (ring->count > LOGBOOK_MAX_ENTRIES || ring->head >= LOGBOOK_MAX_ENTRIES) {
    /* Not a file this format could have written. Started fresh rather than
     * trusted, the same as a magic number that does not match. */
    return 0;
  }
  return version;
}

/*
 * Copy a format 1 log into format 2, slot for slot, then put the copy in its
 * place. The old file stays whole until the rename, and littlefs renames in
 * one step, so a power cut part way leaves the old log to copy again at the
 * next start up. A copy that fails leaves the old log where it is, and the
 * logbook absent until then, rather than started fresh over it.
 */
static bool upgradeV1(const LogbookRing &ring) {
  File in = LittleFS.open(LOGBOOK_PATH, "r");
  File out = LittleFS.open(LOGBOOK_NEW_PATH, "w");
  bool ok = in && out && in.seek(LOGBOOK_HEADER_SIZE) && writeHeader(out, ring);
  uint8_t old[LOGBOOK_RECORD_SIZE_V1];
  uint8_t record[LOGBOOK_RECORD_SIZE];
  for (uint16_t i = 0; ok && i < LOGBOOK_MAX_ENTRIES; i++) {
    ok = in.read(old, sizeof(old)) == (int)sizeof(old) &&
         logbookUpgradeV1(old, record, sizeof(record)) &&
         out.write(record, sizeof(record)) == sizeof(record);
  }
  in.close();
  out.close();
  ok = ok && LittleFS.rename(LOGBOOK_NEW_PATH, LOGBOOK_PATH);
  if (!ok) {
    LittleFS.remove(LOGBOOK_NEW_PATH);
    return false;
  }
  sRing = ring;
  return true;
}

/*
 * The partition's own name in partitions.csv, not LittleFS.begin's default
 * of "spiffs". That default matches a partition literally named `spiffs`,
 * which is a label, not the SubType column; this one is named `littlefs`
 * and would otherwise never be found at all, mount failure or not.
 */
#define LOGBOOK_PARTITION_LABEL "littlefs"

bool logbookFsBegin(void) {
  sPresent = false;
  if (!LittleFS.begin(false, "/littlefs", 10, LOGBOOK_PARTITION_LABEL)) {
    /* Not formatted yet, which a partition that has never held anything
     * looks like. One attempt to format and mount, not a silent retry
     * loop: a partition that will not mount should say so rather than spend
     * start up trying. */
    if (!LittleFS.begin(true, "/littlefs", 10, LOGBOOK_PARTITION_LABEL)) {
      return false;
    }
  }
  LogbookRing ring;
  const uint16_t version = readHeader(&ring);
  if (version == LOGBOOK_VERSION) {
    sRing = ring;
  } else if (!(version == LOGBOOK_VERSION_1 ? upgradeV1(ring)
                                            : createEmpty())) {
    return false;
  }
  sPresent = true;
  return true;
}

bool logbookFsPresent(void) {
  return sPresent;
}

/* Whether the open log holds the same station as `e`. A record that cannot
 * be read is passed over: a log that cannot be read in full is still
 * written to, rather than refusing every entry after it. */
static bool holdsSameStation(File &f, const LogbookEntry *e) {
  uint8_t record[LOGBOOK_RECORD_SIZE];
  LogbookEntry stored;
  for (uint16_t i = 0; i < sRing.count; i++) {
    const uint16_t slot = logbookRingSlotAt(&sRing, i);
    if (!f.seek((uint32_t)LOGBOOK_HEADER_SIZE +
                (uint32_t)slot * LOGBOOK_RECORD_SIZE) ||
        f.read(record, sizeof(record)) != (int)sizeof(record) ||
        !logbookDecode(record, sizeof(record), &stored)) {
      continue;
    }
    if (logbookSameStation(&stored, e)) {
      return true;
    }
  }
  return false;
}

LogbookWrite logbookFsAppend(const LogbookEntry *e) {
  if (!sPresent || e == NULL) {
    return LOGBOOK_NOT_WRITTEN;
  }
  uint8_t record[LOGBOOK_RECORD_SIZE];
  if (logbookEncode(e, record, sizeof(record)) != LOGBOOK_RECORD_SIZE) {
    return LOGBOOK_NOT_WRITTEN;
  }

  /*
   * Worked out against a copy first, and only written into `sRing` once
   * the record and the header it depends on are both actually on flash.
   * Committing the slot here and then failing to open or write the file
   * would leave the ring believing an entry exists, or believing the real
   * oldest one was dropped, when neither write ever happened.
   */
  LogbookRing trial = sRing;
  uint16_t slot = logbookRingAppend(&trial);

  File f = LittleFS.open(LOGBOOK_PATH, "r+");
  if (!f) {
    return LOGBOOK_NOT_WRITTEN;
  }
  if (holdsSameStation(f, e)) {
    f.close();
    return LOGBOOK_ALREADY_THERE;
  }
  f.seek((uint32_t)LOGBOOK_HEADER_SIZE + (uint32_t)slot * LOGBOOK_RECORD_SIZE);
  bool ok = f.write(record, sizeof(record)) == sizeof(record);
  /*
   * The header, which is what a reboot trusts to say how many entries
   * there are, is written before `sRing` changes to match it. A record
   * whose bytes reached flash but whose header write did not is not a
   * write that survives a power cycle, so it must not be counted as one
   * even though logbookFsCount would otherwise say it was there.
   */
  ok = ok && writeHeader(f, trial);
  f.close();
  if (ok) {
    sRing = trial;
  }
  return ok ? LOGBOOK_WRITTEN : LOGBOOK_NOT_WRITTEN;
}

uint16_t logbookFsCount(void) {
  return sPresent ? sRing.count : 0;
}

bool logbookFsEntryAt(uint16_t i, LogbookEntry *out) {
  if (!sPresent || out == NULL) {
    return false;
  }
  uint16_t slot = logbookRingSlotAt(&sRing, i);
  if (slot >= LOGBOOK_MAX_ENTRIES) {
    return false;
  }
  File f = LittleFS.open(LOGBOOK_PATH, "r");
  if (!f) {
    return false;
  }
  f.seek((uint32_t)LOGBOOK_HEADER_SIZE + (uint32_t)slot * LOGBOOK_RECORD_SIZE);
  uint8_t record[LOGBOOK_RECORD_SIZE];
  bool ok = f.read(record, sizeof(record)) == (int)sizeof(record);
  f.close();
  return ok && logbookDecode(record, sizeof(record), out);
}
