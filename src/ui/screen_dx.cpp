/*
 * The DX page, page 1 of DX mode.
 *
 * On it: the station name, a character a cell while it arrives, on the amber
 * panel with the frequency under it, the PI on its own tile beside that, six
 * readings in three columns, the last minute of signal as bars, and the four
 * RDS blocks as meters.
 *
 * It draws what it is given and works nothing out, like the RDS screen:
 * screen_dx_state.cpp decides every string. It owns the panel on its own,
 * because the LVGL pool holds one screen at a time.
 */
#include <lvgl.h>
#include <string.h>

#include "../core/dx.h"
#include "../core/strings.h"
#include "draw.h"
#include "fonts.h"
#include "screen.h"
#include "theme.h"

#define DX_W 320
#define DX_H 240
/* Nothing lower: the page has no line of hints, so the graphs reach the
 * bottom margin. */
#define DX_BOTTOM (DX_H - UI_MARGIN)

/* The amber panel, and where its content runs. */
#define PANEL_X UI_MARGIN
#define PANEL_Y 32
#define PANEL_W 208
#define PANEL_H 88
#define PANEL_IN (PANEL_X + UI_PAD)
#define PANEL_RIGHT (PANEL_X + PANEL_W - UI_PAD)

/* The name: eight cells of 11 px while its characters arrive, then one
 * text. A character not yet heard is a short bar, in the ground colour mixed
 * 45 % into the panel's. */
#define PS_BASE 56
#define PS_CELL 11
#define PS_MISS_Y 54
#define PS_MISS_W 8
#define PS_MISS_H 2
#define PS_MISS_MIX 115 /* 45 % of 255. */

#define FREQ_BASE 108

/* The PI tile. Its content runs 8 in from each side. */
#define TILE_X 228
#define TILE_W 80
#define TILE_IN (TILE_X + UI_GAP)
#define TILE_RIGHT (TILE_X + TILE_W - UI_GAP)
#define TILE_MID (TILE_X + TILE_W / 2)
#define PI_LABEL_BASE 52
#define PI_CODE_BASE 84
#define PI_DIGIT_GAP 1
#define COUNTRY_BASE 108
#define COUNTRY_ICON_GAP 2

/* The readings: three columns, two rows. */
#define READ_ROWS 2
#define READ_COLS 3
#define READ_BASE_0 140
#define READ_BASE_1 160
#define READ_VALUE_GAP 4
#define READ_UNIT_GAP 3

/* The history: 60 bars of 3 px on a 4 px pitch inside a rounded box, 4 px
 * in from its top and bottom. */
#define HIST_X UI_MARGIN
#define HIST_Y 168
#define HIST_W 240
#define HIST_H (DX_BOTTOM - HIST_Y)
#define HIST_INSET 4
#define HIST_BAR_W 3
#define HIST_PITCH 4
#define HIST_FIRST 1

/* The blocks: four columns of 13 px, each four segments of 9 by 10 on a
 * 12 px pitch from the top, the letter under them, level with the foot of
 * the history. */
#define BLOCKS_X 260
#define BLOCKS_Y 168
#define BLOCKS_W 48
#define BLOCKS_H 46
#define BLOCK_PITCH 13
#define SEG_W 9
#define SEG_H 10
#define SEG_PITCH 12
#define SEG_BOTTOM (3 * SEG_PITCH)
#define BLOCK_LETTER_BASE (DX_BOTTOM - 1)

/* The stereo mark: two rings of radius 4 drawn 2 px wide, 6 apart, their
 * middle 5 px above the header line. */
#define RING 10
#define RING_STEP 6
#define RING_RISE 5
#define RING_LINE 2
#define STEREO_W 16

static lv_obj_t *sDx;
static lv_obj_t *sTitle;
static lv_obj_t *sRing[2];
static lv_obj_t *sSleep;
static lv_obj_t *sPosition;
static lv_obj_t *sClock;

static lv_obj_t *sPanel;
static lv_obj_t *sPsChar[SCREEN_DX_PS_LEN];
static lv_obj_t *sPsMiss[SCREEN_DX_PS_LEN];
static lv_obj_t *sPty;
static lv_obj_t *sFreq;
static lv_obj_t *sUnit;

