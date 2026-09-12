/**
 * @file screen.h
 * @brief The plain text screen for phase 2.
 *
 * Not the design. `docs/design.html` is phase 4, and every part of this file
 * is replaced when LVGL arrives. This exists to prove the panel, the bus and
 * the pin map work, and to give the radio a face before it has a real one.
 *
 * It takes a struct of strings rather than the radio's own state. That is what
 * keeps `ui/` from including either `core/` or `drivers/`: the layer above
 * fills this in, and the screen knows nothing about tuners or tasks.
 */
#ifndef UI_SCREEN_H
#define UI_SCREEN_H

#include <stdbool.h>
#include <stdint.h>

/** What to show. Every string must stay valid for the length of the call. */
typedef struct {
  const char *band;      /**< "FM", "MW" and so on. */
  const char *frequency; /**< Already formatted, such as "102.80". */
  const char *unit;      /**< "MHz" or "kHz". */
  const char *mode;      /**< The tuning mode in words. */
  int16_t signalTenths;  /**< Signal level in tenths of a dBuV. */
  bool signalValid;      /**< False when the last reading failed. */
  bool stereo;           /**< A stereo pilot is present. */
  bool muted;            /**< Audio is off. */
  bool tunerReady;       /**< The tuner started up. */
  const char *fault;     /**< What went wrong, or NULL when nothing did. */
} ScreenState;

/**
 * Start the panel and draw the parts that never change.
 *
 * @return false when the panel did not come up.
 */
bool screenBegin(void);

/**
 * Show the state.
 *
 * Only the fields that changed since the last call are redrawn. On a panel
 * clocked at 7.5 MHz a full repaint takes about a sixth of a second, so
 * redrawing everything each time would make the frequency visibly crawl
 * behind the knob.
 *
 * @param state  What to show.
 */
void screenShow(const ScreenState *state);

/**
 * Say something across the middle of the screen, on its own.
 *
 * For the moments before there is any state worth showing, such as start up
 * and a tuner that did not answer.
 *
 * @param line1  The first line, or NULL.
 * @param line2  The second line, or NULL.
 */
void screenMessage(const char *line1, const char *line2);

#endif /* UI_SCREEN_H */
