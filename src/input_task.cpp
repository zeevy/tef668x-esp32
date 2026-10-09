/* Implementation of the input layer. */
#include "input_task.h"
#include "debug_log.h"

#include "core/dx.h"
#include "dx_task.h"
#include "menu_task.h"
#include "screen_task.h"
#include "screen_task_dx.h"
#include "ui/draw.h"

#include "board/board.h"
#include "core/logbook.h"
#include "core/scale.h"
#include "core/settings.h"
#include "core/squelch.h"
#include "core/strings.h"
#include "core/touch_cal.h"
#include "drivers/analog.h"
#include "drivers/encoder.h"
#include "drivers/keypad.h"
#include "drivers/logbook_fs.h"
#include "drivers/settings_nvs.h"
#include "drivers/touch.h"
#include "net/ntp.h"
#include "radio_task.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

/* The four buttons, in PanelButton order. */
static Button sButtons[PANEL_BUTTON_COUNT];

/*
 * ENTER's hold, watched the same way a button is: nothing fires on the way
 * down, a tap fires on release, a hold fires once at the long press time.
 * The long press time is BUTTON_LONG_MS, the same one the panel buttons
 * use, so there is one idea of a long press on this radio and not two.
 */
static KeypadHold sEnterHold;

/* How fast the knob is being turned. */
static Acceleration sAcceleration;

/* Digits keyed and not yet entered. */
static char sTyped[INPUT_DIGITS_MAX + 1];
static uint8_t sTypedLen = 0;

/* When the last digit was keyed, so a half typed number does not sit
 *  there for ever waiting for an enter that is not coming. */
static uint32_t sTypedMs = 0;
/* Keypad Timeout, which the settings set before any key can be read. */
static uint32_t sKeypadTimeoutMs = SETTINGS_KEYPAD_TIMEOUT_DEFAULT_S * 1000;

/*
 * How often the keypad is read, in milliseconds.
 *
 * The keypad sits on the tuner's I2C bus, so every read here is a read the
 * radio task has to wait for. Fifty times a second is far quicker than
 * anybody types and costs the bus almost nothing.
 */
#define KEYPAD_POLL_MS 20

/*
 * How long a button press waits for the radio, in milliseconds.
 *
 * Long enough for a retune, which is five I2C writes with a settle after
 * each. The wait is on the loop task, never on the radio task.
 */
#define BUTTON_SETTLE_MS 300

/*
 * How often the volume pot is read, in milliseconds.
 *
 * The same rate the working firmware uses. Faster buys nothing: a hand cannot
 * turn a knob faster than this and the converter needs averaging anyway.
 */
#define POT_POLL_MS 50

static InputStatus sStatus;
#if FEATURE_TOUCH
/* The Touch setting. */
static bool sTouchOn = true;
/* Raw readings made steady, and the map from them to pixels: the one a
 * person made, or the board's own, and whether the screen is shown upside
 * down. */
static TouchFilter sTouchFilter;
static TouchCal sTouchCal;
static bool sTouchUpsideDown = false;
#endif

/* The last pot reading acted on, and whether there is one yet. */
static uint16_t sPot = 0;
static bool sPotKnown = false;

/*
 * How this unit's knob maps to volume and to a squelch threshold.
 *
 * The defaults are one radio's numbers, taken from the reference firmware:
 * the travel runs 120 to 4000 there. This unit reaches 0 and 4095, so they
 * are not wrong here, but a pot that read 200 to 3800 would lose travel at
 * both ends with nothing to say so. Calibration replaces them with what this
 * knob actually reaches.
 */
static PotConfig sPotCfg;

/* Calibration. While it runs the knob does nothing but record how far it
 * goes, so that sweeping to the loud end to find the end stop is not painful.
 * The state machine is in core/input.c, where it can be tested on a PC. */
static PotCalibration sCal;

/* Which presses make a sound. Off unless somebody asks. */
static BeepMode sBeepMode = BEEP_OFF;

/*
 * How long a beep lasts, in milliseconds.
 *
 * The reference firmware's figure for its band edge beep on this chip. Short
 * enough to be a tick rather than a tone.
 */
#define BEEP_MS 50

/*
 * And for a long press, which is deliberately different.
 *
 * A long press is the one you cannot tell has registered: there is nothing to
 * feel, and the moment it fires is decided by a timer rather than by letting
 * go. A tick of a different length says which of the two the radio took.
 */
#define BEEP_LONG_MS 200

/*
 * How many things a person has done to this radio, counted for inputActivity.
 *
 * Separate from the counts in InputStatus, which are per source and are there
 * for the diagnostic page. This one answers a different question, which is
 * whether the radio has been left alone.
 */
static uint32_t sActivity = 0;

/* What the knob was last doing, so a change of job can be acted on. */
static SquelchMode sJob = SQUELCH_OFF;
static bool sJobKnown = false;

/*
 * One press or turn sent over POST /api/key, waiting for the next poll. It goes
 * to the same handler as a real one, so the beep, the wake, a number being
 * typed and the note all follow. The counts in InputStatus are left alone: they
 * say what the hardware saw, and a press from the API counted there would make
 * a dead button look alive.
 */
typedef struct {
  bool pending;
  InputKey key;
  ButtonEvent event;
  int32_t clicks; /* Not 0 for a turn of the knob, which has no key. */
#if FEATURE_TOUCH
  TouchGestureEvent touch; /* Not TOUCH_NOTHING for a touch, which has none. */
  TouchPoint at;           /* Where it began. */
  TouchPoint to;           /* Where a drag ended. */
#endif
} ApiInput;
static ApiInput sApi;
/* While one is handled, so its note says where it came from. */
static bool sFromApi = false;

static void note(const char *what) {
  snprintf(sStatus.lastEvent, sizeof(sStatus.lastEvent), "%s%s",
           sFromApi ? "api " : "", what);
  sStatus.lastEventMs = millis();
  DebugLog.printf("[input] %s\n", what);
}

/*
 * Whether the panel is dark right now, ahead of anything this poll is about
 * to do.
 *
 * `loop()` calls `inputPoll` before `screenTaskPoll`, so an activity count
 * bumped moments ago by this same key, click or turn has not reached
 * `screenTaskPoll` yet, and the wake it causes has not run. This still reads
 * the panel as it was before the input arrived: dark. Acting on the input as
 * well would change something the panel had no chance to show, on a wake
 * that is a straight jump rather than a fade, so there is no window to wait
 * out, only this one read to make before deciding whether to act.
 */
static bool panelIsDimmed(void) {
  /* While the radio says it is going to sleep, a key or a turn only keeps
   * it awake, as the first one on a dimmed panel only lights it; while the
   * boot screen is up, one only skips to the radio screen; and while a
   * firmware write holds the panel, one only closes its failure message. */
  return screenTaskBacklightState(NULL) || screenTaskSleepShowing() ||
         screenTaskBootSkip() || screenTaskUpdateSkip();
}

#if FEATURE_TOUCH
/*
 * Whether a touch does nothing now: the panel dark, the radio going to
 * sleep, the boot screen up, a firmware write or its failure message
 * holding the panel, or a level sweep running, whose readings must not take
 * in the chip's clock. Asked on every poll, so it only looks: panelIsDimmed
 * also ends the boot hold and closes the failure message, which is what a
 * key there is for, not a finger.
 */
static bool touchIgnored(void) {
  return screenTaskBacklightState(NULL) || screenTaskSleepShowing() ||
         screenTaskBootShowing() || screenTaskUpdateHolding() ||
         radioSweepBusy();
}
#endif

static void clearTyped(void) {
  sTypedLen = 0;
  sTyped[0] = '\0';
  memcpy(sStatus.typed, sTyped, sizeof(sTyped));
}

/*
 * Whether a number is still being typed, checked live rather than trusted
 * from whichever poll last touched `sTypedLen`.
 *
 * `pollKeypad` clears an entry that has timed out, but only on its own poll,
 * `KEYPAD_POLL_MS` apart. A caller elsewhere that trusted `sTypedLen` alone
 * could see an entry that timed out up to one of those polls ago and still
 * treat it as current. Working the timeout out here as well means a knob
 * turn or a button pressed in that gap sees the entry as already gone,
 * rather than swallowing an action against a number nobody is typing any
 * more.
 */
static bool typingInProgress(uint32_t nowMs) {
  return sTypedLen > 0 && (uint32_t)(nowMs - sTypedMs) < sKeypadTimeoutMs;
}

/*
 * Cancel a number in progress, and say so. One place for both, so a change
 * to what cancelling does, such as a different note or a beep of its own,
 * reaches every caller.
 */
static void cancelTyping(void) {
  if (sTypedLen == 0) {
    return;
  }
  clearTyped();
  note("typed number cleared");
}

bool inputBegin(EncoderKind kind, EncoderDirection direction) {
  memset(sButtons, 0, sizeof(sButtons));
  memset(&sEnterHold, 0, sizeof(sEnterHold));
  memset(&sAcceleration, 0, sizeof(sAcceleration));
  memset(&sStatus, 0, sizeof(sStatus));
  clearTyped();

  /* Nothing is known about the knob or about what it is for until the first
   * poll, so both are forgotten here rather than carried over. */
  sPotKnown = false;
  sJobKnown = false;
  potCalibrateCancel(&sCal);
  potDefaults(&sPotCfg);

  encoderBegin(kind, direction);
  /* A press already down is the one that woke the radio from sleep, or one
   * held through start up. It is let go before it counts, so the wake does
   * not also open the menu or log the station. */
  for (int i = 0; i < PANEL_BUTTON_COUNT; i++) {
    if (encoderButtonDown((PanelButton)i)) {
      buttonStartHeld(&sButtons[i], millis());
    }
  }
  analogBegin();
  sStatus.keypadPresent = keypadBegin();
#if FEATURE_TOUCH
  touchBegin();
  sStatus.touchChip = touchChipAnswers(touchTemp0());
  /* So a finger already down at the first poll is counted as a press. */
  sStatus.touchOn = sTouchOn;
  memset(&sTouchFilter, 0, sizeof(sTouchFilter));
  {
    TouchCal cal;
    bool stored = false;
    if (touchCalNvsLoadOrBoard(DISPLAY_WIDTH, DISPLAY_HEIGHT, &cal, &stored)) {
      inputTouchCalSet(&cal, stored);
    }
  }
#endif

  /* Read the pot for the status document, and send nothing.
   *
   * Where the radio starts is main.cpp's job, because only it knows what the
   * knob is for: in manual squelch the knob is the squelch control and the
   * volume comes from the stored one instead. Sending a volume here would
   * overwrite that a few milliseconds after the task was started with it.
   *
   * The first pollPot takes over. Its jobChanged is true on the first poll,
   * so it acts at once rather than waiting for the knob to move past the
   * deadband. */
  sPot = potRead();
  sStatus.pot = sPot;
  sStatus.potDb = potVolumeDb(sPot, &sPotCfg);

  return sStatus.keypadPresent;
}

/*
 * Any key or turn while the DX scanner runs stops it and does nothing else,
 * dark panel or not: the scan is muted and moving, and a key pressed then means
 * stop, whatever it would do otherwise. True when it did, and the caller drops
 * the key.
 */
static bool stopScanFirst(void) {
  if (dxTaskScan()->state != DX_SCAN_RUNNING) {
    return false;
  }
  dxTaskScanStop();
  note("DX scan stopped");
  return true;
}

/*
 * Whether the Scanner page is up and not stopped, where it shows no frequency:
 * a turn or a typed number there would move a dial nobody sees, so both are
 * refused, and the page says how to start. `dxShown` is whether DX mode is up,
 * on top or under the RDS screen.
 */
static bool scannerRefusesTuning(bool dxShown) {
  if (!dxShown || screenTaskDxPage() != SCREEN_DX_PAGE_SCAN ||
      dxTaskScan()->state == DX_SCAN_STOPPED) {
    return false;
  }
  screenTaskLogConfirm(txt(STR_RADIO_PRESS_TO_START));
  return true;
}

