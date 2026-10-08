/*
 * The control API: DX mode: the mode itself, its level sweep, its catches
 * and their CSV, the AF check series and the RDS lock timings.
 */
#include "web_api_internal.h"
#include "web_internal.h"

#include "band_scan_task.h"
#include "core/dx.h"
#include "core/dx_timing.h"
#include "drivers/dx_timing_fs.h"
#include "drivers/logbook_fs.h"
#include "dx_task.h"
#include "net/ntp.h"
#include "reply_times.h"
#include "scope_task.h"
#include "screen_task.h"

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

/*
 * POST /api/dx. DX mode: `on=1` opens it, `on=0` closes it, `khz` sets its
 * fixed width while it is open, and `log` writes the catch at that place in
 * GET /api/dx's list to the logbook, and `logid` the catch with that `id`
 * wherever it now is. `scan=1` starts the DX scanner and `scan=0` stops it.
 * `learn=1` learns the local stations, opening DX mode first if it is not
 * open. `sweep=1` takes a level sweep, read with GET /api/dx/sweep, and
 * `baseline` sets the sweep baseline: `now` fixes it on the latest sweep and
 * `auto` makes it the median of the kept sweeps. `learn`, `sweep` and
 * `baseline` each go on their own. The same calls the DX, MODE and BW keys
 * and a hold on a Catches row make, so the panel cannot disagree with what
 * the API was told.
 */
