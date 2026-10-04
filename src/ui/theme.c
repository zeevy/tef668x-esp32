/*
 * The themes, packed for the panel from `core/palette.c`.
 *
 * The palettes are plain bytes so a PC test can check their contrast; this
 * turns each into the 16 bit colours the panel draws with, once, the first
 * time a theme is asked for.
 */
#include "theme.h"

#include <stdbool.h>
#include <stddef.h>

#include "../core/palette.h"
#include "../core/strings.h"

_Static_assert(THEME_CUSTOM_COLOUR_COUNT == PALETTE_ROLE_COUNT,
               "a theme stores one colour per palette role");
_Static_assert(THEME_COUNT == PALETTE_COUNT + 1,
               "the themes are the palettes and the Custom slot");
_Static_assert(THEME_CUSTOM == PALETTE_CUSTOM_THEME,
               "the theme and the palette tables agree where Custom is");

static Theme sThemes[PALETTE_COUNT];
static bool sPacked = false;

/*
 * The one custom slot. Rebuilt from the settings struct's `customTheme` by
 * `themeSetCustomColours`, never read from a palette.
 */
static Theme sCustom;

static uint8_t sCurrent = 0;
static uint32_t sGeneration = 0;

/* One theme from nine rows of red, green and blue, in role order. */
static void pack(Theme *t, const char *name,
                 const uint8_t rgb[THEME_CUSTOM_COLOUR_COUNT][3],
                 PaletteRole swatch, uint8_t scaleLift) {
  ThemeColour c[THEME_CUSTOM_COLOUR_COUNT];
  for (int i = 0; i < THEME_CUSTOM_COLOUR_COUNT; i++) {
    c[i] = THEME_RGB(rgb[i][0], rgb[i][1], rgb[i][2]);
  }
  t->name = name;
  t->ground = c[PALETTE_GROUND];
  t->header = c[PALETTE_HEADER];
  t->rule = c[PALETTE_RULE];
  t->radio = c[PALETTE_RADIO];
  t->broadcast = c[PALETTE_BROADCAST];
  t->measurement = c[PALETTE_MEASUREMENT];
  t->good = c[PALETTE_GOOD];
  t->fault = c[PALETTE_FAULT];
  t->dead = c[PALETTE_DEAD];
  uint8_t scale[3];
  paletteMix(rgb[PALETTE_DEAD], rgb[PALETTE_MEASUREMENT], scaleLift, scale);
  t->scale = THEME_RGB(scale[0], scale[1], scale[2]);
  t->swatch = c[swatch];
}

static void packPalettes(void) {
  if (sPacked) {
    return;
  }
  for (uint8_t i = 0; i < PALETTE_COUNT; i++) {
    const Palette *p = paletteAt(i);
    pack(&sThemes[i], txt(p->name), p->rgb, p->swatch, p->scaleLift);
  }
  sPacked = true;
}

const Theme *themeAt(uint8_t index) {
  packPalettes();
  const uint8_t palette = paletteOfTheme(index);
  if (palette != PALETTE_NONE) {
    return &sThemes[palette];
  }
  if (index == THEME_CUSTOM) {
    return &sCustom;
  }
  /* The default rather than nothing. A settings blob naming a theme this
   * build does not have would otherwise leave the screen with no colours
   * at all, which is a black panel and looks like broken hardware. */
  return &sThemes[0];
}

const Theme *themeCurrent(void) {
  return themeAt(sCurrent);
}

bool themeSet(uint8_t index) {
  if (index >= THEME_COUNT) {
    return false;
  }
  if (index != sCurrent) {
    sCurrent = index;
    sGeneration++;
  }
  return true;
}

uint32_t themeGeneration(void) {
  return sGeneration;
}

void themeSetCustomColours(const uint8_t rgb[THEME_CUSTOM_COLOUR_COUNT][3]) {
  if (rgb == NULL) {
    return;
  }
  /* Called on every settings apply, not only a colour wheel edit, so a
   * change is compared against what is already there rather than assumed:
   * bumping sGeneration on every call would trip screen.cpp's rebuild on
   * every knob tick of an unrelated setting, not just a Custom colour. */
  Theme next;
  pack(&next, txt(STR_THEME_CUSTOM), rgb, PALETTE_GOOD, 0);
  const bool changed =
      next.ground != sCustom.ground || next.header != sCustom.header ||
      next.rule != sCustom.rule || next.radio != sCustom.radio ||
      next.broadcast != sCustom.broadcast ||
      next.measurement != sCustom.measurement || next.good != sCustom.good ||
      next.fault != sCustom.fault || next.dead != sCustom.dead;
  sCustom = next;
  if (changed) {
    sGeneration++;
  }
}
