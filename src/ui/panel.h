/*
 * A panel: one piece of the screen that knows where it is and nothing else.
 *
 * A panel is given a rectangle and draws inside it. **No panel contains the
 * width or the height of a display.** Its right margin is `at->w - UI_MARGIN`
 * and never 310, so the same panel works on a screen of another size or in
 * another orientation without being touched.
 *
 * A layout is then a table of which panel goes where. Layout tables live one
 * file per panel size, starting with `layout_320x240.cpp`.
 */
#ifndef UI_PANEL_H
#define UI_PANEL_H

#include <lvgl.h>

#include "screen.h"

typedef struct {
  int16_t x, y, w, h;
} PanelRect;

/*
 * Build the panel's objects under `parent`, positioned for `at`.
 *
 * Called once when a layout is built. The panel keeps its own objects and its
 * own copy of the rectangle, because there is one of each panel on screen at a
 * time and giving every panel an owning container would cost five more LVGL
 * objects for nothing.
 */
typedef void (*PanelBeginFn)(lv_obj_t *parent, const PanelRect *at);

/* Write the state into the objects. Called on every update. */
typedef void (*PanelShowFn)(const ScreenState *state);

/* The parts of the panel at `at` a touch can act on, as RadioZone ids:
 * writes no more than `max` and returns how many. Kept beside the drawing,
 * so a touch lands where the thing it acts on is drawn. */
typedef int (*PanelZonesFn)(const PanelRect *at, TouchZone *out, int max);

typedef struct {
  PanelBeginFn begin;
  PanelShowFn show;
  PanelZonesFn zones; /* NULL for a panel a touch does nothing on. */
} Panel;

/* One row of a layout table. */
typedef struct {
  const Panel *panel;
  PanelRect at;
} PanelPlacement;

typedef struct {
  const PanelPlacement *placements;
  uint8_t count;
} Layout;

/* The layout table. Defined per panel size; every band uses the same one. */
const Layout *layoutFor(void);

/* The panels of the radio screen. */
extern const Panel panelHeader; /* Band name and the status run. */
extern const Panel panelLcd; /* The frequency panel: name, frequency, level. */
extern const Panel panelTextLine; /* Radio text, or the date. */
extern const Panel panelScale;    /* The sliding tuning scale. */
extern const Panel panelTiles;    /* Mode, SQL, BW and VOL. */

#endif /* UI_PANEL_H */
