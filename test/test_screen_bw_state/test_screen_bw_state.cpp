/*
 * Tests for the bandwidth page's view, built from the page's tiles and the
 * radio's snapshot. Runs on a PC.
 */
#include <unity.h>

#include <stdint.h>
#include <string.h>

#include "core/band_plan.h"
#include "core/bw_page.h"
#include "core/strings.h"
#include "screen_bw_state.h"

/* The separator the page puts between the band and the unit. */
#define DOT " \xC2\xB7 "

static BwTile tiles[BW_PAGE_MAX];
static ScreenBwKeep keep;
static ScreenBw view;

void setUp(void) {
  memset(tiles, 0, sizeof(tiles));
  memset(&keep, 0, sizeof(keep));
  memset(&view, 0, sizeof(view));
}
void tearDown(void) {}

/* What the screen task hands over: the band's tiles, the cursor on the
 * first, automatic in use, both switches off. */
static ScreenBwInputs inputs(BandId band, bool dxMode) {
  ScreenBwInputs in;
  memset(&in, 0, sizeof(in));
  in.count = bwPageTiles(band, dxMode, tiles, BW_PAGE_MAX);
  in.tiles = tiles;
  in.band = band;
  in.dxMode = dxMode;
  in.clock = "19:32";
  return in;
}

static void build(const ScreenBwInputs *in) {
  screenBwStateBuild(in, &keep, &view);
}

/* How many tiles are filled, and how many carry the cursor. */
static uint8_t filledCount(void) {
  uint8_t n = 0;
  for (uint8_t i = 0; i < view.count; i++) {
    n = (uint8_t)(n + (view.tile[i].filled ? 1 : 0));
  }
  return n;
}

static uint8_t cursorCount(void) {
  uint8_t n = 0;
  for (uint8_t i = 0; i < view.count; i++) {
    n = (uint8_t)(n + (view.tile[i].cursor ? 1 : 0));
  }
  return n;
}

/* ------------------------------------------------------- guarded inputs */

static void a_missing_pointer_does_not_crash(void) {
  const ScreenBwInputs in = inputs(BAND_FM, false);
  screenBwStateBuild(NULL, &keep, &view);
  screenBwStateBuild(&in, NULL, &view);
  screenBwStateBuild(&in, &keep, NULL);
  screenBwStateBuild(NULL, NULL, NULL);
}

/* No tiles to read: the header is still filled in, and no tile has a
 * text. */
static void no_tiles_still_fills_the_header(void) {
  ScreenBwInputs in = inputs(BAND_FM, false);
  in.tiles = NULL;
  build(&in);
  TEST_ASSERT_EQUAL_STRING("FM" DOT "kHz", view.context);
  TEST_ASSERT_EQUAL_STRING("1/19", view.position);
  for (uint8_t i = 0; i < SCREEN_BW_TILES; i++) {
    TEST_ASSERT_NULL(view.tile[i].text);
  }
  TEST_ASSERT_FALSE(view.hasSwitches);
}

/* A band that is not one gives no tiles and no band name. */
static void a_band_that_is_not_one_gives_an_empty_page(void) {
  ScreenBwInputs in = inputs((BandId)99, false);
  TEST_ASSERT_EQUAL_UINT8(0, in.count);
  build(&in);
  TEST_ASSERT_EQUAL_UINT8(0, view.count);
  TEST_ASSERT_EQUAL_STRING(DOT "kHz", view.context);
  TEST_ASSERT_EQUAL_STRING("1/0", view.position);
  TEST_ASSERT_FALSE(view.hasSwitches);
}

/* More tiles than the page holds: the page takes what fits. */
static void more_tiles_than_the_page_holds_are_cut(void) {
  static BwTile many[SCREEN_BW_TILES + 6];
  for (uint8_t i = 0; i < SCREEN_BW_TILES + 6; i++) {
    many[i].kind = BW_TILE_WIDTH;
    many[i].khz = (uint16_t)(100 + i);
  }
  ScreenBwInputs in = inputs(BAND_FM, false);
  in.tiles = many;
  in.count = SCREEN_BW_TILES + 6;
  build(&in);
  TEST_ASSERT_EQUAL_UINT8(SCREEN_BW_TILES, view.count);
  TEST_ASSERT_EQUAL_STRING("1/19", view.position);
  TEST_ASSERT_EQUAL_STRING("118", view.tile[SCREEN_BW_TILES - 1].text);
}

/* ------------------------------------------------------------- FM */