static InputLogResult writeLogEntry(uint32_t nowMs);

/*
 * Send one command, wait for the radio, and say where it ended up.
 *
 * The buttons all mean "the next one", and the radio works that out from its
 * own state. Reading the state here, working out the next value and sending
 * that would leave a gap for the state to move in, and the button could then
 * send a value that does not fit the band the radio just landed on.
 *
 * `kind` is one of the three cycles the buttons send: band, bandwidth and
 * tuning mode.
 */
static void cycleAndNote(RadioCommandKind kind) {
  RadioCommand cmd = {};
  cmd.kind = kind;

  RadioError why = RADIO_OK;
  if (radioPostAndSettle(&cmd, BUTTON_SETTLE_MS, &why) != RADIO_POST_DONE ||
      why != RADIO_OK) {
    note(why != RADIO_OK ? radioErrorText(why) : "the radio is busy");
    return;
  }

  /* What it actually reached, read back after it settled. */
  RadioSnapshot now;
  if (!radioGetSnapshot(&now)) {
    note("done");
    return;
  }

  char text[INPUT_EVENT_MAX];
  if (kind == RADIO_CYCLE_BAND) {
    snprintf(text, sizeof(text), "%s", bandName(now.settings.band));
  } else if (kind == RADIO_CYCLE_TUNE_MODE) {
    snprintf(text, sizeof(text), "%s", tuneModeName(now.settings.tuneMode));
  } else if (kind == RADIO_TOGGLE_MUTE) {
    snprintf(text, sizeof(text), "%s",
             now.settings.muted ? "muted" : "unmuted");
  } else if (now.settings.bandwidthKHz == 0) {
    snprintf(text, sizeof(text), "BW automatic");
  } else {
    snprintf(text, sizeof(text), "BW %u kHz",
             (unsigned)now.settings.bandwidthKHz);
  }
  note(text);
}

/* ------------------------------------------- which screen a control goes to */

/*
 * The screen on top, the one a turn, a press or a key goes to. The menu
 * closes every other screen as it opens, and the bandwidth page closes the
 * RDS screen, so at most one of those three is up. The bandwidth page and the
 * RDS screen can each sit over DX mode, a stack one deep, which `dxUnder`
 * says.
 */
typedef enum {
  TOP_RADIO = 0,
  TOP_MENU,
  TOP_BW,
  TOP_RDS,
  TOP_DX,
  TOP_TOUCH_CAL,
  TOP_KEYPAD,
  TOP_SCOPE,
  TOP_COUNT
} InputTop;

/* When the menu last shut under a gesture, for inputAfterMenuQuiet. */
static bool sMenuShut = false;
static uint32_t sMenuShutMs = 0;

/* Called after a gesture the menu was on top for: if it shut the menu, the
 * back gestures go quiet for a moment. */
static void notedMenuShut(bool wasMenu) {
  if (wasMenu && !menuTaskIsOpen()) {
    sMenuShut = true;
    sMenuShutMs = millis();
  }
}

/* A back gesture that arrives just after the menu shut is one too many. */
static bool backTooSoon(bool wasMenu) {
  if (wasMenu || !inputAfterMenuQuiet(sMenuShut, sMenuShutMs, millis())) {
    return false;
  }
  note("back, just after the menu shut: ignored");
  return true;
}

static InputTop inputTop(bool *dxUnder) {
  const bool dx = screenTaskDxIsOpen();
  *dxUnder = false;
  if (menuTaskIsOpen()) {
    return TOP_MENU;
  }
  if (screenTaskTouchCalIsOpen()) {
    return TOP_TOUCH_CAL;
  }
  if (screenTaskKeypadIsOpen()) {
    return TOP_KEYPAD;
  }
  if (screenTaskScopeIsOpen()) {
    return TOP_SCOPE;
  }
  if (screenTaskBwIsOpen()) {
    *dxUnder = dx;
    return TOP_BW;
  }
  if (screenTaskRdsIsOpen()) {
    *dxUnder = dx;
    return TOP_RDS;
  }
  return dx ? TOP_DX : TOP_RADIO;
}

static void enterTyped(void);

/* ---------------------------------------------------------------- turns */

/* The dial, or the stored list in memory mode. */
static void radioTurn(int32_t clicks, uint32_t nowMs, bool) {
  /* One acceleration decision for the batch, then multiplied by how many
   * clicks were in it.
   *
   * Not one call per click. They all carry the same timestamp, because the
   * interrupt counts clicks and does not time them, so every call after the
   * first would see no gap at all and read as a full speed spin. A slow turn
   * that happened while the loop was busy elsewhere would then jump the dial
   * by twenty five steps.
   *
   * Asked even in memory mode, where it is not used, so the speed of the
   * turn is still being followed if the mode changes mid spin. If the radio
   * cannot be read this moment the dial behaviour is kept, which is what
   * every other mode does. */
  const uint8_t factor = accelerationSteps(&sAcceleration, nowMs);
  RadioSnapshot now;
  const bool list = radioGetSnapshot(&now) &&
                    radioKnobMode(&now.settings) == TUNE_MODE_MEMORY;
  const int32_t steps = inputKnobSteps(clicks, factor, list);

  RadioCommand cmd = {};
  cmd.kind = RADIO_STEP;
  cmd.steps = (int16_t)steps;
  radioPost(&cmd);
}

/*
 * The knob belongs to the menu while it is up, and it gets clicks rather than
 * the accelerated steps of the dial. Acceleration exists so that a fast turn
 * crosses a band quickly; in a list of five rows the same turn would skip past
 * everything and land somewhere nobody chose. One click, one row, however
 * hard it is spun.
 */
static void menuTurn(int32_t clicks, uint32_t, bool) {
  menuTaskTurn(clicks);
}

/* The bandwidth page: a tile a click. */
static void bwTurn(int32_t clicks, uint32_t, bool) {
  screenTaskBwTurn(clicks);
}

/*
 * The knob belongs to the RDS screen too, the same reasoning as the menu: four
 * pages is a list, and a page is somewhere to land rather than a distance to
 * cross. The dial does not tune underneath it, because a knob cannot both tune
 * and page at once.
 */
static void rdsTurn(int32_t clicks, uint32_t, bool) {
  screenTaskRdsPage(clicks);
}

/* The Catches page is a list, so the knob moves its cursor, a click a row,
 * and on the Scope page a channel. On the DX page it tunes, and on the
 * Scanner page only once the scan is stopped. */
static void dxTurn(int32_t clicks, uint32_t nowMs, bool dxUnder) {
  const uint8_t page = screenTaskDxPage();
  if (page == SCREEN_DX_PAGE_CATCHES || page == SCREEN_DX_PAGE_SCOPE) {
    screenTaskDxTurn(clicks);
    return;
  }
  if (scannerRefusesTuning(true)) {
    return;
  }
  radioTurn(clicks, nowMs, dxUnder);
}

/* ---------------------------------------------------- the knob's press */

/*
 * The radio screen. The press opens the menu, the way to every setting and
 * every screen with the knob alone, and the hold logs the station, as a hold of
 * ENTER does. The volume knob does what a mute would, and the squelch is set in
 * the menu.
 */
static void radioPress(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    menuTaskOpen();
    note("menu");
  } else if (event == BUTTON_LONG) {
    writeLogEntry(millis());
  }
}

/* In the menu the press is select and the hold is back, at every level. */
static void menuPress(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    menuTaskPress();
  } else if (event == BUTTON_LONG) {
    menuTaskBack();
  }
}

/* The press picks the tile under the cursor, and the page stays up so the
 * widths can be compared. Held, the way out, as the hold means back in the
 * menu. */
static void bwPress(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    screenTaskBwPick();
    note("bandwidth pick");
  } else if (event == BUTTON_LONG) {
    screenTaskBwClose();
    note("bandwidth page closed");
  }
}

/* Held, the way back: to the radio screen, or over DX mode to the DX page.
 * The short press does nothing. Over DX mode it would be the DX page's, and
 * that page is not on show to say what it did. */
static void rdsPress(ButtonEvent event, bool) {
  if (event != BUTTON_LONG) {
    return;
  }
  screenTaskRdsClose();
  note("RDS closed");
}

/* What a hold on a Catches row did, for the status note. The page says it
 * itself. */
static void noteDxLog(DxWriteResult r) {
  note(r == DX_WRITE_DONE          ? "DX catch logged"
       : r == DX_WRITE_NOTHING_NEW ? "DX catch already logged"
       : r == DX_WRITE_IN_LOG      ? "DX catch already in the log"
                                   : "DX catch not logged");
}

/*
 * The knob does the page's own job, and BAND moves between pages. On the DX
 * page the press opens the RDS screen over it, and the hold leaves DX mode. On
 * the Scope page the press sweeps and the hold tunes to the cursor. On the
 * Scanner page the press starts a scan, or goes on after a stop, and the hold
 * leaves DX mode. On the Catches page the press tunes to the catch under the
 * cursor and the hold logs it; DX mode is left from those two with MODE or DX.
 */
static void dxPress(ButtonEvent event, bool) {
  const uint8_t page = screenTaskDxPage();
  if (page == SCREEN_DX_PAGE_DX) {
    if (event == BUTTON_SHORT) {
      screenTaskRdsToggle();
      note("RDS over DX");
    } else if (event == BUTTON_LONG) {
      screenTaskDxClose();
      note("DX closed");
    }
    return;
  }
  if (page == SCREEN_DX_PAGE_SCOPE) {
    /* The press sweeps, or ends the sweep running, and the hold tunes to the
     * cursor's channel. */
    if (event == BUTTON_SHORT) {
      if (dxTaskSweepRunning()) {
        dxTaskSweepStop();
        note("DX sweep stopped");
        return;
      }
      const DxSweepStart r = dxTaskSweep();
      note(r == DX_SWEEP_STARTED ? "DX sweep running" : "DX sweep not started");
      if (r != DX_SWEEP_STARTED) {
        screenTaskLogConfirm(dxTaskSweepText(r));
      }
    } else if (event == BUTTON_LONG) {
      uint32_t khz = 0;
      if (screenTaskDxScopeCursorKHz(&khz)) {
        RadioCommand tune = {};
        tune.kind = RADIO_TUNE;
        tune.freqKHz = khz;
        radioPost(&tune);
        note("DX tune to the scope cursor");
      }
    }
    return;
  }
  if (page == SCREEN_DX_PAGE_SCAN) {
    if (event == BUTTON_SHORT) {
      const DxScanPress r = dxTaskScanPress();
      note(r == DX_SCAN_PRESS_RUNNING ? "DX scan running"
                                      : "DX scan not running");
      if (r != DX_SCAN_PRESS_RUNNING) {
        screenTaskLogConfirm(dxTaskPressText(r));
      }
    } else if (event == BUTTON_LONG) {
      screenTaskDxClose();
      note("DX closed");
    }
    return;
  }
  DxCatch k;
  if (!screenTaskDxCursorCatch(&k)) {
    /* No catch to tune or log: the hold leaves DX mode, as on the DX page
     * and the Scanner, so the knob alone is never stuck here. */
    if (event == BUTTON_LONG) {
      screenTaskDxClose();
      note("DX closed");
    }
    return;
  }
  if (event == BUTTON_SHORT) {
    RadioCommand tune = {};
    tune.kind = RADIO_TUNE;
    tune.freqKHz = k.khz;
    radioPost(&tune);
    note("DX tune to a catch");
  } else if (event == BUTTON_LONG) {
    noteDxLog(screenTaskDxLogCursor());
  }
}

/* ------------------------------------------------------------------ BAND */

/* The press is the next band. Held, the RDS screen, and the same press still
 * closes it. The dial does not tune underneath it: the screen has four pages
 * and the knob moves between them instead, MODE being the usual way out. */
