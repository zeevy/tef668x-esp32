/* Implementation of keeping the stored settings up to date. */
#include "settings_task.h"

#include "core/autosave.h"
#include "core/radio.h"
#include "core/squelch.h"
#include "drivers/settings_nvs.h"
#include "radio_task.h"

#include <Arduino.h>
#include <string.h>

/*
 * How often the settings are compared, in milliseconds.
 *
 * Two struct comparisons and a snapshot copy, so it is cheap, but there is
 * nothing to gain from doing it faster than the radio publishes. The wait
 * before a save is ten seconds, so a quarter of a second either way changes
 * nothing about when one lands.
 */
#define SETTINGS_POLL_MS 250

static Settings *sLive = NULL;
static AutoSave sWhen;
static uint32_t sLastPollMs = 0;

/*
 * The candidate as it was on the previous look.
 *
 * Kept so that "something moved since last time" can be answered separately
 * from "this differs from what is stored". The two are different questions
 * and core/autosave.h needs both: what the radio is set to stays different
 * from what is stored for the whole wait, so one answer cannot restart the
 * clock and decide there is work to do at the same time.
 */
static Settings sPrevious;
static bool sPreviousKnown = false;

static SettingsSaveStatus sStatus;

bool settingsBuildCandidate(const Settings *from, Settings *out,
                            bool *seeking) {
  if (from == NULL || out == NULL) {
    return false;
  }
  RadioSnapshot now;
  if (!radioGetSnapshot(&now)) {
    return false;
  }
  /* Straight into the caller's struct. A local copy here and another in the
   * caller is four hundred bytes of stack on the loop task for nothing. */
  *out = *from;
  radioToSettings(&now.settings, out);
  /* Not part of RadioSettings, so radioToSettings cannot carry it. The
   * squelch mode belongs to the radio task rather than to the dial. */
  out->squelchMode = (uint8_t)radioSquelchMode(NULL);

  /* The volume is put back unless the knob is the squelch.
   *
   * Decision 26 stores a volume for one case only: in manual squelch the
   * knob is the squelch control and nothing else on the radio says how loud
   * to be. In every other mode the knob wins and the stored value is never
   * read, so keeping it up to date buys nothing and costs a great deal.
   *
   * It costs, because the volume follows the knob. Left in, every turn of
   * the volume control makes the settings differ and earns a write to flash
   * ten seconds later. Worse, the volume the radio starts at is mapped from
   * one pot reading with no hysteresis, so a knob sitting near the boundary
   * between two dB can map to a different value than the one stored and make
   * the radio write flash after a power cycle in which nothing was touched.
   * That one is intermittent, depends on where the knob happens to sit, and
   * shows up years later as a worn sector. */
  if (out->squelchMode != (uint8_t)SQUELCH_MANUAL) {
    out->startVolumeDb = from->startVolumeDb;
  }
  if (seeking != NULL) {
    *seeking = now.seeking;
  }
  return true;
}

void settingsTaskBegin(Settings *live, uint32_t idleMs) {
  sLive = live;
  memset(&sStatus, 0, sizeof(sStatus));
  sStatus.idleMs = idleMs;
  sPreviousKnown = false;
  autoSaveInit(&sWhen, idleMs, millis());
}

void settingsTaskSaved(void) {
  autoSaveDone(&sWhen, millis());
  sPreviousKnown = false;
}

void settingsTaskStatus(SettingsSaveStatus *out) {
  if (out != NULL) {
    *out = sStatus;
  }
}

void settingsTaskPoll(void) {
  if (sLive == NULL) {
    return;
  }
  uint32_t nowMs = millis();
  if ((uint32_t)(nowMs - sLastPollMs) < SETTINGS_POLL_MS) {
    return;
  }
  sLastPollMs = nowMs;

  Settings candidate;
  bool seeking = false;
  if (!settingsBuildCandidate(sLive, &candidate, &seeking)) {
    /* The radio could not be read, so there is nothing to compare against.
     * Leaving the clock alone is right: this is not the settings holding
     * still, it is not knowing what they are. */
    return;
  }

  bool differs = memcmp(&candidate, sLive, sizeof(Settings)) != 0;
  bool moved =
      !sPreviousKnown || memcmp(&candidate, &sPrevious, sizeof(Settings)) != 0;
  sPrevious = candidate;
  sPreviousKnown = true;

  bool due = autoSaveDue(&sWhen, differs, moved, seeking, nowMs);
  /* Read after the decision, not before. autoSaveDue is what restarts the
   * clock when something moved, so asking first reports the wait left from
   * the previous look and the countdown never shows its full length while a
   * hand is on the knob. */
  sStatus.differs = differs;
  sStatus.dueInMs = autoSaveWaitMs(&sWhen, differs, nowMs);
  if (!due) {
    return;
  }

  /* The radio can reach states the stored form has no room for, and an AM
   * width read on an FM band is one of them. Refusing here beats writing a
   * blob the next start would throw away without saying so. */
  if (!settingsValid(&candidate)) {
    sStatus.lastFailed = true;
    /* Start the wait again rather than trying every quarter of a second for
     * as long as the radio stays in that state. */
    autoSaveDone(&sWhen, nowMs);
    return;
  }
  if (!settingsNvsSave(&candidate)) {
    sStatus.lastFailed = true;
    autoSaveDone(&sWhen, nowMs);
    Serial.println(F("[settings] the automatic save could not be written"));
    return;
  }

  *sLive = candidate;
  sStatus.lastFailed = false;
  sStatus.saves++;
  sStatus.lastMs = nowMs;
  sStatus.differs = false;
  sStatus.dueInMs = 0;
  autoSaveDone(&sWhen, nowMs);

  char text[16];
  bandFormatFrequency((BandId)candidate.startBand, candidate.startFreqKHz, text,
                      sizeof(text));
  Serial.printf("[settings] saved on its own, %s %s\n", text,
                bandFrequencyUnit((BandId)candidate.startBand));
}
