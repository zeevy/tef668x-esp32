/*
 * Tests for the settings struct, its defaults and its migration.
 * Runs on a PC.
 */
#include <unity.h>

#include "core/backlight.h"
#include "core/band_plan.h"
#include "core/input.h"
#include "core/radio.h"
#include "core/seek.h"
#include "core/settings.h"
#include "core/squelch.h"

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

/* ------------------------------------------- reading an older radio's blob */

/* The version 1 struct, exactly as a radio in the field wrote it. */
#define V1_SIZE 108

/*
 * Build a version 1 blob, the way the old firmware laid it out.
 *
 * Written through offsetof rather than by counting bytes. Version 1's fields
 * are at the same offsets in version 2, because the struct is append only,
 * and saying it this way makes the test fail loudly if that ever stops being
 * true rather than quietly reading the wrong field.
 */
static size_t makeV1(uint8_t *blob, const char *ssid, const char *pass,
                     uint32_t pin) {
  memset(blob, 0, V1_SIZE);
  uint16_t version = 1;
  uint16_t size = V1_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  memcpy(blob + offsetof(Settings, wifiSsid), ssid, strlen(ssid));
  memcpy(blob + offsetof(Settings, wifiPass), pass, strlen(pass));
  memcpy(blob + offsetof(Settings, accessPin), &pin, sizeof(pin));
  return V1_SIZE;
}

static void the_version_1_fields_never_moved(void) {
  /* The invariant the whole migration rests on. If any of these shifts, every
   * radio already in the field reads its own settings from the wrong place. */
  TEST_ASSERT_EQUAL_size_t(0, offsetof(Settings, version));
  TEST_ASSERT_EQUAL_size_t(2, offsetof(Settings, size));
  TEST_ASSERT_EQUAL_size_t(4, offsetof(Settings, wifiSsid));
  TEST_ASSERT_EQUAL_size_t(37, offsetof(Settings, wifiPass));
  TEST_ASSERT_EQUAL_size_t(104, offsetof(Settings, accessPin));
  /* And version 1 ended where the size table says it did. */
  TEST_ASSERT_EQUAL_size_t(V1_SIZE,
                           offsetof(Settings, accessPin) + sizeof(uint32_t));
}

static void a_version_1_blob_still_reads(void) {
  /* The whole point of the version and size pair. A radio that has been
   * running the old firmware must come up with its network and its PIN
   * intact, not reset to the factory. */
  uint8_t blob[V1_SIZE];
  size_t len = makeV1(blob, "TARANG", "hunter2", 123456);

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, len, &out));
  TEST_ASSERT_EQUAL_STRING("TARANG", out.wifiSsid);
  TEST_ASSERT_EQUAL_STRING("hunter2", out.wifiPass);
  TEST_ASSERT_EQUAL_UINT32(123456, out.accessPin);
}

static void a_version_1_blob_gets_the_defaults_for_what_it_never_had(void) {
  uint8_t blob[V1_SIZE];
  size_t len = makeV1(blob, "TARANG", "hunter2", 123456);

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, len, &out));

  Settings fresh;
  settingsDefaults(&fresh);
  TEST_ASSERT_EQUAL_UINT8(fresh.fmRegion, out.fmRegion);
  TEST_ASSERT_EQUAL_UINT8(fresh.mwSpacing, out.mwSpacing);
  TEST_ASSERT_EQUAL_UINT8(fresh.squelchMode, out.squelchMode);
  TEST_ASSERT_EQUAL_UINT8(fresh.amBandwidthKHz, out.amBandwidthKHz);
  /* Including where it comes up. A radio updated from version 1 has never
   * saved a station, so it has to keep coming up where it used to. */
  TEST_ASSERT_EQUAL_UINT8((uint8_t)BAND_FM, out.startBand);
  TEST_ASSERT_EQUAL_UINT32(104000, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT16(fresh.fmDeemphasisUs, out.fmDeemphasisUs);
  /* And it is stamped as the current version, so it is written back whole. */
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
  TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(Settings), out.size);
}

/* How many bytes version 2 wrote. Fixed for good, like V1_SIZE. */
#define V2_SIZE 132

/* And version 3. */
#define V3_SIZE 136

/* And version 4. */
#define V4_SIZE 140

/* And version 5. */
#define V5_SIZE 144

/* And version 6, which is the same length as version 5. */
#define V6_SIZE 144

