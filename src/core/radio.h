/*
 * What the radio is set to, and what a command does to it.
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

#define RADIO_VOLUME_MIN (-60) /* Quietest the chip accepts, in dB. */
#define RADIO_VOLUME_MAX 24    /* Loudest the chip accepts, in dB. */

/*
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
 * Confirmed by listening. Of the four widths the chip offers, on 738 kHz at
 * 41 dBuV, 4 kHz is the clearest. 3 kHz is muffled and 8 kHz lets the
 * neighbouring channel in.
 */
#define RADIO_AM_DEFAULT_BANDWIDTH_KHZ 4

/* What the encoder does when it turns. */
typedef enum {
  TUNE_MODE_MANUAL = 0, /* Steps by the step size. */
  TUNE_MODE_AUTO,       /* Seeks to the next station. */
  TUNE_MODE_MEMORY,     /* Steps through stored channels. */
  TUNE_MODE_METER_BAND, /* Steps inside the meter bands. Shortwave only. */
  TUNE_MODE_COUNT       /* How many there are. Not a mode. */
} TuneMode;

const char *tuneModeName(TuneMode mode);

/* The same, in the three or four capitals a panel row has room for. */
const char *tuneModeShort(TuneMode mode);

/* Everything the radio is set to. */
typedef struct {
  BandId band;           /* Which band. */
  uint32_t freqKHz;      /* Where it is tuned. */
  uint16_t stepKHz;      /* How far one step moves it. */
  uint16_t bandwidthKHz; /* 0 lets the tuner choose, FM only. */
  /*
   * The fixed width DX mode puts in force over `bandwidthKHz`, or 0 when DX
   * mode is off. FM only: on the AM side the radio's own width applies
   * whatever this says.
   *
   * Kept apart rather than written over `bandwidthKHz`, because that is the
   * one that is saved. A radio switched off in DX mode comes back on the
   * radio's own width, not on the DX one. `radioTunerBandwidth` is the width
   * the tuner is actually given.
   */
  uint16_t dxBandwidthKHz;
  int8_t volumeDb;   /* Output gain. */
  bool muted;        /* Audio off. */
  TuneMode tuneMode; /* What the encoder does. */
  /*
   * Where each band was left, in kHz.
   *
   * Coming back to a band returns to the station you were listening to, not
   * to the bottom of the band. Leaving medium wave to check something on FM
   * and coming back to find 522 kHz is the behaviour this exists to stop.
   *
   * A band never visited holds 0, which means the bottom of the band.
   */
  uint32_t bandFreqKHz[BAND_COUNT];
  /*
   * What else each band was left set to.
   *
   * The same idea as bandFreqKHz and for the same reason: these belong to a
   * band rather than to the radio. A filter width chosen on medium wave is
   * not a width shortwave wants, and a step size chosen for picking between
   * two crowded FM stations is not the one for walking medium wave.
   *
   * A zero entry means that band has not been set yet and takes its own
   * default. Zero is not a real step or mode, and on the bandwidth side the
   * FM automatic setting is also zero, which comes to the same thing: it is
   * what an FM band defaults to anyway.
   */
  uint16_t bandBandwidthKHz[BAND_COUNT]; /* Filter width, per band. */
  uint16_t bandStepKHz[BAND_COUNT];      /* Step size, per band. */
  /* The tuning mode is not in here. It belongs to the radio rather than to a
   * band: a width and a step describe what is on air in that band, and the
   * mode describes what the person at the knob is doing. `tuneMode` above is
   * the whole of it. */
  /*
   * Multipath suppression, iMS on the PE5PVB TEF6686_ESP32 firmware's
   * screen. FM only.
   *
   * True means suppress. The reference firmware stores this inverted, so
   * that its setting of 0 turns the feature on, and it ships with the
   * setting at 1 and the feature off. This one means what it says.
   */
  bool multipathSuppression;
  bool equalizer; /* Channel equalizer, EQ on that screen. FM only. */
  /*
   * FM de-emphasis, in microseconds. 50, 75, or 0 for none.
   *
   * 50 everywhere except the Americas. It belongs with the band plan rather
   * than with the weak signal settings, but it is written to the tuner with
   * the rest of the FM features, so it lives here.
   */
  uint16_t deemphasisUs;
  bool forcedMono; /* Stereo refused on purpose, not the automatic blend. */
  /* Weak signal handling. Each is a level in dBuV below which that mechanism
   * starts working, and 0 switches it off. The reference firmware ships all
   * three off, which is why a radio that has never been told otherwise does
   * nothing about a weak signal at all. */
  uint8_t highCutStart;     /* Roll the treble off below this. */
  uint8_t stereoBlendStart; /* Blend towards mono below this. */
  uint8_t stHiBlendStart;   /* Do both together below this. */
  /* The noise blankers, which take out impulse noise rather than hiss.
   *
   * A percentage, not a level in dBuV: 0 switches it off, and the usable
   * range is 50 to 150. Everything else beside these two in the reference's
   * menu is in dBuV, so it is easy to write these as dBuV too, and that
   * gives a range which accepts numbers the feature cannot use and refuses
   * numbers it can. The FM one ships off and the AM one ships at 100, since
   * it is the main lever against the crackle on medium wave and shortwave.
   */
  uint8_t amNoiseBlankerStart; /* AM impulse noise blanker. */
  uint8_t fmNoiseBlankerStart; /* FM impulse noise blanker. */
  /* AM weak signal handling. Start levels in dBuV. MW and SW share one pair
   * and LW has its own. */
  uint8_t amHighCutStart;  /* MW and SW. 0 for off, or 20 to 60. */
  uint8_t lwHighCutStart;  /* LW. The same range. */
  uint8_t amSoftMuteStart; /* MW and SW. 0 to 50. */
  uint8_t lwSoftMuteStart; /* LW. The same range. */
} RadioSettings;

