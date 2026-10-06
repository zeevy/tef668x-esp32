/* The menu, put together. The cursor is in core, the pixels are in ui. */
#include "menu_task.h"

#include <Arduino.h>
#include <esp_flash.h>
#include <stdio.h>
#include <string.h>

#include "band_scan_task.h"
#include "build_id.h"
#include "core/access_pin.h"
#include "core/agc.h"
#include "core/auto_off.h"
#include "core/backlight.h"
#include "core/band_plan.h"
#include "core/battery.h"
#include "core/clock.h"
#include "core/cpu_load.h"
#include "core/dx_scan.h"
#include "core/input.h"
#include "core/logbook.h"
#include "core/memory.h"
#include "core/menu.h"
#include "core/palette.h"
#include "core/radio.h"
#include "core/rds_country.h"
#include "core/settings_table.h"
#include "core/squelch.h"
#include "core/strings.h"
#include "core/update_check.h"
#include "core/version.h"
#include "drivers/battery_adc.h"
#include "drivers/device_id.h"
#include "drivers/logbook_fs.h"
#include "drivers/settings_nvs.h"
#include "drivers/tef668x.h"
#include "dx_task.h"
#include "input_task.h"
#include "lvgl_port.h"
#include "memory_store.h"
#include "net/ntp.h"
#include "net/restart_reason.h"
#include "net/rollback.h"
#include "net/update_check.h"
#include "net/web_update.h"
#include "net/wifi_manager.h"
#include "radio_task.h"
#include "screen_task.h"
#include "settings_task.h"
#include "sleep_task.h"
#include "system_info.h"
#include "ui/draw.h"
#include "ui/screen.h"
#include "ui/theme.h"

/*
 * Where a row's value actually lives.
 *
 * The two that matter are STORED and RADIO, and getting one wrong is not a
 * small bug. The radio task owns what it is tuned to and how it is handling
 * the audio, and the automatic save copies that into the settings ten seconds
 * after it stops moving. So a radio setting written into the settings struct
 * would appear to work and then be overwritten by the radio's own value: a
 * row that undoes itself, with nothing anywhere saying why.
 */
typedef enum {
  SRC_STORED = 0, /* The settings struct. Written through settingsApplyLive. */
  SRC_RADIO,      /* The radio task. Written by posting a command. */
  SRC_ACTION,     /* Does something when pressed. */
  SRC_INFO,       /* Read only. */
} RowSource;

/* Which setting, which reading, or which action. One name each. */
typedef enum {
  ROW_FM_REGION = 0,
  ROW_FM_SEEK,
  ROW_FM_STEP,
  ROW_MW_SPACING,
  ROW_LW_WIDTH,
  ROW_MW_WIDTH,
  ROW_SW_WIDTH,
  ROW_AM_SEEK,
  ROW_LW_STEP,
  ROW_MW_STEP,
  ROW_SW_STEP,
  ROW_SQUELCH_FLOOR,
  ROW_MUTE_RAMP,
  ROW_AGC_TARGET,
  ROW_AGC_BOOST,
  ROW_RDS,
  ROW_RDS_REGION,
  ROW_BACKLIGHT,
  ROW_BACKLIGHT_DIM,
  ROW_DIM_AFTER,
  ROW_FADE_AT_START,
  ROW_BATTERY,
  ROW_THEME,
  ROW_NIGHT_THEME,
  ROW_DISPLAY_ROTATION,
  ROW_LEVEL_OFFSET_FM,
  ROW_LEVEL_OFFSET_AM,
  ROW_CHIME,
  ROW_KEY_BEEPS,
  ROW_EDGE_BEEP,
  ROW_TOUCH,
  ROW_CALIBRATE_TOUCH,
  ROW_NETWORK_TIME,
  ROW_WEB_PIN,
  ROW_HOTSPOT,
  ROW_WEB_SERVER,
  ROW_WIFI,
  ROW_ENCODER,
  ROW_ENCODER_DIR,

  ROW_DEEMPHASIS,
  ROW_IMS,
  ROW_EQ,
  ROW_MONO,
  ROW_HIGH_CUT,
  ROW_STEREO_BLEND,
  ROW_STHI_BLEND,
  ROW_FM_BLANKER,
  ROW_AM_BLANKER,
  ROW_AM_HIGH_CUT,
  ROW_LW_HIGH_CUT,
  ROW_AM_SOFT_MUTE,
  ROW_LW_SOFT_MUTE,
  ROW_SQUELCH_MODE,
  ROW_SQUELCH_LEVEL,
  ROW_FM_SCAN,
  ROW_MW_SCAN,
  ROW_SW_SCAN,
  ROW_LW_SCAN,

  ROW_DX_DWELL,
  ROW_DX_STOP,
  ROW_DX_RANGE,
  ROW_DX_MEM_FIRST,
  ROW_DX_MEM_LAST,
  ROW_DX_LOOP,
  ROW_DX_WIDTH,
  ROW_DX_MUTE,
  ROW_DX_AUTOLOG,
  ROW_DX_LOG_RT,
  ROW_DX_WATCH,
  ROW_DX_LEARN,

  ROW_GOTO_BAND,
  ROW_GOTO_BANDWIDTH,
  ROW_GOTO_RDS,
  ROW_GOTO_DX,
  ROW_GOTO_LOG,
  ROW_GOTO_SLEEP,
  ROW_PRESET_ENTRY,
  ROW_AUTO_OFF,
  ROW_UPDATE_CHECK,
  ROW_UPDATE_INSTALL,
  ROW_RESTART,

  ROW_NET_STATE,
  ROW_NET_ADDRESS,
  ROW_NET_NAME,
  ROW_NET_MAC,
  ROW_ABOUT_VERSION,
  ROW_ABOUT_SLOT,
  ROW_ABOUT_UPTIME,
  ROW_ABOUT_TUNER,
  ROW_ABOUT_DEVELOPER,
  ROW_ABOUT_GITHUB,
  ROW_ABOUT_BUILD,
  ROW_ABOUT_LICENSE,
  ROW_NET_SIGNAL,
  ROW_SYSTEM_RESET,
  ROW_SYSTEM_CPU_0,
  ROW_SYSTEM_CPU_1,
  ROW_SYSTEM_HEAP,
  ROW_SYSTEM_HEAP_LOWEST,
  ROW_SYSTEM_HEAP_BLOCK,
  ROW_SYSTEM_LVGL,
  ROW_SYSTEM_CHIP,
  ROW_SYSTEM_FLASH,
  ROW_DX_START_SCAN,
  ROW_SYSTEM_BATTERY,
  ROW_NET_IP,
  /* One entry of the Station Log. Every entry shares this one row; which
   * entry it is comes from its place in the list. */
  ROW_LOG_ENTRY,
  /* A row that opens a sub-group. Never read or written: what it shows is
   * how many rows the sub-group holds. */
  ROW_SUB,
} RowId;

typedef struct MenuGroup MenuGroup;

typedef struct {
  StrId name;
  RowId id;
  RowSource source;
  int32_t min;
  int32_t max;
  int32_t step;
  /*
   * The values this row is allowed to take, when they are not a plain range.
   *
   * When this is set the knob walks the list by index and never lands between
   * two legal values. That is not tidiness. `settingsValid` accepts 0 or 20
   * to 60 for a start level, 0 or 50 to 150 for a blanker, and 3, 4, 6 or 8
   * for the AM width, and a menu that offers anything else writes a settings
   * blob the validator then refuses **on every automatic save from then on**:
   * not just that setting, the frequency and the band and everything else,
   * silently, until somebody moves the value back.
   */
  const uint8_t *choices;
  uint8_t choiceCount;
  /*
   * Whether the value screen draws the bar.
   *
   * Said here rather than worked out from whether the row has a unit. A list
   * of words has no distance between its entries, and `off, keys, keys and
   * long, every press` would otherwise get a bar because it happens to be
   * four numbers.
   */
  bool bar;
  bool needsRestart; /* Nothing happens until the radio is restarted. */
  /*
   * Ask before doing it.
   *
   * Drawn as a value of no or yes rather than as a screen of its own, so the
   * confirmation costs no new state and works like every other row: turn to
   * yes, press to do it. Only on a row that cannot be undone. A
   * confirmation on every action is one nobody reads.
   */
  bool confirm;
  /* The sub-group this row opens, or NULL for a row that is a value, an
   * action or a reading. */
  const MenuGroup *opens;
} MenuRow;

struct MenuGroup {
  StrId name;
  const MenuRow *rows;
  uint8_t count;
};

/*
 * The values `settingsValid` will accept, for the rows whose legal set has a
 * hole in it. Written out rather than worked out, so the table can be read
 * against `startLevelOk` and `blankerOk` in core/settings.c line by line.
 */
static const uint8_t kStartLevels[] = {
    0,  20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60};
static const uint8_t kBlankers[] = {0,   50,  55,  60,  65,  70,  75,  80,
                                    85,  90,  95,  100, 105, 110, 115, 120,
                                    125, 130, 135, 140, 145, 150};
static const uint8_t kAmWidths[] = {3, 4, 6, 8};

/*
 * The AGC targets, which are 0 or 30 to 80.
 *
 * The same shape as the start levels: `settingsValid` refuses everything
 * between, because a target below AGC_TARGET_MIN is not a quieter setting, it
 * is a number the AGC cannot do anything sensible with.
 */
static const uint8_t kAgcTargets[] = {0,  30, 32, 34, 36, 38, 40, 42, 44,
                                      46, 48, 50, 52, 54, 56, 58, 60, 62,
                                      64, 66, 68, 70, 72, 74, 76, 78, 80};

#define LIST(a) a, (uint8_t)(sizeof(a) / sizeof((a)[0]))
#define NOLIST NULL, 0
/* For a row that is one stored setting: its ends come from the settings
 * table, through rowMin and rowMax, so the menu cannot offer a range the API
 * refuses. */
#define TABLE_RANGE 0, 0

#define GROUP(name, rows) \
  {name, rows, (uint8_t)(sizeof(rows) / sizeof(MenuRow))}

/* A row that opens `group`, drawn with its count and a chevron. */
#define SUB(name, group) \
  {name, ROW_SUB, SRC_INFO, 0, 0, 1, NOLIST, false, false, false, &group}

/*
 * The screens and actions the panel keys reach, so the knob alone reaches
 * them too. Each closes the menu and calls what its key calls: BAND, BW held,
 * BAND held, the DX key and ENTER held. Sleep has no key.
 */
static const MenuRow kGotoRows[] = {
    {STR_MENU_GO_TO_BAND, ROW_GOTO_BAND, SRC_ACTION, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_GO_TO_BANDWIDTH, ROW_GOTO_BANDWIDTH, SRC_ACTION, 0, 0, 1, NOLIST,
     false, false, false},
    {STR_MENU_GO_TO_RDS, ROW_GOTO_RDS, SRC_ACTION, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_GO_TO_DX, ROW_GOTO_DX, SRC_ACTION, 0, 0, 1, NOLIST, false, false,
     false},
    {STR_MENU_GO_TO_LOG, ROW_GOTO_LOG, SRC_ACTION, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_GO_TO_SLEEP, ROW_GOTO_SLEEP, SRC_ACTION, 0, 0, 1, NOLIST, false,
     false, false},
};

/*
 * One row a saved preset, in slot order. The rows are not a table: every one
 * is this same row, its words read from the store when it is drawn, and the
 * count is taken again before every use, the same as the Station Log's.
 */
static const MenuRow kPresetEntryRow[] = {
    {STR_COMMON_DASH, ROW_PRESET_ENTRY, SRC_ACTION, 0, 0, 1, NOLIST, false,
     false, false},
};
static MenuGroup sPresetGroup = {STR_MENU_MEMORY, kPresetEntryRow, 0};

/*
 * The logbook, newest first; a press tunes to an entry's frequency. Its rows
 * are not a table: there can be up to 250 entries, so every one is the same
 * row, its words read from the logbook when it is drawn, and the count is the
 * logbook's own, taken again before every use.
 */
static const MenuRow kLogEntryRow[] = {
    {STR_COMMON_DASH, ROW_LOG_ENTRY, SRC_ACTION, 0, 0, 1, NOLIST, false, false,
     false},
};
static MenuGroup sLogGroup = {STR_MENU_STATION_LOG, kLogEntryRow, 0};

static const MenuRow kStationRows[] = {
    SUB(STR_MENU_MEMORY, sPresetGroup),
    /*
     * The station scans, one a band. Each walks its band, from another band
     * too, adding a memory channel for whatever passes the seek's stop
     * decision at the Seek Sensitivity setting, and is not already stored
     * nearby, through the same call
     * `POST /api/scan` makes. Its own value shows progress while it runs,
     * `textOf`'s scan case, since a scan takes from ten seconds to four
     * minutes and a row that just sat there would read as a radio that had
     * hung.
     */
    {STR_MENU_SCAN_FM_FOR_STATIONS, ROW_FM_SCAN, SRC_ACTION, 0, 0, 1, NOLIST,
     false, false, false},
    {STR_MENU_SCAN_MW, ROW_MW_SCAN, SRC_ACTION, 0, 0, 1, NOLIST, false, false,
     false},
    {STR_MENU_SCAN_SW, ROW_SW_SCAN, SRC_ACTION, 0, 0, 1, NOLIST, false, false,
     false},
    {STR_MENU_SCAN_LW, ROW_LW_SCAN, SRC_ACTION, 0, 0, 1, NOLIST, false, false,
     false},
    SUB(STR_MENU_STATION_LOG, sLogGroup),
};

static const MenuRow kSquelchRows[] = {
    {STR_MENU_SQUELCH, ROW_SQUELCH_MODE, SRC_RADIO, 0, 2, 1, NOLIST, false,
     false, false},
    /* Read only: in manual the knob is the squelch level, and a second way to
     * set one value would be a second owner, undone the moment the knob
     * moved. */
    {STR_MENU_SQUELCH_LEVEL, ROW_SQUELCH_LEVEL, SRC_INFO, 0, 0, 1, NOLIST,
     false, false, false},
    /* 40, which is SQUELCH_FM_LEVEL_FLOOR_MAX_DBUV. Anything above it is
     * refused by `settingsValid` and would stop every save. */
    {STR_MENU_SQUELCH_FLOOR, ROW_SQUELCH_FLOOR, SRC_STORED, TABLE_RANGE, 1,
     NOLIST, true, false, false},
};
static const MenuGroup kSquelchGroup =
    GROUP(STR_MENU_SQUELCH_GROUP, kSquelchRows);

static const MenuRow kAudioRows[] = {
    SUB(STR_MENU_SQUELCH_GROUP, kSquelchGroup),
    /*
     * The volume AGC. The target is the on switch: 0 is off and anything from
     * AGC_TARGET_MIN up is a target, with nothing in between, so the row
     * walks a list rather than a range.
     */
    {STR_MENU_VOLUME_AGC, ROW_AGC_TARGET, SRC_STORED, 0, 0, 1,
     LIST(kAgcTargets), true, false, false},
    {STR_MENU_AGC_BOOST, ROW_AGC_BOOST, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     true, false, false},
    {STR_MENU_MUTE_RAMP, ROW_MUTE_RAMP, SRC_STORED, TABLE_RANGE, 10, NOLIST,
     true, false, false},
};

static const MenuRow kStereoRows[] = {
    {STR_MENU_FORCED_MONO, ROW_MONO, SRC_RADIO, 0, 1, 1, NOLIST, false, false,
     false},
    {STR_MENU_STEREO_BLEND, ROW_STEREO_BLEND, SRC_RADIO, 0, 0, 1,
     LIST(kStartLevels), true, false, false},
    {STR_MENU_ST_HI_BLEND, ROW_STHI_BLEND, SRC_RADIO, 0, 0, 1,
     LIST(kStartLevels), true, false, false},
};
static const MenuGroup kStereoGroup = GROUP(STR_MENU_STEREO, kStereoRows);

