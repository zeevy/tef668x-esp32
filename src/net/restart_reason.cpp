/* Implementation of the two halves of the restart reason. */
#include "restart_reason.h"

#include <Arduino.h>
#include <Preferences.h>
#include <esp_attr.h>
#include <esp_private/panic_internal.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <stdio.h>
#include <string.h>

#include "build_id.h"
#include "core/panic_record.h"
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

/*
 * Where the last crash happened. The panic hook fills `sPanicNow` in RTC
 * memory, which the panic's own restart keeps; the next start copies it to
 * flash, which a power cycle keeps too, and clears it. One flash write per
 * crash.
 */
static RTC_NOINIT_ATTR PanicRecord sPanicNow;
static PanicRecord sLastPanic;
static const char *kPanicNamespace = "tef668x";
static const char *kPanicKey = "panic";

/*
 * Runs in the panic itself, through the hook Arduino's own wrapper of the
 * framework's panic handler calls, with the other core stopped: no heap, no
 * locks, no logging, only writes to RTC memory. Then the framework's handler
 * goes on as before.
 */
static void onPanic(arduino_panic_info_t *info, void *arg) {
  (void)arg;
  PanicRecord *r = &sPanicNow;
  memset(r, 0, sizeof(*r));
  if (info != NULL) {
    r->core = (uint8_t)info->core;
    r->pc = (uint32_t)(uintptr_t)info->pc;
    /* An abort(), a failed assert and the task watchdog reach the handler
     * as the trap that stops the chip; their own words, with the file and
     * line of an assert, are in the abort details. */
    const char *why = g_panic_abort && g_panic_abort_details != NULL
                          ? g_panic_abort_details
                          : info->reason;
    strncpy(r->why, why != NULL ? why : "", sizeof(r->why) - 1);
    const unsigned n = info->backtrace_len < PANIC_RECORD_FRAMES
                           ? info->backtrace_len
                           : PANIC_RECORD_FRAMES;
    for (unsigned i = 0; i < n; i++) {
      r->frames[i] = info->backtrace[i];
    }
    r->frameCount = (uint8_t)n;
  }
  strncpy(r->build, FIRMWARE_BUILD, sizeof(r->build) - 1);
  r->uptimeS = (uint32_t)(esp_timer_get_time() / 1000000);
  panicRecordSeal(r);
}

/* A crash written by the handler is copied to flash once; otherwise the one
 * flash already holds is read. */
static void panicRecordBegin(void) {
  Preferences prefs;
  if (panicRecordValid(&sPanicNow)) {
    sLastPanic = sPanicNow;
    if (prefs.begin(kPanicNamespace, false)) {
      prefs.putBytes(kPanicKey, &sLastPanic, sizeof(sLastPanic));
      prefs.end();
    }
  } else if (prefs.begin(kPanicNamespace, true)) {
    if (prefs.getBytesLength(kPanicKey) == sizeof(sLastPanic)) {
      prefs.getBytes(kPanicKey, &sLastPanic, sizeof(sLastPanic));
    }
    prefs.end();
  }
  panicRecordClear(&sPanicNow);
}

void restartReasonWatchCrashes(void) {
  set_arduino_panic_handler(onPanic, NULL);
}

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
  panicRecordBegin();
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

size_t restartReasonLastPanic(char *out, size_t cap) {
  return panicRecordJson(&sLastPanic, out, cap);
}
