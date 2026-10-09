/* Implementation of keeping the stored settings up to date. */
#include "settings_task.h"
#include "debug_log.h"

#include "band_scan_task.h"
#include "core/autosave.h"
#include "core/backlight.h"
#include "core/clock.h"
#include "core/input.h"
#include "core/radio.h"
#include "core/seek.h"
#include "core/squelch.h"
#include "drivers/display.h"
#include "drivers/settings_nvs.h"
#include "dx_task.h"
#include "input_task.h"
#include "lvgl_port.h"
#include "memory_store.h"
#include "net/ntp.h"
#include "radio_task.h"
#include "screen_task.h"
#include "sleep_task.h"
#include "ui/theme.h"

#include <Arduino.h>
#include <string.h>

/*
 * How often the settings are compared, in milliseconds.
 *
 * Two struct comparisons and a snapshot copy, so it is cheap, but there is
 * nothing to gain from doing it faster than the radio publishes. The wait
 * before a save is ten seconds, so a quarter of a second either way changes
 * nothing about when one lands.
 */
#define SETTINGS_POLL_MS 250

/* Whether the last theme the hour chose was the day one. Day to start with,
 * since the radio starts without the time. */
static bool sThemeDay = true;

static Settings *sLive = NULL;
static AutoSave sWhen;
static uint32_t sLastPollMs = 0;

/*
 * The candidate as it was on the previous look.
 *
 * Kept so that "something moved since last time" can be answered separately
 * from "this differs from what is stored". The two are different questions
 * and core/autosave.h needs both: what the radio is set to stays different
 * from what is stored for the whole wait, so one answer cannot restart the
 * clock and decide there is work to do at the same time.
 */
static Settings sPrevious;
static bool sPreviousKnown = false;

static SettingsSaveStatus sStatus;

/*
 * What a save keeps of the dial.
 *
 * A seek and a band scan walk the dial through channels nobody chose, so
 * every save keeps the station the walk started from, and the rest of what
 * the radio is set to as it is. A band scan of another band keeps the band
 * it started on too. A DX scan keeps the dial: it stops on the catches a
 * person is hunting, and the automatic save waits for it instead.
 */
static void keepWalkStart(RadioSnapshot *now) {
  if (now->seeking && now->seekFromKHz != 0) {
    now->settings.freqKHz = now->seekFromKHz;
    return;
  }
  BandScanFrom from;
  if (bandScanFrom(&from)) {
    bandScanKeep(&now->settings, &from);
  }
}

bool settingsBuildCandidate(const Settings *from, Settings *out, bool *busy) {
  if (from == NULL || out == NULL) {
    return false;
  }
  RadioSnapshot now;
  if (!radioGetSnapshot(&now)) {
    return false;
  }
  keepWalkStart(&now);
  /* Straight into the caller's struct. A local copy here and another in the
   * caller is four hundred bytes of stack on the loop task for nothing. */
  *out = *from;
  radioToSettings(&now.settings, out);
  /* Not part of RadioSettings, so radioToSettings cannot carry it. The
   * squelch mode belongs to the radio task rather than to the dial. */
  out->squelchMode = (uint8_t)radioSquelchMode(NULL);

  /* The volume is put back unless the knob is the squelch.
   *
   * A volume is stored for one case only: in manual squelch the knob is the
   * squelch control and nothing else on the radio says how loud to be. In every
   * other mode the knob wins and the stored value is never read, so keeping it
   * up to date buys nothing and costs a great deal.
   *
   * It costs, because the volume follows the knob. Left in, every turn of
   * the volume control makes the settings differ and earns a write to flash
   * ten seconds later. Worse, the volume the radio starts at is mapped from
   * one pot reading with no hysteresis, so a knob sitting near the boundary
   * between two dB can map to a different value than the one stored and make
   * the radio write flash after a power cycle in which nothing was touched.
   * That one is intermittent, depends on where the knob happens to sit, and
   * shows up years later as a worn sector. */
  if (out->squelchMode != (uint8_t)SQUELCH_MANUAL) {
    out->startVolumeDb = from->startVolumeDb;
  }
  if (busy != NULL) {
    *busy = now.seeking || dxTaskScan()->state == DX_SCAN_RUNNING;
  }
  return true;
}

