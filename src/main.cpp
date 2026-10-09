/*
 * Start up, in the one order that works, and then the loop that runs core 1.
 *
 * `setup` is a sequence with real constraints in it, and each step below says
 * what its own is. The shape of it: the panel comes up first so there is
 * something to look at, the battery is read before anything starts Wi-Fi, the
 * tuner is patched before the radio task takes it, and the network is asked
 * for last because nothing waits on it.
 *
 * `loop` is core 1. The radio task owns the tuner on core 0 and is started
 * from here, and the two only meet through a queue and a locked snapshot.
 */
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "debug_log.h"

#include "band_scan_task.h"
#include "board/board.h"
#include "core/access_pin.h"
#include "core/autosave.h"
#include "core/backlight.h"
#include "core/band_plan.h"
#include "core/input.h"
#include "core/radio.h"
#include "core/settings.h"
#include "core/squelch.h"
#include "core/strings.h"
#include "core/version.h"
#include "drivers/analog.h"
#include "drivers/battery_adc.h"
#include "drivers/device_id.h"
#include "drivers/logbook_fs.h"
#include "drivers/power.h"
#include "drivers/settings_nvs.h"
#include "drivers/tef668x.h"
#include "dx_task.h"
#include "input_task.h"
#include "memory_store.h"
#include "menu_task.h"
#include "net/boot_watchdog.h"
#include "net/ntp.h"
#include "net/ota_service.h"
#include "net/restart_reason.h"
#include "net/rollback.h"
#include "net/update_check.h"
#include "net/web_update.h"
#include "net/wifi_manager.h"
#include "net/xdr_server.h"
#include "radio_task.h"
#include "recovery.h"
#include "scope_task.h"
#include "screen_task.h"
#include "settings_task.h"
#include "sleep_task.h"

/*
 * The loop's stack. The framework gives 8192 bytes, and the loop never went
 * deeper than 5504 of them, but the install of an update from GitHub runs a
 * TLS handshake on this stack, which the 2688 bytes left would not hold.
 * 12 KB keeps a quarter of the stack unreached with the handshake on top, at
 * a cost of 4 KB of heap.
 */
SET_LOOP_TASK_STACK_SIZE(12 * 1024);

/* The live settings, loaded once at boot and written back when they change. */
static Settings gSettings;

/* The PIN this radio is using. */
static uint32_t gAccessPin = 0;

/* Whether the stored settings were read back. False means they were lost. */
static bool gSettingsLoaded = false;

bool settingsWereLoaded(void) {
  return gSettingsLoaded;
}

/*
 * The tone the radio comes up with, and how long it lasts.
 *
 * Longer than any of the other beeps, so that it is a chime rather than a
 * tick and is not mistaken for a key press. The pitch is the same 2000 Hz the
 * rest use, which is the reference firmware's figure on this chip.
 */
#define START_BEEP_HZ 2000
/* How long, in milliseconds. */
#define START_BEEP_MS 400
/* How loud, in tenths of a dB below full scale. */
#define START_BEEP_AMPLITUDE (-50)

/* How the tuner start up went, so the banner and the web page can say. */
static Tef668xError gTunerError = TEF668X_ERR_NOT_READY;

Tef668xError tunerStartError(void) {
  return gTunerError;
}

/*
 * What the radio is, printed on the serial port at every start.
 *
 * The access PIN is printed in full, here and in the flash commands below, on
 * purpose. Only the USB cable reaches this output, and it is the way to get
 * back a forgotten PIN.
 */
