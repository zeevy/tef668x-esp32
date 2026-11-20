/*
 * The server itself: the context every other web_*.cpp file is handed, the
 * firmware upload, and the loop that carries out a reboot once a reply has
 * gone out.
 */

#include "web_update.h"
#include "debug_log.h"

#include "web_internal.h"

#include "band_scan_task.h"
#include "firmware_write.h"
#include "net/restart_reason.h"
#include "net/rollback.h"
#include "net/update_check.h"
#include "radio_task.h"
#include "reply_times.h"
#include "settings_task.h"

#include <Update.h>
#include <mbedtls/sha256.h>

#include "core/update_check.h"

static WebServer sServerItself(WEB_PORT);
/* The one context every web file is handed; see web_internal.h. */
static WebContext sContext = {sServerItself, NULL, 0, false};
static WebContext *const sWeb = &sContext;
/* When the last request came, for webLastRequestMs. */
static uint32_t sLastRequestMs = 0;
/* Whether the server is listening, which follows `webEnabled`. */
static bool sListening = false;

/* Set while an upload is running and the client was not signed in. */
static bool sUploadRejected = false;

/* An upload may name the sha256 of its file, as `POST /update?sha256=`
 * followed by 64 hex digits. The radio then works out the sha256 of what
 * arrived and switches to the new image only when the two are the same, so
 * a file damaged on the way is not started. In the address and not in the
 * form, because a form's fields are only readable once the whole file has
 * come. Without it the upload is taken as before. */
static uint8_t sUploadSha[32];
static mbedtls_sha256_context sUploadShaSum;
/* Whether the sum is started and not yet let go. */
static bool sUploadShaOpen = false;
/* Why the sha256 stops this upload, or NULL when it does not. */
static const char *sUploadShaWrong = NULL;
static const char kShaBadShape[] =
    "sha256 must be 64 hex digits. Nothing was written.";
static const char kShaDiffers[] =
    "The file is not the one that sha256 names. The radio is still running "
    "the old one.";

/* Lets the sum go, however the upload ended. */
static void shaLetGo(void) {
  if (sUploadShaOpen) {
    mbedtls_sha256_free(&sUploadShaSum);
    sUploadShaOpen = false;
  }
}

/* Why the upload running may not write, or NULL when it may. Decided once,
 * as the image begins. */
static const char *sUploadBlocked = NULL;

static const char kOnTrial[] =
    "The firmware the radio is running is still proving itself, "
    "and a restart now would put the old one back. Wait until "
    "cnf in /api/state is true.";
static const char kScanRunning[] =
    "A station scan is running. Try again when it stops.";

/* Set once a multipart part carrying a file has actually been seen. */
static bool sUploadStarted = false;

/* Set on the first write failure, so the rest of the body is taken quietly. */
static bool sUploadFailed = false;

/* Set once the end of the image has been dealt with, and once it was written,
 * for a client that goes away straight after. */
static bool sUploadEnded = false;
static bool sUploadWritten = false;

/*
 * The whole POST body, for the panel's percentage.
 *
 * `HTTPUpload` says how much has arrived but never how much is coming, so the
 * Content-Length of the request is the only figure available. It counts the
 * multipart headers as well as the image, a few hundred bytes on a megabyte
 * and a half, so the percentage is a touch behind the truth and never ahead
 * of it.
 */
static int sUploadTotal = 0;

/* ------------------------------------------------------------------ helpers */

