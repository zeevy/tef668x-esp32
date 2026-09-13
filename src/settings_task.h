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
 * The one place that conversion happens, for the same reason decision 26
 * gives for the band plan: a second copy assembled somewhere else drifts, and
 * then a manual save and an automatic one keep different things.
 *
 * Everything not owned by the radio, the network details and the access PIN
 * among them, is carried over from `from` untouched.
 */
bool settingsBuildCandidate(const Settings *from, Settings *out, bool *seeking);

void settingsTaskBegin(Settings *live, uint32_t idleMs);

/*
 * Look once, and write the settings if they are due.
 *
 * Call this from loop(). It reads a snapshot and compares two structs, so
 * calling it often costs nothing when nothing has moved.
 */
void settingsTaskPoll(void);

/*
 * Tell it the settings were just written by somebody else.
 *
 * For the manual save and for the settings endpoint, so a write from there
 * starts the wait again rather than leaving an automatic one due immediately
 * afterwards.
 */
void settingsTaskSaved(void);

/* What the automatic save has been doing, for the page and the checklist. */
typedef struct {
  uint32_t saves;   /* Automatic saves written since boot. */
  uint32_t lastMs;  /* When the last one was, ms since boot. 0 for none. */
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