static void printBanner(void) {
  char pin[ACCESS_PIN_DIGITS + 1];
  accessPinFormat(gAccessPin, pin);

  uint8_t mac[6];
  deviceMacRead(mac);

  DebugLog.println();
  DebugLog.println(F("tef668x-esp32"));
  DebugLog.printf("  board          %s\n", BOARD_NAME);
  DebugLog.printf("  firmware       %s\n", FIRMWARE_VERSION);
  DebugLog.printf("  running from   %s (%s)\n", rollbackRunningPartition(),
                  rollbackStateText());
  DebugLog.printf("  last restart   %s\n", restartReasonText());
  DebugLog.printf("  mac            %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0],
                  mac[1], mac[2], mac[3], mac[4], mac[5]);
  const Tef668xCapabilities *tuner = tef668xCapabilities();
  if (tuner != NULL) {
    DebugLog.printf("  tuner          %s, patch v%u\n", tuner->part,
                    (unsigned)tuner->patchVersion);
    DebugLog.printf("  tuner words    device %04X hw %04X sw %04X\n",
                    tuner->deviceWord, tuner->hardwareWord,
                    tuner->softwareWord);
    DebugLog.printf("  tuner can do   %s%s%s\n",
                    tuner->hasStereoImprovement ? "stereo improvement " : "",
                    tuner->hasFullSearchRds ? "full search RDS " : "",
                    tuner->hasDigitalRadio ? "digital radio" : "");
  } else {
    DebugLog.printf("  tuner          FAILED: %s\n",
                    tef668xErrorText(gTunerError));
  }
  DebugLog.printf(
      "  access pin     %s%s\n", pin,
      accessPinIsDefault(gAccessPin) ? "   <- still the default" : "");
  if (accessPinIsDefault(gAccessPin)) {
    DebugLog.println();
    DebugLog.println(F("  WARNING: this radio is on the default access PIN."));
    DebugLog.println(
        F("  Anyone who can reach it on the network can change its"));
    DebugLog.println(
        F("  settings and replace its firmware. Set your own PIN on"));
    DebugLog.println(F("  the web page to stop that."));
  }

  /*
   * Four cases, not two. `setup` does not wait for the join, so it is still
   * running when this prints, and joining is the ordinary case. Printing an
   * address of 0.0.0.0 and a flash command built from it would read as a
   * radio that joined nothing.
   */
  if (wifiState() == WIFI_STATE_ACCESS_POINT) {
    DebugLog.printf("  access point   %s, open\n", wifiNetworkName());
    DebugLog.printf("  setup page     http://%s:%u/\n", wifiAddress(),
                    (unsigned)WEB_PORT);
    DebugLog.println(
        F("  no network yet. Join that access point and set the "
          "Wi-Fi details."));
  } else if (wifiState() == WIFI_STATE_JOINING) {
    DebugLog.printf("  network        joining %s\n", gSettings.wifiSsid);
    DebugLog.println();
    DebugLog.println(
        F("  the address is not known yet. The radio prints it as\n"
          "  [wifi] joined ... when the join lands, and puts up its own\n"
          "  access point if it does not."));
  } else if (wifiState() == WIFI_STATE_OFFLINE) {
    DebugLog.println(F("  network        none, and the access point did not"));
    DebugLog.println(
        F("  start either, so nothing can reach this radio. It tries "
          "again on its own."));
  } else {
    DebugLog.printf("  network        %s\n", wifiNetworkName());
    DebugLog.printf("  address        http://%s:%u/\n", wifiAddress(),
                    (unsigned)WEB_PORT);
    DebugLog.printf("  mdns           http://%s.local:%u/\n", BOARD_HOSTNAME,
                    (unsigned)WEB_PORT);
    DebugLog.println();
#if FEATURE_OTA
    DebugLog.printf(
        "  flash it again with:\n"
        "    pio run -e ats125 -t upload --upload-port %s\n",
        wifiAddress());
    DebugLog.printf("    the uploader asks for --auth=%s\n", pin);
#else
    /* With no ArduinoOTA listener the way in is the update page's own POST,
     * after signing in with the PIN. */
    DebugLog.printf(
        "  flash it again with:\n"
        "    curl -c jar -d pin=%s http://%s:%u/auth\n"
        "    curl -b jar -F firmware=@.pio/build/ats125/firmware.bin "
        "http://%s:%u/update\n",
        pin, wifiAddress(), (unsigned)WEB_PORT, wifiAddress(),
        (unsigned)WEB_PORT);
#endif
  }
  DebugLog.println();
}

