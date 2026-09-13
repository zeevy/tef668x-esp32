/*
 * When the radio should write its settings down on its own.
 *
 * A radio that is tuned to a station and switched off should come back on
 * that station. Nothing kept that until now unless somebody asked for it, so
 * a person who tuned and switched off lost it.
 *
 * The whole difficulty is flash wear. The dial moves every fifty milliseconds
 * during a seek and several times a second under a hand, and writing on each
 * of those would put tens of thousands of writes through NVS in an afternoon.
 * So the rule is not "save when it changes", it is "save once the radio has
 * been left alone for a while and what it is set to is not what is stored".
 *
 * It is in `core/` because it is entirely a decision about time, and timing
 * is what cannot be checked by looking at a radio. A save that never fires
 * and a save that fires forty times a minute both look like a radio working
 * normally, and the second one is only visible years later as a dead sector.
 *
 * Every comparison is on a difference, so the `millis()` wrap passes without
 * firing a save.
 */
#ifndef CORE_AUTOSAVE_H
#define CORE_AUTOSAVE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * How long the radio is left alone before it writes itself down, in ms.
 *
 * Measured on this radio on 13 September 2026 rather than chosen, over 154
 * seconds of ordinary use: tuning around, changing band, hunting for a
 * station and settling. Gaps between one input and the next had a median of
 * 0.43 s, a 99th percentile of 6.1 s, and one gap of 13.8 s in the whole
 * session.
 *
 * Ten seconds is about 1.6 times that 99th percentile, so a burst of hunting
 * across a band is one write rather than forty. Fifteen would have saved one
 * further write in that session and left a longer window in which switching
 * the radio off loses the station just found, and closing that window is the
 * point of saving at all. See the usage section of HARDWARE.md.
 */
#define AUTOSAVE_IDLE_MS 10000

/* Where the wait has got to. Zero it before first use. */
typedef struct {
  uint32_t idleMs;       /* How long with nothing moving before a save. */
  uint32_t lastChangeMs; /* When the settings last moved. */
  bool started;          /* A first tick has been seen. */
} AutoSave;

void autoSaveInit(AutoSave *a, uint32_t idleMs, uint32_t nowMs);

/*
 * Ask whether now is the moment to write the settings.
 *
 * Call it as often as convenient. It does no work beyond a subtraction.
 *
 * The two questions it is given are deliberately separate. `moved` restarts
 * the clock and `differs` decides whether there is anything worth writing. A
 * single "has it changed" cannot do both: what the radio is set to stays
 * different from what is stored for the whole wait, so a caller answering
 * that one question would either restart the clock forever or never restart
 * it at all.
 */
bool autoSaveDue(AutoSave *a, bool differs, bool moved, bool busy,
                 uint32_t nowMs);

/*
 * Tell it a save has just been written.
 *
 * The clock starts again, so a write that is followed immediately by another
 * change waits the full time again rather than firing on the next tick.
 */
void autoSaveDone(AutoSave *a, uint32_t nowMs);

/*
 * How long until a save is due, in milliseconds.
 *
 * For a page that wants to say one is coming rather than leaving a person
 * wondering whether anything was kept.
 */
uint32_t autoSaveWaitMs(const AutoSave *a, bool differs, uint32_t nowMs);

#ifdef __cplusplus
}
#endif

#endif /* CORE_AUTOSAVE_H */
