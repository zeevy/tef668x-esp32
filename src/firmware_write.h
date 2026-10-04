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

/*
 * It is over. Written, `ok` true: the panel holds the news until the reboot,
 * which the caller books and notes as an update once it is certain. Not
 * written: the panel says so and the radio is given back, since no reboot is
 * coming.
 */
void firmwareWriteEnd(bool ok);

#endif /* FIRMWARE_WRITE_H */
