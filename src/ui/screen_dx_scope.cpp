/*
 * The DX Scope page.
 *
 * The latest level sweep as a bar for each channel, with a bright tick at
 * the baseline, a dim tick at the peak hold and a dashed line at the noise
 * floor. Under it a strip of the rise over the baseline, in `radio` up and
 * `dead` down. A bright cursor through both, a `good` mark under the dial's
 * channel,
 * and at the foot a tile with the cursor channel's frequency, level and
 * named rise, on the bottom margin: the page has no line of hints, so the
 * level chart takes the height.
 *
 * It draws what it is given, like every screen here, and owns the panel on
 * its own, because the LVGL pool holds one screen at a time.
 */
#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>

#include "../core/strings.h"
#include "draw.h"
#include "fonts.h"
#include "screen.h"
#include "theme.h"

#define SCOPE_W 320
#define SCOPE_H 240
#define BOX_X UI_MARGIN
#define BOX_W (SCOPE_W - 2 * UI_MARGIN)
/* The bars sit this far inside their box. */
#define INSET 4

/* The level chart, the rise strip under it, and the labels under both. */
#define CHART_Y 32
#define CHART_H 116
#define STRIP_Y 152
#define STRIP_H 30
#define AXIS_BASE 192
/* The baseline and floor labels, inside the chart's top corners. */
#define CORNER_IN 8
#define CORNER_BASE 48
#define MIDDLE_BASE 96
/* The top of the chart is kept for those labels: no bar or cursor reaches
 * into it, so nothing runs through their words. */
#define LABEL_BAND 18

/* The level scale: from 3 dB under the noise floor, 64 dB tall, so the
 * floor is a thin band and a station stands clear of it. With no floor,
 * from -10 dBuV. The rise strip holds 10 dB each way. */
#define UNDER_FLOOR_TENTHS 30
#define SCALE_TENTHS 640
#define NO_FLOOR_BOTTOM_TENTHS (-100)
#define RISE_RANGE_TENTHS 100
#define FLOOR_DASH 2
#define FLOOR_DASH_PITCH 4
#define DIAL_MARK_W 5
#define DIAL_MARK_H 3

/* The tile at the foot, ending at y 228 on the bottom margin. */
#define TILE_Y 198
#define TILE_H 30
#define TILE_BASE 219
#define SMALL_UNIT_GAP 3
/* With Touch On the foot row is five blocks: a button for the cursor to the
 * left, the frequency, Sweep in the middle of the screen, the level, and a
 * button for the cursor to the right. Each button 38 wide, the narrowest
 * target measured to hold a tap, the two tiles sharing what is left, all 4
 * apart and the tile's height. */
#define BUTTON_W 38
#define FOOT_GAP 4
#define BUTTONS 3
#define FOOT_TILE_W ((BOX_W - BUTTONS * BUTTON_W - 4 * FOOT_GAP) / 2)
#define FREQ_TILE_X (UI_MARGIN + BUTTON_W + FOOT_GAP)
#define SWEEP_X (FREQ_TILE_X + FOOT_TILE_W + FOOT_GAP)
#define LEVEL_TILE_X (SWEEP_X + BUTTON_W + FOOT_GAP)
/* Left, right and Sweep, in the order of the buttons' objects. */
static const int16_t kButtonX[BUTTONS] = {
    UI_MARGIN, LEVEL_TILE_X + FOOT_TILE_W + FOOT_GAP, SWEEP_X};

/* The page's objects and what the drawing callbacks draw from, on the
 * heap while the page is up: static RAM has no room left.
 * The arrays `shown` points at are the caller's, kept until its next show. */
typedef struct {
  lv_obj_t *root;
  UiFrame frame;
  lv_obj_t *chart;
  lv_obj_t *strip;
  lv_obj_t *from;
  lv_obj_t *mid;
  lv_obj_t *to;
  lv_obj_t *base;
  lv_obj_t *floor;
  lv_obj_t *middle;
  lv_obj_t *tile;
  lv_obj_t *freq;
  lv_obj_t *freqUnit;
  lv_obj_t *level;
  lv_obj_t *levelUnit;
  lv_obj_t *rise;
  lv_obj_t *riseUnit;
  lv_obj_t *riseWord;
  lv_obj_t *button[BUTTONS];
  lv_obj_t *buttonIcon[BUTTONS];
  lv_obj_t *bandRise;  /* The rise, up in the chart, with the buttons. */
  lv_obj_t *levelTile; /* The level's own tile, with the buttons. */
  char bandRiseText[24];
  ScreenScope shown;
  int16_t bottom;
} ScopeUi;
static ScopeUi *sUi;

