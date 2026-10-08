/* Building the DX page's view. See screen_dx_state.h. */
#include "screen_dx_state.h"

#include <stdio.h>
#include <string.h>

#include "core/band_plan.h"
#include "core/clock.h"
#include "core/memory.h"
#include "core/rds.h"
#include "core/rds_country.h"
#include "core/seek.h" /* SeekReading, which the PI rule reads. */
#include "core/strings.h"

void screenDxStateReset(ScreenDxKeep *keep) {
  if (keep == NULL) {
    return;
  }
  memset(keep, 0, sizeof(*keep));
  dxHistoryReset(&keep->history);
  readingHoldReset(&keep->offsetHold);
  readingHoldReset(&keep->usnHold);
}

/* Tenths as a number with one decimal, "-1.8" as well as "45.5". */
static void tenths(int16_t value, char *out, size_t cap) {
  const int v = value < 0 ? -value : value;
  snprintf(out, cap, txt(STR_DX_FMT_TENTHS), value < 0 ? "-" : "", v / 10,
           v % 10);
}

/* Tenths of a per cent as a whole per cent, rounded. */
static void wholePercent(uint16_t valueTenths, char *out, size_t cap) {
  snprintf(out, cap, "%u", (unsigned)((valueTenths + 5u) / 10u));
}

/* Tenths of a kHz as whole kHz with its sign, rounded: "+6", "-1", "0". */
static void wholeOffset(int16_t valueTenths, char *out, size_t cap) {
  const int v =
      valueTenths < 0 ? (valueTenths - 5) / 10 : (valueTenths + 5) / 10;
  if (v == 0) {
    snprintf(out, cap, "0");
  } else {
    snprintf(out, cap, txt(STR_DX_FMT_SIGNED), v);
  }
}

static ScreenDxPi tileOf(DxPiTile tile) {
  switch (tile) {
    case DX_PI_SEEN:
      return SCREEN_DX_PI_SEEN;
    case DX_PI_PARTIAL:
      return SCREEN_DX_PI_PARTIAL;
    case DX_PI_CONFIRMED:
      return SCREEN_DX_PI_CONFIRMED;
    case DX_PI_ZERO:
      return SCREEN_DX_PI_ZERO;
    case DX_PI_NONE:
    default:
      return SCREEN_DX_PI_NONE;
  }
}

static void buildRds(const RdsInfo *rds, const SeekReading *reading,
                     RdsRegion region, uint16_t presetPi, int16_t presetSlot,
                     ScreenDx *out) {
  if (rds->hasPs) {
    out->psShown = true;
    for (int i = 0; i < SCREEN_DX_PS_LEN; i++) {
      out->ps[i] = rds->ps[i];
      out->psHave[i] = true;
    }
  } else {
    for (int i = 0; i < SCREEN_DX_PS_LEN; i++) {
      out->ps[i] = rds->psHeard[i];
      out->psHave[i] = rds->psHeardHave[i];
      out->psShown = out->psShown || rds->psHeardHave[i];
    }
  }
  bool whole = out->psShown;
  for (int i = 0; i < SCREEN_DX_PS_LEN; i++) {
    whole = whole && out->psHave[i];
  }
  if (whole) {
    rdsNameTrim(out->ps, SCREEN_DX_PS_LEN, out->psWhole, sizeof(out->psWhole));
  }
  out->pty = rds->hasPty ? rdsPtyNameIn(rds->pty, region) : NULL;

  const DxPiTile tile = dxPiTile(rds, reading);
  out->pi = tileOf(tile);
  dxPiDigits(rds, tile, out->piDigits);
  /* Kept, since the line points into it until the next build. */
  static DxIdentity id;
  dxStationIdentity(rds, reading, region,
                    presetSlot != MEMORY_NO_SLOT ? presetPi : 0, true, &id);
  if (tile == DX_PI_CONFIRMED) {
    out->country = id.country;
    /* No country is guessed from the PI alone, so while the ECC is missing the
     * tile shows only that it is not known. */
    out->countryUnsure = out->country == NULL;
    /* In North America the call letters take the country's place, with the help
     * mark while they are only worked out from the PI. */
    if (id.call != RDS_CALL_NONE) {
      out->country = id.callText;
      out->countryUnsure = id.call == RDS_CALL_GUESS;
    }
  }
  /* On a preset with a stored PI the line under the PI names the preset:
   * its own station, shown as the stored PI from the first matching block,
   * or another one, confirmed. */
  const DxPresetPi preset = id.preset;
  if (preset != DX_PRESET_NONE) {
    static char line[16];
    snprintf(line, sizeof(line),
             txt(preset == DX_PRESET_MATCH ? STR_DX_FMT_PRESET
                                           : STR_DX_FMT_NOT_PRESET),
             (int)presetSlot + 1);
    out->country = line;
    out->countryUnsure = false;
    out->piOther = preset == DX_PRESET_OTHER;
    if (preset == DX_PRESET_MATCH) {
      out->pi = SCREEN_DX_PI_CONFIRMED;
      snprintf(out->piDigits, sizeof(out->piDigits), "%04X",
               (unsigned)presetPi);
    }
  }
  for (int b = 0; b < 4; b++) {
    out->blockError[b] = rds->hasBlockErrors ? (int8_t)rds->blockError[b] : -1;
  }
}

