/*
 * The time, asked for over the network.
 *
 * This board has no battery backed clock, so every restart begins knowing
 * nothing about the time. SNTP is what sets the clock. The RDS clock group
 * is decoded and reported, but not every station sends an RDS clock, so it
 * does not set this one.
 *
 * Everything about what the time means, including the local offset and how it
 * is written down, is in core/clock.h. This file only fetches it.
 */
#ifndef NET_NTP_H
#define NET_NTP_H

#include <stdbool.h>
#include <stdint.h>

#include "core/clock.h"
#include "core/settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Start asking, if the setting says to, at start up and again after the
 * settings were edited.
 *
 * Safe to call before Wi-Fi has joined. The SNTP client retries on its own
 * and the first answer arrives whenever the network is ready, which is why
 * nothing here blocks waiting for one.
 *
 * Turning the setting off makes the time unknown again rather than leaving
 * the last answer on screen, because a clock that stopped being updated
 * still looks like a clock. The SNTP client keeps running in the
 * background, because it cannot be stopped cleanly once started.
 */
void ntpApply(const Settings *settings);

/*
 * Whether a server has answered since the last restart.
 *
 * False means the radio does not know the time. It never means midnight.
 */
bool ntpSynchronised(void);

/*
 * The local time, or a ClockTime with `known` false.
 *
 * The offset comes from the settings passed to ntpApply, so a
 * caller does not have to hold them.
 */
ClockTime ntpLocalTime(void);

/* The local offset from UTC in minutes, the same one ntpLocalTime uses, so
 * a time kept in UTC can be shown the way the clock shows now. */
int16_t ntpOffsetMinutes(void);

/*
 * Seconds since the last answer from a server, or 0 when there has been none.
 *
 * For the state document, so a person can tell a clock that is being kept up
 * to date from one that stopped being updated an hour ago.
 */
uint32_t ntpSecondsSinceSync(void);

/*
 * Whether a server has answered at all during this run.
 *
 * Different from ntpSynchronised, which also goes false when the setting is
 * turned off. This one says whether ntpSecondsSinceSync means anything.
 */
bool ntpEverSynced(void);

/*
 * The UTC time, as seconds since the epoch, for anything that needs a real
 * date rather than the hour and minute ntpLocalTime prints.
 *
 * False and `*out` left alone when ntpSynchronised is false, the same
 * guard ntpLocalTime uses, so a caller cannot be handed a plausible looking
 * number that is actually still 1 January 1970.
 */
bool ntpEpochUtc(uint32_t *out);

/*
 * The local date, "Saturday, 26th September 2026", into a buffer of
 * CLOCK_DATE_LEN. False and an empty string when the time is not known.
 */
bool ntpLocalDate(char *out, size_t outLen);

#ifdef __cplusplus
}
#endif

#endif /* NET_NTP_H */
