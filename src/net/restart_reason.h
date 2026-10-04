/*
 * Why the radio last started, in one string.
 *
 * Two halves. The chip's own answer from `esp_reset_reason()`, which knows
 * about power, brownouts, panics and the hardware watchdogs, and this
 * firmware's own note from `core/restart_why.h`, which is the only thing that
 * can tell the five software restarts apart.
 *
 * Read once at boot, before anything can overwrite it.
 */
#ifndef NET_RESTART_REASON_H
#define NET_RESTART_REASON_H

#include <esp_system.h>

#include "core/restart_why.h"

/*
 * Read the reason and clear the note.
 *
 * Cleared because the note has to belong to the restart that just happened.
 * Left in place, it would be reported again after the next power cycle, which
 * is the failure this is meant to prevent rather than cause.
 */
void restartReasonBegin(void);

/* Leave a note for the next boot, with the heap's lowest point so far, then
 * restart is the caller's business. */
void restartReasonNote(RestartWhy why);

/*
 * What to put in the state document. Never NULL.
 *
 * "power", "brownout", "panic", "task watchdog" and so on from the chip, with
 * this firmware's note in brackets when there is one: "software (rollback)".
 */
const char *restartReasonText(void);

/*
 * The two halves apart, for a screen that writes its own words for them.
 * `why` is RESTART_WHY_NONE unless the chip says software, since the note is
 * only believed for a software restart. Either pointer may be NULL.
 */
void restartReasonParts(esp_reset_reason_t *chip, RestartWhy *why);

/*
 * The lowest the free heap went in the boot before, in bytes, as it stood
 * when that boot noted its restart. False after a power cycle, a crash or a
 * restart this firmware did not note, since only a noted one left it.
 */
bool restartReasonLastHeap(uint32_t *lowestBytes);

#endif /* NET_RESTART_REASON_H */
