/*
 * The control API: the logbook: writing an entry, changing or deleting one,
 * and reading it back as JSON lines or CSV.
 */
#include "web_api_internal.h"
#include "web_chunked.h"
#include "web_internal.h"

#include "core/rds_country.h"
#include "core/web_text.h"
#include "drivers/logbook_fs.h"
#include "input_task.h"

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

/*
 * POST /api/log with do=edit or do=delete: the entry in slot `id`, whose
 * `time` the caller read with it, so a list read before the ring moved on
 * cannot change another entry. Edit takes `name`, up to 8 characters, and
 * `note`, up to 32, either or both, plain characters only; the readings are
 * never changed.
 */
static void editOrDelete(const String &what) {
  long id = 0;
  if (!apiNumber("id", &id, 0, LOGBOOK_MAX_ENTRIES - 1)) {
    return;
  }
  /* Not apiNumber, whose numbers are signed 32 bit: an entry with no real
   * time holds milliseconds since start, which can be past 2,147,483,647. */
  uint32_t time = 0;
  if (webParseCount(sWeb->server.arg("time").c_str(), &time) != WEB_NUMBER_OK) {
    apiFail(400, "Give time, the entry's own time as GET /api/log has it.");
    return;
  }
  /* With the time, which entry it is: two can share a second. */
  long khz = 0;
  if (!apiNumber("khz", &khz, 1, 200000)) {
    return;
  }
  const bool hasName = sWeb->server.hasArg("name");
  const bool hasNote = sWeb->server.hasArg("note");
  const String name = sWeb->server.arg("name");
  const String note = sWeb->server.arg("note");
  if (what == "edit") {
    if (!hasName && !hasNote) {
      apiFail(400, "Give a name, a note or both.");
      return;
    }
    if ((hasName && !logbookTextFits(name.c_str(), LOGBOOK_NAME_LEN - 1)) ||
        (hasNote && !logbookTextFits(note.c_str(), LOGBOOK_NOTE_LEN - 1))) {
      apiFail(400,
              "A name is up to 8 characters and a note up to 32, letters, "
              "digits, spaces and punctuation only.");
      return;
    }
  }
  const LogbookChange done =
      what == "edit" ? logbookFsEdit((uint16_t)id, time, (uint32_t)khz,
                                     hasName ? name.c_str() : NULL,
                                     hasNote ? note.c_str() : NULL)
                     : logbookFsDelete((uint16_t)id, time, (uint32_t)khz);
  if (done == LOGBOOK_NO_LOG) {
    apiFail(503, "The log could not be opened: its storage did not start.");
    return;
  }
  if (done == LOGBOOK_NOT_READ) {
    apiFail(503, "That entry could not be read, so nothing was changed.");
    return;
  }
  if (done == LOGBOOK_NO_ENTRY) {
    apiFail(404, "There is no entry there. Read the log again.");
    return;
  }
  if (done == LOGBOOK_NOT_IT) {
    apiFail(409,
            "That entry is not there any more: the log has moved on since "
            "it was read. Read it again.");
    return;
  }
  if (done != LOGBOOK_CHANGED) {
    apiFail(503, "Not saved: the log's storage did not take the write.");
    return;
  }
  sWeb->server.send(200, "text/plain",
                    what == "edit" ? "Entry changed.\n" : "Entry deleted.\n");
}

/*
 * POST /api/log. Writes an entry for whatever the radio is on right now,
 * exactly as holding ENTER does, through the same call inputWriteLogEntry
 * makes, so a script and a held key cannot disagree about what got written.
 * With do=edit or do=delete it changes an entry instead.
 *
 * What it actually captures is inputWriteLogEntry's job, not this handler's,
 * so nothing here reads a snapshot or touches the store.
 */
