/*
 * The DX Catches page, the fourth page of DX mode.
 *
 * The confirmed PIs of this session, newest first, six rows to a screen on
 * the menu's row height and pitch, since like the menu the page has no line
 * of hints under them. Columns: the local time, the frequency, the PI, the
 * name, the country, the NEW badge, the best level and how many times it
 * was confirmed. The row under the cursor is amber. With nothing caught it
 * shows an empty box saying so.
 *
 * It draws what it is given, like every screen here, and owns the panel on
 * its own, because the LVGL pool holds one screen at a time.
 */
#include <lvgl.h>

#include "../core/strings.h"
#include "draw.h"
#include "fonts.h"
#include "screen.h"
#include "theme.h"

#define CATCH_W 320
#define CATCH_H 240

#define ROW_X UI_MARGIN
#define ROW_W (CATCH_W - 2 * UI_MARGIN)
/* The text's baseline 6 rows below the row's middle, as on the menu. */
#define ROW_BASE (UI_MENU_ROW_H / 2 + 6)

/* The columns, in pixels from the left. NEW starts at 222 and the level ends at
 * 270, which keeps UI_GAP between the 16 px badge and a four character level
 * such as "48.2". The country and its help mark are never both shown, so the
 * country column ends by 220, and the count's widest likely text,
 * "\xC3\x9799", starts at 276. */
#define COL_TIME 24
#define COL_FREQ 58
#define COL_PI 106
#define COL_PS 146
#define COL_PS_W 56
#define COL_COUNTRY 204
#define COL_NEW 222
#define COL_LEVEL 270
#define COL_COUNT 296
#define COUNTRY_ICON_GAP 1

/* The empty box. */
#define EMPTY_Y 32
#define EMPTY_H 102
#define EMPTY_MID (CATCH_W / 2)
#define EMPTY_ICON_BASE 76
#define EMPTY_TITLE_BASE 100
#define EMPTY_NOTE_BASE 120

typedef struct {
  lv_obj_t *tile;
  lv_obj_t *time;
  lv_obj_t *freq;
  lv_obj_t *pi;
  lv_obj_t *ps;
  lv_obj_t *country;
  lv_obj_t *countryMark;
  lv_obj_t *isNew;
  lv_obj_t *level;
  lv_obj_t *count;
  char fit[40];
} CatchRow;

static lv_obj_t *sRoot;
static UiFrame sFrame;
static CatchRow sRow[SCREEN_CATCH_ROWS];
static lv_obj_t *sEmpty;
static lv_obj_t *sEmptyIcon;
static lv_obj_t *sEmptyTitle;
static lv_obj_t *sEmptyNote;

static int16_t rowTop(uint8_t i) {
  return (int16_t)(UI_ROW_TOP + i * UI_MENU_ROW_PITCH);
}

bool screenCatchesBegin(void) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sRoot != NULL) {
    return true;
  }
  const Theme *t = themeCurrent();
  uiScreenRoot(root, t->ground);

  sRoot = uiBlock(root, t->ground, 0, 0, CATCH_W, CATCH_H);
  uiFrameBegin(&sFrame, sRoot, t, t->radio);
  /* Set now, so the header's room is known on the first show. */
  uiSetText(sFrame.title, txt(STR_DX_TITLE_CATCHES));

  for (uint8_t i = 0; i < SCREEN_CATCH_ROWS; i++) {
    CatchRow *r = &sRow[i];
    r->tile = uiRound(sRoot, t->rule, ROW_X, rowTop(i), ROW_W, UI_MENU_ROW_H,
                      UI_TILE_R);
    r->time = uiLabel(sRoot, &roboto_label, t->dead);
    r->freq = uiLabel(sRoot, &roboto_small, t->measurement);
    r->pi = uiLabel(sRoot, &roboto_text, t->radio);
    r->ps = uiLabel(sRoot, &roboto_small, t->measurement);
    r->country = uiLabel(sRoot, &roboto_label, t->dead);
    r->countryMark = uiLabel(sRoot, &roboto_icons, t->dead);
    uiSetTextStatic(r->countryMark, ICON_HELP);
    r->isNew = uiLabel(sRoot, &roboto_icons, t->good);
    uiSetTextStatic(r->isNew, ICON_NEW);
    r->level = uiLabel(sRoot, &roboto_label, t->measurement);
    r->count = uiLabel(sRoot, &roboto_label, t->dead);
  }

  sEmpty = uiRound(sRoot, t->rule, ROW_X, EMPTY_Y, ROW_W, EMPTY_H, UI_RADIUS);
  sEmptyIcon = uiLabel(sRoot, &roboto_icons, t->dead);
  uiSetTextStatic(sEmptyIcon, ICON_INBOX);
  lv_obj_set_pos(sEmptyIcon, EMPTY_MID - UI_ICON_SIZE / 2,
                 uiIconTop(EMPTY_ICON_BASE));
  sEmptyTitle = uiLabel(sRoot, &roboto_text, t->measurement);
  uiSetTextStatic(sEmptyTitle, txt(STR_DX_CATCHES_EMPTY_TITLE));
  sEmptyNote = uiLabel(sRoot, &roboto_label, t->dead);
  uiSetTextStatic(sEmptyNote, txt(STR_DX_CATCHES_EMPTY_NOTE));
  return true;
}

