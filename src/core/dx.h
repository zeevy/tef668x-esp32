/*
 * FM DX rules that are not already the seek's or the RDS decoder's.
 *
 * Pure logic, no hardware, set from DX readings recorded on this radio, and
 * tested by replaying them.
 *
 * Three questions a DX receiver asks, and where each is answered:
 *
 * - Is there a signal on this channel? `seekShouldStop` in seek.h. In the
 *   readings it stops on the ten stations on air and nothing else, at every
 *   sensitivity from 4 to 6.
 * - Is this PI real? The RDS decoder, after two clean receptions. Noise gave
 *   one clean block A in 206 channels and never two.
 * - Is this PI this channel's? `dxPiConfirmed` below. In the readings it is
 *   not, on every channel beside a strong station.
 *
 * So only the last needed a new rule.
 */
#ifndef CORE_DX_H
#define CORE_DX_H

#include <stdbool.h>
#include <stdint.h>

#include "logbook.h"
#include "rds.h"
#include "rds_country.h"
#include "seek.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Whether the PI the RDS decoder confirmed belongs to the channel tuned.
 *
 * The decoder's own rule holds against noise. What it cannot see is the
 * station next door. On a channel beside a strong station the chip decodes
 * that station's RDS through its sidebands, cleanly: on 93.4 and 93.6 MHz it
 * confirmed 93.5's 0935, and on 98.2 and 98.4 it confirmed 98.3's 26FF. A DX
 * log would record each as a catch on the wrong frequency.
 *
 * The offset tells them apart. The ten stations read 3.2 to 6.2 kHz off
 * centre, because the tuner crystal runs about 50 ppm fast. The channels
 * beside them read 73 to 110 kHz off, on every one of the 69 readings taken
 * over a minute on two of them, because the carrier the chip locks to really
 * is 100 kHz away. 27 of those same readings passed the seek's noise,
 * multipath and level limits at the default, so none of those could have
 * done this. Only strong neighbours were measured.
 *
 * So the PI counts only while the carrier sits inside the seek's own FM
 * offset window. Noise, multipath and level are not asked. A weak DX station
 * whose PI came through clean twice is a catch even when its noise is above
 * what a seek would stop on, and nothing measured yet says how noisy one
 * reads.
 *
 * False when either is NULL, the reading did not arrive, or no PI has been
 * confirmed.
 */
bool dxPiConfirmed(const RdsInfo *rds, const SeekReading *reading);

/*
 * Whether the carrier the chip is locked to is this channel's own: inside
 * the seek's FM offset window, the half of `dxPiConfirmed` that does not
 * need a PI. For anything else RDS gives that is only true of the channel's
 * own station, such as its name. False for a NULL reading or one that did
 * not arrive.
 */
bool dxOnChannel(const SeekReading *reading);

/*
 * Fill in the name and PI a log entry made by hand keeps: only a name and a
 * PI that are this channel's own. Beside a strong station the decoder spells
 * that station's name and confirms its PI, and logging them would put the
 * station 100 kHz from where it is. The name has no PI to test, so it takes
 * the offset alone, which keeps the name of a station that sends PI 0000.
 * Neither is kept when `rds` or `reading` is NULL.
 */
void dxLogIdentity(LogbookEntry *e, const RdsInfo *rds,
                   const SeekReading *reading);

/*
 * Whether the confirmed PI is still being heard: `dxPiConfirmed`, and the
 * last group came with a clean block A carrying that same PI. The decoder
 * keeps a confirmed PI until the dial moves, so on its own it says the
 * station was heard, not that it still is. False once the RDS lock is lost.
 */
bool dxPiHeardNow(const RdsInfo *rds, const SeekReading *reading);

/*
 * Put the station's radio text in a log entry when all of these hold, and
 * leave the entry with none otherwise:
 *
 * - the decoder has a whole radio text;
 * - the dial is on the entry's channel, `dialKHz`, since the text on the air
 *   anywhere else is another station's;
 * - the carrier is the channel's own, `dxOnChannel`, the test the name
 *   takes, since beside a strong station the decoder spells that station's
 *   text;
 * - and, when the entry has a PI, the decoder's confirmed PI is that one.
 */
void dxLogRadioText(LogbookEntry *e, const RdsInfo *rds,
                    const SeekReading *reading, uint32_t dialKHz);

/*
 * The fixed width DX mode puts in force on entry, in kHz, from the tuner's
 * own list.
 *
 * The narrowest that leaves RDS whole. The RDS carrier sits at 57 kHz, and a
 * filter much narrower than the space it and the programme take up cuts into
 * it: measured on 93.5 over 20 seconds, a clean block A came in 6 times in 41
 * at 56 kHz and 114 in 168 at 84, against 209 in 210 at 114. At every one of
 * those widths the channels 100 kHz either side heard none of 93.5's groups,
 * where at 217 kHz, the width the chip picked on its own, they confirmed its
 * PI. The filter is the chip's, so the same holds on any of these radios.
 */
#define DX_BANDWIDTH_DEFAULT_KHZ 114

/* ------------------------------------------------------------ presets -- */

