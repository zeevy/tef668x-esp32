/**
 * @file settings.h
 * @brief Every setting in one versioned struct, never an address map.
 *
 * Adding a setting is adding a field and bumping SETTINGS_VERSION. Reading a
 * struct written by an older firmware goes through settingsFromBlob, which
 * fills anything the old struct did not have with the default.
 *
 * Nothing in here touches hardware or NVS, so it builds and is tested on a PC.
 * The NVS side lives in settings_store.h.
 */
#ifndef CORE_SETTINGS_H
#define CORE_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Bump this whenever a field is added, removed or changes meaning. */
#define SETTINGS_VERSION 1

/** Room for a 32 character SSID and its terminator. */
#define SETTINGS_SSID_LEN 33

/** Room for a 64 character WPA2 passphrase and its terminator. */
#define SETTINGS_PASS_LEN 65

/**
 * The whole of the radio's saved state.
 *
 * @note Fields are only ever appended. Reordering or resizing an existing
 *       field breaks every radio already in the field.
 */
typedef struct {
  uint16_t version;              /**< SETTINGS_VERSION this was written by. */
  uint16_t size;                 /**< sizeof(Settings) when written. */
  char wifiSsid[SETTINGS_SSID_LEN]; /**< Empty means no credentials yet. */
  char wifiPass[SETTINGS_PASS_LEN]; /**< Empty is allowed, for an open network. */
  uint32_t accessPin;            /**< 0 means use the MAC derived default. */
} Settings;

/** Fill a struct with the values a radio leaves the factory with. */
void settingsDefaults(Settings *s);

/**
 * Check a struct is usable.
 *
 * Catches a version this firmware does not know and strings with no
 * terminator, which is what a truncated or corrupt NVS blob looks like.
 */
bool settingsValid(const Settings *s);

/**
 * Read a stored blob into a struct, upgrading it if an older firmware wrote it.
 *
 * A blob whose recorded size does not match the number of bytes given is
 * treated as truncated and rejected, rather than read as far as it goes.
 *
 * @param blob  The bytes read back from NVS.
 * @param len   How many bytes there are.
 * @param out   Receives the settings. Left at defaults when the blob is
 *              unusable.
 * @return true when the blob was read, false when defaults were used instead.
 */
bool settingsFromBlob(const void *blob, size_t len, Settings *out);

/** True when the radio has an SSID to try. */
bool settingsHasWifi(const Settings *s);

/**
 * Copy an SSID and passphrase in, with bounds checking.
 *
 * @return false when either string is too long to store, in which case
 *         nothing is changed.
 */
bool settingsSetWifi(Settings *s, const char *ssid, const char *pass);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SETTINGS_H */
