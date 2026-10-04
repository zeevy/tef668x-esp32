/*
 * Tests for the FM DX catches. Runs on a PC.
 */
#include <unity.h>

#include <string.h>

#include "core/band_plan.h"
#include "core/dx_catch.h"

static DxCatches list;

void setUp(void) {
  dxCatchesReset(&list);
}
void tearDown(void) {}

/* Each on a visit of its own, as a retune between them would make it. */
static DxHearing heard(uint32_t khz, uint16_t pi, int16_t level, uint32_t utc) {
  static uint32_t visits = 0;
  DxHearing h;
  memset(&h, 0, sizeof(h));
  h.visit = ++visits;
  h.khz = khz;
  h.band = (uint8_t)BAND_FM;
  h.pi = pi;
  h.readings.levelDbuVTenths = level;
  h.at.known = true;
  h.at.value = utc;
  return h;
}

static void a_first_hearing_is_a_new_catch(void) {
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  h.ps = "  RED   ";
  DxCatch *k = dxCatchesAdd(&list, &h, true);
  TEST_ASSERT_NOT_NULL(k);
  TEST_ASSERT_EQUAL_UINT8(1, list.count);
  TEST_ASSERT_EQUAL_UINT32(93500, k->khz);
  TEST_ASSERT_EQUAL_UINT16(1, k->count);
  TEST_ASSERT_TRUE(k->isNew);
  TEST_ASSERT_TRUE(k->hasPs);
  TEST_ASSERT_EQUAL_STRING("  RED   ", k->ps);
}

static void the_same_catch_again_counts_up_and_comes_first(void) {
  DxHearing a = heard(93500, 0x0935, 455, 1000);
  DxHearing b = heard(98300, 0x26FF, 482, 1100);
  dxCatchesAdd(&list, &a, true);
  dxCatchesAdd(&list, &b, true);
  TEST_ASSERT_EQUAL_UINT16(0x26FF, list.item[0].pi);
  DxHearing again = heard(93500, 0x0935, 455, 1200);
  DxCatch *k = dxCatchesAdd(&list, &again, false);
  TEST_ASSERT_EQUAL_UINT8(2, list.count);
  TEST_ASSERT_EQUAL_UINT16(0x0935, list.item[0].pi);
  TEST_ASSERT_EQUAL_UINT16(2, k->count);
  TEST_ASSERT_EQUAL_UINT32(1200, k->last.value);
  /* Kept from the first hearing. */
  TEST_ASSERT_TRUE(k->isNew);
}

/* The channel beside a strong station decodes that station's PI. One catch,
 * on the channel where it was heard stronger. */
static void the_same_pi_beside_it_is_one_catch_on_the_stronger(void) {
  DxHearing shoulder = heard(93600, 0x0935, 424, 1000);
  dxCatchesAdd(&list, &shoulder, true);
  DxHearing station = heard(93500, 0x0935, 455, 1010);
  DxCatch *k = dxCatchesAdd(&list, &station, false);
  TEST_ASSERT_EQUAL_UINT8(1, list.count);
  TEST_ASSERT_EQUAL_UINT32(93500, k->khz);
  TEST_ASSERT_EQUAL_INT16(455, k->best.levelDbuVTenths);

  /* Weaker beside it again does not move it. */
  DxHearing again = heard(93400, 0x0935, 439, 1020);
  k = dxCatchesAdd(&list, &again, false);
  TEST_ASSERT_EQUAL_UINT32(93500, k->khz);
  TEST_ASSERT_EQUAL_UINT16(3, k->count);
}

/* Through a wider filter the channel beside a station reads louder without
 * being any nearer it, so only a reading at the same width moves a catch. */
static void a_louder_reading_at_another_width_does_not_move_it(void) {
  DxHearing station = heard(93500, 0x0935, 300, 1000);
  station.readings.bandwidthKHz = 114;
  dxCatchesAdd(&list, &station, true);
  DxHearing wide = heard(93600, 0x0935, 424, 1010);
  wide.readings.bandwidthKHz = 217;
  DxCatch *k = dxCatchesAdd(&list, &wide, false);
  TEST_ASSERT_EQUAL_UINT32(93500, k->khz);
  TEST_ASSERT_EQUAL_INT16(300, k->best.levelDbuVTenths);
  wide.readings.bandwidthKHz = 114;
  k = dxCatchesAdd(&list, &wide, false);
  TEST_ASSERT_EQUAL_UINT32(93600, k->khz);
}

/* The row shows the strongest at the width in force: after a change of
 * width the first reading takes over, louder or not. */
static void the_best_follows_the_width(void) {
  DxHearing h = heard(93500, 0x0935, 424, 1000);
  h.readings.bandwidthKHz = 217;
  DxCatch *k = dxCatchesAdd(&list, &h, true);
  h.readings.bandwidthKHz = 114;
  h.readings.levelDbuVTenths = 300;
  dxCatchesRefresh(&list, &h);
  TEST_ASSERT_EQUAL_INT16(300, k->best.levelDbuVTenths);
  TEST_ASSERT_EQUAL_UINT16(114, k->best.bandwidthKHz);
  TEST_ASSERT_EQUAL_INT16(300, k->entry.levelDbuVTenths);
  h.readings.levelDbuVTenths = 250;
  h.at.value = 1500;
  dxCatchesRefresh(&list, &h);
  TEST_ASSERT_EQUAL_INT16(300, k->best.levelDbuVTenths);
  TEST_ASSERT_EQUAL_INT16(300, k->entry.levelDbuVTenths);
  TEST_ASSERT_EQUAL_UINT32(1000, k->entryAt.value);
}

static void two_channels_apart_is_two_catches(void) {
  DxHearing a = heard(93500, 0x0935, 455, 1000);
  DxHearing b = heard(93700, 0x0935, 300, 1010);
  dxCatchesAdd(&list, &a, true);
  dxCatchesAdd(&list, &b, false);
  TEST_ASSERT_EQUAL_UINT8(2, list.count);
}

static void a_catch_is_found_on_its_channel_or_beside_it(void) {
  DxHearing a = heard(93500, 0x0935, 455, 1000);
  DxHearing b = heard(98300, 0x26FF, 482, 1100);
  dxCatchesAdd(&list, &a, true);
  dxCatchesAdd(&list, &b, true);
  TEST_ASSERT_EQUAL_INT16(1, dxCatchesFind(&list, 0x0935, 93500));
  TEST_ASSERT_EQUAL_INT16(1, dxCatchesFind(&list, 0x0935, 93600));
  TEST_ASSERT_EQUAL_INT16(1, dxCatchesFind(&list, 0x0935, 93400));
  TEST_ASSERT_EQUAL_INT16(0, dxCatchesFind(&list, 0x26FF, 98300));
  /* Past the channel beside it, two channels away, another PI, or no
   * list. */
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFind(&list, 0x0935, 93601));
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFind(&list, 0x0935, 93650));
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFind(&list, 0x0935, 93700));
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFind(&list, 0x1064, 93500));
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFind(NULL, 0x0935, 93500));
}

static void a_full_list_drops_its_oldest(void) {
  for (uint16_t i = 0; i < DX_CATCHES_MAX; i++) {
    DxHearing h = heard(87500 + i * 300u, (uint16_t)(0x1000 + i), 300, i);
    dxCatchesAdd(&list, &h, true);
  }
  TEST_ASSERT_EQUAL_UINT8(DX_CATCHES_MAX, list.count);
  TEST_ASSERT_EQUAL_UINT16(0x1000, list.item[DX_CATCHES_MAX - 1].pi);
  DxHearing h = heard(107900, 0x9999, 300, 99);
  dxCatchesAdd(&list, &h, true);
  TEST_ASSERT_EQUAL_UINT8(DX_CATCHES_MAX, list.count);
  TEST_ASSERT_EQUAL_UINT16(0x9999, list.item[0].pi);
  TEST_ASSERT_EQUAL_UINT16(0x1001, list.item[DX_CATCHES_MAX - 1].pi);
}

