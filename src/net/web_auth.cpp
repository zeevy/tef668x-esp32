/*
 * Who may change the radio: the access PIN, the gate that slows down wrong
 * guesses, the one session a right PIN opens, and the two routes that sign
 * in and change the PIN.
 */
#include "web_internal.h"
#include "web_update.h"

#include <mbedtls/sha1.h>
#include <string.h>

#include "core/xdr.h"
#include "net/ota_service.h"
#include "net/wifi_manager.h"
#include "net/xdr_server.h"
#include "settings_task.h"
#include "sleep_task.h"

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

/* Private to this file: everything else asks through webPinIsDefault and
 * changes it through webPinChanged. */
static uint32_t sAccessPin = 0;
static AccessPinGate sGate;
/* The PC Link's own tries, so a PC left with an old PIN, which FM-DX
 * Webserver retries every 2 s, locks out only the link and never the sign
 * in page. */
static AccessPinGate sXdrGate;

/* Hex session token, or empty when nobody is signed in. */
static char sSessionToken[33] = "";

/* Millisecond count the session stops being accepted at. */
static uint32_t sSessionExpiresMs = 0;

bool inSetupMode(void) {
  /* Not with the hotspot set On: the person using the radio chose it and can
   * sign in on it, and a Wi-Fi form open all the time would let anyone in
   * range move the radio to another network. */
  return wifiState() == WIFI_STATE_ACCESS_POINT &&
         (sWeb->settings == NULL || sWeb->settings->hotspot != WIFI_HOTSPOT_ON);
}

static void newSession(void) {
  /* 128 random bits as 32 hex characters. */
  snprintf(sSessionToken, sizeof(sSessionToken), "%08lx%08lx%08lx%08lx",
           (unsigned long)esp_random(), (unsigned long)esp_random(),
           (unsigned long)esp_random(), (unsigned long)esp_random());
  sSessionExpiresMs = millis() + WEB_SESSION_TTL_SECONDS * 1000UL;
}

bool webPinIsDefault(void) {
  return accessPinIsDefault(sAccessPin);
}

void webPinChanged(uint32_t pin) {
  sAccessPin = pin;
  otaSetPin(pin);
  dropSession();
  accessPinGateReset(&sGate);
  accessPinGateReset(&sXdrGate);
  /* The PCs signed in with the old PIN go too, as the browser's session
   * does. */
  xdrServerSignOutAll();
  Serial.println("[web] the access PIN was changed");
}

void dropSession(void) {
  sSessionToken[0] = '\0';
  sSessionExpiresMs = 0;
}

static bool constantTimeEqual(const char *a, const char *b, size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; i++) {
    diff |= (uint8_t)(a[i] ^ b[i]);
  }
  return diff == 0;
}

