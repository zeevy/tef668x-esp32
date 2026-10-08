/*
 * Tests for the state document the web server sends for /api/state and
 * /status.json. Runs on a PC.
 *
 * The document's own source is compiled here, with stand-ins next to this
 * file for the Arduino and ESP32 headers, and fixed answers below for every
 * task and driver it asks. Each test builds the document and reads it back
 * with a small JSON reader, so a document a browser cannot parse fails here.
 */
#include <unity.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

/* The board header refuses to build without a board, and this is the board
 * the document describes. */
#define BOARD_ATS125 1

#include "../../src/net/web_state.cpp"

/* ------------------------------------------------------- the fixed radio */

/* Station text with a quote, a backslash, a tab and a 0x01 in it, each of
 * which has to be escaped for the document to parse. The name is the eight
 * characters RDS sends. */
static const char kPs[] = "MY\"FM\\\t\x01";
static const char kRt[] = "Now \"Live\"\tC:\\FM\x01 end";

static RadioSnapshot snapshot;
static Settings settings;
static bool tunerStarted;
static bool rdsOn;
static bool joined;
/* Touch readings taken so far, for the fake input layer. */
static uint32_t touchReads;
/* The Touch setting, for the fake input layer. */
static bool touchOn;

/* The context the web files are handed, as web_update.cpp hands it. */
static WebServer server;
static WebContext context = {server, &settings, 0, false};

/* FM 106.40 MHz, a good signal, stereo, and RDS with a name and a text. */
static void fillSnapshot(void) {
  snapshot = RadioSnapshot();
  snapshot.settings.band = BAND_FM;
  snapshot.settings.freqKHz = 106400;
  snapshot.settings.stepKHz = 100;
  snapshot.settings.volumeDb = -6;
  snapshot.settings.tuneMode = TUNE_MODE_MANUAL;
  snapshot.settings.deemphasisUs = 50;

  Tef668xQuality *q = &snapshot.quality;
  q->levelDbuVTenths = 452;
  q->usnTenths = 120;
  q->multipathTenths = 30;
  q->offsetKHzTenths = -4;
  q->bandwidthKHz = 236;
  q->modulationPercent = 64;
  q->stereo = true;
  q->snrDb = 28;
  snapshot.qualityValid = true;
  snapshot.levelSmoothedTenths = 449;
  snapshot.processingValid = true;
  snapshot.squelchMode = SQUELCH_MANUAL;
  snapshot.squelchThresholdTenths = 150;
  snapshot.squelchOpen = true;
  snapshot.memorySlot = 2;
  snapshot.sequence = 4711;

  RdsInfo *r = &snapshot.rds;
  r->synchronised = true;
  r->hasPi = true;
  r->pi = 0x2204;
  r->hasPty = true;
  r->pty = 10;
  r->hasFlags = true;
  r->tp = true;
  r->hasPs = true;
  memcpy(r->ps, kPs, sizeof(kPs));
  memcpy(r->psHeard, kPs, RDS_PS_LEN);
  for (int i = 0; i < RDS_PS_LEN; i++) {
    r->psHeardHave[i] = true;
  }
  r->hasRt = true;
  memcpy(r->rt, kRt, sizeof(kRt));
  r->afCount = 2;
  r->afKHz[0] = 98300;
  r->afKHz[1] = 106400;
  r->clock.valid = true;
  r->clock.year = 2026;
  r->clock.month = 10;
  r->clock.day = 3;
  r->clock.hour = 7;
  r->clock.minute = 30;
  r->clock.offsetHalfHours = 11;
  /* An RT+ tag over the quoted word, so the tag's text has a quote too. */
  r->rtPlus = true;
  r->rtPlusCount = 1;
  r->rtPlusTag[0].type = 1;
  r->rtPlusTag[0].start = 4;
  r->rtPlusTag[0].length = 5;
  r->eonCount = 1;
  r->eon[0].pi = 0x2205;
  r->eon[0].heard = 3;
  r->eon[0].hasPs = true;
  memcpy(r->eon[0].ps, kPs, sizeof(kPs));
  r->minute.spanMs = 60000;
  r->minute.groups = 680;
  r->minute.types[0][0] = 300;
  r->minute.types[2][0] = 200;
  r->groupsSeen = 680;
  r->groupsUsed = 670;
}

