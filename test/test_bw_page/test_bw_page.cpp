/*
 * Tests for the bandwidth page. Runs on a PC.
 */
#include <unity.h>

#include "core/bw_page.h"

static BwTile tiles[BW_PAGE_MAX];

void setUp(void) {}
void tearDown(void) {}

/* How many of the tiles are widths: the switches come after them. */
static uint8_t widthsIn(const BwTile *page, uint8_t count) {
  uint8_t n = 0;
  while (n < count && page[n].kind == BW_TILE_WIDTH) {
    n++;
  }
  return n;
}

static void fm_has_automatic_sixteen_widths_and_two_switches(void) {
  const uint8_t n = bwPageTiles(BAND_FM, false, tiles, BW_PAGE_MAX);
  TEST_ASSERT_EQUAL_UINT8(20, n);
  TEST_ASSERT_EQUAL_UINT8(17, widthsIn(tiles, n));
  TEST_ASSERT_EQUAL_UINT8(BW_TILE_WIDTH, tiles[0].kind);
  TEST_ASSERT_EQUAL_UINT16(0, tiles[0].khz);
  TEST_ASSERT_EQUAL_UINT16(56, tiles[1].khz);
  TEST_ASSERT_EQUAL_UINT16(311, tiles[16].khz);
  TEST_ASSERT_EQUAL_UINT8(BW_TILE_CLOSE, tiles[17].kind);
  TEST_ASSERT_EQUAL_UINT8(BW_TILE_IMS, tiles[18].kind);
  TEST_ASSERT_EQUAL_UINT8(BW_TILE_EQ, tiles[19].kind);
}

/* DX mode is one fixed width. */
static void dx_mode_leaves_out_automatic(void) {
  const uint8_t n = bwPageTiles(BAND_FM, true, tiles, BW_PAGE_MAX);
  TEST_ASSERT_EQUAL_UINT8(19, n);
  TEST_ASSERT_EQUAL_UINT8(16, widthsIn(tiles, n));
  TEST_ASSERT_EQUAL_UINT16(56, tiles[0].khz);
  TEST_ASSERT_EQUAL_UINT8(BW_TILE_CLOSE, tiles[16].kind);
}

/* The AM side has four widths, Close and neither switch. */
static void am_bands_have_their_four_widths_only(void) {
  const BandId am[] = {BAND_MW, BAND_LW, BAND_SW};
  for (uint8_t b = 0; b < 3; b++) {
    const uint8_t n = bwPageTiles(am[b], false, tiles, BW_PAGE_MAX);
    TEST_ASSERT_EQUAL_UINT8(5, n);
    TEST_ASSERT_EQUAL_UINT8(4, widthsIn(tiles, n));
    TEST_ASSERT_EQUAL_UINT16(3, tiles[0].khz);
    TEST_ASSERT_EQUAL_UINT16(8, tiles[3].khz);
    TEST_ASSERT_EQUAL_UINT8(BW_TILE_CLOSE, tiles[4].kind);
  }
}

static void no_room_or_no_band_gives_no_tiles(void) {
  TEST_ASSERT_EQUAL_UINT8(0, bwPageTiles(BAND_FM, false, tiles, 10));
  TEST_ASSERT_EQUAL_UINT8(0, bwPageTiles(BAND_FM, false, tiles, 17));
  TEST_ASSERT_EQUAL_UINT8(0, bwPageTiles(BAND_MW, false, tiles, 3));
  /* Room for the widths but not for Close, or for Close but not the
   * switches. */
  TEST_ASSERT_EQUAL_UINT8(0, bwPageTiles(BAND_MW, false, tiles, 4));
  TEST_ASSERT_EQUAL_UINT8(0, bwPageTiles(BAND_FM, false, tiles, 18));
  TEST_ASSERT_EQUAL_UINT8(0, bwPageTiles(BAND_FM, false, NULL, BW_PAGE_MAX));
  TEST_ASSERT_EQUAL_UINT8(0, bwPageTiles((BandId)99, false, tiles, 19));
}

static void the_cursor_starts_on_the_width_in_use(void) {
  const uint8_t n = bwPageTiles(BAND_FM, false, tiles, BW_PAGE_MAX);
  TEST_ASSERT_EQUAL_UINT8(0, bwPageStart(tiles, n, 0));
  TEST_ASSERT_EQUAL_UINT8(6, bwPageStart(tiles, n, 114));
  /* Not one of them: the first tile. */
  TEST_ASSERT_EQUAL_UINT8(0, bwPageStart(tiles, n, 115));
  TEST_ASSERT_EQUAL_UINT8(0, bwPageStart(NULL, n, 114));
  const uint8_t d = bwPageTiles(BAND_FM, true, tiles, BW_PAGE_MAX);
  TEST_ASSERT_EQUAL_UINT8(5, bwPageStart(tiles, d, 114));
}

static void the_cursor_stops_at_both_ends(void) {
  TEST_ASSERT_EQUAL_UINT8(1, bwPageMove(0, 1, 19));
  TEST_ASSERT_EQUAL_UINT8(0, bwPageMove(2, -5, 19));
  TEST_ASSERT_EQUAL_UINT8(18, bwPageMove(17, 4, 19));
  TEST_ASSERT_EQUAL_UINT8(3, bwPageMove(3, 0, 19));
  TEST_ASSERT_EQUAL_UINT8(0, bwPageMove(3, 1, 0));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(fm_has_automatic_sixteen_widths_and_two_switches);
  RUN_TEST(dx_mode_leaves_out_automatic);
  RUN_TEST(am_bands_have_their_four_widths_only);
  RUN_TEST(no_room_or_no_band_gives_no_tiles);
  RUN_TEST(the_cursor_starts_on_the_width_in_use);
  RUN_TEST(the_cursor_stops_at_both_ends);
  return UNITY_END();
}
