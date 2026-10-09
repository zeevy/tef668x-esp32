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
#include "settings_table.h"
#include "signal.h"
#include "squelch.h"
#include "wifi_join.h"

/*
 * How many bytes each shipped version of the struct took, by version.
 *
 * A version is a known length, so a blob that is not that length is corrupt
 * rather than old. That is the whole migration mechanism. The size a version
 * wrote is fixed once it ships and cannot be asked of the struct as it is
 * today, so every version but the newest is a number written out by hand.
 * When SETTINGS_VERSION goes up, the newest row becomes the number its
 * sizeof gave. Two versions of the same size are a field that went into
 * padding the older one already had; the version field tells them apart.
 */
static const uint16_t kSizeOfVersion[SETTINGS_VERSION + 1] = {
    [1] = 108,
    [2] = 132,
    [3] = 136,
    [4] = 140,
    [5] = 144,
    [6] = 144,
    [7] = 148,
    [8] = 196,
    [9] = 196,
    [10] = 196,
    [11] = 200,
    [12] = 200,
    [13] = 200,
    [14] = 204,
    [15] = 204,
    [16] = 208,
    [17] = 256,
    /* Version 18 cut `customTheme` from sixteen rows to thirteen. */
    [18] = 248,
    [19] = 248,
    [20] = 252,
    [21] = 252,
    [22] = 264,
    [23] = 264,
    [24] = 264,
    [25] = 268,
    [26] = 268,
    [27] = 268,
    [28] = 272,
    [29] = 272,
    [30] = 272,
    [31] = 276,
    [32] = (uint16_t)sizeof(Settings)};

/*
 * Where the fields of each version end, which is not the same as its size.
 *
 * A struct's trailing padding belongs to no field, and a field added later
 * can land inside it. Copying an old blob by its written length would take
 * that new field out of the old padding. So an old blob is copied only up to
 * the first field its version did not have, and every field after it keeps
 * the default settingsDefaults put there. The padding is zero on every radio,
 * because the struct is memset before it is filled, but "it happens to be
 * zero" is not a reason. offsetof keeps this table from drifting from the
 * struct. A row marked "padding" is a field that landed in the padding of
 * the version before it.
 */
