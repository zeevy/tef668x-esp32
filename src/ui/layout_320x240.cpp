/*
 * Where every panel goes on a 320 by 240 panel in landscape.
 *
 * This is the only file that knows the size of the radio screen. A panel is
 * given a rectangle and draws inside it, so another panel size or another
 * orientation is another table in another file and no change to any panel.
 *
 * The layout is the frequency panel with a row of tiles under it. Each panel is
 * given the full width and keeps UI_MARGIN from both sides itself. Top to
 * bottom: the header 0 to 27, 4 rows of air, the frequency panel 32 to 119, the
 * line of radio text or date 120 to 143, the scale 144 to 199, 4 rows of air,
 * the tiles 204 to 231, and 8 rows of air under them to the edge.
 */
#include "panel.h"

#define W 320
#define H 240

#define HEADER_Y 0
#define HEADER_H 28
#define LCD_Y 32
#define LCD_H 88
#define TEXT_Y 120
#define TEXT_H 24
#define SCALE_Y 144
#define SCALE_H 56
#define TILES_Y 204
#define TILES_H 28

/* A 28 px row is about 5 mm on this panel, less than a finger tip. The
 * header's touch area takes the 4 rows of air under it, 32 px, and the
 * tiles' takes the air on both sides, from the scale's last row to the
 * bottom edge, 40 px. Nothing drawn moves. */
#define HEADER_TOUCH_H (LCD_Y - HEADER_Y)
#define TILES_TOUCH_Y (SCALE_Y + SCALE_H)
#define TILES_TOUCH_H (H - TILES_TOUCH_Y)

/*
 * One table for every band. FM, OIRT, LW, MW and SW differ in what the
 * station sends, not in where it goes: on AM there is no name and no radio
 * text, so the name line reads "---" and the line under the panel carries
 * the date.
 */
static const PanelPlacement kAll[] = {
    {&panelHeader,
     {0, HEADER_Y, W, HEADER_H},
     {0, HEADER_Y, W, HEADER_TOUCH_H}},
    {&panelLcd, {0, LCD_Y, W, LCD_H}},
    {&panelTextLine, {0, TEXT_Y, W, TEXT_H}},
    {&panelScale, {0, SCALE_Y, W, SCALE_H}},
    {&panelTiles,
     {0, TILES_Y, W, TILES_H},
     {0, TILES_TOUCH_Y, W, TILES_TOUCH_H}},
};

static const Layout kLayout = {kAll, (uint8_t)(sizeof(kAll) / sizeof(kAll[0]))};

const Layout *layoutFor(void) {
  return &kLayout;
}
