/* Implementation of the debug lines that can go quiet. */
#include "debug_log.h"

#include <Arduino.h>
#include <esp_log.h>
#include <atomic>

DebugLogPrint DebugLog;

/* Read by any task that logs, written by the loop. */
static std::atomic<bool> sQuiet{false};
/* The system log's own printer, put back after a quiet spell. Its levels are
 * left alone: setting them all at once also clears the ones set for single
 * parts, such as the I2C driver's. */
static vprintf_like_t sPrinter = NULL;

static int sayNothing(const char *, va_list) {
  return 0;
}

size_t DebugLogPrint::write(uint8_t c) {
  return sQuiet.load() ? 1 : Serial.write(c);
}

size_t DebugLogPrint::write(const uint8_t *buffer, size_t size) {
  return sQuiet.load() ? size : Serial.write(buffer, size);
}

void DebugLogPrint::quiet(bool on) {
  if (on == sQuiet.load()) {
    return;
  }
  if (on) {
    sQuiet.store(true);
    Serial.flush();
    sPrinter = esp_log_set_vprintf(sayNothing);
    Serial.setDebugOutput(false);
    return;
  }
  esp_log_set_vprintf(sPrinter != NULL ? sPrinter : ::vprintf);
  Serial.setDebugOutput(true);
  sQuiet.store(false);
}
