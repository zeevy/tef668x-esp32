/* The firmware version, in one place. */
#ifndef CORE_VERSION_H
#define CORE_VERSION_H

/* Semantic version of this firmware. Shown on boot and in the web page. */
#define FIRMWARE_VERSION "0.2.0"

/* Which board this image was built for. Comes from the board header. */
#ifndef BOARD_NAME
#define BOARD_NAME "unknown"
#endif

#endif /* CORE_VERSION_H */
