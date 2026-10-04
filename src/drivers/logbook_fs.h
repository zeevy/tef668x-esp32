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
 * a file rewritten: a slot is overwritten in place, never inserted or
 * deleted, so a write here costs one seek and LOGBOOK_RECORD_SIZE bytes
 * however many entries the log already holds.
 */
#ifndef DRIVERS_LOGBOOK_FS_H
#define DRIVERS_LOGBOOK_FS_H

#include "core/logbook.h"

/*
 * Mount littlefs and open the log, creating it if it is not there yet.
 *
 * A format 1 file, from before the radio text, is copied into the current
 * format with every entry kept. Any other file that does not start with the
 * magic number and a version this code knows is treated the same as one
 * that is not there: started fresh rather than read as whatever the bytes
 * happen to decode to. Call once, at start up.
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

/*
 * The i-th oldest entry, i counted from 0.
 *
 * False if i is not less than logbookFsCount() or the record on flash does
 * not decode, which for a file this module wrote itself should never
 * happen, but a truncated write from a power loss mid-write is exactly the
 * case a portable radio has to survive being asked about.
 */
bool logbookFsEntryAt(uint16_t i, LogbookEntry *out);

#endif /* DRIVERS_LOGBOOK_FS_H */