/* A moment's message, such as what a log hold did, takes the right of the
 * header while it shows. The page, the clock and the context give way to it,
 * so the message has the whole run, and come back when it goes. */
static void giveHeaderTo(const char *message, const char **context,
                         const char **position, const char **clock,
                         bool *isMessage) {
  if (message == NULL) {
    return;
  }
  if (context != NULL) {
    *context = NULL;
  }
  *position = message;
  *clock = NULL;
  *isMessage = true;
}

void screenDxStateFeed(ScreenDxKeep *keep, const RadioSnapshot *snap,
                       uint32_t nowMs) {
  if (keep == NULL || snap == NULL) {
    return;
  }
  /* The history and the holds belong to one channel, so a retune starts
   * them again. The reading the snapshot still carries is the old station's
   * until the radio's next read, so the holds start from that next one. */
  if (snap->settings.freqKHz != keep->historyKHz) {
    dxHistoryReset(&keep->history);
    readingHoldReset(&keep->offsetHold);
    readingHoldReset(&keep->usnHold);
    keep->held = false;
    keep->heldReads = snap->qualityReads;
    keep->historyKHz = snap->settings.freqKHz;
  }
  if (!snap->qualityValid) {
    return;
  }
  dxHistoryAdd(&keep->history, nowMs, snap->quality.levelDbuVTenths);
  /* Once per reading, not once per build: the page is built on every poll,
   * and feeding the same reading twice would hold it twice as hard. The
   * limits are the ones measured on this radio for these two readings. */
  if (snap->qualityReads != keep->heldReads) {
    keep->offsetShownTenths = readingHoldFeed(
        &keep->offsetHold, snap->quality.offsetKHzTenths,
        READING_OFFSET_QUANTUM_TENTHS, READING_OFFSET_HYSTERESIS_TENTHS);
    keep->usnShownTenths = readingHoldFeed(
        &keep->usnHold, (int16_t)snap->quality.usnTenths,
        READING_USN_QUANTUM_TENTHS, READING_USN_HYSTERESIS_TENTHS);
    keep->heldReads = snap->qualityReads;
    keep->held = true;
  }
}

