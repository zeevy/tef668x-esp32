/*
 * Tests for the settings struct, its defaults and its migration.
 * Runs on a PC.
 */
#include <unity.h>

#include "core/agc.h"
#include "core/backlight.h"
#include "core/band_plan.h"
#include "core/clock.h"
#include "core/dx_scan.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/palette.h"
#include "core/radio.h"
#include "core/rds_country.h"
#include "core/seek.h"
#include "core/settings.h"
#include "core/settings_table.h"
#include "core/squelch.h"
#include "core/wifi_join.h"

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
  TEST_ASSERT_EQUAL_UINT8(PALETTE_THEME_DEFAULT_DAY, s.theme);
  TEST_ASSERT_EQUAL_UINT8(PALETTE_THEME_DEFAULT_NIGHT, s.nightTheme);
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

/* A newer firmware's struct: this one's fields first, then `extra` bytes of
 * fields this firmware does not know. */
static size_t makeNewer(uint8_t *blob, const Settings *from, size_t extra) {
  memcpy(blob, from, sizeof(Settings));
  memset(blob + sizeof(Settings), 0xA5, extra);
  uint16_t version = SETTINGS_VERSION + 1;
  uint16_t size = (uint16_t)(sizeof(Settings) + extra);
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  return sizeof(Settings) + extra;
}

/* What an update that rolled back leaves: the settings, Wi-Fi and PIN come
 * across, read up to the fields this firmware knows. */
static void a_blob_from_a_newer_firmware_is_read_up_to_the_known_fields(void) {
  Settings written;
  settingsDefaults(&written);
  settingsSetWifi(&written, "MyNetwork", "hunter2hunter2");
  written.accessPin = 123456;
  written.startFreqKHz = 102800;
  uint8_t blob[sizeof(Settings) + 16];
  const size_t len = makeNewer(blob, &written, 16);

  Settings read;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, len, &read));
  TEST_ASSERT_TRUE(settingsHasWifi(&read));
  TEST_ASSERT_EQUAL_STRING("MyNetwork", read.wifiSsid);
  TEST_ASSERT_EQUAL_UINT32(123456, read.accessPin);
  TEST_ASSERT_EQUAL_UINT32(102800, read.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, read.version);
  TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(Settings), read.size);
  TEST_ASSERT_TRUE(settingsValid(&read));
}

/* A newer firmware that only used this one's padding writes the same
 * length, and reads the same way. */
static void a_newer_blob_of_the_same_length_is_read_too(void) {
  Settings written;
  settingsDefaults(&written);
  settingsSetWifi(&written, "MyNetwork", "hunter2hunter2");
  uint8_t blob[sizeof(Settings)];
  const size_t len = makeNewer(blob, &written, 0);
  Settings read;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, len, &read));
  TEST_ASSERT_TRUE(settingsHasWifi(&read));
}

/* A newer version never shrinks the struct, so a shorter one, or one whose
 * size is not its length, is corrupt and the defaults are safer. */
static void a_newer_blob_that_is_short_or_mislabelled_falls_back(void) {
  Settings written;
  settingsDefaults(&written);
  settingsSetWifi(&written, "MyNetwork", "hunter2hunter2");
  uint8_t blob[sizeof(Settings) + 8];
  size_t len = makeNewer(blob, &written, 8);
  Settings read;
  TEST_ASSERT_FALSE(settingsFromBlob(blob, len - 9, &read));
  TEST_ASSERT_FALSE(settingsHasWifi(&read));
  TEST_ASSERT_TRUE(settingsValid(&read));
  TEST_ASSERT_FALSE(settingsFromBlob(blob, len - 1, &read));
  TEST_ASSERT_FALSE(settingsHasWifi(&read));
}

/* A newer firmware's value this one cannot use, such as a hotspot mode it
 * does not have, still costs the struct: the defaults come up. */
static void a_newer_blob_holding_a_value_this_firmware_refuses_falls_back(
    void) {
  Settings written;
  settingsDefaults(&written);
  settingsSetWifi(&written, "MyNetwork", "hunter2hunter2");
  written.hotspot = 9;
  uint8_t blob[sizeof(Settings) + 4];
  const size_t len = makeNewer(blob, &written, 4);
  Settings read;
  TEST_ASSERT_FALSE(settingsFromBlob(blob, len, &read));
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
  /* A blob that claims version 1 at any other length than the version 1
   * size is corrupt, not old. */
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

/* Every setting the table holds is refused one step outside the range the
 * table gives it, the same range the API and the menu offer. */
static void a_value_outside_its_table_range_is_rejected(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));
  for (size_t i = 0; i < settingsTableCount(); i++) {
    const SettingRow *row = settingsTableAt(i);
    const int32_t least =
        !row->isSigned ? 0 : (row->size == 2 ? INT16_MIN : INT8_MIN);
    const int32_t most = row->size == 2
                             ? (row->isSigned ? INT16_MAX : UINT16_MAX)
                             : (row->isSigned ? INT8_MAX : UINT8_MAX);
    if (row->low > least) {
      settingsDefaults(&s);
      settingsTableSet(&s, row, row->low - 1);
      TEST_ASSERT_FALSE_MESSAGE(settingsValid(&s), row->key);
    }
    if (row->high < most) {
      settingsDefaults(&s);
      settingsTableSet(&s, row, row->high + 1);
      TEST_ASSERT_FALSE_MESSAGE(settingsValid(&s), row->key);
    }
  }
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
  size_t len = makeV1(blob, "OLDNET", "hunter2", 123456);

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, len, &out));
  TEST_ASSERT_EQUAL_STRING("OLDNET", out.wifiSsid);
  TEST_ASSERT_EQUAL_STRING("hunter2", out.wifiPass);
  TEST_ASSERT_EQUAL_UINT32(123456, out.accessPin);
}