static lv_obj_t *sTile;
static lv_obj_t *sPiLabel;
static lv_obj_t *sPiMark;
static lv_obj_t *sPiDigit[4];
static lv_obj_t *sCountry;
static lv_obj_t *sCountryMark;

static lv_obj_t *sReadLabel[READ_ROWS * READ_COLS];
static lv_obj_t *sReadValue[READ_ROWS * READ_COLS];
static lv_obj_t *sReadUnit[READ_ROWS * READ_COLS];

static lv_obj_t *sHistory;
static int16_t sHistTenths[SCREEN_DX_HISTORY];
static bool sHistHave[SCREEN_DX_HISTORY];

static lv_obj_t *sBlocks;
static lv_obj_t *sBlockLetter[4];
static int8_t sBlockError[4];

static const StrId kReadNames[READ_ROWS * READ_COLS] = {
    STR_DX_LEVEL,  STR_DX_USN,    STR_DX_WAM,
    STR_DX_OFFSET, STR_COMMON_BW, STR_DX_MOD};
static const StrId kReadUnits[READ_ROWS * READ_COLS] = {
    STR_COMMON_UNIT_DBUV, STR_COMMON_UNIT_PERCENT, STR_COMMON_UNIT_PERCENT,
    STR_COMMON_UNIT_KHZ,  STR_COMMON_UNIT_KHZ,     STR_COMMON_UNIT_PERCENT};
static const int16_t kReadX[READ_COLS] = {UI_MARGIN, 124, TILE_X};

static void onHistoryDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const Theme *t = themeCurrent();

  lv_draw_rect_dsc_t box;
  lv_draw_rect_dsc_init(&box);
  box.radius = UI_TILE_R;
  box.bg_color = uiColour(t->rule);
  box.bg_opa = LV_OPA_COVER;
  lv_draw_rect(layer, &box, &area);

  const uint8_t inner = HIST_H - 2 * HIST_INSET;
  const int16_t bottom = HIST_H - HIST_INSET;
  const lv_color_t bar = uiColour(t->radio);
  for (uint8_t i = 0; i < SCREEN_DX_HISTORY; i++) {
    if (!sHistHave[i]) {
      continue;
    }
    const uint8_t h = dxHistoryBarHeight(sHistTenths[i], inner);
    uiFillRect(layer, &area, (int16_t)(HIST_FIRST + i * HIST_PITCH),
               (int16_t)(bottom - h), HIST_BAR_W, h, bar);
  }
}

static void onBlocksDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const Theme *t = themeCurrent();
  for (uint8_t b = 0; b < 4; b++) {
    const int8_t level = sBlockError[b];
    const uint8_t lit = dxBlockSegments(level);
    /* Clean and a small correction are both good; a large one is the
     * panel's amber; one that could not be put right is a fault. */
    const ThemeColour on = level <= 1   ? t->good
                           : level == 2 ? t->radio
                                        : t->fault;
    for (uint8_t k = 0; k < 4; k++) {
      uiFillRect(layer, &area, (int16_t)(b * BLOCK_PITCH),
                 (int16_t)(SEG_BOTTOM - k * SEG_PITCH), SEG_W, SEG_H,
                 uiColour(k < lit ? on : t->rule));
    }
  }
}

static lv_obj_t *ring(lv_obj_t *parent, const Theme *t) {
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_size(o, RING, RING);
  lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(o, RING_LINE, 0);
  lv_obj_set_style_border_color(o, uiColour(t->good), 0);
  lv_obj_set_style_border_opa(o, LV_OPA_COVER, 0);
  return o;
}

