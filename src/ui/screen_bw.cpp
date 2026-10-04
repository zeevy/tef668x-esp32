/*
 * The bandwidth page.
 *
 * Every width the band takes as a tile in a grid of four, automatic first
 * on FM, and on FM the iMS and equaliser switches in the last row. The
 * width in use, and a switch that is on, is filled amber; the cursor is an
 * amber ring, and a white one on an amber tile. Under the grid, where
 * automatic is in use, what the tuner has chosen.
 *
 * It draws what it is given, like every screen here, and owns the panel on
 * its own, because the LVGL pool holds one screen at a time.
 */
#include <lvgl.h>
#include <stdlib.h>

#include "../core/strings.h"
#include "draw.h"
#include "fonts.h"
#include "screen.h"
#include "theme.h"

#define BW_W 320
#define BW_H 240

/* The grid: four tiles a row, 68 by 30, 8 apart, from y 34 on a 36 pitch,
 * so five rows end at 208 and the note under them sits on the bottom
 * margin. The page has no line of hints. */
#define COLS 4
#define TILE_W 68
#define TILE_H 30
#define TILE_GAP 8
#define GRID_TOP 34
#define ROW_PITCH 36
#define TILE_BASE 21 /* The text's baseline inside a tile. */
#define RING 2
#define NOTE_BASE 226
/* The AM side's four widths sit on the second row rather than the first,
 * so they are not up against the header. */
#define AM_ROW 1
/* The switches' row and columns. */
#define SWITCH_ROW 4
#define SWITCH_COL 2

/* The page's objects, on the heap while it is up: static RAM has no room
 * left. */
typedef struct {
  lv_obj_t *root;
  UiFrame frame;
  lv_obj_t *tile[SCREEN_BW_TILES];
  lv_obj_t *label[SCREEN_BW_TILES];
  lv_obj_t *note;
} BwUi;
static BwUi *sUi;

bool screenBwBegin(void) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sUi != NULL) {
    return true;
  }
  sUi = (BwUi *)calloc(1, sizeof(*sUi));
  if (sUi == NULL) {
    return false;
  }
  const Theme *t = themeCurrent();
  lv_obj_remove_style_all(root);
  lv_obj_set_style_bg_color(root, uiColour(t->ground), 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

  sUi->root = uiBlock(root, t->ground, 0, 0, BW_W, BW_H);
  uiFrameBegin(&sUi->frame, sUi->root, t, t->radio);
  for (uint8_t i = 0; i < SCREEN_BW_TILES; i++) {
    sUi->tile[i] = uiRound(sUi->root, t->rule, 0, 0, TILE_W, TILE_H, UI_TILE_R);
    lv_obj_set_style_border_opa(sUi->tile[i], LV_OPA_COVER, 0);
    sUi->label[i] = uiLabel(sUi->root, &roboto_text, t->measurement);
  }
  sUi->note = uiLabel(sUi->root, &roboto_label, t->dead);
  return true;
}

/* Where tile `i` of `s` sits: the widths four to a row, the switches on
 * the last row's right. */
static void place(const ScreenBw *s, uint8_t i, int16_t *x, int16_t *y) {
  uint8_t row;
  uint8_t col;
  if (s->tile[i].kind != SCREEN_BW_WIDTH) {
    row = SWITCH_ROW;
    col = (uint8_t)(SWITCH_COL + (s->tile[i].kind == SCREEN_BW_EQ ? 1 : 0));
  } else if (!s->hasSwitches) {
    row = AM_ROW;
    col = (uint8_t)(i % COLS);
  } else {
    row = (uint8_t)(i / COLS);
    col = (uint8_t)(i % COLS);
  }
  *x = (int16_t)(UI_MARGIN + col * (TILE_W + TILE_GAP));
  *y = (int16_t)(GRID_TOP + row * ROW_PITCH);
}

void screenBwShow(const ScreenBw *s) {
  if (sUi == NULL || s == NULL) {
    return;
  }
  const Theme *t = themeCurrent();
  uiFrameShow(&sUi->frame, txt(STR_BW_TITLE), s->context, s->position, s->clock,
              NULL, NULL);
  for (uint8_t i = 0; i < SCREEN_BW_TILES; i++) {
    lv_obj_t *tile = sUi->tile[i];
    lv_obj_t *label = sUi->label[i];
    const bool shown = i < s->count;
    uiShowIf(tile, shown);
    uiShowIf(label, shown);
    if (!shown) {
      continue;
    }
    const ScreenBwTile *k = &s->tile[i];
    int16_t x = 0;
    int16_t y = 0;
    place(s, i, &x, &y);
    lv_obj_set_pos(tile, x, y);
    uiSetBgColour(tile, k->filled ? t->radio : t->rule);
    /* The ring is the cursor: amber on a plain tile, white on an amber
     * one, and none elsewhere. */
    lv_obj_set_style_border_width(tile, k->cursor ? RING : 0, 0);
    if (k->cursor) {
      uiSetBorderColour(tile, k->filled ? t->measurement : t->radio);
    }
    uiSetText(label, k->text);
    uiSetColour(label, k->filled   ? t->ground
                       : k->cursor ? t->radio
                                   : t->measurement);
    const int16_t tw = uiTextWidth(label, &roboto_text);
    uiBaseline(label, &roboto_text, (int16_t)(x + (TILE_W - tw) / 2),
               (int16_t)(y + TILE_BASE));
  }
  uiSetOrHide(sUi->note, s->note);
  if (s->note != NULL) {
    uiBaseline(sUi->note, &roboto_label, UI_MARGIN, NOTE_BASE);
  }
}

void screenBwEnd(void) {
  if (sUi == NULL) {
    return;
  }
  if (lv_obj_is_valid(sUi->root)) {
    lv_obj_del(sUi->root);
  }
  free(sUi);
  sUi = NULL;
}
