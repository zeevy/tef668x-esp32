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

static void a_new_store_holds_nothing(void) {
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&store));
  TEST_ASSERT_EQUAL_INT(0, memoryFirstFree(&store));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT,
                        memoryStep(&store, MEMORY_NO_SLOT, true));
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
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryStep(NULL, 0, true));
  TEST_ASSERT_FALSE(memorySlotUsed(NULL, 0));
  TEST_ASSERT_NULL(memoryGet(NULL, 0));
  MemoryChannel c = channel(BAND_FM, 92700, "x");
  TEST_ASSERT_FALSE(memorySet(NULL, 0, &c));
  TEST_ASSERT_FALSE(memoryClear(NULL, 0));
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
  TEST_ASSERT_FALSE(memoryClear(&store, MEMORY_SLOT_COUNT));
  TEST_ASSERT_TRUE(memoryClear(&store, 0));
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

static void clearing_a_slot_leaves_nothing_behind(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "gone");
  TEST_ASSERT_TRUE(memorySet(&store, 7, &c));
  TEST_ASSERT_TRUE(memoryClear(&store, 7));
  TEST_ASSERT_FALSE(memorySlotUsed(&store, 7));
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&store));
  TEST_ASSERT_TRUE(memoryClear(&store, 7));
}

static void the_first_free_slot_is_the_lowest_empty_one(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 1, &c));
  TEST_ASSERT_EQUAL_INT(2, memoryFirstFree(&store));
  TEST_ASSERT_TRUE(memoryClear(&store, 0));
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

static void stepping_goes_over_the_empty_slots(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 2, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 50, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 90, &c));

  TEST_ASSERT_EQUAL_INT(50, memoryStep(&store, 2, true));
  TEST_ASSERT_EQUAL_INT(90, memoryStep(&store, 50, true));
  TEST_ASSERT_EQUAL_INT(50, memoryStep(&store, 90, false));
  TEST_ASSERT_EQUAL_INT(2, memoryStep(&store, 50, false));
  /* Starting from an empty slot works too, which is what happens when a
   * channel is deleted while the radio is sitting on it. */
  TEST_ASSERT_EQUAL_INT(50, memoryStep(&store, 30, true));
  TEST_ASSERT_EQUAL_INT(2, memoryStep(&store, 30, false));
}

static void stepping_wraps_at_both_ends(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 2, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 90, &c));
  TEST_ASSERT_EQUAL_INT(2, memoryStep(&store, 90, true));
  TEST_ASSERT_EQUAL_INT(90, memoryStep(&store, 2, false));
}

static void stepping_from_outside_the_store_lands_on_the_end(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 2, &c));
  TEST_ASSERT_TRUE(memorySet(&store, 90, &c));
  TEST_ASSERT_EQUAL_INT(2, memoryStep(&store, MEMORY_NO_SLOT, true));
  TEST_ASSERT_EQUAL_INT(90, memoryStep(&store, MEMORY_NO_SLOT, false));
  TEST_ASSERT_EQUAL_INT(2, memoryStep(&store, MEMORY_SLOT_COUNT, true));
  TEST_ASSERT_EQUAL_INT(90, memoryStep(&store, MEMORY_SLOT_COUNT, false));
}

static void the_only_channel_steps_to_itself(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 40, &c));
  TEST_ASSERT_EQUAL_INT(40, memoryStep(&store, 40, true));
  TEST_ASSERT_EQUAL_INT(40, memoryStep(&store, 40, false));
}

static void an_empty_store_steps_nowhere(void) {
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryStep(&store, 0, true));
  TEST_ASSERT_EQUAL_INT(MEMORY_NO_SLOT, memoryStep(&store, 0, false));
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

  TEST_ASSERT_EQUAL_INT(1, memoryStep(&store, 0, true));
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
  RUN_TEST(clearing_a_slot_leaves_nothing_behind);
  RUN_TEST(the_first_free_slot_is_the_lowest_empty_one);
  RUN_TEST(a_full_store_has_no_free_slot);
  RUN_TEST(find_needs_the_band_and_the_frequency_to_agree);
  RUN_TEST(find_gives_the_lowest_of_two_that_match);
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

  return UNITY_END();
}