static void a_refresh_takes_the_name_and_a_stronger_level(void) {
  DxHearing h = heard(93500, 0x0935, 300, 1000);
  dxCatchesAdd(&list, &h, true);
  h.ps = "  RED   ";
  h.country = "IN";
  h.readings.levelDbuVTenths = 350;
  dxCatchesRefresh(&list, &h);
  TEST_ASSERT_EQUAL_UINT16(1, list.item[0].count);
  TEST_ASSERT_TRUE(list.item[0].hasPs);
  TEST_ASSERT_TRUE(list.item[0].hasCountry);
  TEST_ASSERT_EQUAL_STRING("IN", list.item[0].country);
  TEST_ASSERT_EQUAL_INT16(350, list.item[0].best.levelDbuVTenths);

  /* A weaker reading leaves the best alone, and another channel is not
   * this catch at all. */
  h.readings.levelDbuVTenths = 200;
  dxCatchesRefresh(&list, &h);
  TEST_ASSERT_EQUAL_INT16(350, list.item[0].best.levelDbuVTenths);
  DxHearing other = heard(98300, 0x26FF, 500, 1000);
  dxCatchesRefresh(&list, &other);
  TEST_ASSERT_EQUAL_UINT16(0x0935, list.item[0].pi);
  TEST_ASSERT_EQUAL_INT16(350, list.item[0].best.levelDbuVTenths);
}

/* The dial on the weaker channel beside a catch still reaches it, and a
 * stronger reading there moves it. */
static void a_refresh_beside_the_catch_reaches_it(void) {
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  dxCatchesAdd(&list, &h, true);
  DxHearing beside = heard(93600, 0x0935, 424, 1010);
  beside.ps = "  RED   ";
  dxCatchesRefresh(&list, &beside);
  TEST_ASSERT_TRUE(list.item[0].hasPs);
  TEST_ASSERT_EQUAL_UINT32(1010, list.item[0].last.value);
  TEST_ASSERT_EQUAL_UINT32(93500, list.item[0].khz);
  beside.readings.levelDbuVTenths = 470;
  dxCatchesRefresh(&list, &beside);
  TEST_ASSERT_EQUAL_UINT32(93600, list.item[0].khz);
  TEST_ASSERT_EQUAL_INT16(470, list.item[0].best.levelDbuVTenths);
  TEST_ASSERT_EQUAL_UINT16(1, list.item[0].count);
}

/* A log that keeps what it is given, refuses it all, or answers that it
 * holds the station already. */
#define FAKE_LOG_MAX 8
typedef struct {
  LogbookEntry e[FAKE_LOG_MAX];
  int n;     /* Entries taken. */
  int tries; /* Writes asked for, taken or not. */
  bool fail;
  bool holds;
} FakeLog;

static FakeLog flog;

static LogbookWrite fakeWrite(void *ctx, const LogbookEntry *e) {
  FakeLog *l = (FakeLog *)ctx;
  l->tries++;
  if (l->fail) {
    return LOGBOOK_NOT_WRITTEN;
  }
  if (l->holds) {
    return LOGBOOK_ALREADY_THERE;
  }
  if (l->n < FAKE_LOG_MAX) {
    l->e[l->n] = *e;
  }
  l->n++;
  return LOGBOOK_WRITTEN;
}

static void only_a_new_catch_is_due_and_only_once(void) {
  memset(&flog, 0, sizeof(flog));
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  DxCatch *old = dxCatchesAdd(&list, &h, false);
  TEST_ASSERT_FALSE(dxCatchDueLog(old));
  DxHearing n = heard(98300, 0x26FF, 482, 1100);
  DxCatch *k = dxCatchesAdd(&list, &n, true);
  TEST_ASSERT_TRUE(dxCatchDueLog(k));
  TEST_ASSERT_EQUAL(DX_WRITE_DONE, dxCatchWrite(k, fakeWrite, &flog));
  TEST_ASSERT_FALSE(dxCatchDueLog(k));
  /* Heard again, even the next day: the log has it. */
  n.at.value = 1100 + 86400;
  dxCatchesRefresh(&list, &n);
  TEST_ASSERT_FALSE(dxCatchDueLog(k));
  TEST_ASSERT_FALSE(dxCatchDueLog(NULL));
}

/* Each entry is the strongest hearing since the one before, with its own
 * time. The page keeps the strongest of all. */
static void an_entry_is_the_strongest_hearing_since_the_last(void) {
  memset(&flog, 0, sizeof(flog));
  DxHearing h = heard(93500, 0x0935, 250, 100000);
  DxCatch *k = dxCatchesAdd(&list, &h, true);
  TEST_ASSERT_EQUAL(DX_WRITE_DONE, dxCatchWrite(k, fakeWrite, &flog));
  h.readings.levelDbuVTenths = 450;
  h.at.value = 100600;
  dxCatchesRefresh(&list, &h);
  h.readings.levelDbuVTenths = 300;
  h.at.value = 100700;
  dxCatchesRefresh(&list, &h);
  TEST_ASSERT_EQUAL(DX_WRITE_DONE, dxCatchWrite(k, fakeWrite, &flog));
  h.readings.levelDbuVTenths = 200;
  h.at.value = 190000;
  dxCatchesRefresh(&list, &h);
  TEST_ASSERT_EQUAL(DX_WRITE_DONE, dxCatchWrite(k, fakeWrite, &flog));
  TEST_ASSERT_EQUAL_INT(3, flog.n);
  TEST_ASSERT_EQUAL_INT16(250, flog.e[0].levelDbuVTenths);
  TEST_ASSERT_EQUAL_UINT32(100000, flog.e[0].timeValue);
  TEST_ASSERT_EQUAL_INT16(450, flog.e[1].levelDbuVTenths);
  TEST_ASSERT_EQUAL_UINT32(100600, flog.e[1].timeValue);
  TEST_ASSERT_EQUAL_INT16(200, flog.e[2].levelDbuVTenths);
  TEST_ASSERT_EQUAL_UINT32(190000, flog.e[2].timeValue);
  TEST_ASSERT_EQUAL_INT16(450, k->best.levelDbuVTenths);
}

/* The channel beside a catch hears it weaker, which does not move it, so
 * that hearing is not an entry for the catch's own channel. */
static void a_hearing_beside_a_catch_stays_out_of_its_entry(void) {
  memset(&flog, 0, sizeof(flog));
  DxHearing h = heard(100000, 0x1234, 400, 1000);
  DxCatch *k = dxCatchesAdd(&list, &h, true);
  TEST_ASSERT_EQUAL(DX_WRITE_DONE, dxCatchWrite(k, fakeWrite, &flog));
  DxHearing beside = heard(100100, 0x1234, 150, 2000);
  k = dxCatchesAdd(&list, &beside, false);
  TEST_ASSERT_EQUAL_UINT32(100000, k->khz);
  TEST_ASSERT_EQUAL_UINT32(2000, k->last.value);
  TEST_ASSERT_FALSE(k->pending);
  TEST_ASSERT_EQUAL(DX_WRITE_NOTHING_NEW, dxCatchWrite(k, fakeWrite, &flog));
  TEST_ASSERT_EQUAL_INT(1, flog.tries);
}
/* Logged on the channel beside the station, then heard stronger on the
 * station's own: the right channel gets its entry too. */
static void a_catch_that_moves_is_due_again_on_its_new_channel(void) {
  memset(&flog, 0, sizeof(flog));
  DxHearing shoulder = heard(93400, 0x0935, 424, 1000);
  DxCatch *k = dxCatchesAdd(&list, &shoulder, true);
  TEST_ASSERT_EQUAL(DX_WRITE_DONE, dxCatchWrite(k, fakeWrite, &flog));
  DxHearing station = heard(93500, 0x0935, 455, 1010);
  k = dxCatchesAdd(&list, &station, false);
  TEST_ASSERT_EQUAL_UINT32(93500, k->khz);
  TEST_ASSERT_TRUE(dxCatchDueLog(k));
  TEST_ASSERT_EQUAL(DX_WRITE_DONE, dxCatchWrite(k, fakeWrite, &flog));
  TEST_ASSERT_EQUAL_UINT32(93400, flog.e[0].freqKHz);
  TEST_ASSERT_EQUAL_UINT32(93500, flog.e[1].freqKHz);
  TEST_ASSERT_EQUAL_INT16(455, flog.e[1].levelDbuVTenths);
}

