/* The drawing every panel shares. */
#include "draw.h"

#include <stdio.h>
#include <string.h>

lv_color_t uiColour(ThemeColour c) {
  /* The theme keeps colours the way the panel wants them. LVGL wants the three
   * parts, so this takes them back apart. Doing it here rather than holding
   * two copies means there is one place a colour is written down. */
  return lv_color_make((uint8_t)((c >> 8) & 0xF8), (uint8_t)((c >> 3) & 0xFC),
                       (uint8_t)((c << 3) & 0xF8));
}

lv_obj_t *uiLabel(lv_obj_t *parent, const lv_font_t *font, ThemeColour c) {
  lv_obj_t *l = lv_label_create(parent);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, uiColour(c), 0);
  lv_label_set_text(l, "");
  return l;
}

lv_obj_t *uiBlock(lv_obj_t *parent, ThemeColour c, int16_t x, int16_t y,
                  int16_t w, int16_t h) {
  lv_obj_t *b = lv_obj_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_size(b, w, h);
  lv_obj_set_style_bg_color(b, uiColour(c), 0);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  return b;
}

lv_obj_t *uiRound(lv_obj_t *parent, ThemeColour c, int16_t x, int16_t y,
                  int16_t w, int16_t h, int16_t radius) {
  lv_obj_t *b = uiBlock(parent, c, x, y, w, h);
  lv_obj_set_style_radius(b, radius, 0);
  return b;
}

void uiFillRect(lv_layer_t *layer, const lv_area_t *obj, int16_t x, int16_t y,
                int16_t w, int16_t h, lv_color_t c) {
  if (w <= 0 || h <= 0) {
    return;
  }
  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.bg_color = c;
  dsc.bg_opa = LV_OPA_COVER;
  lv_area_t a;
  a.x1 = (int32_t)obj->x1 + x;
  a.y1 = (int32_t)obj->y1 + y;
  a.x2 = a.x1 + w - 1;
  a.y2 = a.y1 + h - 1;
  lv_draw_rect(layer, &dsc, &a);
}

int16_t uiTextWidth(lv_obj_t *label, const lv_font_t *font) {
  lv_point_t size;
  lv_text_get_size(&size, lv_label_get_text(label), font, 0, 0, LV_COORD_MAX,
                   LV_TEXT_FLAG_NONE);
  return (int16_t)size.x;
}

static int16_t textWidth(const char *text, const lv_font_t *font) {
  lv_point_t size;
  lv_text_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  return (int16_t)size.x;
}

const char *uiFitText(const char *text, const lv_font_t *font, int16_t room,
                      char *out, size_t cap) {
  if (text == NULL || textWidth(text, font) <= room || out == NULL || cap < 4) {
    return text;
  }
  /* Shortened from the end until the start and the ellipsis fit. A name here
   * is at most a few dozen characters, so a walk down is cheap, and it only
   * runs for a string that did not fit. The ellipsis is three bytes of UTF-8
   * and the terminator one more. A cut never lands inside a character of
   * more than one byte, such as a middle dot, and the spaces before the cut
   * go, so it reads "AND THEN…" rather than "AND THEN …". */
  size_t n = strlen(text);
  if (n > cap - 4) {
    n = cap - 4;
  }
  for (;;) {
    while (n > 0 && ((uint8_t)text[n] & 0xC0) == 0x80) {
      n--;
    }
    size_t kept = n;
    while (kept > 0 && text[kept - 1] == ' ') {
      kept--;
    }
    memcpy(out, text, kept);
    memcpy(out + kept, UI_ELLIPSIS, 4);
    if (n == 0 || textWidth(out, font) <= room) {
      return out;
    }
    n--;
  }
}

const char *uiWrapAtSpaces(const char *text, const lv_font_t *font,
                           int16_t width, char *out, size_t cap) {
  if (text == NULL || out == NULL || cap == 0) {
    return text;
  }
  size_t at = 0;
  size_t lineStart = 0;
  const char *p = text;
  out[0] = '\0';
  for (;;) {
    while (*p == ' ') {
      p++;
    }
    if (*p == '\0') {
      break;
    }
    const char *word = p;
    while (*p != '\0' && *p != ' ') {
      p++;
    }
    const size_t len = (size_t)(p - word);
    /* The separator, the word and the terminator have to fit. */
    if (at + len + 2 > cap) {
      break;
    }
    if (at == lineStart) {
      memcpy(out + at, word, len);
      at += len;
      out[at] = '\0';
      continue;
    }
    /* Tried on this line first, and moved to a new one if it passes the
     * width. The word is already in place either way: only the separator
     * in front of it changes. */
    out[at] = ' ';
    memcpy(out + at + 1, word, len);
    out[at + 1 + len] = '\0';
    if (textWidth(out + lineStart, font) > width) {
      out[at] = '\n';
      lineStart = at + 1;
    }
    at += 1 + len;
  }
  return out;
}