void settingsApplyRadio(const Settings *s) {
  if (s == NULL) {
    return;
  }
  /* The seek sensitivities act at once, unlike the band plan, which is read
   * only at start up: nothing is tuned to them, so there is nothing for a
   * change to be unfair to. */
  SeekConfig seekCfg;
  seekDefaults(&seekCfg);
  seekCfg.fmSensitivity = s->fmScanSensitivity;
  seekCfg.amSensitivity = s->amScanSensitivity;
  radioSetSeekConfig(&seekCfg);

  radioSetSoftMuteMs(s->softMuteMs);
  radioSetEdgeBeep(s->beepEdge != 0);
  radioSetSquelchFloor(s->fmSquelchFloor);
  radioSetRdsEnabled(s->rdsEnabled != 0);
  /* The volume AGC. A target of zero switches it off, so this one call is
   * both the setting and the switch. */
  radioSetAgc(s->agcTargetPercent, s->agcBoostDb);
}

/* The theme for the hour: the day one from 06:00 to 17:59 and whenever the
 * time is not known, the night one otherwise. */
static uint8_t themeForNow(const Settings *s) {
  return clockIsDay(ntpLocalTime()) ? s->theme : s->nightTheme;
}

void settingsApplyTheme(const Settings *s) {
  if (s == NULL) {
    return;
  }
  /* The custom slot is loaded whether or not it is the active theme, so a
   * colour wheel edit shows up the moment somebody switches to it rather than
   * needing a second change to be noticed. Changing the active theme repaints
   * the screen and touches nothing else. */
  themeSetCustomColours(s->customTheme);
  themeSet(themeForNow(s));
}

void settingsApplyScreen(const Settings *s) {
  if (s == NULL) {
    return;
  }
  screenTaskSetBatteryShow(s->batteryShow);
  screenTaskSetRdsRegion(s->rdsRegion);
  /* The DX SETUP menu: the next scan and the next DX mode read them, and a scan
   * running now takes the mute and the auto log at once. */
  dxTaskApplySettings(s);
}

void settingsApplyInput(const Settings *s) {
  if (s == NULL) {
    return;
  }
  inputSetBeeps((BeepMode)s->beepKey);
  inputSetTouch(s->touchOff == 0);
  inputSetTouchUpsideDown(s->displayRotation == 180);
  inputSetKeypadTimeout(s->keypadTimeoutS);
  /* A new time starts the count again, the same as a key would, and a time
   * chosen before a restart or a sleep counts from the start. */
  sleepTaskSetMinutes(s->autoOffMinutes);
}

void settingsApplyLive(const Settings *s) {
  if (s == NULL) {
    return;
  }
  settingsApplyTheme(s);
  settingsApplyScreen(s);
  settingsApplyRadio(s);
  settingsApplyInput(s);

  /* The clock acts at once. Turning it off has to take the time off the
   * screen straight away, or a clock nothing is updating any more carries on
   * looking like a clock. */
  ntpApply(s);

  /* The panel light changes while the person is looking at it, which is the
   * only way a brightness can be chosen. */
  BacklightConfig backlight;
  backlightFromSettings(s, &backlight);
  screenTaskSetBacklight(&backlight);

  /* Turned while the person is looking at it, the same reasoning as the
   * backlight above: a flip a person cannot see happen is a flip they
   * cannot be sure they chose. */
  if (displayRotationSet(s->displayRotation == 180 ? 180 : 0)) {
    lvglPortRedrawAll();
  }
}

void settingsTaskBegin(Settings *live, uint32_t idleMs) {
  sLive = live;
  memset(&sStatus, 0, sizeof(sStatus));
  sStatus.idleMs = idleMs;
  sPreviousKnown = false;
  autoSaveInit(&sWhen, idleMs, millis());
}

/*
 * Write `candidate` to NVS and keep the books: the live copy, the save count
 * and the failed flag the state reports, and the automatic save's wait,
 * which starts again either way so a refusal is not retried every quarter of
 * a second. The one write the three savers share.
 *
 * Refused when the stored form has no room for it: the radio can reach
 * states that do not fit, an AM width on an FM band among them, and writing
 * one means the next start throws the whole blob away without saying so.
 */
