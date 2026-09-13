/*
 * Every setting in one versioned struct, never an address map.
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

/* For BAND_COUNT, which sizes the per band arrays below. A count written here
 * instead would go wrong silently the day a band is added. */
#include "band_plan.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bump this whenever a field is added, removed or changes meaning. */
#define SETTINGS_VERSION 9

/* Room for a 32 character SSID and its terminator. */
#define SETTINGS_SSID_LEN 33

/* Room for a 64 character WPA2 passphrase and its terminator. */
#define SETTINGS_PASS_LEN 65

/*
 * The whole of the radio's saved state.
 *
 * Fields are only ever appended. Reordering or resizing an existing
 * field breaks every radio already in the field.
 */
typedef struct {
  uint16_t version;                 /* SETTINGS_VERSION this was written by. */
  uint16_t size;                    /* sizeof(Settings) when written. */
  char wifiSsid[SETTINGS_SSID_LEN]; /* Empty means no credentials yet. */
  char wifiPass[SETTINGS_PASS_LEN]; /* Empty is allowed, for an open network. */
  uint32_t accessPin;               /* 0 is the default PIN, 000000. */

  /* --- Added in version 2. Everything below here defaults on an older blob.
   *
   * Grouped the way a person thinks about them: the ones that describe where
   * the radio is, then the FM side, then the AM side. The struct order is
   * append only whatever the grouping, because reordering breaks every radio
   * already in the field.
   * ------------------------------------------------------------------- */

  /* Where the radio is and how it is built. */
  uint8_t fmRegion;    /* Which FM band plan. See FmRegion in band_plan.h. */
  uint8_t mwSpacing;   /* Medium wave channel spacing. See MwSpacing. */
  uint8_t encoderKind; /* Which encoder is fitted. See EncoderKind. */
  uint8_t encoderDirection; /* Which way round. See EncoderDirection. */

  /* What decides whether the audio is open. See SquelchMode. */
  uint8_t squelchMode;

  /*
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
  uint8_t startBand;     /* See BandId. */
  uint32_t startFreqKHz; /* 0 means the bottom of that band. */

  /* The FM side. */
  uint8_t fmMultipathSuppression; /* iMS. 0 or 1. */
  uint8_t fmEqualizer;            /* EQ. 0 or 1. */
  uint8_t fmForcedMono;           /* Refuse stereo on purpose. 0 or 1. */
  uint8_t fmHighCutStart;         /* Roll treble off below this, dBuV. */
  uint8_t fmStereoBlendStart;     /* Blend to mono below this, dBuV. */
  uint8_t fmStHiBlendStart;       /* Do both below this, dBuV. */
  uint8_t fmNoiseBlankerStart;    /* Per cent. 0, or 50 to 150. */
  uint16_t fmDeemphasisUs;        /* 50 here, 75 in the Americas, 0 off. */

  /* The AM side. */
  uint8_t amNoiseBlankerStart; /* Per cent. 0, or 50 to 150. */
  uint8_t amBandwidthKHz;      /* The width an AM band starts on. */

  /* Version 3. How fussy seek is about what counts as a station. Separate
   * for the two sides, because the two have different rules and different
   * numbers behind them. See SeekConfig in seek.h. */
  uint8_t fmScanSensitivity; /* 1 to 6. Higher stops on weaker signals. */
  uint8_t amScanSensitivity; /* 1 to 6. */

  /* Version 4. What this unit's volume knob actually reaches, learned by
   * turning it end to end. Zero for both means it has never been calibrated
   * and the built in figures are used, which are one radio's numbers. */
  uint16_t potRawMin; /* Reading at the quiet end. */
  uint16_t potRawMax; /* Reading at the loud end. */

  /* Version 5. The polish, all of it switchable off. A radio that beeps at
   * you and cannot be told to stop is worse than one that never beeped. */
  /*
   * How long the audio takes to go quiet before it is cut, milliseconds.
   *
   * 0 cuts instantly. Applies to a deliberate mute, to the squelch closing,
   * and to the brief mute around a bandwidth change.
   */
  uint16_t softMuteMs;
  uint8_t beepKey;  /* Which presses make a sound. See BeepMode. */
  uint8_t beepEdge; /* Beep when the dial wraps at a band edge. 0 or 1. */

  /* Version 6. */
  /*
   * Sound a longer tone at start up, once the tuner is ready. 0 or 1.
   *
   * It cannot come any earlier than that: the tone generator is inside the
   * tuner, so there is nothing to beep with until the tuner has been patched
   * and made active.
   */
  uint8_t beepStart;

  /* Version 7. The panel. Every one of these acts the moment it is written,
   * because the only way to choose a brightness is to look at the panel set
   * to it. A setting that could not be seen until the radio had been left
   * alone for a minute could not be chosen at all. */
  uint8_t backlightPercent; /* How bright the panel is in use. 5 to 100. */
  /*
   * How bright it goes once the radio is left alone. 0 to 100.
   *
   * A value at or above backlightPercent means the dim does nothing.
   */
  uint8_t backlightDimPercent;
  /*
   * How long the radio is left alone before it dims, in seconds. 0 never.
   *
   * Off by default. See decision 28.
   */
  uint8_t backlightDimAfterS;
  /* Fade the panel up at start up rather than snapping it on. 0 or 1. */
  uint8_t backlightFade;

  /* Version 8. What each band was left set to, so coming back to a band
   * returns it to how you had it rather than to a default, and so that
   * survives a power cycle.
   *
   * Indexed by BandId. A zero entry means that band has never been set and
   * takes its own default. Zero is not a real step or tuning mode, and a
   * zero bandwidth is the FM automatic setting, which is what an FM band
   * defaults to anyway, so zero means the same thing in all three. */
  uint32_t bandFreqKHz[BAND_COUNT];      /* Where each band was left. */
  uint16_t bandBandwidthKHz[BAND_COUNT]; /* Filter width, per band. */
  uint16_t bandStepKHz[BAND_COUNT];      /* Step size, per band. */
  uint8_t bandTuneMode[BAND_COUNT];      /* See TuneMode, per band. */

  /* Version 9. */
  /*
   * The level an FM signal has to reach for the auto squelch, in dBuV.
   *
   * 0 switches the floor off and leaves the squelch judging on noise,
   * multipath and offset alone, which is what the reference firmware does
   * and what this radio did before the floor existed.
   *
   * It is a setting because it is the fragile number. The channel beside a
   * strong station is what it rejects, and how strong that channel reads
   * depends on where you are and what is on air: measured here it moved
   * three dB in a few hours, and the usable window between the loudest
   * shoulder and the weakest station is a handful of dB. Somebody in another
   * place will need to move it, and a number nobody can reach is a number
   * that goes wrong quietly.
   *
   * Whole dBuV rather than tenths, and 0 meaning off, to match every other
   * setting here. The cost is that a floor of exactly 0.0 dBuV cannot be
   * asked for, which is no loss: a dead channel on this radio reads below
   * zero, so 0.0 rejects nothing a station would fail.
   */
  uint8_t fmSquelchFloor;
} Settings;

void settingsDefaults(Settings *s);

/*
 * Check a struct is usable.
 *
 * Catches a version this firmware does not know and strings with no
 * terminator, which is what a truncated or corrupt NVS blob looks like.
 */
bool settingsValid(const Settings *s);

/*
 * Read a stored blob into a struct, upgrading it if an older firmware wrote it.
 *
 * A blob whose recorded size does not match the number of bytes given is
 * treated as truncated and rejected, rather than read as far as it goes.
 */
bool settingsFromBlob(const void *blob, size_t len, Settings *out);

bool settingsHasWifi(const Settings *s);

bool settingsSetWifi(Settings *s, const char *ssid, const char *pass);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SETTINGS_H */
