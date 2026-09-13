/* Implementation of the pot and the analogue S-meter. */
#include "analog.h"

#include "board/board.h"

#include <Arduino.h>
#include <math.h>

/* How many reads are averaged for one pot value. */
#define POT_SAMPLES 8

/* The meter's PWM channel. The backlight has channel 0. */
#define SMETER_CHANNEL 1
#define SMETER_HZ 1000 /* Slow enough that the needle sees an average. */
#define SMETER_BITS 8  /* 0 to 255. See the note in smeterShow. */

/* Full deflection. */
#define SMETER_FULL 255

static bool sDriven = false;

void analogBegin(void) {
  /* Once only. Attaching the meter pin to LEDC a second time is refused by
   * the core and prints an error, and this is now called from two places:
   * once before the radio task starts so the volume knob can be read, and
   * once by the input layer. */
  static bool started = false;
  if (started) {
    return;
  }
  started = true;

  /* No pinMode for the ADC pin. It is input only and analogRead sets up what
   * it needs. Calling pinMode on 34 to 39 as an output is a way to get
   * nothing at all. */
  analogReadResolution(12);

  ledcAttachChannel(PIN_SMETER_PWM, SMETER_HZ, SMETER_BITS, SMETER_CHANNEL);
  ledcWrite(PIN_SMETER_PWM, 0);
  sDriven = false;
}

uint16_t potRead(void) {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < POT_SAMPLES; i++) {
    sum += (uint32_t)analogRead(PIN_SQUELCH_ADC);
  }
  return (uint16_t)(sum / POT_SAMPLES);
}

void smeterShow(int16_t levelTenths) {
  /*
   * The curve is the working firmware's, exactly: 51 * (10^(level/100) - 1),
   * with the level in tenths of a dBuV, and a flat top at and above 100 dBuV.
   *
   * Eight bits, not more, and that is the part to understand before changing
   * it. That firmware computes this into an int16_t that reaches 511, then
   * writes it with analogWrite and never calls analogWriteResolution, so it
   * lands in an eight bit PWM whose maximum is 255. Everything it computes
   * above 255 is already full deflection, which happens at about 77.8 dBuV,
   * not at the 100 dBuV the formula suggests.
   *
   * Widening this to ten bits would be arithmetically truer to the formula
   * and would halve how far the needle moves for a given signal. The face of
   * the meter was printed against what the radio does, not against what the
   * formula says, so this matches the radio.
   */
  uint32_t duty = 0;

  if (levelTenths > 0) {
    double value = 51.0 * (pow(10.0, (double)levelTenths / 1000.0) - 1.0);
    if (value > (double)SMETER_FULL) {
      value = (double)SMETER_FULL;
    }
    if (value > 0.0) {
      duty = (uint32_t)value;
    }
  }

  ledcWrite(PIN_SMETER_PWM, duty);
  sDriven = true;
}

bool smeterDriven(void) {
  return sDriven;
}
