/**
 * @file display.h
 * @brief The ILI9341 panel: SPI, colours, rectangles and text.
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

/** Colours are 16 bit, five red, six green, five blue. */
typedef uint16_t Colour;

/**
 * Make a colour from eight bit red, green and blue.
 *
 * @param r  Red.
 * @param g  Green.
 * @param b  Blue.
 * @return The colour the panel wants.
 */
static inline Colour displayColour(uint8_t r, uint8_t g, uint8_t b) {
  return (Colour)(((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) |
                  (b >> 3));
}

/**
 * Start the panel.
 *
 * Sets the SPI pins up, resets the panel, sends its initialisation sequence,
 * clears it to black and turns the backlight on.
 *
 * @return false when the panel did not come up. Nothing can be read back from
 *         this panel, so this only catches what can be seen from here.
 */
bool displayBegin(void);

/**
 * Set the backlight.
 *
 * @param percent  0 for off, 100 for full.
 */
void displayBacklight(uint8_t percent);

/**
 * How wide the panel is.
 *
 * @return The width in pixels.
 */
uint16_t displayWidth(void);

/**
 * How tall the panel is.
 *
 * @return The height in pixels.
 */
uint16_t displayHeight(void);

/**
 * Fill a rectangle with one colour.
 *
 * @param x       Left edge.
 * @param y       Top edge.
 * @param w       Width.
 * @param h       Height.
 * @param colour  What to fill it with.
 */
void displayFill(int16_t x, int16_t y, uint16_t w, uint16_t h, Colour colour);

/**
 * Push a rectangle of pixels to the panel.
 *
 * This is what LVGL will call in phase 4, and the reason this driver exists
 * rather than a library.
 *
 * @param x       Left edge.
 * @param y       Top edge.
 * @param w       Width.
 * @param h       Height.
 * @param pixels  w times h colours, left to right then top to bottom.
 */
void displayPush(int16_t x, int16_t y, uint16_t w, uint16_t h,
                 const Colour *pixels);

/**
 * How wide a string would be, in pixels.
 *
 * For working out where to put something before drawing it.
 *
 * @param font  Which font.
 * @param text  The string. NULL is 0 wide.
 * @return The width in pixels.
 */
uint16_t displayTextWidth(const Font *font, const char *text);

/**
 * Draw a string.
 *
 * The background is painted as it goes rather than cleared first, so text that
 * changes does not flash. That matters here: the frequency is redrawn every
 * time the knob moves.
 *
 * @param x       Left edge.
 * @param y       Top edge.
 * @param font    Which font.
 * @param text    The string. Characters the font does not have draw as spaces.
 * @param fg      The text colour.
 * @param bg      What to paint behind it.
 * @return How far along x the drawing got.
 */
int16_t displayText(int16_t x, int16_t y, const Font *font, const char *text,
                    Colour fg, Colour bg);

#endif /* DRIVERS_DISPLAY_H */
