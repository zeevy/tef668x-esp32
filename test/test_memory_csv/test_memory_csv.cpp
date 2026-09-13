/* Tests for the channel list as text. Runs on a PC. */
#include <unity.h>

#include <stdio.h>
#include <string.h>

#include "core/memory_csv.h"

static MemoryStore store;
static MemoryImportResult result;

void setUp(void) {
  memoryInit(&store);
  memset(&result, 0, sizeof(result));
}
void tearDown(void) {}

static MemoryChannel channel(uint8_t band, uint32_t freqKHz, const char *name) {
  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  c.band = band;
  c.freqKHz = freqKHz;
  strncpy(c.name, name, MEMORY_NAME_LEN - 1);
  return c;
}

static bool import(const char *text, MemoryImportMode mode) {
  return memoryImportCsv(&store, mode, text, strlen(text), &result);
}

static const char *line(int slot) {
  static char buffer[MEMORY_CSV_LINE_MAX];
  size_t need = memoryCsvLine(&store, slot, buffer, sizeof(buffer));
  TEST_ASSERT_LESS_THAN_UINT32(sizeof(buffer), need);
  return buffer;
}

static void the_header_names_every_column(void) {
  TEST_ASSERT_EQUAL_STRING("slot,band,frequency,bandwidth,name\n",
                           memoryCsvHeader());
}

static void a_channel_writes_one_line(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "Radio City");
  c.bandwidthKHz = 56;
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));
  TEST_ASSERT_EQUAL_STRING("1,FM,92700,56,Radio City\n", line(0));
}

static void the_slot_is_counted_from_one(void) {
  MemoryChannel c = channel(BAND_MW, 1071, "AIR");
  TEST_ASSERT_TRUE(memorySet(&store, 98, &c));
  TEST_ASSERT_EQUAL_STRING("99,MW,1071,0,AIR\n", line(98));
}

static void an_empty_slot_writes_nothing(void) {
  char buffer[MEMORY_CSV_LINE_MAX];
  TEST_ASSERT_EQUAL_UINT32(0, memoryCsvLine(&store, 0, buffer, sizeof(buffer)));
  TEST_ASSERT_EQUAL_UINT32(0,
                           memoryCsvLine(&store, -1, buffer, sizeof(buffer)));
  TEST_ASSERT_EQUAL_UINT32(0, memoryCsvLine(NULL, 0, buffer, sizeof(buffer)));
  MemoryChannel c = channel(BAND_FM, 92700, "a");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));
  TEST_ASSERT_EQUAL_UINT32(0, memoryCsvLine(&store, 0, NULL, 10));
}

static void a_name_with_a_comma_is_wrapped_in_quotes(void) {
  MemoryChannel c = channel(BAND_MW, 1071, "AIR, Hyderabad");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));
  TEST_ASSERT_EQUAL_STRING("1,MW,1071,0,\"AIR, Hyderabad\"\n", line(0));
}

static void a_quote_inside_a_name_is_doubled(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "The \"One\"");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));
  TEST_ASSERT_EQUAL_STRING("1,FM,92700,0,\"The \"\"One\"\"\"\n", line(0));
}

static void the_longest_line_fits_the_buffer(void) {
  MemoryChannel c = channel(BAND_OIRT, 74000, "");
  memset(c.name, '"', MEMORY_NAME_LEN - 1);
  c.name[MEMORY_NAME_LEN - 1] = '\0';
  c.bandwidthKHz = 311;
  TEST_ASSERT_TRUE(memorySet(&store, 98, &c));
  char buffer[MEMORY_CSV_LINE_MAX];
  size_t need = memoryCsvLine(&store, 98, buffer, sizeof(buffer));
  TEST_ASSERT_LESS_THAN_UINT32(MEMORY_CSV_LINE_MAX, need);
}

static void a_buffer_too_small_says_so_and_still_terminates(void) {
  MemoryChannel c = channel(BAND_FM, 92700, "Radio City");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &c));
  char small[8];
  memset(small, 'x', sizeof(small));
  size_t need = memoryCsvLine(&store, 0, small, sizeof(small));
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(sizeof(small), need);
  TEST_ASSERT_EQUAL_CHAR('\0', small[sizeof(small) - 1]);
  TEST_ASSERT_EQUAL_STRING("1,FM,92", small);
}

