/*
 * The amber panel: what the radio is on, and how well it hears it.
 *
 * A panel of `radio` with all its type in `ground`, like a lit display
 * window. Content sits UI_PAD in from its edge. The name line is on top; the
 * frequency and its unit are along the bottom, with the memory slot stacked
 * above the unit; the level sits against the right edge on the frequency's
 * baseline, with the modulation meter above it.
 */
#include <stdio.h>

#include "../../core/meter.h"
#include "../../core/strings.h"
#include "../draw.h"
#include "../panel.h"

/* Baselines, in rows down from the top of the panel. */
#define NAME_BASE 28
#define SLOT_BASE 52
#define FREQ_BASE 76

/*
 * The modulation meter: 14 segments of 2 by 12 on a 4 pixel pitch, ending at
 * the content's right edge and 8 rows above the cap height of the level
 * under it. The level number says how strong the signal is, so the panel
 * has no signal meter.
 */
#define LCD_METER_SEGS 14
#define LCD_METER_SEG_W 2
#define LCD_METER_SEG_H 12
#define LCD_METER_PITCH 4
#define LCD_METER_W ((LCD_METER_SEGS - 1) * LCD_METER_PITCH + LCD_METER_SEG_W)
#define LCD_METER_BOTTOM 52
#define LCD_METER_TOP (LCD_METER_BOTTOM - LCD_METER_SEG_H)
/* The name stops this far short of the meter, so a long one never runs
 * under it. */
#define LCD_NAME_TO_METER 6
/* An unlit segment is 30 % ground laid over the amber, out of 255. */
#define LCD_METER_UNLIT_GROUND 77

static PanelRect sAt;
static lv_obj_t *sName;
/* The modulation meter: its object, how many segments are lit from the left,
 * and one more lit as the peak mark, or -1 for none. */
static lv_obj_t *sMeter;
static uint8_t sMeterLit;
static int8_t sMeterPeak;
static lv_obj_t *sSlot;
static lv_obj_t *sFrequency;
static lv_obj_t *sUnit;
static lv_obj_t *sLevel;
static lv_obj_t *sLevelUnit;
/* The name as it fits the line. A station name is at most 32 characters. */
static char sNameFit[48];

static void onMeterDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const Theme *t = themeCurrent();
  const lv_color_t lit = uiColour(t->ground);
  const lv_color_t unlit = lv_color_mix(uiColour(t->ground), uiColour(t->radio),
                                        LCD_METER_UNLIT_GROUND);
  for (uint8_t i = 0; i < LCD_METER_SEGS; i++) {
    uiFillRect(layer, &area, (int16_t)(i * LCD_METER_PITCH), 0, LCD_METER_SEG_W,
               LCD_METER_SEG_H,
               (i < sMeterLit || (int)i == sMeterPeak) ? lit : unlit);
  }
}

static void begin(lv_obj_t *parent, const PanelRect *at) {
  sAt = *at;
  const Theme *t = themeCurrent();
  uiRound(parent, t->radio, (int16_t)(at->x + UI_MARGIN), at->y,
          (int16_t)(at->w - 2 * UI_MARGIN), at->h, UI_RADIUS);
  sName = uiLabel(parent, &roboto_menu, t->ground);
  sSlot = uiLabel(parent, &roboto_small, t->ground);
  sFrequency = uiLabel(parent, &roboto_freq, t->ground);
  sUnit = uiLabel(parent, &roboto_text, t->ground);
  sLevel = uiLabel(parent, &roboto_value, t->ground);
  sLevelUnit = uiLabel(parent, &roboto_label, t->ground);
  lv_label_set_text_static(sLevelUnit, txt(STR_COMMON_UNIT_DBUV));

  sMeter = lv_obj_create(parent);
  lv_obj_remove_style_all(sMeter);
  lv_obj_set_size(sMeter, LCD_METER_W, LCD_METER_SEG_H);
  lv_obj_set_pos(sMeter,
                 (int16_t)(at->x + at->w - UI_MARGIN - UI_PAD - LCD_METER_W),
                 (int16_t)(at->y + LCD_METER_TOP));
  lv_obj_add_event_cb(sMeter, onMeterDraw, LV_EVENT_DRAW_MAIN, NULL);
  sMeterLit = 0;
  sMeterPeak = -1;
}