static void handleUploadData(void) {
  /* WebServer calls this same callback for two different things. For a
   * multipart upload it fills _currentUpload first. For any other POST body it
   * takes the raw path instead, leaves _currentUpload null, and calls this
   * anyway, so sWeb->server.upload() dereferences a null pointer and the radio
   * panics. There is no accessor that says which one happened, so the content
   * type is what tells them apart. */
  if (!sWeb->server.header("Content-Type").startsWith("multipart/")) {
    return;
  }

  HTTPUpload &upload = sWeb->server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    sUploadRejected = !signedIn();
    sUploadBlocked = NULL;
    sUploadShaWrong = NULL;
    shaLetGo();
    sUploadStarted = true;
    sUploadFailed = false;
    sUploadEnded = false;
    sUploadWritten = false;
    if (sUploadRejected) {
      DebugLog.printf("[web] upload refused, no PIN, from %s\n",
                      sWeb->server.client().remoteIP().toString().c_str());
      return;
    }
    /* The same two states the install from GitHub waits for. Checked after
     * the PIN, so a caller with no PIN learns nothing of them. */
    sUploadBlocked = rollbackPending()  ? kOnTrial
                     : bandScanActive() ? kScanRunning
                                        : NULL;
    if (sUploadBlocked != NULL) {
      DebugLog.println(F("[web] upload refused: on trial or scanning"));
      return;
    }
    const bool hasSha = sWeb->server.hasArg("sha256");
    /* The length is the argument's own: one with a NUL in it reads as 64
     * letters to a text function and is not a sha256. */
    const String sha = sWeb->server.arg("sha256");
    if (hasSha &&
        (sha.length() != 64 || !updateParseSha256(sha.c_str(), sUploadSha))) {
      sUploadShaWrong = kShaBadShape;
      DebugLog.println(F("[web] upload refused: sha256 is not 64 hex digits"));
      return;
    }
    DebugLog.printf("[web] upload started: %s\n", upload.filename.c_str());
    sUploadTotal = sWeb->server.clientContentLength();
    if (hasSha) {
      mbedtls_sha256_init(&sUploadShaSum);
      mbedtls_sha256_starts(&sUploadShaSum, 0);
      sUploadShaOpen = true;
    }
    firmwareWriteBegin();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Update.printError(DebugLog);
      /* Marked and left. The radio and the panel are given back at the end of
       * the transfer and nowhere else, for the reason written over the end
       * branch below. */
      sUploadFailed = true;
    }
    return;
  }

  if (sUploadRejected || sUploadBlocked != NULL ||
      sUploadShaWrong == kShaBadShape) {
    /* Nothing was begun, so the rest of the body is taken and dropped. */
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    /* The whole upload is one pass of the loop, 15 s or more, so the task
     * watchdog is fed per chunk rather than once per pass. */
    feedLoopWDT();
    if (sUploadFailed) {
      /* Already dead. Take the rest of the body quietly rather than printing
       * an error line for every chunk of a large file. */
      return;
    }
    if (sUploadShaOpen) {
      mbedtls_sha256_update(&sUploadShaSum, upload.buf, upload.currentSize);
    }
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(DebugLog);
      sUploadFailed = true;
      return;
    }
    if (sUploadTotal > 0) {
      firmwareWriteProgress(
          (int)((uint64_t)upload.totalSize * 100U / (uint32_t)sUploadTotal));
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    /*
     * A write that died earlier gives the radio and the panel back here, at
     * the end of the transfer, and not at the moment it died.
     *
     * Two reasons. The browser carries on sending the rest of the image, which
     * on this handler is the loop task busy for several more seconds, so a
     * release at the moment of the failure would bring the tuner back in the
     * middle of a transfer. And `screenTaskPoll` does not run at all while that
     * is happening, so the four second `UPDATE FAILED` hold would run out
     * before anything could draw it and a person would never see why their
     * update did not take.
     *
     * It also gives up once only: `Update.end` fails as well on an update
     * that is already in error, so this branch returns before it.
     */
    sUploadEnded = true;
    if (sUploadFailed) {
      /* So the next upload starts from a clean state rather than on top of a
       * half written one. `hasError` stays true through it, which is what
       * tells the browser the image was refused. */
      /* Read before the abort, which puts its own error in its place. A
       * first byte that is not an image's is the file's fault; the rest is
       * the flash's. */
      const FirmwareEnd why = Update.getError() == UPDATE_ERROR_MAGIC_BYTE
                                  ? FIRMWARE_END_FILE
                                  : FIRMWARE_END_WRITE;
      if (Update.isRunning()) {
        Update.abort();
      }
      shaLetGo();
      firmwareWriteEnd(why);
      return;
    }
    if (sUploadShaOpen) {
      /* Before `Update.end`, which is what makes the new image the one the
       * radio starts next. */
      uint8_t digest[32];
      mbedtls_sha256_finish(&sUploadShaSum, digest);
      shaLetGo();
      if (memcmp(digest, sUploadSha, sizeof(digest)) != 0) {
        DebugLog.println(F("[web] upload refused: sha256 differs"));
        sUploadShaWrong = kShaDiffers;
        Update.abort();
        firmwareWriteEnd(FIRMWARE_END_FILE);
        return;
      }
    }
    if (Update.end(true)) {
      DebugLog.printf("[web] upload finished, %u bytes written\n",
                      (unsigned)upload.totalSize);
      /* Held until the reboot, which `handleUploadDone` asks for once the
       * reply is out. */
      sUploadWritten = true;
      firmwareWriteEnd(FIRMWARE_END_OK);
    } else {
      /* The whole file came and is not an image the radio can start. */
      Update.printError(DebugLog);
      firmwareWriteEnd(FIRMWARE_END_FILE);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    /* The web server says aborted straight after the end when the client
     * has gone by then, and `handleUploadDone` does not run. */
    if (sUploadWritten) {
      /* The image is written and the radio boots it at the next restart, so
       * the restart the reply would have asked for happens now. */
      restartReasonNote(RESTART_WHY_UPDATE);
      sWeb->rebootAfterReply = true;
      return;
    }
    if (sUploadEnded) {
      /* Given up at the end already. */
      return;
    }
    Update.abort();
    shaLetGo();
    DebugLog.println("[web] upload aborted");
    firmwareWriteEnd(FIRMWARE_END_DOWNLOAD);
  }
}