/* --------------------------------- fixed answers from the rest of the radio */

bool radioRdsEnabled(void) {
  return rdsOn;
}
bool radioGetSnapshot(RadioSnapshot *out) {
  *out = snapshot;
  return true;
}
uint32_t radioTaskStackFree(void) {
  return 1840;
}
Tef668xError tunerStartError(void) {
  return TEF668X_ERR_NO_DEVICE;
}

const Tef668xCapabilities *tef668xCapabilities(void) {
  static Tef668xCapabilities caps;
  caps.patchVersion = 205;
  caps.part = "TEF6686";
  return tunerStarted ? &caps : NULL;
}
const Tef668xDiagnostics *tef668xDiagnostics(void) {
  static Tef668xDiagnostics diag;
  diag.sawDevice = true;
  diag.xtalAdc = 12;
  diag.xtal = "9.216 MHz";
  return &diag;
}
const char *tef668xErrorText(Tef668xError) {
  return "no device";
}
bool tef668xLastIdentification(uint16_t *device, uint16_t *hardware,
                               uint16_t *software) {
  *device = 0x0E05;
  *hardware = 0x0101;
  *software = 0x0205;
  return true;
}

int8_t screenTaskLevelOffsetDb(BandId) {
  return 0;
}
bool screenTaskBacklightState(uint8_t *percent) {
  *percent = 80;
  return false;
}
int16_t screenTaskSignalShown(void) {
  return 45;
}
uint16_t screenTaskSwapMs(void) {
  return 12;
}

void bandScanProgress(uint16_t *done, uint16_t *total) {
  *done = 0;
  *total = 0;
}
bool bandScanLastResult(BandScanResult *out) {
  *out = BandScanResult();
  out->band = BAND_MW;
  out->found = 14;
  out->added = 2;
  out->noRoom = 3;
  out->complete = true;
  return true;
}
bool bandScanActive(void) {
  return false;
}

void inputStatusGet(InputStatus *out) {
  *out = InputStatus();
  out->keypadPresent = true;
  out->clicks = 120;
  out->presses = 33;
  snprintf(out->lastEvent, sizeof(out->lastEvent), "%s", "BAND long");
  out->lastEventMs = 61000;
  out->pot = 2048;
  out->potDb = -12;
  out->lines = 0xFFFF;
  out->linesOk = 1;
  out->touchOn = touchOn;
  if (touchReads > 0) {
    out->touchPen = true;
    out->touchDowns = 2;
    out->touchReads = touchReads;
    out->touch.x = 2257;
    out->touch.y = 2194;
    out->touch.z1 = 682;
    out->touch.z2 = 4095;
    out->touchMapped = true;
    out->touchAt.x = 160;
    out->touchAt.y = 119;
    out->touchCalStored = true;
  }
}
bool inputPotCalibrating(uint16_t *, uint16_t *) {
  return false;
}

WifiState wifiState(void) {
  return joined ? WIFI_STATE_ONLINE : WIFI_STATE_ACCESS_POINT;
}
bool wifiRssiDbm(int8_t *out) {
  *out = -58;
  return joined;
}
const char *wifiAddress(void) {
  return joined ? "192.0.2.40" : "192.168.4.1";
}

const char *rollbackRunningPartition(void) {
  return "app0";
}
bool rollbackPending(void) {
  return false;
}
/* The update check: a newer release found, so both of its fields show. */
const char *updateCheckStateName(void) {
  return "found";
}
const char *updateCheckVersion(void) {
  return "0.2.0";
}
uint32_t updateCheckSize(void) {
  return 1714848;
}
const char *restartReasonText(void) {
  return "power";
}
bool restartReasonLastHeap(uint32_t *) {
  return false;
}
bool webPinIsDefault(void) {
  return false;
}

/* The PC Link: two PCs signed in, one turned away. */
static uint8_t pcClients = 2;
bool xdrServerListening(void) {
  return true;
}
uint8_t xdrServerClients(void) {
  return pcClients;
}
bool xdrServerClientAddress(uint8_t index, char *out, size_t cap) {
  if (index >= pcClients) {
    return false;
  }
  snprintf(out, cap, "192.168.1.%u", 20u + index);
  return true;
}
uint32_t xdrServerRefused(void) {
  return 1;
}

