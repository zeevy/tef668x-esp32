/* Implementation of the ArduinoOTA listener. */
#include "ota_service.h"

#include "board/board.h"

#include <ArduinoOTA.h>

#include "radio_task.h"

static bool sInProgress = false;
static int sLastPercent = -1;

void otaBegin(const char *password) {
  ArduinoOTA.setHostname(BOARD_HOSTNAME);
  ArduinoOTA.setPassword(password);

  ArduinoOTA.onStart([]() {
    /* Down and muted before the write starts, so an update over the air does
     * not end in a click. This is the route CLAUDE.md says is normally used,
     * so leaving it out would mean the ramp only covered the browser. The
     * radio does not come back from this: an update reboots. */
    radioHush();
    sInProgress = true;
    sLastPercent = -1;
    const char *what =
        ArduinoOTA.getCommand() == U_FLASH ? "firmware" : "filesystem";
    Serial.printf("[ota] update started, writing the %s\n", what);
  });

  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    if (total == 0) {
      return;
    }
    int percent = (int)((done * 100U) / total);
    if (percent != sLastPercent && percent % 10 == 0) {
      sLastPercent = percent;
      Serial.printf("[ota] %d%%\n", percent);
    }
  });

  ArduinoOTA.onEnd([]() {
    sInProgress = false;
    Serial.println("[ota] update written, rebooting into it");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    /* The radio was hushed when the transfer started and there is now no
     * reboot coming, so it has to be let go again. Without this a transfer
     * that breaks halfway leaves a radio that answers every request, reports
     * a station and a good signal, and makes no sound.
     *
     * Only when a transfer really started. An invitation carrying the wrong
     * password fails before onStart, and anyone on the network can send one,
     * so resuming on every error would let a stranger reach the audio path
     * without the PIN. */
    bool wasRunning = sInProgress;
    sInProgress = false;
    if (wasRunning) {
      radioResume();
    }
    const char *reason = "unknown";
    switch (error) {
      case OTA_AUTH_ERROR:
        reason = "wrong password";
        break;
      case OTA_BEGIN_ERROR:
        reason = "could not start, image too big";
        break;
      case OTA_CONNECT_ERROR:
        reason = "connection failed";
        break;
      case OTA_RECEIVE_ERROR:
        reason = "transfer failed";
        break;
      case OTA_END_ERROR:
        reason = "image did not finish";
        break;
      default:
        break;
    }
    Serial.printf("[ota] update failed: %s\n", reason);
  });

  ArduinoOTA.begin();
}

void otaLoop(void) {
  ArduinoOTA.handle();
}

bool otaInProgress(void) {
  return sInProgress;
}