/* The log holds the station already, written by hand or in an earlier
 * session: the catch is as good as written, and is not tried again. */
static void a_catch_the_log_holds_already_is_marked_written(void) {
  memset(&flog, 0, sizeof(flog));
  flog.holds = true;
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  DxCatch *k = dxCatchesAdd(&list, &h, true);
  TEST_ASSERT_EQUAL(DX_WRITE_IN_LOG, dxCatchWrite(k, fakeWrite, &flog));
  TEST_ASSERT_TRUE(k->logged);
  TEST_ASSERT_FALSE(dxCatchDueLog(k));
  TEST_ASSERT_EQUAL(DX_WRITE_NOTHING_NEW, dxCatchWrite(k, fakeWrite, &flog));
  TEST_ASSERT_EQUAL_INT(1, flog.tries);
  TEST_ASSERT_EQUAL_INT(0, flog.n);
}

static void a_write_that_fails_marks_nothing(void) {
  memset(&flog, 0, sizeof(flog));
  flog.fail = true;
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  DxCatch *k = dxCatchesAdd(&list, &h, true);
  TEST_ASSERT_EQUAL(DX_WRITE_FAILED, dxCatchWrite(k, fakeWrite, &flog));
  TEST_ASSERT_FALSE(k->logged);
  TEST_ASSERT_TRUE(dxCatchDueLog(k));
  TEST_ASSERT_EQUAL(DX_WRITE_FAILED, dxCatchWrite(k, NULL, &flog));
  TEST_ASSERT_EQUAL(DX_WRITE_NO_CATCH, dxCatchWrite(NULL, fakeWrite, &flog));
}

/* DX mode opens on OIRT too, and the log names the band it was on. */
static void an_oirt_catch_is_logged_as_oirt(void) {
  DxHearing h = heard(69830, 0x2201, 300, 1000);
  h.band = (uint8_t)BAND_OIRT;
  DxCatch *k = dxCatchesAdd(&list, &h, true);
  LogbookEntry e;
  dxCatchToLog(k, &e);
  TEST_ASSERT_EQUAL_UINT8(BAND_OIRT, e.band);
  TEST_ASSERT_EQUAL_UINT32(69830, e.freqKHz);
}

static void the_log_entry_is_the_catch_at_its_best(void) {
  DxHearing h = heard(93500, 0x0935, 455, 100000);
  h.ps = "  RED   ";
  h.readings.usnTenths = 50;
  h.readings.multipathTenths = 50;
  h.readings.stereo = true;
  h.readings.bandwidthKHz = 114;
  DxCatch *k = dxCatchesAdd(&list, &h, true);
  LogbookEntry e;
  dxCatchToLog(k, &e);
  TEST_ASSERT_TRUE(e.timeKnown);
  TEST_ASSERT_EQUAL_UINT32(100000, e.timeValue);
  TEST_ASSERT_EQUAL_UINT8(BAND_FM, e.band);
  TEST_ASSERT_EQUAL_UINT32(93500, e.freqKHz);
  TEST_ASSERT_EQUAL_INT16(455, e.levelDbuVTenths);
  TEST_ASSERT_EQUAL_UINT16(50, e.usnTenths);
  TEST_ASSERT_TRUE(e.stereo);
  TEST_ASSERT_EQUAL_UINT16(114, e.bandwidthKHz);
  TEST_ASSERT_TRUE(e.hasName);
  TEST_ASSERT_EQUAL_STRING("  RED   ", e.name);
  TEST_ASSERT_TRUE(e.hasPi);
  TEST_ASSERT_EQUAL_HEX16(0x0935, e.pi);
  dxCatchToLog(NULL, &e);
  TEST_ASSERT_FALSE(e.hasPi);
  dxCatchToLog(k, NULL);
}

static void nothing_happens_without_somewhere_to_put_it(void) {
  DxHearing h = heard(93500, 0x0935, 455, 1);
  TEST_ASSERT_NULL(dxCatchesAdd(NULL, &h, true));
  TEST_ASSERT_NULL(dxCatchesAdd(&list, NULL, true));
  dxCatchesRefresh(NULL, &h);
  dxCatchesRefresh(&list, &h);
  TEST_ASSERT_EQUAL_UINT8(0, list.count);
  dxCatchesReset(NULL);
}

static DxSeen seen;

static void the_seen_set_marks_a_pi_once(void) {
  dxSeenReset(&seen);
  TEST_ASSERT_FALSE(dxSeenHas(&seen, 0x0935));
  TEST_ASSERT_TRUE(dxSeenAdd(&seen, 0x0935));
  TEST_ASSERT_TRUE(dxSeenHas(&seen, 0x0935));
  TEST_ASSERT_FALSE(dxSeenAdd(&seen, 0x0935));
  TEST_ASSERT_FALSE(dxSeenHas(NULL, 0x0935));
  TEST_ASSERT_FALSE(dxSeenAdd(NULL, 0x0935));
  dxSeenReset(NULL);
}

static void a_full_seen_set_forgets_the_first_caught(void) {
  dxSeenReset(&seen);
  for (uint16_t i = 0; i < DX_SEEN_MAX; i++) {
    dxSeenAdd(&seen, (uint16_t)(0x1000 + i));
  }
  TEST_ASSERT_TRUE(dxSeenAdd(&seen, 0x9999));
  TEST_ASSERT_FALSE(dxSeenHas(&seen, 0x1000));
  TEST_ASSERT_TRUE(dxSeenHas(&seen, 0x1001));
  TEST_ASSERT_TRUE(dxSeenHas(&seen, 0x9999));
  TEST_ASSERT_EQUAL_UINT16(DX_SEEN_MAX, seen.count);
}

static void the_seen_set_goes_to_bytes_and_back(void) {
  dxSeenReset(&seen);
  dxSeenAdd(&seen, 0x0935);
  dxSeenAdd(&seen, 0x26FF);
  static uint8_t bytes[DX_SEEN_BYTES];
  TEST_ASSERT_EQUAL(DX_SEEN_BYTES, dxSeenEncode(&seen, bytes, sizeof(bytes)));
  static DxSeen back;
  TEST_ASSERT_TRUE(dxSeenDecode(bytes, sizeof(bytes), &back));
  TEST_ASSERT_EQUAL_UINT16(2, back.count);
  TEST_ASSERT_TRUE(dxSeenHas(&back, 0x26FF));
  TEST_ASSERT_EQUAL(0, dxSeenEncode(&seen, bytes, DX_SEEN_BYTES - 1));
  TEST_ASSERT_EQUAL(0, dxSeenEncode(NULL, bytes, sizeof(bytes)));
}

/* Bytes this format did not write come back as an empty set, never as PIs
 * made of whatever the file held. */
