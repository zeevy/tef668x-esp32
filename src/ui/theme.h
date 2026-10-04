/*
 * The colours a screen is allowed to use.
 *
 * A theme is a set of named roles, and no screen ever names a colour. That
 * is what lets each one ship as data rather than as its own copy of the
 * drawing code, and it is why every field below says what the colour is for
 * instead of what it looks like. There are nine roles.
 *
 * A new radio draws Clear Day by day and Nightwatch by night,
 * PALETTE_THEME_DEFAULT_DAY and _NIGHT. The values themselves live in
 * `core/palette.c`, where a PC test checks every pair a screen draws for
 * contrast, and this file packs them for the panel. Adding a theme is adding
 * a palette there. Nothing else changes.
 */
#ifndef UI_THEME_H
#define UI_THEME_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A colour as the panel wants it: five red, six green, five blue. */
typedef uint16_t ThemeColour;

#define THEME_RGB(r, g, b)                       \
  ((ThemeColour)((((uint16_t)(r) & 0xF8) << 8) | \
                 (((uint16_t)(g) & 0xFC) << 3) | ((b) >> 3)))

/*
 * How many colours a theme stores: the nine roles, in the order the `Theme`
 * struct below lists them. `core/settings.h`'s `customTheme` holds at least
 * this many rows in the same order, so a caller filling one in never has to
 * know the count by another name.
 */
#define THEME_CUSTOM_COLOUR_COUNT 9

typedef struct {
  const char *name; /* What it is called in the menu. */

  ThemeColour ground; /* The background, and all type on an amber fill. */
  /* No screen draws with it now. It stays so the stored custom theme keeps
   * its nine colours in the same order. */
  ThemeColour header;
  ThemeColour rule; /* The fill of a tile or a menu row. */

  /*
   * The six that carry meaning.
   *
   * `radio` is what the radio is set to and what it is doing: the
   * frequency's panel, the band name, anything the listener set. `broadcast`
   * is what is arriving from outside: the radio text. `measurement` is a
   * live number the tuner or the clock is reporting right now. `good` is a
   * network joined, a battery charged, a saved choice. `fault` is muted, low,
   * given up. `dead` is something that cannot answer or is switched off; a
   * label sits here too, since a word naming a number is neither reporting
   * nor asking for attention.
   */
  ThemeColour radio;
  ThemeColour broadcast;
  ThemeColour measurement;
  ThemeColour good;
  ThemeColour fault;
  ThemeColour dead;

  /* The tuning scale: `dead` lifted towards `measurement` by the palette's
   * own amount. Not stored; Custom's is its `dead`, since there a person
   * picks every colour and a mix of two unlike ones can come out darker. */
  ThemeColour scale;

  /* The theme picker's fourth swatch, one of the roles above. Not stored:
   * `core/palette.h` says which role it is, and Custom's is `good`. */
  ThemeColour swatch;
} Theme;

/*
 * How many ship: the fifteen palettes of `core/palette.h` and the one Custom
 * slot, which is not a palette but the colours kept in the settings struct.
 * An index is what the settings save, and Custom's is `THEME_CUSTOM`, in the
 * middle: the first five palettes are 0 to 4 and the ten after them 6 to 15.
 * The order a list shows them in is `paletteThemeAt`'s.
 */
#define THEME_COUNT 16
#define THEME_CUSTOM 5

/* One of them, by index. Out of range gives the default rather than NULL. */
const Theme *themeAt(uint8_t index);

/* The one screens draw with now. Never NULL. */
const Theme *themeCurrent(void);

/* Choose one. Out of range is refused and the current one is kept. */
bool themeSet(uint8_t index);

/*
 * Load the colours the Custom slot draws with, in the order this struct
 * lists its own fields.
 *
 * Kept separately from `themeSet` because the two questions are different:
 * this is "what Custom currently looks like", asked once at start up and
 * again whenever the web page changes a colour, and `themeSet` is "which
 * theme is active", asked far less often. A radio with Custom active still
 * needs this called before `themeCurrent` reads anything sensible out of it.
 */
void themeSetCustomColours(const uint8_t rgb[THEME_CUSTOM_COLOUR_COUNT][3]);

/*
 * Goes up every time `themeSet` or `themeSetCustomColours` changes anything.
 *
 * For deciding whether a repaint is owed. A pointer comparison against
 * `themeCurrent()`'s own result cannot see a colour wheel edit to the theme
 * that is already active: Custom is always the same struct address, only
 * its fields change. A caller comparing this instead sees both kinds of
 * change with one number.
 */
uint32_t themeGeneration(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_THEME_H */
