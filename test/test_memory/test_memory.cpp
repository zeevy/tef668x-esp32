/* Tests for the stored channels. Runs on a PC. */
#include <unity.h>

#include <string.h>

#include "core/memory.h"

static MemoryStore store;

void setUp(void) {
  memoryInit(&store);
}
void tearDown(void) {}

static MemoryChannel channel(uint8_t band, uint32_t freqKHz, const char *name) {
  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  c.band = band;
  c.freqKHz = freqKHz;
  c.bandwidthKHz = 0;
  strncpy(c.name, name, MEMORY_NAME_LEN - 1);
  return c;
}

/* Presets 3 and 12 on one station, the case two slots can share. */
static void twoOnOneStation(void) {
  MemoryChannel c = channel(BAND_FM, 102800, "");
  TEST_ASSERT_TRUE(memorySet(&store, 3, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 12, &c));
  c = channel(BAND_FM, 106400, "");
  TEST_ASSERT_TRUE(memorySet(&store, 5, &c));
}

static void pick_keeps_the_chosen_slot_while_it_holds_the_dial(void) {
  twoOnOneStation();
  TEST_ASSERT_EQUAL_INT(12, memoryPick(&store, 12, 3, BAND_FM, 102800));
  TEST_ASSERT_EQUAL_INT(3, memoryPick(&store, 3, 12, BAND_FM, 102800));
}

static void pick_falls_to_the_second_then_to_the_lowest(void) {
  twoOnOneStation();
  TEST_ASSERT_EQUAL_INT(12, memoryPick(&store, 5, 12, BAND_FM, 102800));
  TEST_ASSERT_EQUAL_INT(
      3, memoryPick(&store, MEMORY_NO_SLOT, MEMORY_NO_SLOT, BAND_FM, 102800));
  TEST_ASSERT_EQUAL_INT(3, memoryPick(&store, 5, 5, BAND_FM, 102800));
}

static void pick_answers_no_slot_off_every_preset(void) {
  twoOnOneStation();
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryPick(&store, 12, 3, BAND_FM, 101800));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryPick(&store, 12, 3, BAND_MW, 102800));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryPick(&store, 12, 3, BAND_FM, 0));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryPick(NULL, 12, 3, BAND_FM, 102800));
}

static void pick_passes_over_a_slot_outside_the_store(void) {
  twoOnOneStation();
  TEST_ASSERT_EQUAL_INT(
      12, memoryPick(&store, MEMORY_SLOT_COUNT, 12, BAND_FM, 102800));
  TEST_ASSERT_EQUAL_INT(3, memoryPick(&store, -2, 150, BAND_FM, 102800));
}

static void save_goes_into_the_lowest_free_slot(void) {
  twoOnOneStation();
  int slot = MEMORY_NO_SLOT;
  TEST_ASSERT_EQUAL_INT(
      MEMORY_SAVE_OK,
      memorySaveChannel(&store, BAND_FM, 98300, 0, NULL, &slot));
  TEST_ASSERT_EQUAL_INT(0, slot);
  TEST_ASSERT_EQUAL_UINT32(98300, memoryGet(&store, 0)->freqKHz);
  TEST_ASSERT_EQUAL_STRING("", memoryGet(&store, 0)->name);
}

static void save_replaces_a_named_slot_outright(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "Old name");
  c.bandwidthKHz = 168;
  TEST_ASSERT_TRUE(memorySet(&store, 7, &c));
  int slot = 7;
  TEST_ASSERT_EQUAL_INT(
      MEMORY_SAVE_OK,
      memorySaveChannel(&store, BAND_FM, 101900, 0, "New", &slot));
  TEST_ASSERT_EQUAL_INT(7, slot);
  TEST_ASSERT_EQUAL_UINT32(101900, memoryGet(&store, 7)->freqKHz);
  TEST_ASSERT_EQUAL_UINT16(0, memoryGet(&store, 7)->bandwidthKHz);
  TEST_ASSERT_EQUAL_STRING("New", memoryGet(&store, 7)->name);
}

static void save_into_a_full_list_is_refused_and_changes_nothing(void) {
  MemoryChannel c = channel(BAND_FM, 88000, "");
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    TEST_ASSERT_TRUE(memorySet(&store, i, &c));
  }
  MemoryStore before = store;
  int slot = MEMORY_NO_SLOT;
  TEST_ASSERT_EQUAL_INT(
      MEMORY_SAVE_FULL,
      memorySaveChannel(&store, BAND_FM, 98300, 0, NULL, &slot));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, slot);
  TEST_ASSERT_EQUAL_MEMORY(&before, &store, sizeof(store));
}

