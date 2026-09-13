/*
 * Makes a bad update cost nothing but a reboot.
 *
 * The bootloader is built with CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE, so an
 * image written over the air starts in the pending verify state. If it never
 * says it is healthy, the next boot goes back to the slot it came from.
 *
 * The radio says it is healthy once the network is up and the web server has
 * been answering for a few seconds. That is the smallest check that proves
 * another update can still be pushed, which is the thing that must never
 * break.
 */
#ifndef NET_ROLLBACK_H
#define NET_ROLLBACK_H

#include <Arduino.h>

/* How long the radio has to prove itself before it is rolled back. */
#define ROLLBACK_VERIFY_TIMEOUT_MS 120000UL

/* How long the self check has to hold before the image is marked good. */
#define ROLLBACK_HEALTHY_HOLD_MS 10000UL

void rollbackBegin(void);

/*
 * Feed the self check.
 *
 * Once healthy has held for ROLLBACK_HEALTHY_HOLD_MS the image is marked
 * good and rollback is cancelled. If ROLLBACK_VERIFY_TIMEOUT_MS passes
 * without that, the radio reboots and the bootloader puts the old image back.
 */
void rollbackTick(bool healthy);

bool rollbackPending(void);

const char *rollbackRunningPartition(void);

const char *rollbackStateText(void);

#endif /* NET_ROLLBACK_H */