static int16_t levelHeight(int16_t tenths, int16_t inner) {
  int32_t v = (int32_t)tenths - sUi->bottom;
  if (v < 0) {
    v = 0;
  }
  if (v > SCALE_TENTHS) {
    v = SCALE_TENTHS;
  }
  return (int16_t)(v * inner / SCALE_TENTHS);
}

/* Where channel `i` of `n` starts across `inner` pixels. */
static int16_t columnX(uint16_t i, uint16_t n, int16_t inner) {
  return (int16_t)(INSET + (int32_t)i * inner / n);
}

static int16_t columnW(uint16_t i, uint16_t n, int16_t inner) {
  const int16_t w =
      (int16_t)(columnX((uint16_t)(i + 1), n, inner) - columnX(i, n, inner));
  return w > 0 ? w : 1;
}

static void drawBox(lv_layer_t *layer, const lv_area_t *a, const Theme *t) {
  lv_draw_rect_dsc_t d;
  lv_draw_rect_dsc_init(&d);
  d.radius = UI_TILE_R;
  d.bg_color = uiColour(t->rule);
  d.bg_opa = LV_OPA_COVER;
  lv_draw_rect(layer, &d, a);
}

/* The dial's mark along the foot and the cursor from `top` to the foot. */
static void drawMarks(lv_layer_t *layer, const lv_area_t *a, const Theme *t,
                      int16_t inner, int16_t h, int16_t top) {
  const ScreenScope *s = &sUi->shown;
  if (s->dial < s->count) {
    const int16_t x = columnX(s->dial, s->count, inner);
    uiFillRect(layer, a, (int16_t)(x - DIAL_MARK_W / 2),
               (int16_t)(h - DIAL_MARK_H), DIAL_MARK_W, DIAL_MARK_H,
               uiColour(t->good));
  }
  if (s->cursor < s->count) {
    uiFillRect(layer, a, columnX(s->cursor, s->count, inner), top, 1,
               (int16_t)(h - 1 - top), uiColour(t->measurement));
  }
}

static void onChartDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t a;
  lv_obj_get_coords(obj, &a);
  const Theme *t = themeCurrent();
  const ScreenScope *s = &sUi->shown;
  const int16_t h = (int16_t)(a.y2 - a.y1 + 1);
  const int16_t inner = (int16_t)(a.x2 - a.x1 + 1 - 2 * INSET);
  const int16_t tall = (int16_t)(h - 2 * INSET - LABEL_BAND);
  const int16_t bottom = (int16_t)(h - INSET);
  drawBox(layer, &a, t);
  if (s->count == 0 || s->level == NULL) {
    return;
  }
  if (s->floor != SCREEN_SCOPE_NONE) {
    const int16_t y = (int16_t)(bottom - levelHeight(s->floor, tall));
    for (int16_t x = INSET; x < INSET + inner; x += FLOOR_DASH_PITCH) {
      uiFillRect(layer, &a, x, y, FLOOR_DASH, 1, uiColour(t->dead));
    }
  }
  const lv_color_t bar = uiColour(s->sweeping ? t->dead : t->radio);
  for (uint16_t i = 0; i < s->count; i++) {
    const int16_t x = columnX(i, s->count, inner);
    const int16_t w = columnW(i, s->count, inner);
    if (s->peak != NULL && s->peak[i] != SCREEN_SCOPE_NONE) {
      const int16_t ph = levelHeight(s->peak[i], tall);
      uiFillRect(layer, &a, x, (int16_t)(bottom - ph), w, 1, uiColour(t->dead));
    }
    if (s->level[i] != SCREEN_SCOPE_NONE) {
      const int16_t lh = levelHeight(s->level[i], tall);
      uiFillRect(layer, &a, x, (int16_t)(bottom - lh), w, lh, bar);
    }
    if (s->base != NULL && s->base[i] != SCREEN_SCOPE_NONE) {
      const int16_t bh = levelHeight(s->base[i], tall);
      uiFillRect(layer, &a, x, (int16_t)(bottom - bh), w, 1,
                 uiColour(t->measurement));
    }
  }
  drawMarks(layer, &a, t, inner, h, (int16_t)(INSET + LABEL_BAND));
}

