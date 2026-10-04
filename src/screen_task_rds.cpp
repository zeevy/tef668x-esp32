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
 * The lock timer for the decoder page, kept here rather than in
 * `core/rds.c`, which knows nothing of how long a person has been looking.
 * `sRdsSyncSinceMs` marks the moment `synchronised` last turned true, so the
 * page can say how long the lock has held. The groups a second, the error
 * rate and the block levels come from the decoder's own last minute,
 * `RdsInfo.minute`.
 */
static uint32_t sRdsSyncSinceMs = 0;
static bool sRdsWasSynchronised = false;

void screenTaskRdsRestart(void) {
  sRdsWasSynchronised = false;
}

/*
 * Fill the RDS screen in from the radio, and draw it.
 *
 * The lock is timed here, since only this task knows how long a person has
 * been looking; everything else is screen_rds_state.cpp's.
 */
void screenTaskRdsDraw(uint8_t page) {
  RadioSnapshot snap;
  if (!radioGetSnapshot(&snap)) {
    return;
  }
  static char frequency[24];
  static char sync[16];
  char number[16];

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
  if (bandFormatFrequency(snap.settings.band, snap.settings.freqKHz, number,
                          sizeof(number))) {
    snprintf(frequency, sizeof(frequency), txt(STR_COMMON_FMT_TWO_WORDS),
             number, bandFrequencyUnit(snap.settings.band));
    in.frequency = frequency;
  }

  const uint32_t nowMs = millis();
  if (!radioRdsEnabled()) {
    /* A decoder that is switched off is not a station without RDS. With
     * `rds` off the radio task stops feeding the decoder and every field
     * reads as it does on a station that carries nothing. */
    in.sync = txt(STR_COMMON_OFF);
    sRdsWasSynchronised = false;
  } else if (!snap.rds.synchronised) {
    in.sync = txt(STR_COMMON_NO);
    sRdsWasSynchronised = false;
  } else {
    if (!sRdsWasSynchronised) {
      sRdsSyncSinceMs = nowMs;
      sRdsWasSynchronised = true;
    }
    const uint32_t heldMs = nowMs - sRdsSyncSinceMs;
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
