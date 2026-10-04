/* Implementation of the two halves of the restart reason. */
#include "restart_reason.h"

#include <Arduino.h>
#include <esp_attr.h>
#include <esp_system.h>
#include <stdio.h>

#include "system_info.h"

/*
 * The note, in RTC memory.
 *
 * `RTC_NOINIT_ATTR` keeps its value across a restart and is not zeroed at
 * start up, which is the whole point: the word has to arrive on the other
 * side of `esp_restart`. It holds nothing meaningful after a power cycle,
 * which is what the marker in `restartWhyPack` is for.
 */
static RTC_NOINIT_ATTR uint32_t sNote;
/* The heap's lowest point beside it, the same way. */
static RTC_NOINIT_ATTR uint32_t sHeap[2];

static char sText[40] = "unknown";
static RestartWhy sWhy = RESTART_WHY_NONE;
static bool sHaveLastHeap = false;
static uint32_t sLastHeap = 0;

static const char *chipReasonText(void) {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:
      return "power";
    case ESP_RST_SW:
      return "software";
    case ESP_RST_PANIC:
      return "panic";
    case ESP_RST_INT_WDT:
      return "interrupt watchdog";
    case ESP_RST_TASK_WDT:
      return "task watchdog";
    case ESP_RST_WDT:
      return "watchdog";
    case ESP_RST_DEEPSLEEP:
      return "deep sleep";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_EXT:
      return "reset pin";
    case ESP_RST_SDIO:
      return "sdio";
    default:
      return "unknown";
  }
}

void restartReasonBegin(void) {
  const char *chip = chipReasonText();
  RestartWhy why = restartWhyUnpack(sNote);
  sNote = 0;
  uint32_t lowest = 0;
  const bool haveHeap = restartHeapUnpack(sHeap, &lowest);
  sHeap[0] = 0;
  sHeap[1] = 0;

  /*
   * The note is only believed for a software restart. A panic or a brownout
   * that happens to follow one of ours would otherwise be reported as ours,
   * and the chip's answer is the one that is certain.
   */
  sWhy = esp_reset_reason() == ESP_RST_SW ? why : RESTART_WHY_NONE;
  sHaveLastHeap = sWhy != RESTART_WHY_NONE && haveHeap;
  sLastHeap = sHaveLastHeap ? lowest : 0;
  const char *note = restartWhyText(sWhy);
  if (note != NULL) {
    snprintf(sText, sizeof(sText), "%s (%s)", chip, note);
  } else {
    snprintf(sText, sizeof(sText), "%s", chip);
  }
}

void restartReasonNote(RestartWhy why) {
  sNote = restartWhyPack(why);
  restartHeapPack(systemHeapLowest(), sHeap);
}

const char *restartReasonText(void) {
  return sText;
}

void restartReasonParts(esp_reset_reason_t *chip, RestartWhy *why) {
  if (chip != NULL) {
    *chip = esp_reset_reason();
  }
  if (why != NULL) {
    *why = sWhy;
  }
}

bool restartReasonLastHeap(uint32_t *lowestBytes) {
  if (!sHaveLastHeap || lowestBytes == NULL) {
    return false;
  }
  *lowestBytes = sLastHeap;
  return true;
}
