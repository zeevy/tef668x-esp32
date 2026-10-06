/*
 * The control API: the logbook: writing an entry, and reading it back as
 * JSON lines or CSV.
 */
#include "web_api_internal.h"
#include "web_internal.h"

#include "drivers/logbook_fs.h"
#include "input_task.h"
#include "screen_task.h"

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

/*
 * POST /api/log. Writes an entry for whatever the radio is on right now,
 * exactly as holding ENTER does, through the same call inputWriteLogEntry
 * makes, so a script and a held key cannot disagree about what got written.
 *
 * What it actually captures is inputWriteLogEntry's job, not this handler's,
 * so nothing here reads a snapshot or touches the store.
 */
static void handleApiLogPost(void) {
  if (!requireAuth(false)) {
    return;
  }
  const InputLogResult logged = inputWriteLogEntry();
  if (logged == INPUT_LOG_WALKING) {
    apiFail(409,
            "Not logged. The radio is in the middle of a seek or a scan. Wait "
            "for it to end, or stop it with a tune.");
    return;
  }
  if (logged == INPUT_LOG_ALREADY) {
    apiFail(409,
            "Not logged. The log holds this station already, on this band "
            "and frequency.");
    return;
  }
  if (logged != INPUT_LOG_WRITTEN) {
    apiFail(503,
            "Not written. The radio was busy or the log has nowhere to "
            "write to.");
    return;
  }
  sWeb->server.send(200, "text/plain", "logged\n");
}

/*
 * GET /api/log. Every stored entry, oldest first, one compact JSON object a
 * line, so a script can read this without a JSON array parser and a person
 * can read one line and know what happened. Open to read, like the rest of
 * the state: an entry is what a station broadcast plus a reading already
 * public on `/api/state`, not a secret.
 */
static void handleApiLogGet(void) {
  uint16_t n = logbookFsCount();
  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "application/x-ndjson", "");
  /* Wide enough for every field at its type's own worst case, not just a
   * realistic one: ten digit epoch and frequency, "OIRT", both bools spelled
   * "false", every number at its type's widest and an eight character name
   * with every character escaped as \u00XX all landing in the same line is
   * 243 bytes, so a line that fit every entry seen so far could still be cut
   * short by the next one. */
  char line[256];
  snprintf(line, sizeof(line), "{\"n\":%u}\n", (unsigned)n);
  sWeb->server.sendContent(line);
  for (uint16_t i = 0; i < n; i++) {
    LogbookEntry e;
    if (!logbookFsEntryAt(i, &e)) {
      continue;
    }
    /* With the offset in force for its band, as log.csv. */
    e.levelDbuVTenths = signalShownTenths(
        e.levelDbuVTenths, screenTaskLevelOffsetDb((BandId)e.band));
    /* Escaped: a station's name is whatever it broadcast, and a quote or a
     * backslash in it written raw ends the string early and breaks the line
     * for any JSON reader. Six bytes a character at worst, plus the quotes. */
    char nameField[2 + (LOGBOOK_NAME_LEN - 1) * 6 + 1] = "null";
    if (e.hasName) {
      snprintf(nameField, sizeof(nameField), "\"%s\"",
               jsonEscape(e.name).c_str());
    }
    char piField[8] = "null";
    if (e.hasPi) {
      snprintf(piField, sizeof(piField), "\"%04X\"", e.pi);
    }
    size_t need = (size_t)snprintf(
        line, sizeof(line),
        "{\"time\":%u,\"real\":%s,\"band\":\"%s\",\"khz\":%u,"
        "\"level_dbuv\":%d,\"usn\":%u,\"multipath\":%u,"
        "\"cochannel\":%u,\"snr\":%d,\"stereo\":%s,\"bw_khz\":%u,"
        "\"name\":%s,\"pi\":%s,\"rt\":",
        (unsigned)e.timeValue, e.timeKnown ? "true" : "false",
        bandName((BandId)e.band), (unsigned)e.freqKHz, (int)e.levelDbuVTenths,
        (unsigned)e.usnTenths, (unsigned)e.multipathTenths,
        (unsigned)e.coChannelTenths, (int)e.snrDb, e.stereo ? "true" : "false",
        (unsigned)e.bandwidthKHz, nameField, piField);
    if (need == 0 || need >= sizeof(line)) {
      /* Cut rather than sent short: a truncated line is not valid JSON,
       * and a line missing entirely is at least honestly missing. */
      continue;
    }
    /* The radio text is sent on its own rather than sized into `line`: up
     * to 64 characters, each six bytes escaped at worst. Escaped for the
     * same reason the name is. */
    sWeb->server.sendContent(line);
    if (e.hasRt) {
      sWeb->server.sendContent("\"" + jsonEscape(e.rt) + "\"}\n");
    } else {
      sWeb->server.sendContent("null}\n");
    }
  }
  sWeb->server.sendContent("");
}

/*
 * GET /api/log.csv. The same entries logbookCsvLine already knows how to
 * write, streamed a line at a time so the whole log, up to
 * LOGBOOK_MAX_ENTRIES of it, is never held in RAM at once. `tzo` is read
 * fresh from the settings rather than carried from whenever each entry was
 * written, so a person who reads the log after moving time zones sees every
 * entry in the zone they are asking from now.
 */
static void handleApiLogCsv(void) {
  sWeb->server.sendHeader("Content-Disposition",
                          "attachment; filename=\"logbook.csv\"");
  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "text/csv", "");
  sWeb->server.sendContent(logbookCsvHeader());
  int16_t offsetMinutes = sWeb->settings->clockOffsetMinutes;
  uint16_t n = logbookFsCount();
  /* The longest line, every field at its widest and a name and a radio
   * text of nothing but doubled quotes, is 228 bytes. */
  char line[320];
  for (uint16_t i = 0; i < n; i++) {
    LogbookEntry e;
    if (!logbookFsEntryAt(i, &e)) {
      continue;
    }
    /* Kept as the radio read it, written with the offset in force for its
     * band, as the time goes out in the zone in force. */
    e.levelDbuVTenths = signalShownTenths(
        e.levelDbuVTenths, screenTaskLevelOffsetDb((BandId)e.band));
    size_t need = logbookCsvLine(&e, offsetMinutes, line, sizeof(line));
    if (need == 0 || need >= sizeof(line)) {
      continue;
    }
    sWeb->server.sendContent(line);
  }
  sWeb->server.sendContent("");
}

void webApiLogRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/api/log", HTTP_GET, handleApiLogGet);
  sWeb->server.on("/api/log", HTTP_POST, handleApiLogPost);
  sWeb->server.on("/api/log.csv", HTTP_GET, handleApiLogCsv);
}
