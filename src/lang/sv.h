/*
 * The panel's texts in Swedish. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_SV(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "FM-skanning") \
  X(MENU_SCAN_MW, "AM-skanning") \
  X(MENU_SCAN_SW, "SW-skanning") \
  X(MENU_SCAN_LW, "LW-skanning") \
  X(MENU_SQUELCH, "Spärrläge") \
  X(MENU_SQUELCH_LEVEL, "Spärrnivå") \
  X(MENU_SQUELCH_FLOOR, "Spärrgolv") \
  X(MENU_SQUELCH_GROUP, "Brusspärr") \
  X(MENU_VOLUME_AGC, "Volym-AGC") \
  X(MENU_AGC_BOOST, "AGC-lyft") \
  X(MENU_MUTE_RAMP, "Tystningsramp") \
  X(MENU_BAND_PLAN, "Bandplan") \
  X(MENU_TUNING_STEP, "Steglängd") \
  X(MENU_SEEK_SENSITIVITY, "Sökkänslighet") \
  X(MENU_DE_EMPHASIS, "De-emfas") \
  X(MENU_FORCED_MONO, "Tvinga mono") \
  X(MENU_STEREO_BLEND, "Stereoblandning") \
  X(MENU_ST_HI_BLEND, "Stereo-/diskantblandning") \
  X(MENU_HIGH_CUT, "Lågpass") \
  X(MENU_NOISE_BLANKER, "Noise blanker") \
  X(COMMON_RDS_DECODER, "RDS-dekoder") \
  X(MENU_RDS_REGION, "RDS-region") \
  X(MENU_MW_SPACING, "MW-steg") \
  X(MENU_FILTER_WIDTH, "Filterbandbredd") \
  X(MENU_SOFT_MUTE, "Softmute") \
  X(MENU_DWELL, "Väntetid") \
  X(MENU_STOP_ON, "Stoppa vid") \
  X(MENU_SCAN, "Omfång") \
  X(MENU_MEMORY_FROM, "Förval från") \
  X(MENU_MEMORY_TO, "Förval till") \
  X(MENU_PRESET_RANGE, "Förvalsområde") \
  X(MENU_LOOP_THE_BAND, "Upprepa") \
  X(MENU_DX_WIDTH, "DX-bandbredd") \
  X(MENU_MUTE_WHILE_SCANNING, "Tyst vid skanning") \
  X(MENU_AUTO_LOG_NEW, "Autologgning") \
  X(MENU_LOG_RADIO_TEXT, "Logga radiotext") \
  X(MENU_WATCH_PRESETS, "Bevaka förval") \
  X(MENU_SCOPE_BASELINE, "Scope-referens") \
  X(MENU_FMT_SWEEP_N, "Svep %u") \
  X(DX_FMT_WATCH_UP, "%s upp %d dB") \
  X(MENU_LEARN_LOCALS, "Lär in lokala stationer") \
  X(MENU_START_SCAN, "Starta skanning") \
  X(MENU_DAY_THEME, "Dagtema") \
  X(MENU_NIGHT_THEME, "Natt-tema") \
  X(MENU_THEME, "Tema") \
  X(MENU_BRIGHTNESS, "Ljusstyrka") \
  X(MENU_DIM_LEVEL, "Dimrad nivå") \
  X(MENU_DIM_AFTER, "Dimra efter") \
  X(MENU_ROTATION, "Skärmrotation") \
  X(COMMON_BATTERY, "Batteri") \
  X(MENU_BATTERY_CHECK, "Batterikontroll") \
  X(MENU_LANGUAGE, "Språk \xC2\xB7 Language") \
  X(MENU_ENCODER, "Pulsgivartyp") \
  X(MENU_DIRECTION, "Vridriktning") \
  X(MENU_ENCODER_GROUP, "Pulsgivare") \
  X(MENU_KEY_BEEPS, "Knappljud") \
  X(MENU_BAND_EDGE_BEEP, "Pip vid bandkant") \
  X(MENU_START_CHIME, "Startljud") \
  X(MENU_KEYPAD_TIMEOUT, "Inmatningstid") \
  X(MENU_CALIBRATE_TOUCH, "Kalibrera touch") \
  X(MENU_CLOCK_FROM_NETWORK, "Nätverkstid") \
  X(MENU_WEB_PIN, "Webb-PIN") \
  X(MENU_NEW_PIN, "Ny PIN") \
  X(MENU_FMT_DIGIT_OF, "Siffra %u av %u") \
  X(MENU_STATUS, "Anslutningsstatus") \
  X(MENU_WEB_SERVER, "Webbserver") \
  X(MENU_FMT_PC_CONNECTED, "Anslutna: %u") \
  X(MENU_WEB_ADDRESS, "Webbadress") \
  X(MENU_IP_ADDRESS, "IP-adress") \
  X(MENU_WI_FI_NAME, "Wi-Fi-nätverk") \
  X(MENU_HOTSPOT_NAME, "Hotspotnamn") \
  X(MENU_MAC, "MAC-adress") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi-signal") \
  X(MENU_NETWORK_INFO, "Nätverksinfo") \
  X(MENU_RESTART, "Starta om") \
  X(MENU_AUTO_OFF, "Auto av") \
  X(MENU_UPDATE_CHECK, "Sök uppdateringar") \
  X(MENU_FIRMWARE_UPDATE, "Uppdatering") \
  X(MENU_FMT_UPDATE_TO, "Uppdatera till %s") \
  X(MENU_UPDATE_TITLE, "Uppdatering finns") \
  X(MENU_UPDATE_THIS_RADIO, "Denna radio") \
  X(MENU_UPDATE_NEW_VERSION, "Ny version") \
  X(MENU_UPDATE_DOWNLOAD, "Storlek") \
  X(COMMON_SETTINGS, "Inställningar") \
  X(COMMON_KEPT, "Behålls") \
  X(MENU_UPDATE_NOW, "Uppdatera") \
  X(MENU_LATER, "Senare") \
  X(MENU_UPDATE_UP_TO_DATE, "Aktuell") \
  X(MENU_UPDATE_CHECKING, "Söker") \
  X(MENU_UPDATE_NOT_CHECKED, "Ej sökt") \
  X(MENU_UPDATE_CHECK_FAILED, "Sökfel") \
  X(MENU_NOTE_NO_UPDATE, "Ingen uppdatering") \
  X(MENU_VERSION, "Firmwareversion") \
  X(MENU_RUNNING_FROM, "Startkälla") \
  X(MENU_UPTIME, "Drifttid") \
  X(MENU_BATTERY_VOLTAGE, "Batteri") \
  X(MENU_RESET_REASON, "Omstartsorsak") \
  X(MENU_CPU_CORE_0, "CPU-kärna 0") \
  X(MENU_CPU_CORE_1, "CPU-kärna 1") \
  X(MENU_FREE_HEAP, "Ledig heap") \
  X(MENU_LOWEST_HEAP, "Lägsta heap") \
  X(MENU_LARGEST_BLOCK, "Största block") \
  X(MENU_LVGL_POOL, "LVGL-pool") \
  X(MENU_CHIP, "Krets") \
  X(MENU_FLASH_SIZE, "Flashstorlek") \
  X(RESET_POWER, "Ström på") \
  X(RESET_SOFTWARE, "Mjukvara") \
  X(RESET_PANIC, "Krasch") \
  X(RESET_INT_WATCHDOG, "IRQ-watchdog") \
  X(RESET_TASK_WATCHDOG, "Task-watchdog") \
  X(RESET_DEEP_SLEEP, "Djupsömn") \
  X(RESET_BROWNOUT, "Underspänning") \
  X(RESET_PIN, "Reset-stift") \
  X(RESET_UNKNOWN, "Okänd") \
  X(RESET_ASKED, "Begärd omstart") \
  X(RESET_BOOT_WATCHDOG, "Start-watchdog") \
  X(RESET_ROLLBACK, "Återgång") \
  X(RESET_DISPLAY, "Skärmfel") \
  X(RESET_UPDATE, "Uppdatering") \
  X(MENU_DEVELOPER, "Utvecklare") \
  X(MENU_BUILD, "Bygge") \
  X(MENU_LICENSE, "Licens") \
  X(ABOUT_BUILD_UNKNOWN, "okänt") \
  X(MENU_GO_TO, "Gå till") \
  X(MENU_GO_TO_BANDWIDTH, "Bandbredd") \
  X(MENU_GO_TO_DX, "DX-läge") \
  X(MENU_GO_TO_SCOPE, "Bandscope") \
  X(MENU_GO_TO_BAND, "Nästa band") \
  X(MENU_GO_TO_LOG, "Logga stationen") \
  X(MENU_GO_TO_SAVE, "Spara stationen") \
  X(MENU_FMT_SAVED_AS, "Sparad som P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Redan P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Förvalen är fulla") \
  X(MENU_GO_TO_SLEEP, "Vila") \
  X(MENU_STATIONS, "Stationer") \
  X(MENU_MEMORY, "Förval") \
  X(MENU_AUDIO, "Ljud") \
  X(MENU_FM_SETUP, "FM-mottagning") \
  X(MENU_AM_SETUP, "AM-mottagning") \
  X(MENU_DX_SETUP, "DX-skanner") \
  X(MENU_DISPLAY, "Skärm") \
  X(MENU_CONTROLS, "Reglage") \
  X(MENU_CONNECTIVITY, "Anslutning") \
  X(MENU_DIAGNOSTICS, "Diagnostik") \
  X(MENU_ABOUT, "Om") \
  X(MENU_REGION_WIDE, "Bred") \
  X(MENU_REGION_WORLD, "Hela världen") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Nordamerika") \
  X(MENU_WIFI_JOINED, "Ansluten") \
  X(MENU_WIFI_JOINING, "Ansluter") \
  X(COMMON_OFF, "Av") \
  X(MENU_FMT_KB_OF_KB, "%u av %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V vid start") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, för %u min sedan") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, för %u h sedan") \
  X(MENU_FMT_AGC_GAIN_NOW, "Just nu %+d dB") \
  X(MENU_STATION_LOG, "Loggbok") \
  X(COMMON_NONE, "Ingen") \
  X(MENU_FMT_SCAN_PROGRESS, "Skannar %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Sparat %u") \
  X(MENU_FMT_SCAN_STOPPED, "Stoppad, sparat %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Fullt, %u över") \
  X(COMMON_YES, "Ja") \
  X(COMMON_NO, "Nej") \
  X(MENU_UNKNOWN_VALUE, "Okänd") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Upp och ned") \
  X(MENU_CUT_ONLY, "Bara sänka") \
  X(MENU_NEVER, "Aldrig") \
  X(MENU_BATTERY_PER_CENT, "Procent") \
  X(MENU_BATTERY_VOLTS, "Volt") \
  X(MENU_KEYS, "Bara siffror") \
  X(MENU_KEYS_LONG, "Kort & långt tryck") \
  X(MENU_EVERY_PRESS, "Varje tryck") \
  X(MENU_OPTICAL, "Optisk") \
  X(MENU_REVERSED, "Omvänd") \
  X(MENU_SQUELCH_MAN, "Manuell") \
  X(MENU_ANY_PI, "Varje PI") \
  X(MENU_STOP_NEVER, "Aldrig") \
  X(MENU_NEW_ONLY, "Bara nya stationer") \
  X(MENU_WHOLE_BAND, "Hela bandet") \
  X(MENU_MEMORY_ONLY, "Bara förval") \
  X(MENU_BAND_MEMORY, "Band + förval") \
  X(COMMON_ON, "På") \
  X(MENU_HINT_RESTART, "Gäller efter omstart") \
  X(MENU_TITLE, "Meny") \
  X(COMMON_FM_ONLY, "Bara FM") \
  X(MENU_NOTE_OFF_PLAN, "Ej i bandplanen") \
  X(MENU_NOTE_SWITCH_TO_FM, "Byt till FM först") \
  X(MENU_NOTE_RADIO_BUSY, "Upptagen - försök igen") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Slå på RDS först") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Firmware testas - vänta") \
  X(MENU_NOTE_NOT_STORED, "Ej sparat - återställt") \
  X(MENU_NOTE_NOT_PUT_BACK, "Upptagen - ej återställt") \
  X(MENU_NOTE_NOT_SAVED, "Ej sparat") \
  X(MENU_SAVE, "Spara") \
  X(RADIO_PRESS_TO_START, "Tryck för start") \
  X(COMMON_STILL_TUNING, "Söker - vänta") \
  X(COMMON_SCAN_RUNNING, "Skannar - vänta") \
  X(RADIO_FMT_NO_BAND, "%s: inget band") \
  X(COMMON_NOT_LOGGED, "Ej loggad") \
  X(RADIO_FMT_LOGGED, "Loggad %s") \
  X(RADIO_CHECKING_UPDATES, "SÖKER UPPDATERINGAR\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Skannar %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Uppdateringsfel") \
  X(RADIO_GOING_TO_SLEEP, "Går i viloläge") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Väck: tryck på ratten") \
  X(RADIO_OLD_IMAGE_KEPT, "Förra firmware behålls") \
  X(RDS_SPEECH, "Tal") \
  X(RDS_MUSIC, "Musik") \
  X(COMMON_ALREADY_LOGGED, "Redan loggad") \
  X(DX_RDS_IS_OFF, "RDS är av") \
  X(DX_SCANNING, "Skannar") \
  X(DX_SCAN_FINISHED, "Skanning klar") \
  X(DX_NOTHING_TO_SCAN, "Inget att skanna") \
  X(DX_CHECKING_FOR_UPDATES, "Söker uppdateringar") \
  X(DX_A_SCAN_IS_RUNNING, "Skannar redan") \
  X(DX_CAUGHT_LIST_UNREAD, "Fångstlista saknas") \
  X(DX_NO_MEMORY, "Inga förval") \
  X(DX_NOT_STARTED, "Ej startad") \
  X(DX_SWEEPING, "Sveper") \
  X(COMMON_FMT_TUNE_TYPED, "Ställ in %s") \
  X(RADIO_SQL_OFF, "AV") \
  X(DX_FMT_ROWS_OF, "%u-%u av %u") \
  X(DX_FMT_FOUND, "Hittade %u") \
  X(DX_MODE_SWEEPING, "SVEPER") \
  X(DX_MODE_LEARN_LOCALS, "LÄR IN LOKALA") \
  X(DX_FMT_MODE_MEMORY, "FÖRVAL %u-%u") \
  X(DX_MODE_WHOLE_BAND, "HELA BANDET") \
  X(DX_MODE_BAND_MEMORY, "BAND + FÖRVAL") \
  X(DX_RULE_NO_STOP, "EJ STOPP") \
  X(DX_RULE_STOP_ON_PI, "STOPP VID PI") \
  X(DX_RULE_STOP_ON_NEW, "STOPP VID NY") \
  X(DX_FMT_NOT_PRESET, "ej P%02d") \
  X(DX_CONTEXT_SWEEPING, "sveper") \
  X(DX_PRESS_TO_SWEEP, "Tryck för svep") \
  X(SCOPE_FMT_TITLE, "%s-scope") \
  X(SCOPE_FULL, "Hela") \
  X(DX_FIXED, "FAST") \
  X(DX_FMT_VS, "MOT %s") \
  X(DX_FMT_MEDIAN_OF, "MEDIAN AV %u") \
  X(DX_FMT_FLOOR, "GOLV %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Stäng") \
  X(RECOVERY_PRESS_AGAIN, "Tryck igen") \
  X(PAGE_TUNE, "Ställ in") \
  X(PAGE_DELETE, "Radera") \
  X(PAGE_LEVEL, "Nivå") \
  X(PAGE_NOISE, "Brus") \
  X(PAGE_MULTIPATH, "Flerväg") \
  X(PAGE_COCHANNEL, "Samkanal") \
  X(PAGE_WIDTH, "Bredd") \
  X(PAGE_NOTE_CHANGED, "Ändrad sedan den öppnades") \
  X(PAGE_NOTE_NOT_DELETED, "Ej raderad") \
  X(PAGE_FMT_SAVED, "Sparad %s") \
  X(PAGE_TIME_UNKNOWN, "Okänd tid") \
  X(BW_TITLE, "Bandbredd") \
  X(KEYPAD_TITLE, "Frekvens") \
  X(KEYPAD_CANCEL, "Avbryt") \
  X(DX_CATCHES_EMPTY_TITLE, "Inga fångster än") \
  X(DX_CATCHES_EMPTY_NOTE, "Bekräftade stationer från denna session visas här") \
  X(DX_TITLE_CATCHES, "Fångster") \
  X(DX_NEW, "NY") \
  X(DX_READY, "Redo") \
  X(DX_DWELL, "TID") \
  X(DX_STOPPED, "STOPPAD") \
  X(DX_TITLE_SCANNER, "Skanner") \
  X(DX_LEVEL, "NIVÅ") \
  X(DX_RISE, "DIFF") \
  X(DX_NO_STATION_YET, "Ingen station än") \
  X(DX_NO_RDS_YET, "Inget RDS än") \
  X(DX_NO_ID, "INGET ID") \
  X(RDS_TITLE_TEXT, "Radiotext") \
  X(RDS_TITLE_NETWORKS, "Nätverk") \
  X(RDS_TITLE_DECODER, "Dekoder") \
  X(RDS_NO_COUNTRY, "inget ECC hört") \
  X(RDS_MS_UNKNOWN, "M/T") \
  X(RDS_RTPLUS_NONE, "Inget RT+ från denna station") \
  X(RDS_RTPLUS_WAIT, "Inga RT+-taggar för texten än") \
  X(RDS_AF_TITLE, "ALTERNATIVA FREKVENSER") \
  X(RDS_EON_TITLE, "ANDRA STATIONER, EON") \
  X(RDS_NONE_HEARD, "Inget hört") \
  X(RDS_SYNC_CAPS, "SYNK") \
  X(RDS_GROUPS_S_CAPS, "GRUPP/S") \
  X(RDS_LOST, "TAPPADE") \
  X(RDS_EACH_BLOCK, "VARJE BLOCK") \
  X(RDS_LEGEND_CLEAN, "ren") \
  X(RDS_LEGEND_FIXED, "rättad") \
  X(RDS_LEGEND_LOST, "tappad") \
  X(RDS_ALL_CLEAN, "alla rena") \
  X(RDS_GROUP_TYPES, "SÄNDA GRUPPTYPER") \
  X(RDS_NO_GROUPS, "Inga grupper senaste minuten") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 %u s") \
  X(RDS_COUNTRY_UNLISTED, "ej i listan") \
  X(RDS_NO_TYPES, "Inget rent block B senaste minuten") \
  X(RDS_FMT_EON_MORE, "+%u till") \
  X(RDS_EON_MORE_HEARD, "+ fler") \
  X(RDS_MORE, "fler") \
  X(RADIO_VEIL_UPDATING, "UPPDATERAR FIRMWARE") \
  X(RADIO_VEIL_KEEP_POWER, "Bryt inte strömmen") \
  X(RADIO_MUTE, "TYST") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Ingen tuner på I2C-bussen") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tunern blev aldrig redo") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Tunern ej identifierad") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Ingen patch för denna tuner") \
  X(TUNER_UNKNOWN_TUNER_PART, "Okänd tuner") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tunern nekade skrivning") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tunern svarade inte vid läsning") \
  X(TUNER_OUT_OF_RANGE, "Utanför området") \
  X(TUNER_UNKNOWN, "Okänd") \
  X(BAND_LONG_LW, "Långvåg") \
  X(BAND_LONG_MW, "Mellanvåg") \
  X(BAND_LONG_SW, "Kortvåg") \
  X(MENU_FMT_TUNE_TO, "Band för %s") \
  X(PTY_NONE, "Ingen") \
  X(PTY_NEWS, "Nyheter") \
  X(PTY_CURRENT_AFFAIRS, "Aktualiteter") \
  X(PTY_EDUCATION, "Utbildning") \
  X(PTY_CULTURE, "Kultur") \
  X(PTY_SCIENCE, "Vetenskap") \
  X(PTY_VARIED, "Underhållning") \
  X(PTY_POP_MUSIC, "Popmusik") \
  X(PTY_ROCK_MUSIC, "Rockmusik") \
  X(PTY_EASY_LISTENING, "Lätt musik") \
  X(PTY_LIGHT_CLASSICAL, "Lätt klassiskt") \
  X(PTY_SERIOUS_CLASSICAL, "Klassiskt") \
  X(PTY_OTHER_MUSIC, "Annan musik") \
  X(PTY_WEATHER, "Väder") \
  X(PTY_FINANCE, "Ekonomi") \
  X(PTY_CHILDREN, "Barn") \
  X(PTY_SOCIAL_AFFAIRS, "Sociala frågor") \
  X(PTY_PHONE_IN, "Ring in") \
  X(PTY_TRAVEL, "Resor") \
  X(PTY_LEISURE, "Fritid") \
  X(PTY_COUNTRY_MUSIC, "Countrymusik") \
  X(PTY_NATIONAL_MUSIC, "Nationell musik") \
  X(PTY_FOLK_MUSIC, "Folkmusik") \
  X(PTY_DOCUMENTARY, "Dokumentär") \
  X(PTY_ALARM_TEST, "Larmtest") \
  X(PTY_ALARM, "Larm") \
  X(PTY_NA_NEWS, "Nyheter") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_TALK, "Prat") \
  X(PTY_NA_CLASSIC_ROCK, "Klassisk rock") \
  X(PTY_NA_ADULT_HITS, "Vuxenhits") \
  X(PTY_NA_SOFT_ROCK, "Mjukrock") \
  X(PTY_NA_TOP_40, "Topp 40") \
  X(PTY_NA_SOFT, "Lugnt") \
  X(PTY_NA_NOSTALGIA, "Nostalgi") \
  X(PTY_NA_CLASSICAL, "Klassiskt") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Rhythm and blues") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Lugn rhythm and blues") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Främmande språk") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Religiös musik") \
  X(PTY_NA_RELIGIOUS_TALK, "Religiöst prat") \
  X(PTY_NA_PERSONALITY, "Personlighet") \
  X(PTY_NA_PUBLIC, "Public service") \
  X(PTY_NA_COLLEGE, "Högskola") \
  X(PTY_NA_SPANISH_TALK, "Spanskt prat") \
  X(PTY_NA_SPANISH_MUSIC, "Spansk musik") \
  X(PTY_NA_HIP_HOP, "Hiphop") \
  X(PTY_NA_UNASSIGNED, "Ej tilldelad") \
  X(PTY_NA_WEATHER, "Väder") \
  X(PTY_NA_EMERGENCY_TEST, "Larmtest") \
  X(PTY_NA_EMERGENCY, "Larm") \
  X(COVERAGE_LOCAL, "Lokal") \
  X(COVERAGE_INTERNATIONAL, "Internationell") \
  X(COVERAGE_NATIONAL, "Nationell") \
  X(COVERAGE_SUPRA_REGIONAL, "Överregional") \
  X(RTP_OTHER, "ANNAT") \
  X(RTP_TITLE, "TITEL") \
  X(RTP_TRACK, "SPÅR") \
  X(RTP_WORK, "VERK") \
  X(RTP_MOVEMENT, "SATS") \
  X(RTP_CONDUCTOR, "DIRIGENT") \
  X(RTP_COMPOSER, "KOMPOSITÖR") \
  X(RTP_COMMENT, "KOMMENTAR") \
  X(RTP_NEWS, "NYHETER") \
  X(RTP_LOCAL, "LOKALNYTT") \
  X(RTP_STOCKS, "AKTIER") \
  X(RTP_LOTTERY, "LOTTERI") \
  X(RTP_HOROSCOPE, "HOROSKOP") \
  X(RTP_DIVERSION, "NÖJE") \
  X(RTP_HEALTH, "HÄLSA") \
  X(RTP_EVENT, "EVENEMANG") \
  X(RTP_SCENE, "SCEN") \
  X(RTP_CINEMA, "BIO") \
  X(RTP_FUN, "KUL") \
  X(RTP_DATE, "DATUM") \
  X(RTP_WEATHER, "VÄDER") \
  X(RTP_TRAFFIC, "TRAFIK") \
  X(RTP_ALARM, "LARM") \
  X(RTP_ADVERT, "REKLAM") \
  X(RTP_NEXT, "NÄSTA") \
  X(RTP_PART, "DEL") \
  X(RTP_HOST, "VÄRD") \
  X(RTP_EDITORS, "REDAKTION") \
  X(RTP_FREQUENCY, "FREKVENS") \
  X(RTP_HOMEPAGE, "HEMSIDA") \
  X(RTP_CHANNEL, "KANAL") \
  X(RTP_PHONE, "TELEFON") \
  X(RTP_EMAIL, "E-POST") \
  X(RTP_CHAT, "CHATT") \
  X(RTP_VOTE, "RÖSTA") \
  X(RTP_PLACE, "PLATS") \
  X(RTP_APPOINTMENT, "MÖTE") \
  X(RTP_PURCHASE, "KÖP") \
  X(DAY_SUNDAY, "SÖNDAG") \
  X(DAY_MONDAY, "MÅNDAG") \
  X(DAY_TUESDAY, "TISDAG") \
  X(DAY_WEDNESDAY, "ONSDAG") \
  X(DAY_THURSDAY, "TORSDAG") \
  X(DAY_FRIDAY, "FREDAG") \
  X(DAY_SATURDAY, "LÖRDAG") \
  X(MONTH_JANUARY, "januari") \
  X(MONTH_FEBRUARY, "februari") \
  X(MONTH_MARCH, "mars") \
  X(MONTH_APRIL, "april") \
  X(MONTH_MAY, "maj") \
  X(MONTH_JUNE, "juni") \
  X(MONTH_JULY, "juli") \
  X(MONTH_AUGUST, "augusti") \
  X(MONTH_SEPTEMBER, "september") \
  X(MONTH_OCTOBER, "oktober") \
  X(MONTH_NOVEMBER, "november") \
  X(MONTH_DECEMBER, "december") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_FMT_LINE, "%s %d%s %s %d") \
  X(DATE_TODAY, "Idag") \
  X(DATE_YESTERDAY, "Igår") \
  X(DATE_YDAY, "IGÅR") \
  X(MONTH_SHORT_JAN, "jan") \
  X(MONTH_SHORT_FEB, "feb") \
  X(MONTH_SHORT_MAR, "mar") \
  X(MONTH_SHORT_APR, "apr") \
  X(MONTH_SHORT_MAY, "maj") \
  X(MONTH_SHORT_JUN, "jun") \
  X(MONTH_SHORT_JUL, "jul") \
  X(MONTH_SHORT_AUG, "aug") \
  X(MONTH_SHORT_SEP, "sep") \
  X(MONTH_SHORT_OCT, "okt") \
  X(MONTH_SHORT_NOV, "nov") \
  X(MONTH_SHORT_DEC, "dec") \
  X(MONTH_TAG_MAY, "MAJ") \
  X(MONTH_TAG_OCT, "OKT") \
  X(DX_NOW, "nu") \
  X(DX_FMT_MIN_AGO, "för %u min sedan") \
  X(DX_FMT_H_AGO, "för %u h sedan") \
  X(DX_FMT_D_AGO, "för %u d sedan") \
  X(TOUCH_CAL_TITLE, "Kalibrera touch") \
  X(TOUCH_CAL_HOLD, "Håll på cirkeln tills den fylls") \
  X(TOUCH_CAL_CANCEL_HINT, "Vrid eller tryck på ratten för att avbryta") \
  X(TOUCH_CAL_CHECK, "Kontroll") \
  X(TOUCH_CAL_TAP_DOT, "Tryck på punkten") \
  X(TOUCH_CAL_SAVED, "Kalibrering sparad") \
  X(TOUCH_CAL_FMT_LANDED, "Kontrollen missade med %u px") \
  X(TOUCH_CAL_BACK_HINT, "Tryck på ratten för att återgå") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Ej sparad") \
  X(TOUCH_CAL_FMT_MISSED, "Kontrollen missade med %u px,") \
  X(TOUCH_CAL_NO_FIT, "Markeringarna gav ingen kalibrering,") \
  X(TOUCH_CAL_NOT_SAVED, "Den kunde inte skrivas,") \
  X(TOUCH_CAL_OLD_STAYS, "så den gamla behålls") \
  X(TOUCH_CAL_RETRY_HINT, "Tryck: försök igen. Vrid: avsluta")
// clang-format on