uint32_t systemHeapFree(void) {
  return 112000;
}
uint32_t systemHeapLowest(void) {
  return 98000;
}
uint32_t systemHeapLargest(void) {
  return 65524;
}
bool sleepTaskLeftS(uint32_t *) {
  return false;
}

bool lvglPortMemory(uint32_t *usedBytes, uint32_t *totalBytes,
                    uint8_t *usedPercent, uint32_t *largestFree,
                    uint32_t *peakUsed) {
  *usedBytes = 30000;
  *totalBytes = 49152;
  *usedPercent = 61;
  *largestFree = 15000;
  *peakUsed = 33000;
  return true;
}
bool lvglPortPushes(PushSecond *out) {
  *out = PushSecond();
  out->pushes = 25;
  out->pixels = 76800;
  out->busyUs = 41000;
  out->longestUs = 2100;
  out->longestRefreshUs = 9000;
  return true;
}

bool batteryAdcFitted(void) {
  return true;
}
bool batteryAdcRead(uint16_t *) {
  return false;
}
bool batteryAdcAtBoot(uint16_t *milliVolts) {
  *milliVolts = 3950;
  return true;
}

bool ntpSynchronised(void) {
  return true;
}
bool ntpEverSynced(void) {
  return true;
}
uint32_t ntpSecondsSinceSync(void) {
  return 600;
}
ClockTime ntpLocalTime(void) {
  ClockTime t = {7, 30, true};
  return t;
}

void settingsTaskStatus(SettingsSaveStatus *out) {
  *out = SettingsSaveStatus();
  out->saves = 3;
  out->idleMs = 10000;
}

uint16_t memoryStorePi(int) {
  return 0x2204;
}
int memoryStoreCount(void) {
  return 12;
}
bool memoryStoreFailed(void) {
  return false;
}
int memoryStoreCleared(void) {
  return 0;
}
bool logbookFsPresent(void) {
  return true;
}
uint16_t logbookFsCount(void) {
  return 5;
}

/* --------------------------------------------------- a small JSON reader */

/*
 * One value read back. `kind` is '{' for an object, '[' for an array, '"'
 * for a string, '0' for a number, and 't', 'f' or 'n' for true, false and
 * null. An object's names are in `keys`, its values in `items` beside them.
 */
struct Json {
  char kind = 0;
  std::string text; /* A string decoded, or a number as it was written. */
  std::vector<std::string> keys;
  std::vector<Json> items;
};

static const char *skipSpace(const char *p) {
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
    p++;
  }
  return p;
}

static const char *readString(const char *p, std::string *out) {
  if (*p != '"') {
    return NULL;
  }
  for (p++; *p != '"'; p++) {
    const unsigned char c = (unsigned char)*p;
    /* A control character has to be escaped. Sent raw it breaks the parse
     * in every browser. */
    if (c < 0x20) {
      return NULL;
    }
    if (c != '\\') {
      out->push_back((char)c);
      continue;
    }
    p++;
    switch (*p) {
      case '"':
      case '\\':
      case '/':
        out->push_back(*p);
        break;
      case 'b':
        out->push_back('\b');
        break;
      case 'f':
        out->push_back('\f');
        break;
      case 'n':
        out->push_back('\n');
        break;
      case 'r':
        out->push_back('\r');
        break;
      case 't':
        out->push_back('\t');
        break;
      case 'u': {
        char hex[5] = {0};
        for (int i = 0; i < 4; i++) {
          if (!isxdigit((unsigned char)p[i + 1])) {
            return NULL;
          }
          hex[i] = p[i + 1];
        }
        /* The document escapes only control characters, so anything above
         * 0x7f is kept as a question mark. It still counts as valid. */
        const unsigned long code = strtoul(hex, NULL, 16);
        out->push_back(code < 0x80 ? (char)code : '?');
        p += 4;
        break;
      }
      default:
        return NULL;
    }
  }
  return p + 1;
}

