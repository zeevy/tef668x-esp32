/*
 * The menu screen, in the same style as the radio screen.
 *
 * Three shapes on one set of objects: the list of rows, the value being
 * changed, and a named choice picked from a short list. All three share the
 * header, which is the title alone. A fourth, a question with two buttons,
 * is a box of its own over the middle, with the header hidden.
 *
 * Rows are the radio screen's tiles, 296 by 29 and 4 apart, six to a
 * page, and the row the knob is on is filled in `radio` with dark type.
 * A number being changed sits in the middle of the value panel, above a bar
 * with a rounded fill; the header already names what it is.
 *
 * It owns the panel on its own, like the boot screen, because the LVGL pool
 * holds one screen at a time.
 */
#include <ctype.h>
#include <lvgl.h>
#include <string.h>

#include "../core/menu.h"
#include "../core/strings.h"
#include "draw.h"
#include "fonts.h"
#include "screen.h"
#include "theme.h"

#define MENU_W 320
#define MENU_H 240

/* The value editor: the value panel, 100 high from y 32. */
#define EDIT_PANEL_Y 32
#define EDIT_PANEL_H 100
#define EDIT_LABEL_BASE 56
#define EDIT_VALUE_BASE 112
/* The value's capitals sit on the panel's middle, y 81: they are 38 px tall
 * in the number face and 19 px in the word face. */
#define EDIT_NUMBER_BASE 100
#define EDIT_WORD_BASE 91
/* The bar, and the mark at zero on a signed value. */
#define BAR_Y 148
#define BAR_H 12
#define BAR_R 6
#define BAR_ZERO_Y 144
#define BAR_ZERO_W 2
#define BAR_ZERO_H 20
#define BAR_LIMIT_BASE 180
/* With Touch On, minus, Keep and plus under the limits, in a row of three
 * with the tiles' 8 px gaps, ending on the bottom margin. */
#define STEP_Y 192
#define STEP_H 36
#define STEP_SIDE_W 93
#define STEP_KEEP_X (UI_MARGIN + STEP_SIDE_W + UI_GAP)
#define STEP_KEEP_W (MENU_W - 2 * UI_MARGIN - 2 * (STEP_SIDE_W + UI_GAP))
#define STEP_PLUS_X (MENU_W - UI_MARGIN - STEP_SIDE_W)
/* The digits editor: up to six digits on the panel, one cell each, the one
 * being set in a box, and a dot under each below the panel. */
#define DIGITS_MAX 6
#define DIGIT_CELL_W 38
#define DIGIT_BOX_W 34
#define DIGIT_BOX_TOP 68
#define DIGIT_BOX_H 50
#define DIGIT_BOX_R 4
#define DIGIT_DOT_Y 142
#define DIGIT_DOT_W 18
#define DIGIT_DOT_H 4
#define DIGIT_PLACE_BASE 172
/* A digit still to come: the ground colour 45 % into the panel's. */
#define DIGIT_STILL_MIX 115
/* The dialog: a box of the row colour in the middle, its title on the first
 * line with the icon before it, the facts under it, and the two buttons
 * along its foot. A button's word sits 6 below its middle, as a row's does. */
#define DIALOG_X 16
#define DIALOG_Y 22
#define DIALOG_W 288
#define DIALOG_H 196
#define DIALOG_TITLE_BASE (DIALOG_Y + 30)
#define DIALOG_FACT_BASE (DIALOG_Y + 62)
#define DIALOG_FACT_PITCH 24
#define DIALOG_BUTTON_H 34
#define DIALOG_BUTTON_W ((DIALOG_W - 2 * UI_PAD - UI_GAP) / 2)
#define DIALOG_BUTTON_Y (DIALOG_Y + DIALOG_H - UI_PAD - DIALOG_BUTTON_H)
#define DIALOG_BUTTON_BASE (DIALOG_BUTTON_Y + DIALOG_BUTTON_H / 2 + 6)

static lv_obj_t *sMenu;
static UiFrame sFrame;
/* The list or picker on show, for its touch zones: how many rows are drawn,
 * 0 while another shape is up, and the slot the cursor is on. */
static uint8_t sListRows;
static int8_t sListCursor = -1;
/* A value with a bar on show, and its ends, for its touch zones. */
static bool sEditBar;
static int32_t sEditLow;
static int32_t sEditHigh;
static UiRow sRows[SCREEN_MENU_ROWS];
/* A picker label with its first letter a capital. */
static char sRowWord[SCREEN_MENU_ROWS][32];

