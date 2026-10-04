/*
 * Tests for the theme palettes. Runs on a PC.
 *
 * The contrast test is a gate: every pair of colours a screen draws text or an
 * icon with must reach WCAG AA, 4.5:1, in every theme. A palette edit that
 * drops one pair under it fails here rather than on the glass.
 */
#include <unity.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core/palette.h"
#include "core/strings.h"

void setUp(void) {}
void tearDown(void) {}

/* WCAG 2 relative luminance of one colour. */
static double luminance(const uint8_t rgb[3]) {
  double c[3];
  for (int i = 0; i < 3; i++) {
    const double v = rgb[i] / 255.0;
    c[i] = v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4);
  }
  return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2];
}

static double contrast(const uint8_t a[3], const uint8_t b[3]) {
  const double la = luminance(a);
  const double lb = luminance(b);
  return la > lb ? (la + 0.05) / (lb + 0.05) : (lb + 0.05) / (la + 0.05);
}

/*
 * What is drawn on what. Text and icons alike: an icon at 16 pixels carries
 * a state the same way a word does, so it is held to the same ratio.
 */
static const PaletteRole kPairs[][2] = {
    {PALETTE_GROUND, PALETTE_RADIO},       /* panel text, cursor row */
    {PALETTE_RADIO, PALETTE_GROUND},       /* band name, title */
    {PALETTE_BROADCAST, PALETTE_GROUND},   /* radio text */
    {PALETTE_MEASUREMENT, PALETTE_GROUND}, /* clock */
    {PALETTE_DEAD, PALETTE_GROUND},        /* date, hints */
    {PALETTE_GOOD, PALETTE_GROUND},        /* Wi-Fi joined */
    {PALETTE_FAULT, PALETTE_GROUND},       /* muted, low battery */
    {PALETTE_RADIO, PALETTE_RULE},         /* tile and row values */
    {PALETTE_DEAD, PALETTE_RULE},          /* tile labels, group counts */
    {PALETTE_FAULT, PALETTE_RULE},         /* VOL when muted */
    {PALETTE_MEASUREMENT, PALETTE_RULE},   /* menu row labels */
    {PALETTE_GOOD, PALETTE_RULE},          /* the tick on a saved choice */
};

static void every_pair_reaches_wcag_aa_in_every_theme(void) {
  for (uint8_t t = 0; t < PALETTE_COUNT; t++) {
    const Palette *p = paletteAt(t);
    for (size_t i = 0; i < sizeof(kPairs) / sizeof(kPairs[0]); i++) {
      const double r = contrast(p->rgb[kPairs[i][0]], p->rgb[kPairs[i][1]]);
      char why[64];
      snprintf(why, sizeof(why), "%s pair %u is %.2f:1", txt(p->name),
               (unsigned)i, r);
      TEST_ASSERT_TRUE_MESSAGE(r >= 4.5, why);
    }
  }
}

static void the_fifteen_ship_in_order(void) {
  static const char *const kNames[PALETTE_COUNT] = {
      "Nightwatch",    "Daylight", "Red Night", "Phosphor", "Clear",
      "Slate",         "Paper",    "LCD",       "Ember",    "Clear Day",
      "High Contrast", "Mono",     "Hi-Fi",     "Violet",   "Blossom"};
  for (uint8_t i = 0; i < PALETTE_COUNT; i++) {
    TEST_ASSERT_EQUAL_STRING(kNames[i], txt(paletteAt(i)->name));
  }
}

/* Theme numbers 0 to 4 are the first five palettes, 5 is Custom, and 6 to 15
 * are the rest, so a saved theme number always names the same theme. */
static void a_saved_index_names_the_same_theme_as_before(void) {
  for (uint8_t t = 0; t < 5; t++) {
    TEST_ASSERT_EQUAL_UINT8(t, paletteOfTheme(t));
  }
  TEST_ASSERT_EQUAL_UINT8(PALETTE_NONE, paletteOfTheme(PALETTE_CUSTOM_THEME));
  TEST_ASSERT_EQUAL_UINT8(5, paletteOfTheme(6));
  TEST_ASSERT_EQUAL_UINT8(PALETTE_COUNT - 1, paletteOfTheme(PALETTE_COUNT));
  TEST_ASSERT_EQUAL_UINT8(PALETTE_NONE, paletteOfTheme(PALETTE_COUNT + 1));
  TEST_ASSERT_EQUAL_UINT8(PALETTE_NONE, paletteOfTheme(255));
}

/* The list shows every theme once, Nightwatch first and Custom last, and a
 * place and its theme lead back to each other. */
static void the_list_order_holds_every_theme_once(void) {
  bool seen[PALETTE_COUNT + 1] = {false};
  for (uint8_t place = 0; place <= PALETTE_COUNT; place++) {
    const uint8_t theme = paletteThemeAt(place);
    TEST_ASSERT_TRUE(theme <= PALETTE_COUNT);
    TEST_ASSERT_FALSE(seen[theme]);
    seen[theme] = true;
    TEST_ASSERT_EQUAL_UINT8(place, palettePlaceOf(theme));
  }
  TEST_ASSERT_EQUAL_UINT8(0, paletteThemeAt(0));
  TEST_ASSERT_EQUAL_UINT8(PALETTE_CUSTOM_THEME, paletteThemeAt(PALETTE_COUNT));
}

static void a_place_or_theme_past_the_end_gives_the_first(void) {
  TEST_ASSERT_EQUAL_UINT8(0, paletteThemeAt(PALETTE_COUNT + 1));
  TEST_ASSERT_EQUAL_UINT8(0, paletteThemeAt(255));
  TEST_ASSERT_EQUAL_UINT8(0, palettePlaceOf(PALETTE_COUNT + 1));
  TEST_ASSERT_EQUAL_UINT8(0, palettePlaceOf(255));
}