/* And version 7. */
#define V7_SIZE 148

/* And version 8, which is the same length as versions 9 and 10. */
#define V8_SIZE 196

/* And version 9, which version 10 also matches for the same reason. */
#define V9_SIZE 196

/*
 * Build a version 2 blob out of a current one.
 *
 * Version 3 only appended, so the first V2_SIZE bytes of a current struct are
 * exactly what version 2 wrote. Stamping the header and truncating is
 * therefore a real version 2 blob, not an approximation of one.
 */
static size_t makeV2(uint8_t *blob, const Settings *from) {
  memcpy(blob, from, V2_SIZE);
  uint16_t version = 2;
  uint16_t size = V2_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  return V2_SIZE;
}

static void the_version_2_fields_never_moved(void) {
  /* Everything version 2 wrote has to stay where it was, or a radio in the
   * field reads its own settings back as something else. */
  TEST_ASSERT_EQUAL_size_t(104, offsetof(Settings, accessPin));
  TEST_ASSERT_EQUAL_size_t(V2_SIZE, offsetof(Settings, fmScanSensitivity));
  TEST_ASSERT_TRUE(sizeof(Settings) > V2_SIZE);
}

static void a_version_2_blob_gets_the_defaults_for_what_it_never_had(void) {
  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 102800;
  source.fmHighCutStart = 40;
  source.startVolumeDb = -18;

  uint8_t blob[sizeof(Settings)];
  size_t len = makeV2(blob, &source);

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, len, &out));

  /* What version 2 held comes back. */
  TEST_ASSERT_EQUAL_UINT32(102800, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(40, out.fmHighCutStart);
  TEST_ASSERT_EQUAL_INT8(-18, out.startVolumeDb);

  /* What it never had comes back as the default, not as zero. A scan
   * sensitivity of zero is outside the range and the radio would refuse the
   * whole blob at its next start. */
  Settings fresh;
  settingsDefaults(&fresh);
  TEST_ASSERT_EQUAL_UINT8(fresh.fmScanSensitivity, out.fmScanSensitivity);
  TEST_ASSERT_EQUAL_UINT8(fresh.amScanSensitivity, out.amScanSensitivity);
  TEST_ASSERT_TRUE(settingsValid(&out));

  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
  TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(Settings), out.size);
}

static void a_version_2_blob_of_the_wrong_length_is_refused(void) {
  Settings source;
  settingsDefaults(&source);
  uint8_t blob[sizeof(Settings)];
  size_t len = makeV2(blob, &source);
  Settings out;
  TEST_ASSERT_FALSE(settingsFromBlob(blob, len - 1, &out));
  TEST_ASSERT_FALSE(settingsFromBlob(blob, len + 1, &out));
}

static void a_version_3_blob_gets_the_defaults_for_what_it_never_had(void) {
  /* Version 4 only appended, so the first V3_SIZE bytes of a current struct
   * are exactly what version 3 wrote. */
  Settings source;
  settingsDefaults(&source);
  source.fmScanSensitivity = 2;
  source.startFreqKHz = 98300;

  uint8_t blob[sizeof(Settings)];
  memcpy(blob, &source, V3_SIZE);
  uint16_t version = 3;
  uint16_t size = V3_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V3_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(2, out.fmScanSensitivity);
  TEST_ASSERT_EQUAL_UINT32(98300, out.startFreqKHz);
  /* Never calibrated, which is what a radio updated from version 3 is. */
  TEST_ASSERT_EQUAL_UINT16(0, out.potRawMin);
  TEST_ASSERT_EQUAL_UINT16(0, out.potRawMax);
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

static void the_version_3_fields_never_moved(void) {
  TEST_ASSERT_EQUAL_size_t(V2_SIZE, offsetof(Settings, fmScanSensitivity));
  TEST_ASSERT_TRUE(sizeof(Settings) > V3_SIZE);
}

static void a_new_field_inside_old_padding_is_not_read_from_it(void) {
  /* potRawMin sits at offset 134, inside the two bytes version 3 wrote as
   * padding after its last field. A version 3 blob with rubbish in that
   * padding must not have the rubbish read back as a calibration. */
  TEST_ASSERT_EQUAL_size_t(134, offsetof(Settings, potRawMin));
  TEST_ASSERT_TRUE(offsetof(Settings, potRawMin) < V3_SIZE);

  Settings source;
  settingsDefaults(&source);
  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V3_SIZE);
  blob[134] = 0xAB; /* Padding, as far as version 3 was concerned. */
  blob[135] = 0xCD;
  uint16_t version = 3;
  uint16_t size = V3_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V3_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT16(0, out.potRawMin);
  TEST_ASSERT_EQUAL_UINT16(0, out.potRawMax);
}