static void handleApiLogPost(void) {
  if (!requireAuth(false)) {
    return;
  }
  if (sWeb->server.hasArg("do")) {
    const String what = sWeb->server.arg("do");
    if (what != "edit" && what != "delete") {
      apiFail(400,
              "do has to be edit or delete, or left out to log the "
              "station the radio is on.");
      return;
    }
    editOrDelete(what);
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

/* Where GET /api/log's lines go, and how many of the oldest to leave out. */
struct JsonReply {
  ChunkedReply *out;
  uint16_t skip;
};

/* One entry of GET /api/log as a JSON line, into the JsonReply `ctx` points
 * to, unless it is one of the oldest left out. */
static void sendJsonLine(uint16_t id, const LogbookEntry *e, void *ctx) {
  JsonReply &reply = *static_cast<JsonReply *>(ctx);
  if (reply.skip > 0) {
    reply.skip--;
    return;
  }
  ChunkedReply &out = *reply.out;
  /* Wide enough for every field at its type's own worst case, not just a
   * realistic one: ten digit epoch and frequency, "OIRT", both bools spelled
   * "false", every number at its type's widest, an eight character name
   * with every character escaped as \u00XX, a three digit programme type
   * with the longest name, "Soft Rhythm and Blues" at 21 characters, and a
   * country all landing in the same line is 307 characters with the id, so a
   * line that fit every entry seen so far could still be cut short by the
   * next one. */
  char line[320];
  /* Escaped: a station's name is whatever it broadcast, and a quote or a
   * backslash in it written raw ends the string early and breaks the line
   * for any JSON reader. Six bytes a character at worst, plus the quotes. */
  char nameField[2 + (LOGBOOK_NAME_LEN - 1) * 6 + 1] = "null";
  if (e->hasName) {
    snprintf(nameField, sizeof(nameField), "\"%s\"",
             jsonEscape(e->name).c_str());
  }
  char piField[8] = "null";
  if (e->hasPi) {
    snprintf(piField, sizeof(piField), "\"%04X\"", e->pi);
  }
  /* The programme type's number, and its name in the region the settings
   * name, as the state gives them. */
  char ptyField[5] = "null";
  String ptnField = "null";
  if (e->hasPty) {
    snprintf(ptyField, sizeof(ptyField), "%u", (unsigned)e->pty);
    ptnField =
        "\"" +
        jsonEscape(rdsPtyNameIn(e->pty, (RdsRegion)sWeb->settings->rdsRegion)) +
        "\"";
  }
  const char *country = logbookEntryCountry(e);
  char ctyField[8] = "null";
  if (country != NULL) {
    snprintf(ctyField, sizeof(ctyField), "\"%s\"", country);
  }
  size_t need = (size_t)snprintf(
      line, sizeof(line),
      "{\"id\":%u,\"time\":%u,\"real\":%s,\"band\":\"%s\",\"khz\":%u,"
      "\"level_dbuv\":%d,\"usn\":%u,\"multipath\":%u,"
      "\"cochannel\":%u,\"snr\":%d,\"stereo\":%s,\"bw_khz\":%u,"
      "\"name\":%s,\"pi\":%s,\"pty\":%s,\"ptn\":%s,\"cty\":%s,\"rt\":",
      (unsigned)id, (unsigned)e->timeValue, e->timeKnown ? "true" : "false",
      bandName((BandId)e->band), (unsigned)e->freqKHz, (int)e->levelDbuVTenths,
      (unsigned)e->usnTenths, (unsigned)e->multipathTenths,
      (unsigned)e->coChannelTenths, (int)e->snrDb, e->stereo ? "true" : "false",
      (unsigned)e->bandwidthKHz, nameField, piField, ptyField, ptnField.c_str(),
      ctyField);
  if (need == 0 || need >= sizeof(line)) {
    /* Cut rather than sent short: a truncated line is not valid JSON, and a
     * line missing entirely is at least honestly missing. */
    return;
  }
  /* The radio text is sent on its own rather than sized into `line`: up to
   * 64 characters, each six bytes escaped at worst. Escaped for the same
   * reason the name is. */
  out += line;
  out += e->hasRt ? "\"" + jsonEscape(e->rt) + "\"" : String("null");
  /* The person's note, sent on its own as the radio text is. */
  out += e->note[0] != '\0' ? ",\"note\":\"" + jsonEscape(e->note) + "\""
                            : String(",\"note\":null");
  /* Whether a person typed the name, so it is not taken for one received. */
  out += e->renamed ? ",\"renamed\":true}\n" : ",\"renamed\":false}\n";
}

/*
 * GET /api/log. Every stored entry, oldest first, one compact JSON object a
 * line, so a script can read this without a JSON array parser and a person
 * can read one line and know what happened. Open to read, like the rest of
 * the state: an entry is what a station broadcast plus a reading already
 * public on `/api/state`, not a secret. `last=N` sends only the newest N.
 * Read through one open file, and sent
 * 512 bytes at a time: on the loop task, opening the file for each entry
 * costs about 22 ms an entry, and each send about 8 ms, so four sends an
 * entry held the loop 2.4 s for 78 entries.
 */
static void handleApiLogGet(void) {
  /* `last`: only the newest that many, for a page that shows a few. The
   * count on the first line is still the whole log's. */
  const uint16_t count = logbookFsCount();
  long last = count;
  if (sWeb->server.hasArg("last") &&
      !apiNumber("last", &last, 1, LOGBOOK_MAX_ENTRIES)) {
    return;
  }
  ChunkedReply out(sWeb->server);
  out.begin(200, "application/x-ndjson");
  char head[16];
  snprintf(head, sizeof(head), "{\"n\":%u}\n", (unsigned)count);
  out += head;
  JsonReply reply = {&out, (uint16_t)(count > last ? count - last : 0)};
  logbookFsEach(sendJsonLine, &reply);
}

/* What each CSV line needs: the reply, and the clock offset in minutes. */
struct CsvReply {
  ChunkedReply *out;
  int16_t offsetMinutes;
};

/* One entry as a line of CSV, into the CsvReply `ctx` points to. */
static void sendCsvLine(uint16_t, const LogbookEntry *e, void *ctx) {
  CsvReply &reply = *static_cast<CsvReply *>(ctx);
  /* The longest line, every field at its widest and a name, a radio text
   * and a note of nothing but doubled quotes, is 304 bytes. */
  char line[320];
  const size_t need =
      logbookCsvLine(e, reply.offsetMinutes, line, sizeof(line));
  if (need != 0 && need < sizeof(line)) {
    *reply.out += line;
  }
}

/*
 * GET /api/log.csv. The same entries logbookCsvLine already knows how to
 * write, streamed 512 bytes at a time so the whole log, up to
 * LOGBOOK_MAX_ENTRIES of it, is never held in RAM at once. `tzo` is read
 * fresh from the settings rather than carried from whenever each entry was
 * written, so a person who reads the log after moving time zones sees every
 * entry in the zone they are asking from now.
 */
static void handleApiLogCsv(void) {
  sWeb->server.sendHeader("Content-Disposition",
                          "attachment; filename=\"logbook.csv\"");
  ChunkedReply out(sWeb->server);
  out.begin(200, "text/csv");
  out += logbookCsvHeader();
  CsvReply reply = {&out, sWeb->settings->clockOffsetMinutes};
  logbookFsEach(sendCsvLine, &reply);
}

void webApiLogRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/api/log", HTTP_GET, handleApiLogGet);
  sWeb->server.on("/api/log", HTTP_POST, handleApiLogPost);
  sWeb->server.on("/api/log.csv", HTTP_GET, handleApiLogCsv);
}