/*
 * Fill a band plan in from the stored settings.
 *
 * The one place this conversion happens. Every caller that needs to know
 * where a band starts and ends asks for the plan this way, so there is never
 * a second copy built from the defaults that quietly disagrees.
 */
void radioPlanFromSettings(const Settings *settings, BandPlanConfig *out);

/*
 * Fill the radio's starting state in from the stored settings.
 *
 * Everything the tuner has to be told that a person can change: the band and
 * frequency to come up on, the FM features, the blend start levels and the
 * noise blankers.
 */
void radioFromSettings(const Settings *settings, const BandPlanConfig *plan,
                       RadioSettings *out);

/*
 * Copy the parts of the radio's state that are worth keeping back out.
 *
 * Only what a person changed and would expect to find again. The volume is
 * here for one reason: in manual squelch the knob is the squelch control and
 * nothing else on the radio says how loud to be. It is not read in any other
 * mode, where the knob wins. The squelch mode is not here, because it is not
 * part of the radio's settings: ask radioSquelchMode for it.
 */
void radioToSettings(const RadioSettings *radio, Settings *settings);

/*
 * Whether a tuning mode can be used on a band.
 *
 * Meter band stepping only means something on shortwave. A caller that cycles
 * through the modes has to know which ones to skip, or the button appears to
 * do nothing on every other band.
 */
bool radioTuneModeAllowed(TuneMode mode, BandId band);

/*
 * One channel up or down from the dial, as the tuning mode walks the band.
 *
 * The Meter band mode keeps to the shortwave metre bands, round in a circle,
 * and every other mode walks the whole band and wraps at its edges. A seek
 * walks the dial with this, so a seek in Meter band mode keeps to the metre
 * bands the same way the knob does. 0 for a NULL `s`.
 */
uint32_t radioChannelNext(const RadioSettings *s, const BandPlanConfig *plan,
                          bool up);

/*
 * How many channels that walk passes before it is back where it began, so a
 * seek with nothing to find ends rather than going round for ever. 0 for a
 * NULL `s`, a step of 0 or a band that does not exist.
 */
uint32_t radioChannelsRound(const RadioSettings *s, const BandPlanConfig *plan);