static void fm_lists_automatic_first_then_the_widths_and_the_switches(void) {
  const ScreenBwInputs in = inputs(BAND_FM, false);
  build(&in);
  TEST_ASSERT_EQUAL_STRING("FM" DOT "kHz", view.context);
  TEST_ASSERT_EQUAL_STRING("1/19", view.position);
  TEST_ASSERT_EQUAL_STRING("19:32", view.clock);
  TEST_ASSERT_EQUAL_UINT8(19, view.count);
  TEST_ASSERT_TRUE(view.hasSwitches);
  TEST_ASSERT_EQUAL_UINT8(SCREEN_BW_WIDTH, view.tile[0].kind);
  TEST_ASSERT_EQUAL_STRING("AUTO", view.tile[0].text);
  TEST_ASSERT_EQUAL_STRING("56", view.tile[1].text);
  TEST_ASSERT_EQUAL_STRING("114", view.tile[6].text);
  TEST_ASSERT_EQUAL_STRING("311", view.tile[16].text);
  for (uint8_t i = 0; i < 17; i++) {
    TEST_ASSERT_EQUAL_UINT8(SCREEN_BW_WIDTH, view.tile[i].kind);
  }
  TEST_ASSERT_EQUAL_UINT8(SCREEN_BW_IMS, view.tile[17].kind);
  TEST_ASSERT_EQUAL_STRING("iMS Off", view.tile[17].text);
  TEST_ASSERT_EQUAL_UINT8(SCREEN_BW_EQ, view.tile[18].kind);
  TEST_ASSERT_EQUAL_STRING("EQ Off", view.tile[18].text);
}

/* On automatic, the automatic tile is the one filled, and the cursor sits
 * on it. */
static void automatic_in_use_fills_its_own_tile(void) {
  ScreenBwInputs in = inputs(BAND_FM, false);
  in.cursor = bwPageStart(tiles, in.count, 0);
  build(&in);
  TEST_ASSERT_TRUE(view.tile[0].filled);
  TEST_ASSERT_TRUE(view.tile[0].cursor);
  TEST_ASSERT_EQUAL_UINT8(1, filledCount());
  TEST_ASSERT_EQUAL_UINT8(1, cursorCount());
}

static void the_width_in_use_is_filled_and_the_cursor_starts_on_it(void) {
  ScreenBwInputs in = inputs(BAND_FM, false);
  in.widthKHz = 114;
  in.cursor = bwPageStart(tiles, in.count, 114);
  build(&in);
  TEST_ASSERT_EQUAL_STRING("7/19", view.position);
  TEST_ASSERT_EQUAL_STRING("114", view.tile[6].text);
  TEST_ASSERT_TRUE(view.tile[6].filled);
  TEST_ASSERT_TRUE(view.tile[6].cursor);
  TEST_ASSERT_FALSE(view.tile[0].filled);
  TEST_ASSERT_EQUAL_UINT8(1, filledCount());
  TEST_ASSERT_EQUAL_UINT8(1, cursorCount());
}

/* The cursor moved off the width in use: the filled tile stays where the
 * width is, and the ring moves. */
static void the_cursor_can_sit_on_another_tile(void) {
  ScreenBwInputs in = inputs(BAND_FM, false);
  in.widthKHz = 114;
  in.cursor = 17;
  build(&in);
  TEST_ASSERT_EQUAL_STRING("18/19", view.position);
  TEST_ASSERT_TRUE(view.tile[17].cursor);
  TEST_ASSERT_FALSE(view.tile[6].cursor);
  TEST_ASSERT_TRUE(view.tile[6].filled);
  TEST_ASSERT_EQUAL_UINT8(1, cursorCount());
}

static void the_switches_show_the_radio_s_state(void) {
  ScreenBwInputs in = inputs(BAND_FM, false);
  in.widthKHz = 114;
  in.ims = true;
  in.eq = false;
  build(&in);
  TEST_ASSERT_EQUAL_STRING("iMS On", view.tile[17].text);
  TEST_ASSERT_TRUE(view.tile[17].filled);
  TEST_ASSERT_EQUAL_STRING("EQ Off", view.tile[18].text);
  TEST_ASSERT_FALSE(view.tile[18].filled);

  in.ims = false;
  in.eq = true;
  build(&in);
  TEST_ASSERT_EQUAL_STRING("iMS Off", view.tile[17].text);
  TEST_ASSERT_FALSE(view.tile[17].filled);
  TEST_ASSERT_EQUAL_STRING("EQ On", view.tile[18].text);
  TEST_ASSERT_TRUE(view.tile[18].filled);
  /* The width tile and the switch that is on: two filled. */
  TEST_ASSERT_EQUAL_UINT8(2, filledCount());
}

/* The eastern band is FM too, so it gets the switches. */
static void oirt_has_the_switches_too(void) {
  const ScreenBwInputs in = inputs(BAND_OIRT, false);
  build(&in);
  TEST_ASSERT_EQUAL_STRING("OIRT" DOT "kHz", view.context);
  TEST_ASSERT_TRUE(view.hasSwitches);
  TEST_ASSERT_EQUAL_STRING("AUTO", view.tile[0].text);
  TEST_ASSERT_EQUAL_UINT8(SCREEN_BW_EQ, view.tile[view.count - 1].kind);
}

/* ------------------------------------------------------------- DX mode */

