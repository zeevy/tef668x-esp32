/**
 * @file web_update.cpp
 * @brief Implementation of the status page, the setup form and the uploader.
 */
#include "web_update.h"

#include "board/board.h"
#include "core/access_pin.h"
#include "core/version.h"
#include "drivers/settings_nvs.h"
#include "net/rollback.h"
#include "net/wifi_manager.h"

#include <Update.h>
#include <WebServer.h>
#include <esp_random.h>
#include <esp_system.h>

static WebServer sServer(80);
static Settings *sSettings = NULL;
static uint32_t sAccessPin = 0;
static AccessPinGate sGate;
static uint32_t sRequests = 0;

/** Hex session token, or empty when nobody is signed in. */
static char sSessionToken[33] = "";

/** Millisecond count the session stops being accepted at. */
static uint32_t sSessionExpiresMs = 0;

/** Set while an upload is running and the client was not signed in. */
static bool sUploadRejected = false;

/** Set once a multipart part carrying a file has actually been seen. */
static bool sUploadStarted = false;

/** Set on the first write failure, so the rest of the body is taken quietly. */
static bool sUploadFailed = false;

/** Reboot once the reply has gone out, rather than cutting it off. */
static bool sRebootAfterReply = false;

/* ------------------------------------------------------------------ helpers */

/** True when the radio is serving its own access point. */
static bool inSetupMode(void) {
  return wifiState() == WIFI_STATE_ACCESS_POINT;
}

/** Replace HTML metacharacters, so a network name cannot inject markup. */
static String escapeHtml(const char *raw) {
  String out;
  for (const char *p = raw; *p != '\0'; p++) {
    switch (*p) {
      case '&':
        out += F("&amp;");
        break;
      case '<':
        out += F("&lt;");
        break;
      case '>':
        out += F("&gt;");
        break;
      case '"':
        out += F("&quot;");
        break;
      case '\'':
        out += F("&#39;");
        break;
      default:
        out += *p;
        break;
    }
  }
  return out;
}

/** Make a fresh session token from the hardware random number generator. */
static void newSession(void) {
  static const char kHex[] = "0123456789abcdef";
  for (int i = 0; i < 32; i += 8) {
    uint32_t chunk = esp_random();
    for (int b = 0; b < 8; b++) {
      sSessionToken[i + b] = kHex[(chunk >> (b * 4)) & 0x0F];
    }
  }
  sSessionToken[32] = '\0';
  sSessionExpiresMs = millis() + WEB_SESSION_TTL_SECONDS * 1000UL;
}

/** Forget the session, so the cookie stops working. */
static void dropSession(void) {
  sSessionToken[0] = '\0';
  sSessionExpiresMs = 0;
}

/** Compare two equal length strings without leaking where they differ. */
static bool constantTimeEqual(const char *a, const char *b, size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; i++) {
    diff |= (uint8_t)(a[i] ^ b[i]);
  }
  return diff == 0;
}

/** True when this request carries a live session cookie. */
static bool signedIn(void) {
  if (sSessionToken[0] == '\0') {
    return false;
  }
  if ((int32_t)(millis() - sSessionExpiresMs) >= 0) {
    dropSession();
    return false;
  }
  if (!sServer.hasHeader("Cookie")) {
    return false;
  }
  String cookie = sServer.header("Cookie");
  /* Match the name at the start of the header or after a separator, so a
   * cookie called mytefsid is not mistaken for ours. */
  int at = cookie.startsWith("tefsid=") ? 0 : cookie.indexOf("; tefsid=");
  if (at < 0) {
    return false;
  }
  if (at > 0) {
    at += 2;
  }
  String token = cookie.substring(at + 7);
  int end = token.indexOf(';');
  if (end >= 0) {
    token = token.substring(0, end);
  }
  if (token.length() != 32) {
    return false;
  }
  return constantTimeEqual(token.c_str(), sSessionToken, 32);
}

/**
 * Gate for anything that changes the radio.
 *
 * @param allowInSetupMode  True for saving Wi-Fi credentials, which has to
 *                          work on the access point or there is no way back.
 * @return true when the request may go ahead. Sends the refusal itself when
 *         it may not.
 */
static bool requireAuth(bool allowInSetupMode) {
  if (allowInSetupMode && inSetupMode()) {
    return true;
  }
  if (signedIn()) {
    return true;
  }
  sServer.send(403, "text/plain", "Enter the access PIN first.\n");
  return false;
}

/* --------------------------------------------------------------- the pages */

