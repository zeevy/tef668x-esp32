/* Implementation of the screen glue. */
#include "screen_task.h"
#include "drivers/settings_nvs.h"
#include "dx_task.h"
#include "screen_bw_state.h"
#include "screen_state.h"
#include "screen_task_dx.h"
#include "screen_task_rds.h"

#include "band_scan_task.h"
#include "board/board.h"
#include "core/band_plan.h"
#include "core/battery.h"
#include "core/bw_page.h"
#include "core/clock.h"
#include "core/dx.h"
#include "core/memory.h"
#include "core/meter.h"
#include "core/rds.h"
#include "core/rds_country.h"
#include "core/signal.h"
#include "core/strings.h"
#include "core/touch_cal.h"
#include "core/update_screen.h"
#include "core/version.h"
#include "core/wifi_signal.h"
#include "drivers/analog.h"
#include "drivers/battery_adc.h"
#include "drivers/display.h"
#include "input_task.h"
#include "lvgl_port.h"
#include "memory_store.h"
#include "menu_task.h"
#include "net/ntp.h"
#include "net/rollback.h"
#include "net/update_check.h"
#include "net/wifi_manager.h"
#include "radio_task.h"
#include "ui/draw.h"
#include "ui/panel.h"
#include "ui/screen.h"
#include "ui/theme.h"

#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

/*
 * How often the screen is looked at, in milliseconds.
 *
 * The radio publishes once a round, about thirty times a second on FM and
 * ten on AM, and a person cannot read a number that changes faster than this.
 * Twenty five times a second is quicker than a hand can turn the knob.
 */
#define SCREEN_POLL_MS 40

/* How long a failed write stays on the panel before the radio comes back. */
#define UPDATE_FAILED_HOLD_MS 4000UL

/*
 * How long the boot screen stays up after start up has finished, in ms.
 *
 * Start up takes about two seconds, since nothing waits for the network, and
 * without a hold the finished screen is gone before it can be read. The rows
 * fill in during those two seconds, so this is only the time to read the screen
 * once it is complete: six short rows and a version line, which is about a
 * second and a half, which is what this is. Three seconds reads as a wait.
 *
 * It is a deadline the poll watches, not a delay in `setup`, and that is the
 * part that matters whatever the number becomes. Two seconds of `delay` would
 * be two seconds in which the knob, the keypad and the web server do nothing.
 * This way the radio is working for the whole hold and the only thing waiting
 * is the panel.
 */
#define BOOT_HOLD_MS 1500UL

/*
 * How long the panel light takes to go out and come back at the swap.
 *
 * The boot screen does not dissolve into the radio screen, it fades through
 * black, because the two cannot both exist: the LVGL pool holds one of them
 * at a time. So the light goes down, the screens are swapped while nothing
 * can be seen, and the light comes back on the radio.
 *
 * It costs no memory and no LVGL feature, and on this panel it looks the same
 * as a cross fade would: the ground is true black, so fading the light and
 * fading the content are the same picture.
 *
 * The lengths are the panel light's own, not numbers of this file's. The
 * radio already fades up over `BACKLIGHT_FADE_UP_MS` when it is switched on,
 * and a second, faster fade a few seconds later would read as two different
 * mechanisms. One ramp speed on this radio, wherever the light moves.
 */
#define BOOT_SWAP_OUT_MS ((uint32_t)BACKLIGHT_FADE_DOWN_MS)
#define BOOT_SWAP_IN_MS ((uint32_t)BACKLIGHT_FADE_UP_MS)

/*
 * How often the panel light is stepped during the fade at boot, in ms.
 *
 * The fade runs inside screenTaskBegin, so this is a plain delay and nothing
 * else is waiting on it. About forty seven steps over the 750 ms the fade
 * takes, BACKLIGHT_FADE_UP_MS, is finer than the eye follows.
 */
#define BACKLIGHT_STEP_MS 16

static bool sReady = false;
static uint32_t sLastPollMs = 0;

/* What the state builder holds from one poll to the next: see
 * screen_state.h. */
static ScreenBuild sBuild;

/*
 * The battery, read on this task rather than the radio's.
 *
 * The radio task owns the tuner on a 100 ms cadence and a 43 ms RDS cadence,
 * and an ADC read that has to wait for Wi-Fi to let go of the converter has
 * no business on it. Once a second is far more often than a battery moves and
 * the smoothing in core/battery.c is sized for that rate.
 */
#define BATTERY_POLL_MS 1000

/*
 * How long "Logged 106.40" holds the station name band, in milliseconds.
 *
 * Long enough to read once, short enough that it is gone well before the
 * next station change would want the band back. Not measured against
 * anything; there is no signal here to fit a threshold to, only a length a
 * person can read a short line in.
 */
#define LOG_CONFIRM_MS 1500u
static Battery sBattery;
static uint32_t sBatteryPollMs = 0;
/* Given by the layer that owns the settings, the way the panel light is.
 * The radio snapshot carries what the radio is tuned to, and how a reading is
 * displayed is not that. */
static BatteryShow sBatteryShow = BATTERY_SHOW_OFF;
static RdsRegion sRdsRegion = RDS_REGION_EUROPE;
/* The level offsets, added to each level shown. */
static int8_t sLevelOffsetFmDb = 0;
static int8_t sLevelOffsetAmDb = 0;

/* The panel light, and what it was last set to, so it is not rewritten. */
static Backlight sBacklight;
static uint8_t sLastWritten = 0;
static bool sLastWrittenKnown = false;

/* The activity count last seen, so a change of it counts as one wake. */
static uint32_t sLastActivity = 0;
/* When it last changed, for closing a page left alone. */
static uint32_t sLastActivityMs = 0;

/*
 * Whether the loop has run once yet.
 *
 * Start up carries on for seconds after the panel is lit, through the tuner
 * patch and the Wi-Fi join, and nothing moves the backlight along during it.
 * Without this, a radio set to dim after ten seconds and taking twelve to
 * come up would dim the moment it finished booting, in front of the person
 * who has just switched it on. The idle clock starts when the radio is ready,
 * not when the panel was lit.
 */
static bool sFirstPoll = true;

/* Who owns the panel while firmware is being written. The rules are in
 * core/update_screen.h, which is where they can be tested. */
static UpdateScreen sUpdate;

/*
 * The boot screen, and the strings it is holding.
 *
 * The values are copied rather than pointed at. `setup` reports the channel
 * count and the battery out of buffers it is about to reuse, and a screen
 * that kept the pointer would draw whatever landed there next.
 */
static ScreenBoot sBoot;
static char sBootValue[SCREEN_BOOT_STEPS][12];
static char sBootTuner[40];
static bool sBootUp = false;

/* The menu owns the panel. The radio layout does not exist while it does. */
static bool sMenuUp = false;

/* The RDS screen owns it instead. The menu, the bandwidth page and the RDS
 * screen are never up together; the last two can be up over DX mode. */
static bool sRdsUp = false;

/* The bandwidth page: its tiles, its cursor and its text, on the heap while it
 * is up, and NULL when it is down. */
typedef struct {
  BwTile tiles[BW_PAGE_MAX];
  uint8_t count;
  uint8_t cursor;
  BandId band; /* The band the tiles are for. */
  bool overDx; /* Opened over a DX page, which comes back on closing. */
  ScreenBwKeep keep;
  /* What was last drawn, so the poll redraws only when something on the
   * page has moved: a redraw of nineteen tiles every poll keeps the loop
   * busy enough that the polled keys are missed. */
  ScreenBwInputs shown;
  char shownClock[CLOCK_TEXT_LEN];
  UiSleepMark shownSleep; /* The sleep mark the header had. */
  bool drawn;
} BwStore;
static BwStore *sBw = NULL;

/* The touch calibration screen, on the heap while it is up: the calibration
 * being made, and whether what it shows has changed since it was drawn. */
typedef struct {
  TouchCalFlow flow;
  bool changed;
} CalStore;
static CalStore *sCal = NULL;

/* Which of the RDS screen's four pages the knob has moved to. */
static uint8_t sRdsPage = 0;

/* The DX page owns it instead, and puts its own width in force while it does.
 * The width is sent from the poll, and sent again until the queue takes it, so
 * leaving DX mode can never leave the DX width behind. */
static bool sDxUp = false;
static uint8_t sDxPage = SCREEN_DX_PAGE_DX;
static uint8_t sDxCursor = 0; /* The catch the Catches page's cursor is on. */
static uint16_t sDxWidthKHz = DX_BANDWIDTH_DEFAULT_KHZ;
static bool sDxWidthDirty = false;

