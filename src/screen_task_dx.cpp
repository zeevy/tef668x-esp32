/*
 * The DX pages on the panel: each page's view built from the DX session and
 * drawn, and the Scope page's cursor. The session itself, the scanner, the
 * sweeps, the catches, the watch and the log writes, is dx_task.cpp; this
 * file only reads it.
 */
#include "screen_task_dx.h"

#include "core/band_plan.h"
#include "core/clock.h"
#include "dx_task.h"
#include "input_task.h"
#include "memory_store.h"
#include "net/ntp.h"
#include "radio_task.h"
#include "screen_dx_state.h"
#include "screen_task.h"
#include "ui/screen.h"

#include <Arduino.h>
#include <string.h>
#include <algorithm>

/* Each page's text, which has to outlive the build that shows it. */
static ScreenDxKeep sKeep;
static ScreenCatchesKeep sCatchesKeep;
static ScreenScanKeep sScanKeep;
static ScreenScopeKeep sScopeKeep;
/* The Scope page's cursor, a channel of the latest sweep; until it is set it
 * goes to the dial's channel. */
static uint16_t sScopeCursor = 0;
static bool sScopeCursorSet = false;

void screenTaskDxReset(void) {
  screenDxStateReset(&sKeep);
  sScopeCursorSet = false;
}

bool screenTaskDxDraw(uint8_t page, uint8_t *cursor) {
  RadioSnapshot snap;
  if (!radioGetSnapshot(&snap)) {
    return true;
  }
  if (bandModulation(snap.settings.band) != MODULATION_FM) {
    return false;
  }
  /* A catch heard again goes to the front and a new one pushes the rest
   * down, so the cursor follows its catch rather than its row. */
  const DxCatches *list = dxTaskCatches();
  const bool onCatch = list != NULL && *cursor < list->count;
  const uint16_t cursorPi = onCatch ? list->item[*cursor].pi : 0;
  const uint32_t cursorKHz = onCatch ? list->item[*cursor].khz : 0;
  dxTaskStep(&snap);
  const int16_t at =
      onCatch ? dxCatchesFind(list, cursorPi, cursorKHz) : (int16_t)-1;
  if (at >= 0) {
    *cursor = (uint8_t)at;
  }

  /* Local time, as the radio screen shows it; the log keeps UTC and says
   * so itself. Nothing until a server has answered, the same rule as every
   * clock here. */
  char clockText[CLOCK_TEXT_LEN];
  const char *clock = clockFormat(ntpLocalTime(), clockText, sizeof(clockText))
                          ? clockText
                          : NULL;

  if (page == SCREEN_DX_PAGE_SCAN) {
    screenDxStateFeed(&sKeep, &snap, millis());
    ScreenScanInputs in;
    memset(&in, 0, sizeof(in));
    /* The band's edges: the walk's once one has started, and before that
     * the band the dial is on. */
    BandPlanConfig plan;
    const DxScan *scan = dxTaskScan();
    const DxScanWalk *walk = dxTaskWalk();
    const DxSetup *setup = dxTaskSetup();
    if (dxScanTotal(scan) > 0 && walk != NULL) {
      in.lowKHz = walk->lowKHz;
      in.highKHz = walk->highKHz;
    } else if (radioTaskPlan(&plan)) {
      (void)bandLimits(snap.settings.band, &plan, &in.lowKHz, &in.highKHz);
    }
    /* The last walk as it was started, under way, stopped or finished, as
     * the band's edges and the counts are; the settings only once there is
     * none. The stop rule is always the setting's, since a scan under way
     * takes a change of it at once. */
    const bool walking = dxScanTotal(scan) > 0 && walk != NULL;
    in.range = walking ? walk->range : setup->range;
    in.memFirst = walking ? walk->memFirst : setup->memFirst;
    in.memLast = walking ? walk->memLast : setup->memLast;
    in.stop = setup->stop;
    in.learning = walking && walk->learning;
    in.sweeping = scan->state == DX_SCAN_RUNNING && dxTaskSweepRunning();
    in.dwellMs =
        scan->state == DX_SCAN_IDLE ? setup->dwellMs : scan->plan.dwellMs;
    in.scan = scan;
    in.snap = &snap;
    in.catches = list;
    in.page = page;
    in.pages = SCREEN_DX_PAGES;
    in.clock = clock;
    in.nowMs = millis();
    in.confirm = screenTaskHeaderMessage();
    static ScreenScan view;
    screenScanStateBuild(&in, &sScanKeep, &view);
    screenScanShow(&view);
    return true;
  }
  if (page == SCREEN_DX_PAGE_SCOPE) {
    screenDxStateFeed(&sKeep, &snap, millis());
    dxTaskSweepPoll();
    /* With no heap for the sweeps there is nothing to show but the frame
     * the page was built with. */
    DxSweepState sweep;
    if (!dxTaskSweepState(&sweep)) {
      return true;
    }
    const DxSweep *live = sweep.live;
    /* The cursor starts on the dial's channel, or the bottom when the dial
     * is not one of the sweep's. */
    if (live != NULL && !sScopeCursorSet) {
      const int16_t dial = dxSweepChannelOf(live, snap.settings.freqKHz);
      sScopeCursor = dial >= 0 ? (uint16_t)dial : 0;
      sScopeCursorSet = true;
    }
    ScreenScopeInputs in;
    memset(&in, 0, sizeof(in));
    in.live = live;
    in.base = sweep.base;
    in.baseN = sweep.baseN;
    in.baseFixed = sweep.baseFixed;
    in.peak = sweep.peak;
    in.revision = sweep.revision;
    in.sweeping = sweep.running;
    in.cursor = sScopeCursor;
    in.dialKHz = snap.settings.freqKHz;
    in.nowKnown = ntpEpochUtc(&in.nowUtc);
    in.offsetMinutes = ntpOffsetMinutes();
    in.page = page;
    in.pages = SCREEN_DX_PAGES;
    in.clock = clock;
    in.confirm = screenTaskHeaderMessage();
    in.touchOn = inputTouchUsable();
    ScreenScope view;
    screenScopeStateBuild(&in, &sScopeKeep, &view);
    screenScopeShow(&view);
    return true;
  }
  if (page == SCREEN_DX_PAGE_CATCHES) {
    screenDxStateFeed(&sKeep, &snap, millis());
    static ScreenCatches view;
    ScreenCatchesInputs in;
    memset(&in, 0, sizeof(in));
    in.list = list;
    in.cursor = *cursor;
    in.page = page;
    in.pages = SCREEN_DX_PAGES;
    in.clock = clock;
    in.offsetMinutes = ntpOffsetMinutes();
    in.confirm = screenTaskHeaderMessage();
    screenCatchesStateBuild(&in, &sCatchesKeep, &view);
    screenCatchesShow(&view);
    return true;
  }
  ScreenDxInputs in;
  in.snap = &snap;
  in.rdsEnabled = radioRdsEnabled();
  in.clock = clock;
  in.nowMs = millis();
  in.pages = SCREEN_DX_PAGES;
  in.confirm = screenTaskHeaderMessage();
  in.region = screenTaskRdsRegion();
  in.presetPi = memoryStorePi(snap.memorySlot);
  static ScreenDx view;
  screenDxStateBuild(&in, &sKeep, &view);
  screenDxShow(&view);
  return true;
}

