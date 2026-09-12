/**
 * @file test_band_plan.cpp
 * @brief Tests for the band plan. Every edge, every step, both wraps.
 *        Runs on a PC.
 */
#include <unity.h>

#include <string.h>

#include "core/band_plan.h"

static BandPlanConfig cfg;

void setUp(void) {
  bandPlanDefaults(&cfg);
}
void tearDown(void) {}

/** Shorthand, because every test needs the limits. */
static void limits(BandId band, uint32_t *lo, uint32_t *hi) {
  TEST_ASSERT_TRUE(bandLimits(band, &cfg, lo, hi));
}

/* ------------------------------------------------------------- the table -- */

static void the_defaults_are_the_ones_most_radios_ship_with(void) {
  TEST_ASSERT_EQUAL_INT(FM_REGION_WORLD, cfg.fmRegion);
  TEST_ASSERT_EQUAL_INT(MW_SPACING_9K, cfg.mwSpacing);
}

static void every_band_has_a_name_a_modulation_and_sane_edges(void) {
  for (int b = 0; b < BAND_COUNT; b++) {
    uint32_t lo = 0;
    uint32_t hi = 0;
    TEST_ASSERT_TRUE(bandLimits((BandId)b, &cfg, &lo, &hi));
    TEST_ASSERT_GREATER_THAN_UINT32(0, lo);
    TEST_ASSERT_GREATER_THAN_UINT32(lo, hi);
    TEST_ASSERT_NOT_EQUAL(0, strlen(bandName((BandId)b)));
    TEST_ASSERT_GREATER_THAN_UINT32(0, bandDefaultStep((BandId)b, &cfg));
    TEST_ASSERT_TRUE(
        bandStepAllowed((BandId)b, &cfg, bandDefaultStep((BandId)b, &cfg)));
  }
}

static void a_band_id_that_is_not_a_band_is_refused_everywhere(void) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  BandId bad = (BandId)BAND_COUNT;
  TEST_ASSERT_FALSE(bandLimits(bad, &cfg, &lo, &hi));
  TEST_ASSERT_EQUAL_STRING("", bandName(bad));
  TEST_ASSERT_EQUAL_UINT16(0, bandDefaultStep(bad, &cfg));
  TEST_ASSERT_EQUAL_size_t(0, bandStepCount(bad, &cfg));
  TEST_ASSERT_EQUAL_UINT16(0, bandStepAt(bad, &cfg, 0));
  TEST_ASSERT_FALSE(bandStepAllowed(bad, &cfg, 9));
  TEST_ASSERT_FALSE(bandContains(bad, &cfg, 1000));
  TEST_ASSERT_EQUAL_UINT32(1000, bandClamp(bad, &cfg, 1000));
  TEST_ASSERT_EQUAL_UINT32(1000, bandStepUp(bad, &cfg, 1000, 9));
  TEST_ASSERT_EQUAL_UINT32(1000, bandStepDown(bad, &cfg, 1000, 9));
  TEST_ASSERT_EQUAL_UINT32(0, bandTopChannel(bad, &cfg, 9));
}

static void a_null_config_means_the_defaults(void) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  uint32_t nlo = 0;
  uint32_t nhi = 0;
  TEST_ASSERT_TRUE(bandLimits(BAND_FM, &cfg, &lo, &hi));
  TEST_ASSERT_TRUE(bandLimits(BAND_FM, NULL, &nlo, &nhi));
  TEST_ASSERT_EQUAL_UINT32(lo, nlo);
  TEST_ASSERT_EQUAL_UINT32(hi, nhi);
}

/* ------------------------------------------------------------ the edges --- */

static void long_wave_runs_144_to_513(void) {
  uint32_t lo, hi;
  limits(BAND_LW, &lo, &hi);
  TEST_ASSERT_EQUAL_UINT32(144, lo);
  TEST_ASSERT_EQUAL_UINT32(513, hi);
  TEST_ASSERT_EQUAL_UINT16(9, bandDefaultStep(BAND_LW, &cfg));
}

static void medium_wave_edges_follow_the_spacing(void) {
  uint32_t lo, hi;
  cfg.mwSpacing = MW_SPACING_9K;
  limits(BAND_MW, &lo, &hi);
  TEST_ASSERT_EQUAL_UINT32(522, lo);
  TEST_ASSERT_EQUAL_UINT32(1791, hi);
  TEST_ASSERT_EQUAL_UINT16(9, bandDefaultStep(BAND_MW, &cfg));

  cfg.mwSpacing = MW_SPACING_10K;
  limits(BAND_MW, &lo, &hi);
  TEST_ASSERT_EQUAL_UINT32(520, lo);
  TEST_ASSERT_EQUAL_UINT32(1720, hi);
  TEST_ASSERT_EQUAL_UINT16(10, bandDefaultStep(BAND_MW, &cfg));

  /* The two do not cover the same range, which is the whole reason the
   * spacing changes the edges and not just the step. */
  TEST_ASSERT_TRUE(bandContains(BAND_MW, &cfg, 520));
  cfg.mwSpacing = MW_SPACING_9K;
  TEST_ASSERT_FALSE(bandContains(BAND_MW, &cfg, 520));
  TEST_ASSERT_TRUE(bandContains(BAND_MW, &cfg, 1791));
  cfg.mwSpacing = MW_SPACING_10K;
  TEST_ASSERT_FALSE(bandContains(BAND_MW, &cfg, 1791));
}

