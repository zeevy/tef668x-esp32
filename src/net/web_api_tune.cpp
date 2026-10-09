/*
 * The control API: tuning and the panel's controls: tune, step, band,
 * width, step size, sleep, volume, mute, mode, the volume pot, beep, seek,
 * scan, save and cycle.
 */
#include "debug_log.h"
#include "web_api_internal.h"
#include "web_internal.h"

#include "band_scan_task.h"
#include "dx_task.h"
#include "input_task.h"
#include "settings_task.h"
#include "sleep_task.h"

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

/*
 * GET /api/state. The whole of the radio, open to read.
 *
 * The same document as /status.json, from the same builder.
 */
static void handleApiState(void) {
  sWeb->server.send(200, "application/json", buildState());
}

static void handleApiTune(void) {
  if (!requireAuth(false)) {
    return;
  }
  long khz = 0;
  if (!apiNumber("khz", &khz, 1, 30000000L)) {
    return;
  }

  BandPlanConfig plan;
  if (!radioTaskPlan(&plan)) {
    apiFail(503, "The radio is not running.");
    return;
  }
  BandId band;
  if (!bandForFrequency(&plan, (uint32_t)khz, &band)) {
    apiFail(400, "That frequency is in no band.");
    return;
  }
  /* FM tunes in steps of 10 kHz on this chip, so anything finer is the
   * caller's mistake and never reaches the radio task. */
  if (bandModulation(band) == MODULATION_FM && (khz % 10) != 0) {
    apiFail(400, "The tuner cannot reach that. FM tunes in steps of 10 kHz.");
    return;
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_TUNE;
  cmd.freqKHz = (uint32_t)khz;

  char text[24];
  bandFormatWithUnit(band, (uint32_t)khz, text, sizeof(text));
  apiSubmit(&cmd, String(bandName(band)) + " " + text, API_SAY_TUNE);
}

static void handleApiStep(void) {
  if (!requireAuth(false)) {
    return;
  }
  long steps = 0;
  if (!apiNumber("stp", &steps, -1000, 1000)) {
    return;
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_STEP;
  cmd.steps = (int16_t)steps;

  /* Say where it landed, not just that it moved, so a script can check the
   * answer without a second request. */
  apiSubmit(&cmd, String("stepped ") + steps, API_SAY_TUNE);
}

/* The band `bnd` names, in either case, or BAND_COUNT when it names none. */
static BandId apiBandArg(void) {
  const String want = sWeb->server.arg("bnd");
  for (int b = 0; b < BAND_COUNT; b++) {
    if (want.equalsIgnoreCase(bandName((BandId)b))) {
      return (BandId)b;
    }
  }
  return BAND_COUNT;
}

static void handleApiBand(void) {
  if (!requireAuth(false)) {
    return;
  }
  if (!sWeb->server.hasArg("bnd")) {
    apiFail(400, "Give bnd, one of LW MW SW OIRT FM.");
    return;
  }
  BandId band = apiBandArg();
  if (band == BAND_COUNT) {
    apiFail(400, "That is not a band. Use one of LW MW SW OIRT FM.");
    return;
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_BAND;
  cmd.band = band;
  apiSubmit(&cmd, String("band ") + bandName(band), API_SAY_TUNE);
}

static void handleApiBandwidth(void) {
  if (!requireAuth(false)) {
    return;
  }
  long khz = 0;
  if (!apiNumber("khz", &khz, 0, 6000)) {
    return;
  }
  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_BANDWIDTH;
  cmd.bandwidthKHz = (uint16_t)khz;
  if (sWeb->server.hasArg("bnd")) {
    /* `bnd` sets that band's own width from any band, as the menu's per band
     * rows do: the width it comes up with, and the live one when tuned. */
    cmd.band = apiBandArg();
    if (cmd.band == BAND_COUNT) {
      apiFail(400, "That is not a band. Use one of LW MW SW OIRT FM.");
      return;
    }
    cmd.kind = RADIO_SET_BAND_BANDWIDTH;
    /* Said as asked, not read back: the live width is the tuned band's, which
     * is not this one when it is set from another. */
    apiSubmit(&cmd,
              String("bandwidth ") + bandName(cmd.band) + " " + khz + " kHz");
    return;
  }
  apiSubmit(&cmd,
            khz == 0 ? String("bandwidth automatic")
                     : String("bandwidth ") + khz + " kHz",
            API_SAY_BANDWIDTH);
}

static void handleApiStepSize(void) {
  if (!requireAuth(false)) {
    return;
  }
  long khz = 0;
  if (!apiNumber("khz", &khz, 1, 1000)) {
    return;
  }
  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_STEP;
  cmd.stepKHz = (uint16_t)khz;
  if (!sWeb->server.hasArg("bnd")) {
    apiSubmit(&cmd, String("step ") + khz + " kHz");
    return;
  }
  /* `bnd` sets that band's own step from any band, as the menu's per band
   * rows do: the step it comes up with, and the live one when it is tuned. */
  cmd.band = apiBandArg();
  if (cmd.band == BAND_COUNT) {
    apiFail(400, "That is not a band. Use one of LW MW SW OIRT FM.");
    return;
  }
  cmd.kind = RADIO_SET_BAND_STEP;
  apiSubmit(&cmd, String("step ") + bandName(cmd.band) + " " + khz + " kHz");
}

/* Sleep Now, as the Go To row does. Refused while an update is being
 * written or is on trial, since the wake is a restart and a restart rolls the
 * update back. */
static void handleApiSleep(void) {
  if (!requireAuth(false)) {
    return;
  }
  if (!sleepTaskNow()) {
    apiFail(409,
            "Not while new firmware is being written or is on trial. Try "
            "again once it is confirmed.");
    return;
  }
  sWeb->server.send(200, "text/plain",
                    "Going to sleep. Press the knob to wake the radio.\n");
}

static void handleApiVolume(void) {
  if (!requireAuth(false)) {
    return;
  }
  long db = 0;
  if (!apiNumber("db", &db, RADIO_VOLUME_MIN, RADIO_VOLUME_MAX)) {
    return;
  }
  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_VOLUME;
  cmd.volumeDb = (int8_t)db;
  apiSubmit(&cmd, String("volume ") + db + " dB");
}

static void handleApiMute(void) {
  if (!requireAuth(false)) {
    return;
  }
  long on = 0;
  if (!apiNumber("on", &on, 0, 1)) {
    return;
  }
  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_MUTE;
  cmd.muted = on != 0;
  apiSubmit(&cmd, on ? String("muted") : String("unmuted"), API_SAY_MUTE);
}

static void handleApiMode(void) {
  if (!requireAuth(false)) {
    return;
  }
  if (!sWeb->server.hasArg("mod")) {
    apiFail(400, "Give mod, one of Manual Auto Presets MeterBand.");
    return;
  }
  String want = sWeb->server.arg("mod");
  want.toLowerCase();
  want.replace(" ", "");

  TuneMode mode = TUNE_MODE_COUNT;
  for (int m = 0; m < TUNE_MODE_COUNT; m++) {
    String name = tuneModeName((TuneMode)m);
    name.toLowerCase();
    name.replace(" ", "");
    if (want.equals(name)) {
      mode = (TuneMode)m;
      break;
    }
  }
  if (mode == TUNE_MODE_COUNT) {
    apiFail(400,
            "That is not a tuning mode. Use Manual, Auto, Presets or "
            "MeterBand.");
    return;
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_TUNE_MODE;
  cmd.tuneMode = mode;
  apiSubmit(&cmd, String("mode ") + tuneModeName(mode), API_SAY_MODE);
}

/*
 * POST /api/pot. Learn how far this unit's volume knob turns.
 *
 * Takes `act`: `start`, `finish` or `cancel`.
 *
 * The built in ends of travel, 120 to 4000, are one radio's numbers taken
 * from the reference firmware. This unit reaches 0 and 4095, so they are not
 * wrong here, but a knob that read 200 to 3800 would lose travel at both ends
 * with nothing to say so.
 *
 * Between `start` and `finish` the knob sets neither the volume nor the
 * squelch. It only records how far it goes, because finding the loud end stop
 * should not mean sweeping the volume to full on the way. `GET /api/state`
 * reports `pcl` inside `inp` while it runs, with the lowest and highest seen
 * so far.
 *
 * `finish` refuses a sweep narrower than a quarter of the converter, because
 * a knob that barely moved was not swept end to end and storing what it saw
 * would leave the radio with almost no usable travel.
 */
static void handleApiPot(void) {
  if (!requireAuth(false)) {
    return;
  }
  if (!sWeb->server.hasArg("act")) {
    apiFail(400, "Give act, one of start finish cancel.");
    return;
  }
  String want = sWeb->server.arg("act");
  want.toLowerCase();

  if (want == "start") {
    inputPotCalibrateStart();
    sWeb->server.send(
        200, "text/plain",
        "Turn the volume knob all the way to each end, then finish.\n");
    return;
  }

  if (want == "cancel") {
    inputPotCalibrateCancel();
    sWeb->server.send(200, "text/plain", "Nothing changed.\n");
    return;
  }

  if (want != "finish") {
    apiFail(400, "That is not an action. Use start, finish or cancel.");
    return;
  }

  uint16_t rawMin = 0;
  uint16_t rawMax = 0;
  if (!inputPotCalibrateFinish(&rawMin, &rawMax)) {
    apiFail(400, String("The knob only moved from ") + rawMin + " to " +
                     rawMax +
                     ", which is not a full sweep. Nothing changed. Start "
                     "again and turn it all the way to both ends.");
    return;
  }

  Settings pending = *sWeb->settings;
  pending.potRawMin = rawMin;
  pending.potRawMax = rawMax;
  if (!settingsTaskStore(&pending)) {
    /* The knob is already using the new travel, because that is what made it
     * worth telling the person about. Say plainly that it will not survive a
     * power cycle rather than implying the whole thing failed. */
    apiFail(500, String("The knob now runs ") + rawMin + " to " + rawMax +
                     ", but it could not be stored, so it goes back at the "
                     "next start.");
    return;
  }

  String said =
      String("The knob runs ") + rawMin + " to " + rawMax + ", stored.";
  DebugLog.printf("[api] %s\n", said.c_str());
  sWeb->server.send(200, "text/plain", said + "\n");
}

/*
 * POST /api/beep. Sound the tuner's own tone generator.
 *
 * Takes `ms`, 1 to 3000, and optionally `hz` and `hz2`, each 100 to 15000.
 * The radio beeps for that long and stops on its own.
 *
 * This is here because the tone generator is the one piece of audio hardware
 * on this radio that is not the tuner receiving something, and a beep of
 * fifty milliseconds is not long enough to tell "it did not sound" from "it
 * sounded and I missed it". A long tone answers that in one press.
 *
 * A muted radio stays silent. The tone goes through the same output mute as
 * everything else.
 */
static void handleApiBeep(void) {
  if (!requireAuth(false)) {
    return;
  }
  long ms = 0;
  if (!apiNumber("ms", &ms, 1, 3000)) {
    return;
  }
  long hz = 2000;
  long hz2 = 0;
  if (sWeb->server.hasArg("hz") && !apiNumber("hz", &hz, 100, 15000)) {
    return;
  }
  if (sWeb->server.hasArg("hz2") && !apiNumber("hz2", &hz2, 100, 15000)) {
    return;
  }
  if (hz2 == 0) {
    hz2 = hz;
  }
  if (!radioBeepAt((uint16_t)ms, (uint16_t)hz, (uint16_t)hz2)) {
    apiFail(503, "The radio is busy. Try again in a moment.");
    return;
  }
  sWeb->server.send(200, "text/plain", String("beeping for ") + ms + " ms\n");
}

/*
 * POST /api/seek. Hunt for the next station.
 *
 * Takes `dir`: `up` or `down`. Returns as soon as the seek has started, not
 * when it has finished, because a pass of the FM band takes about ten seconds
 * and holding an HTTP request open for that would tie up the one connection
 * this server has.
 *
 * Watch `skg` in `GET /api/state` to see when it stops, and `skf` to see
 * whether it found anything. Any other command stops it.
 */
static void handleApiSeek(void) {
  if (!requireAuth(false)) {
    return;
  }
  if (!sWeb->server.hasArg("dir")) {
    apiFail(400, "Give dir, up or down.");
    return;
  }
  String want = sWeb->server.arg("dir");
  want.toLowerCase();
  bool up = false;
  if (want == "up") {
    up = true;
  } else if (want != "down") {
    apiFail(400, "That is not a direction. Use up or down.");
    return;
  }

  if (!radioSeek(up)) {
    apiFail(503, "The radio is busy. Try again in a moment.");
    return;
  }
  String said = String("seeking ") + (up ? "up" : "down");
  DebugLog.printf("[api] %s\n", said.c_str());
  sWeb->server.send(200, "text/plain", said + "\n");
}

/*
 * POST /api/scan. Start walking a band, adding a memory channel for
 * anything found that is not already stored nearby. `bnd` is FM, MW, SW or
 * LW, FM when left out; on another band the radio changes to it first and
 * comes back at the end, as the menu's scan rows do.
 *
 * Returns as soon as the scan has started, not when it finishes: a band
 * takes from ten seconds to four minutes, and holding an HTTP request open
 * that long would tie up the one connection this server has, the same
 * reason a seek does not hold the connection either. Watch `scn`, `scd`
 * and `sct` in `GET /api/state` to see it move, and `scf`, `sca` and `scc`
 * once it stops.
 */
static void handleApiScan(void) {
  if (!requireAuth(false)) {
    return;
  }

  /* The DX scanner and the band scan would move one dial and share one
   * mute; the DX scanner is stopped with any key or `scan=0`. */
  if (dxTaskScan()->state == DX_SCAN_RUNNING) {
    apiFail(409, "The DX scanner is running. Stop it first.");
    return;
  }
  BandId band = BAND_FM;
  if (sWeb->server.hasArg("bnd")) {
    band = apiBandArg();
  }
  BandScanStartResult started =
      band < BAND_COUNT ? bandScanStart(band) : BAND_SCAN_NOT_WALKED;
  if (started == BAND_SCAN_NOT_WALKED) {
    apiFail(400, "Give bnd, one of FM MW SW LW, or leave it out for FM.");
    return;
  }
  if (started != BAND_SCAN_STARTED) {
    apiFail(503,
            "The radio is busy, checking for updates, or a scan is "
            "already running.");
    return;
  }
  DebugLog.printf("[api] scan: %s started\n", bandName(band));
  sWeb->server.send(200, "text/plain",
                    String("scanning ") + bandName(band) + "\n");
}

/*
 * POST /api/save. Keep what the radio is set to now.
 *
 * Takes nothing. It reads the radio's own state and writes the parts worth
 * keeping into NVS: the band and frequency, the FM features, the weak signal
 * levels, the noise blankers, the de-emphasis, the AM width and the squelch
 * mode. The radio comes up on all of it next time. While a seek or a band
 * scan is walking the dial, the frequency is the station it started from
 * (settingsBuildCandidate).
 *
 * Why this rather than a stored copy of every setting alongside the live one:
 * two copies of the same thing drift, and then a person has to know which of
 * the two a given page is showing. There is one set of live values, changed
 * through the endpoints that act at once, and this puts them somewhere they
 * survive a power cycle.
 *
 * The volume is not kept. It belongs to the knob, and a stored volume would
 * argue with the knob at every start up.
 */
static void handleApiSave(void) {
  if (!requireAuth(false)) {
    return;
  }

  /* Built by the same call the automatic save uses, so the two can never
   * write different subsets of what the radio is set to.
   *
   * One snapshot, not two. Taking a second one for the reply lets the dial
   * move in between, and the message then names a station that was not the
   * one written. */
  Settings pending;
  if (!settingsBuildCandidate(sWeb->settings, &pending, NULL)) {
    apiFail(503, "The radio is busy. Nothing was saved.");
    return;
  }

  /* The radio can reach states the stored form has no room for, and the AM
   * width on an FM band is one of them. Refusing here beats writing a blob
   * the next start would throw away without saying so. */
  if (!settingsValid(&pending)) {
    apiFail(500, "The radio is in a state that cannot be stored.");
    return;
  }
  /* Through the one call, so the write is counted and an automatic save is
   * not left due the moment this one lands. */
  if (!settingsTaskStore(&pending)) {
    apiFail(500, "The settings could not be written. Nothing changed.");
    return;
  }

  /* From what was written, not from a fresh look at the radio. The reply has
   * to name the station that went into NVS. */
  BandId saved = (BandId)pending.startBand;
  char text[24];
  bandFormatWithUnit(saved, pending.startFreqKHz, text, sizeof(text));
  String said = String("Saved. It will come up on ") + text + ", squelch " +
                squelchModeName((SquelchMode)pending.squelchMode) + ".";
  DebugLog.printf("[api] %s\n", said.c_str());
  sWeb->server.send(200, "text/plain", said + "\n");
}

/*
 * POST /api/cycle. The next one, whatever it is now.
 *
 * Takes `wht`: `band`, `bandwidth`, `mode`, `mute` or `features`. This is
 * what the BAND, BW and MODE buttons and the push on the knob send, so a
 * script can drive the radio the way a hand does. If the panel can do it,
 * the API can do it.
 *
 * `features` walks the four combinations of iMS and the channel equalizer,
 * which is the MODE long press.
 *
 * The radio works out the next value from its own state rather than being
 * told one. A caller that read the state, worked out the next value and sent
 * that would leave a gap for the state to move in.
 */
static void handleApiCycle(void) {
  if (!requireAuth(false)) {
    return;
  }
  if (!sWeb->server.hasArg("wht")) {
    apiFail(400, "Give wht, one of band bandwidth mode mute features.");
    return;
  }
  String want = sWeb->server.arg("wht");
  want.toLowerCase();

  RadioCommand cmd = {};
  ApiSay say = API_SAY_TEXT;
  if (want == "band") {
    cmd.kind = RADIO_CYCLE_BAND;
    say = API_SAY_TUNE;
  } else if (want == "bandwidth") {
    cmd.kind = RADIO_CYCLE_BANDWIDTH;
    say = API_SAY_BANDWIDTH;
  } else if (want == "features") {
    cmd.kind = RADIO_CYCLE_FM_FEATURES;
    say = API_SAY_FEATURES;
  } else if (want == "mode") {
    cmd.kind = RADIO_CYCLE_TUNE_MODE;
    say = API_SAY_MODE;
  } else if (want == "mute") {
    cmd.kind = RADIO_TOGGLE_MUTE;
    say = API_SAY_MUTE;
  } else {
    apiFail(400,
            "That is not something to cycle. Use band, bandwidth, mode, "
            "mute or features.");
    return;
  }
  apiSubmit(&cmd, String("cycled ") + want, say);
}

void webApiTuneRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/api/state", HTTP_GET, handleApiState);
  sWeb->server.on("/api/tune", HTTP_POST, handleApiTune);
  sWeb->server.on("/api/step", HTTP_POST, handleApiStep);
  sWeb->server.on("/api/band", HTTP_POST, handleApiBand);
  sWeb->server.on("/api/bandwidth", HTTP_POST, handleApiBandwidth);
  sWeb->server.on("/api/step-size", HTTP_POST, handleApiStepSize);
  sWeb->server.on("/api/volume", HTTP_POST, handleApiVolume);
  sWeb->server.on("/api/sleep", HTTP_POST, handleApiSleep);
  sWeb->server.on("/api/mute", HTTP_POST, handleApiMute);
  sWeb->server.on("/api/mode", HTTP_POST, handleApiMode);
  sWeb->server.on("/api/cycle", HTTP_POST, handleApiCycle);
  sWeb->server.on("/api/save", HTTP_POST, handleApiSave);
  sWeb->server.on("/api/seek", HTTP_POST, handleApiSeek);
  sWeb->server.on("/api/scan", HTTP_POST, handleApiScan);
  sWeb->server.on("/api/beep", HTTP_POST, handleApiBeep);
  sWeb->server.on("/api/pot", HTTP_POST, handleApiPot);
}