static void handleApiDx(void) {
  if (!requireAuth(false)) {
    return;
  }
  const bool sweep = sWeb->server.hasArg("sweep");
  const bool baseline = sWeb->server.hasArg("baseline");
  const bool log = sWeb->server.hasArg("log") || sWeb->server.hasArg("logid");
  if (!sWeb->server.hasArg("on") && !sWeb->server.hasArg("khz") && !log &&
      !sWeb->server.hasArg("scan") && !sWeb->server.hasArg("learn") && !sweep &&
      !baseline) {
    apiFail(400,
            "Give on, 1 or 0, khz, log, logid, scan, 1 or 0, learn=1, "
            "sweep=1, or baseline, now or auto.");
    return;
  }
  if (sweep || baseline) {
    /* The level sweep and its baseline. */
    if (sweep == baseline || sWeb->server.hasArg("on") ||
        sWeb->server.hasArg("khz") || log || sWeb->server.hasArg("scan") ||
        sWeb->server.hasArg("learn")) {
      apiFail(400, "Give sweep or baseline on its own.");
      return;
    }
    if (sweep) {
      long one = 0;
      if (!apiNumber("sweep", &one, 1, 1)) {
        return;
      }
      switch (dxTaskSweep()) {
        case DX_SWEEP_STARTED:
          /* Once it has started, so a refusal leaves the page alone. */
          screenTaskDxShowScope();
          sWeb->server.send(
              200, "text/plain",
              "sweeping, GET /api/dx/sweep has it in about 4 s\n");
          return;
        case DX_SWEEP_CLOSED:
          apiFail(409,
                  "DX mode is not open, so there is no DX width to "
                  "sweep at.");
          return;
        case DX_SWEEP_BUSY:
          apiFail(409,
                  "A sweep or a scan is running. Wait for it, or stop "
                  "the scan.");
          return;
        case DX_SWEEP_NO_MEMORY:
          apiFail(503, "There is no memory for the sweeps.");
          return;
        default:
          apiFail(503,
                  "The radio would not sweep now: it is seeking, off "
                  "FM, restarting, or checking for updates.");
          return;
      }
    }
    const String how = sWeb->server.arg("baseline");
    DxBaseResult r;
    if (how == "now") {
      r = dxTaskBaselineNow();
    } else if (how == "auto") {
      r = dxTaskBaselineAuto();
    } else {
      apiFail(400,
              "baseline is now, to fix the latest sweep, or auto, for "
              "the median of the kept sweeps.");
      return;
    }
    switch (r) {
      case DX_BASE_DONE:
        sWeb->server.send(200, "text/plain",
                          how == "now" ? "baseline fixed on the latest sweep\n"
                                       : "baseline is the median of the kept "
                                         "sweeps\n");
        return;
      case DX_BASE_NO_SWEEP:
        apiFail(409, "There is no sweep yet. Take one with sweep=1.");
        return;
      case DX_BASE_NOT_SAVED:
        apiFail(503, "The storage did not take it. Nothing changed.");
        return;
      default:
        apiFail(503, "There is no memory for it. Nothing changed.");
        return;
    }
  }
  if (sWeb->server.hasArg("learn")) {
    /* Learn the locals, the DX SETUP menu's action: into DX mode if it is
     * not open, the Scanner page up, and the pass running. */
    long learn = 0;
    if (!apiNumber("learn", &learn, 1, 1)) {
      return;
    }
    if (sWeb->server.hasArg("on") || sWeb->server.hasArg("khz") || log ||
        sWeb->server.hasArg("scan")) {
      apiFail(400, "Give learn on its own.");
      return;
    }
    switch (screenTaskDxLearnLocals()) {
      case DX_SCAN_PRESS_RUNNING:
        sWeb->server.send(200, "text/plain", "learning the locals\n");
        return;
      case DX_SCAN_PRESS_NOT_FM:
        apiFail(409, "DX mode is FM only.");
        return;
      case DX_SCAN_PRESS_BUSY:
        apiFail(409, "A scan is running. Stop it first.");
        return;
      case DX_SCAN_PRESS_UPDATE_CHECK:
        apiFail(409,
                "The radio is checking for updates. Try again in a few "
                "seconds.");
        return;
      case DX_SCAN_PRESS_RDS_OFF:
        apiFail(409,
                "The RDS decoder is off, so nothing could be learned. Switch "
                "it on with rds=1 in /api/settings.");
        return;
      case DX_SCAN_PRESS_NO_SEEN:
        apiFail(503,
                "The list of stations caught before could not be read, so "
                "nothing learned could be kept.");
        return;
      case DX_SCAN_PRESS_PANEL_BUSY:
        apiFail(503, "The panel is busy. Close the menu and try again.");
        return;
      case DX_SCAN_PRESS_NO_MEMORY:
        apiFail(503, "There is no memory for the scanner.");
        return;
      default:
        apiFail(503, "The radio could not be read. Try again.");
        return;
    }
  }
  if (log) {
    if (sWeb->server.hasArg("on") || sWeb->server.hasArg("khz") ||
        sWeb->server.hasArg("scan") ||
        (sWeb->server.hasArg("log") && sWeb->server.hasArg("logid"))) {
      apiFail(400, "Give log or logid on its own.");
      return;
    }
    /* A place read a moment ago can hold another catch by now, since a new
     * one goes in at the front and a known one heard again moves there.
     * `logid` names the catch itself, the `id` GET /api/dx gives it, so a
     * click on one row never logs another. */
    long at = 0;
    if (sWeb->server.hasArg("logid")) {
      long id = 0;
      if (!apiNumber("logid", &id, 1, 65535)) {
        return;
      }
      at = dxCatchesFindId(dxTaskCatches(), (uint16_t)id);
      if (at < 0) {
        apiFail(404,
                "That catch is not in the list any more. The list shows "
                "what the radio has now.");
        return;
      }
    } else if (!apiNumber("log", &at, 0, DX_CATCHES_MAX - 1)) {
      return;
    }
    switch (dxTaskLogCatch((uint8_t)at)) {
      case DX_WRITE_DONE: {
        /* The list moves as catches come in, so the reply names what was
         * written rather than trusting the place asked for. */
        const DxCatch *k = &dxTaskCatches()->item[at];
        char freq[16] = "";
        bandFormatFrequency((BandId)k->band, k->khz, freq, sizeof(freq));
        char text[40];
        snprintf(text, sizeof(text), "logged %s %04X\n", freq, (unsigned)k->pi);
        sWeb->server.send(200, "text/plain", text);
        return;
      }
      case DX_WRITE_NO_CATCH:
        apiFail(404, "No catch at that place in the list.");
        return;
      case DX_WRITE_NOTHING_NEW:
        apiFail(409,
                "Already logged. Nothing was heard on its channel "
                "since its last entry.");
        return;
      case DX_WRITE_IN_LOG:
        apiFail(409,
                "Already logged. The log holds this station already, on "
                "this channel.");
        return;
      case DX_WRITE_FAILED:
      default:
        apiFail(503, "Not written. The log has nowhere to write to.");
        return;
    }
  }
  if (sWeb->server.hasArg("on")) {
    long on = 0;
    if (!apiNumber("on", &on, 0, 1)) {
      return;
    }
    if (on == 0) {
      screenTaskDxClose();
    } else {
      switch (screenTaskDxOpen()) {
        case SCREEN_DX_OPEN:
          break;
        case SCREEN_DX_PANEL_BUSY:
          apiFail(503, "The panel is busy. Close the menu and try again.");
          return;
        case SCREEN_DX_RADIO_BUSY:
        case SCREEN_DX_NOT_FM:
        default:
          apiFail(409, "DX mode is FM only.");
          return;
      }
    }
  }
  if (sWeb->server.hasArg("khz")) {
    long khz = 0;
    if (!apiNumber("khz", &khz, 1, UINT16_MAX)) {
      return;
    }
    if (!screenTaskDxIsOpen()) {
      apiFail(409, "DX mode is not open, so it has no width to set.");
      return;
    }
    if (!screenTaskDxSetWidth((uint16_t)khz)) {
      apiFailFmWidth("khz");
      return;
    }
  }
  if (sWeb->server.hasArg("scan")) {
    long scan = 0;
    if (!apiNumber("scan", &scan, 0, 1)) {
      return;
    }
    if (scan == 0) {
      dxTaskScanStop();
    } else if (!screenTaskDxIsOpen()) {
      apiFail(409, "DX mode is not open, so there is nothing to scan with.");
      return;
    } else if (dxTaskScan()->state == DX_SCAN_RUNNING) {
      apiFail(409, "The scan is already running.");
      return;
    } else if (bandScanActive()) {
      apiFail(409, "A band scan is running. Wait for it to finish.");
      return;
    } else {
      switch (dxTaskScanPress()) {
        case DX_SCAN_PRESS_RUNNING:
        case DX_SCAN_PRESS_FINISHED:
          /* Once it has started, so a refusal leaves the page alone. */
          screenTaskDxShowScanner();
          break;
        case DX_SCAN_PRESS_RDS_OFF:
          apiFail(409,
                  "The RDS decoder is off, and the scanner stops only on a "
                  "PI. Switch it on with rds=1 in /api/settings.");
          return;
        case DX_SCAN_PRESS_NO_MEMORY:
          apiFail(503, "There is no memory for the scanner.");
          return;
        case DX_SCAN_PRESS_NOTHING:
          apiFail(409,
                  "There is no channel to scan: every one is stored, or no "
                  "preset in the range is on this band.");
          return;
        default:
          apiFail(503, "The radio could not be read. Try again.");
          return;
      }
    }
  }
  char text[48];
  const DxScan *scan = dxTaskScan();
  if (screenTaskDxIsOpen()) {
    const char *said = scan->state == DX_SCAN_RUNNING   ? ", scan running"
                       : scan->state == DX_SCAN_STOPPED ? ", scan stopped"
                                                        : "";
    snprintf(text, sizeof(text), "DX on, %u kHz%s",
             (unsigned)screenTaskDxWidth(), said);
  } else {
    snprintf(text, sizeof(text), "DX off");
  }
  sWeb->server.send(200, "text/plain", String(text) + "\n");
}