static const MenuRow kRdsRows[] = {
    {STR_COMMON_RDS_DECODER, ROW_RDS, SRC_STORED, TABLE_RANGE, 1, NOLIST, false,
     false, false},
    {STR_MENU_RDS_REGION, ROW_RDS_REGION, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, false, false},
};
static const MenuGroup kRdsGroup = GROUP(STR_MENU_RDS_GROUP, kRdsRows);

static const MenuRow kFmRows[] = {
    {STR_MENU_BAND_PLAN, ROW_FM_REGION, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, true, false},
    /*
     * The radio's, not the settings'. `bandStepKHz` stores FM's step across a
     * power cycle, but `radioApply` reads and writes the tuned band's step as
     * `stepKHz`, not the array, so this row goes through the same snapshot
     * and command every other live row does. The choices are not a compile
     * time list: `rowChoices` walks `bandStepCount`/`bandStepAt` for BAND_FM,
     * so a step added in core/band_plan.c shows up here without this table
     * changing.
     *
     * `min`/`max` here are 0 and 1 rather than the usual 0 and 0 a listed row
     * carries, so that a snapshot that fails right as the row is pressed
     * still gives `menuPress` a real range to work with. A row stuck at 0
     * and 0 counts as nothing to edit, `core/menu.h`'s own rule for an inert
     * row, so a merely busy radio would refuse the press outright instead of
     * reaching the "radio busy, try again" note every other live row gives
     * for the same failure.
     */
    {STR_MENU_TUNING_STEP, ROW_FM_STEP, SRC_RADIO, 0, 1, 1, NOLIST, true, false,
     false},
    {STR_MENU_SEEK_SENSITIVITY, ROW_FM_SEEK, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     true, false, false},
    {STR_MENU_DE_EMPHASIS, ROW_DEEMPHASIS, SRC_RADIO, 0, 2, 1, NOLIST, false,
     false, false},
    SUB(STR_MENU_STEREO, kStereoGroup),
    {STR_MENU_HIGH_CUT, ROW_HIGH_CUT, SRC_RADIO, 0, 0, 1, LIST(kStartLevels),
     true, false, false},
    {STR_MENU_NOISE_BLANKER, ROW_FM_BLANKER, SRC_RADIO, 0, 0, 1,
     LIST(kBlankers), true, false, false},
    {STR_COMMON_IMS, ROW_IMS, SRC_RADIO, 0, 1, 1, NOLIST, false, false, false},
    {STR_MENU_EQUALISER, ROW_EQ, SRC_RADIO, 0, 1, 1, NOLIST, false, false,
     false},
    SUB(STR_MENU_RDS_GROUP, kRdsGroup),
};

/* AM high cut and soft mute. MW and SW share a pair and LW has its own,
 * because the NXP user manual gives LW its own values. Each is sent whenever
 * its band is tuned, so all four can be set from any band. */
static const MenuRow kAmHighCutRows[] = {
    {STR_MENU_MW_SW, ROW_AM_HIGH_CUT, SRC_RADIO, 0, 0, 1, LIST(kStartLevels),
     true, false, false},
    {STR_MENU_LW, ROW_LW_HIGH_CUT, SRC_RADIO, 0, 0, 1, LIST(kStartLevels), true,
     false, false},
};
static const MenuGroup kAmHighCutGroup =
    GROUP(STR_MENU_HIGH_CUT, kAmHighCutRows);

static const MenuRow kAmSoftMuteRows[] = {
    {STR_MENU_MW_SW, ROW_AM_SOFT_MUTE, SRC_RADIO, 0, 50, 1, NOLIST, true, false,
     false},
    {STR_MENU_LW, ROW_LW_SOFT_MUTE, SRC_RADIO, 0, 50, 1, NOLIST, true, false,
     false},
};
static const MenuGroup kAmSoftMuteGroup =
    GROUP(STR_MENU_SOFT_MUTE, kAmSoftMuteRows);

/*
 * Each AM band's own step, set from any band: the step the band comes up
 * with, and the live one when it is the band tuned. LW, MW and SW each allow
 * a different pair, so `rowChoices` asks `bandStepCount`/`bandStepAt` for the
 * row's band rather than a list fixed here. `min`/`max` are 0 and 1 for the
 * same reason as the FM row above.
 */
static const MenuRow kAmStepRows[] = {
    {STR_MENU_LW, ROW_LW_STEP, SRC_RADIO, 0, 1, 1, NOLIST, true, false, false},
    {STR_MENU_MW, ROW_MW_STEP, SRC_RADIO, 0, 1, 1, NOLIST, true, false, false},
    {STR_MENU_SW, ROW_SW_STEP, SRC_RADIO, 0, 1, 1, NOLIST, true, false, false},
};
static const MenuGroup kAmStepGroup = GROUP(STR_MENU_TUNING_STEP, kAmStepRows);

/*
 * Each AM band's own filter width, set from any band in the same way as its
 * step: the width the band comes up with, and the live one when it is the
 * band tuned. The radio's widths, not the stored `amBandwidthKHz`, which is
 * only the width a band starts on before it was ever tuned.
 */
static const MenuRow kAmWidthRows[] = {
    {STR_MENU_LW, ROW_LW_WIDTH, SRC_RADIO, 0, 0, 1, LIST(kAmWidths), true,
     false, false},
    {STR_MENU_MW, ROW_MW_WIDTH, SRC_RADIO, 0, 0, 1, LIST(kAmWidths), true,
     false, false},
    {STR_MENU_SW, ROW_SW_WIDTH, SRC_RADIO, 0, 0, 1, LIST(kAmWidths), true,
     false, false},
};
static const MenuGroup kAmWidthGroup =
    GROUP(STR_MENU_FILTER_WIDTH, kAmWidthRows);

static BandId widthBandOf(RowId id) {
  switch (id) {
    case ROW_LW_WIDTH:
      return BAND_LW;
    case ROW_MW_WIDTH:
      return BAND_MW;
    case ROW_SW_WIDTH:
      return BAND_SW;
    default:
      return BAND_COUNT;
  }
}

static const MenuRow kAmRows[] = {
    {STR_MENU_MW_SPACING, ROW_MW_SPACING, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, true, false},
    SUB(STR_MENU_TUNING_STEP, kAmStepGroup),
    {STR_MENU_SEEK_SENSITIVITY, ROW_AM_SEEK, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     true, false, false},
    SUB(STR_MENU_FILTER_WIDTH, kAmWidthGroup),
    {STR_MENU_NOISE_BLANKER, ROW_AM_BLANKER, SRC_RADIO, 0, 0, 1,
     LIST(kBlankers), true, false, false},
    SUB(STR_MENU_HIGH_CUT, kAmHighCutGroup),
    SUB(STR_MENU_SOFT_MUTE, kAmSoftMuteGroup),
};

static const MenuRow kPresetRangeRows[] = {
    {STR_MENU_MEMORY_FROM, ROW_DX_MEM_FIRST, SRC_STORED, 1, MEMORY_SLOT_COUNT,
     1, NOLIST, true, false, false},
    {STR_MENU_MEMORY_TO, ROW_DX_MEM_LAST, SRC_STORED, 1, MEMORY_SLOT_COUNT, 1,
     NOLIST, true, false, false},
};
static const MenuGroup kPresetRangeGroup =
    GROUP(STR_MENU_PRESET_RANGE, kPresetRangeRows);

/*
 * DX mode: the scan itself, then the scanner, the width and the sound, then
 * the log and the one pass of its own. The limits are settingsValid's. The
 * dwell is in tenths of a second, 0.5 s a click, and the width walks the
 * tuner's FM widths by their place in band_plan.c's list, 1 to 16, since 0
 * there is automatic and DX mode is never automatic.
 */
static const MenuRow kDxRows[] = {
    {STR_MENU_START_SCAN, ROW_DX_START_SCAN, SRC_ACTION, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_DWELL, ROW_DX_DWELL, SRC_STORED, TABLE_RANGE, 5, NOLIST, true,
     false, false},
    {STR_MENU_STOP_ON, ROW_DX_STOP, SRC_STORED, TABLE_RANGE, 1, NOLIST, false,
     false, false},
    {STR_MENU_SCAN, ROW_DX_RANGE, SRC_STORED, TABLE_RANGE, 1, NOLIST, false,
     false, false},
    SUB(STR_MENU_PRESET_RANGE, kPresetRangeGroup),
    {STR_MENU_DX_WIDTH, ROW_DX_WIDTH, SRC_STORED, 1, 16, 1, NOLIST, true, false,
     false},
    {STR_MENU_LOOP_THE_BAND, ROW_DX_LOOP, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, false, false},
    {STR_MENU_MUTE_WHILE_SCANNING, ROW_DX_MUTE, SRC_STORED, TABLE_RANGE, 1,
     NOLIST, false, false, false},
    {STR_MENU_AUTO_LOG_NEW, ROW_DX_AUTOLOG, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, false, false},
    {STR_MENU_LOG_RADIO_TEXT, ROW_DX_LOG_RT, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, false, false},
    {STR_MENU_WATCH_PRESETS, ROW_DX_WATCH, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, false, false},
    {STR_MENU_LEARN_LOCALS, ROW_DX_LEARN, SRC_ACTION, 0, 0, 1, NOLIST, false,
     false, false},
};

/* A place in the list of themes, walked like Band Plan and MW Step rather
 * than shown as a range: a colour scheme has no middle to sit between two of
 * them. The list is grouped by use, so the row's value is the place and the
 * setting is the saved index at it. Unlike Brightness it acts only when the
 * value is kept, `actsOnKeep`, through the rebuild screen.cpp already does
 * for a layout change. */
static const MenuRow kThemeRows[] = {
    {STR_MENU_DAY_THEME, ROW_THEME, SRC_STORED, 0, THEME_COUNT - 1, 1, NOLIST,
     false, false, false},
    {STR_MENU_NIGHT_THEME, ROW_NIGHT_THEME, SRC_STORED, 0, THEME_COUNT - 1, 1,
     NOLIST, false, false, false},
};
static const MenuGroup kThemeGroup = GROUP(STR_MENU_THEME, kThemeRows);

/* Added to every level shown or exported, so the readings can match another
 * receiver's; no threshold reads it. */
static const MenuRow kLevelOffsetRows[] = {
    {STR_MENU_LEVEL_OFFSET_FM, ROW_LEVEL_OFFSET_FM, SRC_STORED, TABLE_RANGE, 1,
     NOLIST, true, false, false},
    {STR_MENU_LEVEL_OFFSET_AM, ROW_LEVEL_OFFSET_AM, SRC_STORED, TABLE_RANGE, 1,
     NOLIST, true, false, false},
};
static const MenuGroup kLevelOffsetGroup =
    GROUP(STR_MENU_LEVEL_OFFSET, kLevelOffsetRows);

static const MenuRow kDisplayRows[] = {
    SUB(STR_MENU_THEME, kThemeGroup),
    {STR_MENU_BRIGHTNESS, ROW_BACKLIGHT, SRC_STORED, TABLE_RANGE, 5, NOLIST,
     true, false, false},
    {STR_MENU_DIM_LEVEL, ROW_BACKLIGHT_DIM, SRC_STORED, TABLE_RANGE, 5, NOLIST,
     true, false, false},
    {STR_MENU_DIM_AFTER, ROW_DIM_AFTER, SRC_STORED, TABLE_RANGE, 5, NOLIST,
     true, false, false},
    /* 0 for the board's own mount and 1 for 180 from it. Every recovery row
     * is also in normal settings, so it can be found without knowing any trick:
     * this is the same value the recovery screen's own "Rotate display" row
     * shows and changes, reachable here without ever holding the knob at power
     * on. It acts when kept, not as the knob turns, and the panel turns live
     * with no restart. */
    {STR_MENU_ROTATION, ROW_DISPLAY_ROTATION, SRC_STORED, 0, 1, 1, NOLIST,
     false, false, false},
    {STR_COMMON_BATTERY, ROW_BATTERY, SRC_STORED, TABLE_RANGE, 1, NOLIST, false,
     false, false},
    SUB(STR_MENU_LEVEL_OFFSET, kLevelOffsetGroup),
    {STR_MENU_FADE_AT_START, ROW_FADE_AT_START, SRC_STORED, TABLE_RANGE, 1,
     NOLIST, false, true, false},
};

/* Network Time's step below -12:00, which reads Off: the two settings,
 * network time on or off and the offset from UTC, on one bar. */
#define NETWORK_TIME_OFF (CLOCK_OFFSET_MIN_MINUTES - 15)

static const MenuRow kNetInfoRows[] = {
    {STR_MENU_STATUS, ROW_NET_STATE, SRC_INFO, 0, 0, 1, NOLIST, false, false,
     false},
    {STR_MENU_WEB_ADDRESS, ROW_NET_ADDRESS, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_IP_ADDRESS, ROW_NET_IP, SRC_INFO, 0, 0, 1, NOLIST, false, false,
     false},
    {STR_MENU_WI_FI_NAME, ROW_NET_NAME, SRC_INFO, 0, 0, 1, NOLIST, false, false,
     false},
    {STR_MENU_WI_FI_SIGNAL, ROW_NET_SIGNAL, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_MAC, ROW_NET_MAC, SRC_INFO, 0, 0, 1, NOLIST, false, false, false},
};
static const MenuGroup kNetInfoGroup =
    GROUP(STR_MENU_NETWORK_INFO, kNetInfoRows);

static const MenuRow kConnectRows[] = {
    /* Applied on the press, not as the knob turns, as are the next two: each
     * step would leave one network for another. Off, only these rows turn
     * Wi-Fi and the web server on again: the browser and the API go with
     * them. */
    {STR_MENU_WIFI, ROW_WIFI, SRC_STORED, TABLE_RANGE, 1, NOLIST, false, false,
     false},
    {STR_MENU_HOTSPOT, ROW_HOTSPOT, SRC_STORED, TABLE_RANGE, 1, NOLIST, false,
     false, false},
    {STR_MENU_WEB_SERVER, ROW_WEB_SERVER, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, false, false},
    /* Set a digit at a time on its own editor, not scrubbed as a number, and
     * saved only on the sixth digit, through the same call the browser's
     * Network page uses. */
    {STR_MENU_WEB_PIN, ROW_WEB_PIN, SRC_STORED, 0,
     (int32_t)(ACCESS_PIN_MODULUS - 1), 1, NOLIST, false, false, false},
    {STR_MENU_CLOCK_FROM_NETWORK, ROW_NETWORK_TIME, SRC_STORED,
     NETWORK_TIME_OFF, CLOCK_OFFSET_MAX_MINUTES, 15, NOLIST, true, false,
     false},
    SUB(STR_MENU_NETWORK_INFO, kNetInfoGroup),
};

static const MenuRow kEncoderRows[] = {
    {STR_MENU_ENCODER, ROW_ENCODER, SRC_STORED, TABLE_RANGE, 1, NOLIST, false,
     true, false},
    {STR_MENU_DIRECTION, ROW_ENCODER_DIR, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, true, false},
};
static const MenuGroup kEncoderGroup =
    GROUP(STR_MENU_ENCODER_GROUP, kEncoderRows);

static const MenuRow kControlRows[] = {
    SUB(STR_MENU_ENCODER_GROUP, kEncoderGroup),
    {STR_MENU_KEY_BEEPS, ROW_KEY_BEEPS, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, false, false},
    {STR_MENU_BAND_EDGE_BEEP, ROW_EDGE_BEEP, SRC_STORED, TABLE_RANGE, 1, NOLIST,
     false, false, false},
    {STR_MENU_START_CHIME, ROW_CHIME, SRC_STORED, TABLE_RANGE, 1, NOLIST, false,
     true, false},
    /* Off leaves the touch screen unread at once, for a panel that touches
     * itself. Recovery has the same row, for when this menu cannot be
     * reached. */
    {STR_MENU_TOUCH, ROW_TOUCH, SRC_STORED, 0, 1, 1, NOLIST, false, false,
     false},
    /* Opens the calibration screen; the knob leaves it, so a glass that
     * reads badly cannot trap anybody there. */
    {STR_MENU_CALIBRATE_TOUCH, ROW_CALIBRATE_TOUCH, SRC_ACTION, 0, 0, 1, NOLIST,
     false, false, false},
};

