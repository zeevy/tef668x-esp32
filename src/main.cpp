/**
 * @file main.cpp
 * @brief Phase 0. A minimal image whose only job is to be able to replace
 *        itself.
 *
 * The ATS-125 has an FT232R that is not wired for auto reset, so every serial
 * upload needs someone at the radio holding BOOT and tapping RESET. This image
 * goes on over the cable once. After that it can be replaced over Wi-Fi, from
 * PlatformIO or from a browser, and a bad image puts the old one back on its
 * own.
 *
 * There is no display code, no tuner and no menu here on purpose.
 */
#include <Arduino.h>
#include <ESPmDNS.h>
#include <WiFi.h>

#include "board/board.h"
#include "core/access_pin.h"
#include "core/band_plan.h"
#include "core/input.h"
#include "core/radio.h"
#include "core/settings.h"
#include "core/squelch.h"
#include "core/version.h"
#include "drivers/analog.h"
#include "drivers/device_id.h"
#include "drivers/settings_nvs.h"
#include "drivers/tef668x.h"
#include "input_task.h"
#include "net/boot_watchdog.h"
#include "net/ota_service.h"
#include "net/rollback.h"
#include "net/web_update.h"
#include "net/wifi_manager.h"
#include "radio_task.h"
#include "screen_task.h"

/** The live settings, loaded once at boot and written back when they change. */
static Settings gSettings;

/** The PIN this radio is using. */
static uint32_t gAccessPin = 0;

/** How the tuner start up went, so the banner and the web page can say. */
static Tef668xError gTunerError = TEF668X_ERR_NOT_READY;

Tef668xError tunerStartError(void) {
  return gTunerError;
}

/** Print everything someone standing at the serial port needs to know. */
static void printBanner(void) {
  char pin[ACCESS_PIN_DIGITS + 1];
  accessPinFormat(gAccessPin, pin);

  uint8_t mac[6];
  deviceMacRead(mac);

  Serial.println();
  Serial.println(F("tef668x-esp32"));
  Serial.printf("  board          %s\n", BOARD_NAME);
  Serial.printf("  firmware       %s\n", FIRMWARE_VERSION);
  Serial.printf("  running from   %s (%s)\n", rollbackRunningPartition(),
                rollbackStateText());
  Serial.printf("  mac            %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],
                mac[1], mac[2], mac[3], mac[4], mac[5]);
  const Tef668xCapabilities *tuner = tef668xCapabilities();
  if (tuner != NULL) {
    Serial.printf("  tuner          %s, patch v%u\n", tuner->part,
                  (unsigned)tuner->patchVersion);
    Serial.printf("  tuner words    device %04X hw %04X sw %04X\n",
                  tuner->deviceWord, tuner->hardwareWord, tuner->softwareWord);
    Serial.printf("  tuner can do   %s%s%s\n",
                  tuner->hasStereoImprovement ? "stereo improvement " : "",
                  tuner->hasFullSearchRds ? "full search RDS " : "",
                  tuner->hasDigitalRadio ? "digital radio" : "");
  } else {
    Serial.printf("  tuner          FAILED: %s\n",
                  tef668xErrorText(gTunerError));
  }
  Serial.printf(
      "  access pin     %s%s\n", pin,
      accessPinIsDefault(gAccessPin) ? "   <- still the default" : "");
  if (accessPinIsDefault(gAccessPin)) {
    Serial.println();
    Serial.println(F("  WARNING: this radio is on the default access PIN."));
    Serial.println(
        F("  Anyone who can reach it on the network can change its"));
    Serial.println(
        F("  settings and replace its firmware. Set your own PIN on"));
    Serial.println(F("  the web page to stop that."));
  }

  if (wifiState() == WIFI_STATE_ACCESS_POINT) {
    Serial.printf("  access point   %s, open\n", wifiNetworkName());
    Serial.printf("  setup page     http://%s/\n", wifiAddress());
    Serial.println(
        F("  no network yet. Join that access point and set the "
          "Wi-Fi details."));
  } else {
    Serial.printf("  network        %s\n", wifiNetworkName());
    Serial.printf("  address        http://%s/\n", wifiAddress());
    Serial.printf("  mdns           http://%s.local/\n", BOARD_HOSTNAME);
    Serial.println();
    Serial.printf(
        "  flash it again with:\n"
        "    pio run -e ats125 -t upload --upload-port %s\n",
        wifiAddress());
    Serial.printf("    the uploader asks for --auth=%s\n", pin);
  }
  Serial.println();
}

