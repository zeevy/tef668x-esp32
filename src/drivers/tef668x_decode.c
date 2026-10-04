#include "drivers/tef668x_decode.h"

#include <string.h>

#include "core/signal.h"

/*
 * Status bits in the RDS read. The rest of the word is not documented
 * anywhere public and is left alone.
 */
#define RDS_STATUS_DATA_AVAILABLE 15 /* A group is waiting in the registers. */
#define RDS_STATUS_PI_ONLY 13        /* Only block A is real. Not a group. */
#define RDS_STATUS_SYNCHRONISED 9    /* Locked to an RDS bit stream. */

static uint16_t word16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] << 8 | (uint16_t)p[1]);
}

void tef668xDecodeQuality(const uint8_t *buf, bool fm, Tef668xQuality *q) {
  q->status = word16(buf);
  q->levelDbuVTenths = (int16_t)word16(buf + 2);
  q->usnTenths = word16(buf + 4);
  if (fm) {
    q->multipathTenths = word16(buf + 6);
    q->coChannelTenths = 0;
  } else {
    q->multipathTenths = 0;
    q->coChannelTenths = word16(buf + 6);
  }
  q->offsetKHzTenths = (int16_t)word16(buf + 8);
  q->bandwidthKHz = (uint16_t)(word16(buf + 10) / 10);
  q->modulationPercent = (int16_t)((int16_t)word16(buf + 12) / 10);
  /* Worked out, not read. The chip does not report it, and the two sides put
   * their noise on different scales, so which one this came from matters. */
  q->snrDb = signalSnrDb(q->levelDbuVTenths, q->usnTenths, fm);
}

void tef668xDecodeRds(const uint8_t *buf, Tef668xRdsRead *out) {
  memset(out, 0, sizeof(*out));
  const uint16_t status = word16(buf);
  out->status = status;
  out->read = true;
  out->synchronised = (status & (1u << RDS_STATUS_SYNCHRONISED)) != 0;
  if ((status & (1u << RDS_STATUS_DATA_AVAILABLE)) == 0 ||
      (status & (1u << RDS_STATUS_PI_ONLY)) != 0) {
    return;
  }
  /* The error word holds two bits per block, block A in the top pair. */
  const uint16_t errors = word16(buf + 10);
  for (int i = 0; i < 4; i++) {
    out->block[i] = word16(buf + 2 + i * 2);
    out->error[i] = (uint8_t)((errors >> (14 - i * 2)) & 0x03);
  }
  out->haveGroup = true;
}