static void onStripDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t a;
  lv_obj_get_coords(obj, &a);
  const Theme *t = themeCurrent();
  const ScreenScope *s = &sUi->shown;
  const int16_t h = (int16_t)(a.y2 - a.y1 + 1);
  const int16_t inner = (int16_t)(a.x2 - a.x1 + 1 - 2 * INSET);
  const int16_t half = (int16_t)((h - 2 * INSET) / 2);
  const int16_t zero = (int16_t)(INSET + half);
  drawBox(layer, &a, t);
  if (s->count == 0 || s->level == NULL) {
    return;
  }
  uiFillRect(layer, &a, INSET, zero, inner, 1, uiColour(t->dead));
  if (s->base != NULL) {
    const lv_color_t up = uiColour(s->sweeping ? t->dead : t->radio);
    for (uint16_t i = 0; i < s->count; i++) {
      if (s->level[i] == SCREEN_SCOPE_NONE || s->base[i] == SCREEN_SCOPE_NONE) {
        continue;
      }
      int32_t px =
          ((int32_t)s->level[i] - s->base[i]) * half / RISE_RANGE_TENTHS;
      if (px > half) {
        px = half;
      }
      if (px < -half) {
        px = -half;
      }
      const int16_t x = columnX(i, s->count, inner);
      const int16_t w = columnW(i, s->count, inner);
      if (px > 0) {
        uiFillRect(layer, &a, x, (int16_t)(zero - px), w, (int16_t)px, up);
      } else if (px < 0) {
        uiFillRect(layer, &a, x, (int16_t)(zero + 1), w, (int16_t)-px,
                   uiColour(t->dead));
      }
    }
  }
  drawMarks(layer, &a, t, inner, h, 1);
}

static lv_obj_t *drawnBox(int16_t y, int16_t h, lv_event_cb_t draw) {
  lv_obj_t *o = lv_obj_create(sUi->root);
  lv_obj_remove_style_all(o);
  lv_obj_set_pos(o, BOX_X, y);
  lv_obj_set_size(o, BOX_W, h);
  lv_obj_add_event_cb(o, draw, LV_EVENT_DRAW_MAIN, NULL);
  return o;
}

bool screenScopeBegin(void) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sUi != NULL) {
    return true;
  }
  sUi = (ScopeUi *)calloc(1, sizeof(*sUi));
  if (sUi == NULL) {
    return false;
  }
  const Theme *t = themeCurrent();
  uiScreenRoot(root, t->ground);

  sUi->bottom = NO_FLOOR_BOTTOM_TENTHS;
  sUi->root = uiBlock(root, t->ground, 0, 0, SCOPE_W, SCOPE_H);
  uiFrameBegin(&sUi->frame, sUi->root, t, t->radio);
  /* Set now, so the header's room is known on the first show. */
  uiSetText(sUi->frame.title, txt(STR_DX_TITLE_SCOPE));
  sUi->chart = drawnBox(CHART_Y, CHART_H, onChartDraw);
  sUi->strip = drawnBox(STRIP_Y, STRIP_H, onStripDraw);
  sUi->from = uiLabel(sUi->root, &roboto_label, t->dead);
  sUi->mid = uiLabel(sUi->root, &roboto_label, t->dead);
  sUi->to = uiLabel(sUi->root, &roboto_label, t->dead);
  sUi->base = uiLabel(sUi->root, &roboto_label, t->measurement);
  sUi->floor = uiLabel(sUi->root, &roboto_label, t->dead);
  sUi->middle = uiLabel(sUi->root, &roboto_small, t->radio);

  sUi->tile =
      uiRound(sUi->root, t->rule, UI_MARGIN, TILE_Y, BOX_W, TILE_H, UI_TILE_R);
  /* Before the texts, so they are drawn over it. */
  sUi->levelTile = uiRound(sUi->root, t->rule, LEVEL_TILE_X, TILE_Y,
                           FOOT_TILE_W, TILE_H, UI_TILE_R);
  uiShowIf(sUi->levelTile, false);
  sUi->freq = uiLabel(sUi->root, &roboto_text, t->radio);
  sUi->freqUnit = uiLabel(sUi->root, &roboto_label, t->dead);
  uiSetTextStatic(sUi->freqUnit, txt(STR_COMMON_UNIT_MHZ));
  sUi->level = uiLabel(sUi->root, &roboto_text, t->measurement);
  sUi->levelUnit = uiLabel(sUi->root, &roboto_label, t->dead);
  uiSetTextStatic(sUi->levelUnit, txt(STR_COMMON_UNIT_DBUV));
  sUi->rise = uiLabel(sUi->root, &roboto_text, t->radio);
  sUi->riseUnit = uiLabel(sUi->root, &roboto_label, t->dead);
  uiSetTextStatic(sUi->riseUnit, txt(STR_COMMON_UNIT_DB));
  sUi->riseWord = uiLabel(sUi->root, &roboto_label, t->dead);
  uiSetTextStatic(sUi->riseWord, txt(STR_DX_RISE));
  static const char *const kIcons[BUTTONS] = {ICON_CHEVRON_LEFT, ICON_CHEVRON,
                                              ICON_PLAY};
  for (uint8_t i = 0; i < BUTTONS; i++) {
    sUi->button[i] = uiRound(sUi->root, t->rule, kButtonX[i], TILE_Y, BUTTON_W,
                             TILE_H, UI_TILE_R);
    sUi->buttonIcon[i] = uiLabel(sUi->root, &roboto_icons, t->measurement);
    uiSetTextStatic(sUi->buttonIcon[i], kIcons[i]);
    lv_obj_set_pos(
        sUi->buttonIcon[i],
        (int16_t)(kButtonX[i] + (BUTTON_W - UI_ICON_SIZE) / 2),
        (int16_t)(TILE_Y + (TILE_H - UI_ICON_SIZE) / 2 + UI_ICON_INK_DROP));
  }
  sUi->bandRise = uiLabel(sUi->root, &roboto_label, t->radio);
  /* The touch blocks hidden until a show asks for them, so a page never
   * shown has none of them. */
  for (uint8_t i = 0; i < BUTTONS; i++) {
    uiShowIf(sUi->button[i], false);
    uiShowIf(sUi->buttonIcon[i], false);
  }
  uiShowIf(sUi->bandRise, false);
  return true;
}

