/*
 * The DX Scanner page, the third page of DX mode.
 *
 * The scan panel says what the scan is doing: its mark and mode, the stop
 * rule, the frequency, the dwell and how much of it is left. Under it the
 * band's progress, and at the foot a tile with the station on the channel,
 * filled once the scan has stopped on it. The page has no line of hints, so
 * the tile sits on the bottom margin, where the Scope page has its own, and
 * the progress sits halfway between it and the panel.
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

#define SCAN_W 320
#define SCAN_H 240

/* The scan panel, and where its content runs. */
#define PANEL_X UI_MARGIN
#define PANEL_Y 32
#define PANEL_W (SCAN_W - 2 * UI_MARGIN)
#define PANEL_H 88
#define PANEL_IN (PANEL_X + UI_PAD)
#define PANEL_RIGHT (PANEL_X + PANEL_W - UI_PAD)
#define MODE_BASE 56
#define DWELL_BASE 84
#define FREQ_BASE 108

/* The progress bar and its labels, 24 below the panel and 26 above the
 * tile. */
#define BAR_Y 144
#define BAR_H 12
#define BAR_R 6
#define BAR_LABEL_BASE 172

/* The station tile at the foot, ending at y 228 on the bottom margin. */
#define TILE_Y 198
#define TILE_H 30
#define TILE_BASE 219
/* The name sits further from the PI than a label from its value. */
#define PS_GAP 12

/* The NEW pill beside the title, 18 past its end, wherever the title
 * starts. */
#define PILL_GAP 18
#define PILL_Y 6
#define PILL_W 36
#define PILL_H 18
#define PILL_BASE 19

/* A unit follows a small number closer than UI_TIGHT. */
#define SMALL_UNIT_GAP 3

static lv_obj_t *sRoot;
static UiFrame sFrame;
static lv_obj_t *sPill;
static lv_obj_t *sPillText;
static lv_obj_t *sPanel;
static lv_obj_t *sMark;
static lv_obj_t *sMode;
static lv_obj_t *sRule;
static lv_obj_t *sFreq;
static lv_obj_t *sUnit;
static lv_obj_t *sReady;
static lv_obj_t *sDwellLabel;
static lv_obj_t *sDwell;
static lv_obj_t *sLeft;
static lv_obj_t *sLeftUnit;
static lv_obj_t *sTrack;
static lv_obj_t *sFill;
static lv_obj_t *sFrom;
static lv_obj_t *sTo;
static lv_obj_t *sStep;
static lv_obj_t *sTile;
static lv_obj_t *sPiLabel;
static lv_obj_t *sPi;
static lv_obj_t *sPs;
static lv_obj_t *sLevel;
static lv_obj_t *sLevelUnit;
static lv_obj_t *sLevelWord;
static lv_obj_t *sNote;

bool screenScanBegin(void) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sRoot != NULL) {
    return true;
  }
  const Theme *t = themeCurrent();
  uiScreenRoot(root, t->ground);

  sRoot = uiBlock(root, t->ground, 0, 0, SCAN_W, SCAN_H);
  uiFrameBegin(&sFrame, sRoot, t, t->radio);
  /* Set now, so the header's room is known on the first show. */
  uiSetText(sFrame.title, txt(STR_DX_TITLE_SCANNER));
  sPill = uiRound(sRoot, t->good, 0, PILL_Y, PILL_W, PILL_H, PILL_H / 2);
  sPillText = uiLabel(sRoot, &roboto_label, t->ground);
  uiSetTextStatic(sPillText, txt(STR_DX_NEW));

  sPanel =
      uiRound(sRoot, t->radio, PANEL_X, PANEL_Y, PANEL_W, PANEL_H, UI_RADIUS);
  sMark = uiLabel(sRoot, &roboto_icons, t->ground);
  lv_obj_set_pos(sMark, PANEL_IN, uiIconTop(MODE_BASE));
  sMode = uiLabel(sRoot, &roboto_small, t->ground);
  sRule = uiLabel(sRoot, &roboto_small, t->ground);
  sFreq = uiLabel(sRoot, &roboto_freq, t->ground);
  sUnit = uiLabel(sRoot, &roboto_text, t->ground);
  uiSetTextStatic(sUnit, txt(STR_COMMON_UNIT_MHZ));
  sReady = uiLabel(sRoot, &roboto_menu, t->ground);
  uiSetTextStatic(sReady, txt(STR_DX_READY));
  uiBaseline(sReady, &roboto_menu, PANEL_IN, FREQ_BASE);
  sDwellLabel = uiLabel(sRoot, &roboto_label, t->ground);
  uiSetTextStatic(sDwellLabel, txt(STR_DX_DWELL));
  sDwell = uiLabel(sRoot, &roboto_small, t->ground);
  sLeft = uiLabel(sRoot, &roboto_value, t->ground);
  sLeftUnit = uiLabel(sRoot, &roboto_label, t->ground);
  uiSetTextStatic(sLeftUnit, txt(STR_COMMON_UNIT_S));

  sTrack = uiRound(sRoot, t->rule, UI_MARGIN, BAR_Y, PANEL_W, BAR_H, BAR_R);
  sFill = uiRound(sRoot, t->radio, UI_MARGIN, BAR_Y, BAR_H, BAR_H, BAR_R);
  sFrom = uiLabel(sRoot, &roboto_label, t->dead);
  sTo = uiLabel(sRoot, &roboto_label, t->dead);
  sStep = uiLabel(sRoot, &roboto_label, t->dead);

  sTile =
      uiRound(sRoot, t->rule, UI_MARGIN, TILE_Y, PANEL_W, TILE_H, UI_TILE_R);
  sPiLabel = uiLabel(sRoot, &roboto_label, t->dead);
  uiSetTextStatic(sPiLabel, txt(STR_COMMON_PI));
  sPi = uiLabel(sRoot, &roboto_text, t->radio);
  sPs = uiLabel(sRoot, &roboto_text, t->measurement);
  sLevel = uiLabel(sRoot, &roboto_text, t->measurement);
  sLevelUnit = uiLabel(sRoot, &roboto_label, t->dead);
  uiSetTextStatic(sLevelUnit, txt(STR_COMMON_UNIT_DBUV));
  sLevelWord = uiLabel(sRoot, &roboto_label, t->dead);
  uiSetTextStatic(sLevelWord, txt(STR_DX_LEVEL));
  sNote = uiLabel(sRoot, &roboto_small, t->dead);
  return true;
}