static void save_of_what_no_preset_can_hold_is_refused_and_changes_nothing(
    void) {
  twoOnOneStation();
  MemoryStore before = store;
  int slot = MEMORY_NO_SLOT;
  TEST_ASSERT_EQUAL_INT(
      MEMORY_SAVE_INVALID,
      memorySaveChannel(&store, BAND_FM, 98300, 999, NULL, &slot));
  TEST_ASSERT_EQUAL_INT(
      MEMORY_SAVE_INVALID,
      memorySaveChannel(&store, BAND_FM, 98300, 0, "a\nb", &slot));
  TEST_ASSERT_EQUAL_INT(MEMORY_SAVE_INVALID,
                        memorySaveChannel(&store, BAND_FM, 0, 0, NULL, &slot));
  TEST_ASSERT_EQUAL_INT(
      MEMORY_SAVE_INVALID,
      memorySaveChannel(&store, BAND_COUNT, 98300, 0, NULL, &slot));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, slot);
  /* A named slot outside the store is refused and left as it was. */
  int outside = MEMORY_SLOT_COUNT;
  TEST_ASSERT_EQUAL_INT(
      MEMORY_SAVE_INVALID,
      memorySaveChannel(&store, BAND_FM, 98300, 0, NULL, &outside));
  TEST_ASSERT_EQUAL_INT(MEMORY_SLOT_COUNT, outside);
  outside = -2;
  TEST_ASSERT_EQUAL_INT(
      MEMORY_SAVE_INVALID,
      memorySaveChannel(&store, BAND_FM, 98300, 0, NULL, &outside));
  TEST_ASSERT_EQUAL_INT(-2, outside);
  TEST_ASSERT_EQUAL_MEMORY(&before, &store, sizeof(store));
  TEST_ASSERT_EQUAL_INT(
      MEMORY_SAVE_INVALID,
      memorySaveChannel(NULL, BAND_FM, 98300, 0, NULL, &slot));
  TEST_ASSERT_EQUAL_INT(
      MEMORY_SAVE_INVALID,
      memorySaveChannel(&store, BAND_FM, 98300, 0, NULL, NULL));
}

static void save_cuts_a_long_name_to_fit(void) {
  int slot = MEMORY_NO_SLOT;
  TEST_ASSERT_EQUAL_INT(MEMORY_SAVE_OK,
                        memorySaveChannel(&store, BAND_FM, 98300, 0,
                                          "Radio Mirchi Long Name", &slot));
  TEST_ASSERT_EQUAL_STRING("Radio Mirchi Lon", memoryGet(&store, slot)->name);
}

/* The used slots in slot order, the gaps passed over, for the menu's list
 * of presets. */
static void the_nth_used_slot_passes_over_the_empty_ones(void) {
  MemoryChannel a = channel(BAND_FM, 98300, "MIRCHI");
  MemoryChannel b = channel(BAND_MW, 738, "");
  TEST_ASSERT_TRUE(memorySet(&store, 2, &a));
  TEST_ASSERT_TRUE(memorySet(&store, 40, &b));
  TEST_ASSERT_TRUE(memorySet(&store, MEMORY_SLOT_COUNT - 1, &a));
  TEST_ASSERT_EQUAL_INT(2, memoryNthUsed(&store, 0));
  TEST_ASSERT_EQUAL_INT(40, memoryNthUsed(&store, 1));
  TEST_ASSERT_EQUAL_INT(MEMORY_SLOT_COUNT - 1, memoryNthUsed(&store, 2));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryNthUsed(&store, 3));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryNthUsed(&store, -1));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryNthUsed(NULL, 0));
}

/* A step through the store under the default band plan, where every FM
 * channel these tests store can be tuned. */
static int step(int from, bool up) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  return memoryStepTunable(&store, &plan, from, up);
}

static void a_new_store_holds_nothing(void) {
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&store));
  TEST_ASSERT_EQUAL_INT(0, memoryFirstFree(&store));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, step(MEMORY_NO_SLOT, true));
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    TEST_ASSERT_FALSE(memorySlotUsed(&store, i));
    TEST_ASSERT_NULL(memoryGet(&store, i));
  }
}

static void a_null_store_answers_without_crashing(void) {
  memoryInit(NULL);
  TEST_ASSERT_EQUAL_INT(0, memoryCount(NULL));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryFirstFree(NULL));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryFind(NULL, BAND_FM, 92700));
  TEST_ASSERT_FALSE(memorySlotUsed(NULL, 0));
  TEST_ASSERT_NULL(memoryGet(NULL, 0));
  MemoryChannel c = channel(BAND_FM, 92700, "x");
  TEST_ASSERT_FALSE(memorySet(NULL, 0, &c));
  TEST_ASSERT_FALSE(memoryChannelValid(NULL));
  TEST_ASSERT_FALSE(memoryChannelTunable(NULL, NULL));
}

