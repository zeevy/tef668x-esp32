/* NVS backing for the settings struct. */
#include "settings_nvs.h"

#include "board/board.h"

#include <Preferences.h>
#include <string.h>

/* NVS namespace. Kept short, NVS allows fifteen characters. */
static const char *kNamespace = "tef668x";

/* The one key the whole struct lives under. */
static const char *kKey = "settings";

bool settingsNvsStored(void) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {
    return false;
  }
  const size_t len = prefs.getBytesLength(kKey);
  prefs.end();
  return len != 0;
}

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

/* The touch calibration, under a key of its own beside the settings: a
 * calibration is made by hand on one radio, and keeping it out of the
 * settings struct means the struct, and its version, never change for it. */
static const char *kTouchCalKey = "touchcal";

/* The kept calibration, if there is one this screen can use. */
static bool touchCalNvsLoad(int16_t width, int16_t height, TouchCal *out) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {
    return false;
  }
  TouchCalBlob blob;
  const bool ok =
      prefs.getBytes(kTouchCalKey, &blob, sizeof(blob)) == sizeof(blob) &&
      touchCalBlobRead(&blob, width, height, out);
  prefs.end();
  return ok;
}

bool touchCalNvsSave(const TouchCal *cal) {
  TouchCalBlob blob;
  memset(&blob, 0, sizeof(blob));
  blob.version = TOUCH_CAL_BLOB_VERSION;
  blob.cal = *cal;
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  const size_t written = prefs.putBytes(kTouchCalKey, &blob, sizeof(blob));
  prefs.end();
  return written == sizeof(blob);
}

bool touchCalNvsErase(void) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  /* Nothing stored is already erased. */
  const bool ok = !prefs.isKey(kTouchCalKey) || prefs.remove(kTouchCalKey);
  prefs.end();
  return ok;
}

bool touchCalNvsLoadOrBoard(int16_t width, int16_t height, TouchCal *out,
                            bool *stored) {
  *stored = touchCalNvsLoad(width, height, out);
  if (*stored) {
    return true;
  }
#if FEATURE_TOUCH
  static const TouchPoint kCorners[4] = TOUCH_CAL_CORNERS;
  return touchCalFromCorners(kCorners, width, height, out);
#else
  return false;
#endif
}