static const MenuRow kSystemRows[] = {
    /* In five minute steps; the API takes any minute, and a time set there
     * is shown as it is. */
    {STR_MENU_AUTO_OFF, ROW_AUTO_OFF, SRC_STORED, TABLE_RANGE, 5, NOLIST, true,
     false, false},
    /* Turned on, the radio looks in this start too, once it is on the
     * network. */
    {STR_MENU_UPDATE_CHECK, ROW_UPDATE_CHECK, SRC_STORED, TABLE_RANGE, 1,
     NOLIST, false, false, false},
    /* Named for the newer version while one is known, and then its press
     * opens the same offer the radio shows at start. Otherwise its value says
     * why there is nothing to install. */
    {STR_MENU_FIRMWARE_UPDATE, ROW_UPDATE_INSTALL, SRC_ACTION, 0, 0, 1, NOLIST,
     false, false, false},
    {STR_MENU_RESTART, ROW_RESTART, SRC_ACTION, 0, 1, 1, NOLIST, false, false,
     true},
};

static const MenuRow kDiagnosticRows[] = {
    {STR_MENU_RUNNING_FROM, ROW_ABOUT_SLOT, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_UPTIME, ROW_ABOUT_UPTIME, SRC_INFO, 0, 0, 1, NOLIST, false, false,
     false},
    {STR_MENU_BATTERY_VOLTAGE, ROW_SYSTEM_BATTERY, SRC_INFO, 0, 0, 1, NOLIST,
     false, false, false},
    {STR_COMMON_TUNER, ROW_ABOUT_TUNER, SRC_INFO, 0, 0, 1, NOLIST, false, false,
     false},
    {STR_MENU_RESET_REASON, ROW_SYSTEM_RESET, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_CPU_CORE_0, ROW_SYSTEM_CPU_0, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_CPU_CORE_1, ROW_SYSTEM_CPU_1, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_FREE_HEAP, ROW_SYSTEM_HEAP, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_LOWEST_HEAP, ROW_SYSTEM_HEAP_LOWEST, SRC_INFO, 0, 0, 1, NOLIST,
     false, false, false},
    {STR_MENU_LARGEST_BLOCK, ROW_SYSTEM_HEAP_BLOCK, SRC_INFO, 0, 0, 1, NOLIST,
     false, false, false},
    {STR_MENU_LVGL_POOL, ROW_SYSTEM_LVGL, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_CHIP, ROW_SYSTEM_CHIP, SRC_INFO, 0, 0, 1, NOLIST, false, false,
     false},
    {STR_MENU_FLASH_SIZE, ROW_SYSTEM_FLASH, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
};

static const MenuRow kAboutRows[] = {
    {STR_MENU_VERSION, ROW_ABOUT_VERSION, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_BUILD, ROW_ABOUT_BUILD, SRC_INFO, 0, 0, 1, NOLIST, false, false,
     false},
    {STR_MENU_DEVELOPER, ROW_ABOUT_DEVELOPER, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_LICENSE, ROW_ABOUT_LICENSE, SRC_INFO, 0, 0, 1, NOLIST, false,
     false, false},
    {STR_MENU_GITHUB, ROW_ABOUT_GITHUB, SRC_INFO, 0, 0, 1, NOLIST, false, false,
     false},
};

/*
 * The twelve groups, in the order a person walks them: the screens and
 * actions the keys reach, what is stored, what comes out of the speaker, the
 * two sides of the tuner, DX mode, the panel, the network, the knob and the
 * noises the radio makes, the box, its readings, and who made the firmware.
 *
 * FM and AM are separate because this radio has two sides: iMS, the
 * equaliser, de-emphasis, the high cut, the blends, the FM blanker and RDS are
 * FM only, and the MW spacing, the AM filter width and the AM blanker are AM
 * only. One combined group would have half its rows inert on every band,
 * which teaches a person to ignore the list.
 *
 * A run of rows that belong together is a sub-group, opened from a row of its
 * own, one level deep. Rows that only show a value sit in Diagnostics and in
 * Connectivity's Network Info, apart from the ones that change something.
 */
static const MenuGroup kGroups[] = {
    GROUP(STR_MENU_GO_TO, kGotoRows),
    GROUP(STR_MENU_STATIONS, kStationRows),
    GROUP(STR_MENU_AUDIO, kAudioRows),
    GROUP(STR_MENU_FM_SETUP, kFmRows),
    GROUP(STR_MENU_AM_SETUP, kAmRows),
    GROUP(STR_MENU_DX_SETUP, kDxRows),
    GROUP(STR_MENU_DISPLAY, kDisplayRows),
    GROUP(STR_MENU_CONNECTIVITY, kConnectRows),
    GROUP(STR_MENU_CONTROLS, kControlRows),
    GROUP(STR_MENU_SYSTEM, kSystemRows),
    GROUP(STR_MENU_DIAGNOSTICS, kDiagnosticRows),
    GROUP(STR_MENU_ABOUT, kAboutRows),
};

#define GROUP_COUNT ((uint8_t)(sizeof(kGroups) / sizeof(MenuGroup)))

/* What each RDS region is called. In the order of `RdsRegion`. */
static const StrId kRdsRegionNames[RDS_REGION_COUNT] = {
    STR_MENU_RDS_REGION_EUROPE,
    STR_MENU_RDS_REGION_NORTH_AMERICA,
};

/* What each FM band plan is called. In the order of `FmRegion`. */
static const StrId kRegionNames[FM_REGION_COUNT] = {
    STR_MENU_REGION_FULL,   STR_MENU_REGION_JAPAN, STR_MENU_REGION_WIDE,
    STR_MENU_REGION_87_108, STR_MENU_REGION_WORLD,
};

static Menu sMenu;
static Settings *sLive = NULL;
/*
 * The settings as the knob is turning them.
 *
 * A copy, not the live struct. Nothing is stored until the edit is accepted,
 * so a cancel has somewhere to go back to and a sweep of the brightness costs
 * one write to flash instead of one for every click.
 */
static Settings sPending;
/* The Web PIN's digits while it is being set. */
static AccessPinEdit sPin;
static uint8_t sTopGroup;
static uint8_t sTopRow;
static uint8_t sTopSub;

/*
 * Something to say, along the bottom, until the next gesture.
 *
 * For a refusal or a failure that has nowhere else to go, such as the radio
 * being too busy to read, a row that needs FM, or a setting that could not
 * be saved. There is no serial cable on this radio, so without this each one
 * is a press that redraws the identical screen, which is what a broken
 * button looks like.
 *
 * It clears on the next turn or press rather than on a timer, because the
 * menu only draws when something moves and a timer would need a poll that
 * does not exist.
 */
static const char *sNote = NULL;

/* The band a station scan row walks. False for any other row. */
static bool scanRowBand(RowId id, BandId *band) {
  switch (id) {
    case ROW_FM_SCAN:
      *band = BAND_FM;
      return true;
    case ROW_MW_SCAN:
      *band = BAND_MW;
      return true;
    case ROW_SW_SCAN:
      *band = BAND_SW;
      return true;
    case ROW_LW_SCAN:
      *band = BAND_LW;
      return true;
    default:
      return false;
  }
}

/* A scan's count is on the panel, so the next poll draws again: while the
 * scan runs to keep the count up, and once after it ends to take it off. */
static bool sScanCountShown = false;

/*
 * One reading of the radio per gesture, not one per row.
 *
 * `radioGetSettings` waits up to fifty milliseconds for the radio's lock. A
 * draw of the FM group asks for a value on five rows and asks whether each is
 * available, which is ten of those calls, so one click of the knob could hold
 * the loop task for half a second while a seek had the lock. The loop task is
 * the web server, the keypad and the panel light, so that is the radio going
 * unresponsive in the one place a person is actively using it.
 *
 * Taken once when a gesture arrives and reused until the next one. Every row on
 * the screen is then drawn from the same moment, which is also the honest
 * answer: five rows read a few milliseconds apart could disagree. Only the
 * settings, since no row reads anything else, not a whole snapshot, which would
 * cost 856 bytes of static RAM.
 */
static RadioSettings sGesture; /* The menu reads only the settings. */
static bool sGestureValid = false;
static bool sGestureTaken = false;

static void gestureBegins(void) {
  sGestureTaken = false;
  sNote = NULL;
}

static bool settingsNow(RadioSettings *out) {
  if (!sGestureTaken) {
    sGestureValid = radioGetSettings(&sGesture);
    sGestureTaken = true;
  }
  if (!sGestureValid) {
    return false;
  }
  *out = sGesture;
  return true;
}

/* Where the strings live while LVGL holds pointers to them. 32 fits the
 * longest value, the 30 character GitHub address in About. */
static char sValueText[SCREEN_MENU_ROWS][32];

/* A preset row's name, "P03 " and up to 16 characters of its own name. */
static char sPresetName[SCREEN_MENU_ROWS][24];

/* The n-th saved preset's row: "P03 RADIO ONE", or "P03" with no name, and its
 * frequency with the unit. A dash when the list moved under the cursor. */
static void presetRowText(uint8_t n, char *name, size_t nameLen, char *value,
                          size_t valueLen) {
  MemoryChannel c;
  const int slot = memoryStoreNthUsed(n, &c);
  if (slot == MEMORY_NO_SLOT) {
    snprintf(name, nameLen, "%s", txt(STR_COMMON_DASH));
    value[0] = '\0';
    return;
  }
  const int w =
      snprintf(name, nameLen, txt(STR_RADIO_FMT_MEMORY_SLOT), slot + 1);
  if (c.name[0] != '\0' && w > 0 && (size_t)w < nameLen) {
    snprintf(name + w, nameLen - (size_t)w, " %s", c.name);
  }
  bandFormatWithUnit((BandId)c.band, c.freqKHz, value, valueLen);
}
/* A title with its way back in it, for pathOf. 48 fits the longest,
 * "FM Reception > Seek Sensitivity", with room to spare. */
static char sPathText[48];

/* "Audio > Squelch": the list a screen sits in, then its own name, as the
 * title, so a person can see where back goes. One at a time: a draw shows
 * one title. */
static const char *pathOf(const char *parent, const char *name) {
  snprintf(sPathText, sizeof(sPathText), txt(STR_MENU_FMT_PATH), parent, name);
  return sPathText;
}

static const MenuGroup *group(void) {
  return &kGroups[sMenu.group < GROUP_COUNT ? sMenu.group : 0];
}

/* The list being looked at: the open sub-group, or else the open group. */
static const MenuGroup *list(void) {
  const MenuGroup *g = group();
  if (sMenu.inSub && sMenu.sub < g->count && g->rows[sMenu.sub].opens != NULL) {
    return g->rows[sMenu.sub].opens;
  }
  return g;
}

/*
 * Whether this row can do anything on the band the radio is on.
 *
 * Only the FM step row asks. It sets the FM band's step, so on any other
 * band it reads `FM only` and cannot be edited, rather than offering steps
 * that mean nothing there.
 */
typedef enum {
  ROW_ON = 0,   /* It applies here, and the value can be read. */
  ROW_OFF_BAND, /* The radio is on a band this row means nothing on. */
  ROW_UNKNOWN,  /* The radio could not be read this moment. */
} RowAvailable;

static RowAvailable rowAvailable(const MenuRow *row) {
  if (row == NULL) {
    return ROW_UNKNOWN;
  }
  if (row->id != ROW_FM_STEP) {
    return ROW_ON;
  }
  RadioSettings now;
  if (!settingsNow(&now)) {
    /* Three answers, not two: a radio that could not be read this moment is
     * not a band the row means nothing on, and the row says which. */
    return ROW_UNKNOWN;
  }
  /*
   * BAND_FM only, not OIRT. Both are MODULATION_FM, but OIRT's steps are 10
   * and 30 against FM's 50, 100 and 200: a wrong value from this row would
   * be refused by bandStepAllowed rather than reach the tuner, but the row
   * would still be offering steps that mean nothing on the band it is
   * tuned to.
   */
  return now.band == BAND_FM ? ROW_ON : ROW_OFF_BAND;
}

/*
 * The Station Log's rows as last read, each kept in the slot its place in
 * the log gives it, so one click of the knob reads one entry and not six:
 * every read opens the logbook file, and the browser's whole log reads at
 * about 22 ms an entry. A window of six rows holds six places in a row, so
 * no two share a slot. Forgotten when the list opens and when the count
 * moves.
 */
typedef struct {
  bool valid;
  uint16_t at; /* Its place in the log, from the oldest. */
  char name[LOGBOOK_NAME_LEN + 8];
  char value[16];
} LogRow;
static LogRow sLogRows[SCREEN_MENU_ROWS];
static uint16_t sLogRowsCount;

static void logRowsForget(void) {
  for (uint8_t i = 0; i < SCREEN_MENU_ROWS; i++) {
    sLogRows[i].valid = false;
  }
}

/* The words for the entry at `at`, counted from the oldest. */
static const LogRow *logRow(uint16_t at) {
  LogRow *r = &sLogRows[at % SCREEN_MENU_ROWS];
  if (r->valid && r->at == at) {
    return r;
  }
  LogbookEntry e;
  if (logbookFsEntryAt(at, &e)) {
    logbookEntryLabel(&e, r->name, sizeof(r->name));
    logbookEntryFrequency(&e, r->value, sizeof(r->value));
  } else {
    snprintf(r->name, sizeof(r->name), "%s", txt(STR_COMMON_DASH));
    r->value[0] = '\0';
  }
  r->at = at;
  r->valid = true;
  return r;
}

/* The Station Log's count as the logbook has it now. */
static void logCountNow(void) {
  const uint16_t n = logbookFsCount();
  sLogGroup.count = (uint8_t)(n < UINT8_MAX ? n : UINT8_MAX);
}

/* The Presets group's count: the saved presets. */
static void presetCountNow(void) {
  sPresetGroup.count = (uint8_t)memoryStoreCount();
}

static const MenuRow *rowAt(uint8_t index) {
  const MenuGroup *g = list();
  if (g == &sLogGroup) {
    return index < g->count ? &kLogEntryRow[0] : NULL;
  }
  if (g == &sPresetGroup) {
    return index < g->count ? &kPresetEntryRow[0] : NULL;
  }
  if (g->rows == NULL || index >= g->count) {
    return NULL;
  }
  return &g->rows[index];
}

/* The band a per band step row sets, or BAND_COUNT for any other row. */
static BandId stepBandOf(RowId id) {
  switch (id) {
    case ROW_LW_STEP:
      return BAND_LW;
    case ROW_MW_STEP:
      return BAND_MW;
    case ROW_SW_STEP:
      return BAND_SW;
    default:
      return BAND_COUNT;
  }
}

/* ------------------------------------------------------------ the values */

/*
 * The stored rows whose value is one setting as it is, by the key the
 * settings table and the API know it by, so the menu reads, writes and
 * bounds each through the same row the API does. The rows that combine or
 * translate values, the themes, the rotation, the DX preset range and width,
 * Network Time and the Web PIN, are written out in storedValue and storedSet.
 */
static const struct {
  RowId id;
  const char *key;
} kRowSettings[] = {
    {ROW_FM_REGION, "rgn"},
    {ROW_FM_SEEK, "fsn"},
    {ROW_MW_SPACING, "spc"},
    {ROW_AM_SEEK, "asn"},
    {ROW_SQUELCH_FLOOR, "sqf"},
    {ROW_MUTE_RAMP, "smu"},
    {ROW_AGC_TARGET, "agt"},
    {ROW_AGC_BOOST, "agb"},
    {ROW_RDS, "rds"},
    {ROW_RDS_REGION, "rrg"},
    {ROW_BACKLIGHT, "blt"},
    {ROW_BACKLIGHT_DIM, "bdm"},
    {ROW_DIM_AFTER, "bds"},
    {ROW_AUTO_OFF, "slp"},
    {ROW_UPDATE_CHECK, "upc"},
    {ROW_FADE_AT_START, "blf"},
    {ROW_BATTERY, "bat"},
    {ROW_DX_DWELL, "ddw"},
    {ROW_DX_STOP, "dst"},
    {ROW_DX_RANGE, "dsc"},
    {ROW_DX_LOOP, "dlp"},
    {ROW_DX_MUTE, "dmu"},
    {ROW_DX_AUTOLOG, "dal"},
    {ROW_DX_LOG_RT, "drt"},
    {ROW_DX_WATCH, "dwt"},
    {ROW_LEVEL_OFFSET_FM, "fof"},
    {ROW_LEVEL_OFFSET_AM, "aof"},
    {ROW_CHIME, "bps"},
    {ROW_KEY_BEEPS, "bpk"},
    {ROW_EDGE_BEEP, "bpe"},
    {ROW_HOTSPOT, "hsp"},
    {ROW_WEB_SERVER, "web"},
    {ROW_WIFI, "wif"},
    {ROW_ENCODER, "enc"},
    {ROW_ENCODER_DIR, "edr"},
};

static const SettingRow *rowSetting(RowId id) {
  for (size_t i = 0; i < sizeof(kRowSettings) / sizeof(kRowSettings[0]); i++) {
    if (kRowSettings[i].id == id) {
      return settingsTableFind(kRowSettings[i].key);
    }
  }
  return NULL;
}

static int32_t storedValue(RowId id, const Settings *s) {
  if (s == NULL) {
    return 0;
  }
  const SettingRow *setting = rowSetting(id);
  if (setting != NULL) {
    return settingsTableGet(s, setting);
  }
  switch (id) {
    case ROW_THEME:
      return palettePlaceOf(s->theme);
    case ROW_NIGHT_THEME:
      return palettePlaceOf(s->nightTheme);
    case ROW_DISPLAY_ROTATION:
      return s->displayRotation == 180 ? 1 : 0;
    case ROW_TOUCH:
      /* Stored as off, so the row reads Off then On and a right turn means
       * on, as on every other switch. */
      return s->touchOff != 0 ? 0 : 1;
    case ROW_DX_MEM_FIRST:
      return s->dxMemFirst;
    case ROW_DX_MEM_LAST:
      return s->dxMemLast;
    case ROW_DX_WIDTH:
      /* Its place in the FM widths, the number the knob walks. */
      for (size_t i = 1; i < bandBandwidthCount(BAND_FM); i++) {
        if (bandBandwidthAt(BAND_FM, i) == s->dxWidthKHz) {
          return (int32_t)i;
        }
      }
      return 1;
    case ROW_NETWORK_TIME:
      return s->ntpEnabled ? s->clockOffsetMinutes : NETWORK_TIME_OFF;
    case ROW_WEB_PIN:
      return (int32_t)s->accessPin;
    default:
      return 0;
  }
}

static void storedSet(RowId id, Settings *s, int32_t v) {
  if (s == NULL) {
    return;
  }
  const SettingRow *setting = rowSetting(id);
  if (setting != NULL) {
    settingsTableSet(s, setting, v);
    return;
  }
  switch (id) {
    case ROW_THEME:
      s->theme = paletteThemeAt((uint8_t)v);
      break;
    case ROW_NIGHT_THEME:
      s->nightTheme = paletteThemeAt((uint8_t)v);
      break;
    case ROW_DISPLAY_ROTATION:
      s->displayRotation = v != 0 ? 180 : 0;
      break;
    case ROW_TOUCH:
      s->touchOff = v != 0 ? 0 : 1;
      break;
    /* The other end moves with it rather than the edit being refused: the
     * first channel is never after the last. */
    case ROW_DX_MEM_FIRST:
      s->dxMemFirst = (uint8_t)v;
      if (s->dxMemLast < s->dxMemFirst) {
        s->dxMemLast = s->dxMemFirst;
      }
      break;
    case ROW_DX_MEM_LAST:
      s->dxMemLast = (uint8_t)v;
      if (s->dxMemFirst > s->dxMemLast) {
        s->dxMemFirst = s->dxMemLast;
      }
      break;
    case ROW_DX_WIDTH:
      s->dxWidthKHz = bandBandwidthAt(BAND_FM, (size_t)v);
      break;
    case ROW_NETWORK_TIME:
      /* Off leaves the stored offset alone, for the browser's page and the
       * API, which set the two apart. On this bar the step after Off is
       * -12:00, so turning it back on here starts from there. */
      if (v <= NETWORK_TIME_OFF) {
        s->ntpEnabled = 0;
      } else {
        s->ntpEnabled = 1;
        s->clockOffsetMinutes = (int16_t)v;
      }
      break;
    case ROW_WEB_PIN:
      s->accessPin = (uint32_t)v;
      break;
    default:
      break;
  }
}

/*
 * What the radio has this setting at, and whether it could be read at all.
 *
 * The second half is not a nicety. The snapshot waits fifty milliseconds for
 * the radio's lock and a seek can hold it longer. Zero is a real value on
 * every one of these rows, so a failed read must not answer zero: the list
 * would draw `off` for a setting that is on, and an edit started on that
 * zero would write zero to the tuner when the knob is held to cancel.
 */
static bool radioValueRead(RowId id, int32_t *out) {
  if (id == ROW_SQUELCH_MODE) {
    *out = (int32_t)radioSquelchMode(NULL);
    return true;
  }
  RadioSettings now;
  if (!settingsNow(&now)) {
    return false;
  }
  *out = 0;
  switch (id) {
    case ROW_LW_WIDTH:
    case ROW_MW_WIDTH:
    case ROW_SW_WIDTH:
      *out = radioBandWidth(&now, widthBandOf(id));
      break;
    case ROW_FM_STEP:
      *out = now.stepKHz;
      break;
    case ROW_LW_STEP:
    case ROW_MW_STEP:
    case ROW_SW_STEP: {
      /* The tuned band's step is the live one; the array has it only once
       * the band is left. */
      const BandId band = stepBandOf(id);
      *out = band == now.band ? now.stepKHz : now.bandStepKHz[band];
      break;
    }
    default:
      break;
  }
  switch (id) {
    case ROW_DEEMPHASIS:
      /* Three choices rather than the microseconds themselves, so the knob
       * walks off, 50 and 75 instead of seven hundred numbers that are not
       * settings. */
      *out = now.deemphasisUs == 0 ? 0 : now.deemphasisUs == 50 ? 1 : 2;
      break;
    case ROW_IMS:
      *out = now.multipathSuppression ? 1 : 0;
      break;
    case ROW_EQ:
      *out = now.equalizer ? 1 : 0;
      break;
    case ROW_MONO:
      *out = now.forcedMono ? 1 : 0;
      break;
    case ROW_HIGH_CUT:
      *out = now.highCutStart;
      break;
    case ROW_STEREO_BLEND:
      *out = now.stereoBlendStart;
      break;
    case ROW_STHI_BLEND:
      *out = now.stHiBlendStart;
      break;
    case ROW_FM_BLANKER:
      *out = now.fmNoiseBlankerStart;
      break;
    case ROW_AM_BLANKER:
      *out = now.amNoiseBlankerStart;
      break;
    case ROW_AM_HIGH_CUT:
      *out = now.amHighCutStart;
      break;
    case ROW_LW_HIGH_CUT:
      *out = now.lwHighCutStart;
      break;
    case ROW_AM_SOFT_MUTE:
      *out = now.amSoftMuteStart;
      break;
    case ROW_LW_SOFT_MUTE:
      *out = now.lwSoftMuteStart;
      break;
    default:
      break;
  }
  return true;
}

/* Put a value into the radio, and say whether the command went out. */
static bool radioSet(RowId id, int32_t v) {
  RadioCommand cmd = {};
  switch (id) {
    case ROW_LW_WIDTH:
    case ROW_MW_WIDTH:
    case ROW_SW_WIDTH:
      cmd.kind = RADIO_SET_BAND_BANDWIDTH;
      cmd.band = widthBandOf(id);
      cmd.bandwidthKHz = (uint16_t)v;
      return radioPost(&cmd);
    case ROW_FM_STEP:
      cmd.kind = RADIO_SET_STEP;
      cmd.stepKHz = (uint16_t)v;
      return radioPost(&cmd);
    case ROW_LW_STEP:
    case ROW_MW_STEP:
    case ROW_SW_STEP:
      cmd.kind = RADIO_SET_BAND_STEP;
      cmd.band = stepBandOf(id);
      cmd.stepKHz = (uint16_t)v;
      return radioPost(&cmd);
    case ROW_DEEMPHASIS:
      cmd.kind = RADIO_SET_DEEMPHASIS;
      cmd.deemphasisUs = (uint16_t)(v == 0 ? 0 : (v == 1 ? 50 : 75));
      return radioPost(&cmd);
    case ROW_IMS:
      cmd.kind = RADIO_SET_MPH_SUPPRESSION;
      cmd.on = v != 0;
      return radioPost(&cmd);
    case ROW_EQ:
      cmd.kind = RADIO_SET_EQUALIZER;
      cmd.on = v != 0;
      return radioPost(&cmd);
    case ROW_MONO:
      cmd.kind = RADIO_SET_MONO;
      cmd.on = v != 0;
      return radioPost(&cmd);
    case ROW_HIGH_CUT:
    case ROW_STEREO_BLEND:
    case ROW_STHI_BLEND: {
      /* The one value; the radio keeps the other two as they are. */
      const int at = id == ROW_HIGH_CUT ? 0 : (id == ROW_STEREO_BLEND ? 1 : 2);
      cmd.kind = RADIO_SET_WEAK_SIGNAL;
      cmd.weak[at] = (uint8_t)v;
      cmd.members = RADIO_MEMBER(at);
      return radioPost(&cmd);
    }
    case ROW_FM_BLANKER:
    case ROW_AM_BLANKER: {
      const int at = id == ROW_AM_BLANKER ? 0 : 1;
      cmd.kind = RADIO_SET_NOISE_BLANKER;
      cmd.blanker[at] = (uint8_t)v;
      cmd.members = RADIO_MEMBER(at);
      return radioPost(&cmd);
    }
    case ROW_AM_HIGH_CUT:
    case ROW_LW_HIGH_CUT:
    case ROW_AM_SOFT_MUTE:
    case ROW_LW_SOFT_MUTE: {
      const int at = id - ROW_AM_HIGH_CUT;
      cmd.kind = RADIO_SET_AM_WEAK_SIGNAL;
      cmd.amWeak[at] = (uint8_t)v;
      cmd.members = RADIO_MEMBER(at);
      return radioPost(&cmd);
    }
    case ROW_SQUELCH_MODE:
      radioSetSquelchMode((SquelchMode)v);
      return true;
    default:
      return false;
  }
}

/*
 * The choices for a row, resolved for right now.
 *
 * Compile time for every row except the step rows: FM's for BAND_FM, and
 * LW's, MW's and SW's each for its own band, since each allows a different
 * pair. They walk bandStepCount/bandStepAt rather than a list written here,
 * so a step added to core/band_plan.c appears on the row without this table
 * changing.
 */
static const uint8_t *rowChoices(const MenuRow *row, uint8_t *count) {
  static uint8_t buf[BAND_STEP_MAX_COUNT];
  *count = 0;
  if (row == NULL) {
    return NULL;
  }
  const BandId band = row->id == ROW_FM_STEP ? BAND_FM : stepBandOf(row->id);
  if (band == BAND_COUNT) {
    *count = row->choiceCount;
    return row->choices;
  }
  BandPlanConfig plan;
  if (!radioTaskPlan(&plan)) {
    return NULL;
  }
  size_t n = bandStepCount(band, &plan);
  if (n > sizeof(buf)) {
    n = sizeof(buf); /* Never happens on this radio; caught rather than
                        overrunning if a band ever grew a fourth step. */
  }
  for (size_t i = 0; i < n; i++) {
    buf[i] = (uint8_t)bandStepAt(band, &plan, i);
  }
  *count = (uint8_t)n;
  return n > 0 ? buf : NULL;
}

/* Whether a row walks a list of legal values rather than a plain range. */
static bool rowIsListed(const MenuRow *row) {
  uint8_t count;
  return rowChoices(row, &count) != NULL;
}

/* A row's ends: for a row that is one stored setting, the settings table's,
 * so the menu offers the range the API takes; for the rest, the row's own. A
 * listed row walks its list, so its own ends stand. */
static int32_t rowMin(const MenuRow *row) {
  const SettingRow *setting = rowIsListed(row) ? NULL : rowSetting(row->id);
  return setting != NULL ? setting->low : row->min;
}

static int32_t rowMax(const MenuRow *row) {
  const SettingRow *setting = rowIsListed(row) ? NULL : rowSetting(row->id);
  return setting != NULL ? setting->high : row->max;
}

/* How many entries that list has, right now. */
static uint8_t rowChoiceCount(const MenuRow *row) {
  uint8_t count;
  rowChoices(row, &count);
  return count;
}

/*
 * Turn a real value into the index of its place in the row's list.
 *
 * The nearest one below, so a value the radio came up with that is not in the
 * list still lands somewhere sensible rather than at the top.
 */
static int32_t indexOf(const MenuRow *row, int32_t value) {
  uint8_t count;
  const uint8_t *choices = rowChoices(row, &count);
  if (choices == NULL || count == 0) {
    return value;
  }
  int32_t best = 0;
  for (uint8_t i = 0; i < count; i++) {
    if ((int32_t)choices[i] <= value) {
      best = i;
    }
  }
  return best;
}

/* And back again. */
static int32_t valueOfIndex(const MenuRow *row, int32_t index) {
  uint8_t count;
  const uint8_t *choices = rowChoices(row, &count);
  if (choices == NULL || count == 0) {
    return index;
  }
  if (index < 0) {
    index = 0;
  }
  if (index >= count) {
    index = count - 1;
  }
  return choices[index];
}

/* ------------------------------------------------------------- the words */

/*
 * CPU use per core over the last whole second, read from each core's idle
 * task run time on the clock the scheduler counts it on. Sampled from the
 * loop once a second whether the menu is open or not, so the rows have a
 * number the moment they are shown rather than a second later.
 */
static CpuSample sCpuLast[2];
static uint8_t sCpuPercent[2];
static bool sCpuKnown[2];
static bool sCpuStarted = false;

static void cpuSampleOf(BaseType_t core, CpuSample *out) {
  TaskStatus_t idle;
  vTaskGetInfo(xTaskGetIdleTaskHandleForCore(core), &idle, pdFALSE, eInvalid);
  out->idleUs = idle.ulRunTimeCounter;
  out->clockUs = portGET_RUN_TIME_COUNTER_VALUE();
}

static void cpuTick(void) {
  for (BaseType_t core = 0; core < 2; core++) {
    CpuSample now;
    cpuSampleOf(core, &now);
    if (sCpuStarted) {
      sCpuKnown[core] =
          cpuBusyPercent(&sCpuLast[core], &now, &sCpuPercent[core]);
    }
    sCpuLast[core] = now;
  }
  sCpuStarted = true;
}

static void cpuText(uint8_t core, char *out, size_t len) {
  if (!sCpuKnown[core]) {
    snprintf(out, len, "%s", txt(STR_COMMON_DASH));
    return;
  }
  snprintf(out, len, txt(STR_MENU_FMT_PERCENT), (int)sCpuPercent[core]);
}

static StrId resetReasonId(void) {
  esp_reset_reason_t chip = ESP_RST_UNKNOWN;
  RestartWhy why = RESTART_WHY_NONE;
  restartReasonParts(&chip, &why);
  /* This firmware's own note first: the chip says only "software" for all
   * five of its restarts. */
  switch (why) {
    case RESTART_WHY_ASKED:
      return STR_RESET_ASKED;
    case RESTART_WHY_BOOT_WATCHDOG:
      return STR_RESET_BOOT_WATCHDOG;
    case RESTART_WHY_ROLLBACK:
      return STR_RESET_ROLLBACK;
    case RESTART_WHY_DISPLAY:
      return STR_RESET_DISPLAY;
    case RESTART_WHY_UPDATE:
      return STR_RESET_UPDATE;
    case RESTART_WHY_NONE:
    default:
      break;
  }
  switch (chip) {
    case ESP_RST_POWERON:
      return STR_RESET_POWER;
    case ESP_RST_SW:
      return STR_RESET_SOFTWARE;
    case ESP_RST_PANIC:
      return STR_RESET_PANIC;
    case ESP_RST_INT_WDT:
      return STR_RESET_INT_WATCHDOG;
    case ESP_RST_TASK_WDT:
      return STR_RESET_TASK_WATCHDOG;
    case ESP_RST_WDT:
      return STR_RESET_WATCHDOG;
    case ESP_RST_DEEPSLEEP:
      return STR_RESET_DEEP_SLEEP;
    case ESP_RST_BROWNOUT:
      return STR_RESET_BROWNOUT;
    case ESP_RST_EXT:
      return STR_RESET_PIN;
    default:
      return STR_RESET_UNKNOWN;
  }
}

static void infoText(RowId id, char *out, size_t len) {
  switch (id) {
    case ROW_SQUELCH_LEVEL: {
      /* The Manual threshold, which the knob sets while the squelch is in
       * Manual. Auto does not use it, so in Auto and Off this is the last
       * level the knob set, or Off when it has set none since the start. */
      int16_t tenths = 0;
      (void)radioSquelchMode(&tenths);
      const int dbuv = tenths / 10;
      if (dbuv == 0) {
        snprintf(out, len, "%s", txt(STR_COMMON_OFF));
      } else {
        snprintf(out, len, txt(STR_MENU_FMT_DBUV), dbuv);
      }
      return;
    }
    case ROW_NET_STATE: {
      WifiState st = wifiState();
      snprintf(out, len, "%s",
               txt(st == WIFI_STATE_ONLINE         ? STR_MENU_WIFI_JOINED
                   : st == WIFI_STATE_JOINING      ? STR_MENU_WIFI_JOINING
                   : st == WIFI_STATE_ACCESS_POINT ? STR_MENU_WIFI_HOTSPOT
                                                   : STR_COMMON_OFF));
      return;
    }
    case ROW_NET_ADDRESS:
      /* With the web server's port, since it is not 80 and a browser given
       * the address alone does not find the page. The radio's name where it
       * is announced, since the network can move the address and the name
       * stays; the address otherwise. No address, no port: there is no page
       * to reach. Off when either switch is off, since there is then no page
       * at any address. */
      if (sLive != NULL && (!sLive->webEnabled || !sLive->wifiEnabled)) {
        snprintf(out, len, "%s", txt(STR_COMMON_OFF));
      } else if (strcmp(wifiAddress(), "0.0.0.0") == 0) {
        snprintf(out, len, "%s", wifiAddress());
      } else if (wifiAnnouncedName() != NULL) {
        snprintf(out, len, txt(STR_MENU_FMT_NAME_ADDRESS), wifiAnnouncedName(),
                 (unsigned)WEB_PORT);
      } else {
        snprintf(out, len, "%s:%u", wifiAddress(), (unsigned)WEB_PORT);
      }
      return;
    case ROW_NET_IP:
      /* The address itself, for a phone or a browser that cannot look a
       * .local name up. */
      if (sLive != NULL && !sLive->wifiEnabled) {
        snprintf(out, len, "%s", txt(STR_COMMON_OFF));
      } else {
        snprintf(out, len, "%s", wifiAddress());
      }
      return;
    case ROW_NET_NAME:
      snprintf(out, len, "%s", wifiNetworkName());
      return;
    case ROW_NET_MAC: {
      uint8_t mac[6];
      if (!deviceMacRead(mac)) {
        snprintf(out, len, "%s", txt(STR_COMMON_DASH));
        return;
      }
      /* All six bytes, as a router's list of devices shows them, so the
       * radio can be found there. The widest, 17 characters, fits beside the
       * name. */
      snprintf(out, len, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1],
               mac[2], mac[3], mac[4], mac[5]);
      return;
    }
    case ROW_ABOUT_VERSION:
      snprintf(out, len, "%s", FIRMWARE_VERSION);
      return;
    case ROW_ABOUT_DEVELOPER:
      snprintf(out, len, "%s", txt(STR_ABOUT_DEVELOPER));
      return;
    case ROW_ABOUT_GITHUB:
      snprintf(out, len, "%s", txt(STR_ABOUT_GITHUB));
      return;
    case ROW_ABOUT_BUILD:
      snprintf(out, len, "%s",
               FIRMWARE_BUILD[0] != '\0' ? FIRMWARE_BUILD
                                         : txt(STR_ABOUT_BUILD_UNKNOWN));
      return;
    case ROW_ABOUT_LICENSE:
      snprintf(out, len, "%s", txt(STR_ABOUT_LICENSE));
      return;
    case ROW_ABOUT_SLOT:
      snprintf(out, len, "%s", rollbackRunningPartition());
      return;
    case ROW_ABOUT_UPTIME: {
      uint32_t s = millis() / 1000;
      if (s < 3600) {
        snprintf(out, len, txt(STR_MENU_FMT_MINUTES), (unsigned)(s / 60));
      } else {
        snprintf(out, len, txt(STR_MENU_FMT_HOURS_MINUTES),
                 (unsigned)(s / 3600), (unsigned)((s % 3600) / 60));
      }
      return;
    }
    case ROW_SYSTEM_BATTERY: {
      /* Live only while Wi-Fi is off. GPIO 13 is ADC2, which the Wi-Fi driver
       * holds while it runs, so with it on every read fails and the reading
       * is the one taken at start up, which the value says. */
      uint16_t mv = 0;
      if (wifiState() == WIFI_STATE_OFFLINE && batteryAdcRead(&mv)) {
        snprintf(out, len, txt(STR_MENU_FMT_VOLTS), (unsigned)(mv / 1000),
                 (unsigned)((mv % 1000) / 10));
      } else if (batteryAdcAtBoot(&mv)) {
        snprintf(out, len, txt(STR_MENU_FMT_VOLTS_AT_START),
                 (unsigned)(mv / 1000), (unsigned)((mv % 1000) / 10));
      } else {
        snprintf(out, len, "%s", txt(STR_COMMON_DASH));
      }
      return;
    }
    case ROW_NET_SIGNAL: {
      int8_t rssi = 0;
      if (wifiRssiDbm(&rssi)) {
        snprintf(out, len, txt(STR_MENU_FMT_DBM), (int)rssi);
      } else {
        snprintf(out, len, "%s", txt(STR_COMMON_DASH));
      }
      return;
    }
    case ROW_SYSTEM_RESET:
      snprintf(out, len, "%s", txt(resetReasonId()));
      return;
    case ROW_SYSTEM_CPU_0:
      cpuText(0, out, len);
      return;
    case ROW_SYSTEM_CPU_1:
      cpuText(1, out, len);
      return;
    case ROW_SYSTEM_HEAP:
      snprintf(out, len, txt(STR_MENU_FMT_KB),
               (unsigned)(systemHeapFree() / 1024));
      return;
    case ROW_SYSTEM_HEAP_LOWEST:
      snprintf(out, len, txt(STR_MENU_FMT_KB),
               (unsigned)(systemHeapLowest() / 1024));
      return;
    case ROW_SYSTEM_HEAP_BLOCK:
      snprintf(out, len, txt(STR_MENU_FMT_KB),
               (unsigned)(systemHeapLargest() / 1024));
      return;
    case ROW_SYSTEM_LVGL: {
      uint32_t used = 0;
      uint32_t total = 0;
      if (!lvglPortMemory(&used, &total, NULL, NULL, NULL)) {
        snprintf(out, len, "%s", txt(STR_COMMON_DASH));
        return;
      }
      snprintf(out, len, txt(STR_MENU_FMT_KB_OF_KB), (unsigned)(used / 1024),
               (unsigned)(total / 1024));
      return;
    }
    case ROW_SYSTEM_CHIP: {
      /* The revision comes as major times 100 plus minor: 301 is v3.1. */
      const unsigned rev = ESP.getChipRevision();
      snprintf(out, len, txt(STR_MENU_FMT_CHIP), ESP.getChipModel(), rev / 100,
               rev % 100);
      return;
    }
    case ROW_SYSTEM_FLASH: {
      /* The chip's own size, from its ID. The image header says what the
       * build was told, which is not always what is fitted. Read once: the
       * read holds off both cores' flash cache while it runs, and the chip
       * fitted does not change. */
      static uint32_t bytes = 0;
      static bool read = false;
      if (!read) {
        read = true;
        if (esp_flash_get_physical_size(NULL, &bytes) != ESP_OK) {
          bytes = 0;
        }
      }
      if (bytes == 0) {
        snprintf(out, len, "%s", txt(STR_COMMON_DASH));
        return;
      }
      snprintf(out, len, txt(STR_MENU_FMT_MB),
               (unsigned)(bytes / (1024UL * 1024UL)));
      return;
    }
    case ROW_ABOUT_TUNER: {
      const Tef668xCapabilities *caps = tef668xCapabilities();
      if (caps != NULL) {
        snprintf(out, len, txt(STR_MENU_FMT_TUNER_PATCH), caps->part,
                 (unsigned)caps->patchVersion);
      } else {
        snprintf(out, len, "%s", txt(STR_COMMON_NONE));
      }
      return;
    }
    default:
      snprintf(out, len, "%s", txt(STR_COMMON_DASH));
      return;
  }
}

