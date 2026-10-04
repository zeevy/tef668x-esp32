/* Implementation of the network time client. */
#include "ntp.h"

#include <Arduino.h>
#include <esp_sntp.h>
#include <time.h>

/*
 * Which servers to ask.
 *
 * The NTP pool, which is what everything else on a home network uses, then
 * NIST and Google. Three of them, run by three different groups, so one
 * unreachable server does not leave the radio without a clock.
 *
 * Not configurable. A server address is one more thing that can be typed
 * wrong, and the failure looks the same as no network at all.
 */
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"
#define NTP_SERVER_3 "time.google.com"

/*
 * The earliest time that counts as a real answer, as a Unix timestamp.
 *
 * 1 January 2020. Before any server has answered, the ESP32's clock reads a
 * few seconds past 1 January 1970, and that is a valid looking time that
 * would print as 00:00 and be believed. Anything below this is the radio's
 * own counter rather than an answer, which is the distinction that matters.
 */
#define NTP_PLAUSIBLE_AFTER 1577836800L

static bool sEnabled = false;
static bool sStarted = false;
static int16_t sOffsetMinutes = 0;

/*
 * When a server last answered.
 *
 * Written from the SNTP client's own task and read from the loop task, so it
 * is volatile. Both are aligned 32 bit words on this chip, which is why a
 * lock is not needed for a value that is only ever written whole.
 */
static volatile uint32_t sLastSyncMs = 0;
static volatile bool sEverSynced = false;

/*
 * Called by the SNTP client every time it sets the clock.
 *
 * This is the only honest source for "when did the time last arrive". Asking
 * `time()` says whether the clock looks plausible, which stays true forever
 * once it has been set even if the network went away hours ago.
 */
static void onTimeSynced(struct timeval *tv) {
  (void)tv;
  sLastSyncMs = millis();
  sEverSynced = true;
}

/*
 * Whether the system clock is holding a real answer.
 *
 * Asked rather than remembered, because the clock also survives a soft reset
 * and can already be right before the first callback of this run arrives.
 */
static bool clockLooksSet(void) {
  return (long)time(NULL) >= NTP_PLAUSIBLE_AFTER;
}

void ntpBegin(const Settings *settings) {
  ntpApply(settings);
}

void ntpApply(const Settings *settings) {
  if (settings == NULL) {
    return;
  }
  sOffsetMinutes = settings->clockOffsetMinutes;
  const bool wanted = settings->ntpEnabled != 0;

  if (wanted && !sStarted) {
    sntp_set_time_sync_notification_cb(onTimeSynced);
    /* The offset arguments are left at zero and the offset is applied in
     * core/clock.c instead. Two places that both know how to shift a time is
     * two places to get the day rollover wrong, and only one of them can be
     * tested on a PC. */
    configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    sStarted = true;
  }

  /*
   * Turning it off is `sEnabled` and nothing else. Nothing stops the SNTP
   * client cleanly once configTime has started it, so the clock keeps being
   * set in the background; what changes is that ntpSynchronised goes false,
   * which takes the time off the screen at once. A clock left there with
   * nothing updating it would still look like a clock.
   *
   * `sEverSynced` is deliberately not cleared. It records whether a server
   * has answered during this run, which stays true whether or not anybody
   * wants the clock shown, and clearing it would make ntpSecondsSinceSync
   * report "never" next to a clock that is visibly working the moment the
   * setting was switched back on.
   */
  sEnabled = wanted;
}

bool ntpSynchronised(void) {
  return sEnabled && clockLooksSet();
}

int16_t ntpOffsetMinutes(void) {
  return sOffsetMinutes;
}

ClockTime ntpLocalTime(void) {
  ClockTime t;
  t.hour = 0;
  t.minute = 0;
  t.known = false;
  if (!ntpSynchronised()) {
    return t;
  }
  const time_t now = time(NULL);
  struct tm utc;
  if (gmtime_r(&now, &utc) == NULL) {
    return t;
  }
  return clockLocal((int32_t)utc.tm_hour * 60 + utc.tm_min, sOffsetMinutes);
}

uint32_t ntpSecondsSinceSync(void) {
  if (!sEverSynced) {
    return 0;
  }
  return (uint32_t)((millis() - sLastSyncMs) / 1000U);
}

bool ntpEverSynced(void) {
  return sEverSynced;
}

bool ntpLocalDate(char *out, size_t outLen) {
  uint32_t now;
  if (!ntpEpochUtc(&now)) {
    if (out != NULL && outLen > 0) {
      out[0] = '\0';
    }
    return false;
  }
  return clockFormatDate(now, sOffsetMinutes, out, outLen);
}

bool ntpEpochUtc(uint32_t *out) {
  if (out == NULL || !ntpSynchronised()) {
    return false;
  }
  *out = (uint32_t)time(NULL);
  return true;
}
