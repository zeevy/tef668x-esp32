/**
 * @file settings.c
 * @brief Implementation of the settings struct, its defaults and migration.
 */
#include "settings.h"

#include <string.h>

/* For the ranges below. A setting that names a band, an encoder or a squelch
 * mode is checked against that enum and not against a number written here,
 * because a number written here goes wrong silently the day the enum gains a
 * member. */
#include "band_plan.h"
#include "input.h"
#include "radio.h"
#include "squelch.h"

/**
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
      return (uint16_t)sizeof(Settings);
    default:
      return 0;
  }
}

/** A blend start level: off, or high enough that a signal reaches it. */
static bool startLevelOk(uint8_t level) {
  return level == 0 || (level >= 20 && level <= 60);
}

/** A noise blanker percentage: off, or inside what the chip uses. */
static bool blankerOk(uint8_t percent) {
  return percent == 0 || (percent >= 50 && percent <= 150);
}

/** True when a char array holds a terminated string inside its own bounds. */
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
   * one of these is a setting that is switched on and does nothing, which is
   * the failure this project keeps finding, so it is refused here rather than
   * in whichever caller happens to exist. */
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

  /* Copy in only as much as both sides agree exists. Anything this firmware
   * added since keeps the default that settingsDefaults just wrote. */
  size_t copy = len < sizeof(Settings) ? len : sizeof(Settings);
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
