/*
 * LVGL joined to this radio's panel.
 *
 * The one place that knows about both LVGL and `drivers/display.h`. `ui/` talks
 * to `core/` through an API and never touches a driver, and the panel driver is
 * ours while the toolkit is not, so the joint between the two lives out here in
 * the composition root beside `radio_task.cpp` and `main.cpp`.
 *
 * Everything above this draws with LVGL and never sees a pixel. Everything
 * below it takes a rectangle and pushes it. `displayPush` is the whole of the
 * interface between them, because that one call is all LVGL asks of a display
 * driver.
 */
#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "core/push_time.h"

/*
 * How many lines of the screen one draw buffer holds.
 *
 * There is no PSRAM, so a full 320x240 frame at two bytes a pixel is 150 KB
 * and out of the question. LVGL draws a strip at a time instead, and this is
 * how tall a strip is: 320 by 10 pixels, 6.25 KB, and a whole screen is 24
 * of them.
 *
 * Bigger is fewer strips. Smaller is less RAM, which is what this radio is
 * shortest of, and RAM wins here. LVGL draws each object once for every strip
 * it crosses, so a full repaint takes about 200 ms at 10 lines against about
 * 145 ms at 20, and the menu or another screen opens about 20 to 55 ms later.
 * 20 lines would cost 6.4 KB more. One step of the scrolling radio text,
 * 296 by 18, goes to the panel in two pushes.
 *
 * One buffer, not two. Two only helps when the push happens in the background
 * and drawing can carry on into the other one, and `displayPush` writes to
 * the SPI bus and returns when it is done. A second buffer would be RAM that
 * nothing could use.
 */
#define LVGL_DRAW_LINES 10

/*
 * Start LVGL and give it the panel.
 *
 * The panel must already be up: this does not call `displayBegin`. Returns
 * false if LVGL could not be given its display, and a caller that sees that
 * should carry on without a screen rather than stop, because a radio with a
 * dead panel still has to be reachable over the air to be fixed.
 */
bool lvglPortBegin(void);

/*
 * Let LVGL do its work, and tell it what the time is.
 *
 * Call it often from the task that owns the screen and from nowhere else.
 * LVGL is built here with no operating system of its own, which is only safe
 * because exactly one task ever enters it.
 *
 * Returns how long LVGL would like before the next call, in milliseconds.
 */
uint32_t lvglPortPoll(void);

/*
 * Paint what is on the screen now, and do not come back until it is done.
 *
 * `lvglPortPoll` is on a 16 ms refresh period, so two calls close together
 * paint once and a single call right after the last one paints nothing at
 * all. That is fine for the radio, which is redrawn many times a second, and
 * wrong for a screen that is drawn once and then has to be visible: a
 * firmware write that fails puts up two screens microseconds apart, and the
 * second one would never reach the panel.
 *
 * Same rule as the poll: from the task that owns the screen and nowhere else.
 */
void lvglPortRefreshNow(void);

/*
 * Have the whole screen drawn again on the next poll, not only the parts
 * that changed. For a panel that turned: what is already on the glass stays
 * where it was written.
 *
 * Same rule as the poll: from the task that owns the screen and nowhere else.
 */
void lvglPortRedrawAll(void);

/*
 * How much of LVGL's heap is in use, for the diagnostics page.
 *
 * A screen that quietly runs out of memory stops updating and looks like a
 * frozen radio, so this is worth being able to see before that happens.
 * `peakUsed` is the most it has had in use since boot, as LVGL counts its
 * blocks: their payload only, without the block headers and the pool's own
 * control data, and without fragmentation. So a pool sized from it has to
 * leave room for those, and is judged against `totalBytes`, the part a
 * block can come from, never against LV_MEM_SIZE. Any pointer may be NULL.
 * Returns false when LVGL is not running.
 */
bool lvglPortMemory(uint32_t *usedBytes, uint32_t *totalBytes,
                    uint8_t *usedPercent, uint32_t *largestFree,
                    uint32_t *peakUsed);

/*
 * The pushes to the panel in the last whole second: how many, how many
 * pixels, how long they took in all and the longest one. The time is the
 * push alone, waiting for the SPI bus included, not LVGL's drawing into its
 * buffer. The task that runs LVGL waits for every push, so the total is
 * time that task could not spend on anything else. Beside them the longest
 * refresh, drawing and pushes together, which for a screen drawn again whole
 * is how long that takes.
 *
 * Returns false when LVGL is not running or no whole second has passed yet.
 */
bool lvglPortPushes(PushSecond *out);

#endif /* LVGL_PORT_H */