static const char *readDigits(const char *p) {
  if (!isdigit((unsigned char)*p)) {
    return NULL;
  }
  while (isdigit((unsigned char)*p)) {
    p++;
  }
  return p;
}

static const char *readNumber(const char *p, std::string *out) {
  const char *start = p;
  if (*p == '-') {
    p++;
  }
  /* No leading zeros: 0 alone, or a digit from 1 up. */
  p = *p == '0' ? p + 1 : readDigits(p);
  if (p != NULL && *p == '.') {
    p = readDigits(p + 1);
  }
  if (p != NULL && (*p == 'e' || *p == 'E')) {
    p++;
    if (*p == '+' || *p == '-') {
      p++;
    }
    p = readDigits(p);
  }
  if (p != NULL) {
    out->assign(start, (size_t)(p - start));
  }
  return p;
}

static const char *readValue(const char *p, Json *v) {
  p = skipSpace(p);
  if (*p == '{' || *p == '[') {
    const char close = *p == '{' ? '}' : ']';
    v->kind = *p;
    p = skipSpace(p + 1);
    if (*p == close) {
      return p + 1;
    }
    for (;;) {
      if (v->kind == '{') {
        std::string key;
        p = readString(skipSpace(p), &key);
        if (p == NULL) {
          return NULL;
        }
        /* A name twice in one object hides one of its two values. */
        for (const std::string &k : v->keys) {
          if (k == key) {
            return NULL;
          }
        }
        p = skipSpace(p);
        if (*p != ':') {
          return NULL;
        }
        p++;
        v->keys.push_back(key);
      }
      v->items.emplace_back();
      p = readValue(p, &v->items.back());
      if (p == NULL) {
        return NULL;
      }
      p = skipSpace(p);
      if (*p == close) {
        return p + 1;
      }
      if (*p != ',') {
        return NULL;
      }
      p++;
    }
  }
  if (*p == '"') {
    v->kind = '"';
    return readString(p, &v->text);
  }
  static const char *const kWords[] = {"true", "false", "null"};
  for (const char *word : kWords) {
    if (strncmp(p, word, strlen(word)) == 0) {
      v->kind = word[0];
      return p + strlen(word);
    }
  }
  v->kind = '0';
  return readNumber(p, &v->text);
}

/* The whole of `text` is one JSON value, with nothing after it. */
static bool readJson(const char *text, Json *out) {
  const char *end = readValue(text, out);
  return end != NULL && *skipSpace(end) == '\0';
}

/* The value under `key`, failing the test when the object has none. */
static const Json &at(const Json &object, const char *key) {
  for (size_t i = 0; i < object.keys.size(); i++) {
    if (object.keys[i] == key) {
      return object.items[i];
    }
  }
  TEST_FAIL_MESSAGE(key);
  return object;
}

/* Build the document and read it back, failing the test if it does not
 * parse. The document goes in the message, so a failure shows it. */
static Json state(void) {
  const String text = buildState();
  Json doc;
  TEST_ASSERT_TRUE_MESSAGE(readJson(text.c_str(), &doc), text.c_str());
  return doc;
}

void setUp(void) {
  webStateRegisterRoutes(&context);
  tunerStarted = true;
  rdsOn = true;
  joined = true;
  touchReads = 0;
  touchOn = true;
  settingsDefaults(&settings);
  fillSnapshot();
}
void tearDown(void) {}

/* ----------------------------------------------------------------- tests */

static void the_reader_refuses_broken_json(void) {
  static const char *const bad[] = {"",           "{",
                                    "{\"a\":1,}", "{\"a\":\"x\ty\"}",
                                    "{\"a\":01}", "{\"a\":tru}",
                                    "[1 2]",      "{\"a\":\"\\q\"}",
                                    "{} x",       "{\"a\":1,\"a\":2}"};
  for (const char *text : bad) {
    Json doc;
    TEST_ASSERT_FALSE_MESSAGE(readJson(text, &doc), text);
  }
  Json doc;
  TEST_ASSERT_TRUE(readJson(
      " {\"a\":[1,-2.5e3,true,false,null,\"\\u0041\\\"\"],\"b\":{}} ", &doc));
  TEST_ASSERT_EQUAL_STRING("A\"", at(doc, "a").items[5].text.c_str());
}

