/*
 * Every setting in one versioned struct, never an address map.
 *
 * Adding a setting is adding a field at the end, and bumping SETTINGS_VERSION
 * when the struct grows. Reading a struct written by an older firmware goes
 * through settingsFromBlob, which fills anything the old struct did not have
 * with the default; one written by a newer firmware is read up to the fields
 * this one knows.
 *
 * Nothing in here touches hardware or NVS, so it builds and is tested on a PC.
 * The NVS side lives in drivers/settings_nvs.h.
 */
#ifndef CORE_SETTINGS_H
#define CORE_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* For BAND_COUNT, which sizes the per band arrays below. A count written here
 * instead would go wrong silently the day a band is added. */
#include "band_plan.h"

/* For BatteryShow, which `batteryShow` below is one of. */
#include "battery.h"
#include "meter.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bump this when the struct grows. A new version only adds fields after the
 * old ones and never moves or changes an old one, so a firmware can still
 * read a newer firmware's settings up to the fields it knows, which is what
 * an update that rolls back needs. A field that fits in the padding an older
 * version left keeps the version, since that firmware reads the byte as
 * padding.
 */
#define SETTINGS_VERSION 31

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
   * volume is stored at all, even though a stored volume must never argue
   * with the knob.
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
  /* 50 in most of the world, 75 in the Americas, 0 off. */
  uint16_t fmDeemphasisUs;

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
   * Off by default. A dim happens when nobody is looking, and a dark panel
   * looks like a fault, so a person switches it on for themselves.
   */
  uint8_t backlightDimAfterS;
  /* Fade the panel up at start up rather than snapping it on. 0 or 1. */
  uint8_t backlightFade;

  /* Version 8. What each band was left set to, so coming back to a band
   * returns it to how you had it rather than to a default, and so that
   * survives a power cycle.
   *
   * Indexed by BandId. A zero entry means that band has never been set and
   * takes its own default. Zero is not a real step, and a zero bandwidth is
   * the FM automatic setting, which is what an FM band defaults to anyway,
   * so zero means the same thing in both. */
  uint32_t bandFreqKHz[BAND_COUNT];      /* Where each band was left. */
  uint16_t bandBandwidthKHz[BAND_COUNT]; /* Filter width, per band. */
  uint16_t bandStepKHz[BAND_COUNT];      /* Step size, per band. */
  /*
   * The tuning mode, per band, up to version 12. Nothing reads it now.
   *
   * The mode became one setting for the whole radio in version 13. The five
   * bytes stay because fields here are only ever appended: taking them out
   * would move every field below them and turn every stored blob into
   * nonsense. Version 13 reads this one last time, to carry the mode of the
   * band the radio was on across the upgrade.
   */
  uint8_t bandTuneModeV12[BAND_COUNT];

  /* Version 9. */
  /*
   * The level an FM signal has to reach for the auto squelch, in dBuV.
   *
   * 0 switches the floor off and leaves the squelch judging on noise,
   * multipath and offset alone, which is what the PE5PVB TEF6686_ESP32
   * firmware does.
   *
   * It is a setting because it is the fragile number. The channel beside a
   * strong station is what it rejects, and how strong that channel reads
   * depends on where you are and what is on air: measured on this radio it
   * moved three dB in a few hours, and the usable window between the loudest
   * shoulder and the weakest station is a handful of dB. Somebody in another
   * place will need to move it, and a number nobody can reach is a number that
   * goes wrong quietly.
   *
   * Whole dBuV rather than tenths, and 0 meaning off, to match every other
   * setting here. The cost is that a floor of exactly 0.0 dBuV cannot be
   * asked for, which is no loss: a dead channel on this radio reads below
   * zero, so 0.0 rejects nothing a station would fail.
   */
  uint8_t fmSquelchFloor;

  /* Version 10. */
  /*
   * Whether the RDS decoder runs at all. Nonzero is on, which is the default.
   *
   * Off is not only about not wanting the station name. The decoder is asked
   * for a group every 43 ms on FM, and the radio task wakes for that as well
   * as for its own 100 ms tuner poll, so it comes round about thirty times a
   * second instead of ten. Measured on the radio: 30.8 ms a round on FM with
   * RDS on against 99.8 on medium wave, where it does not run.
   * Turning it off gives that back.
   */
  uint8_t rdsEnabled;

  /* Version 11. */
  /*
   * Whether the radio asks an NTP server for the time. Nonzero is on, which
   * is the default.
   *
   * This is the only source of time the radio has. There is no battery backed
   * clock on this board, and the RDS clock time is shown but never sets the
   * clock, so without NTP the radio does not know what time it is and says so
   * by showing nothing.
   */
  uint8_t ntpEnabled;
  /*
   * How far local time is from UTC, in minutes, signed.
   *
   * Minutes rather than hours because half and quarter hour zones are real:
   * India is +330, Nepal is +345, and Chatham is +765 in winter. A setting in
   * hours cannot express where this radio actually is.
   *
   * The default is 0, which is UTC. That is deliberately not the offset for
   * the place this radio was built, because a default that is right in one
   * country is silently wrong in every other, and a clock that is wrong by a
   * whole number of hours looks exactly like a clock that is right. Somebody
   * setting this up sets it once.
   *
   * Nothing here adjusts for daylight saving. Working out whether a date is
   * inside a summer time rule needs the rules for every zone, which is a
   * table that goes out of date, so the offset is what the person says it is.
   */
  int16_t clockOffsetMinutes;

  /* Version 12. */
  /*
   * Whether the battery is shown, and as what. See BatteryShow.
   *
   * Off by default, and that is not shyness. The sense pin is on ADC2 and the
   * Wi-Fi driver owns ADC2, so a reading is not guaranteed on a radio that is
   * always joined, and the divider ratio is not checked against a meter.
   * Shipping it on would put a number on the panel that may not be the
   * voltage of anything. Somebody who turns it on has decided to look.
   */
  uint8_t batteryShow;

  /* Version 13. */
  /*
   * What the knob does, for the whole radio rather than for each band.
   *
   * See TuneMode in radio.h. The step size and the filter width stay with
   * their band because they are properties of what is on air there; the mode
   * is a property of what the person at the knob is doing, and carrying it
   * from band to band is what they expect.
   *
   * Meter band stepping is the exception the band still gets a say in: it
   * only exists on shortwave, so leaving shortwave in that mode falls back to
   * manual rather than carrying a mode the band cannot do.
   */
  uint8_t tuneMode;

  /* Version 14. The volume AGC. */
  /*
   * The modulation depth every station is brought towards, as a percentage.
   *
   * **0 switches the AGC off, and that is what ships.** The setting doubles
   * as the on switch, because a target is the only thing it needs to run and
   * a separate switch would be a second way to say the same thing.
   *
   * See AGC_TARGET_MIN and AGC_TARGET_MAX in core/agc.h for the range and
   * what the trade is: a lower target evens stations out more and makes
   * everything quieter.
   */
  uint8_t agcTargetPercent;
  /*
   * The most it may add to a quiet station, in dB. 0 is cut only.
   *
   * Off by default even when the AGC is on, because boost clips a station
   * with a low average and full peaks, and it lifts the noise on a weak
   * signal. See AGC_BOOST_MAX.
   */
  uint8_t agcBoostDb;

  /* Version 15. */
  /*
   * The width of a meter block and the gap after it, in pixels. Nothing
   * reads them: every meter has a fixed shape. The two bytes stay so the
   * stored blob keeps its layout, and settingsValid still checks them
   * against METER_SEG_W_MIN and the three beside it in core/meter.h.
   */
  uint8_t meterSegW;
  uint8_t meterSegGap;

  /* Version 16. */
  /*
   * The top of a signal meter, FM and AM, in dBuV. Nothing reads them: the
   * panel has no signal meter, and the scale the tuning scale and the
   * browser use is fixed, core/signal.h. The two bytes stay so the stored
   * blob keeps its layout, because fields here are only ever appended.
   */
  uint8_t sigFullFmDbuV;
  uint8_t sigFullAmDbuV;

  /* Version 17. */
  /*
   * Which theme is drawn by day, by saved index: a palette of
   * core/palette.h, or PALETTE_CUSTOM_THEME for the custom colours below.
   * `nightTheme` is the one drawn at night.
   *
   * A colour, not a number that means anything on its own, so nothing here
   * bounds it against a range the way most bytes in this struct are: the
   * count of themes that ship can grow without this needing a wider type.
   * The saved indexes run from 0 to PALETTE_COUNT, and paletteOfTheme gives
   * PALETTE_NONE for an index past the end.
   */
  uint8_t theme;
  /*
   * The one custom theme's own colours, red, green and blue a byte each, in
   * the order of PaletteRole in core/palette.h: ground, header, rule, then
   * the six that carry meaning. Those are the first nine rows.
   * The last four held the modulation meter's ramp and the menu's selection
   * highlight up to version 20. Nothing draws with them now, and they stay
   * so the struct keeps the shape an older blob was written in.
   *
   * Kept here rather than as the packed 16 bit colour the panel actually
   * draws with, because that packing is a property of this one display
   * controller and core/ knows neither a screen nor a panel. ui/theme.c
   * does the packing when this slot is the one selected. The row count is
   * written by hand rather than read from ui/theme.h's own constant for
   * the same reason: core/ does not include ui/.
   */
  uint8_t customTheme[13][3];

  /* Version 19. */
  /*
   * 0 or 180. The panel's own two readable orientations, the only two
   * `drivers/display.cpp`'s MADCTL pair are: mirroring one axis alone gives
   * a mirror image, not a rotation, which is why there is no 90 or 270
   * here. Recovery mode never reads this: it draws at 0 regardless, since
   * a wrong value here may be exactly what put the radio in recovery.
   */
  uint8_t displayRotation;

  /* Version 20. AM weak signal handling, each a start level in dBuV. MW and
   * SW share a pair and LW has its own, because the NXP manual gives LW its
   * own suggested values. */
  uint8_t amHighCutStart;  /* MW and SW. 0 for off, or 20 to 60. */
  uint8_t lwHighCutStart;  /* LW. The same range. */
  uint8_t amSoftMuteStart; /* MW and SW. 0 to 50, the chip's own range. */
  uint8_t lwSoftMuteStart; /* LW. The same range. */

  /* Version 21. No new field: the five themes replace the ten, so a `theme`
   * written by version 20 or older is read as Nightwatch. */

  /* Version 22. The DX Scanner menu. The first byte sits in the padding
   * version 21 left after `lwSoftMuteStart`. */
  uint8_t dxStopRule;  /* A DxStopRule, core/dx_scan.h. */
  uint8_t dxScanRange; /* A DxScanRange. */
  uint8_t dxMemFirst;  /* The memory-only scan's first slot, 1 to 99. */
  uint8_t dxMemLast;   /* Its last, from dxMemFirst to 99. */
  uint8_t dxLoop;      /* Go round the band again rather than stop. */
  uint8_t dxScanMute;  /* Muted between channels. */
  uint8_t dxAutoLog;   /* Write a NEW catch to the log without being asked. */
  uint16_t dxDwellTenths; /* Tenths of a second on each channel, 5 to 300. */
  uint16_t dxWidthKHz;    /* The width DX mode opens with, an FM width. */

  /* Version 23. In the two bytes version 22 left as padding after
   * `dxWidthKHz`, so the struct did not grow. */
  uint8_t rdsRegion; /* An RdsRegion, core/rds_country.h. */

  /* Version 24. In the byte version 23 left as padding after `rdsRegion`,
   * so the struct did not grow. */
  uint8_t dxLogRt; /* Put the radio text in each log entry. */

  /* Version 25. Past the end of version 24's 264 bytes. */
  uint8_t dxWatch; /* Watch the presets in DX mode's range. */

  /* Version 26. In the padding version 25 left after `dxWatch`, so the
   * struct did not grow. Added to every level shown or exported, FM for FM
   * and OIRT, AM for LW, MW and SW, in whole dB, SIGNAL_LEVEL_OFFSET_MIN_DB
   * to SIGNAL_LEVEL_OFFSET_MAX_DB. */
  int8_t levelOffsetFmDb;
  int8_t levelOffsetAmDb;

  /* Version 27. In the last byte of padding version 26 left after
   * `levelOffsetAmDb`, so the struct did not grow. The theme drawn from
   * 18:00 to 05:59 local time, read the same way as `theme`, which is the
   * one drawn by day and whenever the time is not known. */
  uint8_t nightTheme;

  /* Version 28. Past the end of version 27's 268 bytes. What the radio's own
   * hotspot may do, a WifiHotspot, core/wifi_join.h: Auto, On or Off. */
  uint8_t hotspot;

  /* Version 29. In the padding version 28 left after `hotspot`, so the
   * struct did not grow. 1 for on, the default; 0 for off. */
  uint8_t webEnabled;  /* The pages, the API and updates over Wi-Fi. */
  uint8_t wifiEnabled; /* Wi-Fi at all: the network and the hotspot. */

  /* Version 30. Auto off in one byte, in the last byte of padding version
   * 29 left. Only read now, into `autoOffMinutes`: version 31 needed two
   * bytes, for times up to 600 minutes. */
  uint8_t autoOffMinutesV30;

  /* Version 31. Past the end of version 30's 272 bytes. How many minutes
   * alone before the radio goes to sleep, one `autoOffMinutesOk` takes; 0
   * for never. */
  uint16_t autoOffMinutes;

  /* In the padding version 31 leaves after `autoOffMinutes`, with the
   * version left at 31, so a firmware that does not know it still reads
   * these settings after a rollback. Every struct starts zeroed, so a blob
   * written before it reads as 0. 1 to look on GitHub for a newer release
   * once the radio is on the network; 0, the default, never to look. */
  uint8_t updateCheck;

  /* In the last byte of padding version 31 leaves, with the version left at
   * 31 for the same reason as `updateCheck`. Kept as off rather than on, so
   * the 0 a blob written before it holds reads as touch on, the default. 1
   * for the touch screen not to be read, for a panel that touches itself. */
  uint8_t touchOff;
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

/*
 * Store a network to join. A hotspot set On goes back to Auto, since new
 * network details are given to be joined, and On never tries the stored
 * network. False, and nothing changed, for a name or passphrase too long.
 */
bool settingsSetWifi(Settings *s, const char *ssid, const char *pass);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SETTINGS_H */