bool screenDxBegin(void) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sDx != NULL) {
    return true;
  }
  const Theme *t = themeCurrent();

  uiScreenRoot(root, t->ground);

  sDx = uiBlock(root, t->ground, 0, 0, DX_W, DX_H);

  sTitle = uiLabel(sDx, &roboto_title, t->radio);
  uiSetTextStatic(sTitle, txt(STR_DX_TITLE));
  uiBaseline(sTitle, &roboto_title, UI_MARGIN, UI_HEAD_TITLE_BASE);
  sRing[0] = ring(sDx, t);
  sRing[1] = ring(sDx, t);
  sSleep = uiLabel(sDx, &roboto_icons, t->dead);
  uiSetTextStatic(sSleep, ICON_SLEEP);
  sPosition = uiLabel(sDx, &roboto_label, t->dead);
  sClock = uiLabel(sDx, &roboto_small, t->measurement);

  sPanel =
      uiRound(sDx, t->radio, PANEL_X, PANEL_Y, PANEL_W, PANEL_H, UI_RADIUS);
  const lv_color_t miss =
      lv_color_mix(uiColour(t->ground), uiColour(t->radio), PS_MISS_MIX);
  for (uint8_t i = 0; i < SCREEN_DX_PS_LEN; i++) {
    sPsChar[i] = uiLabel(sDx, &roboto_menu, t->ground);
    sPsMiss[i] =
        uiBlock(sDx, t->ground,
                (int16_t)(PANEL_IN + i * PS_CELL + (PS_CELL - PS_MISS_W) / 2),
                PS_MISS_Y, PS_MISS_W, PS_MISS_H);
    lv_obj_set_style_bg_color(sPsMiss[i], miss, 0);
  }
  sPty = uiLabel(sDx, &roboto_small, t->ground);
  sFreq = uiLabel(sDx, &roboto_freq, t->ground);
  sUnit = uiLabel(sDx, &roboto_text, t->ground);

  sTile = uiRound(sDx, t->rule, TILE_X, PANEL_Y, TILE_W, PANEL_H, UI_RADIUS);
  sPiLabel = uiLabel(sDx, &roboto_label, t->dead);
  uiSetTextStatic(sPiLabel, txt(STR_COMMON_PI));
  uiBaseline(sPiLabel, &roboto_label, TILE_IN, PI_LABEL_BASE);
  sPiMark = uiLabel(sDx, &roboto_icons, t->dead);
  lv_obj_set_pos(sPiMark, TILE_RIGHT - UI_ICON_SIZE, uiIconTop(PI_LABEL_BASE));
  for (uint8_t i = 0; i < 4; i++) {
    sPiDigit[i] = uiLabel(sDx, &roboto_value, t->ground);
  }
  sCountry = uiLabel(sDx, &roboto_small, t->ground);
  sCountryMark = uiLabel(sDx, &roboto_icons, t->dead);
  uiSetTextStatic(sCountryMark, ICON_HELP);

  for (uint8_t i = 0; i < READ_ROWS * READ_COLS; i++) {
    const int16_t base = i < READ_COLS ? READ_BASE_0 : READ_BASE_1;
    sReadLabel[i] = uiLabel(sDx, &roboto_label, t->dead);
    uiSetTextStatic(sReadLabel[i], txt(kReadNames[i]));
    uiBaseline(sReadLabel[i], &roboto_label, kReadX[i % READ_COLS], base);
    sReadValue[i] = uiLabel(sDx, &roboto_text, t->measurement);
    sReadUnit[i] = uiLabel(sDx, &roboto_label, t->dead);
    uiSetTextStatic(sReadUnit[i], txt(kReadUnits[i]));
  }

  memset(sHistHave, 0, sizeof(sHistHave));
  sHistory = lv_obj_create(sDx);
  lv_obj_remove_style_all(sHistory);
  lv_obj_set_pos(sHistory, HIST_X, HIST_Y);
  lv_obj_set_size(sHistory, HIST_W, HIST_H);
  lv_obj_add_event_cb(sHistory, onHistoryDraw, LV_EVENT_DRAW_MAIN, NULL);

  for (uint8_t b = 0; b < 4; b++) {
    sBlockError[b] = -1;
  }
  sBlocks = lv_obj_create(sDx);
  lv_obj_remove_style_all(sBlocks);
  lv_obj_set_pos(sBlocks, BLOCKS_X, BLOCKS_Y);
  lv_obj_set_size(sBlocks, BLOCKS_W, BLOCKS_H);
  lv_obj_add_event_cb(sBlocks, onBlocksDraw, LV_EVENT_DRAW_MAIN, NULL);
  static const StrId kLetters[4] = {STR_DX_BLOCK_A, STR_DX_BLOCK_B,
                                    STR_DX_BLOCK_C, STR_DX_BLOCK_D};
  for (uint8_t b = 0; b < 4; b++) {
    sBlockLetter[b] = uiLabel(sDx, &roboto_label, t->dead);
    uiSetTextStatic(sBlockLetter[b], txt(kLetters[b]));
  }
  return true;
}

