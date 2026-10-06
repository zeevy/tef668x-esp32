/*
 * The screen: pick a layout, build it, and hand it the state.
 *
 * There is no drawing here. A screen is a table of panels and this walks the
 * table. Which table it walks comes from the file that knows the size of the
 * display; there is one, and every band uses it.
 */
#include "screen.h"

#include <ctype.h>
#include <lvgl.h>
#include <stdio.h>

#include "../core/strings.h"
#include "draw.h"
#include "panel.h"

static bool sReady = false;
static lv_obj_t *sRoot;
static lv_obj_t *sVeil;
/* A message: a title and a detail, centred. */
static lv_obj_t *sMessage;
static lv_obj_t *sDetail;
static char sDetailText[96];

/*
 * The firmware write, in the shape of the menu's value editor:
 * the amber panel with the caption and the percentage, the bar under it and
 * the warning under that. A separate set of objects from the message, so a
 * fault and a percentage never fight over one label's font and place, and
 * both share `sVeil` as the same opaque background.
 */
static lv_obj_t *sVeilPanel;
static lv_obj_t *sVeilCaption;
static lv_obj_t *sVeilPercent;
static lv_obj_t *sVeilPercentSign;
static lv_obj_t *sVeilBar;
static lv_obj_t *sVeilFoot;
static int16_t sVeilFill;

#define MESSAGE_TITLE_BASE 128
#define MESSAGE_DETAIL_BASE 152
#define VEIL_PANEL_Y 32
#define VEIL_PANEL_H 100
#define VEIL_CAPTION_BASE 56
#define VEIL_PERCENT_BASE 112
#define VEIL_BAR_Y 148
#define VEIL_BAR_H 12
#define VEIL_BAR_R 6
#define VEIL_FOOT_BASE 184

/* The track and the rounded fill up to `sVeilFill`. */
static void onVeilBarDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const Theme *t = themeCurrent();
  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.radius = VEIL_BAR_R;
  dsc.bg_opa = LV_OPA_COVER;
  dsc.bg_color = uiColour(t->rule);
  lv_draw_rect(layer, &dsc, &area);
  if (sVeilFill > 0) {
    lv_area_t a = area;
    a.x2 = a.x1 + sVeilFill - 1;
    dsc.bg_color = uiColour(t->radio);
    lv_draw_rect(layer, &dsc, &a);
  }
}

/*
 * Which layout is built right now, or NULL when none is.
 *
 * There is one table. The panels are torn down and built again only when the
 * theme changes, which does not happen in a loop.
 */
static const Layout *sBuilt;

/* Everything the radio screen is made of, in one container. */
static lv_obj_t *sPage;

/*
 * Which theme generation the built panels were drawn in. See
 * themeGeneration: a plain pointer to themeCurrent()'s result cannot see a
 * colour wheel edit to the theme that is already active, since Custom is
 * always the same struct address and only its fields change.
 *
 * A colour is set once at construction on some panels and never again in
 * their own show(), which is fine while there is one theme and wrong the
 * moment there are two: nothing would repaint them. Rebuilding is the answer:
 * changing the theme repaints the screen and touches nothing else, and it
 * reuses buildLayout rather than auditing every panel for a colour it only
 * reads once.
 */
static uint32_t sBuiltThemeGen;