static void radioBand(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    cycleAndNote(RADIO_CYCLE_BAND);
  } else if (event == BUTTON_LONG) {
    screenTaskRdsToggle();
  }
}

/* The RDS screen is a view of one station, and a short press of this key is a
 * change of band. Changing band under it would leave the screen describing a
 * station the radio is no longer on until the next redraw. Over DX mode it
 * would move the DX page, which is not on show. The long press closes it. */
static void rdsBand(ButtonEvent event, bool) {
  if (event == BUTTON_LONG) {
    screenTaskRdsToggle();
  }
}

/* DX mode is FM, and BAND is its next page key on every page. Held, it opens
 * the RDS screen over DX mode, which keeps its width, and held again it goes
 * back to the DX page. */
static void dxBand(ButtonEvent event, bool) {
  if (event == BUTTON_LONG) {
    screenTaskRdsToggle();
  } else if (event == BUTTON_SHORT) {
    screenTaskDxNextPage();
  }
}

/* -------------------------------------------------------------------- BW */

/* The bandwidth page: every width the band takes, and on FM the iMS and
 * equaliser switches, which the FM SETUP menu also has. Over the DX page it
 * offers DX mode's widths. */
static void openBandwidthPage(void) {
  note(screenTaskBwOpen() ? "bandwidth page"
                          : "bandwidth page, the panel is busy");
}

/* DX mode's own fixed width, which is not the radio's and is not saved; the
 * radio's own comes back on the way out. Only on the DX page, which shows
 * it: the knob and keys do each page's own job. */
static void cycleDxWidth(void) {
  if (screenTaskDxPage() != SCREEN_DX_PAGE_DX) {
    return;
  }
  char text[24];
  snprintf(text, sizeof(text), "DX BW %u kHz",
           (unsigned)screenTaskDxCycleWidth());
  note(text);
}

static void radioBandwidth(ButtonEvent event, bool) {
  if (event == BUTTON_LONG) {
    openBandwidthPage();
  } else if (event == BUTTON_SHORT) {
    cycleAndNote(RADIO_CYCLE_BANDWIDTH);
  }
}

/* The tap that leaves the page; the hold that opened it does nothing more
 * while it is up. */
static void bwBandwidth(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    screenTaskBwClose();
    note("bandwidth page closed");
  }
}

/* Over the radio screen the tap is the radio's next width; over DX mode it is
 * DX mode's, as on the DX page. */
static void rdsBandwidth(ButtonEvent event, bool dxUnder) {
  if (event == BUTTON_LONG) {
    openBandwidthPage();
  } else if (event == BUTTON_SHORT) {
    if (dxUnder) {
      cycleDxWidth();
    } else {
      cycleAndNote(RADIO_CYCLE_BANDWIDTH);
    }
  }
}

static void dxBandwidth(ButtonEvent event, bool) {
  if (event == BUTTON_LONG) {
    openBandwidthPage();
  } else if (event == BUTTON_SHORT) {
    cycleDxWidth();
  }
}

/* ------------------------------------------------------------------ MODE */

/* The tap is the tuning mode; the hold opens the menu, which a press of the
 * knob also opens. The same gesture closes it, so the way out is the way in
 * and nobody has to remember a second button. */
static void radioMode(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    cycleAndNote(RADIO_CYCLE_TUNE_MODE);
  } else if (event == BUTTON_LONG) {
    menuTaskOpen();
  }
}

/*
 * One level back: an edit is undone, a group is left, and from the group list
 * the menu closes. A tap, because opening has to be a long press, a short one
 * being the tuning mode, and the way back should be the quickest gesture on the
 * radio. It also stops this key changing the tuning mode behind a screen that
 * cannot show it. The hold closes the menu.
 */
static void menuMode(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    menuTaskBack();
  } else if (event == BUTTON_LONG) {
    menuTaskClose();
  }
}

/* Out of the bandwidth page, the RDS screen and DX mode, the same tap that
 * leaves the menu. On the RDS screen the long press of BAND that opened it
 * still closes it too, so a person who only remembers the one gesture is not
 * left stuck on it. The hold opens the menu from any of them. */
static void bwMode(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    screenTaskBwClose();
  } else if (event == BUTTON_LONG) {
    menuTaskOpen();
  }
}

static void rdsMode(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    screenTaskRdsClose();
  } else if (event == BUTTON_LONG) {
    menuTaskOpen();
  }
}

static void dxMode(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    screenTaskDxClose();
  } else if (event == BUTTON_LONG) {
    menuTaskOpen();
  }
}

/* ----------------------------------------------------------------- ENTER */

/* ENTER picks in the menu as the knob's press does, and a hold goes back as
 * the knob's hold does. */
static void menuEnter(ButtonEvent event, uint32_t, bool) {
  if (event == BUTTON_SHORT) {
    menuTaskPress();
    note("menu pick");
  } else {
    menuTaskBack();
    note("menu back");
  }
}

/* ENTER picks on the bandwidth page as the knob's press does; a hold does
 * nothing there. */
static void bwEnter(ButtonEvent event, uint32_t, bool) {
  if (event == BUTTON_SHORT) {
    screenTaskBwPick();
    note("bandwidth pick");
  }
}

/* Everywhere else the tap takes a typed number and the hold logs the
 * station. A hold is a different action from committing what was typed, the
 * same as any other control here that is not a digit or ENTER itself: it
 * cancels a number in progress rather than acting alongside it. */
static void dialEnter(ButtonEvent event, uint32_t nowMs, bool) {
  if (event == BUTTON_SHORT) {
    enterTyped();
    return;
  }
  if (typingInProgress(nowMs)) {
    cancelTyping();
  }
  writeLogEntry(nowMs);
}

/*
 * ENTER on the RDS screen and the DX pages: a press moves to the next page,
 * round to the first after the last, unless a frequency is being typed, which
 * a press tunes there as anywhere. The hold logs the station, as on the radio
 * screen.
 */
static void rdsEnter(ButtonEvent event, uint32_t nowMs, bool dxUnder) {
  if (event == BUTTON_SHORT && !typingInProgress(nowMs)) {
    screenTaskRdsPage(1);
    note("RDS page by ENTER");
    return;
  }
  dialEnter(event, nowMs, dxUnder);
}

static void dxEnter(ButtonEvent event, uint32_t nowMs, bool dxUnder) {
  if (event == BUTTON_SHORT && !typingInProgress(nowMs)) {
    screenTaskDxNextPage();
    note("DX page by ENTER");
    return;
  }
  dialEnter(event, nowMs, dxUnder);
}

/* --------------------------------------------------------------- keypad */

/* A digit is a frequency and the DX key a screen, and neither means anything
 * while the menu is up, but for the digits of the Web PIN while it is being
 * set. Left live they would move the radio behind a screen that cannot show
 * it. */
static void menuKey(int8_t key, uint32_t, bool) {
  if (key >= 0 && key <= 9 && menuTaskDigit((uint8_t)key)) {
    note("digit of the PIN");
    return;
  }
  note("key ignored in the menu");
}

/* A digit would type a frequency behind a page that is not showing one, and
 * the DX key would change what the page is over. */
static void bwKey(int8_t, uint32_t, bool) {
  note("key ignored on the bandwidth page");
}

/* A digit is typed, and DX opens or closes DX mode. `dxShown` is whether DX
 * mode is up, on top or under the RDS screen. */
static void dialKey(int8_t key, uint32_t nowMs, bool dxShown) {
  if (key == KEYPAD_DX) {
    if (typingInProgress(nowMs)) {
      /* Cancels the entry rather than also opening DX mode, the same as
       * every other key here that is not a digit or ENTER. */
      cancelTyping();
      return;
    }
    /* DX mode, and the same key leaves it. FM only: every page of it is
     * about an FM station. */
    if (dxShown) {
      screenTaskDxClose();
      note("DX closed");
      return;
    }
    switch (screenTaskDxOpen()) {
      case SCREEN_DX_OPEN:
        note("DX");
        break;
      case SCREEN_DX_PANEL_BUSY:
        note("DX, the panel is busy");
        break;
      case SCREEN_DX_RADIO_BUSY:
        note("DX, the radio is busy");
        screenTaskLogConfirm(txt(STR_MENU_NOTE_RADIO_BUSY));
        break;
      case SCREEN_DX_NOT_FM:
      default:
        note("DX mode is FM only");
        screenTaskLogConfirm(txt(STR_MENU_NOTE_SWITCH_TO_FM));
        break;
    }
    return;
  }
  if (scannerRefusesTuning(dxShown)) {
    note("digit refused on the Scanner");
    return;
  }
  if (sTypedLen >= INPUT_DIGITS_MAX) {
    /* Past what any frequency needs. Ignore the digit rather than losing the
     * front of the number, which is the part that says which band it is.
     *
     * The timeout is deliberately not refreshed here. A digit that was thrown
     * away must not keep a full and unusable buffer alive. */
    note("too many digits");
    return;
  }
  /* Only a digit that was actually kept restarts the clock. */
  sTypedMs = nowMs;
  sTyped[sTypedLen++] = (char)('0' + key);
  sTyped[sTypedLen] = '\0';
  memcpy(sStatus.typed, sTyped, sizeof(sTyped));
  char text[INPUT_EVENT_MAX];
  snprintf(text, sizeof(text), "key %d, typed %s", (int)key, sTyped);
  note(text);
}

static void radioKey(int8_t key, uint32_t nowMs, bool) {
  dialKey(key, nowMs, false);
}

static void rdsKey(int8_t key, uint32_t nowMs, bool dxUnder) {
  dialKey(key, nowMs, dxUnder);
}

static void dxKey(int8_t key, uint32_t nowMs, bool) {
  dialKey(key, nowMs, true);
}

/* ------------------------------------------------- the frequency keypad */

/* Its OK: the keypad goes first, so the radio screen, or the menu's choice
 * of bands, can have the panel, then the number is tuned as ENTER tunes it. */
static void keypadOk(void) {
  screenTaskKeypadClose();
  enterTyped();
}

static void keypadCancel(void) {
  cancelTyping();
  screenTaskKeypadClose();
}

/* The last digit typed taken back, which keeps the number alive as a new
 * digit would. */
static void typedBackspace(uint32_t nowMs) {
  if (sTypedLen == 0) {
    return;
  }
  sTypedMs = nowMs;
  sTyped[--sTypedLen] = '\0';
  memcpy(sStatus.typed, sTyped, sizeof(sTyped));
  note("typed digit taken back");
}

/* On the keypad the keys type, ENTER and the knob's press are OK, MODE is
 * Cancel and its hold the menu; the rest do nothing there. */
static void keypadTurn(int32_t, uint32_t, bool) {}

static void keypadPress(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    keypadOk();
  }
}

/* MODE held drops the number as Cancel does, so a half typed one does not
 * sit under the menu and eat its first turn. */
static void keypadMode(ButtonEvent event, bool) {
  keypadCancel();
  if (event == BUTTON_LONG) {
    menuTaskOpen();
  }
}

static void keypadEnter(ButtonEvent event, uint32_t, bool) {
  if (event == BUTTON_SHORT) {
    keypadOk();
  }
}

static void keypadKey(int8_t key, uint32_t nowMs, bool) {
  if (key == KEYPAD_DX) {
    keypadCancel();
    return;
  }
  dialKey(key, nowMs, false);
  /* A digit refused at the length limit still keeps the number, so it and
   * the keypad run out together. */
  if (sTypedLen > 0) {
    sTypedMs = nowMs;
  }
  screenTaskKeypadKeyed();
}

/* -------------------------------------------- touch calibration screen */

/* While the touch calibration screen is up, the knob answers it, as its
 * foot line says, and any other control leaves it, the old calibration
 * kept. */
static void calTurn(int32_t, uint32_t, bool) {
  screenTaskTouchCalKnob(false);
}

static void calPress(ButtonEvent, bool) {
  screenTaskTouchCalKnob(true);
}

