/* Implementation of recovery mode. See recovery.h. */
#include "recovery.h"

#include "board/board.h"
#include "core/input.h"
#include "core/strings.h"
#include "core/touch_cal.h"
#include "core/wifi_join.h"
#include "drivers/display.h"
#include "drivers/encoder.h"
#include "drivers/settings_nvs.h"
#include "drivers/touch.h"
#include "lvgl_port.h"
#include "net/boot_watchdog.h"
#include "screen_task.h"
#include "ui/screen.h"

#include <Arduino.h>
#include <esp_ota_ops.h>
#include <string.h>
#include <algorithm>

/* The seven rows. The last, "Exit and start radio", is the way out every other
 * list in this UI ends with. */
typedef enum {
  RECOVERY_ROW_ROTATE = 0,
  /* Shown on every board: the one board there is, the ATS-125, has touch
   * fitted. */
  RECOVERY_ROW_TOUCH,
  RECOVERY_ROW_CALIBRATE,
  RECOVERY_ROW_HOTSPOT,
  RECOVERY_ROW_ROLLBACK,
  RECOVERY_ROW_ERASE,
  RECOVERY_ROW_EXIT,
} RecoveryRowId;

/* Set when a roll back was asked for and did not happen, so the row can say
 * so rather than sit there as though it were still thinking. */
static bool sRollbackFailed = false;
/* The same for a row whose settings could not be written: a restart would
 * come back on the old settings with nothing to say so. */
static bool sSaveFailed[SCREEN_RECOVERY_ROWS];

static const StrId kRecoveryNames[SCREEN_RECOVERY_ROWS] = {
    STR_RECOVERY_ROTATE_DISPLAY,       STR_RECOVERY_TOUCH,
    STR_RECOVERY_CALIBRATE_TOUCH,      STR_RECOVERY_START_HOTSPOT,
    STR_RECOVERY_ROLL_BACK_FIRMWARE,   STR_RECOVERY_ERASE_SETTINGS,
    STR_RECOVERY_EXIT_AND_START_RADIO,
};

/*
 * What the foot line says while a row waits for its second press. Every row
 * that changes something asks first, since each restarts the radio and
 * Erase Settings loses the Wi-Fi details with the rest; Calibrate Touch and
 * Exit change nothing at once, so they act on the first press.
 */
static const StrId kRecoveryAsk[SCREEN_RECOVERY_ROWS] = {
    STR_RECOVERY_ASK_ROTATE,
    /* With touch on; askLine gives the other way round. */
    STR_RECOVERY_ASK_TOUCH_OFF,
    /* Changes nothing until a calibration passes its check. */
    STR_COUNT,
    STR_RECOVERY_ASK_HOTSPOT,
    STR_RECOVERY_ASK_ROLLBACK,
    STR_RECOVERY_ASK_ERASE,
    STR_COUNT,
};

/* The foot line while `row` waits for its second press. Touch's says which
 * way it is about to go. */
static StrId askLine(RecoveryRowId row, const Settings *settings) {
  if (row == RECOVERY_ROW_TOUCH && settings->touchOff != 0) {
    return STR_RECOVERY_ASK_TOUCH_ON;
  }
  return kRecoveryAsk[row];
}

/*
 * Whether the knob is held right now, read twice twenty milliseconds apart.
 *
 * One raw read of a pin with no debounce, right after the supply rail
 * settles, can be a glitch. A false positive costs a five second detour
 * through this screen, and a false negative loses the one way in when
 * nothing else on the radio can be trusted. Two reads that agree guard
 * against the first one being a glitch. The 20 ms is a simple guard, not a
 * measured figure.
 */
static bool knobHeld(void) {
  if (!encoderButtonDown(PANEL_BUTTON_ENCODER)) {
    return false;
  }
  delay(20);
  return encoderButtonDown(PANEL_BUTTON_ENCODER);
}

/* One click, one row, no acceleration: the same rule the RDS screen's own
 * `screenTaskRdsPage` already turns the knob by. A row is a place to land, not
 * a distance to cross. */
static uint8_t moveCursor(uint8_t cursor, int32_t clicks) {
  if (clicks == 0) {
    return cursor;
  }
  return (uint8_t)std::clamp<int32_t>(cursor + (clicks < 0 ? -1 : 1), 0,
                                      SCREEN_RECOVERY_ROWS - 1);
}

/* How long the knob must read up before a press can count again, so a
 * press that bounces as it lets go is one press. */
#define KNOB_QUIET_MS 50

/* Wait for the knob to be up and quiet, and drop any turns so far. */
static void knobQuiet(void) {
  uint32_t upMs = millis();
  while ((uint32_t)(millis() - upMs) < KNOB_QUIET_MS) {
    if (encoderButtonDown(PANEL_BUTTON_ENCODER)) {
      upMs = millis();
    }
    lvglPortPoll();
    delay(5);
  }
  (void)encoderTake();
}