/* One array of a sweep's levels, tenths of a dBuV, null for a channel with
 * no reading, or `minus`'s level taken away when it is given. */
static void sendSweepLevels(const char *name, const DxSweep *s,
                            const DxSweep *minus) {
  char chunk[256];
  size_t used = (size_t)snprintf(chunk, sizeof(chunk), ",\"%s\":", name);
  if (s == NULL) {
    sWeb->server.sendContent(String(chunk) + "null");
    return;
  }
  chunk[used++] = '[';
  chunk[used] = '\0';
  for (uint16_t i = 0; i < s->count; i++) {
    char one[12];
    const int16_t v = s->level[i];
    const bool none = v == DX_SWEEP_NO_READING ||
                      (minus != NULL && minus->level[i] == DX_SWEEP_NO_READING);
    if (none) {
      snprintf(one, sizeof(one), "%snull", i ? "," : "");
    } else {
      snprintf(one, sizeof(one), "%s%d", i ? "," : "",
               minus != NULL ? (int)v - (int)minus->level[i] : (int)v);
    }
    const size_t len = strlen(one);
    if (used + len + 2 > sizeof(chunk)) {
      sWeb->server.sendContent(chunk);
      used = 0;
      chunk[0] = '\0';
    }
    memcpy(chunk + used, one, len + 1);
    used += len;
  }
  chunk[used++] = ']';
  chunk[used] = '\0';
  sWeb->server.sendContent(chunk);
}

