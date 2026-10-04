/*
 * The fifteen themes.
 *
 * Every text colour here reaches a contrast of 4.5:1 against the background
 * it is drawn on.
 */
#include "palette.h"

#define RGB(v) {(uint8_t)((v) >> 16), (uint8_t)((v) >> 8), (uint8_t)(v)}

static const Palette kPalettes[PALETTE_COUNT] = {
    /* Indoors and the evening. Amber on near black. */
    {STR_THEME_NIGHTWATCH,
     {RGB(0x0B0F13), RGB(0x0A0F14), RGB(0x1E2630), RGB(0xFFB200), RGB(0x4AC2EE),
      RGB(0xE6E6E6), RGB(0x31C29C), RGB(0xFF5D4A), RGB(0x8B95A4)},
     PALETTE_GOOD,
     128},
    /* Direct sun. Dark type on a light ground holds its contrast against
     * reflections off the glass, where light type on black washes out. */
    {STR_THEME_DAYLIGHT,
     {RGB(0xF4F1E8), RGB(0xE9E4D6), RGB(0xE4DECF), RGB(0x9C4C00), RGB(0x005A96),
      RGB(0x101418), RGB(0x0A7050), RGB(0xB8231A), RGB(0x565F6C)},
     PALETTE_GOOD,
     128},
    /* A dark sky. Only red and orange, light above about 620 nm, which
     * barely affects the eye's adjustment to the dark. Good and fault differ
     * in brightness and in icon shape, since hue cannot separate them. */
    {STR_THEME_RED_NIGHT,
     {RGB(0x000000), RGB(0x070000), RGB(0x240805), RGB(0xFF3A1A), RGB(0xFF8A66),
      RGB(0xFFB49E), RGB(0xE0583A), RGB(0xFF1500), RGB(0xCC583F)},
     PALETTE_DEAD,
     0},
    /* The highest contrast of the five. Green reads brightest to the eye;
     * fault stays red so a warning still stands out. */
    {STR_THEME_PHOSPHOR,
     {RGB(0x010A04), RGB(0x021007), RGB(0x0D2A17), RGB(0x3DFF8A), RGB(0xA6FFD0),
      RGB(0xE0FFEC), RGB(0x3DFF8A), RGB(0xFF5D4A), RGB(0x63A67E)},
     PALETTE_DEAD,
     128},
    /* Safe for colour blind eyes: Okabe-Ito hues, and fault is orange, so
     * good and fault never rely on red against green. */
    {STR_THEME_CLEAR,
     {RGB(0x000000), RGB(0x06080B), RGB(0x17222D), RGB(0x56B4E9), RGB(0xF0E442),
      RGB(0xF2F2F2), RGB(0x2BB58C), RGB(0xE69F00), RGB(0x98A2AE)},
     PALETTE_GOOD,
     128},
    /* Long evenings. A dark grey ground rather than near black, which a
     * bright panel on black can blur for eyes with astigmatism, and soft
     * hues. */
    {STR_THEME_SLATE,
     {RGB(0x23272E), RGB(0x2A2F38), RGB(0x353B47), RGB(0x88C0D0), RGB(0xA3BE8C),
      RGB(0xE5E9F0), RGB(0xC0F0FF), RGB(0xF0A070), RGB(0xA8AEB8)},
     PALETTE_GOOD,
     128},
    /* Hard sun. Black and white, the largest step this panel can make, with
     * one red, as on e-paper. Good is black like the panel, so the picker
     * shows dead. */
    {STR_THEME_PAPER,
     {RGB(0xFFFFFF), RGB(0xF4F4F4), RGB(0xE0E0E0), RGB(0x000000), RGB(0x404040),
      RGB(0x000000), RGB(0x000000), RGB(0xA8002A), RGB(0x5C5C5C)},
     PALETTE_DEAD,
     128},
    /* Shade and bright rooms. A backlit segment LCD: dark ink on grey green,
     * less glare than white. Fault is magenta, which protan eyes still tell
     * from the dark ink where a dark red would not be. */
    {STR_THEME_LCD,
     {RGB(0xCED8BC), RGB(0xC2CCAE), RGB(0xB8C4A2), RGB(0x2C3A24), RGB(0x1E3250),
      RGB(0x101810), RGB(0x1E4A10), RGB(0x7A1060), RGB(0x4A4A4A)},
     PALETTE_GOOD,
     128},
    /* The bedside. No blue but in the fault colour, and a panel giving about
     * half the light of Nightwatch's. Fault is pink rather than red, which
     * red and green colour blind eyes could not tell from the orange panel.
     * The scale keeps dead for Red Night's reason. */
    {STR_THEME_EMBER,
     {RGB(0x080400), RGB(0x100800), RGB(0x321A00), RGB(0xE07000), RGB(0xF5B800),
      RGB(0xFFD800), RGB(0xB4C800), RGB(0xFF6A9A), RGB(0xB48400)},
     PALETTE_GOOD,
     0},
    /* Clear for outdoors: the same Okabe-Ito hues, darkened until each
     * reads on a white ground. */
    {STR_THEME_CLEAR_DAY,
     {RGB(0xFFFFFF), RGB(0xF0F2F4), RGB(0xE4E8EC), RGB(0x0062A0), RGB(0x8A3A6C),
      RGB(0x101418), RGB(0x006E50), RGB(0xA33200), RGB(0x5A6470)},
     PALETTE_GOOD,
     128},
    /* Low vision and older eyes. Yellow on black, every pair 7:1 or more,
     * WCAG AAA, and no text in blue, which older lenses dim. */
    {STR_THEME_HIGH_CONTRAST,
     {RGB(0x080808), RGB(0x101010), RGB(0x202020), RGB(0xFFE14D), RGB(0xFFFFFF),
      RGB(0xFFFFFF), RGB(0x6CCBFF), RGB(0xFF8A6A), RGB(0xB4B4B4)},
     PALETTE_GOOD,
     128},
    /* No colour at all, for a colour cast, a panel seen off its angle or no
     * colour vision. Rank is brightness alone, and a fault is the brightest
     * grey, white. */
    {STR_THEME_MONO,
     {RGB(0x080808), RGB(0x101010), RGB(0x202020), RGB(0xB8B8B8), RGB(0xE0E0E0),
      RGB(0xE8E8E8), RGB(0xC4C4C4), RGB(0xFFFFFF), RGB(0x8C8C8C)},
     PALETTE_GOOD,
     128},
    /* The blue green of a hi-fi tuner's fluorescent display. One hue, like
     * Phosphor. Not for a dark sky: this is the colour the eye's night
     * vision answers to most. */
    {STR_THEME_HI_FI,
     {RGB(0x081010), RGB(0x0C1616), RGB(0x143030), RGB(0x2EE6D6), RGB(0xA8FFF4),
      RGB(0xE6FFFB), RGB(0x7DF2A0), RGB(0xFF8A5A), RGB(0x5FA8A0)},
     PALETTE_GOOD,
     128},
    /* A look. Lavender on deep plum. Fault is pink red, where a plain red
     * falls under 4.5:1 on the tile for protan eyes. */
    {STR_THEME_VIOLET,
     {RGB(0x140F1E), RGB(0x1A1426), RGB(0x2C2440), RGB(0xBD93F9), RGB(0x8BE9FD),
      RGB(0xF8F8F2), RGB(0x50FA7B), RGB(0xFF7A90), RGB(0x9AA3D0)},
     PALETTE_GOOD,
     128},
    /* A look. Pink on near black, with an orange fault that stays apart from
     * the pink panel for red and green colour blind eyes. */
    {STR_THEME_BLOSSOM,
     {RGB(0x100A0E), RGB(0x180C14), RGB(0x2E1A28), RGB(0xFF8FC0), RGB(0xA8DCFF),
      RGB(0xFFEEF6), RGB(0x6EE2B0), RGB(0xFF7A30), RGB(0xB08CA4)},
     PALETTE_GOOD,
     128},
};

