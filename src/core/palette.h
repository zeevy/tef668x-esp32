/*
 * The colours of the themes that ship, as plain red, green and blue bytes.
 *
 * Kept in core rather than in ui/theme.c so the contrast of every pair a
 * screen draws can be checked on a PC. ui/theme.c packs these into the
 * panel's own 16 bit colours; nothing here knows a panel.
 *
 * Fifteen themes, each with a job, which palette.c gives beside each one.
 * Every text colour reaches WCAG AA, 4.5:1, against what it is drawn on,
 * and a unit test checks every pair.
 */
#ifndef CORE_PALETTE_H
#define CORE_PALETTE_H

#include <stdint.h>

#include "core/strings.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The nine colour roles, in the order ui/theme.h's Theme struct lists its
 * colours and core/settings.h's `customTheme` stores its rows.
 */
typedef enum {
  PALETTE_GROUND = 0,  /* The background, and all type on an amber fill. */
  PALETTE_HEADER,      /* One step off ground. Kept for a filled header. */
  PALETTE_RULE,        /* The fill of a tile or a menu row. */
  PALETTE_RADIO,       /* What the radio is set to: panel fill, band, values. */
  PALETTE_BROADCAST,   /* What the station sends: the radio text. */
  PALETTE_MEASUREMENT, /* A live reading, such as the clock. */
  PALETTE_GOOD,        /* Joined, charged, the saved choice. */
  PALETTE_FAULT,       /* Muted, low battery. */
  PALETTE_DEAD,        /* Labels, icons at rest, the date, the scale. */
  PALETTE_ROLE_COUNT
} PaletteRole;

typedef struct {
  StrId name; /* What the menu and the web page call it. */
  uint8_t rgb[PALETTE_ROLE_COUNT][3];
  /*
   * The fourth of the four swatches the theme picker shows after ground,
   * radio and broadcast: `good`, or `dead` where good would repeat a colour
   * already shown.
   */
  PaletteRole swatch;
  /*
   * How far the tuning scale moves from `dead` towards `measurement`, out of
   * 255. In `dead` its one pixel marks and small numbers read dim on the
   * glass though their contrast passes, so most themes lift it halfway. Red
   * Night keeps it at 0: a brighter scale there is more light, much of it
   * green, against the dark adaptation the theme exists for.
   */
  uint8_t scaleLift;
} Palette;

/* Nightwatch, Daylight, Red Night, Phosphor and Clear, then Slate, Paper,
 * LCD, Ember, Clear Day, High Contrast, Mono, Hi-Fi, Violet and Blossom. */
#define PALETTE_COUNT 15

/* One of them, by index. Out of range gives Nightwatch, the first. */
const Palette *paletteAt(uint8_t index);

/*
 * The saved theme index of Custom, which is not a palette. The first five
 * palettes are saved as 0 to 4 and the ten after them as 6 to 15, so an
 * index saved before the ten existed still names the theme it did.
 */
#define PALETTE_CUSTOM_THEME 5

/* The saved theme indexes a new radio, or one whose settings were erased,
 * starts with: Clear Day by day and Nightwatch by night. A radio that has
 * themes saved keeps them. */
#define PALETTE_THEME_DEFAULT_DAY 10
#define PALETTE_THEME_DEFAULT_NIGHT 0

/* What `paletteOfTheme` gives for Custom and for an index past the end. */
#define PALETTE_NONE 0xFF

/* The palette a saved theme index draws with, or PALETTE_NONE. */
uint8_t paletteOfTheme(uint8_t theme);

/*
 * The order the menu and the web page list the themes in, grouped by when
 * each is used, Custom last. `paletteThemeAt` is the saved theme index at a
 * place in that list and `palettePlaceOf` the place of a saved index; one
 * past the end of either gives 0, the first place and Nightwatch.
 */
uint8_t paletteThemeAt(uint8_t place);
uint8_t palettePlaceOf(uint8_t theme);

/* The colour `lift` of the way, out of 255, from `from` to `to`. */
void paletteMix(const uint8_t from[3], const uint8_t to[3], uint8_t lift,
                uint8_t out[3]);

#ifdef __cplusplus
}
#endif

#endif /* CORE_PALETTE_H */
