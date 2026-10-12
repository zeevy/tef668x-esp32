/*
 * A record of what was heard and when.
 *
 * An entry is written once and holds everything the radio knew at that
 * moment. Its readings are never changed after, which is what tells it apart
 * from a memory channel: a channel is somewhere to go back to and a person
 * chooses and names it, an entry is something that happened and is worth
 * keeping because of when it happened. From the web page a person can still
 * correct its name, which then says it was renamed, write a note on it, or
 * delete it.
 *
 * Nothing here touches a file or the network. The ring math and the record
 * format are pure, so they build and are tested on a PC; where the bytes
 * actually live is drivers/logbook_fs.h.
 */
#ifndef CORE_LOGBOOK_H
#define CORE_LOGBOOK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * How many entries the store holds.
 *
 * A DX log is a rolling record of recent catches, not an archive, so the
 * oldest is dropped rather than a write ever being refused. At the packed
 * record size below, 250 of them and the file's ten byte header are 34,260
 * bytes, a small slice of the 2 MB littlefs partition.
 */
#define LOGBOOK_MAX_ENTRIES 250

/* RDS station name, 8 characters plus the terminator. Matches RDS_PS_LEN. */
#define LOGBOOK_NAME_LEN 9

/* RDS radio text, 64 characters plus the terminator. Matches RDS_RT_LEN. */
#define LOGBOOK_RT_LEN 65

/* A person's own note on an entry, 32 characters plus the terminator. */
#define LOGBOOK_NOTE_LEN 33

/*
 * The exact size of one packed record, in bytes.
 *
 * Fixed by logbookEncode, not by sizeof(LogbookEntry): the struct below is
 * for a caller to fill in, and the padding a compiler puts in it is not the
 * same on every target that might read this file back. A new field needs
 * a new format version in drivers/logbook_fs.h and a way to read the old
 * records, which logbookUpgrade is.
 */
#define LOGBOOK_RECORD_SIZE 137

/* The formats before: format 1 had no radio text, format 2 no note and no
 * renamed mark, format 3 no programme type and no country code. Each is the
 * start of the one after, the same bytes in the same place. */
#define LOGBOOK_RECORD_SIZE_V1 35
#define LOGBOOK_RECORD_SIZE_V2 101
#define LOGBOOK_RECORD_SIZE_V3 135

/* One catch, everything the radio knew about it at the moment it was held. */
typedef struct {
  /*
   * False means `timeValue` is milliseconds since boot, not a real time.
   * NTP gives the right time only when the radio has joined a network since
   * it booted, which for a portable is often not true, and an entry with no
   * time at all is worth far less than one that says plainly it is only
   * counting from power on.
   */
  bool timeKnown;
  uint32_t timeValue; /* UTC epoch seconds if timeKnown, else millis(). */
  uint8_t band;       /* BandId, kept as a plain number so this needs no
                          include of band_plan.h to decode. */
  uint32_t freqKHz;
  int16_t levelDbuVTenths;
  uint16_t usnTenths;       /* FM ultrasonic noise. 0 off FM. */
  uint16_t multipathTenths; /* FM multipath. 0 off FM. */
  uint16_t coChannelTenths; /* AM co-channel interference. 0 off AM. */
  int8_t snrDb;
  bool stereo;
  uint16_t bandwidthKHz;
  bool hasName;
  char name[LOGBOOK_NAME_LEN];
  bool hasPi;
  uint16_t pi;
  /* The station's radio text, only when a whole one had been heard from the
   * channel's own station and Log Radio Text is on. */
  bool hasRt;
  char rt[LOGBOOK_RT_LEN];
  /* A person's note, empty for none. Format 3. */
  char note[LOGBOOK_NOTE_LEN];
  /* The name was typed by a person, not received: a log keeps the two
   * apart, since a received name is evidence of what was heard. Format 3. */
  bool renamed;
  /* The programme type the station sent, 1 to 31, and its extended country
   * code, which with the PI names the country. Each only when heard from the
   * channel's own station. Format 4. */
  bool hasPty;
  uint8_t pty;
  bool hasEcc;
  uint8_t ecc;
} LogbookEntry;

/*
 * Where entries are in the store, oldest to newest.
 *
 * A ring rather than a list, so dropping the oldest once the store is full
 * is moving one index rather than rewriting the file: `head` is the slot
 * holding the oldest entry, and appending past capacity moves `head` on
 * instead of ever growing past LOGBOOK_MAX_ENTRIES.
 */
typedef struct {
  uint16_t head;  /* Index of the oldest entry. Meaningless while count is 0. */
  uint16_t count; /* How many entries are stored, 0 to LOGBOOK_MAX_ENTRIES. */
} LogbookRing;

void logbookRingInit(LogbookRing *ring);

