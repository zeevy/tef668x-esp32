/* Implementation of the settings struct, its defaults and migration. */
#include "settings.h"

#include <stddef.h>
#include <string.h>

/* For the ranges below. A setting that names a band, an encoder or a squelch
 * mode is checked against that enum and not against a number written here,
 * because a number written here goes wrong silently the day the enum gains a
 * member. */
#include "agc.h"
#include "auto_off.h"
#include "backlight.h"
#include "band_plan.h"
#include "battery.h"
#include "clock.h"
#include "dx.h"
#include "dx_scan.h"
#include "input.h"
#include "memory.h"
#include "palette.h"
#include "radio.h"
#include "rds_country.h"
#include "seek.h"
#include "signal.h"
#include "squelch.h"
#include "wifi_join.h"

/*
 * How many bytes each shipped version of the struct took.
 *
 * Every version but the newest is a number written out by hand, and the
 * newest is sizeof(Settings). When SETTINGS_VERSION goes up, the entry that
 * was sizeof becomes the number it gave. That is the whole migration
 * mechanism: a version is a known length, so a blob that
 * is not that length is corrupt rather than old.
 */
static uint16_t settingsSizeOfVersion(uint16_t version) {
  switch (version) {
    case 1:
      /* Written out by hand, because sizeof(Settings) is the newest
       * version's size. A radio holding a version 1 blob wrote exactly this
       * many bytes, and that never changes. */
      return 108;
    case 2:
      /* Written out by hand for the same reason version 1 is. A radio
       * holding a version 2 blob wrote exactly this many bytes. */
      return 132;
    case 3:
      /* Written out by hand, like 1 and 2. A radio holding a version 3 blob
       * wrote exactly this many bytes. */
      return 136;
    case 4:
      /* Written out by hand, like the versions before it. */
      return 140;
    case 5:
      /* Written out by hand, like the versions before it. */
      return 144;
    case 6:
      /* The same 144 bytes version 5 wrote. Version 6 added one byte,
       * `beepStart`, and it went into padding version 5 already had, so the
       * struct did not grow. The two are told apart by the version field,
       * which is what that field is for. */
      return 144;
    case 7:
      /* Written out by hand, like the versions before it. */
      return 148;
    case 8:
      /* Written out by hand, like the versions before it. */
      return 196;
    case 9:
      /* Written out by hand, like the versions before it. */
      return 196;
    case 10:
      /* The same 196 bytes version 9 wrote. Version 10 added one byte,
       * `rdsEnabled`, and it went into padding version 9 already had, so the
       * struct did not grow. The same case as `beepStart` in version 6. */
      return 196;
    case 11:
      /* Written out by hand, like the versions before it. */
      return 200;
    case 12:
      /* Written out by hand, like the versions before it. */
      return 200;
    case 13:
      /* 200, written out by hand rather than as `sizeof`. The size a version
       * wrote is fixed once it ships and cannot be asked of the struct as it
       * is today, so every older version in this table is a number. */
      return 200;
    case 14:
      /* 204, written out by hand for the reason version 13 gives. Version 14
       * put two bytes on the end for the volume AGC, past the last byte
       * version 13 filled, so the struct grew. */
      return 204;
    case 15:
      /* 204, written out by hand now that version 16 has grown the struct.
       * Version 15 added the two meter segment bytes into padding version 14
       * already had, so it wrote the same 204 bytes version 14 did. */
      return 204;
    case 16:
      /* 208, written out by hand now that version 17 has grown the struct
       * again. Version 16 put the two signal scale bytes on the end at
       * offsets 204 and 205, past everything version 15 wrote, growing the
       * struct to 208 with two bytes of padding after them. */
      return 208;
    case 17:
      /* 256, written out by hand now that version 18 has changed the
       * struct again. Version 17 put `theme` and a sixteen row
       * `customTheme` on the end, past the two bytes of padding version
       * 16 left. */
      return 256;
    case 18:
      /* Version 18 shrank `customTheme` from sixteen rows to thirteen, so
       * the struct is smaller than version 17's despite being newer. 248,
       * written out by hand now that version 19 has changed the struct
       * again. */
      return 248;
    case 19:
      /* `displayRotation` landed in the single byte of padding version 18
       * already left after `customTheme`, so the struct did not grow: the
       * same 248 bytes, the two versions told apart by the version field. */
      return 248;
    case 20:
      /* 252, written out by hand now that version 21 exists. */
      return 252;
    case 21:
      /* The same 252 bytes. Version 21 added no field; it changed what
       * `theme` means. Written out by hand now that version 22 exists. */
      return 252;
    case 22:
      /* 264, written out by hand now that version 23 exists. */
      return 264;
    case 23:
      /* The same 264 bytes. `rdsRegion` went into the padding version 22
       * left after `dxWidthKHz`, so only the version field tells the two
       * apart. Written out by hand now that version 24 exists. */
      return 264;
    case 24:
      /* The same 264 bytes again. `dxLogRt` went into the byte version 23
       * left after `rdsRegion`. Written out by hand now that version 25
       * exists. */
      return 264;
    case 25:
      /* 268, written out by hand now that version 26 exists. */
      return 268;
    case 26:
      /* The same 268 bytes: the two offsets went into version 25's
       * padding after `dxWatch`. Written out by hand now that version 27
       * exists. */
      return 268;
    case 27:
      /* The same 268 bytes again: `nightTheme` went into version 26's last
       * byte of padding. Written out by hand now that version 28 exists. */
      return 268;
    case 28:
      /* 272, written out by hand now that version 29 exists. */
      return 272;
    case 29:
      /* The same 272 bytes: the two switches went into version 28's
       * padding after `hotspot`. Written out by hand now that version 30
       * exists. */
      return 272;
    case 30:
      /* The same 272 bytes again: auto off went into version 29's last byte
       * of padding. Written out by hand now that version 31 exists. */
      return 272;
    case 31:
      /* 276: two bytes for auto off past version 30's end, and two of
       * padding. */
      return (uint16_t)sizeof(Settings);
    default:
      return 0;
  }
}

