/**
 * @file squelch.h
 * @brief When to silence the audio between stations.
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

#include <stdbool.h>
#include <stdint.h>

#include "band_plan.h"

#ifdef __cplusplus
extern "C" {
#endif

/** What decides whether the audio is open. */
typedef enum {
  SQUELCH_OFF = 0, /**< Always open. The pot is the volume. */
  SQUELCH_AUTO,    /**< From the signal quality. The pot is the volume. */
  SQUELCH_MANUAL,  /**< From a threshold, which the pot sets. */
  SQUELCH_MODE_COUNT
} SquelchMode;

/**
 * The mode in words.
 *
 * @param mode  Which mode.
 * @return Its name, or an empty string when it is not a mode. Never NULL.
 */
const char *squelchModeName(SquelchMode mode);

/** What a reading has to beat for the audio to open. */
typedef struct {
  uint16_t
      fmNoiseTenths; /**< FM ultrasonic noise limit, tenths of a percent. */
  uint16_t fmMultipathTenths; /**< FM multipath limit. */
  uint16_t fmOffsetTenths;    /**< How far off centre FM may sit, tenths kHz. */
  uint16_t amNoiseTenths;     /**< AM noise limit. */
  uint16_t amOffsetTenths;    /**< How far off centre AM may sit. */
  uint16_t holdMs;            /**< How long a bad reading must last to shut. */
} SquelchConfig;

/**
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
 * @param out  Receives the defaults.
 */
void squelchDefaults(SquelchConfig *out);

/** What the squelch has seen. Call squelchInit before first use. */
typedef struct {
  bool open;        /**< The audio is open now. */
  uint32_t badMs;   /**< When the reading first went bad. */
  bool waiting;     /**< A bad reading is being held before shutting. */
  uint32_t lostMs;  /**< When the readings started failing. */
  bool lost;        /**< Readings are failing now. */
  SquelchMode mode; /**< The mode the hold was started under. */
  BandId band;      /**< The band it was started on. */
} Squelch;

/**
 * Set a squelch up, open.
 *
 * Open, not shut, and not merely zeroed. A zeroed squelch is a shut one, and
 * a radio that starts shut and has not yet had a reading it can judge is a
 * radio that comes up silent for no reason it can explain.
 *
 * @param s  The squelch. Cleared.
 */
void squelchInit(Squelch *s);

/** One reading, as much of it as the squelch cares about. */
typedef struct {
  bool valid;               /**< The reading came back. False shuts nothing. */
  int16_t levelTenths;      /**< Signal level, tenths of a dBuV. */
  uint16_t noiseTenths;     /**< Ultrasonic noise. */
  uint16_t multipathTenths; /**< Multipath. FM only. */
  int16_t offsetTenths;     /**< How far off centre, tenths of a kHz. */
} SquelchReading;

/**
 * Work out whether the audio should be open.
 *
 * @param s          The squelch.
 * @param cfg        The thresholds. NULL means the defaults.
 * @param mode       Which mode.
 * @param band       Which band, which decides the rule Auto uses.
 * @param reading    What the tuner says.
 * @param thresholdTenths  For Manual, the level the signal has to beat, in
 *                   tenths of a dBuV.
 * @param nowMs      The millisecond count now.
 * @return true when the audio should be open.
 */
bool squelchUpdate(Squelch *s, const SquelchConfig *cfg, SquelchMode mode,
                   BandId band, const SquelchReading *reading,
                   int16_t thresholdTenths, uint32_t nowMs);

/**
 * Turn a pot reading into a manual squelch threshold.
 *
 * The bottom of the travel is always open and the top is the highest level
 * the chip reports, so the whole knob is usable.
 *
 * @param raw  The pot, 0 to 4095.
 * @return The threshold in tenths of a dBuV.
 */
int16_t squelchThresholdFromPot(uint16_t raw);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SQUELCH_H */