/* A value with its label or unit, the pair ending at `right`. */
static void rightPair(lv_obj_t *first, const lv_font_t *firstFont,
                      lv_obj_t *second, const lv_font_t *secondFont,
                      int16_t gap, int16_t right, int16_t base) {
  uiBaselineRight(second, secondFont, right, base);
  const int16_t sw = uiTextWidth(second, secondFont);
  uiBaselineRight(first, firstFont, (int16_t)(right - sw - gap), base);
}

static void showPanel(const ScreenScan *s) {
  /* A mark while the scan walks or holds on a station. A ready scanner has
   * none, because "Ready" in place of the frequency already says it. */
  const char *mark = s->state == SCREEN_SCAN_RUNNING   ? ICON_PLAY
                     : s->state == SCREEN_SCAN_STOPPED ? ICON_PAUSE
                                                       : NULL;
  uiSetOrHide(sMark, mark);
  uiSetOrHide(sMode,
              s->state == SCREEN_SCAN_STOPPED ? txt(STR_DX_STOPPED) : s->mode);
  uiBaseline(sMode, &roboto_small,
             mark != NULL ? PANEL_IN + UI_ICON_SIZE + UI_TIGHT : PANEL_IN,
             MODE_BASE);
  uiSetOrHide(sRule, s->rule);
  if (s->rule != NULL) {
    uiBaselineRight(sRule, &roboto_small, PANEL_RIGHT, MODE_BASE);
  }

  uiShowIf(sReady, s->frequency == NULL);
  uiSetOrHide(sFreq, s->frequency);
  uiShowIf(sUnit, s->frequency != NULL);
  if (s->frequency != NULL) {
    uiBaseline(sFreq, &roboto_freq, PANEL_IN, FREQ_BASE);
    const int16_t fw = uiTextWidth(sFreq, &roboto_freq);
    uiBaseline(sUnit, &roboto_text, (int16_t)(PANEL_IN + fw + UI_UNIT_GAP),
               FREQ_BASE);
  }

  uiSetOrHide(sDwell, s->dwell);
  uiShowIf(sDwellLabel, s->dwell != NULL);
  if (s->dwell != NULL) {
    rightPair(sDwellLabel, &roboto_label, sDwell, &roboto_small, UI_TIGHT,
              PANEL_RIGHT, DWELL_BASE);
  }
  uiSetOrHide(sLeft, s->left);
  uiShowIf(sLeftUnit, s->left != NULL);
  if (s->left != NULL) {
    rightPair(sLeft, &roboto_value, sLeftUnit, &roboto_label, SMALL_UNIT_GAP,
              PANEL_RIGHT, FREQ_BASE);
  }
}

static void showProgress(const ScreenScan *s) {
  uiShowIf(sFill, s->hasProgress);
  if (s->hasProgress) {
    /* Never narrower than it is tall, so its rounded ends stay round. */
    int32_t w = ((int32_t)PANEL_W * s->progressPermille + 500) / 1000;
    if (w < BAR_H) {
      w = BAR_H;
    }
    lv_obj_set_width(sFill, (int32_t)w);
  }
  uiSetOrHide(sFrom, s->from);
  if (s->from != NULL) {
    uiBaseline(sFrom, &roboto_label, UI_MARGIN, BAR_LABEL_BASE);
  }
  uiSetOrHide(sTo, s->to);
  if (s->to != NULL) {
    uiBaselineRight(sTo, &roboto_label, SCAN_W - UI_MARGIN, BAR_LABEL_BASE);
  }
  uiSetOrHide(sStep, s->step);
  if (s->step != NULL) {
    const int16_t w = uiTextWidth(sStep, &roboto_label);
    uiBaseline(sStep, &roboto_label, (int16_t)(SCAN_W / 2 - w / 2),
               BAR_LABEL_BASE);
  }
}

