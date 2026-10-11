/*
 * The web page and the routes outside the API that answer in words.
 *
 * The page is one file, gzipped in flash, sent as it is with its ETag, so a
 * browser that has it already is answered 304 and downloads nothing. The page
 * asks the radio for everything else over the API. Here too: joining a Wi-Fi
 * network, which the setup hotspot allows with no PIN, and the plain text
 * answer the routes outside the API give.
 */
#include "debug_log.h"
#include "web_internal.h"

#include "net/web_page.h"
#include "net/wifi_manager.h"
#include "settings_task.h"

#include <WebServer.h>

static WebContext *sWeb = NULL;

void sendResult(int code, const char *message) {
  sWeb->server.sendHeader("Cache-Control", "no-store");
  sWeb->server.send(code, "text/plain", String(message) + "\n");
}

/* GET /. Asked again each time, never cached for good: a new firmware brings
 * a new page to the same address, and the ETag says when it has. */
static void handlePage(void) {
  sWeb->server.sendHeader("Cache-Control", "no-cache");
  sWeb->server.sendHeader("ETag", kWebPageEtag);
  if (sWeb->server.header("If-None-Match") == kWebPageEtag) {
    sWeb->server.send(304, "text/plain", "");
    return;
  }
  sWeb->server.sendHeader("Content-Encoding", "gzip");
  sWeb->server.send_P(200, "text/html", (PGM_P)kWebPageGz, kWebPageGzLen);
}

/* POST /wifi. The network to join, from the setup hotspot with no PIN, or
 * with the PIN otherwise. */
static void handleWifi(void) {
  if (!requireAuth(true)) {
    return;
  }
  if (!sWeb->server.hasArg("sid")) {
    sendResult(400, "A network name is needed.");
    return;
  }
  String ssid = sWeb->server.arg("sid");
  String pass =
      sWeb->server.hasArg("pwd") ? sWeb->server.arg("pwd") : String("");
  Settings pending = *sWeb->settings;
  if (!settingsSetWifi(&pending, ssid.c_str(), pass.c_str())) {
    sendResult(400, "That network name or passphrase is too long.");
    return;
  }
  if (!settingsTaskStore(&pending)) {
    sendResult(500, "Nothing saved: the settings could not be written.");
    return;
  }
  DebugLog.printf("[web] new credentials saved for %s\n", pending.wifiSsid);
  sendResult(200,
             "Saved. The radio is trying that network now. If it works it "
             "will be on tef668x.local:8080, and this hotspot will stop.");
  /* Answer first, then move the radio, or the reply never reaches the browser
   * that is connected to the hotspot being torn down. NetworkClient has no
   * send buffer to flush in this core, so closing the socket is what puts the
   * bytes on the wire. */
  sWeb->server.client().stop();
  delay(200);
  wifiRetryNow();
}

void webPageRegisterRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/", HTTP_GET, handlePage);
  sWeb->server.on("/wifi", HTTP_POST, handleWifi);
}