/* The run at the right of the header: the stereo mark, the page and the
 * clock, ending at the right margin. The page can be a moment's message,
 * which is cut to the room left of the title, the sleep mark and the stereo
 * mark's place. */
static void showHeader(const ScreenDx *dx) {
  char fit[40];
  const char *position =
      uiFitHeaderText(dx->position, DX_W,
                      (int16_t)(UI_MARGIN + uiTextWidth(sTitle, &roboto_title) +
                                UI_GAP + STEREO_W + UI_GAP),
                      fit, sizeof(fit));
  int16_t right = (int16_t)(DX_W - UI_MARGIN);
  uiSetOrHide(sClock, dx->clock);
  if (dx->clock != NULL) {
    const int16_t cw = uiTextWidth(sClock, &roboto_small);
    uiBaseline(sClock, &roboto_small, (int16_t)(right - cw), UI_HEAD_RUN_BASE);
    right = (int16_t)(right - cw - UI_GAP);
  }
  uiSetOrHide(sPosition, position);
  /* A moment's message in amber, so it is seen; the page in grey. */
  uiSetColour(sPosition, dx->positionIsMessage ? themeCurrent()->radio
                                               : themeCurrent()->dead);
  if (position != NULL) {
    const int16_t pw = uiTextWidth(sPosition, &roboto_label);
    uiBaseline(sPosition, &roboto_label, (int16_t)(right - pw),
               UI_HEAD_RUN_BASE);
    right = (int16_t)(right - pw - UI_GAP);
  }
  const int16_t left = (int16_t)(right - STEREO_W);
  const int16_t top = (int16_t)(UI_HEAD_RUN_BASE - RING_RISE - RING / 2);
  for (uint8_t k = 0; k < 2; k++) {
    uiShowIf(sRing[k], dx->stereo);
    lv_obj_set_pos(sRing[k], (int16_t)(left + k * RING_STEP), top);
  }
  uiShowSleepMark(sSleep);
  lv_obj_set_pos(sSleep, (int16_t)(left - UI_GAP - UI_ICON_SIZE),
                 UI_HEAD_ICON_TOP);
}

static void showName(const ScreenDx *dx) {
  /* The whole name in the first cell's label, or a character a cell. */
  const bool whole = dx->psWhole[0] != '\0';
  int16_t nameW = SCREEN_DX_PS_LEN * PS_CELL;
  for (uint8_t i = 0; i < SCREEN_DX_PS_LEN; i++) {
    const bool have = !whole && dx->psShown && dx->psHave[i];
    const bool missing = !whole && dx->psShown && !dx->psHave[i];
    uiShowIf(sPsMiss[i], missing);
    if (whole && i == 0) {
      uiSetOrHide(sPsChar[0], dx->psWhole);
      uiBaseline(sPsChar[0], &roboto_menu, PANEL_IN, PS_BASE);
      nameW = uiTextWidth(sPsChar[0], &roboto_menu);
      continue;
    }
    char one[2] = {have ? dx->ps[i] : ' ', '\0'};
    if (!have || one[0] == ' ') {
      uiSetOrHide(sPsChar[i], NULL);
      continue;
    }
    uiSetOrHide(sPsChar[i], one);
    const int16_t w = uiTextWidth(sPsChar[i], &roboto_menu);
    uiBaseline(sPsChar[i], &roboto_menu,
               (int16_t)(PANEL_IN + i * PS_CELL + (PS_CELL - w) / 2), PS_BASE);
  }
  /* The programme type takes what the name leaves, at the right. */
  const int16_t ptyRoom = (int16_t)(PANEL_RIGHT - PANEL_IN - nameW - UI_GAP);
  char fitted[48];
  const char *pty = dx->pty != NULL ? uiFitText(dx->pty, &roboto_small, ptyRoom,
                                                fitted, sizeof(fitted))
                                    : NULL;
  uiSetOrHide(sPty, pty);
  if (pty != NULL) {
    uiBaselineRight(sPty, &roboto_small, PANEL_RIGHT, PS_BASE);
  }
  uiSetOrHide(sFreq, dx->frequency);
  uiBaseline(sFreq, &roboto_freq, PANEL_IN, FREQ_BASE);
  uiSetOrHide(sUnit, dx->unit);
  if (dx->frequency != NULL && dx->unit != NULL) {
    const int16_t fw = uiTextWidth(sFreq, &roboto_freq);
    uiBaseline(sUnit, &roboto_text, (int16_t)(PANEL_IN + fw + UI_UNIT_GAP),
               FREQ_BASE);
  }
}

