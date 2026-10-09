#include "settings_table.h"

#include <string.h>

#include "agc.h"
#include "auto_off.h"
#include "backlight.h"
#include "band_plan.h"
#include "battery.h"
#include "dx_scan.h"
#include "input.h"
#include "memory.h"
#include "rds_country.h"
#include "seek.h"
#include "squelch.h"
#include "wifi_join.h"

/* The width and place of a field come from the struct itself, so a field
 * that is widened cannot be written at its old width. */
#define FIELD(name) offsetof(Settings, name), sizeof(((Settings *)0)->name)
#define ROW(key, name, low, high, acts) {key, FIELD(name), low, high, acts}

#define NOW SETTING_ACTS_NOW
#define AT_START SETTING_ACTS_AT_START
#define NEXT_DX SETTING_ACTS_NEXT_DX

/* In the order the API lists them when it is given none. */
static const SettingRow kRows[] = {
    ROW("rgn", fmRegion, 0, FM_REGION_COUNT - 1, AT_START),
    ROW("spc", mwSpacing, 0, MW_SPACING_10K, AT_START),
    ROW("enc", encoderKind, 0, ENCODER_OPTICAL, AT_START),
    ROW("edr", encoderDirection, 0, ENCODER_REVERSED, AT_START),
    ROW("fsn", fmScanSensitivity, SEEK_SENSITIVITY_MIN, SEEK_SENSITIVITY_MAX,
        NOW),
    ROW("asn", amScanSensitivity, SEEK_SENSITIVITY_MIN, SEEK_SENSITIVITY_MAX,
        NOW),
    ROW("smu", softMuteMs, 0, 500, NOW),
    ROW("bpk", beepKey, 0, BEEP_MODE_COUNT - 1, NOW),
    ROW("bpe", beepEdge, 0, 1, NOW),
    ROW("bps", beepStart, 0, 1, AT_START),
    ROW("sqf", fmSquelchFloor, 0, SQUELCH_FM_LEVEL_FLOOR_MAX_DBUV, NOW),
    ROW("blt", backlightPercent, BACKLIGHT_MIN_AWAKE, 100, NOW),
    ROW("bdm", backlightDimPercent, 0, 100, NOW),
    ROW("bds", backlightDimAfterS, 0, BACKLIGHT_DIM_AFTER_MAX_S, NOW),
    ROW("rds", rdsEnabled, 0, 1, NOW),
    ROW("ntp", ntpEnabled, 0, 1, NOW),
    ROW("bat", batteryShow, 0, BATTERY_SHOW_COUNT - 1, NOW),
    ROW("agt", agcTargetPercent, 0, AGC_TARGET_MAX, NOW),
    ROW("agb", agcBoostDb, 0, AGC_BOOST_MAX, NOW),
    ROW("thm", theme, 0, UINT8_MAX, NOW),
    ROW("thn", nightTheme, 0, UINT8_MAX, NOW),
    /* The outer bound; only 0 and 180 are legal, and the caller refuses
     * what lies between. */
    ROW("rot", displayRotation, 0, 180, NOW),
    ROW("dst", dxStopRule, 0, DX_STOP_COUNT - 1, NOW),
    ROW("dsc", dxScanRange, 0, DX_RANGE_COUNT - 1, NEXT_DX),
    ROW("dmf", dxMemFirst, 1, MEMORY_SLOT_COUNT, NEXT_DX),
    ROW("dml", dxMemLast, 1, MEMORY_SLOT_COUNT, NEXT_DX),
    ROW("dlp", dxLoop, 0, 1, NOW),
    ROW("dmu", dxScanMute, 0, 1, NOW),
    ROW("dal", dxAutoLog, 0, 1, NOW),
    ROW("ddw", dxDwellTenths, DX_SCAN_DWELL_MIN_TENTHS,
        DX_SCAN_DWELL_MAX_TENTHS, NOW),
    /* The outer bound; the caller refuses a value that is not one of the
     * tuner's widths. */
    ROW("dbw", dxWidthKHz, 1, UINT16_MAX, NEXT_DX),
    ROW("rrg", rdsRegion, 0, RDS_REGION_COUNT - 1, NOW),
    ROW("drt", dxLogRt, 0, 1, NOW),
    ROW("dwt", dxWatch, 0, 1, NOW),
    ROW("hsp", hotspot, 0, WIFI_HOTSPOT_COUNT - 1, NOW),
    ROW("web", webEnabled, 0, 1, NOW),
    ROW("wif", wifiEnabled, 0, 1, NOW),
    ROW("slp", autoOffMinutes, 0, AUTO_OFF_MAX_MINUTES, NOW),
    ROW("upc", updateCheck, 0, 1, NOW),
    ROW("tof", touchOff, 0, 1, NOW),
    ROW("kpt", keypadTimeoutS, SETTINGS_KEYPAD_TIMEOUT_MIN_S,
        SETTINGS_KEYPAD_TIMEOUT_MAX_S, NOW),
    ROW("pcl", pcLink, 0, 1, NOW),
};

size_t settingsTableCount(void) {
  return sizeof(kRows) / sizeof(kRows[0]);
}

const SettingRow *settingsTableAt(size_t i) {
  return i < settingsTableCount() ? &kRows[i] : NULL;
}

const SettingRow *settingsTableFind(const char *key) {
  if (key == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < settingsTableCount(); i++) {
    if (strcmp(kRows[i].key, key) == 0) {
      return &kRows[i];
    }
  }
  return NULL;
}

int32_t settingsTableGet(const Settings *s, const SettingRow *row) {
  if (s == NULL || row == NULL) {
    return 0;
  }
  const uint8_t *at = (const uint8_t *)s + row->offset;
  if (row->size == 2) {
    uint16_t v;
    memcpy(&v, at, sizeof(v));
    return (int32_t)v;
  }
  return (int32_t)at[0];
}

void settingsTableSet(Settings *s, const SettingRow *row, int32_t value) {
  if (s == NULL || row == NULL) {
    return;
  }
  uint8_t *at = (uint8_t *)s + row->offset;
  if (row->size == 2) {
    const uint16_t v = (uint16_t)value;
    memcpy(at, &v, sizeof(v));
    return;
  }
  at[0] = (uint8_t)value;
}
