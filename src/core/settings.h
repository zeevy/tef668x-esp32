/**
 * @file settings.h
 * @brief Every setting in one versioned struct, never an address map.
 *
 * Adding a setting is adding a field and bumping SETTINGS_VERSION. Reading a
 * struct written by an older firmware goes through settingsFromBlob, which
 * fills anything the old struct did not have with the default.
 *
 * Nothing in here touches hardware or NVS, so it builds and is tested on a PC.
 * The NVS side lives in settings_store.h.
 */
#ifndef CORE_SETTINGS_H
#define CORE_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Bump this whenever a field is added, removed or changes meaning. */
#define SETTINGS_VERSION 5

/** Room for a 32 character SSID and its terminator. */
#define SETTINGS_SSID_LEN 33

/** Room for a 64 character WPA2 passphrase and its terminator. */
#define SETTINGS_PASS_LEN 65

/**
 * The whole of the radio's saved state.
 *
 * @note Fields are only ever appended. Reordering or resizing an existing
 *       field breaks every radio already in the field.
 */
typedef struct {
  uint16_t version; /**< SETTINGS_VERSION this was written by. */
  uint16_t size;    /**< sizeof(Settings) when written. */
  char wifiSsid[SETTINGS_SSID_LEN]; /**< Empty means no credentials yet. */
  char wifiPass
      [SETTINGS_PASS_LEN]; /**< Empty is allowed, for an open network. */
  uint32_t accessPin;      /**< 0 is the default PIN, 000000. */

  /* --- Added in version 2. Everything below here defaults on an older blob.
   *
   * Grouped the way a person thinks about them: the ones that describe where
   * the radio is, then the FM side, then the AM side. The struct order is
   * append only whatever the grouping, because reordering breaks every radio
   * already in the field.
   * ------------------------------------------------------------------- */

  /* Where the radio is and how it is built. */
  uint8_t fmRegion;    /**< Which FM band plan. See FmRegion in band_plan.h. */
  uint8_t mwSpacing;   /**< Medium wave channel spacing. See MwSpacing. */
  uint8_t encoderKind; /**< Which encoder is fitted. See EncoderKind. */
  uint8_t encoderDirection; /**< Which way round. See EncoderDirection. */

  /** What decides whether the audio is open. See SquelchMode. */
  uint8_t squelchMode;

  /**
   * The volume to come up at when the knob is not the volume control.
   *
   * There is one knob, and in manual squelch it is the squelch. So in that
   * one mode nothing on the radio says how loud to be, and reading the knob
   * would come up at whatever the threshold happens to map to, which is full
   * volume at one end and silence at the other, with nothing to correct it.
   *
   * In every other mode the knob wins and this is not read. That is why a
   * volume is stored at all, given that a stored volume arguing with the
   * knob is exactly what decision 26 refuses.
   */
  int8_t startVolumeDb;

  /* Where it comes up. The band and the frequency it was last left on, so it
   * returns to the station rather than to a frequency written into the
   * firmware. */
  uint8_t startBand;     /**< See BandId. */
  uint32_t startFreqKHz; /**< 0 means the bottom of that band. */

  /* The FM side. */
  uint8_t fmMultipathSuppression; /**< iMS. 0 or 1. */
  uint8_t fmEqualizer;            /**< EQ. 0 or 1. */
  uint8_t fmForcedMono;           /**< Refuse stereo on purpose. 0 or 1. */
  uint8_t fmHighCutStart;         /**< Roll treble off below this, dBuV. */
  uint8_t fmStereoBlendStart;     /**< Blend to mono below this, dBuV. */
  uint8_t fmStHiBlendStart;       /**< Do both below this, dBuV. */
  uint8_t fmNoiseBlankerStart;    /**< Per cent. 0, or 50 to 150. */
  uint16_t fmDeemphasisUs;        /**< 50 here, 75 in the Americas, 0 off. */

  /* The AM side. */
  uint8_t amNoiseBlankerStart; /**< Per cent. 0, or 50 to 150. */
  uint8_t amBandwidthKHz;      /**< The width an AM band starts on. */

  /* Version 3. How fussy seek is about what counts as a station. Separate
   * for the two sides, because the two have different rules and different
   * numbers behind them. See SeekConfig in seek.h. */
  uint8_t fmScanSensitivity; /**< 1 to 6. Higher stops on weaker signals. */
  uint8_t amScanSensitivity; /**< 1 to 6. */

  /* Version 4. What this unit's volume knob actually reaches, learned by
   * turning it end to end. Zero for both means it has never been calibrated
   * and the built in figures are used, which are one radio's numbers. */
  uint16_t potRawMin; /**< Reading at the quiet end. */
  uint16_t potRawMax; /**< Reading at the loud end. */

  /* Version 5. The polish, all of it switchable off. A radio that beeps at
   * you and cannot be told to stop is worse than one that never beeped. */
  /**
   * How long the audio takes to go quiet before it is cut, milliseconds.
   *
   * 0 cuts instantly. Applies to a deliberate mute, to the squelch closing,
   * and to the brief mute around a bandwidth change.
   */
  uint16_t softMuteMs;
  uint8_t beepKey;  /**< Which presses make a sound. See BeepMode. */
  uint8_t beepEdge; /**< Beep when the dial wraps at a band edge. 0 or 1. */
} Settings;

/**
 * Fill a struct with the values a radio leaves the factory with.
 *
 * @param s  Receives the defaults.
 */
void settingsDefaults(Settings *s);

/**
 * Check a struct is usable.
 *
 * Catches a version this firmware does not know and strings with no
 * terminator, which is what a truncated or corrupt NVS blob looks like.
 *
 * @param s  The settings to check.
 * @return true when every field is inside its bounds.
 */
bool settingsValid(const Settings *s);

/**
 * Read a stored blob into a struct, upgrading it if an older firmware wrote it.
 *
 * A blob whose recorded size does not match the number of bytes given is
 * treated as truncated and rejected, rather than read as far as it goes.
 *
 * @param blob  The bytes read back from NVS.
 * @param len   How many bytes there are.
 * @param out   Receives the settings. Left at defaults when the blob is
 *              unusable.
 * @return true when the blob was read, false when defaults were used instead.
 */
bool settingsFromBlob(const void *blob, size_t len, Settings *out);

/**
 * True when the radio has an SSID to try.
 *
 * @param s  The settings to look at.
 * @return true when an SSID is stored.
 */
bool settingsHasWifi(const Settings *s);

/**
 * Copy an SSID and passphrase in, with bounds checking.
 *
 * @param s     The settings to write into.
 * @param ssid  The network name. NULL is refused and returns false.
 * @param pass  The passphrase, or NULL and empty for an open network.
 * @return false when either string is too long to store, in which case
 *         nothing is changed.
 */
bool settingsSetWifi(Settings *s, const char *ssid, const char *pass);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SETTINGS_H */