static void a_version_1_blob_gets_the_defaults_for_what_it_never_had(void) {
  uint8_t blob[V1_SIZE];
  size_t len = makeV1(blob, "OLDNET", "hunter2", 123456);

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

/* How many bytes version 10 wrote. Fixed for good, like the ones above. */
#define V10_SIZE 196

/* How many bytes version 11 wrote. Fixed for good, like the ones above. */
#define V11_SIZE 200

/* Version 12 wrote the same 200 bytes. `tuneMode` went into its padding. */
#define V12_SIZE 200

/* And version 13 the same again. It filled the last byte version 12 had. */
#define V13_SIZE 200

/*
 * What version 14 writes.
 *
 * It grew for the first time since version 9: the two AGC settings went on
 * the end because version 13 had used the last byte of padding. 202 bytes of
 * fields rounded up to 204 by the alignment the struct already had.
 */
#define V14_SIZE 204

/*
 * Version 15 writes the same 204 bytes.
 *
 * The two meter segment settings went into the two bytes version 14 wrote as
 * padding after `agcBoostDb`, so the struct did not grow and only the version
 * field tells the two apart. The same case as versions 6, 10, 12 and 13.
 */
#define V15_SIZE 204

/*
 * Version 16 writes 208.
 *
 * The two signal scale settings went on the end at offsets 204 and 205,
 * because version 15 had filled its last byte, so the struct grew for the
 * first time since version 14. 206 bytes of fields rounded up to 208 by the
 * alignment the struct already had.
 */
#define V16_SIZE 208

/*
 * Version 17 writes 256.
 *
 * `theme` went into the two bytes version 16 left as padding, and
 * `customTheme`'s forty eight bytes went on the end past it, so the struct
 * grew from 208 to 256.
 */
#define V17_SIZE 256

/*
 * Version 18 writes 248.
 *
 * Version 18 shrank `customTheme` from sixteen rows to thirteen, nine bytes
 * narrower, so the struct got smaller for the first time: 256 down to 248 once
 * the alignment the struct already had is applied.
 */
#define V18_SIZE 248

/*
 * Version 20 writes 252, and so does version 21, which added no field.
 *
 * Version 19 put `displayRotation` in the padding byte at 246, so it still
 * wrote 248. Version 20's four AM start levels begin at 247, in the padding
 * after it, and run past the old end.
 */
#define V20_SIZE 252

/*
 * Version 22 writes 264: the DX SETUP menu's seven bytes from the padding
 * at 251, then its two 16-bit fields. Version 23's `rdsRegion` went into
 * its padding at 262, and version 24's `dxLogRt` into the byte after it, so
 * both write 264 too.
 */
#define V22_SIZE 264

/* Version 25 writes 268: `dxWatch` at 264, and the padding after it, where
 * version 26's two level offsets and version 27's night theme went, so both
 * write 268 too. */
#define V25_SIZE 268

/* Version 28 writes 272: `hotspot` at 268, and three bytes of padding after
 * it, where version 29's two switches and version 30's auto off byte went,
 * so both write 272 too. Version 31 writes 276. */
#define V28_SIZE 272

/* Version 31 writes 276: its two byte auto off at 272, then the update
 * check and the touch switch at 274 and 275, in what it first wrote as
 * padding. */
#define V31_SIZE 276

/* What the struct is today: version 32's keypad timeout at 276, and three
 * bytes of padding. */
#define V32_SIZE 280

/* Stamp the first V31_SIZE bytes of a current struct as the version 31 blob
 * they are: version 32 only appended. */
static void stampV31(uint8_t *blob) {
  uint16_t version = 31;
  uint16_t size = V31_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
}

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
   * kFieldEndOfVersion in settings.c exists to stop. */
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
  }
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

static void a_stored_tuning_mode_outside_the_enum_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.tuneMode = (uint8_t)TUNE_MODE_COUNT - 1;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.tuneMode = (uint8_t)TUNE_MODE_COUNT;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_version_8_blob_gets_the_defaults_for_what_it_never_had(void) {
  /* fmSquelchFloor sits at 193, inside the three bytes version 8 wrote as
   * padding after its last per band array. Version 9 is therefore the same
   * length as version 8, and the version field is the only thing telling
   * them apart, which is the case worth a test of its own. */
  TEST_ASSERT_EQUAL_size_t(193, offsetof(Settings, fmSquelchFloor));

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

static void a_version_10_blob_gets_the_defaults_for_what_it_never_had(void) {
  /* ntpEnabled sits at 195, inside the single byte version 10 wrote as
   * padding after rdsEnabled. Version 11 is a different length because
   * clockOffsetMinutes needs two byte alignment and lands at 196, but the
   * byte before it is still old padding, and copying by the written length
   * would take ntpEnabled out of it. The same case as versions 8 and 9. */
  TEST_ASSERT_EQUAL_size_t(195, offsetof(Settings, ntpEnabled));
  TEST_ASSERT_EQUAL_size_t(196, offsetof(Settings, clockOffsetMinutes));

  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 106400;
  source.rdsEnabled = 0;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V10_SIZE);
  /* Padding, as far as version 10 was concerned. Zero would have read as
   * switched off, which is why this is deliberately not zero. */
  blob[195] = 0xEF;
  uint16_t version = 10;
  uint16_t size = V10_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V10_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT32(106400, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(0, out.rdsEnabled);

  Settings fresh;
  settingsDefaults(&fresh);
  TEST_ASSERT_EQUAL_UINT8(fresh.ntpEnabled, out.ntpEnabled);
  TEST_ASSERT_TRUE(out.ntpEnabled != 0);
  TEST_ASSERT_EQUAL_INT16(0, out.clockOffsetMinutes);
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

static void a_version_11_blob_gets_the_defaults_for_what_it_never_had(void) {
  /* `batteryShow` sits at 198, in the padding version 11 wrote after
   * `clockOffsetMinutes`, so version 12 is the same length as version 11 and
   * only the version field tells them apart. The same case as versions 8, 9
   * and 10. */
  TEST_ASSERT_EQUAL_size_t(198, offsetof(Settings, batteryShow));
  /* The struct is a different size than version 11's now, because later
   * versions grew it. What matters here is that nothing before
   * `batteryShow` moved, which the offset above is the real check of. */
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));

  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 95000;
  source.clockOffsetMinutes = 330;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V11_SIZE);
  /* Padding, as far as version 11 was concerned, and deliberately a value
   * that would be a legal setting if it leaked through. */
  blob[198] = 2;
  blob[199] = 0xEF;
  uint16_t version = 11;
  uint16_t size = V11_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V11_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT32(95000, out.startFreqKHz);
  TEST_ASSERT_EQUAL_INT16(330, out.clockOffsetMinutes);
  TEST_ASSERT_EQUAL_UINT8((uint8_t)BATTERY_SHOW_OFF, out.batteryShow);
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

/*
 * The case the field table exists for, and the first where getting it wrong
 * would be obvious rather than lucky.
 *
 * Both meter fields have a minimum of 1, so a version 14 blob copied by its
 * written length would bring two zeros in and `settingsValid` would refuse
 * the whole struct. The radio would come up on defaults with the PIN, the
 * station and the calibration gone.
 */
static void a_version_14_blob_does_not_read_its_padding_as_a_meter(void) {
  TEST_ASSERT_EQUAL_size_t(202, offsetof(Settings, meterSegW));
  TEST_ASSERT_EQUAL_size_t(203, offsetof(Settings, meterSegGap));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));
  TEST_ASSERT_EQUAL_size_t(V14_SIZE, V15_SIZE);

  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 106400;
  source.agcTargetPercent = 50;
  source.agcBoostDb = 4;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V14_SIZE);
  /* Padding, as far as version 14 was concerned. Zero is what a radio in the
   * field actually holds here, and zero is outside both ranges. */
  blob[202] = 0;
  blob[203] = 0;
  uint16_t version = 14;
  uint16_t size = V14_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V14_SIZE, &out));
  /* What version 14 held comes back. */
  TEST_ASSERT_EQUAL_UINT32(106400, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(50, out.agcTargetPercent);
  TEST_ASSERT_EQUAL_UINT8(4, out.agcBoostDb);
  /* And the two new ones are the defaults, not the zeros that were there. */
  TEST_ASSERT_EQUAL_UINT8(METER_SEG_W, out.meterSegW);
  TEST_ASSERT_EQUAL_UINT8(METER_SEG_GAP, out.meterSegGap);
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

/* Anything that would leak through the padding is caught, not just zero. */
static void a_version_14_blob_with_dirty_padding_is_still_clean(void) {
  Settings source;
  settingsDefaults(&source);

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V14_SIZE);
  /* Values that would be legal settings if they leaked through, so a test
   * that passed only because zero is refused would fail here. */
  blob[202] = 7;
  blob[203] = 4;
  uint16_t version = 14;
  uint16_t size = V14_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V14_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(METER_SEG_W, out.meterSegW);
  TEST_ASSERT_EQUAL_UINT8(METER_SEG_GAP, out.meterSegGap);
}

