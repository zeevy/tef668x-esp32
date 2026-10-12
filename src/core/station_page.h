/*
 * The lines of a saved station's page: everything a preset or a station log
 * entry holds, written as the page shows it.
 *
 * Plain C, no hardware: the menu draws these lines on the radio, and the
 * screenshot tool draws the same ones, so the two cannot drift.
 */
#ifndef CORE_STATION_PAGE_H
#define CORE_STATION_PAGE_H

#include <stdbool.h>
#include <stdint.h>

#include "logbook.h"
#include "memory.h"
#include "rds_country.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tiles under the station, two rows of three. */
#define STATION_PAGE_TILES 6

typedef struct {
  char title[40]; /* "106.40 MHz  MAGIC": the frequency and the name. */
  char sub[96];   /* "Today 17:08 · PI 1064 · IN · Pop Music". */
  /* Each tile's label, NULL after the last, and its value. */
  const char *label[STATION_PAGE_TILES];
  char value[STATION_PAGE_TILES][16];
  char text[LOGBOOK_RT_LEN];   /* The radio text, empty for none. */
  char note[LOGBOOK_NOTE_LEN]; /* The note, empty for none. */
} StationPage;

/*
 * A station log entry's page. `nowKnown` with `nowUtc` and the local
 * `offsetMinutes` write its time as the log lists do, "Today 17:08"; with
 * the radio's clock not set, its day and time, "9 Oct 21:10". `region` names the programme type as
 * the RDS pages do. False for a NULL pointer.
 */
bool stationPageLog(const LogbookEntry *e, bool nowKnown, uint32_t nowUtc,
                    int16_t offsetMinutes, RdsRegion region, StationPage *out);

/* A preset's page: slot `slot`, counted from 1. False for a NULL pointer or
 * an empty slot. */
bool stationPagePreset(const MemoryChannel *c, uint8_t slot, StationPage *out);

#ifdef __cplusplus
}
#endif

#endif /* CORE_STATION_PAGE_H */