static void show(const ScreenState *s) {
  const int16_t left = (int16_t)(sAt.x + UI_MARGIN + UI_PAD);
  const int16_t right = (int16_t)(sAt.x + sAt.w - UI_MARGIN - UI_PAD);

  /*
   * The name line: a logbook confirmation for the second and a half it
   * holds, then the station's own name, then a person's name for the
   * memory channel, and "---" when there is none, so the line never reads as
   * a panel that failed to fill.
   */
  const char *name =
      s->logConfirm != NULL && s->logConfirm[0] != '\0'     ? s->logConfirm
      : s->stationName != NULL && s->stationName[0] != '\0' ? s->stationName
      : s->memoryName != NULL && s->memoryName[0] != '\0'
          ? s->memoryName
          : txt(STR_COMMON_NO_VALUE);
  /* A station pads its eight characters with spaces, " MAGIC  ", and the
   * line starts at the content edge whatever the padding. */
  while (*name == ' ') {
    name++;
  }
  if (*name == '\0') {
    name = txt(STR_COMMON_NO_VALUE);
  }
  uiSetText(sName,
            uiFitText(name, &roboto_menu,
                      (int16_t)(right - LCD_METER_W - LCD_NAME_TO_METER - left),
                      sNameFit, sizeof(sNameFit)));
  uiBaseline(sName, &roboto_menu, left, (int16_t)(sAt.y + NAME_BASE));

  /*
   * The frequency, or the digits being typed in its place. While typing
   * there is no unit and no slot: which unit the entry lands in is not
   * known until it is finished, and the slot is left behind the moment a
   * person starts typing a new frequency.
   */
  const bool typing = s->typing != NULL && s->typing[0] != '\0';
  const bool haveFrequency =
      !typing && s->frequency != NULL && s->frequency[0] != '\0';
  uiSetOrHide(sFrequency, typing ? s->typing : s->frequency);
  uiBaseline(sFrequency, &roboto_freq, left, (int16_t)(sAt.y + FREQ_BASE));
  const int16_t stackX =
      (int16_t)(left + uiTextWidth(sFrequency, &roboto_freq) + UI_UNIT_GAP);
  const bool haveUnit = haveFrequency && s->unit != NULL;
  uiSetOrHide(sUnit, haveUnit ? s->unit : NULL);
  uiBaseline(sUnit, &roboto_text, stackX, (int16_t)(sAt.y + FREQ_BASE));
  uiSetOrHide(sSlot, haveUnit ? s->memory : NULL);
  uiBaseline(sSlot, &roboto_small, stackX, (int16_t)(sAt.y + SLOT_BASE));

  /* The level, left out when there is no reading. */
  uiShowIf(sLevel, s->signalValid);
  uiShowIf(sLevelUnit, s->signalValid);
  if (s->signalValid) {
    char text[12];
    snprintf(text, sizeof(text), "%d", (int)s->signalDbuV);
    uiSetText(sLevel, text);
    const int16_t uw = uiTextWidth(sLevelUnit, &roboto_label);
    uiBaseline(sLevelUnit, &roboto_label, (int16_t)(right - uw),
               (int16_t)(sAt.y + FREQ_BASE));
    const int16_t lw = uiTextWidth(sLevel, &roboto_value);
    uiBaseline(sLevel, &roboto_value, (int16_t)(right - uw - UI_TIGHT - lw),
               (int16_t)(sAt.y + FREQ_BASE));
  }
  /* The count core/meter.h gives: any reading above zero lights one, and
   * only a full one lights all, with the peak mark lit as the last segment
   * it reaches. Left out with no reading: an unlit meter would read as no
   * modulation, which is a reading. Redrawn only when what it shows has
   * changed. */
  const uint8_t lit = meterSegmentsLit(s->modulationPercent, LCD_METER_SEGS);
  const int8_t peak = s->modulationPeakValid && s->modulationPeakPercent > 0
                          ? (int8_t)(meterSegmentsLit(s->modulationPeakPercent,
                                                      LCD_METER_SEGS) -
                                     1)
                          : (int8_t)-1;
  uiShowIf(sMeter, s->modulationValid);
  if (s->modulationValid && (lit != sMeterLit || peak != sMeterPeak)) {
    sMeterLit = lit;
    sMeterPeak = peak;
    lv_obj_invalidate(sMeter);
  }
}

static int zones(const PanelRect *at, TouchZone *out, int max) {
  if (max < 1) {
    return 0;
  }
  out[0] = {at->x, at->y, at->w, at->h, RADIO_ZONE_PANEL};
  return 1;
}

const Panel panelLcd = {begin, show, zones};