static void showLabels(const ScreenScope *s) {
  uiSetOrHide(sUi->from, s->from);
  if (s->from != NULL) {
    uiBaseline(sUi->from, &roboto_label, UI_MARGIN, AXIS_BASE);
  }
  uiSetOrHide(sUi->mid, s->mid);
  if (s->mid != NULL) {
    const int16_t w = uiTextWidth(sUi->mid, &roboto_label);
    uiBaseline(sUi->mid, &roboto_label, (int16_t)(SCOPE_W / 2 - w / 2),
               AXIS_BASE);
  }
  uiSetOrHide(sUi->to, s->to);
  if (s->to != NULL) {
    uiBaselineRight(sUi->to, &roboto_label, SCOPE_W - UI_MARGIN, AXIS_BASE);
  }
  uiSetOrHide(sUi->base, s->baseText);
  if (s->baseText != NULL) {
    uiBaseline(sUi->base, &roboto_label, BOX_X + CORNER_IN, CORNER_BASE);
  }
  uiSetOrHide(sUi->floor, s->floorText);
  if (s->floorText != NULL) {
    uiBaselineRight(sUi->floor, &roboto_label, BOX_X + BOX_W - CORNER_IN,
                    CORNER_BASE);
  }
  const char *middle = s->sweeping ? txt(STR_DX_SWEEPING) : s->empty;
  uiSetOrHide(sUi->middle, middle);
  if (middle != NULL) {
    const int16_t w = uiTextWidth(sUi->middle, &roboto_small);
    /* During a sweep the old bars are still drawn, so the word goes up
     * among the labels. */
    uiBaseline(sUi->middle, &roboto_small, (int16_t)(SCOPE_W / 2 - w / 2),
               s->sweeping ? CORNER_BASE : MIDDLE_BASE);
  }
}

/* The three buttons, shown with Touch On, and grey while a sweep runs: the
 * glass is not read then, so the knob moves the cursor and its press stops
 * the sweep. */
static void showButtons(const ScreenScope *s, const Theme *t) {
  for (uint8_t i = 0; i < BUTTONS; i++) {
    uiShowIf(sUi->button[i], s->buttons);
    uiShowIf(sUi->buttonIcon[i], s->buttons);
    if (s->buttons) {
      uiSetColour(sUi->buttonIcon[i], s->sweeping ? t->dead : t->measurement);
    }
  }
}

