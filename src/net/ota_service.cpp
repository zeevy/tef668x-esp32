/**
 * @file ota_service.cpp
 * @brief Implementation of the ArduinoOTA listener.
 */
#include "ota_service.h"

#include "board/board.h"

#include <ArduinoOTA.h>

static bool sInProgress = false;
static int sLastPercent = -1;

void otaBegin(const char *password) {
  ArduinoOTA.setHostname(BOARD_HOSTNAME);
  ArduinoOTA.setPassword(password);

  ArduinoOTA.onStart([]() {
    sInProgress = true;
    sLastPercent = -1;
    const char *what = ArduinoOTA.getCommand() == U_FLASH ? "firmware"
                                                          : "filesystem";
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
    sInProgress = false;
    const char *reason = "unknown";
    switch (error) {
      case OTA_AUTH_ERROR: reason = "wrong password"; break;
      case OTA_BEGIN_ERROR: reason = "could not start, image too big"; break;
      case OTA_CONNECT_ERROR: reason = "connection failed"; break;
      case OTA_RECEIVE_ERROR: reason = "transfer failed"; break;
      case OTA_END_ERROR: reason = "image did not finish"; break;
      default: break;
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