/* When the boot screen is due to go, once start up has said it may. */
static uint32_t sBootHoldUntilMs = 0;
static bool sBootHolding = false;

/* The logbook write confirmation. Empty text means nothing to show, which a
 * fresh boot already is without needing an extra flag. */
static char sLogConfirmText[32];
static uint32_t sLogConfirmStartMs = 0;

/*
 * The fade through black, between the boot screen and the radio.
 *
 * Its own little state machine rather than part of core/backlight.c, because
 * it is not the panel light deciding anything: it is one transition that
 * borrows the light for about a second and a half, 750 ms out and 750 ms
 * in, and gives it straight back. The light's own rules carry on the moment
 * this is done.
 */
typedef enum {
  BOOT_SWAP_NONE = 0, /* Not swapping. The light belongs to backlight.c. */
  BOOT_SWAP_OUT,      /* Going dark, the old screen still up. */
  BOOT_SWAP_IN,       /* Coming back, the new screen underneath. */
} BootSwap;

static BootSwap sSwap = BOOT_SWAP_NONE;
static uint32_t sSwapFromMs = 0;
static uint8_t sSwapLevel = 0;

/* The menu opens and closes with no fade; only start up fades. */

/* Start the fade through black. swapAct makes the change while nothing can
 * be seen. */
static void swapBegin(void) {
  if (sSwap != BOOT_SWAP_NONE) {
    return;
  }
  sSwap = BOOT_SWAP_OUT;
  sSwapFromMs = millis();
  sSwapLevel = backlightLevel(&sBacklight);
}

/*
 * What each row is called, in the order of `BootStep`.
 *
 * Here rather than in `ui/`, for the reason the whole file exists: the screen
 * draws what it is given and knows nothing about tuners or keypads, and this
 * is the one place that knows both.
 */
static const StrId kBootNames[SCREEN_BOOT_STEPS] = {
    STR_BOOT_SETTINGS, STR_COMMON_TUNER, STR_BOOT_RADIO,
    STR_BOOT_CHANNELS, STR_BOOT_KEYPAD,  STR_COMMON_BATTERY,
};

static void writeBacklight(uint8_t percent) {
  if (sLastWrittenKnown && percent == sLastWritten) {
    return;
  }
  displayBacklight(percent);
  sLastWritten = percent;
  sLastWrittenKnown = true;
}

bool screenTaskBegin(const BacklightConfig *cfg, uint16_t rotationDegrees) {
  /* The DX session's notices go on the confirmation line, as the panel's
   * own do. */
  dxTaskSetSay(screenTaskLogConfirm, screenTaskLogConfirmAt);
  /* The panel first. LVGL renders into its own buffer and hands finished
   * rectangles to displayPush, which refuses everything until the ILI9341 has
   * had its reset and its start up sequence. Nothing else reports a fault in
   * that case: LVGL runs, the objects are built, the heap looks healthy and
   * the glass stays black. */
  batteryReset(&sBattery);
  screenStateReset(&sBuild);
  /*
   * Seed from the sample main.cpp took before Wi-Fi started.
   *
   * Measured on this radio: that sample reads, and every read after Wi-Fi is up
   * fails. GPIO 13 is ADC2 and the Wi-Fi driver holds ADC2 for the whole time
   * it is running, which is the whole time on this radio. So the boot sample is
   * not a nicety, it is the reading.
   *
   * The polling below is kept anyway. It costs eight ADC reads a second on a
   * task with nothing else to do, and it is what would pick a fresh value up
   * if the radio ever ran with Wi-Fi off.
   */
  uint16_t bootMv = 0;
  if (batteryAdcAtBoot(&bootMv)) {
    batteryFeed(&sBattery, bootMv, true);
  }

  if (!displayBegin()) {
    Serial.println(F("[screen] the panel did not come up"));
    return false;
  }
  /* Before the first pixel. A MADCTL change only affects what is written
   * after it, so turning the panel once the boot screen is up would leave
   * that screen half one way and half the other. */
  displayRotationSet(rotationDegrees == 180 ? 180 : 0);
  /* Then LVGL, because the screens are built out of its objects and there is
   * nothing to build them on until it has a display. */
  if (!lvglPortBegin()) {
    Serial.println(F("[screen] LVGL did not start"));
    return false;
  }
  /*
   * The boot screen, with every row waiting and nothing ticked.
   *
   * It goes up before the light does, so the fade reveals a screen with
   * something on it rather than filling in afterwards, and `setup` writes
   * each row as the thing itself answers.
   *
   * The radio layout is not built yet. It is about 15.6 KB of a 24 KB LVGL
   * pool, and both at once does not fit: LVGL asserts on a failed allocation,
   * which restarts the radio. `screenTaskBootEnd` builds the layout as the
   * boot screen goes.
   */
  memset(&sBoot, 0, sizeof(sBoot));
  sBoot.product = txt(STR_RADIO_PRODUCT_NAME);
  /* The board alone: the version is on the amber panel, beside the tuner. */
  sBoot.board = BOARD_NAME_DISPLAY;
  sBoot.total = SCREEN_BOOT_STEPS;
  for (uint8_t i = 0; i < SCREEN_BOOT_STEPS; i++) {
    sBoot.steps[i].name = txt(kBootNames[i]);
    sBoot.steps[i].mark = SCREEN_BOOT_WAITING;
  }
  sBootUp = screenBootBegin();
  if (sBootUp) {
    screenBootShow(&sBoot);
  } else {
    /* No boot screen, so the radio layout is built now and says what little
     * it can. A panel with nothing on it reads as broken hardware. */
    sReady = screenBegin();
    if (!sReady) {
      Serial.println(F("[screen] the screens could not be built"));
      return false;
    }
    screenMessage(txt(STR_RADIO_PRODUCT_NAME),
                  BOARD_NAME_DISPLAY " " FIRMWARE_VERSION);
  }
  /* Drawn now rather than left to the first poll, so there is something on
   * the glass for the fade below to reveal. */
  lvglPortPoll();

  /* The message goes on the glass before any light does, so the fade reveals
   * something rather than coming up on a blank panel and filling in after. */
  backlightInit(&sBacklight, cfg, millis());
  writeBacklight(backlightLevel(&sBacklight));
  while (backlightFading(&sBacklight)) {
    delay(BACKLIGHT_STEP_MS);
    writeBacklight(backlightUpdate(&sBacklight, millis()));
  }
  writeBacklight(backlightUpdate(&sBacklight, millis()));

  sLastActivity = inputActivity();
  sLastActivityMs = millis();
  return true;
}

void screenTaskSetBatteryShow(uint8_t show) {
  sBatteryShow =
      show < (uint8_t)BATTERY_SHOW_COUNT ? (BatteryShow)show : BATTERY_SHOW_OFF;
}

void screenTaskSetLevelOffsets(int8_t fmDb, int8_t amDb) {
  /* Out of range is ignored, as for the signal scale, so a caller that has
   * not checked leaves the offsets as the settings last said. */
  if (fmDb >= SIGNAL_LEVEL_OFFSET_MIN_DB &&
      fmDb <= SIGNAL_LEVEL_OFFSET_MAX_DB &&
      amDb >= SIGNAL_LEVEL_OFFSET_MIN_DB &&
      amDb <= SIGNAL_LEVEL_OFFSET_MAX_DB) {
    sLevelOffsetFmDb = fmDb;
    sLevelOffsetAmDb = amDb;
  }
}

int8_t screenTaskLevelOffsetDb(BandId band) {
  return bandModulation(band) == MODULATION_FM ? sLevelOffsetFmDb
                                               : sLevelOffsetAmDb;
}

void screenTaskSetRdsRegion(uint8_t region) {
  sRdsRegion =
      region < RDS_REGION_COUNT ? (RdsRegion)region : RDS_REGION_EUROPE;
}

RdsRegion screenTaskRdsRegion(void) {
  return sRdsRegion;
}

void screenTaskSetBacklight(const BacklightConfig *cfg) {
  backlightSetConfig(&sBacklight, cfg, millis());
  if (sReady) {
    writeBacklight(backlightLevel(&sBacklight));
  }
}

bool screenTaskBacklightState(uint8_t *percent) {
  if (percent != NULL) {
    *percent = backlightLevel(&sBacklight);
  }
  return backlightDimmed(&sBacklight);
}

void screenTaskLogConfirm(const char *text) {
  if (text == NULL) {
    return;
  }
  snprintf(sLogConfirmText, sizeof(sLogConfirmText), "%s", text);
  sLogConfirmStartMs = millis();
}