static void a_channel_comes_back_as_it_went_in(void) {
  MemoryChannel c = channel(BAND_MW, 1071, "Vividh Bharati");
  c.bandwidthKHz = 6;
  TEST_ASSERT_TRUE(memorySet(&store, 4, &c));
  const MemoryChannel *back = memoryGet(&store, 4);
  TEST_ASSERT_NOT_NULL(back);
  TEST_ASSERT_EQUAL_UINT32(1071, back->freqKHz);
  TEST_ASSERT_EQUAL_UINT16(6, back->bandwidthKHz);
  TEST_ASSERT_EQUAL_UINT8(BAND_MW, back->band);
  TEST_ASSERT_EQUAL_STRING("Vividh Bharati", back->name);
  TEST_ASSERT_EQUAL_INT(1, memoryCount(&store));
}

static void the_slot_numbers_on_the_boundary_and_either_side(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "ok");
  TEST_ASSERT_FALSE(memorySlotInRange(-1));
  TEST_ASSERT_TRUE(memorySlotInRange(0));
  TEST_ASSERT_TRUE(memorySlotInRange(MEMORY_SLOT_COUNT - 1));
  TEST_ASSERT_FALSE(memorySlotInRange(MEMORY_SLOT_COUNT));

  TEST_ASSERT_FALSE(memorySet(&store, -1, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));
  TEST_ASSERT_TRUE(memorySet(&store, MEMORY_SLOT_COUNT - 1, &c));
  TEST_ASSERT_FALSE(memorySet(&store, MEMORY_SLOT_COUNT, &c));
}

static void a_channel_with_no_frequency_is_an_empty_slot(void) {
  MemoryChannel c = channel(BAND_FM, 0, "nowhere");
  TEST_ASSERT_FALSE(memoryChannelValid(&c));
  TEST_ASSERT_FALSE(memorySet(&store, 0, &c));
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&store));
}

static void a_band_this_radio_does_not_have_is_refused(void) {
  MemoryChannel c = channel(BAND_COUNT, 92700, "nowhere");
  TEST_ASSERT_FALSE(memoryChannelValid(&c));
  c.band = 200;
  TEST_ASSERT_FALSE(memoryChannelValid(&c));
}

static void a_name_that_never_ends_is_refused(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "");
  memset(c.name, 'A', MEMORY_NAME_LEN);
  TEST_ASSERT_FALSE(memoryChannelValid(&c));
  c.name[MEMORY_NAME_LEN - 1] = '\0';
  TEST_ASSERT_TRUE(memoryChannelValid(&c));
}

static void a_name_carrying_a_newline_is_refused(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "two\nlines");
  TEST_ASSERT_FALSE(memoryChannelValid(&c));
  MemoryChannel tab = channel(BAND_FM, 92700, "a\tb");
  TEST_ASSERT_FALSE(memoryChannelValid(&tab));
  MemoryChannel high = channel(BAND_FM, 92700, "x");
  high.name[0] = (char)0x7F;
  TEST_ASSERT_FALSE(memoryChannelValid(&high));
  MemoryChannel edges = channel(BAND_FM, 92700, " ~");
  TEST_ASSERT_TRUE(memoryChannelValid(&edges));
}

static void a_name_may_hold_any_printable_character(void) {
  const char *names[] = {"=1+1", "-Fm", "@Radio", "100% Hits", "Radio 1+1"};
  for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
    MemoryChannel c = channel(BAND_FM, 92700, names[i]);
    TEST_ASSERT_TRUE(memoryChannelValid(&c));
  }
}

static void an_empty_name_is_allowed(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "");
  TEST_ASSERT_TRUE(memoryChannelValid(&c));
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));
}

static void a_width_that_band_does_not_offer_is_refused(void) {
  MemoryChannel am = channel(BAND_MW, 1071, "am");
  am.bandwidthKHz = 5;
  TEST_ASSERT_FALSE(memoryChannelValid(&am));
  am.bandwidthKHz = 6;
  TEST_ASSERT_TRUE(memoryChannelValid(&am));

  MemoryChannel fm = channel(BAND_FM, 92700, "fm");
  fm.bandwidthKHz = 6;
  TEST_ASSERT_FALSE(memoryChannelValid(&fm));
  fm.bandwidthKHz = 56;
  TEST_ASSERT_TRUE(memoryChannelValid(&fm));
}

static void a_width_of_zero_is_allowed_on_every_band(void) {
  for (int b = 0; b < BAND_COUNT; b++) {
    MemoryChannel c = channel((uint8_t)b, 1000, "auto");
    c.bandwidthKHz = 0;
    TEST_ASSERT_TRUE(memoryChannelValid(&c));
  }
}