/* DX mode is one fixed width, so there is no automatic tile. */
static void dx_mode_leaves_out_automatic(void) {
  ScreenBwInputs in = inputs(BAND_FM, true);
  in.widthKHz = 114;
  in.cursor = bwPageStart(tiles, in.count, 114);
  build(&in);
  TEST_ASSERT_EQUAL_UINT8(18, view.count);
  TEST_ASSERT_EQUAL_STRING("6/18", view.position);
  TEST_ASSERT_EQUAL_STRING("56", view.tile[0].text);
  for (uint8_t i = 0; i < view.count; i++) {
    TEST_ASSERT_NOT_EQUAL(0, strcmp("AUTO", view.tile[i].text));
  }
  TEST_ASSERT_TRUE(view.tile[5].filled);
  TEST_ASSERT_TRUE(view.tile[5].cursor);
  TEST_ASSERT_EQUAL_UINT8(1, filledCount());
  TEST_ASSERT_TRUE(view.hasSwitches);
}

/* ------------------------------------------------------------- AM */

static void the_am_bands_have_four_widths_and_no_switches(void) {
  const struct {
    BandId band;
    const char *context;
  } am[] = {{BAND_LW, "LW" DOT "kHz"},
            {BAND_MW, "MW" DOT "kHz"},
            {BAND_SW, "SW" DOT "kHz"}};
  for (uint8_t b = 0; b < 3; b++) {
    ScreenBwInputs in = inputs(am[b].band, false);
    in.widthKHz = 4;
    in.cursor = bwPageStart(tiles, in.count, 6);
    build(&in);
    TEST_ASSERT_EQUAL_STRING(am[b].context, view.context);
    TEST_ASSERT_EQUAL_UINT8(4, view.count);
    TEST_ASSERT_EQUAL_STRING("3/4", view.position);
    TEST_ASSERT_FALSE(view.hasSwitches);
    TEST_ASSERT_EQUAL_STRING("3", view.tile[0].text);
    TEST_ASSERT_EQUAL_STRING("4", view.tile[1].text);
    TEST_ASSERT_EQUAL_STRING("6", view.tile[2].text);
    TEST_ASSERT_EQUAL_STRING("8", view.tile[3].text);
    for (uint8_t i = 0; i < 4; i++) {
      TEST_ASSERT_EQUAL_UINT8(SCREEN_BW_WIDTH, view.tile[i].kind);
    }
    TEST_ASSERT_TRUE(view.tile[1].filled);
    TEST_ASSERT_TRUE(view.tile[2].cursor);
    TEST_ASSERT_EQUAL_UINT8(1, filledCount());
    TEST_ASSERT_EQUAL_UINT8(1, cursorCount());
  }
}

/* --------------------------------------------------------- the note */

/* On automatic the note says what the tuner has settled on. */
static void automatic_says_what_the_tuner_picked(void) {
  ScreenBwInputs in = inputs(BAND_FM, false);
  in.chipKnown = true;
  in.chipKHz = 217;
  build(&in);
  TEST_ASSERT_EQUAL_STRING("Auto: 217 kHz", view.note);
}

/* No note when the tuner has not answered, at a fixed width, where the tile
 * says it, or in DX mode. */
static void no_note_without_automatic_and_a_reading(void) {
  ScreenBwInputs in = inputs(BAND_FM, false);
  in.chipKnown = false;
  in.chipKHz = 217;
  build(&in);
  TEST_ASSERT_NULL(view.note);

  in.chipKnown = true;
  in.widthKHz = 114;
  build(&in);
  TEST_ASSERT_NULL(view.note);

  in = inputs(BAND_FM, true);
  in.chipKnown = true;
  in.chipKHz = 114;
  build(&in);
  TEST_ASSERT_NULL(view.note);
}

/* A new build starts clean, so a note from the last one does not stay. */
static void a_new_build_drops_the_last_note(void) {
  ScreenBwInputs in = inputs(BAND_FM, false);
  in.chipKnown = true;
  in.chipKHz = 217;
  build(&in);
  TEST_ASSERT_NOT_NULL(view.note);
  in.widthKHz = 84;
  build(&in);
  TEST_ASSERT_NULL(view.note);
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(a_missing_pointer_does_not_crash);
  RUN_TEST(no_tiles_still_fills_the_header);
  RUN_TEST(a_band_that_is_not_one_gives_an_empty_page);
  RUN_TEST(more_tiles_than_the_page_holds_are_cut);

  RUN_TEST(fm_lists_automatic_first_then_the_widths_and_the_switches);
  RUN_TEST(automatic_in_use_fills_its_own_tile);
  RUN_TEST(the_width_in_use_is_filled_and_the_cursor_starts_on_it);
  RUN_TEST(the_cursor_can_sit_on_another_tile);
  RUN_TEST(the_switches_show_the_radio_s_state);
  RUN_TEST(oirt_has_the_switches_too);

  RUN_TEST(dx_mode_leaves_out_automatic);

  RUN_TEST(the_am_bands_have_four_widths_and_no_switches);

  RUN_TEST(automatic_says_what_the_tuner_picked);
  RUN_TEST(no_note_without_automatic_and_a_reading);
  RUN_TEST(a_new_build_drops_the_last_note);

  return UNITY_END();
}
