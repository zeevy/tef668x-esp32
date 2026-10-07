/*
 * The header: the band on the left, with the metre band beside it on SW, and
 * on the right one run of, from the right, the menu symbol while Touch is On,
 * the clock, Wi-Fi, the battery and the sleep mark. No speaker: the V: tile
 * says when the radio is muted.
 *
 * No fill. Every item is centred on one line, 14 rows down: the band name
 * sits on baseline 21 and the rest on baseline 20, one row lower so that the
 * larger face looks centred. The run is laid out from the right edge inwards, 8
 * apart, and an item that is missing is left out so the rest close up to the
 * edge. The battery is the shape alone as a per cent, filled to it, and as
 * volts a small upright battery, filled the same way, just before the number in
 * the clock's face.
 */
#include "../draw.h"
#include "../panel.h"

/* Where the words sit, in rows down from the top of the band. */
#define BAND_BASE 21
#define RUN_BASE 20
/* The line every item is centred on. */
#define CENTRE_Y 14

/* The drawn battery: a 24 by 12 outline and a 2 by 6 nub on its end. */
#define BATT_W 24
#define BATT_H 12
#define BATT_NUB_W 2
#define BATT_NUB_H 6
#define BATT_FILL_MAX (BATT_W - 4)
/* Beside the volts, the battery stands up: a 7 by 11 outline and a 3 by 2
 * nub on top, filled from the bottom. */
#define VBATT_W 7
#define VBATT_H 11
#define VBATT_NUB_W 3
#define VBATT_NUB_H 2
#define VBATT_FILL_MAX (VBATT_H - 4)
/* From the symbol to the number, closer than a label to its value, so the
 * two read as one item. */
#define VBATT_GAP 2
/* A fifth of a lithium cell is about half an hour of listening, which is
 * enough notice to find a cable. */
#define BATT_LOW_PERCENT 20

static PanelRect sAt;
static lv_obj_t *sBand;
static lv_obj_t *sMeterBand;
static lv_obj_t *sSleep;
static lv_obj_t *sWifi;
static lv_obj_t *sWifiBars;
static lv_obj_t *sBattText;
static lv_obj_t *sBattBody;
static lv_obj_t *sBattFill;
static lv_obj_t *sBattNub;
static lv_obj_t *sClock;
static lv_obj_t *sMenu;

/* Top of an icon's box, so its ink is centred on the header's line. */
static int16_t iconTop(void) {
  return (int16_t)(sAt.y + CENTRE_Y - UI_ICON_SIZE / 2 + UI_ICON_INK_DROP);
}

static void begin(lv_obj_t *parent, const PanelRect *at) {
  sAt = *at;
  const Theme *t = themeCurrent();
  sBand = uiLabel(parent, &roboto_title, t->radio);
  sMeterBand = uiLabel(parent, &roboto_small, t->dead);
  sSleep = uiLabel(parent, &roboto_icons, t->dead);
  uiSetTextStatic(sSleep, ICON_SLEEP);
  sWifi = uiLabel(parent, &roboto_icons, t->dead);
  /* Drawn after sWifi, so it sits on top: the lit arcs over the outline. */
  sWifiBars = uiLabel(parent, &roboto_icons, t->good);
  sBattText = uiLabel(parent, &roboto_small, t->dead);
  sBattBody = lv_obj_create(parent);
  lv_obj_remove_style_all(sBattBody);
  lv_obj_set_size(sBattBody, BATT_W, BATT_H);
  lv_obj_set_style_radius(sBattBody, 2, 0);
  lv_obj_set_style_border_width(sBattBody, 1, 0);
  lv_obj_set_style_border_color(sBattBody, uiColour(t->dead), 0);
  sBattFill = uiRound(parent, t->good, 0, 0, 0, BATT_H - 4, 1);
  sBattNub = uiBlock(parent, t->dead, 0, 0, BATT_NUB_W, BATT_NUB_H);
  sClock = uiLabel(parent, &roboto_small, t->measurement);
  sMenu = uiLabel(parent, &roboto_icons, t->dead);
  uiSetTextStatic(sMenu, ICON_MENU);
}