/** Bring the radio up. Runs once, and everything here has to finish. */
void setup() {
  /* First of all, so an image that hangs anywhere below still restarts and
   * gets rolled back instead of needing the cable. */
  bootWatchdogArm();

  Serial.begin(115200);
  delay(200);

  /* Next, so the image knows whether it is on trial before anything else can
   * fail. */
  rollbackBegin();

  settingsNvsLoad(&gSettings);

  gAccessPin = gSettings.accessPin;

  /* The panel first, so there is something to look at while the rest starts.
   * It was after the tuner at first, and the radio could be heard before
   * anything appeared on the screen, which reads as a fault rather than as a
   * fast start. The panel needs nothing else to be up. */
  screenTaskBegin();

  /* The tuner takes a patch over I2C before it will do anything, so this is
   * where that happens. It is independent of the network, and a failure must
   * not stop the radio being reachable, because being reachable is how a fix
   * gets installed. */
  gTunerError = tef668xBegin();
  if (gTunerError != TEF668X_OK) {
    Serial.printf("[tuner] start up failed: %s\n",
                  tef668xErrorText(gTunerError));
  }

  /* From here the tuner belongs to the radio task on core 0, and nothing
   * else touches it. Commands go in through a queue and state comes out as a
   * snapshot. */
  BandPlanConfig plan;
  radioPlanFromSettings(&gSettings, &plan);
  /* Where the volume knob is pointing, read before the radio task starts.
   * The task unmutes at the end of its first push, so a volume sent after
   * that is heard as a moment at whatever the default was, which is full. */
  analogBegin();
  /* There is one knob. In manual squelch it is the squelch control, so
   * reading a volume off it would come up at whatever the threshold maps to,
   * which is full volume at one end and silence at the other, and nothing
   * would correct it: the knob never touches the volume in that mode. The
   * stored volume is for that one case. Everywhere else the knob wins. */
  int8_t startVolume = gSettings.startVolumeDb;
  if (gSettings.squelchMode != (uint8_t)SQUELCH_MANUAL) {
    startVolume = potVolumeDb(potRead(), NULL);
  }

  if (!radioTaskStart(&gSettings, &plan, startVolume)) {
    Serial.println(F("[radio] the radio task could not start"));
    /* The tuner was muted at the end of its start up, and the task is what
     * unmutes it. Without this the radio is silent for good, which is worse
     * than the wrong station: a radio making no sound reads as dead. */
    tef668xSetMute(false);
  } else {
    RadioSnapshot snap;
    char text[16];
    if (radioGetSnapshot(&snap)) {
      bandFormatFrequency(snap.settings.band, snap.settings.freqKHz, text,
                          sizeof(text));
      Serial.printf("[radio] task started on %s %s\n", text,
                    bandFrequencyUnit(snap.settings.band));
    }
  }

  /* The knob and the keypad. They post to the same queue the web API uses,
   * so there is one path into the tuner and not two. */
  if (!inputBegin((EncoderKind)gSettings.encoderKind,
                  (EncoderDirection)gSettings.encoderDirection)) {
    Serial.println(F("[input] no keypad answered at 0x20, knob only"));
  }

  wifiBegin(&gSettings);

  if (!MDNS.begin(BOARD_HOSTNAME)) {
    Serial.println(F("[mdns] could not start, use the address instead"));
  } else {
    MDNS.addService("http", "tcp", 80);
  }

  char pin[ACCESS_PIN_DIGITS + 1];
  accessPinFormat(gAccessPin, pin);
  otaBegin(pin);

  webBegin(&gSettings, gAccessPin);

  printBanner();

  /* Setup got to the end, so the loop can take over from here. */
  bootWatchdogDisarm();
}

/** Service the network, the updater and the self check. Runs forever. */
void loop() {
  /* First, so a turn of the knob is acted on before anything slower runs. */
  inputPoll();
  screenTaskPoll();

  wifiLoop();
  otaLoop();
  webLoop();

  /* An image written over the air is on trial until the radio proves it can
   * still be reached. Being reachable is the whole job of this phase, so it
   * is the right thing to check. */
  if (!otaInProgress()) {
    rollbackTick(wifiReachable());
  }

  delay(2);
}