static bool writeSettings(const Settings *candidate, uint32_t nowMs) {
  autoSaveDone(&sWhen, nowMs);
  if (!settingsValid(candidate)) {
    sStatus.lastFailed = true;
    DebugLog.println(F("[settings] what the radio is set to cannot be stored"));
    return false;
  }
  if (!settingsNvsSave(candidate)) {
    sStatus.lastFailed = true;
    DebugLog.println(F("[settings] the save could not be written"));
    return false;
  }
  *sLive = *candidate;
  sStatus.lastFailed = false;
  sStatus.saves++;
  sStatus.differs = false;
  sStatus.dueInMs = 0;
  sPreviousKnown = false;
  return true;
}

/* The station a save wrote, for the serial log. */
static void logSaved(const char *how, const Settings *s) {
  char text[24];
  bandFormatWithUnit((BandId)s->startBand, s->startFreqKHz, text, sizeof(text));
  DebugLog.printf("[settings] saved %s, %s\n", how, text);
}

void settingsTaskSaveNow(void) {
  if (sLive == NULL) {
    return;
  }
  Settings candidate;
  if (!settingsBuildCandidate(sLive, &candidate, NULL)) {
    DebugLog.println(
        F("[settings] the radio could not be read, nothing saved"));
    return;
  }
  if (memcmp(&candidate, sLive, sizeof(Settings)) == 0) {
    /* Already stored. Saying so is worth a line: "nothing saved" and
     * "nothing to save" look the same from outside and mean opposite
     * things. */
    DebugLog.println(F("[settings] already stored, nothing to write"));
    return;
  }
  if (writeSettings(&candidate, millis())) {
    logSaved("before the restart", &candidate);
  }
}

void settingsTaskStop(void) {
  settingsTaskSaveNow();
  memoryStoreSaveNow();
  /* Hushed before DX mode is left: leaving stops a scan and lifts its mute,
   * and the catch writes after it take long enough to hear the channel. */
  radioHush();
  dxTaskLeave();
}

void settingsTaskRestart(void) {
  settingsTaskStop();
  delay(200);
  ESP.restart();
}

bool settingsTaskStore(const Settings *candidate) {
  if (sLive == NULL || candidate == NULL) {
    return false;
  }
  if (memcmp(candidate, sLive, sizeof(Settings)) == 0) {
    /* Nothing moved. NVS is 20 KB and a write for no change is a write. */
    return true;
  }
  if (!writeSettings(candidate, millis())) {
    return false;
  }
  settingsApplyLive(sLive);
  return true;
}

void settingsTaskStatus(SettingsSaveStatus *out) {
  if (out != NULL) {
    *out = sStatus;
  }
}

void settingsTaskPoll(void) {
  if (sLive == NULL) {
    return;
  }
  uint32_t nowMs = millis();
  if ((uint32_t)(nowMs - sLastPollMs) < SETTINGS_POLL_MS) {
    return;
  }
  sLastPollMs = nowMs;

  /* Day to night and back, and the clock first learning the time at night.
   * Only on the change, so a theme being tried in the menu stays on the
   * panel while it is tried. */
  const bool day = clockIsDay(ntpLocalTime());
  if (day != sThemeDay) {
    sThemeDay = day;
    themeSet(themeForNow(sLive));
  }

  Settings candidate;
  bool busy = false;
  if (!settingsBuildCandidate(sLive, &candidate, &busy)) {
    /* The radio could not be read, so there is nothing to compare against.
     * Leaving the clock alone is right: this is not the settings holding
     * still, it is not knowing what they are. */
    return;
  }

  bool differs = memcmp(&candidate, sLive, sizeof(Settings)) != 0;
  bool moved =
      !sPreviousKnown || memcmp(&candidate, &sPrevious, sizeof(Settings)) != 0;
  sPrevious = candidate;
  sPreviousKnown = true;

  bool due = autoSaveDue(&sWhen, differs, moved, busy, nowMs);
  /* Read after the decision, not before. autoSaveDue is what restarts the
   * clock when something moved, so asking first reports the wait left from
   * the previous look and the countdown never shows its full length while a
   * hand is on the knob. */
  sStatus.differs = differs;
  sStatus.dueInMs = autoSaveWaitMs(&sWhen, differs, nowMs);
  if (!due) {
    return;
  }

  if (writeSettings(&candidate, nowMs)) {
    logSaved("on its own", &candidate);
  }
}
