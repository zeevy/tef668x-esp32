// The lines of a saved station's page, for a log entry and a preset.
#include <stdio.h>
#include <string.h>
#include <unity.h>

#include "core/clock.h"
#include "core/station_page.h"

void setUp(void) {}
void tearDown(void) {}

static const uint32_t kNow = 1791619436;

// An FM entry with everything a log keeps.
static LogbookEntry fmEntry(void) {
  LogbookEntry e;
  memset(&e, 0, sizeof(e));
  e.timeKnown = true;
  e.timeValue = kNow - 3600;
  e.band = BAND_FM;
  e.freqKHz = 106400;
  e.levelDbuVTenths = 452;
  e.usnTenths = 124;
  e.multipathTenths = 26;
  e.snrDb = 28;
  e.stereo = true;
  e.bandwidthKHz = 236;
  e.hasName = true;
  strcpy(e.name, " MAGIC  ");
  e.hasPi = true;
  e.pi = 0x5123;
  e.hasRt = true;
  strcpy(e.rt, "MAGICFM VINTOO MAIMARACHIPODAAM!   ");
  strcpy(e.note, "Clear on the balcony");
  e.hasPty = true;
  e.pty = 10;
  e.hasEcc = true;
  e.ecc = 0xF2;
  return e;
}

static void the_log_entry_page_has_the_station_its_readings_and_texts(void) {
  const LogbookEntry e = fmEntry();
  StationPage p;
  TEST_ASSERT_TRUE(stationPageLog(&e, true, kNow, 330, RDS_REGION_EUROPE, &p));
  TEST_ASSERT_EQUAL_STRING("106.40 MHz  MAGIC", p.title);
  char when[CLOCK_WHEN_LEN];
  TEST_ASSERT_TRUE(clockFormatWhen(e.timeValue, kNow, 330, CLOCK_WHEN_ROW, when,
                                   sizeof(when)));
  char sub[96];
  snprintf(sub, sizeof(sub), "%s \xC2\xB7 PI 5123 \xC2\xB7 IN \xC2\xB7 %s",
           when, rdsPtyNameIn(10, RDS_REGION_EUROPE));
  TEST_ASSERT_EQUAL_STRING(sub, p.sub);
  const char *labels[] = {"Level",     "SNR",    "Noise",
                          "Multipath", "Stereo", "Width"};
  const char *values[] = {"45.2 dB\xC2\xB5V", "28 dB", "12 %", "3 %", "Yes",
                          "236 kHz"};
  for (int i = 0; i < STATION_PAGE_TILES; i++) {
    TEST_ASSERT_EQUAL_STRING(labels[i], p.label[i]);
    TEST_ASSERT_EQUAL_STRING(values[i], p.value[i]);
  }
  TEST_ASSERT_EQUAL_STRING("MAGICFM VINTOO MAIMARACHIPODAAM!", p.text);
  TEST_ASSERT_EQUAL_STRING("Clear on the balcony", p.note);
}

// What is not kept is left out, never written as a guess: no country
// without the ECC, no type without one, no radio text, no name.
static void what_an_entry_lacks_is_left_out(void) {
  LogbookEntry e = fmEntry();
  e.hasEcc = false;
  e.hasPty = false;
  e.hasRt = false;
  e.hasName = false;
  e.note[0] = '\0';
  StationPage p;
  TEST_ASSERT_TRUE(stationPageLog(&e, true, kNow, 330, RDS_REGION_EUROPE, &p));
  TEST_ASSERT_EQUAL_STRING("106.40 MHz", p.title);
  TEST_ASSERT_NOT_NULL(strstr(p.sub, "PI 5123"));
  TEST_ASSERT_NULL(strstr(p.sub, "IN"));
  TEST_ASSERT_EQUAL_STRING("", p.text);
  TEST_ASSERT_EQUAL_STRING("", p.note);
  // A PTY of 0 is none.
  e.hasPty = true;
  e.pty = 0;
  TEST_ASSERT_TRUE(stationPageLog(&e, true, kNow, 330, RDS_REGION_EUROPE, &p));
  TEST_ASSERT_NULL(strstr(p.sub, rdsPtyNameIn(0, RDS_REGION_EUROPE)));
}

// On AM the co-channel reading stands in for the FM ones, and a level
// below zero keeps its sign.
static void an_am_entry_has_the_co_channel_reading(void) {
  LogbookEntry e = fmEntry();
  e.band = BAND_SW;
  e.freqKHz = 13845;
  e.levelDbuVTenths = -5;
  e.coChannelTenths = 74;
  e.bandwidthKHz = 4;
  StationPage p;
  TEST_ASSERT_TRUE(stationPageLog(&e, true, kNow, 330, RDS_REGION_EUROPE, &p));
  TEST_ASSERT_EQUAL_STRING("13845 kHz  MAGIC", p.title);
  const char *labels[] = {"Level", "SNR", "Co-channel", "Width"};
  const char *values[] = {"-0.5 dB\xC2\xB5V", "28 dB", "7 %", "4 kHz"};
  for (int i = 0; i < 4; i++) {
    TEST_ASSERT_EQUAL_STRING(labels[i], p.label[i]);
    TEST_ASSERT_EQUAL_STRING(values[i], p.value[i]);
  }
  TEST_ASSERT_NULL(p.label[4]);
}

