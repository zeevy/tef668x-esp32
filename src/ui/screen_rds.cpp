/*
 * Everything the RDS decoder knows, over four pages.
 *
 * It draws what it is given and works nothing out. Which fields have
 * arrived, what they are called and how they are formatted are all decided
 * in the layer that knows what a radio is; this file owns pixels, and the
 * knob only moves which page is on screen.
 *
 * Four pages in the same style the radio and DX screens use: an amber panel for
 * what matters most, grey tiles for the rest, symbols for flags, and the same
 * frame, the page's name on the left of the header and the station's frequency,
 * the page and the clock on the right. There is no line of hints along the
 * bottom, so every page runs down to the bottom margin.
 *
 * Only the page on screen is built. A page change empties the body and
 * builds the next, so the LVGL pool holds one page of objects, not four.
 * It owns the panel on its own, like the menu, because the pool holds one
 * screen at a time.
 */
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "../core/strings.h"
#include "draw.h"
#include "fonts.h"
#include "screen.h"
#include "theme.h"

#define RDS_W 320
#define RDS_H 240
#define RIGHT (RDS_W - UI_MARGIN)
#define BOTTOM (RDS_H - UI_MARGIN) /* Nothing lower on any page. */

/* The amber panel and the tile beside it, the DX page's own measures. */
#define PANEL_Y 32
#define STATION_PANEL_W 208
#define TILE_X 228
#define TILE_W 80
#define TILE_MID (TILE_X + TILE_W / 2)
#define WIDE_W (RDS_W - 2 * UI_MARGIN)

static lv_obj_t *sRds;
static lv_obj_t *sBody;
static UiFrame sFrame;
static int8_t sBuilt = -1;

static const StrId kTitle[SCREEN_RDS_PAGES] = {
    STR_RDS_TITLE_IDENTITY, STR_RDS_TITLE_TEXT, STR_RDS_TITLE_NETWORKS,
    STR_RDS_TITLE_DECODER};

/* A value, or a dash in the dead colour when the radio has no answer. */
static void setValue(lv_obj_t *o, const char *v, ThemeColour on,
                     const Theme *t) {
  uiSetText(o, v != NULL ? v : txt(STR_COMMON_DASH));
  uiSetColour(o, v != NULL ? on : t->dead);
}

static lv_obj_t *staticLabel(const lv_font_t *font, ThemeColour c,
                             const char *text, int16_t x, int16_t base) {
  lv_obj_t *o = uiLabel(sBody, font, c);
  uiSetTextStatic(o, text);
  uiBaseline(o, font, x, base);
  return o;
}

/* ------------------------------------------------------ page 1, station */

/*
 * Who the station is, then what it says it is doing, then the rest. The
 * name and programme type on the amber panel with the PI tile beside it,
 * the four symbol tiles straight under them, and four readings as tiles
 * standing on the bottom, the last ending at y 226 as a list row does.
 *
 * The ECC sits on the PI tile's top line, since the country under the PI
 * is read from the two together.
 */
#define P1_PANEL_H 76
#define P1_PS_BASE 62
#define P1_PTY_BASE 96
#define P1_PI_LABEL_BASE 50
#define P1_PI_BASE 80
#define P1_COUNTRY_BASE 100
#define P1_CHIP_Y 116
#define P1_CHIP_H 28
/* TP and TA share the left half and music or speech has the right, so the
 * row lines up with the readings under it. */
static const int16_t kChipX[3] = {UI_MARGIN, UI_MARGIN + 76, UI_MARGIN + 152};
static const int16_t kChipW[3] = {68, 68, 144};
#define P1_CHIP_BASE (P1_CHIP_Y + 19)
#define P1_CHIP_ICON_GAP 5 /* The symbol's ink to its word. */
#define P1_READ_Y 152
#define P1_READ_W 144
#define P1_READ_H 35
#define P1_READ_PITCH_X (P1_READ_W + UI_GAP)
#define P1_READ_PITCH_Y (P1_READ_H + 4)
#define P1_READ_BASE 23 /* The text's baseline, down from a tile's top. */
#define P1_READ_PAD 10
#define P1_READ_LABEL_W 40 /* Wider than LANG and PTYN, the longest. */

/* The coverage area, read from the PI, and the clock come first, since
 * most stations send them. */
static const StrId kReadLabels[4] = {STR_RDS_AREA, STR_RDS_CT, STR_RDS_PTYN,
                                     STR_RDS_LANG};
#define P1_READ_PTYN 2 /* The one the station writes itself. */

static lv_obj_t *sPs, *sPtyNum, *sPtyName, *sPi, *sCountry, *sEcc;
static lv_obj_t *sReadValue[4];
static lv_obj_t *sChipIcon[3], *sChipWord[3];
static lv_obj_t *sDiIcon, *sDiWord;

static void buildStation(const Theme *t) {
  uiRound(sBody, t->radio, UI_MARGIN, PANEL_Y, STATION_PANEL_W, P1_PANEL_H,
          UI_RADIUS);
  sPs = uiLabel(sBody, &roboto_name, t->ground);
  sPtyNum = uiLabel(sBody, &roboto_label, t->ground);
  sPtyName = uiLabel(sBody, &roboto_text, t->ground);
  uiRound(sBody, t->rule, TILE_X, PANEL_Y, TILE_W, P1_PANEL_H, UI_TILE_R);
  staticLabel(&roboto_label, t->dead, txt(STR_COMMON_PI), TILE_X + UI_GAP,
              P1_PI_LABEL_BASE);
  sEcc = uiLabel(sBody, &roboto_label, t->dead);
  sPi = uiLabel(sBody, &roboto_value, t->radio);
  sCountry = uiLabel(sBody, &roboto_small, t->measurement);
  for (uint8_t i = 0; i < 3; i++) {
    uiRound(sBody, t->rule, kChipX[i], P1_CHIP_Y, kChipW[i], P1_CHIP_H,
            UI_TILE_R);
    sChipIcon[i] = uiLabel(sBody, &roboto_icons, t->dead);
    sChipWord[i] = uiLabel(sBody, &roboto_label, t->dead);
  }
  sDiIcon = uiLabel(sBody, &roboto_icons, t->dead);
  sDiWord = uiLabel(sBody, &roboto_label, t->measurement);
  for (uint8_t i = 0; i < 4; i++) {
    const int16_t x = (int16_t)(UI_MARGIN + (i % 2) * P1_READ_PITCH_X);
    const int16_t y = (int16_t)(P1_READ_Y + (i / 2) * P1_READ_PITCH_Y);
    uiRound(sBody, t->rule, x, y, P1_READ_W, P1_READ_H, UI_TILE_R);
    staticLabel(&roboto_label, t->dead, txt(kReadLabels[i]),
                (int16_t)(x + P1_READ_PAD), (int16_t)(y + P1_READ_BASE));
    sReadValue[i] = uiLabel(sBody, &roboto_small, t->measurement);
  }
}