/* The value editor. */
static lv_obj_t *sPanel;
/* A list longer than the window shows a scroll bar in the right margin: a
 * track as tall as the six rows, and a thumb for the part on screen. */
#define SCROLL_X (MENU_W - UI_MARGIN / 2 - 1)
#define SCROLL_W 3
#define SCROLL_H (UI_MENU_ROW_PITCH * (SCREEN_MENU_ROWS - 1) + UI_MENU_ROW_H)
#define SCROLL_THUMB_MIN 8
static lv_obj_t *sTrack;
static lv_obj_t *sThumb;
static lv_obj_t *sLabel;
/* Minus, Keep and plus, built only while they show, every part a child of
 * one layer so it goes as one. */
static lv_obj_t *sSteps;
static lv_obj_t *sValueBig;
static lv_obj_t *sValueWord;
static lv_obj_t *sValueUnit;
static lv_obj_t *sBar;
static lv_obj_t *sBarMin;
static lv_obj_t *sBarMax;
/* The PIN digits' parent, built only while the PIN editor shows and the
 * dialog only while it shows: the pool holds the list and the editor with
 * room for the big face's glyphs only when these two are not there too. */
static lv_obj_t *sDigits;
static lv_obj_t *sDigitBox;
static lv_obj_t *sDigit[DIGITS_MAX];
static lv_obj_t *sDigitDots;
static lv_obj_t *sDigitPlace;
static uint8_t sDigitCount;
static uint8_t sDigitAt;
static int16_t sBarFill;
static int16_t sBarZero =
    -1; /* -1 for a value with no zero inside its range. */
/* The dialog, every part a child of one layer so it goes as one. */
static lv_obj_t *sDialog;
static lv_obj_t *sDialogTitle;
static lv_obj_t *sDialogLabel[SCREEN_DIALOG_FACTS];
static lv_obj_t *sDialogValue[SCREEN_DIALOG_FACTS];
static lv_obj_t *sButton[2];
static lv_obj_t *sButtonWord[2];

static void hideRows(void) {
  for (uint8_t i = 0; i < SCREEN_MENU_ROWS; i++) {
    uiRowHide(&sRows[i]);
    lv_obj_remove_flag(sRows[i].value, UI_FLAG_SECRET);
  }
}

static void hideDigits(void) {
  if (sDigits == NULL) {
    return;
  }
  lv_obj_delete(sDigits);
  sDigits = NULL;
  sDigitBox = NULL;
  for (uint8_t i = 0; i < DIGITS_MAX; i++) {
    sDigit[i] = NULL;
  }
  sDigitDots = NULL;
  sDigitPlace = NULL;
  sDigitCount = 0;
  sDigitAt = 0;
}

static void hideDialog(void) {
  if (sDialog == NULL) {
    return;
  }
  lv_obj_delete(sDialog);
  sDialog = NULL;
  sDialogTitle = NULL;
  for (uint8_t i = 0; i < SCREEN_DIALOG_FACTS; i++) {
    sDialogLabel[i] = NULL;
    sDialogValue[i] = NULL;
  }
  for (uint8_t i = 0; i < 2; i++) {
    sButton[i] = NULL;
    sButtonWord[i] = NULL;
  }
}

/* A parent the size of the screen, so its children are placed as they
 * would be on the screen itself. */
static lv_obj_t *wholeScreen(void) {
  lv_obj_t *o = lv_obj_create(sMenu);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, MENU_W, MENU_H);
  return o;
}

static void onDotsDraw(lv_event_t *e);

static void buildDigits(const Theme *t) {
  if (sDigits != NULL) {
    return;
  }
  sDigits = wholeScreen();
  /* The box before the digits, so the digit it holds is drawn over it. */
  sDigitBox = uiRound(sDigits, t->ground, 0, DIGIT_BOX_TOP, DIGIT_BOX_W,
                      DIGIT_BOX_H, DIGIT_BOX_R);
  for (uint8_t i = 0; i < DIGITS_MAX; i++) {
    sDigit[i] = uiLabel(sDigits, &roboto_freq, t->ground);
    lv_obj_add_flag(sDigit[i], UI_FLAG_SECRET);
  }
  sDigitDots = lv_obj_create(sDigits);
  lv_obj_remove_style_all(sDigitDots);
  lv_obj_set_pos(sDigitDots, 0, DIGIT_DOT_Y);
  lv_obj_set_size(sDigitDots, MENU_W, DIGIT_DOT_H);
  lv_obj_add_event_cb(sDigitDots, onDotsDraw, LV_EVENT_DRAW_MAIN, NULL);
  sDigitPlace = uiLabel(sDigits, &roboto_small, t->dead);
}

