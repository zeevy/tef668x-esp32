/* The DX session. See dx_task.h. */
#include "dx_task.h"
#include "debug_log.h"

#include "band_scan_task.h"
#include "core/band_plan.h"
#include "core/clock.h"
#include "core/dx.h"
#include "core/dx_timing.h"
#include "core/dx_watch.h"
#include "core/rds_country.h"
#include "core/settings.h"
#include "core/strings.h"
#include "drivers/dx_seen_fs.h"
#include "drivers/dx_sweep_fs.h"
#include "drivers/dx_timing_fs.h"
#include "drivers/logbook_fs.h"
#include "memory_store.h"
#include "net/ntp.h"
#include "net/update_check.h"
#include "radio_task.h"
#include "reply_times.h"

#include <Arduino.h>
#include <esp_random.h>
#include <stdlib.h>
#include <string.h>

/* The catches, the PIs ever caught, and the catch the dial is on. On the
 * heap, taken the first time DX mode opens: 4.2 KB the static segment has
 * no room for. NULL when the heap could not give it, and then nothing is
 * caught. */
static DxSession *sSession = NULL;

/* Whether DX mode is open, and the width it has in force, as the screen
 * last said. */
static bool sOpen = false;
static uint16_t sWidthKHz = DX_BANDWIDTH_DEFAULT_KHZ;

/* Where the session's notices go, the screen's confirmation line. */
static DxSay sSay = NULL;
static DxSayLogged sSayLogged = NULL;

static void say(const char *text) {
  if (sSay != NULL) {
    sSay(text);
  }
}

void dxTaskSetSay(DxSay sayTo, DxSayLogged logged) {
  sSay = sayTo;
  sSayLogged = logged;
}

void dxTaskSetWidth(uint16_t khz) {
  sWidthKHz = khz;
}

static DxTime timeNow(void) {
  DxTime t;
  uint32_t epoch = 0;
  t.known = ntpEpochUtc(&epoch);
  t.value = t.known ? epoch : millis();
  return t;
}

/* `ctx` is the snapshot the write was decided on, for the radio text, or
 * NULL when there is none to hand and the entry goes without. */
static LogbookWrite writeLog(void *ctx, const LogbookEntry *e) {
  LogbookEntry entry = *e;
  if (ctx != NULL) {
    dxTaskAddRadioText(&entry, (const RadioSnapshot *)ctx);
  }
  const LogbookWrite w = logbookFsAppend(&entry);
  if (w == LOGBOOK_NOT_WRITTEN) {
    DebugLog.println(F("[dx] a catch could not be written to the log"));
  }
  return w;
}

/* A failed save stays dirty and is tried again once the set has changed,
 * and when DX mode closes, not on every poll. */
static void saveSeenIfDirty(bool evenIfTried) {
  if (sSession == NULL || !sSession->seenDirty ||
      (sSession->seenTried && !evenIfTried)) {
    return;
  }
  /* A learning pass saves once, when it ends, not once for each local. */
  if (sSession->learning) {
    return;
  }
  sSession->seenTried = true;
  if (dxSeenFsSave(&sSession->seen)) {
    sSession->seenDirty = false;
  } else {
    DebugLog.println(F("[dx] the seen PIs could not be saved"));
  }
}

/* Say on the page what a hold did, naming the frequency written, which can
 * be the channel beside the one tuned when the catch was heard stronger
 * there. */
static void confirmLog(DxWriteResult r, const DxCatch *k) {
  if (r == DX_WRITE_DONE && k != NULL) {
    if (sSayLogged != NULL) {
      sSayLogged(k->band, k->khz);
    }
  } else {
    say(txt(r == DX_WRITE_NOTHING_NEW || r == DX_WRITE_IN_LOG
                ? STR_COMMON_ALREADY_LOGGED
                : STR_COMMON_NOT_LOGGED));
  }
}

/*
 * Whether a PI is heard now as the channel's own, for the catches and the
 * scanner alike. Not until the tuner is on DX mode's width: a reading through
 * the radio's own wider filter reads louder, and a catch's best and its move to
 * the stronger channel are only fair at one width. The width checked is the
 * chip's own reading of its filter. It lands one soft mute after the setting,
 * and it reads back exactly the width set at every one of the sixteen FM
 * widths.
 */
static bool heardNow(const RadioSnapshot &snap) {
  const SeekReading reading =
      radioSeekReading(&snap.quality, snap.qualityValid);
  return dxPiHeardNow(&snap.rds, &reading) &&
         snap.quality.bandwidthKHz == sWidthKHz;
}

/* The scanner, and the tune it is waiting to hand the radio: a full queue takes
 * it at the next poll rather than losing it. */
static bool sScanTunePending = false;
static uint32_t sScanTuneKHz = 0;
/* Whether that tune is the one back to where the scan started. */
static bool sScanTuneBack = false;
static bool sScanMuted = false;

/* The DX SETUP menu's settings, as settingsApplyLive last pushed them. Until it
 * does, the defaults settingsDefaults gives. */
static DxSetup sSetup = {DX_STOP_NEW,
                         DX_RANGE_BAND_LESS_MEMORY,
                         1,
                         MEMORY_SLOT_COUNT,
                         false,
                         true,
                         true,
                         DX_SCAN_DWELL_MS,
                         DX_BANDWIDTH_DEFAULT_KHZ,
                         true,
                         true,
                         false};

/* The scan and what it walks. On the heap with the catches, taken the
 * first time DX mode opens, since the static segment has no room left;
 * before then every scan reads as idle. */
typedef struct {
  DxScan scan;
  DxScanWalk walk;
  DxScanBand band;
} ScanStore;
static ScanStore *sScanStore = NULL;
static const DxScan kNoScan = {};

/*
 * What a band walk passes over: a channel the band plan gives to another
 * band, such as OIRT inside the full FM region, since a tune there moves
 * the radio to that band and leaves it remembering the scan's channel as
 * its own; and, for the band less the memory channels, a station already
 * stored.
 */