/*
 * GET /api/dx/sweep. The latest level sweep: when it was taken
 * and at what width, its channels from `low` in `step`, the noise floor,
 * and four arrays of `count` levels in tenths of a dBuV, null where a
 * channel has no reading: `level`, the baseline it is held against,
 * `rise` over it, and the `peak` held since DX mode opened. The baseline is
 * the median of `baseline_sweeps` kept sweeps before this one, or one fixed
 * by hand; with none, it and `rise` are null rather than 0. Open to read:
 * a level is already public on /api/state.
 */
static void handleApiDxSweepGet(void) {
  DxSweepState v;
  if (!dxTaskSweepView(&v)) {
    apiFail(503, "There is no memory for the sweeps.");
    return;
  }
  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "application/json", "");
  char head[256];
  if (v.live == NULL) {
    snprintf(head, sizeof(head),
             "{\"rev\":%u,\"running\":%s,\"abandoned\":%s,\"time\":null,"
             "\"real\":false,\"count\":0",
             (unsigned)v.revision, v.running ? "true" : "false",
             v.abandoned ? "true" : "false");
    sWeb->server.sendContent(head);
  } else {
    const int16_t floor = dxSweepFloor(v.live);
    char floorText[8] = "null";
    if (floor != DX_SWEEP_NO_READING) {
      snprintf(floorText, sizeof(floorText), "%d", (int)floor);
    }
    const char *base = v.base == NULL ? "null"
                       : v.baseFixed  ? "\"fixed\""
                                      : "\"median\"";
    snprintf(head, sizeof(head),
             "{\"rev\":%u,\"running\":%s,\"abandoned\":%s,\"time\":%u,"
             "\"real\":%s,\"took_ms\":%u,\"width\":%u,\"low\":%u,"
             "\"step\":%u,\"count\":%u,\"floor\":%s,\"baseline\":%s,"
             "\"baseline_sweeps\":%u,\"lvo\":%d",
             (unsigned)v.revision, v.running ? "true" : "false",
             v.abandoned ? "true" : "false", (unsigned)v.live->at,
             v.live->timeKnown ? "true" : "false", (unsigned)v.live->tookMs,
             (unsigned)v.live->widthKHz, (unsigned)v.live->lowKHz,
             (unsigned)v.live->stepKHz, (unsigned)v.live->count, floorText,
             base, (unsigned)v.baseN, (int)screenTaskLevelOffsetDb(BAND_FM));
    sWeb->server.sendContent(head);
    sendSweepLevels("level", v.live, NULL);
    sendSweepLevels("baseline_level", v.base, NULL);
    sendSweepLevels("rise", v.base != NULL ? v.live : NULL, v.base);
    sendSweepLevels("peak", v.peak, NULL);
  }
  sWeb->server.sendContent("}\n");
  sWeb->server.sendContent("");
}

/*
 * POST /api/scope sweep=1: a sweep of the band the radio is on for the band
 * scope, outside DX mode too, FM through DX mode's width and AM through the
 * radio's own. `span` in kHz, 0 or left out for the whole band, else that
 * span round the dial; the whole of SW is too many channels, so SW takes a
 * span. It returns at once; GET /api/scope has it once `running` is false.
 * Muted while it runs, about 4 s for the whole FM band and 7.5 s for MW, and
 * the web server answers nothing then.
 */
