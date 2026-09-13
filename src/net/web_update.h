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

/* How long a session cookie stays good for, in seconds. */
#define WEB_SESSION_TTL_SECONDS 1800UL

void webBegin(Settings *settings, uint32_t accessPin);

void webLoop(void);

bool webHasServed(void);

uint32_t webRequestCount(void);

#endif /* NET_WEB_UPDATE_H */
