/* Implementation of the FM DX catches. */
#include "dx_catch.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "band_plan.h"
#include "bytes.h"
#include "signal.h"

/* How far apart two channels can be and still be one catch: the channel
 * beside a station, and no further. */
#define DX_SHOULDER_KHZ 100

/* "DXS1", then the count and the next slot, then the PIs. */
#define DX_SEEN_MAGIC 0x31535844UL

void dxCatchesReset(DxCatches *list) {
  if (list != NULL) {
    memset(list, 0, sizeof(*list));
  }
}

static void copyName(char *to, size_t cap, bool *has, const char *from) {
  if (from == NULL) {
    return;
  }
  strncpy(to, from, cap - 1);
  to[cap - 1] = '\0';
  *has = true;
}

/* Whether reading `now` takes over from `was`: louder, or at another width,
 * where a level is no measure against one at this width and the latest
 * width's readings take over. */
static bool takesOver(const DxReadings *was, const DxReadings *now) {
  return now->bandwidthKHz != was->bandwidthKHz ||
         now->levelDbuVTenths > was->levelDbuVTenths;
}

/* Each flag the hearing has replaces the one kept; one it lacks leaves it. */
static void takeFlags(DxRdsFlags *k, const DxRdsFlags *h) {
  if (h->hasPty) {
    k->hasPty = true;
    k->pty = h->pty;
  }
  if (h->hasFlags) {
    k->hasFlags = true;
    k->tp = h->tp;
    k->ta = h->ta;
  }
  if (h->hasEcc) {
    k->hasEcc = true;
    k->ecc = h->ecc;
  }
}

static void takeHearing(DxCatch *k, const DxHearing *h) {
  copyName(k->ps, sizeof(k->ps), &k->hasPs, h->ps);
  copyName(k->country, sizeof(k->country), &k->hasCountry, h->country);
  takeFlags(&k->rds, &h->rds);
  k->last = h->at;
  /* Readings only from its own channel: the row and the log show them with
   * its frequency. */
  if (h->khz != k->khz) {
    return;
  }
  if (takesOver(&k->best, &h->readings)) {
    k->best = h->readings;
  }
  if (h->at.known &&
      (!k->timedAt.known || takesOver(&k->timed, &h->readings))) {
    k->timed = h->readings;
    k->timedAt = h->at;
  }
  if (!k->pending || takesOver(&k->entry, &h->readings)) {
    k->entry = h->readings;
    k->entryAt = h->at;
    k->pending = true;
  }
}

/* A catch heard again, on its channel or beside it: it moves to where it
 * was heard stronger, and is due an entry there. Only at the width its best
 * was read at, since a wider filter reads the channel beside a station
 * louder without it being any nearer the station. */
static void hearAgain(DxCatch *k, const DxHearing *h) {
  if (k->khz != h->khz && h->readings.bandwidthKHz == k->best.bandwidthKHz &&
      h->readings.levelDbuVTenths > k->best.levelDbuVTenths) {
    k->khz = h->khz;
    k->logged = false;
    /* The timed row was heard on the channel it left. */
    k->timedAt.known = false;
  }
  takeHearing(k, h);
}

/* Move item `i` to the front, keeping the rest in order. */
static DxCatch *toFront(DxCatches *list, uint8_t i) {
  DxCatch k = list->item[i];
  memmove(&list->item[1], &list->item[0], (size_t)i * sizeof(DxCatch));
  list->item[0] = k;
  return &list->item[0];
}

int16_t dxCatchesFindId(const DxCatches *list, uint16_t id) {
  if (list == NULL || id == 0) {
    return -1;
  }
  for (uint8_t i = 0; i < list->count; i++) {
    if (list->item[i].id == id) {
      return i;
    }
  }
  return -1;
}

int16_t dxCatchesFindExact(const DxCatches *list, uint16_t pi, uint32_t khz) {
  if (list == NULL) {
    return -1;
  }
  for (uint8_t i = 0; i < list->count; i++) {
    if (list->item[i].pi == pi && list->item[i].khz == khz) {
      return i;
    }
  }
  return -1;
}

