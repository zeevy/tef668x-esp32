/* Implementation of the littlefs-backed logbook. */
#include "logbook_fs.h"

#include <LittleFS.h>
#include <esp_random.h>
#include <string.h>

#include "core/bytes.h"
#include "core/rds.h"

/* Format 3's file. A format 1 or 2 log stays in LOGBOOK_OLD_PATH as it was:
 * copied, never changed, so firmware from before format 3, which a rollback
 * can go back to, still finds its own log there. */
#define LOGBOOK_PATH "/logbook3.bin"
#define LOGBOOK_OLD_PATH "/logbook.bin"
/* Where a copy is written before it takes LOGBOOK_PATH's place: an older log
 * copied into this format, or the log with one entry deleted. */
#define LOGBOOK_NEW_PATH "/logbook.new"
/* The old file's header and newest record as they were when it was copied.
 * Only firmware from before writes to the old file, after a rollback or a
 * downgrade, so the copy is made again when they differ, and never just
 * because an update has started. */
#define LOGBOOK_MARK_PATH "/logbook3.mark"

/*
 * Changes if the header or record layout ever does. An older file is copied
 * into this format, entry for entry; any other is started fresh rather than
 * misread.
 */
#define LOGBOOK_MAGIC 0x4C4F4731UL /* "LOG1" */
#define LOGBOOK_VERSION 3
#define LOGBOOK_VERSION_2 2 /* No note, no renamed mark. */
#define LOGBOOK_VERSION_1 1 /* No radio text either. */

#define LOGBOOK_HEADER_SIZE 10 /* magic(4) + version(2) + head(2) + count(2) */
#define LOGBOOK_MARK_MAX (LOGBOOK_HEADER_SIZE + LOGBOOK_RECORD_SIZE_V2)

static bool sPresent = false;
static LogbookRing sRing;
/* Counts every change, so a screen that keeps rows knows to read them again. */
static uint32_t sGeneration = 0;

static uint32_t recordAt(uint16_t slot) {
  return (uint32_t)LOGBOOK_HEADER_SIZE + (uint32_t)slot * LOGBOOK_RECORD_SIZE;
}

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

/* Zeroed records from where `f` is to the end of a full file, so a slot that
 * has never been written decodes to a harmless all-zero entry rather than
 * whatever the flash happened to hold. */
static bool padTo(File &f, uint16_t fromSlot) {
  static const uint8_t zero[LOGBOOK_RECORD_SIZE] = {0};
  bool ok = true;
  for (uint16_t i = fromSlot; ok && i < LOGBOOK_MAX_ENTRIES; i++) {
    ok = f.write(zero, sizeof(zero)) == sizeof(zero);
  }
  return ok;
}

/* A freshly created file: an empty ring and every slot zeroed. */
static bool createEmpty(void) {
  File f = LittleFS.open(LOGBOOK_PATH, "w");
  if (!f) {
    return false;
  }
  LogbookRing empty;
  logbookRingInit(&empty);
  const bool ok = writeHeader(f, empty) && padTo(f, 0);
  f.close();
  if (ok) {
    sRing = empty;
  }
  return ok;
}

/* The format of the file at `path`, with its ring in `ring`, or 0 when there
 * is no file there this code can read. */