static void handleUploadDone(void) {
  bool rejected = sUploadRejected;
  bool sawImage = sUploadStarted;
  const char *blocked = sUploadBlocked;
  const char *shaWrong = sUploadShaWrong;
  sUploadRejected = false;
  sUploadStarted = false;
  sUploadBlocked = NULL;
  sUploadShaWrong = NULL;

  /* The PIN was checked as the image began. A session that ran out during
   * the transfer leaves it the signed in person's, and refusing here would
   * leave a written image, a hushed radio and a panel held at the end with
   * no reboot coming. The upload handler only runs for a multipart part that
   * carries a filename, so a POST with no file part is checked here instead,
   * or an unauthenticated request could ask for the reboot. */
  if (rejected || (!sawImage && !signedIn())) {
    sWeb->server.send(403, "text/plain", "Enter the access PIN first.\n");
    return;
  }

  if (!sawImage) {
    sendResult(400, "No firmware file was sent.");
    return;
  }

  if (blocked != NULL) {
    sendResult(409, blocked);
    return;
  }

  if (shaWrong != NULL) {
    sendResult(400, shaWrong);
    return;
  }

  if (Update.hasError()) {
    sendResult(500,
               "The image was not accepted. The radio is still running the "
               "old one.");
    return;
  }

  sendResult(200,
             "The radio is rebooting into the new image. If it does not come "
             "up and pass its self check, the old image comes back on its "
             "own.");
  /* Noted here rather than where the restart happens, because that one line
   * serves this and the reboot button both, and telling them apart is the
   * whole reason the note exists. */
  restartReasonNote(RESTART_WHY_UPDATE);
  sWeb->rebootAfterReply = true;
}

static void handleReboot(void) {
  if (!requireAuth(false)) {
    return;
  }
  /* A new image is on trial until its self check passes. Restarting it now
   * is exactly what the bootloader reads as a failed image, so the old one
   * would come back with nothing to say why. */
  if (rollbackPending()) {
    sendResult(409,
               "The new firmware is still proving itself, and a restart now "
               "would put the old one back. Wait until cnf in /api/state is "
               "true, about ten seconds after the radio joins the network.");
    return;
  }
  sendResult(200, "The radio is restarting.");
  restartReasonNote(RESTART_WHY_ASKED);
  sWeb->rebootAfterReply = true;
}

/*
 * Install the newer release the update check found, from GitHub. The same
 * call the menu's offer makes. The answer goes out first; the download runs
 * on the loop's next pass and the panel shows its progress.
 */
static void handleUpdateInstall(void) {
  if (!requireAuth(false)) {
    return;
  }
  switch (updateCheckInstall()) {
    case UPDATE_INSTALL_STARTING:
      sendResult(202,
                 "The radio is downloading the new firmware from GitHub and "
                 "writing it. Its screen shows the progress, and it restarts "
                 "into the new firmware when it is done.");
      return;
    case UPDATE_INSTALL_ON_TRIAL:
      sendResult(409, kOnTrial);
      return;
    case UPDATE_INSTALL_BUSY:
      sendResult(409, kScanRunning);
      return;
    case UPDATE_INSTALL_NOTHING:
    default:
      sendResult(409, "No newer release was found at the check of this start.");
      return;
  }
}