/*
 * What the PI says about the preset the dial is on, when that preset has a
 * stored PI.
 *
 * The preset's own station from the first clean block A that carries the
 * stored PI, on this channel by the offset, rather than the two a PI needs
 * to be confirmed: a weak station gives few clean blocks, and one that
 * matches a known PI is that station, since a misread block landing on one
 * particular sixteen bit value is about one in 65,536. Another station only
 * once a PI that is not the stored one is confirmed here, so a misread
 * block never says so.
 */
typedef enum {
  DX_PRESET_NONE,  /* No stored PI, or nothing heard to say either way. */
  DX_PRESET_MATCH, /* The preset's own station. */
  DX_PRESET_OTHER  /* Another station, confirmed, on the preset. */
} DxPresetPi;

DxPresetPi dxPresetPi(const RdsInfo *rds, const SeekReading *reading,
                      uint16_t storedPi);

/*
 * The PI a preset with none should learn: true, and `*out` set, when
 * `storedPi` is 0 and a PI is confirmed as this channel's own. A stored PI
 * is never replaced this way, so another station passing through cannot
 * take the preset over.
 */
bool dxPresetLearn(const RdsInfo *rds, const SeekReading *reading,
                   uint16_t storedPi, uint16_t *out);

/* ------------------------------------------------------------ the page -- */

/*
 * What the PI tile on the DX page shows. The page draws each one differently,
 * by shape and not only by colour.
 */
typedef enum {
  DX_PI_NONE,      /* Nothing heard: an empty grey tile. */
  DX_PI_SEEN,      /* Heard, no digit in doubt, not confirmed here yet. */
  DX_PI_PARTIAL,   /* Heard, and a digit changed between the last two. */
  DX_PI_CONFIRMED, /* Confirmed, and this channel's own. */
  DX_PI_ZERO       /* The station sends 0000. */
} DxPiTile;

/*
 * Which of those it is. A PI the decoder confirmed but that is not this
 * channel's, by `dxPiConfirmed`, is only seen: the tile never goes amber for
 * the station next door.
 */
DxPiTile dxPiTile(const RdsInfo *rds, const SeekReading *reading);

/* Who the station is, for the line beside its PI. */
typedef struct {
  /* The country's two letter code from the ECC and the PI, or NULL. */
  const char *country;
  /* In North America the call letters, which take the country's place:
   * RDS_CALL_NONE when there are none, RDS_CALL_GUESS when they are only
   * worked out from the PI. */
  RdsCall call;
  char callText[RDS_CALL_TEXT_LEN];
  /* On a preset with a stored PI, whether this is its station. */
  DxPresetPi preset;
} DxIdentity;

/*
 * Who the station is, from its RDS and the reading it came with: the
 * country, the call letters and whether it is the preset's own station, by
 * the one rule the DX page, the RDS screen and the API all show them by.
 *
 * `pageRule` is the DX page's stricter one: a country or call letters only
 * for a PI confirmed on this channel's own carrier, so the station next door
 * is never named. Without it they are named once the PI is known at all, as
 * the RDS screen does. `presetPi` is the preset's stored PI, or 0 when the
 * radio is on no preset or the preset has none.
 */
void dxStationIdentity(const RdsInfo *rds, const SeekReading *reading,
                       RdsRegion region, uint16_t presetPi, bool pageRule,
                       DxIdentity *out);

/*
 * The four characters the tile shows, `?` for a digit in doubt. `out` must
 * hold 5 bytes. Empty for DX_PI_NONE and when `out` or `rds` is NULL.
 */
void dxPiDigits(const RdsInfo *rds, DxPiTile tile, char *out);

/*
 * How many of the four segments of a block's meter to light from the block's
 * error level: 4 clean, 3 put right with a small error, 2 with a large one,
 * 1 not put right, and 0 for a level below 0, which means no group has
 * arrived, or above 3.
 */
uint8_t dxBlockSegments(int8_t level);

/*
 * The last minute of signal level, one bar a second.
 *
 * Each second keeps the highest reading in it, because a meteor ping can be
 * over in half a second and a sample taken once a second would usually miss
 * it. A second with no reading in it has no bar, which is different from a
 * bar at the bottom: the first is "not known", the second is "nothing
 * there".
 */
#define DX_HISTORY_SECONDS 60

typedef struct {
  int16_t peakTenths[DX_HISTORY_SECONDS];
  bool have[DX_HISTORY_SECONDS];
  uint8_t newest;        /* The slot for `newestSecond`. */
  uint32_t newestSecond; /* Milliseconds / 1000, when it was last added. */
  bool started;
} DxHistory;

/* Empty, as on every retune: a history belongs to one channel. */
void dxHistoryReset(DxHistory *h);

/* One reading, at `nowMs` on the millisecond clock. */
void dxHistoryAdd(DxHistory *h, uint32_t nowMs, int16_t levelTenths);

/*
 * Bar `bar` of the history as it stands at `nowMs`, 0 the oldest and
 * DX_HISTORY_SECONDS - 1 the current second. False when that second had no
 * reading.
 */
bool dxHistoryBar(const DxHistory *h, uint32_t nowMs, uint8_t bar,
                  int16_t *peakTenths);

/*
 * The height of a bar in pixels, on a scale of 0 to 70 dBuV across
 * `innerHeight`. At least one pixel, so a second that had a reading never looks
 * like one that did not.
 */
uint8_t dxHistoryBarHeight(int16_t levelTenths, uint8_t innerHeight);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DX_H */