static const char *unitOf(const MenuRow *row);

/* The two DX rows whose knob number is not the value itself: the dwell in
 * tenths, "2.5", and the width by its place in the FM widths, "114". */
static void dxNumber(const MenuRow *row, int32_t v, char *out, size_t len) {
  if (row->id == ROW_DX_WIDTH) {
    snprintf(out, len, "%u", (unsigned)bandBandwidthAt(BAND_FM, (size_t)v));
  } else {
    snprintf(out, len, "%d.%d", (int)(v / 10), (int)(v % 10));
  }
}

/* What the update row says: the newer release's size, or why there is
 * nothing to install. */
static void updateRowValue(char *out, size_t len) {
  StrId why = STR_COMMON_OFF;
  switch (updateCheckState()) {
    case UPDATE_STATE_FOUND: {
      char mb[12];
      updateFormatMegabytes(updateCheckSize(), mb, sizeof(mb));
      snprintf(out, len, txt(STR_MENU_FMT_MEGABYTES), mb);
      return;
    }
    case UPDATE_STATE_NONE:
      why = STR_MENU_UPDATE_UP_TO_DATE;
      break;
    case UPDATE_STATE_WAITING:
      why = STR_MENU_UPDATE_NOT_CHECKED;
      break;
    case UPDATE_STATE_CHECKING:
      why = STR_MENU_UPDATE_CHECKING;
      break;
    case UPDATE_STATE_FAILED:
      why = STR_MENU_UPDATE_CHECK_FAILED;
      break;
    case UPDATE_STATE_OFF:
    default:
      break;
  }
  snprintf(out, len, "%s", txt(why));
}

