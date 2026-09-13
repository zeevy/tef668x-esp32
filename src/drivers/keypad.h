/*
 * The numeric keypad, on a PCA9555 I/O expander at I2C 0x20.
 *
 * Twelve keys on sixteen lines: the digits 0 to 9, an enter key and DX. A
 * line reads low while its key is held.
 *
 * The expander has an interrupt line, but it is only a hint that something
 * changed. The keys are read over I2C either way, because a missed interrupt
 * would otherwise mean a key that never arrives.
 */
#ifndef DRIVERS_KEYPAD_H
#define DRIVERS_KEYPAD_H

#include <stdbool.h>
#include <stdint.h>

/* The enter key, next to the digits. Digits are 0 to 9 and are themselves. */
#define KEYPAD_ENTER 10

/*
 * The DX key.
 *
 * On line 2 of the expander. The PE5PVB firmware reads every line except this
 * one in its keypad routine, which is why copying that routine left the key
 * dead. Confirmed on this radio: pressing DX pulls line 2 low and no other.
 */
#define KEYPAD_DX 11

/* Nothing is being pressed. */
#define KEYPAD_NONE (-1)

bool keypadBegin(void);

bool keypadPresent(void);

/*
 * Read which key is down.
 *
 * Reports a key once, when it goes down. Holding it does not repeat, and
 * letting go arms the next press. Two keys at once report nothing, because
 * there is no way to tell which one was meant.
 */
bool keypadRead(int8_t *key, uint16_t *lines);

/*
 * The raw sixteen lines, for a diagnostic page.
 *
 * A key that does nothing is either a dead switch or a wrong map, and those
 * look identical from above. This is how they are told apart without opening
 * the radio.
 */
bool keypadRawLines(uint16_t *bits);

#endif /* DRIVERS_KEYPAD_H */
