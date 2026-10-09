/* Tests for the table of stored settings. Runs on a PC. */
#include <unity.h>

#include <ctype.h>
#include <string.h>

#include "core/band_plan.h"
#include "core/memory.h"
#include "core/settings.h"
#include "core/settings_table.h"

void setUp(void) {}
void tearDown(void) {}

/* Two rows whose range is only an outer bound: the caller refuses the
 * values inside it that are not legal. */
static bool outerBoundOnly(const SettingRow *row) {
  return strcmp(row->key, "rot") == 0 || strcmp(row->key, "dbw") == 0;
}

static void every_key_is_three_small_letters_and_used_once(void) {
  TEST_ASSERT_TRUE(settingsTableCount() > 0);
  for (size_t i = 0; i < settingsTableCount(); i++) {
    const SettingRow *row = settingsTableAt(i);
    TEST_ASSERT_NOT_NULL(row);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(3, strlen(row->key), row->key);
    for (int c = 0; c < 3; c++) {
      TEST_ASSERT_TRUE_MESSAGE(islower((unsigned char)row->key[c]), row->key);
    }
    for (size_t j = i + 1; j < settingsTableCount(); j++) {
      TEST_ASSERT_NOT_EQUAL_MESSAGE(
          0, strcmp(row->key, settingsTableAt(j)->key), row->key);
    }
  }
}

/* Each row is its own field: inside the struct, and no two rows on the
 * same byte. */
static void every_row_is_its_own_field_inside_the_struct(void) {
  for (size_t i = 0; i < settingsTableCount(); i++) {
    const SettingRow *row = settingsTableAt(i);
    TEST_ASSERT_TRUE_MESSAGE(row->size == 1 || row->size == 2, row->key);
    TEST_ASSERT_TRUE_MESSAGE(row->offset + row->size <= sizeof(Settings),
                             row->key);
    for (size_t j = i + 1; j < settingsTableCount(); j++) {
      const SettingRow *other = settingsTableAt(j);
      const bool apart = row->offset + row->size <= other->offset ||
                         other->offset + other->size <= row->offset;
      TEST_ASSERT_TRUE_MESSAGE(apart, row->key);
    }
  }
}

/* The range fits the field, so a value the API takes is never cut short. */
static void every_range_fits_its_field(void) {
  for (size_t i = 0; i < settingsTableCount(); i++) {
    const SettingRow *row = settingsTableAt(i);
    TEST_ASSERT_TRUE_MESSAGE(row->low <= row->high, row->key);
    const int32_t most = row->size == 2 ? 65535 : 255;
    TEST_ASSERT_TRUE_MESSAGE(row->high <= most, row->key);
    TEST_ASSERT_TRUE_MESSAGE(row->low >= 0, row->key);
  }
}

/* The defaults sit inside every row's range. */
static void the_defaults_are_inside_every_range(void) {
  Settings s;
  settingsDefaults(&s);
  for (size_t i = 0; i < settingsTableCount(); i++) {
    const SettingRow *row = settingsTableAt(i);
    const int32_t v = settingsTableGet(&s, row);
    TEST_ASSERT_TRUE_MESSAGE(v >= row->low && v <= row->high, row->key);
  }
}

/* Both ends of every range read back as written, at the field's own width
 * and sign, and leave settings the radio would load. */
static void both_ends_of_every_range_are_valid_settings(void) {
  for (size_t i = 0; i < settingsTableCount(); i++) {
    const SettingRow *row = settingsTableAt(i);
    if (outerBoundOnly(row)) {
      continue;
    }
    const int32_t ends[2] = {row->low, row->high};
    for (int e = 0; e < 2; e++) {
      Settings s;
      settingsDefaults(&s);
      /* The first preset of the DX walk has to stay at or below the last,
       * so each end is tried with the other one out of the way. */
      if (strcmp(row->key, "dmf") == 0) {
        s.dxMemLast = MEMORY_SLOT_COUNT;
      }
      if (strcmp(row->key, "dml") == 0) {
        s.dxMemFirst = 1;
      }
      settingsTableSet(&s, row, ends[e]);
      TEST_ASSERT_EQUAL_INT32_MESSAGE(ends[e], settingsTableGet(&s, row),
                                      row->key);
      TEST_ASSERT_TRUE_MESSAGE(settingsValid(&s), row->key);
    }
  }
}