/* Puts a symbol so its ink, not its box, starts at `left`, and returns where
 * its ink ends. The symbols are drawn to different widths in the same box,
 * 10 px for traffic and 16 px for speech, so placing them by the box leaves
 * uneven gaps before their words. Every symbol is one three byte character. */
static int16_t placeIcon(lv_obj_t *o, const char *icon, int16_t left,
                         int16_t top) {
  const uint8_t *b = (const uint8_t *)icon;
  const uint32_t letter = ((uint32_t)(b[0] & 0x0F) << 12) |
                          ((uint32_t)(b[1] & 0x3F) << 6) | (b[2] & 0x3F);
  lv_font_glyph_dsc_t g;
  int16_t ofs = 0;
  int16_t w = UI_ICON_SIZE;
  if (lv_font_get_glyph_dsc(&roboto_icons, &g, letter, 0)) {
    ofs = g.ofs_x;
    w = (int16_t)g.box_w;
  }
  lv_obj_set_pos(o, (int16_t)(left - ofs), top);
  return (int16_t)(left + w);
}

/* A symbol tile, three ways: the symbol lit in the broadcast colour when
 * the station says yes, the word alone in the measurement colour when it
 * says no, and both dead when it has not said. The symbol starts where the
 * readings' labels do, and its word follows it. */
static void showChip(uint8_t i, const char *icon, const char *word,
                     ScreenRdsFlag state, const Theme *t) {
  const int16_t left = (int16_t)(kChipX[i] + P1_READ_PAD);
  const bool on = state == SCREEN_RDS_YES;
  uiSetText(sChipIcon[i], icon);
  uiSetColour(sChipIcon[i], on ? t->broadcast : t->dead);
  uiSetText(sChipWord[i], word);
  uiSetColour(sChipWord[i],
              state == SCREEN_RDS_UNKNOWN ? t->dead : t->measurement);
  const int16_t inkEnd =
      placeIcon(sChipIcon[i], icon, left, uiIconTop(P1_CHIP_BASE));
  uiBaseline(sChipWord[i], &roboto_label, (int16_t)(inkEnd + P1_CHIP_ICON_GAP),
             P1_CHIP_BASE);
}