/**
 * Shared page head.
 *
 * Bootstrap 5 comes from a CDN, which is what makes this readable on a phone
 * without shipping a stylesheet. The catch is that the setup page is served
 * on the radio's own access point, where there is no internet and the CDN
 * cannot load. So the small stylesheet below is not decoration: it is what
 * the page falls back to, and it has to be enough on its own. Phase 5 moves
 * Bootstrap onto the filesystem and drops the CDN.
 */
static String pageHead(const char *title) {
  String out;
  out.reserve(1500);
  out +=
      F("<!doctype html><html lang=en data-bs-theme=dark><head>"
        "<meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>");
  out += title;
  out +=
      F("</title>"
        "<link rel=stylesheet crossorigin=anonymous "
        "href='https://cdn.jsdelivr.net/npm/bootstrap@" BOOTSTRAP_VERSION
        "/dist/css/bootstrap.min.css'>"
        "<style>"
        /* Enough on its own when the CDN cannot be reached. */
        "body{background:#0b0f13;color:#e6e6e6;"
        "font:15px/1.5 system-ui,sans-serif;margin:0;padding:16px}"
        ".wrap{max-width:680px;margin:0 auto}"
        "h1{font-size:22px}h2{font-size:16px;color:#ffb200;margin-top:26px}"
        "table{width:100%;border-collapse:collapse}"
        "td{padding:7px 0;border-bottom:1px solid #24313d}"
        "input,button{font:inherit}"
        /* Bootstrap is themed to match the radio rather than the other
            * way round, so the two look the same either side of a CDN
            * failure. */
        ":root{--bs-body-bg:#0b0f13;--bs-body-color:#e6e6e6;"
        "--bs-border-color:#24313d;--bs-primary:#ffb200;"
        "--bs-link-color:#4ac2ee;--bs-link-hover-color:#7bd4f3}"
        ".card{background:#121a22;border-color:#24313d}"
        ".btn-primary{--bs-btn-bg:#ffb200;--bs-btn-border-color:#ffb200;"
        "--bs-btn-color:#0b0f13;--bs-btn-hover-bg:#ffc340;"
        "--bs-btn-hover-border-color:#ffc340;--bs-btn-hover-color:#0b0f13;"
        "--bs-btn-active-bg:#e6a000;--bs-btn-active-border-color:#e6a000}"
        ".form-control{background:#121a22;border-color:#24313d;"
        "color:#e6e6e6}"
        ".form-control:focus{background:#121a22;color:#e6e6e6;"
        "border-color:#ffb200;box-shadow:none}"
        ".table{--bs-table-color:#e6e6e6;--bs-table-bg:transparent;"
        "--bs-table-border-color:#24313d}"
        "h2{font-size:16px;color:#ffb200;letter-spacing:.02em}"
        "</style></head><body><div class='wrap container-sm py-3'>");
  return out;
}

/** Shared page tail. */
static const char *pageTail(void) {
  return "</div></body></html>";
}