static void buildDialog(const Theme *t) {
  if (sDialog != NULL) {
    return;
  }
  sDialog = wholeScreen();
  (void)uiRound(sDialog, t->rule, DIALOG_X, DIALOG_Y, DIALOG_W, DIALOG_H,
                UI_RADIUS);
  lv_obj_t *icon = uiLabel(sDialog, &roboto_icons, t->radio);
  uiSetTextStatic(icon, ICON_NEW);
  lv_obj_set_pos(icon, DIALOG_X + UI_PAD, uiIconTop(DIALOG_TITLE_BASE));
  sDialogTitle = uiLabel(sDialog, &roboto_title, t->radio);
  for (uint8_t i = 0; i < SCREEN_DIALOG_FACTS; i++) {
    sDialogLabel[i] = uiLabel(sDialog, &roboto_small, t->dead);
    sDialogValue[i] = uiLabel(sDialog, &roboto_small, t->measurement);
  }
  for (uint8_t i = 0; i < 2; i++) {
    sButton[i] =
        uiRound(sDialog, t->ground,
                (int16_t)(DIALOG_X + UI_PAD + i * (DIALOG_BUTTON_W + UI_GAP)),
                DIALOG_BUTTON_Y, DIALOG_BUTTON_W, DIALOG_BUTTON_H, UI_TILE_R);
    sButtonWord[i] = uiLabel(sDialog, &roboto_text, t->measurement);
  }
}

static void buildSteps(const Theme *t) {
  if (sSteps != NULL) {
    return;
  }
  sSteps = wholeScreen();
  static const int16_t kStepX[3] = {UI_MARGIN, STEP_KEEP_X, STEP_PLUS_X};
  static const int16_t kStepW[3] = {STEP_SIDE_W, STEP_KEEP_W, STEP_SIDE_W};
  for (uint8_t i = 0; i < 3; i++) {
    const bool keep = i == 1;
    (void)uiRound(sSteps, keep ? t->radio : t->rule, kStepX[i], STEP_Y,
                  kStepW[i], STEP_H, UI_TILE_R);
    const lv_font_t *face = keep ? &roboto_text : &roboto_icons;
    lv_obj_t *mark = uiLabel(sSteps, face, keep ? t->ground : t->measurement);
    uiSetTextStatic(mark, i == 0 ? ICON_MINUS
                          : keep ? txt(STR_MENU_KEEP)
                                 : ICON_PLUS);
    const int16_t w = uiTextWidth(mark, face);
    const int16_t x = (int16_t)(kStepX[i] + (kStepW[i] - w) / 2);
    if (keep) {
      /* The capitals on the button's middle, as a tile's word sits. */
      uiBaseline(
          mark, face, x,
          (int16_t)(STEP_Y + STEP_H / 2 + lv_font_get_line_height(face) / 3));
    } else {
      lv_obj_set_pos(
          mark, x,
          (int16_t)(STEP_Y + (STEP_H - UI_ICON_SIZE) / 2 + UI_ICON_INK_DROP));
    }
  }
}

/* A label's colour set only when it changes, as `uiSetColour` does for a
 * theme role, for a colour mixed from two. */
static void setTextColour(lv_obj_t *o, lv_color_t c) {
  if (!lv_color_eq(lv_obj_get_style_text_color(o, LV_PART_MAIN), c)) {
    lv_obj_set_style_text_color(o, c, 0);
  }
}

/* Where digit `i` of `count` starts, so the row of them is centred. */
static int16_t digitCellX(uint8_t i, uint8_t count) {
  return (int16_t)((MENU_W - count * DIGIT_CELL_W) / 2 + i * DIGIT_CELL_W);
}

/* A dot under each digit: the one being set in `radio`, the ones before it
 * in `dead`, the ones still to come in `rule`. */
static void onDotsDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const Theme *t = themeCurrent();
  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.radius = DIGIT_DOT_H / 2;
  dsc.bg_opa = LV_OPA_COVER;
  for (uint8_t i = 0; i < sDigitCount; i++) {
    dsc.bg_color = uiColour(i == sDigitAt  ? t->radio
                            : i < sDigitAt ? t->dead
                                           : t->rule);
    lv_area_t a;
    a.x1 = digitCellX(i, sDigitCount) + (DIGIT_CELL_W - DIGIT_DOT_W) / 2;
    a.y1 = area.y1;
    a.x2 = a.x1 + DIGIT_DOT_W - 1;
    a.y2 = a.y1 + DIGIT_DOT_H - 1;
    lv_draw_rect(layer, &dsc, &a);
  }
}

