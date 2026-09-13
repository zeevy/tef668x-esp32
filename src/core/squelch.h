/*
 * When to silence the audio between stations.
 *
 * Pure logic, no hardware. Given a reading and a mode it says open or shut,
 * which is what makes the awkward parts testable on a PC: the hold that stops
 * it chattering on a signal sitting exactly on the threshold, the different
 * rule each band needs, and the fact that it must never override a deliberate
 * mute.
 *
 * The three modes exist because there is one knob. The pot on this radio is
 * the volume control or the squelch control, never both at once, so which job
 * it has follows from the mode:
 *
 * | Mode | The pot is | The threshold comes from |
 * |---|---|---|
 * | Off | volume | nothing, the audio is always open |
 * | Auto | volume | the signal quality |
 * | Manual | the squelch | the pot |
 *
 * There is one Auto, not one per band. The rule inside it has to differ by
 * band, because the tuner reports multipath on FM and not on AM and the two
 * sides need different offset tolerances, but that is the radio's problem and
 * not something to choose between.
 */
#ifndef CORE_SQUELCH_H
#define CORE_SQUELCH_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "band_plan.h"
#include "signal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* What decides whether the audio is open. */
typedef enum {
  SQUELCH_OFF = 0, /* Always open. The pot is the volume. */
  SQUELCH_AUTO,    /* From the signal quality. The pot is the volume. */
  SQUELCH_MANUAL,  /* From a threshold, which the pot sets. */
  SQUELCH_MODE_COUNT
} SquelchMode;

const char *squelchModeName(SquelchMode mode);

/*
 * The FM level floor this radio ships with, in whole dBuV.
 *
 * Measured, and the working is in test/fixtures/squelch/README.md. It sits
 * above the loudest reading the channel beside a strong station produced,
 * and below the weakest station ever measured here, which was 19.6 dBuV in
 * the seek sweep.
 */
#define SQUELCH_FM_LEVEL_FLOOR_DBUV 15

/*
 * The highest floor a person may ask for, in whole dBuV.
 *
 * Every station measured on this radio reads above 19.6 dBuV, so a floor
 * past 40 would be asking the squelch to mute the whole band. The range
 * stops well short of that rather than letting somebody switch their radio
 * off by accident.
 */
#define SQUELCH_FM_LEVEL_FLOOR_MAX_DBUV 40

/* Put in fmLevelFloorTenths to switch the level floor off entirely. */
#define SQUELCH_LEVEL_FLOOR_OFF INT16_MIN

/*
 * How many readings the level average needs before the floor is applied.
 *
 * The average takes its first reading as the answer outright, so until a few
 * have gone in there is no smoothing and the floor would be judged on a
 * single raw sample. That cuts both ways and both are bad: a shoulder's raw
 * reading reaches well above the floor and would open the audio, and a
 * station's first reading after a retune can be taken before the tuner has
 * settled and would mute one.
 *
 * So the floor waits, and until it does the noise, multipath and offset
 * tests decide on their own, which is what the squelch did before the floor
 * existed. Three tenths of a second at the radio's hundred millisecond poll.
 *
 * Every retune starts that wait again, so the floor is not applied for a
 * moment after each step of the dial. That is deliberate and it is the right
 * way round: the floor exists for a radio left sitting on the channel beside
 * a strong station, and a dial being turned is not sitting anywhere. Judging
 * it on the first reading instead would risk muting a station because its
 * reading arrived before the tuner had settled, and muting a station is the
 * fault this squelch is least allowed to have.
 */
#define SQUELCH_LEVEL_SETTLE 3

/*
 * How many of the last few readings have to be good to open the audio.
 *
 * One is not enough. The channel beside a strong station produces isolated
 * readings that pass every test at once, and because opening is immediate
 * and shutting waits out the hold, one such reading unmutes for a full
 * second. Measured on 13 September 2026 on 91.0, beside 91.1: two readings
 * in sixty passed everything, they were three apart, and each one on its own
 * would have opened the audio.
 *
 * Two in a row is the wrong shape though, and would be worse than the fault
 * it fixes. A signal sitting exactly on a limit alternates good and bad from
 * one reading to the next, so it never gets two together and would stay
 * muted for ever. That is a station somebody is listening to.
 *
 * Two of the last three does both. Alternating readings reach it on the
 * second good one, and readings three apart never do. The cost is one extra
 * poll before a station is heard, a tenth of a second, against the 0.17 to
 * 0.25 s the retune itself already takes.
 */
#define SQUELCH_OPEN_READINGS 2

/* How far back to look for them. */
#define SQUELCH_OPEN_WINDOW 3

