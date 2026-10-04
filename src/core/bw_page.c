/* Implementation of the bandwidth page. */
#include "bw_page.h"

#include <stddef.h>

uint8_t bwPageTiles(BandId band, bool dxMode, BwTile *out, uint8_t cap) {
  if (out == NULL) {
    return 0;
  }
  const size_t widths = bandBandwidthCount(band);
  if (widths == 0) {
    return 0;
  }
  const bool fm = bandModulation(band) == MODULATION_FM;
  uint8_t n = 0;
  for (size_t i = 0; i < widths; i++) {
    const uint16_t khz = bandBandwidthAt(band, i);
    if (dxMode && khz == 0) {
      continue;
    }
    if (n >= cap) {
      return 0;
    }
    out[n].kind = BW_TILE_WIDTH;
    out[n].khz = khz;
    n++;
  }
  if (fm) {
    if (n + 2 > cap) {
      return 0;
    }
    out[n].kind = BW_TILE_IMS;
    out[n].khz = 0;
    out[n + 1].kind = BW_TILE_EQ;
    out[n + 1].khz = 0;
    n = (uint8_t)(n + 2);
  }
  return n;
}

uint8_t bwPageStart(const BwTile *tiles, uint8_t count, uint16_t khz) {
  for (uint8_t i = 0; tiles != NULL && i < count; i++) {
    if (tiles[i].kind == BW_TILE_WIDTH && tiles[i].khz == khz) {
      return i;
    }
  }
  return 0;
}

uint8_t bwPageMove(uint8_t cursor, int32_t clicks, uint8_t count) {
  if (count == 0) {
    return 0;
  }
  int32_t at = (int32_t)cursor + clicks;
  if (at < 0) {
    at = 0;
  }
  if (at > count - 1) {
    at = count - 1;
  }
  return (uint8_t)at;
}
