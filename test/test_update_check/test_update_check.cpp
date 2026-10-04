/* Tests for the update check: the manifest, versions, addresses and when to
 * look. Runs on a PC. */
#include <unity.h>

#include "core/update_check.h"

#include <stdio.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

#define SLOT 3145728u
#define SHA "31c2b0a2c23a06f50eef8fdd11b5fae99ded4a30597693972e0e114242083d0c"
#define URL                                                          \
  "https://github.com/zeevy/tef668x-esp32/releases/download/v0.2.0/" \
  "firmware-ats125.bin"

/* A manifest as the release workflow writes it, with one field swapped. */
static char sJson[1024];
static const char *manifest(const char *version, const char *board,
                            const char *url, const char *size,
                            const char *sha) {
  snprintf(sJson, sizeof(sJson),
           "{\n  \"version\": \"%s\",\n  \"board\": \"%s\",\n  \"url\": "
           "\"%s\",\n  \"size\": %s,\n  \"sha256\": \"%s\"\n}\n",
           version, board, url, size, sha);
  return sJson;
}

static UpdateManifestResult parse(const char *json, UpdateManifest *out) {
  return updateParseManifest(json, strlen(json), "ats125", SLOT, out);
}

static UpdateManifestResult parseGood(const char *version, const char *board,
                                      const char *url, const char *size,
                                      const char *sha) {
  UpdateManifest m;
  return parse(manifest(version, board, url, size, sha), &m);
}

/* ------------------------------------------------------------- versions */

static void versions_read_as_three_numbers(void) {
  UpdateVersion v;
  TEST_ASSERT_TRUE(updateParseVersion("0.2.0", &v));
  TEST_ASSERT_EQUAL_UINT16(0, v.major);
  TEST_ASSERT_EQUAL_UINT16(2, v.minor);
  TEST_ASSERT_EQUAL_UINT16(0, v.patch);
  TEST_ASSERT_TRUE(updateParseVersion("65535.65535.65535", &v));
  TEST_ASSERT_EQUAL_UINT16(65535, v.patch);
  TEST_ASSERT_TRUE(updateParseVersion("10.0.7", &v));
  TEST_ASSERT_EQUAL_UINT16(10, v.major);
}

static void anything_but_x_y_z_is_not_a_version(void) {
  UpdateVersion v = {9, 9, 9};
  const char *bad[] = {"",       "1",      "1.2",    "1.2.3.4",    "a.b.c",
                       "1.2.c",  "1..3",   ".1.2",   "1.2.",       "1.2.65536",
                       "01.2.3", "1.02.3", "1.2.03", "123456.0.0", "-1.2.3",
                       "1.2.3 ", " 1.2.3", "v1.2.3", "1.2.3-rc.1"};
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
    TEST_ASSERT_FALSE_MESSAGE(updateParseVersion(bad[i], &v), bad[i]);
  }
  /* A refused version leaves the output as it was. */
  TEST_ASSERT_EQUAL_UINT16(9, v.major);
  TEST_ASSERT_FALSE(updateParseVersion(NULL, &v));
  TEST_ASSERT_FALSE(updateParseVersion("1.2.3", NULL));
}

static void versions_compare_part_by_part_as_numbers(void) {
  UpdateVersion a, b;
  updateParseVersion("0.10.0", &a);
  updateParseVersion("0.9.0", &b);
  TEST_ASSERT_TRUE(updateCompareVersions(&a, &b) > 0);
  TEST_ASSERT_TRUE(updateCompareVersions(&b, &a) < 0);
  updateParseVersion("0.2.0", &a);
  updateParseVersion("0.2.0", &b);
  TEST_ASSERT_EQUAL_INT(0, updateCompareVersions(&a, &b));
  updateParseVersion("0.2.1", &a);
  TEST_ASSERT_TRUE(updateCompareVersions(&a, &b) > 0);
  updateParseVersion("1.0.0", &a);
  updateParseVersion("0.99.99", &b);
  TEST_ASSERT_TRUE(updateCompareVersions(&a, &b) > 0);
}

/* ------------------------------------------------------------- manifests */

