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
#include "settings.h"

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
 *
 * Confirmed by listening on 12 September 2026. All four widths the chip offers
 * were tried in turn on 738 kHz at 41 dBuV, and 4 kHz was the clearest. 3 kHz
 * is muffled and 8 kHz lets the neighbouring channel in.
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
  /**
   * Where each band was left, in kHz.
   *
   * Coming back to a band returns to the station you were listening to, not
   * to the bottom of the band. Leaving medium wave to check something on FM
   * and coming back to find 522 kHz is the behaviour this exists to stop.
   *
   * A band never visited holds 0, which means the bottom of the band.
   */
  uint32_t bandFreqKHz[BAND_COUNT];
  /**
   * Multipath suppression, iMS on the old radio's screen. FM only.
   *
   * True means suppress. The reference firmware stores this inverted, so
   * that its setting of 0 turns the feature on, and it ships with the
   * setting at 1 and the feature off. This one means what it says.
   */
  bool multipathSuppression;
  bool equalizer; /**< Channel equalizer, EQ on that screen. FM only. */
  /**
   * FM de-emphasis, in microseconds. 50, 75, or 0 for none.
   *
   * 50 everywhere except the Americas. It belongs with the band plan rather
   * than with the weak signal settings, but it is written to the tuner with
   * the rest of the FM features, so it lives here.
   */
  uint16_t deemphasisUs;
  bool forcedMono; /**< Stereo refused on purpose, not the automatic blend. */
  /* Weak signal handling. Each is a level in dBuV below which that mechanism
   * starts working, and 0 switches it off. The reference firmware ships all
   * three off, which is why a radio that has never been told otherwise does
   * nothing about a weak signal at all. */
  uint8_t highCutStart;     /**< Roll the treble off below this. */
  uint8_t stereoBlendStart; /**< Blend towards mono below this. */
  uint8_t stHiBlendStart;   /**< Do both together below this. */
  /* The noise blankers, which take out impulse noise rather than hiss.
   *
   * A percentage, not a level in dBuV: 0 switches it off, and the usable
   * range is 50 to 150. Everything else in the reference's menu beside these
   * two is in dBuV, which is how they came to be written as dBuV here, given
   * a range that accepted numbers the feature cannot use and refused numbers
   * it can. Both ship off, and the AM one is the main lever against the
   * crackle on medium wave and shortwave. */
  uint8_t amNoiseBlankerStart; /**< AM impulse noise blanker. */
  uint8_t fmNoiseBlankerStart; /**< FM impulse noise blanker. */
} RadioSettings;

/**
 * Fill a band plan in from the stored settings.
 *
 * The one place this conversion happens. Every caller that needs to know
 * where a band starts and ends asks for the plan this way, so there is never
 * a second copy built from the defaults that quietly disagrees. That has
 * already happened once, in the tune endpoint, and it only stayed harmless
 * because the two copies were identical.
 *
 * @param settings  The stored settings.
 * @param out       Receives the plan.
 */
void radioPlanFromSettings(const Settings *settings, BandPlanConfig *out);

/**
 * Fill the radio's starting state in from the stored settings.
 *
 * Everything the tuner has to be told that a person can change: the band and
 * frequency to come up on, the FM features, the blend start levels and the
 * noise blankers.
 *
 * @param settings  The stored settings.
 * @param plan      The band plan, from radioPlanFromSettings.
 * @param out       Receives the settings the radio starts from.
 */
void radioFromSettings(const Settings *settings, const BandPlanConfig *plan,
                       RadioSettings *out);

/**
 * Copy the parts of the radio's state that are worth keeping back out.
 *
 * Only what a person changed and would expect to find again. The volume is
 * here for one reason: in manual squelch the knob is the squelch control and
 * nothing else on the radio says how loud to be. It is not read in any other
 * mode, where the knob wins. The squelch mode is not here, because it is not
 * part of the radio's settings: ask radioSquelchMode for it.
 *
 * @param radio     What the radio is set to now.
 * @param settings  Receives the parts worth storing.
 */
void radioToSettings(const RadioSettings *radio, Settings *settings);

/**
 * Whether a tuning mode can be used on a band.
 *
 * Meter band stepping only means something on shortwave. A caller that cycles
 * through the modes has to know which ones to skip, or the button appears to
 * do nothing on every other band.
 *
 * @param mode  Which mode.
 * @param band  Which band.
 * @return true when that mode is available there.
 */
bool radioTuneModeAllowed(TuneMode mode, BandId band);

