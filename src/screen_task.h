/*
 * Fills the screen in from the radio's state.
 *
 * Another composition root, like input_task.h and for the same reason. The
 * screen in `ui/` takes plain strings and knows nothing about tuners, tasks or
 * snapshots. This is the one place that knows both.
 *
 * It runs on the loop task. Drawing is SPI, so it never touches the tuner and
 * never holds the radio task up.
 *
 * The panel light is here too. It is a property of the screen rather than of
 * anything the radio receives, and the logic that decides how bright it
 * should be is in core/backlight.h where it can be tested.
 */
#ifndef SCREEN_TASK_H
#define SCREEN_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/touch_cal.h"
#include "ui/screen.h"

#include "dx_task.h"

#include "core/backlight.h"
#include "core/band_plan.h"
#include "core/rds_country.h"

/*
 * Start the panel, show the boot message and bring the light up.
 *
 * The fade runs inside this call rather than from the poll. Start up carries
 * on for seconds after this returns, through the tuner patch and the Wi-Fi
 * join, and a fade driven from the loop would leave the panel dark for all of
 * it.
 *
 * `rotationDegrees` is the stored display rotation, 0 or 180. It is applied
 * before anything is drawn, so the boot screen is already the right way up.
 */
bool screenTaskBegin(const BacklightConfig *cfg, uint16_t rotationDegrees);

/*
 * The self tests the boot screen reports, in the order they are drawn.
 *
 * The first three are the left column, the next three the right, and Touch
 * a row of its own across both under them. Every one
 * of them is something `setup` is told by the thing itself, and every one can
 * come back false. That is the test for whether a row belongs here. There is
 * no `Panel` row, because a panel that did not start has no boot screen to
 * put a cross on, so the row could only ever say yes.
 *
 * The clock is not here, because it needs a network that arrives after the
 * radio does.
 */
typedef enum {
  BOOT_STEP_SETTINGS = 0, /* The stored settings were read back. */
  BOOT_STEP_TUNER,        /* The tuner took its patch. */
  BOOT_STEP_RADIO,        /* The radio task started and owns the tuner. */
  BOOT_STEP_CHANNELS, /* The channel store opened. The value is the count. */
  BOOT_STEP_KEYPAD,   /* A keypad answered at 0x20. */
  BOOT_STEP_BATTERY,  /* The one battery reading this boot gets. */
  BOOT_STEP_TOUCH,    /* The touch chip answered with its temperature. */
} BootStep;

/*
 * Report one self test, and draw it.
 *
 * Called from `setup` as each thing comes up, so the screen fills in as the
 * radio does. It draws immediately: the poll that normally redraws the panel
 * does not run until `loop` does.
 *
 * `value` is what the step found, or NULL for a plain tick. It is copied, so
 * a caller may pass a buffer it is about to reuse.
 */
void screenTaskBootStep(BootStep step, bool ok, const char *value);

/* What the tuner says it is, once it has said it. Copied, like `value`. */
void screenTaskBootTuner(const char *text);

/*
 * Take a row off the screen, because this radio does not have the thing.
 *
 * A board with no battery fitted must not carry a red cross beside `Battery`
 * on every boot for the life of the radio. A cross means something answered
 * and answered badly; hardware that is not there answered nothing. The row
 * goes, and the total goes down with it, so the count still ends full.
 */
void screenTaskBootAbsent(BootStep step);

/*
 * Start up has finished, so let the boot screen go.
 *
 * It does not go at once. The screen is held for about a second and a half
 * from here so there is time to read it, and the poll is what takes it down.
 * The hold is a deadline rather than a delay: the radio, the knob, the keypad
 * and the web server are all working for the whole of it, and only the panel
 * is still showing start up.
 *
 * Nothing else takes the boot screen down, so a caller that never calls this
 * leaves the radio behind it for ever.
 */
void screenTaskBootEnd(void);

/*
 * How long the last screen change took, in milliseconds.
 *
 * The delete, the build, the first draw and the repaint, with the light off
 * for all of it. Published because the fade either side of it is a number
 * this file chose and this is a number the hardware decides, and without it
 * "the transition feels slow" cannot be argued with.
 *
 * 0 until a screen has been changed.
 */
uint16_t screenTaskSwapMs(void);

