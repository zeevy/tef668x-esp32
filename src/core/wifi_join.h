/*
 * When to try the network, when to give up on it, and when to try again.
 *
 * All of the deciding and none of the doing. It is here rather than in `net/`
 * because it is timeouts and transitions, which is the shape of thing that
 * goes wrong quietly: a join that never times out looks the same as a network
 * that is slow, and a retry that never fires looks the same as a router that
 * is still down. None of that can be tested with a radio in the room, and all
 * of it can be tested on a PC.
 *
 * The caller owns the radio. It reads the link, calls `wifiJoinStep` once a
 * pass, and carries out the one action it is handed back. Nothing in here
 * touches WiFi, waits, or knows how long anything takes.
 */
#ifndef CORE_WIFI_JOIN_H
#define CORE_WIFI_JOIN_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * How long to wait for a join before giving up and starting the access point.
 *
 * Twenty seconds. It is cheap to wait that long: the join runs alongside
 * everything else, so the radio works throughout, with the Wi-Fi symbol crossed
 * out because it is not on a network.
 */
#define WIFI_JOIN_TIMEOUT_MS 20000UL

/* How often to retry the stored network while the access point is up. */
#define WIFI_AP_RETRY_INTERVAL_MS 300000UL

/*
 * How long the link may read down before the station is given up.
 *
 * A duration and not a count of failed reads. The caller runs this every pass
 * of a loop that ends in `delay(2)`, so a count moves with how busy the loop
 * is, and only a duration can promise how long a short drop may last before
 * it costs five minutes off the air.
 *
 * 400 ms rides out a few failed reads in a row. A longer drop, such as a
 * router restart, gives the station up. With the hotspot on Auto the radio
 * then serves its access point and tries the network again after
 * WIFI_AP_RETRY_INTERVAL_MS. Raise it here and nowhere else.
 */
#define WIFI_DROP_GRACE_MS 400UL

/* Where the radio's network connection has got to. */
typedef enum {
  WIFI_STATE_OFFLINE,     /* Nothing running yet. */
  WIFI_STATE_JOINING,     /* Trying the stored credentials. */
  WIFI_STATE_ONLINE,      /* Joined, with an IP address. */
  WIFI_STATE_ACCESS_POINT /* Serving its own network so it can be fixed. */
} WifiState;

/* The one thing the caller is asked to do this pass. */
typedef enum {
  WIFI_DO_NOTHING = 0,
  WIFI_DO_BEGIN_JOIN,  /* Start the station and try the credentials. */
  WIFI_DO_START_AP,    /* Give up on the station and serve the setup page. */
  WIFI_DO_CAME_ONLINE, /* The join succeeded. Read the address, start mDNS. */
  WIFI_DO_STAY_ONLINE, /* Still joined. Read the address again in case DHCP
                          moved it. */
  WIFI_DO_TURN_OFF     /* Leave the radio on no network: Wi-Fi is switched
                          off, or the hotspot is Off and nothing is stored. */
} WifiJoinAction;

/*
 * What the hotspot, the radio's own access point, is allowed to do. The
 * setting, kept as a byte in Settings.
 */
typedef enum {
  /* Only when the stored network cannot be joined, or none is stored. */
  WIFI_HOTSPOT_AUTO = 0,
  /* Always, in place of the stored network, which is not tried at all. */
  WIFI_HOTSPOT_ON,
  /* Never. The stored network is tried again and again, and with none
   * stored the radio is on no network until the setting changes or the
   * recovery screen starts the hotspot. */
  WIFI_HOTSPOT_OFF,
  WIFI_HOTSPOT_COUNT
} WifiHotspot;

/* What the caller can see about the radio right now. */
typedef struct {
  uint32_t nowMs;
  bool hasCredentials; /* There is an SSID stored to try. */
  bool linkUp;         /* The station is associated and has an address. */
  uint8_t apClients;   /* How many are on the access point. */
  /*
   * Somebody asked for the stored network to be tried now.
   *
   * Set for one pass after credentials are saved, or the hotspot setting
   * changed, so the person who just did it sees the result rather than
   * waiting out the retry interval. It wins over everything but Wi-Fi
   * switched off and a hotspot set On, including the hold that keeps the
   * access point up while a client is on it.
   */
  bool retryNow;
  uint8_t hotspot; /* A WifiHotspot. */
  /* False when Wi-Fi is switched off: no network and no hotspot, whatever
   * else is set. */
  bool wifiOn;
} WifiJoinInput;

/* Everything the decision needs to remember between passes. */
typedef struct {
  WifiState state;
  uint32_t joinStartedMs;
  uint32_t lastRetryMs;
  /*
   * When the link first read down, and whether it has.
   *
   * A separate flag rather than a zero in the timestamp, because zero is a
   * real millisecond: a link that drops in the first pass after boot would
   * otherwise restart its own grace period every pass and never give up.
   */
  bool dropping;
  uint32_t droppedAtMs;
  /*
   * Whether anything has been attempted yet.
   *
   * Tells boot apart from the one failure that lands back on OFFLINE, which
   * is an access point that would not start. Boot should try at once and that
   * failure should back off, and a zero `lastRetryMs` cannot tell them apart
   * because zero is also a real millisecond.
   */
  bool tried;
} WifiJoin;

/* Start in OFFLINE with nothing tried. */
void wifiJoinReset(WifiJoin *j);

/*
 * Advance the decision by one pass and say what to do about it.
 *
 * Call it as often as you like: it is driven by `nowMs` and by the link, not
 * by how many times it has run. A NULL argument does nothing and asks for
 * nothing, because a caller that has not been started yet must not be told to
 * tear the radio down.
 */
WifiJoinAction wifiJoinStep(WifiJoin *j, const WifiJoinInput *in);

/*
 * Whether `apClients` is going to be looked at this pass.
 *
 * Only the access point retry reads it, and only once the interval has
 * elapsed, so on every other pass the caller can leave it at zero rather than
 * asking the radio. Reading it is a call into the Wi-Fi driver, and the
 * caller runs this several hundred times a second.
 *
 * It is here rather than in the caller so that the interval stays in one
 * place. A caller that knows when the retry is due is a caller that has a
 * copy of the decision.
 */
bool wifiJoinWantsApClients(const WifiJoin *j, uint32_t nowMs,
                            bool hasCredentials);

#ifdef __cplusplus
}
#endif

#endif /* CORE_WIFI_JOIN_H */