static void a_file_that_is_not_ours_is_an_empty_set(void) {
  static uint8_t bytes[DX_SEEN_BYTES];
  static DxSeen back;
  memset(bytes, 0xFF, sizeof(bytes));
  TEST_ASSERT_FALSE(dxSeenDecode(bytes, sizeof(bytes), &back));
  TEST_ASSERT_EQUAL_UINT16(0, back.count);

  dxSeenReset(&seen);
  dxSeenAdd(&seen, 0x0935);
  dxSeenEncode(&seen, bytes, sizeof(bytes));
  TEST_ASSERT_FALSE(dxSeenDecode(bytes, DX_SEEN_BYTES - 1, &back));
  bytes[4] = 0xFF; /* A count past the end. */
  bytes[5] = 0xFF;
  TEST_ASSERT_FALSE(dxSeenDecode(bytes, sizeof(bytes), &back));
  dxSeenEncode(&seen, bytes, sizeof(bytes));
  bytes[6] = 1; /* A next slot on a set that is not full. */
  TEST_ASSERT_FALSE(dxSeenDecode(bytes, sizeof(bytes), &back));
  dxSeenEncode(&seen, bytes, sizeof(bytes));
  bytes[7] = 0x40; /* A next slot past the end. */
  TEST_ASSERT_FALSE(dxSeenDecode(bytes, sizeof(bytes), &back));
  TEST_ASSERT_FALSE(dxSeenDecode(NULL, sizeof(bytes), &back));
  TEST_ASSERT_FALSE(dxSeenDecode(bytes, sizeof(bytes), NULL));
}

/* ------------------------------------------------------- the session -- */

static DxSession sess;

static void startSession(bool seenKnown) {
  dxSessionReset(&sess);
  sess.seenKnown = seenKnown;
  memset(&flog, 0, sizeof(flog));
}

static void a_catch_is_written_once_its_name_arrives(void) {
  startSession(true);
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(0, flog.n);
  TEST_ASSERT_TRUE(sess.catches.item[0].isNew);
  /* Not in the seen set until its entry is in the log: switched off now,
   * it would still be NEW next time. */
  TEST_ASSERT_FALSE(dxSeenHas(&sess.seen, 0x0935));
  TEST_ASSERT_FALSE(sess.seenDirty);
  h.ps = "  RED   ";
  h.at.value = 1002;
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  TEST_ASSERT_EQUAL_STRING("  RED   ", flog.e[0].name);
  TEST_ASSERT_TRUE(dxSeenHas(&sess.seen, 0x0935));
  TEST_ASSERT_TRUE(sess.seenDirty);
  /* Heard on, and polls with nothing heard: nothing more. */
  h.at.value = 1010;
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  dxSessionHear(&sess, 93500, NULL, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  TEST_ASSERT_EQUAL_UINT16(1, sess.catches.item[0].count);
}

/* The same for the auto log: a NEW catch whose station the log holds goes
 * in the seen set, so the next session does not take it for NEW. */
static void a_new_catch_the_log_holds_goes_in_the_seen_set(void) {
  startSession(true);
  flog.holds = true;
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  h.ps = "  RED   ";
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.tries);
  TEST_ASSERT_EQUAL_INT(0, flog.n);
  TEST_ASSERT_TRUE(sess.catches.item[0].logged);
  TEST_ASSERT_TRUE(dxSeenHas(&sess.seen, 0x0935));
  h.at.value = 1010;
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.tries);
}