static void handleApiScopePost(void) {
  if (!requireAuth(false)) {
    return;
  }
  long sweep = 0;
  long span = 0;
  if (!apiNumber("sweep", &sweep, 1, 1) ||
      (sWeb->server.hasArg("span") && !apiNumber("span", &span, 0, 30000))) {
    return;
  }
  switch (scopeTaskSweep((uint32_t)span)) {
    case SCOPE_STARTED:
      sWeb->server.send(200, "text/plain", "sweeping\n");
      return;
    case SCOPE_NO_MEMORY:
      apiFail(503, "There is no memory for the sweeps.");
      return;
    case SCOPE_TOO_WIDE:
      apiFail(400,
              "More channels than a sweep holds, 431. On SW ask for a span, "
              "for example span=360.");
      return;
    default:
      apiFail(409,
              "Not started: a sweep, a check, a band scan or the update check "
              "is under way, or a seek runs.");
      return;
  }
}

/* GET /api/scope: the band scope's last sweep, as GET /api/dx/sweep gives
 * DX mode's, with no baseline: `band` the band it was swept on, `time` UTC
 * seconds when `real`, `level` in tenths of a dBuV, null for a channel with
 * no reading, and `lvo` that band's level offset. `floor` only for the whole
 * band, where a quarter of the channels is the noise. Open to read. */
static void handleApiScopeGet(void) {
  ScopeView v;
  scopeTaskView(&v);
  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "application/json", "");
  char head[256];
  const DxSweep *s = v.latest;
  if (s == NULL) {
    snprintf(head, sizeof(head),
             "{\"rev\":%u,\"running\":%s,\"abandoned\":%s,\"time\":null,"
             "\"real\":false,\"count\":0",
             (unsigned)v.revision, v.running ? "true" : "false",
             v.abandoned ? "true" : "false");
    sWeb->server.sendContent(head);
  } else {
    const int16_t floor = v.whole ? dxSweepFloor(s) : DX_SWEEP_NO_READING;
    char floorText[8] = "null";
    if (floor != DX_SWEEP_NO_READING) {
      snprintf(floorText, sizeof(floorText), "%d", (int)floor);
    }
    snprintf(head, sizeof(head),
             "{\"rev\":%u,\"running\":%s,\"abandoned\":%s,\"time\":%u,"
             "\"real\":%s,\"took_ms\":%u,\"width\":%u,\"low\":%u,"
             "\"step\":%u,\"count\":%u,\"whole\":%s,\"floor\":%s,"
             "\"band\":\"%s\",\"lvo\":%d",
             (unsigned)v.revision, v.running ? "true" : "false",
             v.abandoned ? "true" : "false", (unsigned)s->at,
             s->timeKnown ? "true" : "false", (unsigned)s->tookMs,
             (unsigned)s->widthKHz, (unsigned)s->lowKHz, (unsigned)s->stepKHz,
             (unsigned)s->count, v.whole ? "true" : "false", floorText,
             bandName(v.band), (int)screenTaskLevelOffsetDb(v.band));
    sWeb->server.sendContent(head);
    sendSweepLevels("level", s, NULL);
  }
  sWeb->server.sendContent("}\n");
  sWeb->server.sendContent("");
}

/*
 * GET /api/dx. DX mode as the panel has it: whether it is open, the page
 * and the Catches cursor, then this session's catches, newest first, one
 * compact JSON object a line as GET /api/log does. `i` is the place POST
 * /api/dx log takes. Open to read, like the log: a catch is
 * what a station broadcast plus a reading already public on /api/state.
 */