static void showStation(const ScreenRds *r, const Theme *t) {
  char fit[48];
  const int16_t panelRoom = (int16_t)(STATION_PANEL_W - 2 * UI_PAD);
  uiSetText(sPs, uiFitText(r->ps != NULL ? r->ps : txt(STR_COMMON_DASH),
                           &roboto_name, panelRoom, fit, sizeof(fit)));
  uiBaseline(sPs, &roboto_name, UI_MARGIN + UI_PAD, P1_PS_BASE);
  /* The programme type's number at the right of its name's line. */
  uiSetOrHide(sPtyNum, r->ptyNumber);
  uiBaselineRight(sPtyNum, &roboto_label, UI_MARGIN + STATION_PANEL_W - UI_PAD,
                  P1_PTY_BASE);
  const int16_t ptyRoom =
      (int16_t)(panelRoom - (r->ptyNumber != NULL
                                 ? uiTextWidth(sPtyNum, &roboto_label) + UI_GAP
                                 : 0));
  uiSetText(sPtyName,
            uiFitText(r->ptyName != NULL ? r->ptyName : txt(STR_COMMON_DASH),
                      &roboto_text, ptyRoom, fit, sizeof(fit)));
  uiBaseline(sPtyName, &roboto_text, UI_MARGIN + UI_PAD, P1_PTY_BASE);

  char ecc[16];
  if (r->ecc != NULL) {
    snprintf(ecc, sizeof(ecc), txt(STR_COMMON_FMT_TWO_WORDS), txt(STR_RDS_ECC),
             r->ecc);
  }
  uiSetOrHide(sEcc, r->ecc != NULL ? ecc : NULL);
  uiBaselineRight(sEcc, &roboto_label, TILE_X + TILE_W - UI_GAP,
                  P1_PI_LABEL_BASE);
  setValue(sPi, r->pi, r->piSure ? t->radio : t->dead, t);
  uiBaseline(sPi, &roboto_value,
             (int16_t)(TILE_MID - uiTextWidth(sPi, &roboto_value) / 2),
             P1_PI_BASE);
  /* A country code, or in the label face and dead, why there is none. */
  const lv_font_t *cf = r->countryNamed ? &roboto_small : &roboto_label;
  uiSetFont(sCountry, cf);
  /* A name is fitted, since a station's own RT+ name can be wider than the
   * tile. A reason is not: each one was sized to the tile's full width. */
  char fitCountry[16];
  const char *country = r->country != NULL ? r->country : "";
  if (r->countryNamed) {
    country = uiFitText(country, cf, TILE_W - 2 * UI_GAP, fitCountry,
                        sizeof(fitCountry));
  }
  uiSetText(sCountry, country);
  uiSetColour(sCountry, r->countryFault                       ? t->fault
                        : r->countryNamed && !r->countryGuess ? t->measurement
                                                              : t->dead);
  uiBaseline(sCountry, cf, (int16_t)(TILE_MID - uiTextWidth(sCountry, cf) / 2),
             P1_COUNTRY_BASE);

  showChip(0, ICON_TRAFFIC, txt(STR_RDS_TP), r->tp, t);
  showChip(1, ICON_CAMPAIGN, txt(STR_RDS_TA), r->ta, t);
  /* Music or speech is a word either way, so a known answer lights it. */
  showChip(
      2, r->speech == SCREEN_RDS_YES ? ICON_SPEECH : ICON_MUSIC,
      txt(r->speech == SCREEN_RDS_YES  ? STR_RDS_SPEECH
          : r->speech == SCREEN_RDS_NO ? STR_RDS_MUSIC
                                       : STR_RDS_MS_UNKNOWN),
      r->speech == SCREEN_RDS_UNKNOWN ? SCREEN_RDS_UNKNOWN : SCREEN_RDS_YES, t);
  /* Stereo or mono in the header after the title, once the station says. */
  const bool diKnown = r->stereo != SCREEN_RDS_UNKNOWN && r->message == NULL;
  uiSetOrHide(sDiIcon, diKnown ? ICON_EQ : NULL);
  uiSetOrHide(
      sDiWord,
      diKnown ? txt(r->stereo == SCREEN_RDS_YES ? STR_RDS_STEREO : STR_RDS_MONO)
              : NULL);
  uiSetColour(sDiIcon, r->stereo == SCREEN_RDS_YES ? t->broadcast : t->dead);
  const int16_t diX =
      (int16_t)(UI_MARGIN + uiTextWidth(sFrame.title, &roboto_title) + UI_PAD);
  const int16_t diInkEnd =
      placeIcon(sDiIcon, ICON_EQ, diX, uiIconTop(UI_HEAD_RUN_BASE));
  uiBaseline(sDiWord, &roboto_label, (int16_t)(diInkEnd + P1_CHIP_ICON_GAP),
             UI_HEAD_RUN_BASE);

  /* Each reading at the right of its tile, after its label. */
  const char *const values[4] = {r->area, r->ct, r->ptyn, r->language};
  for (uint8_t i = 0; i < 4; i++) {
    const int16_t right = (int16_t)(UI_MARGIN + (i % 2) * P1_READ_PITCH_X +
                                    P1_READ_W - P1_READ_PAD);
    uiSetText(
        sReadValue[i],
        uiFitText(values[i] != NULL ? values[i] : txt(STR_COMMON_DASH),
                  &roboto_small, P1_READ_W - 2 * P1_READ_PAD - P1_READ_LABEL_W,
                  fit, sizeof(fit)));
    uiSetColour(sReadValue[i], values[i] == NULL   ? t->dead
                               : i == P1_READ_PTYN ? t->broadcast
                                                   : t->measurement);
    uiBaselineRight(
        sReadValue[i], &roboto_small, right,
        (int16_t)(P1_READ_Y + (i / 2) * P1_READ_PITCH_Y + P1_READ_BASE));
  }
}

/* ---------------------------------------------------- page 2, radio text */

/* The RT+ rows, or the note that stands for them, stand on the bottom: each
 * row has the height of a list row and the last ends at y 226, as the
 * menu's does. The amber panel takes all the height above them. */
#define P2_TEXT_W (WIDE_W - 2 * UI_PAD)
#define P2_ROWS_END (BOTTOM - 2)
#define P2_TAG_H 30
#define P2_TAG_PITCH 34
#define P2_TAG_BASE 21    /* The text's baseline, down from a row's top. */
#define P2_TAG_TEXT_X 112 /* Clear of APPOINTMENT, the longest label. */
#define P2_NOTE_Y (P2_ROWS_END - P2_TAG_H)
#define P2_NOTE_LABEL_BASE (P2_NOTE_Y - 3)
#define P2_PANEL_TO_ROW 8 /* The panel's bottom to the first row. */
/* With no row, the panel ends 12 above the top of the note's label, whose
 * face rises 13 above its baseline. */
#define P2_NOTE_PANEL_END (P2_NOTE_LABEL_BASE - 13 - 12)
/* The last line's baseline stays this far above the panel's bottom. */
#define P2_TEXT_FOOT 20

/* The radio text in the larger face when all its lines fit the panel, and
 * otherwise in the smaller, which holds the longest text in three lines of
 * the shortest panel. */
typedef struct {
  const lv_font_t *font;
  int16_t base; /* The first line's baseline. */
  int16_t pitch;
} TextFace;
static const TextFace kTextFace[2] = {{&roboto_menu, 60, 26},
                                      {&roboto_text, 56, 22}};

static lv_obj_t *sTextPanel, *sText;
static lv_obj_t *sTagTile[SCREEN_RDS_TAGS], *sTagLabel[SCREEN_RDS_TAGS],
    *sTagText[SCREEN_RDS_TAGS];
static lv_obj_t *sRunning;
static lv_obj_t *sNoteLabel, *sNoteTile, *sNote;

static void buildText(const Theme *t) {
  sTextPanel = uiRound(sBody, t->radio, UI_MARGIN, PANEL_Y, WIDE_W,
                       P2_NOTE_PANEL_END - PANEL_Y, UI_RADIUS);
  sText = uiLabel(sBody, &roboto_text, t->ground);
  lv_obj_set_width(sText, P2_TEXT_W);
  for (uint8_t i = 0; i < SCREEN_RDS_TAGS; i++) {
    sTagTile[i] =
        uiRound(sBody, t->rule, UI_MARGIN, 0, WIDE_W, P2_TAG_H, UI_TILE_R);
    sTagLabel[i] = uiLabel(sBody, &roboto_label, t->dead);
    sTagText[i] = uiLabel(sBody, &roboto_small, t->broadcast);
  }
  sRunning = uiLabel(sBody, &roboto_icons, t->good);
  uiSetTextStatic(sRunning, ICON_PLAY);
  sNoteLabel = staticLabel(&roboto_label, t->dead, txt(STR_RDS_RTPLUS),
                           UI_MARGIN, P2_NOTE_LABEL_BASE);
  sNoteTile = uiRound(sBody, t->rule, UI_MARGIN, P2_NOTE_Y, WIDE_W, P2_TAG_H,
                      UI_TILE_R);
  sNote = uiLabel(sBody, &roboto_small, t->dead);
}

