/* Implementation of the join and access point fallback. */
#include "wifi_manager.h"

#include "board/board.h"
#include "drivers/device_id.h"
#include "ota_service.h"
#include "web_update.h"

#include <ESPmDNS.h>
#include <WiFi.h>

static char sAddress[16] = "0.0.0.0";
static char sNetwork[SETTINGS_SSID_LEN] = "";
static char sApSsid[SETTINGS_SSID_LEN] = "";
/* The live settings, not a copy of them: they are written only on the loop
 * task, which is the task this runs on, so they cannot change under a
 * step. */
static const Settings *sSettings = NULL;
static WifiJoin sJoin;
/* The hotspot and Wi-Fi settings the join last acted on, so a change is
 * acted on at once rather than at the next retry. */
static uint8_t sHotspotSeen = WIFI_HOTSPOT_AUTO;
static uint8_t sWifiSeen = 1;
/* The web server setting the name on the network last followed. */
static uint8_t sWebSeen = 1;

static void buildApSsid(void) {
  uint8_t mac[6];
  deviceMacRead(mac);
  snprintf(sApSsid, sizeof(sApSsid), "%s-%02X%02X", BOARD_AP_PREFIX, mac[4],
           mac[5]);
}

static void captureStationAddress(void) {
  IPAddress ip = WiFi.localIP();
  snprintf(sAddress, sizeof(sAddress), "%u.%u.%u.%u", ip[0], ip[1], ip[2],
           ip[3]);
}

/*
 * Start the station and ask for the credentials to be tried.
 *
 * Returns once the request is in. Whether it worked is read off the link on a
 * later pass, which is what stops this file blocking the loop task.
 */
