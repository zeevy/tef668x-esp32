/*
 * Joins the stored network, and starts an access point, the hotspot, when it
 * cannot.
 *
 * Wrong credentials must never mean reaching for the USB cable, so on Auto, the
 * default, a failed join always ends in a hotspot the user can reach from a
 * phone. It can also be set On, the hotspot alone, or Off, the stored network
 * alone; the recovery screen sets it On again.
 */
#ifndef NET_WIFI_MANAGER_H
#define NET_WIFI_MANAGER_H

#include "core/settings.h"
/* WifiState, the timeouts, and the decision itself. Everything about when to
 * try and when to give up lives there, so that it can be tested on a PC. */
#include "core/wifi_join.h"

#include <Arduino.h>

/*
 * Start trying the network, and return straight away.
 *
 * It does not wait for the join. Nothing in this file waits for anything, and
 * that is the whole point of it: `wifiLoop` runs on the Arduino loop task
 * alongside the panel, the knob, the web server and the over the air update,
 * so a second spent here is a second in which the radio has stopped.
 *
 * With no stored credentials the access point comes straight up, because
 * there is nothing to try and the setup page is the only way to be given
 * something to try.
 */
/* `settings` is kept as a pointer and read on every pass, so it must be
 * the live settings and outlive the radio, as `gSettings` does; a retry
 * reads the same ones, so new credentials are stored there first. */
void wifiBegin(const Settings *settings);

/*
 * Step the join along. Call it once a pass, cheaply and often.
 *
 * It reads the link, asks `core/wifi_join.c` what to do, and does that one
 * thing. On a pass where there is nothing to do it reads one status word and
 * returns.
 */
void wifiLoop(void);

/*
 * All four states, and the only accessor that can tell them apart.
 *
 * The state document and the header symbol both read this rather than a
 * flag each. A radio that is trying is not the same as one that has given
 * up, and one on nothing at all is not the same as either.
 */
WifiState wifiState(void);

/*
 * Whether the radio is on the network its settings ask for: joined to the
 * stored one, or, with the hotspot set On, serving the hotspot, or with
 * Wi-Fi switched off, on none.
 *
 * Not the hotspot on Auto. There it means the stored credentials did not
 * work, so an image judged on reaching it would mark itself good while
 * unable to reach the network it is meant to be on, and the way back would
 * be the cable flash that the rollback exists to avoid. With the hotspot
 * set On, the hotspot is the network asked for, and an image updated over
 * it must be able to keep itself.
 */
bool wifiOnWantedNetwork(void);

const char *wifiAddress(void);

/*
 * The name the radio is announcing on the network it is on, its own hotspot
 * included, without the .local a browser adds to it. NULL while the web
 * server is off or when the announcing failed to start, since the name then
 * finds nothing.
 */
const char *wifiAnnouncedName(void);

const char *wifiNetworkName(void);

/*
 * How strong the joined network's signal is, in dBm.
 *
 * False and `*out` left alone unless `wifiState` is WIFI_STATE_ONLINE. A
 * radio serving its own access point, still joining, or off the network
 * entirely has nothing here to report: RSSI on this chip describes the
 * station side of the radio, not the access point it can also run, and a
 * number handed back for either of the other three states would be read as
 * a link that does not exist.
 */
bool wifiRssiDbm(int8_t *out);

/*
 * Try the stored credentials again without waiting for the retry timer.
 *
 * Used right after new credentials are saved, so the user sees whether they
 * worked instead of waiting five minutes.
 *
 * It only asks. The join happens on the next few passes of `wifiLoop`, and
 * whether it worked shows up in `wifiState`, so the caller is free to answer
 * the browser first and let the radio catch up.
 */
void wifiRetryNow(void);

#endif /* NET_WIFI_MANAGER_H */
