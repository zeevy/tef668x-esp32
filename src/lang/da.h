/*
 * The panel's texts in Danish. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_DA(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "FM-stationsscan") \
  X(MENU_SCAN_MW, "AM-stationsscan") \
  X(MENU_SCAN_SW, "SW-stationsscan") \
  X(MENU_SCAN_LW, "LW-stationsscan") \
  X(MENU_SQUELCH, "Squelch-tilstand") \
  X(MENU_SQUELCH_LEVEL, "Squelch-niveau") \
  X(MENU_SQUELCH_FLOOR, "Nedre grænse") \
  X(MENU_VOLUME_AGC, "Lydstyrke-AGC") \
  X(MENU_AGC_BOOST, "AGC-boost") \
  X(MENU_MUTE_RAMP, "Dæmperampe") \
  X(MENU_BAND_PLAN, "Båndplan") \
  X(MENU_TUNING_STEP, "Trinstørrelse") \
  X(MENU_SEEK_SENSITIVITY, "Søgefølsomhed") \
  X(MENU_FORCED_MONO, "Tving mono") \
  X(MENU_STEREO_BLEND, "Stereoblanding") \
  X(MENU_ST_HI_BLEND, "Stereo-/diskantblanding") \
  X(MENU_HIGH_CUT, "Diskantfilter") \
  X(MENU_NOISE_BLANKER, "Støjfjerner") \
  X(COMMON_RDS_DECODER, "RDS-dekoder") \
  X(MENU_RDS_REGION, "RDS-region") \
  X(MENU_MW_SPACING, "MW-trin") \
  X(MENU_FILTER_WIDTH, "Filterbredde") \
  X(MENU_SOFT_MUTE, "Blød dæmpning") \
  X(MENU_DWELL, "Ventetid") \
  X(MENU_STOP_ON, "Stop ved") \
  X(MENU_SCAN, "Scanområde") \
  X(MENU_MEMORY_FROM, "Forvalg fra") \
  X(MENU_MEMORY_TO, "Forvalg til") \
  X(MENU_PRESET_RANGE, "Forvalgsområde") \
  X(MENU_LOOP_THE_BAND, "Gentag bånd") \
  X(MENU_DX_WIDTH, "Scan-båndbredde") \
  X(MENU_MUTE_WHILE_SCANNING, "Dæmp under scan") \
  X(MENU_AUTO_LOG_NEW, "Auto-log stationer") \
  X(MENU_LOG_RADIO_TEXT, "Log radiotekst") \
  X(MENU_WATCH_PRESETS, "Overvåg forvalg") \
  X(MENU_SCOPE_BASELINE, "Scope-reference") \
  X(MENU_FMT_SWEEP_N, "Gennemløb %u") \
  X(DX_FMT_WATCH_UP, "%s op %d dB") \
  X(MENU_LEARN_LOCALS, "Lær lokale stationer") \
  X(MENU_START_SCAN, "Start scan") \
  X(MENU_DAY_THEME, "Dagtema") \
  X(MENU_NIGHT_THEME, "Nattema") \
  X(MENU_THEME, "Tema") \
  X(MENU_BRIGHTNESS, "Lysstyrke") \
  X(MENU_DIM_LEVEL, "Dimniveau") \
  X(MENU_DIM_AFTER, "Dim efter") \
  X(MENU_ROTATION, "Skærmrotation") \
  X(COMMON_BATTERY, "Batteri") \
  X(MENU_BATTERY_CHECK, "Batteritjek") \
  X(MENU_LANGUAGE, "Sprog \xC2\xB7 Language") \
  X(MENU_ENCODER, "Encodertype") \
  X(MENU_DIRECTION, "Drejeretning") \
  X(MENU_KEY_BEEPS, "Tastebip") \
  X(MENU_BAND_EDGE_BEEP, "Båndkantbip") \
  X(MENU_START_CHIME, "Startlyd") \
  X(MENU_KEYPAD_TIMEOUT, "Indtastningstid") \
  X(MENU_CALIBRATE_TOUCH, "Kalibrer touch") \
  X(MENU_CLOCK_FROM_NETWORK, "Netværkstid") \
  X(MENU_WEB_PIN, "Web-PIN") \
  X(MENU_NEW_PIN, "Ny PIN") \
  X(MENU_FMT_DIGIT_OF, "Ciffer %u af %u") \
  X(MENU_STATUS, "Forbindelsesstatus") \
  X(MENU_WEB_SERVER, "Webserver") \
  X(MENU_FMT_PC_CONNECTED, "%u forbundet") \
  X(MENU_WEB_ADDRESS, "Webadresse") \
  X(MENU_IP_ADDRESS, "IP-adresse") \
  X(MENU_WI_FI_NAME, "Wi-Fi-netværk") \
  X(MENU_HOTSPOT_NAME, "Hotspot-navn") \
  X(MENU_MAC, "MAC-adresse") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi-signal") \
  X(MENU_NETWORK_INFO, "Netværksinfo") \
  X(MENU_RESTART, "Genstart radio") \
  X(MENU_AUTO_OFF, "Auto-sluk") \
  X(MENU_UPDATE_CHECK, "Tjek opdateringer") \
  X(MENU_FIRMWARE_UPDATE, "Firmwareopdatering") \
  X(MENU_FMT_UPDATE_TO, "Opdater til %s") \
  X(MENU_UPDATE_TITLE, "Ny opdatering") \
  X(MENU_UPDATE_THIS_RADIO, "Denne radio") \
  X(MENU_UPDATE_NEW_VERSION, "Ny version") \
  X(COMMON_SETTINGS, "Indstillinger") \
  X(COMMON_KEPT, "Bevares") \
  X(MENU_UPDATE_NOW, "Opdater") \
  X(MENU_LATER, "Senere") \
  X(MENU_UPDATE_UP_TO_DATE, "Opdateret") \
  X(MENU_UPDATE_CHECKING, "Tjekker") \
  X(MENU_UPDATE_NOT_CHECKED, "Ikke tjekket") \
  X(MENU_UPDATE_CHECK_FAILED, "Tjek fejlede") \
  X(MENU_NOTE_NO_UPDATE, "Ingen opdatering") \
  X(MENU_VERSION, "Firmwareversion") \
  X(MENU_RUNNING_FROM, "Bootkilde") \
  X(MENU_UPTIME, "Oppetid") \
  X(MENU_BATTERY_VOLTAGE, "Batterispænding") \
  X(MENU_RESET_REASON, "Genstartsårsag") \
  X(MENU_CPU_CORE_0, "CPU-kerne 0") \
  X(MENU_CPU_CORE_1, "CPU-kerne 1") \
  X(MENU_FREE_HEAP, "Ledig heap") \
  X(MENU_LOWEST_HEAP, "Laveste heap") \
  X(MENU_LARGEST_BLOCK, "Største blok") \
  X(MENU_LVGL_POOL, "LVGL-pool") \
  X(MENU_FLASH_SIZE, "Flash-størrelse") \
  X(RESET_POWER, "Strøm til") \
  X(RESET_PANIC, "Panik") \
  X(RESET_INT_WATCHDOG, "IRQ-watchdog") \
  X(RESET_TASK_WATCHDOG, "Task-watchdog") \
  X(RESET_DEEP_SLEEP, "Dyb dvale") \
  X(RESET_BROWNOUT, "Lav spænding") \
  X(RESET_PIN, "Reset-pin") \
  X(RESET_UNKNOWN, "Ukendt") \
  X(RESET_ASKED, "Genstart valgt") \
  X(RESET_BOOT_WATCHDOG, "Boot-watchdog") \
  X(RESET_ROLLBACK, "Tilbagerulning") \
  X(RESET_DISPLAY, "Skærmfejl") \
  X(RESET_UPDATE, "Opdatering") \
  X(MENU_DEVELOPER, "Udvikler") \
  X(MENU_LICENSE, "Licens") \
  X(ABOUT_BUILD_UNKNOWN, "ukendt") \
  X(MENU_GO_TO, "Gå til") \
  X(MENU_GO_TO_BANDWIDTH, "Båndbredde") \
  X(MENU_GO_TO_DX, "DX-tilstand") \
  X(MENU_GO_TO_SCOPE, "Båndscope") \
  X(MENU_GO_TO_BAND, "Næste bånd") \
  X(MENU_GO_TO_LOG, "Log aktuel station") \
  X(MENU_GO_TO_SAVE, "Gem aktuel station") \
  X(MENU_FMT_SAVED_AS, "Gemt som P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Allerede P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Alle pladser optaget") \
  X(MENU_GO_TO_SLEEP, "Dvale") \
  X(MENU_STATIONS, "Stationer") \
  X(MENU_MEMORY, "Forvalg") \
  X(MENU_AUDIO, "Lyd") \
  X(MENU_FM_SETUP, "FM-modtagelse") \
  X(MENU_AM_SETUP, "AM-modtagelse") \
  X(MENU_DX_SETUP, "DX-scanner") \
  X(MENU_DISPLAY, "Skærm") \
  X(MENU_CONTROLS, "Betjening") \
  X(MENU_CONNECTIVITY, "Netværk") \
  X(MENU_DIAGNOSTICS, "Diagnostik") \
  X(MENU_ABOUT, "Om") \
  X(MENU_REGION_FULL, "Fuld") \
  X(MENU_REGION_WIDE, "Bred") \
  X(MENU_REGION_WORLD, "Hele verden") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Nordamerika") \
  X(MENU_WIFI_JOINED, "Forbundet") \
  X(MENU_WIFI_JOINING, "Forbinder") \
  X(COMMON_OFF, "Fra") \
  X(MENU_FMT_HOURS_MINUTES, "%u t %u m") \
  X(MENU_FMT_KB_OF_KB, "%u af %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V ved start") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, %u min siden") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, %u t siden") \
  X(MENU_FMT_AGC_GAIN_NOW, "Forstærkning nu %+d dB") \
  X(MENU_STATION_LOG, "Logbog") \
  X(COMMON_NONE, "Ingen") \
  X(MENU_FMT_SCAN_SAVED, "%u gemt") \
  X(MENU_FMT_SCAN_STOPPED, "Stoppet, %u gemt") \
  X(MENU_FMT_SCAN_NO_ROOM, "Fuld, %u ikke gemt") \
  X(COMMON_YES, "Ja") \
  X(COMMON_NO, "Nej") \
  X(MENU_UNKNOWN_VALUE, "Ukendt") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "På hovedet") \
  X(MENU_CUT_ONLY, "Kun sænkning") \
  X(MENU_NEVER, "Aldrig") \
  X(MENU_BATTERY_PER_CENT, "Procent") \
  X(MENU_BATTERY_VOLTS, "Volt") \
  X(MENU_KEYS, "Kun tastatur") \
  X(MENU_KEYS_LONG, "Kort og langt tryk") \
  X(MENU_EVERY_PRESS, "Hvert tryk") \
  X(MENU_OPTICAL, "Optisk") \
  X(MENU_REVERSED, "Omvendt") \
  X(MENU_SQUELCH_MAN, "Manuel") \
  X(MENU_ANY_PI, "Enhver PI") \
  X(MENU_STOP_NEVER, "Aldrig") \
  X(MENU_NEW_ONLY, "Kun nye stationer") \
  X(MENU_WHOLE_BAND, "Hele båndet") \
  X(MENU_MEMORY_ONLY, "Kun forvalg") \
  X(MENU_BAND_MEMORY, "Bånd + forvalg") \
  X(COMMON_ON, "Til") \
  X(MENU_HINT_RESTART, "Gælder efter genstart") \
  X(COMMON_FM_ONLY, "Kun FM") \
  X(MENU_NOTE_OFF_PLAN, "Ikke i båndplanen") \
  X(MENU_NOTE_SWITCH_TO_FM, "Skift til FM først") \
  X(MENU_NOTE_RADIO_BUSY, "Radio optaget - prøv igen") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Slå RDS til først") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Opdatering testes - vent") \
  X(MENU_NOTE_NOT_STORED, "Ikke gemt - gendannet") \
  X(MENU_NOTE_NOT_PUT_BACK, "Optaget - ikke gendannet") \
  X(MENU_NOTE_NOT_SAVED, "Ikke gemt") \
  X(MENU_SAVE, "Gem") \
  X(RADIO_PRESS_TO_START, "Tryk for at starte") \
  X(COMMON_STILL_TUNING, "Søger stadig - vent") \
  X(COMMON_SCAN_RUNNING, "Scan kører - vent") \
  X(RADIO_FMT_NO_BAND, "%s: intet bånd") \
  X(COMMON_NOT_LOGGED, "Ikke logget") \
  X(RADIO_FMT_LOGGED, "Logget %s") \
  X(RADIO_CHECKING_UPDATES, "TJEKKER OPDATERINGER\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Scanner %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Opdatering fejlede") \
  X(RADIO_GOING_TO_SLEEP, "Går i dvale") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Tryk på knappen for at vække") \
  X(RADIO_OLD_IMAGE_KEPT, "Tidligere firmware bevaret") \
  X(RDS_SPEECH, "Tale") \
  X(RDS_MUSIC, "Musik") \
  X(COMMON_ALREADY_LOGGED, "Allerede logget") \
  X(DX_RDS_IS_OFF, "RDS er fra") \
  X(DX_SCANNING, "Scanner") \
  X(DX_SCAN_FINISHED, "Scan færdig") \
  X(DX_NOTHING_TO_SCAN, "Intet at scanne") \
  X(DX_CHECKING_FOR_UPDATES, "Tjekker opdateringer") \
  X(DX_A_SCAN_IS_RUNNING, "Scan kører allerede") \
  X(DX_CAUGHT_LIST_UNREAD, "Fangstliste mangler") \
  X(DX_NO_MEMORY, "Ingen forvalg") \
  X(DX_NOT_STARTED, "Ikke startet") \
  X(DX_SWEEPING, "Gennemløb") \
  X(COMMON_FMT_TUNE_TYPED, "Indstil %s") \
  X(RADIO_SQL_OFF, "FRA") \
  X(DX_FMT_ROWS_OF, "%u-%u af %u") \
  X(DX_FMT_FOUND, "%u fundet") \
  X(DX_MODE_SWEEPING, "GENNEMLØB") \
  X(DX_MODE_LEARN_LOCALS, "LÆR LOKALE") \
  X(DX_FMT_MODE_MEMORY, "FORVALG %u-%u") \
  X(DX_MODE_WHOLE_BAND, "HELE BÅNDET") \
  X(DX_MODE_BAND_MEMORY, "BÅND + FORVALG") \
  X(DX_RULE_NO_STOP, "NONSTOP") \
  X(DX_RULE_STOP_ON_PI, "STOP VED PI") \
  X(DX_RULE_STOP_ON_NEW, "STOP VED NY") \
  X(DX_FMT_NOT_PRESET, "ikke P%02d") \
  X(DX_CONTEXT_SWEEPING, "gennemløb") \
  X(DX_PRESS_TO_SWEEP, "Tryk for gennemløb") \
  X(SCOPE_FMT_TITLE, "%s-scope") \
  X(SCOPE_FULL, "Fuld") \
  X(DX_FIXED, "FAST") \
  X(DX_FMT_MEDIAN_OF, "MEDIAN AF %u") \
  X(DX_FMT_FLOOR, "STØJ %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Luk") \
  X(RECOVERY_PRESS_AGAIN, "Tryk igen") \
  X(PAGE_TUNE, "Indstil") \
  X(PAGE_DELETE, "Slet") \
  X(PAGE_LEVEL, "Niveau") \
  X(PAGE_NOISE, "Støj") \
  X(PAGE_MULTIPATH, "Flervejs") \
  X(PAGE_COCHANNEL, "Samkanal") \
  X(PAGE_WIDTH, "Bredde") \
  X(PAGE_BAND, "Bånd") \
  X(PAGE_NOTE_CHANGED, "Ændret siden åbning") \
  X(PAGE_NOTE_NOT_DELETED, "Ikke slettet") \
  X(PAGE_FMT_SAVED, "Gemt %s") \
  X(PAGE_TIME_UNKNOWN, "Tid ukendt") \
  X(BW_TITLE, "Båndbredde") \
  X(KEYPAD_TITLE, "Frekvens") \
  X(KEYPAD_CANCEL, "Annuller") \
  X(DX_CATCHES_EMPTY_TITLE, "Ingen fangster endnu") \
  X(DX_CATCHES_EMPTY_NOTE, "Bekræftede stationer fra denne session vises her") \
  X(DX_TITLE_CATCHES, "Fangster") \
  X(DX_NEW, "NY") \
  X(DX_READY, "Klar") \
  X(DX_DWELL, "TID") \
  X(DX_STOPPED, "STOPPET") \
  X(DX_LEVEL, "NIV.") \
  X(DX_RISE, "DIFF") \
  X(DX_NO_STATION_YET, "Ingen station endnu") \
  X(DX_NO_RDS_YET, "Ingen RDS endnu") \
  X(DX_NO_ID, "INGEN ID") \
  X(RDS_TITLE_TEXT, "Radiotekst") \
  X(RDS_TITLE_NETWORKS, "Netværk") \
  X(RDS_TITLE_DECODER, "Dekoder") \
  X(RDS_NO_COUNTRY, "ingen ECC hørt") \
  X(RDS_RTPLUS_NONE, "Intet RT+ hørt fra denne station") \
  X(RDS_RTPLUS_WAIT, "Endnu ingen RT+ tags til teksten") \
  X(RDS_AF_TITLE, "ALTERNATIVE FREKVENSER") \
  X(RDS_EON_TITLE, "ANDRE STATIONER, EON") \
  X(RDS_NONE_HEARD, "Ingen hørt") \
  X(RDS_GROUPS_S_CAPS, "GRP/S") \
  X(RDS_LOST, "TAB") \
  X(RDS_EACH_BLOCK, "HVER BLOK") \
  X(RDS_LEGEND_CLEAN, "ren") \
  X(RDS_LEGEND_FIXED, "rettet") \
  X(RDS_LEGEND_LOST, "tabt") \
  X(RDS_ALL_CLEAN, "alle rene") \
  X(RDS_GROUP_TYPES, "GRUPPETYPER") \
  X(RDS_NO_GROUPS, "Ingen grupper i det sidste minut") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 i %u s") \
  X(RDS_COUNTRY_UNLISTED, "ikke opført") \
  X(RDS_NO_TYPES, "Ingen blok B læst rent i det sidste minut") \
  X(RDS_FMT_SYNC_H_M, "%ut%02um") \
  X(RDS_FMT_EON_MORE, "+%u flere") \
  X(RDS_EON_MORE_HEARD, "+ flere") \
  X(RDS_MORE, "flere") \
  X(RADIO_VEIL_UPDATING, "OPDATERER FIRMWARE") \
  X(RADIO_VEIL_KEEP_POWER, "Sluk ikke for strømmen") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Ingen tuner på I2C-bussen") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tuner blev ikke klar") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Tuner ikke identificeret") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Ingen firmware-patch til denne tuner") \
  X(TUNER_UNKNOWN_TUNER_PART, "Ukendt tuner") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tuner afviste skrivning") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tuner svarede ikke på læsning") \
  X(TUNER_OUT_OF_RANGE, "Uden for område") \
  X(TUNER_UNKNOWN, "Ukendt") \
  X(BAND_LONG_LW, "Langbølge") \
  X(BAND_LONG_MW, "Mellembølge") \
  X(BAND_LONG_SW, "Kortbølge") \
  X(MENU_FMT_TUNE_TO, "Indstil %s på") \
  X(PTY_NONE, "Ingen") \
  X(PTY_NEWS, "Nyheder") \
  X(PTY_CURRENT_AFFAIRS, "Aktualitet") \
  X(PTY_EDUCATION, "Uddannelse") \
  X(PTY_CULTURE, "Kultur") \
  X(PTY_SCIENCE, "Videnskab") \
  X(PTY_VARIED, "Blandet") \
  X(PTY_POP_MUSIC, "Popmusik") \
  X(PTY_ROCK_MUSIC, "Rockmusik") \
  X(PTY_EASY_LISTENING, "Let musik") \
  X(PTY_LIGHT_CLASSICAL, "Let klassisk") \
  X(PTY_SERIOUS_CLASSICAL, "Klassisk") \
  X(PTY_OTHER_MUSIC, "Anden musik") \
  X(PTY_WEATHER, "Vejr") \
  X(PTY_FINANCE, "Økonomi") \
  X(PTY_CHILDREN, "Børn") \
  X(PTY_SOCIAL_AFFAIRS, "Samfund") \
  X(PTY_PHONE_IN, "Ring ind") \
  X(PTY_TRAVEL, "Rejser") \
  X(PTY_LEISURE, "Fritid") \
  X(PTY_COUNTRY_MUSIC, "Countrymusik") \
  X(PTY_NATIONAL_MUSIC, "National musik") \
  X(PTY_OLDIES_MUSIC, "Gamle hits") \
  X(PTY_FOLK_MUSIC, "Folkemusik") \
  X(PTY_DOCUMENTARY, "Dokumentar") \
  X(PTY_ALARM_TEST, "Alarmtest") \
  X(PTY_NA_NEWS, "Nyheder") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_NOSTALGIA, "Nostalgi") \
  X(PTY_NA_CLASSICAL, "Klassisk") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Rhythm & Blues") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Soft Rhythm & Blues") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Fremmedsprog") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Religiøs musik") \
  X(PTY_NA_RELIGIOUS_TALK, "Religiøs tale") \
  X(PTY_NA_PERSONALITY, "Personlighed") \
  X(PTY_NA_PUBLIC, "Offentlig") \
  X(PTY_NA_SPANISH_TALK, "Spansk tale") \
  X(PTY_NA_SPANISH_MUSIC, "Spansk musik") \
  X(PTY_NA_UNASSIGNED, "Ikke tildelt") \
  X(PTY_NA_WEATHER, "Vejr") \
  X(PTY_NA_EMERGENCY_TEST, "Nødtest") \
  X(PTY_NA_EMERGENCY, "Nødsituation") \
  X(COVERAGE_LOCAL, "Lokal") \
  X(COVERAGE_SUPRA_REGIONAL, "Overregional") \
  X(RTP_OTHER, "ANDET") \
  X(RTP_TITLE, "TITEL") \
  X(RTP_TRACK, "NUMMER") \
  X(RTP_ARTIST, "KUNSTNER") \
  X(RTP_WORK, "VÆRK") \
  X(RTP_MOVEMENT, "SATS") \
  X(RTP_CONDUCTOR, "DIRIGENT") \
  X(RTP_COMPOSER, "KOMPONIST") \
  X(RTP_COMMENT, "KOMMENTAR") \
  X(RTP_NEWS, "NYHEDER") \
  X(RTP_LOCAL, "LOKALNYT") \
  X(RTP_STOCKS, "AKTIER") \
  X(RTP_LOTTERY, "LOTTO") \
  X(RTP_HOROSCOPE, "HOROSKOP") \
  X(RTP_DIVERSION, "ADSPREDELSE") \
  X(RTP_HEALTH, "SUNDHED") \
  X(RTP_EVENT, "BEGIVENHED") \
  X(RTP_CINEMA, "BIOGRAF") \
  X(RTP_FUN, "SJOV") \
  X(RTP_DATE, "DATO") \
  X(RTP_WEATHER, "VEJR") \
  X(RTP_TRAFFIC, "TRAFIK") \
  X(RTP_ADVERT, "REKLAME") \
  X(RTP_NEXT, "NÆSTE") \
  X(RTP_PART, "DEL") \
  X(RTP_HOST, "VÆRT") \
  X(RTP_EDITORS, "REDAKTION") \
  X(RTP_FREQUENCY, "FREKVENS") \
  X(RTP_HOMEPAGE, "HJEMMESIDE") \
  X(RTP_CHANNEL, "KANAL") \
  X(RTP_PHONE, "TELEFON") \
  X(RTP_EMAIL, "E-MAIL") \
  X(RTP_VOTE, "AFSTEMNING") \
  X(RTP_PLACE, "STED") \
  X(RTP_APPOINTMENT, "AFTALE") \
  X(RTP_PURCHASE, "KØB") \
  X(DAY_SUNDAY, "SØNDAG") \
  X(DAY_MONDAY, "MANDAG") \
  X(DAY_TUESDAY, "TIRSDAG") \
  X(DAY_WEDNESDAY, "ONSDAG") \
  X(DAY_THURSDAY, "TORSDAG") \
  X(DAY_FRIDAY, "FREDAG") \
  X(DAY_SATURDAY, "LØRDAG") \
  X(MONTH_JANUARY, "januar") \
  X(MONTH_FEBRUARY, "februar") \
  X(MONTH_MARCH, "marts") \
  X(MONTH_APRIL, "april") \
  X(MONTH_MAY, "maj") \
  X(MONTH_JUNE, "juni") \
  X(MONTH_JULY, "juli") \
  X(MONTH_AUGUST, "august") \
  X(MONTH_SEPTEMBER, "september") \
  X(MONTH_OCTOBER, "oktober") \
  X(MONTH_NOVEMBER, "november") \
  X(MONTH_DECEMBER, "december") \
  X(DATE_SUFFIX_TH, ".") \
  X(DATE_SUFFIX_ST, ".") \
  X(DATE_SUFFIX_ND, ".") \
  X(DATE_SUFFIX_RD, ".") \
  X(DATE_TODAY, "I dag") \
  X(DATE_YESTERDAY, "I går") \
  X(DATE_YDAY, "I GÅR") \
  X(DATE_FMT_WHEN_DATE, "%d. %s %s") \
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
  X(DX_FMT_MIN_AGO, "%u min siden") \
  X(DX_FMT_H_AGO, "%u t siden") \
  X(DX_FMT_D_AGO, "%u d siden") \
  X(TOUCH_CAL_TITLE, "Kalibrer touch") \
  X(TOUCH_CAL_HOLD, "Hold på cirklen, til den er fyldt") \
  X(TOUCH_CAL_CANCEL_HINT, "Drej eller tryk på knappen: afbryd") \
  X(TOUCH_CAL_CHECK, "Kontrol") \
  X(TOUCH_CAL_TAP_DOT, "Tryk på prikken") \
  X(TOUCH_CAL_SAVED, "Kalibrering gemt") \
  X(TOUCH_CAL_FMT_LANDED, "Kontrollen afveg %u px") \
  X(TOUCH_CAL_BACK_HINT, "Tryk på knappen: tilbage") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Ikke gemt") \
  X(TOUCH_CAL_FMT_MISSED, "Kontrollen afveg %u px,") \
  X(TOUCH_CAL_NO_FIT, "Punkterne gav ingen kalibrering,") \
  X(TOUCH_CAL_NOT_SAVED, "Den kunne ikke gemmes,") \
  X(TOUCH_CAL_OLD_STAYS, "så den gamle kalibrering bevares") \
  X(TOUCH_CAL_RETRY_HINT, "Tryk på knappen: prøv igen, drej: afslut")
// clang-format on
