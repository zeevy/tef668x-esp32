/*
 * What the web_*.cpp files share: the server handle, the session and
 * upload state, and the handful of helpers each needs from another's job.
 *
 * Not installed outside net/: nothing beyond web_update.h's own three
 * functions is any other layer's business.
 */
#ifndef NET_WEB_INTERNAL_H
#define NET_WEB_INTERNAL_H

#include "core/access_pin.h"
#include "core/settings.h"

#include <Arduino.h>
#include <WebServer.h>

/*
 * What every web file needs from the server: the server itself, the
 * settings it reads and writes, the count of requests served, and the flag
 * that reboots once a reply has gone out, which web_update.cpp's loop acts
 * on. One of them, owned by web_update.cpp and handed to each file as it
 * registers its routes, rather than shared as globals.
 */
struct WebContext {
  WebServer &server;
  Settings *settings;
  uint32_t requests;
  bool rebootAfterReply;
};

/* web_auth.cpp's: the PIN, the session and the two routes that use them. */
bool inSetupMode(void);
bool signedIn(void);
bool requireAuth(bool allowInSetupMode);
void dropSession(void);
void webAuthBegin(uint32_t accessPin);
void webAuthRegisterRoutes(WebContext *web);

/* Whether the access PIN is still the one every radio ships with. */
bool webPinIsDefault(void);

/* web_pages.cpp's own reply and redirect helpers. web_auth.cpp's sign in and
 * PIN handlers use both, and web_update.cpp's upload and reboot handlers use
 * sendResult. */
void sendResult(int code, const char *title, const char *message, bool bad);
const char *safeNext(const String &want);

/* web_state.cpp's own document, which web_api_tune.cpp serves as
 * GET /api/state, and its JSON escape, which the web_api_*.cpp files use. */
String buildState(void);
String jsonEscape(const char *raw);

void webPagesRegisterRoutes(WebContext *web);
void webApiRegisterRoutes(WebContext *web);
void webStateRegisterRoutes(WebContext *web);

#endif /* NET_WEB_INTERNAL_H */