int16_t dxCatchesFind(const DxCatches *list, uint16_t pi, uint32_t khz) {
  /* The one on that very channel first: a catch that moved beside another
   * of the same PI must not take that one's hearings. */
  const int16_t exact = dxCatchesFindExact(list, pi, khz);
  if (list == NULL || exact >= 0) {
    return exact;
  }
  for (uint8_t i = 0; i < list->count; i++) {
    const DxCatch *k = &list->item[i];
    uint32_t apart = k->khz > khz ? k->khz - khz : khz - k->khz;
    if (k->pi == pi && apart <= DX_SHOULDER_KHZ) {
      return i;
    }
  }
  return -1;
}

DxCatch *dxCatchesAdd(DxCatches *list, const DxHearing *h, bool piIsNew) {
  if (list == NULL || h == NULL) {
    return NULL;
  }
  const int16_t at = dxCatchesFind(list, h->pi, h->khz);
  if (at >= 0) {
    DxCatch *k = &list->item[at];
    if (k->visit != h->visit) {
      k->count++;
      k->visit = h->visit;
    }
    hearAgain(k, h);
    return toFront(list, (uint8_t)at);
  }

  if (list->count < DX_CATCHES_MAX) {
    list->count++;
  } else {
    list->dropped++;
  }
  memmove(&list->item[1], &list->item[0],
          (size_t)(list->count - 1) * sizeof(DxCatch));
  DxCatch *k = &list->item[0];
  memset(k, 0, sizeof(*k));
  /* 0 is never an id, so one wrapped past 65535 starts again at 1. */
  list->lastId = (uint16_t)(list->lastId == UINT16_MAX ? 1 : list->lastId + 1);
  k->id = list->lastId;
  k->khz = h->khz;
  k->band = h->band;
  k->pi = h->pi;
  k->best = h->readings;
  k->count = 1;
  k->visit = h->visit;
  k->isNew = piIsNew;
  takeHearing(k, h);
  return k;
}

void dxCatchesRefresh(DxCatches *list, const DxHearing *h) {
  if (h == NULL || dxCatchesFind(list, h->pi, h->khz) != 0) {
    return;
  }
  hearAgain(&list->item[0], h);
}

bool dxCatchDueLog(const DxCatch *k) {
  return k != NULL && k->isNew && !k->autoSkip && !k->logged && k->pending;
}

void dxCatchToLog(const DxCatch *k, LogbookEntry *out) {
  if (out == NULL) {
    return;
  }
  memset(out, 0, sizeof(*out));
  if (k == NULL) {
    return;
  }
  out->timeKnown = k->entryAt.known;
  out->timeValue = k->entryAt.value;
  out->band = k->band;
  out->freqKHz = k->khz;
  out->levelDbuVTenths = k->entry.levelDbuVTenths;
  out->usnTenths = k->entry.usnTenths;
  out->multipathTenths = k->entry.multipathTenths;
  out->snrDb = k->entry.snrDb;
  out->stereo = k->entry.stereo;
  out->bandwidthKHz = k->entry.bandwidthKHz;
  out->hasName = k->hasPs;
  memcpy(out->name, k->ps, sizeof(out->name));
  out->hasPi = true;
  out->pi = k->pi;
}

/* The PE5PVB firmware's own header, so the converter finds every column. */
const char *dxCatchCsvHeader(void) {
  return "Date,Time,Frequency,PI,Signal,Stereo,TA,TP,PTY,ECC,PS,Radiotext\n";
}

/* Whether `t` goes before `u`: no time first, then the earlier. */
static bool timeBefore(const DxTime *t, const DxTime *u) {
  if (!t->known) {
    return u->known;
  }
  return u->known && t->value < u->value;
}

uint8_t dxCatchesByTime(const DxCatches *list, uint8_t *order) {
  if (list == NULL || order == NULL) {
    return 0;
  }
  /* An insertion sort, stable, of at most DX_CATCHES_MAX places. */
  for (uint8_t i = 0; i < list->count; i++) {
    uint8_t j = i;
    for (; j > 0 && timeBefore(&list->item[i].timedAt,
                               &list->item[order[j - 1]].timedAt);
         j--) {
      order[j] = order[j - 1];
    }
    order[j] = i;
  }
  return list->count;
}