/*
 * The RDS screen, reached by holding BAND and left with a tap of MODE.
 *
 * Four pages, and the knob belongs to this screen rather than to the dial
 * while it is up: `screenTaskRdsPage` takes the clicks `input_task` would
 * otherwise turn into a tuning step, one click one page the same way the
 * menu takes one click one row. Unlike the menu it redraws from the poll,
 * because what it shows arrives a group at a time and a station takes
 * seconds to say its name.
 *
 * `screenTaskRdsToggle` is the gesture that opens it, and also closes it, so
 * the long press that opened it keeps working as a way out too. Opening the
 * menu closes it, because the LVGL pool holds one screen at a time.
 */
void screenTaskRdsToggle(void);
void screenTaskRdsClose(void);
bool screenTaskRdsIsOpen(void);

/*
 * Which screen is up, for GET /api/screen: "boot", "menu", "bandwidth", "rds",
 * "dx" or "radio", and in `page`, when not NULL, the page counted from 0 on the
 * RDS and DX screens and 0 on the rest.
 */
const char *screenTaskShowing(uint8_t *page);
void screenTaskRdsPage(int32_t clicks);

/*
 * The bandwidth page, reached by holding BW and left with a tap of BW or of
 * MODE. It opens over the radio screen or over the DX page it found up, and
 * puts that back when it closes. The knob moves its cursor a tile a click, and
 * a press, or ENTER, picks: a width for the radio, or DX mode's width when it
 * opened over a DX page, or one of the two switches. It stays up after a pick,
 * so the widths can be compared. Opening the menu closes it, one screen in the
 * pool.
 */
bool screenTaskBwOpen(void);
void screenTaskBwClose(void);
bool screenTaskBwIsOpen(void);

/*
 * The frequency keypad, a screen of its own in place of the radio screen,
 * opened by a touch on the frequency. It shows the number typed, the same
 * one the keys type, and closes by its Cancel and OK, by MODE, or after
 * Keypad Timeout with no key used.
 */
bool screenTaskKeypadOpen(void);
void screenTaskKeypadClose(void);
bool screenTaskKeypadIsOpen(void);
/* A key of it used: the time it stays up starts again, and it is drawn. */
void screenTaskKeypadKeyed(void);

/*
 * The touch calibration screen, from the radio screen. False when the panel
 * is busy with another screen or has no memory for it. While it is up the
 * input task feeds it each poll's contact, the steady raw point and whether
 * it is unsettled; the knob answers it: in the marks or the check either
 * leaves, the old calibration kept; on a calibration kept a press leaves; on
 * one not kept a press starts again and a turn leaves. A calibration that
 * passes its check is kept in NVS and put in use at once.
 */
bool screenTaskTouchCalOpen(void);
bool screenTaskTouchCalIsOpen(void);
void screenTaskTouchCalFeed(bool contact, bool fresh, TouchPoint raw,
                            bool unsettled, uint32_t nowMs);
void screenTaskTouchCalKnob(bool press);
void screenTaskTouchCalClose(void);

/* What the calibration screen draws for `f`, for recovery's own loop as
 * well as this task. */
void screenTaskTouchCalView(const TouchCalFlow *f, ScreenTouchCal *out);

/* Feed a calibration one poll, and save it the moment it passes its check;
 * one that cannot be saved ends Not kept. True when the screen changed. The
 * calibration screen and recovery's both use it. */
bool screenTaskTouchCalStep(TouchCalFlow *f, bool contact, bool fresh,
                            TouchPoint raw, bool unsettled, uint32_t nowMs);
void screenTaskBwTurn(int32_t clicks);
void screenTaskBwPick(void);

/* A tile tapped: turned to and picked, as the knob and its press would. */
void screenTaskBwTap(uint8_t index);

/*
 * Give the panel to the menu, and take it back.
 *
 * The radio layout comes down before the menu is built and goes back up as the
 * menu is deleted, so only one of them is ever in the LVGL pool. The 24 KB pool
 * cannot hold two screens, and an allocation LVGL cannot make is an assertion,
 * which restarts the radio.
 *
 * `screenTaskMenuDrawn` puts what the menu just drew on the glass now rather
 * than at the next poll. A press that took forty milliseconds to show would
 * read as a press that did not register.
 *
 * These are called by `menu_task`, which is the one place that knows what is
 * in the menu. Nothing else should call them.
 */