static void showText(const ScreenRds *r) {
  const uint8_t rows = r->tagCount;
  const int16_t rowsTop =
      (int16_t)(P2_ROWS_END - rows * P2_TAG_PITCH + (P2_TAG_PITCH - P2_TAG_H));
  const int16_t panelBottom =
      rows > 0 ? (int16_t)(rowsTop - P2_PANEL_TO_ROW) : P2_NOTE_PANEL_END;
  lv_obj_set_height(sTextPanel, (int16_t)(panelBottom - PANEL_Y));

  /* Packed at spaces, and only when the text or the panel changed: the same
   * text arrives on almost every poll. LVGL's own wrapping also breaks after
   * a full stop, which cuts a frequency like 93.50 in two. */
  static char last[80];
  static int16_t lastBottom;
  static uint8_t face;
  static char packed[80];
  const char *text = r->text != NULL ? r->text : txt(STR_COMMON_DASH);
  if (strcmp(text, last) != 0 || panelBottom != lastBottom) {
    snprintf(last, sizeof(last), "%s", text);
    lastBottom = panelBottom;
    for (face = 0; face < 2; face++) {
      const TextFace *f = &kTextFace[face];
      uiWrapAtSpaces(text, f->font, P2_TEXT_W, packed, sizeof(packed));
      int16_t lines = 1;
      for (const char *c = packed; *c != '\0'; c++) {
        lines = (int16_t)(lines + (*c == '\n'));
      }
      if (face == 1 ||
          f->base + (lines - 1) * f->pitch <= panelBottom - P2_TEXT_FOOT) {
        break;
      }
    }
  }
  const TextFace *f = &kTextFace[face];
  uiSetFont(sText, f->font);
  const int32_t space = (int32_t)(f->pitch - lv_font_get_line_height(f->font));
  if (lv_obj_get_style_text_line_space(sText, LV_PART_MAIN) != space) {
    lv_obj_set_style_text_line_space(sText, space, 0);
  }
  uiSetText(sText, packed);
  uiBaseline(sText, f->font, UI_MARGIN + UI_PAD, f->base);

  char fit[64];
  for (uint8_t i = 0; i < SCREEN_RDS_TAGS; i++) {
    const bool on = i < rows;
    uiShowIf(sTagTile[i], on);
    uiShowIf(sTagLabel[i], on);
    uiShowIf(sTagText[i], on);
    if (!on) {
      continue;
    }
    const int16_t y = (int16_t)(rowsTop + i * P2_TAG_PITCH);
    lv_obj_set_y(sTagTile[i], y);
    const int16_t base = (int16_t)(y + P2_TAG_BASE);
    uiSetText(sTagLabel[i], r->tag[i].label);
    uiBaseline(sTagLabel[i], &roboto_label, UI_MARGIN + 10, base);
    const int16_t room =
        (int16_t)(RIGHT - 10 - P2_TAG_TEXT_X - (i == 0 ? 24 : 0));
    uiSetText(sTagText[i],
              uiFitText(r->tag[i].text, &roboto_small, room, fit, sizeof(fit)));
    uiBaseline(sTagText[i], &roboto_small, P2_TAG_TEXT_X, base);
  }
  uiShowIf(sRunning, rows > 0 && r->rtPlusRunning);
  lv_obj_set_pos(sRunning, (int16_t)(RIGHT - 8 - UI_ICON_SIZE),
                 uiIconTop((int16_t)(rowsTop + P2_TAG_BASE)));

  const bool note = rows == 0;
  uiShowIf(sNoteLabel, note);
  uiShowIf(sNoteTile, note);
  uiShowIf(sNote, note);
  if (note) {
    uiSetText(sNote, r->rtPlusNote != NULL ? r->rtPlusNote : "");
    uiBaseline(sNote, &roboto_small, UI_MARGIN + 10, P2_NOTE_Y + 19);
  }
}

/* ---------------------------------------------------- page 3, networks */

/*
 * Every slot has its place: two rows of four frequency tiles, then three
 * network rows, the last ending at y 226 as a list row does. A slot with
 * nothing in it is an outline, so the page keeps its shape while
 * frequencies and networks arrive, and an empty list says None heard at
 * the right of its label. With both lists empty one panel says so for both.
 */
#define P3_AF_LABEL_BASE 48
#define P3_AF_Y 51
#define P3_AF_W 68
#define P3_AF_H 24
#define P3_AF_PITCH_X 76
#define P3_AF_PITCH_Y 28
#define P3_EON_LABEL_BASE 128
#define P3_EON_Y 131
#define P3_EON_H UI_MENU_ROW_H
#define P3_EON_PITCH UI_MENU_ROW_PITCH
#define P3_EON_BASE (P3_EON_H / 2 + 6) /* As on the menu. */
#define P3_PI_X (UI_MARGIN + 10)
#define P3_PS_X 68
#define P3_FREQ_X 154
/* The empty panel's two labels and two answers, centred on the page. */
#define P3_EMPTY_AF_BASE 96
#define P3_EMPTY_EON_BASE 150
#define P3_EMPTY_ANSWER 22 /* An answer's baseline under its label's. */

static lv_obj_t *sAfLabel, *sAfTile[SCREEN_RDS_AF], *sAfText[SCREEN_RDS_AF];
static lv_obj_t *sAfNone;
static lv_obj_t *sEmptyPanel;
static lv_obj_t *sEonLabel;
static lv_obj_t *sEonTile[SCREEN_RDS_EON], *sEonPi[SCREEN_RDS_EON],
    *sEonPs[SCREEN_RDS_EON], *sEonFreq[SCREEN_RDS_EON], *sEonTa[SCREEN_RDS_EON];