/* The converter's flag: a dot for on, a space for off. */
static const char *csvFlag(bool on) {
  return on ? "\xE2\x80\xA2" : " ";
}

size_t dxCatchCsvLine(const DxCatch *k, char *out, size_t cap) {
  if (k == NULL || out == NULL) {
    return 0;
  }
  /* Wide enough for any year gmtime_r can give back, so the compiler sees
   * no format that could be cut short. */
  char date[40] = "";
  char clock[40] = "";
  const DxReadings *row = k->timedAt.known ? &k->timed : &k->best;
  if (k->timedAt.known) {
    time_t at = (time_t)k->timedAt.value;
    struct tm utc;
    if (gmtime_r(&at, &utc) != NULL) {
      snprintf(date, sizeof(date), "%02d-%02d-%04d", utc.tm_mday,
               utc.tm_mon + 1, utc.tm_year + 1900);
      snprintf(clock, sizeof(clock), "%02d:%02d:%02d", utc.tm_hour, utc.tm_min,
               utc.tm_sec);
    }
  }
  char ps[LOGBOOK_NAME_LEN] = "";
  if (k->hasPs) {
    for (size_t i = 0; i + 1 < sizeof(ps) && k->ps[i] != '\0'; i++) {
      const char c = k->ps[i];
      ps[i] = (c == ',' || c == '"' || (unsigned char)c < 0x20) ? ' ' : c;
      ps[i + 1] = '\0';
    }
  }
  char pty[4] = "";
  if (k->rds.hasPty) {
    snprintf(pty, sizeof(pty), "%u", (unsigned)k->rds.pty);
  }
  char ecc[4] = "--";
  if (k->rds.hasEcc) {
    snprintf(ecc, sizeof(ecc), "%02X", (unsigned)k->rds.ecc);
  }
  char freq[16];
  if (!bandFormatFrequency((BandId)k->band, k->khz, freq, sizeof(freq))) {
    return 0;
  }
  char level[8];
  signalFormatLevel(row->levelDbuVTenths, level, sizeof(level));
  return (size_t)snprintf(
      out, cap, "%s,%s,%s MHz,%04X,%s dB\xCE\xBCV,%s,%s,%s,%s,%s,%s,\n", date,
      clock, freq, (unsigned)k->pi, level, csvFlag(row->stereo),
      csvFlag(k->rds.hasFlags && k->rds.ta),
      csvFlag(k->rds.hasFlags && k->rds.tp), pty, ecc, ps);
}

/* Its entry is in the log, and nothing heard since is waiting for one. */
static void markWritten(DxCatch *k) {
  k->logged = true;
  k->pending = false;
}

DxWriteResult dxCatchWrite(DxCatch *k, DxLogWrite write, void *ctx) {
  if (k == NULL) {
    return DX_WRITE_NO_CATCH;
  }
  if (!k->pending) {
    return DX_WRITE_NOTHING_NEW;
  }
  LogbookEntry e;
  dxCatchToLog(k, &e);
  const LogbookWrite w = write != NULL ? write(ctx, &e) : LOGBOOK_NOT_WRITTEN;
  if (w == LOGBOOK_NOT_WRITTEN) {
    return DX_WRITE_FAILED;
  }
  markWritten(k);
  return w == LOGBOOK_WRITTEN ? DX_WRITE_DONE : DX_WRITE_IN_LOG;
}

void dxSeenReset(DxSeen *s) {
  if (s != NULL) {
    memset(s, 0, sizeof(*s));
  }
}

bool dxSeenHas(const DxSeen *s, uint16_t pi) {
  if (s == NULL) {
    return false;
  }
  for (uint16_t i = 0; i < s->count; i++) {
    if (s->pi[i] == pi) {
      return true;
    }
  }
  return false;
}