void screenTaskLogConfirmAt(uint8_t band, uint32_t khz) {
  char freq[16] = "";
  bandFormatFrequency((BandId)band, khz, freq, sizeof(freq));
  char text[32];
  snprintf(text, sizeof(text), txt(STR_RADIO_FMT_LOGGED), freq);
  screenTaskLogConfirm(text);
}

/* The confirmation while it holds, LOG_CONFIRM_MS from screenTaskLogConfirm,
 * or NULL. A refusal's reason, "Switch to FM first", is shown the same way. */
static const char *screenTaskLogConfirmText(void) {
  return sLogConfirmText[0] != '\0' &&
                 (uint32_t)(millis() - sLogConfirmStartMs) < LOG_CONFIRM_MS
             ? sLogConfirmText
             : NULL;
}

const char *screenTaskHeaderMessage(void) {
  static char text[24];
  InputStatus input;
  inputStatusGet(&input);
  if (input.typed[0] != '\0') {
    char typed[INPUT_DIGITS_MAX + 2];
    snprintf(typed, sizeof(typed),
             strlen(input.typed) >= INPUT_DIGITS_MAX ? "%s"
                                                     : txt(STR_RADIO_FMT_TYPED),
             input.typed);
    snprintf(text, sizeof(text), txt(STR_COMMON_FMT_TUNE_TYPED), typed);
    return text;
  }
  return screenTaskLogConfirmText();
}

int16_t screenTaskSignalShown(void) {
  return sBuild.signalDisplay.shownDb;
}

/*
 * Put it on the panel now. Nothing else is going to: the poll is held.
 *
 * `lvglPortRefreshNow` rather than the poll, because two of these can land in
 * the same millisecond. A write that fails at `Update.begin` draws the
 * progress screen and then UPDATE FAILED microseconds apart, and on the
 * poll's 16 ms period the second one would be dropped and the panel would sit
 * on the wrong screen until the hold ran out.
 */
static void updateScreenShow(const char *line1, const char *line2) {
  screenMessage(line1, line2);
  lvglPortRefreshNow();
}

/* How long the last one took, for `pnl.swp` in the state document. */
static uint16_t sSwapActMs = 0;

uint16_t screenTaskSwapMs(void) {
  return sSwapActMs;
}

/*
 * Take the menu down now, with no fade and no waiting.
 *
 * For anything that needs the panel this instant: a firmware write is the one
 * that matters. An update runs inside `webLoop` on the same task, so the poll
 * never comes round while it runs.
 */
static void menuDownNow(void) {
  if (sSwap != BOOT_SWAP_NONE) {
    /* A swap is part way through and its light ramp will never be finished
     * by anybody now. */
    sSwap = BOOT_SWAP_NONE;
    writeBacklight(backlightLevel(&sBacklight));
  }
  if (!sMenuUp) {
    return;
  }
  const uint32_t began = millis();
  sMenuUp = false;
  screenMenuEnd();
  sReady = screenBegin();
  if (!sReady) {
    Serial.println(F("[screen] the radio screen could not be rebuilt"));
    return;
  }
  sSwapActMs = (uint16_t)(millis() - began);
  sLastPollMs = millis() - SCREEN_POLL_MS;
}

void screenTaskUpdateBegin(void) {
  /* The DX page and the RDS screen go as well, for the same reason the menu
   * does, and DX mode first for the same reason: closing it ends an RDS
   * screen held over it without building the DX page back for one frame. */
  screenTaskDxClose();
  screenTaskRdsClose();
  screenTaskTouchCalClose();
  /*
   * The menu goes first, wherever it was, and it goes at once.
   *
   * A write needs the panel and the radio layout under it, and an edit in
   * flight is cancelled rather than kept: nobody accepted it and the radio is
   * about to be replaced. `menuTaskClose` puts the edit back and takes the
   * menu down at once. `menuDownNow` is called as well, because it also ends
   * a swap part way through when no menu was open.
   */
  menuTaskClose();
  menuDownNow();
  if (!sReady) {
    return;
  }
  updateScreenBegin(&sUpdate);
  /* The poll is about to stop running, and it is what drives the light. A
   * panel left dim, or part way through a fade, would show the one screen
   * that has to be readable at whatever brightness it happened to be on. */
  uint32_t nowMs = millis();
  backlightWake(&sBacklight, nowMs);
  writeBacklight(backlightUpdate(&sBacklight, nowMs));
  /* The progress screen from the start, so "Do not remove power" is there
   * before the first byte; its number comes with the first percentage. */
  screenUpdateVeilShow(-1);
  lvglPortRefreshNow();
}

/* The sleep message is up, screenTaskSleepShow. */
static bool sSleepShowing = false;

void screenTaskSleepShow(bool on) {
  if (!on) {
    /* The next poll draws the radio screen, which takes the message down. */
    sSleepShowing = false;
    return;
  }
  screenTaskDxClose();
  screenTaskRdsClose();
  screenTaskBwClose();
  screenTaskTouchCalClose();
  menuTaskClose();
  menuDownNow();
  if (!sReady) {
    return;
  }
  sSleepShowing = true;
  const uint32_t nowMs = millis();
  backlightWake(&sBacklight, nowMs);
  writeBacklight(backlightUpdate(&sBacklight, nowMs));
  screenMessage(txt(STR_RADIO_GOING_TO_SLEEP),
                txt(STR_RADIO_PRESS_KNOB_TO_WAKE));
  lvglPortRefreshNow();
}

bool screenTaskSleepShowing(void) {
  return sSleepShowing;
}

void screenTaskUpdateProgress(int percent) {
  if (!sReady) {
    return;
  }
  /* Drawn at most once per whole number. Every chunk is a few kilobytes, so
   * at a megabyte and a half this would otherwise repaint hundreds of times
   * to say the same thing, on the task the transfer is running on. */
  int shown = 0;
  if (!updateScreenProgress(&sUpdate, percent, &shown)) {
    return;
  }
  screenUpdateVeilShow(shown);
  lvglPortRefreshNow();
}

bool screenTaskUpdateSkip(void) {
  return updateScreenPress(&sUpdate, millis());
}

bool screenTaskUpdateHolding(void) {
  return updateScreenHolds(&sUpdate, millis());
}

void screenTaskUpdateEnd(bool ok) {
  if (!sReady) {
    return;
  }
  updateScreenFinished(&sUpdate, ok, millis(), UPDATE_FAILED_HOLD_MS);
  if (ok) {
    /* The veil's own last frame, the bar full and the percentage at 100,
     * is left on screen rather than swapped for a plain "RESTARTING"
     * message: the write is over, there is nothing left to watch move,
     * and a second screen for the few hundred milliseconds before
     * `ESP.restart()` fires said nothing the veil had not already. */
    return;
  }
  updateScreenShow(txt(STR_RADIO_UPDATE_FAILED), txt(STR_RADIO_OLD_IMAGE_KEPT));
}

void screenTaskBootStep(BootStep step, bool ok, const char *value) {
  if (!sBootUp || (uint8_t)step >= SCREEN_BOOT_STEPS) {
    return;
  }
  ScreenBootStep *row = &sBoot.steps[(uint8_t)step];
  /* Counted once. A step reported twice, which a retry would do, must not
   * push the bar past the end or the count past its total. */
  if (row->mark == SCREEN_BOOT_WAITING && sBoot.done < sBoot.total) {
    sBoot.done++;
  }
  row->mark = ok ? SCREEN_BOOT_OK : SCREEN_BOOT_FAILED;
  if (value != NULL) {
    snprintf(sBootValue[(uint8_t)step], sizeof(sBootValue[0]), "%s", value);
    row->value = sBootValue[(uint8_t)step];
  } else {
    row->value = NULL;
  }
  screenBootShow(&sBoot);
  /* Every step, because `setup` is what is running and the poll that would
   * otherwise draw this is minutes away in radio terms. */
  lvglPortRefreshNow();
}

void screenTaskBootAbsent(BootStep step) {
  if (!sBootUp || (uint8_t)step >= SCREEN_BOOT_STEPS) {
    return;
  }
  ScreenBootStep *row = &sBoot.steps[(uint8_t)step];
  if (row->name == NULL) {
    return;
  }
  row->name = NULL;
  row->value = NULL;
  row->mark = SCREEN_BOOT_WAITING;
  if (sBoot.total > 0) {
    sBoot.total--;
  }
  screenBootShow(&sBoot);
  lvglPortRefreshNow();
}