/* What came of adding an entry to the log. */
typedef enum {
  LOGBOOK_WRITTEN = 0,
  LOGBOOK_ALREADY_THERE, /* The same station is in the log; nothing written. */
  LOGBOOK_NOT_WRITTEN,   /* The store did not take it. */
} LogbookWrite;

/*
 * Whether two entries are the same station, so the log keeps one entry for
 * it: the same band and frequency, and the same PI when both have one. Two
 * stations can share a channel, and only the PI tells them apart; with a PI
 * on one side only, nothing does, so the channel decides. False when either
 * is NULL.
 */
bool logbookSameStation(const LogbookEntry *a, const LogbookEntry *b);

/*
 * Where the next entry goes.
 *
 * Grows the ring while there is room. Once full, the oldest entry is
 * dropped by moving `head` on rather than the write ever being refused,
 * which is decided here once rather than at every caller: a log this
 * small is worth more full of recent catches than empty because the 251st
 * one was turned away.
 */
uint16_t logbookRingAppend(LogbookRing *ring);

/*
 * The slot holding the i-th oldest entry, i counted from 0.
 *
 * Returns LOGBOOK_MAX_ENTRIES, an impossible slot, if i is not less than
 * count, so a caller that walks off the end gets something a bounds check
 * catches rather than another entry's slot handed back by accident.
 */
uint16_t logbookRingSlotAt(const LogbookRing *ring, uint16_t i);

/* The reverse: which entry, counted from the oldest, `slot` holds, into
 * `index`. False when the ring does not hold that slot, which a slot named
 * by a list read before the ring moved on can be, or for NULL. */
bool logbookRingIndexOf(const LogbookRing *ring, uint16_t slot,
                        uint16_t *index);

/*
 * Pack one entry into its fixed size on-flash record.
 *
 * Returns LOGBOOK_RECORD_SIZE on success, 0 if `cap` is too small or either
 * pointer is NULL. Byte order and layout are chosen here, by hand, rather
 * than left to sizeof(LogbookEntry) and a struct copy, so the format does
 * not change under this file just because a compiler flag does.
 */
size_t logbookEncode(const LogbookEntry *e, uint8_t *out, size_t cap);

/*
 * The reverse of logbookEncode.
 *
 * False if `len` is short of LOGBOOK_RECORD_SIZE or either pointer is NULL.
 * There is no other way for this to fail: every byte pattern of the right
 * length decodes to some entry, because every field here is a plain number
 * with nothing in it that could be malformed.
 */
bool logbookDecode(const uint8_t *in, size_t len, LogbookEntry *out);

/*
 * A record of an earlier format, `len` bytes, as one of this format, of
 * LOGBOOK_RECORD_SIZE: the same entry with what the old format had no room
 * for left empty. Each format keeps the one before's bytes in the same place,
 * so this copies them and zeroes the rest. False if either pointer is NULL,
 * `len` is longer than a record or `cap` is short.
 */
bool logbookUpgrade(const uint8_t *in, size_t len, uint8_t *out, size_t cap);

/*
 * Whether a person's text fits a field: at most `max` characters, each
 * printable ASCII, so an exported line stays one line and the panel's font
 * has every character. Empty is allowed. False for NULL.
 */
bool logbookTextFits(const char *text, size_t max);

const char *logbookCsvHeader(void);

/*
 * What an entry is called on the panel's Station Log: its name with the
 * spaces a station pads it with taken off, else "PI 26FF", else its band,
 * so a row is never blank. Always writes something that fits `len`; an
 * empty string only for a NULL entry or a zero `len`.
 */
void logbookEntryLabel(const LogbookEntry *e, char *out, size_t len);

/*
 * The entry's frequency with its unit, "98.30 MHz" or "11990 kHz", as the
 * Station Log shows it. False, and an empty string, for a band the plan
 * does not know.
 */
bool logbookEntryFrequency(const LogbookEntry *e, char *out, size_t len);

/*
 * One entry as a line of CSV, in local time.
 *
 * `offsetMinutes` is the clock offset setting at the moment of export, not
 * whatever was in force when the entry was written, so a person who reads
 * the log after moving time zones sees every entry in the zone they are
 * asking from now, the same as the panel clock already does. Ignored, and
 * UTC is written instead, when the entry's time is not known.
 *
 * A missing name, identifier, radio text or programme type is an empty
 * field, which is how CSV already says "no value" without a placeholder that
 * could be mistaken for one. The country, last, is the ISO code the PI and
 * the ECC name together, empty without both.
 *
 * Returns the length that would have been written, the same convention
 * snprintf uses, so a caller can tell a line that did not fit from one that
 * used the whole buffer exactly.
 */
size_t logbookCsvLine(const LogbookEntry *e, int16_t offsetMinutes, char *out,
                      size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* CORE_LOGBOOK_H */