static void calButton(ButtonEvent, bool) {
  screenTaskTouchCalClose();
}

static void calEnter(ButtonEvent, uint32_t, bool) {
  screenTaskTouchCalClose();
}

static void calKey(int8_t, uint32_t, bool) {
  screenTaskTouchCalClose();
}

#if FEATURE_TOUCH
/* --------------------------------------------------- the radio screen, by touch */

/* A drag on the scale under way: the band and the frequency under the
 * middle when it began, and the last one sent. */
static bool sDialDragging = false;
static BandId sDialBand = BAND_FM;
static uint32_t sDialFromKHz = 0;
static uint32_t sDialSentKHz = 0;

/*
 * The scale follows the finger, `dxPx` from where the drag began, and the
 * frequency under its middle is tuned as it goes, on the band's own channel
 * grid, on the band it began on: a band changed under it, by a key, ends
 * it. By RADIO_TUNE_IN_BAND, not a step: in Auto mode a step seeks and in
 * Presets mode it walks the presets. At most one command a poll, since a drag
 * gives at most one sample a poll.
 */
static void dialDrag(TouchGestureEvent event, int32_t dxPx) {
  RadioSnapshot now;
  BandPlanConfig plan;
  uint32_t low = 0;
  uint32_t high = 0;
  const bool got = radioGetSnapshot(&now);
  if (got && !sDialDragging) {
    sDialDragging = true;
    sDialBand = now.settings.band;
    sDialFromKHz = now.settings.freqKHz;
    sDialSentKHz = sDialFromKHz;
  }
  if (got && now.settings.band == sDialBand && radioTaskPlan(&plan) &&
      bandLimits(now.settings.band, &plan, &low, &high)) {
    const bool fm = bandModulation(now.settings.band) == MODULATION_FM;
    const uint32_t khz = bandNearestChannel(
        now.settings.band, &plan,
        scaleDragKHz(low, high, sDialFromKHz, fm, dxPx), now.settings.stepKHz);
    if (khz != sDialSentKHz) {
      RadioCommand cmd = {};
      cmd.kind = RADIO_TUNE_IN_BAND;
      cmd.freqKHz = khz;
      if (radioPost(&cmd)) {
        sDialSentKHz = khz;
      }
    }
  }
  if (event == TOUCH_DRAG_END) {
    sDialDragging = false;
  }
}

/*
 * The radio screen by touch. Each zone does what the key or the knob it
 * stands for does there, through the same call: the menu symbol at the top
 * left is the knob's press, the band name a tap of BAND, the frequency
 * panel's upper part, with the station name, a hold of BAND for the RDS
 * screen, which is FM only, its lower part, with the frequency, the keypad
 * to type one, and either part held the knob's hold that logs the station;
 * the mode tile a tap of MODE and the bandwidth tile a hold of BW. The SQL
 * tile opens Squelch Mode rather than stepping it, since Manual hands the
 * volume knob to the squelch and a tap that did that unseen could silence
 * the radio.
 * The volume tile mutes, which no key does, and no key opens the keypad,
 * since the keys type a number themselves.
 */
static void radioTouch(TouchGestureEvent event, int zone, TouchPoint start,
                       TouchPoint last, bool dxUnder) {
  if (zone == RADIO_ZONE_SCALE) {
    /* A drag tunes; a tap opens the band scope, the scale's own picture of
     * the band. */
    if (event == TOUCH_DRAG || event == TOUCH_DRAG_END) {
      dialDrag(event, last.x - start.x);
    } else if (event == TOUCH_TAP) {
      note(screenTaskScopeOpen() ? "band scope" : "band scope not opened");
    }
    return;
  }
  if ((zone == RADIO_ZONE_NAME || zone == RADIO_ZONE_FREQ) &&
      event == TOUCH_HOLD) {
    radioPress(BUTTON_LONG, dxUnder);
    return;
  }
  if (event != TOUCH_TAP) {
    return;
  }
  switch (zone) {
    case RADIO_ZONE_BAND:
      radioBand(BUTTON_SHORT, dxUnder);
      break;
    case RADIO_ZONE_MENU:
      radioPress(BUTTON_SHORT, dxUnder);
      break;
    case RADIO_ZONE_NAME:
      radioBand(BUTTON_LONG, dxUnder);
      break;
    case RADIO_ZONE_FREQ:
      note(screenTaskKeypadOpen() ? "keypad" : "keypad, the panel is busy");
      break;
    case RADIO_ZONE_MODE:
      radioMode(BUTTON_SHORT, dxUnder);
      break;
    case RADIO_ZONE_SQL:
      if (menuTaskOpenSquelchMode()) {
        note("squelch mode");
      }
      break;
    case RADIO_ZONE_BW:
      radioBandwidth(BUTTON_LONG, dxUnder);
      break;
    case RADIO_ZONE_VOL:
      cycleAndNote(RADIO_TOGGLE_MUTE);
      break;
    default:
      break;
  }
}

/*
 * DX mode by touch. On every page a swipe left or right is the next page or
 * the one before, the page position the next page as BAND, and the title
 * leaves DX mode as MODE does. On the DX page the station panel and the PI tile
 * open the RDS screen over it, as the knob's press, and the readings the
 * bandwidth page with DX mode's widths, as BW held. On the Scope page the
 * chart moves the cursor and the foot tile tunes to it, as the knob's hold;
 * with Touch On its foot row also has a button each way for the cursor and
 * Sweep, the knob's click and press.
 * On the Scanner page the scan panel starts a scan or goes on with one, as
 * the knob's press; while a scan runs any touch only stops it. On the Catches
 * page a row tapped is tuned and held is logged, and a swipe up or down
 * moves a screen of rows.
 */
static int dxZones(TouchZone *out, int max) {
  const uint8_t page = screenTaskDxPage();
  if (page == SCREEN_DX_PAGE_SCOPE) {
    return screenScopeZones(out, max);
  }
  if (page == SCREEN_DX_PAGE_SCAN) {
    return screenScanZones(out, max);
  }
  if (page == SCREEN_DX_PAGE_CATCHES) {
    return screenCatchesZones(out, max);
  }
  return screenDxZones(out, max, page == SCREEN_DX_PAGE_DX);
}

static void dxTouch(TouchGestureEvent event, int zone, TouchPoint start,
                    TouchPoint last, bool dxUnder) {
  if (zone == DX_ZONE_CHART) {
    /* The Scope's cursor follows the finger; held, it tunes there, as the
     * knob's hold tunes to the cursor. */
    const int32_t channel =
        screenScopeChannelAt(event == TOUCH_TAP ? start.x : last.x);
    if (channel >= 0) {
      screenTaskDxScopeSet((uint16_t)channel);
    }
    if (event == TOUCH_HOLD) {
      dxPress(BUTTON_LONG, dxUnder);
    }
    return;
  }
  if (event == TOUCH_SWIPE_LEFT || event == TOUCH_SWIPE_RIGHT) {
    screenTaskDxStepPage(event == TOUCH_SWIPE_LEFT ? 1 : -1);
    return;
  }
  if (event == TOUCH_SWIPE_UP || event == TOUCH_SWIPE_DOWN) {
    /* The Catches list a screen on or back; its window is a screen of rows
     * from the cursor's own. Nothing elsewhere: on Scope a turn moves the
     * cursor. */
    if (screenTaskDxPage() == SCREEN_DX_PAGE_CATCHES) {
      screenTaskDxTurn(event == TOUCH_SWIPE_UP ? SCREEN_CATCH_ROWS
                                               : -SCREEN_CATCH_ROWS);
    }
    return;
  }
  if (zone >= DX_ZONE_ROW && zone < DX_ZONE_LEFT &&
      (event == TOUCH_TAP || event == TOUCH_HOLD)) {
    /* A catch: turned to, then tuned by a tap or logged by a hold, as the
     * knob's press and hold. */
    const int cursor = screenCatchesCursorSlot();
    if (cursor >= 0) {
      screenTaskDxTurn(zone - DX_ZONE_ROW - cursor);
      dxPress(event == TOUCH_TAP ? BUTTON_SHORT : BUTTON_LONG, dxUnder);
    }
    return;
  }
  if (event != TOUCH_TAP) {
    return;
  }
  switch (zone) {
    case DX_ZONE_BACK:
      dxMode(BUTTON_SHORT, dxUnder);
      break;
    case DX_ZONE_NEXT:
      dxBand(BUTTON_SHORT, dxUnder);
      break;
    case DX_ZONE_PANEL:
    case DX_ZONE_PI:
      /* The knob's press: RDS over DX on the DX page, and on the Scanner
       * page a scan started or gone on with. */
      dxPress(BUTTON_SHORT, dxUnder);
      break;
    case DX_ZONE_READINGS:
      openBandwidthPage();
      break;
    case DX_ZONE_FOOT:
      dxPress(BUTTON_LONG, dxUnder);
      break;
    case DX_ZONE_LEFT:
    case DX_ZONE_RIGHT:
      /* A channel a tap, as a click of the knob. */
      screenTaskDxTurn(zone == DX_ZONE_LEFT ? -1 : 1);
      break;
    case DX_ZONE_SWEEP:
      /* The knob's press on Scope: a sweep, or the one running stopped. */
      dxPress(BUTTON_SHORT, dxUnder);
      break;
    default:
      break;
  }
}

/*
 * The RDS screen by touch: a swipe to the left is the next page and to the
 * right the one before, as the knob turns them; a tap on the page position
 * is the next page; and a tap on the title closes it as MODE does, back to
 * the DX page over DX mode.
 */
static void rdsTouch(TouchGestureEvent event, int zone, TouchPoint, TouchPoint,
                     bool dxUnder) {
  if (event == TOUCH_SWIPE_LEFT || event == TOUCH_SWIPE_RIGHT) {
    screenTaskRdsPage(event == TOUCH_SWIPE_LEFT ? 1 : -1);
  } else if (event == TOUCH_TAP && zone == RDS_ZONE_NEXT) {
    screenTaskRdsPage(1);
  } else if (event == TOUCH_TAP && zone == RDS_ZONE_BACK) {
    rdsMode(BUTTON_SHORT, dxUnder);
  }
}

/*
 * The keypad by touch: each key is the call the keys and ENTER make, the
 * header is Cancel.
 */
static void keypadTouch(TouchGestureEvent event, int zone, TouchPoint,
                        TouchPoint, bool) {
  if (event != TOUCH_TAP) {
    return;
  }
  const int key = zone - KEYPAD_ZONE_KEY;
  const uint32_t nowMs = millis();
  if (zone == KEYPAD_ZONE_BACK || key == SCREEN_KEYPAD_CANCEL) {
    keypadCancel();
  } else if (key == SCREEN_KEYPAD_OK) {
    keypadOk();
  } else if (key == SCREEN_KEYPAD_BACKSPACE) {
    typedBackspace(nowMs);
    screenTaskKeypadKeyed();
  } else if (key >= 0 && key <= 9) {
    keypadKey((int8_t)key, nowMs, false);
  }
}

/*
 * The bandwidth page by touch: a tile tapped is turned to and picked, as the
 * knob and its press would, and the page stays up so widths can be compared
 * by ear; the header closes it as MODE does.
 */
static void bwTouch(TouchGestureEvent event, int zone, TouchPoint, TouchPoint,
                    bool dxUnder) {
  if (event != TOUCH_TAP) {
    return;
  }
  if (zone == BW_ZONE_BACK) {
    bwMode(BUTTON_SHORT, dxUnder);
  } else {
    screenTaskBwTap((uint8_t)(zone - BW_ZONE_TILE));
  }
}

