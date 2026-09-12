/**
 * @file test_settings.cpp
 * @brief Tests for the settings struct, its defaults and its migration.
 *        Runs on a PC.
 */
#include <unity.h>

#include "core/settings.h"

#include <stddef.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void defaults_are_valid_and_have_no_wifi(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));
  TEST_ASSERT_FALSE(settingsHasWifi(&s));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, s.version);
  TEST_ASSERT_EQUAL_UINT16(sizeof(Settings), s.size);
  TEST_ASSERT_EQUAL_UINT32(0, s.accessPin);
}

static void settings_survive_a_round_trip_through_bytes(void) {
  Settings written;
  settingsDefaults(&written);
  TEST_ASSERT_TRUE(settingsSetWifi(&written, "MyNetwork", "hunter2hunter2"));
  written.accessPin = 123456;

  Settings read;
  TEST_ASSERT_TRUE(settingsFromBlob(&written, sizeof(written), &read));
  TEST_ASSERT_EQUAL_STRING("MyNetwork", read.wifiSsid);
  TEST_ASSERT_EQUAL_STRING("hunter2hunter2", read.wifiPass);
  TEST_ASSERT_EQUAL_UINT32(123456, read.accessPin);
  TEST_ASSERT_TRUE(settingsHasWifi(&read));
}

static void a_blob_from_a_newer_firmware_falls_back_to_defaults(void) {
  Settings written;
  settingsDefaults(&written);
  settingsSetWifi(&written, "MyNetwork", "hunter2hunter2");
  written.version = SETTINGS_VERSION + 1;

  Settings read;
  TEST_ASSERT_FALSE(settingsFromBlob(&written, sizeof(written), &read));
  TEST_ASSERT_FALSE(settingsHasWifi(&read));
  TEST_ASSERT_TRUE(settingsValid(&read));
}

static void a_blob_with_version_zero_falls_back_to_defaults(void) {
  Settings written;
  settingsDefaults(&written);
  written.version = 0;

  Settings read;
  TEST_ASSERT_FALSE(settingsFromBlob(&written, sizeof(written), &read));
  TEST_ASSERT_TRUE(settingsValid(&read));
}

static void a_version_one_blob_has_to_be_exactly_the_version_one_size(void) {
  /* Version 1 is the only version there is, so its length is known exactly.
   * A blob that claims version 1 at any other length is corrupt, not old.
   * When version 2 arrives this test gains a sibling that feeds it a real
   * version 1 blob and checks the new fields come back as defaults. */
  struct ShortSettings {
    uint16_t version;
    uint16_t size;
    char wifiSsid[SETTINGS_SSID_LEN];
  } shorter;
  memset(&shorter, 0, sizeof(shorter));
  shorter.version = 1;
  shorter.size = (uint16_t)sizeof(shorter);
  memcpy(shorter.wifiSsid, "OldNetwork", strlen("OldNetwork") + 1);

  Settings read;
  TEST_ASSERT_FALSE(settingsFromBlob(&shorter, sizeof(shorter), &read));
  TEST_ASSERT_TRUE(settingsValid(&read));
  TEST_ASSERT_FALSE(settingsHasWifi(&read));
}

static void a_truncated_blob_is_rejected_rather_than_half_read(void) {
  /* A full struct written, then cut short, which is what a failed NVS write
   * leaves behind. Without the size check the leftover bytes read back as a
   * short SSID and the radio tries to join a network made of rubbish. */
  Settings written;
  settingsDefaults(&written);
  settingsSetWifi(&written, "MyNetwork", "hunter2hunter2");

  Settings read;
  size_t cut = offsetof(Settings, wifiPass);
  TEST_ASSERT_FALSE(settingsFromBlob(&written, cut, &read));
  TEST_ASSERT_TRUE(settingsValid(&read));
  TEST_ASSERT_FALSE(settingsHasWifi(&read));

  /* Five bytes that declare their own length honestly. The size field alone
   * accepts this and reads it back as a valid one character SSID, which is
   * why the length is checked against the version and not just against
   * itself. */
  uint8_t stub[5] = {1, 0, 5, 0, 'X'};
  TEST_ASSERT_FALSE(settingsFromBlob(stub, sizeof(stub), &read));
  TEST_ASSERT_FALSE(settingsHasWifi(&read));
}

static void a_blob_longer_than_the_struct_is_rejected(void) {
  /* Only a newer firmware writes a longer struct, and that is caught by the
   * version check. Anything else this long is corrupt. */
  uint8_t padded[sizeof(Settings) + 16];
  Settings written;
  settingsDefaults(&written);
  memset(padded, 0, sizeof(padded));
  memcpy(padded, &written, sizeof(written));

  Settings read;
  TEST_ASSERT_FALSE(settingsFromBlob(padded, sizeof(padded), &read));
  TEST_ASSERT_TRUE(settingsValid(&read));
}

static void an_empty_blob_falls_back_to_defaults(void) {
  Settings read;
  TEST_ASSERT_FALSE(settingsFromBlob(NULL, 0, &read));
  TEST_ASSERT_TRUE(settingsValid(&read));

  uint8_t nothing[1] = {0};
  TEST_ASSERT_FALSE(settingsFromBlob(nothing, 0, &read));
  TEST_ASSERT_TRUE(settingsValid(&read));
  TEST_ASSERT_FALSE(settingsFromBlob(nothing, 1, &read));
  TEST_ASSERT_TRUE(settingsValid(&read));
}