static void a_good_manifest_is_taken(void) {
  UpdateManifest m;
  TEST_ASSERT_EQUAL_INT(
      UPDATE_MANIFEST_OK,
      parse(manifest("0.2.0", "ats125", URL, "1714848", SHA), &m));
  TEST_ASSERT_EQUAL_STRING("0.2.0", m.versionText);
  TEST_ASSERT_EQUAL_UINT16(2, m.version.minor);
  TEST_ASSERT_EQUAL_STRING(URL, m.url);
  TEST_ASSERT_EQUAL_UINT32(1714848, m.size);
  TEST_ASSERT_EQUAL_STRING(SHA, m.sha256);
}

static void the_manifest_can_be_on_one_line_in_any_order(void) {
  const char *json = "{\"size\":1714848,\"sha256\":\"" SHA "\",\"url\":\"" URL
                     "\",\"board\":\"ats125\",\"version\":\"0.2.0\"}";
  UpdateManifest m;
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_OK, parse(json, &m));
  TEST_ASSERT_EQUAL_UINT32(1714848, m.size);
}

static void an_upper_case_sha256_is_kept_in_lower_case(void) {
  char upper[65];
  for (size_t i = 0; i < 64; i++) {
    const char ch = SHA[i];
    upper[i] = (ch >= 'a' && ch <= 'f') ? (char)(ch - 'a' + 'A') : ch;
  }
  upper[64] = '\0';
  UpdateManifest m;
  TEST_ASSERT_EQUAL_INT(
      UPDATE_MANIFEST_OK,
      parse(manifest("0.2.0", "ats125", URL, "1714848", upper), &m));
  TEST_ASSERT_EQUAL_STRING(SHA, m.sha256);
}

static void a_field_this_firmware_does_not_know_is_passed_over(void) {
  const char *json =
      "{\"version\":\"0.2.0\",\"board\":\"ats125\",\"url\":\"" URL
      "\",\"size\":1714848,\"sha256\":\"" SHA
      "\",\"signature\":\"abc\",\"released\":1791130892,"
      "\"a_key_much_longer_than_sixteen\":\"x\"}";
  UpdateManifest m;
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_OK, parse(json, &m));
}

static void a_bad_version_is_refused(void) {
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_BAD_VERSION,
                        parseGood("0.2", "ats125", URL, "1714848", SHA));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_BAD_VERSION,
                        parseGood("v0.2.0", "ats125", URL, "1714848", SHA));
  TEST_ASSERT_EQUAL_INT(
      UPDATE_MANIFEST_BAD_VERSION,
      parseGood("0.2.0.0.0.0.0.0.0.0.0.0", "ats125", URL, "1714848", SHA));
}

static void a_manifest_for_another_radio_is_refused(void) {
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_WRONG_BOARD,
                        parseGood("0.2.0", "other", URL, "1714848", SHA));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_WRONG_BOARD,
                        parseGood("0.2.0", "ats1250", URL, "1714848", SHA));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_WRONG_BOARD,
                        parseGood("0.2.0", "", URL, "1714848", SHA));
}

static void a_size_of_nothing_or_more_than_the_slot_is_refused(void) {
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_BAD_SIZE,
                        parseGood("0.2.0", "ats125", URL, "0", SHA));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_BAD_SIZE,
                        parseGood("0.2.0", "ats125", URL, "3145729", SHA));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_OK,
                        parseGood("0.2.0", "ats125", URL, "3145728", SHA));
}

static void a_size_that_is_not_a_plain_whole_number_is_refused(void) {
  const char *bad[] = {"-1",         "1.5",   "1e6",  "01714848",
                       "4294967296", "\"1\"", "true", "null"};
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        UPDATE_MANIFEST_NOT_JSON,
        parseGood("0.2.0", "ats125", URL, bad[i], SHA), bad[i]);
  }
}

static void a_sha256_that_is_not_64_hex_digits_is_refused(void) {
  char shortSha[64], longSha[66], notHex[65];
  memcpy(shortSha, SHA, 63);
  shortSha[63] = '\0';
  memcpy(longSha, SHA, 64);
  longSha[64] = '0';
  longSha[65] = '\0';
  memcpy(notHex, SHA, 65);
  notHex[10] = 'g';
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_BAD_SHA256,
                        parseGood("0.2.0", "ats125", URL, "1714848", shortSha));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_BAD_SHA256,
                        parseGood("0.2.0", "ats125", URL, "1714848", longSha));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_BAD_SHA256,
                        parseGood("0.2.0", "ats125", URL, "1714848", notHex));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_BAD_SHA256,
                        parseGood("0.2.0", "ats125", URL, "1714848", ""));
}