/* The scroll bar for a window of `SCREEN_MENU_ROWS` on a list of `total`,
 * from row `top`; none when the whole list fits. */
static void showScroll(uint8_t total, uint8_t top) {
  const bool on = total > SCREEN_MENU_ROWS;
  uiShowIf(sTrack, on);
  uiShowIf(sThumb, on);
  if (!on) {
    return;
  }
  if (top > total - SCREEN_MENU_ROWS) {
    top = (uint8_t)(total - SCREEN_MENU_ROWS);
  }
  int16_t h = (int16_t)(SCROLL_H * SCREEN_MENU_ROWS / total);
  if (h < SCROLL_THUMB_MIN) {
    h = SCROLL_THUMB_MIN;
  }
  const int16_t y = (int16_t)(UI_ROW_TOP + (int32_t)(SCROLL_H - h) * top /
                                               (total - SCREEN_MENU_ROWS));
  lv_obj_set_size(sThumb, SCROLL_W, h);
  lv_obj_set_pos(sThumb, SCROLL_X, y);
}

static void hideSteps(void) {
  if (sSteps != NULL) {
    lv_obj_delete(sSteps);
    sSteps = NULL;
  }
}

static void showEditor(bool on) {
  uiShowIf(sPanel, on);
  if (on) {
    showScroll(0, 0);
  }
  uiShowIf(sBar, on);
  if (!on) {
    hideSteps();
    uiShowIf(sLabel, false);
    uiShowIf(sValueBig, false);
    uiShowIf(sValueWord, false);
    uiShowIf(sValueUnit, false);
    uiShowIf(sBarMin, false);
    uiShowIf(sBarMax, false);
    hideDigits();
  }
}

static void onBarDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const Theme *t = themeCurrent();
  const int16_t w = (int16_t)(area.x2 - area.x1 + 1);
  const int16_t barTop = (int16_t)(BAR_Y - BAR_ZERO_Y);
  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.radius = BAR_R;
  dsc.bg_opa = LV_OPA_COVER;
  dsc.bg_color = uiColour(t->rule);
  lv_area_t a;
  a.x1 = area.x1;
  a.y1 = area.y1 + barTop;
  a.x2 = area.x1 + w - 1;
  a.y2 = a.y1 + BAR_H - 1;
  lv_draw_rect(layer, &dsc, &a);
  dsc.bg_color = uiColour(t->radio);
  a.x2 = area.x1 + sBarFill - 1;
  lv_draw_rect(layer, &dsc, &a);
  if (sBarZero >= 0) {
    uiFillRect(layer, &area, sBarZero, 0, BAR_ZERO_W, BAR_ZERO_H,
               uiColour(t->measurement));
  }
}

bool screenMenuBegin(void) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sMenu != NULL) {
    return true;
  }
  const Theme *t = themeCurrent();

  /* The root is prepared here as well as in `screenBegin`, because this
   * screen can be built before the radio layout has been. */
  uiScreenRoot(root, t->ground);

  sMenu = uiBlock(root, t->ground, 0, 0, MENU_W, MENU_H);
  uiFrameBegin(&sFrame, sMenu, t, t->radio);
  for (uint8_t i = 0; i < SCREEN_MENU_ROWS; i++) {
    uiRowBegin(&sRows[i], sMenu, t, UI_MENU_ROW_H, UI_MENU_ROW_PITCH);
  }
  sTrack = uiRound(sMenu, t->rule, SCROLL_X, UI_ROW_TOP, SCROLL_W, SCROLL_H, 1);
  sThumb = uiRound(sMenu, t->dead, SCROLL_X, UI_ROW_TOP, SCROLL_W, SCROLL_H, 1);

  sPanel = uiRound(sMenu, t->radio, UI_MARGIN, EDIT_PANEL_Y,
                   (int16_t)(MENU_W - 2 * UI_MARGIN), EDIT_PANEL_H, UI_RADIUS);
  sLabel = uiLabel(sMenu, &roboto_small, t->ground);
  sValueBig = uiLabel(sMenu, &roboto_freq, t->ground);
  sValueWord = uiLabel(sMenu, &roboto_name, t->ground);
  sValueUnit = uiLabel(sMenu, &roboto_menu, t->ground);
  sBar = lv_obj_create(sMenu);
  lv_obj_remove_style_all(sBar);
  lv_obj_set_pos(sBar, UI_MARGIN, BAR_ZERO_Y);
  lv_obj_set_size(sBar, (int16_t)(MENU_W - 2 * UI_MARGIN), BAR_ZERO_H);
  lv_obj_add_event_cb(sBar, onBarDraw, LV_EVENT_DRAW_MAIN, NULL);
  sBarMin = uiLabel(sMenu, &roboto_label, t->dead);
  sBarMax = uiLabel(sMenu, &roboto_label, t->dead);
  showEditor(false);
  return true;
}

