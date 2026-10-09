/*
 * The radio's own debug lines, on the USB serial port. They go quiet while a
 * PC has an XDR session on that port, where any other line would read as a
 * reply to it. The same calls as Serial: DebugLog.printf, DebugLog.println.
 */
#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <Print.h>

class DebugLogPrint : public Print {
 public:
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  /* Quiet: this, the framework's own lines and the system log say nothing
   * on the port until it is called again with false. */
  void quiet(bool on);
};

extern DebugLogPrint DebugLog;

#endif /* DEBUG_LOG_H */
