/* NVS backing for the channel list. */
#include "memory_nvs.h"

#include <Preferences.h>

/* The same namespace the settings use. NVS allows fifteen characters. */
static const char *kNamespace = "tef668x";

/* The one key the whole list lives under. */
static const char *kKey = "memory";

bool memoryNvsLoad(MemoryStore *out) {
  memoryInit(out);

  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {
    return false;
  }
  /* Only a blob of exactly the right length is read. A different length is a
   * list written by a firmware whose channel struct was another shape, and
   * reading it would put the fields in the wrong places. */
  bool right = prefs.getBytesLength(kKey) == sizeof(MemoryStore);
  bool ok = right && prefs.getBytes(kKey, out, sizeof(MemoryStore)) ==
                         sizeof(MemoryStore);
  prefs.end();
  if (!ok) {
    memoryInit(out);
  }
  return ok;
}

bool memoryNvsSave(const MemoryStore *m) {
  if (m == NULL) {
    return false;
  }
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }
  size_t written = prefs.putBytes(kKey, m, sizeof(MemoryStore));
  prefs.end();
  return written == sizeof(MemoryStore);
}