bool dxSeenAdd(DxSeen *s, uint16_t pi) {
  if (s == NULL || dxSeenHas(s, pi)) {
    return false;
  }
  if (s->count < DX_SEEN_MAX) {
    s->pi[s->count++] = pi;
    return true;
  }
  s->pi[s->next] = pi;
  s->next = (uint16_t)((s->next + 1) % DX_SEEN_MAX);
  return true;
}

size_t dxSeenEncode(const DxSeen *s, uint8_t *out, size_t cap) {
  if (s == NULL || out == NULL || cap < DX_SEEN_BYTES) {
    return 0;
  }
  memset(out, 0, DX_SEEN_BYTES);
  putU32(out, DX_SEEN_MAGIC);
  putU16(out + 4, s->count);
  putU16(out + 6, s->next);
  for (uint16_t i = 0; i < s->count; i++) {
    putU16(out + 8 + i * 2, s->pi[i]);
  }
  return DX_SEEN_BYTES;
}

bool dxSeenDecode(const uint8_t *in, size_t len, DxSeen *out) {
  if (out == NULL) {
    return false;
  }
  dxSeenReset(out);
  if (in == NULL || len < DX_SEEN_BYTES) {
    return false;
  }
  uint32_t magic = getU32(in);
  uint16_t count = getU16(in + 4);
  uint16_t next = getU16(in + 6);
  if (magic != DX_SEEN_MAGIC || count > DX_SEEN_MAX || next >= DX_SEEN_MAX ||
      (count < DX_SEEN_MAX && next != 0)) {
    return false;
  }
  for (uint16_t i = 0; i < count; i++) {
    out->pi[i] = getU16(in + 8 + i * 2);
  }
  out->count = count;
  out->next = next;
  return true;
}

void dxSessionReset(DxSession *s) {
  if (s != NULL) {
    memset(s, 0, sizeof(*s));
  }
}

/* The catch the dial is on, or NULL. While the dial is on a catch it is
 * item[0]: only a new hearing adds one, and it leaves the last first. It
 * may sit on the channel beside the one tuned, when heard stronger there. */
static DxCatch *onCatch(DxSession *s) {
  if (!s->on || s->catches.count == 0 || s->catches.item[0].pi != s->onPi) {
    return NULL;
  }
  return &s->catches.item[0];
}

/* A PI into the seen set, which then wants saving. */
static void markSeen(DxSession *s, uint16_t pi) {
  if (dxSeenAdd(&s->seen, pi)) {
    s->seenDirty = true;
    s->seenTried = false;
  }
}

/* A NEW catch heard with the auto log off is caught now, since nothing will
 * log it, or it would stay NEW and stop every later scan; and it stays out
 * of the auto log for good. */
static void skipAutoLog(DxSession *s, DxCatch *k) {
  k->autoSkip = true;
  markSeen(s, k->pi);
}

/* A logged NEW catch's PI goes in the seen set, and only then, so a radio
 * switched off before the entry was written still finds it NEW. */
static void markLogged(DxSession *s, DxCatch *k) {
  if (k->isNew) {
    markSeen(s, k->pi);
  }
}

static DxWriteResult sessionWrite(DxSession *s, DxCatch *k, DxLogWrite write,
                                  void *ctx) {
  const DxWriteResult r = dxCatchWrite(k, write, ctx);
  if (r == DX_WRITE_DONE || r == DX_WRITE_IN_LOG) {
    markLogged(s, k);
  }
  return r;
}

static void writeIfDue(DxSession *s, DxCatch *k, DxLogWrite write, void *ctx) {
  if (!s->autoLogOff && !s->learning && dxCatchDueLog(k)) {
    (void)sessionWrite(s, k, write, ctx);
  }
}

static void leave(DxSession *s, DxLogWrite write, void *ctx) {
  writeIfDue(s, onCatch(s), write, ctx);
  s->on = false;
}

/* Once per catch that is due, so a log that will not take it is not asked
 * on every poll. */
