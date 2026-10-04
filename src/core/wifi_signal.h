/*
 * How strong the Wi-Fi link is, as bars a person can read at a glance.
 *
 * RSSI on this chip is in dBm, always negative, closer to zero being
 * stronger. Nothing here reads the radio driver; core/ never does.
 */
#ifndef CORE_WIFI_SIGNAL_H
#define CORE_WIFI_SIGNAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * How many bars a reading of `rssiDbm` is worth, 0 to 3.
 *
 * The two breakpoints, -50 and -60 dBm, are two of the PE5PVB TEF6686_ESP32
 * firmware's own three tiers for this same ESP32 Wi-Fi radio, in
 * TEF6686_ESP32.ino's `ShowRSSI`, not picked here: that firmware draws four bar
 * states, one fewer breakpoint than tiers, and joins them to no bars below its
 * own floor, and this one draws four states too, 0 to 3 rather than 1 to 4, so
 * the same two breakpoints carry over.
 */
uint8_t wifiSignalBars(int8_t rssiDbm);

#ifdef __cplusplus
}
#endif

#endif /* CORE_WIFI_SIGNAL_H */