/* With the buttons the rise goes up into the chart's top band, between its
 * two corner labels, but for while a sweep runs, when its word is there. */
static void showBandRise(const ScreenScope *s, const Theme *t) {
  const bool on = s->buttons && s->cursorRise != NULL && !s->sweeping;
  uiShowIf(sUi->bandRise, on);
  if (!on) {
    return;
  }
  snprintf(sUi->bandRiseText, sizeof(sUi->bandRiseText), "%s %s %s",
           txt(STR_DX_RISE), s->cursorRise, txt(STR_COMMON_UNIT_DB));
  uiSetText(sUi->bandRise, sUi->bandRiseText);
  uiSetColour(sUi->bandRise, s->riseUp ? t->radio : t->dead);
  const int16_t w = uiTextWidth(sUi->bandRise, &roboto_label);
  uiBaseline(sUi->bandRise, &roboto_label, (int16_t)(SCOPE_W / 2 - w / 2),
             CORNER_BASE);
}

static void showTile(const ScreenScope *s, const Theme *t) {
  /* With the buttons the frequency and the level each have a tile of their
   * own, the frequency without the unit the axis under the chart already
   * gives, each centred in its tile, and the rise goes up into the chart. */
  const int16_t tileX = s->buttons ? FREQ_TILE_X : UI_MARGIN;
  const int16_t tileW = s->buttons ? FOOT_TILE_W : BOX_W;
  lv_obj_set_pos(sUi->tile, tileX, TILE_Y);
  lv_obj_set_size(sUi->tile, tileW, TILE_H);
  uiShowIf(sUi->levelTile, s->buttons);
  const int16_t in = (int16_t)(tileX + UI_PAD);
  const int16_t right = (int16_t)(tileX + tileW - UI_PAD);
  uiSetOrHide(sUi->freq, s->cursorFreq);
  uiShowIf(sUi->freqUnit, s->cursorFreq != NULL && !s->buttons);
  if (s->cursorFreq != NULL) {
    const int16_t fw = uiTextWidth(sUi->freq, &roboto_text);
    uiBaseline(sUi->freq, &roboto_text,
               s->buttons ? (int16_t)(tileX + (tileW - fw) / 2) : in,
               TILE_BASE);
    uiBaseline(sUi->freqUnit, &roboto_label,
               (int16_t)(in + fw + SMALL_UNIT_GAP), TILE_BASE);
  }
  uiSetOrHide(sUi->level, s->cursorLevel);
  uiShowIf(sUi->levelUnit, s->cursorLevel != NULL);
  if (s->cursorLevel != NULL) {
    /* The level's pair in the middle of the tile, or of its own tile. */
    const int16_t lw = uiTextWidth(sUi->level, &roboto_text);
    const int16_t uw = uiTextWidth(sUi->levelUnit, &roboto_label);
    const int16_t pair = (int16_t)(lw + SMALL_UNIT_GAP + uw);
    const int16_t x = s->buttons
                          ? (int16_t)(LEVEL_TILE_X + (FOOT_TILE_W - pair) / 2)
                          : (int16_t)(SCOPE_W / 2 - pair / 2);
    uiBaseline(sUi->level, &roboto_text, x, TILE_BASE);
    uiBaseline(sUi->levelUnit, &roboto_label,
               (int16_t)(x + lw + SMALL_UNIT_GAP), TILE_BASE);
  }
  const char *rise = s->buttons ? NULL : s->cursorRise;
  uiSetOrHide(sUi->rise, rise);
  uiShowIf(sUi->riseUnit, rise != NULL);
  uiShowIf(sUi->riseWord, rise != NULL);
  if (rise != NULL) {
    uiSetColour(sUi->rise, s->riseUp ? t->radio : t->dead);
    uiBaselineRight(sUi->riseUnit, &roboto_label, right, TILE_BASE);
    const int16_t uw = uiTextWidth(sUi->riseUnit, &roboto_label);
    uiBaselineRight(sUi->rise, &roboto_text,
                    (int16_t)(right - uw - SMALL_UNIT_GAP), TILE_BASE);
    /* The word says what the number is: how far the channel stands over
     * the base line's tick. */
    const int16_t rw = uiTextWidth(sUi->rise, &roboto_text);
    uiBaselineRight(sUi->riseWord, &roboto_label,
                    (int16_t)(right - uw - SMALL_UNIT_GAP - rw - UI_TIGHT),
                    TILE_BASE);
  }
}