static void short_wave_runs_1700_to_27000(void) {
  uint32_t lo, hi;
  limits(BAND_SW, &lo, &hi);
  TEST_ASSERT_EQUAL_UINT32(1700, lo);
  TEST_ASSERT_EQUAL_UINT32(27000, hi);
  TEST_ASSERT_EQUAL_UINT16(5, bandDefaultStep(BAND_SW, &cfg));
}

static void oirt_runs_65_to_74_megahertz(void) {
  uint32_t lo, hi;
  limits(BAND_OIRT, &lo, &hi);
  TEST_ASSERT_EQUAL_UINT32(65000, lo);
  TEST_ASSERT_EQUAL_UINT32(74000, hi);
  TEST_ASSERT_EQUAL_UINT16(30, bandDefaultStep(BAND_OIRT, &cfg));
}

static void every_fm_region_is_filled_in(void) {
  /* The region tables are sized by FM_REGION_COUNT and written with
   * designated initialisers, so adding a region compiles clean and leaves the
   * new slot as zeros. A radio would then tune to 0 kHz. This is what catches
   * that, so it loops the enum rather than listing the regions. */
  for (int r = 0; r < FM_REGION_COUNT; r++) {
    uint32_t lo = 0;
    uint32_t hi = 0;
    cfg.fmRegion = (FmRegion)r;
    limits(BAND_FM, &lo, &hi);
    TEST_ASSERT_GREATER_THAN_UINT32(0, lo);
    TEST_ASSERT_GREATER_THAN_UINT32(lo, hi);
    TEST_ASSERT_TRUE(bandContains(BAND_FM, &cfg, lo));
    TEST_ASSERT_TRUE(bandContains(BAND_FM, &cfg, hi));
  }
}

static void a_null_out_pointer_is_refused_rather_than_written_through(void) {
  uint32_t lo = 0;
  TEST_ASSERT_FALSE(bandLimits(BAND_FM, &cfg, NULL, &lo));
  TEST_ASSERT_FALSE(bandLimits(BAND_FM, &cfg, &lo, NULL));
  TEST_ASSERT_FALSE(bandForFrequency(&cfg, 104000, NULL));
  bandPlanDefaults(NULL); /* Must not crash. */
}

static void every_fm_region_has_the_edges_the_listing_gives(void) {
  struct {
    FmRegion region;
    uint32_t lo;
    uint32_t hi;
  } expected[] = {
      {FM_REGION_FULL, 65000, 108000},  {FM_REGION_JAPAN, 76000, 95000},
      {FM_REGION_WIDE, 76000, 108000},  {FM_REGION_87_108, 87000, 108000},
      {FM_REGION_WORLD, 87500, 108000},
  };
  for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
    uint32_t lo, hi;
    cfg.fmRegion = expected[i].region;
    limits(BAND_FM, &lo, &hi);
    TEST_ASSERT_EQUAL_UINT32(expected[i].lo, lo);
    TEST_ASSERT_EQUAL_UINT32(expected[i].hi, hi);
  }
}

static void the_edges_themselves_are_inside_the_band(void) {
  for (int b = 0; b < BAND_COUNT; b++) {
    uint32_t lo, hi;
    limits((BandId)b, &lo, &hi);
    TEST_ASSERT_TRUE(bandContains((BandId)b, &cfg, lo));
    TEST_ASSERT_TRUE(bandContains((BandId)b, &cfg, hi));
    TEST_ASSERT_FALSE(bandContains((BandId)b, &cfg, lo - 1));
    TEST_ASSERT_FALSE(bandContains((BandId)b, &cfg, hi + 1));
  }
}

static void clamping_pulls_a_frequency_back_to_the_nearest_edge(void) {
  for (int b = 0; b < BAND_COUNT; b++) {
    uint32_t lo, hi;
    limits((BandId)b, &lo, &hi);
    TEST_ASSERT_EQUAL_UINT32(lo, bandClamp((BandId)b, &cfg, 0));
    TEST_ASSERT_EQUAL_UINT32(lo, bandClamp((BandId)b, &cfg, lo - 1));
    TEST_ASSERT_EQUAL_UINT32(hi, bandClamp((BandId)b, &cfg, hi + 1));
    TEST_ASSERT_EQUAL_UINT32(lo, bandClamp((BandId)b, &cfg, lo));
    TEST_ASSERT_EQUAL_UINT32(hi, bandClamp((BandId)b, &cfg, hi));
  }
}

/* ------------------------------------------------------------- the steps -- */