void screenMenuShow(const ScreenMenu *menu) {
  if (sMenu == NULL || menu == NULL) {
    return;
  }
  const Theme *t = themeCurrent();
  showEditor(false);
  hideRows();
  hideDialog();
  sListRows = 0;
  sEditBar = false;
  sListCursor = -1;
  uiFrameShow(&sFrame, menu->title, NULL, NULL, NULL, NULL, NULL);
  /* The header is the title alone, so not the sleep mark either. */
  uiShowIf(sFrame.sleep, false);
  showScroll(menu->total, menu->top);
  for (uint8_t i = 0; i < SCREEN_MENU_ROWS; i++) {
    const ScreenMenuRow *row = &menu->rows[i];
    if (row->name == NULL) {
      continue;
    }
    sListRows = (uint8_t)(i + 1);
    if (row->selected) {
      sListCursor = (int8_t)i;
    }
    const char *value =
        row->selected && menu->note != NULL ? menu->note : row->value;
    UiRowView view;
    memset(&view, 0, sizeof(view));
    view.slot = i;
    view.name = row->name;
    view.value = value;
    view.cursor = row->selected;
    /* A row that opens a level shows its count quietly, beside the
     * chevron. */
    view.dimValue = row->opens;
    view.opens = row->opens;
    uiRowShow(&sRows[i], t, &view);
    lv_obj_set_flag(sRows[i].value, UI_FLAG_SECRET, row->secret);
  }
}

/* A named choice out of a short list, its first letter a capital the way
 * every row of the list starts with one. */
static void showPicker(const ScreenMenuValue *v) {
  const Theme *t = themeCurrent();
  sListCursor = -1;
  showScroll(v->pickerTotal, v->pickerTop);
  for (uint8_t i = 0; i < SCREEN_MENU_PICKER_ROWS && i < SCREEN_MENU_ROWS;
       i++) {
    const ScreenMenuPickerRow *row = &v->picker[i];
    if (row->name == NULL) {
      continue;
    }
    sListRows = (uint8_t)(i + 1);
    if (row->isCursor) {
      sListCursor = (int8_t)i;
    }
    strncpy(sRowWord[i], row->name, sizeof(sRowWord[i]) - 1);
    sRowWord[i][sizeof(sRowWord[i]) - 1] = '\0';
    sRowWord[i][0] = (char)toupper((unsigned char)sRowWord[i][0]);
    UiRowView view;
    memset(&view, 0, sizeof(view));
    view.slot = i;
    view.name = sRowWord[i];
    view.value = row->isCursor ? v->note : NULL;
    view.cursor = row->isCursor;
    view.saved = row->isSaved;
    view.swatch = v->isTheme;
    view.swatchTheme = row->themeIndex;
    uiRowShow(&sRows[i], t, &view);
  }
}

/*
 * Whether every character is one the frequency face carries: digits, a
 * point, a plus, a minus, a space and a colon. That face is most of why it is
 * small, and a character it lacks draws as nothing at all, so a value with
 * anything else takes the word face instead.
 */
static bool numericFace(const char *text) {
  if (text[0] == '\0') {
    return false;
  }
  for (const char *c = text; *c != '\0'; c++) {
    if (!((*c >= '0' && *c <= '9') || *c == '+' || *c == '-' || *c == '.' ||
          *c == ' ' || *c == ':')) {
      return false;
    }
  }
  return true;
}

/*
 * Digits in place of the value and the bar. Each is its own label, centred
 * in its cell, so a reader of the screen's texts sees them as the panel
 * shows them: the one being set in `radio` inside its box, the ones before
 * it in `ground`, the ones still to come a dim mix of the two.
 */