void screenDxStateBuild(const ScreenDxInputs *in, ScreenDxKeep *keep,
                        ScreenDx *out) {
  if (in == NULL || in->snap == NULL || keep == NULL || out == NULL) {
    return;
  }
  const RadioSnapshot &snap = *in->snap;
  memset(out, 0, sizeof(*out));
  for (int b = 0; b < 4; b++) {
    out->blockError[b] = -1;
  }

  snprintf(keep->position, sizeof(keep->position), txt(STR_DX_FMT_PAGE),
           (unsigned)(in->pages > 0 ? in->pages : 1));
  out->position = keep->position;
  out->clock = in->clock;

  if (bandFormatFrequency(snap.settings.band, snap.settings.freqKHz,
                          keep->frequency, sizeof(keep->frequency))) {
    out->frequency = keep->frequency;
    out->unit = bandFrequencyUnit(snap.settings.band);
  }

  screenDxStateFeed(keep, &snap, in->nowMs);

  const SeekReading reading =
      radioSeekReading(&snap.quality, snap.qualityValid);
  if (snap.qualityValid) {
    const Tef668xQuality &q = snap.quality;

    tenths(signalShownTenths(q.levelDbuVTenths, in->levelOffsetDb), keep->level,
           sizeof(keep->level));
    /* The noise and the offset held, since raw the offset takes over a
     * hundred values in half a minute and cannot be read. Until the new
     * station's first read, each as it is. */
    wholePercent(keep->held ? (uint16_t)keep->usnShownTenths : q.usnTenths,
                 keep->usn, sizeof(keep->usn));
    wholePercent(q.multipathTenths, keep->wam, sizeof(keep->wam));
    wholeOffset(keep->held ? keep->offsetShownTenths : q.offsetKHzTenths,
                keep->offset, sizeof(keep->offset));
    snprintf(keep->bandwidth, sizeof(keep->bandwidth), "%u",
             (unsigned)q.bandwidthKHz);
    snprintf(keep->modulation, sizeof(keep->modulation), "%d",
             (int)q.modulationPercent);
    out->level = keep->level;
    out->usn = keep->usn;
    out->wam = keep->wam;
    out->offset = keep->offset;
    out->bandwidth = keep->bandwidth;
    out->modulation = keep->modulation;
  }
  for (uint8_t i = 0; i < SCREEN_DX_HISTORY; i++) {
    out->historyHave[i] =
        dxHistoryBar(&keep->history, in->nowMs, i, &out->historyTenths[i]);
  }

  /* The mark for a stereo pilot, and nothing without one. At a DX width the
   * seek's rule cannot tell a mono station from an empty channel, so a mono
   * mark would sometimes be drawn on noise. No empty channel showed a pilot at
   * either width measured. */
  out->stereo = snap.qualityValid && snap.quality.stereo;

  if (in->rdsEnabled) {
    buildRds(&snap.rds, &reading, in->region, in->presetPi, snap.memorySlot,
             out);
  }

  giveHeaderTo(in->confirm, NULL, &out->position, &out->clock,
               &out->positionIsMessage);
}

/* The rows of the screen of catches that holds the cursor, and the range
 * they are of. */
static void catchRows(const ScreenCatchesInputs *in, ScreenCatchesKeep *keep,
                      ScreenCatches *out) {
  const DxCatches *list = in->list;
  uint8_t cursor = in->cursor;
  const int16_t offsetMinutes = in->offsetMinutes;
  const int8_t levelOffsetDb = in->levelOffsetDb;
  if (list == NULL || list->count == 0) {
    return;
  }
  if (cursor >= list->count) {
    cursor = (uint8_t)(list->count - 1);
  }
  const uint8_t first =
      (uint8_t)(cursor / SCREEN_CATCH_ROWS * SCREEN_CATCH_ROWS);
  uint8_t shown = (uint8_t)(list->count - first);
  if (shown > SCREEN_CATCH_ROWS) {
    shown = SCREEN_CATCH_ROWS;
  }
  /* A hyphen, not a dash, in words people read on this radio. */
  snprintf(keep->range, sizeof(keep->range), txt(STR_DX_FMT_ROWS_OF),
           (unsigned)(first + 1), (unsigned)(first + shown),
           (unsigned)list->count);
  out->range = keep->range;
  out->rows = shown;
  out->cursor = (uint8_t)(cursor - first);
  for (uint8_t i = 0; i < shown; i++) {
    const DxCatch *k = &list->item[first + i];
    ScreenCatchRow *row = &out->row[i];
    if (k->last.known) {
      if (clockFormat(clockFromEpoch(k->last.value, offsetMinutes),
                      keep->time[i], sizeof(keep->time[i]))) {
        row->time = keep->time[i];
      }
    }
    if (bandFormatFrequency((BandId)k->band, k->khz, keep->frequency[i],
                            sizeof(keep->frequency[i]))) {
      row->frequency = keep->frequency[i];
    }
    snprintf(keep->pi[i], sizeof(keep->pi[i]), "%04X", (unsigned)k->pi);
    row->pi = keep->pi[i];
    row->ps = k->hasPs && rdsNameTrim(k->ps, sizeof(keep->ps[i]), keep->ps[i],
                                      sizeof(keep->ps[i]))
                  ? keep->ps[i]
                  : NULL;
    row->country = k->hasCountry ? k->country : NULL;
    row->countryUnsure = !k->hasCountry;
    row->isNew = k->isNew;
    tenths(signalShownTenths(k->best.levelDbuVTenths, levelOffsetDb),
           keep->level[i], sizeof(keep->level[i]));
    row->level = keep->level[i];
    /* Past 99 it would run into the level, so it says only that. */
    if (k->count > 99) {
      snprintf(keep->count[i], sizeof(keep->count[i]), "%s",
               txt(STR_DX_COUNT_99_PLUS));
    } else {
      snprintf(keep->count[i], sizeof(keep->count[i]), txt(STR_DX_FMT_TIMES),
               (unsigned)k->count);
    }
    row->count = keep->count[i];
  }
}