static void a_written_list_reads_back_the_same(void) {
  MemoryChannel a = channel(BAND_FM, 92700, "Radio City");
  a.bandwidthKHz = 56;
  MemoryChannel b = channel(BAND_MW, 1071, "AIR, Hyd");
  b.bandwidthKHz = 6;
  MemoryChannel c = channel(BAND_SW, 9420, "The \"One\"");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &a));
  TEST_ASSERT_TRUE(memorySet(&store, 49, &b));
  TEST_ASSERT_TRUE(memorySet(&store, 98, &c));

  char text[4096];
  size_t at = 0;
  at += (size_t)snprintf(text + at, sizeof(text) - at, "%s", memoryCsvHeader());
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    at += memoryCsvLine(&store, i, text + at, sizeof(text) - at);
  }

  MemoryStore back;
  memoryInit(&back);
  TEST_ASSERT_TRUE(
      memoryImportCsv(&back, MEMORY_IMPORT_REPLACE, text, at, &result));
  TEST_ASSERT_EQUAL_UINT16(3, result.lines);
  TEST_ASSERT_EQUAL_UINT16(3, result.imported);
  TEST_ASSERT_EQUAL_UINT16(0, result.skipped);
  TEST_ASSERT_EQUAL_MEMORY(&store, &back, sizeof(store));
}

static void merge_fills_the_empty_slots_only(void) {
  MemoryChannel mine = channel(BAND_FM, 100000, "Mine");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &mine));

  TEST_ASSERT_TRUE(
      import("1,FM,92700,0,Theirs\n2,MW,1071,0,New\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(2, result.lines);
  TEST_ASSERT_EQUAL_UINT16(1, result.imported);
  TEST_ASSERT_EQUAL_UINT16(1, result.kept);
  TEST_ASSERT_EQUAL_STRING("Mine", memoryGet(&store, 0)->name);
  TEST_ASSERT_EQUAL_STRING("New", memoryGet(&store, 1)->name);
}

static void replace_throws_the_list_away_first(void) {
  MemoryChannel mine = channel(BAND_FM, 100000, "Mine");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &mine));
  TEST_ASSERT_TRUE(memorySet(&store, 40, &mine));

  TEST_ASSERT_TRUE(import("1,FM,92700,0,Theirs\n", MEMORY_IMPORT_REPLACE));
  TEST_ASSERT_EQUAL_INT(1, memoryCount(&store));
  TEST_ASSERT_EQUAL_STRING("Theirs", memoryGet(&store, 0)->name);
  TEST_ASSERT_FALSE(memorySlotUsed(&store, 40));
}

static void replace_changes_nothing_when_a_line_cannot_be_read(void) {
  MemoryChannel mine = channel(BAND_FM, 100000, "Mine");
  TEST_ASSERT_TRUE(memorySet(&store, 0, &mine));

  TEST_ASSERT_FALSE(import("2,MW,1071,0,Good\nrubbish\n3,SW,9420,0,Also\n",
                           MEMORY_IMPORT_REPLACE));
  TEST_ASSERT_EQUAL_UINT16(2, result.firstBadLine);
  /* Nothing was written, so nothing may be reported as imported. Counts from
   * the lines read before the bad one would say channels landed when none
   * did. */
  TEST_ASSERT_EQUAL_UINT16(0, result.imported);
  TEST_ASSERT_EQUAL_UINT16(0, result.lines);
  TEST_ASSERT_EQUAL_UINT16(0, result.skipped);
  TEST_ASSERT_EQUAL_INT(1, memoryCount(&store));
  TEST_ASSERT_EQUAL_STRING("Mine", memoryGet(&store, 0)->name);
}

