/*
 * The one I2C bus, and the lock that stops two tasks talking at once.
 *
 * The tuner, the keypad expander and the clock all sit on the same two pins.
 * The radio task on core 0 reads the tuner, and the loop task on core 1 reads
 * the keypad. Wire has no lock of its own, so without this the two interleave.
 *
 * Interleaving does not fail loudly. A read is a write of the register wanted
 * followed by a read of the answer, and if another device's transaction lands
 * between those two, the read returns zeros and reports success. That looks
 * exactly like a radio receiving nothing, which is how it went unnoticed on
 * shortwave: a dead band and a broken read both read zero.
 *
 * So the rule is: a whole transaction, not a single Wire call, happens inside
 * the lock. The lock is recursive, because the tuner's read path calls its own
 * write path.
 */
#ifndef DRIVERS_I2C_BUS_H
#define DRIVERS_I2C_BUS_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Start the bus and make the lock.
 *
 * Safe to call again. The first caller sets the pins and the clock rate.
 */
void i2cBusBegin(uint32_t hz);

bool i2cBusTake(uint32_t waitMs);

void i2cBusGive(void);

/* How long a driver should wait for the bus before giving up, in ms. */
#define I2C_BUS_WAIT_MS 100

#endif /* DRIVERS_I2C_BUS_H */