static void a_url_outside_this_repositorys_release_file_is_refused(void) {
  const char *bad[] = {
      "http://github.com/zeevy/tef668x-esp32/releases/download/v0.2.0/"
      "firmware-ats125.bin",
      "https://github.com/someone/tef668x-esp32/releases/download/v0.2.0/"
      "firmware-ats125.bin",
      "https://example.com/zeevy/tef668x-esp32/releases/download/v0.2.0/"
      "firmware-ats125.bin",
      "https://github.com/zeevy/tef668x-esp32/releases/download/v0.2.0/"
      "firmware-other.bin",
      "https://github.com/zeevy/tef668x-esp32/releases/download/v0.2.0/"
      "firmware-ats125.bin.exe",
      "https://github.com/zeevy/tef668x-esp32/releases/download/../../../x/"
      "firmware-ats125.bin",
      "https://github.com/zeevy/tef668x-esp32/releases/download/a..b/"
      "firmware-ats125.bin",
      "https://github.com/zeevy/tef668x-esp32/releases/download/.hidden/"
      "firmware-ats125.bin",
      "https://github.com/zeevy/tef668x-esp32/releases/download/"
      "firmware-ats125.bin",
      "https://github.com/zeevy/tef668x-esp32/releases/download/v0.2.0/x/"
      "firmware-ats125.bin",
      "https://github.com/zeevy/tef668x-esp32/releases/download/"
      "v0123456789012345678901234567890123456789/firmware-ats125.bin",
      "https://github.com/zeevy/tef668x-esp32/releases/download/v0.2.0/"
      "firmware-ats125.bin?x=1",
  };
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
        UPDATE_MANIFEST_BAD_URL,
        parseGood("0.2.0", "ats125", bad[i], "1714848", SHA), bad[i]);
  }
  /* A prerelease tag is fine. */
  TEST_ASSERT_EQUAL_INT(
      UPDATE_MANIFEST_OK,
      parseGood("0.2.0", "ats125",
                "https://github.com/zeevy/tef668x-esp32/releases/download/"
                "v0.2.0-rc.1/firmware-ats125.bin",
                "1714848", SHA));
}

static void a_url_too_long_for_the_buffer_is_refused(void) {
  char url[400];
  snprintf(url, sizeof(url),
           "https://github.com/zeevy/tef668x-esp32/releases/download/"
           "%0200d/firmware-ats125.bin",
           0);
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_BAD_URL,
                        parseGood("0.2.0", "ats125", url, "1714848", SHA));
}

static void a_missing_field_is_refused(void) {
  const char *json =
      "{\"version\":\"0.2.0\",\"board\":\"ats125\",\"url\":\"" URL
      "\",\"size\":1714848}";
  UpdateManifest m;
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_MISSING_FIELD, parse(json, &m));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_MISSING_FIELD, parse("{}", &m));
}

static void a_repeated_field_is_refused(void) {
  const char *json =
      "{\"version\":\"0.2.0\",\"version\":\"9.9.9\",\"board\":\"ats125\","
      "\"url\":\"" URL "\",\"size\":1714848,\"sha256\":\"" SHA "\"}";
  UpdateManifest m;
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_REPEATED_FIELD, parse(json, &m));
  const char *size = "{\"size\":1,\"size\":1714848}";
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_REPEATED_FIELD, parse(size, &m));
}

static void anything_but_one_flat_object_is_refused(void) {
  const char *bad[] = {
      "[]",
      "\"0.2.0\"",
      "{\"version\":\"0.2.0\"",
      "{\"version\":\"0.2.0\"}}",
      "{\"version\":\"0.2.0\"} x",
      "{\"version\" \"0.2.0\"}",
      "{\"version\":\"0.2.0\",}",
      "{\"version\":\"0\\u002e2.0\"}",
      "{\"version\":\"0.2.0\nx\"}",
      "{\"nested\":{\"a\":1}}",
      "{\"list\":[1,2]}",
      "{version:\"0.2.0\"}",
  };
  UpdateManifest m;
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(UPDATE_MANIFEST_NOT_JSON, parse(bad[i], &m),
                                  bad[i]);
  }
}

