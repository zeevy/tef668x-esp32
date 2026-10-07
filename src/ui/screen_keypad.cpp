/*
 * The frequency keypad: digits, a backspace, Cancel and OK, for typing a
 * frequency by touch.
 *
 * The number being typed sits on a panel, the digits so far and nothing
 * more; the panel is empty before the first.
 * Each key is a plain tile, OK the filled one.
 *
 * It owns the panel on its own, like the bandwidth page, because the LVGL
 * pool holds one screen at a time.
 */
#include <lvgl.h>
#include <stdlib.h>

#include "../core/strings.h"
#include "draw.h"
#include "fonts.h"
#include "screen.h"
#include "theme.h"

#define KEYPAD_W 320
#define KEYPAD_H 240

/* Three rows of five: the digits on two, then backspace, Cancel and OK. */
#define PANEL_H 60
#define NUMBER_FONT roboto_freq
#define NUMBER_BASE 82
#define KEY_TOP 98
#define KEY_H 40
#define KEY_GAP 4
#define COLS 5
#define KEY_W ((KEYPAD_W - 2 * UI_MARGIN - (COLS - 1) * KEY_GAP) / COLS)
#define CX(c) (UI_MARGIN + (c) * (KEY_W + KEY_GAP))
#define RY(r) (KEY_TOP + (r) * (KEY_H + KEY_GAP))
#define SPAN(n) ((n) * KEY_W + ((n) - 1) * KEY_GAP)
static const struct {
  int16_t x, y, w, h;
} kBox[SCREEN_KEYPAD_KEYS] = {
    {CX(4), RY(1), KEY_W, KEY_H},   {CX(0), RY(0), KEY_W, KEY_H},
    {CX(1), RY(0), KEY_W, KEY_H},   {CX(2), RY(0), KEY_W, KEY_H},
    {CX(3), RY(0), KEY_W, KEY_H},   {CX(4), RY(0), KEY_W, KEY_H},
    {CX(0), RY(1), KEY_W, KEY_H},   {CX(1), RY(1), KEY_W, KEY_H},
    {CX(2), RY(1), KEY_W, KEY_H},   {CX(3), RY(1), KEY_W, KEY_H},
    {CX(0), RY(2), KEY_W, KEY_H},   {CX(1), RY(2), SPAN(2), KEY_H},
    {CX(3), RY(2), SPAN(2), KEY_H},
};
#define PANEL_Y 32

typedef struct {
  lv_obj_t *root;
  UiFrame frame;
  lv_obj_t *number;
  lv_obj_t *key[SCREEN_KEYPAD_KEYS];
  lv_obj_t *label[SCREEN_KEYPAD_KEYS];
} KeypadUi;
static KeypadUi *sUi;

bool screenKeypadBegin(void) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sUi != NULL) {
    return true;
  }
  sUi = (KeypadUi *)calloc(1, sizeof(*sUi));
  if (sUi == NULL) {
    return false;
  }
  const Theme *t = themeCurrent();
  uiScreenRoot(root, t->ground);
  sUi->root = uiBlock(root, t->ground, 0, 0, KEYPAD_W, KEYPAD_H);
  uiFrameBegin(&sUi->frame, sUi->root, t, t->radio);
  uiRound(sUi->root, t->radio, UI_MARGIN, PANEL_Y, KEYPAD_W - 2 * UI_MARGIN,
          PANEL_H, UI_RADIUS);
  sUi->number = uiLabel(sUi->root, &NUMBER_FONT, t->ground);
  for (uint8_t i = 0; i < SCREEN_KEYPAD_KEYS; i++) {
    const bool ok = i == SCREEN_KEYPAD_OK;
    sUi->key[i] = uiRound(sUi->root, ok ? t->radio : t->rule, kBox[i].x,
                          kBox[i].y, kBox[i].w, kBox[i].h, UI_TILE_R);
    const lv_font_t *face = i < 10                         ? &roboto_value
                            : i == SCREEN_KEYPAD_BACKSPACE ? &roboto_icons
                                                           : &roboto_text;
    sUi->label[i] = uiLabel(sUi->root, face, ok ? t->ground : t->measurement);
    static char digits[10][2];
    const char *text = NULL;
    if (i < 10) {
      digits[i][0] = (char)('0' + i);
      text = digits[i];
    } else if (i == SCREEN_KEYPAD_BACKSPACE) {
      text = ICON_BACKSPACE;
    } else {
      text = txt(i == SCREEN_KEYPAD_OK ? STR_KEYPAD_OK : STR_KEYPAD_CANCEL);
    }
    uiSetTextStatic(sUi->label[i], text);
    const int16_t w = uiTextWidth(sUi->label[i], face);
    if (i == SCREEN_KEYPAD_BACKSPACE) {
      lv_obj_set_pos(sUi->label[i], (int16_t)(kBox[i].x + (kBox[i].w - w) / 2),
                     (int16_t)(kBox[i].y + (kBox[i].h - UI_ICON_SIZE) / 2 +
                               UI_ICON_INK_DROP));
    } else {
      /* The capitals on the key's middle, as a tile's word sits. */
      uiBaseline(sUi->label[i], face,
                 (int16_t)(kBox[i].x + (kBox[i].w - w) / 2),
                 (int16_t)(kBox[i].y + kBox[i].h / 2 +
                           lv_font_get_line_height(face) / 3));
    }
  }
  return true;
}

void screenKeypadShow(const ScreenKeypad *k) {
  if (sUi == NULL || k == NULL) {
    return;
  }
  uiFrameShow(&sUi->frame, txt(STR_KEYPAD_TITLE), k->context, NULL, k->clock,
              NULL, NULL);
  uiSetText(sUi->number, k->typed != NULL ? k->typed : "");
  const int16_t w = uiTextWidth(sUi->number, &NUMBER_FONT);
  uiBaseline(sUi->number, &NUMBER_FONT, (int16_t)((KEYPAD_W - w) / 2),
             NUMBER_BASE);
}

void screenKeypadEnd(void) {
  if (sUi == NULL) {
    return;
  }
  uiDropRoot(&sUi->root);
  free(sUi);
  sUi = NULL;
}

int screenKeypadZones(TouchZone *out, int max) {
  if (sUi == NULL || max < 1 + SCREEN_KEYPAD_KEYS) {
    return 0;
  }
  /* The header, which is Cancel, and each key reaching halfway across the
   * gaps round it. */
  out[0] = {0, 0, KEYPAD_W, (int16_t)(PANEL_Y - 2), KEYPAD_ZONE_BACK};
  const int16_t half = KEY_GAP / 2;
  for (uint8_t i = 0; i < SCREEN_KEYPAD_KEYS; i++) {
    out[1 + i] = {(int16_t)(kBox[i].x - half), (int16_t)(kBox[i].y - half),
                  (int16_t)(kBox[i].w + KEY_GAP),
                  (int16_t)(kBox[i].h + KEY_GAP),
                  (uint8_t)(KEYPAD_ZONE_KEY + i)};
  }
  return 1 + SCREEN_KEYPAD_KEYS;
}

const char *screenKeypadZoneName(int id) {
  static const char *const kNames[] = {
      "",  "back", "0", "1", "2",         "3",      "4", "5",
      "6", "7",    "8", "9", "backspace", "cancel", "ok"};
  return id > 0 && id < (int)(sizeof(kNames) / sizeof(kNames[0])) ? kNames[id]
                                                                  : "";
}
