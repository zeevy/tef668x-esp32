/*
 * When to read the battery again with Wi-Fi paused.
 *
 * The battery sits on ADC2, which the Wi-Fi driver holds for as long as it
 * runs, so with Wi-Fi on the only reading is the one taken at start up. This
 * decides when Wi-Fi may stop for the moment a reading takes: every few
 * minutes, and only while nobody is using the radio or its network. The
 * doing, stopping Wi-Fi, reading and starting it again, is the caller's.
 *
 * Plain C, no hardware: it is given the time and what the radio is doing,
 * and answers yes or no.
 */
#ifndef CORE_BATTERY_CHECK_H
#define CORE_BATTERY_CHECK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Between one reading and the next. A cell runs down over hours, so a
 * reading every five minutes is fresh enough to show, and it keeps the
 * radio off the network for a moment twelve times an hour at most. */
#define BATTERY_CHECK_EVERY_MS (5UL * 60UL * 1000UL)

/* How long with no key, knob or touch before the radio counts as idle. The
 * same five minutes: a person who has not touched it for that long is not
 * reading its panel. */
#define BATTERY_CHECK_IDLE_MS (5UL * 60UL * 1000UL)

/* How long since the last web request before the network counts as quiet.
 * A page that cannot have the live socket asks for the state every 5 s, so
 * a minute with no request means no page is open. */
#define BATTERY_CHECK_QUIET_MS (60UL * 1000UL)

/* What the radio is doing now. */
typedef struct {
  uint32_t nowMs;
  uint32_t lastTryMs;     /* The last try, or start up if none since. */
  bool fitted;            /* The board has a battery sense pin. */
  bool wifiRunning;       /* Off, the battery is read every second anyway. */
  uint32_t lastInputMs;   /* The last key, knob or touch. */
  bool screenOff;         /* The panel is dimmed to nothing. */
  uint8_t livePages;      /* Web pages on the live socket. */
  bool anyRequest;        /* A web request has come since start up. */
  uint32_t lastRequestMs; /* When the last one came, if any has. */
  uint8_t clients;        /* PCs on the PC Link and phones on the hotspot. */
  bool busy;              /* Something a pause would spoil, see the task. */
  bool onTrial;           /* A new firmware still proving it can join. */
} BatteryCheckInput;

/* Whether to pause Wi-Fi and read the battery now. The caller asks only
 * with Battery Check on, and counts a try as its new `lastTryMs` whether the
 * reading arrived or not, so one that keeps failing does not keep the radio
 * off the network. */
bool batteryCheckDue(const BatteryCheckInput *in);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BATTERY_CHECK_H */