static void a_meter_shape_outside_the_range_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));

  s.meterSegW = 0;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.meterSegW = METER_SEG_W_MAX + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.meterSegW = METER_SEG_W_MAX;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.meterSegW = METER_SEG_W_MIN;
  TEST_ASSERT_TRUE(settingsValid(&s));

  s.meterSegGap = 0;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.meterSegGap = METER_SEG_GAP_MAX + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.meterSegGap = METER_SEG_GAP_MAX;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.meterSegGap = METER_SEG_GAP_MIN;
  TEST_ASSERT_TRUE(settingsValid(&s));
}

static void a_version_12_blob_keeps_the_mode_of_the_band_it_was_on(void) {
  /* `tuneMode` sits at 199, in the single byte version 12 wrote as padding
   * after `batteryShow`, so version 13 is the same length as version 12.
   *
   * Up to version 12 the mode belonged to each band. The upgrade takes the
   * one for the band the radio was left on, so a radio that was seeking is
   * still seeking after the update rather than back on manual. */
  TEST_ASSERT_EQUAL_size_t(199, offsetof(Settings, tuneMode));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));

  Settings source;
  settingsDefaults(&source);
  source.startBand = (uint8_t)BAND_SW;
  source.bandTuneModeV12[BAND_SW] = (uint8_t)TUNE_MODE_AUTO;
  source.bandTuneModeV12[BAND_FM] = (uint8_t)TUNE_MODE_MEMORY;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V12_SIZE);
  /* Padding, as far as version 12 was concerned, and deliberately a value
   * outside the enum so that leaking it through would be caught. */
  blob[199] = 0xEF;
  uint16_t version = 12;
  uint16_t size = V12_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V12_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8((uint8_t)TUNE_MODE_AUTO, out.tuneMode);
  TEST_ASSERT_TRUE(settingsValid(&out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

static void a_version_12_blob_with_a_mode_outside_the_enum_falls_back(void) {
  /* The old array was range checked when it was live and is not any more, so
   * the upgrade has to check what it reads rather than trust it. */
  Settings source;
  settingsDefaults(&source);
  source.startBand = (uint8_t)BAND_MW;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V12_SIZE);
  blob[offsetof(Settings, bandTuneModeV12) + BAND_MW] =
      (uint8_t)TUNE_MODE_COUNT;
  uint16_t version = 12;
  uint16_t size = V12_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V12_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8((uint8_t)TUNE_MODE_MANUAL, out.tuneMode);
  TEST_ASSERT_TRUE(settingsValid(&out));
}

static void the_battery_ships_switched_off(void) {
  Settings s;
  settingsDefaults(&s);
  /* Nothing about the reading has been confirmed on this unit, so it is not
   * shown until somebody asks. */
  TEST_ASSERT_EQUAL_UINT8((uint8_t)BATTERY_SHOW_OFF, s.batteryShow);
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.batteryShow = (uint8_t)BATTERY_SHOW_VOLTS;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.batteryShow = (uint8_t)BATTERY_SHOW_COUNT;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void network_time_is_on_by_default_and_the_offset_is_utc(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(s.ntpEnabled != 0);
  /* Not the offset of the place this radio was built. A default that is right
   * in one country is silently wrong everywhere else. */
  TEST_ASSERT_EQUAL_INT16(0, s.clockOffsetMinutes);
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.ntpEnabled = 0;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.ntpEnabled = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_utc_offset_no_place_uses_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  s.clockOffsetMinutes = CLOCK_OFFSET_MAX_MINUTES;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.clockOffsetMinutes = CLOCK_OFFSET_MIN_MINUTES;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.clockOffsetMinutes = CLOCK_OFFSET_MAX_MINUTES + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.clockOffsetMinutes = CLOCK_OFFSET_MIN_MINUTES - 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
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

static void the_soft_mute_and_beep_settings_have_ranges(void) {
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
  size_t len = makeV1(blob, "OLDNET", "hunter2", 1);
  Settings out;
  TEST_ASSERT_FALSE(settingsFromBlob(blob, len - 1, &out));
  TEST_ASSERT_FALSE(settingsFromBlob(blob, len + 1, &out));
}

/* ----------------------------------- the tuner and audio ranges are checked */

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

static void the_tuner_and_start_settings_survive_a_round_trip(void) {
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

static void a_version_13_blob_comes_back_with_the_agc_off(void) {
  /* The two AGC settings went on the end, so a version 13 blob carries
   * nothing for them and they land on their defaults. Off is the right
   * default for a setting that changes what a person hears: a radio that
   * upgrades must sound exactly as it did before. */
  TEST_ASSERT_EQUAL_size_t(200, offsetof(Settings, agcTargetPercent));

  Settings source;
  settingsDefaults(&source);
  source.agcTargetPercent = 60;
  source.agcBoostDb = 4;
  source.startFreqKHz = 95000;

  uint8_t blob[V14_SIZE];
  memcpy(blob, &source, V13_SIZE);
  uint16_t version = 13;
  memcpy(blob, &version, sizeof(version));
  uint16_t size = V13_SIZE;
  memcpy(blob + sizeof(version), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V13_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT32(95000, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(0, out.agcTargetPercent);
  TEST_ASSERT_EQUAL_UINT8(0, out.agcBoostDb);
}

static void an_agc_target_is_zero_or_a_real_one(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));

  s.agcTargetPercent = 0;
  TEST_ASSERT_TRUE(settingsValid(&s));

  /* Both ends of the range, and one step outside each. The band below
   * AGC_TARGET_MIN is not a quieter setting, it is a number the AGC cannot do
   * anything sensible with. */
  s.agcTargetPercent = AGC_TARGET_MIN;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.agcTargetPercent = AGC_TARGET_MAX;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.agcTargetPercent = AGC_TARGET_MIN - 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.agcTargetPercent = AGC_TARGET_MAX + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));

  s.agcTargetPercent = 60;
  s.agcBoostDb = AGC_BOOST_MAX;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.agcBoostDb = AGC_BOOST_MAX + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

/*
 * Version 15 filled its last byte, so the two signal scale bytes went on the
 * end rather than into padding. A version 15 blob is copied whole and the
 * bytes past its end are not read, which is the opposite of the version 14
 * case above and is why the two tables are separate. Nothing reads the two
 * bytes now, but the layout still has them.
 */
static void a_version_15_blob_is_not_read_past_its_end(void) {
  TEST_ASSERT_EQUAL_size_t(204, offsetof(Settings, sigFullFmDbuV));
  TEST_ASSERT_EQUAL_size_t(205, offsetof(Settings, sigFullAmDbuV));
  TEST_ASSERT_EQUAL_size_t(V15_SIZE, offsetof(Settings, sigFullFmDbuV));

  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 95000;
  source.meterSegW = METER_SEG_W_MAX;
  /* Values a version 15 radio could never have written, to prove they are
   * not read back out of the bytes past its end. */
  source.sigFullFmDbuV = 90;
  source.sigFullAmDbuV = 70;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V15_SIZE);
  uint16_t version = 15;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  uint16_t size = V15_SIZE;
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V15_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
  TEST_ASSERT_EQUAL_UINT32(95000, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(METER_SEG_W_MAX, out.meterSegW);
  TEST_ASSERT_EQUAL_UINT8(0, out.sigFullFmDbuV);
  TEST_ASSERT_EQUAL_UINT8(0, out.sigFullAmDbuV);
  TEST_ASSERT_TRUE(settingsValid(&out));
}

/*
 * `theme` sits at 206, in the first of the two bytes version 16 left as
 * padding after `sigFullAmDbuV`, so a version 16 blob is exactly V16_SIZE
 * bytes and reaches one byte into where `theme` now lives. It must not be
 * read from there: that byte was never a colour index as far as version 16
 * knew, and `customTheme` past it was never written at all.
 */
static void a_version_16_blob_does_not_read_its_padding_as_a_theme(void) {
  TEST_ASSERT_EQUAL_size_t(206, offsetof(Settings, theme));
  TEST_ASSERT_EQUAL_size_t(207, offsetof(Settings, customTheme));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));

  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 95000;
  source.sigFullFmDbuV = 90;

  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V16_SIZE);
  /* Padding, as far as version 16 was concerned. The theme comes back as 0
   * whatever this byte holds, because every blob before version 21 goes to
   * theme 0, so the theme check below does not show the byte was skipped. */
  blob[206] = 9;
  uint16_t version = 16;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  uint16_t size = V16_SIZE;
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V16_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
  TEST_ASSERT_EQUAL_UINT32(95000, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(90, out.sigFullFmDbuV);
  TEST_ASSERT_EQUAL_UINT8(0, out.theme);
  Settings defaults;
  settingsDefaults(&defaults);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(defaults.customTheme, out.customTheme,
                                sizeof(defaults.customTheme));
  TEST_ASSERT_TRUE(settingsValid(&out));
}

/*
 * A version 17 blob's `theme` is an index into ui/theme.c's table as that
 * version knew it, and version 18 renamed nine of its ten rows and moved every
 * one but Nightwatch to a different position, so an index copied across unread
 * would silently pick the wrong theme rather than the one it used to name.
 * `customTheme` is the same problem one level further in: its thirteen rows now
 * mean something else than its old sixteen did. `customTheme` resets to the
 * default, the same as a version 16 blob's never-written one does above. The
 * theme becomes 0, because every blob before version 21 goes to theme 0, so
 * the custom colours are the real check here.
 */
static void a_version_17_blob_does_not_carry_over_its_theme(void) {
  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 95000;

  /*
   * A version 17 blob is 256 bytes and this build's own Settings is a
   * different size, so it cannot be built by copying a live struct
   * the way the version 16 test above does: past `theme` at 206 this build's
   * struct and version 17's blob no longer agree on what a byte means.
   * Everything up to there is copied; the old shaped `theme` and forty eight
   * byte `customTheme` past it are written by hand.
   */
  uint8_t blob[V17_SIZE];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, offsetof(Settings, theme));
  blob[206] = 4; /* A legal index in both tables. */
  memset(blob + 207, 0x77, 48);
  uint16_t version = 17;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  uint16_t size = V17_SIZE;
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V17_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
  TEST_ASSERT_EQUAL_UINT32(95000, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(0, out.theme);
  Settings defaults;
  settingsDefaults(&defaults);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(defaults.customTheme, out.customTheme,
                                sizeof(defaults.customTheme));
  TEST_ASSERT_TRUE(settingsValid(&out));
}

/*
 * `displayRotation` sits at 246, in the single byte version 18 left as
 * padding after `customTheme`'s thirty nine bytes (207 to 246), so a
 * version 18 blob is V18_SIZE bytes and reaches into where
 * `displayRotation` now lives
 * without ever having written it. It must not be read from there: a byte
 * that was never a rotation as far as version 18 knew could just as
 * easily read as 180 as 0.
 */
static void a_version_18_blob_does_not_read_its_padding_as_a_rotation(void) {
  TEST_ASSERT_EQUAL_size_t(246, offsetof(Settings, displayRotation));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));

  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 95000;

  uint8_t blob[V18_SIZE];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, offsetof(Settings, displayRotation));
  /* Padding, as far as version 18 was concerned, and deliberately a value
   * that would fail settingsValid outright if it leaked through, so a bug
   * here cannot hide behind a coincidentally legal byte. */
  blob[246] = 0x55;
  uint16_t version = 18;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  uint16_t size = V18_SIZE;
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V18_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
  TEST_ASSERT_EQUAL_UINT32(95000, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(0, out.displayRotation);
  TEST_ASSERT_TRUE(settingsValid(&out));
}