static bool scanSkip(void *ctx, uint32_t khz) {
  const DxScanWalk *w = (const DxScanWalk *)ctx;
  BandId owner;
  if (!bandForFrequency(&w->plan, khz, &owner) || owner != w->band) {
    return true;
  }
  return w->skipStored &&
         memoryStoreFind((uint8_t)w->band, khz) != MEMORY_NO_SLOT;
}

/* The memory walk: slot memFirst onwards, passing over an empty slot, one
 * stored on another band, and one the band plan does not give to this band
 * now, such as a channel stored before the FM region changed, which the
 * radio would not tune or would tune on another band. */
static bool scanMemoryChannel(void *ctx, uint16_t pos, uint32_t *khz) {
  const DxScanWalk *w = (const DxScanWalk *)ctx;
  MemoryChannel ch;
  BandId owner;
  if (!memoryStoreRead((int)(w->memFirst - 1 + pos), &ch) || ch.freqKHz == 0 ||
      ch.band != (uint8_t)w->band ||
      !bandForFrequency(&w->plan, ch.freqKHz, &owner) || owner != w->band) {
    return false;
  }
  *khz = ch.freqKHz;
  return true;
}

static void scanTake(DxScanAction a) {
  if (a.tune) {
    sScanTunePending = true;
    sScanTuneKHz = a.khz;
    sScanTuneBack = sScanStore->scan.state != DX_SCAN_RUNNING;
  }
}

static void scanSendTune(void) {
  if (!sScanTunePending) {
    return;
  }
  /* A walk tune left over once the scan has stopped is dropped. Somebody
   * else has the dial now. */
  if (!sScanTuneBack && sScanStore->scan.state != DX_SCAN_RUNNING) {
    sScanTunePending = false;
    return;
  }
  RadioCommand tune;
  memset(&tune, 0, sizeof(tune));
  tune.kind = RADIO_TUNE;
  tune.freqKHz = sScanTuneKHz;
  /* The tune back is waited for. The scan is then over only once the
   * snapshot is back where it started. So nothing that asks whether the dial
   * is walking finds the last channel looked at. A tune the queue did not
   * take is kept for the next poll, and counts as walking until then. The
   * tunes during the walk are not waited for: the scan checks each landing
   * itself. */
  if (sScanTuneBack) {
    if (radioPostAndSettle(&tune, RADIO_TUNE_BACK_WAIT_MS, NULL) !=
        RADIO_POST_BUSY) {
      sScanTunePending = false;
    }
  } else if (radioPost(&tune)) {
    sScanTunePending = false;
  }
}

/* Muted while running, as a seek and the band scan are, so the channels in
 * between are not heard, unless the DX SETUP menu says to hear them. A band
 * scan running at the same time keeps its own mute. */
static void scanSyncMute(void) {
  /* Kept while the tune back waits, so the last channel is not heard. */
  const bool want =
      sScanStore != NULL && sSetup.mute &&
      (sScanStore->scan.state == DX_SCAN_RUNNING || sScanTunePending);
  if (want == sScanMuted) {
    return;
  }
  if (want || !bandScanActive()) {
    radioSetScanning(want);
  }
  sScanMuted = want;
}

/* Learning the locals: a pass over the whole band that marks every PI heard as
 * caught and stops on none. The session says whether one is under way; it needs
 * a session to learn into. */
static bool learningNow(void) {
  return sSession != NULL && sSession->learning;
}

/* ------------------------------------------------------ the preset watch */

/*
 * One AF_Update check every 2 s: about 1 % of the RDS blocks of the station
 * heard, at the 0.9 of a block a check costs as measured on this radio, and
 * twelve presets gone round in under half a minute.
 */
#define DX_WATCH_EVERY_MS 2000
/* As long as the long press beep, so a flag is heard as more than a key. */
#define DX_WATCH_BEEP_MS 200

/* On the heap, about 1.6 KB, taken each time DX mode opens and given back
 * when it closes, since each opening starts from fresh floors anyway. NULL
 * when the heap could not give it, and then nothing is watched. */
static DxWatch *sWatch = NULL;
static RadioAfCheck sWatchCheck;
static RadioAfSeries sWatchSeries;
static bool sWatchPending = false; /* A check sent and not yet read. */
static uint32_t sWatchLastMs = 0;
static uint8_t sWatchCount = 0;
/* The last preset flagged, for GET /api/dx: 0 for none. */
static uint32_t sWatchUpKHz = 0;
static int16_t sWatchUpRise = 0;
static uint32_t sWatchUpMs = 0;

/* The FM presets in DX mode's range, as the scanner's memory walk takes
 * them, less the dial's own channel and any stored twice. */
static void watchChannels(const RadioSnapshot &snap) {
  uint32_t khz[DX_WATCH_MAX] = {};
  uint8_t n = 0;
  BandPlanConfig plan;
  const bool havePlan = radioTaskPlan(&plan);
  for (uint8_t slot = sSetup.memFirst;
       havePlan && slot <= sSetup.memLast && slot <= MEMORY_SLOT_COUNT;
       slot++) {
    MemoryChannel ch;
    BandId owner;
    if (!memoryStoreRead((int)slot - 1, &ch) || ch.freqKHz == 0 ||
        ch.band != (uint8_t)snap.settings.band ||
        !bandForFrequency(&plan, ch.freqKHz, &owner) ||
        owner != snap.settings.band || ch.freqKHz == snap.settings.freqKHz) {
      continue;
    }
    bool twice = false;
    for (uint8_t i = 0; i < n; i++) {
      twice = twice || khz[i] == ch.freqKHz;
    }
    if (!twice) {
      khz[n++] = ch.freqKHz;
    }
  }
  dxWatchSetChannels(sWatch, khz, n);
  sWatchCount = n;
}