static uint16_t readHeader(const char *path, LogbookRing *ring) {
  File f = LittleFS.open(path, "r");
  if (!f) {
    return 0;
  }
  uint8_t buf[LOGBOOK_HEADER_SIZE];
  bool ok = f.read(buf, sizeof(buf)) == (int)sizeof(buf) &&
            getU32(buf) == LOGBOOK_MAGIC;
  const uint16_t version = ok ? getU16(buf + 4) : 0;
  const long record = version == LOGBOOK_VERSION     ? LOGBOOK_RECORD_SIZE
                      : version == LOGBOOK_VERSION_2 ? LOGBOOK_RECORD_SIZE_V2
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

/* LOGBOOK_NEW_PATH in LOGBOOK_PATH's place. littlefs renames in one step, so
 * a power cut leaves either the old file or the new one, never half of each. */
static bool takeNew(bool ok) {
  ok = ok && LittleFS.rename(LOGBOOK_NEW_PATH, LOGBOOK_PATH);
  if (!ok) {
    LittleFS.remove(LOGBOOK_NEW_PATH);
  }
  return ok;
}

/*
 * Copy an older log into this format, slot for slot, into its own file. The
 * old file is only read: a power cut part way leaves it to copy again at the
 * next start up, and a copy that fails leaves the logbook absent until then
 * rather than started fresh over it.
 */
static bool upgrade(const LogbookRing &ring, size_t oldSize) {
  File in = LittleFS.open(LOGBOOK_OLD_PATH, "r");
  File out = LittleFS.open(LOGBOOK_NEW_PATH, "w");
  bool ok = in && out && in.seek(LOGBOOK_HEADER_SIZE) && writeHeader(out, ring);
  uint8_t old[LOGBOOK_RECORD_SIZE];
  uint8_t record[LOGBOOK_RECORD_SIZE];
  for (uint16_t i = 0; ok && i < LOGBOOK_MAX_ENTRIES; i++) {
    ok = in.read(old, oldSize) == (int)oldSize &&
         logbookUpgrade(old, oldSize, record, sizeof(record)) &&
         out.write(record, sizeof(record)) == sizeof(record);
  }
  if (in) {
    in.close();
  }
  if (out) {
    out.close();
  }
  if (!takeNew(ok)) {
    return false;
  }
  sRing = ring;
  return true;
}

/* The old file's header and newest record into `mark`: their length, or 0
 * when they cannot be read. */
static size_t oldMark(const LogbookRing &ring, size_t oldSize, uint8_t *mark) {
  File f = LittleFS.open(LOGBOOK_OLD_PATH, "r");
  if (!f) {
    return 0;
  }
  const uint16_t newest =
      ring.count == 0 ? 0 : logbookRingSlotAt(&ring, ring.count - 1);
  const bool ok = f.read(mark, LOGBOOK_HEADER_SIZE) == LOGBOOK_HEADER_SIZE &&
                  f.seek(LOGBOOK_HEADER_SIZE + (uint32_t)newest * oldSize) &&
                  f.read(mark + LOGBOOK_HEADER_SIZE, oldSize) == (int)oldSize;
  f.close();
  return ok ? LOGBOOK_HEADER_SIZE + oldSize : 0;
}

/* Whether the mark kept at the last copy equals `mark`. */
static bool sameMark(const uint8_t *mark, size_t len) {
  File f = LittleFS.open(LOGBOOK_MARK_PATH, "r");
  if (!f) {
    return false;
  }
  uint8_t kept[LOGBOOK_MARK_MAX];
  const bool same =
      f.read(kept, sizeof(kept)) == (int)len && memcmp(kept, mark, len) == 0;
  f.close();
  return same;
}

/* Keeps `mark` for the next start up to compare. Not kept, the next start
 * up copies again, which loses nothing as long as the copy is that new. */
static void keepMark(const uint8_t *mark, size_t len) {
  File f = LittleFS.open(LOGBOOK_MARK_PATH, "w");
  if (f) {
    f.write(mark, len);
    f.close();
  }
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
  /* From a random number, not from 0, so a page kept open across a restart
   * cannot take the new log's number for the one it read before. */
  sGeneration = esp_random();
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
  bool ok = true;
  LogbookRing oldRing;
  const uint16_t old = readHeader(LOGBOOK_OLD_PATH, &oldRing);
  const bool haveOld = old == LOGBOOK_VERSION_2 || old == LOGBOOK_VERSION_1;
  const size_t oldSize = old == LOGBOOK_VERSION_2 ? LOGBOOK_RECORD_SIZE_V2
                                                  : LOGBOOK_RECORD_SIZE_V1;
  uint8_t mark[LOGBOOK_MARK_MAX];
  /* An old file whose mark cannot be read is taken as unchanged: copying it
   * again would throw away this format's changes for a file that may not
   * copy anyway. */
  const size_t markLen = haveOld ? oldMark(oldRing, oldSize, mark) : 0;
  const bool oldChanged = markLen != 0 && !sameMark(mark, markLen);
  if (readHeader(LOGBOOK_PATH, &ring) == LOGBOOK_VERSION && !oldChanged) {
    sRing = ring;
  } else if (haveOld) {
    ok = upgrade(oldRing, oldSize);
    if (ok && markLen != 0) {
      keepMark(mark, markLen);
    }
  } else {
    ok = createEmpty();
  }
  sPresent = ok;
  return ok;
}

bool logbookFsPresent(void) {
  return sPresent;
}

uint32_t logbookFsGeneration(void) {
  return sGeneration;
}

/* Whether the open log holds the same station as `e`. A record that cannot
 * be read is passed over: a log that cannot be read in full is still
 * written to, rather than refusing every entry after it. */
static bool holdsSameStation(File &f, const LogbookEntry *e) {
  uint8_t record[LOGBOOK_RECORD_SIZE];
  LogbookEntry stored;
  for (uint16_t i = 0; i < sRing.count; i++) {
    const uint16_t slot = logbookRingSlotAt(&sRing, i);
    if (!f.seek(recordAt(slot)) ||
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
  f.seek(recordAt(slot));
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
    sGeneration++;
  }
  return ok ? LOGBOOK_WRITTEN : LOGBOOK_NOT_WRITTEN;
}

uint16_t logbookFsCount(void) {
  return sPresent ? sRing.count : 0;
}

uint16_t logbookFsSlotAt(uint16_t i) {
  return sPresent ? logbookRingSlotAt(&sRing, i) : LOGBOOK_MAX_ENTRIES;
}

/* The record in `slot`, into `record` and decoded into `out`. */
static bool readRecord(uint16_t slot, uint8_t *record, LogbookEntry *out) {
  File f = LittleFS.open(LOGBOOK_PATH, "r");
  if (!f) {
    return false;
  }
  const bool ok =
      f.seek(recordAt(slot)) &&
      f.read(record, LOGBOOK_RECORD_SIZE) == (int)LOGBOOK_RECORD_SIZE;
  f.close();
  return ok && logbookDecode(record, LOGBOOK_RECORD_SIZE, out);
}

bool logbookFsEach(LogbookVisit visit, void *ctx) {
  if (!sPresent || visit == NULL) {
    return false;
  }
  File f = LittleFS.open(LOGBOOK_PATH, "r");
  if (!f) {
    return false;
  }
  uint8_t record[LOGBOOK_RECORD_SIZE];
  LogbookEntry e;
  for (uint16_t i = 0; i < sRing.count; i++) {
    const uint16_t slot = logbookRingSlotAt(&sRing, i);
    if (f.seek(recordAt(slot)) &&
        f.read(record, sizeof(record)) == (int)sizeof(record) &&
        logbookDecode(record, sizeof(record), &e)) {
      visit(slot, &e, ctx);
    }
  }
  f.close();
  return true;
}

bool logbookFsEntryAt(uint16_t i, LogbookEntry *out) {
  if (!sPresent || out == NULL) {
    return false;
  }
  const uint16_t slot = logbookRingSlotAt(&sRing, i);
  uint8_t record[LOGBOOK_RECORD_SIZE];
  return slot < LOGBOOK_MAX_ENTRIES && readRecord(slot, record, out);
}

/* The entry in `slot`, checked to be the one whose time is `timeValue` and
 * frequency `freqKHz`: the ring may have moved on, or a delete renumbered it,
 * since the caller read the list. Two entries can share a second, two
 * logged together; one second and one frequency is one entry, since the log
 * keeps one entry a station. */
static LogbookChange find(uint16_t slot, uint32_t timeValue, uint32_t freqKHz,
                          uint16_t *index, uint8_t *record, LogbookEntry *e) {
  if (!sPresent) {
    return LOGBOOK_NO_LOG;
  }
  if (!logbookRingIndexOf(&sRing, slot, index)) {
    return LOGBOOK_NO_ENTRY;
  }
  if (!readRecord(slot, record, e)) {
    return LOGBOOK_NOT_READ;
  }
  return e->timeValue == timeValue && e->freqKHz == freqKHz ? LOGBOOK_CHANGED
                                                            : LOGBOOK_NOT_IT;
}

/* Copy a person's text into a fixed field, the rest zeroed, so the same
 * entry always encodes to the same bytes. */
static void putText(char *field, size_t size, const char *text) {
  memset(field, 0, size);
  strncpy(field, text, size - 1);
}

/* Whether `name` is the stored one, the spaces a station pads its name with
 * left out. */
static bool sameName(const LogbookEntry *e, const char *name) {
  char stored[LOGBOOK_NAME_LEN];
  if (!e->hasName ||
      !rdsNameTrim(e->name, sizeof(e->name), stored, sizeof(stored))) {
    stored[0] = '\0';
  }
  return strcmp(stored, name) == 0;
}

LogbookChange logbookFsEdit(uint16_t slot, uint32_t timeValue, uint32_t freqKHz,
                            const char *name, const char *note) {
  uint16_t index = 0;
  uint8_t record[LOGBOOK_RECORD_SIZE];
  LogbookEntry e;
  const LogbookChange found =
      find(slot, timeValue, freqKHz, &index, record, &e);
  if (found != LOGBOOK_CHANGED) {
    return found;
  }
  /* The received name is kept as it came, padding and all, unless another
   * one is given: only then is the entry renamed. */
  if (name != NULL && !sameName(&e, name)) {
    putText(e.name, sizeof(e.name), name);
    e.hasName = name[0] != '\0';
    e.renamed = true;
  }
  if (note != NULL) {
    putText(e.note, sizeof(e.note), note);
  }
  File f = LittleFS.open(LOGBOOK_PATH, "r+");
  const bool ok =
      f &&
      logbookEncode(&e, record, LOGBOOK_RECORD_SIZE) == LOGBOOK_RECORD_SIZE &&
      f.seek(recordAt(slot)) &&
      f.write(record, LOGBOOK_RECORD_SIZE) == LOGBOOK_RECORD_SIZE;
  if (f) {
    f.close();
  }
  if (!ok) {
    return LOGBOOK_NOT_SAVED;
  }
  sGeneration++;
  return LOGBOOK_CHANGED;
}

/*
 * The log written again without the entry, oldest first from slot 0, into
 * its own file that then takes the log's place. A rewrite rather than a mark:
 * littlefs copies a file's data from the point of any change to its end, so
 * a change near the start costs most of the file however small it is, and
 * every append already pays that for its header. The deleted entry's room is
 * free again for the next.
 */
LogbookChange logbookFsDelete(uint16_t slot, uint32_t timeValue,
                              uint32_t freqKHz) {
  uint16_t index = 0;
  uint8_t record[LOGBOOK_RECORD_SIZE];
  LogbookEntry e;
  const LogbookChange found =
      find(slot, timeValue, freqKHz, &index, record, &e);
  if (found != LOGBOOK_CHANGED) {
    return found;
  }
  LogbookRing after;
  logbookRingInit(&after);
  after.count = (uint16_t)(sRing.count - 1);
  File in = LittleFS.open(LOGBOOK_PATH, "r");
  File out = LittleFS.open(LOGBOOK_NEW_PATH, "w");
  bool ok = in && out && writeHeader(out, after);
  for (uint16_t i = 0; ok && i < sRing.count; i++) {
    if (i == index) {
      continue;
    }
    ok = in.seek(recordAt(logbookRingSlotAt(&sRing, i))) &&
         in.read(record, LOGBOOK_RECORD_SIZE) == (int)LOGBOOK_RECORD_SIZE &&
         out.write(record, LOGBOOK_RECORD_SIZE) == LOGBOOK_RECORD_SIZE;
  }
  ok = ok && padTo(out, after.count);
  if (in) {
    in.close();
  }
  if (out) {
    out.close();
  }
  if (!takeNew(ok)) {
    return LOGBOOK_NOT_SAVED;
  }
  sRing = after;
  sGeneration++;
  return LOGBOOK_CHANGED;
}