/*
 * A version 19 blob is 248 bytes. Its last byte, 247, was padding, and is where
 * `amHighCutStart` now lives, so it must not be read. The four AM start levels
 * get their defaults, and an AM blanker stored as 0, the old default, moves to
 * the new one.
 */
static void a_version_19_blob_gets_the_am_defaults_and_the_blanker_on(void) {
  TEST_ASSERT_EQUAL_size_t(247, offsetof(Settings, amHighCutStart));

  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 95000;
  source.amNoiseBlankerStart = 0;

  uint8_t blob[V18_SIZE];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, offsetof(Settings, amHighCutStart));
  blob[247] = 0x55; /* Padding to version 19, and not a legal level. */
  uint16_t version = 19;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  uint16_t size = V18_SIZE;
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));

  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V18_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
  TEST_ASSERT_EQUAL_UINT32(95000, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(47, out.amHighCutStart);
  TEST_ASSERT_EQUAL_UINT8(52, out.lwHighCutStart);
  TEST_ASSERT_EQUAL_UINT8(28, out.amSoftMuteStart);
  TEST_ASSERT_EQUAL_UINT8(34, out.lwSoftMuteStart);
  TEST_ASSERT_EQUAL_UINT8(100, out.amNoiseBlankerStart);

  /* A blanker someone set by hand is a choice, and stays. */
  blob[offsetof(Settings, amNoiseBlankerStart)] = 120;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V18_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(120, out.amNoiseBlankerStart);
}

/* A version 20 radio that turned the blanker off keeps it off. */
static void a_version_20_blanker_that_is_off_stays_off(void) {
  Settings source;
  settingsDefaults(&source);
  source.version = 20;
  source.size = V20_SIZE;
  source.amNoiseBlankerStart = 0;
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob((const uint8_t *)&source, V20_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.amNoiseBlankerStart);
}