static void a_version_4_blob_gets_the_defaults_for_what_it_never_had(void) {
  /* softMuteMs sits at 138, inside the two bytes version 4 wrote as padding
   * after its last field, so this checks the same trap version 3 had: a new
   * field must not be read out of an older version's padding. */
  TEST_ASSERT_EQUAL_size_t(138, offsetof(Settings, softMuteMs));
  TEST_ASSERT_TRUE(offsetof(Settings, softMuteMs) < V4_SIZE);

  Settings source;
  settingsDefaults(&source);
  source.potRawMin = 0;
  source.potRawMax = 4095;
  source.startFreqKHz = 104000;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V4_SIZE);
  blob[138] = 0xAB; /* Padding, as far as version 4 was concerned. */
  blob[139] = 0xCD;
  uint16_t version = 4;
  uint16_t size = V4_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V4_SIZE, &out));

  /* What version 4 held comes back. */
  TEST_ASSERT_EQUAL_UINT16(4095, out.potRawMax);
  TEST_ASSERT_EQUAL_UINT32(104000, out.startFreqKHz);

  /* What it never had comes back as the default, not out of the padding. */
  Settings fresh;
  settingsDefaults(&fresh);
  TEST_ASSERT_EQUAL_UINT16(fresh.softMuteMs, out.softMuteMs);
  TEST_ASSERT_EQUAL_UINT8(0, out.beepKey);
  TEST_ASSERT_EQUAL_UINT8(0, out.beepEdge);
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

static void a_version_5_blob_gets_the_defaults_for_what_it_never_had(void) {
  /* Version 6 is the same length as version 5, because beepStart fits in the
   * two bytes version 5 left as padding. So the length says nothing here and
   * the version is the only thing that tells them apart, which is the case
   * worth a test of its own. */
  TEST_ASSERT_EQUAL_size_t(142, offsetof(Settings, beepStart));

  Settings source;
  settingsDefaults(&source);
  source.softMuteMs = 250;
  source.beepKey = (uint8_t)BEEP_KEYS;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V5_SIZE);
  blob[142] = 0xAB; /* Padding, as far as version 5 was concerned. */
  blob[143] = 0xCD;
  uint16_t version = 5;
  uint16_t size = V5_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V5_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT16(250, out.softMuteMs);
  TEST_ASSERT_EQUAL_UINT8((uint8_t)BEEP_KEYS, out.beepKey);

  Settings fresh;
  settingsDefaults(&fresh);
  TEST_ASSERT_EQUAL_UINT8(fresh.beepStart, out.beepStart);
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

static void a_version_6_blob_gets_the_defaults_for_what_it_never_had(void) {
  /* backlightPercent sits at 143, in the one byte version 6 wrote as padding
   * after beepStart. Copying a version 6 blob by its written length would
   * take that field out of the old padding, which is what
   * settingsFieldEndOfVersion exists to stop. */
  TEST_ASSERT_EQUAL_size_t(143, offsetof(Settings, backlightPercent));

  Settings source;
  settingsDefaults(&source);
  source.softMuteMs = 120;
  source.beepStart = 0;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V6_SIZE);
  blob[143] = 0xEE; /* Padding, as far as version 6 was concerned. */
  uint16_t version = 6;
  uint16_t size = V6_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V6_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT16(120, out.softMuteMs);
  TEST_ASSERT_EQUAL_UINT8(0, out.beepStart);

  Settings fresh;
  settingsDefaults(&fresh);
  TEST_ASSERT_EQUAL_UINT8(fresh.backlightPercent, out.backlightPercent);
  TEST_ASSERT_EQUAL_UINT8(fresh.backlightDimPercent, out.backlightDimPercent);
  TEST_ASSERT_EQUAL_UINT8(fresh.backlightDimAfterS, out.backlightDimAfterS);
  TEST_ASSERT_EQUAL_UINT8(fresh.backlightFade, out.backlightFade);
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