static void a_body_that_is_empty_or_too_long_is_not_read(void) {
  UpdateManifest m;
  char big[UPDATE_MANIFEST_MAX_BYTES + 2];
  memset(big, ' ', sizeof(big));
  big[sizeof(big) - 1] = '\0';
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_TOO_LONG, parse(big, &m));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_TOO_LONG,
                        updateParseManifest("", 0, "ats125", SLOT, &m));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_TOO_LONG,
                        updateParseManifest(NULL, 10, "ats125", SLOT, &m));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_TOO_LONG,
                        updateParseManifest("{}", 2, NULL, SLOT, &m));
  TEST_ASSERT_EQUAL_INT(UPDATE_MANIFEST_TOO_LONG,
                        updateParseManifest("{}", 2, "ats125", SLOT, NULL));
}

static void a_refused_manifest_leaves_the_output_alone(void) {
  UpdateManifest m;
  memset(&m, 0x5A, sizeof(m));
  UpdateManifest before = m;
  parse(manifest("0.2.0", "other", URL, "1714848", SHA), &m);
  TEST_ASSERT_EQUAL_MEMORY(&before, &m, sizeof(m));
}

/* ------------------------------------------------------------- addresses */

static void downloads_go_only_to_the_repository_or_githubs_file_hosts(void) {
  TEST_ASSERT_TRUE(updateUrlAllowed(UPDATE_MANIFEST_URL_PREFIX "ats125.json"));
  TEST_ASSERT_TRUE(updateUrlAllowed(URL));
  TEST_ASSERT_TRUE(updateUrlAllowed(
      "https://release-assets.githubusercontent.com/github-production-release-"
      "asset/23736914/"
      "2ca3a158?sp=r&sv=2018-11-09&se=2026-10-04T17%3A50%3A44Z"));
  TEST_ASSERT_TRUE(
      updateUrlAllowed("https://objects.githubusercontent.com/github-"
                       "production-release-asset-2e65be/1"));
}

static void every_other_address_is_refused(void) {
  const char *bad[] = {
      "",
      "http://github.com/zeevy/tef668x-esp32/releases/latest/download/x",
      "https://github.com/zeevy/tef668x-esp32/archive/main.zip",
      "https://github.com/zeevy/tef668x-esp32-other/releases/download/x",
      "https://github.com/zeevy/tef668x-esp32/releases/../../evil/x",
      "https://github.com/zeevy/tef668x-esp32/releases/%2e%2e/x",
      "https://github.com.evil.example/zeevy/tef668x-esp32/releases/x",
      "https://release-assets.githubusercontent.com.evil.example/x",
      "https://evil.example/https://release-assets.githubusercontent.com/",
      "https://release-assets.githubusercontent.com/a b",
      "https://release-assets.githubusercontent.com/a\tb",
      "https://release-assets.githubusercontent.com\\@evil.example/",
      "ftp://release-assets.githubusercontent.com/x",
  };
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
    TEST_ASSERT_FALSE_MESSAGE(updateUrlAllowed(bad[i]), bad[i]);
  }
  TEST_ASSERT_FALSE(updateUrlAllowed(NULL));
  char longUrl[2100];
  memset(longUrl, 'a', sizeof(longUrl));
  memcpy(longUrl, "https://objects.githubusercontent.com/", 38);
  longUrl[sizeof(longUrl) - 1] = '\0';
  TEST_ASSERT_FALSE(updateUrlAllowed(longUrl));
}

/* ------------------------------------------------------------- when */

static void with_the_setting_off_it_never_checks(void) {
  UpdateDue due;
  updateDueReset(&due);
  UpdateDueInputs in = {false, true, false, false};
  for (int i = 0; i < 5; i++) {
    TEST_ASSERT_FALSE(updateDueStep(&due, &in));
  }
}

static void it_waits_for_the_network_the_trial_and_a_quiet_radio(void) {
  UpdateDue due;
  updateDueReset(&due);
  UpdateDueInputs in = {true, false, false, false};
  TEST_ASSERT_FALSE(updateDueStep(&due, &in));
  in.online = true;
  in.onTrial = true;
  TEST_ASSERT_FALSE(updateDueStep(&due, &in));
  in.onTrial = false;
  in.busy = true;
  TEST_ASSERT_FALSE(updateDueStep(&due, &in));
  in.busy = false;
  TEST_ASSERT_TRUE(updateDueStep(&due, &in));
}

