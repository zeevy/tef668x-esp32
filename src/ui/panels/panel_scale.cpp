/*
 * The tuning scale.
 *
 * A sliding scale, from `core/scale.c`, the way the ATS radios draw theirs:
 * the tuned frequency stays in the middle and the scale moves under it as
 * the radio tunes. A mark every 100 kHz on FM and every 10 kHz on AM, 8
 * pixels apart, long and numbered every tenth, medium every fifth. The marks
 * stand on one row and the numbers sit under them, so a peak never reaches
 * a number.
 *
 * The middle is one mark in `radio`, 2 by 32, drawn whatever the signal, so the
 * dial always shows. On a station the four marks either side of it stand up
 * to a peak as tall as the signal, fading from `radio` to the scale's own
 * colour outwards.
 * Between stations the row stays flat.
 *
 * The scale is a loop, as the tuning is: past 108 MHz it goes on at 87.5.
 * Between the two ends sit four dim marks with no numbers, and a dotted line
 * in their middle marks the seam.
 *
 * One object draws everything in its own draw event, the same pattern the
 * modulation meter uses, so forty marks cost one object in the LVGL pool. The
 * object keeps UI_MARGIN from both sides, and nothing is drawn outside it.
 */
#include "../../core/scale.h"
#include "../draw.h"
#include "../panel.h"

#include <stdio.h>

/* Rows down the object. The marks stand on MARK_BASE, so their lowest row
 * is MARK_BASE - 1; the middle mark and the seam run up from there to
 * MARK_TOP. */
#define MARK_TOP 8
#define MARK_BASE 40
#define NUMBER_BASE 52
/* How tall each kind of mark is. */
#define MARK_LONG_H 16
#define MARK_MEDIUM_H 11
#define MARK_SHORT_H 5
/* The seam: one row lit, one dark. */
#define SEAM_ON 2
#define SEAM_OFF 2

static PanelRect sAt;
static lv_obj_t *sScale;
static int16_t sAvail;
/* What the draw event draws, worked out in `show()`: an LVGL draw callback
 * gets the object and the layer, nothing of its own. */
static ScaleTick sTicks[SCALE_MAX_TICKS];
static int sTickCount;
static int16_t sSeamX[SCALE_MAX_SEAMS];
static int sSeamCount;
/* What the marks were last worked out for. `begin()` clears `sDone`, since a
 * row built again may not be the same width. */
static bool sDone;
static uint32_t sLowKHz;
static uint32_t sSpanKHz;
static uint32_t sTunedKHz;
static bool sFm;
/* How tall the peak stands, 0 to 100, and 0 with no reading. */
static uint8_t sLevel;

static int16_t numberWidth(const char *text) {
  int32_t w = 0;
  for (const char *p = text; *p != '\0'; p++) {
    w += lv_font_get_glyph_width(&roboto_label, (uint32_t)p[0], (uint32_t)p[1]);
  }
  return (int16_t)w;
}

/*
 * A long mark's number, centred on `cx`. Left out when it would cross the
 * edge of the row: a number cut in half reads as a different number.
 */
static void drawNumber(lv_layer_t *layer, const lv_area_t *obj, int16_t cx,
                       uint32_t number, lv_color_t c) {
  char text[12];
  snprintf(text, sizeof(text), "%lu", (unsigned long)number);
  const int16_t w = numberWidth(text);
  if (cx - w / 2 < 0 || cx + (w + 1) / 2 > sAvail) {
    return;
  }
  lv_draw_label_dsc_t dsc;
  lv_draw_label_dsc_init(&dsc);
  dsc.font = &roboto_label;
  dsc.color = c;
  dsc.text = text;
  /* The text is on this stack frame, so LVGL keeps its own copy. */
  dsc.text_local = 1;
  dsc.align = LV_TEXT_ALIGN_CENTER;
  lv_area_t a;
  a.x1 = obj->x1 + cx - w;
  a.x2 = obj->x1 + cx + w;
  a.y1 = obj->y1 + uiRowTop(&roboto_label, NUMBER_BASE);
  a.y2 = a.y1 + roboto_label.line_height - 1;
  lv_draw_label(layer, &dsc, &a);
}