/* The filter width `band` has: the live one when it is the band tuned, else
 * the one it was left on, else the one it comes up with. 0 for a NULL `s` or a
 * band that is not real, and on FM, where 0 is the tuner choosing. */
uint16_t radioBandWidth(const RadioSettings *s, BandId band);

/* The things a caller can ask the radio to do. */
typedef enum {
  RADIO_TUNE = 0,      /* Go to a frequency. */
  RADIO_STEP,          /* Move by whole steps, up or down. */
  RADIO_SET_BAND,      /* Change band. */
  RADIO_SET_STEP,      /* Change the step size. */
  RADIO_SET_BANDWIDTH, /* Change the bandwidth. */
  RADIO_SET_VOLUME,    /* Change the volume. */
  RADIO_SET_MUTE,      /* Mute or unmute. */
  RADIO_SET_TUNE_MODE, /* Change what the encoder does. */
  /* Put DX mode's fixed width in force, `bandwidthKHz` in the command, or
   * 0 to take it off again. FM only. */
  RADIO_SET_DX_BANDWIDTH,
  /*
   * The four below take no argument. They mean "the next one", and the radio
   * works out what that is from what it is set to now.
   *
   * A button cannot do this for itself. It would have to read the state,
   * work out the next value and send that, and in between those two the
   * state can move. Pressing BW right after a keypad tune to medium wave
   * would then send an FM bandwidth of 56 kHz to a band whose widest filter
   * is 8 kHz.
   */
  RADIO_CYCLE_BAND,      /* The next band, wrapping round. */
  RADIO_CYCLE_BANDWIDTH, /* The next bandwidth this band offers. */
  RADIO_CYCLE_TUNE_MODE, /* The next mode this band offers. */
  RADIO_TOGGLE_MUTE,     /* Mute if playing, unmute if muted. */
  /*
   * The next combination of iMS and the channel equalizer.
   *
   * Off, then iMS, then EQ, then both, then off again. One command for two
   * settings, so a caller with a single control can reach all four states
   * without working out the next one itself and racing the radio.
   *
   * Refused on the AM bands, where the tuner has nowhere to put either.
   */
  RADIO_CYCLE_FM_FEATURES,
  /*
   * Sound a short tone through the tuner's own generator.
   *
   * Carried out by the radio task, like a seek, because it takes time and the
   * tuner belongs to that task. Applying it here changes nothing.
   */
  RADIO_BEEP,
  RADIO_SET_MPH_SUPPRESSION, /* Multipath suppression on or off. */
  RADIO_SET_EQUALIZER,       /* Channel equalizer on or off. */
  RADIO_SET_MONO,            /* Force mono, or allow stereo. */
  RADIO_SET_WEAK_SIGNAL,     /* The three weak signal start levels. */
  RADIO_SET_NOISE_BLANKER,   /* The AM and FM impulse noise blankers. */
  RADIO_SET_DEEMPHASIS,      /* The FM de-emphasis time constant. */
  /*
   * Hunt for the next station, up or down.
   *
   * The one command that is not a change of state. It takes time, it walks
   * the dial, and it is carried out by the radio task rather than by
   * radioApply, which is why applying it here does nothing. Any other command
   * arriving stops it.
   */
  RADIO_SEEK,
  /*
   * Tune to a stored channel by its slot.
   *
   * Carried out by the radio task, like a seek, because the channel list is
   * not part of this struct. Applying it here changes nothing.
   *
   * A command rather than the caller reading the channel and sending a tune,
   * so that the knob in memory mode and a request over HTTP go through one
   * piece of code. Two of them would drift, and the band, the width and the
   * slot the radio thinks it is on all have to move together.
   */
  RADIO_RECALL,
  /* The four AM weak signal start levels. Settable from any band, the same
   * as the blankers, because each applies the moment its band is tuned. */
  RADIO_SET_AM_WEAK_SIGNAL,
  /* One band's step, `band` and `stepKHz`, from any band: the step that
   * band comes up with, and the live step too when it is the band tuned. */
  RADIO_SET_BAND_STEP,
  /* One band's filter width, `band` and `bandwidthKHz`, from any band, the
   * same way: the width that band comes up with, and the live one too when it
   * is the band tuned. */
  RADIO_SET_BAND_BANDWIDTH,
  /* Go to `freqKHz` on the band tuned, refused when that band does not hold
   * it, for a drag of the scale. RADIO_TUNE picks the band by the frequency,
   * lowest first, so where two bands overlap, the bottom of SW under the top
   * of MW or OIRT inside the full FM region, it would change band. */
  RADIO_TUNE_IN_BAND
} RadioCommandKind;