static void settings_valid_rejects_a_version_it_does_not_know(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.version = 0;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.version = SETTINGS_VERSION + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void an_unterminated_passphrase_is_rejected(void) {
  /* The SSID and the passphrase are checked separately, so both need a test
   * or half the check can rot unnoticed. */
  Settings s;
  settingsDefaults(&s);
  memset(s.wifiPass, 'P', SETTINGS_PASS_LEN);
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void setting_wifi_with_no_ssid_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  settingsSetWifi(&s, "Keep", "ThisOne");
  TEST_ASSERT_FALSE(settingsSetWifi(&s, NULL, "anything"));
  TEST_ASSERT_EQUAL_STRING("Keep", s.wifiSsid);
  TEST_ASSERT_EQUAL_STRING("ThisOne", s.wifiPass);
}

static void an_unterminated_string_is_rejected(void) {
  Settings written;
  settingsDefaults(&written);
  memset(written.wifiSsid, 'A', SETTINGS_SSID_LEN);

  Settings read;
  TEST_ASSERT_FALSE(settingsFromBlob(&written, sizeof(written), &read));
  TEST_ASSERT_TRUE(settingsValid(&read));
  TEST_ASSERT_FALSE(settingsHasWifi(&read));
}

static void a_pin_outside_six_digits_is_rejected(void) {
  Settings s;
  settingsDefaults(&s);
  s.accessPin = 999999;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.accessPin = 1000000;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void the_longest_allowed_ssid_and_passphrase_fit(void) {
  Settings s;
  settingsDefaults(&s);
  char ssid[SETTINGS_SSID_LEN];
  char pass[SETTINGS_PASS_LEN];
  memset(ssid, 'S', sizeof(ssid) - 1);
  ssid[sizeof(ssid) - 1] = '\0';
  memset(pass, 'P', sizeof(pass) - 1);
  pass[sizeof(pass) - 1] = '\0';
  TEST_ASSERT_TRUE(settingsSetWifi(&s, ssid, pass));
  TEST_ASSERT_TRUE(settingsValid(&s));
  TEST_ASSERT_EQUAL_STRING(ssid, s.wifiSsid);
  TEST_ASSERT_EQUAL_STRING(pass, s.wifiPass);
}

static void one_character_too_many_is_refused_and_changes_nothing(void) {
  Settings s;
  settingsDefaults(&s);
  settingsSetWifi(&s, "Keep", "ThisOne");

  char ssid[SETTINGS_SSID_LEN + 1];
  memset(ssid, 'S', sizeof(ssid) - 1);
  ssid[sizeof(ssid) - 1] = '\0';
  TEST_ASSERT_FALSE(settingsSetWifi(&s, ssid, "short"));
  TEST_ASSERT_EQUAL_STRING("Keep", s.wifiSsid);

  char pass[SETTINGS_PASS_LEN + 1];
  memset(pass, 'P', sizeof(pass) - 1);
  pass[sizeof(pass) - 1] = '\0';
  TEST_ASSERT_FALSE(settingsSetWifi(&s, "short", pass));
  TEST_ASSERT_EQUAL_STRING("Keep", s.wifiSsid);
  TEST_ASSERT_EQUAL_STRING("ThisOne", s.wifiPass);
}

static void an_open_network_with_no_passphrase_is_allowed(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsSetWifi(&s, "OpenNetwork", ""));
  TEST_ASSERT_TRUE(settingsSetWifi(&s, "OpenNetwork", NULL));
  TEST_ASSERT_TRUE(settingsHasWifi(&s));
  TEST_ASSERT_EQUAL_STRING("", s.wifiPass);
}

static void setting_wifi_clears_the_old_value_completely(void) {
  Settings s;
  settingsDefaults(&s);
  settingsSetWifi(&s, "AVeryLongNetworkName", "AVeryLongPassphrase");
  settingsSetWifi(&s, "Short", "Tiny");
  TEST_ASSERT_EQUAL_STRING("Short", s.wifiSsid);
  TEST_ASSERT_EQUAL_STRING("Tiny", s.wifiPass);
  /* Nothing of the old value is left behind past the terminator. */
  for (size_t i = strlen("Short"); i < SETTINGS_SSID_LEN; i++) {
    TEST_ASSERT_EQUAL_CHAR('\0', s.wifiSsid[i]);
  }
  for (size_t i = strlen("Tiny"); i < SETTINGS_PASS_LEN; i++) {
    TEST_ASSERT_EQUAL_CHAR('\0', s.wifiPass[i]);
  }
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(defaults_are_valid_and_have_no_wifi);
  RUN_TEST(settings_survive_a_round_trip_through_bytes);
  RUN_TEST(a_blob_from_a_newer_firmware_falls_back_to_defaults);
  RUN_TEST(a_blob_with_version_zero_falls_back_to_defaults);
  RUN_TEST(a_version_one_blob_has_to_be_exactly_the_version_one_size);
  RUN_TEST(a_truncated_blob_is_rejected_rather_than_half_read);
  RUN_TEST(a_blob_longer_than_the_struct_is_rejected);
  RUN_TEST(an_empty_blob_falls_back_to_defaults);
  RUN_TEST(settings_valid_rejects_a_version_it_does_not_know);
  RUN_TEST(an_unterminated_passphrase_is_rejected);
  RUN_TEST(setting_wifi_with_no_ssid_is_refused);
  RUN_TEST(an_unterminated_string_is_rejected);
  RUN_TEST(a_pin_outside_six_digits_is_rejected);
  RUN_TEST(the_longest_allowed_ssid_and_passphrase_fit);
  RUN_TEST(one_character_too_many_is_refused_and_changes_nothing);
  RUN_TEST(an_open_network_with_no_passphrase_is_allowed);
  RUN_TEST(setting_wifi_clears_the_old_value_completely);
  return UNITY_END();
}
