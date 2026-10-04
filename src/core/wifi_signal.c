/* Implementation of the RSSI to bars mapping. */
#include "wifi_signal.h"

uint8_t wifiSignalBars(int8_t rssiDbm) {
  if (rssiDbm > -50) {
    return 3;
  }
  if (rssiDbm > -60) {
    return 2;
  }
  if (rssiDbm > -70) {
    return 1;
  }
  return 0;
}
