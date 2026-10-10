/*
 * The bandwidth page: every width the band takes as a tile, then a Close
 * tile, and on FM the iMS and equaliser switches after them. A hold of BW opens it and the knob
 * picks.
 *
 * Pure logic: which tiles, in what order, where the cursor starts and how
 * it moves. The widths are the band plan's own list, the one the BW key,
 * the menu and the API already use, so the page cannot offer a width the
 * radio would refuse.
 */
#ifndef CORE_BW_PAGE_H
#define CORE_BW_PAGE_H

#include <stdbool.h>
#include <stdint.h>

#include "band_plan.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The most tiles: automatic and sixteen FM widths, Close, iMS and the
 * equaliser. */
#define BW_PAGE_MAX 20

typedef enum {
  BW_TILE_WIDTH = 0, /* A width; `khz` 0 is automatic. */
  BW_TILE_IMS,       /* The iMS switch. */
  BW_TILE_EQ,        /* The equaliser switch. */
  BW_TILE_CLOSE,     /* Closes the page, for a finger or the knob alone. */
} BwTileKind;

typedef struct {
  uint8_t kind; /* A BwTileKind. */
  uint16_t khz;
} BwTile;

/*
 * The tiles for `band`, widths first in the band plan's order, then on an
 * FM band the two switches. In DX mode automatic is left out, since DX mode
 * is one fixed width. Returns how many, 0 for a band that is not one or no
 * room.
 */
uint8_t bwPageTiles(BandId band, bool dxMode, BwTile *out, uint8_t cap);

/* Where the cursor starts: on the width in use, or the first tile when it
 * is not one of them. */
uint8_t bwPageStart(const BwTile *tiles, uint8_t count, uint16_t khz);

/* The cursor moved by `clicks`, a tile a click, stopping at both ends. */
uint8_t bwPageMove(uint8_t cursor, int32_t clicks, uint8_t count);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BW_PAGE_H */