static void handleApiDxGet(void) {
  const DxCatches *list = dxTaskCatches();
  const uint8_t n = list != NULL ? list->count : 0;
  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "application/x-ndjson", "");
  /* Every field at its widest, with an eight character name escaped as
   * \u00XX throughout, is 239 bytes. */
  char line[256];
  const DxScan *scan = dxTaskScan();
  const char *scanState = scan->state == DX_SCAN_RUNNING   ? "running"
                          : scan->state == DX_SCAN_STOPPED ? "stopped"
                                                           : "idle";
  /* Only a scan under way or stopped has a channel; a finished one has
   * put the dial back. */
  const uint32_t scanKHz = scan->state == DX_SCAN_IDLE ? 0 : scan->atKHz;
  snprintf(line, sizeof(line),
           "{\"on\":%s,\"page\":%u,\"cursor\":%u,\"n\":%u,"
           "\"dropped\":%u,\"sweeping\":%s,\"sweep_rev\":%u,"
           "\"scan\":\"%s\",\"scan_khz\":%u,"
           "\"scan_found\":%u,\"scan_passed\":%u,\"scan_total\":%u,"
           "\"lvo\":%d",
           screenTaskDxIsOpen() ? "true" : "false",
           (unsigned)screenTaskDxPage(), (unsigned)screenTaskDxCursor(),
           (unsigned)n, list != NULL ? (unsigned)list->dropped : 0u,
           dxTaskSweepRunning() ? "true" : "false",
           (unsigned)dxTaskSweepRevision(), scanState, (unsigned)scanKHz,
           (unsigned)scan->found, (unsigned)dxScanPassed(scan),
           (unsigned)dxScanTotal(scan), (int)screenTaskLevelOffsetDb(BAND_FM));
  sWeb->server.sendContent(line);
  /* The preset watch, in the same object. */
  uint8_t watched = 0;
  uint32_t upKHz = 0;
  int16_t rise = 0;
  uint32_t upMs = 0;
  const bool watching = dxTaskWatchState(&watched, &upKHz, &rise, &upMs);
  if (upKHz != 0) {
    snprintf(line, sizeof(line),
             ",\"watch\":%s,\"watch_n\":%u,\"watch_up\":%u,"
             "\"watch_rise\":%d,\"watch_ago_ms\":%u}\n",
             watching ? "true" : "false", (unsigned)watched, (unsigned)upKHz,
             (int)rise, (unsigned)(millis() - upMs));
  } else {
    snprintf(line, sizeof(line),
             ",\"watch\":%s,\"watch_n\":%u,\"watch_up\":null}\n",
             watching ? "true" : "false", (unsigned)watched);
  }
  sWeb->server.sendContent(line);
  for (uint8_t i = 0; i < n; i++) {
    const DxCatch *k = &list->item[i];
    char nameField[2 + (LOGBOOK_NAME_LEN - 1) * 6 + 1] = "null";
    if (k->hasPs) {
      snprintf(nameField, sizeof(nameField), "\"%s\"",
               jsonEscape(k->ps).c_str());
    }
    /* Two letters from rds_country's own table, never from the air. */
    char countryField[8] = "null";
    if (k->hasCountry) {
      snprintf(countryField, sizeof(countryField), "\"%s\"", k->country);
    }
    size_t need = (size_t)snprintf(
        line, sizeof(line),
        "{\"i\":%u,\"id\":%u,\"time\":%u,\"real\":%s,\"band\":\"%s\",\"khz\":%"
        "u,"
        "\"pi\":\"%04X\",\"name\":%s,\"country\":%s,\"new\":%s,"
        "\"count\":%u,\"level_dbuv\":%d,\"logged\":%s,\"due\":%s}\n",
        (unsigned)i, (unsigned)k->id, (unsigned)k->last.value,
        k->last.known ? "true" : "false", bandName((BandId)k->band),
        (unsigned)k->khz, (unsigned)k->pi, nameField, countryField,
        k->isNew ? "true" : "false", (unsigned)k->count,
        (int)k->best.levelDbuVTenths, k->logged ? "true" : "false",
        k->pending ? "true" : "false");
    if (need == 0 || need >= sizeof(line)) {
      continue;
    }
    sWeb->server.sendContent(line);
  }
  sWeb->server.sendContent("");
}

