/*
 * Keeps the stored settings up to date with what the radio is set to.
 *
 * Another composition root, like input_task.h and screen_task.h. It is the one
 * place that knows the radio task, the settings struct and NVS at once.
 *
 * Two jobs. It builds the candidate settings from what the radio is set to
 * now, which is the same thing `POST /api/save` does and is shared with it so
 * the two can never write different subsets. And it writes that down on its
 * own once the radio has been left alone, so a station tuned and then
 * switched off is still there next time.
 *
 * The decision about when is in core/autosave.h, where it can be tested.
 */
#ifndef SETTINGS_TASK_H
#define SETTINGS_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/settings.h"

/*
 * Build what would be stored if the settings were written right now.
 *
 * The one place that conversion happens, for the same reason the band plan has
 * only one: a second copy assembled somewhere else drifts, and then a manual
 * save and an automatic one keep different things.
 *
 * Everything not owned by the radio, the network details and the access PIN
 * among them, is carried over from `from` untouched.
 *
 * While a seek or a band scan is walking the dial, the frequency is the one
 * the walk started from, so no save, automatic, by hand or before a restart,
 * stores a channel nobody chose. `busy`, when not NULL, says a seek or a DX
 * scan is walking it, which holds the automatic save off.
 */
bool settingsBuildCandidate(const Settings *from, Settings *out, bool *busy);

void settingsTaskBegin(Settings *live, uint32_t idleMs);

/*
 * Look once, and write the settings if they are due.
 *
 * Call this from loop(). It reads a snapshot and compares two structs, so
 * calling it often costs nothing when nothing has moved.
 */
void settingsTaskPoll(void);

/*
 * Write the settings down now, without waiting for the radio to be left
 * alone.
 *
 * For a restart somebody asked for: a firmware update either way round, and
 * both restarts, the menu's and the browser's. The automatic save waits ten
 * seconds after the last thing moved, which is right for flash wear, but it
 * means a station tuned and then updated straight away would be thrown away by
 * the reboot. A person who has just tuned and then pressed update has every
 * reason to expect the radio to come back where they left it.
 *
 * Nothing is written when what the radio is set to is already what is
 * stored, so an update started twice does not cost two writes to flash.
 * While a seek or a scan is walking the dial, the station it started from is
 * kept, as every save does (settingsBuildCandidate).
 *
 * Call it before the radio is hushed. It reads the radio to build what to
 * store, and a hushed radio has stopped publishing.
 *
 * Nothing comes back. The caller carries on whether it was written or not,
 * since an update or restart refused because a save failed would be worse than
 * one that comes back on the previous station. A failed write sets the same
 * failed flag the automatic save reports, and the serial log says which case it
 * was.
 */
void settingsTaskSaveNow(void);

/*
 * A restart somebody asked for, from the menu or the browser. Never returns.
 *
 * What the radio is set to is kept first, since the automatic save may still
 * be up to ten seconds off, and so is a preset change still settling. Then
 * the radio is hushed, so the restart does not end in a click, and the DX
 * catch the dial is on is written, as closing DX mode does. The caller checks
 * `rollbackPending` and notes the reason before this. Loop task only, where
 * the menu and the web server run.
 */
void settingsTaskRestart(void);

/*
 * What a restart does before restarting, for the radio going to sleep: what
 * it is set to and the presets are saved, the sound is taken down and parked,
 * and the DX catch is written. The radio stays silent after it.
 */
void settingsTaskStop(void);

/*
 * Put the settings that act at once into the radio, the screen and the input.
 *
 * The one place that happens. There are two ways to change a setting on this
 * radio, and both have to end here, or a setting changed from the menu
 * behaves differently from the same setting changed from the web page. The
 * radio has one way in, and the screen is one of its callers.
 *
 * Only the ones that act at once. The six read at start up are not here,
 * because nothing this function could do would make them take effect, and a
 * function that pretended otherwise would be the more misleading of the two.
 */
void settingsApplyLive(const Settings *s);

/*
 * The parts of settingsApplyLive. Start up calls each on its own, in the
 * order the radio comes up, and settingsApplyLive calls them all, so a
 * setting is applied in one place and a change and a restart cannot treat it
 * differently. The panel light and the rotation are handed to the panel as
 * it begins, and the clock to the network time as it begins, so those three
 * are the only ones settingsApplyLive applies itself.
 */

/* The theme for the hour, and the custom slot's colours. Before the panel is
 * built at start up: some of what a panel draws is set once, when it is
 * built. */
void settingsApplyTheme(const Settings *s);

/* What the panel shows from the settings: the battery, the RDS region and
 * the DX setup. Once the panel task has begun. */
void settingsApplyScreen(const Settings *s);

/* The radio half: seek, soft mute, band edge beep, squelch floor, RDS and
 * the volume AGC. Once the radio task exists, since they are held under its
 * lock. */
void settingsApplyRadio(const Settings *s);

/* The key beeps, the Touch switch, which way up a touch is read and the auto
 * off time, which counts from the call. Once the input has begun. */
void settingsApplyInput(const Settings *s);

/*
 * Store a whole settings struct: check it, write it, apply it, count it.
 *
 * For anything that changes a setting and wants it kept: the menu, the web
 * pages and the API. It refuses a struct `settingsValid` rejects rather than
 * writing a blob the next start would throw away without saying so, and it
 * records the write in the save status, so `asv` in the state document counts
 * every write to a 20 KB flash partition and not only the automatic ones.
 *
 * Returns false when it was refused or could not be written.
 */
bool settingsTaskStore(const Settings *candidate);

/* What the automatic save has been doing, for the web page and for checks run
 * on the radio. */
typedef struct {
  uint32_t saves;   /* Saves written since boot, automatic and asked for. */
  bool differs;     /* What the radio is set to is not what is stored. */
  uint32_t dueInMs; /* How long until a save, 0 when none is waiting. */
  bool lastFailed;  /* The last write was refused or did not go through. */
  uint32_t idleMs;  /* The wait in use. 0 means the automatic save is off. */
} SettingsSaveStatus;

/*
 * What the automatic save has been doing.
 *
 * Published because a save that never happens and a save that happens
 * constantly both look like a radio working normally from outside, and the
 * second one only shows up years later as a worn out sector.
 */
void settingsTaskStatus(SettingsSaveStatus *out);

#endif /* SETTINGS_TASK_H */
