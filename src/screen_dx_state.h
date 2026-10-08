/*
 * The DX page's view, built from the radio's snapshot.
 *
 * The same split as screen_state.cpp: the screen task reads the snapshot and
 * the clock into `ScreenDxInputs`, and this turns them into a `ScreenDx`. It
 * reaches no driver, no lock and no clock of its own, so the renderer in
 * `tools/screenshot.cpp` can run it on a PC. Glue like the task files, not a
 * layer of its own.
 */
#ifndef SCREEN_DX_STATE_H
#define SCREEN_DX_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "core/dx.h"
#include "core/dx_catch.h"
#include "core/dx_scan.h"
#include "core/dx_sweep.h"
#include "core/rds_country.h"
#include "radio_task.h"
#include "ui/screen.h"

typedef struct {
  const RadioSnapshot *snap; /* What the radio is doing. Never NULL. */
  bool rdsEnabled;           /* The RDS decoder is switched on. */
  const char *clock;         /* Local, "22:47", or NULL when it is not known. */
  uint32_t nowMs;            /* The millisecond clock, for the history. */
  uint8_t pages;             /* How many DX pages there are. */
  /* What a log just did, shown in the header for a moment, or NULL. */
  const char *confirm;
  RdsRegion region;     /* Whether a PI is read as call letters. */
  int8_t levelOffsetDb; /* The FM level offset, whole dB. */
  /* The stored PI of the preset the dial is on, `snap->memorySlot`, or 0
   * when it is on none or the preset has learnt none. */
  uint16_t presetPi;
} ScreenDxInputs;

/* What the builder keeps between builds: the last minute of signal, the
 * offset and noise held still enough to read, and the text the view points
 * at, which has to outlive the call that shows it. */
typedef struct {
  DxHistory history;
  uint32_t historyKHz; /* The channel the history and the holds are of. */
  /* The offset and the noise as shown, each fed once per reading the tuner
   * takes, `heldReads` being the count of reads last fed. */
  ReadingHold offsetHold;
  ReadingHold usnHold;
  int16_t offsetShownTenths;
  int16_t usnShownTenths;
  uint32_t heldReads;
  bool held;
  char position[8];
  char frequency[16];
  char level[12];
  char usn[8];
  char wam[8];
  char offset[8];
  char bandwidth[8];
  char modulation[8];
} ScreenDxKeep;

/* Start again, as on opening the page. */
void screenDxStateReset(ScreenDxKeep *keep);

/* Add the snapshot's level to the minute of history, starting it again on a
 * retune. screenDxStateBuild does it; the other DX pages call it on every
 * poll too, so the history has no gap for the time they were up. */
void screenDxStateFeed(ScreenDxKeep *keep, const RadioSnapshot *snap,
                       uint32_t nowMs);

/* One reading of the snapshot into the view. */
void screenDxStateBuild(const ScreenDxInputs *in, ScreenDxKeep *keep,
                        ScreenDx *out);

/* What the Catches page's text points at, one set per row. */
typedef struct {
  char range[24];
  char position[8];
  char time[SCREEN_CATCH_ROWS][8];
  char frequency[SCREEN_CATCH_ROWS][12];
  char pi[SCREEN_CATCH_ROWS][6];
  char ps[SCREEN_CATCH_ROWS][RDS_PS_LEN + 1];
  char level[SCREEN_CATCH_ROWS][10];
  char count[SCREEN_CATCH_ROWS][10];
} ScreenCatchesKeep;

/* Everything the Catches page is built from, read by the caller. */
typedef struct {
  const DxCatches *list; /* The catches, or NULL for none. */
  uint8_t cursor;        /* The catch the cursor is on, 0 the newest. */
  uint8_t page;          /* The page's place in DX mode, */
  uint8_t pages;         /* and how many pages there are. */
  const char *clock;     /* The local clock, or NULL. */
  int16_t offsetMinutes; /* The local offset the times are shown in. */
  int8_t levelOffsetDb;  /* The FM level offset the levels are shown with. */
  /* What a hold on a row just did, shown in the header for a moment, or
   * NULL. */
  const char *confirm;
} ScreenCatchesInputs;