void screenTaskBootTuner(const char *text) {
  if (!sBootUp) {
    return;
  }
  if (text == NULL) {
    sBoot.tuner = NULL;
  } else {
    snprintf(sBootTuner, sizeof(sBootTuner), "%s", text);
    sBoot.tuner = sBootTuner;
  }
  screenBootShow(&sBoot);
  lvglPortRefreshNow();
}

bool screenTaskBootSkip(void) {
  if (sBootUp && sBootHolding && sSwap == BOOT_SWAP_NONE) {
    sBootHoldUntilMs = millis(); /* The next poll starts the swap. */
  }
  return sBootUp || sSwap != BOOT_SWAP_NONE;
}

bool screenTaskBootShowing(void) {
  return sBootUp || sSwap != BOOT_SWAP_NONE;
}

void screenTaskBootEnd(void) {
  if (!sBootUp || sBootHolding) {
    return;
  }
  /* Start the hold rather than tearing the screen down. `setup` has finished
   * and the loop is about to run, so from here the radio works and the panel
   * is the only thing still showing start up. */
  sBootHolding = true;
  sBootHoldUntilMs = millis() + BOOT_HOLD_MS;
}

/*
 * Take the boot screen down and build the radio layout.
 *
 * In this order, so only one of the two is ever in the LVGL pool. The pool
 * cannot hold both.
 */
static void bootHandOver(void) {
  sBootUp = false;
  sBootHolding = false;
  screenBootEnd();
  sReady = screenBegin();
  if (!sReady) {
    Serial.println(F("[screen] the screens could not be built"));
  }
}

bool screenTaskMenuBegin(void) {
  if (sMenuUp) {
    return true;
  }
  /* The DX page, the bandwidth page or the RDS screen goes first if it is
   * up. One screen in the pool. DX mode first: closing it ends a bandwidth
   * page or an RDS screen held over it without building the DX page back for
   * one frame. */
  screenTaskDxClose();
  screenTaskBwClose();
  screenTaskRdsClose();
  screenTaskTouchCalClose();
  if (!sReady) {
    /* No radio layout means the panel never came up, and a menu drawn on a
     * panel nobody can see is settings changed blind. */
    return false;
  }
  if (sSwap != BOOT_SWAP_NONE) {
    /* The start up fade is still running. One screen change at a time, or the
     * light ends up driven by two things at once. */
    return false;
  }
  /* At once, with no fade. The caller draws as soon as this returns. */
  const uint32_t began = millis();
  /* The menu's minute alone starts now, and the light comes up. A menu a
   * person opened was opened by a key, which did both already; the offer of
   * a newer release opens by itself, maybe long after the last key, and
   * would otherwise shut again at once, or sit on a dimmed panel. */
  sLastActivityMs = began;
  backlightWake(&sBacklight, began);
  screenEnd();
  sReady = false;
  if (!screenMenuBegin()) {
    sReady = screenBegin();
    return false;
  }
  sMenuUp = true;
  sSwapActMs = (uint16_t)(millis() - began);
  return true;
}

void screenTaskMenuDrawn(void) {
  if (!sMenuUp) {
    return;
  }
  lvglPortRefreshNow();
}

void screenTaskMenuEnd(void) {
  /* At once, the same as opening. `menuDownNow` also puts the light back if a
   * fade was somehow part way through. */
  menuDownNow();
}

/*
 * Do the screen change itself, where nothing can be seen.
 *
 * One change fades: start up handing the panel to the radio. The menu and the
 * RDS screen change on the spot, because every length of fade tried for the
 * menu read as a wait.
 *
 * Two screens cannot exist at once whatever the timing: the LVGL pool is a
 * static array in DRAM, 24 KB of it, and it cannot be grown enough to hold
 * both. See LV_MEM_SIZE in lv_conf.h, where the link failure is recorded.
 */
static void swapAct(void) {
  const uint32_t began = millis();
  bootHandOver();
  sSwapActMs = (uint16_t)(millis() - began);
}

ScreenDxOpenResult screenTaskDxCanOpen(void) {
  if (sDxUp) {
    return SCREEN_DX_OPEN;
  }
  RadioSnapshot now;
  if (!radioGetSnapshot(&now)) {
    return SCREEN_DX_RADIO_BUSY;
  }
  return bandModulation(now.settings.band) == MODULATION_FM ? SCREEN_DX_OPEN
                                                            : SCREEN_DX_NOT_FM;
}

ScreenDxOpenResult screenTaskDxOpen(void) {
  if (sDxUp) {
    return SCREEN_DX_OPEN;
  }
  const ScreenDxOpenResult can = screenTaskDxCanOpen();
  if (can != SCREEN_DX_OPEN) {
    return can;
  }
  screenTaskBwClose();
  screenTaskRdsClose();
  if (!sReady || sMenuUp || sSwap != BOOT_SWAP_NONE) {
    return SCREEN_DX_PANEL_BUSY;
  }
  screenEnd();
  sReady = false;
  if (!screenDxBegin()) {
    sReady = screenBegin();
    return SCREEN_DX_PANEL_BUSY;
  }
  sDxUp = true;
  sDxPage = SCREEN_DX_PAGE_DX;
  sDxCursor = 0;
  /* Every entry starts on the DX SETUP menu's width, and a fresh history. The
   * width chosen with BW lasts until DX mode is left. */
  sDxWidthKHz = dxTaskOpenWidth();
  sDxWidthDirty = true;
  dxTaskSetWidth(sDxWidthKHz);
  dxTaskRestart();
  screenTaskDxReset();
  (void)screenTaskDxDraw(sDxPage, &sDxCursor);
  lvglPortRefreshNow();
  return SCREEN_DX_OPEN;
}

static bool dxPageBegin(uint8_t page) {
  switch (page) {
    case SCREEN_DX_PAGE_SCOPE:
      return screenScopeBegin();
    case SCREEN_DX_PAGE_SCAN:
      return screenScanBegin();
    case SCREEN_DX_PAGE_CATCHES:
      return screenCatchesBegin();
    default:
      return screenDxBegin();
  }
}

static void dxPageEnd(uint8_t page) {
  switch (page) {
    case SCREEN_DX_PAGE_SCOPE:
      screenScopeEnd();
      break;
    case SCREEN_DX_PAGE_SCAN:
      screenScanEnd();
      break;
    case SCREEN_DX_PAGE_CATCHES:
      screenCatchesEnd();
      break;
    default:
      screenDxEnd();
      break;
  }
}

/* Out of DX mode, with no DX page left on the panel, and the radio screen
 * back. The width goes back from the next poll. */
static void dxLeaveToRadioScreen(void) {
  sDxUp = false;
  sDxWidthDirty = true;
  dxTaskLeave();
  sReady = screenBegin();
  if (!sReady) {
    Serial.println(F("[screen] the radio screen could not be rebuilt"));
    return;
  }
  sLastPollMs = millis() - SCREEN_POLL_MS;
}

/* The DX page back once a screen held over it goes, or out of DX mode if the
 * pool cannot hold the page. */
static void dxPageBack(void) {
  if (!dxPageBegin(sDxPage)) {
    dxLeaveToRadioScreen();
    return;
  }
  (void)screenTaskDxDraw(sDxPage, &sDxCursor);
  lvglPortRefreshNow();
}

void screenTaskDxClose(void) {
  if (!sDxUp) {
    return;
  }
  if (sBw != NULL) {
    /* The bandwidth page holds the panel over the DX page, so it is what
     * goes, and the DX page is not there to end. */
    screenBwEnd();
    free(sBw);
    sBw = NULL;
  } else if (sRdsUp) {
    /* The RDS screen, held over the DX page the same way. */
    sRdsUp = false;
    screenRdsEnd();
  } else {
    dxPageEnd(sDxPage);
  }
  dxLeaveToRadioScreen();
}

bool screenTaskDxIsOpen(void) {
  return sDxUp;
}

uint8_t screenTaskDxPage(void) {
  return sDxPage;
}

static void dxShowPage(uint8_t page) {
  if (sBw != NULL || sRdsUp) {
    /* The bandwidth page or the RDS screen holds the panel: the page asked
     * for comes up when it closes. */
    sDxPage = page;
    sDxCursor = 0;
    screenTaskDxScopeReset();
    return;
  }
  dxPageEnd(sDxPage);
  sDxPage = page;
  /* The Catches page opens on the newest catch, the station most likely
   * still playing, not on whichever one the cursor followed down, and the
   * Scope page on the dial's channel. */
  sDxCursor = 0;
  screenTaskDxScopeReset();
  if (!dxPageBegin(sDxPage)) {
    /* The pool could not hold the page. Out of DX mode altogether, rather
     * than a DX width in force behind a screen that is not there. */
    dxLeaveToRadioScreen();
    return;
  }
  (void)screenTaskDxDraw(sDxPage, &sDxCursor);
  lvglPortRefreshNow();
}