void screenScopeShow(const ScreenScope *s) {
  if (sUi == NULL || s == NULL) {
    return;
  }
  const Theme *t = themeCurrent();
  /* The bars are drawn again only when something they show has moved, not
   * on every poll. */
  const bool redraw =
      s->revision != sUi->shown.revision || s->count != sUi->shown.count ||
      s->cursor != sUi->shown.cursor || s->dial != sUi->shown.dial ||
      s->sweeping != sUi->shown.sweeping || s->floor != sUi->shown.floor ||
      (s->base == NULL) != (sUi->shown.base == NULL) ||
      (s->peak == NULL) != (sUi->shown.peak == NULL);
  sUi->shown = *s;
  sUi->bottom = s->floor != SCREEN_SCOPE_NONE
                    ? (int16_t)(s->floor - UNDER_FLOOR_TENTHS)
                    : (int16_t)NO_FLOOR_BOTTOM_TENTHS;
  /* No next page mark after a moment's message. */
  uiFrameMarks(&sUi->frame, true, !s->positionIsMessage);
  char fit[40];
  const char *position = uiFitHeaderText(
      s->position, SCOPE_W, (int16_t)(uiFrameTitleEnd(&sUi->frame) + UI_GAP),
      fit, sizeof(fit));
  uiFrameShow(&sUi->frame, txt(STR_DX_TITLE_SCOPE), s->context, position,
              s->clock, NULL, NULL);
  uiSetColour(sUi->frame.position, s->positionIsMessage ? themeCurrent()->radio
                                                        : themeCurrent()->dead);
  if (redraw) {
    lv_obj_invalidate(sUi->chart);
    lv_obj_invalidate(sUi->strip);
  }
  showLabels(s);
  showTile(s, t);
  showButtons(s, t);
  showBandRise(s, t);
}

void screenScopeEnd(void) {
  if (sUi == NULL) {
    return;
  }
  uiDropRoot(&sUi->root);
  free(sUi);
  sUi = NULL;
}

int screenScopeZones(TouchZone *out, int max) {
  if (sUi == NULL || max < 4 || screenDxZones(out, max, false) < 3) {
    return 0;
  }
  /* The chart with its strip and axis down to two rows above the tile, and
   * the tile to the foot. */
  const int16_t top = out[2].y;
  const int16_t foot = (int16_t)(TILE_Y - 2);
  const int16_t footH = (int16_t)(SCOPE_H - foot);
  out[2] = {0, top, SCOPE_W, (int16_t)(foot - top), DX_ZONE_CHART};
  if (!sUi->shown.buttons || max < 8) {
    out[3] = {0, foot, SCOPE_W, footH, DX_ZONE_FOOT};
    return 4;
  }
  /* The foot row's five, meeting halfway across the gaps between them; a
   * tap on either tile tunes, as the one tile does without the buttons. */
  const int16_t half = FOOT_GAP / 2;
  const int16_t freqL = (int16_t)(FREQ_TILE_X - half);
  const int16_t sweepL = (int16_t)(SWEEP_X - half);
  const int16_t levelL = (int16_t)(LEVEL_TILE_X - half);
  const int16_t rightL = (int16_t)(kButtonX[1] - half);
  out[3] = {0, foot, freqL, footH, DX_ZONE_LEFT};
  out[4] = {freqL, foot, (int16_t)(sweepL - freqL), footH, DX_ZONE_FOOT};
  out[5] = {sweepL, foot, (int16_t)(levelL - sweepL), footH, DX_ZONE_SWEEP};
  out[6] = {levelL, foot, (int16_t)(rightL - levelL), footH, DX_ZONE_FOOT};
  out[7] = {rightL, foot, (int16_t)(SCOPE_W - rightL), footH, DX_ZONE_RIGHT};
  return 8;
}

int32_t screenScopeChannelAt(int16_t x) {
  if (sUi == NULL || sUi->shown.count == 0) {
    return -1;
  }
  /* The other way round from where columnX starts a channel: the last one
   * starting at or before the pixel under the finger. */
  const int32_t n = sUi->shown.count;
  const int32_t inner = BOX_W - 2 * INSET;
  const int32_t at = (int32_t)x - BOX_X - INSET;
  const int32_t i = at < 0 ? 0 : ((at + 1) * n - 1) / inner;
  return i < n ? i : n - 1;
}
