/*
 * Which of the firmware's own restarts fired, in a word that survives one.
 *
 * `esp_reset_reason()` answers a coarser question than the one that gets
 * asked. Every restart this firmware performs, the boot watchdog, the
 * rollback, an LVGL assertion, a reboot somebody asked for and the one at the
 * end of an update, comes back from it as `ESP_RST_SW`. Five very different
 * events and one word for all of them, so that word alone cannot explain a
 * real restart.
 *
 * So the reason is written down before restarting, in a word kept in RTC
 * memory, and read back on the next boot. The packing and the checking are
 * here, away from the hardware, because both of the ways this goes wrong are
 * quiet ones: a word that was never written being believed, and a word from
 * an hour ago being reported as the reason for a power cycle.
 */
#ifndef CORE_RESTART_WHY_H
#define CORE_RESTART_WHY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Which of this firmware's restarts it was. */
typedef enum {
  RESTART_WHY_NONE = 0,      /* Nobody wrote one down. */
  RESTART_WHY_ASKED,         /* POST /reboot, the button on the page. */
  RESTART_WHY_BOOT_WATCHDOG, /* setup did not finish in time. */
  RESTART_WHY_ROLLBACK,      /* The image failed its self check. */
  RESTART_WHY_DISPLAY,       /* An LVGL assertion. */
  RESTART_WHY_UPDATE         /* Into an image that was just written. */
} RestartWhy;

/*
 * Wrap a reason up with a marker, ready to be left in RTC memory.
 *
 * The marker is what stops an uninitialised word being read as a reason. RTC
 * memory holds its contents across a restart and holds nothing in particular
 * across a power cycle, and a random word that happened to equal 3 would
 * otherwise report a rollback that never happened.
 */
uint32_t restartWhyPack(RestartWhy why);

/* Read one back. Anything that is not a packed reason reads as NONE. */
RestartWhy restartWhyUnpack(uint32_t word);

/*
 * The name of a reason, for the state document.
 *
 * NONE has no name and returns NULL, so a caller cannot accidentally print
 * "none" as though it meant something.
 */
const char *restartWhyText(RestartWhy why);

/*
 * The heap's lowest point in the boot before, kept across one restart.
 *
 * The web server answers nothing while an update is written and the radio
 * restarts as soon as it is, so the update's own low point can only be read
 * after the restart. Two words, the number and a check made from it, so the
 * random contents RTC memory holds after a power cycle are not read as one.
 */
void restartHeapPack(uint32_t lowestBytes, uint32_t out[2]);

/* Read one back. False, and `lowestBytes` left alone, unless the check
 * matches; false for a NULL as well. */
bool restartHeapUnpack(const uint32_t in[2], uint32_t *lowestBytes);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RESTART_WHY_H */
