/* The bandwidth page's view. See screen_bw_state.h. */
#include "screen_bw_state.h"

#include <stdio.h>
#include <string.h>

#include "core/strings.h"

void screenBwStateBuild(const ScreenBwInputs *in, ScreenBwKeep *keep,
                        ScreenBw *out) {
  if (in == NULL || keep == NULL || out == NULL) {
    return;
  }
  memset(out, 0, sizeof(*out));
  const uint8_t count =
      in->count < SCREEN_BW_TILES ? in->count : (uint8_t)SCREEN_BW_TILES;
  snprintf(keep->context, sizeof(keep->context), txt(STR_BW_FMT_CONTEXT),
           bandName(in->band));
  out->context = keep->context;
  snprintf(keep->position, sizeof(keep->position), "%u/%u",
           (unsigned)(in->cursor + 1), (unsigned)count);
  out->position = keep->position;
  out->clock = in->clock;
  out->count = count;
  for (uint8_t i = 0; i < count && in->tiles != NULL; i++) {
    const BwTile *t = &in->tiles[i];
    ScreenBwTile *k = &out->tile[i];
    k->cursor = i == in->cursor;
    if (t->kind == BW_TILE_WIDTH) {
      k->kind = SCREEN_BW_WIDTH;
      if (t->khz == 0) {
        snprintf(keep->text[i], sizeof(keep->text[i]), "%s",
                 txt(STR_COMMON_AUTO));
      } else {
        snprintf(keep->text[i], sizeof(keep->text[i]), "%u", (unsigned)t->khz);
      }
      k->filled = t->khz == in->widthKHz;
    } else {
      const bool ims = t->kind == BW_TILE_IMS;
      k->kind = ims ? SCREEN_BW_IMS : SCREEN_BW_EQ;
      const bool on = ims ? in->ims : in->eq;
      snprintf(keep->text[i], sizeof(keep->text[i]),
               txt(STR_COMMON_FMT_TWO_WORDS),
               txt(ims ? STR_COMMON_IMS : STR_BW_EQ),
               txt(on ? STR_COMMON_ON : STR_COMMON_OFF));
      k->filled = on;
      out->hasSwitches = true;
    }
    k->text = keep->text[i];
  }
  /* What automatic has settled on, from the tuner's own reading, only
   * while automatic is in use: at a fixed width the tile says it. */
  if (!in->dxMode && in->widthKHz == 0 && in->chipKnown) {
    snprintf(keep->note, sizeof(keep->note), txt(STR_BW_FMT_AUTO_AT),
             (unsigned)in->chipKHz);
    out->note = keep->note;
  }
}
