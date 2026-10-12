/*
 * Keeps the logbook on the littlefs partition.
 *
 * This lives in drivers/ and not in core/ because it touches LittleFS.
 * The ring, the record format and the CSV lines are all in core/logbook.h
 * and are tested there; what is here is only where the bytes actually sit.
 *
 * One file, fixed size from the moment it is created: a ten byte header
 * (a magic number, a format version, and the ring's head and count) followed
 * by LOGBOOK_MAX_ENTRIES fixed size slots. Fixed size is what makes dropping
 * the oldest entry a house-keeping index moved by core/logbook.c rather than
 * a file rewritten: an append overwrites one slot in place. Deleting an entry
 * writes the log again without it, oldest first from slot 0, so its room is
 * free for the next and every entry's slot changes.
 */
#ifndef DRIVERS_LOGBOOK_FS_H
#define DRIVERS_LOGBOOK_FS_H

#include "core/logbook.h"

/*
 * Mount littlefs and open the log, creating it if it is not there yet.
 *
 * An older file, format 3 from before the programme type and the country
 * code, or else format 1 or 2 from before the note, is copied into the
 * current format, in a file of its own, with every entry kept in its ring
 * order; the old file is left as it was, for firmware from before to find
 * after a rollback. It is copied again only when it has changed since the
 * last copy, which means firmware from before has logged to it. Any other
 * file that does not start with the magic number and a version this code
 * knows is treated the same as one that is not there: started fresh rather
 * than read as whatever the bytes happen to decode to. Call once, at start
 * up.
 */
bool logbookFsBegin(void);

/*
 * Whether logbookFsBegin found or made a working file.
 *
 * False means every other call here is a no-op that reports failure rather
 * than a hidden write to a filesystem that never mounted, which is what
 * silent data loss on a portable would otherwise look like.
 */
bool logbookFsPresent(void);

/*
 * Add one entry, dropping the oldest if the store is already full.
 *
 * Nothing is written when the log already holds the same station,
 * logbookSameStation, anywhere in it: the log keeps one entry a station, and
 * every writer comes through here. That reads every stored entry, one file
 * open for all of them, on the loop task.
 *
 * `e` is copied through logbookEncode before anything touches the file, so
 * a caller that filled in an entry does not have to know the store is a
 * fixed size ring underneath.
 */
LogbookWrite logbookFsAppend(const LogbookEntry *e);

/* How many entries are stored right now, 0 to LOGBOOK_MAX_ENTRIES. */
uint16_t logbookFsCount(void);

/* Goes up with every append, edit and delete, so a screen that keeps rows of
 * the log knows when to read them again: the count alone can stay the same
 * across a change. */
uint32_t logbookFsGeneration(void);

/*
 * The i-th oldest entry, i counted from 0.
 *
 * False if i is not less than logbookFsCount() or the record on flash does
 * not decode, which for a file this module wrote itself should never
 * happen, but a truncated write from a power loss mid-write is exactly the
 * case a portable radio has to survive being asked about. Opens the file for
 * the one entry; logbookFsEach reads them all through one open.
 */
bool logbookFsEntryAt(uint16_t i, LogbookEntry *out);

/* The slot the i-th entry is in, the same i as logbookFsEntryAt, or
 * LOGBOOK_MAX_ENTRIES. A web page names an entry by it, with its time and
 * frequency as the check; a delete changes every entry's slot. */
uint16_t logbookFsSlotAt(uint16_t i);

/*
 * Every entry, oldest first, read through one open file: `visit` is called
 * with each one's slot and the entry. A record that cannot be read is passed
 * over. False when the log is not there or will not open.
 */
typedef void (*LogbookVisit)(uint16_t slot, const LogbookEntry *e, void *ctx);
bool logbookFsEach(LogbookVisit visit, void *ctx);

/* What came of changing or deleting an entry. */
typedef enum {
  LOGBOOK_CHANGED = 0,
  LOGBOOK_NO_LOG,    /* The log is not there: the partition did not mount. */
  LOGBOOK_NO_ENTRY,  /* The ring holds no entry in that slot. */
  LOGBOOK_NOT_IT,    /* The slot holds another entry: time or kHz differ. */
  LOGBOOK_NOT_READ,  /* The entry could not be read. */
  LOGBOOK_NOT_SAVED, /* The file did not take the write. */
} LogbookChange;

/*
 * Change the name, the note or both of the entry in `slot`, whose time is
 * `timeValue` and frequency `freqKHz`; NULL leaves one as it is. A name
 * other than the one stored marks the entry renamed. The time and the
 * frequency are the check that the slot still holds the entry the caller
 * read: the ring may have moved on, or a delete renumbered it, since. The
 * texts are the caller's to check, with logbookTextFits.
 */
LogbookChange logbookFsEdit(uint16_t slot, uint32_t timeValue, uint32_t freqKHz,
                            const char *name, const char *note);

/* Delete the entry in `slot`, checked as for an edit: the log is written
 * again without it, oldest first, so every slot changes. */
LogbookChange logbookFsDelete(uint16_t slot, uint32_t timeValue,
                              uint32_t freqKHz);

#endif /* DRIVERS_LOGBOOK_FS_H */
