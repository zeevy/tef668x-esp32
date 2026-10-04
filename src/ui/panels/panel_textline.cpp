/*
 * The line under the amber panel: what the station is saying, or the date.
 *
 * Radio text in `broadcast`, scrolling inside the margins when it is longer
 * than the line, which a station's 64 characters always are. With no radio
 * text, which is every AM band and many FM stations, the date in the same
 * colour. With neither, nothing: the date waits for an NTP answer, like the
 * clock.
 */
#include "../draw.h"
#include "../panel.h"

/* The baseline, 20 rows down, which is 8 rows under the panel above. */
#define LINE_BASE 20

static PanelRect sAt;
static lv_obj_t *sText;
static lv_obj_t *sDate;

/*
 * Keep the scrolling text's redraw to its own box.
 *
 * A label asks for a margin of a quarter of the line height on every side,
 * in case a glyph reaches past its box, and every step of the scroll then
 * redraws and pushes that margin too: 304 by 26 pixels rather than 296 by
 * 18. A scrolling label clips its text to its own box, so the margin only
 * ever holds the ground behind it. This runs before the label's own handler
 * and stops it, and the label has no shadow, outline or transform that
 * would need the margin either.
 */
static void noMargin(lv_event_t *e) {
  lv_event_stop_processing(e);
}

static void begin(lv_obj_t *parent, const PanelRect *at) {
  sAt = *at;
  const Theme *t = themeCurrent();
  sText = uiLabel(parent, &roboto_text, t->broadcast);
  lv_obj_set_width(sText, (int16_t)(at->w - 2 * UI_MARGIN));
  lv_obj_set_height(sText, (int32_t)lv_font_get_line_height(&roboto_text));
  /* Circular rather than back and forth: a station's 64 characters never
   * finish inside one screen width, so there is no natural place for a back
   * and forth scroll to turn round that would not cut a word. LVGL drives
   * the motion on its own once this is set. */
  lv_label_set_long_mode(sText, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_add_event_cb(
      sText, noMargin,
      (lv_event_code_t)(LV_EVENT_REFR_EXT_DRAW_SIZE | LV_EVENT_PREPROCESS),
      NULL);
  lv_obj_refresh_ext_draw_size(sText);
  sDate = uiLabel(parent, &roboto_small, t->broadcast);
}

static void show(const ScreenState *s) {
  const int16_t x = (int16_t)(sAt.x + UI_MARGIN);
  const int16_t base = (int16_t)(sAt.y + LINE_BASE);
  const bool haveText = s->radioText != NULL && s->radioText[0] != '\0';
  uiShowIf(sText, haveText);
  if (haveText) {
    uiSetText(sText, s->radioText);
    uiBaseline(sText, &roboto_text, x, base);
  }
  uiSetOrHide(sDate, haveText ? NULL : s->date);
  uiBaseline(sDate, &roboto_small, x, base);
}

const Panel panelTextLine = {begin, show};