static lv_obj_t *sEonNone;
static lv_obj_t *sEonMore;

/* A slot's tile: filled when it holds something, an outline when not. */
static lv_obj_t *slot(const Theme *t, int16_t x, int16_t y, int16_t w,
                      int16_t h) {
  lv_obj_t *o = uiRound(sBody, t->rule, x, y, w, h, UI_TILE_R);
  lv_obj_set_style_border_width(o, 1, 0);
  lv_obj_set_style_border_color(o, uiColour(t->rule), 0);
  return o;
}

static void fillSlot(lv_obj_t *o, bool full, const Theme *t) {
  uiSetBgColour(o, full ? t->rule : t->ground);
}

static void buildNetworks(const Theme *t) {
  sEmptyPanel = uiRound(sBody, t->rule, UI_MARGIN, PANEL_Y, WIDE_W,
                        BOTTOM - 2 - PANEL_Y, UI_TILE_R);
  sAfLabel = uiLabel(sBody, &roboto_label, t->dead);
  uiSetTextStatic(sAfLabel, txt(STR_RDS_AF_TITLE));
  for (uint8_t i = 0; i < SCREEN_RDS_AF; i++) {
    sAfTile[i] =
        slot(t, (int16_t)(UI_MARGIN + (i % 4) * P3_AF_PITCH_X),
             (int16_t)(P3_AF_Y + (i / 4) * P3_AF_PITCH_Y), P3_AF_W, P3_AF_H);
    sAfText[i] = uiLabel(sBody, &roboto_text, t->measurement);
  }
  sAfNone = uiLabel(sBody, &roboto_label, t->dead);
  uiSetTextStatic(sAfNone, txt(STR_RDS_NONE_HEARD));
  sEonLabel = uiLabel(sBody, &roboto_label, t->dead);
  uiSetTextStatic(sEonLabel, txt(STR_RDS_EON_TITLE));
  for (uint8_t i = 0; i < SCREEN_RDS_EON; i++) {
    sEonTile[i] = slot(t, UI_MARGIN, (int16_t)(P3_EON_Y + i * P3_EON_PITCH),
                       WIDE_W, P3_EON_H);
    sEonPi[i] = uiLabel(sBody, &roboto_text, t->radio);
    sEonPs[i] = uiLabel(sBody, &roboto_small, t->measurement);
    sEonFreq[i] = uiLabel(sBody, &roboto_label, t->dead);
    sEonTa[i] = uiLabel(sBody, &roboto_icons, t->dead);
    uiSetTextStatic(sEonTa[i], ICON_CAMPAIGN);
  }
  sEonNone = uiLabel(sBody, &roboto_label, t->dead);
  uiSetTextStatic(sEonNone, txt(STR_RDS_NONE_HEARD));
  sEonMore = uiLabel(sBody, &roboto_label, t->measurement);
}

/* A list's label, and its None heard, either on one line at `base` or,
 * on the empty panel, both in the middle of the page with the answer in
 * the larger face under the label. */
static void showListLabel(lv_obj_t *label, lv_obj_t *none, bool empty,
                          bool panel, int16_t base) {
  uiShowIf(none, empty);
  if (panel) {
    uiBaseline(label, &roboto_label,
               (int16_t)(RDS_W / 2 - uiTextWidth(label, &roboto_label) / 2),
               base);
    uiSetFont(none, &roboto_text);
    uiBaseline(none, &roboto_text,
               (int16_t)(RDS_W / 2 - uiTextWidth(none, &roboto_text) / 2),
               (int16_t)(base + P3_EMPTY_ANSWER));
    return;
  }
  uiBaseline(label, &roboto_label, UI_MARGIN, base);
  uiSetFont(none, &roboto_label);
  uiBaselineRight(none, &roboto_label, RIGHT, base);
}

static void showNetworks(const ScreenRds *r, const Theme *t) {
  const bool panel = r->afCount == 0 && r->eonCount == 0;
  uiShowIf(sEmptyPanel, panel);
  showListLabel(sAfLabel, sAfNone, r->afCount == 0, panel,
                panel ? P3_EMPTY_AF_BASE : P3_AF_LABEL_BASE);
  showListLabel(sEonLabel, sEonNone, r->eonCount == 0, panel,
                panel ? P3_EMPTY_EON_BASE : P3_EON_LABEL_BASE);

  for (uint8_t i = 0; i < SCREEN_RDS_AF; i++) {
    const bool on = i < r->afCount;
    uiShowIf(sAfTile[i], !panel);
    fillSlot(sAfTile[i], on, t);
    uiShowIf(sAfText[i], on);
    if (!on) {
      continue;
    }
    uiSetText(sAfText[i], r->af[i]);
    const int16_t mid =
        (int16_t)(UI_MARGIN + (i % 4) * P3_AF_PITCH_X + P3_AF_W / 2);
    uiBaseline(sAfText[i], &roboto_text,
               (int16_t)(mid - uiTextWidth(sAfText[i], &roboto_text) / 2),
               (int16_t)(P3_AF_Y + (i / 4) * P3_AF_PITCH_Y + P3_AF_H / 2 + 6));
  }

  char fit[40];
  for (uint8_t i = 0; i < SCREEN_RDS_EON; i++) {
    const bool on = i < r->eonCount;
    uiShowIf(sEonTile[i], !panel);
    fillSlot(sEonTile[i], on, t);
    uiShowIf(sEonPi[i], on);
    uiShowIf(sEonPs[i], on);
    uiShowIf(sEonFreq[i], on && r->eon[i].freqs != NULL);
    uiShowIf(sEonTa[i], on && r->eon[i].ta != SCREEN_RDS_UNKNOWN);
    if (!on) {
      continue;
    }
    const ScreenRdsEon *e = &r->eon[i];
    const int16_t base = (int16_t)(P3_EON_Y + i * P3_EON_PITCH + P3_EON_BASE);
    uiSetText(sEonPi[i], e->pi);
    uiBaseline(sEonPi[i], &roboto_text, P3_PI_X, base);
    setValue(sEonPs[i],
             e->ps != NULL
                 ? uiFitText(e->ps, &roboto_small, P3_FREQ_X - P3_PS_X - UI_GAP,
                             fit, sizeof(fit))
                 : NULL,
             t->measurement, t);
    uiBaseline(sEonPs[i], &roboto_small, P3_PS_X, base);
    if (e->freqs != NULL) {
      uiSetText(sEonFreq[i],
                uiFitText(e->freqs, &roboto_label, 120, fit, sizeof(fit)));
      uiBaseline(sEonFreq[i], &roboto_label, P3_FREQ_X, base);
    }
    uiSetColour(sEonTa[i], e->ta == SCREEN_RDS_YES ? t->fault : t->dead);
    lv_obj_set_pos(sEonTa[i], (int16_t)(RIGHT - 10 - UI_ICON_SIZE),
                   uiIconTop(base));
  }
  /* Networks that did not fit are counted, not dropped without a word, on
   * the label's line, which says None heard instead when none came. */
  const bool more = r->eonCount > 0 && (r->eonMore > 0 || r->eonMoreHeard);
  uiShowIf(sEonMore, more);
  if (more) {
    /* A count only when it is the whole of the rest. */
    char text[24];
    if (r->eonMoreHeard) {
      snprintf(text, sizeof(text), "%s", txt(STR_RDS_EON_MORE_HEARD));
    } else {
      snprintf(text, sizeof(text), txt(STR_RDS_FMT_EON_MORE),
               (unsigned)r->eonMore);
    }
    uiSetText(sEonMore, text);
    uiBaselineRight(sEonMore, &roboto_label, RIGHT, P3_EON_LABEL_BASE);
  }
}

