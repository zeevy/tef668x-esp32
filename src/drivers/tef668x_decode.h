/*
 * The tuner's replies turned from bytes into readings, with no bus and no
 * hardware, so the decode can be tested on a PC with byte buffers.
 *
 * Every word the chip sends is two bytes, high byte first. Level, offset and
 * modulation are signed quantities sent in those unsigned words, and reading
 * one as unsigned turns a small negative into a number in the thousands, so
 * the decode is written once, here, and both quality readers use it.
 */
#ifndef DRIVERS_TEF668X_DECODE_H
#define DRIVERS_TEF668X_DECODE_H

#include <stdbool.h>
#include <stdint.h>

#include "drivers/tef668x.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One word of a tuner reply, which sends the high byte first. */
static inline uint16_t word16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] << 8 | (uint16_t)p[1]);
}

/* The size of a quality reply: seven words. */
#define TEF668X_QUALITY_BYTES 14
/* The size of an RDS reply: the status, four blocks and the error word. */
#define TEF668X_RDS_BYTES 12

/*
 * Turn a quality reply into `q`.
 *
 * The words are the status, the level in tenths of a dBuV, the noise, then
 * multipath on FM or co-channel on AM, the offset in tenths of a kHz, the
 * bandwidth in tenths of a kHz and the modulation in tenths of a percent. The
 * SNR is worked out from the level and the noise. `stereo` is left as it
 * was, since the pilot comes from a different command.
 */
void tef668xDecodeQuality(const uint8_t *buf, bool fm, Tef668xQuality *q);

/*
 * Turn an RDS reply into `out`, all of it set.
 *
 * A reply with no group waiting, or one that carries block A alone, is not
 * a group: `haveGroup` stays false and the blocks stay zero, so nothing
 * decodes three blocks of whatever the registers held.
 */
void tef668xDecodeRds(const uint8_t *buf, Tef668xRdsRead *out);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_TEF668X_DECODE_H */