static void watchFlag(uint32_t khz, int16_t rise) {
  sWatchUpKHz = khz;
  sWatchUpRise = rise;
  sWatchUpMs = millis();
  char freq[16];
  bandFormatFrequency(BAND_FM, khz, freq, sizeof(freq));
  char text[32];
  snprintf(text, sizeof(text), txt(STR_DX_FMT_WATCH_UP), freq,
           (int)((rise + 5) / 10));
  say(text);
  if (sSetup.beep) {
    radioBeep(DX_WATCH_BEEP_MS);
  }
  DebugLog.printf("[dx] watch: %s\n", text);
}

/* The watch's turn in a DX page poll: read the check that came back, or
 * send the next when one is due. Not while a scan, a learning pass or a
 * sweep runs, which move the dial or need the radio to themselves. */
static void watchStep(const RadioSnapshot &snap) {
  if (sWatch == NULL) {
    return;
  }
  if (sWatchPending) {
    if (radioAfBusy()) {
      return;
    }
    sWatchPending = false;
    /* A check that overlapped a reply going out is not a reading: every check
     * taken during one that read high, and none outside one did, in checks
     * measured on this radio. */
    const bool nearReply =
        replyTimesGapMs(sWatchCheck.atMs,
                        sWatchCheck.atMs + (sWatchCheck.us + 999u) / 1000u) ==
        0;
    const bool ok = !sWatchSeries.stopped && sWatchSeries.done == 1 &&
                    sWatchCheck.err == TEF668X_OK && !nearReply;
    int16_t rise = 0;
    if (dxWatchFeed(sWatch, sWatchSeries.khz, sWatchCheck.level, ok, &rise) ==
        DX_WATCH_UP) {
      watchFlag(sWatchSeries.khz, rise);
    }
    return;
  }
  const bool scanning =
      sScanStore != NULL && sScanStore->scan.state == DX_SCAN_RUNNING;
  if (!sSetup.watch || scanning || learningNow() || radioSweepBusy() ||
      radioAfBusy() ||
      (uint32_t)(millis() - sWatchLastMs) < DX_WATCH_EVERY_MS) {
    return;
  }
  sWatchLastMs = millis();
  watchChannels(snap);
  uint32_t khz = 0;
  if (!dxWatchNext(sWatch, &khz)) {
    return;
  }
  /* Through the filter DX mode has in force, so a check hears what the
   * page does, not the neighbours a wider one would. */
  const uint16_t width =
      snap.qualityValid && snap.quality.bandwidthKHz != 0 &&
              bandBandwidthAllowed(BAND_FM, snap.quality.bandwidthKHz)
          ? snap.quality.bandwidthKHz
          : sSetup.widthKHz;
  memset(&sWatchSeries, 0, sizeof(sWatchSeries));
  sWatchSeries.khz = khz;
  sWatchSeries.widthKHz = width;
  sWatchSeries.count = 1;
  sWatchSeries.everyMs = 10;
  sWatchSeries.check = &sWatchCheck;
  sWatchPending = radioAfStart(&sWatchSeries);
}

bool dxTaskWatchState(uint8_t *count, uint32_t *upKHz, int16_t *riseTenths,
                      uint32_t *upMs) {
  if (count != NULL) {
    *count = sWatchCount;
  }
  if (upKHz != NULL) {
    *upKHz = sWatchUpKHz;
  }
  if (riseTenths != NULL) {
    *riseTenths = sWatchUpRise;
  }
  if (upMs != NULL) {
    *upMs = sWatchUpMs;
  }
  return sSetup.watch && sWatch != NULL && sOpen;
}

/* The end of a learning pass, however it ends, and the one save of all it
 * learned. */
static void learnEnd(void) {
  if (!learningNow()) {
    return;
  }
  sSession->learning = false;
  saveSeenIfDirty(true);
}

/*
 * Whether the PI heard now is NEW: its catch, which hear() has just made or
 * met, was never caught before, and it was not in the seen set before this
 * poll, which a NEW catch written earlier in the session now is. With no
 * seen set to ask, or no catches at all, NEW cannot be told, and every PI
 * counts as NEW so a scan stopping on NEW still stops rather than walking
 * past everything.
 */
static bool heardIsNew(const RadioSnapshot &snap, bool seenBefore) {
  if (sSession == NULL || !sSession->seenKnown) {
    return true;
  }
  const int16_t at =
      dxCatchesFind(&sSession->catches, snap.rds.pi, snap.settings.freqKHz);
  return at >= 0 && sSession->catches.item[at].isNew && !seenBefore;
}

/* Whether the stop rule stops on the PI heard now. */
static bool stopsOn(const RadioSnapshot &snap, bool seenBefore) {
  if (learningNow()) {
    return false;
  }
  switch (sSetup.stop) {
    case DX_STOP_ANY_PI:
      return true;
    case DX_STOP_NEVER:
      return false;
    case DX_STOP_NEW:
    default:
      return heardIsNew(snap, seenBefore);
  }
}

/*
 * Whether the scan may leave a channel whose PI it has heard. Under Never a
 * NEW catch is logged once its name has come, so the scan waits out the
 * dwell for the name rather than leaving at once and logging it with only
 * its PI. Every other rule stops, or has nothing to log.
 */
static bool heardEnough(const RadioSnapshot &snap) {
  if (learningNow() || sSetup.stop != DX_STOP_NEVER || sSession == NULL) {
    return true;
  }
  const int16_t at =
      dxCatchesFind(&sSession->catches, snap.rds.pi, snap.settings.freqKHz);
  if (at < 0) {
    return true;
  }
  const DxCatch *k = &sSession->catches.item[at];
  return !dxCatchDueLog(k) || k->hasPs || sSession->autoLogOff;
}

/* The level sweep: the latest, what it is held against, and the one the radio
 * task is filling in. On the heap with the catches, taken the first time DX
 * mode opens or the API asks. */