/* ----------------------------------------------------- page 4, decoder */

/*
 * The four counts as a block of tiles at the top left, the group types in
 * a fixed grid beside them, three rows of two, most sent first, and the
 * four blocks' bars across the whole width below, the last bar's text on
 * y 224. Nothing moves when the number of group types changes.
 */
#define P4_TILE_Y 32
#define P4_TILE_W 70
#define P4_TILE_H 44
#define P4_TILE_GAP 4
#define P4_TILE_LABEL_DY 14
#define P4_TILE_VALUE_DY 38
#define P4_SIDE_X 164 /* The group types' column, right of the tiles. */
#define P4_SIDE_W 144
#define P4_GROUP_LABEL_BASE 46
#define P4_GROUP_BASE 63 /* The first row's text. */
#define P4_GROUP_PITCH 26
#define P4_GROUP_W 68
#define P4_GROUP_COL (P4_GROUP_W + UI_GAP)
#define P4_GROUP_BAR_DY 4 /* A row's bar, under its text's baseline. */
#define P4_GROUP_BAR_H 4
#define P4_BLOCK_LABEL_BASE 145
#define P4_BLOCK_FIRST 154 /* The first bar's top. */
#define P4_BLOCK_PITCH 20
#define P4_BAR_H 10
#define P4_BLOCK_TEXT_DY 10 /* A bar's text, centred on it. */
#define P4_BLOCK_BAR_X 28
#define P4_BLOCK_BAR_W 196
/* The one object the bars are drawn in covers only them. */
#define P4_BARS_TOP (P4_GROUP_BASE + P4_GROUP_BAR_DY)
#define P4_BARS_H (P4_BLOCK_FIRST + 3 * P4_BLOCK_PITCH + P4_BAR_H - P4_BARS_TOP)

static const StrId kStatLabels[4] = {STR_RDS_SYNC_CAPS, STR_RDS_GROUPS_S_CAPS,
                                     STR_RDS_BLER, STR_RDS_LOST};

static lv_obj_t *sStat[4];
static lv_obj_t *sBlockText[4];
static lv_obj_t *sBars;
static lv_obj_t *sGroupLabel[SCREEN_RDS_GROUPS], *sGroupPct[SCREEN_RDS_GROUPS];
static lv_obj_t *sNoGroups;

static int16_t statX(uint8_t i) {
  return (int16_t)(UI_MARGIN + (i % 2) * (P4_TILE_W + P4_TILE_GAP));
}

static int16_t statY(uint8_t i) {
  return (int16_t)(P4_TILE_Y + (i / 2) * (P4_TILE_H + P4_TILE_GAP));
}

static int16_t groupX(uint8_t i) {
  return (int16_t)(P4_SIDE_X + (i % 2) * P4_GROUP_COL);
}

static int16_t groupBase(uint8_t i) {
  return (int16_t)(P4_GROUP_BASE + (i / 2) * P4_GROUP_PITCH);
}

/* What the bar object draws, kept from the last show, so it is drawn
 * again only when that changed. */
typedef struct {
  bool blocksKnown;
  uint16_t fixedTenths[4];
  uint16_t lostTenths[4];
  uint8_t groupCount;
  uint8_t group[SCREEN_RDS_GROUPS];
} BarsShown;
static BarsShown sShown;

/* Every bar on the page in one object, drawing itself, the pattern the
 * modulation meter uses, so ten bars cost one object in the pool. */