void uiBaseline(lv_obj_t *label, const lv_font_t *font, int16_t x,
                int16_t baselineY) {
  const int16_t ascent =
      (int16_t)(lv_font_get_line_height(font) - font->base_line);
  lv_obj_set_pos(label, x, (int16_t)(baselineY - ascent));
}

void uiBaselineRight(lv_obj_t *label, const lv_font_t *font, int16_t x,
                     int16_t baselineY) {
  uiBaseline(label, font, (int16_t)(x - uiTextWidth(label, font)), baselineY);
}

int16_t uiRowTop(const lv_font_t *font, int16_t baselineY) {
  return (int16_t)(baselineY -
                   (lv_font_get_line_height(font) - font->base_line));
}

/* How far a symbol's middle sits above the baseline of the text beside it. */
#define ICON_RISE 4

int16_t uiIconTop(int16_t baselineY) {
  return (int16_t)(baselineY - ICON_RISE - UI_ICON_SIZE / 2);
}

void uiSetText(lv_obj_t *o, const char *text) {
  const char *current = lv_label_get_text(o);
  if (current != NULL && strcmp(current, text) == 0) {
    return;
  }
  lv_label_set_text(o, text);
}

void uiShowIf(lv_obj_t *o, bool on) {
  if (on) {
    lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
  }
}

void uiSetColour(lv_obj_t *o, ThemeColour c) {
  const lv_color_t want = uiColour(c);
  if (lv_color_eq(lv_obj_get_style_text_color(o, LV_PART_MAIN), want)) {
    return;
  }
  lv_obj_set_style_text_color(o, want, 0);
}

void uiSetBgColour(lv_obj_t *o, ThemeColour c) {
  const lv_color_t want = uiColour(c);
  if (lv_color_eq(lv_obj_get_style_bg_color(o, LV_PART_MAIN), want)) {
    return;
  }
  lv_obj_set_style_bg_color(o, want, 0);
}

void uiSetBorderColour(lv_obj_t *o, ThemeColour c) {
  const lv_color_t want = uiColour(c);
  if (lv_color_eq(lv_obj_get_style_border_color(o, LV_PART_MAIN), want)) {
    return;
  }
  lv_obj_set_style_border_color(o, want, 0);
}

void uiSetFont(lv_obj_t *o, const lv_font_t *font) {
  if (lv_obj_get_style_text_font(o, LV_PART_MAIN) == font) {
    return;
  }
  lv_obj_set_style_text_font(o, font, 0);
}

void uiSetTextStatic(lv_obj_t *o, const char *text) {
  const char *current = lv_label_get_text(o);
  if (current != NULL && strcmp(current, text) == 0) {
    return;
  }
  lv_label_set_text_static(o, text);
}