static void a_version_7_blob_gets_the_defaults_for_what_it_never_had(void) {
  /* Version 7's fields ran right to the end of its struct with no padding
   * left over, so version 8's first field starts exactly where version 7
   * stopped. That is the invariant worth pinning: it is what says the copy
   * takes all of version 7 and none of version 8. */
  TEST_ASSERT_EQUAL_size_t(V7_SIZE, offsetof(Settings, bandFreqKHz));

  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 98300;
  source.backlightDimAfterS = 30;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V7_SIZE);
  uint16_t version = 7;
  uint16_t size = V7_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V7_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT32(98300, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(30, out.backlightDimAfterS);

  /* Every band comes back unset, which means each takes its own default. A
   * radio updated from version 7 has never had anywhere to store these. */
  for (size_t i = 0; i < BAND_COUNT; i++) {
    TEST_ASSERT_EQUAL_UINT32(0, out.bandFreqKHz[i]);
    TEST_ASSERT_EQUAL_UINT16(0, out.bandBandwidthKHz[i]);
    TEST_ASSERT_EQUAL_UINT16(0, out.bandStepKHz[i]);
    TEST_ASSERT_EQUAL_UINT8(0, out.bandTuneMode[i]);
  }
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

static void a_stored_tuning_mode_outside_the_enum_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.bandTuneMode[BAND_SW] = (uint8_t)TUNE_MODE_COUNT - 1;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.bandTuneMode[BAND_SW] = (uint8_t)TUNE_MODE_COUNT;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_version_8_blob_gets_the_defaults_for_what_it_never_had(void) {
  /* fmSquelchFloor sits at 193, inside the three bytes version 8 wrote as
   * padding after its last per band array. Version 9 is therefore the same
   * length as version 8, and the version field is the only thing telling
   * them apart, which is the case worth a test of its own. */
  TEST_ASSERT_EQUAL_size_t(193, offsetof(Settings, fmSquelchFloor));
  TEST_ASSERT_EQUAL_size_t(V8_SIZE, sizeof(Settings));

  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 91100;
  source.bandFreqKHz[BAND_MW] = 738;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V8_SIZE);
  blob[193] = 0xAB; /* Padding, as far as version 8 was concerned. */
  blob[194] = 0xCD;
  blob[195] = 0xEF;
  uint16_t version = 8;
  uint16_t size = V8_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V8_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT32(91100, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT32(738, out.bandFreqKHz[BAND_MW]);

  Settings fresh;
  settingsDefaults(&fresh);
  TEST_ASSERT_EQUAL_UINT8(fresh.fmSquelchFloor, out.fmSquelchFloor);
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

static void a_version_9_blob_gets_the_defaults_for_what_it_never_had(void) {
  /* rdsEnabled sits at 194, inside the two bytes version 9 wrote as padding
   * after fmSquelchFloor. Version 10 is therefore the same length as version
   * 9, and the version field is the only thing telling them apart. The same
   * case as version 8 above, and worth its own test for the same reason: a
   * blob copied by its written length would take the new field out of the
   * old padding and a radio would come back with RDS switched off by a byte
   * that never meant anything. */
  TEST_ASSERT_EQUAL_size_t(194, offsetof(Settings, rdsEnabled));
  TEST_ASSERT_EQUAL_size_t(V9_SIZE, sizeof(Settings));

  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 93500;
  source.fmSquelchFloor = 12;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V9_SIZE);
  /* Padding, as far as version 9 was concerned. Zero would have read as
   * switched off, which is why this is deliberately not zero. */
  blob[194] = 0x00;
  blob[195] = 0xEF;
  uint16_t version = 9;
  uint16_t size = V9_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V9_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT32(93500, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(12, out.fmSquelchFloor);

  Settings fresh;
  settingsDefaults(&fresh);
  TEST_ASSERT_EQUAL_UINT8(fresh.rdsEnabled, out.rdsEnabled);
  TEST_ASSERT_TRUE(out.rdsEnabled != 0);
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

static void the_rds_decoder_is_on_by_default(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(s.rdsEnabled != 0);
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.rdsEnabled = 0;
  TEST_ASSERT_TRUE(settingsValid(&s));
}

static void the_squelch_floor_has_a_range(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_EQUAL_UINT8(SQUELCH_FM_LEVEL_FLOOR_DBUV, s.fmSquelchFloor);
  TEST_ASSERT_TRUE(settingsValid(&s));

  s.fmSquelchFloor = 0; /* Off is a real choice. */
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.fmSquelchFloor = SQUELCH_FM_LEVEL_FLOOR_MAX_DBUV;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.fmSquelchFloor = SQUELCH_FM_LEVEL_FLOOR_MAX_DBUV + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void the_panel_light_settings_have_ranges(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));

  /* The boundary, one below and one above. Anything under the floor is a
   * panel nobody can read while they are using the radio, and the only
   * control for it is the page that has just gone dark. */
  s.backlightPercent = BACKLIGHT_MIN_AWAKE;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.backlightPercent = BACKLIGHT_MIN_AWAKE - 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.backlightPercent = 100;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.backlightPercent = 101;
  TEST_ASSERT_FALSE(settingsValid(&s));

  /* The dim level has no floor. It is left on purpose and any input brings
   * the panel back, so nothing can be stranded by it. */
  settingsDefaults(&s);
  s.backlightDimPercent = 0;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.backlightDimPercent = 100;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.backlightDimPercent = 101;
  TEST_ASSERT_FALSE(settingsValid(&s));

  settingsDefaults(&s);
  s.backlightDimAfterS = 0; /* Never dim is a real choice. */
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.backlightDimAfterS = BACKLIGHT_DIM_AFTER_MAX_S;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.backlightDimAfterS = BACKLIGHT_DIM_AFTER_MAX_S + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));

  settingsDefaults(&s);
  s.backlightFade = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void the_polish_settings_have_ranges(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));

  s.softMuteMs = 0; /* Off is a real choice: cut instantly. */
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.softMuteMs = 500;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.softMuteMs = 501;
  TEST_ASSERT_FALSE(settingsValid(&s));

  settingsDefaults(&s);
  /* The beep is a mode, not a switch: off, keys, keys and long presses, or
   * every press. */
  for (uint8_t m = 0; m < (uint8_t)BEEP_MODE_COUNT; m++) {
    s.beepKey = m;
    TEST_ASSERT_TRUE(settingsValid(&s));
  }
  s.beepKey = (uint8_t)BEEP_MODE_COUNT;
  TEST_ASSERT_FALSE(settingsValid(&s));

  s.beepKey = (uint8_t)BEEP_KEYS;
  s.beepEdge = 9;
  TEST_ASSERT_FALSE(settingsValid(&s));

  settingsDefaults(&s);
  s.beepStart = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_pot_calibration_is_judged_on_the_loud_end(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s)); /* Both zero, so not measured. */

  /* The case this radio actually produces. Its knob reads 0 at the bottom, so
   * a correct sweep gives 0 and 4095, and a rule that took a zero quiet end
   * to mean "not measured" would refuse it. */
  s.potRawMin = 0;
  s.potRawMax = 4095;
  TEST_ASSERT_TRUE(settingsValid(&s));

  /* A quiet end with no loud end says nothing. */
  s.potRawMin = 120;
  s.potRawMax = 0;
  TEST_ASSERT_FALSE(settingsValid(&s));

  /* The loud end has to be above the quiet one. */
  s.potRawMin = 3000;
  s.potRawMax = 100;
  TEST_ASSERT_FALSE(settingsValid(&s));

  /* And neither can be past the end of the converter. */
  s.potRawMin = 0;
  s.potRawMax = 5000;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_scan_sensitivity_outside_the_range_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  s.fmScanSensitivity = 0;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.fmScanSensitivity = SEEK_SENSITIVITY_MAX + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.fmScanSensitivity = SEEK_SENSITIVITY_MAX;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.amScanSensitivity = 9;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_version_1_blob_of_the_wrong_length_is_refused(void) {
  /* The size in the header is not enough on its own: a corrupt blob can
   * declare a length that matches its own truncation. */
  uint8_t blob[V1_SIZE];
  size_t len = makeV1(blob, "TARANG", "hunter2", 1);
  Settings out;
  TEST_ASSERT_FALSE(settingsFromBlob(blob, len - 1, &out));
  TEST_ASSERT_FALSE(settingsFromBlob(blob, len + 1, &out));
}

