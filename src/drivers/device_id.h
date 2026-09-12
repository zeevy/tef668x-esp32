/**
 * @file device_id.h
 * @brief Reads the radio's own MAC address.
 *
 * `WiFi.macAddress(mac)` cannot be used for this. In Arduino core 3.x it goes
 * to `NetworkInterface::macAddress()`, which returns NULL and writes nothing
 * at all when the interface has not been brought up yet. Calling it before
 * Wi-Fi starts leaves the caller's buffer holding whatever was on the stack,
 * and the compiler cannot warn about it because the buffer is passed by
 * pointer.
 *
 * That matters here because the access PIN is derived from the MAC, and it is
 * worked out before Wi-Fi starts. `esp_read_mac` reads the eFuse directly and
 * works at any point, so it is what this uses.
 */
#ifndef DRIVERS_DEVICE_ID_H
#define DRIVERS_DEVICE_ID_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Read the station MAC address.
 *
 * @param mac  Receives six bytes. Zeroed first, so a failed read leaves a
 *             known value rather than stack rubbish.
 * @return true when the read worked.
 */
bool deviceMacRead(uint8_t mac[6]);

#endif /* DRIVERS_DEVICE_ID_H */