static const size_t kFieldEndOfVersion[SETTINGS_VERSION + 1] = {
    [1] = offsetof(Settings, fmRegion),
    [2] = offsetof(Settings, fmScanSensitivity),
    [3] = offsetof(Settings, potRawMin), /* padding */
    [4] = offsetof(Settings, softMuteMs),
    [5] = offsetof(Settings, beepStart),
    [6] = offsetof(Settings, backlightPercent), /* padding */
    [7] = offsetof(Settings, bandFreqKHz),
    [8] = offsetof(Settings, fmSquelchFloor),
    [9] = offsetof(Settings, rdsEnabled),   /* padding */
    [10] = offsetof(Settings, ntpEnabled),  /* padding */
    [11] = offsetof(Settings, batteryShow), /* padding */
    [12] = offsetof(Settings, tuneMode),    /* padding */
    [13] = offsetof(Settings, agcTargetPercent),
    /* Padding. Both meter segment fields have a minimum of 1, so copying
     * version 14 by its length would bring in two zeros, settingsValid would
     * refuse the struct, and the radio would come up on defaults without its
     * PIN, its station and its calibration. */
    [14] = offsetof(Settings, meterSegW),
    [15] = offsetof(Settings, sigFullFmDbuV),
    [16] = offsetof(Settings, theme),
    /* Version 18 renamed and moved the theme rows, so a version 17 `theme`
     * and `customTheme` mean something else today and keep their defaults. */
    [17] = offsetof(Settings, theme),
    /* Padding. Settings is 248 bytes in both version 18 and 19, so the
     * length alone cannot tell the two apart. */
    [18] = offsetof(Settings, displayRotation),
    [19] = offsetof(Settings, amHighCutStart), /* padding */
    [20] = offsetof(Settings, dxStopRule),     /* padding */
    [21] = offsetof(Settings, dxStopRule),     /* padding */
    [22] = offsetof(Settings, rdsRegion),      /* padding */
    [23] = offsetof(Settings, dxLogRt),        /* padding */
    [24] = offsetof(Settings, dxWatch),
    [25] = offsetof(Settings, levelOffsetFmDb), /* padding */
    [26] = offsetof(Settings, nightTheme),      /* padding */
    [27] = offsetof(Settings, hotspot),
    [28] = offsetof(Settings, webEnabled),        /* padding */
    [29] = offsetof(Settings, autoOffMinutesV30), /* padding */
    [30] = offsetof(Settings, autoOffMinutes),
    [31] = offsetof(Settings, keypadTimeoutS),
    [32] = sizeof(Settings)};

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
  /* Off: a radio looks on GitHub only when its owner asks. */
  s->updateCheck = 0;
  /* Touch on, since every ATS-125 has the touch screen fitted. */
  s->touchOff = 0;
  /* Version 32. */
  s->keypadTimeoutS = SETTINGS_KEYPAD_TIMEOUT_DEFAULT_S;
  /* Off: nothing listens for a PC until its owner asks. */
  s->pcLink = 0;
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

  /* Every range here is the hardware's, not a preference. A value outside
   * one is a setting that is switched on and does nothing, so it is refused
   * here rather than in whichever caller happens to exist.
   *
   * First every setting the table holds, against the same range the API and
   * the menu offer. Checked from the one table, so the menu can never store a
   * value this refuses, which would make every save after it fail. */
  for (size_t i = 0; i < settingsTableCount(); i++) {
    const SettingRow *row = settingsTableAt(i);
    const int32_t v = settingsTableGet(s, row);
    if (v < row->low || v > row->high) {
      return false;
    }
  }

  /* Then what the table cannot say: the fields it does not hold, and the
   * legal sets with a hole in them. The frequency is not checked here,
   * because what it has to be inside is the band plan, and that is a
   * different setting. radioFromSettings judges it against the plan and
   * falls back to the band's own start. */
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
  if (s->dxMemFirst > s->dxMemLast ||
      !bandBandwidthAllowed(BAND_FM, s->dxWidthKHz)) {
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
  /* The tuning mode. The step and the width are not checked here: what they
   * have to be inside is the band plan, which is a different setting, and
   * radioApply judges each against the band it belongs to and falls back to
   * that band's default. A mode is different, because it is an enum and a
   * value outside it would index nothing. */
  if (s->tuneMode >= (uint8_t)TUNE_MODE_COUNT) {
    return false;
  }
  /* The AGC target is zero or a real target, never in between: a number
   * below AGC_TARGET_MIN is not a quieter setting, it is one nothing
   * sensible can be done with. */
  if (s->agcTargetPercent != 0 && s->agcTargetPercent < AGC_TARGET_MIN) {
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

/* Fields this firmware does not read, put back on every load to what this
 * firmware does, and stored so with the next save, so an older one that
 * reads them after a rollback does the same: no level offset, and the panel
 * faded up at start. */
static void keepUnused(Settings *s) {
  s->levelOffsetFmDb = 0;
  s->levelOffsetAmDb = 0;
  s->backlightFade = 1;
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

  if (version == 0) {
    return false;
  }
  if (version > SETTINGS_VERSION) {
    /*
     * Written by a newer firmware, which is what an update that rolled back
     * leaves. A newer version only adds fields after these and never changes
     * an old one, so its first sizeof(Settings) bytes are this firmware's own
     * struct. Refusing it would bring the radio up on the defaults, its Wi-Fi
     * details and PIN gone, after a rollback that is meant to cost nothing.
     * Shorter than this struct, or with a size that is not its length, it is
     * corrupt rather than newer.
     */
    if (storedSize != len || len < sizeof(Settings)) {
      return false;
    }
    memcpy(out, blob, sizeof(Settings));
    out->version = SETTINGS_VERSION;
    out->size = (uint16_t)sizeof(Settings);
    if (out->updateCheck > 1) {
      out->updateCheck = 0;
    }
    if (out->touchOff > 1) {
      out->touchOff = 0;
    }
    if (out->pcLink > 1) {
      out->pcLink = 0;
    }
    keepUnused(out);
    if (!settingsValid(out)) {
      settingsDefaults(out);
      return false;
    }
    return true;
  }

  /* A blob has to be exactly the size the firmware that wrote it used. The
   * size field alone is not enough, because a corrupt blob can declare a
   * small size that matches its own truncated length and then read back as a
   * valid one character SSID. */
  if (storedSize != len || storedSize != kSizeOfVersion[version]) {
    return false;
  }

  /* Copy in only as far as that version's fields go, not as far as it wrote.
   * Anything this firmware added since keeps the default that settingsDefaults
   * just put there. See kFieldEndOfVersion for why the two differ. */
  size_t copy = kFieldEndOfVersion[version];
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

  /* The update check and the touch switch live in version 31's padding.
   * Anything but 1 there is the default, so a stray byte costs the setting
   * and not the whole struct. */
  if (out->updateCheck > 1) {
    out->updateCheck = 0;
  }
  if (out->touchOff > 1) {
    out->touchOff = 0;
  }
  /* The PC Link lives in version 32's padding, the same way. */
  if (out->pcLink > 1) {
    out->pcLink = 0;
  }
  keepUnused(out);

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