static void handleNotFound(void) {
  /* A script on an API path that is not there, or with a method the path
   * does not take, is told so. Sent to the home page, it would get a page and
   * a 200 once the redirect is followed, which reads as success. */
  if (sWeb->server.uri().startsWith("/api/")) {
    sWeb->server.send(
        404, "text/plain",
        "Nothing on the API answers this. Check the path, and whether "
        "it takes GET or POST.\n");
    return;
  }
  sWeb->server.sendHeader("Location", "/");
  sWeb->server.send(302, "text/plain", "");
}

/* --------------------------------------------------- start up and the loop */

void webBegin(Settings *settings, uint32_t accessPin) {
  sWeb->settings = settings;
  webAuthBegin(accessPin);

  /* Content-Type is collected because handleUploadData needs it to tell a
   * real upload from a raw POST body, the next three because GET /api/live
   * answers a WebSocket handshake, and If-None-Match because the page is not
   * sent again to a browser that has it. */
  const char *keep[] = {"Cookie",
                        "Content-Type",
                        "Upgrade",
                        "Sec-WebSocket-Key",
                        "Sec-WebSocket-Version",
                        "If-None-Match"};
  sWeb->server.collectHeaders(keep, sizeof(keep) / sizeof(keep[0]));

  /* Every request the server has parsed is counted once, before its route
   * or the not-found handler runs. */
  sWeb->server.addMiddleware([](WebServer &, Middleware::Callback next) {
    sWeb->requests++;
    sLastRequestMs = millis();
    return next();
  });

  webAuthRegisterRoutes(sWeb);
  sWeb->server.on("/update", HTTP_POST, handleUploadDone, handleUploadData);
  sWeb->server.on("/reboot", HTTP_POST, handleReboot);
  sWeb->server.on("/update/install", HTTP_POST, handleUpdateInstall);
  webPageRegisterRoutes(sWeb);
  webApiRegisterRoutes(sWeb);
  webStateRegisterRoutes(sWeb);
  webLiveRegisterRoutes(sWeb);
  sWeb->server.onNotFound(handleNotFound);
  sListening = settings == NULL || settings->webEnabled;
  if (sListening) {
    sWeb->server.begin();
  }
}

bool webLastRequestMs(uint32_t *atMs) {
  if (sWeb->requests == 0 || atMs == NULL) {
    return false;
  }
  *atMs = sLastRequestMs;
  return true;
}

void webLoop(void) {
  /* Off in the settings: nothing listens, so nothing on the network can
   * change the radio or update it. Followed here, so the switch acts
   * whichever way it was changed. */
  const bool wanted = sWeb->settings == NULL || sWeb->settings->webEnabled;
  if (wanted != sListening) {
    sListening = wanted;
    if (wanted) {
      sWeb->server.begin();
    } else {
      dropSession();
      webLiveCloseAll();
      sWeb->server.stop();
    }
    DebugLog.println(wanted ? F("[web] listening") : F("[web] off"));
  }
  if (!sListening) {
    return;
  }
  /* Nothing is served while a level sweep runs, about 4 s on FM and 7.5 s on
   * MW: a reply going out raises the channel being read by up to 25 dB, and a
   * request made then is answered just after, inside the pages' 10 s wait. */
  if (radioSweepBusy()) {
    return;
  }
  const uint32_t before = sWeb->requests;
  const uint32_t startMs = millis();
  /* An upload's flags belong to the request that set them: the web server
   * runs its upload and its reply in one call. One whose body broke off
   * never reaches `handleUploadDone` to clear them, and the next request,
   * which may carry no file and no PIN, must not be taken for it. */
  sUploadStarted = false;
  sUploadRejected = false;
  sUploadBlocked = NULL;
  sUploadShaWrong = NULL;
  radioSetNetServing(true);
  sWeb->server.handleClient();
  radioSetNetServing(false);
  if (sWeb->requests != before) {
    replyTimesNote(startMs, millis());
  }
  webLiveLoop();

  if (sWeb->rebootAfterReply) {
    sWeb->rebootAfterReply = false;
    DebugLog.println("[web] rebooting on request");
    /* Closed now, so each page hears the radio went and falls back to
     * asking, rather than showing the last state as live. */
    webLiveCloseAll();
    Serial.flush();
    /* The firmware upload sets the same flag. Its save in here writes
     * nothing, because the radio has been parked since the transfer started
     * and cannot have moved. */
    settingsTaskRestart();
  }
}