static void the_tail_of_a_name_is_zeroed_when_it_is_stored(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "");
  memset(c.name, 'B', MEMORY_NAME_LEN - 1);
  c.name[MEMORY_NAME_LEN - 1] = '\0';
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));

  MemoryChannel shortName = channel(BAND_FM, 92700, "Hi");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &shortName));
  const MemoryChannel *back = memoryGet(&store, 0);
  for (size_t i = strlen("Hi"); i < MEMORY_NAME_LEN; i++) {
    TEST_ASSERT_EQUAL_CHAR('\0', back->name[i]);
  }
}

static void the_first_free_slot_is_the_lowest_empty_one(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 1, &c));
  TEST_ASSERT_EQUAL_INT(2, memoryFirstFree(&store));
  memset(&store.slot[0], 0, sizeof(store.slot[0]));
  TEST_ASSERT_EQUAL_INT(0, memoryFirstFree(&store));
}

static void a_full_store_has_no_free_slot(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    TEST_ASSERT_TRUE(memorySet(&store, i, &c));
  }
  TEST_ASSERT_EQUAL_INT(MEMORY_SLOT_COUNT, memoryCount(&store));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryFirstFree(&store));
}

static void find_needs_the_band_and_the_frequency_to_agree(void) {
  MemoryChannel fm = channel(BAND_FM, 92700, "fm");
  MemoryChannel oirt = channel(BAND_OIRT, 70000, "oirt");
  TEST_ASSERT_TRUE(memorySet(&store, 3, &fm));
  TEST_ASSERT_TRUE(memorySet(&store, 9, &oirt));

  TEST_ASSERT_EQUAL_INT(3, memoryFind(&store, BAND_FM, 92700));
  TEST_ASSERT_EQUAL_INT(9, memoryFind(&store, BAND_OIRT, 70000));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryFind(&store, BAND_OIRT, 92700));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryFind(&store, BAND_FM, 92800));
  /* An empty slot is a frequency of 0, so looking for 0 must never find one. */
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryFind(&store, BAND_FM, 0));
}

static void find_gives_the_lowest_of_two_that_match(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 20, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 40, &c));
  TEST_ASSERT_EQUAL_INT(20, memoryFind(&store, BAND_FM, 92700));
}

static void find_near_catches_a_close_frequency(void) {
  /* A scan and a person's own save rarely land on the exact same
   * frequency: a scan walks a fixed raster and a person can store
   * whatever the dial was on. */
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 3, &c));
  TEST_ASSERT_EQUAL_INT(3, memoryFindNear(&store, BAND_FM, 92700, 100));
  TEST_ASSERT_EQUAL_INT(3, memoryFindNear(&store, BAND_FM, 92640, 100));
  TEST_ASSERT_EQUAL_INT(3, memoryFindNear(&store, BAND_FM, 92760, 100));
}

static void find_near_refuses_past_the_tolerance(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 3, &c));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryFindNear(&store, BAND_FM, 92900, 100));
  /* The band still has to agree, the same as memoryFind. */
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryFindNear(&store, BAND_OIRT, 92700, 100));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryFindNear(&store, BAND_FM, 0, 100));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryFindNear(NULL, BAND_FM, 92700, 100));
}

static void stepping_goes_over_the_empty_slots(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 2, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 50, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 90, &c));

  TEST_ASSERT_EQUAL_INT(50, step(2, true));
  TEST_ASSERT_EQUAL_INT(90, step(50, true));
  TEST_ASSERT_EQUAL_INT(50, step(90, false));
  TEST_ASSERT_EQUAL_INT(2, step(50, false));
  /* Starting from an empty slot works too, which is what happens when a
   * channel is deleted while the radio is sitting on it. */
  TEST_ASSERT_EQUAL_INT(50, step(30, true));
  TEST_ASSERT_EQUAL_INT(2, step(30, false));
}

static void stepping_wraps_at_both_ends(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 2, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 90, &c));
  TEST_ASSERT_EQUAL_INT(2, step(90, true));
  TEST_ASSERT_EQUAL_INT(90, step(2, false));
}

static void stepping_from_outside_the_store_lands_on_the_end(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 2, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 90, &c));
  TEST_ASSERT_EQUAL_INT(2, step(MEMORY_NO_SLOT, true));
  TEST_ASSERT_EQUAL_INT(90, step(MEMORY_NO_SLOT, false));
  TEST_ASSERT_EQUAL_INT(2, step(MEMORY_SLOT_COUNT, true));
  TEST_ASSERT_EQUAL_INT(90, step(MEMORY_SLOT_COUNT, false));
}

