/*
 * What LVGL calls when one of its assertions fails.
 *
 * Kept in its own header because `lv_conf.h` names it, and `lv_conf.h` is
 * pulled into every LVGL translation unit. Anything heavier than a single
 * declaration here would be compiled a hundred times over.
 */
#ifndef LVGL_ASSERT_H
#define LVGL_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Says what happened on the serial port, then restarts. Never returns. */
void lvglAssertFailed(void);

#ifdef __cplusplus
}
#endif

#endif /* LVGL_ASSERT_H */