static void showDigits(const ScreenMenuValue *v) {
  const Theme *t = themeCurrent();
  uiShowIf(sValueBig, false);
  uiShowIf(sValueWord, false);
  uiShowIf(sValueUnit, false);
  uiShowIf(sBar, false);
  uiShowIf(sBarMin, false);
  uiShowIf(sBarMax, false);
  uiSetOrHide(sLabel, v->label);
  uiBaseline(sLabel, &roboto_small, (int16_t)(UI_MARGIN + UI_PAD),
             EDIT_LABEL_BASE);
  const char *digits = v->digits != NULL ? v->digits : "";
  const size_t len = strlen(digits);
  const uint8_t count = (uint8_t)(len < DIGITS_MAX ? len : DIGITS_MAX);
  if (count == 0) {
    hideDigits();
    return;
  }
  buildDigits(t);
  const uint8_t at = v->digitAt < count ? v->digitAt : (uint8_t)(count - 1);
  const lv_color_t still =
      lv_color_mix(uiColour(t->ground), uiColour(t->radio), DIGIT_STILL_MIX);
  char one[2] = {0, 0};
  for (uint8_t i = 0; i < DIGITS_MAX; i++) {
    if (i >= count) {
      uiShowIf(sDigit[i], false);
      continue;
    }
    one[0] = digits[i];
    uiShowIf(sDigit[i], true);
    uiSetText(sDigit[i], one);
    setTextColour(sDigit[i], i == at  ? uiColour(t->radio)
                             : i < at ? uiColour(t->ground)
                                      : still);
    const int16_t w = uiTextWidth(sDigit[i], &roboto_freq);
    uiBaseline(sDigit[i], &roboto_freq,
               (int16_t)(digitCellX(i, count) + (DIGIT_CELL_W - w) / 2),
               EDIT_VALUE_BASE);
  }
  uiShowIf(sDigitBox, true);
  lv_obj_set_pos(
      sDigitBox,
      (int16_t)(digitCellX(at, count) + (DIGIT_CELL_W - DIGIT_BOX_W) / 2),
      DIGIT_BOX_TOP);
  uiShowIf(sDigitDots, true);
  if (count != sDigitCount || at != sDigitAt) {
    sDigitCount = count;
    sDigitAt = at;
    lv_obj_invalidate(sDigitDots);
  }
  uiSetOrHide(sDigitPlace, v->digitPlace);
  if (v->digitPlace != NULL) {
    const int16_t w = uiTextWidth(sDigitPlace, &roboto_small);
    uiBaseline(sDigitPlace, &roboto_small, (int16_t)((MENU_W - w) / 2),
               DIGIT_PLACE_BASE);
  }
}