/*
 * The touch calibration screen, run in recovery's own loop: the input task
 * has not started, so the knob and the touch chip are read here. The
 * screen is drawn as the board is mounted, since recovery ignores the
 * stored rotation. Turning or pressing the knob leaves, as on the radio,
 * and a calibration that passes its check is kept for the next start.
 */
static void calibrate(void) {
#if FEATURE_TOUCH
  screenRecoveryEnd();
  if (!screenTouchCalBegin(true)) {
    (void)screenRecoveryBegin();
    return;
  }
  touchBegin();
  TouchCal guide;
  bool stored = false;
  if (!touchCalNvsLoadOrBoard(DISPLAY_WIDTH, DISPLAY_HEIGHT, &guide, &stored)) {
    screenTouchCalEnd();
    (void)screenRecoveryBegin();
    return;
  }
  TouchCalFlow flow;
  touchCalFlowBegin(&flow, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, &guide);
  TouchFilter filter;
  memset(&filter, 0, sizeof(filter));
  bool changed = true;
  knobQuiet();
  bool wasDown = false;
  while (true) {
    const int32_t clicks = encoderTake();
    const bool down = encoderButtonDown(PANEL_BUTTON_ENCODER);
    const bool pressed = wasDown && !down;
    wasDown = down;
    if (clicks != 0 || pressed) {
      if (pressed && touchCalFlowFailed(&flow)) {
        touchCalFlowBegin(&flow, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, &guide);
        changed = true;
      } else {
        break;
      }
    }
    TouchReading r;
    memset(&r, 0, sizeof(r));
    r.pen = touchPenDown();
    if (r.pen) {
      TouchRaw raw;
      touchTake(&r, &raw);
    }
    TouchPoint at = {0, 0};
    bool unsettled = true;
    const bool contact = touchFilterFeed(&filter, &r, &at, &unsettled);
    if (screenTaskTouchCalStep(&flow, contact, r.read, at, unsettled,
                               millis())) {
      changed = true;
    }
    if (changed) {
      changed = false;
      ScreenTouchCal view;
      screenTaskTouchCalView(&flow, &view);
      screenTouchCalShow(&view);
    }
    lvglPortPoll();
    delay(10);
  }
  screenTouchCalEnd();
  (void)screenRecoveryBegin();
  knobQuiet();
#endif
}

/* Write `settings` and restart on them; if the write fails, mark `row` and
 * stay, so the row can say Failed. */
static void saveAndRestart(RecoveryRowId row, const Settings *settings) {
  if (!settingsNvsSave(settings)) {
    sSaveFailed[row] = true;
    return;
  }
  ESP.restart();
}

/*
 * What pressing the row under the cursor does.
 *
 * Every row ends in a restart, or stays on this screen when its change could
 * not be made. Rotate, Touch and Hotspot only matter to the next start,
 * since this screen never draws by the value it just changed. Rollback and
 * Erase Settings are a restart by their own nature, and Exit is a plain
 * restart.
 * Every row but Exit asks for a second press first, `kRecoveryAsk`.
 */
static void act(RecoveryRowId row, Settings *settings) {
  switch (row) {
    /* Each puts the old value back if the write failed, so the screen goes
     * on showing what is stored: the Rotate row shows the rotation. */
    case RECOVERY_ROW_ROTATE: {
      const uint8_t was = settings->displayRotation;
      settings->displayRotation = was == 180 ? 0 : 180;
      saveAndRestart(row, settings);
      settings->displayRotation = was;
      return;
    }
    case RECOVERY_ROW_TOUCH: {
      /* The way to stop a panel that touches itself when the menu cannot be
       * used for it. Only the knob works this screen, so the panel cannot
       * fight the change. */
      const uint8_t was = settings->touchOff;
      settings->touchOff = was != 0 ? 0 : 1;
      saveAndRestart(row, settings);
      settings->touchOff = was;
      return;
    }
    case RECOVERY_ROW_HOTSPOT: {
      /* The way back to a radio set Off with no network it can join, or one
       * that is somewhere its network is not: the hotspot, on every start
       * until the setting is changed again. */
      const uint8_t was = settings->hotspot;
      settings->hotspot = WIFI_HOTSPOT_ON;
      saveAndRestart(row, settings);
      settings->hotspot = was;
      return;
    }
    case RECOVERY_ROW_ROLLBACK:
      /* Returns only when it could not roll back. It restarts otherwise. */
      esp_ota_mark_app_invalid_rollback_and_reboot();
      sRollbackFailed = true;
      return;
    case RECOVERY_ROW_ERASE: {
      /* The `Settings` struct and the touch calibration only, on purpose:
       * the 99 memory channels and the logbook are not settings and a person
       * recovering a broken radio is not asking to lose either. */
      /* The calibration goes first, so a Failed always means the settings
       * were not touched; a calibration gone on its own only brings back
       * the board's map, which this row was going to do anyway. */
      Settings fresh;
      settingsDefaults(&fresh);
      if (!touchCalNvsErase()) {
        sSaveFailed[row] = true;
        return;
      }
      saveAndRestart(row, &fresh);
      return;
    }
    case RECOVERY_ROW_CALIBRATE:
      calibrate();
      return;
    case RECOVERY_ROW_EXIT:
      ESP.restart();
      return;
  }
}