static void buildLayout(const Layout *layout) {
  /* The veil and the message are built after the panels so that they cover
   * them, so they go too and come back at the end. */
  lv_obj_clean(sRoot);
  sPage = uiBlock(sRoot, themeCurrent()->ground, 0, 0,
                  (int16_t)lv_obj_get_width(sRoot),
                  (int16_t)lv_obj_get_height(sRoot));
  for (uint8_t i = 0; i < layout->count; i++) {
    layout->placements[i].panel->begin(sPage, &layout->placements[i].at);
  }

  /*
   * Across the middle, for when there is nothing else to say.
   *
   * The veil is what makes it a message rather than an overprint. A fault is
   * shown while the rest of the screen is still being written every 40 ms, so
   * without something opaque behind it the words land on top of the frequency
   * and the meter and both become unreadable.
   */
  const Theme *t = themeCurrent();
  const int16_t w = (int16_t)lv_obj_get_width(sRoot);
  const int16_t h = (int16_t)lv_obj_get_height(sRoot);
  sVeil = uiBlock(sPage, t->ground, 0, 0, w, h);
  lv_obj_add_flag(sVeil, LV_OBJ_FLAG_HIDDEN);
  sMessage = uiLabel(sPage, &roboto_menu, t->measurement);
  lv_obj_add_flag(sMessage, LV_OBJ_FLAG_HIDDEN);
  sDetail = uiLabel(sPage, &roboto_text, t->dead);
  lv_obj_add_flag(sDetail, LV_OBJ_FLAG_HIDDEN);

  sVeilPanel = uiRound(sPage, t->radio, UI_MARGIN, VEIL_PANEL_Y,
                       (int16_t)(w - 2 * UI_MARGIN), VEIL_PANEL_H, UI_RADIUS);
  lv_obj_add_flag(sVeilPanel, LV_OBJ_FLAG_HIDDEN);
  sVeilCaption = uiLabel(sPage, &roboto_small, t->ground);
  uiSetTextStatic(sVeilCaption, txt(STR_RADIO_VEIL_UPDATING));
  lv_obj_add_flag(sVeilCaption, LV_OBJ_FLAG_HIDDEN);
  sVeilPercent = uiLabel(sPage, &roboto_freq, t->ground);
  lv_obj_add_flag(sVeilPercent, LV_OBJ_FLAG_HIDDEN);
  sVeilPercentSign = uiLabel(sPage, &roboto_menu, t->ground);
  uiSetTextStatic(sVeilPercentSign, txt(STR_COMMON_UNIT_PERCENT));
  lv_obj_add_flag(sVeilPercentSign, LV_OBJ_FLAG_HIDDEN);
  sVeilBar = lv_obj_create(sPage);
  lv_obj_remove_style_all(sVeilBar);
  lv_obj_set_pos(sVeilBar, UI_MARGIN, VEIL_BAR_Y);
  lv_obj_set_size(sVeilBar, (int16_t)(w - 2 * UI_MARGIN), VEIL_BAR_H);
  lv_obj_add_event_cb(sVeilBar, onVeilBarDraw, LV_EVENT_DRAW_MAIN, NULL);
  lv_obj_add_flag(sVeilBar, LV_OBJ_FLAG_HIDDEN);
  sVeilFoot = uiLabel(sPage, &roboto_small, t->measurement);
  uiSetTextStatic(sVeilFoot, txt(STR_RADIO_VEIL_KEEP_POWER));
  lv_obj_add_flag(sVeilFoot, LV_OBJ_FLAG_HIDDEN);

  sBuilt = layout;
  sBuiltThemeGen = themeGeneration();
}

bool screenBegin(void) {
  if (sReady) {
    return true;
  }
  sRoot = lv_screen_active();
  if (sRoot == NULL) {
    return false;
  }
  uiScreenRoot(sRoot, themeCurrent()->ground);

  buildLayout(layoutFor());
  sReady = true;
  return true;
}

int screenRadioZones(TouchZone *out, int max) {
  const Layout *layout = layoutFor();
  int n = 0;
  for (uint8_t i = 0; i < layout->count && n < max; i++) {
    const PanelPlacement *p = &layout->placements[i];
    if (p->panel->zones != NULL) {
      n += p->panel->zones(&p->at, out + n, max - n);
    }
  }
  return n;
}

const char *screenRadioZoneName(int id) {
  static const char *const kNames[] = {"",     "band", "menu", "panel", "scale",
                                       "mode", "sql",  "bw",   "vol"};
  return id > 0 && id < (int)(sizeof(kNames) / sizeof(kNames[0])) ? kNames[id]
                                                                  : "";
}

void screenEnd(void) {
  if (!sReady) {
    return;
  }
  /*
   * Deleted, not hidden, and that is forced rather than chosen.
   *
   * Hiding it would make opening the menu a flag instead of a rebuild, but
   * the LVGL pool is a static array in DRAM and there is no room to grow it
   * enough for two screens. LV_MEM_SIZE in lv_conf.h gives the numbers.
   */
  lv_obj_clean(sRoot);
  sPage = NULL;
  sVeil = NULL;
  sMessage = NULL;
  sDetail = NULL;
  sVeilPanel = NULL;
  sVeilCaption = NULL;
  sVeilPercent = NULL;
  sVeilPercentSign = NULL;
  sVeilBar = NULL;
  sVeilFoot = NULL;
  sBuilt = NULL;
  sBuiltThemeGen = 0;
  sReady = false;
}