bool signedIn(void) {
  if (sSessionToken[0] == '\0') {
    return false;
  }
  if ((int32_t)(millis() - sSessionExpiresMs) >= 0) {
    dropSession();
    return false;
  }
  if (!sWeb->server.hasHeader("Cookie")) {
    return false;
  }
  String cookie = sWeb->server.header("Cookie");
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

bool requireAuth(bool allowInSetupMode) {
  const bool allowed = (allowInSetupMode && inSetupMode()) || signedIn();
  /* Every write is through here, so here is where a write counts as using
   * the radio for auto off. A read is not, so a page left polling does not
   * keep the radio awake all night. */
  if (allowed && sWeb->server.method() != HTTP_GET) {
    sleepTaskUsed();
  }
  if (allowed) {
    return true;
  }
  sWeb->server.send(403, "text/plain", "Enter the access PIN first.\n");
  return false;
}

static void handleAuth(void) {
  uint32_t now = millis();

  if (accessPinGateLocked(&sGate, now)) {
    uint32_t waitMs = accessPinGateRetryAfterMs(&sGate, now);
    sWeb->server.sendHeader("Retry-After", String((waitMs + 999UL) / 1000UL));
    sendResult(429, "Too many tries",
               "Too many wrong PINs. Try again in a minute.", true);
    return;
  }

  uint32_t given = 0;
  if (!sWeb->server.hasArg("pin") ||
      !accessPinParse(sWeb->server.arg("pin").c_str(), &given)) {
    /* A malformed PIN still counts, or the six digit check is free to skip. */
    accessPinGateCheck(&sGate, sAccessPin, sAccessPin + 1, now);
    sendResult(400, "Wrong PIN", "A PIN is six digits.", true);
    return;
  }

  if (!accessPinGateCheck(&sGate, sAccessPin, given, now)) {
    Serial.printf("[web] wrong PIN from %s\n",
                  sWeb->server.client().remoteIP().toString().c_str());
    sendResult(403, "Wrong PIN", "That PIN is not right.", true);
    return;
  }

  newSession();
  String cookie = "tefsid=";
  cookie += sSessionToken;
  cookie += "; Path=/; Max-Age=" + String(WEB_SESSION_TTL_SECONDS) +
            "; HttpOnly; SameSite=Strict";
  sWeb->server.sendHeader("Set-Cookie", cookie);
  /* Back to the page the form was on, checked against the pages this
   * firmware serves. An unchecked target out of the request would send the
   * browser wherever the poster liked. */
  sWeb->server.sendHeader(
      "Location", safeNext(sWeb->server.hasArg("nxt") ? sWeb->server.arg("nxt")
                                                      : String("/")));
  sWeb->server.send(303, "text/plain", "");
}

static void handleSetPin(void) {
  if (!requireAuth(false)) {
    return;
  }

  uint32_t wanted = 0;
  if (!sWeb->server.hasArg("pin") ||
      !accessPinParse(sWeb->server.arg("pin").c_str(), &wanted)) {
    sendResult(400, "PIN not changed", "A PIN is six digits.", true);
    return;
  }

  Settings pending = *sWeb->settings;
  pending.accessPin = wanted;
  /* Through the one call every other setting uses: it checks
   * the whole struct, writes it, and counts the write in `asv.n`. */
  if (!settingsTaskStore(&pending)) {
    sendResult(500, "PIN not changed", "The settings could not be written.",
               true);
    return;
  }
  webPinChanged(wanted);

  sendResult(200, "PIN changed",
             accessPinIsDefault(wanted)
                 ? "That is the default PIN, so the radio is still open to "
                   "anyone on the network. Sign in again."
                 : "Sign in again with the new PIN.",
             accessPinIsDefault(wanted));
}

bool webAuthXdrLogin(const char *salt, const char *line) {
  const uint32_t now = millis();
  if (salt == NULL || line == NULL || accessPinGateLocked(&sXdrGate, now)) {
    return false;
  }
  char joined[XDR_SALT_LEN + ACCESS_PIN_DIGITS + 1];
  const size_t saltLen = strnlen(salt, XDR_SALT_LEN);
  memcpy(joined, salt, saltLen);
  accessPinFormat(sAccessPin, joined + saltLen);
  uint8_t digest[XDR_DIGEST_LEN];
  char expected[XDR_DIGEST_HEX + 1];
  const bool hashed = mbedtls_sha1((const unsigned char *)joined,
                                   saltLen + ACCESS_PIN_DIGITS, digest) == 0;
  if (hashed) {
    xdrHex(digest, expected);
  }
  const bool right = hashed && xdrDigestMatches(line, expected);
  accessPinGateCheck(&sXdrGate, sAccessPin, right ? sAccessPin : sAccessPin + 1,
                     now);
  return right;
}

void webAuthBegin(uint32_t accessPin) {
  sAccessPin = accessPin;
  accessPinGateReset(&sGate);
  accessPinGateReset(&sXdrGate);
  dropSession();
}

void webAuthRegisterRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/auth", HTTP_POST, handleAuth);
  sWeb->server.on("/setpin", HTTP_POST, handleSetPin);
}