static void textOf(const MenuRow *row, int32_t v, char *out, size_t len) {
  if (row == NULL) {
    out[0] = '\0';
    return;
  }
  /* The cursor moves an index on these rows, and every line below is written
   * about the value. */
  if (rowIsListed(row)) {
    v = valueOfIndex(row, v);
  }
  if (row->source == SRC_INFO) {
    infoText(row->id, out, len);
    return;
  }
  if (row->source == SRC_ACTION) {
    BandId scanBand = BAND_FM;
    BandScanFrom scanning;
    if (scanRowBand(row->id, &scanBand) && bandScanFrom(&scanning) &&
        scanning.scanned == scanBand) {
      /* A scan takes from ten seconds to four minutes; a row that just sat
       * there for that whole stretch would read as a radio that had hung. */
      uint16_t done = 0;
      uint16_t total = 0;
      bandScanProgress(&done, &total);
      snprintf(out, len, txt(STR_MENU_FMT_SCAN_PROGRESS), (unsigned)done,
               (unsigned)total);
      sScanCountShown = true;
      return;
    }
    /* What the band's last scan kept, so a full store, where every new
     * station found no slot, and a scan stopped part way do not read as a
     * band with nothing new. */
    BandScanResult last;
    if (scanRowBand(row->id, &scanBand) && !bandScanActive() &&
        bandScanLastResult(&last) && last.band == scanBand) {
      if (last.noRoom > 0) {
        snprintf(out, len, txt(STR_MENU_FMT_SCAN_NO_ROOM),
                 (unsigned)last.noRoom);
      } else if (!last.complete) {
        snprintf(out, len, txt(STR_MENU_FMT_SCAN_STOPPED),
                 (unsigned)last.added);
      } else {
        snprintf(out, len, txt(STR_MENU_FMT_SCAN_SAVED), (unsigned)last.added);
      }
      return;
    }
    if (row->id == ROW_UPDATE_INSTALL) {
      updateRowValue(out, len);
      return;
    }
    /* A plain action has nothing to say until it is pressed. One that asks
     * first says what it is about to do. */
    snprintf(out, len, "%s",
             row->confirm ? txt(v != 0 ? STR_COMMON_YES : STR_COMMON_NO) : "");
    return;
  }
  switch (row->id) {
    case ROW_FM_REGION:
      snprintf(out, len, "%s",
               txt(v >= 0 && v < FM_REGION_COUNT ? kRegionNames[v]
                                                 : STR_MENU_UNKNOWN_VALUE));
      return;
    case ROW_RDS_REGION:
      snprintf(out, len, "%s",
               txt(v >= 0 && v < RDS_REGION_COUNT ? kRdsRegionNames[v]
                                                  : STR_MENU_UNKNOWN_VALUE));
      return;
    case ROW_MW_SPACING:
      snprintf(out, len, txt(STR_MENU_FMT_KHZ), v == 0 ? 9 : 10);
      return;
    case ROW_THEME:
    case ROW_NIGHT_THEME:
      snprintf(out, len, "%s", themeAt(paletteThemeAt((uint8_t)v))->name);
      return;
    case ROW_DISPLAY_ROTATION:
      snprintf(out, len, "%s",
               txt(v == 1 ? STR_COMMON_ROTATION_UPSIDE_DOWN
                          : STR_COMMON_ROTATION_NORMAL));
      return;
    case ROW_LW_WIDTH:
    case ROW_MW_WIDTH:
    case ROW_SW_WIDTH:
    case ROW_FM_STEP:
    case ROW_LW_STEP:
    case ROW_MW_STEP:
    case ROW_SW_STEP:
      snprintf(out, len, txt(STR_MENU_FMT_KHZ), (int)v);
      return;
    case ROW_FM_SEEK:
    case ROW_AM_SEEK:
      snprintf(out, len, "%d", (int)v);
      return;
    case ROW_AM_SOFT_MUTE:
    case ROW_LW_SOFT_MUTE:
      /* No off here: 0 is a level like any other. */
      snprintf(out, len, txt(STR_MENU_FMT_DBUV), (int)v);
      return;
    case ROW_SQUELCH_FLOOR:
    case ROW_HIGH_CUT:
    case ROW_AM_HIGH_CUT:
    case ROW_LW_HIGH_CUT:
    case ROW_STEREO_BLEND:
    case ROW_STHI_BLEND:
      if (v == 0) {
        snprintf(out, len, "%s", txt(STR_COMMON_OFF));
      } else {
        snprintf(out, len, txt(STR_MENU_FMT_DBUV), (int)v);
      }
      return;
    case ROW_MUTE_RAMP:
      snprintf(out, len, txt(STR_MENU_FMT_MS), (int)v);
      return;
    case ROW_AUTO_OFF:
      if (v == 0) {
        snprintf(out, len, "%s", txt(STR_COMMON_OFF));
      } else {
        snprintf(out, len, txt(STR_MENU_FMT_MINUTES), (unsigned)v);
      }
      return;
    case ROW_AGC_TARGET:
      /* Off is a word, because 0 per cent modulation is a real number and
       * this is not one: it is the AGC not running at all. */
      if (v == 0) {
        snprintf(out, len, "%s", txt(STR_COMMON_OFF));
        return;
      }
      {
        /* With what it is doing now, since a target above a station's own
         * level with no boost allowed changes nothing, and only the gain
         * shows that. */
        int8_t gain = 0;
        if (radioAgcGain(&gain)) {
          snprintf(out, len, txt(STR_MENU_FMT_AGC_ROW), (unsigned)v, (int)gain);
        } else {
          snprintf(out, len, txt(STR_MENU_FMT_PERCENT), (int)v);
        }
      }
      return;
    case ROW_AGC_BOOST:
      if (v == 0) {
        snprintf(out, len, "%s", txt(STR_MENU_CUT_ONLY));
      } else {
        snprintf(out, len, txt(STR_MENU_FMT_DB), (int)v);
      }
      return;
    case ROW_BACKLIGHT:
    case ROW_BACKLIGHT_DIM:
      snprintf(out, len, txt(STR_MENU_FMT_PERCENT), (int)v);
      return;
    case ROW_LEVEL_OFFSET_FM:
    case ROW_LEVEL_OFFSET_AM:
      /* A sign on every offset but none, so +3 and -3 read apart. */
      snprintf(out, len, txt(v == 0 ? STR_MENU_FMT_DB : STR_MENU_FMT_SIGNED_DB),
               (int)v);
      return;
    case ROW_DIM_AFTER:
      /* Zero is not "zero seconds", it is "never", and the two read as
       * opposite things on a panel. */
      if (v == 0) {
        snprintf(out, len, "%s", txt(STR_MENU_NEVER));
      } else {
        snprintf(out, len, txt(STR_MENU_FMT_SECONDS), (int)v);
      }
      return;
    case ROW_BATTERY:
      snprintf(out, len, "%s",
               txt(v == BATTERY_SHOW_OFF       ? STR_COMMON_OFF
                   : v == BATTERY_SHOW_PERCENT ? STR_MENU_BATTERY_PER_CENT
                                               : STR_MENU_BATTERY_VOLTS));
      return;
    case ROW_KEY_BEEPS:
      snprintf(out, len, "%s",
               txt(v == BEEP_OFF             ? STR_COMMON_OFF
                   : v == BEEP_KEYS          ? STR_MENU_KEYS
                   : v == BEEP_KEYS_AND_LONG ? STR_MENU_KEYS_LONG
                                             : STR_MENU_EVERY_PRESS));
      return;
    case ROW_HOTSPOT:
      snprintf(out, len, "%s",
               txt(v == WIFI_HOTSPOT_ON    ? STR_COMMON_ON
                   : v == WIFI_HOTSPOT_OFF ? STR_COMMON_OFF
                                           : STR_MENU_AUTO));
      return;
    case ROW_WEB_PIN: {
      char pin[ACCESS_PIN_DIGITS + 1];
      accessPinFormat((uint32_t)v, pin);
      snprintf(out, len, "%s", pin);
      return;
    }
    case ROW_NETWORK_TIME: {
      if (v <= NETWORK_TIME_OFF) {
        snprintf(out, len, "%s", txt(STR_COMMON_OFF));
        return;
      }
      (void)clockFormatOffset((int16_t)v, out, len);
      return;
    }
    case ROW_ENCODER:
      snprintf(
          out, len, "%s",
          txt(v == ENCODER_STANDARD ? STR_MENU_STANDARD : STR_MENU_OPTICAL));
      return;
    case ROW_ENCODER_DIR:
      snprintf(out, len, "%s",
               txt(v == ENCODER_NORMAL ? STR_MENU_NORMAL : STR_MENU_REVERSED));
      return;
    case ROW_DEEMPHASIS:
      snprintf(out, len, "%s",
               txt(v == 0 ? STR_COMMON_OFF
                          : (v == 1 ? STR_MENU_DEEMPHASIS_50
                                    : STR_MENU_DEEMPHASIS_75)));
      return;
    case ROW_FM_BLANKER:
    case ROW_AM_BLANKER:
      if (v == 0) {
        snprintf(out, len, "%s", txt(STR_COMMON_OFF));
      } else {
        snprintf(out, len, txt(STR_MENU_FMT_PERCENT), (int)v);
      }
      return;
    case ROW_SQUELCH_MODE:
      snprintf(
          out, len, "%s",
          txt(v == SQUELCH_OFF ? STR_COMMON_OFF
                               : (v == SQUELCH_AUTO ? STR_MENU_AUTO
                                                    : STR_MENU_SQUELCH_MAN)));
      return;
    case ROW_DX_DWELL:
    case ROW_DX_WIDTH:
      dxNumber(row, v, out, len);
      snprintf(out + strlen(out), len - strlen(out),
               txt(STR_MENU_FMT_UNIT_AFTER), unitOf(row));
      return;
    case ROW_DX_STOP:
      snprintf(out, len, "%s",
               txt(v == DX_STOP_ANY_PI  ? STR_MENU_ANY_PI
                   : v == DX_STOP_NEVER ? STR_MENU_STOP_NEVER
                                        : STR_MENU_NEW_ONLY));
      return;
    case ROW_DX_RANGE:
      snprintf(out, len, "%s",
               txt(v == DX_RANGE_BAND     ? STR_MENU_WHOLE_BAND
                   : v == DX_RANGE_MEMORY ? STR_MENU_MEMORY_ONLY
                                          : STR_MENU_BAND_MEMORY));
      return;
    case ROW_DX_MEM_FIRST:
    case ROW_DX_MEM_LAST:
      snprintf(out, len, "%d", (int)v);
      return;
    default:
      snprintf(out, len, "%s", txt(v != 0 ? STR_COMMON_ON : STR_COMMON_OFF));
      return;
  }
}

