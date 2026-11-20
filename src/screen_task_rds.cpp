/* Implementation of the RDS screen's view. */
#include "screen_task_rds.h"

#include "core/band_plan.h"
#include "core/clock.h"
#include "core/rds.h"
#include "core/strings.h"
#include "memory_store.h"
#include "net/ntp.h"
#include "radio_task.h"
#include "screen_rds_state.h"
#include "screen_task.h"
#include "ui/screen.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

/*
 * Fill the RDS screen in from the radio, and draw it.
 */
void screenTaskRdsDraw(uint8_t page) {
  RadioSnapshot snap;
  if (!radioGetSnapshot(&snap)) {
    return;
  }
  static char frequency[48];
  static char sync[32];

  ScreenRdsInputs in;
  memset(&in, 0, sizeof(in));
  in.rds = &snap.rds;
  in.page = page;
  in.region = screenTaskRdsRegion();
  in.reading = radioSeekReading(&snap.quality, snap.qualityValid);
  in.presetSlot = snap.memorySlot;
  in.presetPi = memoryStorePi(snap.memorySlot);
  in.message = screenTaskHeaderMessage();
  /* The header's clock, the same text the radio screen shows: nothing until a
   * server has answered. */
  static char clock[CLOCK_TEXT_LEN];
  if (clockFormat(ntpLocalTime(), clock, sizeof(clock))) {
    in.clock = clock;
  }
  if (bandFormatWithUnit(snap.settings.band, snap.settings.freqKHz, frequency,
                         sizeof(frequency))) {
    in.frequency = frequency;
  }

  const uint32_t nowMs = millis();
  if (!radioRdsEnabled()) {
    /* A decoder that is switched off is not a station without RDS. With
     * `rds` off the radio task stops feeding the decoder and every field
     * reads as it does on a station that carries nothing. */
    in.sync = txt(STR_COMMON_OFF);
  } else if (!snap.rds.synchronised) {
    in.sync = txt(STR_COMMON_NO);
  } else {
    /* Both are millis(), and the lock began before `nowMs` was read. */
    const uint32_t heldMs = nowMs - snap.rds.syncSinceMs;
    if (heldMs < 60000UL) {
      snprintf(sync, sizeof(sync), txt(STR_RDS_FMT_SYNC_S),
               (unsigned)(heldMs / 1000UL));
    } else if (heldMs >= 3600000UL) {
      /* Hours from the first, so a lock held all day still fits its tile. */
      snprintf(sync, sizeof(sync), txt(STR_RDS_FMT_SYNC_H_M),
               (unsigned)(heldMs / 3600000UL),
               (unsigned)((heldMs / 60000UL) % 60UL));
    } else {
      snprintf(sync, sizeof(sync), txt(STR_RDS_FMT_SYNC_M_S),
               (unsigned)(heldMs / 60000UL),
               (unsigned)((heldMs / 1000UL) % 60UL));
    }
    in.sync = sync;
    in.syncGood = true;
  }

  ScreenRds view;
  screenRdsStateBuild(&in, &view);
  screenRdsShow(&view);
}