/*
 * Where the fields of a version end, which is not the same as its size.
 *
 * A struct's trailing padding belongs to no field, and a field added later
 * can land inside it. `potRawMin` sits at offset 134, inside the two bytes
 * version 3 wrote as padding after its last field. Copying a version 3 blob
 * by its written length would take the new field out of that old padding.
 *
 * It is zero on every radio in the field, because the struct is memset before
 * it is filled, so this has never gone wrong. It is written this way because
 * "it happens to be zero" is not a reason, and the next field to land in
 * padding may not be so lucky.
 *
 * Expressed with offsetof rather than as numbers, so it cannot drift from the
 * struct the way a hand written offset would.
 */
static size_t settingsFieldEndOfVersion(uint16_t version) {
  switch (version) {
    case 1:
      return offsetof(Settings, fmRegion);
    case 2:
      return offsetof(Settings, fmScanSensitivity);
    case 3:
      return offsetof(Settings, potRawMin);
    case 4:
      return offsetof(Settings, softMuteMs);
    case 5:
      return offsetof(Settings, beepStart);
    case 6:
      /* `backlightPercent` sits at offset 143, in the single byte version 6
       * wrote as padding after `beepStart`. The same case as `potRawMin`
       * above, and the reason this table is separate from the size one. */
      return offsetof(Settings, backlightPercent);
    case 7:
      return offsetof(Settings, bandFreqKHz);
    case 8:
      return offsetof(Settings, fmSquelchFloor);
    case 9:
      /* `rdsEnabled` sits at offset 194, in the padding version 9 wrote after
       * `fmSquelchFloor`. The same case as `potRawMin` and `backlightPercent`
       * above, and the reason this table is separate from the size one. */
      return offsetof(Settings, rdsEnabled);
    case 10:
      /* `ntpEnabled` sits at offset 195, in the padding version 10 wrote
       * after `rdsEnabled`. The same case as `potRawMin`, `backlightPercent`
       * and `rdsEnabled` above. */
      return offsetof(Settings, ntpEnabled);
    case 11:
      /* `batteryShow` sits at offset 198, in the padding version 11 wrote
       * after `clockOffsetMinutes`. The same case as `potRawMin`,
       * `backlightPercent`, `rdsEnabled` and `ntpEnabled` above. */
      return offsetof(Settings, batteryShow);
    case 12:
      /* `tuneMode` sits at offset 199, in the single byte version 12 wrote as
       * padding after `batteryShow`. The same case as `potRawMin`,
       * `backlightPercent`, `rdsEnabled` and `ntpEnabled` above. */
      return offsetof(Settings, tuneMode);
    case 13:
      /* Everything up to the AGC settings, which version 14 added on the end
       * rather than into padding: version 13 filled the last byte it had. */
      return offsetof(Settings, agcTargetPercent);
    case 14:
      /* `meterSegW` and `meterSegGap` sit at offsets 202 and 203, in the two
       * bytes version 14 wrote as padding after `agcBoostDb`. Both have a
       * minimum of 1, so a version 14 blob copied by its written length
       * brings two zeros in, `settingsValid` refuses the struct, and the
       * radio comes up on defaults having lost its PIN, its station and its
       * calibration. The same case as `potRawMin`, `backlightPercent`,
       * `rdsEnabled`, `ntpEnabled`, `batteryShow` and `tuneMode` above. */
      return offsetof(Settings, meterSegW);
    case 15:
      /* The two signal scale bytes sit at 204 and 205, past the end of what
       * version 15 wrote, so a version 15 blob is copied whole. Unlike
       * `meterSegW` in version 14 there is no padding to land in: version 15
       * filled its last byte. */
      return offsetof(Settings, sigFullFmDbuV);
    case 16:
      /* `theme` sits right after `sigFullAmDbuV`, where version 16 filled
       * its last byte with no padding behind it. Version 16's own end is
       * here rather than at sizeof(Settings), which now also counts
       * `theme` and `customTheme` and would otherwise hand a version 16
       * blob's copy both of those fields it never wrote. */
      return offsetof(Settings, theme);
    case 17:
      /* A version 17 blob's own `theme` and `customTheme` are not carried
       * across. `theme` is an index into ui/theme.c's table, and version
       * 18 renamed nine of its ten rows and moved every one of them but
       * Nightwatch to a different position, so an old index picked at
       * random out of the new table is not the theme it used to be, which
       * is the same kind of wrong answer a stale `customTheme` shape would
       * give if copied byte for byte into a struct whose rows now mean
       * something else. Both reset to what settingsDefaults just put
       * there, the same as a field this version never wrote at all. */
      return offsetof(Settings, theme);
    case 18:
      /* `displayRotation` sits in the single byte of padding version 18
       * left after `customTheme`. Copying a version 18 blob by its own
       * written length, 248, would read that byte as this radio's own
       * rotation rather than the neighbour it actually is, and `Settings`
       * happens to be 248 bytes on both sides of this change, so the
       * length check alone cannot tell an old blob from a new one here.
       * The same case as `potRawMin` and every field after it above. */
      return offsetof(Settings, displayRotation);
    case 19:
      /* `amHighCutStart` sits at offset 247, in the single byte version 19
       * wrote as padding after `displayRotation`. The same case as
       * `potRawMin` and every field after it above. */
      return offsetof(Settings, amHighCutStart);
    case 20:
    case 21:
      /* `dxStopRule` sits at offset 251, in the byte versions 20 and 21
       * wrote as padding after `lwSoftMuteStart`. The same case as
       * `potRawMin` and every field after it above. */
      return offsetof(Settings, dxStopRule);
    case 22:
      /* `rdsRegion` sits at offset 262, in the two bytes version 22 wrote
       * as padding after `dxWidthKHz`, and the struct is 264 bytes on both
       * sides of the change. The same case as `potRawMin` and every field
       * after it above. */
      return offsetof(Settings, rdsRegion);
    case 23:
      /* `dxLogRt` sits at offset 263, in the byte version 23 wrote as
       * padding after `rdsRegion`. The same case again. */
      return offsetof(Settings, dxLogRt);
    case 24:
      /* `dxWatch` starts at 264, just past version 24's end. */
      return offsetof(Settings, dxWatch);
    case 25:
      /* `levelOffsetFmDb` sits at 265, in the padding version 25 wrote
       * after `dxWatch`. The same case as `rdsRegion` above. */
      return offsetof(Settings, levelOffsetFmDb);
    case 26:
      /* `nightTheme` sits at 267, in the byte version 26 wrote as padding
       * after `levelOffsetAmDb`. The same case as `rdsRegion` above. */
      return offsetof(Settings, nightTheme);
    case 27:
      /* `hotspot` starts at 268, just past version 27's end. */
      return offsetof(Settings, hotspot);
    case 28:
      /* `webEnabled` sits at 269, in the padding version 28 wrote after
       * `hotspot`. The same case as `rdsRegion` above. */
      return offsetof(Settings, webEnabled);
    case 29:
      /* `autoOffMinutesV30` sits at 271, in the byte version 29 wrote as
       * padding after `wifiEnabled`. The same case as `rdsRegion` above. */
      return offsetof(Settings, autoOffMinutesV30);
    case 30:
      /* `autoOffMinutes` starts at 272, just past version 30's end. */
      return offsetof(Settings, autoOffMinutes);
    case 31:
      return sizeof(Settings);
    default:
      return 0;
  }
}