static void showPi(const ScreenDx *dx, const Theme *t) {
  /* Another station on the preset is confirmed, but not amber: the tile
   * goes grey with a red cross, so the change is in the shape as well as
   * the colour. */
  const bool on = dx->pi == SCREEN_DX_PI_CONFIRMED && !dx->piOther;
  uiSetBgColour(sTile, on ? t->radio : t->rule);
  uiSetColour(sPiLabel, on ? t->ground : t->dead);

  const char *mark = NULL;
  ThemeColour markColour = t->dead;
  switch (dx->piOther ? SCREEN_DX_PI_NONE : dx->pi) {
    case SCREEN_DX_PI_CONFIRMED:
      mark = ICON_CHECK_CIRCLE;
      markColour = t->ground;
      break;
    case SCREEN_DX_PI_PARTIAL:
      mark = ICON_HELP;
      break;
    case SCREEN_DX_PI_SEEN:
      mark = ICON_SCHEDULE;
      break;
    case SCREEN_DX_PI_ZERO:
      mark = ICON_BLOCK;
      markColour = t->fault;
      break;
    case SCREEN_DX_PI_NONE:
    default:
      break;
  }
  if (dx->piOther) {
    mark = ICON_CROSS;
    markColour = t->fault;
  }
  uiShowIf(sPiMark, mark != NULL);
  if (mark != NULL) {
    uiSetTextStatic(sPiMark, mark);
    uiSetColour(sPiMark, markColour);
  }

  /* The four digits as one run centred on the tile, each its own label so
   * a digit in doubt can take its own colour. */
  const bool digits = dx->pi != SCREEN_DX_PI_NONE && dx->piDigits[0] != '\0';
  int16_t widths[4] = {0, 0, 0, 0};
  int16_t total = 0;
  for (uint8_t i = 0; i < 4; i++) {
    char one[2] = {digits ? dx->piDigits[i] : ' ', '\0'};
    uiShowIf(sPiDigit[i], digits);
    if (!digits) {
      continue;
    }
    uiSetText(sPiDigit[i], one);
    ThemeColour c = on ? t->ground : t->radio;
    if (dx->pi == SCREEN_DX_PI_ZERO || one[0] == '?') {
      c = t->dead;
    }
    uiSetColour(sPiDigit[i], c);
    widths[i] = uiTextWidth(sPiDigit[i], &roboto_value);
    total = (int16_t)(total + widths[i] + (i > 0 ? PI_DIGIT_GAP : 0));
  }
  int16_t x = (int16_t)(TILE_MID - total / 2);
  for (uint8_t i = 0; digits && i < 4; i++) {
    uiBaseline(sPiDigit[i], &roboto_value, x, PI_CODE_BASE);
    x = (int16_t)(x + widths[i] + PI_DIGIT_GAP);
  }

  /* The country, the help mark when it is not known, or NO ID. */
  const bool zero = dx->pi == SCREEN_DX_PI_ZERO;
  const char *country = zero ? txt(STR_DX_NO_ID) : dx->country;
  /* A station's own RT+ name can be sixteen characters, wider than the tile;
   * a country code and four call letters always fit. */
  char fitCountry[16];
  if (country != NULL) {
    country = uiFitText(country, &roboto_small, TILE_RIGHT - TILE_IN,
                        fitCountry, sizeof(fitCountry));
  }
  uiSetOrHide(sCountry, country);
  uiShowIf(sCountryMark, !zero && dx->countryUnsure);
  uiSetColour(sCountry, zero || dx->piOther ? t->fault
                        : on                ? t->ground
                                            : t->measurement);
  uiSetColour(sCountryMark, on ? t->ground : t->dead);
  const int16_t cw = country != NULL ? uiTextWidth(sCountry, &roboto_small) : 0;
  const bool withMark = !zero && dx->countryUnsure;
  const int16_t run =
      (int16_t)(cw + (withMark ? (cw > 0 ? COUNTRY_ICON_GAP : 0) + UI_ICON_SIZE
                               : 0));
  const int16_t start = (int16_t)(TILE_MID - run / 2);
  if (country != NULL) {
    uiBaseline(sCountry, &roboto_small, start, COUNTRY_BASE);
  }
  if (withMark) {
    lv_obj_set_pos(sCountryMark, (int16_t)(start + run - UI_ICON_SIZE),
                   uiIconTop(COUNTRY_BASE));
  }
}