void screenCatchesStateBuild(const ScreenCatchesInputs *in,
                             ScreenCatchesKeep *keep, ScreenCatches *out) {
  if (in == NULL || keep == NULL || out == NULL) {
    return;
  }
  memset(out, 0, sizeof(*out));
  snprintf(keep->position, sizeof(keep->position), "%u/%u",
           (unsigned)(in->page + 1), (unsigned)(in->pages > 0 ? in->pages : 1));
  out->position = keep->position;
  out->clock = in->clock;
  catchRows(in, keep, out);
  giveHeaderTo(in->confirm, &out->range, &out->position, &out->clock,
               &out->positionIsMessage);
}

/* Thousandths as a whole number and one decimal, cut rather than rounded:
 * kHz as the bar's MHz labels, "87.5", and milliseconds as the dwell and
 * its countdown, which then never reads its next value early. */
static void oneDecimal(uint32_t thousandths, char *out, size_t cap) {
  snprintf(out, cap, "%u.%u", (unsigned)(thousandths / 1000u),
           (unsigned)((thousandths % 1000u) / 100u));
}

void screenScanStateBuild(const ScreenScanInputs *in, ScreenScanKeep *keep,
                          ScreenScan *out) {
  if (in == NULL || in->scan == NULL || in->snap == NULL || keep == NULL ||
      out == NULL) {
    return;
  }
  const DxScan *scan = in->scan;
  const RadioSnapshot &snap = *in->snap;
  const BandId band = snap.settings.band;
  memset(out, 0, sizeof(*out));

  snprintf(keep->found, sizeof(keep->found), txt(STR_DX_FMT_FOUND),
           (unsigned)scan->found);
  out->found = keep->found;
  snprintf(keep->position, sizeof(keep->position), "%u/%u",
           (unsigned)(in->page + 1), (unsigned)(in->pages > 0 ? in->pages : 1));
  out->position = keep->position;
  out->clock = in->clock;

  const bool memory = !in->learning && in->range == DX_RANGE_MEMORY;
  if (in->sweeping) {
    out->mode = txt(STR_DX_MODE_SWEEPING);
  } else if (in->learning) {
    out->mode = txt(STR_DX_MODE_LEARN_LOCALS);
  } else if (memory) {
    snprintf(keep->mode, sizeof(keep->mode), txt(STR_DX_FMT_MODE_MEMORY),
             (unsigned)in->memFirst, (unsigned)in->memLast);
    out->mode = keep->mode;
  } else {
    out->mode = txt(in->range == DX_RANGE_BAND ? STR_DX_MODE_WHOLE_BAND
                                               : STR_DX_MODE_BAND_MEMORY);
  }
  out->rule =
      txt(in->learning || in->stop == DX_STOP_NEVER ? STR_DX_RULE_NO_STOP
          : in->stop == DX_STOP_ANY_PI              ? STR_DX_RULE_STOP_ON_PI
                                                    : STR_DX_RULE_STOP_ON_NEW);
  char dwell[12];
  oneDecimal(in->dwellMs, dwell, sizeof(dwell));
  snprintf(keep->dwell, sizeof(keep->dwell), txt(STR_DX_FMT_DWELL_S), dwell);
  out->dwell = keep->dwell;

  const bool running = scan->state == DX_SCAN_RUNNING;
  const bool stopped = scan->state == DX_SCAN_STOPPED;
  out->state = running   ? SCREEN_SCAN_RUNNING
               : stopped ? SCREEN_SCAN_STOPPED
                         : SCREEN_SCAN_IDLE;

  const uint32_t shownKHz = running   ? scan->atKHz
                            : stopped ? snap.settings.freqKHz
                                      : 0;
  if (shownKHz != 0 && bandFormatFrequency(band, shownKHz, keep->frequency,
                                           sizeof(keep->frequency))) {
    out->frequency = keep->frequency;
  }
  if (running) {
    oneDecimal(dxScanLeftMs(scan, in->nowMs), keep->left, sizeof(keep->left));
    out->left = keep->left;
  }

  if (memory) {
    snprintf(keep->from, sizeof(keep->from), txt(STR_DX_FMT_CHANNEL),
             (unsigned)in->memFirst);
    snprintf(keep->to, sizeof(keep->to), txt(STR_DX_FMT_CHANNEL),
             (unsigned)in->memLast);
  } else {
    oneDecimal(in->lowKHz, keep->from, sizeof(keep->from));
    oneDecimal(in->highKHz, keep->to, sizeof(keep->to));
  }
  out->from = keep->from;
  out->to = keep->to;
  if (running || stopped) {
    const uint16_t passed = dxScanPassed(scan);
    const uint16_t total = dxScanTotal(scan);
    out->hasProgress = true;
    out->progressPermille = (uint16_t)((uint32_t)passed * 1000u / total);
    snprintf(keep->step, sizeof(keep->step), txt(STR_DX_FMT_STEP_OF),
             (unsigned)passed, (unsigned)total);
    out->step = keep->step;
  }

  /* The tile: what is heard on the channel, and only while the dial is on
   * the channel the page names. */
  const bool onChannel = (running && scan->landed) ||
                         (stopped && snap.settings.freqKHz == scan->atKHz);
  const RdsInfo &rds = snap.rds;
  if (onChannel) {
    /* The DX page's own reading of the PI, so a digit in doubt shows as
     * `?` and a station sending 0000 as 0000, never as a firm PI. */
    const SeekReading reading =
        radioSeekReading(&snap.quality, snap.qualityValid);
    const DxPiTile tile = dxPiTile(&rds, &reading);
    if (tile != DX_PI_NONE) {
      dxPiDigits(&rds, tile, keep->pi);
      out->pi = keep->pi;
      out->piSure = tile == DX_PI_CONFIRMED;
    }
  }
  if (onChannel && snap.qualityValid) {
    tenths(signalShownTenths(snap.quality.levelDbuVTenths, in->levelOffsetDb),
           keep->level, sizeof(keep->level));
    out->level = keep->level;
  }
  if (out->pi == NULL) {
    out->note = txt(out->level != NULL || running ? STR_DX_NO_RDS_YET
                                                  : STR_DX_NO_STATION_YET);
  }
  out->stationOn = stopped && scan->onCatch && onChannel && rds.hasPi;
  if (out->stationOn) {
    if (rds.hasPs &&
        rdsNameTrim(rds.ps, sizeof(keep->ps), keep->ps, sizeof(keep->ps))) {
      out->ps = keep->ps;
    }
    const int16_t at =
        dxCatchesFind(in->catches, rds.pi, snap.settings.freqKHz);
    out->isNew = at >= 0 && in->catches->item[at].isNew;
  }

  giveHeaderTo(in->confirm, &out->found, &out->position, &out->clock,
               &out->positionIsMessage);
}