/*
 * The menu by touch: a row tapped is turned to and pressed, as the knob
 * would, the header is Back as MODE is, and a swipe up or down pages the
 * list. On a value with a bar, a finger on the bar sets it and a tap on the
 * value panel saves it, as the knob's press does; minus and plus are a click
 * of the knob each way, and Save its press; a finger resting on minus or
 * plus steps again and again. On Restart Radio's question a tap on No or Yes
 * takes it. On the Web PIN its keys type a digit as a key does, backspace
 * goes back one, and Cancel is Back. A hold on the others acts once, as a
 * tap does.
 */
static void menuTouch(TouchGestureEvent event, int zone, TouchPoint start,
                      TouchPoint last, bool) {
  if (zone >= MENU_ZONE_PIN_KEY && zone < MENU_ZONE_PIN_KEY + SCREEN_PIN_KEYS) {
    if (event == TOUCH_TAP || event == TOUCH_HOLD) {
      const int key = zone - MENU_ZONE_PIN_KEY;
      if (key == SCREEN_PIN_CANCEL) {
        menuTaskBack();
      } else if (key == SCREEN_PIN_BACKSPACE) {
        menuTaskDigitBack();
      } else {
        menuTaskDigit((uint8_t)((key + 1) % 10));
      }
    }
    return;
  }
  if (zone == MENU_ZONE_BUTTON || zone == MENU_ZONE_BUTTON + 1) {
    if (event == TOUCH_TAP || event == TOUCH_HOLD) {
      menuTaskTapButton((uint8_t)(zone - MENU_ZONE_BUTTON));
    }
    return;
  }
  if (zone == MENU_ZONE_MINUS || zone == MENU_ZONE_SAVE ||
      zone == MENU_ZONE_PLUS) {
    /* A finger resting on minus or plus repeats; a hold sent over the API
     * steps once. */
    if (event == TOUCH_TAP || event == TOUCH_HOLD || event == TOUCH_REPEAT) {
      if (zone == MENU_ZONE_SAVE) {
        menuTaskPress();
      } else {
        menuTaskTurn(zone == MENU_ZONE_PLUS ? 1 : -1);
      }
    }
    return;
  }
  if (zone == MENU_ZONE_BAR) {
    /* The value follows the finger, as the knob would turn it there. A
     * zone that follows a drag gives no swipe; a finger resting to the hold
     * time sets the value under it, and the touch ends there. */
    menuTaskBarTo(screenMenuBarValue(event == TOUCH_TAP ? start.x : last.x));
    return;
  }
  if (event == TOUCH_SWIPE_UP || event == TOUCH_SWIPE_DOWN) {
    menuTaskPage(event == TOUCH_SWIPE_UP ? 1 : -1);
    return;
  }
  if (event != TOUCH_TAP) {
    return;
  }
  if (zone == MENU_ZONE_BACK) {
    menuTaskBack();
    return;
  }
  if (zone == MENU_ZONE_PANEL) {
    menuTaskPress();
    return;
  }
  const int cursor = screenMenuCursorSlot();
  if (cursor >= 0) {
    menuTaskTapRow(zone - MENU_ZONE_ROW - cursor);
  }
}
#endif

/* -------------------------------------------------------- the band scope */

/*
 * The band scope works as DX mode's Scope page does: the knob moves the
 * cursor a channel a click, its press, or ENTER, sweeps or ends the sweep
 * running, and its hold tunes to the cursor. BAND switches between the whole
 * band and the span round the dial, and MODE closes it, held the menu. By
 * touch the chart moves the cursor and a hold tunes there, the foot tile
 * tunes, the title closes it and the page position is BAND. The number keys
 * do nothing, since a typed number would not show.
 */
static void scopeTurn(int32_t clicks, uint32_t, bool) {
  screenTaskScopeTurn(clicks);
}

static void scopePress(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    ScopeView v;
    scopeTaskView(&v);
    if (v.running) {
      radioSweepCancel();
      note("scope sweep stopped");
    } else {
      note(screenTaskScopeSweep() == SCOPE_STARTED ? "scope sweep running"
                                                   : "scope sweep not started");
    }
    return;
  }
  uint32_t khz = 0;
  if (event == BUTTON_LONG && screenTaskScopeCursorKHz(&khz)) {
    RadioCommand tune = {};
    tune.kind = RADIO_TUNE;
    tune.freqKHz = khz;
    radioPost(&tune);
    note("tune to the scope cursor");
  }
}

static void scopeBand(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    screenTaskScopeSpanToggle();
    note("scope span");
  }
}

static void scopeMode(ButtonEvent event, bool) {
  if (event == BUTTON_SHORT) {
    screenTaskScopeClose();
  } else if (event == BUTTON_LONG) {
    menuTaskOpen();
  }
}

static void scopeEnter(ButtonEvent event, uint32_t, bool dxUnder) {
  scopePress(event, dxUnder);
}

static void scopeKey(int8_t, uint32_t, bool) {
  note("key ignored on the band scope");
}

#if FEATURE_TOUCH
static void scopeTouch(TouchGestureEvent event, int zone, TouchPoint start,
                       TouchPoint last, bool dxUnder) {
  if (zone == DX_ZONE_CHART) {
    const int32_t channel =
        screenScopeChannelAt(event == TOUCH_TAP ? start.x : last.x);
    if (channel >= 0) {
      screenTaskScopeSet((uint16_t)channel);
    }
    if (event == TOUCH_HOLD) {
      scopePress(BUTTON_LONG, dxUnder);
    }
    return;
  }
  if (event != TOUCH_TAP) {
    return;
  }
  switch (zone) {
    case DX_ZONE_BACK:
      scopeMode(BUTTON_SHORT, dxUnder);
      break;
    case DX_ZONE_NEXT:
      scopeBand(BUTTON_SHORT, dxUnder);
      break;
    case DX_ZONE_FOOT:
      scopePress(BUTTON_LONG, dxUnder);
      break;
    case DX_ZONE_LEFT:
    case DX_ZONE_RIGHT:
      screenTaskScopeTurn(zone == DX_ZONE_LEFT ? -1 : 1);
      break;
    case DX_ZONE_SWEEP:
      scopePress(BUTTON_SHORT, dxUnder);
      break;
    default:
      break;
  }
}
#endif

/* ------------------------------------------------------------ the table */

/* What each screen does with each control. A button's slot may be NULL, a
 * button the screen ignores; the knob, ENTER and the keypad are called
 * without a check, so every screen fills those. A new screen is a new row. */
typedef struct {
  void (*turn)(int32_t clicks, uint32_t nowMs, bool dxUnder);
  void (*press)(ButtonEvent event, bool dxUnder);
  void (*band)(ButtonEvent event, bool dxUnder);
  void (*bandwidth)(ButtonEvent event, bool dxUnder);
  void (*mode)(ButtonEvent event, bool dxUnder);
  void (*enter)(ButtonEvent event, uint32_t nowMs, bool dxUnder);
  void (*key)(int8_t key, uint32_t nowMs, bool dxUnder);
#if FEATURE_TOUCH
  /* A gesture in one of the screen's zones, `start` where it began and
   * `last` where it is now; NULL for a screen a touch does nothing on. */
  void (*touch)(TouchGestureEvent event, int zone, TouchPoint start,
                TouchPoint last, bool dxUnder);
  /* The screen's zones, the way screenRadioZones gives them, and their
   * names; NULL for a screen with none. */
  int (*zones)(TouchZone *out, int max);
  const char *(*zoneName)(int zone);
  /* The zone that follows a drag rather than taking a swipe, or 0. */
  int dragZone;
#endif
} ScreenInput;

static const ScreenInput kScreenInput[TOP_COUNT] = {
    /* TOP_RADIO */
    {radioTurn, radioPress, radioBand, radioBandwidth, radioMode, dialEnter,
     radioKey
#if FEATURE_TOUCH
     ,
     radioTouch, screenRadioZones, screenRadioZoneName, RADIO_ZONE_SCALE
#endif
    },
    /* TOP_MENU */
    {menuTurn, menuPress, NULL, NULL, menuMode, menuEnter, menuKey
#if FEATURE_TOUCH
     ,
     menuTouch, screenMenuZones, screenMenuZoneName, MENU_ZONE_BAR
#endif
    },
    /* TOP_BW */
    {bwTurn, bwPress, NULL, bwBandwidth, bwMode, bwEnter, bwKey
#if FEATURE_TOUCH
     ,
     bwTouch, screenBwZones, screenBwZoneName, 0
#endif
    },
    /* TOP_RDS */
    {rdsTurn, rdsPress, rdsBand, rdsBandwidth, rdsMode, rdsEnter, rdsKey
#if FEATURE_TOUCH
     ,
     rdsTouch, screenRdsZones, screenRdsZoneName, 0
#endif
    },
    /* TOP_DX */
    {dxTurn, dxPress, dxBand, dxBandwidth, dxMode, dxEnter, dxKey
#if FEATURE_TOUCH
     ,
     dxTouch, dxZones, screenDxZoneName, DX_ZONE_CHART
#endif
    },
    /* TOP_TOUCH_CAL */
    {calTurn, calPress, calButton, calButton, calButton, calEnter, calKey},
    /* TOP_KEYPAD */
    {keypadTurn, keypadPress, NULL, NULL, keypadMode, keypadEnter, keypadKey
#if FEATURE_TOUCH
     ,
     keypadTouch, screenKeypadZones, screenKeypadZoneName, 0
#endif
    },
    /* TOP_SCOPE */
    {scopeTurn, scopePress, scopeBand, NULL, scopeMode, scopeEnter, scopeKey
#if FEATURE_TOUCH
     ,
     scopeTouch, screenScopeZones, screenDxZoneName, DX_ZONE_CHART
#endif
    },
};

/* The row for the screen on top. */
static const ScreenInput *screenInput(bool *dxUnder) {
  return &kScreenInput[inputTop(dxUnder)];
}

/* A turn of the knob, from the knob itself or from POST /api/key. */
static void onTurn(int32_t clicks, uint32_t nowMs) {
  sActivity++;
  if (stopScanFirst()) {
    return;
  }

  /* Waking the panel is this turn's whole job when it arrives dark. See
   * panelIsDimmed.
   *
   * Every click `encoderTake` just drained goes with it, not one detent's
   * worth. A spin that lands in one batch is a spin nobody could read the
   * screen for, so acting on the later clicks is the same fault as acting on
   * the first. The batch is only what accumulated since the last poll, a
   * couple of milliseconds unless the loop was busy redrawing or serving a
   * page. */
  if (panelIsDimmed()) {
    return;
  }

  /*
   * A number in progress means this turn is a change of mind, the same
   * reasoning as the buttons. It cancels the entry and does not also step
   * the dial or move a menu row: a turn made to change what is typed and a
   * turn made to tune are two different intentions, and this one is the
   * first. The keypad is the one place a number is typed on purpose, so
   * there its own handlers decide.
   */
  if (typingInProgress(nowMs) && !screenTaskKeypadIsOpen()) {
    cancelTyping();
    return;
  }

  bool dxUnder = false;
  screenInput(&dxUnder)->turn(clicks, nowMs, dxUnder);
}

static void pollEncoder(uint32_t nowMs) {
  int32_t clicks = encoderTake();
  if (clicks == 0) {
    return;
  }
  sStatus.clicks += (uint32_t)(clicks < 0 ? -clicks : clicks);
  /* Turned with the knob held down is an ordinary turn. The press is then
   * spent, so letting go does not also open the menu and no hold fires
   * after it. Pressing the knob does not turn it on this radio: ten presses
   * held about a second each gave no click, so no click is set aside at the
   * start of a press. */
  Button *knob = &sButtons[PANEL_BUTTON_ENCODER];
  if (knob->level) {
    knob->handled = true;
  }
  onTurn(clicks, nowMs);
}

/* A press of a panel button, from the button itself or from POST /api/key.
 * `wasTyping` is whether a number was being typed before this press. */