/*
 * GET /api/dx.csv. This session's catches in the TEF CSV, dxCatchCsvLine,
 * in the order of their time, oldest first, since the converter matches a
 * GPS track to the rows with a pointer that only moves forward. Rows with
 * no time, which it skips, come first. The file is named
 * for the UTC minute it was taken, so two downloads do not overwrite each
 * other. Open to read, as GET /api/dx is.
 */
static void handleApiDxCsv(void) {
  char disposition[128] = "attachment; filename=\"dx-catches.csv\"";
  uint32_t epoch = 0;
  if (ntpEpochUtc(&epoch)) {
    time_t now = (time_t)epoch;
    struct tm utc;
    if (gmtime_r(&now, &utc) != NULL) {
      snprintf(disposition, sizeof(disposition),
               "attachment; filename=\"dx-catches-%04d%02d%02d-%02d%02dZ.csv\"",
               utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour,
               utc.tm_min);
    }
  }
  sWeb->server.sendHeader("Content-Disposition", disposition);
  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "text/csv; charset=utf-8", "");
  sWeb->server.sendContent(dxCatchCsvHeader());
  uint8_t order[DX_CATCHES_MAX];
  const DxCatches *list = dxTaskCatches();
  const uint8_t n = dxCatchesByTime(list, order);
  char line[128];
  const int8_t levelOffset = screenTaskLevelOffsetDb(BAND_FM);
  for (uint8_t i = 0; i < n; i++) {
    /* The level as shown, with the FM offset; the catch keeps
     * the radio's own. Both readings, since the line takes the timed one
     * or, with none, the strongest. */
    DxCatch k = list->item[order[i]];
    k.timed.levelDbuVTenths =
        signalShownTenths(k.timed.levelDbuVTenths, levelOffset);
    k.best.levelDbuVTenths =
        signalShownTenths(k.best.levelDbuVTenths, levelOffset);
    size_t need = dxCatchCsvLine(&k, line, sizeof(line));
    if (need == 0 || need >= sizeof(line)) {
      continue;
    }
    sWeb->server.sendContent(line);
  }
  sWeb->server.sendContent("");
}

/*
 * POST /api/afcheck: a series of AF_Update checks of `khz` while the radio
 * stays on its station, `n` of them `ms` apart, read through `bw` kHz, the
 * DX width by default. It returns at once; GET /api/afcheck gives the
 * readings once the series has ended. Measures what the preset watch's
 * checks cost: the gap in the audio and the RDS blocks the station loses.
 * `stop=1` ends the series under way.
 */
static RadioAfSeries sAfSeriesStore;

static RadioAfSeries *sAfSeries = NULL; /* Set once a series has run. */
static RadioAfCheck *sAfChecks = NULL;  /* On the heap, taken on first use. */

static void handleApiAfCheckPost(void) {
  if (!requireAuth(false)) {
    return;
  }
  if (sWeb->server.hasArg("stop")) {
    radioAfCancel();
    sWeb->server.send(200, "text/plain", "stopping\n");
    return;
  }
  long khz = 0;
  long n = 0;
  long ms = 0;
  long bw = DX_BANDWIDTH_DEFAULT_KHZ;
  if (!apiNumber("khz", &khz, 65000, 108000) ||
      !apiNumber("n", &n, 1, RADIO_AF_MAX) ||
      !apiNumber("ms", &ms, 10, 10000)) {
    return;
  }
  if (sWeb->server.hasArg("bw")) {
    if (!apiNumber("bw", &bw, 1, UINT16_MAX)) {
      return;
    }
    if (!bandBandwidthAllowed(BAND_FM, (uint16_t)bw)) {
      apiFailFmWidth("bw");
      return;
    }
  }
  if (radioAfBusy()) {
    apiFail(409,
            "A check is running, the preset watch's or an earlier series. "
            "Send again, or stop=1 to end a series.");
    return;
  }
  if (sAfChecks == NULL) {
    sAfChecks = (RadioAfCheck *)calloc(RADIO_AF_MAX, sizeof(*sAfChecks));
    if (sAfChecks == NULL) {
      apiFail(503, "No memory for the checks.");
      return;
    }
  }
  sAfSeries = &sAfSeriesStore;
  sAfSeries->check = sAfChecks;
  sAfSeries->khz = (uint32_t)khz;
  sAfSeries->count = (uint16_t)n;
  sAfSeries->everyMs = (uint16_t)ms;
  sAfSeries->widthKHz = (uint16_t)bw;
  if (!radioAfStart(sAfSeries)) {
    apiFail(409,
            "Not started: the radio is not on FM, is seeking or sweeping, or "
            "that channel is outside the band or not on a 10 kHz step.");
    return;
  }
  sWeb->server.send(200, "text/plain", "started\n");
}

