/**
 * @file board.h
 * @brief Picks the board header from the build flag and checks it is complete.
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

#endif /* BOARD_BOARD_H */