static void a_step_a_band_does_not_offer_is_refused(void) {
  TEST_ASSERT_TRUE(bandStepAllowed(BAND_FM, &cfg, 50));
  TEST_ASSERT_TRUE(bandStepAllowed(BAND_FM, &cfg, 100));
  TEST_ASSERT_TRUE(bandStepAllowed(BAND_FM, &cfg, 200));
  TEST_ASSERT_FALSE(bandStepAllowed(BAND_FM, &cfg, 9));
  TEST_ASSERT_FALSE(bandStepAllowed(BAND_FM, &cfg, 30));
  TEST_ASSERT_FALSE(bandStepAllowed(BAND_FM, &cfg, 0));

  TEST_ASSERT_TRUE(bandStepAllowed(BAND_OIRT, &cfg, 30));
  TEST_ASSERT_FALSE(bandStepAllowed(BAND_OIRT, &cfg, 100));

  TEST_ASSERT_TRUE(bandStepAllowed(BAND_SW, &cfg, 5));
  TEST_ASSERT_FALSE(bandStepAllowed(BAND_SW, &cfg, 9));
}

static void medium_wave_offers_the_step_that_matches_its_spacing(void) {
  cfg.mwSpacing = MW_SPACING_9K;
  TEST_ASSERT_TRUE(bandStepAllowed(BAND_MW, &cfg, 9));
  TEST_ASSERT_FALSE(bandStepAllowed(BAND_MW, &cfg, 10));

  cfg.mwSpacing = MW_SPACING_10K;
  TEST_ASSERT_TRUE(bandStepAllowed(BAND_MW, &cfg, 10));
  TEST_ASSERT_FALSE(bandStepAllowed(BAND_MW, &cfg, 9));
}

static void the_step_list_and_the_count_agree(void) {
  for (int b = 0; b < BAND_COUNT; b++) {
    size_t n = bandStepCount((BandId)b, &cfg);
    TEST_ASSERT_GREATER_THAN_size_t(0, n);
    for (size_t i = 0; i < n; i++) {
      uint16_t step = bandStepAt((BandId)b, &cfg, i);
      TEST_ASSERT_GREATER_THAN_UINT16(0, step);
      TEST_ASSERT_TRUE(bandStepAllowed((BandId)b, &cfg, step));
    }
    /* One past the end is not a step. */
    TEST_ASSERT_EQUAL_UINT16(0, bandStepAt((BandId)b, &cfg, n));
  }
}

static void stepping_with_a_step_the_band_refuses_changes_nothing(void) {
  TEST_ASSERT_EQUAL_UINT32(104000, bandStepUp(BAND_FM, &cfg, 104000, 9));
  TEST_ASSERT_EQUAL_UINT32(104000, bandStepDown(BAND_FM, &cfg, 104000, 9));
  TEST_ASSERT_EQUAL_UINT32(104000, bandStepUp(BAND_FM, &cfg, 104000, 0));
}

/* -------------------------------------------------------------- the wrap -- */

static void stepping_up_from_the_low_edge_lands_on_the_next_channel(void) {
  TEST_ASSERT_EQUAL_UINT32(87600, bandStepUp(BAND_FM, &cfg, 87500, 100));
  TEST_ASSERT_EQUAL_UINT32(531, bandStepUp(BAND_MW, &cfg, 522, 9));
  TEST_ASSERT_EQUAL_UINT32(153, bandStepUp(BAND_LW, &cfg, 144, 9));
  TEST_ASSERT_EQUAL_UINT32(1705, bandStepUp(BAND_SW, &cfg, 1700, 5));
  TEST_ASSERT_EQUAL_UINT32(65030, bandStepUp(BAND_OIRT, &cfg, 65000, 30));
}

static void stepping_up_from_the_top_channel_wraps_to_the_low_edge(void) {
  struct {
    BandId band;
    uint16_t step;
  } cases[] = {
      {BAND_LW, 9},   {BAND_MW, 9},   {BAND_SW, 5},
      {BAND_FM, 100}, {BAND_FM, 200}, {BAND_OIRT, 30},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    uint32_t lo, hi;
    limits(cases[i].band, &lo, &hi);
    uint32_t top = bandTopChannel(cases[i].band, &cfg, cases[i].step);
    TEST_ASSERT_TRUE(bandContains(cases[i].band, &cfg, top));
    TEST_ASSERT_EQUAL_UINT32(
        lo, bandStepUp(cases[i].band, &cfg, top, cases[i].step));
  }
}

static void stepping_down_from_the_low_edge_wraps_to_the_top_channel(void) {
  struct {
    BandId band;
    uint16_t step;
  } cases[] = {
      {BAND_LW, 9},   {BAND_MW, 9},   {BAND_SW, 5},
      {BAND_FM, 100}, {BAND_FM, 200}, {BAND_OIRT, 30},
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    uint32_t lo, hi;
    limits(cases[i].band, &lo, &hi);
    uint32_t top = bandTopChannel(cases[i].band, &cfg, cases[i].step);
    TEST_ASSERT_EQUAL_UINT32(
        top, bandStepDown(cases[i].band, &cfg, lo, cases[i].step));
  }
}

