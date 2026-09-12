/**
 * @file tef668x.h
 * @brief The tuner. Everything that talks to the TEF668x over I2C.
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

/** What went wrong, so the boot screen can name it instead of hanging. */
typedef enum {
  TEF668X_OK = 0,        /**< It worked. */
  TEF668X_ERR_NO_DEVICE, /**< Nothing answered at the I2C address. */
  TEF668X_ERR_NOT_READY, /**< The chip never reported itself ready. */
  TEF668X_ERR_IDENTIFY,  /**< The identification could not be read. */
  TEF668X_ERR_NO_PATCH, /**< The chip wants a patch this build does not carry. */
  TEF668X_ERR_UNKNOWN_PART, /**< The device word is not a part we know. */
  TEF668X_ERR_WRITE,        /**< An I2C write was not acknowledged. */
  TEF668X_ERR_READ,         /**< An I2C read came back short. */
  TEF668X_ERR_RANGE         /**< A caller asked for something out of range. */
} Tef668xError;

/**
 * The error in words, for the serial log and the boot screen.
 *
 * @param error  The error to describe.
 * @return A short phrase. Never NULL.
 */
const char *tef668xErrorText(Tef668xError error);

/** Which part is fitted, and therefore what the radio can do. */
typedef struct {
  uint16_t deviceWord;   /**< Raw identification word. */
  uint16_t hardwareWord; /**< Raw hardware version word. */
  uint16_t softwareWord; /**< Raw software version word. */
  uint16_t patchVersion; /**< 102 or 205, worked out from the version words. */
  const char *part;      /**< "TEF6686" and so on. Never NULL. */
  bool hasStereoImprovement; /**< FMSI. TEF6687 and TEF6689 only. */
  bool hasFullSearchRds;     /**< TEF6687 and TEF6689 only. */
  bool hasDigitalRadio;      /**< DR. TEF6688 and TEF6689 only. */
} Tef668xCapabilities;

/** How good the signal is right now. */
typedef struct {
  int16_t levelDbuVTenths;   /**< Signal level in tenths of a dBuV. */
  uint16_t usnTenths;        /**< Ultrasonic noise, tenths of a percent. FM. */
  uint16_t multipathTenths;  /**< Multipath, tenths of a percent. FM. */
  uint16_t coChannelTenths;  /**< Co-channel interference, tenths. AM. */
  int16_t offsetKHzTenths;   /**< How far off centre the station is. */
  uint16_t bandwidthKHz;     /**< The bandwidth the tuner settled on. */
  int16_t modulationPercent; /**< Modulation depth. See the note below. */
  bool stereo;               /**< A stereo pilot is present. FM. */
} Tef668xQuality;

/**
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
 *
 * @return TEF668X_OK, or what stopped it.
 */
Tef668xError tef668xBegin(void);

/**
 * What the fitted part can do.
 *
 * @return The capability set, or NULL when tef668xBegin has not succeeded.
 */
const Tef668xCapabilities *tef668xCapabilities(void);

/**
 * Put the tuner into its working state, or into standby.
 *
 * Standby is what the power saving in a later phase will use. A chip in
 * standby still answers on the bus and still returns a quality status, but
 * every field in it is meaningless, so nothing should read from it there.
 *
 * @param active  true to receive, false for standby.
 * @return TEF668X_OK, or what stopped it.
 */
Tef668xError tef668xSetActive(bool active);

/**
 * Tune an FM frequency.
 *
 * @param freqKHz  The frequency in kHz, so 104000 is 104.00 MHz.
 * @return TEF668X_OK, or TEF668X_ERR_RANGE when the chip cannot reach it.
 */
Tef668xError tef668xTuneFm(uint32_t freqKHz);

/**
 * Tune an AM frequency, on long, medium or short wave.
 *
 * @param freqKHz  The frequency in kHz.
 * @return TEF668X_OK, or TEF668X_ERR_RANGE when the chip cannot reach it.
 */
Tef668xError tef668xTuneAm(uint32_t freqKHz);

/**
 * What the tuner is set to now, from the last tune that worked.
 *
 * The quality registers live in different modules for AM and FM and return
 * different fields, so a reader has to know which side it is on. Asking the
 * driver is better than every caller keeping its own copy and one of them
 * getting it wrong.
 *
 * @param freqKHz  Receives the frequency in kHz. May be NULL.
 * @param isFm     Receives true for FM or OIRT, false for AM. May be NULL.
 * @return false when nothing has been tuned yet.
 */
