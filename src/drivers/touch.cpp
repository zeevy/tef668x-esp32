/* Implementation of the XPT2046 touch driver. */
#include "touch.h"

#include "board/board.h"
#include "display.h"

#include <Arduino.h>
#include <SPI.h>

#if FEATURE_TOUCH

/*
 * The XPT2046's fastest clock, 2.5 MHz, which the ESP32 makes exactly as its
 * 80 MHz bus divided by 32. The panel runs the same bus at 40 MHz, so every
 * read sets its own clock and the panel's next push sets 40 MHz back.
 */
#define TOUCH_SPI_HZ 2500000

/*
 * The chip's control bytes. Each is a start bit, the input to convert, 12
 * bits, differential, and two power bits. Differential measures against the
 * glass's own drive, so the readings do not move with the supply.
 *
 * Power bits 01 keep the converter on between conversions, with the pen line
 * off. Power bits 00 power down after the conversion and turn the pen line
 * back on, so the last conversion of every read uses them.
 */
#define XPT_X 0xD1     /* X position. */
#define XPT_Y 0x91     /* Y position. */
#define XPT_Z1 0xB1    /* First pressure input. */
#define XPT_Z2 0xC1    /* Second pressure input. */
#define XPT_Y_END 0x90 /* Y position, then power down with the pen line on. */

static SPISettings sSettings(TOUCH_SPI_HZ, MSBFIRST, SPI_MODE0);

/*
 * One conversion. The answer follows the control byte as 16 clocks: one busy
 * clock, the 12 bits high bit first, then three zeros.
 */
static uint16_t convert(SPIClass &spi, uint8_t command) {
  spi.transfer(command);
  return (uint16_t)((spi.transfer16(0) >> 3) & 0x0FFF);
}

void touchBegin(void) {
  pinMode(PIN_TOUCH_IRQ, INPUT);
  /* The chip may start with its pen line off. One conversion that ends in
   * power down turns it on. */
  SPIClass &spi = displaySpi();
  spi.beginTransaction(sSettings);
  digitalWrite(PIN_TOUCH_CS, LOW);
  (void)convert(spi, XPT_Y_END);
  digitalWrite(PIN_TOUCH_CS, HIGH);
  spi.endTransaction();
}

bool touchPenDown(void) {
  return digitalRead(PIN_TOUCH_IRQ) == LOW;
}

void touchRead(TouchRaw *out) {
  if (out == NULL) {
    return;
  }
  SPIClass &spi = displaySpi();
  spi.beginTransaction(sSettings);
  digitalWrite(PIN_TOUCH_CS, LOW);
  /* The first conversion after the drive moves to a new pair of inputs can
   * catch the glass before it has settled, so it is made twice and the
   * second kept. Z2 uses the same drive as Z1, so it needs no second. */
  (void)convert(spi, XPT_Z1);
  out->z1 = convert(spi, XPT_Z1);
  out->z2 = convert(spi, XPT_Z2);
  (void)convert(spi, XPT_X);
  out->x = convert(spi, XPT_X);
  (void)convert(spi, XPT_Y);
  out->y = convert(spi, XPT_Y_END);
  digitalWrite(PIN_TOUCH_CS, HIGH);
  spi.endTransaction();
}

#endif /* FEATURE_TOUCH */

void touchTake(TouchReading *r, TouchRaw *raw) {
  touchRead(raw);
  r->read = true;
  r->raw.x = (int16_t)raw->x;
  r->raw.y = (int16_t)raw->y;
  r->z1 = raw->z1;
}
