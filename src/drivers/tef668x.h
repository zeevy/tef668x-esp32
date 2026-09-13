/*
 * The tuner. Everything that talks to the TEF668x over I2C.
 *
 * Two things about this chip drive the shape of this interface.
 *
 * It has no usable firmware in ROM, so a patch goes over I2C at every power
 * on before it will tune anything. See tef668x_patch.h.
 *
 * And the family is not one chip. A TEF6686 has no stereo improvement and no
 * full search RDS, a TEF6689 has both. The driver reads what is actually
 * fitted and publishes a capability set, so a board with a different part gets
 * the extra features without a code change and the UI simply does not offer
 * what is missing. Nothing above this layer tests for a part number.
 *
 * Frequencies are in kilohertz on every band, matching core/band_plan.h. The
 * chip wants tens of kilohertz on FM and kilohertz on AM, and that conversion
 * happens in here rather than leaking upwards.
 *
 * Every call says whether it worked. The firmware this replaces returns void
 * from its tuner writes and never notices an I2C failure, which is why a dead
 * bus there looks like a radio that is simply silent.
 */
#ifndef DRIVERS_TEF668X_H
#define DRIVERS_TEF668X_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* What went wrong, so the boot screen can name it instead of hanging. */
typedef enum {
  TEF668X_OK = 0,        /* It worked. */
  TEF668X_ERR_NO_DEVICE, /* Nothing answered at the I2C address. */
  TEF668X_ERR_NOT_READY, /* The chip never reported itself ready. */
  TEF668X_ERR_IDENTIFY,  /* The identification could not be read. */
  TEF668X_ERR_NO_PATCH,  /* The chip wants a patch this build does not carry. */
  TEF668X_ERR_UNKNOWN_PART, /* The device word is not a part we know. */
  TEF668X_ERR_WRITE,        /* An I2C write was not acknowledged. */
  TEF668X_ERR_READ,         /* An I2C read came back short. */
  TEF668X_ERR_RANGE         /* A caller asked for something out of range. */
} Tef668xError;

const char *tef668xErrorText(Tef668xError error);

/* Which part is fitted, and therefore what the radio can do. */
typedef struct {
  uint16_t deviceWord;   /* Raw identification word. */
  uint16_t hardwareWord; /* Raw hardware version word. */
  uint16_t softwareWord; /* Raw software version word. */
  uint16_t patchVersion; /* 102 or 205, worked out from the version words. */
  const char *part;      /* "TEF6686" and so on. Never NULL. */
  bool hasStereoImprovement; /* FMSI. TEF6687 and TEF6689 only. */
  bool hasFullSearchRds;     /* TEF6687 and TEF6689 only. */
  bool hasDigitalRadio;      /* DR. TEF6688 and TEF6689 only. */
} Tef668xCapabilities;

/* How good the signal is right now. */
typedef struct {
  int16_t levelDbuVTenths;   /* Signal level in tenths of a dBuV. */
  uint16_t usnTenths;        /* Ultrasonic noise, tenths of a percent. FM. */
  uint16_t multipathTenths;  /* Multipath, tenths of a percent. FM. */
  uint16_t coChannelTenths;  /* Co-channel interference, tenths. AM. */
  int16_t offsetKHzTenths;   /* How far off centre the station is. */
  uint16_t bandwidthKHz;     /* The bandwidth the tuner settled on. */
  int16_t modulationPercent; /* Modulation depth. See the note below. */
  bool stereo;               /* A stereo pilot is present. FM. */
  /*
   * Signal to noise in dB, worked out rather than read.
   *
   * The chip does not report it. See core/signal.h for the line it comes
   * from and why that line is copied rather than derived.
   */
  int8_t snrDb;
} Tef668xQuality;

/*
 * Bring the tuner up: reset it, patch it, set its clock, power it on.
 *
 * This happens every time, even when the chip reports itself already patched.
 * After an update over the air the ESP32 reboots and the tuner does not, so it
 * comes up holding the settings the previous firmware gave it. Trusting its
 * "already patched" answer would mean a firmware update could not change
 * anything about the tuner until someone power cycled the radio.
 *
 * Safe to call again. Any earlier result is forgotten first, so a call that
 * fails leaves no stale capability set behind.
 */
Tef668xError tef668xBegin(void);

const Tef668xCapabilities *tef668xCapabilities(void);

/*
 * Put the tuner into its working state, or into standby.
 *
 * Standby is what the power saving in a later phase will use. A chip in
 * standby still answers on the bus and still returns a quality status, but
 * every field in it is meaningless, so nothing should read from it there.
 */
Tef668xError tef668xSetActive(bool active);

Tef668xError tef668xTuneFm(uint32_t freqKHz);

Tef668xError tef668xTuneAm(uint32_t freqKHz);

