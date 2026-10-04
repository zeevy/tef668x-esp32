/* NVS backing for the channel list. */
#include "memory_nvs.h"

#include <Arduino.h>
#include <Preferences.h>
#include <stdlib.h>

/* The same namespace the settings use. NVS allows fifteen characters. */
static const char *kNamespace = "tef668x";

/* The one key the whole list lives under. */
static const char *kKey = "memory";

bool memoryNvsLoad(MemoryStore *out, int *clearedOut) {
  memoryInit(out);
  if (clearedOut != NULL) {
    *clearedOut = 0;
  }

  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {
    return false;
  }
  /* Read whole, whatever length it is, and let core/memory.c decide which
   * form it is in. Nothing longer than this firmware's own form is taken:
   * that is a later version, which memoryFromBlob would refuse anyway. On
   * the heap for the moment it takes, rather than about 2.8 KB of the loop
   * task's stack or a buffer kept for the one read at start up. */
  size_t len = prefs.getBytesLength(kKey);
  bool ok = false;
  if (len > 0 && len <= memoryBlobSize()) {
    uint8_t *blob = (uint8_t *)malloc(len);
    if (blob != NULL) {
      ok = prefs.getBytes(kKey, blob, len) == len &&
           memoryFromBlob(blob, len, out);
      free(blob);
    }
  }
  prefs.end();
  if (!ok) {
    memoryInit(out);
    return false;
  }
  /* The length is all the read above checks, so the contents are untrusted
   * until here. Only memorySet looks at what it is given, and a list that
   * reached flash by any other route has never been looked at. */
  int lost = memorySanitize(out);
  if (clearedOut != NULL) {
    *clearedOut = lost;
  }
  if (lost != 0) {
    Serial.printf("[memory] %d stored channels were not valid, cleared\n",
                  lost);
  }
  return true;
}

bool memoryNvsSaveBlob(const uint8_t *blob, size_t size) {
  if (blob == NULL || size != memoryBlobSize()) {
    return false;
  }
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  const bool ok = prefs.putBytes(kKey, blob, size) == size;
  prefs.end();
  return ok;
}