typedef struct {
  DxSweep live;   /* The latest sweep. No channels before any. */
  DxSweep base;   /* The baseline, when baseN is not 0. */
  DxSweep peak;   /* The highest of each channel since DX mode opened. */
  DxSweep next;   /* The radio task's while `running`. */
  uint8_t baseN;  /* How many sweeps the baseline comes from; 0 for none. */
  bool baseFixed; /* One sweep fixed by hand rather than the median. */
  bool running;
  bool abandoned;    /* The last sweep was ended before the top. */
  uint16_t revision; /* Moves whenever live, base or peak change. */
  /* When each kept sweep was taken, newest first, so the Scope Baseline list
   * is written without reading the 7 KB of them back from littlefs. */
  uint8_t keptCount;
  DxKeptSweep kept[DX_SWEEP_KEEP];
  /* Where the fixed baseline is among the kept sweeps, 0 the newest, moved
   * on as each new sweep is kept; -1 when it is not one of them. */
  int8_t baseKept;
} SweepStore;
static SweepStore *sSweep = NULL;

/* The same sweep: the same channels, time and readings. */
static bool sameSweep(const DxSweep *a, const DxSweep *b) {
  return dxSweepSameChannels(a, b) && a->timeKnown == b->timeKnown &&
         a->at == b->at &&
         memcmp(a->level, b->level, a->count * sizeof(a->level[0])) == 0;
}

/* Note when each sweep in `h` was taken. */
static void noteKept(const DxSweepHistory *h) {
  sSweep->keptCount = h->count;
  for (uint8_t i = 0; i < h->count; i++) {
    sSweep->kept[i].timeKnown = h->item[i].timeKnown;
    sSweep->kept[i].at = h->item[i].at;
  }
}

/* The median of the kept sweeps in `h`, leaving out the live one. */
static void sweepMedian(const DxSweepHistory *h) {
  const uint8_t skip =
      h->count > 0 && sameSweep(&h->item[0], &sSweep->live) ? 1 : 0;
  sSweep->baseN = dxSweepMedian(&h->item[skip], (uint8_t)(h->count - skip),
                                &sSweep->live, &sSweep->base);
}

/* Taken, and the kept sweeps and the baseline fixed by hand read, once. A
 * history is 7 KB, so it is on the heap only while it is read. */
static void sweepEnsure(void) {
  if (sSweep == NULL) {
    /* Zeroed is no sweep and no baseline. */
    sSweep = (SweepStore *)calloc(1, sizeof(*sSweep));
    if (sSweep == NULL) {
      DebugLog.println(F("[dx] no memory for the level sweep"));
      return;
    }
    /* Somewhere new each boot, so a page left open over a restart does
     * not take a new sweep for the one it has. */
    sSweep->revision = (uint16_t)esp_random();
  } else if (sSweep->live.count > 0 || sSweep->running) {
    return;
  }
  DxSweepHistory *h = (DxSweepHistory *)malloc(sizeof(*h));
  if (h == NULL) {
    return;
  }
  /* The revision moves only when something was read, so asking again while
   * no sweep is kept does not tell every open page that one changed. */
  bool read = false;
  dxSweepFsLoad(DX_SWEEP_BASE_PATH, h);
  if (h->count > 0) {
    sSweep->base = h->item[0];
    sSweep->baseFixed = true;
    sSweep->baseN = 1;
    read = true;
  }
  dxSweepFsLoad(DX_SWEEP_PATH, h);
  noteKept(h);
  /* Found by the whole sweep, so one taken while the clock was not set is
   * found too. */
  sSweep->baseKept = -1;
  for (uint8_t i = 0; sSweep->baseFixed && i < h->count; i++) {
    if (sameSweep(&h->item[i], &sSweep->base)) {
      sSweep->baseKept = (int8_t)i;
      break;
    }
  }
  if (h->count > 0) {
    sSweep->live = h->item[0];
    if (!sSweep->baseFixed) {
      sweepMedian(h);
    }
    read = true;
  }
  free(h);
  if (read) {
    sSweep->revision++;
  }
}

/* A sweep the radio task has finished becomes the live one: timed, added
 * to the peak hold, held against the sweeps kept before it, and kept. */
static void sweepPoll(void) {
  if (sSweep == NULL || !sSweep->running || radioSweepBusy()) {
    return;
  }
  sSweep->running = false;
  DxSweep *s = &sSweep->next;
  /* No channels is a sweep the radio ended early; nothing of it is kept. */
  sSweep->abandoned = s->count == 0;
  if (sSweep->abandoned) {
    /* Nothing kept, but a page showing it as running must hear it ended. */
    sSweep->revision++;
    return;
  }
  const DxTime t = timeNow();
  s->timeKnown = t.known;
  s->at = t.known ? t.value : 0;
  sSweep->live = *s;
  dxSweepPeak(&sSweep->peak, s);
  sSweep->revision++;
  DxSweepHistory *h = (DxSweepHistory *)malloc(sizeof(*h));
  if (h == NULL) {
    DebugLog.println(F("[dx] no memory to keep the sweep"));
    if (!sSweep->baseFixed) {
      sSweep->baseN = 0;
    }
    return;
  }
  dxSweepFsLoad(DX_SWEEP_PATH, h);
  if (!sSweep->baseFixed) {
    sSweep->baseN = dxSweepMedian(h->item, h->count, s, &sSweep->base);
  }
  dxSweepKeep(h, s);
  if (!dxSweepFsSave(DX_SWEEP_PATH, h)) {
    DebugLog.println(F("[dx] the sweep could not be saved"));
  }
  noteKept(h);
  if (sSweep->baseKept >= 0) {
    sSweep->baseKept = sSweep->baseKept + 1 < DX_SWEEP_KEEP
                           ? (int8_t)(sSweep->baseKept + 1)
                           : (int8_t)-1;
  }
  free(h);
}

static bool sweepRunning(void) {
  sweepPoll();
  return sSweep != NULL && sSweep->running;
}

static DxSweepStart sweepStart(void) {
  sweepEnsure();
  if (sSweep == NULL) {
    return DX_SWEEP_NO_MEMORY;
  }
  if (sweepRunning() || bandScanActive()) {
    return DX_SWEEP_BUSY;
  }
  /* The update check's transmitting would raise every level read. */
  if (updateCheckRunning() || !radioSweepStart(&sSweep->next, NULL)) {
    return DX_SWEEP_REFUSED;
  }
  sSweep->running = true;
  sSweep->abandoned = false;
  return DX_SWEEP_STARTED;
}

