/* Implementation of the heap readings. See system_info.h. */
#include "system_info.h"

#include <Arduino.h>

uint32_t systemHeapFree(void) {
  return ESP.getFreeHeap();
}

uint32_t systemHeapLowest(void) {
  return ESP.getMinFreeHeap();
}

uint32_t systemHeapLargest(void) {
  static uint32_t largest = 0;
  static uint32_t atMs = 0;
  static bool read = false;
  if (!read || (uint32_t)(millis() - atMs) >= 1000UL) {
    largest = ESP.getMaxAllocHeap();
    atMs = millis();
    read = true;
  }
  return largest;
}
