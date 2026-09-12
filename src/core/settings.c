/**
 * @file settings.c
 * @brief Implementation of the settings struct, its defaults and migration.
 */
#include "settings.h"

#include <string.h>

/**
 * How many bytes each shipped version of the struct took.
 *
 * When SETTINGS_VERSION goes to 2, the entry for 1 becomes the number this
 * sizeof produces today, written out by hand, and 2 gets the sizeof. That is
 * the whole migration mechanism: a version is a known length, so a blob that
 * is not that length is corrupt rather than old.
 */
static uint16_t settingsSizeOfVersion(uint16_t version) {
  switch (version) {
    case 1:
      return (uint16_t)sizeof(Settings);
    default:
      return 0;
  }
}

/** True when a char array holds a terminated string inside its own bounds. */
static bool terminated(const char *s, size_t cap) {
  for (size_t i = 0; i < cap; i++) {
    if (s[i] == '\0') {
      return true;
    }
  }
  return false;
}

void settingsDefaults(Settings *s) {
  memset(s, 0, sizeof(*s));
  s->version = SETTINGS_VERSION;
  s->size = (uint16_t)sizeof(Settings);
  s->wifiSsid[0] = '\0';
  s->wifiPass[0] = '\0';
  s->accessPin = 0;
}

bool settingsValid(const Settings *s) {
  if (s->version == 0 || s->version > SETTINGS_VERSION) {
    return false;
  }
  if (!terminated(s->wifiSsid, SETTINGS_SSID_LEN)) {
    return false;
  }
  if (!terminated(s->wifiPass, SETTINGS_PASS_LEN)) {
    return false;
  }
  if (s->accessPin >= 1000000UL) {
    return false;
  }
  return true;
}

bool settingsFromBlob(const void *blob, size_t len, Settings *out) {
  settingsDefaults(out);
  if (blob == NULL || len < sizeof(uint16_t) * 2) {
    return false;
  }

  uint16_t version;
  uint16_t storedSize;
  memcpy(&version, blob, sizeof(version));
  memcpy(&storedSize, (const uint8_t *)blob + sizeof(version),
         sizeof(storedSize));

  if (version == 0 || version > SETTINGS_VERSION) {
    /* Written by a newer firmware, or plain corrupt. Defaults are safer than
     * reading fields that may have moved. */
    return false;
  }

  /* A blob has to be exactly the size the firmware that wrote it used. The
   * size field alone is not enough, because a corrupt blob can declare a
   * small size that matches its own truncated length and then read back as a
   * valid one character SSID. */
  if (storedSize != len || storedSize != settingsSizeOfVersion(version)) {
    return false;
  }

  /* Copy in only as much as both sides agree exists. Anything this firmware
   * added since keeps the default that settingsDefaults just wrote. */
  size_t copy = len < sizeof(Settings) ? len : sizeof(Settings);
  memcpy(out, blob, copy);
  out->version = SETTINGS_VERSION;
  out->size = (uint16_t)sizeof(Settings);

  if (!settingsValid(out)) {
    settingsDefaults(out);
    return false;
  }
  return true;
}

bool settingsHasWifi(const Settings *s) {
  return s->wifiSsid[0] != '\0';
}

bool settingsSetWifi(Settings *s, const char *ssid, const char *pass) {
  if (ssid == NULL) {
    return false;
  }
  if (pass == NULL) {
    pass = "";
  }
  if (strlen(ssid) >= SETTINGS_SSID_LEN || strlen(pass) >= SETTINGS_PASS_LEN) {
    return false;
  }
  memset(s->wifiSsid, 0, SETTINGS_SSID_LEN);
  memset(s->wifiPass, 0, SETTINGS_PASS_LEN);
  memcpy(s->wifiSsid, ssid, strlen(ssid));
  memcpy(s->wifiPass, pass, strlen(pass));
  return true;
}