/* How long RDS took on the channel the scanner is on, kept on flash as each
 * dwell ends. `sTimingDialKHz` is the dial at the last look, 0 when no scan was
 * running then. */
static DxTiming sTiming;
static uint32_t sTimingDialKHz = 0;

static void timingEnd(void) {
  if (!dxTimingEnd(&sTiming, millis())) {
    return;
  }
  uint32_t utc = 0;
  if (!ntpEpochUtc(&utc)) {
    utc = 0;
  }
  /* The longest line, every field at its widest, is 81 bytes. */
  char line[96];
  const size_t n = dxTimingCsvLine(&sTiming, utc, line, sizeof(line));
  if (n == 0 || n >= sizeof(line) || !dxTimingFsAppend(line)) {
    DebugLog.println(F("[dx] an RDS time could not be written"));
  }
}

/* A dwell starts when the dial moves to a new channel while the scan runs,
 * and ends when the dial leaves it or the scan stops. The channel a scan
 * starts on is not timed: its RDS was found before the scan began. */
static void timingStep(const RadioSnapshot &snap, bool running) {
  const uint32_t dial = snap.settings.freqKHz;
  if (!running) {
    if (sTiming.active) {
      timingEnd();
    }
    sTimingDialKHz = 0;
    return;
  }
  const uint32_t now = millis();
  const bool moved = sTimingDialKHz != 0 && dial != sTimingDialKHz;
  sTimingDialKHz = dial;
  if (moved) {
    if (sTiming.active) {
      timingEnd();
    }
    dxTimingStart(&sTiming, dial, now);
  }
  dxTimingFeed(&sTiming, snap.settings.freqKHz, &snap.rds,
               snap.qualityValid ? snap.quality.levelDbuVTenths : INT16_MIN,
               snap.quality.bandwidthKHz, now);
}

static void scanStep(const RadioSnapshot &snap, bool heard, bool seenBefore) {
  if (sScanStore == NULL) {
    return;
  }
  /* With the decoder switched off mid-scan nothing could stop it. */
  if (sScanStore->scan.state == DX_SCAN_RUNNING && !radioRdsEnabled()) {
    dxTaskScanStop();
    say(txt(STR_DX_RDS_IS_OFF));
    return;
  }
  /* A fresh scan's first channel waits for the sweep it starts with. */
  if (sweepRunning()) {
    scanSyncMute();
    return;
  }
  scanTake(dxScanPoll(&sScanStore->scan, millis(), snap.settings.freqKHz, heard,
                      heard && !heardEnough(snap),
                      heard && stopsOn(snap, seenBefore)));
  scanSendTune();
  timingStep(snap, sScanStore->scan.state == DX_SCAN_RUNNING);
  if (learningNow() && sScanStore->scan.state != DX_SCAN_RUNNING) {
    learnEnd();
  }
  scanSyncMute();
}

/* A fresh scan from the dial in `snap`: the whole band when learning,
 * otherwise what the DX SETUP menu says to walk. */
static DxScanAction scanStartFresh(const RadioSnapshot &snap, bool learning) {
  const DxScanAction none = {false, 0};
  if (!radioTaskPlan(&sScanStore->walk.plan) ||
      !bandLimits(snap.settings.band, &sScanStore->walk.plan,
                  &sScanStore->walk.lowKHz, &sScanStore->walk.highKHz)) {
    return none;
  }
  sScanStore->walk.band = snap.settings.band;
  const uint8_t range = learning ? (uint8_t)DX_RANGE_BAND : sSetup.range;
  sScanStore->walk.range = range;
  sScanStore->walk.skipStored = range == DX_RANGE_BAND_LESS_MEMORY;
  sScanStore->walk.memFirst = sSetup.memFirst;
  sScanStore->walk.memLast = sSetup.memLast;
  sScanStore->walk.learning = learning;
  DxScanPlan plan;
  memset(&plan, 0, sizeof(plan));
  plan.dwellMs = sSetup.dwellMs;
  plan.loop = !learning && sSetup.loop;
  plan.ctx = &sScanStore->walk;
  if (range == DX_RANGE_MEMORY) {
    plan.count = (uint16_t)(sSetup.memLast - sSetup.memFirst + 1);
    plan.channel = scanMemoryChannel;
  } else {
    sScanStore->band.lowKHz = sScanStore->walk.lowKHz;
    sScanStore->band.stepKHz =
        bandDefaultStep(sScanStore->walk.band, &sScanStore->walk.plan);
    sScanStore->band.skip = scanSkip;
    sScanStore->band.skipCtx = &sScanStore->walk;
    plan.count =
        dxScanBandCount(sScanStore->walk.lowKHz, sScanStore->walk.highKHz,
                        sScanStore->band.stepKHz);
    plan.channel = dxScanBandChannel;
    plan.ctx = &sScanStore->band;
  }
  return dxScanStart(&sScanStore->scan, &plan, snap.settings.freqKHz);
}

/*
 * Whether a press goes on from the stopped scan rather than starting again:
 * it is on the band the dial is on, and it walks what the DX SETUP menu
 * says to walk now, or it is a learning pass, which a press goes on with.
 */
static bool resumesStopped(BandId band) {
  const DxScanWalk *w = &sScanStore->walk;
  if (sScanStore->scan.state != DX_SCAN_STOPPED || w->band != band) {
    return false;
  }
  if (w->learning) {
    return true;
  }
  return w->range == sSetup.range &&
         (w->range != DX_RANGE_MEMORY ||
          (w->memFirst == sSetup.memFirst && w->memLast == sSetup.memLast));
}

