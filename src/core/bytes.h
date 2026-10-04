/*
 * Little endian numbers in a byte buffer, for the formats this radio keeps
 * on flash: the logbook, the DX seen PIs and the DX level sweep.
 *
 * A byte at a time, so a format does not depend on the byte order of
 * whatever compiles it.
 */
#ifndef CORE_BYTES_H
#define CORE_BYTES_H

#include <stdint.h>

static inline void putU16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFF);
  p[1] = (uint8_t)(v >> 8);
}

static inline void putU32(uint8_t *p, uint32_t v) {
  putU16(p, (uint16_t)(v & 0xFFFF));
  putU16(p + 2, (uint16_t)(v >> 16));
}

static inline uint16_t getU16(const uint8_t *p) {
  return (uint16_t)(p[0] | (p[1] << 8));
}

/* Each byte widened before the shift, since a top byte of 0x80 or more
 * shifted by 24 as an int does not fit one. */
static inline uint32_t getU32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

#endif /* CORE_BYTES_H */