/** The status page, with whichever forms the caller is allowed to use. */
static void handleRoot(void) {
  sRequests++;
  char pin[ACCESS_PIN_DIGITS + 1];
  accessPinFormat(sAccessPin, pin);
  (void)pin; /* Never sent to the browser. It is read off the radio. */

  String out = pageHead("TEF668X");
  out +=
      F("<h1 class='h4 mb-0' style='letter-spacing:.04em'>TEF668X</h1>"
        "<p class='text-secondary small mb-3' "
        "style='letter-spacing:.04em'>");
  out += F(BOARD_NAME_DISPLAY " &middot; V" FIRMWARE_VERSION);
  out += F("</p>");

  if (accessPinIsDefault(sAccessPin)) {
    out +=
        F("<div class='alert alert-danger py-2 px-3 small' role=alert "
          "style='background:#3a1410;border:1px solid #7a2a1e;"
          "color:#ffb4a2'><strong>This radio is on the default access PIN, "
          "000000.</strong> Anyone who can reach it on the network can "
          "change its settings and replace its firmware. Set your own PIN "
          "below.</div>");
  }

  out += F("<h2>Status</h2><table class='table table-sm align-middle'>");
  out += "<tr><td class='text-secondary'>Network</td><td>" +
         escapeHtml(wifiNetworkName()) + "</td></tr>";
  out += "<tr><td>Address</td><td>" + String(wifiAddress()) + "</td></tr>";
  out += "<tr><td>Mode</td><td>";
  out += inSetupMode() ? F("access point, setup") : F("joined a network");
  out += F("</td></tr>");
  out += "<tr><td>Running from</td><td>" + String(rollbackRunningPartition()) +
         "</td></tr>";
  out += "<tr><td>Image</td><td class=";
  out += rollbackPending() ? F("warn>") : F("ok>");
  out += String(rollbackStateText()) + "</td></tr>";
  out += "<tr><td>Free heap</td><td>" + String(ESP.getFreeHeap()) +
         " bytes</td></tr>";
  out += "<tr><td>Up for</td><td>" + String(millis() / 1000UL) +
         " seconds</td></tr>";
  out += F("</table>");

  if (inSetupMode()) {
    out +=
        F("<h2>Wi-Fi</h2>"
          "<p class='text-secondary small'>The radio could not join a network, "
          "so it is "
          "serving this page on its own. Enter the details and it will "
          "try again.</p>"
          "<form method=post action='/wifi'>"
          "<label class='form-label small text-secondary'>Network name</label>"
          "<input class=form-control name=ssid maxlength=32 required>"
          "<label class='form-label small text-secondary'>Passphrase, leave "
          "empty for an open network</label>"
          "<input class=form-control name=pass type=password maxlength=64>"
          "<button class='btn btn-primary mt-3' type=submit>Save and "
          "join</button></form>");
  }

  if (signedIn()) {
    if (!inSetupMode()) {
      out += F(
          "<h2>Wi-Fi</h2>"
          "<form method=post action='/wifi'>"
          "<label class='form-label small text-secondary'>Network name</label>"
          "<input class=form-control name=ssid maxlength=32 required value='");
      out += escapeHtml(sSettings->wifiSsid);
      out +=
          F("'><label class='form-label small text-secondary mt-2'>Passphrase, "
            "leave empty for an open network</label>"
            "<input class=form-control name=pass type=password maxlength=64>"
            "<button class='btn btn-primary mt-3' type=submit>Save and "
            "join</button></form>");
    }
    out +=
        F("<h2>Firmware</h2>"
          "<p class='text-secondary small'>Pick the .bin from "
          ".pio/build/ats125/firmware.bin. The radio reboots into it and "
          "puts the old one back on its own if it does not come up.</p>"
          "<form method=post action='/update' enctype='multipart/form-data'>"
          "<input class=form-control type=file name=firmware accept='.bin' "
          "required>"
          "<button class='btn btn-primary mt-3' type=submit>Upload and "
          "reboot</button></form>"
          "<h2>Access PIN</h2>"
          "<p class='text-secondary small'>Six digits. Changing it takes "
          "effect at once and "
          "signs every browser out, this one included.</p>"
          "<form method=post action='/setpin'>"
          "<label class='form-label small text-secondary'>New PIN</label>"
          "<input class=form-control name=pin inputmode=numeric "
          "pattern='[0-9]{6}' maxlength=6 required>"
          "<button class='btn btn-primary mt-3' type=submit>Change "
          "PIN</button></form>"
          "<h2>Reboot</h2>"
          "<form method=post action='/reboot'>"
          "<button class='btn btn-outline-secondary mt-2' type=submit>Reboot "
          "now</button></form>");
  } else {
    out +=
        F("<h2>Access PIN</h2>"
          "<p class='text-secondary small'>Six digits. A new radio is on "
          "000000. Five wrong "
          "tries locks this for a minute.</p>"
          "<form method=post action='/auth'>"
          "<label class='form-label small text-secondary'>PIN</label>"
          "<input class=form-control name=pin inputmode=numeric "
          "pattern='[0-9]{6}' maxlength=6 required>"
          "<button class='btn btn-primary mt-3' "
          "type=submit>Unlock</button></form>");
  }

  out += pageTail();
  sServer.send(200, "text/html", out);
}

/** A short page that says what happened and links back. */
static void sendResult(int code, const char *title, const char *message,
                       bool bad) {
  String out = pageHead(title);
  out += F("<h1 class=h5>");
  out += title;
  out += F("</h1><p style='color:");
  out += bad ? F("#ff8a72'>") : F("#31c29c'>");
  out += message;
  out += F("</p><p><a href='/' style='color:#4ac2ee'>Back</a></p>");
  out += pageTail();
  sServer.send(code, "text/html", out);
}