static void the_whole_document_parses_as_one_object(void) {
  const Json doc = state();
  TEST_ASSERT_EQUAL_INT('{', doc.kind);
  TEST_ASSERT_EQUAL_STRING("ats125", at(doc, "brd").text.c_str());
  TEST_ASSERT_EQUAL_INT('{', at(doc, "tun").kind);
  TEST_ASSERT_EQUAL_INT('{', at(doc, "inp").kind);
}

static void the_band_and_the_frequency_are_reported(void) {
  const Json doc = state();
  const Json &tun = at(doc, "tun");
  TEST_ASSERT_EQUAL_STRING("FM", at(tun, "bnd").text.c_str());
  TEST_ASSERT_EQUAL_INT('0', at(tun, "khz").kind);
  TEST_ASSERT_EQUAL_STRING("106400", at(tun, "khz").text.c_str());
}

/* The PC Link: open, who is in by address, and how many were turned away. */
static void the_pc_link_says_who_is_signed_in(void) {
  const Json doc = state();
  const Json &pcl = at(doc, "pcl");
  TEST_ASSERT_EQUAL_INT('t', at(pcl, "on").kind);
  TEST_ASSERT_EQUAL_STRING("2", at(pcl, "cli").text.c_str());
  TEST_ASSERT_EQUAL_UINT32(2, at(pcl, "ips").items.size());
  TEST_ASSERT_EQUAL_STRING("192.168.1.21",
                           at(pcl, "ips").items[1].text.c_str());
  TEST_ASSERT_EQUAL_STRING("1", at(pcl, "ref").text.c_str());
  pcClients = 0;
  const Json none = state();
  TEST_ASSERT_EQUAL_UINT32(0, at(at(none, "pcl"), "ips").items.size());
  pcClients = 2;
}

/* The last scan's counts, the stations with no free slot among them, so a
 * full store reads differently from a band with nothing new on it. */
static void the_last_scan_says_how_many_found_no_room(void) {
  const Json doc = state();
  const Json &tun = at(doc, "tun");
  TEST_ASSERT_EQUAL_STRING("14", at(tun, "scf").text.c_str());
  TEST_ASSERT_EQUAL_STRING("2", at(tun, "sca").text.c_str());
  TEST_ASSERT_EQUAL_STRING("3", at(tun, "scr").text.c_str());
  TEST_ASSERT_EQUAL_INT('t', at(tun, "scc").kind);
}

static void station_text_with_quotes_and_control_characters_comes_back_whole(
    void) {
  const Json doc = state();
  const Json &rds = at(at(doc, "tun"), "rds");
  TEST_ASSERT_EQUAL_STRING(kPs, at(rds, "ps").text.c_str());
  TEST_ASSERT_EQUAL_STRING(kPs, at(rds, "psh").text.c_str());
  TEST_ASSERT_EQUAL_STRING(kRt, at(rds, "rt").text.c_str());
  TEST_ASSERT_EQUAL_STRING(kPs, at(at(rds, "eon").items[0], "ps").text.c_str());
}

static void the_image_check_and_the_stack_marks_are_reported(void) {
  const Json doc = state();
  TEST_ASSERT_EQUAL_INT('t', at(doc, "cnf").kind);
  const Json &stk = at(doc, "stk");
  TEST_ASSERT_EQUAL_INT('{', stk.kind);
  TEST_ASSERT_EQUAL_STRING("1840", at(stk, "rad").text.c_str());
  TEST_ASSERT_EQUAL_STRING("2600", at(stk, "lop").text.c_str());
}

static void a_found_update_is_reported_with_its_version_and_size(void) {
  const Json doc = state();
  const Json &upd = at(doc, "upd");
  TEST_ASSERT_EQUAL_INT('{', upd.kind);
  TEST_ASSERT_EQUAL_STRING("found", at(upd, "st").text.c_str());
  TEST_ASSERT_EQUAL_STRING("0.2.0", at(upd, "ver").text.c_str());
  TEST_ASSERT_EQUAL_STRING("1714848", at(upd, "sz").text.c_str());
}

