/*
 * The FM DX catches: this session's list, and the PIs ever caught.
 *
 * A catch is a PI confirmed as the tuned channel's own, `dxPiConfirmed` in
 * dx.h, while DX mode is open. The list lives in RAM for the session. The
 * set of PIs ever caught is kept in littlefs, so a PI never caught before
 * can be marked NEW. Pure logic: where the bytes live is drivers/dx_seen_fs.h.
 */
#ifndef CORE_DX_CATCH_H
#define CORE_DX_CATCH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "logbook.h"

#ifdef __cplusplus
extern "C" {
#endif

/* How many catches a session holds. Past this the oldest goes. */
#define DX_CATCHES_MAX 32

/* A moment on the radio's clock: UTC seconds when the time is known, and
 * milliseconds since boot when it is not, as the logbook keeps it. */
typedef struct {
  bool known;
  uint32_t value;
} DxTime;

/* The station's own RDS flags as last heard, for the CSV export. Each has
 * its own `has`, so a flag not heard yet is told apart from one heard off.
 * The CSV cannot keep that apart for TA and TP, see dxCatchCsvLine. */
typedef struct {
  bool hasPty;
  uint8_t pty;
  bool hasFlags;
  bool tp;
  bool ta;
  bool hasEcc;
  uint8_t ecc;
} DxRdsFlags;

/* The readings a catch was heard best with, kept for its log entry. */
typedef struct {
  int16_t levelDbuVTenths;
  uint16_t usnTenths;
  uint16_t multipathTenths;
  int8_t snrDb;
  bool stereo;
  uint16_t bandwidthKHz;
} DxReadings;

typedef struct {
  uint16_t id; /* Given once when first caught, never 0, so a page can name
                * this catch however the list moves. */
  uint32_t khz;
  uint8_t band; /* A BandId: DX mode is on FM or OIRT. */
  uint16_t pi;
  bool hasPs;
  char ps[LOGBOOK_NAME_LEN];
  bool hasCountry;
  char country[3];
  DxRdsFlags rds;
  DxReadings best; /* Its strongest on its own channel at the width last
                     * heard, for the page. */
  /* The strongest hearing on its own channel that had a time, and that
   * time, for the CSV, since the converter skips a row with none. A hearing
   * before the clock was set never counts here, and `best` does not care
   * about the time at all. */
  DxReadings timed;
  DxTime timedAt;
  uint16_t count; /* How many times it was confirmed, a retune apart. */
  uint32_t visit; /* The dial visit it was last heard on, DxHearing's. */
  DxTime last;    /* When it was last heard. */
  bool isNew;     /* Its PI had never been caught before. */
  bool logged;    /* Written to the logbook this session, on this channel. */
  /* Caught NEW while the auto log was off, and put in the seen set then:
   * the auto log never writes it, even once switched back on. */
  bool autoSkip;
  /* What its next log entry holds: the strongest hearing on its own channel
   * since the last entry, and when that was, so an entry never pairs one
   * moment's readings with another moment's time or another channel's
   * frequency. `pending` is false while nothing has been heard there since
   * the last entry, which is only ever after one was written. */
  bool pending;
  DxReadings entry;
  DxTime entryAt;
} DxCatch;

/* Newest first: item[0] is the one heard most recently. */
typedef struct {
  DxCatch item[DX_CATCHES_MAX];
  uint8_t count;
  /* How many were dropped off the end this session to make room, so the
   * page can say the list is not the whole session. */
  uint16_t dropped;
  uint16_t lastId; /* The id the newest catch was given. */
} DxCatches;

/* One confirmation, as the caller saw it. */
typedef struct {
  uint32_t khz;
  uint8_t band; /* A BandId. */
  uint16_t pi;
  const char *ps;      /* The station's name, or NULL when none has come. */
  const char *country; /* rdsCountryCode, or NULL. */
  DxRdsFlags rds;
  DxReadings readings;
  DxTime at;
  /* Which stay of the dial on a channel this is. Hearings on one visit are
   * one confirmation however often the PI comes and goes, so two stations
   * taking turns on a channel do not count each other up. */
  uint32_t visit;
} DxHearing;

/* Empties a catches list. Only the unit tests use it, to build a list of
 * their own. The DX session clears its list in dxSessionReset. */
void dxCatchesReset(DxCatches *list);

/*
 * A PI newly confirmed on a channel. Returns the catch it went into, which
 * is now item[0], or NULL for a NULL argument.
 *
 * The same PI on the same channel is the same catch, and counts up when the
 * hearing is on another visit of the dial. The same PI 100 kHz away is the
 * same catch too, kept on whichever channel it was heard stronger on: the
 * channel beside a strong station can decode that station's RDS, as
 * measured on this radio, and this is the second guard against logging it
 * there, since a NEW catch that moves is logged again on the channel it
 * moved to. A new catch goes at the front, and a full list drops its
 * oldest. `piIsNew` is whether the PI had ever been caught before, and is
 * kept from the first hearing.
 */
DxCatch *dxCatchesAdd(DxCatches *list, const DxHearing *h, bool piIsNew);

/*
 * Where the catch of `pi` heard on `khz` is in the list: the one with that
 * PI on that very channel, else the first on the channel beside it, the
 * same rule dxCatchesAdd merges by. -1 when there is none, or for a NULL
 * list.
 */
int16_t dxCatchesFind(const DxCatches *list, uint16_t pi, uint32_t khz);

/* Where the catch with that `id` is, -1 when it is not in the list or for
 * a NULL list. */
int16_t dxCatchesFindId(const DxCatches *list, uint16_t id);

/* Where the catch of `pi` on exactly `khz` is, -1 when there is none or
 * for a NULL list. */
int16_t dxCatchesFindExact(const DxCatches *list, uint16_t pi, uint32_t khz);

/*
 * The same catch heard again while the dial stays on it: a name or a
 * country that has arrived since, and on its own channel a stronger
 * reading, or any reading at a new width. Counts nothing. Does nothing
 * unless item[0] is this PI on this channel or the one beside it, and moves
 * it to the channel it is heard stronger on, as dxCatchesAdd does. A NEW
 * catch that moves is due a log entry again, since the entry it has names
 * the other channel.
 */
void dxCatchesRefresh(DxCatches *list, const DxHearing *h);

/*
 * Whether the auto log writes a catch: its PI had never been caught before,
 * it was not caught with the auto log off, and it has no entry on this
 * channel yet. Only NEW catches, so each station goes in the logbook once,
 * ever, and the locals heard every evening do not push the entries logged
 * by hand out of its 250. Any other catch is logged by hand.
 */
bool dxCatchDueLog(const DxCatch *k);

/* The logbook entry for a catch: its strongest hearing on its own channel
 * since the last entry, at the width last heard, with that hearing's time. */
void dxCatchToLog(const DxCatch *k, LogbookEntry *out);

/* The places of `list`'s catches in the order of `timedAt`, oldest first,
 * the ones with no time before any with one, into `order`, which holds
 * DX_CATCHES_MAX. Returns how many, 0 for a NULL argument. */
uint8_t dxCatchesByTime(const DxCatches *list, uint8_t *order);

/*
 * The TEF CSV, the logbook format of the PE5PVB firmware, which the FMLIST
 * converter CSVtoURDS takes as is: the header, and one catch as a line.
 *
 * Its strongest hearing on its own channel that had a time, `timed` and
 * `timedAt`: the level and stereo are that hearing's, while the name, PTY,
 * TP, TA and ECC are the latest heard. The Catches page shows `best` and
 * the time it was last heard instead. The date is DD-MM-YYYY and the time
 * HH:MM:SS, both UTC, because the converter reads a date with hyphens as
 * day, month, year and the time as UTC. A catch heard before the clock was
 * set has both empty and the level of `best`, a row the converter skips,
 * rather than a time it would read as now. The converter splits on every
 * comma and knows no quotes, so a comma or a double quote in the name
 * becomes a space. TA and TP are a dot or a space, as the converter reads
 * them, and a flag not yet heard is a space, which it reads as off: the
 * format has no third value. The radio text column is empty: a catch keeps
 * no radio text.
 *
 * The line returns its length as snprintf does, 0 for a NULL argument or a
 * band that is not a real one. A length of `cap` or more means it did not
 * fit.
 */
const char *dxCatchCsvHeader(void);

size_t dxCatchCsvLine(const DxCatch *k, char *out, size_t cap);

/* Where a log entry goes, and what came of it. */
typedef LogbookWrite (*DxLogWrite)(void *ctx, const LogbookEntry *e);

typedef enum {
  DX_WRITE_DONE,
  DX_WRITE_NO_CATCH,    /* No such catch. */
  DX_WRITE_NOTHING_NEW, /* Nothing heard on its channel since its entry. */
  DX_WRITE_IN_LOG,      /* The log holds this station already. */
  DX_WRITE_FAILED,      /* The log did not take it. */
} DxWriteResult;

/* Write a catch's entry now, due or not, and mark it written when `write`
 * says it was, or that the log holds the station already. With nothing
 * heard since the last entry `write` is not asked, since the entry would be
 * a copy of the one already in the log. */
DxWriteResult dxCatchWrite(DxCatch *k, DxLogWrite write, void *ctx);

/*
 * The PIs ever caught, in the order they were first caught.
 *
 * 512 of them, a kilobyte. Past that the one caught longest ago is dropped,
 * so a PI can only be marked NEW wrongly after 512 others have been caught
 * since it.
 */
#define DX_SEEN_MAX 512
#define DX_SEEN_BYTES (8 + DX_SEEN_MAX * 2)

typedef struct {
  uint16_t pi[DX_SEEN_MAX];
  uint16_t count;
  uint16_t next; /* Where the next goes once full. */
} DxSeen;

void dxSeenReset(DxSeen *s);
bool dxSeenHas(const DxSeen *s, uint16_t pi);

/* Add a PI. True when it was not there before, so the set wants saving. */
bool dxSeenAdd(DxSeen *s, uint16_t pi);

/* The set as bytes for the file, DX_SEEN_BYTES of them, and back. Encode
 * returns the length, 0 when `cap` is short. Decode is false for bytes this
 * format did not write, and leaves `out` empty. */
size_t dxSeenEncode(const DxSeen *s, uint8_t *out, size_t cap);
bool dxSeenDecode(const uint8_t *in, size_t len, DxSeen *out);

/*
 * A DX session: the catches, the PIs ever caught, and the catch the dial is
 * on. Zeroed is a fresh one.
 *
 * A NEW catch, dxCatchDueLog, is written to the log once its name has
 * arrived, which is some groups after the PI, so a radio switched off on it
 * still has it. With no name it is written when the dial leaves it or DX
 * mode closes. A write at the name that fails is not tried again on every
 * poll, only on leaving.
 */
typedef struct {
  DxCatches catches;
  DxSeen seen;
  bool seenKnown; /* The seen set was read. Until then nothing is NEW. */
  /* The seen set gained a PI that is not saved yet. A PI goes in only once
   * its NEW entry is in the log, so a radio switched off between the two
   * still finds it NEW and logs it next time. */
  bool seenDirty;
  bool seenTried;   /* A save of the set as it is now was tried and failed. */
  uint32_t dialKHz; /* The channel tuned at the last look. */
  uint32_t visit;   /* Counts up each time the dial moves. */
  bool on;          /* The dial is on a catch, item[0]. */
  uint16_t onPi;
  bool onTried; /* The write at the name has been tried. */
  /* The DX Scanner menu's auto log switched off: nothing is written unless by
   * hand, and a NEW catch goes into the seen set when heard, since nothing
   * will log it. Zeroed is on, which is the default. */
  bool autoLogOff;
  /* Learning the locals: every PI heard goes into the seen set at once, as
   * caught, with no log entry and no place in the catches list, so the
   * first evening of DX does not log every local. */
  bool learning;
} DxSession;

void dxSessionReset(DxSession *s);

/*
 * One look at the radio. `tunedKHz` is the dial and `h` the PI heard now as
 * the channel's own, `dxPiHeardNow` in dx.h, or NULL when there is none;
 * its `visit` is ignored and set here. Writes through `write` whatever is
 * due: the catch the dial left, and the one it is on once its name has
 * come. A PI never caught before is NEW only while the seen set is known.
 * `seenDirty` says when the set wants saving; a set that was never read
 * never does, since saving it would write it empty over the real one.
 */
void dxSessionHear(DxSession *s, uint32_t tunedKHz, const DxHearing *h,
                   DxLogWrite write, void *ctx);

/* DX mode is closing: write every catch that is due, oldest first, since
 * nothing will see the dial leave the one it is on now. That one stays the
 * catch the dial is on, so opening DX mode again on it is not a second
 * hearing. */
void dxSessionClose(DxSession *s, DxLogWrite write, void *ctx);

/* Write catch `index` now, due or not: a hold on its Catches row. A NEW
 * catch goes into the seen set once written, as the auto log's do. */
DxWriteResult dxSessionWrite(DxSession *s, uint8_t index, DxLogWrite write,
                             void *ctx);

/*
 * The seen set has been read into `s->seen`. Every catch heard before it
 * could be is decided now: one whose PI is not in it is NEW, and is written
 * through `write` if due, oldest first, except a nameless one on the dial,
 * which waits for its name; one already logged by hand goes in the set.
 */
void dxSessionSeenLoaded(DxSession *s, DxLogWrite write, void *ctx);

/* An ordinary log entry with PI `pi` on `khz` was written, a held ENTER:
 * the catch of that PI on that very channel is logged, so the auto log does
 * not write the same station again. A station not caught yet is left to
 * the catches, and NEW stays what it says, never caught in DX mode. */
void dxSessionNoteLogged(DxSession *s, uint32_t khz, uint16_t pi);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DX_CATCH_H */
