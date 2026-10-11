#include "battery_check.h"

#include <stddef.h>

bool batteryCheckDue(const BatteryCheckInput *in) {
  if (in == NULL || !in->fitted) {
    return false;
  }
  /* With Wi-Fi off the converter is free and is read every second; with a
   * new firmware on trial, stopping Wi-Fi would undo the join it has to
   * prove. */
  if (!in->wifiRunning || in->onTrial || in->busy) {
    return false;
  }
  /* Differences, not comparisons, so the 49 day wrap of the clock does not
   * matter. */
  if ((uint32_t)(in->nowMs - in->lastTryMs) < BATTERY_CHECK_EVERY_MS) {
    return false;
  }
  const bool idle = in->screenOff || (uint32_t)(in->nowMs - in->lastInputMs) >=
                                         BATTERY_CHECK_IDLE_MS;
  if (!idle) {
    return false;
  }
  /* Nobody on the network: no page open, no PC on the PC Link, no phone on
   * the hotspot. */
  if (in->livePages > 0 || in->clients > 0) {
    return false;
  }
  return !in->anyRequest ||
         (uint32_t)(in->nowMs - in->lastRequestMs) >= BATTERY_CHECK_QUIET_MS;
}