void recoveryCheckAndRun(Settings *settings) {
  encoderBegin((EncoderKind)settings->encoderKind,
               (EncoderDirection)settings->encoderDirection);
  if (!knobHeld()) {
    return;
  }

  /*
   * `bootWatchdogArm` is the first thing `setup` does, forty five seconds
   * to reach the disarm near its own end or the radio restarts on the
   * assumption it hung. Recovery is a closed loop inside `setup` that can
   * legitimately sit here for as long as a person is reading seven rows and
   * deciding, and a panel responding to the knob is already the proof
   * this radio has not hung, the exact thing the watchdog exists to
   * catch. Disarmed here rather than left to fire mid-read.
   */
  bootWatchdogDisarm();

  if (!displayBegin() || !lvglPortBegin() || !screenRecoveryBegin()) {
    /* No panel to show this on. Recovery with nothing on the glass is not
     * recovery, so this falls back to the normal boot rather than sitting
     * in a loop nobody can see. */
    return;
  }
  /* `displayBegin` clears the panel and leaves the backlight off on
   * purpose, so whoever starts it decides how the light comes up; the
   * normal boot's own fade is that decision, made in `screen_task.cpp`,
   * and nothing here ever runs that. Recovery is the one screen a stored
   * brightness of 0, or any other display setting, must not be able to
   * hide: full, straight on, no fade to watch for. */
  displayBacklight(100);

  uint8_t cursor = 0;
  bool wasDown = encoderButtonDown(PANEL_BUTTON_ENCODER);
  /*
   * The knob is already down: `knobHeld` needed that to reach here. A tap
   * only counts once the knob has been seen to come back up on its own
   * first, so the release that ends the hold-to-enter gesture is never
   * read as a tap on row 0, `cursor`'s own starting value.
   */
  bool armed = false;
  /* Asked once. Nothing on this screen writes a firmware slot. */
  const bool rollbackPossible = esp_ota_check_rollback_is_possible();

  /* The row waiting for its second press, or -1. Moving off it cancels. */
  int8_t asked = -1;

  while (true) {
    const uint8_t was = cursor;
    cursor = moveCursor(cursor, encoderTake());
    if (cursor != was) {
      asked = -1;
    }

    const bool down = encoderButtonDown(PANEL_BUTTON_ENCODER);
    const bool pressed = armed && wasDown && !down;
    if (!down) {
      armed = true;
    }
    wasDown = down;

    const char *rotation =
        txt(settings->displayRotation == 180 ? STR_COMMON_ROTATION_UPSIDE_DOWN
                                             : STR_COMMON_ROTATION_NORMAL);
    const char *touch =
        txt(settings->touchOff != 0 ? STR_COMMON_OFF : STR_COMMON_ON);

    ScreenRecovery view;
    memset(&view, 0, sizeof(view));
    view.cursor = cursor;
    for (uint8_t i = 0; i < SCREEN_RECOVERY_ROWS; i++) {
      view.rows[i].name = txt(kRecoveryNames[i]);
      view.rows[i].value = sSaveFailed[i] ? txt(STR_RECOVERY_FAILED)
                           : (i == RECOVERY_ROW_ROTATE) ? rotation
                           : (i == RECOVERY_ROW_TOUCH)  ? touch
                                                        : NULL;
    }
    /* Only an image that came over the air has an older one to go back to.
     * After a USB flash there is none, and the row says so rather than
     * doing nothing when pressed. */
    if (!rollbackPossible) {
      view.rows[RECOVERY_ROW_ROLLBACK].value = txt(STR_COMMON_NONE);
      view.rows[RECOVERY_ROW_ROLLBACK].inert = true;
    } else if (sRollbackFailed) {
      view.rows[RECOVERY_ROW_ROLLBACK].value = txt(STR_RECOVERY_FAILED);
    }

    /* A first press on a row that asks only marks it; the second acts. */
    if (pressed && !view.rows[cursor].inert) {
      if (kRecoveryAsk[cursor] != STR_COUNT && asked != (int8_t)cursor) {
        asked = (int8_t)cursor;
      } else {
        asked = -1;
        act((RecoveryRowId)cursor, settings);
      }
    }
    if (asked >= 0) {
      view.rows[asked].value = txt(STR_RECOVERY_PRESS_AGAIN);
      view.hint = txt(askLine((RecoveryRowId)asked, settings));
    }
    screenRecoveryShow(&view);
    lvglPortPoll();
    delay(10);
  }
}