/** The things a caller can ask the radio to do. */
typedef enum {
  RADIO_TUNE = 0,      /**< Go to a frequency. */
  RADIO_STEP,          /**< Move by whole steps, up or down. */
  RADIO_SET_BAND,      /**< Change band. */
  RADIO_SET_STEP,      /**< Change the step size. */
  RADIO_SET_BANDWIDTH, /**< Change the bandwidth. */
  RADIO_SET_VOLUME,    /**< Change the volume. */
  RADIO_SET_MUTE,      /**< Mute or unmute. */
  RADIO_SET_TUNE_MODE, /**< Change what the encoder does. */
  /*
   * The four below take no argument. They mean "the next one", and the radio
   * works out what that is from what it is set to now.
   *
   * A button cannot do this for itself. It would have to read the state,
   * work out the next value and send that, and in between those two the
   * state can move. That is not theory: pressing BW right after tuning with
   * the keypad sent an FM bandwidth of 56 kHz to a radio that had just
   * arrived on medium wave, where the widest filter is 8 kHz.
   */
  RADIO_CYCLE_BAND,          /**< The next band, wrapping round. */
  RADIO_CYCLE_BANDWIDTH,     /**< The next bandwidth this band offers. */
  RADIO_CYCLE_TUNE_MODE,     /**< The next mode this band offers. */
  RADIO_TOGGLE_MUTE,         /**< Mute if playing, unmute if muted. */
  RADIO_SET_MPH_SUPPRESSION, /**< Multipath suppression on or off. */
  RADIO_SET_EQUALIZER,       /**< Channel equalizer on or off. */
  RADIO_SET_MONO,            /**< Force mono, or allow stereo. */
  RADIO_SET_WEAK_SIGNAL,     /**< The three weak signal start levels. */
  RADIO_SET_NOISE_BLANKER,   /**< The AM and FM impulse noise blankers. */
  RADIO_SET_DEEMPHASIS       /**< The FM de-emphasis time constant. */
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
  bool on;               /**< The three FM feature commands. */
  uint8_t weak[3];       /**< RADIO_SET_WEAK_SIGNAL: cut, blend, both. */
  uint8_t blanker[2];    /**< RADIO_SET_NOISE_BLANKER: AM then FM. */
  uint16_t deemphasisUs; /**< RADIO_SET_DEEMPHASIS: 50, 75 or 0. */
  TuneMode tuneMode;     /**< RADIO_SET_TUNE_MODE. */
} RadioCommand;

/** Why a command was refused, so a caller can say something useful. */
typedef enum {
  RADIO_OK = 0,        /**< It was applied. */
  RADIO_ERR_BAND,      /**< Not a band this radio has. */
  RADIO_ERR_FM_ONLY,   /**< The band is real, the feature is FM only. */
  RADIO_ERR_FREQUENCY, /**< Not inside any band. */
  RADIO_ERR_STEP,      /**< Not a step size that band offers. */
  RADIO_ERR_BANDWIDTH, /**< Out of range, or not allowed on this band. */
  RADIO_ERR_VOLUME,    /**< Outside what the chip takes. */
  RADIO_ERR_TUNE_MODE, /**< Not a mode, or not one this band allows. */
  RADIO_ERR_RANGE,     /**< A value outside what that setting accepts. */
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
 * How long the volume takes to come up at switch on, in milliseconds.
 *
 * The radio is otherwise at full listening volume from the first moment it
 * unmutes, which is startling in a quiet room and is the first thing anybody
 * notices about it.
 */
#define RADIO_FADE_MS 1500

/**
 * How long it takes to come back after a band change.
 *
 * Much shorter than the one at switch on. A band change already goes silent
 * while the tuner moves, and this only softens the return.
 *
 * Band changes and jumps only, never an ordinary tune. Turning the knob is a
 * tune as well, and a fade on each click would make the whole dial feel slow.
 *
 * Six hundred milliseconds rather than four. The chip takes whole dB, so the
 * number of steps a fade can have is the number of times the volume is moved
 * during it, and at four hundred that was twenty steps across twenty five dB.
 * Audible as steps. Six hundred gives thirty, each under a dB.
 */
#define RADIO_BAND_FADE_MS 600

/**
 * How far below the target a fade starts, in dB.
 *
 * Not the whole way from silence. The chip takes whole dB, so a fade across
 * sixty of them in four tenths of a second can only ever be a series of
 * jumps, and that is what it sounded like. Twenty five dB is far enough to
 * hear as a fade and close enough that each step is small.
 */
#define RADIO_FADE_DEPTH_DB 25

/**
 * How often the volume is moved while a fade runs, in milliseconds.
 *
 * The radio task otherwise wakes on its hundred millisecond poll, which gives
 * a band change fade four steps in total. Twenty gives twenty.
 */
#define RADIO_FADE_STEP_MS 20

/**
 * The volume to use while the radio is starting.
 *
 * Rises from silence to the target over RADIO_FADE_MS. The target is read
 * every time rather than captured at the start, so the knob still works
 * during the fade: turning it down while the radio comes up does what a
 * person would expect, and the fade simply lands somewhere quieter.
 *
 * @param targetDb    Where the volume is going, which is where the knob says.
 * @param elapsedMs   How long since the fade started.
 * @param durationMs  How long the fade lasts. 0 means no fade at all.
 * @return The volume to set now. Equals targetDb once the fade is over.
 */
int8_t radioFadeVolume(int8_t targetDb, uint32_t elapsedMs,
                       uint16_t durationMs);

/** Which parts of the tuner have to be told about a change. */
typedef struct {
  bool retune;    /**< The frequency or the band moved. */
  bool bandwidth; /**< The filter width has to be set. */
  bool volume;    /**< The output gain has to be set. */
  bool mute;      /**< The mute has to be set. */
  bool features;  /**< The FM features have to be set. */
} RadioPush;

/**
 * Work out what actually has to be sent to the tuner.
 *
 * The point of this is what it leaves out. Moving the dial has to mute the
 * audio first, or the tuner bursts noise while the PLL moves. Turning the
 * volume knob does not, and doing it anyway chops the sound every time the
 * knob moves a step, which is exactly what a volume control must not do.
 *
 * @param from   What the tuner was last told. NULL means it was told nothing,
 *               so everything is sent.
 * @param to     What it should be set to.
 * @return Which parts to send. All false means there is nothing to do.
 */
RadioPush radioPushNeeded(const RadioSettings *from, const RadioSettings *to);

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
