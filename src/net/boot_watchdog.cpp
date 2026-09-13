/* Implementation of the boot timer. */
#include "boot_watchdog.h"

#include <Arduino.h>
#include <esp_system.h>
#include <esp_timer.h>

static esp_timer_handle_t sTimer = NULL;

static void onTimeout(void *arg) {
  (void)arg;
  ets_printf("[boot] setup did not finish in time, restarting\n");
  esp_restart();
}

void bootWatchdogArm(void) {
  if (sTimer != NULL) {
    return;
  }
  esp_timer_create_args_t args = {};
  args.callback = onTimeout;
  args.dispatch_method = ESP_TIMER_TASK;
  args.name = "bootwdt";
  if (esp_timer_create(&args, &sTimer) != ESP_OK) {
    sTimer = NULL;
    return;
  }
  esp_timer_start_once(sTimer, (uint64_t)BOOT_WATCHDOG_TIMEOUT_MS * 1000ULL);
}

void bootWatchdogDisarm(void) {
  if (sTimer == NULL) {
    return;
  }
  esp_timer_stop(sTimer);
  esp_timer_delete(sTimer);
  sTimer = NULL;
}