/*
 * Keep the new image on trial until this firmware says otherwise.
 *
 * `initArduino` runs before `setup` and, with `CONFIG_APP_ROLLBACK_ENABLE`
 * on, marks a pending image good there and then unless this hook says to
 * wait. The hook is weak, so a firmware that does not override it can never
 * roll back: the image is already good by the time any line of ours runs, and
 * `cnf` in the state document reads true seconds after an over the air flash.
 *
 * Returning true moves the decision to `net/rollback.cpp`, which is where the
 * rule lives: an image is good once it has joined the stored network and held
 * it, because an image that cannot join cannot be flashed again without the
 * cable.
 *
 * It must be `extern "C"`. The weak symbol it replaces is C, and a C++ name
 * would be mangled, link cleanly, and silently not override anything.
 */
extern "C" bool verifyRollbackLater(void) {
  return true;
}

/*
 * How long a watched task may go without coming back to the chip's task
 * watchdog before the radio restarts, in milliseconds. The loop is watched
 * from the end of setup and the radio task from its first round; the state
 * then gives the restart as "task watchdog".
 *
 * Twice the longest a healthy loop pass can take. The network library waits
 * up to ten seconds on one send to a phone that has dropped off the network,
 * ten tries of one second each, and up to five more for a request that
 * arrives slowly. The radio task's longest round is a level sweep, about 4 s
 * for the 211 channels of FM and 7.5 s for MW's 142; it feeds the watchdog a
 * channel at a time, so a longer one is not taken for a hung task. The
 * framework's own 5 s would restart the radio whenever a phone went out of
 * range in the middle of a reply.
 */
#define TASK_WATCHDOG_MS 30000