/* Every theme a version 20 radio saved becomes Nightwatch, and the Custom
 * colours it kept are carried across untouched. */
static void a_version_20_theme_becomes_nightwatch_and_keeps_custom(void) {
  Settings source;
  settingsDefaults(&source);
  source.version = 20;
  source.size = V20_SIZE;
  source.theme = 10;
  source.customTheme[3][0] = 0x12;
  source.customTheme[3][1] = 0x34;
  source.customTheme[3][2] = 0x56;
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob((const uint8_t *)&source, V20_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.theme);
  TEST_ASSERT_EQUAL_UINT8(0x12, out.customTheme[3][0]);
  TEST_ASSERT_EQUAL_UINT8(0x34, out.customTheme[3][1]);
  TEST_ASSERT_EQUAL_UINT8(0x56, out.customTheme[3][2]);
}

/* A version 21 radio keeps the theme it picked. */
static void a_version_21_theme_is_kept(void) {
  Settings source;
  settingsDefaults(&source);
  source.version = 21;
  source.size = V20_SIZE;
  source.theme = 3;
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob((const uint8_t *)&source, V20_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(3, out.theme);
}

/*
 * A version 21 blob is 252 bytes. Its last byte, 251, was padding, and is
 * where `dxStopRule` now lives, so it must not be read. The DX SETUP
 * fields get their defaults: DX mode scans as it did before the menu.
 */
static void a_version_21_blob_gets_the_dx_defaults(void) {
  TEST_ASSERT_EQUAL_size_t(251, offsetof(Settings, dxStopRule));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));
  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 95000;
  uint8_t blob[V20_SIZE];
  memcpy(blob, &source, V20_SIZE);
  blob[251] = 0x55;
  uint16_t version = 21;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  uint16_t size = V20_SIZE;
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V20_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT32(95000, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(DX_STOP_NEW, out.dxStopRule);
  TEST_ASSERT_EQUAL_UINT8(DX_RANGE_BAND_LESS_MEMORY, out.dxScanRange);
  TEST_ASSERT_EQUAL_UINT8(1, out.dxMemFirst);
  TEST_ASSERT_EQUAL_UINT8(MEMORY_SLOT_COUNT, out.dxMemLast);
  TEST_ASSERT_EQUAL_UINT8(0, out.dxLoop);
  TEST_ASSERT_EQUAL_UINT8(1, out.dxScanMute);
  TEST_ASSERT_EQUAL_UINT8(1, out.dxAutoLog);
  TEST_ASSERT_EQUAL_UINT16(25, out.dxDwellTenths);
  TEST_ASSERT_EQUAL_UINT16(114, out.dxWidthKHz);
  TEST_ASSERT_TRUE(settingsValid(&out));
}

/* The region went into version 22's padding, so a version 22 blob is the
 * same length as version 23's, and the byte there must not be read as a
 * region. */
static void a_version_22_blob_gets_the_default_region(void) {
  TEST_ASSERT_EQUAL_size_t(262, offsetof(Settings, rdsRegion));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));
  Settings source;
  settingsDefaults(&source);
  source.startFreqKHz = 95000;
  uint8_t blob[V22_SIZE];
  memcpy(blob, &source, V22_SIZE);
  /* A value that would be a legal region if it leaked through. */
  blob[262] = RDS_REGION_NORTH_AMERICA;
  uint16_t version = 22;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  uint16_t size = V22_SIZE;
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V22_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT32(95000, out.startFreqKHz);
  TEST_ASSERT_EQUAL_UINT8(RDS_REGION_EUROPE, out.rdsRegion);
  TEST_ASSERT_TRUE(settingsValid(&out));
}

/* North America is not the default, so it proves the field is copied from a
 * version 23 blob rather than filled in afresh. */
static void a_version_23_blob_keeps_north_america(void) {
  Settings written;
  settingsDefaults(&written);
  written.rdsRegion = RDS_REGION_NORTH_AMERICA;
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(&written, sizeof(written), &out));
  TEST_ASSERT_EQUAL_UINT8(RDS_REGION_NORTH_AMERICA, out.rdsRegion);
}

/* Log Radio Text went into version 23's last byte of padding, so a version
 * 23 blob is the same 264 bytes as version 24's, and that byte must not be
 * read as the switch. The default is on, so the byte is written off to show
 * it. */
static void a_version_23_blob_gets_the_radio_text_logged(void) {
  TEST_ASSERT_EQUAL_size_t(263, offsetof(Settings, dxLogRt));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));
  Settings source;
  settingsDefaults(&source);
  source.rdsRegion = RDS_REGION_NORTH_AMERICA;
  uint8_t blob[V22_SIZE];
  memcpy(blob, &source, V22_SIZE);
  blob[263] = 0;
  uint16_t version = 23;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  uint16_t size = V22_SIZE;
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V22_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(RDS_REGION_NORTH_AMERICA, out.rdsRegion);
  TEST_ASSERT_EQUAL_UINT8(1, out.dxLogRt);
  TEST_ASSERT_TRUE(settingsValid(&out));
}

/* Off is not the default, so it proves the field is copied from a version
 * 24 blob. */
static void a_version_24_blob_keeps_the_radio_text_off(void) {
  Settings written;
  settingsDefaults(&written);
  written.dxLogRt = 0;
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(&written, sizeof(written), &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.dxLogRt);
}

/* Watch Presets starts just past version 24's 264 bytes, so a version 24
 * blob, which is 264 long, gets the default, on. */
static void a_version_24_blob_gets_the_watch_on(void) {
  TEST_ASSERT_EQUAL_size_t(264, offsetof(Settings, dxWatch));
  Settings source;
  settingsDefaults(&source);
  source.dxLogRt = 0;
  uint8_t blob[V22_SIZE];
  memcpy(blob, &source, V22_SIZE);
  uint16_t version = 24;
  uint16_t size = V22_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V22_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.dxLogRt);
  TEST_ASSERT_EQUAL_UINT8(1, out.dxWatch);
  TEST_ASSERT_TRUE(settingsValid(&out));
}

/* Off is not the default, so it proves the field is copied from a version
 * 25 blob. */
static void a_version_25_blob_keeps_the_watch_off(void) {
  Settings written;
  settingsDefaults(&written);
  written.dxWatch = 0;
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(&written, sizeof(written), &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.dxWatch);
}

/* The offsets went into version 25's padding, so a version 25 blob is as
 * long as a version 26 one, and those bytes must not be read as offsets. */
static void a_version_25_blob_gets_no_level_offset(void) {
  TEST_ASSERT_EQUAL_size_t(265, offsetof(Settings, levelOffsetFmDb));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));
  Settings source;
  settingsDefaults(&source);
  source.dxWatch = 0;
  uint8_t blob[V25_SIZE];
  memcpy(blob, &source, V25_SIZE);
  /* Values that would be legal offsets if they leaked through. */
  blob[265] = 5;
  blob[266] = (uint8_t)-7;
  uint16_t version = 25;
  uint16_t size = V25_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V25_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.dxWatch);
  TEST_ASSERT_EQUAL_INT8(0, out.levelOffsetFmDb);
  TEST_ASSERT_EQUAL_INT8(0, out.levelOffsetAmDb);
  TEST_ASSERT_TRUE(settingsValid(&out));
}