static void up_then_down_returns_to_where_it_started(void) {
  uint32_t f = 104000;
  TEST_ASSERT_EQUAL_UINT32(
      f, bandStepDown(BAND_FM, &cfg, bandStepUp(BAND_FM, &cfg, f, 100), 100));
  uint32_t m = 999;
  TEST_ASSERT_EQUAL_UINT32(
      m, bandStepDown(BAND_MW, &cfg, bandStepUp(BAND_MW, &cfg, m, 9), 9));
}

static void the_top_channel_is_not_always_the_top_edge(void) {
  /* 87.5 to 108.0 in 200 kHz steps is 102.5 steps, so the last channel is
   * 107.9 and stepping up from there wraps rather than reaching 108.0. */
  TEST_ASSERT_EQUAL_UINT32(107900, bandTopChannel(BAND_FM, &cfg, 200));
  TEST_ASSERT_EQUAL_UINT32(87500, bandStepUp(BAND_FM, &cfg, 107900, 200));
  TEST_ASSERT_TRUE(bandContains(BAND_FM, &cfg, 108000));

  /* Where the range divides evenly, it is the top edge. */
  TEST_ASSERT_EQUAL_UINT32(108000, bandTopChannel(BAND_FM, &cfg, 100));
  TEST_ASSERT_EQUAL_UINT32(1791, bandTopChannel(BAND_MW, &cfg, 9));
  TEST_ASSERT_EQUAL_UINT32(513, bandTopChannel(BAND_LW, &cfg, 9));
  TEST_ASSERT_EQUAL_UINT32(27000, bandTopChannel(BAND_SW, &cfg, 5));
}

static void a_frequency_off_the_grid_is_pulled_back_onto_it(void) {
  /* 87.55 is not a 100 kHz channel. Up goes to 87.6, down goes to 87.5,
   * rather than carrying the 50 kHz offset along forever. */
  TEST_ASSERT_EQUAL_UINT32(87600, bandStepUp(BAND_FM, &cfg, 87550, 100));
  TEST_ASSERT_EQUAL_UINT32(87500, bandStepDown(BAND_FM, &cfg, 87550, 100));

  /* 1000 kHz is not on the 9 kHz grid, which starts at 522. The channels
   * either side are 999 and 1008. */
  TEST_ASSERT_EQUAL_UINT32(1008, bandStepUp(BAND_MW, &cfg, 1000, 9));
  TEST_ASSERT_EQUAL_UINT32(999, bandStepDown(BAND_MW, &cfg, 1000, 9));
  TEST_ASSERT_TRUE((999 - 522) % 9 == 0);
  TEST_ASSERT_TRUE((1008 - 522) % 9 == 0);
}

static void stepping_from_outside_the_band_comes_back_inside(void) {
  uint32_t lo, hi;
  limits(BAND_FM, &lo, &hi);
  TEST_ASSERT_EQUAL_UINT32(lo, bandStepUp(BAND_FM, &cfg, 1000, 100));
  TEST_ASSERT_EQUAL_UINT32(bandTopChannel(BAND_FM, &cfg, 100),
                           bandStepDown(BAND_FM, &cfg, 200000, 100));
}

static void an_absurd_frequency_still_wraps_to_the_low_edge(void) {
  /* The step arithmetic used to overflow. On shortwave, stepping up from
   * 4294967295 computed 1700 + (858993118 + 1) * 5, which wraps a uint32 and
   * came back as 4, a frequency outside the band and off the grid. Reachable
   * from POST /api/tune the moment that endpoint exists. */
  for (int b = 0; b < BAND_COUNT; b++) {
    size_t n = bandStepCount((BandId)b, &cfg);
    uint32_t lo, hi;
    limits((BandId)b, &lo, &hi);
    for (size_t i = 0; i < n; i++) {
      uint16_t step = bandStepAt((BandId)b, &cfg, i);
      uint32_t top = bandTopChannel((BandId)b, &cfg, step);

      uint32_t up = bandStepUp((BandId)b, &cfg, UINT32_MAX, step);
      TEST_ASSERT_EQUAL_UINT32(lo, up);
      TEST_ASSERT_TRUE(bandContains((BandId)b, &cfg, up));

      uint32_t down = bandStepDown((BandId)b, &cfg, UINT32_MAX, step);
      TEST_ASSERT_EQUAL_UINT32(top, down);
      TEST_ASSERT_TRUE(bandContains((BandId)b, &cfg, down));

      /* Zero is below every band, so it wraps the other way. */
      TEST_ASSERT_EQUAL_UINT32(lo, bandStepUp((BandId)b, &cfg, 0, step));
      TEST_ASSERT_EQUAL_UINT32(top, bandStepDown((BandId)b, &cfg, 0, step));

      /* The top edge itself wraps, whether or not it is on the grid. */
      TEST_ASSERT_EQUAL_UINT32(lo, bandStepUp((BandId)b, &cfg, hi, step));
    }
  }
}