void screenTaskDxScopeReset(void) {
  sScopeCursorSet = false;
}

void screenTaskDxScopeTurn(int32_t clicks) {
  DxSweepState sweep;
  if (!dxTaskSweepState(&sweep) || sweep.live == NULL || clicks == 0) {
    return;
  }
  sScopeCursor = (uint16_t)std::clamp<int32_t>(sScopeCursor + clicks, 0,
                                               sweep.live->count - 1);
  sScopeCursorSet = true;
}

void screenTaskDxScopeSet(uint16_t channel) {
  DxSweepState sweep;
  if (!dxTaskSweepState(&sweep) || sweep.live == NULL ||
      sweep.live->count == 0) {
    return;
  }
  sScopeCursor =
      channel < sweep.live->count ? channel : (uint16_t)(sweep.live->count - 1);
  sScopeCursorSet = true;
}

bool screenTaskDxScopeCursorKHz(uint32_t *khz) {
  DxSweepState sweep;
  if (!dxTaskSweepState(&sweep) || sweep.live == NULL || khz == NULL) {
    return false;
  }
  const uint16_t at = sScopeCursor < sweep.live->count
                          ? sScopeCursor
                          : (uint16_t)(sweep.live->count - 1);
  *khz = dxSweepKHzOf(sweep.live, at);
  return *khz != 0;
}