static void a_nameless_catch_is_written_on_leaving(void) {
  startSession(true);
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  dxSessionHear(&sess, 93500, NULL, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(0, flog.n);
  dxSessionHear(&sess, 93600, NULL, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  TEST_ASSERT_FALSE(flog.e[0].hasName);
  TEST_ASSERT_FALSE(sess.on);
}

/* A log that will not take it is tried at the name and once more on
 * leaving, not on every poll between. */
static void a_failed_write_is_tried_again_only_on_leaving(void) {
  startSession(true);
  flog.fail = true;
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  h.ps = "  RED   ";
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  for (int i = 0; i < 5; i++) {
    dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  }
  TEST_ASSERT_EQUAL_INT(1, flog.tries);
  dxSessionHear(&sess, 98300, NULL, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(2, flog.tries);
  TEST_ASSERT_EQUAL_INT(0, flog.n);
  TEST_ASSERT_FALSE(dxSeenHas(&sess.seen, 0x0935));
}

static void another_pi_on_the_channel_writes_the_first_before_it(void) {
  startSession(true);
  DxHearing a = heard(93500, 0x0935, 455, 1000);
  DxHearing b = heard(93500, 0x5A01, 300, 1100);
  dxSessionHear(&sess, 93500, &a, fakeWrite, &flog);
  dxSessionHear(&sess, 93500, &b, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  TEST_ASSERT_EQUAL_HEX16(0x0935, flog.e[0].pi);
  TEST_ASSERT_EQUAL_UINT8(2, sess.catches.count);
  TEST_ASSERT_EQUAL_HEX16(0x5A01, sess.onPi);
}

/* Two stations taking turns on one channel, the dial never moving: one
 * confirmation each, not one per turn. */
static void stations_taking_turns_on_a_channel_count_once(void) {
  startSession(true);
  DxHearing a = heard(93500, 0x0935, 455, 1000);
  DxHearing b = heard(93500, 0x5A01, 300, 1100);
  for (int i = 0; i < 5; i++) {
    dxSessionHear(&sess, 93500, &a, fakeWrite, &flog);
    dxSessionHear(&sess, 93500, &b, fakeWrite, &flog);
  }
  TEST_ASSERT_EQUAL_UINT8(2, sess.catches.count);
  TEST_ASSERT_EQUAL_UINT16(1, sess.catches.item[0].count);
  TEST_ASSERT_EQUAL_UINT16(1, sess.catches.item[1].count);
  /* A retune away and back is a second. */
  dxSessionHear(&sess, 98300, NULL, fakeWrite, &flog);
  dxSessionHear(&sess, 93500, &a, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_UINT16(2, sess.catches.item[0].count);
}

/* Closing DX mode writes what is due, and opening it again on the same
 * station is not a second hearing. */
static void closing_and_opening_again_counts_nothing(void) {
  startSession(true);
  DxHearing h = heard(106400, 0x1064, 700, 1000);
  dxSessionHear(&sess, 106400, &h, fakeWrite, &flog);
  dxSessionClose(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  h.at.value = 1100;
  dxSessionHear(&sess, 106400, &h, fakeWrite, &flog);
  dxSessionClose(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_UINT16(1, sess.catches.item[0].count);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  dxSessionClose(NULL, fakeWrite, &flog);
}

/* A seen set that could not be read marks nothing NEW and is never offered
 * for saving: saving it would write it empty over the real one. */
static void new_needs_a_seen_set_that_was_read(void) {
  startSession(false);
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  dxSessionHear(&sess, 98300, NULL, fakeWrite, &flog);
  TEST_ASSERT_FALSE(sess.catches.item[0].isNew);
  TEST_ASSERT_EQUAL_UINT16(0, sess.seen.count);
  TEST_ASSERT_FALSE(sess.seenDirty);
  TEST_ASSERT_EQUAL_INT(0, flog.tries);

  startSession(true);
  dxSeenAdd(&sess.seen, 0x0935);
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  TEST_ASSERT_FALSE(sess.catches.item[0].isNew);
  DxHearing other = heard(98300, 0x26FF, 482, 1100);
  dxSessionHear(&sess, 98300, &other, fakeWrite, &flog);
  TEST_ASSERT_TRUE(sess.catches.item[0].isNew);
}

static void a_catch_caught_before_is_not_written(void) {
  startSession(true);
  dxSeenAdd(&sess.seen, 0x1064);
  DxHearing h = heard(106400, 0x1064, 700, 1000);
  h.ps = " MAGIC  ";
  dxSessionHear(&sess, 106400, &h, fakeWrite, &flog);
  dxSessionHear(&sess, 106500, NULL, fakeWrite, &flog);
  dxSessionClose(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(0, flog.tries);
  TEST_ASSERT_EQUAL_UINT8(1, sess.catches.count);
}

/* The set read the next time DX mode opens: every catch heard before it is
 * decided then, and a NEW one is written and its PI saved. */
static void a_seen_set_read_late_still_marks_the_catch(void) {
  startSession(false);
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  h.ps = "  RED   ";
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  TEST_ASSERT_FALSE(sess.catches.item[0].isNew);
  dxSessionClose(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(0, flog.tries);
  dxSessionSeenLoaded(&sess, fakeWrite, &flog);
  TEST_ASSERT_TRUE(sess.seenKnown);
  TEST_ASSERT_TRUE(sess.catches.item[0].isNew);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  TEST_ASSERT_TRUE(sess.seenDirty);
  TEST_ASSERT_FALSE(sess.seenTried);
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  dxSessionHear(&sess, 98300, NULL, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.n);

  /* One the dial has left, and one whose PI the set already has. */
  startSession(false);
  DxHearing other = heard(98300, 0x26FF, 482, 1100);
  DxHearing local = heard(106400, 0x1064, 700, 1200);
  dxSessionHear(&sess, 98300, &other, fakeWrite, &flog);
  dxSessionHear(&sess, 106400, &local, fakeWrite, &flog);
  dxSeenAdd(&sess.seen, 0x1064);
  dxSessionSeenLoaded(&sess, fakeWrite, &flog);
  TEST_ASSERT_TRUE(sess.catches.item[1].isNew);
  TEST_ASSERT_FALSE(sess.catches.item[0].isNew);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  TEST_ASSERT_EQUAL_HEX16(0x26FF, flog.e[0].pi);
  dxSessionSeenLoaded(NULL, fakeWrite, &flog);
}

/* A held ENTER writes the ordinary entry and tells the session, and a hold
 * on a row writes through it, so a NEW catch logged by hand is not written
 * again by the auto log. */
static void a_catch_logged_by_hand_is_not_written_again(void) {
  startSession(true);
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  dxSessionNoteLogged(&sess, 93700, 0x0935); /* Two channels away: not it. */
  TEST_ASSERT_FALSE(sess.catches.item[0].logged);
  dxSessionNoteLogged(&sess, 93500, 0x0935);
  TEST_ASSERT_TRUE(sess.catches.item[0].logged);
  TEST_ASSERT_TRUE(dxSeenHas(&sess.seen, 0x0935));
  h.ps = "  RED   ";
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  dxSessionHear(&sess, 98300, NULL, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(0, flog.tries);
  dxSessionNoteLogged(NULL, 93500, 0x0935);

  TEST_ASSERT_EQUAL(DX_WRITE_DONE, dxSessionWrite(&sess, 0, fakeWrite, &flog));
  TEST_ASSERT_EQUAL(DX_WRITE_NOTHING_NEW,
                    dxSessionWrite(&sess, 0, fakeWrite, &flog));
  TEST_ASSERT_EQUAL(DX_WRITE_NO_CATCH,
                    dxSessionWrite(&sess, 1, fakeWrite, &flog));
  TEST_ASSERT_EQUAL(DX_WRITE_NO_CATCH,
                    dxSessionWrite(NULL, 0, fakeWrite, &flog));
}

/* Logged by hand before the seen set could say: once it can, the PI goes
 * in, so the next session does not log it again, and the auto log does
 * not write it a second time now. */
static void a_hand_log_before_the_set_was_read_goes_in_it_later(void) {
  startSession(false);
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  TEST_ASSERT_EQUAL(DX_WRITE_DONE, dxSessionWrite(&sess, 0, fakeWrite, &flog));
  TEST_ASSERT_FALSE(dxSeenHas(&sess.seen, 0x0935));
  dxSessionSeenLoaded(&sess, fakeWrite, &flog);
  h.ps = "  RED   ";
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  TEST_ASSERT_TRUE(sess.catches.item[0].isNew);
  TEST_ASSERT_TRUE(dxSeenHas(&sess.seen, 0x0935));
  TEST_ASSERT_TRUE(sess.seenDirty);
  dxSessionHear(&sess, 98300, NULL, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
}

/* A catch written on the channel beside the dial, then heard louder on the
 * dial's own: its entry there is written at once, not left for leaving. */
static void a_catch_that_moves_on_the_dial_is_written_there_at_once(void) {
  startSession(true);
  flog.fail = true;
  DxHearing beside = heard(93400, 0x0935, 500, 1000);
  beside.ps = "  RED   ";
  dxSessionHear(&sess, 93400, &beside, fakeWrite, &flog);
  dxSessionHear(&sess, 93500, NULL, fakeWrite, &flog);
  flog.fail = false;
  DxHearing on = heard(93500, 0x0935, 400, 1010);
  on.ps = "  RED   ";
  dxSessionHear(&sess, 93500, &on, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  TEST_ASSERT_EQUAL_UINT32(93400, flog.e[0].freqKHz);
  on.readings.levelDbuVTenths = 600;
  dxSessionHear(&sess, 93500, &on, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(2, flog.n);
  TEST_ASSERT_EQUAL_UINT32(93500, flog.e[1].freqKHz);
}

/* A hand log of a station not caught yet leaves it to the catches: NEW,
 * and written once when caught. One that named the channel beside a catch
 * leaves the catch's own entry due. */
static void a_hand_log_reaches_only_the_catch_on_its_channel(void) {
  startSession(true);
  dxSessionNoteLogged(&sess, 98300, 0x26FF);
  TEST_ASSERT_FALSE(dxSeenHas(&sess.seen, 0x26FF));
  TEST_ASSERT_FALSE(sess.seenDirty);
  DxHearing h = heard(98300, 0x26FF, 482, 1000);
  h.ps = " MIRCHI ";
  dxSessionHear(&sess, 98300, &h, fakeWrite, &flog);
  TEST_ASSERT_TRUE(sess.catches.item[0].isNew);
  TEST_ASSERT_EQUAL_INT(1, flog.n);

  /* The hand log named the shoulder: the station's own channel keeps its
   * entry due. */
  memset(&flog, 0, sizeof(flog));
  DxHearing b = heard(93500, 0x0935, 455, 1100);
  dxSessionHear(&sess, 93500, &b, fakeWrite, &flog);
  dxSessionNoteLogged(&sess, 93600, 0x0935);
  TEST_ASSERT_FALSE(sess.catches.item[0].logged);
  dxSessionHear(&sess, 98300, NULL, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  TEST_ASSERT_EQUAL_UINT32(93500, flog.e[0].freqKHz);
}

/* The set read late while the dial is on a nameless NEW catch: it waits for
 * its name. One whose write failed then is tried again on closing. */
static void a_late_read_waits_for_the_name_and_closing_tries_again(void) {
  startSession(false);
  DxHearing old = heard(98300, 0x26FF, 482, 1000);
  DxHearing on = heard(93500, 0x0935, 455, 1100);
  dxSessionHear(&sess, 98300, &old, fakeWrite, &flog);
  dxSessionHear(&sess, 93500, &on, fakeWrite, &flog);
  flog.fail = true;
  dxSessionSeenLoaded(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.tries); /* 98.3, not the one on the dial. */
  flog.fail = false;
  on.ps = "  RED   ";
  dxSessionHear(&sess, 93500, &on, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(1, flog.n);
  TEST_ASSERT_EQUAL_STRING("  RED   ", flog.e[0].name);
  dxSessionClose(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(2, flog.n);
  TEST_ASSERT_EQUAL_HEX16(0x26FF, flog.e[1].pi);
}

/* Written together, they go in the log in the order they were heard. */
static void catches_written_together_go_in_oldest_first(void) {
  startSession(false);
  DxHearing a = heard(93500, 0x0935, 455, 1000);
  DxHearing b = heard(98300, 0x26FF, 482, 1100);
  DxHearing c = heard(101900, 0x1234, 300, 1200);
  dxSessionHear(&sess, 93500, &a, fakeWrite, &flog);
  dxSessionHear(&sess, 98300, &b, fakeWrite, &flog);
  dxSessionHear(&sess, 101900, &c, fakeWrite, &flog);
  dxSessionHear(&sess, 106400, NULL, fakeWrite, &flog);
  dxSessionSeenLoaded(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(3, flog.n);
  TEST_ASSERT_EQUAL_UINT32(1000, flog.e[0].timeValue);
  TEST_ASSERT_EQUAL_UINT32(1100, flog.e[1].timeValue);
  TEST_ASSERT_EQUAL_UINT32(1200, flog.e[2].timeValue);
}

/* With the auto log off nothing is written but by hand. */
static void the_auto_log_switched_off_writes_nothing(void) {
  startSession(true);
  sess.autoLogOff = true;
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  h.ps = "  RED   ";
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  dxSessionHear(&sess, 98300, NULL, fakeWrite, &flog);
  dxSessionClose(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(0, flog.tries);
  TEST_ASSERT_TRUE(sess.catches.item[0].isNew);
  /* Caught now, since nothing will log it, so it stops no later scan. */
  TEST_ASSERT_TRUE(dxSeenHas(&sess.seen, 0x0935));
  /* The auto log switched back on does not write it later either. */
  sess.autoLogOff = false;
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  dxSessionHear(&sess, 98300, NULL, fakeWrite, &flog);
  dxSessionClose(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(0, flog.tries);
  TEST_ASSERT_FALSE(dxCatchDueLog(&sess.catches.item[0]));
  TEST_ASSERT_EQUAL(DX_WRITE_DONE, dxSessionWrite(&sess, 0, fakeWrite, &flog));
}

/* A catch heard before the seen set was read, with the auto log off, is
 * caught when the set arrives and never written by the auto log. */
static void a_late_seen_set_with_the_auto_log_off_writes_nothing(void) {
  startSession(false);
  sess.autoLogOff = true;
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  h.ps = "  RED   ";
  dxSessionHear(&sess, 93500, &h, fakeWrite, &flog);
  dxSessionSeenLoaded(&sess, fakeWrite, &flog);
  TEST_ASSERT_TRUE(sess.catches.item[0].isNew);
  TEST_ASSERT_TRUE(dxSeenHas(&sess.seen, 0x0935));
  sess.autoLogOff = false;
  dxSessionClose(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(0, flog.tries);
}

/* Learning the locals: each PI heard goes into the seen set as caught, is
 * not logged and is not listed, so a later scan passes over it. */
static void learning_marks_every_pi_caught_without_logging(void) {
  startSession(true);
  sess.learning = true;
  DxHearing a = heard(106400, 0x1064, 700, 1000);
  a.ps = " MAGIC  ";
  DxHearing b = heard(98300, 0x26FF, 482, 1100);
  dxSessionHear(&sess, 106400, &a, fakeWrite, &flog);
  dxSessionHear(&sess, 98300, &b, fakeWrite, &flog);
  dxSessionClose(&sess, fakeWrite, &flog);
  TEST_ASSERT_EQUAL_INT(0, flog.tries);
  TEST_ASSERT_TRUE(dxSeenHas(&sess.seen, 0x1064));
  TEST_ASSERT_TRUE(dxSeenHas(&sess.seen, 0x26FF));
  TEST_ASSERT_TRUE(sess.seenDirty);
  /* The locals take no place in the session's catches. */
  TEST_ASSERT_EQUAL_UINT8(0, sess.catches.count);
  /* Without a seen set to write to, it learns nothing. */
  startSession(false);
  sess.learning = true;
  dxSessionHear(&sess, 106400, &a, fakeWrite, &flog);
  TEST_ASSERT_FALSE(sess.seenDirty);
}

static void a_session_that_is_not_there_is_safe(void) {
  DxHearing h = heard(93500, 0x0935, 455, 1000);
  dxSessionHear(NULL, 93500, &h, fakeWrite, &flog);
  dxSessionReset(NULL);
}

/* The TEF CSV, the columns and formats the FMLIST converter reads. */
static void the_csv_header_is_the_pe5pvb_one(void) {
  TEST_ASSERT_EQUAL_STRING(
      "Date,Time,Frequency,PI,Signal,Stereo,TA,TP,PTY,ECC,PS,Radiotext\n",
      dxCatchCsvHeader());
}

static void a_catch_with_everything_heard_is_one_csv_line(void) {
  DxHearing h = heard(93500, 0x0935, 432, 1790487752);
  h.ps = "  RED   ";
  h.readings.stereo = true;
  h.rds.hasPty = true;
  h.rds.pty = 10;
  h.rds.hasFlags = true;
  h.rds.tp = true;
  h.rds.ta = false;
  h.rds.hasEcc = true;
  h.rds.ecc = 0xE0;
  const DxCatch *k = dxCatchesAdd(&list, &h, true);
  char line[128];
  size_t n = dxCatchCsvLine(k, line, sizeof(line));
  TEST_ASSERT_EQUAL_STRING(
      "27-09-2026,05:42:32,93.50 MHz,0935,43.2 dB\xCE\xBCV,"
      "\xE2\x80\xA2, ,\xE2\x80\xA2,10,E0,  RED   ,\n",
      line);
  TEST_ASSERT_EQUAL(strlen(line), n);
}

/* Nothing the decoder has not heard is written as a value: no name, no
 * programme type and no ECC stay empty or "--", flags not heard are a
 * space, and a time off the clock before it was set leaves the date and the
 * time empty, so the converter skips the row. */
static void a_catch_with_nothing_heard_writes_no_values(void) {
  DxHearing h = heard(98300, 0x26FF, 434, 5000);
  h.at.known = false;
  const DxCatch *k = dxCatchesAdd(&list, &h, false);
  char line[128];
  dxCatchCsvLine(k, line, sizeof(line));
  TEST_ASSERT_EQUAL_STRING(",,98.30 MHz,26FF,43.4 dB\xCE\xBCV, , , ,,--,,\n",
                           line);
}

/* The converter splits on every comma and knows no quotes. */
static void a_comma_in_the_name_becomes_a_space(void) {
  DxHearing h = heard(65930, 0x1234, -5, 1790487710);
  h.band = (uint8_t)BAND_OIRT;
  h.ps = "A,B,C\tD";
  const DxCatch *k = dxCatchesAdd(&list, &h, false);
  char line[128];
  dxCatchCsvLine(k, line, sizeof(line));
  TEST_ASSERT_EQUAL_STRING(
      "27-09-2026,05:41:50,65.93 MHz,1234,-0.5 dB\xCE\xBCV,"
      " , , ,,--,A B C D,\n",
      line);
}

/* The time and level of the loudest hearing, not of the last one. */
static void the_csv_line_is_the_loudest_hearing_and_its_time(void) {
  DxHearing loud = heard(93500, 0x0935, 455, 1790487710);
  DxHearing weak = heard(93500, 0x0935, 401, 1790496442);
  dxCatchesAdd(&list, &loud, false);
  const DxCatch *k = dxCatchesAdd(&list, &weak, false);
  TEST_ASSERT_EQUAL_UINT32(1790496442, k->last.value);
  char line[128];
  dxCatchCsvLine(k, line, sizeof(line));
  TEST_ASSERT_EQUAL_STRING_LEN("27-09-2026,05:41:50,93.50 MHz,0935,45.5 ", line,
                               40);
}

/* A hearing that lacks a flag keeps the one heard before; one that has it
 * replaces it. */
static void rds_flags_are_kept_until_heard_again(void) {
  DxHearing a = heard(93500, 0x0935, 455, 1000);
  a.rds.hasPty = true;
  a.rds.pty = 10;
  a.rds.hasFlags = true;
  a.rds.ta = true;
  a.rds.hasEcc = true;
  a.rds.ecc = 0xE0;
  dxCatchesAdd(&list, &a, false);
  DxHearing bare = heard(93500, 0x0935, 455, 1100);
  const DxCatch *k = dxCatchesAdd(&list, &bare, false);
  TEST_ASSERT_TRUE(k->rds.hasPty);
  TEST_ASSERT_EQUAL_UINT8(10, k->rds.pty);
  TEST_ASSERT_TRUE(k->rds.ta);
  TEST_ASSERT_EQUAL_UINT8(0xE0, k->rds.ecc);
  DxHearing b = heard(93500, 0x0935, 455, 1200);
  b.rds.hasPty = true;
  b.rds.pty = 1;
  b.rds.hasFlags = true;
  b.rds.ta = false;
  b.rds.hasEcc = true;
  b.rds.ecc = 0xE2;
  k = dxCatchesAdd(&list, &b, false);
  TEST_ASSERT_EQUAL_UINT8(1, k->rds.pty);
  TEST_ASSERT_FALSE(k->rds.ta);
  TEST_ASSERT_EQUAL_UINT8(0xE2, k->rds.ecc);
}

/* A line that does not fit says so by its length, and a NULL writes none. */
static void a_short_csv_buffer_and_null_are_safe(void) {
  DxHearing h = heard(93500, 0x0935, 455, 1790487710);
  const DxCatch *k = dxCatchesAdd(&list, &h, false);
  char line[16];
  size_t n = dxCatchCsvLine(k, line, sizeof(line));
  TEST_ASSERT_TRUE(n >= sizeof(line));
  TEST_ASSERT_EQUAL(15, strlen(line));
  TEST_ASSERT_EQUAL(0, dxCatchCsvLine(NULL, line, sizeof(line)));
  TEST_ASSERT_EQUAL(0, dxCatchCsvLine(k, NULL, 0));
}

/* The CSV goes oldest first by the time of each row, since the converter
 * walks a GPS track forward; rows with no time, which it skips, first. */
static void catches_by_time_are_oldest_first_untimed_first(void) {
  DxHearing a = heard(93500, 0x0935, 455, 3000);
  DxHearing b = heard(98300, 0x26FF, 434, 1000);
  DxHearing c = heard(91100, 0x3712, 452, 0);
  c.at.known = false;
  DxHearing d = heard(106400, 0x1064, 569, 2000);
  dxCatchesAdd(&list, &a, false);
  dxCatchesAdd(&list, &b, false);
  dxCatchesAdd(&list, &c, false);
  dxCatchesAdd(&list, &d, false);
  uint8_t order[DX_CATCHES_MAX];
  TEST_ASSERT_EQUAL_UINT8(4, dxCatchesByTime(&list, order));
  TEST_ASSERT_EQUAL_UINT16(0x3712, list.item[order[0]].pi);
  TEST_ASSERT_EQUAL_UINT16(0x26FF, list.item[order[1]].pi);
  TEST_ASSERT_EQUAL_UINT16(0x1064, list.item[order[2]].pi);
  TEST_ASSERT_EQUAL_UINT16(0x0935, list.item[order[3]].pi);
  TEST_ASSERT_EQUAL_UINT8(0, dxCatchesByTime(NULL, order));
  TEST_ASSERT_EQUAL_UINT8(0, dxCatchesByTime(&list, NULL));
}

/* A best heard before the clock was set stays the best, and the CSV row is
 * the strongest hearing that had a time, so the converter does not skip it.
 * A louder hearing with no time never takes a timed row's place. */
static void the_csv_row_is_the_strongest_hearing_with_a_time(void) {
  DxHearing early = heard(93500, 0x0935, 455, 100);
  early.at.known = false;
  dxCatchesAdd(&list, &early, false);
  DxHearing later = heard(93500, 0x0935, 401, 1790487710);
  const DxCatch *k = dxCatchesAdd(&list, &later, false);
  TEST_ASSERT_EQUAL_INT16(455, k->best.levelDbuVTenths);
  TEST_ASSERT_TRUE(k->timedAt.known);
  TEST_ASSERT_EQUAL_UINT32(1790487710, k->timedAt.value);
  TEST_ASSERT_EQUAL_INT16(401, k->timed.levelDbuVTenths);
  char line[128];
  dxCatchCsvLine(k, line, sizeof(line));
  TEST_ASSERT_EQUAL_STRING_LEN("27-09-2026,05:41:50,93.50 MHz,0935,40.1 ", line,
                               40);
  DxHearing weaker = heard(93500, 0x0935, 390, 1790496442);
  k = dxCatchesAdd(&list, &weaker, false);
  TEST_ASSERT_EQUAL_UINT32(1790487710, k->timedAt.value);
  DxHearing louderUntimed = heard(93500, 0x0935, 480, 200);
  louderUntimed.at.known = false;
  k = dxCatchesAdd(&list, &louderUntimed, false);
  TEST_ASSERT_EQUAL_INT16(480, k->best.levelDbuVTenths);
  TEST_ASSERT_EQUAL_UINT32(1790487710, k->timedAt.value);
  TEST_ASSERT_EQUAL_INT16(401, k->timed.levelDbuVTenths);
}

/* A shoulder hearing is held against the strongest of all, timed or not,
 * so the clock being set does not let a catch move beside its station. */
static void the_clock_does_not_weaken_the_move_guard(void) {
  DxHearing early = heard(100000, 0x0935, 500, 100);
  early.at.known = false;
  dxCatchesAdd(&list, &early, false);
  DxHearing later = heard(100000, 0x0935, 200, 1790487710);
  dxCatchesAdd(&list, &later, false);
  DxHearing shoulder = heard(100100, 0x0935, 250, 1790487720);
  const DxCatch *k = dxCatchesAdd(&list, &shoulder, false);
  TEST_ASSERT_EQUAL_UINT32(100000, k->khz);
}

/* A full list says how many it pushed out, so the page can say so. */
static void a_full_list_counts_what_it_drops(void) {
  for (uint32_t i = 0; i < DX_CATCHES_MAX + 3; i++) {
    DxHearing h = heard(88000 + i * 200, (uint16_t)(0x1000 + i), 400, 1000 + i);
    dxCatchesAdd(&list, &h, false);
  }
  TEST_ASSERT_EQUAL_UINT8(DX_CATCHES_MAX, list.count);
  TEST_ASSERT_EQUAL_UINT16(3, list.dropped);
}

/* The converter writes the name inside double quotes in its own output. */
static void a_quote_in_the_name_becomes_a_space(void) {
  DxHearing h = heard(93500, 0x0935, 455, 1790487710);
  h.ps = "R\"1\"";
  const DxCatch *k = dxCatchesAdd(&list, &h, false);
  char line[128];
  dxCatchCsvLine(k, line, sizeof(line));
  TEST_ASSERT_NOT_NULL(strstr(line, ",R 1 ,"));
  TEST_ASSERT_NULL(strchr(line, '"'));
}

/* A band that is not a real one has no frequency to write. */
static void a_catch_on_no_band_writes_no_line(void) {
  DxHearing h = heard(93500, 0x0935, 455, 1790487710);
  h.band = 0xFF;
  const DxCatch *k = dxCatchesAdd(&list, &h, false);
  char line[128];
  TEST_ASSERT_EQUAL(0, dxCatchCsvLine(k, line, sizeof(line)));
}

/* A row names its catch by PI and channel; the same PI 100 kHz away or on
 * another channel is not it. */
static void a_catch_is_found_by_its_exact_channel(void) {
  DxHearing a = heard(93500, 0x0935, 455, 1000);
  DxHearing b = heard(97200, 0x0935, 400, 1100);
  dxCatchesAdd(&list, &a, false);
  dxCatchesAdd(&list, &b, false);
  TEST_ASSERT_EQUAL_INT16(1, dxCatchesFindExact(&list, 0x0935, 93500));
  TEST_ASSERT_EQUAL_INT16(0, dxCatchesFindExact(&list, 0x0935, 97200));
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFindExact(&list, 0x0935, 93600));
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFindExact(&list, 0x26FF, 93500));
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFindExact(NULL, 0x0935, 93500));
}

/* A catch that moves to the stronger channel beside it leaves its timed
 * row behind, since that was heard on the other channel. */
static void a_move_drops_the_timed_row_of_the_old_channel(void) {
  DxHearing a = heard(98000, 0xC201, 400, 1790487710);
  dxCatchesAdd(&list, &a, false);
  DxHearing b = heard(98100, 0xC201, 450, 0);
  b.at.known = false;
  const DxCatch *k = dxCatchesAdd(&list, &b, false);
  TEST_ASSERT_EQUAL_UINT32(98100, k->khz);
  TEST_ASSERT_FALSE(k->timedAt.known);
  DxHearing c = heard(98100, 0xC201, 420, 1790487800);
  k = dxCatchesAdd(&list, &c, false);
  TEST_ASSERT_TRUE(k->timedAt.known);
  TEST_ASSERT_EQUAL_INT16(420, k->timed.levelDbuVTenths);
}

/* Two catches of one PI can end up beside each other when one moves. A
 * hearing on the very channel of one goes to that one. */
static void a_hearing_goes_to_the_catch_on_its_very_channel(void) {
  DxHearing a = heard(100000, 0x1234, 400, 1000);
  DxHearing b = heard(100200, 0x1234, 400, 1100);
  dxCatchesAdd(&list, &a, false);
  dxCatchesAdd(&list, &b, false);
  DxHearing louder = heard(100100, 0x1234, 450, 1200);
  dxCatchesAdd(&list, &louder, false);
  TEST_ASSERT_EQUAL_UINT8(2, list.count);
  TEST_ASSERT_EQUAL_UINT32(100100, list.item[0].khz);
  TEST_ASSERT_EQUAL_UINT32(100000, list.item[1].khz);
  TEST_ASSERT_EQUAL_INT16(1, dxCatchesFind(&list, 0x1234, 100000));
  DxHearing again = heard(100000, 0x1234, 400, 1300);
  const DxCatch *k = dxCatchesAdd(&list, &again, false);
  TEST_ASSERT_EQUAL_UINT32(100000, k->khz);
  TEST_ASSERT_EQUAL_UINT16(2, k->count);
  TEST_ASSERT_EQUAL_UINT16(2, list.item[1].count);
}

/* A catch keeps its id however the list moves, and a dropped one is gone. */
static void a_catch_keeps_its_id_as_the_list_moves(void) {
  DxHearing a = heard(93500, 0x0935, 455, 1000);
  DxHearing b = heard(98300, 0x26FF, 434, 1100);
  const uint16_t idA = dxCatchesAdd(&list, &a, false)->id;
  const uint16_t idB = dxCatchesAdd(&list, &b, false)->id;
  TEST_ASSERT_NOT_EQUAL(0, idA);
  TEST_ASSERT_NOT_EQUAL(idA, idB);
  TEST_ASSERT_EQUAL_INT16(1, dxCatchesFindId(&list, idA));
  DxHearing again = heard(93500, 0x0935, 455, 1200);
  dxCatchesAdd(&list, &again, false);
  TEST_ASSERT_EQUAL_INT16(0, dxCatchesFindId(&list, idA));
  TEST_ASSERT_EQUAL_UINT16(idA, list.item[0].id);
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFindId(&list, 0));
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFindId(NULL, idA));
  for (uint32_t i = 0; i < DX_CATCHES_MAX; i++) {
    DxHearing h = heard(88000 + i * 200, (uint16_t)(0x1000 + i), 400, 2000 + i);
    dxCatchesAdd(&list, &h, false);
  }
  TEST_ASSERT_EQUAL_INT16(-1, dxCatchesFindId(&list, idA));
}

/* Past 65535 the ids start again at 1, never 0. */
static void ids_wrap_past_zero(void) {
  list.lastId = UINT16_MAX;
  DxHearing a = heard(93500, 0x0935, 455, 1000);
  TEST_ASSERT_EQUAL_UINT16(1, dxCatchesAdd(&list, &a, false)->id);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(a_first_hearing_is_a_new_catch);
  RUN_TEST(the_same_catch_again_counts_up_and_comes_first);
  RUN_TEST(the_same_pi_beside_it_is_one_catch_on_the_stronger);
  RUN_TEST(a_louder_reading_at_another_width_does_not_move_it);
  RUN_TEST(the_best_follows_the_width);
  RUN_TEST(two_channels_apart_is_two_catches);
  RUN_TEST(a_catch_is_found_on_its_channel_or_beside_it);
  RUN_TEST(a_full_list_drops_its_oldest);
  RUN_TEST(a_refresh_takes_the_name_and_a_stronger_level);
  RUN_TEST(a_refresh_beside_the_catch_reaches_it);
  RUN_TEST(only_a_new_catch_is_due_and_only_once);
  RUN_TEST(an_entry_is_the_strongest_hearing_since_the_last);
  RUN_TEST(a_hearing_beside_a_catch_stays_out_of_its_entry);
  RUN_TEST(a_catch_that_moves_is_due_again_on_its_new_channel);
  RUN_TEST(a_catch_the_log_holds_already_is_marked_written);
  RUN_TEST(a_write_that_fails_marks_nothing);
  RUN_TEST(an_oirt_catch_is_logged_as_oirt);
  RUN_TEST(the_log_entry_is_the_catch_at_its_best);
  RUN_TEST(nothing_happens_without_somewhere_to_put_it);
  RUN_TEST(the_seen_set_marks_a_pi_once);
  RUN_TEST(a_full_seen_set_forgets_the_first_caught);
  RUN_TEST(the_seen_set_goes_to_bytes_and_back);
  RUN_TEST(a_file_that_is_not_ours_is_an_empty_set);
  RUN_TEST(a_catch_is_written_once_its_name_arrives);
  RUN_TEST(a_new_catch_the_log_holds_goes_in_the_seen_set);
  RUN_TEST(a_nameless_catch_is_written_on_leaving);
  RUN_TEST(a_failed_write_is_tried_again_only_on_leaving);
  RUN_TEST(another_pi_on_the_channel_writes_the_first_before_it);
  RUN_TEST(closing_and_opening_again_counts_nothing);
  RUN_TEST(new_needs_a_seen_set_that_was_read);
  RUN_TEST(a_catch_caught_before_is_not_written);
  RUN_TEST(a_seen_set_read_late_still_marks_the_catch);
  RUN_TEST(stations_taking_turns_on_a_channel_count_once);
  RUN_TEST(a_catch_logged_by_hand_is_not_written_again);
  RUN_TEST(a_hand_log_before_the_set_was_read_goes_in_it_later);
  RUN_TEST(a_catch_that_moves_on_the_dial_is_written_there_at_once);
  RUN_TEST(a_hand_log_reaches_only_the_catch_on_its_channel);
  RUN_TEST(a_late_read_waits_for_the_name_and_closing_tries_again);
  RUN_TEST(catches_written_together_go_in_oldest_first);
  RUN_TEST(the_auto_log_switched_off_writes_nothing);
  RUN_TEST(a_late_seen_set_with_the_auto_log_off_writes_nothing);
  RUN_TEST(learning_marks_every_pi_caught_without_logging);
  RUN_TEST(a_session_that_is_not_there_is_safe);
  RUN_TEST(the_csv_header_is_the_pe5pvb_one);
  RUN_TEST(a_catch_with_everything_heard_is_one_csv_line);
  RUN_TEST(a_catch_with_nothing_heard_writes_no_values);
  RUN_TEST(a_comma_in_the_name_becomes_a_space);
  RUN_TEST(the_csv_line_is_the_loudest_hearing_and_its_time);
  RUN_TEST(rds_flags_are_kept_until_heard_again);
  RUN_TEST(a_short_csv_buffer_and_null_are_safe);
  RUN_TEST(catches_by_time_are_oldest_first_untimed_first);
  RUN_TEST(the_csv_row_is_the_strongest_hearing_with_a_time);
  RUN_TEST(the_clock_does_not_weaken_the_move_guard);
  RUN_TEST(a_catch_is_found_by_its_exact_channel);
  RUN_TEST(a_move_drops_the_timed_row_of_the_old_channel);
  RUN_TEST(a_hearing_goes_to_the_catch_on_its_very_channel);
  RUN_TEST(a_catch_keeps_its_id_as_the_list_moves);
  RUN_TEST(ids_wrap_past_zero);
  RUN_TEST(a_full_list_counts_what_it_drops);
  RUN_TEST(a_quote_in_the_name_becomes_a_space);
  RUN_TEST(a_catch_on_no_band_writes_no_line);
  return UNITY_END();
}