static DxScanPress scanBegin(bool learning) {
  if (sScanStore == NULL || (learning && sSession == NULL)) {
    return DX_SCAN_PRESS_NO_MEMORY;
  }
  if (sScanStore->scan.state == DX_SCAN_RUNNING || bandScanActive()) {
    return DX_SCAN_PRESS_BUSY;
  }
  if (updateCheckRunning()) {
    return DX_SCAN_PRESS_UPDATE_CHECK;
  }
  /* A PI is the only thing that stops it, and with the decoder off none
   * ever would: it would walk the band muted and read like an empty one. A
   * learning pass hears nothing without it either. */
  if (!radioRdsEnabled()) {
    return DX_SCAN_PRESS_RDS_OFF;
  }
  RadioSnapshot snap;
  if (!radioGetSnapshot(&snap)) {
    return DX_SCAN_PRESS_NO_RADIO;
  }
  /* A Learn locals always starts from the bottom of the band; a press goes
   * on from a stop, a learning pass's too. */
  const bool resume = !learning && resumesStopped(snap.settings.band);
  const bool learns = resume ? sScanStore->walk.learning : learning;
  /* What is learned goes into the seen set, so with none read it would
   * run its whole pass and learn nothing. */
  if (learns && (sSession == NULL || !sSession->seenKnown)) {
    return DX_SCAN_PRESS_NO_SEEN;
  }
  const DxScanAction a =
      resume ? dxScanResume(&sScanStore->scan, snap.settings.freqKHz)
             : scanStartFresh(snap, learning);
  if (learns && sScanStore->scan.state == DX_SCAN_RUNNING) {
    sSession->learning = true;
  }
  /* A fresh scan starts with a sweep of the band, so the sweeps build up
   * without anybody asking. Its first channel waits for it. */
  if (!resume && sScanStore->scan.state == DX_SCAN_RUNNING) {
    (void)sweepStart();
  }
  scanTake(a);
  if (!sweepRunning()) {
    scanSendTune();
  }
  scanSyncMute();
  if (sScanStore->scan.state == DX_SCAN_RUNNING) {
    return DX_SCAN_PRESS_RUNNING;
  }
  return a.tune ? DX_SCAN_PRESS_FINISHED : DX_SCAN_PRESS_NOTHING;
}

DxScanPress dxTaskScanPress(void) {
  return scanBegin(false);
}

DxScanPress dxTaskLearn(void) {
  return scanBegin(true);
}

const char *dxTaskPressText(DxScanPress r) {
  switch (r) {
    case DX_SCAN_PRESS_RUNNING:
      return txt(STR_DX_SCANNING);
    case DX_SCAN_PRESS_FINISHED:
      return txt(STR_DX_SCAN_FINISHED);
    case DX_SCAN_PRESS_NOTHING:
      return txt(STR_DX_NOTHING_TO_SCAN);
    case DX_SCAN_PRESS_BUSY:
      return txt(STR_DX_A_SCAN_IS_RUNNING);
    case DX_SCAN_PRESS_UPDATE_CHECK:
      return txt(STR_DX_CHECKING_FOR_UPDATES);
    case DX_SCAN_PRESS_RDS_OFF:
      return txt(STR_DX_RDS_IS_OFF);
    case DX_SCAN_PRESS_NO_SEEN:
      return txt(STR_DX_CAUGHT_LIST_UNREAD);
    case DX_SCAN_PRESS_NO_MEMORY:
      return txt(STR_DX_NO_MEMORY);
    case DX_SCAN_PRESS_NOT_FM:
      return txt(STR_COMMON_FM_ONLY);
    case DX_SCAN_PRESS_NO_RADIO:
    case DX_SCAN_PRESS_PANEL_BUSY:
    default:
      return txt(STR_DX_NOT_STARTED);
  }
}

void dxTaskApplySettings(const Settings *s) {
  if (s == NULL) {
    return;
  }
  sSetup.stop = s->dxStopRule;
  sSetup.range = s->dxScanRange;
  sSetup.memFirst = s->dxMemFirst;
  sSetup.memLast = s->dxMemLast;
  sSetup.loop = s->dxLoop != 0;
  sSetup.mute = s->dxScanMute != 0;
  sSetup.autoLog = s->dxAutoLog != 0;
  sSetup.dwellMs = (uint32_t)s->dxDwellTenths * 100u;
  sSetup.widthKHz = s->dxWidthKHz;
  sSetup.logRt = s->dxLogRt != 0;
  sSetup.watch = s->dxWatch != 0;
  sSetup.beep = s->beepKey != 0;
  if (sSession != NULL) {
    sSession->autoLogOff = !sSetup.autoLog;
  }
  /* The dwell and the loop reach a scan under way too; what it walks is
   * fixed at its start, and the width at DX mode's. A learning pass takes
   * the dwell but goes round once, whatever the loop says. */
  if (sScanStore != NULL && sScanStore->scan.state != DX_SCAN_IDLE) {
    sScanStore->scan.plan.dwellMs = sSetup.dwellMs;
    if (!sScanStore->walk.learning) {
      sScanStore->scan.plan.loop = sSetup.loop;
    }
  }
  scanSyncMute();
}

void dxTaskAddRadioText(LogbookEntry *e, const RadioSnapshot *snap) {
  if (e == NULL || snap == NULL || !sSetup.logRt) {
    return;
  }
  const SeekReading reading =
      radioSeekReading(&snap->quality, snap->qualityValid);
  dxLogRadioText(e, &snap->rds, &reading, snap->settings.freqKHz);
}

uint16_t dxTaskOpenWidth(void) {
  return sSetup.widthKHz;
}

void dxTaskScanStop(void) {
  if (sScanStore != NULL) {
    dxScanStop(&sScanStore->scan);
  }
  /* The sweep a fresh scan starts with goes with it, or the radio stays
   * muted for the rest of the band after the key that stopped the scan. */
  if (sweepRunning()) {
    radioSweepCancel();
  }
  sScanTunePending = false;
  learnEnd();
  scanSyncMute();
}

const DxScan *dxTaskScan(void) {
  return sScanStore != NULL ? &sScanStore->scan : &kNoScan;
}

bool dxTaskDialWalking(const RadioSnapshot *now) {
  return now->seeking || bandScanActive() ||
         dxTaskScan()->state == DX_SCAN_RUNNING || sScanTunePending;
}