static void centre(lv_obj_t *o, const lv_font_t *font, int16_t baseline) {
  const int16_t w = uiTextWidth(o, font);
  uiBaseline(o, font, (int16_t)(EMPTY_MID - w / 2), baseline);
}

static void showRow(uint8_t i, const ScreenCatchRow *row, bool on,
                    const Theme *t) {
  CatchRow *r = &sRow[i];
  const int16_t base = (int16_t)(rowTop(i) + ROW_BASE);
  const ThemeColour fg = on ? t->ground : t->measurement;
  const ThemeColour dim = on ? t->ground : t->dead;
  uiShowIf(r->tile, true);
  uiSetBgColour(r->tile, on ? t->radio : t->rule);

  uiSetOrHide(r->time, row->time);
  uiSetColour(r->time, dim);
  uiBaseline(r->time, &roboto_label, COL_TIME, base);
  uiSetOrHide(r->freq, row->frequency);
  uiSetColour(r->freq, fg);
  uiBaseline(r->freq, &roboto_small, COL_FREQ, base);
  uiSetOrHide(r->pi, row->pi);
  uiSetColour(r->pi, on ? t->ground : t->radio);
  uiBaseline(r->pi, &roboto_text, COL_PI, base);

  const char *ps = row->ps != NULL ? uiFitText(row->ps, &roboto_small, COL_PS_W,
                                               r->fit, sizeof(r->fit))
                                   : NULL;
  uiSetOrHide(r->ps, ps);
  uiSetColour(r->ps, fg);
  uiBaseline(r->ps, &roboto_small, COL_PS, base);

  uiSetOrHide(r->country, row->country);
  uiSetColour(r->country, dim);
  uiBaseline(r->country, &roboto_label, COL_COUNTRY, base);
  const int16_t cw =
      row->country != NULL ? uiTextWidth(r->country, &roboto_label) : 0;
  uiShowIf(r->countryMark, row->countryUnsure);
  uiSetColour(r->countryMark, dim);
  lv_obj_set_pos(r->countryMark,
                 (int16_t)(COL_COUNTRY + cw + (cw > 0 ? COUNTRY_ICON_GAP : 0)),
                 uiIconTop(base));

  uiShowIf(r->isNew, row->isNew);
  uiSetColour(r->isNew, on ? t->ground : t->good);
  lv_obj_set_pos(r->isNew, COL_NEW, uiIconTop(base));

  uiSetOrHide(r->level, row->level);
  uiSetColour(r->level, fg);
  if (row->level != NULL) {
    uiBaselineRight(r->level, &roboto_label, COL_LEVEL, base);
  }
  uiSetOrHide(r->count, row->count);
  uiSetColour(r->count, dim);
  if (row->count != NULL) {
    uiBaselineRight(r->count, &roboto_label, COL_COUNT, base);
  }
}

static void hideRow(uint8_t i) {
  CatchRow *r = &sRow[i];
  lv_obj_t *const all[] = {r->tile,  r->time,    r->freq,        r->pi,
                           r->ps,    r->country, r->countryMark, r->isNew,
                           r->level, r->count};
  for (size_t k = 0; k < sizeof(all) / sizeof(all[0]); k++) {
    uiShowIf(all[k], false);
  }
}

void screenCatchesShow(const ScreenCatches *c) {
  if (sRoot == NULL || c == NULL) {
    return;
  }
  const Theme *t = themeCurrent();

  char fit[40];
  const char *position = uiFitHeaderText(
      c->position, CATCH_W,
      (int16_t)(UI_MARGIN + uiTextWidth(sFrame.title, &roboto_title) + UI_GAP),
      fit, sizeof(fit));
  uiFrameShow(&sFrame, txt(STR_DX_TITLE_CATCHES), c->range, position, c->clock,
              NULL, NULL);
  uiSetColour(sFrame.position, c->positionIsMessage ? themeCurrent()->radio
                                                    : themeCurrent()->dead);

  const bool empty = c->rows == 0;
  uiShowIf(sEmpty, empty);
  uiShowIf(sEmptyIcon, empty);
  uiShowIf(sEmptyTitle, empty);
  uiShowIf(sEmptyNote, empty);
  if (empty) {
    centre(sEmptyTitle, &roboto_text, EMPTY_TITLE_BASE);
    centre(sEmptyNote, &roboto_label, EMPTY_NOTE_BASE);
  }
  for (uint8_t i = 0; i < SCREEN_CATCH_ROWS; i++) {
    if (i < c->rows) {
      showRow(i, &c->row[i], i == c->cursor, t);
    } else {
      hideRow(i);
    }
  }
}

void screenCatchesEnd(void) {
  if (sRoot == NULL) {
    return;
  }
  uiDropRoot(&sRoot);
}
