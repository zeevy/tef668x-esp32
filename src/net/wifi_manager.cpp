/**
 * @file wifi_manager.cpp
 * @brief Implementation of the join and access point fallback.
 */
#include "wifi_manager.h"

#include "board/board.h"
#include "drivers/device_id.h"

#include <WiFi.h>

static WifiState sState = WIFI_STATE_OFFLINE;
static char sAddress[16] = "0.0.0.0";
static char sNetwork[SETTINGS_SSID_LEN] = "";
static char sApSsid[SETTINGS_SSID_LEN] = "";
static uint32_t sLastRetryMs = 0;
static uint8_t sDropCount = 0;
static Settings sSettings;

/** Build the access point name, ending in the last two MAC bytes. */
static void buildApSsid(void) {
  uint8_t mac[6];
  deviceMacRead(mac);
  snprintf(sApSsid, sizeof(sApSsid), "%s-%02X%02X", BOARD_AP_PREFIX, mac[4],
           mac[5]);
}

/** Record the current station address so callers can print it. */
static void captureStationAddress(void) {
  IPAddress ip = WiFi.localIP();
  snprintf(sAddress, sizeof(sAddress), "%u.%u.%u.%u", ip[0], ip[1], ip[2],
           ip[3]);
}

/**
 * Try the stored credentials once.
 *
 * @return true when the radio joined inside WIFI_JOIN_TIMEOUT_MS.
 */
static bool tryJoin(const Settings *settings) {
  if (!settingsHasWifi(settings)) {
    return false;
  }

  sState = WIFI_STATE_JOINING;
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(BOARD_HOSTNAME);
  WiFi.begin(settings->wifiSsid, settings->wifiPass);

  uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - started >= WIFI_JOIN_TIMEOUT_MS) {
      WiFi.disconnect(true);
      return false;
    }
    delay(100);
  }

  captureStationAddress();
  snprintf(sNetwork, sizeof(sNetwork), "%s", settings->wifiSsid);
  sState = WIFI_STATE_ONLINE;
  return true;
}

/** Start the setup access point so the radio stays reachable. */
static void startAccessPoint(void) {
  buildApSsid();
  WiFi.mode(WIFI_AP);
  /* Open on purpose. The user has no working network at this point, so a
   * passphrase they cannot be told is the same as no way in. The setup page
   * served here only takes Wi-Fi credentials. Firmware upload and reboot
   * still need the access PIN. */
  if (!WiFi.softAP(sApSsid)) {
    /* Nothing is reachable. Say so, because the rollback self check reads
     * this and an image with no working Wi-Fi must not be marked good. */
    Serial.println(F("[wifi] the access point did not start"));
    snprintf(sAddress, sizeof(sAddress), "0.0.0.0");
    snprintf(sNetwork, sizeof(sNetwork), "none");
    sState = WIFI_STATE_OFFLINE;
    sLastRetryMs = millis();
    return;
  }

  IPAddress ip = WiFi.softAPIP();
  snprintf(sAddress, sizeof(sAddress), "%u.%u.%u.%u", ip[0], ip[1], ip[2],
           ip[3]);
  snprintf(sNetwork, sizeof(sNetwork), "%s", sApSsid);
  sState = WIFI_STATE_ACCESS_POINT;
  sLastRetryMs = millis();
}

void wifiBegin(const Settings *settings) {
  sSettings = *settings;
  sDropCount = 0;
  if (!tryJoin(&sSettings)) {
    startAccessPoint();
  }
}

void wifiLoop(void) {
  if (sState == WIFI_STATE_ONLINE) {
    if (WiFi.status() != WL_CONNECTED) {
      /* One bad poll is not a lost network. A router rebooting for a few
       * seconds should not cost five minutes off the air, so wait for a run
       * of failures before giving up the station. */
      if (sDropCount < WIFI_DROP_TOLERANCE) {
        sDropCount++;
        return;
      }
      Serial.println(
          F("[wifi] lost the network, falling back to the access "
            "point"));
      sDropCount = 0;
      startAccessPoint();
    } else {
      sDropCount = 0;
      captureStationAddress();
    }
    return;
  }

  if (sState != WIFI_STATE_ACCESS_POINT && sState != WIFI_STATE_OFFLINE) {
    return;
  }
  if (!settingsHasWifi(&sSettings)) {
    return;
  }
  if (millis() - sLastRetryMs < WIFI_AP_RETRY_INTERVAL_MS) {
    return;
  }

  /* The retry blocks for up to WIFI_JOIN_TIMEOUT_MS and tears the access
   * point down to do it. Do not do that to someone who is sitting on the
   * setup page. */
  if (sState == WIFI_STATE_ACCESS_POINT && WiFi.softAPgetStationNum() > 0) {
    sLastRetryMs = millis();
    return;
  }

  sLastRetryMs = millis();
  if (!tryJoin(&sSettings)) {
    startAccessPoint();
  }
}

WifiState wifiState(void) {
  return sState;
}

bool wifiReachable(void) {
  return sState == WIFI_STATE_ONLINE || sState == WIFI_STATE_ACCESS_POINT;
}

const char *wifiAddress(void) {
  return sAddress;
}

const char *wifiNetworkName(void) {
  return sNetwork;
}

bool wifiRetryNow(const Settings *settings) {
  sSettings = *settings;
  if (tryJoin(&sSettings)) {
    return true;
  }
  startAccessPoint();
  return false;
}