static void showReadings(const ScreenDx *dx) {
  const char *const values[READ_ROWS * READ_COLS] = {
      dx->level, dx->usn, dx->wam, dx->offset, dx->bandwidth, dx->modulation};
  for (uint8_t i = 0; i < READ_ROWS * READ_COLS; i++) {
    const int16_t base = i < READ_COLS ? READ_BASE_0 : READ_BASE_1;
    const bool have = values[i] != NULL;
    uiSetOrHide(sReadValue[i], values[i]);
    uiShowIf(sReadUnit[i], have);
    if (!have) {
      continue;
    }
    int16_t x =
        (int16_t)(kReadX[i % READ_COLS] +
                  uiTextWidth(sReadLabel[i], &roboto_label) + READ_VALUE_GAP);
    uiBaseline(sReadValue[i], &roboto_text, x, base);
    x = (int16_t)(x + uiTextWidth(sReadValue[i], &roboto_text) + READ_UNIT_GAP);
    uiBaseline(sReadUnit[i], &roboto_label, x, base);
  }
}

static void showGraphs(const ScreenDx *dx, const Theme *t) {
  if (memcmp(sHistTenths, dx->historyTenths, sizeof(sHistTenths)) != 0 ||
      memcmp(sHistHave, dx->historyHave, sizeof(sHistHave)) != 0) {
    memcpy(sHistTenths, dx->historyTenths, sizeof(sHistTenths));
    memcpy(sHistHave, dx->historyHave, sizeof(sHistHave));
    lv_obj_invalidate(sHistory);
  }
  if (memcmp(sBlockError, dx->blockError, sizeof(sBlockError)) != 0) {
    memcpy(sBlockError, dx->blockError, sizeof(sBlockError));
    lv_obj_invalidate(sBlocks);
  }
  for (uint8_t b = 0; b < 4; b++) {
    uiSetColour(sBlockLetter[b], sBlockError[b] < 0 ? t->dead : t->measurement);
    const int16_t w = uiTextWidth(sBlockLetter[b], &roboto_label);
    uiBaseline(sBlockLetter[b], &roboto_label,
               (int16_t)(BLOCKS_X + b * BLOCK_PITCH + (SEG_W - w + 1) / 2),
               BLOCK_LETTER_BASE);
  }
}

void screenDxShow(const ScreenDx *dx) {
  if (sDx == NULL || dx == NULL) {
    return;
  }
  const Theme *t = themeCurrent();
  showHeader(dx);
  showName(dx);
  showPi(dx, t);
  showReadings(dx);
  showGraphs(dx, t);
}

void screenDxEnd(void) {
  if (sDx == NULL) {
    return;
  }
  uiDropRoot(&sDx);
}