static void it_checks_once_per_start(void) {
  UpdateDue due;
  updateDueReset(&due);
  UpdateDueInputs in = {true, true, false, false};
  TEST_ASSERT_TRUE(updateDueStep(&due, &in));
  TEST_ASSERT_FALSE(updateDueStep(&due, &in));
  TEST_ASSERT_FALSE(updateDueStep(&due, &in));
  updateDueReset(&due);
  TEST_ASSERT_TRUE(updateDueStep(&due, &in));
}

static void turning_the_setting_on_later_checks_in_the_same_start(void) {
  UpdateDue due;
  updateDueReset(&due);
  UpdateDueInputs in = {false, true, false, false};
  TEST_ASSERT_FALSE(updateDueStep(&due, &in));
  in.enabled = true;
  TEST_ASSERT_TRUE(updateDueStep(&due, &in));
}

static void the_due_rule_does_nothing_with_a_null(void) {
  UpdateDue due;
  updateDueReset(&due);
  updateDueReset(NULL);
  UpdateDueInputs in = {true, true, false, false};
  TEST_ASSERT_FALSE(updateDueStep(NULL, &in));
  TEST_ASSERT_FALSE(updateDueStep(&due, NULL));
}

/* ------------------------------------------------------------- the size */

static void sizes_show_in_megabytes_with_one_decimal(void) {
  char out[16];
  updateFormatMegabytes(1714848, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("1.7", out);
  updateFormatMegabytes(0, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("0.0", out);
  updateFormatMegabytes(49999, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("0.0", out);
  updateFormatMegabytes(50000, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("0.1", out);
  updateFormatMegabytes(999999, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("1.0", out);
  updateFormatMegabytes(4294967295u, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("4295.0", out);
}

static void a_size_that_does_not_fit_writes_nothing(void) {
  char out[3] = "xx";
  updateFormatMegabytes(1714848, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("", out);
  updateFormatMegabytes(1714848, NULL, 8);
  updateFormatMegabytes(1714848, out, 0);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(versions_read_as_three_numbers);
  RUN_TEST(anything_but_x_y_z_is_not_a_version);
  RUN_TEST(versions_compare_part_by_part_as_numbers);
  RUN_TEST(a_good_manifest_is_taken);
  RUN_TEST(the_manifest_can_be_on_one_line_in_any_order);
  RUN_TEST(an_upper_case_sha256_is_kept_in_lower_case);
  RUN_TEST(a_field_this_firmware_does_not_know_is_passed_over);
  RUN_TEST(a_bad_version_is_refused);
  RUN_TEST(a_manifest_for_another_radio_is_refused);
  RUN_TEST(a_size_of_nothing_or_more_than_the_slot_is_refused);
  RUN_TEST(a_size_that_is_not_a_plain_whole_number_is_refused);
  RUN_TEST(a_sha256_that_is_not_64_hex_digits_is_refused);
  RUN_TEST(a_url_outside_this_repositorys_release_file_is_refused);
  RUN_TEST(a_url_too_long_for_the_buffer_is_refused);
  RUN_TEST(a_missing_field_is_refused);
  RUN_TEST(a_repeated_field_is_refused);
  RUN_TEST(anything_but_one_flat_object_is_refused);
  RUN_TEST(a_body_that_is_empty_or_too_long_is_not_read);
  RUN_TEST(a_refused_manifest_leaves_the_output_alone);
  RUN_TEST(downloads_go_only_to_the_repository_or_githubs_file_hosts);
  RUN_TEST(every_other_address_is_refused);
  RUN_TEST(with_the_setting_off_it_never_checks);
  RUN_TEST(it_waits_for_the_network_the_trial_and_a_quiet_radio);
  RUN_TEST(it_checks_once_per_start);
  RUN_TEST(turning_the_setting_on_later_checks_in_the_same_start);
  RUN_TEST(the_due_rule_does_nothing_with_a_null);
  RUN_TEST(sizes_show_in_megabytes_with_one_decimal);
  RUN_TEST(a_size_that_does_not_fit_writes_nothing);
  return UNITY_END();
}
