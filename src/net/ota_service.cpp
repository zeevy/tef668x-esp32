/* Implementation of the ArduinoOTA listener. */
#include "ota_service.h"
#include "debug_log.h"

#include "board/board.h"

#if FEATURE_OTA

#include <ArduinoOTA.h>
#include <ESPmDNS.h>

#include "core/access_pin.h"
#include "firmware_write.h"
#include "net/restart_reason.h"

static bool sInProgress = false;
static int sLastPercent = -1;

/*
 * Wrong PINs on UDP 3232: five, then the listener is closed for a minute.
 * Without a limit, a script gets through six digits in minutes. Its own gate
 * rather than the web page's, because each endpoint has its own limit.
 *
 * ArduinoOTA checks the PIN itself and only reports a wrong one afterwards,
 * so the lock cannot refuse an attempt on its own. Closing the listener is
 * what refuses them: nothing sent to a closed port is read.
 */
static AccessPinGate sGate;
static bool sClosed = false;

void otaBegin(uint32_t pin) {
  char password[ACCESS_PIN_DIGITS + 1];
  accessPinFormat(pin, password);
  ArduinoOTA.setHostname(BOARD_HOSTNAME);
  /* The responder belongs to `wifi_manager.cpp`, which starts it and calls
   * `otaAdvertise` on every path that brings the network up. Left on here,
   * the `end` in `otaSetPin` would stop the whole responder, and the web
   * page would lose its name along with the uploader. */
  ArduinoOTA.setMdnsEnabled(false);
  ArduinoOTA.setPassword(password);

  ArduinoOTA.onStart([]() {
    /* The same as the browser's upload: the station saved, the radio down
     * and muted, and the panel over to the write, so an update over the air
     * does not end in a click. */
    firmwareWriteBegin();
    sInProgress = true;
    sLastPercent = -1;
    /* The PIN was right, so the wrong ones before it no longer count. */
    accessPinGateReset(&sGate);
    const char *what =
        ArduinoOTA.getCommand() == U_FLASH ? "firmware" : "filesystem";
    DebugLog.printf("[ota] update started, writing the %s\n", what);
  });

  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    if (total == 0) {
      return;
    }
    int percent = (int)((done * 100U) / total);
    /* Every step to the panel, which redraws only when the whole number
     * moves. Every tenth to the log, which nobody is watching in real time. */
    firmwareWriteProgress(percent);
    if (percent != sLastPercent && percent % 10 == 0) {
      sLastPercent = percent;
      DebugLog.printf("[ota] %d%%\n", percent);
    }
  });

  ArduinoOTA.onEnd([]() {
    sInProgress = false;
    firmwareWriteEnd(true);
    /* ArduinoOTA restarts the radio itself the moment this returns, so the
     * note has to be left here. Without it the one restart a person is
     * certain to have caused reports the same bare "software" as the ones
     * they are trying to explain. */
    restartReasonNote(RESTART_WHY_UPDATE);
    DebugLog.println("[ota] update written, rebooting into it");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    /* The radio was hushed when the transfer started and there is now no
     * reboot coming, so it is given back, and the panel says so.
     *
     * Only when a transfer really started. An invitation carrying the wrong
     * password fails before onStart, and anyone on the network can send one,
     * so giving back on every error would let a stranger reach the audio
     * path and write UPDATE FAILED on the panel without the PIN. */
    bool wasRunning = sInProgress;
    sInProgress = false;
    if (wasRunning) {
      firmwareWriteEnd(false);
    }
    const char *reason = "unknown";
    switch (error) {
      case OTA_AUTH_ERROR:
        reason = "wrong password";
        /* Two different values, which the gate counts as one wrong try.
         * espota tries each PIN twice, the second time as an MD5 hash, so
         * one upload with a wrong PIN counts two. */
        accessPinGateCheck(&sGate, 0, 1, millis());
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
    DebugLog.printf("[ota] update failed: %s\n", reason);
  });

  ArduinoOTA.begin();
}

void otaSetPin(uint32_t pin) {
  char password[ACCESS_PIN_DIGITS + 1];
  accessPinFormat(pin, password);
  /*
   * A restart of the listener, not `setPassword` alone. Once an invitation
   * has been answered with a challenge, ArduinoOTA waits for the reply with
   * no time limit, and `setPassword` does nothing in that state without
   * saying so. `end` puts it back to idle, so the new password always takes.
   *
   * Safe from here: a web handler and `otaLoop` both run on the loop task,
   * and a transfer runs inside one `handle` call, so nothing is ever cut
   * off mid update.
   */
  ArduinoOTA.end();
  ArduinoOTA.setPassword(password);
  /* The person using the radio just proved the PIN on the web page, so a
   * lock left by wrong guesses at the old one is lifted, as the web page's
   * own gate is. */
  accessPinGateReset(&sGate);
  sClosed = false;
  ArduinoOTA.begin();
  DebugLog.println(F("[ota] the password is now the new PIN"));
}

void otaAdvertise(void) {
  /* 3232 is the ArduinoOTA default port, and the password set above is what
   * makes the advert say authentication is needed. */
  MDNS.enableArduino(3232, true);
}

void otaLoop(bool open) {
  /* Here rather than in the error callback, which runs inside `handle` with
   * the socket still in use. The next pass is soon enough: `handle` reads
   * one packet a call, and this runs before it. */
  bool locked = !open || accessPinGateLocked(&sGate, millis());
  if (locked != sClosed) {
    sClosed = locked;
    if (locked) {
      ArduinoOTA.end();
      DebugLog.println(open ? F("[ota] five wrong PINs, closed for a minute")
                            : F("[ota] closed, the web server is off"));
    } else {
      ArduinoOTA.begin();
      DebugLog.println(F("[ota] open again"));
    }
  }
  /* Does nothing while closed: `end` leaves ArduinoOTA uninitialised. */
  ArduinoOTA.handle();
}

bool otaInProgress(void) {
  return sInProgress;
}

#else /* FEATURE_OTA */

/* ArduinoOTA is off in this board's header. Every flash after the first
 * goes through the web page's /update, so the listener, its RAM and UDP 3232
 * are not built in. */
void otaBegin(uint32_t pin) {
  (void)pin;
}

void otaSetPin(uint32_t pin) {
  (void)pin;
}

void otaAdvertise(void) {}

void otaLoop(bool open) {
  (void)open;
}

bool otaInProgress(void) {
  return false;
}

#endif /* FEATURE_OTA */