static void the_only_channel_steps_to_itself(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 40, &c));
  TEST_ASSERT_EQUAL_INT(40, step(40, true));
  TEST_ASSERT_EQUAL_INT(40, step(40, false));
}

static void an_empty_store_steps_nowhere(void) {
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, step(0, true));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, step(0, false));
}

static void a_channel_outside_the_band_plan_in_force_is_not_tunable(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  MemoryChannel c = channel(BAND_FM, 76000, "japan");
  /* Stored while the radio was set to the full FM range, then read back after
   * the region moved to 87.5 to 108. The channel is still a sound record of
   * what was stored, so it stays in the list, but it cannot be tuned. */
  TEST_ASSERT_TRUE(memoryChannelValid(&c));
  plan.fmRegion = FM_REGION_WORLD;
  TEST_ASSERT_FALSE(memoryChannelTunable(&c, &plan));
  plan.fmRegion = FM_REGION_FULL;
  TEST_ASSERT_TRUE(memoryChannelTunable(&c, &plan));
  TEST_ASSERT_FALSE(memoryChannelTunable(&c, NULL));
}

static void a_channel_the_plan_would_put_on_another_band_is_not_tunable(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  plan.fmRegion = FM_REGION_FULL;
  /* 70 MHz is inside the full FM region and inside OIRT, and the band plan
   * picks OIRT for it. A channel stored as FM there would tune to OIRT, with
   * OIRT's step size, so it does not count as reachable. */
  MemoryChannel asFm = channel(BAND_FM, 70000, "overlap");
  MemoryChannel asOirt = channel(BAND_OIRT, 70000, "overlap");
  TEST_ASSERT_TRUE(memoryChannelValid(&asFm));
  TEST_ASSERT_FALSE(memoryChannelTunable(&asFm, &plan));
  TEST_ASSERT_TRUE(memoryChannelTunable(&asOirt, &plan));

  TEST_ASSERT_TRUE(memorySet(&store, 0, &asFm));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryStepTunable(&store, &plan, MEMORY_NO_SLOT, true));
}

static void a_frequency_in_no_band_at_all_is_not_tunable(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  MemoryChannel gap = channel(BAND_SW, 40000, "above the top");
  TEST_ASSERT_TRUE(memoryChannelValid(&gap));
  TEST_ASSERT_FALSE(memoryChannelTunable(&gap, &plan));
}

static void a_channel_can_be_valid_and_still_not_be_on_its_band(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  /* What an imported file can carry: a real band, a real frequency, and the
   * two not belonging together. It is a sound record and the list keeps it,
   * but tuning it would land the radio on medium wave while the list went on
   * saying FM. */
  MemoryChannel c = channel(BAND_FM, 1000, "wrong band");
  TEST_ASSERT_TRUE(memoryChannelValid(&c));
  TEST_ASSERT_FALSE(memoryChannelTunable(&c, &plan));
}

static void stepping_goes_over_a_channel_the_band_plan_cannot_reach(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  plan.fmRegion = FM_REGION_WORLD;
  MemoryChannel reachable = channel(BAND_FM, 92700, "here");
  MemoryChannel outside = channel(BAND_FM, 76000, "japan");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &reachable));
  TEST_ASSERT_TRUE(memorySet(&store, 1, &outside));
  TEST_ASSERT_TRUE(memorySet(&store, 2, &reachable));

  TEST_ASSERT_EQUAL_INT(2, memoryStepTunable(&store, &plan, 0, true));
  TEST_ASSERT_EQUAL_INT(0, memoryStepTunable(&store, &plan, 2, true));
  TEST_ASSERT_EQUAL_INT(0, memoryStepTunable(&store, &plan, 2, false));
}

static void stepping_down_past_the_first_slot_wraps_to_the_last(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));
  TEST_ASSERT_TRUE(memorySet(&store, MEMORY_SLOT_COUNT - 1, &c));
  TEST_ASSERT_EQUAL_INT(MEMORY_SLOT_COUNT - 1,
                        memoryStepTunable(&store, &plan, 0, false));
  TEST_ASSERT_EQUAL_INT(
      0, memoryStepTunable(&store, &plan, MEMORY_SLOT_COUNT - 1, true));
}

static void a_store_with_nothing_reachable_steps_nowhere(void) {
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  plan.fmRegion = FM_REGION_WORLD;
  MemoryChannel outside = channel(BAND_FM, 76000, "japan");
  TEST_ASSERT_TRUE(memorySet(&store, 5, &outside));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryStepTunable(&store, &plan, MEMORY_NO_SLOT, true));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryStepTunable(&store, NULL, MEMORY_NO_SLOT, true));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryStepTunable(NULL, &plan, MEMORY_NO_SLOT, true));
}

