/**
 * @file settings_nvs.cpp
 * @brief NVS backing for the settings struct.
 */
#include "settings_nvs.h"

#include <Preferences.h>

/** NVS namespace. Kept short, NVS allows fifteen characters. */
static const char *kNamespace = "tef668x";

/** The one key the whole struct lives under. */
static const char *kKey = "settings";

bool settingsNvsLoad(Settings *out) {
  settingsDefaults(out);

  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {
    /* No namespace yet, which is what a radio out of the factory looks like. */
    return false;
  }

  size_t len = prefs.getBytesLength(kKey);
  if (len == 0) {
    prefs.end();
    return false;
  }

  /* Read into a buffer sized by what is stored, not by the current struct, so
   * a blob written by an older and smaller struct still reads back. */
  uint8_t *blob = (uint8_t *)malloc(len);
  if (blob == NULL) {
    prefs.end();
    return false;
  }

  size_t got = prefs.getBytes(kKey, blob, len);
  prefs.end();

  bool ok = (got == len) && settingsFromBlob(blob, got, out);
  free(blob);
  return ok;
}

bool settingsNvsSave(const Settings *s) {
  if (!settingsValid(s)) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  size_t written = prefs.putBytes(kKey, s, sizeof(Settings));
  prefs.end();
  return written == sizeof(Settings);
}

bool settingsNvsClear(void) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  bool ok = prefs.remove(kKey);
  prefs.end();
  return ok;
}