/*
 * What the tuner is set to now, from the last tune that worked.
 *
 * The quality registers live in different modules for AM and FM and return
 * different fields, so a reader has to know which side it is on. Asking the
 * driver is better than every caller keeping its own copy and one of them
 * getting it wrong.
 */
bool tef668xCurrentTune(uint32_t *freqKHz, bool *isFm);

Tef668xError tef668xSetFmBandwidth(uint16_t bandwidthKHz);

Tef668xError tef668xSetAmBandwidth(uint16_t bandwidthKHz);

Tef668xError tef668xSetVolume(int8_t decibels);

Tef668xError tef668xSetMute(bool muted);

/*
 * The AM noise blanker, which removes impulse noise rather than hiss.
 *
 * Off as this radio ships, because that is how the reference ships. It is the
 * main thing the chip offers against the crackle that makes medium wave and
 * shortwave tiring to listen to, so it is worth trying before concluding that
 * a band is simply noisy.
 *
 * The start is a **percentage, not a level in dBuV**, and the reference's own
 * menu offers 0 or 50 to 150. Everything else in that menu is in dBuV, which
 * is how this was written as dBuV at first and given a range that accepted
 * numbers the feature cannot use and refused numbers it can.
 */
Tef668xError tef668xSetAmNoiseBlanker(uint8_t startPercent);

Tef668xError tef668xSetFmNoiseBlanker(uint8_t startPercent);

/*
 * Multipath suppression, which the old firmware and its screen call iMS.
 *
 * FM only. It is the feature that makes a station suffering reflections
 * listenable, and it is off until something turns it on.
 */
Tef668xError tef668xSetMultipathSuppression(bool on);

/*
 * The channel equalizer, which that firmware calls EQ.
 *
 * FM only, and also off by default.
 */
Tef668xError tef668xSetChannelEqualizer(bool on);

/*
 * How far the adaptive bandwidth may open.
 *
 * FM only, and only meaningful while the bandwidth is on automatic. On a
 * strong clean signal the filter is allowed to open further, which is more
 * treble and better stereo separation. On anything else it is held back,
 * because a wide filter on a weak signal is mostly the neighbours.
 */
Tef668xError tef668xSetBandwidthExtension(bool wide);

/*
 * Force mono, or allow stereo.
 *
 * FM only. Not the same as the automatic blend, which drops to mono on its
 * own as a signal weakens. This is the switch a person throws.
 */
Tef668xError tef668xSetMono(bool mono);

/*
 * Read the signal quality.
 *
 * `modulationPercent` is signed on purpose. The chip returns an
 * unsigned word, and a genuinely negative reading comes back as a
 * number in the thousands. Reading it as signed turns that back into
 * the small negative it was, which is what stops a bad sample being
 * taken for a very loud station. See test/fixtures/agc/README.md, where
 * that wrap is one of the cases the captures were kept for.
 */
Tef668xError tef668xReadQuality(bool fm, Tef668xQuality *quality);

/*
 * Weak signal handling: how far to back off as a signal gets worse.
 *
 * Three separate mechanisms, each with a level at which it starts. A start of
 * 0 switches that one off, which is how the reference firmware ships all
 * three. Levels are in dBuV.
 */
Tef668xError tef668xSetWeakSignal(uint8_t highCutStart, uint8_t stereoStart,
                                  uint8_t stHiBlendStart);

/*
 * FM de-emphasis time constant.
 *
 * The transmitter lifts the treble and the receiver has to put it back. 50 us
 * everywhere except the Americas, which use 75 us. Getting it wrong is not
 * subtle: everything sounds dull, or everything sounds shrill.
 */
Tef668xError tef668xSetDeemphasis(uint16_t microseconds);

/*
 * Start or stop the tuner's own tone generator.
 *
 * The one piece of audio hardware on this radio that is not the tuner
 * receiving something. It feeds a tone into the audio path, so it is heard
 * whatever the dial is doing, and it is how the radio beeps.
 *
 * The tone goes through the output mute, so a muted radio stays silent. The
 * caller decides whether to lift the mute around it and put it back, which is
 * what the reference firmware does for its band edge beep.
 *
 * The generator takes two amplitude and frequency pairs. Giving both the
 * same produces one tone, which is what the reference firmware does. Giving
 * them different frequencies is what makes a DTMF pair possible, if the two
 * are separate generators rather than the two output channels.
 */
Tef668xError tef668xTone(bool on, int16_t amplitude, uint16_t freqHz,
                         uint16_t freqHz2);