/* A store that came back from flash rather than through memorySet. The tests
 * that put a bad slot in do not use memorySet, because memorySet is what these
 * slots got past. */
static void sanitize_clears_a_name_that_never_ends(void) {
  MemoryChannel c = channel(BAND_FM, 104000, "");
  memset(c.name, 'A', MEMORY_NAME_LEN);
  store.slot[3] = c;
  TEST_ASSERT_EQUAL_INT(1, memorySanitize(&store));
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&store));
  TEST_ASSERT_EQUAL_UINT32(0, store.slot[3].freqKHz);
  TEST_ASSERT_EQUAL_CHAR('\0', store.slot[3].name[0]);
}

static void sanitize_clears_a_name_with_a_control_character(void) {
  MemoryChannel c = channel(BAND_FM, 104000, "BBC");
  c.name[1] = '\n';
  store.slot[0] = c;
  TEST_ASSERT_EQUAL_INT(1, memorySanitize(&store));
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&store));
}

static void sanitize_clears_a_band_this_radio_does_not_have(void) {
  store.slot[0] = channel(BAND_COUNT, 104000, "BBC");
  TEST_ASSERT_EQUAL_INT(1, memorySanitize(&store));
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&store));
}

static void sanitize_clears_a_width_the_band_does_not_offer(void) {
  MemoryChannel c = channel(BAND_FM, 104000, "BBC");
  c.bandwidthKHz = 3;
  store.slot[0] = c;
  TEST_ASSERT_EQUAL_INT(1, memorySanitize(&store));
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&store));
}

static void sanitize_leaves_a_good_store_alone(void) {
  MemoryChannel a = channel(BAND_FM, 104000, "BBC");
  MemoryChannel b = channel(BAND_MW, 909, "");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &a));
  TEST_ASSERT_TRUE(memorySet(&store, 98, &b));
  MemoryStore before = store;
  TEST_ASSERT_EQUAL_INT(0, memorySanitize(&store));
  TEST_ASSERT_EQUAL_INT(2, memoryCount(&store));
  TEST_ASSERT_EQUAL_MEMORY(&before, &store, sizeof(store));
}

/* An empty slot carrying junk is tidied but is not a channel, so nothing was
 * lost and the count says so. */
static void sanitize_counts_only_the_slots_that_held_a_channel(void) {
  MemoryChannel bad = channel(BAND_FM, 104000, "");
  memset(bad.name, 'A', MEMORY_NAME_LEN);
  store.slot[0] = bad;
  store.slot[1] = bad;
  store.slot[2] = bad;
  store.slot[2].freqKHz = 0;
  TEST_ASSERT_EQUAL_INT(2, memorySanitize(&store));
  TEST_ASSERT_EQUAL_CHAR('\0', store.slot[2].name[0]);
}

static void sanitize_takes_a_null_store(void) {
  TEST_ASSERT_EQUAL_INT(0, memorySanitize(NULL));
}

/* ------------------------------------------------------------ stored form */

/* A list with a few channels spread over it, so a slot copied to the wrong
 * place or a field shifted would show. */
static void fillSome(MemoryStore *m) {
  memoryInit(m);
  MemoryChannel a = channel(BAND_FM, 106400, "MAGIC");
  MemoryChannel b = channel(BAND_MW, 1071, "Vividh Bharati");
  b.bandwidthKHz = 6;
  MemoryChannel c = channel(BAND_FM, 91100, "");
  memorySet(m, 0, &a);
  memorySet(m, 41, &b);
  memorySet(m, MEMORY_SLOT_COUNT - 1, &c);
}

/*
 * The pin on the stored form. It fails when MemoryChannel or the slot count
 * changes, which changes what is written to flash: bump MEMORY_BLOB_VERSION,
 * teach memoryFromBlob to read the versions before into the new shape, and
 * move this number.
 */
static void a_store_is_the_size_version_2_writes(void) {
  TEST_ASSERT_EQUAL_UINT32(2772, sizeof(MemoryStore));
  TEST_ASSERT_EQUAL_INT(2, MEMORY_BLOB_VERSION);
  TEST_ASSERT_EQUAL_UINT32(8 + 2772, memoryBlobSize());
}

/* A list in version 1's layout, 24 bytes a channel and no PI. */
static void toVersion1(const MemoryStore *m, uint8_t *out) {
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    const MemoryChannel *c = &m->slot[i];
    uint8_t *at = out + i * 24;
    memcpy(at, &c->freqKHz, 4);
    memcpy(at + 4, &c->bandwidthKHz, 2);
    at[6] = c->band;
    memcpy(at + 7, c->name, MEMORY_NAME_LEN);
  }
}

