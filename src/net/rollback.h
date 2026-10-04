/*
 * Makes a bad update cost nothing but a reboot.
 *
 * The bootloader is built with CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE, so an
 * image written over the air starts in the pending verify state. If it never
 * says it is healthy, the next boot goes back to the slot it came from.
 *
 * The radio says it is healthy once it has been on the network its settings
 * ask for, wifiOnWantedNetwork(), for ROLLBACK_HEALTHY_HOLD_MS, 10 s. A
 * hotspot it fell back to does not count. That is the smallest check that
 * proves another update can still be pushed, which is the thing that must
 * never break.
 */
#ifndef NET_ROLLBACK_H
#define NET_ROLLBACK_H

#include <Arduino.h>

/*
 * How long the radio has to prove itself before it is rolled back.
 *
 * Seven minutes, and the figure is arithmetic rather than taste: it has to be
 * long enough to contain a second attempt at the network, or a router that
 * happens to be down takes a perfectly good image away.
 *
 * A join gives up after `WIFI_JOIN_TIMEOUT_MS`, 20 s, and the next one is
 * `WIFI_AP_RETRY_INTERVAL_MS`, 300 s, after that. So one attempt fails by
 * about 22 s, the second finishes by about 342 s, and 420 s leaves room for
 * the ten second healthy hold on top.
 *
 * The cost is that a genuinely broken image sits there for seven minutes
 * instead of two. That is the right way round: a slow recovery from a real
 * failure is better than a fast recovery from a failure that did not happen.
 */
#define ROLLBACK_VERIFY_TIMEOUT_MS 420000UL

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