// The entry's time: unknown when made before the clock was set, its day and
// time while the radio's clock is not set now.
static void the_time_says_when_it_is_not_known(void) {
  LogbookEntry e = fmEntry();
  e.timeKnown = false;
  e.timeValue = 12345;
  StationPage p;
  TEST_ASSERT_TRUE(stationPageLog(&e, true, kNow, 330, RDS_REGION_EUROPE, &p));
  TEST_ASSERT_EQUAL_STRING_LEN("Time not known", p.sub, 14);
  e = fmEntry();
  TEST_ASSERT_TRUE(stationPageLog(&e, false, 0, 330, RDS_REGION_EUROPE, &p));
  char when[CLOCK_WHEN_LEN];
  TEST_ASSERT_TRUE(
      clockFormatWhen(e.timeValue, 0, 330, CLOCK_WHEN_ROW, when, sizeof(when)));
  TEST_ASSERT_NULL(strstr(when, "Today"));
  TEST_ASSERT_EQUAL_STRING_LEN(when, p.sub, strlen(when));
}

static void a_preset_page_has_its_slot_band_width_and_pi(void) {
  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  c.freqKHz = 93500;
  c.band = BAND_FM;
  strcpy(c.name, "RED FM ");
  c.pi = 0x0935;
  StationPage p;
  TEST_ASSERT_TRUE(
      stationPagePreset(&c, 2, true, kNow, 330, RDS_REGION_EUROPE, &p));
  TEST_ASSERT_EQUAL_STRING("93.50 MHz  RED FM", p.title);
  TEST_ASSERT_EQUAL_STRING("P02 \xC2\xB7 PI 0935", p.sub);
  TEST_ASSERT_EQUAL_STRING("Band", p.label[0]);
  TEST_ASSERT_EQUAL_STRING("FM", p.value[0]);
  TEST_ASSERT_EQUAL_STRING("Width", p.label[1]);
  TEST_ASSERT_EQUAL_STRING("AUTO", p.value[1]);
  TEST_ASSERT_NULL(p.label[2]);
  // No time kept: no line for it.
  TEST_ASSERT_EQUAL_STRING("", p.text);
  // A width of its own, and no PI learnt yet.
  c.bandwidthKHz = 114;
  c.pi = 0;
  TEST_ASSERT_TRUE(
      stationPagePreset(&c, 2, true, kNow, 330, RDS_REGION_EUROPE, &p));
  TEST_ASSERT_EQUAL_STRING("114 kHz", p.value[1]);
  TEST_ASSERT_EQUAL_STRING("P02", p.sub);
}

// The programme type and the country it learnt or was saved with, and when
// it was saved; a country only from the PI and the ECC together.
static void a_preset_page_has_its_type_country_and_time(void) {
  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  c.freqKHz = 93500;
  c.band = BAND_FM;
  c.pi = 0x5123;
  c.pty = 10;
  c.ecc = 0xF2;
  c.savedUtc = kNow - 3600;
  StationPage p;
  TEST_ASSERT_TRUE(
      stationPagePreset(&c, 3, true, kNow, 330, RDS_REGION_EUROPE, &p));
  char sub[96];
  snprintf(sub, sizeof(sub), "P03 \xC2\xB7 PI 5123 \xC2\xB7 IN \xC2\xB7 %s",
           rdsPtyNameIn(10, RDS_REGION_EUROPE));
  TEST_ASSERT_EQUAL_STRING(sub, p.sub);
  char when[CLOCK_WHEN_LEN];
  TEST_ASSERT_TRUE(clockFormatWhen(c.savedUtc, kNow, 330, CLOCK_WHEN_ROW, when,
                                   sizeof(when)));
  char saved[40];
  snprintf(saved, sizeof(saved), "Saved %s", when);
  TEST_ASSERT_EQUAL_STRING(saved, p.text);
  c.pi = 0;
  TEST_ASSERT_TRUE(
      stationPagePreset(&c, 3, true, kNow, 330, RDS_REGION_EUROPE, &p));
  TEST_ASSERT_NULL(strstr(p.sub, "IN"));
}

static void nothing_is_written_for_an_empty_slot_or_no_pointer(void) {
  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  StationPage p;
  TEST_ASSERT_FALSE(
      stationPagePreset(&c, 1, true, kNow, 0, RDS_REGION_EUROPE, &p));
  TEST_ASSERT_FALSE(
      stationPagePreset(NULL, 1, true, kNow, 0, RDS_REGION_EUROPE, &p));
  TEST_ASSERT_FALSE(stationPageLog(NULL, true, kNow, 0, RDS_REGION_EUROPE, &p));
  const LogbookEntry e = fmEntry();
  TEST_ASSERT_FALSE(stationPageLog(&e, true, kNow, 0, RDS_REGION_EUROPE, NULL));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(the_log_entry_page_has_the_station_its_readings_and_texts);
  RUN_TEST(what_an_entry_lacks_is_left_out);
  RUN_TEST(an_am_entry_has_the_co_channel_reading);
  RUN_TEST(the_time_says_when_it_is_not_known);
  RUN_TEST(a_preset_page_has_its_slot_band_width_and_pi);
  RUN_TEST(a_preset_page_has_its_type_country_and_time);
  RUN_TEST(nothing_is_written_for_an_empty_slot_or_no_pointer);
  return UNITY_END();
}