static void sameChannelsNoPi(const MemoryStore *want, const MemoryStore *got) {
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    TEST_ASSERT_EQUAL_UINT32(want->slot[i].freqKHz, got->slot[i].freqKHz);
    TEST_ASSERT_EQUAL_UINT16(want->slot[i].bandwidthKHz,
                             got->slot[i].bandwidthKHz);
    TEST_ASSERT_EQUAL_UINT8(want->slot[i].band, got->slot[i].band);
    TEST_ASSERT_EQUAL_STRING(want->slot[i].name, got->slot[i].name);
    TEST_ASSERT_EQUAL_UINT16(0, got->slot[i].pi);
  }
}

/* A version 1 list with its header, as every radio has in flash before the
 * PI: every channel kept, every PI 0. */
static void a_version_1_list_is_read_with_no_pi(void) {
  MemoryStore before;
  fillSome(&before);
  static uint8_t blob[8 + 2376];
  const uint32_t magic = 0x4D454D53UL;
  const uint16_t version = 1;
  const uint16_t body = 2376;
  memcpy(blob, &magic, 4);
  memcpy(blob + 4, &version, 2);
  memcpy(blob + 6, &body, 2);
  toVersion1(&before, blob + 8);
  MemoryStore after;
  TEST_ASSERT_TRUE(memoryFromBlob(blob, sizeof(blob), &after));
  sameChannelsNoPi(&before, &after);
  TEST_ASSERT_EQUAL_STRING("Vividh Bharati", memoryGet(&after, 41)->name);
}

static void a_list_comes_back_from_its_stored_form(void) {
  MemoryStore before;
  fillSome(&before);
  before.slot[41].pi = 0x5A17;
  static uint8_t blob[8 + 2772];
  TEST_ASSERT_EQUAL_UINT32(sizeof(blob),
                           memoryToBlob(&before, blob, sizeof(blob)));
  MemoryStore after;
  TEST_ASSERT_TRUE(memoryFromBlob(blob, sizeof(blob), &after));
  TEST_ASSERT_EQUAL_MEMORY(&before, &after, sizeof(MemoryStore));
  TEST_ASSERT_EQUAL_STRING("Vividh Bharati", memoryGet(&after, 41)->name);
  TEST_ASSERT_EQUAL_UINT16(0x5A17, memoryGet(&after, 41)->pi);
}

/* What the first firmware wrote: the bare version 1 list, no header. */
static void a_bare_list_from_the_first_firmware_is_read(void) {
  MemoryStore before;
  fillSome(&before);
  static uint8_t bare[2376];
  toVersion1(&before, bare);
  MemoryStore after;
  TEST_ASSERT_TRUE(memoryFromBlob(bare, sizeof(bare), &after));
  sameChannelsNoPi(&before, &after);
}

static void a_later_version_is_refused_and_leaves_the_list_empty(void) {
  MemoryStore before;
  fillSome(&before);
  static uint8_t blob[8 + 2772];
  memoryToBlob(&before, blob, sizeof(blob));
  const uint16_t later = MEMORY_BLOB_VERSION + 1;
  memcpy(blob + 4, &later, sizeof(later));
  MemoryStore after;
  fillSome(&after);
  TEST_ASSERT_FALSE(memoryFromBlob(blob, sizeof(blob), &after));
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&after));
}

static void a_header_that_does_not_match_what_follows_is_refused(void) {
  MemoryStore before;
  fillSome(&before);
  static uint8_t blob[8 + 2772];
  memoryToBlob(&before, blob, sizeof(blob));
  MemoryStore after;
  /* One byte short of what the header says. */
  TEST_ASSERT_FALSE(memoryFromBlob(blob, sizeof(blob) - 1, &after));
  /* A body length that is not this list's. */
  const uint16_t wrong = 2771;
  memcpy(blob + 6, &wrong, sizeof(wrong));
  TEST_ASSERT_FALSE(memoryFromBlob(blob, sizeof(blob), &after));
}

static void a_bare_list_of_another_length_is_refused(void) {
  static uint8_t bare[2376 + 4];
  memset(bare, 0, sizeof(bare));
  MemoryStore after;
  TEST_ASSERT_FALSE(memoryFromBlob(bare, sizeof(bare), &after));
  TEST_ASSERT_FALSE(memoryFromBlob(bare, 2372, &after));
  TEST_ASSERT_FALSE(memoryFromBlob(bare, 0, &after));
  TEST_ASSERT_FALSE(memoryFromBlob(bare, 7, &after));
}