/* The two outer bound rows: every legal value is valid. */
static void the_legal_values_of_the_outer_bound_rows_are_valid(void) {
  Settings s;
  settingsDefaults(&s);
  const SettingRow *rot = settingsTableFind("rot");
  settingsTableSet(&s, rot, 180);
  TEST_ASSERT_EQUAL_UINT8(180, s.displayRotation);
  TEST_ASSERT_TRUE(settingsValid(&s));
  settingsDefaults(&s);
  const SettingRow *dbw = settingsTableFind("dbw");
  for (size_t i = 0; i < bandBandwidthCount(BAND_FM); i++) {
    const uint16_t khz = bandBandwidthAt(BAND_FM, i);
    if (khz == 0) {
      continue;
    }
    settingsTableSet(&s, dbw, khz);
    TEST_ASSERT_EQUAL_UINT16(khz, s.dxWidthKHz);
    TEST_ASSERT_TRUE(settingsValid(&s));
  }
}

/* A two byte field is written and read whole, a byte as a byte. */
static void a_row_writes_its_own_field_at_its_own_width(void) {
  Settings s;
  settingsDefaults(&s);
  settingsTableSet(&s, settingsTableFind("slp"), 600);
  TEST_ASSERT_EQUAL_UINT16(600, s.autoOffMinutes);
  settingsTableSet(&s, settingsTableFind("ddw"), 300);
  TEST_ASSERT_EQUAL_UINT16(300, s.dxDwellTenths);
  settingsTableSet(&s, settingsTableFind("rgn"), 2);
  TEST_ASSERT_EQUAL_UINT8(2, s.fmRegion);
}

static void when_each_acts_is_on_the_row(void) {
  TEST_ASSERT_EQUAL(SETTING_ACTS_AT_START, settingsTableFind("rgn")->acts);
  TEST_ASSERT_EQUAL(SETTING_ACTS_AT_START, settingsTableFind("bps")->acts);
  TEST_ASSERT_EQUAL(SETTING_ACTS_NEXT_DX, settingsTableFind("dbw")->acts);
  TEST_ASSERT_EQUAL(SETTING_ACTS_NEXT_DX, settingsTableFind("dmf")->acts);
  TEST_ASSERT_EQUAL(SETTING_ACTS_NOW, settingsTableFind("thm")->acts);
}

static void a_key_that_is_not_a_setting_finds_nothing(void) {
  TEST_ASSERT_NULL(settingsTableFind("zzz"));
  TEST_ASSERT_NULL(settingsTableFind(""));
  TEST_ASSERT_NULL(settingsTableFind(NULL));
  TEST_ASSERT_NULL(settingsTableAt(settingsTableCount()));
  TEST_ASSERT_EQUAL_INT32(0, settingsTableGet(NULL, settingsTableAt(0)));
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_EQUAL_INT32(0, settingsTableGet(&s, NULL));
  settingsTableSet(NULL, settingsTableAt(0), 1); /* Must not crash. */
  settingsTableSet(&s, NULL, 1);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(every_key_is_three_small_letters_and_used_once);
  RUN_TEST(every_row_is_its_own_field_inside_the_struct);
  RUN_TEST(every_range_fits_its_field);
  RUN_TEST(the_defaults_are_inside_every_range);
  RUN_TEST(both_ends_of_every_range_are_valid_settings);
  RUN_TEST(the_legal_values_of_the_outer_bound_rows_are_valid);
  RUN_TEST(a_row_writes_its_own_field_at_its_own_width);
  RUN_TEST(when_each_acts_is_on_the_row);
  RUN_TEST(a_key_that_is_not_a_setting_finds_nothing);
  return UNITY_END();
}