static void hear(const RadioSnapshot &snap, bool heard) {
  if (sSession == NULL) {
    return;
  }
  DxHearing h;
  memset(&h, 0, sizeof(h));
  if (heard) {
    h.khz = snap.settings.freqKHz;
    h.band = (uint8_t)snap.settings.band;
    h.pi = snap.rds.pi;
    h.ps = snap.rds.hasPs ? snap.rds.ps : NULL;
    h.country =
        snap.rds.hasEcc ? rdsCountryCode(snap.rds.pi, snap.rds.ecc) : NULL;
    h.rds.hasPty = snap.rds.hasPty;
    h.rds.pty = snap.rds.pty;
    h.rds.hasFlags = snap.rds.hasFlags;
    h.rds.tp = snap.rds.tp;
    h.rds.ta = snap.rds.ta;
    h.rds.hasEcc = snap.rds.hasEcc;
    h.rds.ecc = snap.rds.ecc;
    h.readings.levelDbuVTenths = snap.quality.levelDbuVTenths;
    h.readings.usnTenths = snap.quality.usnTenths;
    h.readings.multipathTenths = snap.quality.multipathTenths;
    h.readings.snrDb = snap.quality.snrDb;
    h.readings.stereo = snap.quality.stereo;
    h.readings.bandwidthKHz = snap.quality.bandwidthKHz;
    h.at = timeNow();
  }
  dxSessionHear(sSession, snap.settings.freqKHz, heard ? &h : NULL, writeLog,
                (void *)&snap);
  saveSeenIfDirty(false);
}

void dxTaskRestart(void) {
  sOpen = true;
  sweepEnsure();
  if (sSweep != NULL) {
    sSweep->peak.count = 0;
    sSweep->revision++;
  }
  if (sScanStore == NULL) {
    /* Zeroed is an idle scan. */
    sScanStore = (ScanStore *)calloc(1, sizeof(*sScanStore));
    if (sScanStore == NULL) {
      DebugLog.println(F("[dx] no memory for the scanner"));
    }
  }
  if (sSession == NULL) {
    sSession = (DxSession *)malloc(sizeof(*sSession));
    if (sSession == NULL) {
      DebugLog.println(F("[dx] no memory for the catches"));
      return;
    }
    dxSessionReset(sSession);
    /* Ids start somewhere new each boot, so a page left open over a
     * restart does not name a new catch by an old catch's id. */
    sSession->catches.lastId = (uint16_t)esp_random();
  }
  sSession->autoLogOff = !sSetup.autoLog;
  /* Each opening watches from fresh floors: a floor from an hour ago says
   * nothing about the band now. */
  if (sWatch == NULL) {
    sWatch = (DxWatch *)calloc(1, sizeof(*sWatch));
    if (sWatch == NULL) {
      DebugLog.println(F("[dx] no memory for the preset watch"));
    }
  }
  sWatchPending = false;
  sWatchCount = 0;
  sWatchUpKHz = 0;
  sWatchUpRise = 0;
  sWatchUpMs = 0;
  /* Read once. A read that failed is tried again the next time DX mode
   * opens, not on every poll. */
  if (!sSession->seenKnown && dxSeenFsLoad(&sSession->seen)) {
    dxSessionSeenLoaded(sSession, writeLog, NULL);
    saveSeenIfDirty(false);
  }
}

void dxTaskLeave(void) {
  sOpen = false;
  /* A scan does not outlive DX mode: the next one starts from the bottom
   * of the band and the dial it finds, not from wherever this one was. */
  dxTaskScanStop();
  /* Nor does its dwell, which would otherwise be timed from a start long
   * gone once the next scan began. */
  if (sTiming.active) {
    timingEnd();
  }
  sTimingDialKHz = 0;
  if (sScanStore != NULL) {
    dxScanReset(&sScanStore->scan);
  }
  dxSessionClose(sSession, writeLog, NULL);
  saveSeenIfDirty(true);
  free(sWatch);
  sWatch = NULL;
}

const DxCatches *dxTaskCatches(void) {
  return sSession != NULL ? &sSession->catches : NULL;
}

DxWriteResult dxTaskLogCatch(uint8_t index) {
  RadioSnapshot snap;
  const bool have = radioGetSnapshot(&snap);
  const DxWriteResult r =
      dxSessionWrite(sSession, index, writeLog, have ? &snap : NULL);
  if (r != DX_WRITE_NO_CATCH) {
    confirmLog(r, &sSession->catches.item[index]);
  }
  saveSeenIfDirty(false);
  return r;
}

void dxTaskNoteLogged(uint32_t khz, uint16_t pi) {
  dxSessionNoteLogged(sSession, khz, pi);
  saveSeenIfDirty(false);
}

/* DX mode's own work on each poll, whatever the panel shows: catches, the
 * scanner's walk and the watch on the presets. */
static void dxStep(const RadioSnapshot &snap) {
  const bool heard = heardNow(snap);
  /* Whether the PI was caught before this poll: a NEW catch logged by
   * hear() just now still stops the scan, one logged minutes ago does not. */
  const bool seenBefore = heard && sSession != NULL && sSession->seenKnown &&
                          dxSeenHas(&sSession->seen, snap.rds.pi);
  hear(snap, heard);
  scanStep(snap, heard, seenBefore);
  watchStep(snap);
}

void dxTaskStep(const RadioSnapshot *snap) {
  if (snap != NULL) {
    dxStep(*snap);
  }
}

const DxSetup *dxTaskSetup(void) {
  return &sSetup;
}

const DxScanWalk *dxTaskWalk(void) {
  return sScanStore != NULL ? &sScanStore->walk : NULL;
}

bool dxTaskSweepState(DxSweepState *out) {
  if (out == NULL) {
    return false;
  }
  memset(out, 0, sizeof(*out));
  if (sSweep == NULL) {
    return false;
  }
  out->live = sSweep->live.count > 0 ? &sSweep->live : NULL;
  out->base = sSweep->baseN > 0 ? &sSweep->base : NULL;
  out->baseN = sSweep->baseN;
  out->baseFixed = sSweep->baseFixed;
  out->peak = sSweep->peak.count > 0 ? &sSweep->peak : NULL;
  out->running = sSweep->running;
  out->abandoned = sSweep->abandoned;
  out->revision = sSweep->revision;
  return true;
}