static void every_channel_on_every_band_stays_inside_the_band(void) {
  for (int b = 0; b < BAND_COUNT; b++) {
    size_t n = bandStepCount((BandId)b, &cfg);
    for (size_t i = 0; i < n; i++) {
      uint16_t step = bandStepAt((BandId)b, &cfg, i);
      uint32_t lo, hi;
      limits((BandId)b, &lo, &hi);
      uint32_t f = lo;
      /* Walk the whole band for the coarse steps, and a slice for the fine
       * ones, checking nothing ever falls outside. */
      int guard = 0;
      do {
        TEST_ASSERT_TRUE(bandContains((BandId)b, &cfg, f));
        f = bandStepUp((BandId)b, &cfg, f, step);
      } while (f != lo && ++guard < 6000);
    }
  }
}

/* ------------------------------------------------- which band is it in --- */

static void a_frequency_is_matched_to_its_band(void) {
  BandId b;
  TEST_ASSERT_TRUE(bandForFrequency(&cfg, 200, &b));
  TEST_ASSERT_EQUAL_INT(BAND_LW, b);
  TEST_ASSERT_TRUE(bandForFrequency(&cfg, 1000, &b));
  TEST_ASSERT_EQUAL_INT(BAND_MW, b);
  TEST_ASSERT_TRUE(bandForFrequency(&cfg, 9420, &b));
  TEST_ASSERT_EQUAL_INT(BAND_SW, b);
  TEST_ASSERT_TRUE(bandForFrequency(&cfg, 104000, &b));
  TEST_ASSERT_EQUAL_INT(BAND_FM, b);
}

static void a_frequency_in_no_band_is_reported_as_such(void) {
  BandId b = BAND_FM;
  TEST_ASSERT_FALSE(bandForFrequency(&cfg, 1, &b));
  TEST_ASSERT_FALSE(bandForFrequency(&cfg, 50000, &b));
  TEST_ASSERT_FALSE(bandForFrequency(&cfg, 500000, &b));
  /* Nothing was written on failure. */
  TEST_ASSERT_EQUAL_INT(BAND_FM, b);
}

static void oirt_wins_over_the_full_fm_region_where_they_overlap(void) {
  /* With FM set to its full 65 to 108 MHz range the two bands overlap. OIRT
   * is checked first, so 70 MHz comes back as OIRT rather than FM. */
  cfg.fmRegion = FM_REGION_FULL;
  BandId b;
  TEST_ASSERT_TRUE(bandForFrequency(&cfg, 70000, &b));
  TEST_ASSERT_EQUAL_INT(BAND_OIRT, b);
  TEST_ASSERT_TRUE(bandForFrequency(&cfg, 104000, &b));
  TEST_ASSERT_EQUAL_INT(BAND_FM, b);
}

/* ------------------------------------------------------- meter bands ----- */

static void the_meter_bands_are_ordered_and_do_not_overlap(void) {
  size_t n = swMeterBandCount();
  TEST_ASSERT_EQUAL_size_t(15, n);
  uint32_t lastHigh = 0;
  for (size_t i = 0; i < n; i++) {
    const SwMeterBand *m = swMeterBandAt(i);
    TEST_ASSERT_NOT_NULL(m);
    TEST_ASSERT_GREATER_THAN_UINT32(lastHigh, m->lowKHz);
    TEST_ASSERT_GREATER_THAN_UINT32(m->lowKHz, m->highKHz);
    /* Every meter band is inside shortwave. */
    TEST_ASSERT_TRUE(bandContains(BAND_SW, &cfg, m->lowKHz));
    TEST_ASSERT_TRUE(bandContains(BAND_SW, &cfg, m->highKHz));
    lastHigh = m->highKHz;
  }
  TEST_ASSERT_NULL(swMeterBandAt(n));
}

static void a_frequency_maps_to_its_meter_band_or_to_none(void) {
  const SwMeterBand *m = swMeterBandFor(9420);
  TEST_ASSERT_NOT_NULL(m);
  TEST_ASSERT_EQUAL_UINT16(31, m->metres);

  /* The edges belong to the band. */
  TEST_ASSERT_NOT_NULL(swMeterBandFor(9400));
  TEST_ASSERT_NOT_NULL(swMeterBandFor(9900));
  /* Just outside does not. Most of shortwave is between two bands. */
  TEST_ASSERT_NULL(swMeterBandFor(9399));
  TEST_ASSERT_NULL(swMeterBandFor(9901));
  TEST_ASSERT_NULL(swMeterBandFor(1750));

  /* 160 metres is amateur, not broadcast, but the firmware being replaced
   * steps through it, so it is in the list. */
  const SwMeterBand *top = swMeterBandFor(1800);
  TEST_ASSERT_NOT_NULL(top);
  TEST_ASSERT_EQUAL_UINT16(160, top->metres);
}