void screenTaskDxNextPage(void) {
  screenTaskDxStepPage(1);
}

void screenTaskDxStepPage(int dir) {
  if (sDxUp) {
    dxShowPage((uint8_t)((sDxPage + SCREEN_DX_PAGES + (dir < 0 ? -1 : 1)) %
                         SCREEN_DX_PAGES));
  }
}

DxScanPress screenTaskDxLearnLocals(void) {
  RadioSnapshot now;
  if (!radioGetSnapshot(&now)) {
    return DX_SCAN_PRESS_NO_RADIO;
  }
  if (bandModulation(now.settings.band) != MODULATION_FM) {
    return DX_SCAN_PRESS_NOT_FM;
  }
  if (!radioRdsEnabled()) {
    return DX_SCAN_PRESS_RDS_OFF;
  }
  if (bandScanActive() || dxTaskScan()->state == DX_SCAN_RUNNING) {
    return DX_SCAN_PRESS_BUSY;
  }
  const bool opened = !sDxUp;
  if (opened && screenTaskDxOpen() != SCREEN_DX_OPEN) {
    return DX_SCAN_PRESS_PANEL_BUSY;
  }
  const DxScanPress r = dxTaskLearn();
  if (r == DX_SCAN_PRESS_RUNNING) {
    screenTaskDxShowScanner();
  } else if (opened) {
    /* Refused after all: out again, rather than left in DX mode on its
     * narrow width with nothing running. */
    screenTaskDxClose();
  }
  return r;
}

void screenTaskDxShowScanner(void) {
  if (sDxUp && sDxPage != SCREEN_DX_PAGE_SCAN) {
    dxShowPage(SCREEN_DX_PAGE_SCAN);
  }
}

void screenTaskDxShowScope(void) {
  if (sDxUp && sDxPage != SCREEN_DX_PAGE_SCOPE) {
    dxShowPage(SCREEN_DX_PAGE_SCOPE);
  }
}

void screenTaskDxTurn(int32_t clicks) {
  if (!sDxUp || clicks == 0) {
    return;
  }
  if (sDxPage == SCREEN_DX_PAGE_SCOPE) {
    /* A channel a click, the cursor stopping at both ends. */
    screenTaskDxScopeTurn(clicks);
    (void)screenTaskDxDraw(sDxPage, &sDxCursor);
    lvglPortRefreshNow();
    return;
  }
  if (sDxPage != SCREEN_DX_PAGE_CATCHES) {
    return;
  }
  /* A row a click, stopping at both ends. */
  const DxCatches *list = dxTaskCatches();
  const uint8_t count = list != NULL ? list->count : 0;
  sDxCursor = count == 0 ? 0
                         : (uint8_t)std::clamp<int32_t>(sDxCursor + clicks, 0,
                                                        count - 1);
  (void)screenTaskDxDraw(sDxPage, &sDxCursor);
  lvglPortRefreshNow();
}

bool screenTaskDxCursorCatch(DxCatch *out) {
  const DxCatches *list = dxTaskCatches();
  if (!sDxUp || sDxPage != SCREEN_DX_PAGE_CATCHES || list == NULL ||
      sDxCursor >= list->count) {
    return false;
  }
  if (out != NULL) {
    *out = list->item[sDxCursor];
  }
  return true;
}

uint8_t screenTaskDxCursor(void) {
  return sDxCursor;
}

DxWriteResult screenTaskDxLogCursor(void) {
  return screenTaskDxCursorCatch(NULL) ? dxTaskLogCatch(sDxCursor)
                                       : DX_WRITE_NO_CATCH;
}

bool screenTaskDxSetWidth(uint16_t khz) {
  if (!sDxUp || khz == 0 || !bandBandwidthAllowed(BAND_FM, khz)) {
    return false;
  }
  sDxWidthKHz = khz;
  sDxWidthDirty = true;
  dxTaskSetWidth(khz);
  return true;
}

uint16_t screenTaskDxWidth(void) {
  return sDxUp ? sDxWidthKHz : 0;
}

uint16_t screenTaskDxCycleWidth(void) {
  if (!sDxUp) {
    return 0;
  }
  /* The next of the tuner's fixed widths. The automatic 0 is the one the
   * FM list starts with, and it is skipped: DX mode is a fixed width. */
  uint16_t next = bandBandwidthNext(BAND_FM, sDxWidthKHz);
  if (next == 0) {
    next = bandBandwidthNext(BAND_FM, next);
  }
  sDxWidthKHz = next;
  sDxWidthDirty = true;
  dxTaskSetWidth(next);
  return next;
}

/* Fill in the bandwidth page from the radio and draw it. False, and the
 * page closed, when the radio has left the band the tiles are for. */
static bool bwDraw(void) {
  RadioSnapshot now;
  if (sBw == NULL || !radioGetSnapshot(&now)) {
    return true;
  }
  if (now.settings.band != sBw->band) {
    screenTaskBwClose();
    return false;
  }
  char clockText[CLOCK_TEXT_LEN];
  ScreenBwInputs in;
  memset(&in, 0, sizeof(in));
  in.tiles = sBw->tiles;
  in.count = sBw->count;
  in.cursor = sBw->cursor;
  in.band = sBw->band;
  in.dxMode = sBw->overDx;
  in.widthKHz = sBw->overDx ? sDxWidthKHz : now.settings.bandwidthKHz;
  in.chipKnown = now.qualityValid;
  in.chipKHz = now.quality.bandwidthKHz;
  in.ims = now.settings.multipathSuppression;
  in.eq = now.settings.equalizer;
  in.clock = clockFormat(ntpLocalTime(), clockText, sizeof(clockText))
                 ? clockText
                 : NULL;
  const bool sameClock =
      (in.clock == NULL) == (sBw->shownClock[0] == '\0') &&
      (in.clock == NULL || strcmp(in.clock, sBw->shownClock) == 0);
  if (sBw->drawn && sameClock && sBw->shownSleep == uiSleepMark() &&
      in.cursor == sBw->shown.cursor && in.widthKHz == sBw->shown.widthKHz &&
      in.chipKnown == sBw->shown.chipKnown &&
      in.chipKHz == sBw->shown.chipKHz && in.ims == sBw->shown.ims &&
      in.eq == sBw->shown.eq) {
    return true;
  }
  sBw->shown = in;
  sBw->shownSleep = uiSleepMark();
  snprintf(sBw->shownClock, sizeof(sBw->shownClock), "%s",
           in.clock != NULL ? in.clock : "");
  sBw->drawn = true;
  ScreenBw view;
  screenBwStateBuild(&in, &sBw->keep, &view);
  screenBwShow(&view);
  return true;
}

bool screenTaskBwOpen(void) {
  if (sBw != NULL) {
    return true;
  }
  if (sMenuUp || sSwap != BOOT_SWAP_NONE) {
    return false;
  }
  screenTaskRdsClose();
  if (!sReady && !sDxUp) {
    return false;
  }
  RadioSnapshot now;
  if (!radioGetSnapshot(&now)) {
    return false;
  }
  BwStore *s = (BwStore *)calloc(1, sizeof(*s));
  if (s == NULL) {
    Serial.println(F("[screen] no memory for the bandwidth page"));
    return false;
  }
  s->band = now.settings.band;
  s->overDx = sDxUp;
  s->count = bwPageTiles(s->band, sDxUp, s->tiles, BW_PAGE_MAX);
  if (s->count == 0) {
    free(s);
    return false;
  }
  s->cursor = bwPageStart(s->tiles, s->count,
                          sDxUp ? sDxWidthKHz : now.settings.bandwidthKHz);
  /* The panel: from the DX page or from the radio screen, and back to it
   * if the page cannot be built. */
  if (sDxUp) {
    dxPageEnd(sDxPage);
  } else {
    screenEnd();
    sReady = false;
  }
  if (!screenBwBegin()) {
    if (sDxUp) {
      if (!dxPageBegin(sDxPage)) {
        dxLeaveToRadioScreen();
      }
    } else {
      sReady = screenBegin();
    }
    free(s);
    return false;
  }
  sBw = s;
  (void)bwDraw();
  lvglPortRefreshNow();
  return true;
}