/* What a row's value is in, when it is in anything. */
static const char *unitOf(const MenuRow *row) {
  if (row == NULL) {
    return NULL;
  }
  switch (row->id) {
    case ROW_BACKLIGHT:
    case ROW_BACKLIGHT_DIM:
    case ROW_FM_BLANKER:
    case ROW_AM_BLANKER:
      return txt(STR_COMMON_UNIT_PERCENT);
    case ROW_DIM_AFTER:
      return txt(STR_COMMON_UNIT_S);
    case ROW_AUTO_OFF:
      return txt(STR_COMMON_UNIT_MIN);
    case ROW_MUTE_RAMP:
      return txt(STR_MENU_UNIT_MS);
    case ROW_AGC_TARGET:
      return txt(STR_COMMON_UNIT_PERCENT);
    case ROW_AGC_BOOST:
      return txt(STR_COMMON_UNIT_DB);
    case ROW_SQUELCH_FLOOR:
    case ROW_HIGH_CUT:
    case ROW_AM_HIGH_CUT:
    case ROW_LW_HIGH_CUT:
    case ROW_AM_SOFT_MUTE:
    case ROW_LW_SOFT_MUTE:
    case ROW_STEREO_BLEND:
    case ROW_STHI_BLEND:
      return txt(STR_MENU_UNIT_DBUV);
    case ROW_LW_WIDTH:
    case ROW_MW_WIDTH:
    case ROW_SW_WIDTH:
    case ROW_FM_STEP:
    case ROW_LW_STEP:
    case ROW_MW_STEP:
    case ROW_SW_STEP:
    case ROW_DX_WIDTH:
      return txt(STR_COMMON_UNIT_KHZ);
    case ROW_DX_DWELL:
      return txt(STR_COMMON_UNIT_S);
    default:
      return NULL;
  }
}

/*
 * Whether a bar under the value means anything.
 *
 * True for a number between two limits. False for a row whose values are a
 * list of words, because the distance between `per cent` and `volts` is not a
 * distance.
 */
static bool hasRange(const MenuRow *row) {
  return row != NULL && row->bar;
}

/* The value on its own, with no unit on the end, for the big face. */
static void bareOf(const MenuRow *row, int32_t v, char *out, size_t len) {
  if (row == NULL) {
    out[0] = '\0';
    return;
  }
  if (row->id == ROW_DX_DWELL || row->id == ROW_DX_WIDTH) {
    dxNumber(row, v, out, len);
    return;
  }
  const int32_t real = rowIsListed(row) ? valueOfIndex(row, v) : v;
  if (unitOf(row) != NULL && real != 0) {
    snprintf(out, len, "%d", (int)real);
    return;
  }
  /* Zero has a word on most of these rows, and the word is the answer. */
  textOf(row, v, out, len);
}

/*
 * What the row is set to now, as the number the cursor moves.
 *
 * For a row with a list of legal values that is the index, not the value:
 * the knob walks the list and can never land between two entries the
 * validator would refuse.
 */
static bool valueNowRead(const MenuRow *row, const Settings *s, int32_t *out) {
  if (row == NULL || out == NULL) {
    return false;
  }
  int32_t raw = 0;
  if (row->source == SRC_RADIO) {
    if (!radioValueRead(row->id, &raw)) {
      return false;
    }
  } else if (row->source == SRC_STORED) {
    raw = storedValue(row->id, s);
  }
  *out = rowIsListed(row) ? indexOf(row, raw) : raw;
  return true;
}

/* The same, for a caller with nothing useful to do about a failed read. */
static int32_t valueNow(const MenuRow *row, const Settings *s) {
  int32_t v = 0;
  valueNowRead(row, s, &v);
  return v;
}

/* --------------------------------------------------------- what it draws */

static MenuShape shapeNow(void) {
  logCountNow();
  presetCountNow();
  MenuShape shape;
  shape.groupCount = GROUP_COUNT;
  shape.rowCount = list()->count;
  shape.kind = MENU_ROW_VALUE;
  shape.min = 0;
  shape.max = 0;
  shape.step = 1;
  shape.opensCount = 0;
  const MenuRow *row = rowAt(sMenu.row);
  if (row != NULL && row->opens != NULL) {
    shape.kind = MENU_ROW_OPENS;
    shape.opensCount = row->opens->count;
  } else if (row != NULL) {
    shape.min = rowMin(row);
    shape.max = rowMax(row);
    shape.step = row->step;
    if (rowIsListed(row)) {
      /* An index into the list, so the ends are the ends of the list. */
      shape.min = 0;
      shape.max = rowChoiceCount(row) - 1;
      shape.step = 1;
    }
    if (row->source == SRC_INFO || rowAvailable(row) == ROW_OFF_BAND) {
      /* Only the band stops an edit. A radio that could not be read is not a
       * row that cannot be edited: the edit itself refuses, with a note that
       * says the radio is busy. */
      shape.kind = MENU_ROW_INFO;
    } else if (row->source == SRC_ACTION && !row->confirm) {
      shape.kind = MENU_ROW_ACTION;
    }
  }
  return shape;
}

/*
 * Where the picker's own window starts.
 *
 * Kept between draws the same way `sTopGroup` and `sTopRow` are: so the
 * visible rows do not jump around as the cursor moves near an edge. Reset at
 * the top of every edit in `drawValue` itself, since the window is only ever
 * read while one is open.
 */
static uint8_t sTopPicker;

/* The Day Theme and Night Theme rows, which share the picker with each
 * theme's own swatches. */
static bool isThemeRow(const MenuRow *row) {
  return row->id == ROW_THEME || row->id == ROW_NIGHT_THEME;
}

/*
 * A named choice from a short list: an enum has no distance, so it gets a list
 * rather than the bar `hasRange` draws.
 *
 * Every row that reaches here is a plain range starting at 0, not one of the
 * few rows that walk a `choices` list: nothing in `kGroups` combines
 * `bar = false` with a list, so `textOf` is called with the window index
 * itself standing in for the real value, the same thing `drawValue`'s own bar
 * path already does for `sMenu.value`. A `bar = false` listed row, should
 * one ever exist, needs this widened first.
 */
static void drawPicker(const MenuRow *row, ScreenMenuValue *view) {
  /* As long as the row's own copy of each name, so no name is cut here. */
  static char text[SCREEN_MENU_PICKER_ROWS][32];
  const uint8_t total = (uint8_t)(rowMax(row) - rowMin(row) + 1);
  sTopPicker = menuWindowTop((uint8_t)sMenu.value, total,
                             SCREEN_MENU_PICKER_ROWS, sTopPicker);
  view->isPicker = true;
  view->isTheme = isThemeRow(row);
  view->pickerTotal = total;
  view->pickerTop = sTopPicker;
  for (uint8_t i = 0; i < SCREEN_MENU_PICKER_ROWS; i++) {
    const uint8_t at = (uint8_t)(sTopPicker + i);
    if (at >= total) {
      view->picker[i].name = NULL;
      continue;
    }
    textOf(row, (int32_t)at, text[i], sizeof(text[0]));
    view->picker[i].name = text[i];
    view->picker[i].isCursor = at == (uint8_t)sMenu.value;
    view->picker[i].isSaved = at == (uint8_t)sMenu.was;
    view->picker[i].themeIndex = paletteThemeAt(at);
  }
}

/* A limit under the bar with its unit, "5 %", so the ends say what they
 * are in the way the value above them does. */
static void limitText(const MenuRow *row, int32_t v, char *out, size_t len) {
  /* Rows whose value is not a plain number: the ends are written the way
   * the value is, "+14:00" not the 840 minutes it is stored as, "+15 dB"
   * with its sign, and Off rather than 0 min. */
  if (row->id == ROW_DX_DWELL || row->id == ROW_DX_WIDTH ||
      row->id == ROW_NETWORK_TIME || row->id == ROW_LEVEL_OFFSET_FM ||
      row->id == ROW_LEVEL_OFFSET_AM || row->id == ROW_AUTO_OFF) {
    textOf(row, v, out, len);
    return;
  }
  const char *unit = unitOf(row);
  if (unit != NULL) {
    snprintf(out, len, txt(STR_MENU_FMT_NUMBER_UNIT), (int)v, unit);
  } else {
    snprintf(out, len, "%d", (int)v);
  }
}

/*
 * Whether a row waits for the press before it acts. The rest apply as the
 * knob turns, so a brightness can be judged by looking at it. Rotation
 * cannot: it turns the whole screen, so the cursor would start moving the
 * other way on the glass while the person is still choosing. The two
 * themes cannot either: the picker would repaint in each theme the cursor
 * passes, and the cursor itself would change colour on every click, so the
 * list stays in the theme drawn and each row's swatches show its own.
 * They act once, when kept.
 */
static bool actsOnKeep(const MenuRow *row) {
  return row != NULL &&
         (row->id == ROW_DISPLAY_ROTATION || row->id == ROW_WEB_PIN ||
          row->id == ROW_HOTSPOT || row->id == ROW_WEB_SERVER ||
          row->id == ROW_WIFI || isThemeRow(row));
}

/* Whether the Web PIN is being set, where a turn, a press and a digit key go
 * to its own digits rather than to the menu's one value. */
static bool pinEditing(void) {
  const MenuRow *row = rowAt(sMenu.row);
  return menuIsEditing(&sMenu) && row != NULL && row->id == ROW_WEB_PIN;
}

/* The PIN editor's own view: the six digits on the panel in place of the
 * value and the bar. */
static void drawPinValue(ScreenMenuValue *view) {
  static char digits[ACCESS_PIN_DIGITS + 1];
  static char place[24];
  accessPinFormat(accessPinEditValue(&sPin), digits);
  snprintf(place, sizeof(place), txt(STR_MENU_FMT_DIGIT_OF),
           (unsigned)(sPin.at + 1), (unsigned)ACCESS_PIN_DIGITS);
  view->isDigits = true;
  view->digits = digits;
  view->digitAt = sPin.at;
  view->digitPlace = place;
  view->label = txt(STR_MENU_NEW_PIN);
  view->hasRange = false;
  view->note = sNote;
}

/* A value on the bar screen turned away from the one saved. Not the Web
 * PIN, saved digit by digit, nor a yes or no confirmation, nor a picker,
 * whose tick already marks the saved choice. */
static bool barValueChanged(const MenuRow *row) {
  return row != NULL && hasRange(row) && row->source != SRC_ACTION &&
         row->id != ROW_WEB_PIN && sMenu.value != sMenu.was;
}

