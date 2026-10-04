/* Implementation of the littlefs-backed DX level sweeps. */
#include "dx_sweep_fs.h"

#include <LittleFS.h>
#include <stdlib.h>

#include "logbook_fs.h"

/* The file is up to 7 KB, too big for the loop task's stack, so each read
 * and write takes its bytes from the heap and gives them back. */

void dxSweepFsLoad(const char *path, DxSweepHistory *out) {
  out->count = 0;
  if (!logbookFsPresent() || !LittleFS.exists(path)) {
    return;
  }
  File f = LittleFS.open(path, "r");
  if (!f) {
    return;
  }
  const size_t size = f.size();
  uint8_t *bytes =
      size <= DX_SWEEP_FILE_MAX ? (uint8_t *)malloc(size > 0 ? size : 1) : NULL;
  if (bytes != NULL && f.read(bytes, size) == (int)size) {
    (void)dxSweepDecode(bytes, size, out);
  }
  f.close();
  free(bytes);
}

bool dxSweepFsSave(const char *path, const DxSweepHistory *h) {
  if (!logbookFsPresent()) {
    return false;
  }
  const size_t size = dxSweepEncodedSize(h);
  uint8_t *bytes = (uint8_t *)malloc(size);
  if (bytes == NULL) {
    return false;
  }
  bool ok = dxSweepEncode(h, bytes, size) == size;
  if (ok) {
    File f = LittleFS.open(path, "w");
    ok = f && f.write(bytes, size) == size;
    if (f) {
      f.close();
    }
  }
  free(bytes);
  return ok;
}

bool dxSweepFsRemove(const char *path) {
  if (!logbookFsPresent() || !LittleFS.exists(path)) {
    return true;
  }
  return LittleFS.remove(path);
}
