/* Auto off and Sleep Now, joined to the input, the radio and the panel. */
#include "sleep_task.h"

#include <Arduino.h>

#include "band_scan_task.h"
#include "core/auto_off.h"
#include "core/dx_scan.h"
#include "drivers/analog.h"
#include "drivers/power.h"
#include "drivers/tef668x.h"
#include "dx_task.h"
#include "input_task.h"
#include "net/ota_service.h"
#include "net/rollback.h"
#include "radio_task.h"
#include "screen_task.h"
#include "settings_task.h"
#include "ui/draw.h"

/* How long Sleep Now waits, so the browser's reply is sent before the network
 * goes. The same wait a Wi-Fi change from the API gives. */
#define SLEEP_NOW_DELAY_MS 200
/* How long "Going to Sleep" shows first: long enough to read from across a
 * room, and any touch in it keeps the radio awake. */
#define SLEEP_MESSAGE_MS 5000UL

static AutoOff sAuto;
static bool sStarted = false;
static uint32_t sLastActivity = 0;
static bool sFading = false;
static bool sSleepAsked = false;
static uint32_t sSleepAtMs = 0;
/* The message is up and the radio sleeps when it runs out, unless touched. */
static bool sGoing = false;
static uint32_t sGoingAtMs = 0;
static uint32_t sGoingActivity = 0;
static const char *sGoingWhy = "";

static void start(void) {
  if (!sStarted) {
    sStarted = true;
    autoOffInit(&sAuto, 0, millis());
    sLastActivity = inputActivity();
  }
}

void sleepTaskSetMinutes(uint16_t minutes) {
  start();
  if (minutes != sAuto.minutes) {
    autoOffSetMinutes(&sAuto, minutes, millis());
  }
}

void sleepTaskUsed(void) {
  start();
  autoOffUsed(&sAuto, millis());
}

/*
 * Things that keep the radio awake while they run. An update on trial, since
 * the wake is a restart and a restart rolls it back, and one being written.
 * A band scan and the DX scanner, which are left to run on their own; the
 * count starts once they stop.
 */
static bool keptAwake(void) {
  return rollbackPending() || otaInProgress() || bandScanActive() ||
         dxTaskScan()->state == DX_SCAN_RUNNING;
}

static void goToSleep(const char *why) {
  Serial.printf("[sleep] %s, going to sleep\n", why);
  uiSetSleepMark(UI_SLEEP_NONE);
  settingsTaskStop();
  /* The radio task is parked by the stop, so nothing else is on the bus. */
  tef668xSetActive(false);
  smeterShow(0);
  powerDeepSleep();
}

/* The message first; goToSleep when it has shown for SLEEP_MESSAGE_MS. */
static void beginSleep(const char *why, uint32_t nowMs) {
  if (sGoing) {
    return;
  }
  sGoing = true;
  sGoingAtMs = nowMs;
  sGoingActivity = inputActivity();
  sGoingWhy = why;
  Serial.printf("[sleep] %s, going to sleep in %lu ms\n", why,
                (unsigned long)SLEEP_MESSAGE_MS);
  screenTaskSleepShow(true);
  /* The sound goes with the message, over the same few seconds. After auto
   * off's own fade it is already down. */
  radioSetSleepFade(true, SLEEP_MESSAGE_MS);
}

void sleepTaskPoll(void) {
  start();
  const uint32_t nowMs = millis();
  if (sGoing) {
    if (inputActivity() != sGoingActivity || keptAwake()) {
      /* Touched while it said it was going: it stays awake, and the count
       * starts again below. */
      Serial.println(F("[sleep] touched, staying awake"));
      sGoing = false;
      sSleepAsked = false;
      screenTaskSleepShow(false);
      /* The sound comes back; auto off's count starts again below. */
      sFading = false;
      radioSetSleepFade(false, SLEEP_MESSAGE_MS);
    } else if ((uint32_t)(nowMs - sGoingAtMs) >= SLEEP_MESSAGE_MS) {
      goToSleep(sGoingWhy);
    }
  } else if (sSleepAsked && (int32_t)(nowMs - sSleepAtMs) >= 0) {
    beginSleep("asked for", nowMs);
  }
  const uint32_t activity = inputActivity();
  if (activity != sLastActivity || keptAwake()) {
    sLastActivity = activity;
    autoOffUsed(&sAuto, nowMs);
  }
  const AutoOffPhase phase = autoOffPhase(&sAuto, nowMs);
  uiSetSleepMark(sAuto.minutes == 0       ? UI_SLEEP_NONE
                 : phase >= AUTO_OFF_WARN ? UI_SLEEP_SOON
                                          : UI_SLEEP_ON);
  const bool fading = phase >= AUTO_OFF_FADE;
  if (fading != sFading) {
    sFading = fading;
    radioSetSleepFade(fading, AUTO_OFF_FADE_MS);
  }
  if (phase == AUTO_OFF_SLEEP) {
    beginSleep("left alone", nowMs);
  }
}

bool sleepTaskNow(void) {
  /* Nor while one is being written, which sleep would cut off half way. */
  if (rollbackPending() || otaInProgress()) {
    return false;
  }
  sSleepAsked = true;
  sSleepAtMs = millis() + SLEEP_NOW_DELAY_MS;
  return true;
}

bool sleepTaskLeftS(uint32_t *out) {
  start();
  uint32_t leftMs = 0;
  if (!autoOffLeftMs(&sAuto, millis(), &leftMs)) {
    return false;
  }
  if (out != NULL) {
    *out = (leftMs + 999) / 1000;
  }
  return true;
}
