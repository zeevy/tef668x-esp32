/**
 * @file radio.h
 * @brief What the radio is set to, and what a command does to it.
 *
 * This is the state machine, and it is deliberately free of hardware. Given
 * what the radio is set to now and a command, it works out what it should be
 * set to next. Whether that reaches a chip, and how, is somebody else's job.
 *
 * Keeping it here means the awkward parts can be tested on a PC: stepping off
 * the end of a band, changing band and landing somewhere sensible, a volume
 * that would go past what the chip takes, a bandwidth the band does not offer.
 * Those are the things that are painful to check by hand on a radio and easy
 * to get wrong.
 *
 * Frequencies are in kilohertz, matching core/band_plan.h.
 */
#ifndef CORE_RADIO_H
#define CORE_RADIO_H

#include <stdbool.h>
#include <stdint.h>

#include "band_plan.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Loudest and quietest the chip accepts, in dB. */
#define RADIO_VOLUME_MIN (-60) /**< Quietest the chip accepts, in dB. */
#define RADIO_VOLUME_MAX 24    /**< Loudest the chip accepts, in dB. */

/**
 * The bandwidth an AM band starts on, in kHz.
 *
 * Not a guess and not a taste. AM broadcast channels are 9 or 10 kHz apart,
 * and a receiver has to pass one of them without letting the neighbour in, so
 * the usable width is about half the spacing. The vendor's own selectivity
 * figures are quoted at 3 kHz for the same reason.
 *
 * There is no automatic setting on the AM side to fall back on, which is why
 * a band change has to choose something rather than leave the FM setting in
 * place.
 */
#define RADIO_AM_DEFAULT_BANDWIDTH_KHZ 4

/** What the encoder does when it turns. */
typedef enum {
  TUNE_MODE_MANUAL = 0, /**< Steps by the step size. */
  TUNE_MODE_AUTO,       /**< Seeks to the next station. */
  TUNE_MODE_MEMORY,     /**< Steps through stored channels. */
  TUNE_MODE_METER_BAND, /**< Steps between meter bands. Shortwave only. */
  TUNE_MODE_COUNT       /**< How many there are. Not a mode. */
} TuneMode;

/**
 * The name of a tuning mode, for the screen.
 *
 * @param mode  Which mode.
 * @return Its name, or an empty string when it is not a mode. Never NULL.
 */
const char *tuneModeName(TuneMode mode);

/** Everything the radio is set to. */
typedef struct {
  BandId band;           /**< Which band. */
  uint32_t freqKHz;      /**< Where it is tuned. */
  uint16_t stepKHz;      /**< How far one step moves it. */
  uint16_t bandwidthKHz; /**< 0 lets the tuner choose, FM only. */
  int8_t volumeDb;       /**< Output gain. */
  bool muted;            /**< Audio off. */
  TuneMode tuneMode;     /**< What the encoder does. */
} RadioSettings;

/** The things a caller can ask the radio to do. */
typedef enum {
  RADIO_TUNE = 0,      /**< Go to a frequency. */
  RADIO_STEP,          /**< Move by whole steps, up or down. */
  RADIO_SET_BAND,      /**< Change band. */
  RADIO_SET_STEP,      /**< Change the step size. */
  RADIO_SET_BANDWIDTH, /**< Change the bandwidth. */
  RADIO_SET_VOLUME,    /**< Change the volume. */
  RADIO_SET_MUTE,      /**< Mute or unmute. */
  RADIO_SET_TUNE_MODE  /**< Change what the encoder does. */
} RadioCommandKind;

/** One thing to do. Only the field its kind names is read. */
typedef struct {
  RadioCommandKind kind; /**< Which of the fields below matters. */
  uint32_t freqKHz;      /**< RADIO_TUNE. */
  int16_t steps;         /**< RADIO_STEP. Negative goes down. */
  BandId band;           /**< RADIO_SET_BAND. */
  uint16_t stepKHz;      /**< RADIO_SET_STEP. */
  uint16_t bandwidthKHz; /**< RADIO_SET_BANDWIDTH. */
  int8_t volumeDb;       /**< RADIO_SET_VOLUME. */
  bool muted;            /**< RADIO_SET_MUTE. */
  TuneMode tuneMode;     /**< RADIO_SET_TUNE_MODE. */
} RadioCommand;

/** Why a command was refused, so a caller can say something useful. */
typedef enum {
  RADIO_OK = 0,        /**< It was applied. */
  RADIO_ERR_BAND,      /**< Not a band this radio has. */
  RADIO_ERR_FREQUENCY, /**< Not inside any band. */
  RADIO_ERR_STEP,      /**< Not a step size that band offers. */
  RADIO_ERR_BANDWIDTH, /**< Out of range, or not allowed on this band. */
  RADIO_ERR_VOLUME,    /**< Outside what the chip takes. */
  RADIO_ERR_TUNE_MODE, /**< Not a mode, or not one this band allows. */
  RADIO_ERR_UNKNOWN    /**< Not a command. */
} RadioError;

/**
 * The reason in words.
 *
 * @param error  What went wrong.
 * @return A short phrase. Never NULL.
 */
const char *radioErrorText(RadioError error);

/**
 * Fill in what a radio is set to when it has never been set.
 *
 * @param settings  Receives the defaults.
 * @param plan      The regional band choices. NULL means the defaults.
 */
void radioDefaults(RadioSettings *settings, const BandPlanConfig *plan);

/**
 * Work out what a command does.
 *
 * The settings are only changed when the command is accepted, so a refused
 * command leaves the radio exactly as it was rather than half moved.
 *
 * @param settings  What it is set to. Updated in place on success.
 * @param plan      The regional band choices. NULL means the defaults.
 * @param command   What to do.
 * @return RADIO_OK, or why not.
 */
RadioError radioApply(RadioSettings *settings, const BandPlanConfig *plan,
                      const RadioCommand *command);

/**
 * Whether two settings differ in a way the tuner has to be told about.
 *
 * Lets the task skip talking to the chip when nothing it cares about moved.
 *
 * @param a  One set of settings.
 * @param b  The other.
 * @return true when the tuner needs a new instruction.
 */
bool radioNeedsRetune(const RadioSettings *a, const RadioSettings *b);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RADIO_H */
