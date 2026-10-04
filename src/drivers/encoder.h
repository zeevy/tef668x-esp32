/*
 * The rotary encoder and the panel buttons, as pins.
 *
 * The quadrature decoding itself is in core/input.h and is tested on a PC.
 * This file is only the part that cannot be: which pins, and an interrupt
 * fast enough that a spun knob does not drop clicks.
 *
 * Pins 34, 36 and 39 are input only on the ESP32 and have no internal pull
 * resistors, so the board carries its own. That is why these are set up as
 * plain inputs and not as pull ups. All four switches read low when pressed.
 */
#ifndef DRIVERS_ENCODER_H
#define DRIVERS_ENCODER_H

#include <stdbool.h>
#include <stdint.h>

#include "core/input.h"

/* Which panel button, for encoderButtonDown. */
typedef enum {
  PANEL_BUTTON_BAND = 0, /* BAND. Long press opens the RDS screen. */
  PANEL_BUTTON_BW,       /* BW, bandwidth. */
  PANEL_BUTTON_MODE,     /* MODE, cycles the tuning mode. */
  PANEL_BUTTON_ENCODER,  /* The push on the knob itself. */
  PANEL_BUTTON_COUNT
} PanelButton;

/*
 * Set the encoder and the buttons up.
 *
 * Attaches an interrupt to both encoder lines. Safe to call again, which
 * simply reapplies the settings.
 */
void encoderBegin(EncoderKind kind, EncoderDirection direction);

/*
 * Take the clicks turned since the last call.
 *
 * Reading clears the count, so no click is ever counted twice and none is
 * lost between calls. Safe against the interrupt.
 */
int32_t encoderTake(void);

/*
 * Whether a button is down right now.
 *
 * Raw, with no debouncing. The debouncing and the short, long and double
 * press machine live in core/input.h, which is where they can be tested.
 */
bool encoderButtonDown(PanelButton button);

const char *panelButtonName(PanelButton button);

#endif /* DRIVERS_ENCODER_H */
