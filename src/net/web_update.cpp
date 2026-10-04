/*
 * The server itself: the context every other web_*.cpp file is handed, the
 * vendored assets, the firmware upload, and the loop that carries out a
 * reboot once a reply has gone out.
 */

#include "web_update.h"

#include "web_internal.h"

#include "firmware_write.h"
#include "net/restart_reason.h"
#include "net/rollback.h"
#include "net/web_assets.h"
#include "radio_task.h"
#include "reply_times.h"
#include "settings_task.h"

#include <Update.h>

static WebServer sServerItself(WEB_PORT);
/* The one context every web file is handed; see web_internal.h. */
static WebContext sContext = {sServerItself, NULL, 0, false};
static WebContext *const sWeb = &sContext;
/* Whether the server is listening, which follows `webEnabled`. */
static bool sListening = false;

/* Set while an upload is running and the client was not signed in. */
static bool sUploadRejected = false;

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

static void sendVendored(const char *contentType, const uint8_t *gz,
                         size_t gzLen) {
  sWeb->server.sendHeader("Content-Encoding", "gzip");
  sWeb->server.sendHeader("Cache-Control",
                          "public, max-age=31536000, immutable");
  sWeb->server.send_P(200, contentType, (PGM_P)gz, gzLen);
}

static void handlePicoCss(void) {
  sWeb->requests++;
  sendVendored("text/css", kPicoCssGz, kPicoCssGzLen);
}

static void handleHtmxJs(void) {
  sWeb->requests++;
  sendVendored("application/javascript", kHtmxJsGz, kHtmxJsGzLen);
}

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
    sUploadStarted = true;
    sUploadFailed = false;
    sUploadEnded = false;
    sUploadWritten = false;
    if (sUploadRejected) {
      Serial.printf("[web] upload refused, no PIN, from %s\n",
                    sWeb->server.client().remoteIP().toString().c_str());
      return;
    }
    Serial.printf("[web] upload started: %s\n", upload.filename.c_str());
    sUploadTotal = sWeb->server.clientContentLength();
    firmwareWriteBegin();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Update.printError(Serial);
      /* Marked and left. The radio and the panel are given back at the end of
       * the transfer and nowhere else, for the reason written over the end
       * branch below. */
      sUploadFailed = true;
    }
    return;
  }

  if (sUploadRejected) {
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
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
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
      if (Update.isRunning()) {
        Update.abort();
      }
      firmwareWriteEnd(false);
      return;
    }
    if (Update.end(true)) {
      Serial.printf("[web] upload finished, %u bytes written\n",
                    (unsigned)upload.totalSize);
      /* Held until the reboot, which `handleUploadDone` asks for once the
       * reply is out. */
      sUploadWritten = true;
      firmwareWriteEnd(true);
    } else {
      Update.printError(Serial);
      firmwareWriteEnd(false);
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
    Serial.println("[web] upload aborted");
    firmwareWriteEnd(false);
  }
}

static void handleUploadDone(void) {
  sWeb->requests++;
  bool rejected = sUploadRejected;
  bool sawImage = sUploadStarted;
  sUploadRejected = false;
  sUploadStarted = false;

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
    sendResult(400, "Nothing uploaded", "No firmware file was sent.", true);
    return;
  }

  if (Update.hasError()) {
    sendResult(500, "Update failed",
               "The image was not accepted. The radio is still running the "
               "old one.",
               true);
    return;
  }

  sendResult(200, "Update written",
             "The radio is rebooting into the new image. If it does not come "
             "up and pass its self check, the old image comes back on its "
             "own.",
             false);
  /* Noted here rather than where the restart happens, because that one line
   * serves this and the reboot button both, and telling them apart is the
   * whole reason the note exists. */
  restartReasonNote(RESTART_WHY_UPDATE);
  sWeb->rebootAfterReply = true;
}

static void handleReboot(void) {
  sWeb->requests++;
  if (!requireAuth(false)) {
    return;
  }
  /* A new image is on trial until its self check passes. Restarting it now
   * is exactly what the bootloader reads as a failed image, so the old one
   * would come back with nothing to say why. */
  if (rollbackPending()) {
    sendResult(409, "Not restarted",
               "The new firmware is still proving itself, and a restart now "
               "would put the old one back. Wait until cnf in /api/state is "
               "true, about ten seconds after the radio joins the network.",
               true);
    return;
  }
  sendResult(200, "Rebooting", "The radio is restarting.", false);
  restartReasonNote(RESTART_WHY_ASKED);
  sWeb->rebootAfterReply = true;
}

static void handleNotFound(void) {
  sWeb->requests++;
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
   * real upload from a raw POST body. */
  const char *keep[] = {"Cookie", "Content-Type"};
  sWeb->server.collectHeaders(keep, 2);

  sWeb->server.on("/pico.min.css", HTTP_GET, handlePicoCss);
  sWeb->server.on("/htmx.min.js", HTTP_GET, handleHtmxJs);
  webAuthRegisterRoutes(sWeb);
  sWeb->server.on("/update", HTTP_POST, handleUploadDone, handleUploadData);
  sWeb->server.on("/reboot", HTTP_POST, handleReboot);
  webPagesRegisterRoutes(sWeb);
  webApiRegisterRoutes(sWeb);
  webStateRegisterRoutes(sWeb);
  sWeb->server.onNotFound(handleNotFound);
  sListening = settings == NULL || settings->webEnabled;
  if (sListening) {
    sWeb->server.begin();
  }
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
      sWeb->server.stop();
    }
    Serial.println(wanted ? F("[web] listening") : F("[web] off"));
  }
  if (!sListening) {
    return;
  }
  /* Nothing is served while a level sweep runs, about 4 s: a reply going
   * out raises the channel being read by up to 25 dB, and a request made
   * then is answered just after, inside the pages' 4 s wait. */
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
  radioSetNetServing(true);
  sWeb->server.handleClient();
  radioSetNetServing(false);
  if (sWeb->requests != before) {
    replyTimesNote(startMs, millis());
  }

  if (sWeb->rebootAfterReply) {
    sWeb->rebootAfterReply = false;
    Serial.println("[web] rebooting on request");
    Serial.flush();
    /* The firmware upload sets the same flag. Its save in here writes
     * nothing, because the radio has been parked since the transfer started
     * and cannot have moved. */
    settingsTaskRestart();
  }
}