static void beginStation(const Settings *settings) {
  WiFi.persistent(false);
  /* Before the mode, which is when the framework gives the station its
   * name. Set after it, the name reaches nothing, and the router lists the
   * radio as esp32- and the last three bytes of its MAC. */
  WiFi.setHostname(BOARD_HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(settings->wifiSsid, settings->wifiPass);
}

static void startAccessPoint(void) {
  buildApSsid();
  WiFi.mode(WIFI_AP);
  /* The hotspot has no password, on purpose. On Auto it starts because the
   * stored network did not work, so the person using the radio has no other
   * way in, and a password they cannot be told would lock them out. Anyone
   * in range can join it. The setup page served here only takes Wi-Fi
   * credentials. Firmware upload and reboot still need the access PIN. */
  if (!WiFi.softAP(sApSsid)) {
    /* Nothing is reachable. Say so, because the rollback self check reads
     * this and an image with no working Wi-Fi must not be marked good. */
    Serial.println(F("[wifi] the access point did not start"));
    snprintf(sAddress, sizeof(sAddress), "0.0.0.0");
    snprintf(sNetwork, sizeof(sNetwork), "none");
    /*
     * Mode off as well as state OFFLINE. WIFI_STATE_OFFLINE means nothing is
     * running, and WiFi.mode(WIFI_AP) above is still in effect until this
     * says otherwise, which would leave the driver holding ADC2 for as long
     * as the retry backoff lasts.
     */
    if (!WiFi.mode(WIFI_MODE_NULL)) {
      /* Nothing else to do about it: the state is OFFLINE either way, and
       * there is no fourth state for a driver that will not do either job.
       * Said here so it is not a silent second failure on top of the one
       * above. */
      Serial.println(F("[wifi] could not turn the radio off either"));
    }
    /*
     * Back to OFFLINE rather than left claiming an access point that is not
     * there. The decision treats OFFLINE as "try the stored network", so the
     * next pass starts a join, which is the only other way back.
     */
    sJoin.state = WIFI_STATE_OFFLINE;
    return;
  }

  IPAddress ip = WiFi.softAPIP();
  snprintf(sAddress, sizeof(sAddress), "%u.%u.%u.%u", ip[0], ip[1], ip[2],
           ip[3]);
  snprintf(sNetwork, sizeof(sNetwork), "%s", sApSsid);
}

/* Whether the name is being announced, for wifiAnnouncedName. */
static bool sNameAnnounced = false;

/*
 * mDNS, once there is an interface for it to answer on.
 *
 * A responder started before the station has an address answers on nothing,
 * and the join finishes well after setup returns, so this runs on the pass
 * the address actually arrives rather than from setup.
 */
static void startResponder(void) {
  MDNS.end();
  sNameAnnounced = false;
  /* With the web server off there is nothing on the network to find. */
  if (sSettings != NULL && !sSettings->webEnabled) {
    return;
  }
  if (!MDNS.begin(BOARD_HOSTNAME)) {
    Serial.println(F("[mdns] could not start, use the address instead"));
    return;
  }
  sNameAnnounced = true;
  MDNS.addService("http", "tcp", WEB_PORT);
  /* Restarting the responder drops every service registered on the old one,
   * so the ArduinoOTA service is added again here. With FEATURE_OTA at 0
   * otaAdvertise does nothing. Flashing by name is what that service is
   * for. */
  otaAdvertise();
}

/* One pass of the decision, and the one thing it asks for. */
static void step(bool retryNow) {
  if (sSettings == NULL) {
    /* Not begun: nothing to join with and no access point to serve. */
    return;
  }
  WifiJoinInput in;
  in.nowMs = millis();
  in.hasCredentials = settingsHasWifi(sSettings);
  in.linkUp = WiFi.status() == WL_CONNECTED;
  /* Asked for rather than always read: it is a call into the Wi-Fi driver,
   * and this runs on every pass of the loop task. */
  in.apClients = wifiJoinWantsApClients(&sJoin, in.nowMs, in.hasCredentials)
                     ? (uint8_t)WiFi.softAPgetStationNum()
                     : 0;
  in.hotspot = sSettings->hotspot;
  in.wifiOn = sSettings->wifiEnabled != 0;
  if (in.hotspot != sHotspotSeen || sSettings->wifiEnabled != sWifiSeen) {
    sHotspotSeen = in.hotspot;
    sWifiSeen = sSettings->wifiEnabled;
    retryNow = true;
  }
  in.retryNow = retryNow;
  /* The web server switched on or off while on a network: the name follows
   * it. */
  if (sSettings->webEnabled != sWebSeen) {
    sWebSeen = sSettings->webEnabled;
    if (sJoin.state == WIFI_STATE_ONLINE ||
        sJoin.state == WIFI_STATE_ACCESS_POINT) {
      startResponder();
    }
  }

  switch (wifiJoinStep(&sJoin, &in)) {
    case WIFI_DO_BEGIN_JOIN:
      beginStation(sSettings);
      break;
    case WIFI_DO_START_AP:
      Serial.println(F("[wifi] no network, serving the setup page"));
      startAccessPoint();
      /* The responder runs on the access point too. Somebody who has never
       * seen this radio before is being asked to find it, and the name is
       * the only thing they have been told. */
      if (sJoin.state == WIFI_STATE_ACCESS_POINT) {
        startResponder();
      }
      break;
    case WIFI_DO_CAME_ONLINE:
      captureStationAddress();
      snprintf(sNetwork, sizeof(sNetwork), "%s", sSettings->wifiSsid);
      startResponder();
      /*
       * The banner cannot say any of this. It prints while the join is
       * still running, so this pass is the first moment the address is known,
       * and a person at the bench has nothing else to read it from.
       */
      Serial.printf(
          "[wifi] joined %s, at http://%s:%u/ and http://%s.local:%u/\n",
          sNetwork, sAddress, (unsigned)WEB_PORT, BOARD_HOSTNAME,
          (unsigned)WEB_PORT);
#if FEATURE_OTA
      Serial.printf("[wifi] flash it over the air with --upload-port %s\n",
                    sAddress);
#else
      Serial.printf(
          "[wifi] flash it over the air: sign in at http://%s:%u/auth, "
          "then post the image to /update\n",
          sAddress, (unsigned)WEB_PORT);
#endif
      break;
    case WIFI_DO_STAY_ONLINE:
      captureStationAddress();
      break;
    case WIFI_DO_TURN_OFF:
      Serial.println(
          F("[wifi] off: switched off, or no network and no "
            "hotspot"));
      MDNS.end();
      if (!WiFi.mode(WIFI_MODE_NULL)) {
        Serial.println(F("[wifi] could not turn the radio off"));
      }
      snprintf(sAddress, sizeof(sAddress), "0.0.0.0");
      sNetwork[0] = '\0';
      break;
    case WIFI_DO_NOTHING:
      break;
  }
}

void wifiBegin(const Settings *settings) {
  sSettings = settings;
  sHotspotSeen =
      settings != NULL ? settings->hotspot : (uint8_t)WIFI_HOTSPOT_AUTO;
  sWifiSeen = settings != NULL ? settings->wifiEnabled : 1;
  sWebSeen = settings != NULL ? settings->webEnabled : 1;
  wifiJoinReset(&sJoin);
  step(false);
}

void wifiLoop(void) {
  step(false);
}

WifiState wifiState(void) {
  return sJoin.state;
}

bool wifiOnWantedNetwork(void) {
  if (sJoin.state == WIFI_STATE_ONLINE) {
    return true;
  }
  /* Wi-Fi switched off: no network is the one asked for. */
  if (sSettings != NULL && !sSettings->wifiEnabled) {
    return true;
  }
  return sSettings != NULL && sSettings->hotspot == WIFI_HOTSPOT_ON &&
         sJoin.state == WIFI_STATE_ACCESS_POINT;
}

const char *wifiAddress(void) {
  return sAddress;
}

const char *wifiNetworkName(void) {
  return sNetwork;
}

bool wifiRssiDbm(int8_t *out) {
  if (out == NULL || sJoin.state != WIFI_STATE_ONLINE) {
    return false;
  }
  /* WiFi.RSSI() reads a signed 8 bit register on the radio directly, so
   * this never blocks and never reaches the network the way a scan would.
   */
  *out = (int8_t)WiFi.RSSI();
  return true;
}

void wifiRetryNow(void) {
  step(true);
}

const char *wifiAnnouncedName(void) {
  return sNameAnnounced ? BOARD_HOSTNAME : NULL;
}
