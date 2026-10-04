/*
 * The DX session: the scanner, the level sweeps, the catches and their log
 * writes, the watch on the presets and the RDS lock timings, kept for DX
 * mode. The screen task tells it when DX mode opens, closes and changes its
 * width, and takes its step on each poll; the screen's pages and the API
 * read it and call it here, so neither goes through the other for it.
 */
#ifndef DX_TASK_H
#define DX_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/dx_catch.h"
#include "core/dx_scan.h"
#include "core/dx_sweep.h"
#include "core/settings.h"
#include "radio_task.h"

/* The DX pages, in the order BAND moves through them. */
#define SCREEN_DX_PAGE_DX 0
#define SCREEN_DX_PAGE_SCOPE 1
#define SCREEN_DX_PAGE_SCAN 2
#define SCREEN_DX_PAGE_CATCHES 3
#define SCREEN_DX_PAGES 4

/* One step of the session on `snap`: hear the radio for catches, then the
 * scanner and the watch on the presets. The DX page calls it as it draws. */
void dxTaskStep(const RadioSnapshot *snap);

/* The step, with nothing drawn, for while another screen holds the panel
 * over DX mode: catches, the scanner and the watch go on. False when the
 * radio has left FM, which DX mode cannot follow it to; true, and nothing
 * done, when the snapshot cannot be read. */
bool dxTaskTick(void);

/* What the session reports for a person to see, "Logged 106.40" or why a
 * press did nothing: a text, or a station just logged. The screen gives
 * these once, at start up, and they are not called before. */
typedef void (*DxSay)(const char *text);
typedef void (*DxSayLogged)(uint8_t band, uint32_t khz);
void dxTaskSetSay(DxSay say, DxSayLogged logged);

/* The width DX mode has in force, from the screen as it opens and whenever
 * it changes, for the hearing, which counts only readings taken through
 * it. */
void dxTaskSetWidth(uint16_t khz);

/* The DX SETUP menu's settings as the session holds them. */
typedef struct {
  uint8_t stop;  /* A DxStopRule. */
  uint8_t range; /* A DxScanRange. */
  uint8_t memFirst;
  uint8_t memLast;
  bool loop;
  bool mute;
  bool autoLog;
  uint32_t dwellMs;
  uint16_t widthKHz;
  bool logRt;
  bool watch; /* Watch Presets. */
  bool beep;  /* Key beeps are on, so a watch flag beeps too. */
} DxSetup;
const DxSetup *dxTaskSetup(void);

/* What a scan walks, kept from its start for its resumes. */
typedef struct {
  BandId band;
  BandPlanConfig plan;
  uint8_t range;    /* The DxScanRange it walks. */
  bool skipStored;  /* The band less the memory channels. */
  uint8_t memFirst; /* The memory walk's first slot, from 1, and its last. */
  uint8_t memLast;
  bool learning;   /* A Learn locals pass. */
  uint32_t lowKHz; /* The band's edges, for the page's labels. */
  uint32_t highKHz;
} DxScanWalk;
/* The walk of the last scan started, or NULL before DX mode first opened. */
const DxScanWalk *dxTaskWalk(void);

/* The level sweeps as the Scope page reads them: the latest, the baseline
 * and the peak, each NULL when there is none, and whether one is running. */
typedef struct {
  const DxSweep *live;
  const DxSweep *base;
  uint8_t baseN; /* How many sweeps the baseline comes from. */
  bool baseFixed;
  const DxSweep *peak;
  bool running;
  uint16_t revision; /* Moves whenever live, base or peak change. */
} DxSweepState;
/* False, and `out` cleared, before there is room for the sweeps. */
bool dxTaskSweepState(DxSweepState *out);
/* Take a finished sweep from the radio task, for the Scope page. */
void dxTaskSweepPoll(void);

/* Start the DX page's history again, for DX mode just opened. */
void dxTaskRestart(void);

/* DX mode is closing: write the catch the dial is on to the log if it is
 * due, since nothing will see the dial leave it now. */
void dxTaskLeave(void);

/* This session's catches, newest first. NULL when the heap had no room for
 * them, which the callers treat as an empty list. */
const DxCatches *dxTaskCatches(void);

/* Write catch `index` to the log now, the Catches page's hold and
 * `POST /api/dx log`. */
DxWriteResult dxTaskLogCatch(uint8_t index);

/* An ordinary log entry with PI `pi` on `khz` was written: the catch of it
 * is logged, so the auto log does not write the same station again. */
void dxTaskNoteLogged(uint32_t khz, uint16_t pi);

typedef enum {
  DX_SCAN_PRESS_RUNNING,      /* Started, or gone on after a stop. */
  DX_SCAN_PRESS_FINISHED,     /* Gone on from the top: finished at once. */
  DX_SCAN_PRESS_NOTHING,      /* No channel to walk: all stored, or none in
                           * memory. */
  DX_SCAN_PRESS_BUSY,         /* Already running, or a band scan is. */
  DX_SCAN_PRESS_UPDATE_CHECK, /* The update check is out on the network,
                               * and its transmitting would raise the
                               * levels read. */
  DX_SCAN_PRESS_RDS_OFF, /* The RDS decoder is off, so nothing could stop it. */
  DX_SCAN_PRESS_NO_RADIO,   /* The radio could not be read. */
  DX_SCAN_PRESS_NO_SEEN,    /* Learning, and the seen set could not be read. */
  DX_SCAN_PRESS_NO_MEMORY,  /* The heap had no room for the scanner. */
  DX_SCAN_PRESS_NOT_FM,     /* Learn locals only: the radio is not on FM. */
  DX_SCAN_PRESS_PANEL_BUSY, /* Learn locals only: DX mode would not open. */
} DxScanPress;

