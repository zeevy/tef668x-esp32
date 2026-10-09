/*
 * The four tiles along the bottom: what the radio is set to.
 *
 * The same four on every band, in `rule` with a radius of 6, UI_GAP apart
 * and each as wide as a quarter of the row. The tuning mode is a value on
 * its own. The squelch, the filter width and the volume each have a symbol
 * in `dead`, waves, an up and down arrow and a speaker, then the value in
 * `radio` after it, centred together in the tile. When a person has muted
 * the radio, the speaker gets a cross, the value reads MUTE and both turn
 * `fault`; while the squelch holds the sound, the volume turns `dead`. The
 * tile stays the same grey, so nothing but the symbol and the words change.
 * The header has no speaker, so this tile is the one place either shows.
 */
#include "../../core/strings.h"
#include "../draw.h"
#include "../panel.h"

#define TILE_COUNT 4
/* The baseline, 20 rows down the 28 row tile. */
#define TILE_BASE 20
/* Between a tile's symbol and its value. */
#define SYMBOL_GAP 3

static PanelRect sAt;
static lv_obj_t *sLabel[TILE_COUNT];
static lv_obj_t *sValue[TILE_COUNT];

static int16_t tileW(const PanelRect *at) {
  return (int16_t)((at->w - 2 * UI_MARGIN - (TILE_COUNT - 1) * UI_GAP) /
                   TILE_COUNT);
}

static int16_t tileX(const PanelRect *at, int i) {
  return (int16_t)(at->x + UI_MARGIN + i * (tileW(at) + UI_GAP));
}

static void begin(lv_obj_t *parent, const PanelRect *at) {
  sAt = *at;
  const Theme *t = themeCurrent();
  /* NULL marks the tile with no symbol. The volume's is set as it shows. */
  static const char *const kSymbols[TILE_COUNT] = {NULL, ICON_SQUELCH,
                                                   ICON_WIDTH, ICON_VOLUME};
  for (int i = 0; i < TILE_COUNT; i++) {
    uiRound(parent, t->rule, tileX(&sAt, i), at->y, tileW(&sAt), at->h,
            UI_TILE_R);
    sLabel[i] = uiLabel(parent, &roboto_icons, t->dead);
    if (kSymbols[i] != NULL) {
      lv_label_set_text_static(sLabel[i], kSymbols[i]);
    }
    uiShowIf(sLabel[i], kSymbols[i] != NULL);
    sValue[i] = uiLabel(parent, &roboto_small, t->radio);
  }
}

static void placeTile(int i, const char *value, ThemeColour label,
                      ThemeColour colour) {
  const bool hasLabel = i != 0;
  const bool hasValue = value != NULL && value[0] != '\0';
  uiSetOrHide(sValue[i], hasValue ? value : NULL);
  uiSetColour(sValue[i], colour);
  uiSetColour(sLabel[i], label);
  const int16_t lw = hasLabel ? (int16_t)(UI_ICON_SIZE + SYMBOL_GAP) : 0;
  const int16_t vw = hasValue ? uiTextWidth(sValue[i], &roboto_small) : 0;
  int16_t x = (int16_t)(tileX(&sAt, i) + (tileW(&sAt) - lw - vw) / 2);
  if (hasLabel) {
    /* Placed by its middle, as every symbol is, on the tile's middle. */
    lv_obj_set_pos(
        sLabel[i], x,
        (int16_t)(sAt.y + sAt.h / 2 - UI_ICON_SIZE / 2 + UI_ICON_INK_DROP));
    x = (int16_t)(x + lw);
  }
  uiBaseline(sValue[i], &roboto_small, x, (int16_t)(sAt.y + TILE_BASE));
}

static void show(const ScreenState *s) {
  const Theme *t = themeCurrent();
  placeTile(0, s->tuneMode, t->dead, t->radio);
  placeTile(1, s->squelchMode, t->dead, t->radio);
  placeTile(2, s->filter, t->dead, t->radio);
  const bool muted = s->audio == SCREEN_AUDIO_MUTED;
  const bool held = s->audio == SCREEN_AUDIO_SQUELCHED;
  lv_label_set_text_static(sLabel[3], muted ? ICON_VOLUME_OFF : ICON_VOLUME);
  placeTile(3, muted ? txt(STR_RADIO_MUTE) : s->volume,
            muted ? t->fault : t->dead,
            muted  ? t->fault
            : held ? t->dead
                   : t->radio);
}

/* A tile each, meeting halfway across the gaps between them so no tap on
 * the row falls between two, and the outer ones out to the row's ends. */
static int zones(const PanelRect *at, TouchZone *out, int max) {
  if (max < TILE_COUNT) {
    return 0;
  }
  int16_t left = at->x;
  for (int i = 0; i < TILE_COUNT; i++) {
    const int16_t right = i + 1 < TILE_COUNT
                              ? (int16_t)(tileX(at, i + 1) - UI_GAP / 2)
                              : (int16_t)(at->x + at->w);
    out[i] = {left, at->y, (int16_t)(right - left), at->h,
              (uint8_t)(RADIO_ZONE_MODE + i)};
    left = right;
  }
  return TILE_COUNT;
}

const Panel panelTiles = {begin, show, zones};