static void the_stored_form_takes_null_and_short_buffers(void) {
  MemoryStore m;
  fillSome(&m);
  static uint8_t blob[8 + 2772];
  TEST_ASSERT_EQUAL_UINT32(0, memoryToBlob(NULL, blob, sizeof(blob)));
  TEST_ASSERT_EQUAL_UINT32(0, memoryToBlob(&m, NULL, sizeof(blob)));
  TEST_ASSERT_EQUAL_UINT32(0, memoryToBlob(&m, blob, sizeof(blob) - 1));
  TEST_ASSERT_FALSE(memoryFromBlob(NULL, sizeof(blob), &m));
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&m));
  TEST_ASSERT_FALSE(memoryFromBlob(blob, sizeof(blob), NULL));
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(a_new_store_holds_nothing);
  RUN_TEST(a_null_store_answers_without_crashing);
  RUN_TEST(a_channel_comes_back_as_it_went_in);
  RUN_TEST(the_slot_numbers_on_the_boundary_and_either_side);
  RUN_TEST(a_channel_with_no_frequency_is_an_empty_slot);
  RUN_TEST(a_band_this_radio_does_not_have_is_refused);
  RUN_TEST(a_name_that_never_ends_is_refused);
  RUN_TEST(a_name_carrying_a_newline_is_refused);
  RUN_TEST(a_name_may_hold_any_printable_character);
  RUN_TEST(an_empty_name_is_allowed);
  RUN_TEST(a_width_that_band_does_not_offer_is_refused);
  RUN_TEST(a_width_of_zero_is_allowed_on_every_band);
  RUN_TEST(the_tail_of_a_name_is_zeroed_when_it_is_stored);
  RUN_TEST(the_first_free_slot_is_the_lowest_empty_one);
  RUN_TEST(a_full_store_has_no_free_slot);
  RUN_TEST(find_needs_the_band_and_the_frequency_to_agree);
  RUN_TEST(find_gives_the_lowest_of_two_that_match);
  RUN_TEST(find_near_catches_a_close_frequency);
  RUN_TEST(find_near_refuses_past_the_tolerance);
  RUN_TEST(stepping_goes_over_the_empty_slots);
  RUN_TEST(stepping_wraps_at_both_ends);
  RUN_TEST(stepping_from_outside_the_store_lands_on_the_end);
  RUN_TEST(the_only_channel_steps_to_itself);
  RUN_TEST(an_empty_store_steps_nowhere);
  RUN_TEST(a_channel_outside_the_band_plan_in_force_is_not_tunable);
  RUN_TEST(stepping_goes_over_a_channel_the_band_plan_cannot_reach);
  RUN_TEST(stepping_down_past_the_first_slot_wraps_to_the_last);
  RUN_TEST(a_store_with_nothing_reachable_steps_nowhere);
  RUN_TEST(a_channel_the_plan_would_put_on_another_band_is_not_tunable);
  RUN_TEST(a_frequency_in_no_band_at_all_is_not_tunable);
  RUN_TEST(a_channel_can_be_valid_and_still_not_be_on_its_band);
  RUN_TEST(sanitize_clears_a_name_that_never_ends);
  RUN_TEST(sanitize_clears_a_name_with_a_control_character);
  RUN_TEST(sanitize_clears_a_band_this_radio_does_not_have);
  RUN_TEST(sanitize_clears_a_width_the_band_does_not_offer);
  RUN_TEST(sanitize_leaves_a_good_store_alone);
  RUN_TEST(sanitize_counts_only_the_slots_that_held_a_channel);
  RUN_TEST(sanitize_takes_a_null_store);

  RUN_TEST(a_store_is_the_size_version_2_writes);
  RUN_TEST(a_version_1_list_is_read_with_no_pi);
  RUN_TEST(a_list_comes_back_from_its_stored_form);
  RUN_TEST(a_bare_list_from_the_first_firmware_is_read);
  RUN_TEST(a_later_version_is_refused_and_leaves_the_list_empty);
  RUN_TEST(a_header_that_does_not_match_what_follows_is_refused);
  RUN_TEST(a_bare_list_of_another_length_is_refused);
  RUN_TEST(the_stored_form_takes_null_and_short_buffers);
  RUN_TEST(pick_keeps_the_chosen_slot_while_it_holds_the_dial);
  RUN_TEST(pick_falls_to_the_second_then_to_the_lowest);
  RUN_TEST(pick_answers_no_slot_off_every_preset);
  RUN_TEST(pick_passes_over_a_slot_outside_the_store);
  RUN_TEST(save_goes_into_the_lowest_free_slot);
  RUN_TEST(save_replaces_a_named_slot_outright);
  RUN_TEST(save_into_a_full_list_is_refused_and_changes_nothing);
  RUN_TEST(save_of_what_no_preset_can_hold_is_refused_and_changes_nothing);
  RUN_TEST(save_cuts_a_long_name_to_fit);
  RUN_TEST(the_nth_used_slot_passes_over_the_empty_ones);

  return UNITY_END();
}