void screenTaskBwClose(void) {
  if (sBw == NULL) {
    return;
  }
  const bool overDx = sBw->overDx;
  screenBwEnd();
  free(sBw);
  sBw = NULL;
  if (overDx && sDxUp) {
    dxPageBack();
    return;
  }
  sReady = screenBegin();
  if (!sReady) {
    Serial.println(F("[screen] the radio screen could not be rebuilt"));
    return;
  }
  sLastPollMs = millis() - SCREEN_POLL_MS;
}

bool screenTaskBwIsOpen(void) {
  return sBw != NULL;
}

void screenTaskTouchCalView(const TouchCalFlow *f, ScreenTouchCal *out) {
  if (f == NULL || out == NULL) {
    return;
  }
  ScreenTouchCal v;
  memset(&v, 0, sizeof(v));
  switch (f->step) {
    case TOUCH_CAL_MARK:
      v.step = SCREEN_TOUCH_CAL_MARK;
      break;
    case TOUCH_CAL_CHECK:
      v.step = SCREEN_TOUCH_CAL_CHECK;
      break;
    case TOUCH_CAL_KEPT:
      v.step = SCREEN_TOUCH_CAL_KEPT;
      break;
    case TOUCH_CAL_NO_FIT:
      v.step = SCREEN_TOUCH_CAL_NO_FIT;
      break;
    case TOUCH_CAL_NOT_SAVED:
      v.step = SCREEN_TOUCH_CAL_NOT_SAVED;
      break;
    default:
      v.step = SCREEN_TOUCH_CAL_MISSED;
      break;
  }
  v.mark =
      f->mark < SCREEN_TOUCH_CAL_MARKS ? f->mark : SCREEN_TOUCH_CAL_MARKS - 1;
  v.fillPct = (uint8_t)(f->filled * 100 / TOUCH_CAL_FILL);
  for (uint8_t i = 0; i < SCREEN_TOUCH_CAL_MARKS; i++) {
    const TouchPoint m = touchCalFlowMark(f, i);
    v.markX[i] = m.x;
    v.markY[i] = m.y;
  }
  const TouchPoint dot = touchCalFlowCheckDot(f);
  v.dotX = dot.x;
  v.dotY = dot.y;
  v.offPx = f->checkOffPx;
  *out = v;
}

/* What the calibration shows, from where it has got to. */
static void calDraw(void) {
  if (sCal == NULL) {
    return;
  }
  ScreenTouchCal v;
  screenTaskTouchCalView(&sCal->flow, &v);
  screenTouchCalShow(&v);
}

/* A calibration started, with the map in use as its guide. */
static bool calStart(CalStore *c) {
  TouchCal guide;
  bool upsideDown = false;
  if (!inputTouchCalGet(&guide, &upsideDown)) {
    return false;
  }
  touchCalFlowBegin(&c->flow, DISPLAY_WIDTH, DISPLAY_HEIGHT, upsideDown,
                    &guide);
  c->changed = true;
  return true;
}

bool screenTaskTouchCalOpen(void) {
  if (sCal != NULL) {
    return true;
  }
  if (!sReady || sMenuUp || sDxUp || sRdsUp || sBw != NULL ||
      sSwap != BOOT_SWAP_NONE) {
    return false;
  }
  CalStore *c = (CalStore *)calloc(1, sizeof(*c));
  if (c == NULL) {
    Serial.println(F("[screen] no memory for the touch calibration"));
    return false;
  }
  if (!calStart(c)) {
    free(c);
    return false;
  }
  screenEnd();
  sReady = false;
  if (!screenTouchCalBegin(false)) {
    sReady = screenBegin();
    free(c);
    return false;
  }
  sCal = c;
  calDraw();
  lvglPortRefreshNow();
  return true;
}

bool screenTaskTouchCalIsOpen(void) {
  return sCal != NULL;
}

bool screenTaskTouchCalStep(TouchCalFlow *f, bool contact, bool fresh,
                            TouchPoint raw, bool unsettled, uint32_t nowMs) {
  /* The flow changes nothing more once it is kept, so this saves once. */
  const bool changed =
      touchCalFlowFeed(f, contact, fresh, raw, unsettled, nowMs);
  if (changed && f->step == TOUCH_CAL_KEPT && !touchCalNvsSave(&f->result)) {
    f->step = TOUCH_CAL_NOT_SAVED;
  }
  return changed;
}

void screenTaskTouchCalFeed(bool contact, bool fresh, TouchPoint raw,
                            bool unsettled, uint32_t nowMs) {
  if (sCal == NULL) {
    return;
  }
  /* The rotation changed from the web page while marking or checking: the
   * marks now show turned, and the readings so far were taken against the
   * old places, so it starts again. */
  bool upsideDown = false;
  (void)inputTouchCalGet(NULL, &upsideDown);
  const TouchCalStep step = sCal->flow.step;
  if ((step == TOUCH_CAL_MARK || step == TOUCH_CAL_CHECK) &&
      upsideDown != sCal->flow.upsideDown) {
    (void)calStart(sCal);
    return;
  }
  if (!screenTaskTouchCalStep(&sCal->flow, contact, fresh, raw, unsettled,
                              nowMs)) {
    return;
  }
  sCal->changed = true;
  if (sCal->flow.step == TOUCH_CAL_KEPT) {
    /* Saved, so in use from now. */
    inputTouchCalSet(&sCal->flow.result, true);
  }
}

void screenTaskTouchCalKnob(bool press) {
  if (sCal == NULL) {
    return;
  }
  if (press && touchCalFlowFailed(&sCal->flow)) {
    (void)calStart(sCal);
    return;
  }
  screenTaskTouchCalClose();
}

void screenTaskTouchCalClose(void) {
  if (sCal == NULL) {
    return;
  }
  screenTouchCalEnd();
  free(sCal);
  sCal = NULL;
  sReady = screenBegin();
  if (!sReady) {
    Serial.println(F("[screen] the radio screen could not be rebuilt"));
    return;
  }
  sLastPollMs = millis() - SCREEN_POLL_MS;
}

void screenTaskBwTurn(int32_t clicks) {
  if (sBw == NULL || clicks == 0) {
    return;
  }
  sBw->cursor = bwPageMove(sBw->cursor, clicks, sBw->count);
  if (bwDraw()) {
    lvglPortRefreshNow();
  }
}

void screenTaskBwTap(uint8_t index) {
  if (sBw == NULL || index >= sBw->count) {
    return;
  }
  screenTaskBwTurn((int32_t)index - sBw->cursor);
  screenTaskBwPick();
}

void screenTaskBwPick(void) {
  if (sBw == NULL || sBw->cursor >= sBw->count) {
    return;
  }
  const BwTile *t = &sBw->tiles[sBw->cursor];
  RadioCommand cmd = {};
  if (t->kind == BW_TILE_WIDTH) {
    if (sBw->overDx) {
      /* DX mode's own width, not the radio's. */
      (void)screenTaskDxSetWidth(t->khz);
      return;
    }
    cmd.kind = RADIO_SET_BANDWIDTH;
    cmd.bandwidthKHz = t->khz;
  } else {
    RadioSnapshot now;
    if (!radioGetSnapshot(&now)) {
      return;
    }
    const bool ims = t->kind == BW_TILE_IMS;
    cmd.kind = ims ? RADIO_SET_MPH_SUPPRESSION : RADIO_SET_EQUALIZER;
    cmd.on = ims ? !now.settings.multipathSuppression : !now.settings.equalizer;
  }
  (void)radioPost(&cmd);
}

void screenTaskRdsToggle(void) {
  if (sRdsUp) {
    screenTaskRdsClose();
    return;
  }
  screenTaskBwClose();
  if ((!sReady && !sDxUp) || sMenuUp || sSwap != BOOT_SWAP_NONE) {
    /* The panel belongs to something else, or is part way through a change.
     * One screen at a time, always. */
    return;
  }
  /* RDS is an FM service: on the AM bands every page would be empty. */
  RadioSnapshot now;
  if (radioGetSnapshot(&now) &&
      bandModulation(now.settings.band) != MODULATION_FM) {
    screenTaskLogConfirm(txt(STR_MENU_NOTE_SWITCH_TO_FM));
    return;
  }
  /* From the radio screen, or over DX mode, which stays open underneath at
   * its own width and comes back on the page it was on. */
  if (sDxUp) {
    dxPageEnd(sDxPage);
  } else {
    screenEnd();
    sReady = false;
  }
  if (!screenRdsBegin()) {
    if (sDxUp) {
      dxPageBack();
    } else {
      sReady = screenBegin();
    }
    return;
  }
  sRdsUp = true;
  /* Always opens on page one, and its own timers start fresh: a station
   * looked at, left, and looked at again should not report a lock held
   * since the first visit. */
  sRdsPage = 0;
  screenTaskRdsRestart();
  screenTaskRdsDraw(sRdsPage);
  lvglPortRefreshNow();
}

