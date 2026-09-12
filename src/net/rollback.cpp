/**
 * @file rollback.cpp
 * @brief Implementation of the pending verify self check.
 */
#include "rollback.h"

#include <esp_err.h>
#include <esp_ota_ops.h>

static bool sPending = false;
static bool sMarkedGood = false;
static uint32_t sStartedMs = 0;
static uint32_t sHealthySinceMs = 0;
static bool sHealthyHeld = false;
static char sPartition[17] = "unknown";

void rollbackBegin(void) {
  sStartedMs = millis();
  sHealthySinceMs = 0;
  sHealthyHeld = false;

  const esp_partition_t *running = esp_ota_get_running_partition();
  if (running != NULL) {
    snprintf(sPartition, sizeof(sPartition), "%s", running->label);
  }

  esp_ota_img_states_t state;
  if (running != NULL &&
      esp_ota_get_state_partition(running, &state) == ESP_OK) {
    sPending = (state == ESP_OTA_IMG_PENDING_VERIFY);
  } else {
    sPending = false;
  }

  if (!sPending) {
    /* Either this image was already accepted, or it came in over the cable
     * and was never on trial. Nothing to prove. */
    sMarkedGood = true;
  }
}

void rollbackTick(bool healthy) {
  if (sMarkedGood) {
    return;
  }

  uint32_t now = millis();

  if (healthy) {
    if (sHealthySinceMs == 0) {
      sHealthySinceMs = now;
    }
    if (!sHealthyHeld && now - sHealthySinceMs >= ROLLBACK_HEALTHY_HOLD_MS) {
      sHealthyHeld = true;
    }
  } else {
    /* Lost it again. The hold has to start over. */
    sHealthySinceMs = 0;
    sHealthyHeld = false;
  }

  if (sHealthyHeld) {
    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    if (err == ESP_OK) {
      sMarkedGood = true;
      sPending = false;
      Serial.println("[rollback] self check passed, image marked good");
      return;
    }
    /* The mark failed, which usually means otadata could not be written.
     * Fall through to the timeout rather than sitting on trial forever. */
    Serial.printf("[rollback] could not mark the image good: %s\n",
                  esp_err_to_name(err));
  }

  if (now - sStartedMs >= ROLLBACK_VERIFY_TIMEOUT_MS) {
    Serial.println("[rollback] self check did not pass in time, going back to "
                   "the previous image");
    Serial.flush();
    /* Reboot without marking the image good. The bootloader sees an image
     * still in pending verify and boots the other slot instead. */
    esp_restart();
  }
}

bool rollbackPending(void) {
  return sPending && !sMarkedGood;
}

const char *rollbackRunningPartition(void) {
  return sPartition;
}

const char *rollbackStateText(void) {
  if (sMarkedGood) {
    return "confirmed";
  }
  if (sPending) {
    return "on trial, waiting for the self check";
  }
  return "unknown";
}
