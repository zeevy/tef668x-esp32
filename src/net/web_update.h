/*
 * The browser way in: status, Wi-Fi setup and firmware upload.
 *
 * This is the second way to flash the radio, and the one a user without
 * PlatformIO has. It also serves the setup page on the access point, which is
 * how wrong Wi-Fi credentials get fixed without a cable.
 *
 * Anything that changes the radio needs the access PIN. The one exception is
 * saving Wi-Fi credentials while the radio is serving its own access point,
 * because that is the recovery path and the user has no other way in.
 */
#ifndef NET_WEB_UPDATE_H
#define NET_WEB_UPDATE_H

#include "core/settings.h"

#include <Arduino.h>

/*
 * The port the pages and the API listen on.
 *
 * Every URL carries it: the dashboard, the setup page on the access point,
 * the control API and every `curl` call. A browser given the bare
 * host name gets nothing, and that is the cost of not being on 80. The
 * recovery path pays it too, so the setup page is `http://192.168.4.1:8080/`
 * and a phone has to be told the port.
 *
 * mDNS advertises this port, so anything that finds the radio by name finds
 * the right port with it.
 */
#define WEB_PORT 8080

/* How long a session cookie stays good for, in seconds. */
#define WEB_SESSION_TTL_SECONDS 1800UL

void webBegin(Settings *settings, uint32_t accessPin);

void webLoop(void);

/*
 * Make `pin` the access PIN everywhere it is used, once the caller has
 * stored it: the web sign in and the OTA password. The session was opened
 * with the old PIN, so it goes, and wrong guesses at the old PIN stop
 * counting against the new one. The one place a PIN change happens, so the
 * browser, the API and the panel's menu cannot do different things.
 */
void webPinChanged(uint32_t pin);

#endif /* NET_WEB_UPDATE_H */