void dxTaskSweepPoll(void) {
  sweepPoll();
}

bool dxTaskTick(void) {
  RadioSnapshot snap;
  if (!radioGetSnapshot(&snap)) {
    return true;
  }
  if (bandModulation(snap.settings.band) != MODULATION_FM) {
    return false;
  }
  dxStep(snap);
  return true;
}

DxSweepStart dxTaskSweep(void) {
  if (!sOpen) {
    return DX_SWEEP_CLOSED;
  }
  if (dxTaskScan()->state == DX_SCAN_RUNNING) {
    return DX_SWEEP_BUSY;
  }
  return sweepStart();
}

bool dxTaskSweepView(DxSweepState *out) {
  sweepEnsure();
  sweepPoll();
  if (!dxTaskSweepState(out)) {
    return false;
  }
  /* A baseline fixed on other channels or at another width does not hold
   * against this sweep. */
  if (out->live == NULL ||
      (out->base != NULL && !dxSweepSameChannels(out->base, out->live))) {
    out->base = NULL;
    out->baseN = 0;
  }
  return true;
}

/* Fix `h->item[n]` as the baseline, kept across restarts. `h` is used up. */
static DxBaseResult fixBaseline(DxSweepHistory *h, uint8_t n) {
  h->item[0] = h->item[n];
  h->count = 1;
  if (!dxSweepFsSave(DX_SWEEP_BASE_PATH, h)) {
    return DX_BASE_NOT_SAVED;
  }
  sSweep->base = h->item[0];
  sSweep->baseFixed = true;
  sSweep->baseN = 1;
  sSweep->baseKept = (int8_t)n;
  sSweep->revision++;
  return DX_BASE_DONE;
}

DxBaseResult dxTaskBaselineNow(void) {
  sweepEnsure();
  if (sSweep == NULL) {
    return DX_BASE_NO_MEMORY;
  }
  sweepPoll();
  if (sSweep->live.count == 0) {
    return DX_BASE_NO_SWEEP;
  }
  DxSweepHistory *h = (DxSweepHistory *)malloc(sizeof(*h));
  if (h == NULL) {
    return DX_BASE_NO_MEMORY;
  }
  h->item[0] = sSweep->live;
  const DxBaseResult r = fixBaseline(h, 0);
  free(h);
  return r;
}

DxBaseResult dxTaskBaselineKept(uint8_t n) {
  sweepEnsure();
  if (sSweep == NULL) {
    return DX_BASE_NO_MEMORY;
  }
  sweepPoll();
  DxSweepHistory *h = (DxSweepHistory *)malloc(sizeof(*h));
  if (h == NULL) {
    return DX_BASE_NO_MEMORY;
  }
  dxSweepFsLoad(DX_SWEEP_PATH, h);
  const DxBaseResult r =
      n == 0 || n >= h->count ? DX_BASE_NO_SWEEP : fixBaseline(h, n);
  free(h);
  return r;
}

uint8_t dxTaskKept(DxKeptSweep *out, uint8_t cap, int8_t *chosen) {
  if (chosen != NULL) {
    *chosen = 0;
  }
  sweepEnsure();
  if (sSweep == NULL) {
    return 0;
  }
  sweepPoll();
  const uint8_t count = sSweep->keptCount > 0 ? sSweep->keptCount - 1 : 0;
  for (uint8_t i = 0; i < count && i < cap; i++) {
    out[i] = sSweep->kept[i + 1];
  }
  if (chosen != NULL && sSweep->baseFixed) {
    *chosen = sSweep->baseKept >= 1 ? sSweep->baseKept : (int8_t)-1;
  }
  return count;
}

DxBaseResult dxTaskBaselineNext(void) {
  int8_t chosen = 0;
  const uint8_t count = dxTaskKept(NULL, 0, &chosen);
  if (count == 0) {
    return DX_BASE_NO_SWEEP;
  }
  const int next = chosen < 0 ? 1 : chosen + 1;
  return next > count ? dxTaskBaselineAuto()
                      : dxTaskBaselineKept((uint8_t)next);
}

DxBaseResult dxTaskBaselineAuto(void) {
  sweepEnsure();
  if (sSweep == NULL) {
    return DX_BASE_NO_MEMORY;
  }
  sweepPoll();
  if (!dxSweepFsRemove(DX_SWEEP_BASE_PATH)) {
    return DX_BASE_NOT_SAVED;
  }
  sSweep->baseFixed = false;
  sSweep->baseN = 0;
  sSweep->revision++;
  DxSweepHistory *h = (DxSweepHistory *)malloc(sizeof(*h));
  if (h == NULL) {
    /* No baseline until the next sweep works one out. */
    return DX_BASE_DONE;
  }
  dxSweepFsLoad(DX_SWEEP_PATH, h);
  if (sSweep->live.count > 0) {
    sweepMedian(h);
  }
  free(h);
  return DX_BASE_DONE;
}

bool dxTaskSweepRunning(void) {
  return sweepRunning();
}

uint16_t dxTaskSweepRevision(void) {
  return sSweep != NULL ? sSweep->revision : 0;
}

void dxTaskSweepStop(void) {
  if (sweepRunning()) {
    radioSweepCancel();
  }
}

const char *dxTaskSweepText(DxSweepStart r) {
  switch (r) {
    case DX_SWEEP_STARTED:
      return txt(STR_DX_SWEEPING);
    case DX_SWEEP_BUSY:
      return txt(STR_DX_A_SCAN_IS_RUNNING);
    case DX_SWEEP_NO_MEMORY:
      return txt(STR_DX_NO_MEMORY);
    case DX_SWEEP_CLOSED:
    case DX_SWEEP_REFUSED:
    default:
      return txt(STR_DX_NOT_STARTED);
  }
}