/* Not the default, and negative, so it proves the fields are copied from a
 * version 26 blob whole. */
static void a_version_26_blob_keeps_its_offsets(void) {
  Settings written;
  settingsDefaults(&written);
  written.version = 26;
  written.size = V25_SIZE;
  written.levelOffsetFmDb = -12;
  written.levelOffsetAmDb = 9;
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(&written, V25_SIZE, &out));
  TEST_ASSERT_EQUAL_INT8(-12, out.levelOffsetFmDb);
  TEST_ASSERT_EQUAL_INT8(9, out.levelOffsetAmDb);
}

/* The night theme went into version 26's last padding byte, so a version 26
 * blob is as long as a version 27 one. That byte is not a theme: the one
 * theme it had is kept by night as well as by day. */
static void a_version_26_blob_keeps_its_theme_at_night(void) {
  TEST_ASSERT_EQUAL_size_t(267, offsetof(Settings, nightTheme));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));
  Settings source;
  settingsDefaults(&source);
  source.theme = 10;
  uint8_t blob[V25_SIZE];
  memcpy(blob, &source, V25_SIZE);
  blob[267] = 3;
  uint16_t version = 26;
  uint16_t size = V25_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V25_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(10, out.theme);
  TEST_ASSERT_EQUAL_UINT8(10, out.nightTheme);
}

/* A night theme of its own, not the day one, is copied back as it was. */
static void a_version_27_blob_keeps_its_night_theme(void) {
  Settings written;
  settingsDefaults(&written);
  TEST_ASSERT_EQUAL_UINT8(0, written.nightTheme);
  written.theme = 10;
  written.nightTheme = 2;
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(&written, sizeof(written), &out));
  TEST_ASSERT_EQUAL_UINT8(10, out.theme);
  TEST_ASSERT_EQUAL_UINT8(2, out.nightTheme);
}

/* A version 27 blob is 268 bytes and has no hotspot byte, so the hotspot
 * keeps the default, Auto, as every radio before it behaved. */
static void a_version_27_blob_gets_the_hotspot_on_auto(void) {
  TEST_ASSERT_EQUAL_size_t(268, offsetof(Settings, hotspot));
  Settings source;
  settingsDefaults(&source);
  source.nightTheme = 4;
  uint8_t blob[V25_SIZE];
  memcpy(blob, &source, V25_SIZE);
  uint16_t version = 27;
  uint16_t size = V25_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V25_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(4, out.nightTheme);
  TEST_ASSERT_EQUAL_UINT8(WIFI_HOTSPOT_AUTO, out.hotspot);
}

/* The two switches went into version 28's padding, so a version 28 blob is
 * as long as version 29's, and those bytes must not be read as switched off:
 * both come on. */
static void a_version_28_blob_gets_wifi_and_the_web_server_on(void) {
  TEST_ASSERT_EQUAL_size_t(269, offsetof(Settings, webEnabled));
  TEST_ASSERT_EQUAL_size_t(270, offsetof(Settings, wifiEnabled));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));
  Settings source;
  settingsDefaults(&source);
  source.hotspot = WIFI_HOTSPOT_OFF;
  uint8_t blob[V28_SIZE];
  memcpy(blob, &source, V28_SIZE);
  blob[269] = 0;
  blob[270] = 0;
  uint16_t version = 28;
  uint16_t size = V28_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V28_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(WIFI_HOTSPOT_OFF, out.hotspot);
  TEST_ASSERT_EQUAL_UINT8(1, out.webEnabled);
  TEST_ASSERT_EQUAL_UINT8(1, out.wifiEnabled);
}

/* Version 30's auto off byte went into version 29's last byte of padding, so
 * a version 29 blob is as long as version 30's, and that byte must not be
 * read as a time: it comes up off, whatever the padding held. */
static void a_version_29_blob_gets_auto_off_off(void) {
  TEST_ASSERT_EQUAL_size_t(271, offsetof(Settings, autoOffMinutesV30));
  Settings source;
  settingsDefaults(&source);
  source.wifiEnabled = 0;
  uint8_t blob[V28_SIZE];
  memcpy(blob, &source, V28_SIZE);
  blob[271] = 30;
  uint16_t version = 29;
  uint16_t size = V28_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V28_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.wifiEnabled);
  TEST_ASSERT_EQUAL_UINT16(0, out.autoOffMinutes);
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
}

/* Version 30 kept auto off in one byte at 271. It comes across into the
 * two byte field version 31 has at 272. */
static void a_version_30_blob_keeps_its_auto_off_time(void) {
  TEST_ASSERT_EQUAL_size_t(272, offsetof(Settings, autoOffMinutes));
  Settings source;
  settingsDefaults(&source);
  uint8_t blob[V28_SIZE];
  memcpy(blob, &source, V28_SIZE);
  blob[271] = 15;
  uint16_t version = 30;
  uint16_t size = V28_SIZE;
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V28_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT16(15, out.autoOffMinutes);
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
  TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(Settings), out.size);
}

/* The update check sits in version 31's padding with the version left at
 * 31. A blob written before it holds 0 there, since every struct starts
 * zeroed, and a stray byte reads as off rather than costing the struct. */
static void the_update_check_in_version_31_padding_reads_as_off(void) {
  TEST_ASSERT_EQUAL_size_t(274, offsetof(Settings, updateCheck));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));
  Settings source;
  settingsDefaults(&source);
  source.autoOffMinutes = 45;
  uint8_t blob[V31_SIZE];
  memcpy(blob, &source, V31_SIZE);
  stampV31(blob);
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V31_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT16(45, out.autoOffMinutes);
  TEST_ASSERT_EQUAL_UINT8(0, out.updateCheck);
  blob[274] = 1;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V31_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(1, out.updateCheck);
  blob[274] = 7;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V31_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.updateCheck);
  TEST_ASSERT_EQUAL_UINT16(45, out.autoOffMinutes);
}

/* The touch switch sits in version 31's padding with the version left at
 * 31, kept as off so a blob written before it, 0 there, reads as touch on;
 * 1 is off, and a stray byte reads as on rather than costing the struct. */
static void the_touch_switch_in_version_31_padding_reads_as_on(void) {
  TEST_ASSERT_EQUAL_size_t(275, offsetof(Settings, touchOff));
  TEST_ASSERT_EQUAL_size_t(V32_SIZE, sizeof(Settings));
  Settings source;
  settingsDefaults(&source);
  source.updateCheck = 1;
  uint8_t blob[V31_SIZE];
  memcpy(blob, &source, V31_SIZE);
  stampV31(blob);
  blob[275] = 0;
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V31_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.touchOff);
  blob[275] = 1;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V31_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(1, out.touchOff);
  blob[275] = 9;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V31_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.touchOff);
  TEST_ASSERT_EQUAL_UINT8(1, out.updateCheck);
}

/* A newer firmware's blob keeps touch off, and a stray byte there reads as
 * on rather than refusing every setting. */