static void drawValue(void) {
  const MenuRow *row = rowAt(sMenu.row);
  if (row == NULL) {
    return;
  }
  static char bare[20];
  static char lowText[12];
  static char highText[12];
  ScreenMenuValue view;
  memset(&view, 0, sizeof(view));

  view.name = pathOf(txt(list()->name), txt(row->name));
  bareOf(row, sMenu.value, bare, sizeof(bare));
  view.value = bare;
  const int32_t real =
      rowIsListed(row) ? valueOfIndex(row, sMenu.value) : sMenu.value;
  view.unit = real != 0 ? unitOf(row) : NULL;
  view.hasRange = hasRange(row);
  const bool listed = rowIsListed(row);
  const uint8_t choiceCount = listed ? rowChoiceCount(row) : 0;
  view.min = listed ? 0 : rowMin(row);
  view.max = listed ? choiceCount - 1 : rowMax(row);
  view.at = sMenu.value;
  /* The numbers under the ends of the bar are what the ends mean, not where
   * the cursor is allowed to go. On a list row those are the first and last
   * values, not 0 and the length of the list. */
  limitText(row, listed ? valueOfIndex(row, 0) : rowMin(row), lowText,
            sizeof(lowText));
  limitText(row, listed ? valueOfIndex(row, choiceCount - 1) : rowMax(row),
            highText, sizeof(highText));
  view.minText = lowText;
  view.maxText = highText;
  /* Only what the screen cannot say on its own: what the last press did,
   * that a value turned to is not saved until the press, or that the
   * setting waits for a restart. */
  view.note = sNote != NULL          ? sNote
              : barValueChanged(row) ? txt(STR_MENU_NOTE_NOT_SAVED)
              : row->needsRestart    ? txt(STR_MENU_HINT_RESTART)
                                     : NULL;
  /* The AGC's two rows say what it does as the knob turns, the gain taking
   * a few seconds to follow. */
  static char gainNote[24];
  int8_t gain = 0;
  if (view.note == NULL &&
      (row->id == ROW_AGC_TARGET || row->id == ROW_AGC_BOOST) &&
      radioAgcGain(&gain)) {
    snprintf(gainNote, sizeof(gainNote), txt(STR_MENU_FMT_AGC_GAIN_NOW),
             (int)gain);
    view.note = gainNote;
  }

  /*
   * The third shape, alongside the list and the bar: a named choice out of
   * a short list rather than a number or a word standing alone. Every
   * confirmation (`Restart`) is `bar = false` too and is kept on
   * the plain word screen instead, since "no" and "yes" are not a list to
   * scroll.
   */
  if (row->id == ROW_WEB_PIN) {
    drawPinValue(&view);
  } else if (!hasRange(row) && row->source != SRC_ACTION) {
    drawPicker(row, &view);
  }

  screenMenuValueShow(&view);
  screenTaskMenuDrawn();
}

static void draw(void) {
  if (sMenu.level == MENU_EDIT) {
    drawValue();
    return;
  }
  ScreenMenu view;
  memset(&view, 0, sizeof(view));

  logCountNow();
  presetCountNow();
  const bool onGroups = sMenu.level == MENU_GROUPS;
  const MenuGroup *shown = list();
  const uint8_t count = onGroups ? GROUP_COUNT : shown->count;
  const uint8_t cursor = onGroups ? sMenu.group : sMenu.row;
  /* Its own window, so a sub-group scrolled to its end gives the group back
   * with the window where it was left. */
  uint8_t *top = onGroups ? &sTopGroup : sMenu.inSub ? &sTopSub : &sTopRow;
  *top = menuWindowTop(cursor, count, SCREEN_MENU_ROWS, *top);

  view.total = count;
  view.top = *top;
  view.title = onGroups      ? txt(STR_MENU_TITLE)
               : sMenu.inSub ? pathOf(txt(group()->name), txt(shown->name))
                             : txt(shown->name);

  for (uint8_t i = 0; i < SCREEN_MENU_ROWS; i++) {
    const uint8_t at = (uint8_t)(*top + i);
    if (at >= count) {
      break;
    }
    view.rows[i].selected = at == cursor;
    if (onGroups) {
      view.rows[i].name = txt(kGroups[at].name);
      snprintf(sValueText[i], sizeof(sValueText[0]), "%u",
               (unsigned)kGroups[at].count);
      view.rows[i].value = sValueText[i];
      view.rows[i].opens = true;
      continue;
    }
    if (shown == &sLogGroup) {
      if (count != sLogRowsCount) {
        sLogRowsCount = count;
        logRowsForget();
      }
      /* Newest first, and the logbook counts from the oldest. */
      const LogRow *r = logRow((uint16_t)(count - 1 - at));
      view.rows[i].name = r->name;
      view.rows[i].value = r->value[0] != '\0' ? r->value : NULL;
      continue;
    }
    if (shown == &sPresetGroup) {
      presetRowText(at, sPresetName[i], sizeof(sPresetName[0]), sValueText[i],
                    sizeof(sValueText[0]));
      view.rows[i].name = sPresetName[i];
      view.rows[i].value = sValueText[i][0] != '\0' ? sValueText[i] : NULL;
      continue;
    }
    const MenuRow *row = &shown->rows[at];
    if (row->opens != NULL) {
      view.rows[i].name = txt(row->name);
      snprintf(sValueText[i], sizeof(sValueText[0]), "%u",
               (unsigned)row->opens->count);
      view.rows[i].value = sValueText[i];
      view.rows[i].opens = true;
      continue;
    }
    /* On its own hotspot, the network named is the hotspot, which a person
     * joins to reach the radio rather than one the radio joined. */
    view.rows[i].name =
        txt(row->id == ROW_NET_NAME && wifiState() == WIFI_STATE_ACCESS_POINT
                ? STR_MENU_HOTSPOT_NAME
                : row->name);
    if (row->id == ROW_UPDATE_INSTALL && updateCheckVersion() != NULL) {
      static char sUpdateName[32];
      snprintf(sUpdateName, sizeof(sUpdateName), txt(STR_MENU_FMT_UPDATE_TO),
               updateCheckVersion());
      view.rows[i].name = sUpdateName;
    }
    view.rows[i].secret = row->id == ROW_WEB_PIN;
    const RowAvailable here = rowAvailable(row);
    if (here == ROW_OFF_BAND) {
      /* Not a value, and not a blank either. A row that goes empty on one
       * band reads as a radio that has lost a setting. */
      snprintf(sValueText[i], sizeof(sValueText[0]), "%s",
               txt(STR_COMMON_FM_ONLY));
    } else if (here == ROW_UNKNOWN) {
      /* A dash, which is this project's word for cannot say. It is not the
       * same as off and it is not the same as `FM only`. */
      snprintf(sValueText[i], sizeof(sValueText[0]), "%s",
               txt(STR_COMMON_DASH));
    } else {
      textOf(row, valueNow(row, sLive), sValueText[i], sizeof(sValueText[0]));
    }
    view.rows[i].value = sValueText[i][0] != '\0' ? sValueText[i] : NULL;
  }

  /* What the last press on the cursor's row did, such as a value that could
   * not be stored, drawn on that row. */
  view.note = sNote;

  screenMenuShow(&view);
  screenTaskMenuDrawn();
}

/* ------------------------------------------------------------ the doings */

/* Whether DX mode could open now; if not, the menu stays up with a note
 * saying why. */
static bool dxCanOpenOrNote(void) {
  switch (screenTaskDxCanOpen()) {
    case SCREEN_DX_OPEN:
    case SCREEN_DX_PANEL_BUSY:
      return true;
    case SCREEN_DX_RADIO_BUSY:
      sNote = txt(STR_MENU_NOTE_RADIO_BUSY);
      return false;
    case SCREEN_DX_NOT_FM:
    default:
      sNote = txt(STR_MENU_NOTE_SWITCH_TO_FM);
      return false;
  }
}

static void fire(const MenuRow *row) {
  if (row == NULL) {
    return;
  }
  switch (row->id) {
    case ROW_LOG_ENTRY: {
      /* Tunes to the entry with the command `POST /api/tune` sends, after the
       * check a preset gets: an entry whose frequency the band plan now gives
       * to another band would land there. The menu stays open, as on the
       * preset rows. Newest first, and the logbook counts from the oldest. */
      const uint16_t count = logbookFsCount();
      LogbookEntry e;
      BandPlanConfig plan;
      BandId band = BAND_COUNT;
      if (sMenu.row >= count ||
          !logbookFsEntryAt((uint16_t)(count - 1 - sMenu.row), &e) ||
          !radioTaskPlan(&plan)) {
        sNote = txt(STR_MENU_NOTE_RADIO_BUSY);
        return;
      }
      if (!bandForFrequency(&plan, e.freqKHz, &band) || band != e.band) {
        sNote = txt(STR_MENU_NOTE_OFF_PLAN);
        return;
      }
      RadioCommand cmd = {};
      cmd.kind = RADIO_TUNE;
      cmd.freqKHz = e.freqKHz;
      if (!radioPost(&cmd)) {
        sNote = txt(STR_MENU_NOTE_RADIO_BUSY);
        return;
      }
      Serial.printf("[menu] tuned to log entry at %u kHz\n",
                    (unsigned)e.freqKHz);
      return;
    }
    case ROW_PRESET_ENTRY: {
      /* Tunes to the preset with the same check and the same command as
       * `POST /api/presets` with do=recall. The menu stays open, so presets
       * can be heard one after another. */
      MemoryChannel c;
      const int slot = memoryStoreNthUsed((int)sMenu.row, &c);
      if (slot == MEMORY_NO_SLOT) {
        /* Cleared from the browser since the row was drawn: the redraw after
         * this press shows the list as it is now. */
        return;
      }
      BandPlanConfig plan;
      if (!radioTaskPlan(&plan)) {
        sNote = txt(STR_MENU_NOTE_RADIO_BUSY);
        return;
      }
      if (!memoryChannelTunable(&c, &plan)) {
        Serial.printf("[menu] preset %d is not on the band plan\n", slot + 1);
        sNote = txt(STR_MENU_NOTE_OFF_PLAN);
        return;
      }
      RadioCommand cmd = {};
      cmd.kind = RADIO_RECALL;
      cmd.memorySlot = slot;
      if (!radioPost(&cmd)) {
        sNote = txt(STR_MENU_NOTE_RADIO_BUSY);
        return;
      }
      Serial.printf("[menu] tuned to preset %d\n", slot + 1);
      return;
    }
    case ROW_FM_SCAN:
    case ROW_MW_SCAN:
    case ROW_SW_SCAN:
    case ROW_LW_SCAN: {
      BandId band = BAND_FM;
      if (!scanRowBand(row->id, &band)) {
        return;
      }
      if (bandScanActive()) {
        /* A press on the running scan's own row stops it, which is the one
         * way to stop it without leaving the menu. */
        BandScanFrom from;
        if (bandScanFrom(&from) && from.scanned == band) {
          bandScanStop();
          Serial.printf("[menu] %s scan stopped\n", bandName(band));
        }
        return;
      }
      if (bandScanStart(band) != BAND_SCAN_STARTED) {
        Serial.println(F("[menu] scan refused, radio busy"));
        sNote = txt(STR_MENU_NOTE_RADIO_BUSY);
        return;
      }
      Serial.printf("[menu] %s scan started\n", bandName(band));
      return;
    }
    case ROW_DX_LEARN: {
      /* Out of the menu and into DX mode's Scanner, where the pass runs and can
       * be watched, and any key stops it. */
      RadioSnapshot now;
      if (!radioGetSnapshot(&now)) {
        sNote = txt(STR_MENU_NOTE_RADIO_BUSY);
        return;
      }
      if (bandModulation(now.settings.band) != MODULATION_FM) {
        sNote = txt(STR_MENU_NOTE_SWITCH_TO_FM);
        return;
      }
      if (!radioRdsEnabled()) {
        sNote = txt(STR_MENU_NOTE_SWITCH_RDS_ON);
        return;
      }
      menuTaskClose();
      const DxScanPress r = screenTaskDxLearnLocals();
      if (r != DX_SCAN_PRESS_RUNNING) {
        /* On whichever screen is up now, since the menu has gone. */
        screenTaskLogConfirm(dxTaskPressText(r));
      }
      return;
    }
    case ROW_GOTO_BANDWIDTH:
      menuTaskClose();
      if (!screenTaskBwOpen()) {
        Serial.println(F("[menu] the bandwidth page did not open"));
      }
      return;
    case ROW_CALIBRATE_TOUCH:
      menuTaskClose();
      if (!screenTaskTouchCalOpen()) {
        Serial.println(F("[menu] the touch calibration did not open"));
      }
      return;
    case ROW_GOTO_RDS:
      menuTaskClose();
      screenTaskRdsToggle();
      return;
    case ROW_GOTO_DX: {
      if (!dxCanOpenOrNote()) {
        return;
      }
      menuTaskClose();
      if (screenTaskDxOpen() != SCREEN_DX_OPEN) {
        Serial.println(F("[menu] DX mode did not open"));
      }
      return;
    }
    case ROW_DX_START_SCAN: {
      /* What `POST /api/dx` with on=1 and scan=1 does: DX mode open, the
       * scanner's own press, and its page up once it runs. The menu closes
       * DX mode as it opens, so it is opened here every time. */
      if (!dxCanOpenOrNote()) {
        return;
      }
      menuTaskClose();
      if (screenTaskDxOpen() != SCREEN_DX_OPEN) {
        screenTaskLogConfirm(dxTaskPressText(DX_SCAN_PRESS_PANEL_BUSY));
        return;
      }
      const DxScanPress r = dxTaskScanPress();
      if (r == DX_SCAN_PRESS_RUNNING || r == DX_SCAN_PRESS_FINISHED) {
        screenTaskDxShowScanner();
      } else {
        screenTaskLogConfirm(dxTaskPressText(r));
      }
      return;
    }
    case ROW_GOTO_BAND:
      /* On the radio screen, which shows the band it reached. */
      menuTaskClose();
      inputNextBand();
      return;
    case ROW_GOTO_LOG:
      /* On the radio screen, which shows the Logged mark. */
      menuTaskClose();
      inputWriteLogEntry();
      return;
    case ROW_GOTO_SLEEP:
      /* The same reason Restart gives: the wake is a restart, and a restart
       * before the self check passes rolls the new firmware back. */
      if (!sleepTaskNow()) {
        sNote = txt(STR_MENU_NOTE_UPDATE_ON_TRIAL);
        return;
      }
      menuTaskClose();
      return;
    case ROW_UPDATE_INSTALL:
      if (!menuTaskOpenUpdateOffer()) {
        sNote = txt(STR_MENU_NOTE_NO_UPDATE);
      }
      return;
    case ROW_RESTART:
      /* The same reason `/reboot` gives: a restart before the self check
       * passes rolls the new firmware back. */
      if (rollbackPending()) {
        sNote = txt(STR_MENU_NOTE_UPDATE_ON_TRIAL);
        return;
      }
      Serial.println(F("[menu] restarting, asked for from the menu"));
      restartReasonNote(RESTART_WHY_ASKED);
      settingsTaskRestart();
      return;
    default:
      return;
  }
}

static bool applyPending(const MenuRow *row) {
  if (row == NULL) {
    return false;
  }
  if (row->source == SRC_ACTION) {
    /* A confirmation has nothing to apply as it turns. */
    return true;
  }
  const int32_t real =
      rowIsListed(row) ? valueOfIndex(row, sMenu.value) : sMenu.value;
  if (row->source == SRC_RADIO) {
    return radioSet(row->id, real);
  }
  if (row->source == SRC_STORED) {
    storedSet(row->id, &sPending, real);
    settingsApplyLive(&sPending);
    return true;
  }
  return false;
}

/* Write what the edit ended on, once, through the one call that counts it. */
static void keepEdit(const MenuRow *row) {
  if (sLive == NULL || row == NULL) {
    return;
  }
  if (row->source != SRC_STORED) {
    /* A radio row is the radio's. It is already applied, and the automatic
     * save copies it into the settings once the dial stops moving. Writing it
     * here as well would be the same value written twice from two places. */
    return;
  }
  /*
   * Built from the settings as they are this moment, with only this row's
   * value put into them.
   *
   * `sPending` was copied when the edit began, and the automatic save can
   * have written something else in between: sitting on a row for ten seconds
   * after changing the squelch was enough to put the old squelch mode back
   * into flash. One row changed, so one field changes.
   */
  const int32_t real =
      rowIsListed(row) ? valueOfIndex(row, sMenu.value) : sMenu.value;
  sPending = *sLive;
  storedSet(row->id, &sPending, real);
  if (!settingsTaskStore(&sPending)) {
    sNote = txt(STR_MENU_NOTE_NOT_STORED);
    settingsApplyLive(sLive);
    return;
  }
  if (row->id == ROW_WEB_PIN) {
    /* Stored first, then made the PIN, the order the browser keeps, so a
     * radio that loses power between the two still has the PIN it says. */
    webPinChanged((uint32_t)real);
  }
}