/* What the chip is doing to the audio right now, rather than what it was told. */
typedef struct {
  /*
   * Three numbers with no unit that anybody has established.
   *
   * The reference firmware divides each raw word by ten and calls them
   * highcut, stereo and sthiblend, and then never uses them, so there is
   * nothing to say what the result counts. They were written here as kHz,
   * which was a guess, and a reading of 47 on a station being rolled off is
   * not 47 kHz of audio.
   *
   * What they are good for is watching them move: zero means the mechanism
   * is doing nothing, and rising means it is. That is enough to tell a
   * feature that is working from one that was never switched on, which is
   * why this read exists. Do not put a unit on them until one is known.
   */
  uint16_t highCut;   /* The treble roll off, as the chip reports it. */
  uint16_t stereo;    /* The stereo blend. */
  uint16_t stHiBlend; /* The combined blend. */
} Tef668xProcessing;

/*
 * Read what the chip is currently applying.
 *
 * Not what it was told: what it has decided to do about the signal in front
 * of it. The blend and roll off features move continuously with the signal,
 * so without this there is no way to tell a feature that is working from one
 * that was never switched on. FM only.
 *
 * The reference firmware implements this read and never calls it.
 */
Tef668xError tef668xReadProcessing(Tef668xProcessing *out);

/*
 * Read the raw quality bytes, exactly as they came off the wire.
 *
 * For working out what a reading means when the decoded numbers look wrong.
 */
Tef668xError tef668xReadQualityRaw(bool fm, uint8_t out[14]);

/*
 * One read of the chip's RDS decoder.
 *
 * `haveGroup` is the field that matters. The chip is asked every 43 ms and
 * most of those reads have nothing new in them, and the words it returns then
 * are not a group. Anything that reads `block` without checking `haveGroup`
 * is decoding whatever happened to be in the register.
 *
 * `synchronised` is a different question and is answered on every read. It
 * says the decoder is locked to an RDS bit stream, which is how a station
 * with no RDS is told apart from one whose groups have not arrived yet.
 *
 * `error` is the chip's own confidence in each block, 0 for a clean block, 1
 * or 2 for one it corrected, and 3 for one it could not. A block at 3 carries
 * wrong bits, so it is not the data it claims to be.
 */
typedef struct {
  /*
   * The status word exactly as the chip sent it.
   *
   * Only two of its bits are documented anywhere public, and the flags below
   * are worked out from those. Keeping the whole word means a station that
   * decodes on the reference firmware and not on this one can be looked at
   * without a serial cable.
   */
  uint16_t status;
  bool read;         /* The chip answered. Without this everything is zero. */
  bool synchronised; /* The decoder is locked to an RDS stream. */
  bool haveGroup;    /* A new group arrived. Everything below is only then. */
  uint16_t block[4]; /* A, B, C, D. */
  uint8_t error[4]; /* Per block: 0 clean, 1 or 2 corrected, 3 not corrected. */
} Tef668xRdsRead;

/*
 * Switch the RDS decoder on and start it again.
 *
 * Sent on every FM tune, not once at start up. The chip holds the previous
 * station's group in its register, so without this the first read after a
 * retune hands over the station that was left behind, and it is
 * indistinguishable from the new one having answered immediately.
 *
 * Full search is a TEF6687 and TEF6689 feature. On a part without it the flag
 * is ignored here rather than sent, because what the chip does with a mode it
 * does not have is not known.
 */
Tef668xError tef668xSetRds(bool fullSearch);

/*
 * Read one group from the RDS decoder.
 *
 * FM only. `out` is cleared first, so a caller that ignores `haveGroup` gets
 * zeros rather than the group before it.
 */
Tef668xError tef668xReadRds(Tef668xRdsRead *out);

/*
 * What happened during start up, whether it worked or not.
 *
 * The boot self test in a later phase shows this, and it is the only way to
 * tell a tuner that was skipped from one that was patched and still would not
 * answer, without a serial cable.
 */
typedef struct {
  bool sawDevice;       /* Something acknowledged at the I2C address. */
  bool readBootStatus;  /* The operation status came back. */
  uint8_t bootStatus;   /* What it said. 0 means not patched yet. */
  bool patchLoaded;     /* A patch was written this boot. */
  uint16_t patchTried;  /* Which version was written, 0 if none. */
  uint16_t patchWanted; /* Which version the chip then asked for. */
  uint16_t xtalAdc;     /* What the crystal sense pin read. */
  const char *xtal;     /* Which crystal that was taken to mean. */
} Tef668xDiagnostics;

const Tef668xDiagnostics *tef668xDiagnostics(void);

/*
 * The identification words, even when start up later failed.
 *
 * Useful when the chip answered but this build could not make sense of what
 * it said, which is otherwise invisible without a serial cable.
 */
bool tef668xLastIdentification(uint16_t *device, uint16_t *hardware,
                               uint16_t *software);

/*
 * How the tuner start up went.
 *
 * Defined by the application rather than the driver, because the driver does
 * not know when start up happened. It is declared here so the web page can
 * report the reason without the caller reaching into main.
 */
Tef668xError tunerStartError(void);

#endif /* DRIVERS_TEF668X_H */