static void an_index_past_the_end_gives_the_default(void) {
  TEST_ASSERT_EQUAL_PTR(paletteAt(0), paletteAt(PALETTE_COUNT));
  TEST_ASSERT_EQUAL_PTR(paletteAt(0), paletteAt(255));
}

static void nightwatch_keeps_its_amber_and_ground(void) {
  const Palette *p = paletteAt(0);
  const uint8_t radio[3] = {0xFF, 0xB2, 0x00};
  const uint8_t ground[3] = {0x0B, 0x0F, 0x13};
  TEST_ASSERT_EQUAL_UINT8_ARRAY(radio, p->rgb[PALETTE_RADIO], 3);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(ground, p->rgb[PALETTE_GROUND], 3);
}

/* The picker's fourth swatch shows a colour the first three do not, so no
 * theme's row reads as three colours. */
static void the_fourth_swatch_is_a_new_colour(void) {
  for (uint8_t t = 0; t < PALETTE_COUNT; t++) {
    const Palette *p = paletteAt(t);
    const uint8_t *four = p->rgb[p->swatch];
    const PaletteRole shown[] = {PALETTE_GROUND, PALETTE_RADIO,
                                 PALETTE_BROADCAST};
    for (size_t i = 0; i < 3; i++) {
      TEST_ASSERT_FALSE_MESSAGE(memcmp(four, p->rgb[shown[i]], 3) == 0,
                                txt(p->name));
    }
  }
}

static void a_mix_runs_from_one_colour_to_the_other(void) {
  const uint8_t from[3] = {0x8B, 0x95, 0xA4};
  const uint8_t to[3] = {0xE6, 0xE6, 0xE6};
  uint8_t out[3];
  paletteMix(from, to, 0, out);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(from, out, 3);
  paletteMix(from, to, 255, out);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(to, out, 3);
  paletteMix(from, to, 128, out);
  const uint8_t half[3] = {0xB9, 0xBE, 0xC5};
  TEST_ASSERT_EQUAL_UINT8_ARRAY(half, out, 3);
}

/* The scale is lifted to be read more easily, so a lift that left it darker
 * than `dead`, or under the floor every other text meets, would undo that. */
static void the_scale_is_never_dimmer_than_dead(void) {
  for (uint8_t t = 0; t < PALETTE_COUNT; t++) {
    const Palette *p = paletteAt(t);
    uint8_t scale[3];
    paletteMix(p->rgb[PALETTE_DEAD], p->rgb[PALETTE_MEASUREMENT], p->scaleLift,
               scale);
    const double lifted = contrast(scale, p->rgb[PALETTE_GROUND]);
    const double dead = contrast(p->rgb[PALETTE_DEAD], p->rgb[PALETTE_GROUND]);
    char why[64];
    snprintf(why, sizeof(why), "%s scale is %.2f:1 against %.2f:1",
             txt(p->name), lifted, dead);
    TEST_ASSERT_TRUE_MESSAGE(lifted >= dead && lifted >= 4.5, why);
  }
}

static void red_night_keeps_its_scale_in_dead(void) {
  TEST_ASSERT_EQUAL_STRING("Red Night", txt(paletteAt(2)->name));
  TEST_ASSERT_EQUAL_UINT8(0, paletteAt(2)->scaleLift);
}

/* Ember is for the bedside: no blue in any role but fault, and its scale
 * keeps dead, since lifting it toward the clock adds light. */
static void ember_has_no_blue_but_its_fault(void) {
  const Palette *p = paletteAt(8);
  TEST_ASSERT_EQUAL_STRING("Ember", txt(p->name));
  for (uint8_t r = 0; r < PALETTE_ROLE_COUNT; r++) {
    if (r != PALETTE_FAULT) {
      TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, p->rgb[r][2], "a role with blue");
    }
  }
  TEST_ASSERT_EQUAL_UINT8(0, p->scaleLift);
}

/* The default themes are the ones named: Clear Day by day, Nightwatch by
 * night. */
static void the_default_themes_are_clear_day_and_nightwatch(void) {
  TEST_ASSERT_EQUAL_STRING(
      "Clear Day",
      txt(paletteAt(paletteOfTheme(PALETTE_THEME_DEFAULT_DAY))->name));
  TEST_ASSERT_EQUAL_STRING(
      "Nightwatch",
      txt(paletteAt(paletteOfTheme(PALETTE_THEME_DEFAULT_NIGHT))->name));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(every_pair_reaches_wcag_aa_in_every_theme);
  RUN_TEST(the_fifteen_ship_in_order);
  RUN_TEST(a_saved_index_names_the_same_theme_as_before);
  RUN_TEST(the_list_order_holds_every_theme_once);
  RUN_TEST(a_place_or_theme_past_the_end_gives_the_first);
  RUN_TEST(an_index_past_the_end_gives_the_default);
  RUN_TEST(nightwatch_keeps_its_amber_and_ground);
  RUN_TEST(the_fourth_swatch_is_a_new_colour);
  RUN_TEST(a_mix_runs_from_one_colour_to_the_other);
  RUN_TEST(the_scale_is_never_dimmer_than_dead);
  RUN_TEST(red_night_keeps_its_scale_in_dead);
  RUN_TEST(ember_has_no_blue_but_its_fault);
  RUN_TEST(the_default_themes_are_clear_day_and_nightwatch);
  return UNITY_END();
}