/* The Catches page from `in`. The screen shows the SCREEN_CATCH_ROWS catches
 * that hold the cursor. */
void screenCatchesStateBuild(const ScreenCatchesInputs *in,
                             ScreenCatchesKeep *keep, ScreenCatches *out);

/* What the Scanner page's text points at. */
typedef struct {
  char found[16];
  char position[8];
  char mode[20];
  char frequency[16];
  char dwell[16];
  char left[16];
  char from[16];
  char to[16];
  char step[16];
  char pi[6];
  char ps[RDS_PS_LEN + 1];
  char level[12];
} ScreenScanKeep;

typedef struct {
  const DxScan *scan;
  const RadioSnapshot *snap; /* Never NULL. */
  const DxCatches *catches;  /* For the NEW pill, or NULL. */
  uint32_t lowKHz;           /* The band's edges, for the labels under */
  uint32_t highKHz;          /* the bar of a band walk. */
  /* The DX SETUP menu's: what is walked, what stops it, and how long each
   * channel gets. */
  uint8_t range; /* A DxScanRange. */
  uint8_t memFirst;
  uint8_t memLast;
  uint8_t stop; /* A DxStopRule. */
  bool learning;
  bool sweeping; /* The sweep a fresh scan starts with is running. */
  uint32_t dwellMs;
  uint8_t page;
  uint8_t pages;
  const char *clock; /* Local, or NULL. */
  uint32_t nowMs;
  const char *confirm;  /* A moment's message for the header, or NULL. */
  int8_t levelOffsetDb; /* The FM level offset, whole dB. */
} ScreenScanInputs;

/*
 * The Scanner page from the scan and the radio. Running, the frequency is
 * the channel being listened to and the tile what has been heard on it so
 * far; stopped, the frequency is the dial and the tile is filled when the
 * stop was a confirmed PI on it.
 */
void screenScanStateBuild(const ScreenScanInputs *in, ScreenScanKeep *keep,
                          ScreenScan *out);

/* What the Scope page's text points at. */
typedef struct {
  char context[16];
  char position[8];
  char from[8];
  char mid[8];
  char to[8];
  char baseText[16];
  char floorText[16];
  char freq[12];
  char level[12];
  char rise[12];
} ScreenScopeKeep;

typedef struct {
  const DxSweep *live; /* The latest sweep, or NULL before any. */
  const DxSweep *base; /* Its baseline, or NULL. */
  uint8_t baseN;       /* How many sweeps the baseline comes from. */
  bool baseFixed;      /* One sweep fixed by hand. */
  const DxSweep *peak; /* The peak hold, or NULL. */
  uint16_t revision;   /* Moves when any of the three change. */
  bool sweeping;
  uint16_t cursor;  /* The cursor's channel of `live`. */
  uint32_t dialKHz; /* Where the dial is. */
  bool nowKnown;    /* The UTC clock is set, for the sweep's age. */
  uint32_t nowUtc;
  uint8_t page;
  uint8_t pages;
  const char *clock;    /* Local, or NULL. */
  const char *confirm;  /* A moment's message for the header, or NULL. */
  int8_t levelOffsetDb; /* The FM level offset, whole dB. */
  bool touchOn; /* Touch is On and the chip answered: the touch buttons. */
  /* The band scope's: its title, its span in place of the page position, and
   * a span is not the whole band, so it has no noise floor. NULL and false
   * on DX mode's page. */
  const char *title;
  const char *position;
  bool span;
  const uint16_t *marks;
  uint8_t markCount;
  const uint16_t *catches;
  uint8_t catchCount;
} ScreenScopeInputs;

/*
 * The Scope page from the latest sweep. A baseline or a peak hold that reads
 * other channels than the sweep is left out rather than drawn against the wrong
 * ones. The age shows only when the sweep's time and the clock are both known.
 */
void screenScopeStateBuild(const ScreenScopeInputs *in, ScreenScopeKeep *keep,
                           ScreenScope *out);

#endif /* SCREEN_DX_STATE_H */
