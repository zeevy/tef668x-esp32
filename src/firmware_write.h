/*
 * The radio's side of writing a new firmware image, whichever way it comes:
 * the browser's upload or the ArduinoOTA listener. The image itself is
 * written by whoever receives it; this is what the radio does around it, so
 * the two ways in cannot drift apart.
 *
 * Loop task only, where both ways in run.
 */
#ifndef FIRMWARE_WRITE_H
#define FIRMWARE_WRITE_H

#include <stdbool.h>

/*
 * A write is starting: what the radio is set to and the presets go into flash,
 * the panel goes over to the write, and the radio goes quiet and stops talking
 * to the tuner.
 */
void firmwareWriteBegin(void);

/* How far it has got, 0 to 100, for the panel. */
void firmwareWriteProgress(int percent);

/* How a write ended. The three that are not OK are what the panel can tell a
 * person, who can then act on it: try again, get the right file, or look at
 * the radio's flash. */
typedef enum {
  FIRMWARE_END_OK,
  FIRMWARE_END_DOWNLOAD, /* The file did not all arrive. */
  FIRMWARE_END_FILE,     /* It arrived and is not an image for this radio. */
  FIRMWARE_END_WRITE     /* The flash did not take it. */
} FirmwareEnd;

/*
 * It is over. Written: the panel holds the news until the reboot, which the
 * caller books and notes as an update once it is certain. Not written: the
 * panel says so, with why, and the radio is given back, since no reboot is
 * coming.
 */
void firmwareWriteEnd(FirmwareEnd how);

#endif /* FIRMWARE_WRITE_H */
