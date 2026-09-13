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
#include "core/seek.h"
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

/** Whether the stored settings were read back. False means they were lost. */
static bool gSettingsLoaded = false;

bool settingsWereLoaded(void) {
  return gSettingsLoaded;
}

/**
 * The tone the radio comes up with, and how long it lasts.
 *
 * Longer than any of the other beeps, so that it is a chime rather than a
 * tick and is not mistaken for a key press. The pitch is the same 2000 Hz the
 * rest use, which is the reference firmware's figure on this chip.
 */
#define START_BEEP_HZ 2000
/** How long, in milliseconds. */
#define START_BEEP_MS 400
/** How loud, in tenths of a dB below full scale. */
#define START_BEEP_AMPLITUDE (-50)

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

  /* Kept, because a failed load is silent otherwise: the radio comes up on
   * the defaults, the PIN goes back to 000000 and the stored station and
   * calibration are gone, with nothing anywhere saying why. */
  gSettingsLoaded = settingsNvsLoad(&gSettings);

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

  /* What this unit's knob actually reaches, if anybody has ever measured it.
   * Built before the start volume is read, because that reading goes through
   * the same mapping. */
  PotConfig pot;
  potDefaults(&pot);
  if (gSettings.potRawMax != 0) {
    potApplyCalibration(&pot, gSettings.potRawMin, gSettings.potRawMax);
  }
  /* There is one knob. In manual squelch it is the squelch control, so
   * reading a volume off it would come up at whatever the threshold maps to,
   * which is full volume at one end and silence at the other, and nothing
   * would correct it: the knob never touches the volume in that mode. The
   * stored volume is for that one case. Everywhere else the knob wins. */
  int8_t startVolume = gSettings.startVolumeDb;
  if (gSettings.squelchMode != (uint8_t)SQUELCH_MANUAL) {
    startVolume = potVolumeDb(potRead(), &pot);
  }

  /* The radio says it is awake, before it says anything else.
   *
   * It cannot come any earlier. The tone generator is inside the tuner, so
   * there is nothing to beep with until the patch has gone in and the chip is
   * active. This is also the last moment it can be done directly: from the
   * next few lines the tuner belongs to the radio task.
   *
   * The output is muted at the end of tef668xBegin, so the mute comes off for
   * the tone and goes back on afterwards. Nothing of the station is heard in
   * between, because the audio path is switched to the generator for the
   * length of the tone and back at the end of it. */
  if (gSettings.beepStart != 0 && gTunerError == TEF668X_OK) {
    tef668xSetVolume(startVolume);
    tef668xSetMute(false);
    if (tef668xTone(true, START_BEEP_AMPLITUDE, START_BEEP_HZ, START_BEEP_HZ) ==
        TEF668X_OK) {
      delay(START_BEEP_MS);
    }
    /* Whether or not the tone started. Turning it off is also what puts the
     * audio path back on the tuner, so skipping it after a failure is how a
     * radio ends up silent. */
    tef668xTone(false, 0, 0, 0);
    tef668xSetMute(true);
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
  /* After inputBegin, which puts the built in figures back. */
  inputSetPotConfig(&pot);

  /* How fussy seek is, which is stored per band. Set after the task exists,
   * because it is held under the task's lock. */
  SeekConfig seekCfg;
  seekDefaults(&seekCfg);
  seekCfg.fmSensitivity = gSettings.fmScanSensitivity;
  seekCfg.amSensitivity = gSettings.amScanSensitivity;
  radioSetSeekConfig(&seekCfg);

  /* The polish, all of it switchable off in the settings. */
  radioSetSoftMuteMs(gSettings.softMuteMs);
  radioSetEdgeBeep(gSettings.beepEdge != 0);
  inputSetBeeps((BeepMode)gSettings.beepKey);

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
    /* Joined the stored network, not merely answering on its own access
     * point. An image that cannot join is an image that cannot be updated
     * over the air, so marking it good would leave the cable as the only way
     * back, which is what the rollback exists to avoid. */
    rollbackTick(wifiJoinedNetwork());
  }

  delay(2);
}
