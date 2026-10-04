/*
 * The bandwidth page's view, built from the page's tiles and the radio's
 * snapshot. The same split as screen_dx_state.cpp: the screen task reads the
 * radio into `ScreenBwInputs` and this turns them into a `ScreenBw`, touching
 * no driver, so tools/screenshot.cpp runs it on a PC.
 */
#ifndef SCREEN_BW_STATE_H
#define SCREEN_BW_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "core/band_plan.h"
#include "core/bw_page.h"
#include "ui/screen.h"

/* What the page's text points at. */
typedef struct {
  char context[16];
  char position[8];
  char note[32];
  char text[BW_PAGE_MAX][12];
} ScreenBwKeep;

typedef struct {
  const BwTile *tiles; /* bwPageTiles's, `count` of them. */
  uint8_t count;
  uint8_t cursor;
  BandId band;
  bool dxMode;       /* The width is DX mode's. */
  uint16_t widthKHz; /* The width in use, 0 for automatic. */
  bool chipKnown;    /* The tuner's own reading of its width is known. */
  uint16_t chipKHz;  /* What it read. */
  bool ims;          /* The two switches, as the radio has them. */
  bool eq;
  const char *clock; /* Local, or NULL. */
} ScreenBwInputs;

void screenBwStateBuild(const ScreenBwInputs *in, ScreenBwKeep *keep,
                        ScreenBw *out);

#endif /* SCREEN_BW_STATE_H */