static bool startLevelOk(uint8_t level) {
  return level == 0 || (level >= 20 && level <= 60);
}

static bool blankerOk(uint8_t percent) {
  return percent == 0 || (percent >= 50 && percent <= 150);
}

static bool terminated(const char *s, size_t cap) {
  for (size_t i = 0; i < cap; i++) {
    if (s[i] == '\0') {
      return true;
    }
  }
  return false;
}

void settingsDefaults(Settings *s) {
  memset(s, 0, sizeof(*s));
  s->version = SETTINGS_VERSION;
  s->size = (uint16_t)sizeof(Settings);
  s->wifiSsid[0] = '\0';
  s->wifiPass[0] = '\0';
  s->accessPin = 0;

  /* Version 2. These are the values the radio ships with, and every one of
   * them is either the PE5PVB TEF6686_ESP32 firmware's or something
   * measured on this radio. None is a preference. */
  s->fmRegion = (uint8_t)FM_REGION_WORLD; /* 87.5 to 108. */
  s->mwSpacing = (uint8_t)MW_SPACING_9K;  /* Europe, Africa and Asia. */
  s->encoderKind = (uint8_t)ENCODER_STANDARD;
  s->encoderDirection = (uint8_t)ENCODER_NORMAL;
  /* Nothing goes quiet unasked. */
  s->squelchMode = (uint8_t)SQUELCH_OFF;
  /* Only read in manual squelch, and the only way to store that mode is
   * /api/save, which stores the volume in the same call. So this is not
   * reachable in practice. It is quiet rather than loud because a wrong
   * guess that is quiet can be turned up and a wrong guess that is loud
   * cannot be taken back. */
  s->startVolumeDb = -20;
  s->startBand = (uint8_t)BAND_FM;
  /* 104.0 MHz, so a radio updated from a build with no start frequency
   * setting comes up where that build put it, not at the bottom of the
   * band. */
  s->startFreqKHz = 104000;

  s->fmMultipathSuppression = 0;
  s->fmEqualizer = 0;
  s->fmForcedMono = 0;
  s->fmHighCutStart = 0; /* Off, as the reference ships. */
  s->fmStereoBlendStart = 0;
  s->fmStHiBlendStart = 0;
  s->fmNoiseBlankerStart = 0;
  s->fmDeemphasisUs = 50; /* 75 in the Americas. Audible either way. */

  /* 100 per cent, the chip's own default. The NXP manual ships both AM
   * blankers on at 100, and impulse noise is most of what makes medium wave
   * and shortwave tiring to listen to. */
  s->amNoiseBlankerStart = 100;
  s->amBandwidthKHz = 4; /* Measured clearest on 738 kHz. */

  /* Version 3. The reference firmware's default, and the one a sweep of the
   * FM band on this radio shows stopping on every station on air and on
   * nothing else. */
  s->fmScanSensitivity = SEEK_SENSITIVITY_DEFAULT;
  s->amScanSensitivity = SEEK_SENSITIVITY_DEFAULT;

  /* Version 4. Not calibrated, so the built in travel is used. */
  s->potRawMin = 0;
  s->potRawMax = 0;

  /* Version 5. The ramp on, the beeps off. A ramp is only noticed when it is
   * missing, so it is a safe default. A beep is noticed every time, so it is
   * not. */
  s->softMuteMs = RADIO_SOFT_MUTE_MS;
  s->beepKey = (uint8_t)BEEP_OFF;
  s->beepEdge = 0;

  /* Version 6. On, unlike the other beeps. The chime says the radio came on
   * at all, and at that moment silence looks the same as a fault. */
  s->beepStart = 1;

  /* Version 7. The panel full on, the fade at boot on, the dim off. A fade
   * is only noticed when it is missing, and a panel that goes dark on its
   * own reads as a fault to anybody who did not ask for it. */
  s->backlightPercent = 100;
  s->backlightDimPercent = 20;
  s->backlightDimAfterS = 0;
  s->backlightFade = 1;

  /* Version 8. Every band unset, so each takes its own default until somebody
   * leaves it somewhere. memset above already did this; it is written out
   * because a zero that means something has to be stated, not inferred. */
  for (size_t i = 0; i < BAND_COUNT; i++) {
    s->bandFreqKHz[i] = 0;
    s->bandBandwidthKHz[i] = 0;
    s->bandStepKHz[i] = 0;
    s->bandTuneModeV12[i] = 0;
  }

  /* Version 9. Measured on this radio. */
  s->fmSquelchFloor = SQUELCH_FM_LEVEL_FLOOR_DBUV;

  /* Version 10. On, because a radio that decodes RDS and does not show it is
   * not what anybody expects, and the cost is a third of a millisecond of
   * tuner bus traffic every 43 ms. */
  s->rdsEnabled = 1;

  /* Version 11. On, because NTP is the only source of time on this radio.
   * The RDS clock time is shown but never sets the clock. */
  s->ntpEnabled = 1;
  /* UTC. The reasoning for not defaulting to a real place is in settings.h. */
  s->clockOffsetMinutes = 0;

  /* Version 12. Off, for the reasons in settings.h: nothing about the reading
   * has been confirmed on this unit. */
  s->batteryShow = (uint8_t)BATTERY_SHOW_OFF;

  /* Version 13. Manual, which is what a radio nobody has told otherwise
   * should do: the knob steps and nothing moves on its own. */
  s->tuneMode = (uint8_t)TUNE_MODE_MANUAL;

  /*
   * The volume AGC, off.
   *
   * A target of zero is the off switch, and it ships off for the same reason
   * everything else in this struct that changes what a person hears ships
   * off: a radio should do what it did yesterday until somebody asks it not
   * to. An upgrade from an older blob lands here too, because the bytes for
   * these two were never written by it.
   */
  s->agcTargetPercent = 0;
  s->agcBoostDb = 0;

  /* Version 15. Unused, and kept only so the stored blob keeps its layout,
   * like the sigFull* bytes. Set to values settingsValid accepts. */
  s->meterSegW = METER_SEG_W;
  s->meterSegGap = METER_SEG_GAP;

  /* Version 17 the day theme and version 27 the night one: Clear Day by
   * day, in light that would wash out a dark theme, and Nightwatch by
   * night. */
  s->theme = PALETTE_THEME_DEFAULT_DAY;
  s->nightTheme = PALETTE_THEME_DEFAULT_NIGHT;
  /* Version 28. Auto, which is how the hotspot behaved before the setting. */
  s->hotspot = WIFI_HOTSPOT_AUTO;
  /* Version 29. Both on, as every radio before the switches was. */
  s->webEnabled = 1;
  s->wifiEnabled = 1;
  /* Version 30 and 31. Never, so no radio goes to sleep because it was
   * updated. */
  s->autoOffMinutesV30 = 0;
  s->autoOffMinutes = 0;
  /*
   * The custom slot's own starting colours, so picking it before ever
   * touching a colour wheel still shows a considered theme rather than a
   * copy of Nightwatch under a different name. A cool, icy identity, kept
   * distinct from Nightwatch's amber and cyan: `measurement` against
   * `ground` is 10.9:1, well past the 4.5:1 a reading needs, `radio` and
   * `broadcast` sit close together at 12.5:1 and 12.4:1 so neither reads
   * as more important than the other, and the structural roles,
   * `ground`/`header`/`rule`, all stay under 1.3:1 against each other.
   *
   * Thirteen rows in the order ui/theme.h's Theme struct lists its fields.
   * Only the first nine are drawn; see `customTheme` in settings.h.
   */
  static const uint8_t kCustomDefaultRgb[13][3] = {
      {0x0A, 0x10, 0x16}, /* ground */
      {0x09, 0x12, 0x1A}, /* header */
      {0x1B, 0x2A, 0x36}, /* rule */
      {0x4F, 0xE3, 0xFF}, /* radio, icy cyan */
      {0xFF, 0xC8, 0x57}, /* broadcast, warm gold */
      {0xC9, 0xD8, 0xE0}, /* measurement */
      {0x33, 0xD6, 0xA0}, /* good */
      {0xFF, 0x8F, 0x80}, /* fault */
      {0x93, 0xA8, 0xBE}, /* dead */
      {0x33, 0xD6, 0xA0}, /* not drawn */
      {0x6F, 0xCF, 0xC0}, /* not drawn */
      {0x4F, 0xE3, 0xFF}, /* not drawn */
      {0x1B, 0x2A, 0x36}, /* not drawn */
  };
  memcpy(s->customTheme, kCustomDefaultRgb, sizeof(kCustomDefaultRgb));

  /* Version 19. The panel as it left the board, not flipped. */
  s->displayRotation = 0;

  /* Version 20. The NXP manual's suggested values "for improved field
   * performance": high cut from 47 dBuV on MW and SW and 52 on LW, soft mute
   * from 34 dBuV on LW. MW and SW soft mute keeps the chip's own default of
   * 28, because the manual suggests nothing else for them. */
  s->amHighCutStart = 47;
  s->lwHighCutStart = 52;
  s->amSoftMuteStart = 28;
  s->lwSoftMuteStart = 34;

  /* Version 22. DX mode as it scans in version 21, so an update changes
   * nothing until a person changes it: the band less the stored channels, a
   * stop on NEW only, muted, the auto log on, once round, 2.5 s a channel,
   * and 114 kHz, the narrowest width measured to leave RDS whole. */
  s->dxStopRule = DX_STOP_NEW;
  s->dxScanRange = DX_RANGE_BAND_LESS_MEMORY;
  s->dxMemFirst = 1;
  s->dxMemLast = MEMORY_SLOT_COUNT;
  s->dxLoop = 0;
  s->dxScanMute = 1;
  s->dxAutoLog = 1;
  s->dxDwellTenths = DX_SCAN_DWELL_MS / 100u;
  s->dxWidthKHz = DX_BANDWIDTH_DEFAULT_KHZ;
  s->rdsRegion = RDS_REGION_EUROPE;
  s->dxLogRt = 1;
  s->dxWatch = 1;
  s->levelOffsetFmDb = 0;
  s->levelOffsetAmDb = 0;
}

