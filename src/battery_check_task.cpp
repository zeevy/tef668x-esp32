/* Implementation of the battery read again with Wi-Fi paused. */
#include "battery_check_task.h"

#include <Arduino.h>

#include "core/battery_check.h"
#include "debug_log.h"
#include "drivers/battery_adc.h"
#include "net/rollback.h"
#include "net/update_check.h"
#include "net/web_update.h"
#include "net/wifi_manager.h"
#include "net/xdr_server.h"
#include "radio_task.h"
#include "screen_task.h"

static const Settings *sSettings = NULL;
/* The last try, or start up if none since. */
static uint32_t sLastTryMs = 0;
static uint16_t sCount = 0;
static uint32_t sPauseMs = 0;
static uint32_t sBackMs = 0;
/* While Wi-Fi is not back yet: when it was started again, and the state it
 * was in before the pause, which is what back means. */
static bool sWaitingBack = false;
static uint32_t sResumedAtMs = 0;
static WifiState sStateBefore = WIFI_STATE_OFFLINE;

void batteryCheckTaskBegin(const Settings *settings) {
  sSettings = settings;
  sLastTryMs = millis();
}

/* A seek or an AF check reads the tuner on the way, which a fresh join's
 * traffic would spoil, and an update check fetches over the network. A
 * snapshot that could not be read is taken as busy, not as idle. */
static bool radioOrNetBusy(void) {
  RadioSnapshot snap;
  return !radioGetSnapshot(&snap) || snap.seeking || radioAfBusy() ||
         updateCheckRunning();
}

void batteryCheckTaskPoll(bool (*busy)(void)) {
  if (sSettings == NULL) {
    return;
  }
  const uint32_t now = millis();
  const WifiState link = wifiState();
  const bool onNetwork =
      link == WIFI_STATE_ONLINE || link == WIFI_STATE_ACCESS_POINT;
  /* Back once on the home network, or on the hotspot it was serving: not on
   * the fallback hotspot after a join that failed. */
  if (sWaitingBack && (link == WIFI_STATE_ONLINE || link == sStateBefore)) {
    sBackMs = now - sResumedAtMs;
    sWaitingBack = false;
  }
  /* The cheap tests first, so most passes cost a comparison or two and
   * nothing reaches into the Wi-Fi driver. */
  if (!sSettings->batteryCheck || !onNetwork ||
      (uint32_t)(now - sLastTryMs) < BATTERY_CHECK_EVERY_MS) {
    return;
  }
  BatteryCheckInput in = {};
  in.nowMs = now;
  in.lastTryMs = sLastTryMs;
  in.fitted = batteryAdcFitted();
  in.wifiRunning = onNetwork;
  in.lastInputMs = screenTaskLastInputMs();
  uint8_t percent = 0;
  in.screenOff = screenTaskBacklightState(&percent) && percent == 0;
  in.livePages = webLivePages();
  in.anyRequest = webLastRequestMs(&in.lastRequestMs);
  in.clients = (uint8_t)(xdrServerClients() + wifiApClients());
  in.onTrial = rollbackPending();
  in.busy = busy();
  if (!batteryCheckDue(&in) || radioOrNetBusy()) {
    return;
  }
  sLastTryMs = now;
  sStateBefore = link;
  const uint32_t startMs = millis();
  wifiPause();
  /* Read at once: on this radio a read straight after the stop and one after
   * a fresh 50 ms settle came within 8 mV of each other. */
  uint16_t mv = 0;
  const bool read = batteryAdcRead(&mv);
  wifiResume();
  /* The whole hold on the loop, the stop and the start of the driver too. */
  sPauseMs = millis() - startMs;
  sResumedAtMs = millis();
  sBackMs = 0;
  sWaitingBack = true;
  sCount++;
  if (read) {
    screenTaskBatteryTake(mv);
  }
  DebugLog.printf("[battery] %s %u mV, the loop held %lu ms\n",
                  read ? "read" : "not read", read ? (unsigned)mv : 0u,
                  (unsigned long)sPauseMs);
}

uint16_t batteryCheckTaskCount(void) {
  return sCount;
}

uint32_t batteryCheckTaskPauseMs(void) {
  return sPauseMs;
}

uint32_t batteryCheckTaskBackMs(void) {
  return sBackMs;
}