static void touch_off_survives_a_newer_blob(void) {
  Settings source;
  settingsDefaults(&source);
  source.touchOff = 1;
  uint8_t blob[V32_SIZE + 8];
  memset(blob, 0, sizeof(blob));
  memcpy(blob, &source, V32_SIZE);
  uint16_t version = SETTINGS_VERSION + 1;
  uint16_t size = (uint16_t)sizeof(blob);
  memcpy(blob + offsetof(Settings, version), &version, sizeof(version));
  memcpy(blob + offsetof(Settings, size), &size, sizeof(size));
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, sizeof(blob), &out));
  TEST_ASSERT_EQUAL_UINT8(1, out.touchOff);
  blob[275] = 9;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, sizeof(blob), &out));
  TEST_ASSERT_EQUAL_UINT8(0, out.touchOff);
}

/* A version 31 blob has no keypad timeout, so it gets the 20 s a new radio
 * starts with, and the rest of it is kept. */
static void a_version_31_blob_gets_the_default_keypad_timeout(void) {
  TEST_ASSERT_EQUAL_size_t(V31_SIZE, offsetof(Settings, keypadTimeoutS));
  TEST_ASSERT_EQUAL_UINT16(32, SETTINGS_VERSION);
  Settings source;
  settingsDefaults(&source);
  source.touchOff = 1;
  source.keypadTimeoutS = 45;
  uint8_t blob[V31_SIZE];
  memcpy(blob, &source, V31_SIZE);
  stampV31(blob);
  Settings out;
  TEST_ASSERT_TRUE(settingsFromBlob(blob, V31_SIZE, &out));
  TEST_ASSERT_EQUAL_UINT8(20, out.keypadTimeoutS);
  TEST_ASSERT_EQUAL_UINT8(1, out.touchOff);
  TEST_ASSERT_EQUAL_UINT16(SETTINGS_VERSION, out.version);
  TEST_ASSERT_EQUAL_UINT16((uint16_t)sizeof(Settings), out.size);
}