static void merge_keeps_going_past_a_line_it_cannot_read(void) {
  TEST_ASSERT_TRUE(import("1,FM,92700,0,Good\nrubbish\n3,SW,9420,0,Also\n",
                          MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(3, result.lines);
  TEST_ASSERT_EQUAL_UINT16(2, result.imported);
  TEST_ASSERT_EQUAL_UINT16(1, result.skipped);
  TEST_ASSERT_EQUAL_UINT16(2, result.firstBadLine);
  TEST_ASSERT_EQUAL_INT(2, memoryCount(&store));
}

static void blank_lines_and_comments_are_not_data(void) {
  TEST_ASSERT_TRUE(
      import("\n# a note\n\n1,FM,92700,0,One\n\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.lines);
  TEST_ASSERT_EQUAL_UINT16(1, result.imported);
  TEST_ASSERT_EQUAL_UINT16(0, result.skipped);
}

static void the_header_is_skipped_whatever_its_case(void) {
  TEST_ASSERT_TRUE(
      import("SLOT,BAND,FREQUENCY,BANDWIDTH,NAME\n"
             "1,FM,92700,0,One\n",
             MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.lines);
  TEST_ASSERT_EQUAL_UINT16(0, result.skipped);
}

static void a_line_that_is_neither_header_nor_channel_is_an_error(void) {
  TEST_ASSERT_TRUE(import("name,frequency\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.lines);
  TEST_ASSERT_EQUAL_UINT16(1, result.skipped);
}

static void carriage_returns_are_taken_off(void) {
  TEST_ASSERT_TRUE(
      import("slot,band,frequency,bandwidth,name\r\n"
             "1,FM,92700,0,One\r\n",
             MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.imported);
  TEST_ASSERT_EQUAL_STRING("One", memoryGet(&store, 0)->name);
}

static void a_file_with_no_last_newline_still_reads(void) {
  TEST_ASSERT_TRUE(import("1,FM,92700,0,One", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.imported);
  TEST_ASSERT_EQUAL_STRING("One", memoryGet(&store, 0)->name);
}

static void a_band_name_is_read_in_any_case(void) {
  TEST_ASSERT_TRUE(import("1,fm,92700,0,a\n2,Mw,1071,0,b\n3,oIrT,70000,0,c\n",
                          MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(3, result.imported);
  TEST_ASSERT_EQUAL_UINT8(BAND_FM, memoryGet(&store, 0)->band);
  TEST_ASSERT_EQUAL_UINT8(BAND_MW, memoryGet(&store, 1)->band);
  TEST_ASSERT_EQUAL_UINT8(BAND_OIRT, memoryGet(&store, 2)->band);
}

static void a_band_name_this_radio_does_not_have_is_refused(void) {
  TEST_ASSERT_TRUE(import("1,VHF,92700,0,a\n1,F,92700,0,b\n1,FMM,92700,0,c\n",
                          MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(3, result.skipped);
  TEST_ASSERT_EQUAL_INT(0, memoryCount(&store));
}

static void the_slot_number_on_the_boundary_and_either_side(void) {
  TEST_ASSERT_TRUE(import("0,FM,92700,0,a\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.skipped);

  memoryInit(&store);
  TEST_ASSERT_TRUE(
      import("1,FM,92700,0,a\n99,FM,92800,0,b\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(2, result.imported);
  TEST_ASSERT_TRUE(memorySlotUsed(&store, 0));
  TEST_ASSERT_TRUE(memorySlotUsed(&store, MEMORY_SLOT_COUNT - 1));

  memoryInit(&store);
  TEST_ASSERT_TRUE(import("100,FM,92700,0,a\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.skipped);
}

static void a_number_that_is_not_one_is_refused(void) {
  TEST_ASSERT_TRUE(
      import("1a,FM,92700,0,a\n"
             "1,FM,92.7,0,b\n"
             "1,FM,,0,c\n"
             "1,FM,-92700,0,d\n"
             "1,FM,0,0,e\n",
             MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(5, result.lines);
  TEST_ASSERT_EQUAL_UINT16(5, result.skipped);
}

static void a_number_too_big_to_hold_is_refused_not_wrapped(void) {
  TEST_ASSERT_TRUE(
      import("1,FM,4294967296,0,a\n"
             "1,FM,99999999999999999999,0,b\n"
             "1,MW,1071,70000,c\n",
             MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(3, result.skipped);
}

static void a_quote_that_never_closes_is_refused(void) {
  TEST_ASSERT_TRUE(import("1,FM,92700,0,\"never ends\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.skipped);
}

static void anything_after_a_closing_quote_is_refused(void) {
  TEST_ASSERT_TRUE(import("1,\"FM\"x,92700,0,a\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.skipped);
}

static void a_sixth_column_is_refused(void) {
  TEST_ASSERT_TRUE(import("1,FM,92700,0,name,extra\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.skipped);
}

static void too_few_columns_are_refused(void) {
  TEST_ASSERT_TRUE(import("1,FM,92700\n1,FM\n1\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(3, result.skipped);
}

static void a_name_longer_than_the_store_is_cut_and_counted(void) {
  TEST_ASSERT_TRUE(import("1,FM,92700,0,A name far too long to keep\n",
                          MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.imported);
  TEST_ASSERT_EQUAL_UINT16(1, result.truncated);
  TEST_ASSERT_EQUAL_STRING("A name far too l", memoryGet(&store, 0)->name);
}

static void a_quoted_name_too_long_is_cut_and_counted(void) {
  TEST_ASSERT_TRUE(import("1,FM,92700,0,\"A name, far too long to keep\"\n",
                          MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.imported);
  TEST_ASSERT_EQUAL_UINT16(1, result.truncated);
  TEST_ASSERT_EQUAL_STRING("A name, far too ", memoryGet(&store, 0)->name);
}

static void a_line_as_long_as_the_header_but_not_it_is_an_error(void) {
  /* The same length as the header, so the only thing that can tell them apart
   * is comparing the characters. */
  TEST_ASSERT_TRUE(
      import("slot,band,frequency,bandwidth,namz\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(1, result.lines);
  TEST_ASSERT_EQUAL_UINT16(1, result.skipped);
}

static void a_width_the_band_does_not_offer_is_refused(void) {
  TEST_ASSERT_TRUE(
      import("1,MW,1071,5,a\n2,FM,92700,6,b\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_UINT16(2, result.skipped);
}

static void the_arguments_are_checked(void) {
  TEST_ASSERT_FALSE(memoryImportCsv(NULL, MEMORY_IMPORT_MERGE, "x", 1, NULL));
  TEST_ASSERT_FALSE(
      memoryImportCsv(&store, MEMORY_IMPORT_MERGE, NULL, 1, &result));
  TEST_ASSERT_FALSE(
      memoryImportCsv(&store, (MemoryImportMode)7, "x", 1, &result));
  TEST_ASSERT_TRUE(memoryImportCsv(&store, MEMORY_IMPORT_MERGE, "", 0, NULL));
}

static void two_lines_naming_one_slot(void) {
  TEST_ASSERT_TRUE(
      import("1,FM,92700,0,First\n1,FM,92800,0,Second\n", MEMORY_IMPORT_MERGE));
  TEST_ASSERT_EQUAL_STRING("First", memoryGet(&store, 0)->name);
  TEST_ASSERT_EQUAL_UINT16(1, result.kept);

  memoryInit(&store);
  TEST_ASSERT_TRUE(import("1,FM,92700,0,First\n1,FM,92800,0,Second\n",
                          MEMORY_IMPORT_REPLACE));
  TEST_ASSERT_EQUAL_STRING("Second", memoryGet(&store, 0)->name);
}

static void a_full_file_of_ninety_nine_channels_lands(void) {
  char text[8192];
  size_t at = 0;
  for (int i = 1; i <= MEMORY_SLOT_COUNT; i++) {
    at += (size_t)snprintf(text + at, sizeof(text) - at,
                           "%d,FM,%d,0,Station %d\n", i, 87500 + i * 100, i);
  }
  TEST_ASSERT_TRUE(
      memoryImportCsv(&store, MEMORY_IMPORT_REPLACE, text, at, &result));
  TEST_ASSERT_EQUAL_UINT16(MEMORY_SLOT_COUNT, result.imported);
  TEST_ASSERT_EQUAL_INT(MEMORY_SLOT_COUNT, memoryCount(&store));
}

int main(int, char **) {
  UNITY_BEGIN();

  RUN_TEST(the_header_names_every_column);
  RUN_TEST(a_channel_writes_one_line);
  RUN_TEST(the_slot_is_counted_from_one);
  RUN_TEST(an_empty_slot_writes_nothing);
  RUN_TEST(a_name_with_a_comma_is_wrapped_in_quotes);
  RUN_TEST(a_quote_inside_a_name_is_doubled);
  RUN_TEST(the_longest_line_fits_the_buffer);
  RUN_TEST(a_buffer_too_small_says_so_and_still_terminates);
  RUN_TEST(a_written_list_reads_back_the_same);
  RUN_TEST(merge_fills_the_empty_slots_only);
  RUN_TEST(replace_throws_the_list_away_first);
  RUN_TEST(replace_changes_nothing_when_a_line_cannot_be_read);
  RUN_TEST(merge_keeps_going_past_a_line_it_cannot_read);
  RUN_TEST(blank_lines_and_comments_are_not_data);
  RUN_TEST(the_header_is_skipped_whatever_its_case);
  RUN_TEST(a_line_that_is_neither_header_nor_channel_is_an_error);
  RUN_TEST(carriage_returns_are_taken_off);
  RUN_TEST(a_file_with_no_last_newline_still_reads);
  RUN_TEST(a_band_name_is_read_in_any_case);
  RUN_TEST(a_band_name_this_radio_does_not_have_is_refused);
  RUN_TEST(the_slot_number_on_the_boundary_and_either_side);
  RUN_TEST(a_number_that_is_not_one_is_refused);
  RUN_TEST(a_number_too_big_to_hold_is_refused_not_wrapped);
  RUN_TEST(a_quote_that_never_closes_is_refused);
  RUN_TEST(anything_after_a_closing_quote_is_refused);
  RUN_TEST(a_sixth_column_is_refused);
  RUN_TEST(too_few_columns_are_refused);
  RUN_TEST(a_name_longer_than_the_store_is_cut_and_counted);
  RUN_TEST(a_quoted_name_too_long_is_cut_and_counted);
  RUN_TEST(a_line_as_long_as_the_header_but_not_it_is_an_error);
  RUN_TEST(a_width_the_band_does_not_offer_is_refused);
  RUN_TEST(the_arguments_are_checked);
  RUN_TEST(two_lines_naming_one_slot);
  RUN_TEST(a_full_file_of_ninety_nine_channels_lands);

  return UNITY_END();
}