bool screenTaskMenuBegin(void);
void screenTaskMenuDrawn(void);
void screenTaskMenuEnd(void);

/*
 * Redraw whatever changed, and move the panel light along.
 *
 * Call this from loop(). It reads a snapshot and compares against what is
 * already on the panel, so calling it often costs nothing when nothing moved.
 */
void screenTaskPoll(void);

/*
 * The panel while firmware is being written, for both update routes.
 *
 * The write takes over the screen because it takes over the radio: nothing
 * can be tuned, and the radio reboots at the end of it. A panel still showing
 * a station is the misleading kind of true. On both routes the audio is
 * hushed as well, at the start, by `firmwareWriteBegin`.
 *
 * These are called from the update callbacks rather than from the poll, and
 * they draw immediately, because `screenTaskPoll` does not run during either
 * route: a browser upload sits inside the POST handler on this task for the
 * whole transfer, and ArduinoOTA reads the whole image inside one `handle()`
 * call.
 *
 * The hold is still needed, for what happens after. A write that fails hands
 * the panel back at the end of the callback, and the next poll would paint
 * the station over the failure before anyone had read it.
 *
 * `percent` is -1 when the size is not known, which shows the screen without
 * a number rather than a made up one.
 */
void screenTaskUpdateBegin(void);

void screenTaskUpdateProgress(int percent);

/*
 * The write finished. `ok` true means a reboot is coming and the screen stays
 * as it is until it happens; false puts up a failure for a few seconds and
 * then gives the panel back, because the radio carries on running the image
 * it already had.
 */
void screenTaskUpdateEnd(bool ok);

/*
 * Whether the battery is shown, and as what. One of BatteryShow.
 *
 * Given rather than read out of the radio snapshot, because the snapshot
 * carries what the radio is tuned to and this is not that. Acts at once, like
 * the panel light: a setting whose whole point is what appears on the screen
 * cannot sensibly wait for a restart.
 */
void screenTaskSetBatteryShow(uint8_t show);

/*
 * The RDS region, one of RdsRegion, which decides whether the RDS page and
 * the DX page read a PI as North American call letters. Given, like the
 * battery, and acts at once. screenTaskRdsRegion reads it back.
 */
void screenTaskSetRdsRegion(uint8_t region);

/*
 * The level offsets: whole dB added to every level the panel shows, FM for FM
 * and OIRT, AM for LW, MW and SW. Given, like the battery, and acts at once;
 * out of range is ignored. screenTaskLevelOffsetDb gives the one for `band`,
 * for the other screens and the web pages.
 */
void screenTaskSetLevelOffsets(int8_t fmDb, int8_t amDb);
int8_t screenTaskLevelOffsetDb(BandId band);
RdsRegion screenTaskRdsRegion(void);

void screenTaskSetBacklight(const BacklightConfig *cfg);

bool screenTaskBacklightState(uint8_t *percent);

/*
 * Show `text` in the station name band for a second and a half, then let it
 * go back to the station or memory name.
 *
 * A later call while one is already showing restarts the second and a half
 * rather than queuing, so two holds close together read as the second one
 * having happened rather than a banner stuck describing the first.
 */
void screenTaskLogConfirm(const char *text);

/* "Logged" and the frequency written, the confirmation every log gives. */
void screenTaskLogConfirmAt(uint8_t band, uint32_t khz);

/*
 * What the header of the RDS screen and the DX pages says for a moment in
 * place of its page and clock: the digits being typed, "Tune 104-", since
 * those screens have no frequency of their own to draw them in, or else the
 * log confirmation. NULL with neither.
 */
const char *screenTaskHeaderMessage(void);

/*
 * The message before the radio sleeps, "Going to Sleep" and how it wakes,
 * over the radio screen with every other screen closed and the panel lit,
 * held there until `on` is false again, when the radio screen comes back.
 */
void screenTaskSleepShow(bool on);
bool screenTaskSleepShowing(void);

/*
 * Whether the boot screen is still up, held after start up or fading into
 * the radio screen. While it is held this also ends the hold, so the radio
 * screen comes at once: a touch then skips to the radio rather than changing
 * it out of sight.
 */
bool screenTaskBootSkip(void);

/* Whether the boot screen is still up, held or fading, and nothing more:
 * unlike screenTaskBootSkip, asking does not end the hold. */
bool screenTaskBootShowing(void);

