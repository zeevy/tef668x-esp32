/*
 * The four tiles along the bottom: what the radio is set to.
 *
 * The same four on every band, in `rule` with a radius of 6, UI_GAP apart
 * and each as wide as a quarter of the row. The tuning mode is a value on
 * its own. SQ:, BW: and V: are a label in `dead` in the value's own face,
 * then the value in `radio` straight after the colon, centred together in
 * the tile. When a person has muted the radio, V: reads MUTE and both turn
 * `fault`; while the squelch holds the sound, the volume turns `dead`. The
 * tile stays the same grey, so nothing but the words changes. The header has
 * no speaker, so this tile is the one place either shows.
 */
#include "../../core/strings.h"
#include "../draw.h"
#include "../panel.h"

#define TILE_COUNT 4
/* The baseline, 20 rows down the 28 row tile. */
#define TILE_BASE 20

static PanelRect sAt;
static lv_obj_t *sLabel[TILE_COUNT];
static lv_obj_t *sValue[TILE_COUNT];

static int16_t tileW(void) {
  return (int16_t)((sAt.w - 2 * UI_MARGIN - (TILE_COUNT - 1) * UI_GAP) /
                   TILE_COUNT);
}

static int16_t tileX(int i) {
  return (int16_t)(sAt.x + UI_MARGIN + i * (tileW() + UI_GAP));
}

static void begin(lv_obj_t *parent, const PanelRect *at) {
  sAt = *at;
  const Theme *t = themeCurrent();
  /* STR_COUNT marks the tile with no label. */
  static const StrId kNames[TILE_COUNT] = {STR_COUNT, STR_RADIO_SQL,
                                           STR_RADIO_BW_TILE, STR_RADIO_VOL};
  for (int i = 0; i < TILE_COUNT; i++) {
    uiRound(parent, t->rule, tileX(i), at->y, tileW(), at->h, UI_TILE_R);
    sLabel[i] = uiLabel(parent, &roboto_small, t->dead);
    if (kNames[i] != STR_COUNT) {
      lv_label_set_text_static(sLabel[i], txt(kNames[i]));
    }
    uiShowIf(sLabel[i], kNames[i] != STR_COUNT);
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
  const int16_t lw = hasLabel ? uiTextWidth(sLabel[i], &roboto_small) : 0;
  const int16_t vw = hasValue ? uiTextWidth(sValue[i], &roboto_small) : 0;
  int16_t x = (int16_t)(tileX(i) + (tileW() - lw - vw) / 2);
  const int16_t base = (int16_t)(sAt.y + TILE_BASE);
  if (hasLabel) {
    uiBaseline(sLabel[i], &roboto_small, x, base);
    x = (int16_t)(x + lw);
  }
  uiBaseline(sValue[i], &roboto_small, x, base);
}

static void show(const ScreenState *s) {
  const Theme *t = themeCurrent();
  placeTile(0, s->tuneMode, t->dead, t->radio);
  placeTile(1, s->squelchMode, t->dead, t->radio);
  placeTile(2, s->filter, t->dead, t->radio);
  const bool muted = s->audio == SCREEN_AUDIO_MUTED;
  const bool held = s->audio == SCREEN_AUDIO_SQUELCHED;
  placeTile(3, muted ? txt(STR_RADIO_MUTE) : s->volume,
            muted ? t->fault : t->dead,
            muted  ? t->fault
            : held ? t->dead
                   : t->radio);
}

const Panel panelTiles = {begin, show};
