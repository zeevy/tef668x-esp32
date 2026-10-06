/*
 * The control API: the presets: setting and recalling them, and their CSV both
 * ways.
 */
#include "web_api_internal.h"
#include "web_internal.h"

#include "memory_store.h"
#include "radio_task.h"

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

static bool apiSlot(int *out) {
  long slot = 0;
  if (!apiNumber("slot", &slot, 1, MEMORY_SLOT_COUNT)) {
    return false;
  }
  *out = (int)slot - 1;
  return true;
}

static void handleApiMemoryCsv(void) {
  sWeb->server.sendHeader("Content-Disposition",
                          "attachment; filename=\"presets.csv\"");
  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "text/csv", "");
  sWeb->server.sendContent(memoryCsvHeader());
  char line[MEMORY_CSV_LINE_MAX];
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    size_t need = memoryStoreLine(i, line, sizeof(line));
    if (need == 0 || need >= sizeof(line)) {
      continue;
    }
    sWeb->server.sendContent(line);
  }
  sWeb->server.sendContent("");
}

/* Only the name and the width can be wrong in a preset built from a frequency
 * the request gave and that was checked on the way in. Saying which two saves
 * the caller guessing among four fields. */
static const char kPresetInvalid[] =
    "That preset cannot be stored. The name has to be plain text, and the "
    "bandwidth has to be one this band offers.";

/* The answer to a preset written: where, and what it now holds. */
static void sayPreset(int slot, BandId band, uint32_t khz) {
  char text[24];
  bandFormatWithUnit(band, khz, text, sizeof(text));
  sWeb->server.send(200, "text/plain",
                    String("Preset ") + (slot + 1) + " is " + bandName(band) +
                        " " + text + "\n");
}

/*
 * POST /api/presets. What to do comes in `do`.
 *
 * | `do` | What it does |
 * |---|---|
 * | `set` | Write a slot from `khz`, and `bw` and `name` if given |
 * | `recall` | Tune to a slot |
 */
static void handleApiMemoryPost(void) {
  if (!requireAuth(false)) {
    return;
  }
  if (!sWeb->server.hasArg("do")) {
    apiFail(400, "Give do=set or recall.");
    return;
  }
  String what = sWeb->server.arg("do");
  /* Checked before anything else is read, so an action nobody recognises is
   * answered with that rather than with a complaint about a missing slot. */
  if (what != "set" && what != "recall") {
    apiFail(400, "do has to be set or recall.");
    return;
  }

  int slot = MEMORY_NO_SLOT;
  if (!apiSlot(&slot)) {
    return;
  }

  if (what == "recall") {
    MemoryChannel c;
    if (!memoryStoreRead(slot, &c)) {
      apiFail(404, String("Preset ") + (slot + 1) + " is empty.");
      return;
    }
    BandPlanConfig plan;
    if (radioTaskPlan(&plan) && !memoryChannelTunable(&c, &plan)) {
      apiFail(409, String("Preset ") + (slot + 1) + " says " +
                       bandName((BandId)c.band) +
                       " but that frequency is not on that band here. Change "
                       "the preset or the band plan.");
      return;
    }
    RadioCommand cmd = {};
    cmd.kind = RADIO_RECALL;
    cmd.memorySlot = (int16_t)slot;
    apiSubmit(&cmd, String("recalled ") + (slot + 1), API_SAY_TUNE);
    return;
  }

  /* What is left is `set`. */
  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  /* What is in the slot now, so a request that gives only the frequency keeps
   * the name and the width that are already there. Writing a fresh channel
   * would wipe the name of anybody correcting a frequency. */
  bool had = memoryStoreRead(slot, &c);

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
  if (!had || c.band != (uint8_t)band) {
    /* A frequency that moves the channel to another band cannot keep the
     * old band's filter width, and there is no width the new band is known
     * to want, so it goes back to letting the radio choose. */
    c.bandwidthKHz = 0;
  }
  if (!had || c.band != (uint8_t)band || c.freqKHz != (uint32_t)khz) {
    /* Another frequency is another station: the stored PI goes, and the
     * slot learns the new one when it is confirmed there. */
    c.pi = 0;
  }
  c.band = (uint8_t)band;
  c.freqKHz = (uint32_t)khz;
  if (sWeb->server.hasArg("bw")) {
    long bw = 0;
    if (!apiNumber("bw", &bw, 0, 400)) {
      return;
    }
    c.bandwidthKHz = (uint16_t)bw;
  }

  if (sWeb->server.hasArg("name")) {
    String name = sWeb->server.arg("name");
    strncpy(c.name, name.c_str(), MEMORY_NAME_LEN - 1);
    c.name[MEMORY_NAME_LEN - 1] = '\0';
  }

  if (!memoryChannelValid(&c)) {
    apiFail(400, kPresetInvalid);
    return;
  }
  if (!memoryStoreWrite(slot, &c)) {
    apiFail(503, "The preset list could not be reached.");
    return;
  }
  sayPreset(slot, (BandId)c.band, c.freqKHz);
}

/*
 * POST /api/presets/import. The file is the body, the mode is in the query.
 *
 * `mode=merge` fills the empty slots only. `mode=replace` throws the list
 * away and loads the file, and refuses the whole file if one line cannot be
 * read, so a bad file never leaves a half loaded list.
 */
static void handleApiMemoryImport(void) {
  if (!requireAuth(false)) {
    return;
  }
  MemoryImportMode mode = MEMORY_IMPORT_MERGE;
  if (sWeb->server.hasArg("mode")) {
    String text = sWeb->server.arg("mode");
    if (text == "replace") {
      mode = MEMORY_IMPORT_REPLACE;
    } else if (text != "merge") {
      apiFail(400, "mode has to be merge or replace.");
      return;
    }
  }
  if (!sWeb->server.hasArg("plain")) {
    /* The server only fills `plain` when the content type is not
     * application/x-www-form-urlencoded, which is what curl -d sends unless
     * it is told otherwise. Saying so here saves the caller working out why
     * a file they did send looks missing. */
    apiFail(400,
            "Send the CSV as the body of the request, with a content type of "
            "text/csv.");
    return;
  }
  String body = sWeb->server.arg("plain");
  MemoryImportResult result;
  if (!memoryStoreImport(mode, body.c_str(), body.length(), &result)) {
    apiFail(400, String("Line ") + result.firstBadLine +
                     " could not be read, so nothing was changed.");
    return;
  }
  String said = String(result.imported) + " of " + result.lines +
                " lines imported, " + result.skipped + " refused, " +
                result.kept + " already taken, " + result.truncated +
                " names cut short";
  Serial.printf("[api] %s\n", said.c_str());
  sWeb->server.send(200, "text/plain", said + "\n");
}

void webApiPresetsRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/api/presets", HTTP_POST, handleApiMemoryPost);
  sWeb->server.on("/api/presets.csv", HTTP_GET, handleApiMemoryCsv);
  sWeb->server.on("/api/presets/import", HTTP_POST, handleApiMemoryImport);
}