static_assert(SCREEN_SCOPE_NONE == DX_SWEEP_NO_READING,
              "the page and the sweep mean one thing by no reading");

void screenScopeStateBuild(const ScreenScopeInputs *in, ScreenScopeKeep *keep,
                           ScreenScope *out) {
  if (in == NULL || keep == NULL || out == NULL) {
    return;
  }
  memset(out, 0, sizeof(*out));
  snprintf(keep->position, sizeof(keep->position), "%u/%u",
           (unsigned)(in->page + 1), (unsigned)(in->pages > 0 ? in->pages : 1));
  out->position = in->position != NULL ? in->position : keep->position;
  out->title = in->title;
  out->am = in->am;
  out->marks = in->marks;
  out->markCount = in->markCount;
  out->catches = in->catches;
  out->catchCount = in->catchCount;
  out->clock = in->clock;
  out->revision = in->revision;
  out->sweeping = in->sweeping;
  out->buttons = in->touchOn;
  out->floor = SCREEN_SCOPE_NONE;
  out->dial = UINT16_MAX;

  const DxSweep *live =
      in->live != NULL && in->live->count > 0 ? in->live : NULL;
  if (in->sweeping) {
    out->context = txt(STR_DX_CONTEXT_SWEEPING);
  } else if (live != NULL && live->timeKnown && in->nowKnown &&
             dxSweepAge(live->at, in->nowUtc, keep->context,
                        sizeof(keep->context))) {
    out->context = keep->context;
  }

  if (live == NULL) {
    out->empty = in->sweeping ? NULL : txt(STR_DX_PRESS_TO_SWEEP);
  } else {
    out->count = live->count;
    out->level = live->level;
    const DxSweep *base =
        dxSweepSameChannels(in->base, live) && in->baseN > 0 ? in->base : NULL;
    if (base != NULL) {
      out->base = base->level;
      if (in->baseFixed) {
        snprintf(keep->baseText, sizeof(keep->baseText), "%s",
                 txt(STR_DX_FIXED));
      } else {
        snprintf(keep->baseText, sizeof(keep->baseText),
                 txt(STR_DX_FMT_MEDIAN_OF), (unsigned)in->baseN);
      }
      out->baseText = keep->baseText;
    }
    if (dxSweepSameChannels(in->peak, live)) {
      out->peak = in->peak->level;
    }
    out->floor = in->span ? SCREEN_SCOPE_NONE : dxSweepFloor(live);
    if (out->floor != SCREEN_SCOPE_NONE) {
      char floor[8];
      tenths(out->floor, floor, sizeof(floor));
      snprintf(keep->floorText, sizeof(keep->floorText), txt(STR_DX_FMT_FLOOR),
               floor);
      out->floorText = keep->floorText;
    }
    const int16_t dial = dxSweepChannelOf(live, in->dialKHz);
    out->dial = dial >= 0 ? (uint16_t)dial : UINT16_MAX;
    out->cursor =
        in->cursor < live->count ? in->cursor : (uint16_t)(live->count - 1);

    /* MHz with one decimal on FM, "87.5", and whole kHz on AM, "522". */
    const uint32_t high = dxSweepKHzOf(live, (uint16_t)(live->count - 1));
    const uint32_t mid = live->lowKHz + (high - live->lowKHz) / 2;
    if (in->am) {
      snprintf(keep->from, sizeof(keep->from), "%u", (unsigned)live->lowKHz);
      snprintf(keep->mid, sizeof(keep->mid), "%u", (unsigned)mid);
      snprintf(keep->to, sizeof(keep->to), "%u", (unsigned)high);
    } else {
      oneDecimal(live->lowKHz, keep->from, sizeof(keep->from));
      oneDecimal(mid, keep->mid, sizeof(keep->mid));
      oneDecimal(high, keep->to, sizeof(keep->to));
    }
    out->from = keep->from;
    out->mid = keep->mid;
    out->to = keep->to;

    (void)bandFormatFrequency(in->am ? BAND_MW : BAND_FM,
                              dxSweepKHzOf(live, out->cursor), keep->freq,
                              sizeof(keep->freq));
    out->cursorFreq = keep->freq;
    const int16_t level = live->level[out->cursor];
    if (level != DX_SWEEP_NO_READING) {
      tenths(signalShownTenths(level, in->levelOffsetDb), keep->level,
             sizeof(keep->level));
      out->cursorLevel = keep->level;
      if (base != NULL && base->level[out->cursor] != DX_SWEEP_NO_READING) {
        const int rise = (int)level - (int)base->level[out->cursor];
        tenths((int16_t)rise, keep->rise + 1, sizeof(keep->rise) - 1);
        /* A sign on every rise but none, so +0.4 and -0.4 read apart. */
        if (rise > 0) {
          keep->rise[0] = '+';
          out->cursorRise = keep->rise;
        } else {
          out->cursorRise = keep->rise + 1;
        }
        out->riseUp = rise > 0;
      }
    }
  }

  giveHeaderTo(in->confirm, &out->context, &out->position, &out->clock,
               &out->positionIsMessage);
}