/* Put back what the row was before the edit started. */
static void undoEdit(const MenuRow *row) {
  if (row == NULL) {
    return;
  }
  const int32_t real =
      rowIsListed(row) ? valueOfIndex(row, sMenu.value) : sMenu.value;
  if (row->source == SRC_RADIO) {
    if (!radioSet(row->id, real)) {
      /* The screen has already gone back to the old number and the tuner is
       * still on the new one. Saying so is all that can be done here, and it
       * beats a radio that quietly keeps a value somebody cancelled. */
      sNote = txt(STR_MENU_NOTE_NOT_PUT_BACK);
    }
    return;
  }
  if (row->source == SRC_STORED && sLive != NULL) {
    settingsApplyLive(sLive);
  }
}

/* ----------------------------------------------------------- the outside */

void menuTaskPoll(void) {
  const uint32_t nowMs = millis();
  static uint32_t sCpuAtMs = 0;
  const bool second = (uint32_t)(nowMs - sCpuAtMs) >= 1000;
  if (second) {
    sCpuAtMs = nowMs;
    cpuTick();
  }
  if (!menuIsOpen(&sMenu)) {
    return;
  }
  if (sMenu.level == MENU_EDIT) {
    /* The AGC's gain follows a new target over a few seconds. */
    const MenuRow *row = rowAt(sMenu.row);
    if (second && !pinEditing() && row != NULL &&
        (row->id == ROW_AGC_TARGET || row->id == ROW_AGC_BOOST)) {
      draw();
    }
    return;
  }
  /* Most rows only change when the knob is turned or pressed, and `draw`
   * runs from those calls already. Three things move on their own: a scan
   * and the squelch level under the volume pot, redrawn four times a second
   * so they keep up, and the readings in Diagnostics and Network Info and
   * the AGC's gain in Audio, redrawn once a second, right after the CPU
   * sample, so they show the radio as it is now. */
  static uint32_t sScanAtMs = 0;
  /* A scan started or ended from the browser, with the menu already open,
   * changes a row's value too: the count while it runs and what it saved
   * after. */
  static bool sScanWasActive = false;
  if (bandScanActive() != sScanWasActive) {
    sScanWasActive = !sScanWasActive;
    sScanCountShown = true;
  }
  if (sScanCountShown && (uint32_t)(nowMs - sScanAtMs) >= 250) {
    sScanAtMs = nowMs;
    /* Set again by the draw while the scan runs and its row is on screen. */
    sScanCountShown = false;
    draw();
    return;
  }
  const MenuGroup *g = list();
  static uint32_t sSquelchAtMs = 0;
  if (sMenu.level == MENU_ROWS && g->rows == kSquelchRows &&
      (uint32_t)(nowMs - sSquelchAtMs) >= 250) {
    sSquelchAtMs = nowMs;
    draw();
    return;
  }
  if (second && sMenu.level == MENU_ROWS &&
      (g->rows == kDiagnosticRows || g->rows == kNetInfoRows ||
       g->rows == kAudioRows)) {
    draw();
  }
}

void menuTaskBegin(Settings *live) {
  sLive = live;
  menuReset(&sMenu);
  sTopGroup = 0;
  sTopRow = 0;
  sTopSub = 0;
}

/* Where the menu was when it last closed, so it opens there again until the
 * radio restarts. */
static Menu sLeft;
static bool sLeftKept = false;

static void keepWhereLeft(void) {
  sLeft = sMenu;
  sLeftKept = true;
}

/* The choice of bands for a typed number, menuTaskOpenChoice, or the offer
 * of a newer release, menuTaskOpenUpdateOffer. */
static struct {
  bool active;
  bool update; /* The offer: Update and Later. */
  uint8_t count;
  uint8_t cursor;
  char typed[INPUT_DIGITS_MAX + 1];
  BandTypedReading reading[SCREEN_MENU_ROWS];
} sChoice;

static void drawChoice(void) {
  static char title[32];
  ScreenMenu view;
  memset(&view, 0, sizeof(view));
  if (sChoice.update) {
    /* What a person would want to know before saying yes: what they have,
     * what they would get, how big it is, and that nothing they set is
     * lost. */
    const char *version = updateCheckVersion();
    char mb[12];
    updateFormatMegabytes(updateCheckSize(), mb, sizeof(mb));
    snprintf(sValueText[0], sizeof(sValueText[0]), txt(STR_MENU_FMT_MEGABYTES),
             mb);
    ScreenMenuDialog dialog;
    memset(&dialog, 0, sizeof(dialog));
    dialog.title = txt(STR_MENU_UPDATE_TITLE);
    dialog.label[0] = txt(STR_MENU_UPDATE_THIS_RADIO);
    dialog.value[0] = FIRMWARE_VERSION;
    dialog.label[1] = txt(STR_MENU_UPDATE_NEW_VERSION);
    dialog.value[1] = version != NULL ? version : "";
    dialog.label[2] = txt(STR_MENU_UPDATE_DOWNLOAD);
    dialog.value[2] = sValueText[0];
    dialog.label[3] = txt(STR_MENU_UPDATE_SETTINGS);
    dialog.value[3] = txt(STR_MENU_UPDATE_KEPT);
    dialog.button[0] = txt(STR_MENU_UPDATE_NOW);
    dialog.button[1] = txt(STR_MENU_LATER);
    dialog.cursor = sChoice.cursor;
    screenMenuDialogShow(&dialog);
    return;
  }
  snprintf(title, sizeof(title), txt(STR_MENU_FMT_TUNE_TO), sChoice.typed);
  view.title = title;
  for (uint8_t i = 0; i < sChoice.count; i++) {
    const BandTypedReading *r = &sChoice.reading[i];
    (void)bandFormatWithUnit(r->band, r->freqKHz, sValueText[i],
                             sizeof(sValueText[0]));
    view.rows[i].name = bandLongName(r->band);
    view.rows[i].value = sValueText[i];
    view.rows[i].selected = i == sChoice.cursor;
  }
  screenMenuShow(&view);
}

static void choiceEnd(void) {
  sChoice.active = false;
  screenTaskMenuEnd();
}

bool menuTaskOpenChoice(const char *typed, const BandTypedReading *readings,
                        uint8_t count) {
  gestureBegins();
  if (sLive == NULL || menuIsOpen(&sMenu) || sChoice.active ||
      readings == NULL || count < 2 || !screenTaskMenuBegin()) {
    return false;
  }
  sChoice.active = true;
  sChoice.update = false;
  sChoice.count = count < SCREEN_MENU_ROWS ? count : SCREEN_MENU_ROWS;
  sChoice.cursor = 0;
  snprintf(sChoice.typed, sizeof(sChoice.typed), "%s",
           typed != NULL ? typed : "");
  memcpy(sChoice.reading, readings, sizeof(readings[0]) * sChoice.count);
  drawChoice();
  return true;
}

/*
 * The offer of a newer release: at start, when the check finds one, and from
 * the System row. From the row it takes over the menu's screen, so the menu
 * shuts and the offer is drawn where it was, with no fade between them.
 * Later, or a minute with no answer, leaves it in the System group.
 */
bool menuTaskOpenUpdateOffer(void) {
  if (sLive == NULL || sChoice.active || updateCheckVersion() == NULL) {
    return false;
  }
  if (menuIsOpen(&sMenu)) {
    keepWhereLeft();
    (void)menuClose(&sMenu);
  } else if (!screenTaskMenuBegin()) {
    return false;
  }
  sChoice.active = true;
  sChoice.update = true;
  sChoice.count = 2;
  sChoice.cursor = 0;
  /* Shown now, so an offer still waiting from the check is not shown again
   * after Later. */
  (void)updateCheckTakeOffer();
  drawChoice();
  return true;
}

bool menuTaskIsOpen(void) {
  return menuIsOpen(&sMenu) || sChoice.active;
}

void menuTaskOpen(void) {
  gestureBegins();
  if (sLive == NULL || menuIsOpen(&sMenu) || sChoice.active) {
    return;
  }
  if (menuOpen(&sMenu) != MENU_OPENED) {
    return;
  }
  if (sLeftKept) {
    /* Where it was left, with the windows it had; a row a shrunken list no
     * longer has falls back to its first. */
    menuRestore(&sMenu, &sLeft);
    logCountNow();
    presetCountNow();
    if (sMenu.level == MENU_ROWS && list()->count == 0) {
      /* The list is empty now: one level up, on the row that opens it. */
      if (sMenu.inSub) {
        sMenu.inSub = false;
        sMenu.row = sMenu.sub;
      } else {
        sMenu.level = MENU_GROUPS;
      }
    }
    if (sMenu.level == MENU_ROWS && sMenu.row >= list()->count) {
      sMenu.row = 0;
    }
  } else {
    sTopGroup = 0;
    sTopRow = 0;
    sTopSub = 0;
  }
  if (!screenTaskMenuBegin()) {
    /* No screen, so no menu. A menu being driven by a knob with nothing on
     * the panel is a radio changing settings nobody can see. */
    menuClose(&sMenu);
    return;
  }
  /* The screen exists the moment that call returns, because the menu changes
   * with no fade, so it is drawn here rather than waiting for a swap to
   * finish. */
  draw();
}

void menuTaskClose(void) {
  sNote = NULL;
  if (sChoice.active) {
    choiceEnd();
    return;
  }
  if (!menuIsOpen(&sMenu)) {
    return;
  }
  const MenuRow *row = rowAt(sMenu.row);
  keepWhereLeft();
  if (menuClose(&sMenu) == MENU_EDIT_UNDONE) {
    /* The value was never accepted, so the radio goes back to what it was. */
    undoEdit(row);
  }
  screenTaskMenuEnd();
}

void menuTaskTurn(int32_t clicks) {
  gestureBegins();
  if (sChoice.active && clicks != 0) {
    /* One click, one row, stopping at the ends, as every list here does. */
    if (clicks < 0 && sChoice.cursor > 0) {
      sChoice.cursor--;
    } else if (clicks > 0 && sChoice.cursor + 1 < sChoice.count) {
      sChoice.cursor++;
    }
    drawChoice();
    return;
  }
  if (!menuIsOpen(&sMenu) || clicks == 0) {
    return;
  }
  if (pinEditing()) {
    accessPinEditTurn(&sPin, clicks);
    sMenu.value = (int32_t)accessPinEditValue(&sPin);
    draw();
    return;
  }
  const MenuShape shape = shapeNow();
  const int32_t before = sMenu.value;
  const MenuResult what = menuTurn(&sMenu, clicks, &shape);
  if (what == MENU_NOTHING) {
    return;
  }
  if (what == MENU_EDIT_MOVED && !actsOnKeep(rowAt(sMenu.row))) {
    /* Applied as it turns, so a brightness can be chosen by looking at the
     * panel and a blanker by listening. Nothing is stored yet. */
    if (!applyPending(rowAt(sMenu.row))) {
      /* The write was refused, which on a radio row means the task was busy.
       * The number goes back rather than standing on the screen as though it
       * had been set. */
      sMenu.value = before;
    }
  }
  draw();
}

void menuTaskPress(void) {
  gestureBegins();
  if (sChoice.active && sChoice.update) {
    /* Update, or Later. Either way the offer shuts; the install runs on
     * the loop's next pass and takes the panel for its progress. */
    const bool now = sChoice.cursor == 0;
    choiceEnd();
    if (now) {
      const UpdateInstallAsk ask = updateCheckInstall();
      if (ask != UPDATE_INSTALL_STARTING) {
        Serial.printf("[menu] update not started, reason %d\n", (int)ask);
      }
    }
    return;
  }
  if (sChoice.active) {
    /* The command ENTER on a number that needed no choice sends, which
     * changes band as well as tuning. */
    RadioCommand cmd = {};
    cmd.kind = RADIO_TUNE;
    cmd.freqKHz = sChoice.reading[sChoice.cursor].freqKHz;
    if (!radioPost(&cmd)) {
      Serial.println(F("[menu] typed choice not tuned, radio busy"));
    }
    choiceEnd();
    return;
  }
  if (!menuIsOpen(&sMenu)) {
    return;
  }
  /* Before the sixth digit a press only moves on; the sixth keeps the PIN
   * the way any row's press keeps its value. */
  if (pinEditing() && !accessPinEditNext(&sPin)) {
    draw();
    return;
  }
  const MenuShape shape = shapeNow();
  const MenuRow *row = rowAt(sMenu.row);
  const MenuResult what = menuPress(&sMenu, &shape);
  switch (what) {
    case MENU_EDIT_STARTED: {
      if (sLive == NULL) {
        break;
      }
      sPending = *sLive;
      /* A window left over from the last row edited is not this row's own
       * window: it might not even hold this row's cursor. */
      sTopPicker = 0;
      if (row != NULL && row->confirm) {
        /* A confirmation starts at no, whatever it said last time. */
        sMenu.value = 0;
        sMenu.was = 0;
        break;
      }
      int32_t at = 0;
      if (!valueNowRead(row, sLive, &at)) {
        /* The radio could not be read, so there is nothing to edit from.
         * Starting anyway would begin at zero, and a cancel would then write
         * that zero to the tuner. */
        menuBack(&sMenu);
        Serial.println(F("[menu] the radio is busy, try that row again"));
        sNote = txt(STR_MENU_NOTE_RADIO_BUSY);
        break;
      }
      sMenu.value = at;
      sMenu.was = at;
      if (row != NULL && row->id == ROW_WEB_PIN) {
        accessPinEditBegin(&sPin, (uint32_t)at);
      }
      break;
    }
    case MENU_EDIT_KEPT:
      if (row != NULL && row->confirm) {
        if (sMenu.value != 0) {
          fire(row);
        }
      } else {
        keepEdit(row);
      }
      break;
    case MENU_FIRED:
      fire(row);
      break;
    case MENU_ENTERED_SUB:
      /* Read afresh: something may have been logged since it was last open. */
      logRowsForget();
      break;
    case MENU_NOTHING:
      return;
    default:
      break;
  }
  /* An action can leave the menu, Learn locals for DX mode, and then there
   * is no menu to draw. */
  if (!menuIsOpen(&sMenu)) {
    return;
  }
  draw();
}

void menuTaskBack(void) {
  gestureBegins();
  if (sChoice.active) {
    choiceEnd();
    return;
  }
  if (!menuIsOpen(&sMenu)) {
    return;
  }
  const MenuRow *row = rowAt(sMenu.row);
  if (sMenu.level == MENU_GROUPS) {
    keepWhereLeft(); /* This back shuts it. */
  }
  /* A value turned to and then left: its row says it was not saved. */
  const bool changed = sMenu.level == MENU_EDIT && barValueChanged(row);
  const MenuResult what = menuBack(&sMenu);
  switch (what) {
    case MENU_EDIT_UNDONE:
      undoEdit(row);
      if (changed && sNote == NULL) {
        sNote = txt(STR_MENU_NOTE_NOT_SAVED);
      }
      break;
    case MENU_CLOSED_NOW:
      screenTaskMenuEnd();
      return;
    case MENU_NOTHING:
      return;
    default:
      break;
  }
  draw();
}

bool menuTaskDigit(uint8_t digit) {
  gestureBegins();
  if (!menuIsOpen(&sMenu) || !pinEditing() || digit > 9) {
    return false;
  }
  const bool last = accessPinEditType(&sPin, digit);
  sMenu.value = (int32_t)accessPinEditValue(&sPin);
  if (last) {
    menuTaskPress();
  } else {
    draw();
  }
  return true;
}
