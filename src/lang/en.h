/*
 * Every text the panel shows, in English. One line per text: the id, the text
 * as the code prints it, and where on the panel it is used.
 *
 * The code never quotes a text; it says txt(STR_ID). core/strings.h builds the
 * StrId enum from this list and core/strings.c builds the table, so the two can
 * never drift. A new text is a new line here. Another language is another file
 * with the same ids and its own texts.
 *
 * Ids are named by area: MENU, RDS, RTP, DX, BW, BOOT, RADIO, RECOVERY, RESET,
 * TUNER, PTY, COVERAGE, LANG, DAY, MONTH, DATE, THEME, BAND, ABOUT, LOG, and
 * COMMON for a text shown in more than one area. FMT marks a printf format;
 * its arguments are in the note.
 *
 * \xC2\xB7 is the middle dot.
 */
#pragma once

// clang-format off
#define STRINGS(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "FM Station Scan", "Menu row name, Stations group") \
  X(MENU_SCAN_MW, "AM Station Scan", "Menu row name, Stations group: the scan of medium wave") \
  X(MENU_SCAN_SW, "SW Station Scan", "Menu row name, Stations group") \
  X(MENU_SCAN_LW, "LW Station Scan", "Menu row name, Stations group") \
  X(MENU_SQUELCH, "Squelch Mode", "Menu row name, Audio > Squelch") \
  X(MENU_SQUELCH_LEVEL, "Squelch Level", "Menu row name, Audio > Squelch") \
  X(MENU_SQUELCH_FLOOR, "Squelch Floor", "Menu row name, Audio > Squelch") \
  X(MENU_SQUELCH_GROUP, "Squelch", "Sub-group name, Audio > Squelch") \
  X(MENU_VOLUME_AGC, "Volume AGC", "Menu row name, Audio group") \
  X(MENU_AGC_BOOST, "AGC Boost", "Menu row name, Audio group") \
  X(MENU_MUTE_RAMP, "Mute Ramp", "Menu row name, Audio group") \
  X(MENU_BAND_PLAN, "Band Plan", "Menu row name, FM Reception group") \
  X(MENU_TUNING_STEP, "Tuning Step", "Menu row name, FM Reception group; sub-group name, AM Reception > Tuning Step") \
  X(MENU_SEEK_SENSITIVITY, "Seek Sensitivity", "Menu row name, FM Reception group; menu row name, AM Reception group") \
  X(MENU_DE_EMPHASIS, "De-emphasis", "Menu row name, FM Reception group") \
  X(MENU_FORCED_MONO, "Force Mono", "Menu row name, FM Reception > Stereo") \
  X(MENU_STEREO_BLEND, "Stereo Blend", "Menu row name, FM Reception > Stereo") \
  X(MENU_ST_HI_BLEND, "Stereo / High Blend", "Menu row name, FM Reception > Stereo") \
  X(MENU_STEREO, "Stereo", "Sub-group name, FM Reception > Stereo") \
  X(MENU_HIGH_CUT, "High Cut", "Menu row name, FM Reception group; sub-group name, AM Reception > High Cut") \
  X(MENU_NOISE_BLANKER, "Noise Blanker", "Menu row name, FM Reception group; menu row name, AM Reception group") \
  X(COMMON_IMS, "iMS", "Menu row name, FM Reception group; switch tile name") \
  X(MENU_EQUALISER, "Equalizer", "Menu row name, FM Reception group") \
  X(COMMON_RDS_DECODER, "RDS Decoder", "Menu row name, FM Reception > RDS") \
  X(MENU_RDS_REGION, "RDS Region", "Menu row name, FM Reception > RDS") \
  X(MENU_RDS_GROUP, "RDS", "Sub-group name, FM Reception > RDS") \
  X(MENU_MW_SPACING, "MW Step", "Menu row name, AM Reception group") \
  X(MENU_FILTER_WIDTH, "Filter Bandwidth", "Sub-group name, AM Reception > Filter Bandwidth") \
  X(MENU_SOFT_MUTE, "Soft Mute", "Sub-group name, AM Reception > Soft Mute") \
  X(MENU_MW_SW, "MW / SW", "Menu row name, AM Reception > High Cut and > Soft Mute: the pair MW and SW share") \
  X(MENU_LW, "LW", "Menu row name, AM Reception > High Cut, > Soft Mute, > Tuning Step and > Filter Bandwidth") \
  X(MENU_MW, "MW", "Menu row name, AM Reception > Tuning Step and > Filter Bandwidth") \
  X(MENU_SW, "SW", "Menu row name, AM Reception > Tuning Step and > Filter Bandwidth") \
  X(MENU_DWELL, "Scan Dwell", "Menu row name, DX Scanner group") \
  X(MENU_STOP_ON, "Stop Condition", "Menu row name, DX Scanner group") \
  X(MENU_SCAN, "Scan Range", "Menu row name, DX Scanner group") \
  X(MENU_MEMORY_FROM, "Preset Start", "Menu row name, DX Scanner > Preset Range") \
  X(MENU_MEMORY_TO, "Preset End", "Menu row name, DX Scanner > Preset Range") \
  X(MENU_PRESET_RANGE, "Preset Range", "Sub-group name, DX Scanner > Preset Range") \
  X(MENU_LOOP_THE_BAND, "Loop Band", "Menu row name, DX Scanner group") \
  X(MENU_DX_WIDTH, "Scan Bandwidth", "Menu row name, DX Scanner group: the width DX mode opens with") \
  X(MENU_MUTE_WHILE_SCANNING, "Mute During Scan", "Menu row name, DX Scanner group") \
  X(MENU_AUTO_LOG_NEW, "Auto-Log Stations", "Menu row name, DX Scanner group: a NEW catch logs itself") \
  X(MENU_LOG_RADIO_TEXT, "Log Radio Text", "Menu row name, DX Scanner group") \
  X(MENU_WATCH_PRESETS, "Watch Presets", "Menu row name, DX Scanner group") \
  X(DX_FMT_WATCH_UP, "%s up %d dB", "Confirm text when a watched preset comes up, frequency and rise; on a DX page it shows in the header") \
  X(MENU_LEARN_LOCALS, "Learn Local Stations", "Menu row name, DX Scanner group") \
  X(MENU_START_SCAN, "Start Scan", "Menu row name, DX Scanner group: opens DX mode on the Scanner page and starts the scan, FM only") \
  X(MENU_DAY_THEME, "Day Theme", "Menu row name, Display > Theme: the theme from 6 AM to 6 PM") \
  X(MENU_NIGHT_THEME, "Night Theme", "Menu row name, Display > Theme: the theme from 6 PM to 6 AM") \
  X(MENU_THEME, "Theme", "Sub-group name, Display > Theme") \
  X(MENU_BRIGHTNESS, "Brightness", "Menu row name, Display group") \
  X(MENU_DIM_LEVEL, "Dim Level", "Menu row name, Display group") \
  X(MENU_DIM_AFTER, "Dim After", "Menu row name, Display group") \
  X(MENU_ROTATION, "Display Rotation", "Menu row name, Display group") \
  X(COMMON_BATTERY, "Battery", "Menu row name, Display group; boot screen step name") \
  X(MENU_LEVEL_OFFSET_FM, "FM Level Offset", "Menu row name, Display > Level Offset") \
  X(MENU_LEVEL_OFFSET_AM, "AM Level Offset", "Menu row name, Display > Level Offset") \
  X(MENU_LEVEL_OFFSET, "Level Offset", "Sub-group name, Display > Level Offset") \
  X(MENU_FADE_AT_START, "Startup Fade", "Menu row name, Display group") \
  X(MENU_ENCODER, "Encoder Type", "Menu row name, Controls > Encoder") \
  X(MENU_DIRECTION, "Encoder Direction", "Menu row name, Controls > Encoder") \
  X(MENU_ENCODER_GROUP, "Encoder", "Sub-group name, Controls > Encoder") \
  X(MENU_KEY_BEEPS, "Key Beeps", "Menu row name, Controls group") \
  X(MENU_BAND_EDGE_BEEP, "Band Edge Beep", "Menu row name, Controls group") \
  X(MENU_START_CHIME, "Startup Chime", "Menu row name, Controls group") \
  X(MENU_TOUCH, "Touch", "Menu row name, Controls group: Off stops the radio reading the touch screen") \
  X(MENU_CLOCK_FROM_NETWORK, "Network Time", "Menu row name, Connectivity group: Off, or the offset from UTC") \
  X(MENU_WEB_PIN, "Web PIN", "Menu row name, Connectivity group, and the title of its editor") \
  X(MENU_NEW_PIN, "New PIN", "Menu, label on the panel of the Web PIN editor") \
  X(MENU_FMT_DIGIT_OF, "Digit %u of %u", "Menu, under the Web PIN editor, the digit being set and how many there are") \
  X(MENU_STATUS, "Connection Status", "Menu row name, Connectivity > Network Info, read only") \
  X(MENU_HOTSPOT, "Hotspot", "Menu row name, Connectivity group: Auto, On or Off for the radio's own access point") \
  X(MENU_WEB_SERVER, "Web Server", "Menu row name, Connectivity group: On or Off for the pages, the API and updates over Wi-Fi") \
  X(MENU_WIFI, "Wi-Fi", "Menu row name, Connectivity group: On or Off for Wi-Fi at all") \
  X(MENU_WEB_ADDRESS, "Web Address", "Menu row name, Connectivity > Network Info, read only: the radio's name and port to type in a browser, or its address while the name is not announced") \
  X(MENU_IP_ADDRESS, "IP Address", "Menu row name, Connectivity > Network Info, read only: the address the network gave the radio") \
  X(MENU_FMT_NAME_ADDRESS, "%s.local:%u", "Value of the Web Address row: the radio's name on the network and the web server's port") \
  X(MENU_WI_FI_NAME, "Wi-Fi Network", "Menu row name, Connectivity > Network Info, read only") \
  X(MENU_HOTSPOT_NAME, "Hotspot Name", "Menu row name, Connectivity > Network Info, read only, in place of Wi-Fi Network while the radio serves its hotspot") \
  X(MENU_MAC, "MAC Address", "Menu row name, Connectivity > Network Info, read only") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi Signal", "Menu row name, Connectivity > Network Info, read only") \
  X(MENU_NETWORK_INFO, "Network Info", "Sub-group name, Connectivity > Network Info: the read only rows") \
  X(MENU_RESTART, "Restart Radio", "Menu row name, System group") \
  X(MENU_AUTO_OFF, "Auto Off", "Menu row name, System group: minutes alone before the radio sleeps") \
  X(MENU_UPDATE_CHECK, "Check for Updates", "Menu row name, System group: On to look on GitHub for a newer release once the radio is on the network") \
  X(MENU_FIRMWARE_UPDATE, "Firmware Update", "Menu row name, System group, while no newer release is known; its value says why") \
  X(MENU_FMT_UPDATE_TO, "Update to %s", "Menu row name, System group, while a newer release is known. %s is the newer version, such as 0.2.0") \
  X(MENU_UPDATE_TITLE, "Update available", "Update offer: the title of the box") \
  X(MENU_UPDATE_THIS_RADIO, "This radio", "Update offer, first fact: the version this radio runs") \
  X(MENU_UPDATE_NEW_VERSION, "New version", "Update offer, second fact: the version on offer") \
  X(MENU_UPDATE_DOWNLOAD, "Download", "Update offer, third fact: the image size") \
  X(MENU_UPDATE_SETTINGS, "Settings", "Update offer, fourth fact: what happens to the settings") \
  X(MENU_UPDATE_KEPT, "Kept", "Update offer, value of the Settings fact") \
  X(MENU_UPDATE_NOW, "Update", "Update offer, left button: download and install the newer version") \
  X(MENU_LATER, "Later", "Update offer, right button: close the offer") \
  X(MENU_FMT_MEGABYTES, "%s MB", "Value of the Update to row and of the offer's Download fact: the image size, %s such as 1.7") \
  X(MENU_UPDATE_UP_TO_DATE, "Up to date", "Value of the Firmware Update row: no newer release") \
  X(MENU_UPDATE_CHECKING, "Checking", "Value of the Firmware Update row: the radio is looking on GitHub now") \
  X(MENU_UPDATE_NOT_CHECKED, "Not checked", "Value of the Firmware Update row: Check for Updates is on and the radio has not looked yet in this start") \
  X(MENU_UPDATE_CHECK_FAILED, "Check failed", "Value of the Firmware Update row: GitHub could not be reached, or the release was refused") \
  X(MENU_NOTE_NO_UPDATE, "No update found", "Menu, note after pressing Firmware Update while no newer release is known") \
  X(MENU_VERSION, "Firmware Version", "Menu row name, About group, read only") \
  X(MENU_RUNNING_FROM, "Boot Source", "Menu row name, Diagnostics group, read only") \
  X(MENU_UPTIME, "Uptime", "Menu row name, Diagnostics group, read only") \
  X(MENU_BATTERY_VOLTAGE, "Battery Voltage", "Menu row name, Diagnostics group, read only: the cell's voltage") \
  X(COMMON_TUNER, "Tuner", "Menu row name, Diagnostics group, read only; boot screen step name; title of the message screen when the tuner reports a fault; the detail is `tef668xErrorText()`") \
  X(MENU_RESET_REASON, "Reset Reason", "Menu row name, Diagnostics group, read only; the value is one of the RESET_ texts") \
  X(MENU_CPU_CORE_0, "CPU Core 0", "Menu row name, Diagnostics group, read only; the value is how busy the core was over the last second") \
  X(MENU_CPU_CORE_1, "CPU Core 1", "Menu row name, Diagnostics group, read only") \
  X(MENU_FREE_HEAP, "Free Heap", "Menu row name, Diagnostics group, read only") \
  X(MENU_LOWEST_HEAP, "Lowest Heap", "Menu row name, Diagnostics group, read only; the least free heap since boot") \
  X(MENU_LARGEST_BLOCK, "Largest Block", "Menu row name, Diagnostics group, read only; the largest free heap block") \
  X(MENU_LVGL_POOL, "LVGL Pool", "Menu row name, Diagnostics group, read only; LVGL's own heap in use and its size") \
  X(MENU_CHIP, "Chip", "Menu row name, Diagnostics group, read only") \
  X(MENU_FLASH_SIZE, "Flash Size", "Menu row name, Diagnostics group, read only") \
  X(RESET_POWER, "Power On", "Diagnostics group, value of the Reset Reason row") \
  X(RESET_SOFTWARE, "Software", "Diagnostics group, value of the Reset Reason row, a software restart with no note") \
  X(RESET_PANIC, "Panic", "Diagnostics group, value of the Reset Reason row") \
  X(RESET_INT_WATCHDOG, "IRQ Watchdog", "Diagnostics group, value of the Reset Reason row, the interrupt watchdog") \
  X(RESET_TASK_WATCHDOG, "Task Watchdog", "Diagnostics group, value of the Reset Reason row") \
  X(RESET_WATCHDOG, "Watchdog", "Diagnostics group, value of the Reset Reason row, any other watchdog") \
  X(RESET_DEEP_SLEEP, "Deep Sleep", "Diagnostics group, value of the Reset Reason row") \
  X(RESET_BROWNOUT, "Brownout", "Diagnostics group, value of the Reset Reason row") \
  X(RESET_PIN, "Reset Pin", "Diagnostics group, value of the Reset Reason row") \
  X(RESET_UNKNOWN, "Unknown", "Diagnostics group, value of the Reset Reason row when the chip gives no reason this firmware knows") \
  X(RESET_ASKED, "Restart Asked", "Diagnostics group, value of the Reset Reason row after Restart Radio in the menu or on the web page") \
  X(RESET_BOOT_WATCHDOG, "Boot Watchdog", "Diagnostics group, value of the Reset Reason row after start up did not finish in time") \
  X(RESET_ROLLBACK, "Rollback", "Diagnostics group, value of the Reset Reason row after a new image failed its self check") \
  X(RESET_DISPLAY, "Display Fault", "Diagnostics group, value of the Reset Reason row after an LVGL assertion") \
  X(RESET_UPDATE, "Update", "Diagnostics group, value of the Reset Reason row after a firmware update") \
  X(MENU_DEVELOPER, "Developer", "Menu row name, About group, read only") \
  X(MENU_GITHUB, "GitHub", "Menu row name, About group, read only") \
  X(MENU_BUILD, "Build", "Menu row name, About group, read only; the value is the git commit the firmware was built from, with + when its sources had changes not yet committed") \
  X(MENU_LICENSE, "License", "Menu row name, About group, read only") \
  X(ABOUT_DEVELOPER, "zeevy", "About group, value of the Developer row") \
  X(ABOUT_GITHUB, "github.com/zeevy/tef668x-esp32", "About group, value of the GitHub row") \
  X(ABOUT_BUILD_UNKNOWN, "unknown", "About group, value of the Build row when the firmware was built without git") \
  X(ABOUT_LICENSE, "GPLv3", "About group, value of the License row") \
  X(MENU_GO_TO, "Go To", "Menu group name, first in the list: the screens and actions the panel keys reach, for the knob alone") \
  X(MENU_GO_TO_BANDWIDTH, "Bandwidth", "Menu row name, Go To group: opens the bandwidth page") \
  X(MENU_GO_TO_RDS, "RDS", "Menu row name, Go To group: opens the RDS screen") \
  X(MENU_GO_TO_DX, "DX Mode", "Menu row name, Go To group: opens DX mode, FM only") \
  X(MENU_GO_TO_BAND, "Next Band", "Menu row name, Go To group: steps to the next band, as BAND does") \
  X(MENU_GO_TO_LOG, "Log Current Station", "Menu row name, Go To group: writes the station to the logbook") \
  X(MENU_GO_TO_SLEEP, "Sleep", "Menu row name, Go To group: the radio fades and sleeps until the knob is pressed") \
  X(MENU_STATIONS, "Stations", "Menu group name: the presets, the station scans and the station log") \
  X(MENU_MEMORY, "Presets", "Sub-group name, Stations > Presets, also its title when open") \
  X(MENU_AUDIO, "Audio", "Menu group name") \
  X(MENU_FM_SETUP, "FM Reception", "Menu group name") \
  X(MENU_AM_SETUP, "AM Reception", "Menu group name") \
  X(MENU_DX_SETUP, "DX Scanner", "Menu group name") \
  X(MENU_DISPLAY, "Display", "Menu group name") \
  X(MENU_CONTROLS, "Controls", "Menu group name") \
  X(MENU_CONNECTIVITY, "Connectivity", "Menu group name") \
  X(MENU_SYSTEM, "System", "Menu group name") \
  X(MENU_DIAGNOSTICS, "Diagnostics", "Menu group name: the read only readings of the box") \
  X(MENU_ABOUT, "About", "Menu group name") \
  X(MENU_REGION_FULL, "Full", "Menu, value of the Band plan row, picked from `kRegionNames[]` by `FmRegion` index") \
  X(MENU_REGION_JAPAN, "Japan", "Menu, value of the Band plan row") \
  X(MENU_REGION_WIDE, "Wide", "Menu, value of the Band plan row") \
  X(MENU_REGION_87_108, "87-108 MHz", "Menu, value of the Band plan row") \
  X(MENU_REGION_WORLD, "Worldwide", "Menu, value of the Band plan row") \
  X(MENU_RDS_REGION_EUROPE, "Europe", "Menu, value of the RDS Region row: Europe and the rest of the world") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "North America", "Menu, value of the RDS Region row: the PI is read as call letters") \
  X(MENU_WIFI_JOINED, "Connected", "Menu, value of the Status row, Wi-Fi state") \
  X(MENU_WIFI_JOINING, "Connecting", "Menu, value of the Status row") \
  X(MENU_WIFI_HOTSPOT, "Hotspot", "Menu, value of the Status row while the radio serves its own access point") \
  X(COMMON_OFF, "Off", "Value of the Status row; value of a level row at zero (Squelch floor, High cut, blends, Squelch level); value of the Volume AGC row at zero; value of the Battery row; value of the Key beeps row; value of the De-emphasis row; value of the Noise blanker rows at zero; value of the Squelch row; value of every on/off row; Sync tile on RDS page 4 with the decoder off; switch tile state") \
  X(COMMON_DASH, "-", "Value of the MAC row when the address cannot be read; value of an info row with nothing to say; value of a row the radio could not read; mark of a boot step still waiting; value of an empty field on RDS page 1; radio text on RDS page 2 when none; value of an empty tile or block row on RDS page 4") \
  X(MENU_FMT_MINUTES, "%u min", "Menu, value of the Uptime row under one hour, %u is minutes; value of the Auto Off row") \
  X(MENU_FMT_HOURS_MINUTES, "%u h %u m", "Menu, value of the Uptime row, hours and minutes") \
  X(MENU_FMT_KB, "%u KB", "Menu, value of the Free Heap, Lowest Heap and Largest Block rows, 1 KB is 1024 bytes") \
  X(MENU_FMT_KB_OF_KB, "%u of %u KB", "Menu, value of the LVGL Pool row, in use and size") \
  X(MENU_FMT_MB, "%u MB", "Menu, value of the Flash Size row") \
  X(MENU_FMT_VOLTS, "%u.%02u V", "Menu, value of the Battery Voltage row, read now") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V at start", "Menu, value of the Battery Voltage row: the reading taken at start up, since Wi-Fi holds the converter while it runs") \
  X(MENU_FMT_PATH, "%s > %s", "Menu, the title of a sub-group or of a setting's own screen: the list it sits in, then its own name") \
  X(MENU_FMT_AGC_ROW, "%u %%, %+d dB", "Menu, value of the Volume AGC row while it runs: its target and what it is doing to the sound now") \
  X(MENU_FMT_AGC_GAIN_NOW, "Gain now %+d dB", "Menu, note under the bar of the Volume AGC and AGC Boost screens: what the AGC is doing to the sound now") \
  X(LOG_FMT_PI, "PI %04X", "Station Log, an entry with a PI and no name: its PI in hex") \
  X(MENU_STATION_LOG, "Station Log", "Sub-group name, Stations > Station Log: the logbook, newest first") \
  X(MENU_FMT_CHIP, "%s v%u.%u", "Menu, value of the Chip row, model and revision") \
  X(MENU_FMT_DBM, "%d dBm", "Menu, value of the Wi-Fi Signal row") \
  X(COMMON_NONE, "None", "Value of the Tuner row when no tuner answered; value of the Restore Previous Firmware row on the recovery screen with no older image") \
  X(MENU_FMT_SCAN_PROGRESS, "Scan %u/%u", "Menu, value of a station scan row while its scan runs, done/total") \
  X(MENU_FMT_SCAN_SAVED, "Saved %u", "Menu, value of a station scan row after its band's last scan: presets it saved") \
  X(MENU_FMT_SCAN_STOPPED, "Stopped, %u saved", "Menu, value of a station scan row after its band's last scan, when a tune or the radio stopped it before the end: presets it saved") \
  X(MENU_FMT_SCAN_NO_ROOM, "Full, %u not saved", "Menu, value of a station scan row after its band's last scan, when new stations found every preset slot full: how many") \
  X(COMMON_YES, "Yes", "Value of the Restart Radio confirm row turned to yes") \
  X(COMMON_NO, "No", "Value of a confirm action row at rest; Sync tile on RDS page 4 when not synchronised") \
  X(MENU_UNKNOWN_VALUE, "Unknown", "Menu, value of the Band plan row for an index outside the table") \
  X(MENU_FMT_KHZ, "%d kHz", "Menu, value of the MW spacing row, 9 or 10; value of the Filter width and Tuning step rows") \
  X(COMMON_ROTATION_NORMAL, "Normal", "Value of the Rotation row and of its picker, and of recovery's Rotate Display row: the display the right way up") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Upside Down", "Value of the Rotation row and of its picker, and of recovery's Rotate Display row: the display turned half way round") \
  X(MENU_FMT_DBUV, "%d dBuV", "Menu, value of the Soft mute rows; value of those level rows above zero") \
  X(MENU_FMT_MS, "%d ms", "Menu, value of the Mute ramp row") \
  X(MENU_FMT_PERCENT, "%d %%", "Menu, value of the Volume AGC row, per cent; value of the Brightness and Dim level rows; value of the Noise blanker rows; value of the CPU Core rows") \
  X(MENU_CUT_ONLY, "Cut Only", "Menu, value of the AGC boost row at zero") \
  X(MENU_FMT_DB, "%d dB", "Menu, value of the AGC boost row; value of a level offset row at 0") \
  X(MENU_FMT_SIGNED_DB, "%+d dB", "Menu, value of the FM and AM Level Offset rows, with the sign") \
  X(MENU_NEVER, "Never", "Menu, value of the Dim after row at zero") \
  X(MENU_FMT_SECONDS, "%d s", "Menu, value of the Dim after row, seconds") \
  X(MENU_BATTERY_PER_CENT, "Percent", "Menu, value of the Battery row") \
  X(MENU_BATTERY_VOLTS, "Volts", "Menu, value of the Battery row") \
  X(MENU_KEYS, "Keypad Only", "Menu, value of the Key beeps row: keypad digits beep") \
  X(MENU_KEYS_LONG, "Short & Long Press", "Menu, value of the Key beeps row: keypad digits and long presses beep") \
  X(MENU_EVERY_PRESS, "Every Press", "Menu, value of the Key beeps row: every key and button beeps") \
  X(MENU_STANDARD, "Standard", "Menu, value of the Encoder row") \
  X(MENU_OPTICAL, "Optical", "Menu, value of the Encoder row") \
  X(MENU_NORMAL, "Normal", "Menu, value of the Direction row") \
  X(MENU_REVERSED, "Reversed", "Menu, value of the Direction row") \
  X(MENU_DEEMPHASIS_50, "50 us", "Menu, value of the De-emphasis row") \
  X(MENU_DEEMPHASIS_75, "75 us", "Menu, value of the De-emphasis row") \
  X(MENU_AUTO, "Auto", "Menu, value of the Squelch row and of the Hotspot row") \
  X(MENU_SQUELCH_MAN, "Manual", "Menu, value of the Squelch row") \
  X(MENU_FMT_UNIT_AFTER, " %s", "Menu, appended to the Dwell and DX width number, %s is the unit from `unitOf()` (concatenation)") \
  X(MENU_ANY_PI, "Any PI", "Menu, value of the Stop on row") \
  X(MENU_STOP_NEVER, "Never", "Menu, value of the Stop on row") \
  X(MENU_NEW_ONLY, "New Stations Only", "Menu, value of the Stop on row") \
  X(MENU_WHOLE_BAND, "Whole Band", "Menu, value of the Scan row") \
  X(MENU_MEMORY_ONLY, "Presets Only", "Menu, value of the Scan row") \
  X(MENU_BAND_MEMORY, "Band + Presets", "Menu, value of the Scan row") \
  X(COMMON_ON, "On", "Value of every on/off row (Forced mono, iMS, Equaliser, RDS decoder, Loop the band, and so on); switch tile state") \
  X(COMMON_UNIT_PERCENT, "%", "Unit from `unitOf()`, under the bar ends for Brightness, Dim level, Noise blanker rows; unit for Volume AGC; reading unit, three times; per cent sign after the OTA progress number") \
  X(COMMON_UNIT_S, "s", "Unit for Dim after; unit for Dwell; unit after the seconds left") \
  X(COMMON_UNIT_MIN, "min", "Unit for Auto Off") \
  X(MENU_UNIT_MS, "ms", "Menu, unit for Mute ramp") \
  X(COMMON_UNIT_DB, "dB", "Unit for AGC boost; unit after the rise over base") \
  X(MENU_UNIT_DBUV, "dBuV", "Menu, unit for the level rows") \
  X(COMMON_UNIT_KHZ, "kHz", "Unit for Filter width, Tuning step, DX width; reading unit, twice; frequency unit for AM bands") \
  X(MENU_FMT_NUMBER_UNIT, "%d %s", "Menu, limit text under each bar end, number and unit (concatenation of value and `unitOf()`)") \
  X(MENU_HINT_RESTART, "Applies after restart", "Menu, note for a row with needsRestart: on the picker's cursor row, under the bar otherwise") \
  X(MENU_TITLE, "Menu", "Menu, title of the group list") \
  X(COMMON_FM_ONLY, "FM only", "Value of an FM-only row on AM; Scanner press result") \
  X(MENU_NOTE_OFF_PLAN, "Not on the band plan", "Menu, note after pressing a preset or Station Log row whose frequency is not on the band it names under the band plan now") \
  X(MENU_NOTE_SWITCH_TO_FM, "Switch to FM first", "Menu, note after DX Mode, Start Scan or Learn locals on AM; name line after the DX key or a BAND hold on AM") \
  X(MENU_NOTE_RADIO_BUSY, "Radio busy - try again", "Menu, note after a station scan refused; note after Learn locals with no snapshot; note when a row could not be read to start editing; note after a preset or Station Log row press with no snapshot or band plan") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Turn on RDS first", "Menu, note after Learn locals with RDS off") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Update on trial - wait", "Menu, note after Restart Radio or Sleep while an OTA image is on trial") \
  X(MENU_NOTE_NOT_STORED, "Not saved - restored", "Menu, note when the settings store refused the value") \
  X(MENU_NOTE_NOT_PUT_BACK, "Radio busy - not restored", "Menu, note when the undo could not reach the tuner") \
  X(MENU_NOTE_NOT_SAVED, "Not Saved", "Menu, under the bar once a value differs from the saved one, and on its row after backing out of it") \
  X(RADIO_PRESS_TO_START, "Press to start", "Scanner page, header message when a turn or a typed digit is refused there") \
  X(COMMON_STILL_TUNING, "Still tuning - wait", "Confirm text on the name line after a logbook hold while a seek or a scan walks the dial") \
  X(RADIO_FMT_NO_BAND, "%s is in no band", "Name line, a moment after ENTER on a typed number no band holds") \
  X(COMMON_NOT_LOGGED, "Not logged", "Confirm text on the name line when the logbook hold found no snapshot; confirm text when the logbook file could not be written; confirm text after a failed catch write") \
  X(BOOT_SETTINGS, "Settings", "Boot screen step name, `kBootNames[]`") \
  X(BOOT_RADIO, "Radio", "Boot screen step name") \
  X(BOOT_CHANNELS, "Presets", "Boot screen step name") \
  X(BOOT_KEYPAD, "Keypad", "Boot screen step name") \
  X(RADIO_PRODUCT_NAME, "TEF668X", "Boot screen title (product); title of the message screen when the boot screen could not be built") \
  X(RADIO_FMT_LOGGED, "Logged %s", "Confirm text after a logbook write, %s is the frequency: on the name line of the radio screen, in the header of a DX page") \
  X(RADIO_CHECKING_UPDATES, "Checking for updates\xE2\x80\xA6", "Line under the amber panel, in place of the radio text, while the update check runs") \
  X(RADIO_UPDATE_FAILED, "Update Failed", "Title of the message screen after a failed OTA") \
  X(RADIO_GOING_TO_SLEEP, "Going to Sleep", "Title of the message screen shown for a moment before the radio sleeps") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Press the knob to wake", "Detail line of that message: the only way to wake the radio") \
  X(RADIO_OLD_IMAGE_KEPT, "Previous firmware kept", "Detail line of that message") \
  X(RDS_STEREO, "Stereo", "RDS page 1 header, the station says stereo") \
  X(RDS_MONO, "Mono", "RDS page 1 header, the station says mono") \
  X(COMMON_FMT_TWO_WORDS, "%s %s", "Header context on the RDS screens, frequency and unit; ECC and its code on RDS page 1; tile text for the iMS and EQ switches, name and state") \
  X(RDS_PI_ZERO, "0000", "RDS page 1, PI tile, when the station sends a zero PI") \
  X(RDS_SPEECH, "Speech", "RDS page 1, music or speech tile") \
  X(RDS_MUSIC, "Music", "RDS page 1, music or speech tile") \
  X(RDS_FMT_BAD_BLOCKS, "%u/%u", "RDS page 4, LOST tile, blocks lost of all blocks in the last minute") \
  X(RDS_FMT_BLER, "%.1f %%", "RDS page 4, BLER tile, over the last minute") \
  X(COMMON_ALREADY_LOGGED, "Already Logged", "Confirm text on the name line after a logbook hold, and in the header of a DX page after a catch hold, when the log holds that station already or nothing new was heard") \
  X(DX_RDS_IS_OFF, "RDS is Off", "Confirm text when a scan stops because the decoder went off; Scanner press result") \
  X(DX_SCANNING, "Scanning", "Result of a Scanner press, `dxTaskPressText()`, shown as confirm text") \
  X(DX_SCAN_FINISHED, "Scan Complete", "Scanner press result") \
  X(DX_NOTHING_TO_SCAN, "Nothing to Scan", "Scanner press result") \
  X(DX_CHECKING_FOR_UPDATES, "Checking For Updates", "Scanner press result while the update check is out on the network") \
  X(DX_A_SCAN_IS_RUNNING, "Scan Already Running", "Scanner press result; Scope press result") \
  X(DX_CAUGHT_LIST_UNREAD, "Catch List Unavailable", "Scanner press result") \
  X(DX_NO_MEMORY, "No Presets", "Scanner press result; Scope press result") \
  X(DX_NOT_STARTED, "Not Started", "Scanner press result; Scope press result") \
  X(DX_SWEEPING, "Sweeping", "Result of a Scope press, `dxTaskSweepText()`; middle text on the Scope page during a sweep") \
  X(RADIO_FMT_TYPED, "%s-", "Typed digits on the radio screen with a hyphen cursor while more can follow") \
  X(COMMON_FMT_TUNE_TYPED, "Tune %s", "Header of the RDS screen and the DX pages while digits are typed: what ENTER will tune to") \
  X(RADIO_FMT_VOL_DB, "%ddB", "V: tile value") \
  X(RADIO_SQL_OFF, "OFF", "SQ: tile value") \
  X(COMMON_AUTO, "AUTO", "SQ: tile value; tile text for the automatic width") \
  X(RADIO_FMT_SQL_DB, "%ddB", "SQ: tile value in Manual squelch, the level the volume knob sets, in whole dB") \
  X(RADIO_BW_DYN, "DYN", "BW tile value while the tuner picks the width") \
  X(RADIO_TUNE_MANUAL, "MAN", "Tuning mode tile on the radio screen; the tile is 68 px wide and no mode is longer than AUTO") \
  X(RADIO_TUNE_AUTO, "AUTO", "Tuning mode cell on the radio screen, seek on a knob turn") \
  X(RADIO_TUNE_MEMORY, "MEM", "Tuning mode cell on the radio screen, the knob walks the presets") \
  X(RADIO_TUNE_METER, "MTR", "Tuning mode cell on the radio screen, the knob walks the SW metre bands") \
  X(RADIO_FMT_BW_K, "%uk", "BW tile value, width in kHz") \
  X(COMMON_FMT_PERCENT, "%u%%", "Battery per cent on the web page") \
  X(RADIO_FMT_MEMORY_SLOT, "P%02d", "Preset slot mark on the radio screen") \
  X(RADIO_FMT_METER_BAND, "%u m", "Radio screen header, the SW metre band beside the band name") \
  X(DX_FMT_TENTHS, "%s%d.%d", "Tenths as one decimal, %s is a minus or nothing, used for level readings on every DX page") \
  X(DX_FMT_SIGNED, "%+d", "OFFSET reading with its sign on the DX page") \
  X(DX_FMT_PAGE, "1/%u", "Page position in the DX page header") \
  X(DX_FMT_ROWS_OF, "%u-%u of %u", "Header context on the Catches page, rows shown of total") \
  X(DX_COUNT_99_PLUS, "99+", "Catch count on a Catches row past 99") \
  X(DX_FMT_TIMES, "\xC3\x97%u", "Catch count on a Catches row, multiplication sign and number") \
  X(DX_FMT_FOUND, "%u found", "Header context on the Scanner page") \
  X(DX_MODE_SWEEPING, "SWEEPING", "Mode text on the Scanner page") \
  X(DX_MODE_LEARN_LOCALS, "LEARN LOCALS", "Mode text on the Scanner page") \
  X(DX_FMT_MODE_MEMORY, "PRESETS %u-%u", "Mode text on the Scanner page, slot range") \
  X(DX_MODE_WHOLE_BAND, "WHOLE BAND", "Mode text on the Scanner page") \
  X(DX_MODE_BAND_MEMORY, "BAND + PRESETS", "Mode text on the Scanner page") \
  X(DX_RULE_NO_STOP, "NO STOP", "Rule text on the Scanner page") \
  X(DX_RULE_STOP_ON_PI, "STOP ON PI", "Rule text on the Scanner page") \
  X(DX_RULE_STOP_ON_NEW, "STOP ON NEW", "Rule text on the Scanner page") \
  X(DX_FMT_DWELL_S, "%s s", "Dwell on the Scanner page, one decimal and the unit") \
  X(DX_FMT_PRESET, "P%02d", "Under the PI on the DX page and RDS page 1: the preset the dial is on, whose stored PI this station sends") \
  X(DX_FMT_NOT_PRESET, "not P%02d", "Under the PI on the DX page and RDS page 1, in red: another station, confirmed, on the preset the dial is on") \
  X(DX_FMT_CHANNEL, "CH %u", "From end of the Scanner bar in memory range; to end of the Scanner bar in memory range") \
  X(DX_FMT_STEP_OF, "%u / %u", "Step text under the Scanner bar, passed of total") \
  X(DX_CONTEXT_SWEEPING, "sweeping", "Header context on the Scope page during a sweep") \
  X(DX_PRESS_TO_SWEEP, "Press to Sweep", "Middle text on an empty Scope page") \
  X(DX_FIXED, "FIXED", "Base line label on the Scope page") \
  X(DX_FMT_MEDIAN_OF, "MEDIAN OF %u", "Base line label on the Scope page, sweep count") \
  X(DX_FMT_FLOOR, "FLOOR %s", "Floor label on the Scope page, %s is the level in tenths") \
  X(BW_FMT_CONTEXT, "%s \xC2\xB7 kHz", "Header context on the Bandwidth page, band name and unit") \
  X(BW_EQ, "EQ", "Bandwidth page, switch tile name") \
  X(BW_FMT_AUTO_AT, "Auto: %u kHz", "Bandwidth page, note under the tiles, the width the chip picked") \
  X(RECOVERY_ROTATE_DISPLAY, "Rotate Display", "Recovery screen row name") \
  X(RECOVERY_TOUCH, "Touch", "Recovery screen row name: turns reading the touch screen on or off and restarts") \
  X(RECOVERY_START_HOTSPOT, "Start Hotspot", "Recovery screen row name: sets the Hotspot setting to On and restarts") \
  X(RECOVERY_ROLL_BACK_FIRMWARE, "Restore Previous Firmware", "Recovery screen row name") \
  X(RECOVERY_ERASE_SETTINGS, "Erase Settings", "Recovery screen row name") \
  X(RECOVERY_EXIT_AND_START_RADIO, "Exit & Start Radio", "Recovery screen row name") \
  X(RECOVERY_FAILED, "Failed", "Recovery screen, value of the Restore Previous Firmware row after a failed roll back, and of a row whose settings could not be written") \
  X(RECOVERY_PRESS_AGAIN, "Press again", "Recovery screen, value of a row waiting for its second press") \
  X(RECOVERY_ASK_ROTATE, "Press again to turn the display over", "Recovery screen foot line while Rotate Display waits for its second press") \
  X(RECOVERY_ASK_TOUCH_OFF, "Press again to turn touch off and restart", "Recovery screen foot line while Touch, now On, waits for its second press") \
  X(RECOVERY_ASK_TOUCH_ON, "Press again to turn touch on and restart", "Recovery screen foot line while Touch, now Off, waits for its second press") \
  X(RECOVERY_ASK_HOTSPOT, "Press again to start the hotspot and restart", "Recovery screen foot line while Start Hotspot waits for its second press") \
  X(RECOVERY_ASK_ROLLBACK, "Press again to go back to the firmware before this one", "Recovery screen foot line while Restore Previous Firmware waits for its second press") \
  X(RECOVERY_ASK_ERASE, "Press again to erase every setting, Wi-Fi too", "Recovery screen foot line while Erase Settings waits for its second press") \
  X(BOOT_NEW, "New", "Boot screen value on the Settings step when defaults are in use") \
  X(COMMON_FMT_VOLTS, "%u.%02uV", "Boot screen value on the Battery step, volts") \
  X(COMMON_FMT_VOLTS_TENTHS, "%u.%u", "Battery volts in the header when Battery is set to volts, one decimal, with a small upright battery just before it") \
  X(MENU_FMT_TUNER_PATCH, "%s patch %u", "Menu, value of the Tuner row, Diagnostics group: part name and patch version") \
  X(BOOT_FMT_TUNER_VERSION, "%s v%s", "Boot screen tuner line, part name and the firmware's version") \
  X(BOOT_SELF_TEST, "Self Test", "Left hint on the boot screen") \
  X(COMMON_NO_VALUE, "---", "Tuner line on the boot screen until the tuner answers; name line on the radio screen with no station or memory name; name line when the station name is only spaces") \
  X(BW_TITLE, "Bandwidth", "Title of the Bandwidth page") \
  X(DX_CATCHES_EMPTY_TITLE, "No Catches Yet", "Empty state title on the Catches page") \
  X(DX_CATCHES_EMPTY_NOTE, "Confirmed stations from this session appear here", "Empty state note on the Catches page") \
  X(DX_TITLE_CATCHES, "Catches", "Title of the Catches page") \
  X(DX_NEW, "NEW", "Pill text on the Scanner page for a new catch") \
  X(COMMON_UNIT_MHZ, "MHz", "Unit after the frequency on the Scanner page; unit after the cursor frequency; frequency unit, `bandFrequencyUnit()`; radio screen, DX page, RDS header") \
  X(DX_READY, "Ready", "DX mode, shown in place of the frequency before a scan starts") \
  X(DX_DWELL, "DWELL", "DX mode, label over the dwell value") \
  X(COMMON_PI, "PI", "Label on the station tile; label on the PI tile; label on the PI tile of RDS page 1") \
  X(COMMON_UNIT_DBUV, "dB\xC2\xB5V", "Unit after the level; unit after the cursor level; reading unit, `kReadUnits[]`; unit after the signal level on the radio screen") \
  X(DX_STOPPED, "STOPPED", "DX mode, mode text while the scan is stopped") \
  X(DX_TITLE_SCANNER, "Scanner", "Title of the Scanner page") \
  X(DX_TITLE_SCOPE, "Scope", "Title of the Scope page") \
  X(DX_LEVEL, "LEVEL", "Reading label on the DX page, `kReadNames[]`; label in the Scanner page's station tile") \
  X(DX_RISE, "RISE", "Label before the rise over the base line in the Scope page's tile") \
  X(DX_NO_STATION_YET, "No station yet", "The Scanner page's tile before a scan, with no station on it") \
  X(DX_NO_RDS_YET, "No RDS yet", "The Scanner page's tile on a channel with no PI heard yet") \
  X(DX_USN, "USN", "DX mode, reading label") \
  X(DX_WAM, "WAM", "DX mode, reading label") \
  X(DX_OFFSET, "OFFSET", "DX mode, reading label") \
  X(COMMON_BW, "BW", "Reading label; tile label") \
  X(DX_MOD, "MOD", "DX mode, reading label") \
  X(DX_TITLE, "DX", "Title of the DX page") \
  X(DX_BLOCK_A, "A", "DX mode, RDS block letter under the error bars, `kLetters[]`") \
  X(DX_BLOCK_B, "B", "DX mode, RDS block letter") \
  X(DX_BLOCK_C, "C", "DX mode, RDS block letter") \
  X(DX_BLOCK_D, "D", "DX mode, RDS block letter") \
  X(DX_NO_ID, "NO ID", "DX mode, country line when the PI is zero") \
  X(RDS_TITLE_IDENTITY, "RDS", "Title of RDS page 1, the station, `kTitle[]` picked by page index") \
  X(RDS_TITLE_TEXT, "Radio Text", "Title of RDS page 2") \
  X(RDS_TITLE_NETWORKS, "Networks", "Title of RDS page 3, the alternative frequencies and other networks") \
  X(RDS_TITLE_DECODER, "Decoder", "Title of RDS page 4") \
  X(RDS_FMT_PTY_NUMBER, "PTY %u", "RDS page 1, in the amber panel, at the right of the programme type name's line") \
  X(RDS_NO_COUNTRY, "no ECC heard", "RDS page 1, PI tile, when no ECC has come to name the country") \
  X(RDS_AREA, "AREA", "RDS page 1, label of the PI's coverage area") \
  X(RDS_LANG, "LANG", "RDS page 1, label of the programme language") \
  X(RDS_ECC, "ECC", "RDS page 1, before the extended country code on the PI tile") \
  X(RDS_TP, "TP", "RDS page 1, traffic programme tile") \
  X(RDS_TA, "TA", "RDS page 1, traffic announcement tile") \
  X(RDS_MS_UNKNOWN, "M/S", "RDS page 1, music or speech tile before the station has said which") \
  X(RDS_RTPLUS, "RT+", "RDS page 2, label above the RT+ note") \
  X(RDS_RTPLUS_NONE, "No RT+ heard from this station", "RDS page 2, when no RT+ announcement has been heard twice") \
  X(RDS_RTPLUS_WAIT, "No RT+ tags for this text yet", "RDS page 2, RT+ announced but no tag for the text on air") \
  X(RDS_AF_TITLE, "ALTERNATIVE FREQUENCIES", "RDS page 3, label above the frequency tiles") \
  X(RDS_EON_TITLE, "OTHER STATIONS, EON", "RDS page 3, label above the other networks") \
  X(RDS_NONE_HEARD, "None heard", "RDS page 3, at the right of a list's label, or under it on the empty panel, when nothing was heard for that list") \
  X(RDS_FMT_MORE, "+%u", "RDS page 3, the last frequency tile, how many more did not fit") \
  X(RDS_EON_SEPARATOR, " \xC2\xB7 ", "RDS page 3, between the frequencies of another network (concatenation)") \
  X(RDS_SYNC_CAPS, "SYNC", "RDS page 4, stat tile label") \
  X(RDS_GROUPS_S_CAPS, "GROUPS/S", "RDS page 4, stat tile label") \
  X(RDS_LOST, "LOST", "RDS page 4, stat tile label, blocks lost of all blocks") \
  X(RDS_EACH_BLOCK, "EACH BLOCK", "RDS page 4, label above the four block bars") \
  X(RDS_LEGEND_CLEAN, "clean", "RDS page 4, legend word in the clean colour") \
  X(RDS_LEGEND_FIXED, "fixed", "RDS page 4, legend word in the corrected colour") \
  X(RDS_LEGEND_LOST, "lost", "RDS page 4, legend word in the lost colour") \
  X(RDS_ALL_CLEAN, "all clean", "RDS page 4, right of a block bar with nothing corrected or lost") \
  X(RDS_FMT_BLOCK, "%u %% \xC2\xB7 %u.%u %%", "RDS page 4, right of a block bar: corrected, then lost with one decimal") \
  X(RDS_GROUP_TYPES, "GROUP TYPES SENT", "RDS page 4, label above the group type bars") \
  X(RDS_NO_GROUPS, "No groups in the last minute", "RDS page 4, in place of the group type bars") \
  X(RDS_FMT_PERCENT, "%u %%", "RDS page 4, share of a group type") \
  X(RDS_FMT_SYNC_S, "%us", "RDS page 4, sync tile, seconds locked") \
  X(RDS_FMT_SYNC_M_S, "%um%02us", "RDS page 4, sync tile, minutes and seconds locked") \
  X(RDS_FMT_RATE, "%.1f", "RDS page 4, groups a second over the last minute") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 last %u s", "RDS page 4, header context, the frequency then how many seconds the counts cover") \
  X(RDS_COUNTRY_UNLISTED, "not listed", "RDS page 1, PI tile, an ECC heard with no country for it and this PI") \
  X(RDS_NO_TYPES, "No block B read clean in the last minute", "RDS page 4, in place of the group type bars when groups came but no type could be read") \
  X(RDS_FMT_SYNC_H_M, "%uh%02um", "RDS page 4, sync tile, hours and minutes locked") \
  X(RDS_FMT_EON_MORE, "+%u more", "RDS page 3, right of the EON label, networks that did not fit") \
  X(RDS_EON_MORE_HEARD, "+ more", "RDS page 3, right of the EON label, when more networks were named than the radio keeps, so how many is not known") \
  X(RDS_MORE, "more", "RDS page 3, after an other network's frequencies when it named more than the radio keeps") \
  X(RDS_BLOCK_A, "A", "RDS page 4, block letter beside its bar") \
  X(RDS_BLOCK_B, "B", "RDS page 4, block letter beside its bar") \
  X(RDS_BLOCK_C, "C", "RDS page 4, block letter beside its bar") \
  X(RDS_BLOCK_D, "D", "RDS page 4, block letter beside its bar") \
  X(RDS_FMT_GROUP_A, "%uA", "RDS page 4, group type label, A version") \
  X(RDS_FMT_GROUP_B, "%uB", "RDS page 4, group type label, B version") \
  X(RDS_PTYN, "PTYN", "RDS page 1, label of the programme type name") \
  X(RDS_CT, "CT", "RDS page 1, label of the station clock") \
  X(RDS_BLER, "BLER", "RDS page 4, stat tile label") \
  X(RDS_FMT_PAGE, "%u/%u", "Page position in the RDS header, this page of `SCREEN_RDS_PAGES`") \
  X(RECOVERY_TITLE, "Recovery", "Title of the recovery screen") \
  X(RECOVERY_SAFE_DISPLAY, "Safe Display", "Header context on the recovery screen") \
  X(RECOVERY_HINT, "Knob held at power-on: safe theme and rotation", "Left hint on the recovery screen") \
  X(RADIO_VEIL_UPDATING, "UPDATING FIRMWARE", "Caption on the OTA progress veil") \
  X(RADIO_VEIL_KEEP_POWER, "Do not remove power", "Foot line on the OTA progress veil") \
  X(THEME_CUSTOM, "Custom", "Name of the custom theme, shown as the Theme row value in the menu") \
  X(RADIO_SQL, "SQ:", "Tile label on the radio screen, `kNames[]`, before the squelch value") \
  X(RADIO_BW_TILE, "BW:", "Tile label on the radio screen, before the filter value") \
  X(RADIO_VOL, "V:", "Tile label on the radio screen, before the volume") \
  X(RADIO_MUTE, "MUTE", "V: tile value while a person has muted the radio") \
  X(TUNER_OK, "OK", "Tuner error text, never shown because the fault is only drawn when the error is not OK") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Tuner not found on I2C bus", "Detail of the Tuner fault message") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tuner did not become ready", "Detail of the Tuner fault message") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Tuner identification failed", "Detail of the Tuner fault message") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "No firmware patch for this tuner", "Detail of the Tuner fault message") \
  X(TUNER_UNKNOWN_TUNER_PART, "Unknown tuner", "Detail of the Tuner fault message") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tuner rejected write", "Detail of the Tuner fault message") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tuner did not respond to read", "Detail of the Tuner fault message") \
  X(TUNER_OUT_OF_RANGE, "Out of Range", "Detail of the Tuner fault message") \
  X(TUNER_UNKNOWN, "Unknown", "Detail of the Tuner fault message, and the part name on the boot screen when the chip is not one this build knows") \
  X(BAND_LW, "LW", "Band name, `kBands[]`; radio screen band mark and Bandwidth page context") \
  X(BAND_MW, "MW", "Band name") \
  X(BAND_SW, "SW", "Band name") \
  X(BAND_OIRT, "OIRT", "Band name") \
  X(BAND_FM, "FM", "Band name") \
  X(BAND_LONG_LW, "Long Wave", "Band's full name, the choice of bands for a typed number") \
  X(BAND_LONG_MW, "Medium Wave", "Band's full name, the choice of bands for a typed number") \
  X(BAND_LONG_SW, "Shortwave", "Band's full name, the choice of bands for a typed number") \
  X(BAND_LONG_OIRT, "OIRT", "Band's full name, the choice of bands for a typed number") \
  X(BAND_LONG_FM, "FM", "Band's full name, the choice of bands for a typed number") \
  X(MENU_FMT_TUNE_TO, "Tune %s to", "Title of the choice of bands for a typed number, %s the digits typed") \
  X(PTY_NONE, "None", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_NEWS, "News", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_CURRENT_AFFAIRS, "Current Affairs", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_INFORMATION, "Information", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_SPORT, "Sport", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_EDUCATION, "Education", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_DRAMA, "Drama", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_CULTURE, "Culture", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_SCIENCE, "Science", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_VARIED, "Varied", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_POP_MUSIC, "Pop Music", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_ROCK_MUSIC, "Rock Music", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_EASY_LISTENING, "Easy Listening", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_LIGHT_CLASSICAL, "Light Classical", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_SERIOUS_CLASSICAL, "Classical", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_OTHER_MUSIC, "Other Music", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_WEATHER, "Weather", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_FINANCE, "Finance", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_CHILDREN, "Children", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_SOCIAL_AFFAIRS, "Social Affairs", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_RELIGION, "Religion", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_PHONE_IN, "Phone-In", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_TRAVEL, "Travel", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_LEISURE, "Leisure", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_JAZZ_MUSIC, "Jazz", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_COUNTRY_MUSIC, "Country Music", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_NATIONAL_MUSIC, "National Music", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_OLDIES_MUSIC, "Oldies", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_FOLK_MUSIC, "Folk Music", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_DOCUMENTARY, "Documentary", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_ALARM_TEST, "Alarm Test", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_ALARM, "Alarm", "PTY name, on RDS page 1 and the DX page") \
  X(PTY_NA_NEWS, "News", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_INFORMATION, "Information", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_SPORTS, "Sports", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_TALK, "Talk", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_ROCK, "Rock", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_CLASSIC_ROCK, "Classic Rock", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_ADULT_HITS, "Adult Hits", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_SOFT_ROCK, "Soft Rock", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_TOP_40, "Top 40", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_COUNTRY, "Country", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_OLDIES, "Oldies", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_SOFT, "Soft", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_NOSTALGIA, "Nostalgia", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_JAZZ, "Jazz", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_CLASSICAL, "Classical", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Rhythm and Blues", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Soft Rhythm and Blues", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Foreign Language", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Religious Music", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_RELIGIOUS_TALK, "Religious Talk", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_PERSONALITY, "Personality", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_PUBLIC, "Public", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_COLLEGE, "College", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_SPANISH_TALK, "Spanish Talk", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_SPANISH_MUSIC, "Spanish Music", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_HIP_HOP, "Hip-Hop", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_UNASSIGNED, "Unassigned", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_WEATHER, "Weather", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_EMERGENCY_TEST, "Emergency Test", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(PTY_NA_EMERGENCY, "Emergency", "RBDS programme type name, NRSC-4-B table F.2, on North America") \
  X(COVERAGE_LOCAL, "Local", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_INTERNATIONAL, "International", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_NATIONAL, "National", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_SUPRA_REGIONAL, "Supra-Regional", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_1, "Regional 1", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_2, "Regional 2", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_3, "Regional 3", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_4, "Regional 4", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_5, "Regional 5", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_6, "Regional 6", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_7, "Regional 7", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_8, "Regional 8", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_9, "Regional 9", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_10, "Regional 10", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_11, "Regional 11", "Coverage area from the PI, AREA field on RDS page 1") \
  X(COVERAGE_REGIONAL_12, "Regional 12", "Coverage area from the PI, AREA field on RDS page 1") \
  X(LANG_ALBANIAN, "Albanian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_BRETON, "Breton", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_CATALAN, "Catalan", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_CROATIAN, "Croatian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_WELSH, "Welsh", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_CZECH, "Czech", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_DANISH, "Danish", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_GERMAN, "German", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ENGLISH, "English", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SPANISH, "Spanish", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ESPERANTO, "Esperanto", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ESTONIAN, "Estonian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_BASQUE, "Basque", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_FAROESE, "Faroese", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_FRENCH, "French", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_FRISIAN, "Frisian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_IRISH, "Irish", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_GAELIC, "Gaelic", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_GALICIAN, "Galician", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ICELANDIC, "Icelandic", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ITALIAN, "Italian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_LAPPISH, "Sami", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_LATIN, "Latin", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_LATVIAN, "Latvian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_LUXEMBOURGIAN, "Luxembourgish", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_LITHUANIAN, "Lithuanian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_HUNGARIAN, "Hungarian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_MALTESE, "Maltese", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_DUTCH, "Dutch", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_NORWEGIAN, "Norwegian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_OCCITAN, "Occitan", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_POLISH, "Polish", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_PORTUGUESE, "Portuguese", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ROMANIAN, "Romanian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ROMANSH, "Romansh", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SERBIAN, "Serbian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SLOVAK, "Slovak", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SLOVENE, "Slovene", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_FINNISH, "Finnish", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SWEDISH, "Swedish", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_TURKISH, "Turkish", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_FLEMISH, "Flemish", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_WALLOON, "Walloon", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_BACKGROUND, "Background", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ZULU, "Zulu", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_VIETNAMESE, "Vietnamese", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_UZBEK, "Uzbek", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_URDU, "Urdu", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_UKRAINIAN, "Ukrainian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_THAI, "Thai", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_TELUGU, "Telugu", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_TATAR, "Tatar", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_TAMIL, "Tamil", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_TAJIK, "Tajik", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SWAHILI, "Swahili", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SRANAN_TONGO, "Sranan Tongo", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SOMALI, "Somali", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SINHALA, "Sinhala", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SHONA, "Shona", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_SERBO_CROAT, "Serbo-Croat", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_RUTHENIAN, "Ruthenian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_RUSSIAN, "Russian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_QUECHUA, "Quechua", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_PASHTO, "Pashto", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_PUNJABI, "Punjabi", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_PERSIAN, "Persian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_PAPIAMENTO, "Papiamento", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ORIYA, "Odia", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_NEPALI, "Nepali", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_NDEBELE, "Ndebele", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_MARATHI, "Marathi", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_MOLDAVIAN, "Moldavian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_MALAY, "Malay", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_MALAGASY, "Malagasy", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_MACEDONIAN, "Macedonian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_LAO, "Lao", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_KOREAN, "Korean", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_KHMER, "Khmer", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_KAZAKH, "Kazakh", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_KANNADA, "Kannada", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_JAPANESE, "Japanese", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_INDONESIAN, "Indonesian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_HINDI, "Hindi", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_HEBREW, "Hebrew", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_HAUSA, "Hausa", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_GUARANI, "Guarani", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_GUJARATI, "Gujarati", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_GREEK, "Greek", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_GEORGIAN, "Georgian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_FULANI, "Fulani", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_DARI, "Dari", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_CHUVASH, "Chuvash", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_CHINESE, "Chinese", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_BURMESE, "Burmese", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_BULGARIAN, "Bulgarian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_BENGALI, "Bengali", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_BELARUSIAN, "Belarusian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_BAMBARA, "Bambara", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_AZERBAIJANI, "Azerbaijani", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ASSAMESE, "Assamese", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ARMENIAN, "Armenian", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_ARABIC, "Arabic", "Programme language from group 1A, LANG on RDS page 1") \
  X(LANG_AMHARIC, "Amharic", "Programme language from group 1A, LANG on RDS page 1") \
  X(RTP_OTHER, "OTHER", "RT+ content type label on RDS page 2") \
  X(RTP_TITLE, "TITLE", "RT+ content type label on RDS page 2") \
  X(RTP_ALBUM, "ALBUM", "RT+ content type label on RDS page 2") \
  X(RTP_TRACK, "TRACK", "RT+ content type label on RDS page 2") \
  X(RTP_ARTIST, "ARTIST", "RT+ content type label on RDS page 2") \
  X(RTP_WORK, "WORK", "RT+ content type label on RDS page 2") \
  X(RTP_MOVEMENT, "MOVEMENT", "RT+ content type label on RDS page 2") \
  X(RTP_CONDUCTOR, "CONDUCTOR", "RT+ content type label on RDS page 2") \
  X(RTP_COMPOSER, "COMPOSER", "RT+ content type label on RDS page 2") \
  X(RTP_BAND, "BAND", "RT+ content type label on RDS page 2") \
  X(RTP_COMMENT, "COMMENT", "RT+ content type label on RDS page 2") \
  X(RTP_GENRE, "GENRE", "RT+ content type label on RDS page 2") \
  X(RTP_NEWS, "NEWS", "RT+ content type label on RDS page 2") \
  X(RTP_LOCAL, "LOCAL NEWS", "RT+ content type label on RDS page 2") \
  X(RTP_STOCKS, "STOCKS", "RT+ content type label on RDS page 2") \
  X(RTP_SPORT, "SPORT", "RT+ content type label on RDS page 2") \
  X(RTP_LOTTERY, "LOTTERY", "RT+ content type label on RDS page 2") \
  X(RTP_HOROSCOPE, "HOROSCOPE", "RT+ content type label on RDS page 2") \
  X(RTP_DIVERSION, "DIVERSION", "RT+ content type label on RDS page 2") \
  X(RTP_HEALTH, "HEALTH", "RT+ content type label on RDS page 2") \
  X(RTP_EVENT, "EVENT", "RT+ content type label on RDS page 2") \
  X(RTP_SCENE, "SCENE", "RT+ content type label on RDS page 2") \
  X(RTP_CINEMA, "CINEMA", "RT+ content type label on RDS page 2") \
  X(RTP_FUN, "FUN", "RT+ content type label on RDS page 2") \
  X(RTP_DATE, "DATE", "RT+ content type label on RDS page 2") \
  X(RTP_WEATHER, "WEATHER", "RT+ content type label on RDS page 2") \
  X(RTP_TRAFFIC, "TRAFFIC", "RT+ content type label on RDS page 2") \
  X(RTP_ALARM, "ALARM", "RT+ content type label on RDS page 2") \
  X(RTP_ADVERT, "ADVERT", "RT+ content type label on RDS page 2") \
  X(RTP_URL, "URL", "RT+ content type label on RDS page 2") \
  X(RTP_STATION, "STATION", "RT+ content type label on RDS page 2") \
  X(RTP_PROGRAM, "PROGRAM", "RT+ content type label on RDS page 2") \
  X(RTP_NEXT, "NEXT", "RT+ content type label on RDS page 2") \
  X(RTP_PART, "PART", "RT+ content type label on RDS page 2") \
  X(RTP_HOST, "HOST", "RT+ content type label on RDS page 2") \
  X(RTP_EDITORS, "EDITORS", "RT+ content type label on RDS page 2") \
  X(RTP_FREQUENCY, "FREQUENCY", "RT+ content type label on RDS page 2") \
  X(RTP_HOMEPAGE, "HOMEPAGE", "RT+ content type label on RDS page 2") \
  X(RTP_CHANNEL, "CHANNEL", "RT+ content type label on RDS page 2") \
  X(RTP_HOTLINE, "HOTLINE", "RT+ content type label on RDS page 2") \
  X(RTP_STUDIO, "STUDIO", "RT+ content type label on RDS page 2") \
  X(RTP_PHONE, "PHONE", "RT+ content type label on RDS page 2") \
  X(RTP_SMS, "SMS", "RT+ content type label on RDS page 2") \
  X(RTP_EMAIL, "EMAIL", "RT+ content type label on RDS page 2") \
  X(RTP_MMS, "MMS", "RT+ content type label on RDS page 2") \
  X(RTP_CHAT, "CHAT", "RT+ content type label on RDS page 2") \
  X(RTP_VOTE, "VOTE", "RT+ content type label on RDS page 2") \
  X(RTP_PLACE, "PLACE", "RT+ content type label on RDS page 2") \
  X(RTP_APPOINTMENT, "APPOINTMENT", "RT+ content type label on RDS page 2") \
  X(RTP_ID, "ID", "RT+ content type label on RDS page 2") \
  X(RTP_PURCHASE, "PURCHASE", "RT+ content type label on RDS page 2") \
  X(COMMON_FMT_CLOCK, "%02u:%02u", "Clock in every header, and the CT field on RDS page 1; hour and minute") \
  X(DAY_SUNDAY, "SUNDAY", "Day name in the date line under the amber panel, in capitals") \
  X(DAY_MONDAY, "MONDAY", "Day name in the date line under the amber panel, in capitals") \
  X(DAY_TUESDAY, "TUESDAY", "Day name in the date line under the amber panel, in capitals") \
  X(DAY_WEDNESDAY, "WEDNESDAY", "Day name in the date line under the amber panel, in capitals") \
  X(DAY_THURSDAY, "THURSDAY", "Day name in the date line under the amber panel, in capitals") \
  X(DAY_FRIDAY, "FRIDAY", "Day name in the date line under the amber panel, in capitals") \
  X(DAY_SATURDAY, "SATURDAY", "Day name in the date line under the amber panel, in capitals") \
  X(MONTH_JANUARY, "January", "Month name in the date line under the amber panel") \
  X(MONTH_FEBRUARY, "February", "Month name in the date line under the amber panel") \
  X(MONTH_MARCH, "March", "Month name in the date line under the amber panel") \
  X(MONTH_APRIL, "April", "Month name in the date line under the amber panel") \
  X(MONTH_MAY, "May", "Month name in the date line under the amber panel") \
  X(MONTH_JUNE, "June", "Month name in the date line under the amber panel") \
  X(MONTH_JULY, "July", "Month name in the date line under the amber panel") \
  X(MONTH_AUGUST, "August", "Month name in the date line under the amber panel") \
  X(MONTH_SEPTEMBER, "September", "Month name in the date line under the amber panel") \
  X(MONTH_OCTOBER, "October", "Month name in the date line under the amber panel") \
  X(MONTH_NOVEMBER, "November", "Month name in the date line under the amber panel") \
  X(MONTH_DECEMBER, "December", "Month name in the date line under the amber panel") \
  X(DATE_SUFFIX_TH, "th", "Ordinal suffix on the day of the month") \
  X(DATE_SUFFIX_ST, "st", "Ordinal suffix on the day of the month") \
  X(DATE_SUFFIX_ND, "nd", "Ordinal suffix on the day of the month") \
  X(DATE_SUFFIX_RD, "rd", "Ordinal suffix on the day of the month") \
  X(DATE_FMT_LINE, "%s, %d%s %s %d", "The date line, day name, day and suffix, month, year") \
  X(DX_NOW, "now", "Scope page header context, age of the sweep shown") \
  X(DX_FMT_MIN_AGO, "%u min ago", "Scope page header context") \
  X(DX_FMT_H_AGO, "%u h ago", "Scope page header context") \
  X(DX_FMT_D_AGO, "%u d ago", "Scope page header context") \
  X(THEME_NIGHTWATCH, "Nightwatch", "Theme name, Theme row value in the menu via `themeAt()->name`") \
  X(THEME_DAYLIGHT, "Daylight", "Theme name") \
  X(THEME_RED_NIGHT, "Red Night", "Theme name") \
  X(THEME_PHOSPHOR, "Phosphor", "Theme name") \
  X(THEME_CLEAR, "Clear", "Theme name") \
  X(THEME_SLATE, "Slate", "Theme name") \
  X(THEME_PAPER, "Paper", "Theme name") \
  X(THEME_LCD, "LCD", "Theme name") \
  X(THEME_EMBER, "Ember", "Theme name") \
  X(THEME_CLEAR_DAY, "Clear Day", "Theme name") \
  X(THEME_HIGH_CONTRAST, "High Contrast", "Theme name") \
  X(THEME_MONO, "Mono", "Theme name") \
  X(THEME_HI_FI, "Hi-Fi", "Theme name") \
  X(THEME_VIOLET, "Violet", "Theme name") \
  X(THEME_BLOSSOM, "Blossom", "Theme name")
// clang-format on
