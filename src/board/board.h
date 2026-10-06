/*
 * Picks the board header from the build flag and checks it is complete.
 *
 * Every board gets one header with its pin map and its feature flags. Nothing
 * outside board/ ever tests for a board name, it tests for a feature flag.
 */
#ifndef BOARD_BOARD_H
#define BOARD_BOARD_H

#if defined(BOARD_ATS125)
#include "board_ats125.h"
#else
#error "No board selected. Add -DBOARD_ATS125 or another board flag."
#endif

#ifndef BOARD_NAME
#error "The board header must define BOARD_NAME."
#endif

#ifndef BOARD_NAME_DISPLAY
#error "The board header must define BOARD_NAME_DISPLAY."
#endif

#ifndef BOARD_HOSTNAME
#error "The board header must define BOARD_HOSTNAME."
#endif

#ifndef FEATURE_PANEL_BUTTONS
#error "The board header must define FEATURE_PANEL_BUTTONS, 1 or 0."
#endif

#if FEATURE_PANEL_BUTTONS &&                                 \
    (!defined(PIN_BUTTON_BAND) || !defined(PIN_BUTTON_BW) || \
     !defined(PIN_BUTTON_MODE))
#error "FEATURE_PANEL_BUTTONS 1 needs the BAND, BW and MODE key pins."
#endif

#ifndef FEATURE_TOUCH
#error "The board header must define FEATURE_TOUCH, 1 or 0."
#endif

#if FEATURE_TOUCH && (!defined(PIN_SPI_MISO) || !defined(PIN_TOUCH_CS) || \
                      !defined(PIN_TOUCH_IRQ) || !defined(TOUCH_CAL_CORNERS))
#error "FEATURE_TOUCH 1 needs the touch pins and TOUCH_CAL_CORNERS."
#endif

#endif /* BOARD_BOARD_H */