/* The bit for value `i` of a command's `members`. */
#define RADIO_MEMBER(i) ((uint8_t)(1u << (i)))

/* One thing to do. Only the field its kind names is read. */
typedef struct {
  RadioCommandKind kind; /* Which of the fields below matters. */
  uint32_t freqKHz;      /* RADIO_TUNE and RADIO_TUNE_IN_BAND. */
  int16_t steps;         /* RADIO_STEP. Negative goes down. */
  BandId band;           /* RADIO_SET_BAND and both RADIO_SET_BAND_*. */
  uint16_t stepKHz;      /* RADIO_SET_STEP, RADIO_SET_BAND_STEP. */
  uint16_t bandwidthKHz; /* RADIO_SET_BANDWIDTH, RADIO_SET_BAND_BANDWIDTH. */
  int8_t volumeDb;       /* RADIO_SET_VOLUME. */
  bool muted;            /* RADIO_SET_MUTE. */
  bool on;               /* The three FM feature commands. */
  uint8_t weak[3];       /* RADIO_SET_WEAK_SIGNAL: cut, blend, both. */
  uint8_t blanker[2];    /* RADIO_SET_NOISE_BLANKER: AM then FM. */
  /* RADIO_SET_AM_WEAK_SIGNAL: high cut MW and SW, high cut LW, soft mute MW
   * and SW, soft mute LW. */
  uint8_t amWeak[4];
  /*
   * For the three commands above, which of their values to change, one bit
   * each, RADIO_MEMBER(0) for the first. The rest keep what the radio holds.
   * Merged by the radio, so a caller changing one value never has to read
   * the others first, and two callers changing different ones cannot undo
   * each other.
   */
  uint8_t members;
  uint16_t deemphasisUs; /* RADIO_SET_DEEMPHASIS: 50, 75 or 0. */
  bool up;               /* RADIO_SEEK: true to hunt upwards. */
  uint16_t beepMs;       /* RADIO_BEEP: how long the tone lasts. */
  uint16_t beepHz;       /* RADIO_BEEP: the tone. */
  uint16_t beepHz2;      /* RADIO_BEEP: the second channel's tone. */
  TuneMode tuneMode;     /* RADIO_SET_TUNE_MODE. */
  int16_t memorySlot;    /* RADIO_RECALL. Counted from 0. */
} RadioCommand;

/* Why a command was refused or did not reach the tuner, so a caller can say
 * something useful. */
typedef enum {
  RADIO_OK = 0,        /* It was applied. */
  RADIO_ERR_BAND,      /* Not a band this radio has. */
  RADIO_ERR_FM_ONLY,   /* The band is real, the feature is FM only. */
  RADIO_ERR_FREQUENCY, /* Not inside any band. */
  RADIO_ERR_STEP,      /* Not a step size that band offers. */
  RADIO_ERR_BANDWIDTH, /* Out of range, or not allowed on this band. */
  RADIO_ERR_VOLUME,    /* Outside what the chip takes. */
  RADIO_ERR_TUNE_MODE, /* Not a mode, or not one this band allows. */
  RADIO_ERR_RANGE,     /* A value outside what that setting accepts. */
  /*
   * Memory tuning mode with no stored channel to move to.
   *
   * Told apart from every other refusal on purpose. A knob that does nothing
   * because the list is empty and a knob that does nothing because the radio
   * is broken look the same from the outside, and this is the one the person
   * can fix.
   */
  RADIO_ERR_NO_CHANNEL,
  /*
   * A stored channel that cannot be tuned as it stands.
   *
   * Its frequency is not on the band it names. A band plan change can do it,
   * and so can an imported file. Told apart from an empty slot because the
   * channel is there and can be corrected.
   */
  RADIO_ERR_CHANNEL_BAND,
  /*
   * The radio could not get at the state the command changes.
   *
   * Only the commands that read a value, work out the next one and write it
   * back can report this, because only they can be spoiled by not holding the
   * lock for both halves. Nothing is changed and the caller is told, which is
   * the honest answer: writing anyway can land on a value nobody asked for,
   * and that is worse than a press that did nothing.
   */
  RADIO_ERR_BUSY,
  /*
   * Accepted, but the write to the tuner failed.
   *
   * The settings hold the new value and the radio writes all of them again
   * every round until the tuner takes them, so the change may still arrive.
   * Told apart from a refusal because nothing was wrong with the command:
   * sending it again does no good until the tuner answers.
   */
  RADIO_ERR_TUNER,
  RADIO_ERR_UNKNOWN /* Not a command. */
} RadioError;

