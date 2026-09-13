/* Implementation of the settings struct, its defaults and migration. */
#include "settings.h"

#include <stddef.h>
#include <string.h>

/* For the ranges below. A setting that names a band, an encoder or a squelch
 * mode is checked against that enum and not against a number written here,
 * because a number written here goes wrong silently the day the enum gains a
 * member. */
#include "backlight.h"
#include "band_plan.h"
#include "input.h"
#include "radio.h"
#include "seek.h"
#include "squelch.h"

/*
 * How many bytes each shipped version of the struct took.
 *
 * When SETTINGS_VERSION goes to 2, the entry for 1 becomes the number this
 * sizeof produces today, written out by hand, and 2 gets the sizeof. That is
 * the whole migration mechanism: a version is a known length, so a blob that
 * is not that length is corrupt rather than old.
 */
static uint16_t settingsSizeOfVersion(uint16_t version) {
  switch (version) {
    case 1:
      /* Written out by hand, because sizeof(Settings) is version 2 now. A
       * radio in the field holding a version 1 blob wrote exactly this many
       * bytes, and that never changes again. */
      return 108;
    case 2:
      /* Written out by hand for the same reason version 1 is: sizeof is
       * version 3 now, and a radio in the field holding a version 2 blob
       * wrote exactly this many bytes. */
      return 132;
    case 3:
      /* Written out by hand, like 1 and 2. A radio in the field holding a
       * version 3 blob wrote exactly this many bytes. */
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
   * them is either the reference firmware's or something measured on this
   * radio and written down in HARDWARE.md. None is a preference. */
  s->fmRegion = (uint8_t)FM_REGION_WORLD; /* 87.5 to 108. */
  s->mwSpacing = (uint8_t)MW_SPACING_9K;  /* Right here. */
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
  /* 104.0 MHz. Where this radio parked before there was a setting for it,
   * kept so that a radio updated to this build comes up where it used to.
   * The bottom of the band would be a change nobody asked for. */
  s->startFreqKHz = 104000;

  s->fmMultipathSuppression = 0;
  s->fmEqualizer = 0;
  s->fmForcedMono = 0;
  s->fmHighCutStart = 0; /* Off, as the reference ships. */
  s->fmStereoBlendStart = 0;
  s->fmStHiBlendStart = 0;
  s->fmNoiseBlankerStart = 0;
  s->fmDeemphasisUs = 50; /* 75 in the Americas. Audible either way. */

  s->amNoiseBlankerStart = 0;
  s->amBandwidthKHz = 4; /* Measured clearest on 738 kHz. */

  /* Version 3. The reference firmware's default, and the one the sweep in
   * test/fixtures/seek/ shows stopping on every station that was on air and
   * on nothing else. */
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

  /* Version 6. On, unlike the other beeps. See decision 27. */
  s->beepStart = 1;

  /* Version 7. The panel full on, the fade at boot on, the dim off. See
   * decision 28: a fade is only noticed when it is missing, and a panel that
   * goes dark on its own reads as a fault to anybody who did not ask for
   * it. */
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
    s->bandTuneMode[i] = 0;
  }

  /* Version 9. Measured on this radio, and the working is in
   * test/fixtures/squelch/README.md. */
  s->fmSquelchFloor = SQUELCH_FM_LEVEL_FLOOR_DBUV;
  /* On, because a radio that decodes RDS and does not show it is not what
   * anybody expects, and the cost is a third of a millisecond of tuner bus
   * traffic every 43 ms. */
  s->rdsEnabled = 1;
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
  /* The per band tuning modes. The step and the width are not checked here:
   * what they have to be inside is the band plan, which is a different
   * setting, and radioApply judges each against the band it belongs to and
   * falls back to that band's default. A mode is different, because it is an
   * enum and a value outside it would index nothing. */
  for (size_t i = 0; i < BAND_COUNT; i++) {
    if (s->bandTuneMode[i] >= (uint8_t)TUNE_MODE_COUNT) {
      return false;
    }
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
  return true;
}
