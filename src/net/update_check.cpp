#include "update_check.h"
#include "debug_log.h"

#include "band_scan_task.h"
#include "board/board.h"
#include "core/update_check.h"
#include "core/version.h"
#include "firmware_write.h"
#include "net/restart_reason.h"
#include "net/rollback.h"
#include "net/wifi_manager.h"
#include "radio_task.h"
#include "settings_task.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>

#include <atomic>

/*
 * The CA bundle the framework already carries inside its mbedTLS, the
 * Mozilla list of roots. GitHub and its file host use two different
 * authorities, and GitHub has changed authority before; with the whole list
 * a radio can still reach the next release when that happens again, where a
 * radio carrying only today's two roots could not.
 */
extern const uint8_t kCaBundleStart[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t kCaBundleEnd[] asm("_binary_x509_crt_bundle_end");

/* The bundle's length. Its two ends are linker symbols, so the length is
 * their addresses' difference. */
static void useCaBundle(NetworkClientSecure &client) {
  const size_t len =
      (size_t)((uintptr_t)kCaBundleEnd - (uintptr_t)kCaBundleStart);
  client.setCACertBundle(kCaBundleStart, len);
}

/* A redirect from github.com to the release's own copy, and one from there
 * to the file host, is two. Four leaves room and still ends a loop. */
#define UPDATE_MAX_REDIRECTS 4

/* How long to wait to connect, for the TLS handshake, and for an answer.
 * A check took 3 to 6 s on the radio, handshakes included. Each wait stays
 * well under the task watchdog's 30 s, and the loop, when it is the loop
 * waiting, is fed between the steps. The handshake needs its own limit: the framework's is 120 s, so a
 * network that takes the connection and then stalls would set off the
 * watchdog, and the check would do it again at every start. */
#define UPDATE_CONNECT_MS 8000
#define UPDATE_HANDSHAKE_S 8
#define UPDATE_READ_MS 10000

/* How long a download may go without a byte before it is given up. */
#define UPDATE_STALL_MS 15000

/* Read in pieces this size, the size the browser upload hands over. */
#define UPDATE_CHUNK_BYTES 4096

/*
 * The check runs in a task of its own, which exists only while it runs.
 * On the loop it froze the screen, the knob and the web page for 6.1 s,
 * measured on the radio, and a TLS handshake cannot be cut into steps the
 * way a band scan is. Core 0, below the radio task's priority of 3, so the
 * radio task takes the core whenever it wakes and its RDS timing does not
 * move. The stack holds the TLS handshake, which used 5,360 bytes when it
 * ran on the loop.
 */
#define UPDATE_TASK_STACK 8192
#define UPDATE_TASK_PRIORITY 1
#define UPDATE_TASK_CORE 0

static const Settings *sSettings = NULL;
static UpdateDue sDue;
static UpdateState sState = UPDATE_STATE_OFF;
static UpdateManifest sManifest;
static bool sOfferDue = false;
static bool sInstallWanted = false;
/* Whether the check task was started and its result not yet taken. Only the
 * loop reads or writes this and everything above. */
static bool sChecking = false;

/* What the check task found. It writes these, then sets sCheckDone, and the
 * loop reads them only after it sees sCheckDone set, so the two never use
 * them at the same time. */
static UpdateState sCheckResult = UPDATE_STATE_FAILED;
static UpdateManifest sCheckFound;
static std::atomic<bool> sCheckDone{false};

/* The loop's own task, the only one the loop watchdog knows. */
static TaskHandle_t sLoopTask = NULL;

/* Feed the loop watchdog, when it is the loop that is waiting. The install
 * runs on the loop and the check does not. */
static void feedIfLoop(void) {
  if (xTaskGetCurrentTaskHandle() == sLoopTask) {
    feedLoopWDT();
  }
}

void updateCheckBegin(const Settings *settings) {
  sSettings = settings;
  sLoopTask = xTaskGetCurrentTaskHandle();
  updateDueReset(&sDue);
  sState = UPDATE_STATE_OFF;
  sOfferDue = false;
  sInstallWanted = false;
}

/* The slot a new image is written to, which is the size it may have. */
static uint32_t slotBytes(void) {
  const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
  return next != NULL ? next->size : 0;
}

/*
 * Open `url` and follow its redirects by hand, checking each one with
 * updateUrlAllowed. By hand, because HTTPClient keeps the first host's
 * connection when a redirect goes to another host, and so that every hop is
 * checked rather than only the first.
 *
 * Returns the status of the last answer, with `http` still open on it for the
 * caller to read, or a negative number when a connection failed or a
 * redirect went somewhere it may not.
 */
static int openFollowing(HTTPClient &http, NetworkClientSecure &client,
                         const char *firstUrl) {
  String url = firstUrl;
  static const char *kKeep[] = {"Location"};
  for (int hop = 0; hop <= UPDATE_MAX_REDIRECTS; hop++) {
    if (!updateUrlAllowed(url.c_str())) {
      DebugLog.println(
          F("[update] refused an address outside GitHub releases"));
      return -100;
    }
    feedIfLoop();
    http.setReuse(false);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.setConnectTimeout(UPDATE_CONNECT_MS);
    http.setTimeout(UPDATE_READ_MS);
    http.collectHeaders(kKeep, 1);
    if (!http.begin(client, url)) {
      return -101;
    }
    const int code = http.GET();
    feedIfLoop();
    if (code == 301 || code == 302 || code == 303 || code == 307 ||
        code == 308) {
      url = http.header("Location");
      http.end();
      client.stop();
      continue;
    }
    return code;
  }
  DebugLog.println(F("[update] too many redirects"));
  return -102;
}

/* Read the whole answer into `out`, at most `cap` bytes. False when it is
 * longer or empty. Through getString, which takes a chunked answer apart; a
 * declared length over the cap is refused before anything is read. */
static bool readBody(HTTPClient &http, char *out, size_t cap, size_t *len) {
  if (http.getSize() > (int)cap) {
    return false;
  }
  feedIfLoop();
  const String body = http.getString();
  if (body.length() == 0 || body.length() > cap) {
    return false;
  }
  memcpy(out, body.c_str(), body.length());
  *len = body.length();
  return true;
}

/* Fetch and check the manifest, and say what was found. Runs in the check
 * task and touches nothing the loop uses: `found` is filled in only for
 * UPDATE_STATE_FOUND. */
static UpdateState check(UpdateManifest *found) {
  char url[sizeof(UPDATE_MANIFEST_URL_PREFIX) + 24];
  snprintf(url, sizeof(url), UPDATE_MANIFEST_URL_PREFIX "%s.json", BOARD_NAME);
  char body[UPDATE_MANIFEST_MAX_BYTES];
  size_t len = 0;

  const uint32_t started = millis();
  radioSetNetServing(true);
  NetworkClientSecure client;
  useCaBundle(client);
  client.setHandshakeTimeout(UPDATE_HANDSHAKE_S);
  HTTPClient http;
  const int code = openFollowing(http, client, url);
  const bool read = code == 200 && readBody(http, body, sizeof(body), &len);
  http.end();
  client.stop();
  radioSetNetServing(false);

  if (code == 404) {
    /* No release yet, or none with a manifest for this board. */
    DebugLog.println(F("[update] no release for this board"));
    return UPDATE_STATE_NONE;
  }
  if (!read) {
    DebugLog.printf("[update] check failed, status %d, after %lu ms\n", code,
                    (unsigned long)(millis() - started));
    return UPDATE_STATE_FAILED;
  }

  const UpdateManifestResult result =
      updateParseManifest(body, len, BOARD_NAME, slotBytes(), found);
  if (result != UPDATE_MANIFEST_OK) {
    DebugLog.printf("[update] manifest refused, reason %d\n", (int)result);
    return UPDATE_STATE_FAILED;
  }
  UpdateVersion mine;
  if (!updateParseVersion(FIRMWARE_VERSION, &mine) ||
      updateCompareVersions(&found->version, &mine) <= 0) {
    DebugLog.printf("[update] %s is the latest, nothing newer\n",
                    found->versionText);
    return UPDATE_STATE_NONE;
  }
  DebugLog.printf("[update] %s found after %lu ms\n", found->versionText,
                  (unsigned long)(millis() - started));
  return UPDATE_STATE_FOUND;
}

/* The check task: one check, then it ends. check() returns before the task
 * is deleted, so its client and its buffers are gone by then. */
static void checkTask(void *) {
  sCheckResult = check(&sCheckFound);
  sCheckDone.store(true);
  vTaskDelete(NULL);
}

static void startCheck(void) {
  sCheckDone.store(false);
  if (xTaskCreatePinnedToCore(checkTask, "update", UPDATE_TASK_STACK, NULL,
                              UPDATE_TASK_PRIORITY, NULL,
                              UPDATE_TASK_CORE) != pdPASS) {
    DebugLog.println(F("[update] no memory for the check"));
    sState = UPDATE_STATE_FAILED;
    return;
  }
  sChecking = true;
  sState = UPDATE_STATE_CHECKING;
}

/* Take what the check task found, once it has finished. */
static void takeCheck(void) {
  if (!sCheckDone.load()) {
    return;
  }
  sChecking = false;
  sState = sCheckResult;
  if (sState == UPDATE_STATE_FOUND) {
    sManifest = sCheckFound;
    sOfferDue = true;
  }
}

/* Write the image as it arrives, hashing it on the way, and keep it only if
 * the size and the sha256 both match the manifest. */
static bool download(HTTPClient &http, uint8_t *buf) {
  NetworkClient *stream = http.getStreamPtr();
  const uint32_t size = sManifest.size;
  if (stream == NULL || http.getSize() != (int)size ||
      !Update.begin(size, U_FLASH)) {
    return false;
  }
  mbedtls_sha256_context sha;
  mbedtls_sha256_init(&sha);
  mbedtls_sha256_starts(&sha, 0);
  uint32_t got = 0;
  uint32_t lastByte = millis();
  bool broken = false;
  while (got < size && !broken) {
    feedLoopWDT();
    const int avail = stream->available();
    if (avail <= 0) {
      if (millis() - lastByte > UPDATE_STALL_MS) {
        broken = true;
      }
      delay(1);
      continue;
    }
    size_t want = (size_t)avail;
    if (want > UPDATE_CHUNK_BYTES) {
      want = UPDATE_CHUNK_BYTES;
    }
    if (want > size - got) {
      want = size - got;
    }
    const size_t n = stream->readBytes(buf, want);
    if (n == 0) {
      continue;
    }
    if (Update.write(buf, n) != n) {
      broken = true;
      break;
    }
    mbedtls_sha256_update(&sha, buf, n);
    got += (uint32_t)n;
    lastByte = millis();
    firmwareWriteProgress((int)((uint64_t)got * 100u / size));
  }
  uint8_t digest[32];
  mbedtls_sha256_finish(&sha, digest);
  mbedtls_sha256_free(&sha);

  char hex[65];
  for (int i = 0; i < 32; i++) {
    snprintf(hex + i * 2, 3, "%02x", digest[i]);
  }
  if (broken || got != size || strcmp(hex, sManifest.sha256) != 0) {
    DebugLog.printf("[update] image refused: %lu of %lu bytes, sha256 %s\n",
                    (unsigned long)got, (unsigned long)size,
                    strcmp(hex, sManifest.sha256) == 0 ? "matches" : "differs");
    Update.abort();
    return false;
  }
  return Update.end(true);
}

static void install(void) {
  if (sState != UPDATE_STATE_FOUND || rollbackPending() || bandScanActive()) {
    return;
  }
  DebugLog.printf("[update] installing %s\n", sManifest.versionText);
  uint8_t *buf = (uint8_t *)malloc(UPDATE_CHUNK_BYTES);
  if (buf == NULL) {
    DebugLog.println(F("[update] no memory for the download"));
    return;
  }
  firmwareWriteBegin();
  radioSetNetServing(true);
  bool ok = false;
  {
    NetworkClientSecure client;
    useCaBundle(client);
    client.setHandshakeTimeout(UPDATE_HANDSHAKE_S);
    HTTPClient http;
    const int code = openFollowing(http, client, sManifest.url);
    ok = code == 200 && download(http, buf);
    if (code != 200) {
      DebugLog.printf("[update] download failed, status %d\n", code);
    }
    http.end();
    client.stop();
  }
  radioSetNetServing(false);
  free(buf);
  firmwareWriteEnd(ok);
  if (!ok) {
    /* The old image keeps running, and the update stays offered in System
     * so it can be tried again. */
    return;
  }
  DebugLog.println(F("[update] written, restarting into it"));
  restartReasonNote(RESTART_WHY_UPDATE);
  settingsTaskRestart();
}

void updateCheckLoop(bool busy) {
  if (sSettings == NULL) {
    return;
  }
  if (sChecking) {
    takeCheck();
    return;
  }
  if (sInstallWanted) {
    sInstallWanted = false;
    install();
    return;
  }
  const UpdateDueInputs in = {sSettings->updateCheck != 0,
                              wifiState() == WIFI_STATE_ONLINE,
                              rollbackPending(), busy};
  if (sState == UPDATE_STATE_OFF && in.enabled) {
    sState = UPDATE_STATE_WAITING;
  } else if (sState == UPDATE_STATE_WAITING && !in.enabled) {
    sState = UPDATE_STATE_OFF;
  }
  if (updateDueStep(&sDue, &in)) {
    startCheck();
  }
}

UpdateState updateCheckState(void) {
  return sState;
}

bool updateCheckRunning(void) {
  return sChecking;
}

const char *updateCheckStateName(void) {
  switch (sState) {
    case UPDATE_STATE_WAITING:
      return "wait";
    case UPDATE_STATE_CHECKING:
      return "checking";
    case UPDATE_STATE_NONE:
      return "none";
    case UPDATE_STATE_FOUND:
      return "found";
    case UPDATE_STATE_FAILED:
      return "failed";
    case UPDATE_STATE_OFF:
    default:
      return "off";
  }
}

const char *updateCheckVersion(void) {
  return sState == UPDATE_STATE_FOUND ? sManifest.versionText : NULL;
}

uint32_t updateCheckSize(void) {
  return sState == UPDATE_STATE_FOUND ? sManifest.size : 0;
}

bool updateCheckTakeOffer(void) {
  const bool due = sOfferDue && sState == UPDATE_STATE_FOUND;
  sOfferDue = false;
  return due;
}

UpdateInstallAsk updateCheckInstall(void) {
  if (sState != UPDATE_STATE_FOUND) {
    return UPDATE_INSTALL_NOTHING;
  }
  if (rollbackPending()) {
    return UPDATE_INSTALL_ON_TRIAL;
  }
  if (bandScanActive()) {
    return UPDATE_INSTALL_BUSY;
  }
  sInstallWanted = true;
  return UPDATE_INSTALL_STARTING;
}
