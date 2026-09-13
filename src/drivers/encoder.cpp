/* Implementation of the encoder and panel button pins. */
#include "encoder.h"

#include "board/board.h"

#include <Arduino.h>

/* The decoder. Only the interrupt touches it. */
static Encoder sEncoder;

/* Clicks turned and not yet taken. Shared with the interrupt. */
static volatile int32_t sClicks = 0;

/* Guards the click count against the interrupt on the other core. */
static portMUX_TYPE sMux = portMUX_INITIALIZER_UNLOCKED;

/* Which pin each button is on, in PanelButton order. */
static const uint8_t kButtonPins[PANEL_BUTTON_COUNT] = {
    PIN_BUTTON_BAND, PIN_BUTTON_BW, PIN_BUTTON_MODE, PIN_ENCODER_BUTTON};

/* Their names, in the same order. */
static const char *const kButtonNames[PANEL_BUTTON_COUNT] = {"BAND", "BW",
                                                             "MODE", "PUSH"};

/*
 * Both encoder lines changed, or one of them did.
 *
 * Marked IRAM_ATTR, but be clear about what that does and does not buy here.
 * The Arduino core installs the GPIO interrupt service without the IRAM flag,
 * because CONFIG_ARDUINO_ISR_IRAM is off in the build this project pins, and
 * its dispatcher is not in IRAM either. So this handler is masked while the
 * flash cache is off rather than running from RAM, and calling encoderFeed,
 * which lives in flash, is safe.
 *
 * What it costs: knob edges during a flash write, such as saving settings or
 * taking an update over the air, are not seen. This samples the two levels
 * instead of counting edges, so a pair of edges that collapses into one looks
 * like both lines moving at once, which the table scores as no movement. A
 * click can therefore be lost during a write. That is the right trade for
 * now: those writes are rare and short, and the alternative is holding the
 * decoder in RAM for a case nobody will notice.
 *
 * The work here is a table lookup and an add, which is what keeps it short
 * enough to sit in an interrupt at all.
 */
static void IRAM_ATTR onEncoderEdge(void) {
  int8_t detent = encoderFeed(&sEncoder, digitalRead(PIN_ENCODER_A) != 0,
                              digitalRead(PIN_ENCODER_B) != 0);
  if (detent != 0) {
    portENTER_CRITICAL_ISR(&sMux);
    sClicks += detent;
    portEXIT_CRITICAL_ISR(&sMux);
  }
}

/* True once the interrupts have been attached at least once. */
static bool sArmed = false;

void encoderBegin(EncoderKind kind, EncoderDirection direction) {
  /* Only detach what was attached. Detaching before the interrupt service has
   * ever been installed logs an error at start up that means nothing. */
  if (sArmed) {
    detachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A));
    detachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B));
    sArmed = false;
  }

  encoderInit(&sEncoder, kind, direction);
  portENTER_CRITICAL(&sMux);
  sClicks = 0;
  portEXIT_CRITICAL(&sMux);

  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  for (int i = 0; i < PANEL_BUTTON_COUNT; i++) {
    pinMode(kButtonPins[i], INPUT);
  }

  /* Read both lines once before arming, so the decoder knows where the knob
   * is resting. Without it the first edge after power on looks like a turn. */
  encoderFeed(&sEncoder, digitalRead(PIN_ENCODER_A) != 0,
              digitalRead(PIN_ENCODER_B) != 0);

  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), onEncoderEdge, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_B), onEncoderEdge, CHANGE);
  sArmed = true;
}

int32_t encoderTake(void) {
  portENTER_CRITICAL(&sMux);
  int32_t clicks = sClicks;
  sClicks = 0;
  portEXIT_CRITICAL(&sMux);
  return clicks;
}

bool encoderButtonDown(PanelButton button) {
  if (button >= PANEL_BUTTON_COUNT) {
    return false;
  }
  /* Low is pressed. The board pulls these up. */
  return digitalRead(kButtonPins[button]) == LOW;
}

const char *panelButtonName(PanelButton button) {
  if (button >= PANEL_BUTTON_COUNT) {
    return "?";
  }
  return kButtonNames[button];
}