static void onScaleDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const Theme *t = themeCurrent();
  const lv_color_t scale = uiColour(t->scale);
  const lv_color_t radio = uiColour(t->radio);
  const int16_t middle = (int16_t)(sAvail / 2);
  /* The whole row, cleared first, or the old marks would stay behind as the
   * scale moves. */
  uiFillRect(layer, &area, 0, 0, sAvail, (int16_t)(area.y2 - area.y1 + 1),
             uiColour(t->ground));
  for (int i = 0; i < sTickCount; i++) {
    const ScaleTick *k = &sTicks[i];
    if (k->x < 0 || k->x > sAvail - 1) {
      continue;
    }
    const int32_t dx = k->x - middle;
    if (!k->inBand) {
      /* A dummy at the seam: short, in rule, and never part of the peak,
       * since it is not a frequency. */
      uiFillRect(layer, &area, k->x, (int16_t)(MARK_BASE - MARK_SHORT_H), 1,
                 MARK_SHORT_H, uiColour(t->rule));
      continue;
    }
    /* 0 is a number too big to hold, left blank, core/scale.h. */
    if (k->mark == SCALE_LONG && k->number != 0) {
      drawNumber(layer, &area, k->x, k->number, scale);
    }
    if (scaleUnderMiddle(dx)) {
      continue;
    }
    int16_t h = k->mark == SCALE_LONG     ? (int16_t)MARK_LONG_H
                : k->mark == SCALE_MEDIUM ? (int16_t)MARK_MEDIUM_H
                                          : (int16_t)MARK_SHORT_H;
    lv_color_t c = scale;
    const int rank = scalePeakRank(dx);
    const int16_t peak = scalePeakPx(rank, sLevel);
    if (peak > 0) {
      if (peak > h) {
        h = peak;
      }
      /* Amber at the nearest mark, a quarter of the way to the scale's own
       * colour at each mark outwards. */
      c = lv_color_mix(
          radio, scale,
          (uint8_t)(255 * (SCALE_PEAK_RANKS - rank) / SCALE_PEAK_RANKS));
    }
    uiFillRect(layer, &area, k->x, (int16_t)(MARK_BASE - h), 1, h, c);
  }
  /* The seam stays in `dead`: it is a boundary, and one as bright as the
   * pointer beside it reads as a second pointer. */
  for (int i = 0; i < sSeamCount; i++) {
    for (int16_t y = MARK_TOP; y < MARK_BASE;
         y = (int16_t)(y + SEAM_ON + SEAM_OFF)) {
      uiFillRect(layer, &area, sSeamX[i], y, 1, SEAM_ON, uiColour(t->dead));
    }
  }
  if (sTickCount > 0) {
    uiFillRect(layer, &area, (int16_t)(middle - 1), MARK_TOP, 2,
               (int16_t)(MARK_BASE - MARK_TOP), radio);
  }
}

static void begin(lv_obj_t *parent, const PanelRect *at) {
  sAt = *at;
  sAvail = (int16_t)(sAt.w - 2 * UI_MARGIN);
  sDone = false;
  sScale = lv_obj_create(parent);
  lv_obj_remove_style_all(sScale);
  lv_obj_set_pos(sScale, (int16_t)(sAt.x + UI_MARGIN), sAt.y);
  lv_obj_set_size(sScale, sAvail, sAt.h);
  lv_obj_add_event_cb(sScale, onScaleDraw, LV_EVENT_DRAW_MAIN, NULL);
}

static void show(const ScreenState *s) {
  const uint8_t level = s->signalValid ? s->signalPercent : 0;
  const bool newLevel = level != sLevel;
  sLevel = level;
  bool moved = false;
  /* The marks are worked out again only when the band or the frequency
   * moves. A span of 0 is a band plan that could not be read: no scale at
   * all, rather than one against numbers the radio does not have. */
  if (!sDone || s->sweepLowKHz != sLowKHz || s->sweepSpanKHz != sSpanKHz ||
      s->sweepKHz != sTunedKHz || s->fm != sFm) {
    sDone = true;
    sLowKHz = s->sweepLowKHz;
    sSpanKHz = s->sweepSpanKHz;
    sTunedKHz = s->sweepKHz;
    sFm = s->fm;
    const uint32_t highKHz = sLowKHz + sSpanKHz;
    sTickCount = sSpanKHz == 0 ? 0
                               : scaleTicks(sLowKHz, highKHz, sTunedKHz, sFm,
                                            sAvail, sTicks, SCALE_MAX_TICKS);
    sSeamCount = sSpanKHz == 0 ? 0
                               : scaleSeams(sLowKHz, highKHz, sTunedKHz, sFm,
                                            sAvail, sSeamX, SCALE_MAX_SEAMS);
    moved = true;
  }
  if (moved) {
    lv_obj_invalidate(sScale);
  } else if (newLevel) {
    /* Only the peak's marks follow the signal, so only they are drawn
     * again: the reach of the peak either side of the middle, from the top
     * of the tallest peak down to the row the marks stand on. The whole
     * scale drawn for a new level makes that refresh about 50 ms against 15,
     * which holds the scrolling text still for two or three steps. */
    lv_area_t a;
    lv_obj_get_coords(sScale, &a);
    const int32_t middle = a.x1 + sAvail / 2;
    const int32_t reach = scalePeakReachPx();
    const lv_area_t peak = {middle - reach,
                            a.y1 + MARK_BASE - SCALE_PEAK_MAX_PX,
                            middle + reach, a.y1 + MARK_BASE - 1};
    lv_obj_invalidate_area(sScale, &peak);
  }
}

static int zones(const PanelRect *at, TouchZone *out, int max) {
  if (max < 1) {
    return 0;
  }
  out[0] = {at->x, at->y, at->w, at->h, RADIO_ZONE_SCALE};
  return 1;
}

const Panel panelScale = {begin, show, zones};
