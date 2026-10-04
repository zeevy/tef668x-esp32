/* Implementation of the LVGL port. */
#include "lvgl_port.h"

#include "net/restart_reason.h"

#include <Arduino.h>
#include <esp_system.h>
#include <lvgl.h>

#include "drivers/display.h"

/*
 * The draw buffer.
 *
 * Static rather than allocated, so the memory is accounted for at link time
 * and shows up in the size gate. An allocation this large failing at start up
 * would be the worst moment to find out. On a 4 byte boundary because the SPI
 * driver reads what it sends in 32 bit words.
 */
static uint16_t sDrawBuf[320 * LVGL_DRAW_LINES] __attribute__((aligned(4)));

static lv_display_t *sDisplay = NULL;
static bool sReady = false;
static PushTime sPushes;
static uint32_t sRefreshStartUs = 0;

/*
 * Hand one finished rectangle to the panel.
 *
 * LVGL renders into the buffer above and calls this with the piece it has
 * done. The colour format is set below to RGB565 with each pixel's two bytes
 * swapped, high byte first, which is the order the panel takes, so the
 * buffer goes to the panel as it is.
 */
static void flush(lv_display_t *display, const lv_area_t *area,
                  uint8_t *pixels) {
  const int16_t x = (int16_t)area->x1;
  const int16_t y = (int16_t)area->y1;
  const uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
  const uint16_t h = (uint16_t)(area->y2 - area->y1 + 1);

  const uint32_t startUs = micros();
  displayPush(x, y, w, h, pixels);
  pushTimeAdd(&sPushes, millis(), (uint32_t)w * h, micros() - startUs);

  /* The push is finished by the time it returns, so LVGL is free to draw into
   * the buffer again immediately. */
  lv_display_flush_ready(display);
}

/*
 * Time each refresh, from LVGL starting one to the last push of it. Sent on
 * every refresh period, with or without anything to draw, so the longest of
 * a second is the one that drew the most.
 */
static void onRefresh(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_REFR_START) {
    sRefreshStartUs = micros();
    return;
  }
  pushTimeRefresh(&sPushes, millis(), micros() - sRefreshStartUs);
}

/*
 * What LVGL uses for a clock.
 *
 * Given rather than compiled in, so the one source of time on this radio is
 * the one everything else already uses.
 */
static uint32_t tick(void) {
  return millis();
}

bool lvglPortBegin(void) {
  if (sReady) {
    return true;
  }

  lv_init();
  lv_tick_set_cb(tick);

  sDisplay = lv_display_create(displayWidth(), displayHeight());
  if (sDisplay == NULL) {
    return false;
  }

  lv_display_set_color_format(sDisplay, LV_COLOR_FORMAT_RGB565_SWAPPED);
  lv_display_set_flush_cb(sDisplay, flush);
  lv_display_set_buffers(sDisplay, sDrawBuf, NULL, sizeof(sDrawBuf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_display_add_event_cb(sDisplay, onRefresh, LV_EVENT_REFR_START, NULL);
  lv_display_add_event_cb(sDisplay, onRefresh, LV_EVENT_REFR_READY, NULL);
  pushTimeReset(&sPushes, millis());
  sReady = true;
  return true;
}

uint32_t lvglPortPoll(void) {
  if (!sReady) {
    /* Nothing to do, and nothing will change until something starts LVGL, so
     * a caller polling on this answer should not spin. */
    return 1000;
  }
  return lv_timer_handler();
}

void lvglPortRefreshNow(void) {
  if (!sReady) {
    return;
  }
  lv_refr_now(NULL);
}

void lvglPortRedrawAll(void) {
  if (!sReady) {
    return;
  }
  lv_obj_invalidate(lv_screen_active());
}

bool lvglPortMemory(uint32_t *usedBytes, uint32_t *totalBytes,
                    uint8_t *usedPercent, uint32_t *largestFree,
                    uint32_t *peakUsed) {
  if (!sReady) {
    return false;
  }
  lv_mem_monitor_t monitor;
  lv_mem_monitor(&monitor);
  if (usedBytes != NULL) {
    *usedBytes = (uint32_t)(monitor.total_size - monitor.free_size);
  }
  if (totalBytes != NULL) {
    *totalBytes = (uint32_t)monitor.total_size;
  }
  if (usedPercent != NULL) {
    *usedPercent = monitor.used_pct;
  }
  if (largestFree != NULL) {
    *largestFree = (uint32_t)monitor.free_biggest_size;
  }
  if (peakUsed != NULL) {
    *peakUsed = (uint32_t)monitor.max_used;
  }
  return true;
}

bool lvglPortPushes(PushSecond *out) {
  if (!sReady) {
    return false;
  }
  return pushTimeLast(&sPushes, millis(), out);
}

/*
 * The assertion handler named by `lv_conf.h`.
 *
 * The reasoning for restarting rather than halting is written there. The
 * flush is because `esp_restart` does not wait for the serial port to drain,
 * and without it the one line saying why is the line that gets lost.
 */
extern "C" void lvglAssertFailed(void) {
  Serial.println(F("[lvgl] assertion failed, restarting"));
  Serial.flush();
  restartReasonNote(RESTART_WHY_DISPLAY);
  esp_restart();
  /* Not reached. Here so a compiler that cannot see esp_restart is noreturn
   * does not warn about falling out of a function LVGL expects never to
   * return from. */
  while (true) {
  }
}