static void meter_band_stepping_wraps_at_both_ends(void) {
  /* Up from inside 31m goes to the start of 25m. */
  TEST_ASSERT_EQUAL_UINT32(11600, swMeterBandNext(9420));
  /* Up from above the highest band wraps to the lowest, which is 160 m. */
  TEST_ASSERT_EQUAL_UINT32(1800, swMeterBandNext(26500));
  /* From between two bands it goes to the one above, which is the nearest. */
  TEST_ASSERT_EQUAL_UINT32(9400, swMeterBandNext(9399));
  TEST_ASSERT_EQUAL_UINT32(11600, swMeterBandNext(9901));
  /* 1800 is the start of 160 m, so up goes past it to 120 m. */
  TEST_ASSERT_EQUAL_UINT32(2300, swMeterBandNext(1800));
  /* From below every band it goes to the lowest. */
  TEST_ASSERT_EQUAL_UINT32(1800, swMeterBandNext(1750));
  /* Down from inside 31m goes to the start of 41m, skipping the band it is
   * already in. Otherwise leaving a band would take two presses. */
  TEST_ASSERT_EQUAL_UINT32(7200, swMeterBandPrevious(9420));
  TEST_ASSERT_EQUAL_UINT32(7200, swMeterBandPrevious(9400));
  TEST_ASSERT_EQUAL_UINT32(7200, swMeterBandPrevious(9900));
  /* From between two bands it goes to the one below, which is the nearest. */
  TEST_ASSERT_EQUAL_UINT32(7200, swMeterBandPrevious(9399));
  TEST_ASSERT_EQUAL_UINT32(9400, swMeterBandPrevious(9901));
  /* Down from inside the lowest band wraps to the highest. */
  TEST_ASSERT_EQUAL_UINT32(25670, swMeterBandPrevious(2000));
  TEST_ASSERT_EQUAL_UINT32(25670, swMeterBandPrevious(1750));
}

/* --------------------------------------------------------- formatting ---- */

