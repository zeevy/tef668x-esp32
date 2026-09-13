/**
 * @file wifi_manager.h
 * @brief Joins the stored network, and starts an access point when it cannot.
 *
 * Wrong credentials must never mean reaching for the USB cable, so a failed
 * join always ends in an access point the user can reach from a phone.
 */
#ifndef NET_WIFI_MANAGER_H
#define NET_WIFI_MANAGER_H

#include "core/settings.h"

#include <Arduino.h>

/** How long to wait for a join before giving up and starting the AP. */
#define WIFI_JOIN_TIMEOUT_MS 20000UL

/** How often to retry the stored network while the AP is up. */
#define WIFI_AP_RETRY_INTERVAL_MS 300000UL

/**
 * Failed status polls to tolerate before giving the station up.
 *
 * The loop polls every few milliseconds, so this is only a debounce against
 * a single bad read. A router that really has gone still drops the radio to
 * its access point within a second.
 */
#define WIFI_DROP_TOLERANCE 200

/** Where the radio's network connection has got to. */
typedef enum {
  WIFI_STATE_OFFLINE,     /**< Nothing running yet. */
  WIFI_STATE_JOINING,     /**< Trying the stored credentials. */
  WIFI_STATE_ONLINE,      /**< Joined, with an IP address. */
  WIFI_STATE_ACCESS_POINT /**< Serving its own network so it can be fixed. */
} WifiState;

/**
 * Bring the network up.
 *
 * Tries the stored credentials if there are any. Falls back to an access
 * point when there are none, or when the join times out.
 *
 * @param settings  The stored settings. Not modified.
 */
void wifiBegin(const Settings *settings);

/** Keep the connection going. Call from the main loop. */
void wifiLoop(void);

/**
 * Where the connection has got to.
 *
 * @return The current state.
 */
WifiState wifiState(void);

/**
 * Whether anything can reach the radio right now.
 *
 * @return true when it is on a network or serving its own access point.
 */
bool wifiReachable(void);

/**
 * Whether the radio joined the network it was told to join.
 *
 * Narrower than wifiReachable, which also counts the fallback access point.
 * The access point means the stored credentials did not work, so an image
 * judged on wifiReachable would mark itself good while unable to reach the
 * network it is meant to be on, and the way back would be the cable flash
 * that the rollback exists to avoid.
 *
 * @return true only when it is on the stored network.
 */
bool wifiJoinedNetwork(void);

/**
 * The radio's address as text, on the network or on its own access point.
 *
 * @return A dotted quad. Never NULL.
 */
const char *wifiAddress(void);

/**
 * The network the radio is on, or the access point it is serving.
 *
 * @return The name. Never NULL.
 */
const char *wifiNetworkName(void);

/**
 * Try the stored credentials again without waiting for the retry timer.
 *
 * Used right after new credentials are saved, so the user sees whether they
 * worked instead of waiting five minutes.
 *
 * @param settings  The settings holding the credentials to try.
 * @return true when the radio ended up online.
 */
bool wifiRetryNow(const Settings *settings);

#endif /* NET_WIFI_MANAGER_H */
