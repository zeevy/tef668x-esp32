/*
 * The ILI9341 panel: SPI, colours and rectangles.
 *
 * Small on purpose. LVGL draws the user interface and asks a panel driver
 * for one thing, which is to take a rectangle of pixels and push it. That is
 * displayPush.
 *
 * Nothing ever reads from the panel. The touch chip shares its SPI bus and
 * does read, through displaySpi.
 */
#ifndef DRIVERS_DISPLAY_H
#define DRIVERS_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Start the panel.
 *
 * Sets the SPI pins up, resets the panel, sends its initialisation sequence
 * and clears it to black. The backlight is left off: the caller decides how
 * it comes up, so that it can be faded rather than snapped on.
 */
bool displayBegin(void);

/*
 * Turn the panel between the board's own mount, 0, and 180 from it, live.
 * `settingsValid` restricts a stored value to one of these two;
 * anything else is a mirror image, not a rotation, and is never sent here.
 *
 * True when the panel turned. Only what is written after the turn lands the
 * new way up, so on true the caller has everything on the glass drawn again.
 */
bool displayRotationSet(uint16_t degrees);

void displayBacklight(uint8_t percent);

uint16_t displayWidth(void);

uint16_t displayHeight(void);

/*
 * Push a rectangle of pixels to the panel.
 *
 * This is what LVGL calls, and the reason this driver exists rather than a
 * library. `bytes` is the rectangle row by row, two bytes a pixel with the
 * high byte first, which is the order the panel takes, so it is sent as it
 * is. It is read in 32 bit words, so it must start on a 4 byte boundary.
 */
void displayPush(int16_t x, int16_t y, uint16_t w, uint16_t h,
                 const uint8_t *bytes);

/*
 * The panel's SPI bus, for the touch chip that shares it.
 *
 * Every user takes it for one operation with its own clock and gives it
 * back, so the two never hold it at once. Both run on the loop task, so a
 * touch read never lands in the middle of a push.
 */
class SPIClass;
SPIClass &displaySpi(void);

#endif /* DRIVERS_DISPLAY_H */