static void showStation(const ScreenScan *s, const Theme *t) {
  const bool on = s->stationOn;
  const ThemeColour fg = on ? t->ground : t->measurement;
  const ThemeColour dim = on ? t->ground : t->dead;
  uiSetBgColour(sTile, on ? t->radio : t->rule);

  const bool has = s->pi != NULL;
  uiShowIf(sPiLabel, has);
  uiSetOrHide(sPi, s->pi);
  uiSetOrHide(sPs, has ? s->ps : NULL);
  if (has) {
    uiSetColour(sPiLabel, dim);
    uiBaseline(sPiLabel, &roboto_label, PANEL_IN, TILE_BASE);
    const int16_t lw = uiTextWidth(sPiLabel, &roboto_label);
    const int16_t piX = (int16_t)(PANEL_IN + lw + UI_TIGHT);
    /* Amber for a PI confirmed here, dim for one only heard, in doubt or
     * 0000, as the DX page tells them apart. */
    uiSetColour(sPi, on ? t->ground : s->piSure ? t->radio : t->dead);
    uiBaseline(sPi, &roboto_text, piX, TILE_BASE);
    if (s->ps != NULL) {
      const int16_t pw = uiTextWidth(sPi, &roboto_text);
      uiSetColour(sPs, fg);
      uiBaseline(sPs, &roboto_text, (int16_t)(piX + pw + PS_GAP), TILE_BASE);
    }
  }
  uiSetOrHide(sLevel, s->level);
  uiShowIf(sLevelUnit, s->level != NULL);
  if (s->level != NULL) {
    uiSetColour(sLevel, fg);
    uiSetColour(sLevelUnit, dim);
    rightPair(sLevel, &roboto_text, sLevelUnit, &roboto_label, SMALL_UNIT_GAP,
              PANEL_RIGHT, TILE_BASE);
  }
  /* With no PI, the level alone is named, and the left says why the tile
   * has no station. */
  const bool named = !has && s->level != NULL;
  uiShowIf(sLevelWord, named);
  if (named) {
    const int16_t right =
        (int16_t)(PANEL_RIGHT - uiTextWidth(sLevelUnit, &roboto_label) -
                  SMALL_UNIT_GAP - uiTextWidth(sLevel, &roboto_text) -
                  UI_TIGHT);
    uiBaselineRight(sLevelWord, &roboto_label, right, TILE_BASE);
  }
  uiSetOrHide(sNote, s->note);
  if (s->note != NULL) {
    uiBaseline(sNote, &roboto_small, PANEL_IN, TILE_BASE);
  }
}

void screenScanShow(const ScreenScan *s) {
  if (sRoot == NULL || s == NULL) {
    return;
  }
  const Theme *t = themeCurrent();
  /* While a scan runs any touch only stops it, so the header promises no
   * back and no next page then; nor a next page after a message. */
  const bool idle = s->state != SCREEN_SCAN_RUNNING;
  uiFrameMarks(&sFrame, idle, idle && !s->positionIsMessage);
  uiSetText(sFrame.title, txt(STR_DX_TITLE_SCANNER));
  const int16_t pillX = (int16_t)(uiFrameTitleEnd(&sFrame) + PILL_GAP);
  lv_obj_set_x(sPill, pillX);
  uiBaseline(
      sPillText, &roboto_label,
      (int16_t)(pillX + (PILL_W - uiTextWidth(sPillText, &roboto_label)) / 2),
      PILL_BASE);
  /* The header's run starts past the title, or past the NEW pill while it
   * shows. */
  const int16_t left = s->isNew ? (int16_t)(pillX + PILL_W + UI_GAP)
                                : (int16_t)(uiFrameTitleEnd(&sFrame) + UI_GAP);
  char fit[40];
  const char *position =
      uiFitHeaderText(s->position, SCAN_W, left, fit, sizeof(fit));
  uiFrameShow(&sFrame, txt(STR_DX_TITLE_SCANNER), s->found, position, s->clock,
              NULL, NULL);
  uiSetColour(sFrame.position, s->positionIsMessage ? themeCurrent()->radio
                                                    : themeCurrent()->dead);

  uiShowIf(sPill, s->isNew);
  uiShowIf(sPillText, s->isNew);

  showPanel(s);
  showProgress(s);
  showStation(s, t);
}

void screenScanEnd(void) {
  if (sRoot == NULL) {
    return;
  }
  uiDropRoot(&sRoot);
}

int screenScanZones(TouchZone *out, int max) {
  if (sRoot == NULL || max < 4 || screenDxZones(out, max, false) < 3) {
    return 0;
  }
  /* The panel down to four rows under it, and the rest below. */
  const int16_t top = out[2].y;
  const int16_t under = (int16_t)(PANEL_Y + PANEL_H + 4);
  out[2] = {0, top, SCAN_W, (int16_t)(under - top), DX_ZONE_PANEL};
  out[3] = {0, under, SCAN_W, (int16_t)(SCAN_H - under), DX_ZONE_BODY};
  return 4;
}