/* ----------------------------------------------- the new ranges are checked */

static void a_blend_start_is_off_or_somewhere_a_signal_reaches(void) {
  /* A level below 20 dBuV switches the mechanism on at a point no signal
   * gets to, so it is on and does nothing. That is the silent no-op. */
  Settings s;
  settingsDefaults(&s);
  s.fmHighCutStart = 0;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.fmHighCutStart = 40;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.fmHighCutStart = 19;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.fmHighCutStart = 61;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_noise_blanker_is_a_percentage(void) {
  Settings s;
  settingsDefaults(&s);
  s.amNoiseBlankerStart = 0;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.amNoiseBlankerStart = 100;
  TEST_ASSERT_TRUE(settingsValid(&s));
  /* The range that reads like dBuV and is not. */
  s.amNoiseBlankerStart = 30;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.amNoiseBlankerStart = 151;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void only_the_widths_the_am_side_has_are_accepted(void) {
  Settings s;
  settingsDefaults(&s);
  uint8_t good[] = {3, 4, 6, 8};
  for (size_t i = 0; i < sizeof(good); i++) {
    s.amBandwidthKHz = good[i];
    TEST_ASSERT_TRUE(settingsValid(&s));
  }
  s.amBandwidthKHz = 5;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.amBandwidthKHz = 0;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void the_stored_volume_stays_inside_what_the_chip_takes(void) {
  /* It is only read in manual squelch, where the knob is the squelch and
   * nothing else says how loud to be. A value outside the chip's range would
   * come up at whatever the driver clamped it to. */
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));

  s.startVolumeDb = 0;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.startVolumeDb = RADIO_VOLUME_MIN;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.startVolumeDb = 1; /* Above what the knob can ask for. */
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.startVolumeDb = RADIO_VOLUME_MIN - 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_deemphasis_that_is_not_one_of_the_two_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  s.fmDeemphasisUs = 50;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.fmDeemphasisUs = 75;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.fmDeemphasisUs = 60;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void the_new_settings_survive_a_round_trip(void) {
  Settings s;
  settingsDefaults(&s);
  s.fmRegion = 1;
  s.mwSpacing = 1;
  s.squelchMode = 2;
  s.fmMultipathSuppression = 1;
  s.fmHighCutStart = 40;
  s.amNoiseBlankerStart = 100;
  s.startFreqKHz = 102800;
  s.startVolumeDb = -30;
  TEST_ASSERT_TRUE(settingsValid(&s));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(&s, sizeof(s), &out));
  TEST_ASSERT_EQUAL_UINT8(1, out.fmRegion);
  TEST_ASSERT_EQUAL_UINT8(2, out.squelchMode);
  TEST_ASSERT_EQUAL_UINT8(40, out.fmHighCutStart);
  TEST_ASSERT_EQUAL_UINT8(100, out.amNoiseBlankerStart);
  TEST_ASSERT_EQUAL_UINT32(102800, out.startFreqKHz);
  TEST_ASSERT_EQUAL_INT8(-30, out.startVolumeDb);
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
  RUN_TEST(the_version_1_fields_never_moved);
  RUN_TEST(a_version_1_blob_still_reads);
  RUN_TEST(a_version_1_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(a_version_1_blob_of_the_wrong_length_is_refused);
  RUN_TEST(the_version_2_fields_never_moved);
  RUN_TEST(a_version_2_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(a_version_2_blob_of_the_wrong_length_is_refused);
  RUN_TEST(a_version_3_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(the_version_3_fields_never_moved);
  RUN_TEST(a_new_field_inside_old_padding_is_not_read_from_it);
  RUN_TEST(a_version_4_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(a_version_5_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(a_version_6_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(a_version_7_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(a_version_8_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(a_version_9_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(the_rds_decoder_is_on_by_default);
  RUN_TEST(the_squelch_floor_has_a_range);
  RUN_TEST(a_stored_tuning_mode_outside_the_enum_is_refused);
  RUN_TEST(the_panel_light_settings_have_ranges);
  RUN_TEST(the_polish_settings_have_ranges);
  RUN_TEST(a_pot_calibration_is_judged_on_the_loud_end);
  RUN_TEST(a_scan_sensitivity_outside_the_range_is_refused);
  RUN_TEST(a_blend_start_is_off_or_somewhere_a_signal_reaches);
  RUN_TEST(a_noise_blanker_is_a_percentage);
  RUN_TEST(only_the_widths_the_am_side_has_are_accepted);
  RUN_TEST(the_stored_volume_stays_inside_what_the_chip_takes);
  RUN_TEST(a_deemphasis_that_is_not_one_of_the_two_is_refused);
  RUN_TEST(the_new_settings_survive_a_round_trip);

  return UNITY_END();
}
