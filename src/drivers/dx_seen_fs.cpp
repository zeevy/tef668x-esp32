/* Implementation of the littlefs-backed set of PIs ever caught. */
#include "dx_seen_fs.h"

#include <LittleFS.h>
#include <stdlib.h>

#include "logbook_fs.h"

#define DX_SEEN_PATH "/dxseen.bin"

/* The file's bytes are a kilobyte, too big for the loop task's stack, which
 * the web server shares, and the static segment has no room left. So each
 * read and write takes them from the heap and gives them back. */

bool dxSeenFsLoad(DxSeen *out) {
  dxSeenReset(out);
  /* logbookFsBegin mounts littlefs, and says whether it did. */
  if (!logbookFsPresent()) {
    return false;
  }
  if (!LittleFS.exists(DX_SEEN_PATH)) {
    return true;
  }
  uint8_t *bytes = (uint8_t *)malloc(DX_SEEN_BYTES);
  if (bytes == NULL) {
    return false;
  }
  File f = LittleFS.open(DX_SEEN_PATH, "r");
  if (!f) {
    free(bytes);
    return false;
  }
  const size_t size = f.size();
  const int got = f.read(bytes, DX_SEEN_BYTES);
  f.close();
  /* A file of the right length that reads short is a read that failed,
   * not a damaged file, so the set is not known. A short file or one that
   * is not ours stays empty: decode leaves it so. */
  const bool readFailed = size == DX_SEEN_BYTES && got != DX_SEEN_BYTES;
  if (got == DX_SEEN_BYTES) {
    (void)dxSeenDecode(bytes, DX_SEEN_BYTES, out);
  }
  free(bytes);
  return !readFailed;
}

bool dxSeenFsSave(const DxSeen *s) {
  uint8_t *bytes = (uint8_t *)malloc(DX_SEEN_BYTES);
  if (bytes == NULL) {
    return false;
  }
  bool ok = dxSeenEncode(s, bytes, DX_SEEN_BYTES) == DX_SEEN_BYTES;
  if (ok) {
    File f = LittleFS.open(DX_SEEN_PATH, "w");
    ok = f && f.write(bytes, DX_SEEN_BYTES) == DX_SEEN_BYTES;
    if (f) {
      f.close();
    }
  }
  free(bytes);
  return ok;
}