static void onBarsDraw(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_current_target(e);
  lv_layer_t *layer = lv_event_get_layer(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const Theme *t = themeCurrent();
  for (uint8_t b = 0; b < 4; b++) {
    const int16_t y =
        (int16_t)(P4_BLOCK_FIRST + b * P4_BLOCK_PITCH - P4_BARS_TOP);
    uiFillRect(layer, &area, P4_BLOCK_BAR_X, y, P4_BLOCK_BAR_W, P4_BAR_H,
               uiColour(t->rule));
    if (!sShown.blocksKnown) {
      continue;
    }
    /* From tenths of a per cent, so one lost block in a thousand shows,
     * two pixels wide at the least, and clean takes what is left so the
     * three never run past the track. */
    int16_t wf =
        (int16_t)((int32_t)P4_BLOCK_BAR_W * sShown.fixedTenths[b] / 1000);
    int16_t wl =
        (int16_t)((int32_t)P4_BLOCK_BAR_W * sShown.lostTenths[b] / 1000);
    if (sShown.fixedTenths[b] > 0 && wf < 2) {
      wf = 2;
    }
    if (sShown.lostTenths[b] > 0 && wl < 2) {
      wl = 2;
    }
    int16_t wc = (int16_t)(P4_BLOCK_BAR_W - wf - wl);
    if (wc < 0) {
      wc = 0;
    }
    int16_t x = P4_BLOCK_BAR_X;
    uiFillRect(layer, &area, x, y, wc, P4_BAR_H, uiColour(t->measurement));
    x = (int16_t)(x + wc);
    uiFillRect(layer, &area, x, y, wf, P4_BAR_H, uiColour(t->radio));
    x = (int16_t)(x + wf);
    uiFillRect(layer, &area, x, y, wl, P4_BAR_H, uiColour(t->fault));
  }
  for (uint8_t i = 0; i < sShown.groupCount; i++) {
    const int16_t y = (int16_t)(groupBase(i) + P4_GROUP_BAR_DY - P4_BARS_TOP);
    uiFillRect(layer, &area, groupX(i), y, P4_GROUP_W, P4_GROUP_BAR_H,
               uiColour(t->rule));
    uiFillRect(layer, &area, groupX(i), y,
               (int16_t)((int32_t)P4_GROUP_W * sShown.group[i] / 100),
               P4_GROUP_BAR_H, uiColour(t->broadcast));
  }
}

static void buildDecoder(const Theme *t) {
  for (uint8_t i = 0; i < 4; i++) {
    uiRound(sBody, t->rule, statX(i), statY(i), P4_TILE_W, P4_TILE_H,
            UI_TILE_R);
    staticLabel(&roboto_label, t->dead, txt(kStatLabels[i]),
                (int16_t)(statX(i) + UI_GAP),
                (int16_t)(statY(i) + P4_TILE_LABEL_DY));
    sStat[i] = uiLabel(sBody, &roboto_small, t->measurement);
  }
  staticLabel(&roboto_label, t->dead, txt(STR_RDS_GROUP_TYPES), P4_SIDE_X,
              P4_GROUP_LABEL_BASE);
  staticLabel(&roboto_label, t->dead, txt(STR_RDS_EACH_BLOCK), UI_MARGIN,
              P4_BLOCK_LABEL_BASE);
  /* The legend is the three words in the three colours of the bars. */
  lv_obj_t *lost = uiLabel(sBody, &roboto_label, t->fault);
  uiSetTextStatic(lost, txt(STR_RDS_LEGEND_LOST));
  uiBaselineRight(lost, &roboto_label, RIGHT, P4_BLOCK_LABEL_BASE);
  int16_t at = (int16_t)(RIGHT - uiTextWidth(lost, &roboto_label) - UI_GAP);
  lv_obj_t *fixed = uiLabel(sBody, &roboto_label, t->radio);
  uiSetTextStatic(fixed, txt(STR_RDS_LEGEND_FIXED));
  uiBaselineRight(fixed, &roboto_label, at, P4_BLOCK_LABEL_BASE);
  at = (int16_t)(at - uiTextWidth(fixed, &roboto_label) - UI_GAP);
  lv_obj_t *clean = uiLabel(sBody, &roboto_label, t->measurement);
  uiSetTextStatic(clean, txt(STR_RDS_LEGEND_CLEAN));
  uiBaselineRight(clean, &roboto_label, at, P4_BLOCK_LABEL_BASE);
  static const StrId kLetters[4] = {STR_RDS_BLOCK_A, STR_RDS_BLOCK_B,
                                    STR_RDS_BLOCK_C, STR_RDS_BLOCK_D};
  for (uint8_t b = 0; b < 4; b++) {
    staticLabel(
        &roboto_label, t->measurement, txt(kLetters[b]), UI_MARGIN,
        (int16_t)(P4_BLOCK_FIRST + b * P4_BLOCK_PITCH + P4_BLOCK_TEXT_DY));
    sBlockText[b] = uiLabel(sBody, &roboto_label, t->measurement);
  }
  for (uint8_t i = 0; i < SCREEN_RDS_GROUPS; i++) {
    sGroupLabel[i] = uiLabel(sBody, &roboto_label, t->measurement);
    sGroupPct[i] = uiLabel(sBody, &roboto_label, t->measurement);
  }
  sNoGroups = uiLabel(sBody, &roboto_label, t->dead);
  sBars = lv_obj_create(sBody);
  lv_obj_remove_style_all(sBars);
  lv_obj_set_pos(sBars, 0, P4_BARS_TOP);
  lv_obj_set_size(sBars, RDS_W, P4_BARS_H);
  memset(&sShown, 0, sizeof(sShown));
  lv_obj_clear_flag(sBars, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(sBars, onBarsDraw, LV_EVENT_DRAW_MAIN, NULL);
}

static void showDecoder(const ScreenRds *r, const Theme *t) {
  /* A number is never cut short: one too wide for its tile in a face drops
   * to the next smaller one, down to the label face. */
  static const lv_font_t *const kValueFaces[4] = {&roboto_menu, &roboto_text,
                                                  &roboto_small, &roboto_label};
  const char *const values[4] = {r->sync, r->rate, r->bler, r->lost};
  for (uint8_t i = 0; i < 4; i++) {
    setValue(sStat[i], values[i], t->measurement, t);
    uint8_t k = 0;
    for (; k < 3; k++) {
      uiSetFont(sStat[i], kValueFaces[k]);
      if (uiTextWidth(sStat[i], kValueFaces[k]) <=
          P4_TILE_W - UI_GAP - UI_TIGHT) {
        break;
      }
    }
    uiSetFont(sStat[i], kValueFaces[k]);
    uiBaseline(sStat[i], kValueFaces[k], (int16_t)(statX(i) + UI_GAP),
               (int16_t)(statY(i) + P4_TILE_VALUE_DY));
  }
  /* Sync is a state, never a missing field: off, not locked and locked are
   * three different true answers, and only locked takes the good colour. */
  uiSetColour(sStat[0], r->syncGood ? t->good : t->dead);

  BarsShown next;
  memset(&next, 0, sizeof(next));
  next.blocksKnown = r->blocksKnown;
  for (uint8_t b = 0; b < 4; b++) {
    next.fixedTenths[b] = r->block[b].fixedTenths;
    next.lostTenths[b] = r->block[b].lostTenths;
    const char *text = !r->blocksKnown            ? txt(STR_COMMON_DASH)
                       : r->block[b].text != NULL ? r->block[b].text
                                                  : txt(STR_RDS_ALL_CLEAN);
    uiSetText(sBlockText[b], text);
    uiSetColour(sBlockText[b], r->blocksKnown && r->block[b].text != NULL
                                   ? t->measurement
                                   : t->dead);
    uiBaselineRight(
        sBlockText[b], &roboto_label, RIGHT,
        (int16_t)(P4_BLOCK_FIRST + b * P4_BLOCK_PITCH + P4_BLOCK_TEXT_DY));
  }

  /* With no type to show, the reason, broken at spaces to the column. */
  next.groupCount = r->groupCount;
  uiShowIf(sNoGroups, r->groupCount == 0);
  if (r->groupCount == 0) {
    char note[64];
    uiSetText(sNoGroups,
              uiWrapAtSpaces(r->groupNote != NULL ? r->groupNote : "",
                             &roboto_label, P4_SIDE_W, note, sizeof(note)));
    uiBaseline(sNoGroups, &roboto_label, P4_SIDE_X, P4_GROUP_BASE);
  }
  for (uint8_t i = 0; i < SCREEN_RDS_GROUPS; i++) {
    const bool on = i < r->groupCount;
    uiShowIf(sGroupLabel[i], on);
    uiShowIf(sGroupPct[i], on);
    if (!on) {
      continue;
    }
    next.group[i] = r->group[i].percent > 100 ? 100 : r->group[i].percent;
    uiSetText(sGroupLabel[i], r->group[i].label);
    uiBaseline(sGroupLabel[i], &roboto_label, groupX(i), groupBase(i));
    char pct[8];
    snprintf(pct, sizeof(pct), txt(STR_RDS_FMT_PERCENT),
             (unsigned)next.group[i]);
    uiSetText(sGroupPct[i], pct);
    uiBaselineRight(sGroupPct[i], &roboto_label,
                    (int16_t)(groupX(i) + P4_GROUP_W), groupBase(i));
  }
  if (memcmp(&next, &sShown, sizeof(next)) != 0) {
    sShown = next;
    lv_obj_invalidate(sBars);
  }
}

/* ------------------------------------------------------------ the screen */

bool screenRdsBegin(void) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sRds != NULL) {
    return true;
  }
  const Theme *t = themeCurrent();

  lv_obj_remove_style_all(root);
  lv_obj_set_style_bg_color(root, uiColour(t->ground), 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

  sRds = uiBlock(root, t->ground, 0, 0, RDS_W, RDS_H);
  uiFrameBegin(&sFrame, sRds, t, t->radio);
  sBody = lv_obj_create(sRds);
  lv_obj_remove_style_all(sBody);
  lv_obj_set_pos(sBody, 0, 0);
  lv_obj_set_size(sBody, RDS_W, RDS_H);
  lv_obj_clear_flag(sBody, LV_OBJ_FLAG_SCROLLABLE);
  sBuilt = -1;
  return true;
}

static void build(uint8_t page, const Theme *t) {
  lv_obj_clean(sBody);
  if (page == 0) {
    buildStation(t);
  } else if (page == 1) {
    buildText(t);
  } else if (page == 2) {
    buildNetworks(t);
  } else {
    buildDecoder(t);
  }
  sBuilt = (int8_t)page;
}

void screenRdsShow(const ScreenRds *rds) {
  if (sRds == NULL || rds == NULL) {
    return;
  }
  const Theme *t = themeCurrent();
  const uint8_t page =
      rds->page >= SCREEN_RDS_PAGES ? SCREEN_RDS_PAGES - 1 : rds->page;
  if ((int8_t)page != sBuilt) {
    build(page, t);
  }
  char pageOf[8];
  snprintf(pageOf, sizeof(pageOf), txt(STR_RDS_FMT_PAGE), (unsigned)(page + 1),
           (unsigned)SCREEN_RDS_PAGES);
  const char *ctx = page == 3 ? rds->span : rds->frequency;
  if (rds->message != NULL) {
    /* The run from the title to the right margin, with page 1's stereo
     * mark giving way too. */
    char fit[40];
    uiSetText(sFrame.title, txt(kTitle[page]));
    const int16_t left =
        (int16_t)(UI_MARGIN + uiTextWidth(sFrame.title, &roboto_title) +
                  UI_GAP);
    uiFrameShow(&sFrame, txt(kTitle[page]), NULL,
                uiFitHeaderText(rds->message, RDS_W, left, fit, sizeof(fit)),
                NULL, NULL, NULL);
    uiSetColour(sFrame.position, t->radio);
  } else {
    uiFrameShow(&sFrame, txt(kTitle[page]), ctx, pageOf, rds->clock, NULL,
                NULL);
    uiSetColour(sFrame.position, t->dead);
  }
  if (page == 0) {
    showStation(rds, t);
  } else if (page == 1) {
    showText(rds);
  } else if (page == 2) {
    showNetworks(rds, t);
  } else {
    showDecoder(rds, t);
  }
}

void screenRdsEnd(void) {
  if (sRds == NULL) {
    return;
  }
  if (lv_obj_is_valid(sRds)) {
    lv_obj_del(sRds);
  }
  sRds = NULL;
  sBody = NULL;
  sBuilt = -1;
}