/*
 * Whether a firmware write holds the panel. A failed write's message is
 * taken down at the next poll, so a press there only closes it.
 */
bool screenTaskUpdateSkip(void);

/* Whether a firmware write, or a failed write's message, holds the panel,
 * and nothing more: unlike screenTaskUpdateSkip, asking does not close the
 * message. */
bool screenTaskUpdateHolding(void);

/*
 * The signal number the panel is actually showing, in whole dBuV.
 *
 * Not the same as the smoothed level in the snapshot. The screen holds its
 * number until the level moves a whole dB from it, so the two differ most of
 * the time, and the held one is what a person is reading.
 *
 * Published because otherwise the only way to check it is to stand at the
 * radio and look, and the checks that matter here are about a number sitting
 * still over half a minute.
 */
int16_t screenTaskSignalShown(void);

/*
 * DX mode. Opening it closes the RDS screen, puts DX mode's own fixed width in
 * force and shows the DX page; closing it puts the radio's own width back and
 * the radio screen. The menu and a firmware write close it the same way they
 * close the RDS screen.
 */
typedef enum {
  SCREEN_DX_OPEN = 0,   /* Open now, or open already. */
  SCREEN_DX_RADIO_BUSY, /* The radio could not be read to say which band. */
  SCREEN_DX_NOT_FM,     /* DX mode is FM only: every page is about an FM
                         * station. */
  SCREEN_DX_PANEL_BUSY, /* The panel belongs to something else. */
} ScreenDxOpenResult;

/* Whether DX mode could open as the radio is now, SCREEN_DX_OPEN or why not,
 * with nothing changed. For a caller that has to know before it closes what
 * is in front of the panel, such as the menu, which keeps its note on show
 * when the answer is no. */
ScreenDxOpenResult screenTaskDxCanOpen(void);

/* Open DX mode, checking screenTaskDxCanOpen first, and say what happened. */
ScreenDxOpenResult screenTaskDxOpen(void);

void screenTaskDxClose(void);
bool screenTaskDxIsOpen(void);

/* Step DX mode's width to the next of the tuner's fixed ones and return it,
 * or 0 when DX mode is not open. */
uint16_t screenTaskDxCycleWidth(void);

/* Set DX mode's width to one of the tuner's fixed FM widths. False when DX
 * mode is not open or the width is not one of them. */
bool screenTaskDxSetWidth(uint16_t khz);

/* The width DX mode wants, or 0 when it is not open. */
uint16_t screenTaskDxWidth(void);

/* Which DX page is up: SCREEN_DX_PAGE_DX, _SCOPE, _SCAN or _CATCHES. */
uint8_t screenTaskDxPage(void);

/* The next DX page, wrapping round: BAND on every DX page. */
void screenTaskDxNextPage(void);

/* The next DX page, `dir` 1, or the one before, -1, going round, for a
 * swipe. */
void screenTaskDxStepPage(int dir);

/* The Scanner page, for a scan started over HTTP, so the panel shows what
 * the radio is doing. Nothing unless DX mode is open. */
void screenTaskDxShowScanner(void);

/* Bring up the Scope page, for a sweep asked for over the API, so the panel
 * shows it running. Nothing unless DX mode is open. */
void screenTaskDxShowScope(void);

/*
 * Learn locals, the one call the DX SETUP menu row and `POST /api/dx learn=1`
 * share: FM, RDS on and no scan running are checked first, DX mode is opened if
 * it is closed, the pass starts with the Scanner page up, and DX mode is closed
 * again if it was opened for a pass that was then refused.
 */
DxScanPress screenTaskDxLearnLocals(void);

/* Move the Catches page's cursor a row per click, or the Scope page's a
 * channel per click. Nothing on the DX page, where the knob tunes. */
void screenTaskDxTurn(int32_t clicks);

/* The catch under the Catches page's cursor. False when that page is not up
 * or the list is empty. */
bool screenTaskDxCursorCatch(DxCatch *out);

/* Where the Catches page's cursor is, 0 the newest catch. */
uint8_t screenTaskDxCursor(void);

/* Write the catch under the cursor to the log, the Catches page's hold.
 * DX_WRITE_NO_CATCH when that page is not up or the list is empty. */
DxWriteResult screenTaskDxLogCursor(void);

#endif /* SCREEN_TASK_H */