static void handleApiAfCheckGet(void) {
  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "application/x-ndjson", "");
  const bool running = radioAfBusy();
  const RadioAfSeries *s = sAfSeries;
  char line[200];
  snprintf(line, sizeof(line),
           "{\"running\":%s,\"done\":%u,\"khz\":%u,\"n\":%u,\"ms\":%u,"
           "\"bw\":%u,\"stopped\":%s}\n",
           running ? "true" : "false",
           (unsigned)(running ? radioAfProgress() : (s ? s->done : 0)),
           (unsigned)(s ? s->khz : 0), (unsigned)(s ? s->count : 0),
           (unsigned)(s ? s->everyMs : 0), (unsigned)(s ? s->widthKHz : 0),
           s != NULL && !running && s->stopped ? "true" : "false");
  sWeb->server.sendContent(line);
  /* The readings only once the radio task has let go of them. */
  for (uint16_t i = 0; s != NULL && !running && i < s->done; i++) {
    const RadioAfCheck *c = &s->check[i];
    /* How far the check was from the nearest reply the web server sent,
     * or -1 with none kept near enough to say. */
    const int32_t gap =
        replyTimesGapMs(c->atMs, c->atMs + (c->us + 999u) / 1000u);
    snprintf(line, sizeof(line),
             "{\"at\":%u,\"level\":%d,\"usn\":%u,\"wam\":%u,"
             "\"off\":%d,\"st\":%u,\"us\":%u,\"err\":%u,\"net\":%ld}\n",
             (unsigned)c->atMs, (int)c->level, (unsigned)c->usn,
             (unsigned)c->wam, (int)c->offset, (unsigned)c->status,
             (unsigned)c->us, (unsigned)c->err,
             (long)(gap == INT32_MAX ? -1 : gap));
    sWeb->server.sendContent(line);
  }
  sWeb->server.sendContent("");
}

/*
 * GET /api/dx/timing.csv: for each channel a DX scan dwelt on and found
 * RDS on, how long from the dial reaching it to the lock, the first group,
 * the first clean block A and the PI confirmed, oldest first. Open to read,
 * like /api/dx.csv.
 */
static void sendTiming(const char *text, size_t len, void *ctx) {
  (void)ctx;
  sWeb->server.sendContent(text, len);
}

static void handleApiDxTimingCsv(void) {
  if (!logbookFsPresent()) {
    /* An empty file would read as nothing recorded yet. */
    apiFail(503, "The storage did not mount, so there is nothing to read.");
    return;
  }
  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "text/csv", "");
  sWeb->server.sendContent(dxTimingCsvHeader());
  dxTimingFsRead(sendTiming, NULL);
  sWeb->server.sendContent("");
}

void webApiDxRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/api/dx", HTTP_GET, handleApiDxGet);
  sWeb->server.on("/api/dx/sweep", HTTP_GET, handleApiDxSweepGet);
  sWeb->server.on("/api/dx.csv", HTTP_GET, handleApiDxCsv);
  sWeb->server.on("/api/dx", HTTP_POST, handleApiDx);
  sWeb->server.on("/api/afcheck", HTTP_POST, handleApiAfCheckPost);
  sWeb->server.on("/api/afcheck", HTTP_GET, handleApiAfCheckGet);
  sWeb->server.on("/api/dx/timing.csv", HTTP_GET, handleApiDxTimingCsv);
  sWeb->server.on("/api/scope", HTTP_POST, handleApiScopePost);
  sWeb->server.on("/api/scope", HTTP_GET, handleApiScopeGet);
}