void screenMenuValueShow(const ScreenMenuValue *v) {
  if (sMenu == NULL || v == NULL) {
    return;
  }
  sListRows = 0;
  sEditBar = false;
  /* With the buttons on the bottom line, the note moves to the top of the
   * value panel. */
  const bool buttons =
      v->buttons && v->hasRange && !v->isPicker && !v->isDigits;
  uiFrameShow(&sFrame, v->name, NULL, NULL, NULL,
              v->isPicker || buttons ? NULL : v->note, NULL);
  uiShowIf(sFrame.sleep, false);
  hideRows();
  hideDialog();
  if (v->isPicker) {
    showEditor(false);
    showPicker(v);
    return;
  }
  showEditor(true);
  if (buttons) {
    buildSteps(themeCurrent());
  } else {
    hideSteps();
  }
  if (v->isDigits) {
    showDigits(v);
    return;
  }
  hideDigits();
  uiSetOrHide(sLabel, buttons ? v->note : NULL);
  uiBaseline(sLabel, &roboto_small, (int16_t)(UI_MARGIN + UI_PAD),
             EDIT_LABEL_BASE);

  /* The value and its unit centred as one on the panel, the way the radio
   * screen keeps a frequency and its unit together. */
  const char *text = v->value != NULL ? v->value : "";
  const bool numeric = numericFace(text);
  lv_obj_t *big = numeric ? sValueBig : sValueWord;
  const lv_font_t *face = numeric ? &roboto_freq : &roboto_name;
  uiShowIf(sValueBig, numeric);
  uiShowIf(sValueWord, !numeric);
  uiSetText(big, text);
  const int16_t width = uiTextWidth(big, face);
  const bool hasUnit = v->unit != NULL && v->unit[0] != '\0';
  int16_t unitWidth = 0;
  uiSetOrHide(sValueUnit, hasUnit ? v->unit : NULL);
  if (hasUnit) {
    unitWidth = (int16_t)(UI_UNIT_GAP + uiTextWidth(sValueUnit, &roboto_menu));
  }
  const int16_t left = (int16_t)((MENU_W - width - unitWidth) / 2);
  const int16_t base = numeric ? EDIT_NUMBER_BASE : EDIT_WORD_BASE;
  uiBaseline(big, face, left, base);
  if (hasUnit) {
    uiBaseline(sValueUnit, &roboto_menu, (int16_t)(left + width + UI_UNIT_GAP),
               base);
  }

  /*
   * The bar, and the two limits that say what its ends are. Left out for a
   * row whose values are words rather than a range: a bar between two
   * things that are not a distance apart would be drawing one.
   */
  uiShowIf(sBar, v->hasRange);
  uiShowIf(sBarMin, v->hasRange);
  uiShowIf(sBarMax, v->hasRange);
  if (!v->hasRange) {
    return;
  }
  sEditBar = true;
  sEditLow = v->min;
  sEditHigh = v->max;
  const int16_t barW = (int16_t)(MENU_W - 2 * UI_MARGIN);
  int32_t span = v->max - v->min;
  int32_t at = v->at - v->min;
  if (span <= 0) {
    span = 1;
    at = 0;
  }
  if (at < 0) {
    at = 0;
  }
  if (at > span) {
    at = span;
  }
  /* At least the bar's own height, so the lowest value still shows a
   * rounded end rather than nothing. */
  int16_t fill = (int16_t)((barW * at + span / 2) / span);
  if (fill < BAR_H) {
    fill = BAR_H;
  }
  const int16_t zero =
      v->min < 0 && v->max > 0
          ? (int16_t)((barW * (-v->min) + span / 2) / span - BAR_ZERO_W / 2)
          : (int16_t)-1;
  if (fill != sBarFill || zero != sBarZero) {
    sBarFill = fill;
    sBarZero = zero;
    lv_obj_invalidate(sBar);
  }
  uiSetText(sBarMin, v->minText != NULL ? v->minText : "");
  uiBaseline(sBarMin, &roboto_label, UI_MARGIN, BAR_LIMIT_BASE);
  uiSetText(sBarMax, v->maxText != NULL ? v->maxText : "");
  uiBaselineRight(sBarMax, &roboto_label, (int16_t)(MENU_W - UI_MARGIN),
                  BAR_LIMIT_BASE);
}

void screenMenuDialogShow(const ScreenMenuDialog *d) {
  if (sMenu == NULL || d == NULL) {
    return;
  }
  sListRows = 0;
  sEditBar = false;
  const Theme *t = themeCurrent();
  /* The box covers the middle of the screen, and the header and the rows
   * would still show round it, so they go. */
  showEditor(false);
  hideRows();
  showScroll(0, 0);
  uiFrameShow(&sFrame, NULL, NULL, NULL, NULL, NULL, NULL);
  uiShowIf(sFrame.sleep, false);
  buildDialog(t);

  uiSetText(sDialogTitle, d->title != NULL ? d->title : "");
  uiBaseline(sDialogTitle, &roboto_title,
             (int16_t)(DIALOG_X + UI_PAD + UI_ICON_SIZE + UI_GAP),
             DIALOG_TITLE_BASE);
  bool ended = false;
  for (uint8_t i = 0; i < SCREEN_DIALOG_FACTS; i++) {
    ended = ended || d->label[i] == NULL;
    uiShowIf(sDialogLabel[i], !ended);
    uiShowIf(sDialogValue[i], !ended);
    if (ended) {
      continue;
    }
    const int16_t base = (int16_t)(DIALOG_FACT_BASE + i * DIALOG_FACT_PITCH);
    uiSetText(sDialogLabel[i], d->label[i]);
    uiBaseline(sDialogLabel[i], &roboto_small, DIALOG_X + UI_PAD, base);
    uiSetText(sDialogValue[i], d->value[i] != NULL ? d->value[i] : "");
    uiBaselineRight(sDialogValue[i], &roboto_small,
                    DIALOG_X + DIALOG_W - UI_PAD, base);
  }
  for (uint8_t i = 0; i < 2; i++) {
    const bool on = i == d->cursor;
    uiSetBgColour(sButton[i], on ? t->radio : t->ground);
    uiSetColour(sButtonWord[i], on ? t->ground : t->measurement);
    uiSetText(sButtonWord[i], d->button[i] != NULL ? d->button[i] : "");
    const int16_t w = uiTextWidth(sButtonWord[i], &roboto_text);
    uiBaseline(sButtonWord[i], &roboto_text,
               (int16_t)(DIALOG_X + UI_PAD + i * (DIALOG_BUTTON_W + UI_GAP) +
                         (DIALOG_BUTTON_W - w) / 2),
               DIALOG_BUTTON_BASE);
  }
}