static void writeAtName(DxSession *s, DxLogWrite write, void *ctx) {
  DxCatch *k = onCatch(s);
  if (k != NULL && k->hasPs && !s->onTried && !s->autoLogOff && !s->learning &&
      dxCatchDueLog(k)) {
    s->onTried = true;
    (void)sessionWrite(s, k, write, ctx);
  }
}

void dxSessionHear(DxSession *s, uint32_t tunedKHz, const DxHearing *h,
                   DxLogWrite write, void *ctx) {
  if (s == NULL) {
    return;
  }
  if (tunedKHz != s->dialKHz) {
    s->dialKHz = tunedKHz;
    s->visit++;
    leave(s, write, ctx);
  }
  if (h == NULL) {
    return;
  }
  /* Learning marks the PI caught and nothing else: the locals do not
   * crowd the session's catches out of its list of 32. */
  if (s->learning) {
    if (s->seenKnown) {
      markSeen(s, h->pi);
    }
    return;
  }
  DxHearing heard = *h;
  heard.visit = s->visit;
  if (s->on && s->onPi == heard.pi) {
    const DxCatch *k = onCatch(s);
    const uint32_t wasKHz = k != NULL ? k->khz : 0;
    dxCatchesRefresh(&s->catches, &heard);
    /* A catch that moved is due its entry on the channel it moved to, and
     * that write belongs at the name as much as the first one. */
    if (k != NULL && k->khz != wasKHz) {
      s->onTried = false;
    }
    writeAtName(s, write, ctx);
    return;
  }
  /* A different PI on the same channel is a different station, so the one
   * before it is left, and written, first. */
  leave(s, write, ctx);
  const bool isNew = s->seenKnown && !dxSeenHas(&s->seen, heard.pi);
  DxCatch *added = dxCatchesAdd(&s->catches, &heard, isNew);
  if (isNew && s->autoLogOff) {
    skipAutoLog(s, added);
  }
  s->on = true;
  s->onPi = heard.pi;
  s->onTried = false;
  writeAtName(s, write, ctx);
}

/* Every catch that is due, the one the dial is on and any whose write
 * failed when the seen set was read, since nothing else will try them. */
void dxSessionClose(DxSession *s, DxLogWrite write, void *ctx) {
  if (s == NULL) {
    return;
  }
  /* Oldest first, so the log keeps them in the order they were heard. */
  for (uint8_t i = s->catches.count; i-- > 0;) {
    writeIfDue(s, &s->catches.item[i], write, ctx);
  }
}

void dxSessionSeenLoaded(DxSession *s, DxLogWrite write, void *ctx) {
  if (s == NULL) {
    return;
  }
  s->seenKnown = true;
  /* Oldest first, so the log keeps them in the order they were heard. */
  for (uint8_t i = s->catches.count; i-- > 0;) {
    DxCatch *k = &s->catches.item[i];
    if (k->isNew || dxSeenHas(&s->seen, k->pi)) {
      continue;
    }
    k->isNew = true;
    if (k->logged) {
      markSeen(s, k->pi);
    } else if (s->autoLogOff) {
      skipAutoLog(s, k);
    } else if (k != onCatch(s) || k->hasPs) {
      /* The one the dial is on waits for its name, as any NEW catch does. */
      writeIfDue(s, k, write, ctx);
    }
  }
}

DxWriteResult dxSessionWrite(DxSession *s, uint8_t index, DxLogWrite write,
                             void *ctx) {
  if (s == NULL || index >= s->catches.count) {
    return DX_WRITE_NO_CATCH;
  }
  return sessionWrite(s, &s->catches.item[index], write, ctx);
}

void dxSessionNoteLogged(DxSession *s, uint32_t khz, uint16_t pi) {
  if (s == NULL) {
    return;
  }
  /* The catch on that very channel. One on the channel beside it keeps its
   * own entry due, so the log has the station where it is even when the
   * hand log named the shoulder. */
  const int16_t at = dxCatchesFindExact(&s->catches, pi, khz);
  if (at >= 0) {
    markWritten(&s->catches.item[at]);
    markLogged(s, &s->catches.item[at]);
  }
}