bool tef668xCurrentTune(uint32_t *freqKHz, bool *isFm);

/**
 * Set the FM bandwidth.
 *
 * @param bandwidthKHz  The bandwidth in kHz, or 0 to let the tuner choose.
 * @return TEF668X_OK, or what stopped it.
 */
Tef668xError tef668xSetFmBandwidth(uint16_t bandwidthKHz);

/**
 * Set the AM bandwidth.
 *
 * @param bandwidthKHz  The bandwidth in kHz. There is no adaptive mode here,
 *                      so 0 is refused rather than guessed at.
 * @return TEF668X_OK, or TEF668X_ERR_RANGE for 0 or anything above 6000.
 */
Tef668xError tef668xSetAmBandwidth(uint16_t bandwidthKHz);

/**
 * Set the output volume.
 *
 * @param decibels  Gain in dB. The chip takes roughly -60 to +24.
 * @return TEF668X_OK, or TEF668X_ERR_RANGE when it is outside that.
 */
Tef668xError tef668xSetVolume(int8_t decibels);

/**
 * Mute or unmute the audio.
 *
 * @param muted  true to mute.
 * @return TEF668X_OK, or what stopped it.
 */
Tef668xError tef668xSetMute(bool muted);

/**
 * Read the signal quality.
 *
 * @param fm       true to read the FM side, false for AM. The two live in
 *                 different modules on the chip and return different fields.
 * @param quality  Receives the reading.
 * @return TEF668X_OK, or what stopped it.
 *
 * @note `modulationPercent` is signed on purpose. The chip returns an
 *       unsigned word, and a genuinely negative reading comes back as a
 *       number in the thousands. Reading it as signed turns that back into
 *       the small negative it was, which is what stops a bad sample being
 *       taken for a very loud station. See test/fixtures/agc/README.md, where
 *       that wrap is one of the cases the captures were kept for.
 */
Tef668xError tef668xReadQuality(bool fm, Tef668xQuality *quality);

/**
 * Read the raw quality bytes, exactly as they came off the wire.
 *
 * For working out what a reading means when the decoded numbers look wrong.
 *
 * @param fm   true for the FM module, false for AM.
 * @param out  Receives 14 bytes.
 * @return TEF668X_OK, or what stopped it.
 */
Tef668xError tef668xReadQualityRaw(bool fm, uint8_t out[14]);

/**
 * What happened during start up, whether it worked or not.
 *
 * The boot self test in a later phase shows this, and it is the only way to
 * tell a tuner that was skipped from one that was patched and still would not
 * answer, without a serial cable.
 */
typedef struct {
  bool sawDevice;       /**< Something acknowledged at the I2C address. */
  bool readBootStatus;  /**< The operation status came back. */
  uint8_t bootStatus;   /**< What it said. 0 means not patched yet. */
  bool patchLoaded;     /**< A patch was written this boot. */
  uint16_t patchTried;  /**< Which version was written, 0 if none. */
  uint16_t patchWanted; /**< Which version the chip then asked for. */
  uint16_t xtalAdc;     /**< What the crystal sense pin read. */
  const char *xtal;     /**< Which crystal that was taken to mean. */
} Tef668xDiagnostics;

/**
 * Read the start up diagnostics.
 *
 * @return The diagnostics. Never NULL, and safe to call before start up.
 */
const Tef668xDiagnostics *tef668xDiagnostics(void);

/**
 * The identification words, even when start up later failed.
 *
 * Useful when the chip answered but this build could not make sense of what
 * it said, which is otherwise invisible without a serial cable.
 *
 * @param device    Receives the device word.
 * @param hardware  Receives the hardware version word.
 * @param software  Receives the software version word.
 * @return false when the identification was never read.
 */
bool tef668xLastIdentification(uint16_t *device, uint16_t *hardware,
                               uint16_t *software);

/**
 * How the tuner start up went.
 *
 * Defined by the application rather than the driver, because the driver does
 * not know when start up happened. It is declared here so the web page can
 * report the reason without the caller reaching into main.
 *
 * @return TEF668X_OK, or what stopped it.
 */
Tef668xError tunerStartError(void);

#endif /* DRIVERS_TEF668X_H */
