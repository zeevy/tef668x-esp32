/*
 * The ILI9341 panel: SPI, colours, rectangles and text.
 *
 * Small on purpose. Decision 25: LVGL draws the user interface in phase 4 and
 * asks a panel driver for one thing, which is to take a rectangle of pixels
 * and push it. That is displayPush, and it is the only part of this file that
 * survives phase 4.
 *
 * The text drawing is here so phase 2 can prove the panel and the bus work
 * without pulling in a drawing library that LVGL would then replace.
 *
 * Nothing ever reads from the panel. That is what settles the open question in
 * HARDWARE.md about pin 19 being both the standby LED and SPI MISO: MISO is
 * never wired, so there is no conflict to resolve.
 */
#ifndef DRIVERS_DISPLAY_H
#define DRIVERS_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#include "ui/font.h"

/* Colours are 16 bit, five red, six green, five blue. */
typedef uint16_t Colour;

static inline Colour displayColour(uint8_t r, uint8_t g, uint8_t b) {
  return (Colour)(((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) |
                  (b >> 3));
}

/*
 * Start the panel.
 *
 * Sets the SPI pins up, resets the panel, sends its initialisation sequence
 * and clears it to black. The backlight is left off: the caller decides how
 * it comes up, so that it can be faded rather than snapped on.
 */
bool displayBegin(void);

void displayBacklight(uint8_t percent);

uint16_t displayWidth(void);

uint16_t displayHeight(void);

void displayFill(int16_t x, int16_t y, uint16_t w, uint16_t h, Colour colour);

/*
 * Push a rectangle of pixels to the panel.
 *
 * This is what LVGL will call in phase 4, and the reason this driver exists
 * rather than a library.
 */
void displayPush(int16_t x, int16_t y, uint16_t w, uint16_t h,
                 const Colour *pixels);

/*
 * How wide a string would be, in pixels.
 *
 * For working out where to put something before drawing it.
 */
uint16_t displayTextWidth(const Font *font, const char *text);

/*
 * Draw a string.
 *
 * The background is painted as it goes rather than cleared first, so text that
 * changes does not flash. That matters here: the frequency is redrawn every
 * time the knob moves.
 */
int16_t displayText(int16_t x, int16_t y, const Font *font, const char *text,
                    Colour fg, Colour bg);

#endif /* DRIVERS_DISPLAY_H */