const char *radioErrorText(RadioError error);

void radioDefaults(RadioSettings *settings, const BandPlanConfig *plan);

/*
 * Work out what a command does.
 *
 * The settings are only changed when the command is accepted, so a refused
 * command leaves the radio exactly as it was rather than half moved.
 */
RadioError radioApply(RadioSettings *settings, const BandPlanConfig *plan,
                      const RadioCommand *command);

/*
 * How long the volume takes to come up at switch on, in milliseconds.
 *
 * The radio is otherwise at full listening volume from the first moment it
 * unmutes, which is startling in a quiet room and is the first thing anybody
 * notices about it.
 */
#define RADIO_FADE_MS 1500

/*
 * How long it takes to come back after a band change.
 *
 * Much shorter than the one at switch on. A band change already goes silent
 * while the tuner moves, and this only softens the return.
 *
 * Band changes and jumps only, never an ordinary tune. Turning the knob is a
 * tune as well, and a fade on each click would make the whole dial feel slow.
 *
 * The chip takes whole dB, so a fade has as many steps as the times the
 * volume is moved during it. With a move every RADIO_FADE_STEP_MS, 600 ms
 * gives 30 steps across the RADIO_FADE_DEPTH_DB of 25 dB, each under a dB.
 * 400 ms gives 20, which are heard as steps.
 */
#define RADIO_BAND_FADE_MS 600

/*
 * How far below the target a fade starts, in dB.
 *
 * Not the whole way from silence. The chip takes whole dB, so a band change
 * fade across 60 dB in 600 ms moves 2 dB at a time and is heard as a series
 * of jumps. 25 dB is far enough to hear as a fade and close enough that each
 * step is small.
 */
#define RADIO_FADE_DEPTH_DB 25

/*
 * How often the volume is moved while a fade runs, in milliseconds.
 *
 * The radio task otherwise wakes on its 100 ms poll, which gives the 600 ms
 * band change fade only 6 steps. At 20 ms it has 30.
 */
#define RADIO_FADE_STEP_MS 20

/*
 * The volume `elapsedMs` into a fade that lasts `durationMs`.
 *
 * Rises in a straight line in dB from RADIO_FADE_DEPTH_DB below the target,
 * or from RADIO_VOLUME_MIN if that is higher, to the target. Used for the
 * switch on fade, RADIO_FADE_MS, and the band change fade,
 * RADIO_BAND_FADE_MS. The target is read
 * every time rather than captured at the start, so the knob still works
 * during the fade: turning it down while the radio comes up does what a
 * person would expect, and the fade simply lands somewhere quieter.
 */
int8_t radioFadeVolume(int8_t targetDb, uint32_t elapsedMs,
                       uint16_t durationMs);

/*
 * How long the audio takes to go quiet before it is cut, in milliseconds.
 *
 * Short. This is not a fade in the musical sense, it is taking the edge off a
 * step: long enough that the step is not a click, short enough that pressing
 * mute still feels like pressing mute. The reference firmware cuts instantly,
 * so there is no known good figure to take from it.
 */
#define RADIO_SOFT_MUTE_MS 120