static void fm_frequencies_read_as_megahertz_with_two_decimals(void) {
  char out[16];
  TEST_ASSERT_TRUE(bandFormatFrequency(BAND_FM, 104000, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("104.00", out);
  TEST_ASSERT_TRUE(bandFormatFrequency(BAND_FM, 87500, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("87.50", out);
  TEST_ASSERT_TRUE(bandFormatFrequency(BAND_FM, 107900, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("107.90", out);
  TEST_ASSERT_TRUE(bandFormatFrequency(BAND_OIRT, 65030, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("65.03", out);
  TEST_ASSERT_EQUAL_STRING("MHz", bandFrequencyUnit(BAND_FM));
}

static void am_frequencies_read_as_grouped_kilohertz(void) {
  char out[16];
  TEST_ASSERT_TRUE(bandFormatFrequency(BAND_SW, 9420, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("9 420", out);
  TEST_ASSERT_TRUE(bandFormatFrequency(BAND_SW, 27000, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("27 000", out);
  TEST_ASSERT_TRUE(bandFormatFrequency(BAND_MW, 999, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("999", out);
  TEST_ASSERT_TRUE(bandFormatFrequency(BAND_MW, 1791, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("1 791", out);
  TEST_ASSERT_TRUE(bandFormatFrequency(BAND_LW, 144, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("144", out);
  TEST_ASSERT_EQUAL_STRING("kHz", bandFrequencyUnit(BAND_MW));
}

static void formatting_into_a_buffer_that_is_too_small_fails_safely(void) {
  char out[4];
  TEST_ASSERT_FALSE(bandFormatFrequency(BAND_FM, 104000, out, sizeof(out)));
  TEST_ASSERT_EQUAL_STRING("", out);
  TEST_ASSERT_FALSE(bandFormatFrequency(BAND_FM, 104000, out, 0));
  TEST_ASSERT_FALSE(bandFormatFrequency(BAND_FM, 104000, NULL, sizeof(out)));

  char ok[16];
  TEST_ASSERT_FALSE(
      bandFormatFrequency((BandId)BAND_COUNT, 104000, ok, sizeof(ok)));
  TEST_ASSERT_EQUAL_STRING("", ok);
}

/* ------------------------------------------------- a number typed on a pad */

static void typing_1028_on_fm_means_102_point_8_megahertz(void) {
  uint32_t khz = 0;
  BandId band = BAND_COUNT;
  TEST_ASSERT_TRUE(bandFromTypedNumber(&cfg, 1028, BAND_FM, &khz, &band));
  TEST_ASSERT_EQUAL_UINT32(102800, khz);
  TEST_ASSERT_EQUAL_INT(BAND_FM, band);
}

static void typing_738_means_738_kilohertz_on_medium_wave(void) {
  uint32_t khz = 0;
  BandId band = BAND_COUNT;
  TEST_ASSERT_TRUE(bandFromTypedNumber(&cfg, 738, BAND_MW, &khz, &band));
  TEST_ASSERT_EQUAL_UINT32(738, khz);
  TEST_ASSERT_EQUAL_INT(BAND_MW, band);
}

static void a_number_that_reads_two_ways_stays_on_the_band_in_use(void) {
  /* 1000 is a medium wave frequency and also 100.0 MHz. Whoever typed it was
   * looking at one band, and that is the one they meant. */
  uint32_t khz = 0;
  BandId band = BAND_COUNT;
  TEST_ASSERT_TRUE(bandFromTypedNumber(&cfg, 1000, BAND_MW, &khz, &band));
  TEST_ASSERT_EQUAL_UINT32(1000, khz);
  TEST_ASSERT_EQUAL_INT(BAND_MW, band);

  TEST_ASSERT_TRUE(bandFromTypedNumber(&cfg, 1000, BAND_FM, &khz, &band));
  TEST_ASSERT_EQUAL_UINT32(100000, khz);
  TEST_ASSERT_EQUAL_INT(BAND_FM, band);
}

static void typing_a_number_in_no_band_is_refused(void) {
  uint32_t khz = 12345;
  BandId band = BAND_FM;
  /* 28000 sits in the gap above shortwave, which ends at 27000, and below
   * OIRT, which starts at 65000. Ten times it is past the top of FM. So
   * there is nothing honest to tune and nothing is written. */
  TEST_ASSERT_FALSE(bandFromTypedNumber(&cfg, 28000, BAND_COUNT, &khz, &band));
  TEST_ASSERT_EQUAL_UINT32(12345, khz);
  TEST_ASSERT_EQUAL_INT(BAND_FM, band);
}

static void almost_any_small_number_lands_somewhere(void) {
  /* Worth writing down because it is surprising. Shortwave alone runs from
   * 1700 to 27000 kHz, and no number can step over a range that wide by
   * multiplying by ten. So a typed number is nearly always tunable somewhere,
   * and the band in use is what decides which reading was meant.
   *
   * 4 stops at 400, which is long wave, before it ever reaches shortwave. */
  uint32_t khz = 0;
  BandId band = BAND_COUNT;
  TEST_ASSERT_TRUE(bandFromTypedNumber(&cfg, 4, BAND_COUNT, &khz, &band));
  TEST_ASSERT_EQUAL_UINT32(400, khz);
  TEST_ASSERT_EQUAL_INT(BAND_LW, band);

  /* And with shortwave preferred, the same digits mean 4000 kHz. */
  TEST_ASSERT_TRUE(bandFromTypedNumber(&cfg, 4, BAND_SW, &khz, &band));
  TEST_ASSERT_EQUAL_UINT32(4000, khz);
  TEST_ASSERT_EQUAL_INT(BAND_SW, band);
}

static void typing_zero_is_refused(void) {
  TEST_ASSERT_FALSE(bandFromTypedNumber(&cfg, 0, BAND_COUNT, NULL, NULL));
}

static void a_huge_typed_number_stops_rather_than_overflowing(void) {
  TEST_ASSERT_FALSE(
      bandFromTypedNumber(&cfg, 4000000000UL, BAND_COUNT, NULL, NULL));
}

static void typing_works_with_no_band_preference(void) {
  uint32_t khz = 0;
  BandId band = BAND_COUNT;
  TEST_ASSERT_TRUE(bandFromTypedNumber(&cfg, 9420, BAND_COUNT, &khz, &band));
  TEST_ASSERT_EQUAL_UINT32(9420, khz);
  TEST_ASSERT_EQUAL_INT(BAND_SW, band);
}

static void a_null_plan_is_refused(void) {
  TEST_ASSERT_FALSE(bandFromTypedNumber(NULL, 1028, BAND_FM, NULL, NULL));
}

/* ------------------------------------------------------------- bandwidths */

static void fm_offers_the_automatic_width_and_am_does_not(void) {
  TEST_ASSERT_EQUAL_UINT16(0, bandBandwidthAt(BAND_FM, 0));
  TEST_ASSERT_EQUAL_UINT16(3, bandBandwidthAt(BAND_MW, 0));
  for (size_t i = 0; i < bandBandwidthCount(BAND_MW); i++) {
    TEST_ASSERT_NOT_EQUAL_UINT16(0, bandBandwidthAt(BAND_MW, i));
  }
}

static void every_am_band_offers_the_same_four_widths(void) {
  BandId am[] = {BAND_LW, BAND_MW, BAND_SW};
  for (size_t b = 0; b < sizeof(am) / sizeof(am[0]); b++) {
    TEST_ASSERT_EQUAL_size_t(4, bandBandwidthCount(am[b]));
    TEST_ASSERT_EQUAL_UINT16(3, bandBandwidthAt(am[b], 0));
    TEST_ASSERT_EQUAL_UINT16(8, bandBandwidthAt(am[b], 3));
  }
}

static void the_bandwidth_button_walks_the_list_and_wraps(void) {
  uint16_t bw = bandBandwidthAt(BAND_FM, 0);
  size_t count = bandBandwidthCount(BAND_FM);
  for (size_t i = 1; i < count; i++) {
    bw = bandBandwidthNext(BAND_FM, bw);
    TEST_ASSERT_EQUAL_UINT16(bandBandwidthAt(BAND_FM, i), bw);
  }
  /* Past the end and back to the start. */
  TEST_ASSERT_EQUAL_UINT16(bandBandwidthAt(BAND_FM, 0),
                           bandBandwidthNext(BAND_FM, bw));
}

static void a_bandwidth_not_in_the_list_starts_again_at_the_front(void) {
  TEST_ASSERT_EQUAL_UINT16(bandBandwidthAt(BAND_FM, 0),
                           bandBandwidthNext(BAND_FM, 999));
  TEST_ASSERT_EQUAL_UINT16(3, bandBandwidthNext(BAND_MW, 999));
}

static void asking_past_the_end_of_the_list_gives_nothing(void) {
  TEST_ASSERT_EQUAL_UINT16(0, bandBandwidthAt(BAND_FM, 99));
  TEST_ASSERT_EQUAL_size_t(0, bandBandwidthCount(BAND_COUNT));
  TEST_ASSERT_EQUAL_UINT16(0, bandBandwidthNext(BAND_COUNT, 100));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(the_defaults_are_the_ones_most_radios_ship_with);
  RUN_TEST(every_band_has_a_name_a_modulation_and_sane_edges);
  RUN_TEST(a_band_id_that_is_not_a_band_is_refused_everywhere);
  RUN_TEST(a_null_config_means_the_defaults);

  RUN_TEST(long_wave_runs_144_to_513);
  RUN_TEST(medium_wave_edges_follow_the_spacing);
  RUN_TEST(short_wave_runs_1700_to_27000);
  RUN_TEST(oirt_runs_65_to_74_megahertz);
  RUN_TEST(every_fm_region_is_filled_in);
  RUN_TEST(a_null_out_pointer_is_refused_rather_than_written_through);
  RUN_TEST(every_fm_region_has_the_edges_the_listing_gives);
  RUN_TEST(the_edges_themselves_are_inside_the_band);
  RUN_TEST(clamping_pulls_a_frequency_back_to_the_nearest_edge);

  RUN_TEST(a_step_a_band_does_not_offer_is_refused);
  RUN_TEST(medium_wave_offers_the_step_that_matches_its_spacing);
  RUN_TEST(the_step_list_and_the_count_agree);
  RUN_TEST(stepping_with_a_step_the_band_refuses_changes_nothing);

  RUN_TEST(stepping_up_from_the_low_edge_lands_on_the_next_channel);
  RUN_TEST(stepping_up_from_the_top_channel_wraps_to_the_low_edge);
  RUN_TEST(stepping_down_from_the_low_edge_wraps_to_the_top_channel);
  RUN_TEST(up_then_down_returns_to_where_it_started);
  RUN_TEST(the_top_channel_is_not_always_the_top_edge);
  RUN_TEST(a_frequency_off_the_grid_is_pulled_back_onto_it);
  RUN_TEST(stepping_from_outside_the_band_comes_back_inside);
  RUN_TEST(an_absurd_frequency_still_wraps_to_the_low_edge);
  RUN_TEST(every_channel_on_every_band_stays_inside_the_band);

  RUN_TEST(a_frequency_is_matched_to_its_band);
  RUN_TEST(a_frequency_in_no_band_is_reported_as_such);
  RUN_TEST(oirt_wins_over_the_full_fm_region_where_they_overlap);

  RUN_TEST(the_meter_bands_are_ordered_and_do_not_overlap);
  RUN_TEST(a_frequency_maps_to_its_meter_band_or_to_none);
  RUN_TEST(meter_band_stepping_wraps_at_both_ends);

  RUN_TEST(fm_frequencies_read_as_megahertz_with_two_decimals);
  RUN_TEST(am_frequencies_read_as_grouped_kilohertz);
  RUN_TEST(formatting_into_a_buffer_that_is_too_small_fails_safely);
  RUN_TEST(typing_1028_on_fm_means_102_point_8_megahertz);
  RUN_TEST(typing_738_means_738_kilohertz_on_medium_wave);
  RUN_TEST(a_number_that_reads_two_ways_stays_on_the_band_in_use);
  RUN_TEST(typing_a_number_in_no_band_is_refused);
  RUN_TEST(almost_any_small_number_lands_somewhere);
  RUN_TEST(typing_zero_is_refused);
  RUN_TEST(a_huge_typed_number_stops_rather_than_overflowing);
  RUN_TEST(typing_works_with_no_band_preference);
  RUN_TEST(a_null_plan_is_refused);

  RUN_TEST(fm_offers_the_automatic_width_and_am_does_not);
  RUN_TEST(every_am_band_offers_the_same_four_widths);
  RUN_TEST(the_bandwidth_button_walks_the_list_and_wraps);
  RUN_TEST(a_bandwidth_not_in_the_list_starts_again_at_the_front);
  RUN_TEST(asking_past_the_end_of_the_list_gives_nothing);

  return UNITY_END();
}
