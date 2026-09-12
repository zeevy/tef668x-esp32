/**
 * @file boot_watchdog.h
 * @brief Restarts the radio if setup never finishes.
 *
 * Rollback only helps if the radio gets as far as rebooting. An image that
 * hangs inside setup, before the main loop ever runs, would sit there in the
 * pending verify state forever and the only way out would be the cable. That
 * is the exact failure this phase exists to make impossible.
 *
 * So a timer is armed as the first thing setup does. If setup does not disarm
 * it in time the timer restarts the chip, the bootloader sees an image still
 * on trial, and the previous image comes back.
 *
 * The timer runs in the esp_timer task, which is above the Arduino loop task,
 * so it still fires when the loop task is stuck in a busy wait.
 */
#ifndef NET_BOOT_WATCHDOG_H
#define NET_BOOT_WATCHDOG_H

#include <stdint.h>

/** How long setup gets before the radio is restarted. */
#define BOOT_WATCHDOG_TIMEOUT_MS 45000UL

/** Arm the timer. Call as the first statement in setup. */
void bootWatchdogArm(void);

/** Disarm the timer. Call once setup has finished. */
void bootWatchdogDisarm(void);

#endif /* NET_BOOT_WATCHDOG_H */