/* The saved theme index at each place in the list, by when each is used:
 * everyday and sun, then night and access, then looks, then Custom. */
static const uint8_t kOrder[PALETTE_COUNT + 1] = {
    0,  /* Nightwatch */
    6,  /* Slate */
    1,  /* Daylight */
    7,  /* Paper */
    8,  /* LCD */
    2,  /* Red Night */
    9,  /* Ember */
    4,  /* Clear */
    10, /* Clear Day */
    11, /* High Contrast */
    12, /* Mono */
    3,  /* Phosphor */
    13, /* Hi-Fi */
    14, /* Violet */
    15, /* Blossom */
    PALETTE_CUSTOM_THEME,
};

const Palette *paletteAt(uint8_t index) {
  return index < PALETTE_COUNT ? &kPalettes[index] : &kPalettes[0];
}

uint8_t paletteOfTheme(uint8_t theme) {
  if (theme < PALETTE_CUSTOM_THEME) {
    return theme;
  }
  if (theme > PALETTE_CUSTOM_THEME && theme <= PALETTE_COUNT) {
    return (uint8_t)(theme - 1);
  }
  return PALETTE_NONE;
}

uint8_t paletteThemeAt(uint8_t place) {
  return place <= PALETTE_COUNT ? kOrder[place] : kOrder[0];
}

uint8_t palettePlaceOf(uint8_t theme) {
  for (uint8_t place = 0; place <= PALETTE_COUNT; place++) {
    if (kOrder[place] == theme) {
      return place;
    }
  }
  return 0;
}

void paletteMix(const uint8_t from[3], const uint8_t to[3], uint8_t lift,
                uint8_t out[3]) {
  for (int i = 0; i < 3; i++) {
    out[i] = (uint8_t)((from[i] * (255 - lift) + to[i] * lift + 127) / 255);
  }
}
