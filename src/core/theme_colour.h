/*
 * Reading and writing a colour the way a web page's colour picker does.
 *
 * `<input type=color>` gives and takes "#rrggbb", six hex digits, so this is
 * what the custom theme's nine roles are carried as between the browser
 * and the settings struct. Nothing here is a colour role or an LVGL type:
 * this is three bytes and a string, and core/ knows neither theme nor panel.
 */
#ifndef CORE_THEME_COLOUR_H
#define CORE_THEME_COLOUR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Read "#rrggbb" or "rrggbb" into three bytes.
 *
 * Case insensitive, exactly six hex digits either way. Refused rather than
 * guessed at on anything else, including a short or a long string: a colour
 * silently clamped from bad input is a colour that does not match what the
 * picker showed.
 */
bool themeColourParse(const char *text, uint8_t *r, uint8_t *g, uint8_t *b);

/*
 * Write "#rrggbb" into a buffer of at least 8 bytes, lower case.
 *
 * Lower case because that is what a browser's own colour input gives back,
 * so a value round tripped through this reads the same either side.
 */
void themeColourFormat(uint8_t r, uint8_t g, uint8_t b, char *out, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* CORE_THEME_COLOUR_H */