void screenShow(const ScreenState *state) {
  if (!sReady || state == NULL) {
    return;
  }
  lv_obj_add_flag(sMessage, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(sDetail, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(sVeil, LV_OBJ_FLAG_HIDDEN);

  const Layout *want = layoutFor();
  if (want != sBuilt || themeGeneration() != sBuiltThemeGen) {
    buildLayout(want);
  }
  for (uint8_t i = 0; i < sBuilt->count; i++) {
    sBuilt->placements[i].panel->show(state);
  }

  /* A fault is the one thing that pushes everything else aside. */
  if (!state->tunerReady && state->fault != NULL) {
    screenMessage(txt(STR_COMMON_TUNER), state->fault);
  }
}

/* Show or hide the firmware write's own set of objects. */
static void showVeilParts(bool on) {
  lv_obj_t *const parts[] = {sVeilPanel,       sVeilCaption, sVeilPercent,
                             sVeilPercentSign, sVeilBar,     sVeilFoot};
  for (size_t k = 0; k < sizeof(parts) / sizeof(parts[0]); k++) {
    uiShowIf(parts[k], on);
  }
}

/* A line centred across the screen on its baseline. */
static void centre(lv_obj_t *o, const lv_font_t *font, int16_t base) {
  const int16_t w = (int16_t)lv_obj_get_width(sRoot);
  uiBaseline(o, font, (int16_t)((w - uiTextWidth(o, font)) / 2), base);
}

void screenMessage(const char *line1, const char *line2) {
  if (!sReady) {
    return;
  }
  /* The title in 21 pixels and the detail under it in 17, both centred,
   * and the detail starting with a capital however it was written. */
  uiSetText(sMessage, line1 != NULL ? line1 : "");
  centre(sMessage, &roboto_menu, MESSAGE_TITLE_BASE);
  const bool hasDetail = line2 != NULL && line2[0] != '\0';
  if (hasDetail) {
    snprintf(sDetailText, sizeof(sDetailText), "%s", line2);
    sDetailText[0] = (char)toupper((unsigned char)sDetailText[0]);
    uiSetText(sDetail, sDetailText);
    centre(sDetail, &roboto_text, MESSAGE_DETAIL_BASE);
  }
  lv_obj_clear_flag(sVeil, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(sMessage, LV_OBJ_FLAG_HIDDEN);
  uiShowIf(sDetail, hasDetail);
  /* The percentage shape is the other thing this veil can show. If a write
   * failed or finished while it was up, this is what takes it back down. */
  showVeilParts(false);
}

void screenUpdateVeilShow(int percent) {
  if (!sReady) {
    return;
  }
  /* Below zero, the size of the write is not known yet: no number and an
   * empty bar, rather than a figure nobody measured. */
  const bool known = percent >= 0;
  if (!known) {
    percent = 0;
  } else if (percent > 100) {
    percent = 100;
  }
  const int16_t w = (int16_t)lv_obj_get_width(sRoot);
  const int16_t barW = (int16_t)(w - 2 * UI_MARGIN);
  int16_t fill = (int16_t)((int32_t)barW * percent / 100);
  if (fill > 0 && fill < VEIL_BAR_H) {
    fill = VEIL_BAR_H;
  }
  if (fill != sVeilFill) {
    sVeilFill = fill;
    lv_obj_invalidate(sVeilBar);
  }

  char digits[8];
  snprintf(digits, sizeof(digits), "%d", percent);
  uiSetText(sVeilPercent, digits);
  const int16_t digitsW = uiTextWidth(sVeilPercent, &roboto_freq);
  const int16_t signW = uiTextWidth(sVeilPercentSign, &roboto_menu);
  const int16_t left = (int16_t)((w - digitsW - UI_UNIT_GAP - signW) / 2);
  uiBaseline(sVeilPercent, &roboto_freq, left, VEIL_PERCENT_BASE);
  uiBaseline(sVeilPercentSign, &roboto_menu,
             (int16_t)(left + digitsW + UI_UNIT_GAP), VEIL_PERCENT_BASE);
  centre(sVeilCaption, &roboto_small, VEIL_CAPTION_BASE);
  centre(sVeilFoot, &roboto_small, VEIL_FOOT_BASE);

  lv_obj_clear_flag(sVeil, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(sMessage, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(sDetail, LV_OBJ_FLAG_HIDDEN);
  showVeilParts(true);
  uiShowIf(sVeilPercent, known);
  uiShowIf(sVeilPercentSign, known);
}