/* A word in the run, ending at `*right`, which then moves past it. */
static void placeWord(lv_obj_t *o, const lv_font_t *font, int16_t *right,
                      int16_t gap) {
  uiShowIf(o, true);
  const int16_t w = uiTextWidth(o, font);
  uiBaseline(o, font, (int16_t)(*right - w), (int16_t)(sAt.y + RUN_BASE));
  *right = (int16_t)(*right - w - gap);
}

static void placeIcon(lv_obj_t *o, int16_t *right, int16_t gap) {
  lv_obj_set_pos(o, (int16_t)(*right - UI_ICON_SIZE), iconTop());
  *right = (int16_t)(*right - UI_ICON_SIZE - gap);
}

static void show(const ScreenState *s) {
  const Theme *t = themeCurrent();

  uiSetOrHide(sBand, s->band);
  uiBaseline(sBand, &roboto_title, (int16_t)(sAt.x + UI_MARGIN),
             (int16_t)(sAt.y + BAND_BASE));
  /* In the clock's face and grey, so it reads as a note on the band rather
   * than as a second band name. */
  uiSetOrHide(sMeterBand, s->meterBand);
  if (s->meterBand != NULL) {
    uiBaseline(sMeterBand, &roboto_small,
               (int16_t)(sAt.x + UI_MARGIN + uiTextWidth(sBand, &roboto_title) +
                         UI_GAP),
               (int16_t)(sAt.y + RUN_BASE));
  }

  int16_t right = (int16_t)(sAt.x + sAt.w - UI_MARGIN);

  /* The menu symbol takes the corner while a finger can use it, deep in the
   * menu's half, as far from the band name's half as it can be. */
  uiShowIf(sMenu, s->menuMark);
  if (s->menuMark) {
    placeIcon(sMenu, &right, UI_GAP);
  }

  /* The clock holds the corner, or sits next to the menu symbol. Absent
   * until a server has answered, and the run closes up over it. */
  if (s->clock != NULL && s->clock[0] != '\0') {
    uiSetText(sClock, s->clock);
    placeWord(sClock, &roboto_small, &right, UI_GAP);
  } else {
    uiShowIf(sClock, false);
  }

  /*
   * Always there, and in its own shape for each state: a symbol that
   * disappears reads as a radio with no networking. Grey until it needs
   * attention. Joined is the outline in grey with the lit arcs over it in
   * good, so a weak link loses its bars and never its shape. The access
   * point is in radio, the one state where a person has something to do.
   */
  const bool joined = s->wifi == SCREEN_WIFI_JOINED;
  uiSetTextStatic(sWifi, joined                          ? ICON_WIFI
                         : s->wifi == SCREEN_WIFI_TRYING ? ICON_WIFI_FIND
                         : s->wifi == SCREEN_WIFI_AP     ? ICON_WIFI_AP
                                                         : ICON_WIFI_OFF);
  uiSetColour(sWifi, s->wifi == SCREEN_WIFI_AP ? t->radio : t->dead);
  const int16_t wifiX = (int16_t)(right - UI_ICON_SIZE);
  placeIcon(sWifi, &right, UI_GAP);
  const bool showBars = joined && s->wifiBars > 0;
  uiShowIf(sWifiBars, showBars);
  if (showBars) {
    uiSetTextStatic(sWifiBars, s->wifiBars >= 3   ? ICON_WIFI
                               : s->wifiBars == 2 ? ICON_WIFI_2_BAR
                                                  : ICON_WIFI_1_BAR);
    lv_obj_set_pos(sWifiBars, wifiX, iconTop());
  }

  /* The battery as a per cent is the shape alone, filled to it: the fill
   * already says the per cent. As volts it is a small upright battery and the
   * number after it, filled the same way. */
  const bool haveBattery = s->batteryValid;
  const bool asVolts = haveBattery && s->batteryText != NULL;
  const bool asShape = haveBattery && !asVolts;
  const uint8_t pct = !haveBattery              ? 0
                      : s->batteryPercent > 100 ? 100
                                                : s->batteryPercent;
  const bool low = haveBattery && pct <= BATT_LOW_PERCENT;
  const ThemeColour edge = low ? t->fault : t->dead;
  uiShowIf(sBattBody, haveBattery);
  uiShowIf(sBattFill, haveBattery);
  uiShowIf(sBattNub, haveBattery);
  uiShowIf(sBattText, asVolts);
  if (asVolts) {
    /* The number first from the right, then the symbol before it. */
    uiSetText(sBattText, s->batteryText);
    uiSetColour(sBattText, edge);
    placeWord(sBattText, &roboto_small, &right, VBATT_GAP);
    const int16_t bodyX = (int16_t)(right - VBATT_W);
    const int16_t bodyY = (int16_t)(sAt.y + CENTRE_Y - VBATT_H / 2);
    lv_obj_set_size(sBattBody, VBATT_W, VBATT_H);
    lv_obj_set_pos(sBattBody, bodyX, bodyY);
    uiSetBorderColour(sBattBody, edge);
    lv_obj_set_size(sBattNub, VBATT_NUB_W, VBATT_NUB_H);
    lv_obj_set_pos(sBattNub, (int16_t)(bodyX + (VBATT_W - VBATT_NUB_W) / 2),
                   (int16_t)(bodyY - VBATT_NUB_H));
    uiSetBgColour(sBattNub, edge);
    int16_t fill = (int16_t)((VBATT_FILL_MAX * pct + 50) / 100);
    if (fill < 1) {
      fill = 1;
    }
    lv_obj_set_size(sBattFill, VBATT_W - 4, fill);
    lv_obj_set_pos(sBattFill, (int16_t)(bodyX + 2),
                   (int16_t)(bodyY + 2 + VBATT_FILL_MAX - fill));
    uiSetBgColour(sBattFill, low ? t->fault : t->good);
    right = (int16_t)(bodyX - UI_GAP);
  } else if (asShape) {
    lv_obj_set_size(sBattBody, BATT_W, BATT_H);
    lv_obj_set_size(sBattNub, BATT_NUB_W, BATT_NUB_H);
    lv_obj_set_height(sBattFill, BATT_H - 4);
    const int16_t bodyX = (int16_t)(right - BATT_NUB_W - BATT_W);
    const int16_t bodyY = (int16_t)(sAt.y + CENTRE_Y - BATT_H / 2);
    lv_obj_set_pos(sBattBody, bodyX, bodyY);
    uiSetBorderColour(sBattBody, edge);
    lv_obj_set_pos(sBattNub, (int16_t)(bodyX + BATT_W),
                   (int16_t)(bodyY + (BATT_H - BATT_NUB_H) / 2));
    uiSetBgColour(sBattNub, edge);
    /* Inside the outline, with a pixel of ground between the two. At least
     * two pixels, so an empty cell still shows it has been read. */
    int16_t fill = (int16_t)((BATT_FILL_MAX * pct + 50) / 100);
    if (fill < 2) {
      fill = 2;
    }
    lv_obj_set_pos(sBattFill, (int16_t)(bodyX + 2), (int16_t)(bodyY + 2));
    lv_obj_set_width(sBattFill, fill);
    uiSetBgColour(sBattFill, low ? t->fault : t->good);
    right = (int16_t)(bodyX - UI_GAP);
  }

  /* While auto off is on: grey, and in radio for the last five minutes,
   * when a key keeps the radio awake. */
  if (uiShowSleepMark(sSleep)) {
    placeIcon(sSleep, &right, UI_GAP);
  }
}

/* The band name half, which steps the band as BAND does, and the status
 * half, which opens the menu as the knob's press does. */
static int zones(const PanelRect *at, TouchZone *out, int max) {
  if (max < 2) {
    return 0;
  }
  const int16_t half = (int16_t)(at->w / 2);
  out[0] = {at->x, at->y, half, at->h, RADIO_ZONE_BAND};
  out[1] = {(int16_t)(at->x + half), at->y, (int16_t)(at->w - half), at->h,
            RADIO_ZONE_MENU};
  return 2;
}

const Panel panelHeader = {begin, show, zones};