void screenTaskRdsClose(void) {
  if (!sRdsUp) {
    return;
  }
  sRdsUp = false;
  screenRdsEnd();
  if (sDxUp) {
    dxPageBack();
    return;
  }
  sReady = screenBegin();
  if (!sReady) {
    Serial.println(F("[screen] the radio screen could not be rebuilt"));
    return;
  }
  sLastPollMs = millis() - SCREEN_POLL_MS;
}

void screenTaskRdsPage(int32_t clicks) {
  if (!sRdsUp || clicks == 0) {
    return;
  }
  /* One click, one page, the same rule the menu turns the knob by: a page
   * is a place, not a distance, and acceleration would carry a fast spin
   * straight past the one a person meant to land on. */
  const int32_t step = clicks < 0 ? -1 : 1;
  int32_t page = (int32_t)sRdsPage + step;
  if (page < 0) {
    page = SCREEN_RDS_PAGES - 1;
  } else if (page >= SCREEN_RDS_PAGES) {
    page = 0;
  }
  sRdsPage = (uint8_t)page;
  screenTaskRdsDraw(sRdsPage);
  lvglPortRefreshNow();
}

bool screenTaskRdsIsOpen(void) {
  return sRdsUp;
}

const char *screenTaskShowing(uint8_t *page) {
  uint8_t p = 0;
  const char *name = "radio";
  if (sBootUp) {
    name = "boot";
  } else if (menuTaskIsOpen()) {
    name = "menu";
  } else if (sCal != NULL) {
    name = "touch-calibration";
  } else if (screenTaskBwIsOpen()) {
    name = "bandwidth";
  } else if (sRdsUp) {
    name = "rds";
    p = sRdsPage;
  } else if (screenTaskDxIsOpen()) {
    name = "dx";
    p = screenTaskDxPage();
  }
  if (page != NULL) {
    *page = p;
  }
  return name;
}

/*
 * A preset with no PI learns the one confirmed on it, so it can later tell
 * its own station from another on the same channel. Twice a second: a PI
 * takes seconds to be confirmed, and the read is a copy under the store's
 * lock. The store writes to flash on its own, after the list has been
 * still for a while, and a slot learns once.
 */
#define PRESET_LEARN_MS 500

static void presetLearnPoll(uint32_t nowMs) {
  static uint32_t lastMs = 0;
  if ((uint32_t)(nowMs - lastMs) < PRESET_LEARN_MS) {
    return;
  }
  lastMs = nowMs;
  /* Not while an update is on trial: learning writes the list in its new
   * form, which the firmware a rollback goes back to may not read. */
  if (rollbackPending()) {
    return;
  }
  RadioSnapshot snap;
  if (!radioGetSnapshot(&snap) || snap.memorySlot == MEMORY_NO_SLOT) {
    return;
  }
  MemoryChannel channel;
  if (!memoryStoreRead(snap.memorySlot, &channel)) {
    return;
  }
  const SeekReading reading =
      radioSeekReading(&snap.quality, snap.qualityValid);
  uint16_t pi = 0;
  if (!dxPresetLearn(&snap.rds, &reading, channel.pi, &pi)) {
    return;
  }
  channel.pi = pi;
  if (memoryStoreWrite(snap.memorySlot, &channel)) {
    Serial.printf("[memory] P%02d learnt PI %04X\n", snap.memorySlot + 1,
                  (unsigned)pi);
  }
}

/*
 * The screen on the panel again in the theme now in use, kept where it was:
 * the same DX page and cursor, the same RDS page, the same bandwidth tile,
 * and a DX scan or sweep still running, since only the drawing is rebuilt.
 *
 * The radio screen sees a new theme for itself and rebuilds. The others take
 * their fills, borders and fixed texts as they open, so without this a
 * screen that is up when the hour turns to night keeps the day's colours
 * for everything it does not redraw.
 * The menu needs nothing: it closes these screens as it opens, and closing
 * it builds the radio screen fresh. A screen the pool cannot hold again
 * falls back the way closing it does.
 */
static void reopenForTheme(void) {
  if (sCal != NULL) {
    screenTouchCalEnd();
    if (screenTouchCalBegin(false)) {
      sCal->changed = true;
      return;
    }
    screenTaskTouchCalClose();
    return;
  }
  if (sBw != NULL) {
    screenBwEnd();
    if (screenBwBegin()) {
      sBw->drawn = false;
      (void)bwDraw();
      return;
    }
    const bool overDx = sBw->overDx;
    free(sBw);
    sBw = NULL;
    if (overDx && sDxUp) {
      dxPageBack();
    } else {
      sReady = screenBegin();
    }
    return;
  }
  if (sRdsUp) {
    screenRdsEnd();
    if (screenRdsBegin()) {
      screenTaskRdsDraw(sRdsPage);
      return;
    }
    sRdsUp = false;
    if (sDxUp) {
      dxPageBack();
    } else {
      sReady = screenBegin();
    }
    return;
  }
  if (sDxUp) {
    dxPageEnd(sDxPage);
    dxPageBack();
  }
}