static void onButton(PanelButton which, ButtonEvent event, bool wasTyping) {
  sActivity++;
  /* A long press only, and longer than a keypad tick so the two are told
   * apart by ear. A short press needs nothing: the band changes, the filter
   * changes, the sound stops, and the result is the feedback. A long press
   * has none of that, and the moment it fires is decided by a timer rather
   * than by letting go. */
  if (sBeepMode >= BEEP_EVERY_PRESS ||
      (sBeepMode >= BEEP_KEYS_AND_LONG && event == BUTTON_LONG)) {
    radioBeep(event == BUTTON_LONG ? BEEP_LONG_MS : BEEP_MS);
  }
  if (stopScanFirst()) {
    return;
  }

  /* Waking the panel is this press's whole job when it arrives dark,
   * short or long alike: a long press only fires once its hold completes,
   * which is the same pass of loop() the wake happens on, so this is
   * still true for it too. See panelIsDimmed.
   *
   * Checked before the note below, not after: a note naming the press
   * that never happened is exactly as untrue as carrying it out, and
   * `inp.lst` in the state document would read no differently for either
   * one. */
  if (panelIsDimmed()) {
    return;
  }

  /*
   * A number in progress means this press is a change of mind, not a
   * request to do the button's own job as well. Both from one press would
   * be acting on two intentions at once, so the button does nothing this
   * time beyond cancelling the entry, the same as any other key here that
   * is not a digit or ENTER. But for on the keypad, where the knob's press
   * is OK and MODE is Cancel.
   */
  if (wasTyping && !screenTaskKeypadIsOpen()) {
    cancelTyping();
    return;
  }

  char seen[INPUT_EVENT_MAX];
  snprintf(seen, sizeof(seen), "%s %s", panelButtonName(which),
           buttonEventName(event));
  note(seen);

  bool dxUnder = false;
  const InputTop top = inputTop(&dxUnder);
  const bool wasMenu = top == TOP_MENU;
  if (((which == PANEL_BUTTON_MODE && event == BUTTON_SHORT) ||
       (which == PANEL_BUTTON_ENCODER && event == BUTTON_LONG)) &&
      backTooSoon(wasMenu)) {
    return;
  }
  const ScreenInput *to = &kScreenInput[top];
  void (*act)(ButtonEvent, bool) = NULL;
  switch (which) {
    case PANEL_BUTTON_BAND:
      act = to->band;
      break;
    case PANEL_BUTTON_BW:
      act = to->bandwidth;
      break;
    case PANEL_BUTTON_MODE:
      act = to->mode;
      break;
    case PANEL_BUTTON_ENCODER:
      act = to->press;
      break;
    default:
      break;
  }
  if (act != NULL) {
    act(event, dxUnder);
  }
  notedMenuShut(wasMenu);
}

static void pollButtons(uint32_t nowMs) {
  /*
   * Read once, before any button below can act on it. Checking it fresh
   * inside the loop would let a first button that cancels the entry make it
   * read as gone for a second button firing in the same pass, so that
   * second button would dispatch for real on the very press meant only to
   * cancel. Two buttons firing in one pass needs a coincidence of timing
   * rather than a normal press, but the fix costs nothing.
   */
  const bool wasTyping = typingInProgress(nowMs);

  for (int i = 0; i < PANEL_BUTTON_COUNT; i++) {
    PanelButton which = (PanelButton)i;
    ButtonEvent event =
        buttonFeed(&sButtons[i], encoderButtonDown(which), nowMs);
    if (event == BUTTON_NONE) {
      continue;
    }
    sStatus.presses++;
    onButton(which, event, wasTyping);
  }
}

