/*
 * The heap as the menu, the state document and the system page show it,
 * read in one place so the three cannot disagree on what a figure means or
 * how often it is safe to read it.
 *
 * Called from the loop task only: the menu and the web server both run
 * there.
 */
#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stdint.h>

/* Free heap now, in bytes. */
uint32_t systemHeapFree(void);

/* The lowest the free heap has been since boot, in bytes. */
uint32_t systemHeapLowest(void);

/*
 * The largest free block, in bytes: what a big allocation needs, and what
 * fragmentation eats. Read at most once a second, because the read walks
 * the heap under its lock, so a figure up to a second old.
 */
uint32_t systemHeapLargest(void);

#endif /* SYSTEM_INFO_H */