static void a_tuner_that_did_not_start_still_gives_valid_json(void) {
  tunerStarted = false;
  const Json doc = state();
  const Json &tun = at(doc, "tun");
  TEST_ASSERT_EQUAL_STRING("no device", at(tun, "err").text.c_str());
  TEST_ASSERT_EQUAL_STRING("0E05", at(tun, "dev").text.c_str());
}

static void rds_off_and_the_access_point_still_give_valid_json(void) {
  rdsOn = false;
  joined = false;
  const Json doc = state();
  TEST_ASSERT_EQUAL_STRING("ap", at(doc, "net").text.c_str());
  TEST_ASSERT_EQUAL_INT('n', at(doc, "rssi").kind);
  TEST_ASSERT_EQUAL_INT('t', at(at(at(doc, "tun"), "rds"), "off").kind);
}

/* Before the first reading the raw values are null, never a 0 that looks
 * like a reading. */
static void the_touch_values_are_null_before_the_first_reading(void) {
  const Json doc = state();
  const Json &tch = at(at(doc, "inp"), "tch");
  TEST_ASSERT_EQUAL_INT('t', at(tch, "on").kind);
  TEST_ASSERT_EQUAL_INT('f', at(tch, "pen").kind);
  TEST_ASSERT_EQUAL_STRING("0", at(tch, "rd").text.c_str());
  TEST_ASSERT_EQUAL_INT('n', at(tch, "x").kind);
  TEST_ASSERT_EQUAL_INT('n', at(tch, "z2").kind);
  TEST_ASSERT_EQUAL_STRING("board", at(tch, "cal").text.c_str());
  TEST_ASSERT_EQUAL_INT('n', at(tch, "px").kind);
  TEST_ASSERT_EQUAL_INT('n', at(tch, "py").kind);
}

/* After one, each comes back as a number, the largest at full width. */
static void a_touch_reading_comes_back_as_numbers(void) {
  touchReads = 7;
  const Json doc = state();
  const Json &tch = at(at(doc, "inp"), "tch");
  TEST_ASSERT_EQUAL_INT('t', at(tch, "pen").kind);
  TEST_ASSERT_EQUAL_STRING("2", at(tch, "dn").text.c_str());
  TEST_ASSERT_EQUAL_STRING("7", at(tch, "rd").text.c_str());
  TEST_ASSERT_EQUAL_STRING("2257", at(tch, "x").text.c_str());
  TEST_ASSERT_EQUAL_STRING("2194", at(tch, "y").text.c_str());
  TEST_ASSERT_EQUAL_STRING("682", at(tch, "z1").text.c_str());
  TEST_ASSERT_EQUAL_STRING("4095", at(tch, "z2").text.c_str());
  TEST_ASSERT_EQUAL_STRING("stored", at(tch, "cal").text.c_str());
  TEST_ASSERT_EQUAL_STRING("160", at(tch, "px").text.c_str());
  TEST_ASSERT_EQUAL_STRING("119", at(tch, "py").text.c_str());
}

/* With the Touch setting off the document says so, so counts that stand
 * still are not read as nobody touching. */
static void touch_turned_off_is_reported(void) {
  touchOn = false;
  const Json doc = state();
  TEST_ASSERT_EQUAL_INT('f', at(at(at(doc, "inp"), "tch"), "on").kind);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(the_reader_refuses_broken_json);
  RUN_TEST(the_whole_document_parses_as_one_object);
  RUN_TEST(the_band_and_the_frequency_are_reported);
  RUN_TEST(the_pc_link_says_who_is_signed_in);
  RUN_TEST(the_last_scan_says_how_many_found_no_room);
  RUN_TEST(station_text_with_quotes_and_control_characters_comes_back_whole);
  RUN_TEST(the_image_check_and_the_stack_marks_are_reported);
  RUN_TEST(a_found_update_is_reported_with_its_version_and_size);
  RUN_TEST(a_tuner_that_did_not_start_still_gives_valid_json);
  RUN_TEST(rds_off_and_the_access_point_still_give_valid_json);
  RUN_TEST(the_touch_values_are_null_before_the_first_reading);
  RUN_TEST(a_touch_reading_comes_back_as_numbers);
  RUN_TEST(touch_turned_off_is_reported);
  return UNITY_END();
}