bool settingsValid(const Settings *s) {
  if (s->version == 0 || s->version > SETTINGS_VERSION) {
    return false;
  }
  if (!terminated(s->wifiSsid, SETTINGS_SSID_LEN)) {
    return false;
  }
  if (!terminated(s->wifiPass, SETTINGS_PASS_LEN)) {
    return false;
  }
  if (s->accessPin >= 1000000UL) {
    return false;
  }

  /* Every range below is the hardware's, not a preference. A value outside
   * one of these is a setting that is switched on and does nothing, so it is
   * refused here rather than in whichever caller happens to exist. */
  if (s->fmRegion >= (uint8_t)FM_REGION_COUNT ||
      s->mwSpacing > (uint8_t)MW_SPACING_10K) {
    return false;
  }
  if (s->encoderKind > (uint8_t)ENCODER_OPTICAL ||
      s->encoderDirection > (uint8_t)ENCODER_REVERSED) {
    return false;
  }
  /* The frequency is not checked here, because what it has to be inside is
   * the band plan, and that is a different setting. radioFromSettings judges
   * it against the plan and falls back to the band's own start. */
  if (s->squelchMode >= (uint8_t)SQUELCH_MODE_COUNT ||
      s->startBand >= (uint8_t)BAND_COUNT) {
    return false;
  }
  if (s->fmMultipathSuppression > 1 || s->fmEqualizer > 1 ||
      s->fmForcedMono > 1) {
    return false;
  }
  /* The blend start levels: off, or somewhere a signal actually reaches. */
  if (!startLevelOk(s->fmHighCutStart) ||
      !startLevelOk(s->fmStereoBlendStart) ||
      !startLevelOk(s->fmStHiBlendStart)) {
    return false;
  }
  /* The AM side. The high cut shares the FM range, which is the chip's. The
   * soft mute start is the chip's own 0 to 50, and it has no off: the manual
   * gives mode 0 for evaluation only. */
  if (!startLevelOk(s->amHighCutStart) || !startLevelOk(s->lwHighCutStart) ||
      s->amSoftMuteStart > 50 || s->lwSoftMuteStart > 50) {
    return false;
  }
  /* The blankers are percentages, not levels. */
  if (!blankerOk(s->fmNoiseBlankerStart) ||
      !blankerOk(s->amNoiseBlankerStart)) {
    return false;
  }
  if (s->fmDeemphasisUs != 0 && s->fmDeemphasisUs != 50 &&
      s->fmDeemphasisUs != 75) {
    return false;
  }
  if (s->amBandwidthKHz != 3 && s->amBandwidthKHz != 4 &&
      s->amBandwidthKHz != 6 && s->amBandwidthKHz != 8) {
    return false;
  }
  if (s->startVolumeDb < RADIO_VOLUME_MIN || s->startVolumeDb > 0) {
    return false;
  }
  /* The only two MADCTL combinations `drivers/display.cpp` can read as a
   * rotation rather than a mirror image. */
  if (s->displayRotation != 0 && s->displayRotation != 180) {
    return false;
  }
  if (s->dxStopRule >= DX_STOP_COUNT || s->dxScanRange >= DX_RANGE_COUNT ||
      s->dxMemFirst < 1 || s->dxMemLast > MEMORY_SLOT_COUNT ||
      s->dxMemFirst > s->dxMemLast || s->dxLoop > 1 || s->dxScanMute > 1 ||
      s->dxAutoLog > 1 || s->dxDwellTenths < DX_SCAN_DWELL_MIN_TENTHS ||
      s->dxDwellTenths > DX_SCAN_DWELL_MAX_TENTHS ||
      !bandBandwidthAllowed(BAND_FM, s->dxWidthKHz) || s->dxWidthKHz == 0) {
    return false;
  }
  if (s->rdsRegion >= RDS_REGION_COUNT || s->dxLogRt > 1 || s->dxWatch > 1 ||
      s->hotspot >= WIFI_HOTSPOT_COUNT || s->webEnabled > 1 ||
      s->wifiEnabled > 1 || s->levelOffsetFmDb < SIGNAL_LEVEL_OFFSET_MIN_DB ||
      s->levelOffsetFmDb > SIGNAL_LEVEL_OFFSET_MAX_DB ||
      s->levelOffsetAmDb < SIGNAL_LEVEL_OFFSET_MIN_DB ||
      s->levelOffsetAmDb > SIGNAL_LEVEL_OFFSET_MAX_DB ||
      !autoOffMinutesOk(s->autoOffMinutes)) {
    return false;
  }
  /* The loud end alone says whether this knob has been measured. Zero there
   * means it has not, because a measured travel has to span a quarter of the
   * converter and so can never end at zero.
   *
   * The quiet end cannot carry that meaning: this unit's knob reads 0 at the
   * bottom, so a correct sweep of it gives 0 and 4095 and a rule that took a
   * zero quiet end to mean "not measured" would refuse the one reading this
   * radio actually produces. */
  if (s->potRawMax == 0) {
    if (s->potRawMin != 0) {
      return false;
    }
  } else if (s->potRawMax <= s->potRawMin) {
    return false;
  }
  if (s->potRawMin > 4095 || s->potRawMax > 4095) {
    return false;
  }
  /* Long enough to be a ramp and short enough that mute still feels like a
   * button. Anything past half a second is somebody typing a number in. */
  if (s->softMuteMs > 500) {
    return false;
  }
  if (s->beepKey >= (uint8_t)BEEP_MODE_COUNT || s->beepEdge > 1 ||
      s->beepStart > 1) {
    return false;
  }
  /* Bright enough to read by. A panel driven to nothing while the radio is
   * being used is a panel that looks broken, and the only control for it is
   * the page that has just gone dark. The dim level has no floor, because
   * that one is left on purpose and any input brings it back. */
  if (s->backlightPercent < BACKLIGHT_MIN_AWAKE || s->backlightPercent > 100) {
    return false;
  }
  if (s->backlightDimPercent > 100 ||
      s->backlightDimAfterS > BACKLIGHT_DIM_AFTER_MAX_S ||
      s->backlightFade > 1) {
    return false;
  }
  /* The tuning mode. The step and the width are not checked here: what they
   * have to be inside is the band plan, which is a different setting, and
   * radioApply judges each against the band it belongs to and falls back to
   * that band's default. A mode is different, because it is an enum and a
   * value outside it would index nothing. */
  if (s->tuneMode >= (uint8_t)TUNE_MODE_COUNT) {
    return false;
  }
  /*
   * The AGC target is zero or a real target, never in between.
   *
   * The same shape as the start levels and the blankers above: zero is off,
   * and the band of values below AGC_TARGET_MIN is not a quieter setting, it
   * is a number nothing sensible can be done with.
   */
  if (s->agcTargetPercent != 0 && (s->agcTargetPercent < AGC_TARGET_MIN ||
                                   s->agcTargetPercent > AGC_TARGET_MAX)) {
    return false;
  }
  if (s->agcBoostDb > AGC_BOOST_MAX) {
    return false;
  }
  /* A floor above anything the band produces would mute every station, so
   * the range stops well short of that. 0 switches it off. */
  if (s->fmSquelchFloor > SQUELCH_FM_LEVEL_FLOOR_MAX_DBUV) {
    return false;
  }
  if (s->fmScanSensitivity < SEEK_SENSITIVITY_MIN ||
      s->fmScanSensitivity > SEEK_SENSITIVITY_MAX ||
      s->amScanSensitivity < SEEK_SENSITIVITY_MIN ||
      s->amScanSensitivity > SEEK_SENSITIVITY_MAX) {
    return false;
  }
  if (s->ntpEnabled > 1) {
    return false;
  }
  if (s->batteryShow >= (uint8_t)BATTERY_SHOW_COUNT) {
    return false;
  }
  if (s->clockOffsetMinutes < CLOCK_OFFSET_MIN_MINUTES ||
      s->clockOffsetMinutes > CLOCK_OFFSET_MAX_MINUTES) {
    return false;
  }
  if (s->meterSegW < METER_SEG_W_MIN || s->meterSegW > METER_SEG_W_MAX ||
      s->meterSegGap < METER_SEG_GAP_MIN ||
      s->meterSegGap > METER_SEG_GAP_MAX) {
    return false;
  }
  return true;
}