static void enterTyped(void) {
  if (sTypedLen == 0) {
    note("enter with nothing typed");
    return;
  }

  uint32_t typed = (uint32_t)strtoul(sTyped, NULL, 10);
  BandPlanConfig plan;
  RadioSnapshot now;
  BandId prefer = BAND_COUNT;
  if (radioGetSnapshot(&now)) {
    prefer = now.settings.band;
  }

  char text[INPUT_EVENT_MAX];
  uint32_t khz = 0;
  BandId band = BAND_COUNT;
  if (!radioTaskPlan(&plan) ||
      !bandFromTypedNumber(&plan, typed, prefer, &khz, &band)) {
    snprintf(text, sizeof(text), txt(STR_RADIO_FMT_NO_BAND), sTyped);
    note(text);
    /* Said on the panel too, or the digits would just vanish. */
    screenTaskLogConfirm(text);
    clearTyped();
    return;
  }

  /* A number the band in use does not hold, but more than one other band
   * does, is a choice for the person who typed it, not the radio. */
  if (band != prefer) {
    BandTypedReading readings[BAND_TYPED_READINGS_MAX];
    const uint8_t count =
        bandTypedReadings(&plan, typed, readings, BAND_TYPED_READINGS_MAX);
    if (count > 1 && menuTaskOpenChoice(sTyped, readings, count)) {
      snprintf(text, sizeof(text), "%s, a choice of %u bands", sTyped,
               (unsigned)count);
      note(text);
      clearTyped();
      return;
    }
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_TUNE;
  cmd.freqKHz = khz;
  if (!radioPostOk(&cmd, BUTTON_SETTLE_MS)) {
    snprintf(text, sizeof(text), "%s was not tuned", sTyped);
    note(text);
    clearTyped();
    return;
  }

  char freq[16];
  bandFormatFrequency(band, khz, freq, sizeof(freq));
  snprintf(text, sizeof(text), "%s %s", bandName(band), freq);
  note(text);
  clearTyped();
}

/*
 * Everything the radio knows right now, written down.
 *
 * Called at the moment ENTER's hold fires, never at the moment it was
 * pressed, the same as any other long press on this radio: the gesture is
 * not certain until the threshold is reached, and a snapshot taken any
 * earlier could describe whatever the dial was doing on the way there.
 */
static InputLogResult writeLogEntry(uint32_t nowMs) {
  RadioSnapshot now;
  if (!radioGetSnapshot(&now)) {
    note("log: radio busy, not written");
    screenTaskLogConfirm(txt(STR_COMMON_NOT_LOGGED));
    return INPUT_LOG_FAILED;
  }
  /* An entry would record a station nobody was listening to. */
  if (dxTaskDialWalking(&now)) {
    note("log: still tuning, not written");
    screenTaskLogConfirm(txt(STR_COMMON_STILL_TUNING));
    return INPUT_LOG_WALKING;
  }

  LogbookEntry e;
  memset(&e, 0, sizeof(e));
  uint32_t epoch = 0;
  if (ntpEpochUtc(&epoch)) {
    e.timeKnown = true;
    e.timeValue = epoch;
  } else {
    e.timeKnown = false;
    e.timeValue = nowMs;
  }
  e.band = (uint8_t)now.settings.band;
  e.freqKHz = now.settings.freqKHz;
  e.levelDbuVTenths = now.quality.levelDbuVTenths;
  e.usnTenths = now.quality.usnTenths;
  e.multipathTenths = now.quality.multipathTenths;
  e.coChannelTenths = now.quality.coChannelTenths;
  e.snrDb = now.quality.snrDb;
  e.stereo = now.quality.stereo;
  e.bandwidthKHz = now.quality.bandwidthKHz;
  const SeekReading reading = radioSeekReading(&now.quality, now.qualityValid);
  dxLogIdentity(&e, &now.rds, &reading);
  dxTaskAddRadioText(&e, &now);

  char freqText[16];
  bandFormatFrequency(now.settings.band, now.settings.freqKHz, freqText,
                      sizeof(freqText));

  const LogbookWrite w = logbookFsAppend(&e);
  if (w == LOGBOOK_NOT_WRITTEN) {
    note("log: not written, no storage");
    screenTaskLogConfirm(txt(STR_COMMON_NOT_LOGGED));
    return INPUT_LOG_FAILED;
  }
  /* A DX catch of this station is logged now too, so the auto log does not
   * write it a second time. */
  if (e.hasPi) {
    dxTaskNoteLogged(e.freqKHz, e.pi);
  }
  if (w == LOGBOOK_ALREADY_THERE) {
    note("log: already in the log, not written");
    screenTaskLogConfirm(txt(STR_COMMON_ALREADY_LOGGED));
    return INPUT_LOG_ALREADY;
  }

  char text[INPUT_EVENT_MAX];
  snprintf(text, sizeof(text), "logged %s %s", bandName(now.settings.band),
           freqText);
  note(text);
  screenTaskLogConfirmAt(e.band, e.freqKHz);
  return INPUT_LOG_WRITTEN;
}

InputLogResult inputWriteLogEntry(void) {
  return writeLogEntry(millis());
}

void inputNextBand(void) {
  sActivity++;
  cycleAndNote(RADIO_CYCLE_BAND);
}

/* ENTER's tap or hold, from the keypad or from POST /api/key. */
static void onEnter(ButtonEvent enterEvent, uint32_t nowMs) {
  sActivity++;
  if ((enterEvent == BUTTON_SHORT && sBeepMode >= BEEP_KEYS) ||
      (enterEvent == BUTTON_LONG && sBeepMode >= BEEP_KEYS_AND_LONG)) {
    radioBeep(enterEvent == BUTTON_LONG ? BEEP_LONG_MS : BEEP_MS);
  }
  /* Waking the panel is this gesture's whole job when it arrives dark.
   * See panelIsDimmed. */
  if (!stopScanFirst() && !panelIsDimmed()) {
    bool dxUnder = false;
    const InputTop top = inputTop(&dxUnder);
    const bool wasMenu = top == TOP_MENU;
    if (enterEvent == BUTTON_LONG && backTooSoon(wasMenu)) {
      return;
    }
    kScreenInput[top].enter(enterEvent, nowMs, dxUnder);
    notedMenuShut(wasMenu);
  }
}

/* A digit or the DX key, from the keypad or from POST /api/key. */
static void onKey(int8_t key, uint32_t nowMs) {
  sActivity++;
  if (sBeepMode >= BEEP_KEYS) {
    /* Every key, including the ones that go on to be refused. The beep says
     * the press was seen, which is the question a person is asking when they
     * press a key and nothing happens. */
    radioBeep(BEEP_MS);
  }
  if (stopScanFirst()) {
    return;
  }

  /* Waking the panel is this key's whole job when it arrives dark. See
   * panelIsDimmed. */
  if (panelIsDimmed()) {
    return;
  }

  bool dxUnder = false;
  screenInput(&dxUnder)->key(key, nowMs, dxUnder);
}

static void pollKeypad(uint32_t nowMs) {
  static uint32_t lastPollMs = 0;
  if ((uint32_t)(nowMs - lastPollMs) < KEYPAD_POLL_MS) {
    return;
  }
  lastPollMs = nowMs;

  /* A number left half typed is dropped, so the next person to press a key
   * is not silently continuing somebody else's. Before the check for the
   * keys' chip, since the touch keypad and the API type without it. */
  if (sTypedLen > 0 && (uint32_t)(nowMs - sTypedMs) >= sKeypadTimeoutMs) {
    note("typed number timed out");
    clearTyped();
    screenTaskKeypadClose();
  }
  if (!sStatus.keypadPresent) {
    return;
  }

  /* One read of the expander, giving both the key and the raw lines. Reading
   * it twice doubles the traffic on the tuner's bus and lets the two answers
   * disagree, so the diagnostic could show a key the radio never acted on.
   *
   * The raw lines go out whether or not they mean a key. A button that is
   * fitted but not in the map reads as a line going low and nothing else
   * happening, and that is the only way to tell it from a dead switch. */
  int8_t key = KEYPAD_NONE;
  uint16_t lines = 0;
  if (!keypadRead(&key, &lines)) {
    return;
  }
  sStatus.lines = lines;
  sStatus.linesOk = 1;

  /*
   * ENTER's hold, fed from the level every poll rather than from the edge
   * below, using the bits keypadRead just took off the expander so this
   * costs the bus nothing extra. Nothing fires on the way down; a tap
   * fires on release and a hold fires once at the long press time, the
   * same shape a button already has, and BUTTON_LONG_MS is the long press
   * time used, so there is one idea of a long press on this radio.
   */
  int8_t enterDown = keypadKeyFromLines(lines) == KEYPAD_ENTER
                         ? (int8_t)KEYPAD_ENTER
                         : (int8_t)-1;
  ButtonEvent enterEvent = keypadHoldFeed(&sEnterHold, enterDown, nowMs, NULL);
  if (enterEvent != BUTTON_NONE) {
    sStatus.presses++;
    onEnter(enterEvent, nowMs);
  }

  if (key == KEYPAD_NONE || key == KEYPAD_ENTER) {
    /* ENTER's own edge is handled above, from the level, so it must not
     * also reach the digit accumulation below: '0' + KEYPAD_ENTER is not a
     * digit anybody typed. */
    return;
  }
  sStatus.presses++;
  onKey(key, nowMs);
}

/*
 * The knob.
 *
 * One knob, one job at a time, and the squelch mode decides which. Off and
 * Auto leave it as the volume; Manual takes it for the squelch threshold.
 *
 * The job changing is the awkward part. Whichever it becomes has to be
 * applied at once from where the knob is now, or the setting it took over
 * keeps a value from a knob position that is long gone: coming back from
 * Manual with the volume stuck where it was before, or entering Manual with
 * a threshold from an old position that silences everything. Neither
 * recovers until the knob is moved past the deadband.
 */
static void pollPot(uint32_t nowMs) {
  static uint32_t lastPollMs = 0;
  if ((uint32_t)(nowMs - lastPollMs) < POT_POLL_MS) {
    return;
  }
  lastPollMs = nowMs;

  /* Asked for directly, not read out of the snapshot. The snapshot goes out
   * once a radio round, about 30 times a second on FM and 10 on AM, so for
   * up to one round after a mode change the knob would still be doing its
   * old job. */
  SquelchMode mode = radioSquelchMode(NULL);
  bool jobChanged = !sJobKnown || mode != sJob;
  sJob = mode;
  sJobKnown = true;

  uint16_t raw = potRead();
  sStatus.pot = raw;

  /* While calibrating the knob only records how far it reaches. It does not
   * set the volume or the squelch, because finding the loud end stop should
   * not mean sweeping the volume to full on the way. */
  if (sCal.active) {
    /* Somebody is standing at the radio sweeping the knob end to end, so
     * every poll counts as them using it. Without this the return below
     * skips the activity count and the panel dims under their hand halfway
     * through the calibration they are doing. potCalibrateSample gives up on
     * its own after a couple of minutes, so a calibration somebody walked
     * away from cannot leave the knob dead or hold the panel lit. */
    sActivity++;
    if (potCalibrateSample(&sCal, raw, nowMs)) {
      return;
    }
    /* It just timed out. Fall through, so the knob takes its job back on
     * this same poll rather than on the next movement. */
    sPotKnown = false;
  }

  bool moved = potMoved(sPot, raw, &sPotCfg);
  if (!jobChanged && sPotKnown && !moved) {
    return;
  }
  /* Only a real turn counts as somebody using the radio. The reading is taken
   * twenty times a second and never sits perfectly still, so counting every
   * one of them would mean the radio was never left alone. */
  if (sPotKnown && moved) {
    sActivity++;
  }
  sPot = raw;
  sPotKnown = true;

  if (mode == SQUELCH_MANUAL) {
    /* The knob is the squelch now, so there is no volume to report from it.
     * Leaving the old number there would read as the volume the knob is
     * pointing at, which it is not. */
    sStatus.potDb = 0;
    int16_t tenths =
        squelchThresholdFromPot(raw, sPotCfg.rawMin, sPotCfg.rawMax);
    radioSetSquelchThreshold(tenths);
    char text[INPUT_EVENT_MAX];
    snprintf(text, sizeof(text), "squelch %s%d.%d dBuV",
             (tenths < 0 && tenths > -10) ? "-" : "", tenths / 10,
             (tenths < 0 ? -tenths : tenths) % 10);
    note(text);
    return;
  }

  int8_t db = potVolumeDb(raw, &sPotCfg);
  if (!jobChanged && db == sStatus.potDb) {
    /* The reading moved but not far enough to be a different volume. */
    return;
  }
  sStatus.potDb = db;

  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_VOLUME;
  cmd.volumeDb = db;
  /* Not settled. The knob can be turned faster than the radio can answer, and
   * waiting for each step would make it feel stiff. The last one sent wins. */
  radioPost(&cmd);
}

void inputSetBeeps(BeepMode mode) {
  sBeepMode = mode < BEEP_MODE_COUNT ? mode : BEEP_OFF;
}

void inputSetPotConfig(const PotConfig *cfg) {
  if (cfg == NULL) {
    potDefaults(&sPotCfg);
    return;
  }
  sPotCfg = *cfg;
}

void inputPotCalibrateStart(void) {
  potCalibrateStart(&sCal, potRead(), millis());
}

bool inputPotCalibrateFinish(uint16_t *rawMin, uint16_t *rawMax) {
  /* Nothing to report unless a calibration was actually running. Reading the
   * extremes first would quote the previous sweep back as though this one had
   * happened. */
  bool running = sCal.active;
  if (rawMin != NULL) {
    *rawMin = running ? sCal.rawMin : 0;
  }
  if (rawMax != NULL) {
    *rawMax = running ? sCal.rawMax : 0;
  }
  if (!potCalibrateFinish(&sCal, &sPotCfg)) {
    return false;
  }
  /* The knob has moved a long way during the sweep, so the next poll acts on
   * where it has been left rather than comparing against a stale reading. */
  sPotKnown = false;
  return true;
}

void inputPotCalibrateCancel(void) {
  potCalibrateCancel(&sCal);
}

bool inputPotCalibrating(uint16_t *rawMin, uint16_t *rawMax) {
  if (rawMin != NULL) {
    *rawMin = sCal.rawMin;
  }
  if (rawMax != NULL) {
    *rawMax = sCal.rawMax;
  }
  return sCal.active;
}

uint32_t inputActivity(void) {
  return sActivity;
}

#if FEATURE_TOUCH
static void apiTouch(const ApiInput *in, uint32_t nowMs);
#endif

static void pollApi(uint32_t nowMs) {
  if (!sApi.pending) {
    return;
  }
  const ApiInput in = sApi;
  sApi.pending = false;
  sFromApi = true;
#if FEATURE_TOUCH
  if (in.touch != TOUCH_NOTHING) {
    apiTouch(&in, nowMs);
    sFromApi = false;
    return;
  }
#endif
  if (in.clicks != 0) {
    onTurn(in.clicks, nowMs);
  } else if (in.key == INPUT_KEY_ENTER) {
    onEnter(in.event, nowMs);
  } else if (in.key == INPUT_KEY_DX) {
    onKey(KEYPAD_DX, nowMs);
  } else if (in.key >= INPUT_KEY_DIGIT_0) {
    onKey((int8_t)(in.key - INPUT_KEY_DIGIT_0), nowMs);
  } else {
    static const PanelButton kButton[] = {PANEL_BUTTON_BAND, PANEL_BUTTON_BW,
                                          PANEL_BUTTON_MODE,
                                          PANEL_BUTTON_ENCODER};
    onButton(kButton[in.key], in.event, typingInProgress(nowMs));
  }
  sFromApi = false;
}

#if FEATURE_TOUCH
/*
 * The shortest time between two touch readings while a finger is down. The
 * loop's own pace often makes it longer: a held finger or pen got a median of
 * 25 to 70 readings a second. The chip is not read at all while nobody
 * touches the glass, so its 2.5 MHz clock, whose multiples fall inside the FM
 * band, is off then. While a pen was held, a level sweep of the FM band with
 * the antenna out read the same with these readings as without them, within
 * the spread between two sweeps of either kind.
 */
#define TOUCH_READ_MS 10
static uint32_t sTouchReadMs = 0;

/*
 * How a touch is told apart, from taps, holds and swipes measured on this
 * glass. A finger wandered at most 14.2 px in the first 0.6 s of a press, so
 * a move past 16 px is a move. Taps wandered at most 14.2 px and whole
 * swipes travelled at least 164, so 64 px of travel is a swipe, and the
 * slowest natural swipe took 771 ms, so a swipe is one done within a second.
 */
static const TouchGestureConfig kGesture = {
    16, 64, 1000, TOUCH_HOLD_MS, TOUCH_REPEAT_DELAY_MS, TOUCH_REPEAT_MS};

static TouchGesture sGesture;
/* The gesture under way has had its first event, and with it the checks a
 * gesture makes once. */
static bool sGestureSeen = false;
/* Its first event stopped a DX scan or cleared a typed number, which is
 * all a touch does then, so the rest of it does nothing. */
static bool sGestureSpent = false;

/* A finger on the glass when touch could not be read, the panel dark, a
 * sweep running, Touch Off or the calibration screen up, is left alone
 * until it lifts, the way a key held at start up is: it went down for none
 * of what is on screen now. */
static bool sTouchHeldOver = false;

/* Drop the gesture under way, if any, with nothing reported. */
static void touchRest(void) {
  memset(&sGesture, 0, sizeof(sGesture));
  sGestureSeen = false;
  sGestureSpent = false;
  sTouchHeldOver = true;
}

/* What a gesture is called in `inp.lst`, by TouchGestureEvent. */
static const char *const kGestureName[] = {
    "",           "tap",         "hold",     "drag",       "drag end",
    "swipe left", "swipe right", "swipe up", "swipe down", "repeat"};

/*
 * One thing a finger did, from the glass or from POST /api/touch: `first`
 * whether it is the first event of its gesture, `zone` the zone of the
 * screen on top it began in, `start` where and `last` where it is now.
 *
 * The checks a key makes for itself are made once for the whole gesture, on
 * its first event, since a drag is many events: the beep by Key Beeps, a tap
 * as a short press, a tap on the keypad as a key and a hold as a long press;
 * a running DX scan, which any touch stops and does nothing else, as any key
 * does; and a number being typed, which a touch off the keypad clears and
 * does nothing else. True when it did one of those two, which is all the
 * gesture does. A drag is noted once, not on
 * every sample. Then the screen on top acts on it, if the zone is one of its
 * own.
 */
static bool onTouch(TouchGestureEvent event, bool first, int zone,
                    TouchPoint start, TouchPoint last, uint32_t nowMs) {
  sActivity++;
  bool dxUnder = false;
  const ScreenInput *in = screenInput(&dxUnder);
  if (first) {
    sDialDragging = false;
    if (!sFromApi) {
      sStatus.touchGestures++;
    }
    const bool hold = event == TOUCH_HOLD;
    /* A tap on the keypad is a key, and beeps as the keys do. */
    const bool pinKey = in == &kScreenInput[TOP_MENU] &&
                        zone >= MENU_ZONE_PIN_KEY &&
                        zone < MENU_ZONE_PIN_KEY + SCREEN_PIN_KEYS;
    const bool key =
        event == TOUCH_TAP &&
        ((in == &kScreenInput[TOP_KEYPAD] && zone != TOUCH_NO_ZONE) || pinKey);
    if (sBeepMode >= BEEP_EVERY_PRESS || (sBeepMode >= BEEP_KEYS && key) ||
        (sBeepMode >= BEEP_KEYS_AND_LONG && hold)) {
      radioBeep(hold ? BEEP_LONG_MS : BEEP_MS);
    }
    if (stopScanFirst()) {
      return true;
    }
    /* But for on the keypad, where a touch is what types it. */
    if (typingInProgress(nowMs) && in != &kScreenInput[TOP_KEYPAD]) {
      cancelTyping();
      return true;
    }
  }
  if (first || event != TOUCH_DRAG) {
    /* A Web PIN digit is noted as one, never as which: GET /api/state,
     * which shows this note, needs no PIN. */
    const bool pinDigit = in == &kScreenInput[TOP_MENU] &&
                          zone >= MENU_ZONE_PIN_KEY &&
                          zone < MENU_ZONE_PIN_KEY + SCREEN_PIN_BACKSPACE;
    const char *where = pinDigit ? "digit of the PIN"
                        : zone != TOUCH_NO_ZONE && in->zoneName != NULL
                            ? in->zoneName(zone)
                            : "";
    char seen[INPUT_EVENT_MAX];
    snprintf(seen, sizeof(seen), "touch %s%s%s", kGestureName[event],
             where[0] != '\0' ? " " : "", where);
    note(seen);
  }
  /* A tap where the menu's Back was, just after the menu shut, is one Back
   * too many, as MODE is then. */
  const bool wasMenu = inputTop(&dxUnder) == TOP_MENU;
  if (event == TOUCH_TAP && screenMenuIsBack(start) && backTooSoon(wasMenu)) {
    return false;
  }
  if (zone != TOUCH_NO_ZONE && in->touch != NULL) {
    in->touch(event, zone, start, last, dxUnder);
  }
  notedMenuShut(wasMenu);
  return false;
}

/* The zones of the screen on top, TOUCH_ZONES_MAX at most, and its row of
 * the table. */
static int screenZones(TouchZone *zones, const ScreenInput **in) {
  bool dxUnder = false;
  *in = screenInput(&dxUnder);
  return (*in)->zones != NULL ? (*in)->zones(zones, TOUCH_ZONES_MAX) : 0;
}

/* The zone of the screen on top holding `at`, whether it follows a drag,
 * and whether it repeats while held: only the value editor's minus and
 * plus. */
static int zoneAt(TouchPoint at, bool *drags, bool *repeats) {
  const ScreenInput *in = NULL;
  TouchZone zones[TOUCH_ZONES_MAX];
  const int n = screenZones(zones, &in);
  const int zone = touchZoneAt(zones, n, at);
  *drags = zone != TOUCH_NO_ZONE && zone == in->dragZone;
  if (repeats != NULL) {
    *repeats = in == &kScreenInput[TOP_MENU] &&
               (zone == MENU_ZONE_MINUS || zone == MENU_ZONE_PLUS);
  }
  return zone;
}

/* Whether a finger could make `event` from `at`, to `to` for a drag: a zone
 * that follows a drag turns any move past the slop into one and never
 * swipes, and no other zone drags. */
static bool gestureFits(TouchGestureEvent event, TouchPoint at, TouchPoint to) {
  bool drags = false;
  (void)zoneAt(at, &drags, NULL);
  if (event != TOUCH_DRAG) {
    return !drags || event < TOUCH_SWIPE_LEFT;
  }
  return drags && (abs(to.x - at.x) > kGesture.slopPx ||
                   abs(to.y - at.y) > kGesture.slopPx);
}

/* A gesture sent over POST /api/touch, made whole at once: a drag is its
 * start and its end. */
static void apiTouch(const ApiInput *in, uint32_t nowMs) {
  bool drags = false;
  const int zone = zoneAt(in->at, &drags, NULL);
  (void)drags;
  if (in->touch == TOUCH_DRAG) {
    if (!onTouch(TOUCH_DRAG, true, zone, in->at, in->to, nowMs)) {
      (void)onTouch(TOUCH_DRAG_END, false, zone, in->at, in->to, nowMs);
    }
    return;
  }
  (void)onTouch(in->touch, true, zone, in->at, in->at, nowMs);
}

/*
 * Feed one steady point, or none, to the gesture under way and act on what
 * it completes.
 */
static void gestureFeed(bool contact, TouchPoint at, bool unsettled,
                        uint32_t nowMs) {
  if (sTouchHeldOver) {
    sTouchHeldOver = contact;
    return;
  }
  bool dxUnder = false;
  uint8_t page = 0;
  (void)screenTaskShowing(&page);
  TouchSample s;
  s.down = contact;
  s.at = at;
  s.unsettled = unsettled;
  s.zoneDrags = false;
  s.zoneRepeats = false;
  s.zone = contact ? zoneAt(at, &s.zoneDrags, &s.zoneRepeats) : TOUCH_NO_ZONE;
  /* The menu's place too, so a gesture begun on one level of it does not
   * act on the next, and the catches drawn, so one begun on a row does not
   * act on another catch the list re-sorted into it. */
  const InputTop top = inputTop(&dxUnder);
  s.screen = menuTaskPlace() << 16 | (uint32_t)top << 8 |
             (uint32_t)dxUnder << 4 | page;
  if (top == TOP_DX && page == SCREEN_DX_PAGE_CATCHES) {
    s.screen ^= (uint32_t)screenCatchesRowsId() << 16;
  }
  const TouchGestureEvent event =
      touchGestureFeed(&sGesture, &kGesture, &s, nowMs);
  if (event != TOUCH_NOTHING && !sGestureSpent) {
    const bool first = !sGestureSeen;
    sGestureSeen = true;
    sGestureSpent = onTouch(event, first, sGesture.zone, sGesture.start,
                            sGesture.last, nowMs);
  }
  if (!sGesture.on) {
    sGestureSeen = false;
    sGestureSpent = false;
  }
}

/*
 * Read the touch chip while a finger is down, keep the reading for the
 * status document, and hand the steady point to the calibration screen
 * while it is open and to the gestures otherwise.
 */
static void pollTouch(uint32_t nowMs) {
  const bool wasOn = sStatus.touchOn;
  /* The calibration screen reads the chip whatever the setting: a person
   * opened it to make the touch screen work. */
  const bool calOpen = screenTaskTouchCalIsOpen();
  sStatus.touchOn = sTouchOn || calOpen;
  if (!sStatus.touchOn) {
    sStatus.touchPen = false;
    memset(&sTouchFilter, 0, sizeof(sTouchFilter));
    touchRest();
    return;
  }
  /* Touch is not read while the panel is dark, and does not wake it: a
   * pocket or a bag can press the glass, and the knob, a key or the volume
   * knob wake it. The same while the boot screen, the going to sleep screen
   * or an update holds the panel, which a key ends, and during a level sweep.
   * The calibration screen is read whatever: a person is holding a mark. */
  const bool resting = !calOpen && touchIgnored();
  const bool pen = touchPenDown();
  /* A finger already down when touch comes on did not go down now. */
  if (pen && !sStatus.touchPen && wasOn) {
    sStatus.touchDowns++;
  }
  sStatus.touchPen = pen;
  TouchReading r;
  memset(&r, 0, sizeof(r));
  r.pen = pen;
  if (resting) {
    memset(&sTouchFilter, 0, sizeof(sTouchFilter));
    touchRest();
    return;
  }
  if (pen && (uint32_t)(nowMs - sTouchReadMs) >= TOUCH_READ_MS) {
    sTouchReadMs = nowMs;
    touchTake(&r, &sStatus.touch);
    sStatus.touchReads++;
  }
  TouchPoint raw = {0, 0};
  bool unsettled = true;
  const bool contact = touchFilterFeed(&sTouchFilter, &r, &raw, &unsettled);
  if (contact) {
    sStatus.touchAt =
        touchTurn(touchCalMap(&sTouchCal, raw, DISPLAY_WIDTH, DISPLAY_HEIGHT),
                  DISPLAY_WIDTH, DISPLAY_HEIGHT, sTouchUpsideDown);
    sStatus.touchMapped = true;
  }
  if (calOpen) {
    if (contact) {
      /* A finger holding a mark is somebody at the radio: the panel stays
       * lit and auto off waits. */
      sActivity++;
    }
    screenTaskTouchCalFeed(contact, r.read, raw, unsettled, nowMs);
    touchRest();
    return;
  }
  gestureFeed(contact, sStatus.touchAt, unsettled, nowMs);
}
#endif

void inputPoll(void) {
  uint32_t nowMs = millis();
  pollEncoder(nowMs);
  pollButtons(nowMs);
  pollPot(nowMs);
  pollKeypad(nowMs);
#if FEATURE_TOUCH
  pollTouch(nowMs);
#endif
  pollApi(nowMs);
}

void inputSetTouch(bool on) {
#if FEATURE_TOUCH
  sTouchOn = on;
  /* At once, so a menu drawn straight after the Touch row is kept carries
   * the right marks. */
  uiSetTouchMarks(inputTouchUsable());
#else
  (void)on;
#endif
}

bool inputTouchUsable(void) {
#if FEATURE_TOUCH
  return sTouchOn && sStatus.touchChip;
#else
  return false;
#endif
}

void inputSetKeypadTimeout(uint8_t seconds) {
  sKeypadTimeoutMs = (uint32_t)seconds * 1000;
}

uint32_t inputKeypadTimeoutMs(void) {
  return sKeypadTimeoutMs;
}

void inputSetTouchUpsideDown(bool upsideDown) {
#if FEATURE_TOUCH
  sTouchUpsideDown = upsideDown;
#else
  (void)upsideDown;
#endif
}

void inputTouchCalSet(const TouchCal *cal, bool stored) {
#if FEATURE_TOUCH
  if (cal != NULL) {
    sTouchCal = *cal;
    sStatus.touchCalStored = stored;
  }
#else
  (void)cal;
  (void)stored;
#endif
}

bool inputTouchCalGet(TouchCal *cal, bool *upsideDown) {
#if FEATURE_TOUCH
  if (cal != NULL) {
    *cal = sTouchCal;
  }
  if (upsideDown != NULL) {
    *upsideDown = sTouchUpsideDown;
  }
  return true;
#else
  (void)cal;
  (void)upsideDown;
  return false;
#endif
}

bool inputPressFromApi(InputKey key, ButtonEvent event) {
  if (sApi.pending || key >= INPUT_KEY_COUNT ||
      !(event == BUTTON_SHORT ||
        (event == BUTTON_LONG && inputKeyTakesLong(key)))) {
    return false;
  }
  sApi.key = key;
  sApi.event = event;
  sApi.clicks = 0;
#if FEATURE_TOUCH
  sApi.touch = TOUCH_NOTHING;
#endif
  sApi.pending = true;
  return true;
}

bool inputTurnFromApi(int32_t clicks) {
  if (sApi.pending || clicks == 0) {
    return false;
  }
  sApi.clicks = clicks;
#if FEATURE_TOUCH
  sApi.touch = TOUCH_NOTHING;
#endif
  sApi.pending = true;
  return true;
}

InputTouchResult inputTouchFromApi(TouchGestureEvent event, TouchPoint at,
                                   TouchPoint to) {
#if FEATURE_TOUCH
  if (sApi.pending) {
    return INPUT_TOUCH_BUSY;
  }
  if (!sTouchOn) {
    return INPUT_TOUCH_OFF;
  }
  if (screenTaskTouchCalIsOpen()) {
    return INPUT_TOUCH_CALIBRATING;
  }
  if (touchIgnored()) {
    return INPUT_TOUCH_NOT_NOW;
  }
  if (!gestureFits(event, at, to)) {
    return INPUT_TOUCH_NOT_HERE;
  }
  sApi.touch = event;
  sApi.at = at;
  sApi.to = to;
  sApi.clicks = 0;
  sApi.pending = true;
  return INPUT_TOUCH_TAKEN;
#else
  (void)event;
  (void)at;
  (void)to;
  return INPUT_TOUCH_OFF;
#endif
}

void inputStatusGet(InputStatus *out) {
  if (out == NULL) {
    return;
  }
  *out = sStatus;
}

int inputScreenZones(InputZone *out, int max) {
#if FEATURE_TOUCH
  if (touchIgnored()) {
    return 0;
  }
  const ScreenInput *in = NULL;
  TouchZone zones[TOUCH_ZONES_MAX];
  const int n = screenZones(zones, &in);
  int i = 0;
  for (; i < n && i < max; i++) {
    out[i].zone = zones[i];
    out[i].name = in->zoneName(zones[i].id);
  }
  return i;
#else
  (void)out;
  (void)max;
  return 0;
#endif
}