/*
 * The Scanner page's press: start a scan from the bottom of the band when idle,
 * or go on after a stop.
 */
DxScanPress dxTaskScanPress(void);

/* Stop the scan where it is, any key while it runs. */
void dxTaskScanStop(void);

/*
 * Learn the locals: a pass over the whole band that marks every PI heard as
 * caught, with no log entry and no NEW badge, and stops on none. The same
 * refusals as a press. DX mode must be open; the menu and the API start it
 * through screenTaskDxLearnLocals, which opens it.
 */
DxScanPress dxTaskLearn(void);

/* What a press or a Learn locals did, a few words for the panel. */
const char *dxTaskPressText(DxScanPress r);

/* The DX SETUP menu's settings, from settingsApplyLive and at start up. */
void dxTaskApplySettings(const Settings *s);

/*
 * Put the radio text heard now in a log entry about to be written, when Log
 * Radio Text is on and `dxLogRadioText` says the text is the entry's station's.
 * Every log write goes through this, the ENTER hold's as well as DX mode's, so
 * the switch has one reader.
 */
void dxTaskAddRadioText(LogbookEntry *e, const RadioSnapshot *snap);

/*
 * The preset watch, for GET /api/dx: true while it runs, with how many presets
 * it watches and the last one it flagged, `upKHz` 0 for none, with its rise in
 * tenths of a dB and millis() when it was flagged. Any pointer may be NULL.
 */
bool dxTaskWatchState(uint8_t *count, uint32_t *upKHz, int16_t *riseTenths,
                      uint32_t *upMs);

/* The width DX mode opens with, the DX SETUP menu's. */
uint16_t dxTaskOpenWidth(void);

/* The scanner, for the keys and the API. Never NULL. */
const DxScan *dxTaskScan(void);

/*
 * Whether the dial in `now` is on a channel nobody chose, because a seek, a
 * band scan or a DX scan is walking it. Anything that acts on where the radio
 * is, a preset saved or cleared or a logbook entry, refuses while this is
 * true. The seek is read from the caller's own snapshot, so the check and
 * what gets written agree about it; the two scans are read as they are now.
 * Both scans wait for their tune back at the end before they count as over,
 * unless the radio is too slow to carry it out in RADIO_TUNE_BACK_WAIT_MS.
 */
bool dxTaskDialWalking(const RadioSnapshot *now);

typedef enum {
  DX_SWEEP_STARTED,
  DX_SWEEP_BUSY,      /* A sweep, a DX scan or a band scan is running. */
  DX_SWEEP_CLOSED,    /* DX mode is not open. */
  DX_SWEEP_NO_MEMORY, /* The heap had no room for the sweeps. */
  DX_SWEEP_REFUSED,   /* The radio would not: a seek, off FM, or going down. */
} DxSweepStart;

/*
 * Sweep the band for its level, at DX mode's width. Returns at once; the sweep
 * becomes the live one when it ends, kept with the last DX_SWEEP_KEEP in
 * littlefs. A tune or anything else sent to the radio meanwhile ends it, as
 * does dxTaskScanStop, and nothing of it is kept. A fresh DX scan starts
 * with one of its own.
 */
DxSweepStart dxTaskSweep(void);

/* The latest sweep and what it is held against, for GET /api/dx/sweep.
 * Pointers into the glue's own store, good until the next call. */
typedef struct {
  bool running;
  bool abandoned;      /* The last sweep was ended before the top. */
  const DxSweep *live; /* NULL before any sweep. */
  const DxSweep *base; /* NULL when there is no baseline for it. */
  uint8_t baseN;       /* How many sweeps the baseline comes from. */
  bool baseFixed;      /* A sweep fixed by hand rather than the median. */
  const DxSweep *peak; /* NULL before a sweep since DX mode opened. */
} DxSweepView;

/* False, and nothing in `out`, when the heap had no room for the sweeps. */
bool dxTaskSweepView(DxSweepView *out);

typedef enum {
  DX_BASE_DONE,
  DX_BASE_NO_SWEEP,  /* There is no sweep to fix. */
  DX_BASE_NOT_SAVED, /* littlefs did not take it. Nothing changed. */
  DX_BASE_NO_MEMORY, /* The heap had no room. Nothing changed. */
} DxBaseResult;

/* Fix the live sweep as the baseline, kept across restarts, until
 * dxTaskBaselineAuto goes back to the median of the kept sweeps. */
DxBaseResult dxTaskBaselineNow(void);
DxBaseResult dxTaskBaselineAuto(void);

/* What a refused sweep press did, a few words for the panel. */
const char *dxTaskSweepText(DxSweepStart r);

/* A sweep is running, and ending it, the Scope page's press. */
bool dxTaskSweepRunning(void);
void dxTaskSweepStop(void);

/* Moves whenever the sweep, its baseline or its peak change, so a page can
 * tell it has an old one. 0 before the sweeps have a store. */
uint16_t dxTaskSweepRevision(void);

#endif /* DX_TASK_H */
