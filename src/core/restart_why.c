/* Which restart it was, packed for RTC memory. No hardware. */
#include "restart_why.h"

#include <stddef.h>

/*
 * The marker, in the top 24 bits, with the reason in the bottom 8.
 *
 * Not a checksum. It only has to be a value that uninitialised memory is
 * unlikely to hold, and any of the five reasons is a small number that
 * uninitialised memory holds rather often.
 */
#define RESTART_WHY_MARK 0x52573100UL
#define RESTART_WHY_MASK 0xFFFFFF00UL

uint32_t restartWhyPack(RestartWhy why) {
  if (why <= RESTART_WHY_NONE || why > RESTART_WHY_UPDATE) {
    return 0;
  }
  return RESTART_WHY_MARK | (uint32_t)why;
}

RestartWhy restartWhyUnpack(uint32_t word) {
  if ((word & RESTART_WHY_MASK) != RESTART_WHY_MARK) {
    return RESTART_WHY_NONE;
  }
  RestartWhy why = (RestartWhy)(word & 0xFFUL);
  if (why <= RESTART_WHY_NONE || why > RESTART_WHY_UPDATE) {
    return RESTART_WHY_NONE;
  }
  return why;
}

const char *restartWhyText(RestartWhy why) {
  switch (why) {
    case RESTART_WHY_ASKED:
      return "asked for";
    case RESTART_WHY_BOOT_WATCHDOG:
      return "boot watchdog";
    case RESTART_WHY_ROLLBACK:
      return "rollback";
    case RESTART_WHY_DISPLAY:
      return "display assertion";
    case RESTART_WHY_UPDATE:
      return "update";
    case RESTART_WHY_NONE:
    default:
      return NULL;
  }
}

/* XORed into the check word. Any value works that a word and its own
 * check are unlikely to match by chance. */
#define RESTART_HEAP_MARK 0x48504D4EUL

void restartHeapPack(uint32_t lowestBytes, uint32_t out[2]) {
  if (out == NULL) {
    return;
  }
  out[0] = lowestBytes;
  out[1] = lowestBytes ^ RESTART_HEAP_MARK;
}

bool restartHeapUnpack(const uint32_t in[2], uint32_t *lowestBytes) {
  if (in == NULL || lowestBytes == NULL) {
    return false;
  }
  if ((in[0] ^ RESTART_HEAP_MARK) != in[1]) {
    return false;
  }
  *lowestBytes = in[0];
  return true;
}