/** Check a submitted PIN and hand out a session cookie. */
static void handleAuth(void) {
  sRequests++;
  uint32_t now = millis();

  if (accessPinGateLocked(&sGate, now)) {
    uint32_t waitMs = accessPinGateRetryAfterMs(&sGate, now);
    sServer.sendHeader("Retry-After", String((waitMs + 999UL) / 1000UL));
    sendResult(429, "Too many tries",
               "Too many wrong PINs. Try again in a minute.", true);
    return;
  }

  uint32_t given = 0;
  if (!sServer.hasArg("pin") ||
      !accessPinParse(sServer.arg("pin").c_str(), &given)) {
    /* A malformed PIN still counts, or the six digit check is free to skip. */
    accessPinGateCheck(&sGate, sAccessPin, sAccessPin + 1, now);
    sendResult(400, "Wrong PIN", "A PIN is six digits.", true);
    return;
  }

  if (!accessPinGateCheck(&sGate, sAccessPin, given, now)) {
    Serial.printf("[web] wrong PIN from %s\n",
                  sServer.client().remoteIP().toString().c_str());
    sendResult(403, "Wrong PIN", "That PIN is not right.", true);
    return;
  }

  newSession();
  String cookie = "tefsid=";
  cookie += sSessionToken;
  cookie += "; Path=/; Max-Age=" + String(WEB_SESSION_TTL_SECONDS) +
            "; HttpOnly; SameSite=Strict";
  sServer.sendHeader("Set-Cookie", cookie);
  sServer.sendHeader("Location", "/");
  sServer.send(303, "text/plain", "");
}

/** Save Wi-Fi credentials and try them straight away. */
static void handleWifi(void) {
  sRequests++;
  if (!requireAuth(true)) {
    return;
  }
  if (!sServer.hasArg("ssid")) {
    sendResult(400, "Nothing saved", "A network name is needed.", true);
    return;
  }

  String ssid = sServer.arg("ssid");
  String pass = sServer.hasArg("pass") ? sServer.arg("pass") : String("");

  Settings pending = *sSettings;
  if (!settingsSetWifi(&pending, ssid.c_str(), pass.c_str())) {
    sendResult(400, "Nothing saved",
               "That network name or passphrase is too long.", true);
    return;
  }
  if (!settingsNvsSave(&pending)) {
    sendResult(500, "Nothing saved", "The settings could not be written.",
               true);
    return;
  }
  *sSettings = pending;

  Serial.printf("[web] new credentials saved for %s\n", pending.wifiSsid);
  sendResult(200, "Saved",
             "Saved. The radio is trying that network now. If it works it "
             "will be on tef668x.local, and this access point will stop.",
             false);

  /* Answer first, then move the radio, or the reply never reaches the browser
   * that is connected to the access point being torn down. NetworkClient has
   * no send buffer to flush in this core, so closing the socket is what puts
   * the bytes on the wire. */
  sServer.client().stop();
  delay(200);
  wifiRetryNow(sSettings);
}

/** Take the uploaded image, chunk by chunk. */
static void handleUploadData(void) {
  /* WebServer calls this same callback for two different things. For a
   * multipart upload it fills _currentUpload first. For any other POST body it
   * takes the raw path instead, leaves _currentUpload null, and calls this
   * anyway, so sServer.upload() dereferences a null pointer and the radio
   * panics. There is no accessor that says which one happened, so the content
   * type is what tells them apart. */
  if (!sServer.header("Content-Type").startsWith("multipart/")) {
    return;
  }

  HTTPUpload &upload = sServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    sUploadRejected = !signedIn();
    sUploadStarted = true;
    sUploadFailed = false;
    if (sUploadRejected) {
      Serial.printf("[web] upload refused, no PIN, from %s\n",
                    sServer.client().remoteIP().toString().c_str());
      return;
    }
    Serial.printf("[web] upload started: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Update.printError(Serial);
      sUploadFailed = true;
    }
    return;
  }

  if (sUploadRejected) {
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (sUploadFailed) {
      /* Already dead. Take the rest of the body quietly rather than printing
       * an error line for every chunk of a large file. */
      return;
    }
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
      sUploadFailed = true;
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("[web] upload finished, %u bytes written\n",
                    (unsigned)upload.totalSize);
    } else {
      Update.printError(Serial);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    Serial.println("[web] upload aborted");
  }
}