bool settingsFromBlob(const void *blob, size_t len, Settings *out) {
  settingsDefaults(out);
  if (blob == NULL || len < sizeof(uint16_t) * 2) {
    return false;
  }

  uint16_t version;
  uint16_t storedSize;
  memcpy(&version, blob, sizeof(version));
  memcpy(&storedSize, (const uint8_t *)blob + sizeof(version),
         sizeof(storedSize));

  if (version == 0 || version > SETTINGS_VERSION) {
    /* Written by a newer firmware, or plain corrupt. Defaults are safer than
     * reading fields that may have moved. */
    return false;
  }

  /* A blob has to be exactly the size the firmware that wrote it used. The
   * size field alone is not enough, because a corrupt blob can declare a
   * small size that matches its own truncated length and then read back as a
   * valid one character SSID. */
  if (storedSize != len || storedSize != settingsSizeOfVersion(version)) {
    return false;
  }

  /* Copy in only as far as that version's fields go, not as far as it wrote.
   * Anything this firmware added since keeps the default that settingsDefaults
   * just put there. See settingsFieldEndOfVersion for why the two differ. */
  size_t copy = settingsFieldEndOfVersion(version);
  if (copy > len) {
    copy = len;
  }
  if (copy > sizeof(Settings)) {
    copy = sizeof(Settings);
  }
  memcpy(out, blob, copy);
  out->version = SETTINGS_VERSION;
  out->size = (uint16_t)sizeof(Settings);

  /* Version 30 kept auto off in one byte. Carried across so a radio with a
   * version 30 blob keeps its time. */
  if (version == 30) {
    out->autoOffMinutes =
        autoOffMinutesOk(out->autoOffMinutesV30) ? out->autoOffMinutesV30 : 0;
  }

  /* Up to version 12 the tuning mode belonged to each band. Carry across the
   * one for the band the radio was left on, so a radio that upgrades comes
   * back doing what it was doing rather than dropping to manual. */
  if (version < 13) {
    const uint8_t was =
        out->startBand < BAND_COUNT ? out->bandTuneModeV12[out->startBand] : 0;
    out->tuneMode = was < (uint8_t)TUNE_MODE_COUNT ? was : 0;
  }

  /* Up to version 19 the AM blanker shipped off, so a stored 0 is the old
   * default far more often than a choice. Version 20 moves it to the new
   * one; a person who wants it off turns it off once more. */
  if (version < 20 && out->amNoiseBlankerStart == 0) {
    out->amNoiseBlankerStart = 100;
  }

  /* Up to version 20 `theme` counted ten themes that no longer ship.
   * Version 21 sends every one of them, Custom included, to Nightwatch. The
   * Custom colours themselves are kept, so picking Custom again brings them
   * back. */
  if (version < 21) {
    out->theme = 0;
  }

  /* Up to version 26 there was one theme. It is kept at night too, so a
   * radio that upgrades looks the same at every hour until a night theme is
   * chosen. */
  if (version < 27) {
    out->nightTheme = out->theme;
  }

  if (!settingsValid(out)) {
    settingsDefaults(out);
    return false;
  }
  return true;
}

bool settingsHasWifi(const Settings *s) {
  return s->wifiSsid[0] != '\0';
}

bool settingsSetWifi(Settings *s, const char *ssid, const char *pass) {
  if (ssid == NULL) {
    return false;
  }
  if (pass == NULL) {
    pass = "";
  }
  if (strlen(ssid) >= SETTINGS_SSID_LEN || strlen(pass) >= SETTINGS_PASS_LEN) {
    return false;
  }
  memset(s->wifiSsid, 0, SETTINGS_SSID_LEN);
  memset(s->wifiPass, 0, SETTINGS_PASS_LEN);
  memcpy(s->wifiSsid, ssid, strlen(ssid));
  memcpy(s->wifiPass, pass, strlen(pass));
  if (s->hotspot == WIFI_HOTSPOT_ON) {
    s->hotspot = WIFI_HOTSPOT_AUTO;
  }
  return true;
}