/* What a reading has to beat for the audio to open. */
typedef struct {
  uint16_t fmNoiseTenths; /* FM ultrasonic noise limit, tenths of a percent. */
  uint16_t fmMultipathTenths; /* FM multipath limit. */
  uint16_t fmOffsetTenths;    /* How far off centre FM may sit, tenths kHz. */
  uint16_t amNoiseTenths;     /* AM noise limit. */
  uint16_t amOffsetTenths;    /* How far off centre AM may sit. */
  uint16_t holdMs;            /* How long a bad reading must last to shut. */
  /*
   * The level an FM signal has to reach, in tenths of a dBuV.
   *
   * SQUELCH_LEVEL_FLOOR_OFF switches it off. A sentinel rather than 0,
   * because a dead channel on this radio reads a little below zero, so 0.0
   * is a floor somebody might reasonably want and it must not collide with
   * "no floor".
   *
   * Judged on the smoothed level, not the reading, which is the whole point
   * of it, and only once the average has settled. See the comment on
   * squelchDefaults and SQUELCH_LEVEL_SETTLE.
   */
  int16_t fmLevelFloorTenths;
} SquelchConfig;

/*
 * The thresholds this radio ships with.
 *
 * From the working PE5PVB firmware, at its default sensitivity of 4. Its
 * noise limit is that setting times 30, so 120. The multipath limit of 230
 * and the offset tolerances are fixed there.
 *
 * These are thresholds, so CLAUDE.md applies: they are not invented. They are
 * also on that firmware's level scale, which this one now matches since it
 * writes the same -7.0 dB level offset. See HARDWARE.md.
 *
 * The hold only exists on the FM side there. It is applied to both here,
 * because a signal sitting exactly on the limit chatters on either band and
 * there is no reason AM should be the one that rattles.
 *
 * The FM level floor is this radio's own, measured here and not taken from
 * that firmware, which has no such gate and chatters on the shoulder of a
 * strong station because of it. See test/fixtures/squelch/README.md.
 */
void squelchDefaults(SquelchConfig *out);

/* What the squelch has seen. Call squelchInit before first use. */
typedef struct {
  /*
   * The level, smoothed, which is what the FM level floor is judged on.
   *
   * A station holds its level steady and the shoulder of a station does not.
   * Measured on 13 September 2026, the shoulder of 101.9 swung from -1.8 to
   * 25.8 dBuV between one reading and the next while the weakest real
   * station moved 2.1 dB in fifty readings. Smoothed, the shoulder collapses
   * to 2.0 to 18.2 and the station does not move at all.
   *
   * So the swing is the signal here, and a running average is what turns it
   * into a number the floor can be set against.
   */
  SignalAverage level;
  /* How many readings have gone into that average, held at the settle. */
  uint8_t levelSamples;
  /*
   * Whether each of the last few readings was good, newest in bit 0.
   *
   * Only the lowest SQUELCH_OPEN_WINDOW bits are read. A history rather than
   * a count of consecutive readings, so that a signal alternating either
   * side of a limit can still open the audio.
   */
  uint8_t goodHistory;
  bool open;        /* The audio is open now. */
  uint32_t badMs;   /* When the reading first went bad. */
  bool waiting;     /* A bad reading is being held before shutting. */
  uint32_t lostMs;  /* When the readings started failing. */
  bool lost;        /* Readings are failing now. */
  SquelchMode mode; /* The mode the hold was started under. */
  BandId band;      /* The band it was started on. */
} Squelch;

/*
 * Set a squelch up, open.
 *
 * Open, not shut, and not merely zeroed. A zeroed squelch is a shut one, and
 * a radio that starts shut and has not yet had a reading it can judge is a
 * radio that comes up silent for no reason it can explain.
 */
void squelchInit(Squelch *s);

/*
 * The dial has moved, so the readings from before it say nothing.
 *
 * Starts the level average again without touching whether the audio is open.
 * Without it the average carries the old station's level across a retune, and
 * landing on a station from a shoulder would hold the audio shut for about a
 * second while the average climbed, which is the front of the station gone.
 *
 * squelchInit does this too. This exists for a plain retune, where the audio
 * should carry on as it was rather than being reopened.
 */
void squelchRetuned(Squelch *s);

/* One reading, as much of it as the squelch cares about. */
typedef struct {
  bool valid;               /* The reading came back. False shuts nothing. */
  int16_t levelTenths;      /* Signal level, tenths of a dBuV. */
  uint16_t noiseTenths;     /* Ultrasonic noise. */
  uint16_t multipathTenths; /* Multipath. FM only. */
  int16_t offsetTenths;     /* How far off centre, tenths of a kHz. */
} SquelchReading;

bool squelchUpdate(Squelch *s, const SquelchConfig *cfg, SquelchMode mode,
                   BandId band, const SquelchReading *reading,
                   int16_t thresholdTenths, uint32_t nowMs);

/*
 * Turn a pot reading into a manual squelch threshold.
 *
 * The bottom of the travel is always open and the top is the highest level
 * the chip reports, so the whole knob is usable.
 *
 * The ends are given rather than assumed. A pot that only reaches 200 to 3800
 * would otherwise lose travel at both ends with nothing to say so: the knob
 * would sit at "always open" for the first stretch and hit the ceiling before
 * the end. Pass 0 and 0 for the full converter range, which is what an
 * uncalibrated radio uses.
 */
int16_t squelchThresholdFromPot(uint16_t raw, uint16_t rawMin, uint16_t rawMax);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SQUELCH_H */