void screenMenuEnd(void) {
  sListRows = 0;
  sEditBar = false;
  if (sMenu == NULL) {
    return;
  }
  hideDigits();
  hideDialog();
  uiDropRoot(&sMenu);
  sPanel = NULL;
  sLabel = NULL;
  sSteps = NULL;
  sValueBig = NULL;
  sValueWord = NULL;
  sValueUnit = NULL;
  sBar = NULL;
  sBarMin = NULL;
  sBarMax = NULL;
  sBarFill = 0;
  sBarZero = -1;
}

/* Where the first row's zone starts, halfway across the gap above it. */
#define ZONE_FIRST (UI_ROW_TOP - (UI_MENU_ROW_PITCH - UI_MENU_ROW_H) / 2)

static const TouchZone kBack = {0, 0, MENU_W, ZONE_FIRST, MENU_ZONE_BACK};

bool screenMenuIsBack(TouchPoint p) {
  return touchZoneAt(&kBack, 1, p) != TOUCH_NO_ZONE;
}

/* The value panel, from the header down to its foot, and the bar with its
 * limits under it. */
#define ZONE_BAR_TOP (EDIT_PANEL_Y + EDIT_PANEL_H)
#define ZONE_BAR_H (BAR_LIMIT_BASE + 8 - ZONE_BAR_TOP)

/* Minus, Keep and plus, from halfway across the gap above them to the
 * screen's foot, and halfway across the gaps between them. */
#define ZONE_STEP_TOP (ZONE_BAR_TOP + ZONE_BAR_H)
#define ZONE_STEP_H (MENU_H - ZONE_STEP_TOP)
#define ZONE_KEEP_X (STEP_KEEP_X - UI_GAP / 2)
#define ZONE_PLUS_X (STEP_PLUS_X - UI_GAP / 2)

int screenMenuZones(TouchZone *out, int max) {
  if (sMenu != NULL && sEditBar && max >= 3) {
    out[0] = kBack;
    out[1] = {0, ZONE_FIRST, MENU_W, (int16_t)(ZONE_BAR_TOP - ZONE_FIRST),
              MENU_ZONE_PANEL};
    out[2] = {0, ZONE_BAR_TOP, MENU_W, ZONE_BAR_H, MENU_ZONE_BAR};
    if (sSteps == NULL || max < 6) {
      return 3;
    }
    out[3] = {0, ZONE_STEP_TOP, ZONE_KEEP_X, ZONE_STEP_H, MENU_ZONE_MINUS};
    out[4] = {ZONE_KEEP_X, ZONE_STEP_TOP, (int16_t)(ZONE_PLUS_X - ZONE_KEEP_X),
              ZONE_STEP_H, MENU_ZONE_KEEP};
    out[5] = {ZONE_PLUS_X, ZONE_STEP_TOP, (int16_t)(MENU_W - ZONE_PLUS_X),
              ZONE_STEP_H, MENU_ZONE_PLUS};
    return 6;
  }
  if (sMenu == NULL || sListRows == 0 || max < 1 + sListRows) {
    return 0;
  }
  const int16_t first = ZONE_FIRST;
  out[0] = kBack;
  for (uint8_t i = 0; i < sListRows; i++) {
    out[1 + i] = {0, (int16_t)(first + i * UI_MENU_ROW_PITCH), MENU_W,
                  UI_MENU_ROW_PITCH, (uint8_t)(MENU_ZONE_ROW + i)};
  }
  return 1 + sListRows;
}

const char *screenMenuZoneName(int id) {
  static const char *const kNames[] = {"",      "back", "row1", "row2",  "row3",
                                       "row4",  "row5", "row6", "panel", "bar",
                                       "minus", "keep", "plus"};
  return id > 0 && id < (int)(sizeof(kNames) / sizeof(kNames[0])) ? kNames[id]
                                                                  : "";
}

int screenMenuCursorSlot(void) {
  return sListRows != 0 ? sListCursor : -1;
}

int32_t screenMenuBarValue(int16_t x) {
  return menuBarValue(x - UI_MARGIN, MENU_W - 2 * UI_MARGIN, BAR_H, sEditLow,
                      sEditHigh);
}
