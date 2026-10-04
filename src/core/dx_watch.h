/*
 * The preset watch: while DX mode is open, the FM presets in DX mode's range
 * are read in turn by AF_Update checks, each a few milliseconds away from
 * the station being heard, and a preset whose level comes up is flagged.
 *
 * Pure logic, no hardware. Which channel to check next, and what a
 * reading means; the checks themselves are the radio task's. The numbers
 * are from AF_Update checks measured on this radio.
 */
#ifndef CORE_DX_WATCH_H
#define CORE_DX_WATCH_H

#include <stdbool.h>
#include <stdint.h>

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * How many readings of a channel its own floor is the median of. Five keeps
 * the largest rise of a quiet channel over it the smallest, 4.3 dB over 975
 * checks, against 5.5 with three and with seven.
 */
#define DX_WATCH_HISTORY 5

/*
 * How far above that floor a reading must be, twice running, in tenths of
 * a dB. In 1080 checks of nine quiet channels on this radio, one reading never
 * rose more than 5.9 dB over the median of the five before it, and a reading
 * and the next together never more than 5.2. 8 dB leaves 2 dB for the tail
 * those could not show. A station less than 8 dB over the channel's floor
 * is not flagged.
 */
#define DX_WATCH_RISE_TENTHS 80

/* As many as there are presets. */
#define DX_WATCH_MAX MEMORY_SLOT_COUNT

/* One watched channel and its last readings. */
typedef struct {
  uint32_t khz;
  int16_t recent[DX_WATCH_HISTORY]; /* Tenths of a dBuV, a ring. */
  uint8_t held;                     /* How many of `recent` are real. */
  uint8_t next;                     /* Where the next one goes. */
} DxWatchChannel;

/* The watch. Zeroed is one with nothing to watch. */
typedef struct {
  DxWatchChannel ch[DX_WATCH_MAX];
  uint8_t count;
  uint8_t cursor;  /* The channel checked next. */
  bool confirming; /* The last reading of `cursor` rose, and is taken again. */
} DxWatch;

/* What a reading meant. */
typedef enum {
  DX_WATCH_QUIET = 0, /* Nothing, or not enough readings yet to say. */
  DX_WATCH_SUSPECT,   /* Rose; the same channel is read again next. */
  DX_WATCH_UP,        /* Rose twice running: flag it. */
} DxWatchResult;

/*
 * The channels to watch, in the order given, up to DX_WATCH_MAX. A channel
 * that was already watched keeps its readings, so the list can be given
 * again whenever the presets or the dial may have changed. A NULL or empty
 * list watches nothing.
 */
void dxWatchSetChannels(DxWatch *w, const uint32_t *khz, uint8_t count);

/* The channel to check next, in `khz`. False with nothing to watch. */
bool dxWatchNext(const DxWatch *w, uint32_t *khz);

/*
 * A check of `khz` came back: `ok` false for one that gave no reading.
 * Says what it meant, with `rise`, when not NULL, how far above the
 * channel's floor it read, in tenths of a dB, or 0 before there is a floor.
 * A reading that rose is left out of the floor, and once a channel is
 * flagged its floor starts again from its new level, so a station that
 * stays up is flagged once, not at every check. A `khz` that is not the
 * channel due is ignored and says QUIET.
 */
DxWatchResult dxWatchFeed(DxWatch *w, uint32_t khz, int16_t levelTenths,
                          bool ok, int16_t *rise);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DX_WATCH_H */