/*
 * The volume on the way down to silence.
 *
 * The mirror of radioFadeVolume: from the target down to the bottom over the
 * duration, rather than up from below it. Kept apart from the fade up because
 * they happen at different moments and are allowed different lengths.
 */
int8_t radioDuckVolume(int8_t fromDb, uint32_t elapsedMs, uint16_t durationMs);

/*
 * The volume on a straight ramp from one level to another, in dB.
 *
 * For the way back from a fade that went further down than the fade up
 * starts from: auto off's goes to the bottom, and the fade up begins
 * RADIO_FADE_DEPTH_DB under the target, so starting that would be a step.
 */
int8_t radioRampVolume(int8_t fromDb, int8_t toDb, uint32_t elapsedMs,
                       uint16_t durationMs);

/*
 * How far into a fade up a given volume sits, in milliseconds.
 *
 * The inverse of radioFadeVolume, for a caller that has to start a fade from
 * a volume the radio is already at rather than from the fade's own floor.
 * A ramp down that is cancelled half way is the case: the volume is part way
 * to silence, and starting a fresh fade there would drop it the rest of the
 * way first and then walk it up, which is a bigger step than the one the
 * ramp exists to remove.
 */
uint32_t radioFadeElapsedAt(int8_t targetDb, int8_t nowDb, uint16_t durationMs);

/* Which parts of the tuner have to be told about a change. */
typedef struct {
  bool retune;    /* The frequency or the band moved. */
  bool bandwidth; /* The filter width has to be set. */
  bool volume;    /* The output gain has to be set. */
  bool mute;      /* The mute has to be set. */
  bool features;  /* The reception features have to be set. */
} RadioPush;

/*
 * Work out what actually has to be sent to the tuner.
 *
 * The point of this is what it leaves out. Moving the dial has to mute the
 * audio first, or the tuner bursts noise while the PLL moves. Turning the
 * volume knob does not, and doing it anyway chops the sound every time the
 * knob moves a step, which is exactly what a volume control must not do.
 */
RadioPush radioPushNeeded(const RadioSettings *from, const RadioSettings *to);

/* The width the tuner is to be given: DX mode's while it is on and the band
 * is FM, otherwise the radio's own. 0 for a NULL `s`. */
uint16_t radioTunerBandwidth(const RadioSettings *s);

/*
 * Whether DX mode's width is the one the tuner has. False for a NULL `s`.
 *
 * The knob does not seek while it is, even in Auto mode, and steps the dial
 * instead: the seek's limits were measured at the radio's own width, and at
 * a DX width they stop on empty channels and miss a station. Measured on this
 * radio.
 */
bool radioDxWidthInForce(const RadioSettings *s);

/*
 * The tuning mode a knob step acts in: the radio's own, except while DX mode
 * is open, where every step is a manual one, a step of the dial whatever the
 * mode is set to, since DX mode is listening a channel at a time. The mode
 * itself is not changed, so it acts again once DX mode closes. MANUAL for a
 * NULL `s`.
 */
TuneMode radioKnobMode(const RadioSettings *s);

/* What the tuner is told about weak AM signals on the band tuned now. */
typedef struct {
  uint8_t highCutStart;  /* dBuV. 0 means off. */
  uint8_t softMuteStart; /* dBuV. */
  uint8_t softMuteSlope; /* dB. */
} AmWeakSignal;

/*
 * Pick the AM weak signal values for the band `s` is on.
 *
 * LW gets its own pair and the NXP manual's LW soft mute slope of 30 dB. MW
 * and SW get the other pair and the chip's own slope of 25 dB. Only the start
 * levels are settings; the slopes are the manual's numbers.
 */
AmWeakSignal radioAmWeakSignal(const RadioSettings *s);

/*
 * Whether two settings differ in a way the tuner has to be told about.
 *
 * Meant to let the radio task skip talking to the chip when nothing it cares
 * about moved. No firmware code calls it yet. The unit tests cover it.
 */
bool radioNeedsRetune(const RadioSettings *a, const RadioSettings *b);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RADIO_H */
