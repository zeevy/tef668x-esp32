/*
 * The RDS screen's view, built from the decoder's answer.
 *
 * Split out of screen_task_rds.cpp so it touches no driver and no clock,
 * and runs on a PC too: tools/screenshot.cpp draws the pages through this
 * same code. What has arrived, what it is called, and how a number is
 * written are all decided here; ui/ draws strings.
 */
#ifndef SCREEN_RDS_STATE_H
#define SCREEN_RDS_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "core/rds.h"
#include "core/rds_country.h"
#include "core/seek.h"
#include "ui/screen.h"

typedef struct {
  const RdsInfo *rds;    /* Never NULL. */
  uint8_t page;          /* 0 to SCREEN_RDS_PAGES - 1. */
  const char *frequency; /* The header's frequency and unit, or NULL. */
  const char *clock;     /* Or NULL until a server has answered. */
  /* The lock as the task times it: off, not locked, or how long. */
  const char *sync;
  bool syncGood;
  RdsRegion region; /* Whether a PI is read as call letters. */
  /* Where the carrier sits, for whether the PI is this channel's own. */
  SeekReading reading;
  /* The preset the dial is on, MEMORY_NO_SLOT for none, and its stored PI,
   * 0 for none learnt. */
  int16_t presetSlot;
  uint16_t presetPi;
  const char *message; /* A moment's message for the header, or NULL. */
} ScreenRdsInputs;

/*
 * Fill `out` in from `in`. Every page is filled, not only `page`: they are
 * cheap. The strings point into buffers of this file, good until the next
 * call.
 */
void screenRdsStateBuild(const ScreenRdsInputs *in, ScreenRds *out);

#endif /* SCREEN_RDS_STATE_H */
