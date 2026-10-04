/* When to try the network. No hardware, no waiting. */
#include "wifi_join.h"

#include <stddef.h>

/* Start a join and note when, so the timeout has something to measure. */
static WifiJoinAction beginJoin(WifiJoin *j, uint32_t nowMs) {
  j->tried = true;
  j->state = WIFI_STATE_JOINING;
  j->joinStartedMs = nowMs;
  j->dropping = false;
  j->droppedAtMs = 0;
  return WIFI_DO_BEGIN_JOIN;
}

/*
 * Give up on the station and serve the setup page instead.
 *
 * The retry clock starts here rather than when the join started, so the
 * interval is time spent on the access point and not time spent trying.
 */
static WifiJoinAction startAp(WifiJoin *j, uint32_t nowMs) {
  j->tried = true;
  j->state = WIFI_STATE_ACCESS_POINT;
  j->lastRetryMs = nowMs;
  j->dropping = false;
  j->droppedAtMs = 0;
  return WIFI_DO_START_AP;
}

/*
 * No network at all: Wi-Fi is switched off, or the hotspot is Off and there
 * is nothing to join.
 *
 * `tried` goes back to false, since nothing failed that needs backing off
 * from: a hotspot set back to Auto then comes up on the next pass.
 */
static WifiJoinAction turnOff(WifiJoin *j) {
  j->tried = false;
  j->state = WIFI_STATE_OFFLINE;
  j->dropping = false;
  j->droppedAtMs = 0;
  return WIFI_DO_TURN_OFF;
}

void wifiJoinReset(WifiJoin *j) {
  if (j == NULL) {
    return;
  }
  j->state = WIFI_STATE_OFFLINE;
  j->joinStartedMs = 0;
  j->lastRetryMs = 0;
  j->dropping = false;
  j->droppedAtMs = 0;
  j->tried = false;
}

bool wifiJoinWantsApClients(const WifiJoin *j, uint32_t nowMs,
                            bool hasCredentials) {
  if (j == NULL || !hasCredentials || j->state != WIFI_STATE_ACCESS_POINT) {
    return false;
  }
  return (uint32_t)(nowMs - j->lastRetryMs) >= WIFI_AP_RETRY_INTERVAL_MS;
}

WifiJoinAction wifiJoinStep(WifiJoin *j, const WifiJoinInput *in) {
  if (j == NULL || in == NULL) {
    return WIFI_DO_NOTHING;
  }

  /* Switched off: nothing runs, and nothing is asked for again until it is
   * switched on, when the state is OFFLINE with nothing tried, so the next
   * pass starts at once. */
  if (!in->wifiOn) {
    return j->state == WIFI_STATE_OFFLINE ? WIFI_DO_NOTHING : turnOff(j);
  }

  /* On: the hotspot and nothing else, whatever is stored or asked for. */
  if (in->hotspot == WIFI_HOTSPOT_ON) {
    return j->state == WIFI_STATE_ACCESS_POINT ? WIFI_DO_NOTHING
                                               : startAp(j, in->nowMs);
  }
  /* Off: where Auto would give up and serve the hotspot, try again. */
  const bool off = in->hotspot == WIFI_HOTSPOT_OFF;

  /*
   * Somebody has just saved credentials, so try them whatever else is going
   * on. It comes first because the alternative is telling a person who typed
   * a password thirty seconds ago that the radio will get round to it in five
   * minutes.
   */
  if (in->retryNow && in->hasCredentials) {
    return beginJoin(j, in->nowMs);
  }

  switch (j->state) {
    case WIFI_STATE_OFFLINE:
      /*
       * Nothing is running. Either this is boot, or the access point would
       * not start, which is the one failure that leaves the radio reachable
       * on nothing at all.
       *
       * Boot goes at once. The failure backs off, because the caller lands
       * back here on the very next pass and a retry every pass is a busy loop
       * on the task this whole file exists to keep free.
       */
      if (j->tried &&
          (uint32_t)(in->nowMs - j->lastRetryMs) < WIFI_AP_RETRY_INTERVAL_MS) {
        return WIFI_DO_NOTHING;
      }
      /* With no credentials there is nothing to try, and the access point is
       * the only way to be given some, unless it is Off. */
      if (!in->hasCredentials) {
        return off ? WIFI_DO_NOTHING : startAp(j, in->nowMs);
      }
      return beginJoin(j, in->nowMs);

    case WIFI_STATE_JOINING:
      if (in->linkUp) {
        j->state = WIFI_STATE_ONLINE;
        j->dropping = false;
        j->droppedAtMs = 0;
        return WIFI_DO_CAME_ONLINE;
      }
      /* Subtraction rather than comparison, so the wrap of millis every
       * forty nine days is one more pass round rather than a join that can
       * never time out. */
      if ((uint32_t)(in->nowMs - j->joinStartedMs) >= WIFI_JOIN_TIMEOUT_MS) {
        return off ? beginJoin(j, in->nowMs) : startAp(j, in->nowMs);
      }
      return WIFI_DO_NOTHING;

    case WIFI_STATE_ONLINE:
      if (in->linkUp) {
        j->dropping = false;
        j->droppedAtMs = 0;
        return WIFI_DO_STAY_ONLINE;
      }
      /* One bad read is not a lost network. The grace period starts at the
       * first one and is measured in time, so it means the same whether the
       * caller asks twice a second or five hundred times. */
      if (!j->dropping) {
        j->dropping = true;
        j->droppedAtMs = in->nowMs;
        return WIFI_DO_NOTHING;
      }
      if ((uint32_t)(in->nowMs - j->droppedAtMs) < WIFI_DROP_GRACE_MS) {
        return WIFI_DO_NOTHING;
      }
      return off ? beginJoin(j, in->nowMs) : startAp(j, in->nowMs);

    case WIFI_STATE_ACCESS_POINT:
      /* Set Off while it was serving: the stored network, or nothing. */
      if (off) {
        return in->hasCredentials ? beginJoin(j, in->nowMs) : turnOff(j);
      }
      if (!in->hasCredentials) {
        return WIFI_DO_NOTHING;
      }
      if ((uint32_t)(in->nowMs - j->lastRetryMs) < WIFI_AP_RETRY_INTERVAL_MS) {
        return WIFI_DO_NOTHING;
      }
      /*
       * A retry takes the access point down to do it. Not while somebody is
       * on it: they are almost certainly on the setup page typing the
       * credentials this retry is about to fail with.
       */
      if (in->apClients > 0) {
        j->lastRetryMs = in->nowMs;
        return WIFI_DO_NOTHING;
      }
      return beginJoin(j, in->nowMs);

    default:
      /* A state outside the enum cannot happen and would index nothing.
       * Starting again is the one recovery that always leaves the radio
       * reachable. */
      wifiJoinReset(j);
      return WIFI_DO_NOTHING;
  }
}