void screenTaskPoll(void) {
  if (sBootUp && sSwap == BOOT_SWAP_NONE) {
    /* Signed subtraction, so the millis wrap is one more pass round rather
     * than a boot screen held for another forty nine days. */
    if (!sBootHolding || (int32_t)(millis() - sBootHoldUntilMs) < 0) {
      /* Still being read. LVGL is still stepped, because a screen nobody
       * services is frozen for real rather than held. */
      lvglPortPoll();
      return;
    }
    swapBegin();
  }

  if (sSwap == BOOT_SWAP_OUT) {
    const uint32_t gone = millis() - sSwapFromMs;
    if (gone < BOOT_SWAP_OUT_MS) {
      writeBacklight((uint8_t)((uint32_t)sSwapLevel *
                               (BOOT_SWAP_OUT_MS - gone) / BOOT_SWAP_OUT_MS));
      lvglPortPoll();
      return;
    }
    /* Dark. Change screens where nothing can be seen. */
    writeBacklight(0);
    swapAct();
    sSwap = BOOT_SWAP_IN;
    sSwapFromMs = millis();
  }

  /* Before anything that can return early: a DX width left in force with
   * no DX page up would be a filter nobody can see, even on a panel that
   * failed to come back. */
  if (sDxWidthDirty) {
    RadioCommand width = {};
    width.kind = RADIO_SET_DX_BANDWIDTH;
    width.bandwidthKHz = sDxUp ? sDxWidthKHz : 0;
    if (radioPost(&width)) {
      sDxWidthDirty = false;
    }
  }

  /* `sReady` means the radio layout exists, and it does not while another
   * screen owns the panel. So every screen counts as ready here, or the fade
   * back in and the redraw below would never run for it. */
  if (!sReady && !sMenuUp && !sRdsUp && !sDxUp && sBw == NULL && sCal == NULL) {
    return;
  }
  uint32_t nowMs = millis();
  presetLearnPoll(nowMs);

  /*
   * A firmware write owns the panel. The poll does not run during the
   * transfer on either route, but a failed write holds its message for a few
   * seconds after it, and redrawing the radio would hide that message.
   */
  if (updateScreenHolds(&sUpdate, nowMs) || sSleepShowing) {
    /* The radio is not redrawn, but LVGL still runs. It has a flush to finish
     * and a refresh timer to service, and a screen nobody is stepping is a
     * screen that is frozen for real rather than held. */
    lvglPortPoll();
    return;
  }

  if (sFirstPoll) {
    sFirstPoll = false;
    backlightWake(&sBacklight, nowMs);
  }

  /* Checked every time round the loop, not on the poll interval below. A
   * press that had to wait up to forty milliseconds for the light to come
   * back would be a press that looked ignored. */
  uint32_t activity = inputActivity();
  if (activity != sLastActivity) {
    sLastActivity = activity;
    sLastActivityMs = nowMs;
    backlightWake(&sBacklight, nowMs);
  }
  /* The menu or the bandwidth page, left alone, goes back to the radio
   * screen the way Back would, an edit nobody accepted undone. Not while a
   * band scan runs, which the menu shows. */
  if ((sMenuUp || sBw != NULL) && !bandScanActive() &&
      inputPageIdle(sLastActivityMs, nowMs)) {
    menuTaskClose();
    screenTaskBwClose();
  }
  if (sSwap == BOOT_SWAP_IN) {
    /*
     * Coming back on the radio screen. The light is driven from here for the
     * length of the ramp and `backlightUpdate` is left alone, because both
     * writing it would mean the ramp and the light's own idea of the level
     * fighting for the same pin.
     */
    const uint32_t back = nowMs - sSwapFromMs;
    if (back < BOOT_SWAP_IN_MS) {
      writeBacklight((uint8_t)((uint32_t)sSwapLevel * back / BOOT_SWAP_IN_MS));
    } else {
      sSwap = BOOT_SWAP_NONE;
      writeBacklight(backlightUpdate(&sBacklight, nowMs));
    }
  } else {
    writeBacklight(backlightUpdate(&sBacklight, nowMs));
  }

  /*
   * Let LVGL work on every pass, not only when the state is rebuilt.
   *
   * Two different rates. Rebuilding the state means taking the radio's lock
   * and formatting a dozen strings, and doing that faster than
   * SCREEN_POLL_MS shows nothing a person could follow.
   * lv_timer_handler is the other half: it steps animations and pushes dirty
   * rectangles. Gating it to the same rate makes the scrolling radio text
   * stutter, because a scroll serviced at 25 Hz behind LVGL's 16 ms refresh
   * period moves in visible jumps.
   *
   * It costs nothing when there is nothing to do: with no animation running
   * and nothing dirty, it looks at its timer list and returns.
   */
  lvglPortPoll();

  /* Every change of theme passes here, from the hour, the menu or the API. */
  static uint32_t drawnTheme = themeGeneration();
  if (themeGeneration() != drawnTheme) {
    drawnTheme = themeGeneration();
    reopenForTheme();
  }

  if (sCal != NULL) {
    /* The touch calibration screen: drawn when the input task's feed
     * changed it. */
    if (sCal->changed) {
      sCal->changed = false;
      calDraw();
      lvglPortRefreshNow();
    }
    return;
  }

  if (sBw != NULL) {
    /* The bandwidth page: redrawn on the radio's cadence, since the width
     * automatic has chosen and the tick move as the radio does. It closes
     * itself if the band changes under it, since its tiles are one band's. */
    if ((uint32_t)(nowMs - sLastPollMs) >= SCREEN_POLL_MS) {
      sLastPollMs = nowMs;
      bwDraw();
    }
    return;
  }

  if (sMenuUp) {
    /* The menu owns the panel, and it draws itself when something moves
     * rather than from here. The light above still runs, so the panel dims
     * and wakes in the menu exactly as it does on the radio screen. */
    return;
  }

  if (sRdsUp) {
    /* The RDS screen is the other way round: it is a view of something that
     * keeps arriving, so it is redrawn on the same cadence the radio screen
     * is. A name takes seconds to come in and the text changes with the
     * song. */
    if ((uint32_t)(nowMs - sLastPollMs) >= SCREEN_POLL_MS) {
      sLastPollMs = nowMs;
      /* Over DX mode, which goes on hearing underneath, and closes with this
       * screen if the radio leaves FM. */
      if (sDxUp && !dxTaskTick()) {
        screenTaskDxClose();
        return;
      }
      screenTaskRdsDraw(sRdsPage);
    }
    return;
  }

  if (sDxUp) {
    /* Redrawn like the RDS screen: the readings and the name keep arriving.
     * The page closes itself if the radio leaves FM, since DX mode has
     * nothing to show there. */
    if ((uint32_t)(nowMs - sLastPollMs) >= SCREEN_POLL_MS) {
      sLastPollMs = nowMs;
      if (!screenTaskDxDraw(sDxPage, &sDxCursor)) {
        screenTaskDxClose();
      }
    }
    return;
  }

  if ((uint32_t)(nowMs - sLastPollMs) < SCREEN_POLL_MS) {
    return;
  }
  sLastPollMs = nowMs;

  RadioSnapshot snap;
  if (!radioGetSnapshot(&snap)) {
    return;
  }

  /*
   * Everything else the state is built from, read here because this is where
   * the other tasks and the drivers are. The builder in screen_state.cpp
   * reaches none of them, so the renderer can run it on a PC.
   */
  ScreenInputs in;
  memset(&in, 0, sizeof(in));
  in.snap = &snap;
  in.planValid = radioTaskPlan(&in.plan);
  in.readChannel = memoryStoreRead;
  in.memoryGeneration = memoryStoreGeneration();
  InputStatus input;
  inputStatusGet(&input);
  in.typed = input.typed;
  /*
   * What the network is doing, in the four states the header can draw.
   *
   * One for one with the state machine, deliberately. The access point had
   * been folded in with not joined, and that is the state where somebody has
   * to go and do something: the radio is serving the page that fixes the
   * credentials, and drawing it as "gave up" hid the one thing worth saying.
   */
  const WifiState link = wifiState();
  in.wifi = link == WIFI_STATE_ONLINE         ? SCREEN_WIFI_JOINED
            : link == WIFI_STATE_JOINING      ? SCREEN_WIFI_TRYING
            : link == WIFI_STATE_ACCESS_POINT ? SCREEN_WIFI_AP
                                              : SCREEN_WIFI_NONE;
  int8_t rssi = 0;
  in.rssiValid = wifiRssiDbm(&rssi);
  in.rssiDbm = rssi;
  /*
   * Only polled while Wi-Fi is not holding ADC2. The Wi-Fi driver takes the pin
   * as soon as its mode is set, which is on the first join attempt, so a read
   * here fails from WIFI_STATE_JOINING onward and stays failing for the rest of
   * the session. `wifiBegin` runs inside `setup`, so the state has already left
   * WIFI_STATE_OFFLINE by the time `loop` calls this the first time; the one
   * reading this screen ever gets from before that point is the boot reading
   * `screenTaskBegin` takes directly, not this poll. This gate only matters for
   * whatever state the radio is in after that, and starts reading again on its
   * own if that is ever OFFLINE.
   */
  if (link == WIFI_STATE_OFFLINE &&
      (uint32_t)(nowMs - sBatteryPollMs) >= BATTERY_POLL_MS) {
    sBatteryPollMs = nowMs;
    uint16_t mv = 0;
    /* Only a reading that arrived is fed in. A failed one is not three
     * strikes towards forgetting what the radio knew at start up, because on
     * this board every read after Wi-Fi comes up fails and the panel would
     * blank three seconds in, every time. */
    if (batteryAdcRead(&mv)) {
      batteryFeed(&sBattery, mv, true);
    }
  }
  in.battery = &sBattery;
  in.batteryShow = sBatteryShow;
  /*
   * The clock, at the far right of the top strip, and only once a server has
   * actually answered.
   *
   * This board has no battery backed clock, so between switching on and the
   * first NTP answer the radio does not know the time. It shows nothing then
   * rather than 00:00, which would look like a working clock at midnight.
   */
  char clockText[CLOCK_TEXT_LEN] = "";
  char dateText[CLOCK_DATE_LEN] = "";
  in.clock = clockFormat(ntpLocalTime(), clockText, sizeof(clockText))
                 ? clockText
                 : NULL;
  in.date = ntpLocalDate(dateText, sizeof(dateText)) ? dateText : NULL;
  in.fault =
      snap.lastError != TEF668X_OK ? tef668xErrorText(snap.lastError) : NULL;
  in.logConfirm = screenTaskLogConfirmText();
  in.notice = updateCheckState() == UPDATE_STATE_CHECKING
                  ? txt(STR_RADIO_CHECKING_UPDATES)
                  : NULL;
  in.levelOffsetDb = screenTaskLevelOffsetDb(snap.settings.band);
  in.nowMs = millis();

  ScreenState state;
  screenStateBuild(&sBuild, &in, &state);
  screenShow(&state);

  /* LVGL draws when it is asked to and not before, so nothing above this line
   * has put a pixel on the panel. */
  lvglPortPoll();

  /* The needle shows what the screen shows. Driven from here rather than from
   * its own place because it is the same job: telling the person holding the
   * radio what the radio is receiving. Smoothed for the same reason, and more
   * so: a needle has weight and a jumping one reads as a loose pointer. */
  smeterShow(snap.qualityValid ? snap.levelSmoothedTenths : 0);
}
