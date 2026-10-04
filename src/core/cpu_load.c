/* Implementation of the CPU busy share. */
#include "cpu_load.h"

#include <stddef.h>

bool cpuBusyPercent(const CpuSample *before, const CpuSample *now,
                    uint8_t *out) {
  if (before == NULL || now == NULL || out == NULL) {
    return false;
  }
  const uint32_t elapsed = now->clockUs - before->clockUs;
  if (elapsed == 0) {
    return false;
  }
  uint32_t idle = now->idleUs - before->idleUs;
  if (idle > elapsed) {
    idle = elapsed;
  }
  /* 64 bit, since a busy time of more than 43 seconds times 100 would not
   * fit in 32. */
  const uint64_t busy = (uint64_t)(elapsed - idle);
  *out = (uint8_t)((busy * 100u + elapsed / 2u) / elapsed);
  return true;
}
