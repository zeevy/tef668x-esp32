/*
 * The XPT2046 touch controller under the glass, on the panel's SPI bus.
 *
 * Raw readings only, with no maths. Each reading is 0 to 4095. X and Y say
 * where the finger is on the glass, along the chip's own two axes, which are
 * not the screen's until a calibration maps them. Z1 and Z2 together say how
 * much of the glass is in contact: with nothing on it Z1 sits near 0. A
 * finger pressed harder reads higher; a pen's small tip reads about the same
 * however hard it is pressed.
 *
 * The chip has a pen line of its own that goes low while a finger is down,
 * so the bus is only used while somebody is touching.
 */
#ifndef DRIVERS_TOUCH_H
#define DRIVERS_TOUCH_H

#include <stdbool.h>
#include <stdint.h>

/* One reading of all four inputs, each 0 to 4095. */
typedef struct {
  uint16_t x;
  uint16_t y;
  uint16_t z1;
  uint16_t z2;
} TouchRaw;

/*
 * Set the pen line up and arm it. Call after displayBegin, which starts the
 * bus and holds the chip select high.
 */
void touchBegin(void);

/* Whether the pen line says a finger is down now. */
bool touchPenDown(void);

/*
 * Read all four inputs once. Takes the bus for seven conversions, 21 bytes
 * at 2.5 MHz, then gives it back to the panel.
 */
void touchRead(TouchRaw *out);

#endif /* DRIVERS_TOUCH_H */