void uiSetOrHide(lv_obj_t *o, const char *text) {
  if (text == NULL || text[0] == '\0') {
    lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  uiSetText(o, text);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
}

void uiFrameBegin(UiFrame *f, lv_obj_t *parent, const Theme *t,
                  ThemeColour title) {
  lv_obj_update_layout(parent);
  f->w = (int16_t)lv_obj_get_width(parent);
  f->title = uiLabel(parent, &roboto_title, title);
  f->context = uiLabel(parent, &roboto_label, t->dead);
  f->position = uiLabel(parent, &roboto_label, t->dead);
  f->clock = uiLabel(parent, &roboto_small, t->measurement);
  f->hintLeft = uiLabel(parent, &roboto_label, t->dead);
  f->hintRight = uiLabel(parent, &roboto_label, t->dead);
  f->sleep = uiLabel(parent, &roboto_icons, t->dead);
  uiSetTextStatic(f->sleep, ICON_SLEEP);
}

static UiSleepMark sSleepMark = UI_SLEEP_NONE;

void uiSetSleepMark(UiSleepMark mark) {
  sSleepMark = mark;
}

UiSleepMark uiSleepMark(void) {
  return sSleepMark;
}

bool uiShowSleepMark(lv_obj_t *o) {
  const bool on = sSleepMark != UI_SLEEP_NONE;
  uiShowIf(o, on);
  if (on) {
    const Theme *t = themeCurrent();
    uiSetColour(o, sSleepMark == UI_SLEEP_SOON ? t->radio : t->dead);
  }
  return on;
}

void uiFrameShow(UiFrame *f, const char *title, const char *context,
                 const char *position, const char *clock, const char *hintLeft,
                 const char *hintRight) {
  const int16_t w = f->w;
  uiSetText(f->title, title != NULL ? title : "");
  uiBaseline(f->title, &roboto_title, UI_MARGIN, UI_HEAD_TITLE_BASE);

  int16_t right = (int16_t)(w - UI_MARGIN);
  lv_obj_t *const runs[3] = {f->clock, f->position, f->context};
  const char *const words[3] = {clock, position, context};
  const lv_font_t *const faces[3] = {&roboto_small, &roboto_label,
                                     &roboto_label};
  for (int k = 0; k < 3; k++) {
    const bool on = words[k] != NULL && words[k][0] != '\0';
    uiSetOrHide(runs[k], on ? words[k] : NULL);
    if (!on) {
      continue;
    }
    const int16_t tw = uiTextWidth(runs[k], faces[k]);
    uiBaseline(runs[k], faces[k], (int16_t)(right - tw), UI_HEAD_RUN_BASE);
    right = (int16_t)(right - tw - UI_GAP);
  }
  if (uiShowSleepMark(f->sleep)) {
    lv_obj_set_pos(f->sleep, (int16_t)(right - UI_ICON_SIZE), UI_HEAD_ICON_TOP);
  }

  uiSetOrHide(f->hintLeft, hintLeft);
  uiBaseline(f->hintLeft, &roboto_label, UI_MARGIN, UI_HINT_BASE);
  uiSetOrHide(f->hintRight, hintRight);
  if (hintRight != NULL) {
    uiBaselineRight(f->hintRight, &roboto_label, (int16_t)(w - UI_MARGIN),
                    UI_HINT_BASE);
  }
}

const char *uiFitHeaderText(const char *text, int16_t w, int16_t left,
                            char *out, size_t cap) {
  const int16_t sleep =
      sSleepMark != UI_SLEEP_NONE ? (int16_t)(UI_ICON_SIZE + UI_GAP) : 0;
  return uiFitText(text, &roboto_label, (int16_t)(w - UI_MARGIN - left - sleep),
                   out, cap);
}

/* The text's baseline sits 6 rows below a row's middle, 21 down a row of
 * 30, and an icon or a swatch on the middle. */
#define ROW_BASE_BELOW_MIDDLE 6
#define SWATCHES 4
#define SWATCH 12
#define SWATCH_GAP 3
#define SWATCH_R 3
#define SWATCH_RUN (SWATCHES * SWATCH + (SWATCHES - 1) * SWATCH_GAP)
/* A swatch's outline is `dead` at nine tenths over what it frames. */
#define SWATCH_EDGE_OPA 230

static void onSwatchDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  const UiRow *r = (const UiRow *)lv_event_get_user_data(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const Theme *row = themeAt(r->themeIndex);
  const ThemeColour swatch[SWATCHES] = {row->ground, row->radio, row->broadcast,
                                        row->swatch};
  for (uint8_t k = 0; k < SWATCHES; k++) {
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.radius = SWATCH_R;
    dsc.bg_color = uiColour(swatch[k]);
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_color = uiColour(themeCurrent()->dead);
    dsc.border_width = 1;
    dsc.border_opa = SWATCH_EDGE_OPA;
    lv_area_t a;
    a.x1 = area.x1 + k * (SWATCH + SWATCH_GAP);
    a.y1 = area.y1;
    a.x2 = a.x1 + SWATCH - 1;
    a.y2 = a.y1 + SWATCH - 1;
    lv_draw_rect(layer, &dsc, &a);
  }
}

void uiRowBegin(UiRow *r, lv_obj_t *parent, const Theme *t, int16_t h,
                int16_t pitch) {
  lv_obj_update_layout(parent);
  const int16_t w = (int16_t)lv_obj_get_width(parent);
  r->w = w;
  r->h = h;
  r->pitch = pitch;
  r->tile = uiRound(parent, t->rule, UI_MARGIN, 0, (int16_t)(w - 2 * UI_MARGIN),
                    h, UI_TILE_R);
  r->name = uiLabel(parent, &roboto_text, t->measurement);
  r->value = uiLabel(parent, &roboto_small, t->radio);
  r->tick = uiLabel(parent, &roboto_icons, t->good);
  lv_label_set_text_static(r->tick, ICON_TICK);
  r->chevron = uiLabel(parent, &roboto_icons, t->dead);
  lv_label_set_text_static(r->chevron, ICON_CHEVRON);
  r->swatch = lv_obj_create(parent);
  lv_obj_remove_style_all(r->swatch);
  lv_obj_set_size(r->swatch, SWATCH_RUN, SWATCH);
  lv_obj_add_event_cb(r->swatch, onSwatchDraw, LV_EVENT_DRAW_MAIN, r);
  uiRowHide(r);
}

void uiRowHide(UiRow *r) {
  uiShowIf(r->tile, false);
  uiShowIf(r->name, false);
  uiShowIf(r->value, false);
  uiShowIf(r->tick, false);
  uiShowIf(r->chevron, false);
  uiShowIf(r->swatch, false);
}

void uiRowShow(UiRow *r, const Theme *t, const UiRowView *v) {
  const uint8_t slot = v->slot;
  const char *name = v->name;
  const char *value = v->value;
  const bool cursor = v->cursor;
  const bool dimValue = v->dimValue;
  const bool opens = v->opens;
  const bool saved = v->saved;
  const int swatchTheme = v->swatch ? (int)v->swatchTheme : -1;
  const int16_t w = r->w;
  const int16_t top = (int16_t)(UI_ROW_TOP + slot * r->pitch);
  const int16_t middle = (int16_t)(top + r->h / 2);
  const int16_t base = (int16_t)(middle + ROW_BASE_BELOW_MIDDLE);
  const int16_t left = (int16_t)(UI_MARGIN + UI_PAD);
  const int16_t iconTop =
      (int16_t)(middle - UI_ICON_SIZE / 2 + UI_ICON_INK_DROP);
  int16_t right = (int16_t)(w - UI_MARGIN - UI_PAD);

  uiShowIf(r->tile, true);
  lv_obj_set_y(r->tile, top);
  uiSetBgColour(r->tile, cursor ? t->radio : t->rule);

  uiShowIf(r->chevron, opens);
  if (opens) {
    uiSetColour(r->chevron, cursor ? t->ground : t->dead);
    lv_obj_set_pos(r->chevron, (int16_t)(right - UI_ICON_SIZE), iconTop);
    right = (int16_t)(right - UI_ICON_SIZE - UI_TIGHT);
  }
  /* The swatches keep the right edge on every row, and the tick of the saved
   * theme sits to their left. */
  uiShowIf(r->swatch, swatchTheme >= 0);
  if (swatchTheme >= 0) {
    r->themeIndex = (uint8_t)swatchTheme;
    lv_obj_set_pos(r->swatch, (int16_t)(right - SWATCH_RUN),
                   (int16_t)(middle - SWATCH / 2));
    lv_obj_invalidate(r->swatch);
    right = (int16_t)(right - SWATCH_RUN - UI_GAP);
  }
  uiShowIf(r->tick, saved);
  if (saved) {
    uiSetColour(r->tick, cursor ? t->ground : t->good);
    lv_obj_set_pos(r->tick, (int16_t)(right - UI_ICON_SIZE), iconTop);
    right = (int16_t)(right - UI_ICON_SIZE - UI_GAP);
  }
  const bool hasValue = value != NULL && value[0] != '\0';
  uiSetOrHide(r->value, hasValue ? value : NULL);
  if (hasValue) {
    uiSetColour(r->value, cursor ? t->ground : dimValue ? t->dead : t->radio);
    const int16_t vw = uiTextWidth(r->value, &roboto_small);
    uiBaseline(r->value, &roboto_small, (int16_t)(right - vw), base);
    right = (int16_t)(right - vw - UI_GAP);
  }

  uiShowIf(r->name, true);
  uiSetText(r->name,
            uiFitText(name != NULL ? name : "", &roboto_text,
                      (int16_t)(right - left), r->fit, sizeof(r->fit)));
  uiSetColour(r->name, cursor ? t->ground : t->measurement);
  uiBaseline(r->name, &roboto_text, left, base);
}

/* Back from LVGL's colour to the theme's five, six and five bits. */
static ThemeColour themeColourOf(lv_color_t c) {
  return (ThemeColour)(((c.red & 0xF8) << 8) | ((c.green & 0xFC) << 3) |
                       (c.blue >> 3));
}

static void roleOf(ThemeColour c, char *out, size_t cap) {
  const Theme *t = themeCurrent();
  const struct {
    ThemeColour colour;
    const char *name;
  } kRoles[] = {{t->radio, "radio"},
                {t->broadcast, "broadcast"},
                {t->measurement, "measurement"},
                {t->good, "good"},
                {t->fault, "fault"},
                {t->dead, "dead"},
                {t->ground, "ground"},
                {t->header, "header"},
                {t->rule, "rule"}};
  size_t at = 0;
  out[0] = '\0';
  for (size_t i = 0; i < sizeof(kRoles) / sizeof(kRoles[0]); i++) {
    if (kRoles[i].colour == c) {
      at += (size_t)snprintf(out + at, cap - at, "%s%s", at > 0 ? "|" : "",
                             kRoles[i].name);
      if (at >= cap) {
        return;
      }
    }
  }
}

static const char *fontName(const lv_font_t *f) {
  static const struct {
    const lv_font_t *font;
    const char *name;
  } kFonts[] = {{&roboto_freq, "freq"},   {&roboto_name, "name"},
                {&roboto_value, "value"}, {&roboto_menu, "menu"},
                {&roboto_title, "title"}, {&roboto_text, "text"},
                {&roboto_small, "small"}, {&roboto_label, "label"},
                {&roboto_icons, "icons"}};
  for (size_t i = 0; i < sizeof(kFonts) / sizeof(kFonts[0]); i++) {
    if (kFonts[i].font == f) {
      return kFonts[i].name;
    }
  }
  return "";
}

static void readBox(lv_obj_t *o, UiBoxSink sink, void *ctx) {
  const bool filled = lv_obj_get_style_bg_opa(o, LV_PART_MAIN) > LV_OPA_TRANSP;
  const int32_t width = lv_obj_get_style_border_width(o, LV_PART_MAIN);
  const bool bordered =
      width > 0 &&
      lv_obj_get_style_border_opa(o, LV_PART_MAIN) > LV_OPA_TRANSP &&
      lv_obj_get_style_border_side(o, LV_PART_MAIN) != LV_BORDER_SIDE_NONE;
  if (!filled && !bordered) {
    return;
  }
  lv_area_t a;
  lv_obj_get_coords(o, &a);
  char fillRole[64] = "";
  char borderRole[64] = "";
  UiBox b;
  b.x = (int16_t)a.x1;
  b.y = (int16_t)a.y1;
  b.w = (int16_t)lv_area_get_width(&a);
  b.h = (int16_t)lv_area_get_height(&a);
  b.filled = filled;
  b.fill =
      filled ? themeColourOf(lv_obj_get_style_bg_color(o, LV_PART_MAIN)) : 0;
  if (filled) {
    roleOf(b.fill, fillRole, sizeof(fillRole));
  }
  b.fillRole = fillRole;
  b.border = bordered ? (uint8_t)(width < 255 ? width : 255) : 0;
  b.borderColour =
      bordered ? themeColourOf(lv_obj_get_style_border_color(o, LV_PART_MAIN))
               : 0;
  if (bordered) {
    roleOf(b.borderColour, borderRole, sizeof(borderRole));
  }
  b.borderRole = borderRole;
  sink(ctx, &b);
}

static void readPanel(lv_obj_t *o, UiTextSink texts, UiBoxSink boxes,
                      void *ctx) {
  if (o == NULL || lv_obj_has_flag(o, LV_OBJ_FLAG_HIDDEN)) {
    return;
  }
  if (lv_obj_check_type(o, &lv_label_class)) {
    const char *text = lv_label_get_text(o);
    if (texts != NULL && text != NULL && text[0] != '\0') {
      lv_area_t a;
      lv_obj_get_coords(o, &a);
      char role[64];
      char stars[8];
      UiText t;
      t.text = text;
      if (lv_obj_has_flag(o, UI_FLAG_SECRET)) {
        size_t n = strlen(text);
        n = n < sizeof(stars) - 1 ? n : sizeof(stars) - 1;
        memset(stars, '*', n);
        stars[n] = '\0';
        t.text = stars;
      }
      t.x = (int16_t)a.x1;
      t.y = (int16_t)a.y1;
      t.w = (int16_t)lv_area_get_width(&a);
      t.h = (int16_t)lv_area_get_height(&a);
      t.colour = themeColourOf(lv_obj_get_style_text_color(o, LV_PART_MAIN));
      roleOf(t.colour, role, sizeof(role));
      t.role = role;
      t.font = fontName(lv_obj_get_style_text_font(o, LV_PART_MAIN));
      texts(ctx, &t);
    }
  }
  /* A label can carry a fill or a border of its own, so it is read as a
   * box too. */
  if (boxes != NULL) {
    readBox(o, boxes, ctx);
  }
  const uint32_t n = lv_obj_get_child_count(o);
  for (uint32_t i = 0; i < n; i++) {
    readPanel(lv_obj_get_child(o, (int32_t)i), texts, boxes, ctx);
  }
}

void uiReadPanel(UiTextSink texts, UiBoxSink boxes, void *ctx) {
  readPanel(lv_screen_active(), texts, boxes, ctx);
  readPanel(lv_layer_top(), texts, boxes, ctx);
}
