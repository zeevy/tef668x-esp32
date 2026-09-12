/**
 * @file version.h
 * @brief The firmware version, in one place.
 */
#ifndef CORE_VERSION_H
#define CORE_VERSION_H

/** Semantic version of this firmware. Shown on boot and in the web page. */
#define FIRMWARE_VERSION "0.1.0"

/** Bootstrap release the web page pulls from the CDN. Pinned, never a range. */
#define BOOTSTRAP_VERSION "5.3.8"

/** Which board this image was built for. Comes from the board header. */
#ifndef BOARD_NAME
#define BOARD_NAME "unknown"
#endif

#endif /* CORE_VERSION_H */