/** Reply once the whole upload has been taken. */
static void handleUploadDone(void) {
  sRequests++;
  bool rejected = sUploadRejected;
  bool sawImage = sUploadStarted;
  sUploadRejected = false;
  sUploadStarted = false;

  /* The upload handler only runs for a multipart part that carries a
   * filename. A POST with no file part never reaches it, so the PIN has to be
   * checked here as well or an unauthenticated request reboots the radio. */
  if (rejected || !signedIn()) {
    sServer.send(403, "text/plain", "Enter the access PIN first.\n");
    return;
  }

  if (!sawImage) {
    sendResult(400, "Nothing uploaded", "No firmware file was sent.", true);
    return;
  }

  if (Update.hasError()) {
    sendResult(500, "Update failed",
               "The image was not accepted. The radio is still running the "
               "old one.",
               true);
    return;
  }

  sendResult(200, "Update written",
             "The radio is rebooting into the new image. If it does not come "
             "up and pass its self check, the old image comes back on its "
             "own.",
             false);
  sRebootAfterReply = true;
}

/** Change the access PIN. */
static void handleSetPin(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }

  uint32_t wanted = 0;
  if (!sServer.hasArg("pin") ||
      !accessPinParse(sServer.arg("pin").c_str(), &wanted)) {
    sendResult(400, "PIN not changed", "A PIN is six digits.", true);
    return;
  }

  Settings pending = *sSettings;
  pending.accessPin = wanted;
  if (!settingsNvsSave(&pending)) {
    sendResult(500, "PIN not changed", "The settings could not be written.",
               true);
    return;
  }
  *sSettings = pending;
  sAccessPin = wanted;

  /* The old session was opened with the old PIN, so it goes. */
  dropSession();
  accessPinGateReset(&sGate);
  Serial.println("[web] the access PIN was changed");

  sendResult(200, "PIN changed",
             accessPinIsDefault(wanted)
                 ? "That is the default PIN, so the radio is still open to "
                   "anyone on the network. Sign in again."
                 : "Sign in again with the new PIN.",
             accessPinIsDefault(wanted));
}

/** Reboot on request. */
static void handleReboot(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  sendResult(200, "Rebooting", "The radio is restarting.", false);
  sRebootAfterReply = true;
}

/** Small JSON for scripts and for the test checklist. */
static void handleStatusJson(void) {
  sRequests++;
  String out;
  out.reserve(320);
  out += F("{\"board\":\"" BOARD_NAME "\",\"version\":\"" FIRMWARE_VERSION
           "\",\"partition\":\"");
  out += rollbackRunningPartition();
  out += F("\",\"imageConfirmed\":");
  out += rollbackPending() ? F("false") : F("true");
  out += F(",\"mode\":\"");
  out += inSetupMode() ? F("accessPoint") : F("station");
  out += F("\",\"address\":\"");
  out += wifiAddress();
  out += F("\",\"pinIsDefault\":");
  out += accessPinIsDefault(sAccessPin) ? F("true") : F("false");
  out += F(",\"freeHeap\":");
  out += String(ESP.getFreeHeap());
  out += F(",\"uptimeSeconds\":");
  out += String(millis() / 1000UL);
  out += F("}");
  sServer.send(200, "application/json", out);
}

/** Anything else. */
static void handleNotFound(void) {
  sRequests++;
  sServer.sendHeader("Location", "/");
  sServer.send(302, "text/plain", "");
}

/* ----------------------------------------------------------------- the API */

void webBegin(Settings *settings, uint32_t accessPin) {
  sSettings = settings;
  sAccessPin = accessPin;
  accessPinGateReset(&sGate);
  dropSession();

  /* Content-Type is collected because handleUploadData needs it to tell a
   * real upload from a raw POST body. */
  const char *keep[] = {"Cookie", "Content-Type"};
  sServer.collectHeaders(keep, 2);

  sServer.on("/", HTTP_GET, handleRoot);
  sServer.on("/status.json", HTTP_GET, handleStatusJson);
  sServer.on("/auth", HTTP_POST, handleAuth);
  sServer.on("/wifi", HTTP_POST, handleWifi);
  sServer.on("/update", HTTP_POST, handleUploadDone, handleUploadData);
  sServer.on("/setpin", HTTP_POST, handleSetPin);
  sServer.on("/reboot", HTTP_POST, handleReboot);
  sServer.onNotFound(handleNotFound);
  sServer.begin();
}

void webLoop(void) {
  sServer.handleClient();

  if (sRebootAfterReply) {
    sRebootAfterReply = false;
    Serial.println("[web] rebooting on request");
    Serial.flush();
    delay(200);
    ESP.restart();
  }
}

bool webHasServed(void) {
  return sRequests > 0;
}

uint32_t webRequestCount(void) {
  return sRequests;
}
