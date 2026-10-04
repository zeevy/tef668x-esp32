/* Implementation of the littlefs-backed DX scan RDS times. */
#include "dx_timing_fs.h"

#include <LittleFS.h>
#include <string.h>

#include "logbook_fs.h"

bool dxTimingFsAppend(const char *line) {
  if (line == NULL || !logbookFsPresent()) {
    return false;
  }
  if (LittleFS.exists(DX_TIMING_PATH)) {
    File f = LittleFS.open(DX_TIMING_PATH, "r");
    const size_t size = f ? f.size() : 0;
    if (f) {
      f.close();
    }
    if (size >= DX_TIMING_FILE_MAX) {
      /* The older file goes, and this one takes its place. */
      LittleFS.remove(DX_TIMING_OLD_PATH);
      LittleFS.rename(DX_TIMING_PATH, DX_TIMING_OLD_PATH);
    }
  }
  File f = LittleFS.open(DX_TIMING_PATH, "a");
  if (!f) {
    return false;
  }
  const size_t len = strlen(line);
  const bool ok = f.write((const uint8_t *)line, len) == len;
  f.close();
  return ok;
}

static void sendFile(const char *path,
                     void (*send)(const char *text, size_t len, void *ctx),
                     void *ctx) {
  if (!LittleFS.exists(path)) {
    return;
  }
  File f = LittleFS.open(path, "r");
  if (!f) {
    return;
  }
  char buf[256];
  int n;
  while ((n = f.read((uint8_t *)buf, sizeof(buf))) > 0) {
    send(buf, (size_t)n, ctx);
  }
  f.close();
}

void dxTimingFsRead(void (*send)(const char *text, size_t len, void *ctx),
                    void *ctx) {
  if (send == NULL || !logbookFsPresent()) {
    return;
  }
  sendFile(DX_TIMING_OLD_PATH, send, ctx);
  sendFile(DX_TIMING_PATH, send, ctx);
}
