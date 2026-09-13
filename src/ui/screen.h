/*
 * The plain text screen for phase 2.
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

/* What to show. Every string must stay valid for the length of the call. */
typedef struct {
  const char *band;      /* "FM", "MW" and so on. */
  const char *frequency; /* Already formatted, such as "102.80". */
  const char *unit;      /* "MHz" or "kHz". */
  const char *mode;      /* The tuning mode in words. */
  /*
   * Signal level in whole dBuV.
   *
   * Whole, not tenths. A tenth of a dB is below anything a person acts on,
   * and a screen printing one changes its last digit ten times a second on a
   * station that is not moving, which reads as flicker. The layer above
   * decides the number and holds it still; see SignalDisplay in
   * core/signal.h.
   */
  int16_t signalDbuV;
  bool signalValid; /* False when the last reading failed. */
  /*
   * You are hearing stereo.
   *
   * Not the same as the chip's pilot flag. Forcing mono does not remove the
   * pilot from the transmission, so the chip keeps reporting it, and a screen
   * driven straight off that flag says stereo while the audio is mono. The
   * screen has to describe what comes out of the speaker.
   */
  bool stereo;
  bool muted; /* Audio is off. */
  /* The two FM reception features, shown because the only control for them
   * is a button that cycles four combinations. A control with no feedback
   * leaves the person counting presses. */
  bool ims;          /* Multipath suppression is on. */
  bool eq;           /* The channel equalizer is on. */
  bool tunerReady;   /* The tuner started up. */
  const char *fault; /* What went wrong, or NULL when nothing did. */
} ScreenState;

bool screenBegin(void);

/*
 * Show the state.
 *
 * Only the fields that changed since the last call are redrawn. On a panel
 * clocked at 7.5 MHz a full repaint takes about a sixth of a second, so
 * redrawing everything each time would make the frequency visibly crawl
 * behind the knob.
 */
void screenShow(const ScreenState *state);

/*
 * Say something across the middle of the screen, on its own.
 *
 * For the moments before there is any state worth showing, such as start up
 * and a tuner that did not answer.
 */
void screenMessage(const char *line1, const char *line2);

#endif /* UI_SCREEN_H */