void setup() {
  /* First of all, so an image that hangs anywhere below still restarts and
   * gets rolled back instead of needing the cable. */
  bootWatchdogArm();
  /* Before the radio task starts, so its first round is already under the
   * longer limit. Core 0's idle task stays watched, as the framework set it. */
  const esp_task_wdt_config_t watchdog = {TASK_WATCHDOG_MS, 1u << 0, true};
  esp_task_wdt_reconfigure(&watchdog);

  Serial.begin(115200);

  /*
   * The battery pin, primed here and read much further down.
   *
   * Making it an ADC input is what starts its divider charging, and it needs
   * 50 ms after that before it reads the real voltage. Priming it first means
   * the 200 ms below and everything after it covers the settling, so the wait
   * costs the boot nothing and the panel is not held back by it. Reading it
   * here instead would be 50 ms of nothing on a dark screen.
   */
  batteryAdcBegin();
  /* Before the display claims the backlight pin, which sleep left held. */
  powerBegin();

  delay(200);

  /* Before anything can leave a new note, and before the banner reads it. */
  restartReasonBegin();

  /* Next, so the image knows whether it is on trial before anything else can
   * fail. */
  rollbackBegin();

  /* Kept, because a failed load is silent otherwise: the radio comes up on
   * the defaults, the PIN goes back to 000000 and the stored station and
   * calibration are gone, with nothing anywhere saying why. */
  gSettingsLoaded = settingsNvsLoad(&gSettings);

  gAccessPin = gSettings.accessPin;

  /*
   * Recovery, checked once, right here: after the settings load, so a rotation
   * or a Wi-Fi credential change made from this screen has something real to
   * write, and before the panel, the tuner or the network claim anything, so a
   * radio that never returns from here has taken nothing with it that the
   * normal sequence below still needs. It only returns when the knob was not
   * held; every way out of recovery itself is a reboot.
   */
  /* Not after the knob woke the radio from sleep: that press is still down,
   * and it is a wake, not a way into recovery. A power on with the knob held
   * still reaches it. */
  if (!powerWokeFromSleep()) {
    recoveryCheckAndRun(&gSettings);
  }

  /* The panel first, so there is something to look at while the rest starts.
   * It needs nothing else to be up, and a radio that can be heard before
   * anything appears on the screen reads as a fault rather than as a fast
   * start.
   *
   * The light comes up inside this call. Start up carries on for seconds
   * after it, through the tuner patch and the Wi-Fi join, so a fade driven
   * from the loop would leave the panel dark for all of it. */
  BacklightConfig backlight;
  backlightFromSettings(&gSettings, &backlight);
  /*
   * The battery, before anything that could bring Wi-Fi up and before the
   * screen, which seeds itself from this sample.
   *
   * The Wi-Fi driver owns the converter this pin is on, so after it starts a
   * failed read and an empty pin look the same. Taken after `wifiBegin` this
   * is the only sample there would ever be, and it would fail, so the panel
   * would show no battery at all. Order is the whole of it.
   */
  batteryAdcSampleAtBoot();

  /*
   * Before the panel is built, not merely before the first poll: some of
   * what a panel draws is set once at construction and never read again in
   * its own show(), so a theme applied any later would need the rebuild
   * screen.cpp does for a live change anyway, and paying for that on every
   * boot that stores a theme other than the default would be a whole screen
   * torn down and built again for nothing.
   */
  settingsApplyTheme(&gSettings);

  screenTaskBegin(&backlight, gSettings.displayRotation);
  /* Once the panel task has begun. The same call a change makes, so nothing
   * it pushes to the panel can be missed at start up. */
  settingsApplyScreen(&gSettings);

  /*
   * The boot screen is up from here, and every row below is written as the
   * thing itself answers rather than at the end. A screen that fills in as
   * the radio comes up says which step is slow, and which step is the one
   * that did not come back.
   *
   * The settings first, because that read already happened: it has to, since
   * the panel light and the battery display are set from it.
   */
  /*
   * Three answers, not two. A radio that has never stored anything is every
   * radio on its first boot after a flash, and it is perfectly well: the
   * defaults are in use and it says so. A radio whose stored blob cannot be
   * read has lost the PIN, the station and the calibration, and that is the
   * one worth a cross.
   */
  screenTaskBootStep(BOOT_STEP_SETTINGS,
                     gSettingsLoaded || !settingsNvsStored(),
                     gSettingsLoaded ? NULL : txt(STR_BOOT_NEW));
  if (!batteryAdcFitted()) {
    /* No battery on this board, so no row. A cross would say the reading
     * failed, and nothing was asked. */
    screenTaskBootAbsent(BOOT_STEP_BATTERY);
  } else {
    uint16_t bootMv = 0;
    char text[12];
    const bool read = batteryAdcAtBoot(&bootMv);
    if (read) {
      snprintf(text, sizeof(text), txt(STR_COMMON_FMT_VOLTS),
               (unsigned)(bootMv / 1000), (unsigned)((bootMv % 1000) / 10));
    }
    /* The one reading this boot gets, because Wi-Fi owns ADC2 once it starts,
     * so the boot screen is the only place it is ever fresh. */
    screenTaskBootStep(BOOT_STEP_BATTERY, read, read ? text : NULL);
  }

  /* The tuner takes a patch over I2C before it will do anything, so this is
   * where that happens. It is independent of the network, and a failure must
   * not stop the radio being reachable, because being reachable is how a fix
   * gets installed. */
  gTunerError = tef668xBegin();
  if (gTunerError != TEF668X_OK) {
    DebugLog.printf("[tuner] start up failed: %s\n",
                    tef668xErrorText(gTunerError));
  }
  screenTaskBootStep(BOOT_STEP_TUNER, gTunerError == TEF668X_OK, NULL);
  if (gTunerError == TEF668X_OK) {
    /* The part is read off the chip, not printed from a constant, so a part
     * on this line means the I2C conversation happened. The firmware's own
     * version beside it; the patch number is in the Diagnostics menu. */
    const Tef668xCapabilities *caps = tef668xCapabilities();
    if (caps != NULL) {
      char tuner[40];
      snprintf(tuner, sizeof(tuner), txt(STR_BOOT_FMT_TUNER_VERSION),
               caps->part, FIRMWARE_VERSION);
      screenTaskBootTuner(tuner);
    }
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

  /* A tone at start up, once the tuner is ready.
   *
   * It cannot come any earlier. The tone generator is inside the tuner, so
   * there is nothing to sound until the patch has gone in and the chip is
   * active. This is also the last moment it can be done directly: from the
   * next few lines the tuner belongs to the radio task.
   *
   * The order keeps the station inaudible throughout. The tone is started
   * first, which switches the audio path to the generator, and only then is
   * the mute lifted. Coming out, the mute goes back on before the tone is
   * stopped, because stopping it puts the path back on the tuner. Doing
   * either the other way round lets one I2C command's worth of untuned FM
   * noise out, which is what the mute at the end of tef668xBegin exists to
   * prevent. */
  if (gSettings.beepStart != 0 && gTunerError == TEF668X_OK) {
    /* Checked, because a failed volume write leaves the chip on its power up
     * default of full scale and the chime would be far louder than the radio
     * is about to be. */
    bool volumeOk = tef668xSetVolume(startVolume) == TEF668X_OK;
    if (volumeOk && tef668xTone(true, START_BEEP_AMPLITUDE, START_BEEP_HZ,
                                START_BEEP_HZ) == TEF668X_OK) {
      tef668xSetMute(false);
      delay(START_BEEP_MS);
      tef668xSetMute(true);
    }
    /* Whether or not any of that worked, and tried again if it fails.
     * Stopping the tone is also what puts the audio path back on the tuner,
     * and nothing else in this firmware ever writes that register, so a radio
     * left pointing at the generator stays silent until the next reboot. */
    for (int i = 0; i < 3; i++) {
      if (tef668xTone(false, 0, 0, 0) == TEF668X_OK) {
        break;
      }
      delay(5);
    }
  }

  /* Before the radio task, because it looks the stored list up on its first
   * round to say which slot the radio came up on. */
  const bool memoryOk = memoryStoreBegin();
  DebugLog.printf("[memory] %d stored channels\n", memoryStoreCount());
  {
    /* A list that came back short marks the row failed and still shows the
     * count. Without the cross the row is the same as a list that was always
     * this length, and the number of channels thrown away only lives in the
     * state document, which needs a browser. The count is the value rather
     * than the count and the loss together because the name and a value of
     * "50, 49 lost" touch in this cell at 320x240. */
    char text[12];
    const bool listWhole = memoryOk && memoryStoreCleared() == 0;
    snprintf(text, sizeof(text), "%d", memoryStoreCount());
    screenTaskBootStep(BOOT_STEP_CHANNELS, listWhole, memoryOk ? text : NULL);
  }

  const bool radioOk = radioTaskStart(&gSettings, &plan, startVolume);
  screenTaskBootStep(BOOT_STEP_RADIO, radioOk, NULL);
  if (!radioOk) {
    DebugLog.println(F("[radio] the radio task could not start"));
    /* The tuner was muted at the end of its start up, and the task is what
     * unmutes it. Without this the radio is silent for good, which is worse
     * than the wrong station: a radio making no sound reads as dead. */
    tef668xSetMute(false);
  } else {
    RadioSnapshot snap;
    char text[24];
    if (radioGetSnapshot(&snap)) {
      bandFormatWithUnit(snap.settings.band, snap.settings.freqKHz, text,
                         sizeof(text));
      DebugLog.printf("[radio] task started on %s\n", text);
    }
  }

  /* The knob and the keypad. They post to the same queue the web API uses,
   * so there is one path into the tuner and not two. */
  const bool keypadOk =
      inputBegin((EncoderKind)gSettings.encoderKind,
                 (EncoderDirection)gSettings.encoderDirection);
  if (!keypadOk) {
    DebugLog.println(F("[input] no keypad answered at 0x20, knob only"));
  }
  screenTaskBootStep(BOOT_STEP_KEYPAD, keypadOk, NULL);
#if FEATURE_TOUCH
  {
    /* Read when the input started, a moment ago. */
    InputStatus input;
    inputStatusGet(&input);
    screenTaskBootStep(BOOT_STEP_TOUCH, input.touchChip, NULL);
  }
#else
  /* No touch chip on this board, so no row. */
  screenTaskBootAbsent(BOOT_STEP_TOUCH);
#endif
  /* After inputBegin, which puts the built in figures back. */
  inputSetPotConfig(&pot);

  /*
   * The logbook's own partition, mounted once here rather than on first
   * use, so a hold of ENTER later either has somewhere to write or knows
   * straight away that it does not. Not a boot screen row: unlike the
   * settings blob nothing else on the radio depends on this mounting, so a
   * person who never holds ENTER has no reason to see a row about it.
   *
   * After the boot screen's own rows are already up, not before them:
   * the first boot after a flash formats a partition nothing has used yet,
   * which is not instant, and the panel must not sit dark waiting on it.
   */
  logbookFsBegin();

  /* The radio's half of the settings, once its task exists, and the beeps,
   * the Touch switch and the auto off time, once the input has begun. */
  settingsApplyRadio(&gSettings);
  settingsApplyInput(&gSettings);

  /* From here the radio keeps its own settings up to date. A station tuned
   * and then switched off is otherwise lost, because nothing was ever written
   * unless somebody asked. */
  settingsTaskBegin(&gSettings, AUTOSAVE_IDLE_MS);

  /* The menu. It reads and writes the same settings struct, through the same
   * call the HTTP endpoint uses, so the panel and the browser cannot drift. */
  menuTaskBegin(&gSettings);

  /*
   * Asks for the network and returns. It does not wait for the join, so the
   * loop below starts within a second or two of the tuner being up rather
   * than after twenty seconds of nothing, and mDNS starts inside the manager
   * on the pass the address actually arrives.
   */
  wifiBegin(&gSettings);

  otaBegin(gAccessPin);

  /* After Wi-Fi, because there is nothing to ask until there is a network,
   * and it does not block waiting for one. */
  ntpApply(&gSettings);

  webBegin(&gSettings, gAccessPin);
  xdrServerBegin(&gSettings);
  scopeTaskBegin(&gSettings);
  updateCheckBegin(&gSettings);

  printBanner();

  /* The radio takes the panel from here. Last, so the boot screen is up for
   * everything above it, including the network being asked for. */
  screenTaskBootEnd();

  /* Setup got to the end, so the loop can take over from here. The framework
   * feeds the task watchdog once per pass of the loop from now on. */
  bootWatchdogDisarm();
  enableLoopWDT();
}

/*
 * Something a person is using that the update offer would close, or a scan
 * or a sweep reading the tuner while the check's traffic goes out: the menu,
 * DX mode, the RDS pages, the bandwidth page, the band scope or the touch
 * calibration, or a screen that is saying something. The check, and its
 * offer, wait until the radio screen is up on its own.
 */
static bool updateWouldInterrupt(void) {
  return bandScanActive() || dxTaskScan()->state == DX_SCAN_RUNNING ||
         radioSweepBusy() || menuTaskIsOpen() || screenTaskDxIsOpen() ||
         screenTaskRdsIsOpen() || screenTaskBwIsOpen() ||
         screenTaskScopeIsOpen() || screenTaskKeypadIsOpen() ||
         screenTaskTouchCalIsOpen() || screenTaskSleepShowing() ||
         screenTaskBootShowing() || screenTaskUpdateHolding();
}

/*
 * The loop runs on the stack set at the top of this file. Measure it with
 * `lop` in GET /api/state, the part it has never reached, and keep at least a
 * quarter of it unreached.
 */
void loop() {
  /* First, so a turn of the knob is acted on before anything slower runs. */
  inputPoll();
  screenTaskPoll();
  settingsTaskPoll();
  memoryStorePoll();
  bandScanTaskPoll();
  menuTaskPoll();
  sleepTaskPoll();

  wifiLoop();
  otaLoop(gSettings.webEnabled != 0);
  webLoop();
  xdrServerLoop();
  scopeTaskPoll();
  {
    /* A newer release is offered once the radio screen is up on its own,
     * worked out here in the same pass that opens it, so nothing can open in
     * between and be closed by the offer. */
    const bool busy = updateWouldInterrupt();
    updateCheckLoop(busy);
    if (!busy && updateCheckTakeOffer()) {
      (void)menuTaskOpenUpdateOffer();
    }
  }

  /* An image written over the air is on trial until the radio proves it can
   * still be reached. Being reachable is how a fix gets installed, so it is
   * the right thing to check. */
  if (!otaInProgress()) {
    /* On the network the settings ask for, not merely answering on a
     * hotspot it fell back to. An image that cannot join is an image that
     * cannot be updated over the air, so marking it good would leave the
     * cable as the only way back, which is what the rollback exists to
     * avoid. */
    rollbackTick(wifiOnWantedNetwork());
  }

  delay(2);
}