/* 20 s on a new radio, 5 to 60 kept, and one past either end refused. */
static void the_keypad_timeout_is_5_to_60_seconds(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_EQUAL_UINT8(20, s.keypadTimeoutS);
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.keypadTimeoutS = 5;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.keypadTimeoutS = 60;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.keypadTimeoutS = 4;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.keypadTimeoutS = 61;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.keypadTimeoutS = 0;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

/* On on a new radio, and only 0 or 1 stored. */
static void touch_is_on_and_takes_only_0_or_1(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_EQUAL_UINT8(0, s.touchOff);
  s.touchOff = 1;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.touchOff = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

/* Off on a new radio, and only 0 or 1. */
static void the_update_check_is_off_and_takes_only_0_or_1(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_EQUAL_UINT8(0, s.updateCheck);
  s.updateCheck = 1;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.updateCheck = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

/* Up to ten hours, and not a minute more. */
static void an_auto_off_time_nobody_can_choose_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_EQUAL_UINT16(0, s.autoOffMinutes);
  s.autoOffMinutes = 600;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.autoOffMinutes = 601;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

/* 0 and 1 only. */
static void a_switch_past_one_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  s.webEnabled = 0;
  s.wifiEnabled = 0;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.webEnabled = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.webEnabled = 1;
  s.wifiEnabled = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

/* Off, the last, is accepted, and one past it is not. */
static void a_hotspot_past_off_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  s.hotspot = WIFI_HOTSPOT_OFF;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.hotspot = WIFI_HOTSPOT_COUNT;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

/* New network details with the hotspot On put it back to Auto, so they are
 * joined; Off stays Off. */
static void new_network_details_turn_a_hotspot_on_back_to_auto(void) {
  Settings s;
  settingsDefaults(&s);
  s.hotspot = WIFI_HOTSPOT_ON;
  TEST_ASSERT_TRUE(settingsSetWifi(&s, "Home", "secret12"));
  TEST_ASSERT_EQUAL_UINT8(WIFI_HOTSPOT_AUTO, s.hotspot);
  s.hotspot = WIFI_HOTSPOT_OFF;
  TEST_ASSERT_TRUE(settingsSetWifi(&s, "Home", "secret12"));
  TEST_ASSERT_EQUAL_UINT8(WIFI_HOTSPOT_OFF, s.hotspot);
  s.hotspot = WIFI_HOTSPOT_ON;
  TEST_ASSERT_FALSE(settingsSetWifi(&s, NULL, "x"));
  TEST_ASSERT_EQUAL_UINT8(WIFI_HOTSPOT_ON, s.hotspot);
}

/* Each on its edge and one past it. */
static void a_level_offset_outside_its_range_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  s.levelOffsetFmDb = SIGNAL_LEVEL_OFFSET_MIN_DB;
  s.levelOffsetAmDb = SIGNAL_LEVEL_OFFSET_MAX_DB;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.levelOffsetFmDb = SIGNAL_LEVEL_OFFSET_MIN_DB - 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.levelOffsetFmDb = 0;
  s.levelOffsetAmDb = SIGNAL_LEVEL_OFFSET_MAX_DB + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_watch_switch_past_on_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_EQUAL_UINT8(1, s.dxWatch);
  s.dxWatch = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_radio_text_switch_past_on_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_EQUAL_UINT8(1, s.dxLogRt);
  s.dxLogRt = 0;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.dxLogRt = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void a_region_outside_the_list_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_EQUAL_UINT8(RDS_REGION_EUROPE, s.rdsRegion);
  s.rdsRegion = RDS_REGION_NORTH_AMERICA;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.rdsRegion = RDS_REGION_COUNT;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

/* Each DX SETUP field on its edge, one past it, and the width only as one
 * of the tuner's FM widths. */
static void a_dx_setting_outside_its_range_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.dxDwellTenths = DX_SCAN_DWELL_MIN_TENTHS;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.dxDwellTenths = DX_SCAN_DWELL_MIN_TENTHS - 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.dxDwellTenths = DX_SCAN_DWELL_MAX_TENTHS;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.dxDwellTenths = DX_SCAN_DWELL_MAX_TENTHS + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
  settingsDefaults(&s);
  s.dxStopRule = DX_STOP_NEVER;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.dxStopRule = DX_STOP_COUNT;
  TEST_ASSERT_FALSE(settingsValid(&s));
  settingsDefaults(&s);
  s.dxScanRange = DX_RANGE_MEMORY;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.dxScanRange = DX_RANGE_COUNT;
  TEST_ASSERT_FALSE(settingsValid(&s));
  settingsDefaults(&s);
  s.dxMemFirst = 0;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.dxMemFirst = 50;
  s.dxMemLast = 49;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.dxMemLast = 50;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.dxMemLast = MEMORY_SLOT_COUNT + 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
  settingsDefaults(&s);
  s.dxLoop = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
  settingsDefaults(&s);
  s.dxScanMute = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
  settingsDefaults(&s);
  s.dxAutoLog = 2;
  TEST_ASSERT_FALSE(settingsValid(&s));
  settingsDefaults(&s);
  s.dxWidthKHz = 56;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.dxWidthKHz = 85;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.dxWidthKHz = 0;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

static void an_am_start_level_outside_its_range_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.amHighCutStart = 0;
  s.lwHighCutStart = 60;
  s.amSoftMuteStart = 0;
  s.lwSoftMuteStart = 50;
  TEST_ASSERT_TRUE(settingsValid(&s));

  settingsDefaults(&s);
  s.amHighCutStart = 19;
  TEST_ASSERT_FALSE(settingsValid(&s));
  settingsDefaults(&s);
  s.lwHighCutStart = 61;
  TEST_ASSERT_FALSE(settingsValid(&s));
  settingsDefaults(&s);
  s.amSoftMuteStart = 51;
  TEST_ASSERT_FALSE(settingsValid(&s));
  settingsDefaults(&s);
  s.lwSoftMuteStart = 51;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

/* The only two real rotations; anything else is a mirror image, not a
 * setting a person would ever mean to store. */
static void a_display_rotation_outside_the_two_real_ones_is_refused(void) {
  Settings s;
  settingsDefaults(&s);
  s.displayRotation = 0;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.displayRotation = 180;
  TEST_ASSERT_TRUE(settingsValid(&s));
  s.displayRotation = 90;
  TEST_ASSERT_FALSE(settingsValid(&s));
  s.displayRotation = 1;
  TEST_ASSERT_FALSE(settingsValid(&s));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(defaults_are_valid_and_have_no_wifi);
  RUN_TEST(settings_survive_a_round_trip_through_bytes);
  RUN_TEST(a_blob_from_a_newer_firmware_is_read_up_to_the_known_fields);
  RUN_TEST(a_newer_blob_of_the_same_length_is_read_too);
  RUN_TEST(a_newer_blob_that_is_short_or_mislabelled_falls_back);
  RUN_TEST(a_newer_blob_holding_a_value_this_firmware_refuses_falls_back);
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
  RUN_TEST(a_value_outside_its_table_range_is_rejected);
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
  RUN_TEST(a_version_10_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(a_version_11_blob_gets_the_defaults_for_what_it_never_had);
  RUN_TEST(a_version_14_blob_does_not_read_its_padding_as_a_meter);
  RUN_TEST(a_version_14_blob_with_dirty_padding_is_still_clean);
  RUN_TEST(a_meter_shape_outside_the_range_is_refused);
  RUN_TEST(a_version_12_blob_keeps_the_mode_of_the_band_it_was_on);
  RUN_TEST(a_version_12_blob_with_a_mode_outside_the_enum_falls_back);
  RUN_TEST(the_battery_ships_switched_off);
  RUN_TEST(network_time_is_on_by_default_and_the_offset_is_utc);
  RUN_TEST(a_utc_offset_no_place_uses_is_refused);
  RUN_TEST(the_rds_decoder_is_on_by_default);
  RUN_TEST(the_squelch_floor_has_a_range);
  RUN_TEST(a_stored_tuning_mode_outside_the_enum_is_refused);
  RUN_TEST(the_panel_light_settings_have_ranges);
  RUN_TEST(the_soft_mute_and_beep_settings_have_ranges);
  RUN_TEST(a_pot_calibration_is_judged_on_the_loud_end);
  RUN_TEST(a_scan_sensitivity_outside_the_range_is_refused);
  RUN_TEST(a_blend_start_is_off_or_somewhere_a_signal_reaches);
  RUN_TEST(a_noise_blanker_is_a_percentage);
  RUN_TEST(only_the_widths_the_am_side_has_are_accepted);
  RUN_TEST(the_stored_volume_stays_inside_what_the_chip_takes);
  RUN_TEST(a_deemphasis_that_is_not_one_of_the_two_is_refused);
  RUN_TEST(the_tuner_and_start_settings_survive_a_round_trip);

  RUN_TEST(a_version_13_blob_comes_back_with_the_agc_off);
  RUN_TEST(an_agc_target_is_zero_or_a_real_one);

  RUN_TEST(a_version_15_blob_is_not_read_past_its_end);

  RUN_TEST(a_version_16_blob_does_not_read_its_padding_as_a_theme);
  RUN_TEST(a_version_17_blob_does_not_carry_over_its_theme);
  RUN_TEST(a_version_18_blob_does_not_read_its_padding_as_a_rotation);
  RUN_TEST(a_display_rotation_outside_the_two_real_ones_is_refused);
  RUN_TEST(a_version_19_blob_gets_the_am_defaults_and_the_blanker_on);
  RUN_TEST(a_version_20_blanker_that_is_off_stays_off);
  RUN_TEST(a_version_20_theme_becomes_nightwatch_and_keeps_custom);
  RUN_TEST(a_version_21_theme_is_kept);
  RUN_TEST(a_version_21_blob_gets_the_dx_defaults);
  RUN_TEST(a_dx_setting_outside_its_range_is_refused);
  RUN_TEST(a_version_22_blob_gets_the_default_region);
  RUN_TEST(a_version_23_blob_keeps_north_america);
  RUN_TEST(a_region_outside_the_list_is_refused);
  RUN_TEST(a_version_23_blob_gets_the_radio_text_logged);
  RUN_TEST(a_version_24_blob_keeps_the_radio_text_off);
  RUN_TEST(a_radio_text_switch_past_on_is_refused);
  RUN_TEST(a_version_24_blob_gets_the_watch_on);
  RUN_TEST(a_version_25_blob_keeps_the_watch_off);
  RUN_TEST(a_watch_switch_past_on_is_refused);
  RUN_TEST(a_version_25_blob_gets_no_level_offset);
  RUN_TEST(a_version_26_blob_keeps_its_offsets);
  RUN_TEST(a_version_26_blob_keeps_its_theme_at_night);
  RUN_TEST(a_version_27_blob_keeps_its_night_theme);
  RUN_TEST(a_version_27_blob_gets_the_hotspot_on_auto);
  RUN_TEST(a_hotspot_past_off_is_refused);
  RUN_TEST(a_version_28_blob_gets_wifi_and_the_web_server_on);
  RUN_TEST(a_version_29_blob_gets_auto_off_off);
  RUN_TEST(a_version_30_blob_keeps_its_auto_off_time);
  RUN_TEST(the_update_check_in_version_31_padding_reads_as_off);
  RUN_TEST(the_update_check_is_off_and_takes_only_0_or_1);
  RUN_TEST(the_touch_switch_in_version_31_padding_reads_as_on);
  RUN_TEST(touch_off_survives_a_newer_blob);
  RUN_TEST(touch_is_on_and_takes_only_0_or_1);
  RUN_TEST(a_version_31_blob_gets_the_default_keypad_timeout);
  RUN_TEST(the_keypad_timeout_is_5_to_60_seconds);
  RUN_TEST(an_auto_off_time_nobody_can_choose_is_refused);
  RUN_TEST(a_switch_past_one_is_refused);
  RUN_TEST(new_network_details_turn_a_hotspot_on_back_to_auto);
  RUN_TEST(a_level_offset_outside_its_range_is_refused);
  RUN_TEST(an_am_start_level_outside_its_range_is_refused);

  return UNITY_END();
}
