/*
 * The panel's texts in Hungarian. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_HU(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "FM szkennelés") \
  X(MENU_SCAN_MW, "AM szkennelés") \
  X(MENU_SCAN_SW, "SW szkennelés") \
  X(MENU_SCAN_LW, "LW szkennelés") \
  X(MENU_SQUELCH, "Zajzár mód") \
  X(MENU_SQUELCH_LEVEL, "Zajzár szint") \
  X(MENU_SQUELCH_FLOOR, "Zajzár minimum") \
  X(MENU_SQUELCH_GROUP, "Zajzár") \
  X(MENU_VOLUME_AGC, "Hangerő AGC") \
  X(MENU_AGC_BOOST, "AGC emelés") \
  X(MENU_MUTE_RAMP, "Némítási rámpa") \
  X(MENU_BAND_PLAN, "Sávterv") \
  X(MENU_TUNING_STEP, "Lépésköz") \
  X(MENU_SEEK_SENSITIVITY, "Keresési érzékenység") \
  X(MENU_DE_EMPHASIS, "Deemfázis") \
  X(MENU_FORCED_MONO, "Csak monó") \
  X(MENU_STEREO_BLEND, "Sztereó keverés") \
  X(MENU_ST_HI_BLEND, "Sztereó / magas keverés") \
  X(MENU_STEREO, "Sztereó") \
  X(MENU_HIGH_CUT, "Magasvágás") \
  X(MENU_NOISE_BLANKER, "Zajkioltó") \
  X(COMMON_RDS_DECODER, "RDS dekóder") \
  X(MENU_RDS_REGION, "RDS régió") \
  X(MENU_MW_SPACING, "MW lépésköz") \
  X(MENU_FILTER_WIDTH, "Szűrő sávszélesség") \
  X(MENU_SOFT_MUTE, "Lágy némítás") \
  X(MENU_DWELL, "Várakozási idő") \
  X(MENU_STOP_ON, "Megállás") \
  X(MENU_SCAN, "Tartomány") \
  X(MENU_MEMORY_FROM, "Első memória") \
  X(MENU_MEMORY_TO, "Utolsó memória") \
  X(MENU_PRESET_RANGE, "Memóriatartomány") \
  X(MENU_LOOP_THE_BAND, "Sáv ismétlése") \
  X(MENU_DX_WIDTH, "Sávszélesség") \
  X(MENU_MUTE_WHILE_SCANNING, "Némítás szkenneléskor") \
  X(MENU_AUTO_LOG_NEW, "Auto naplózás") \
  X(MENU_LOG_RADIO_TEXT, "Rádiószöveg naplózása") \
  X(MENU_WATCH_PRESETS, "Memóriák figyelése") \
  X(MENU_SCOPE_BASELINE, "Spektrum alapvonal") \
  X(MENU_MEDIAN, "Medián") \
  X(MENU_FMT_SWEEP_N, "Söprés %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Helyi adók tanulása") \
  X(MENU_START_SCAN, "Szkennelés indítása") \
  X(MENU_DAY_THEME, "Nappali téma") \
  X(MENU_NIGHT_THEME, "Éjszakai téma") \
  X(MENU_THEME, "Téma") \
  X(MENU_BRIGHTNESS, "Fényerő") \
  X(MENU_DIM_LEVEL, "Tompított fényerő") \
  X(MENU_DIM_AFTER, "Tompítás késleltetése") \
  X(MENU_ROTATION, "Kijelző forgatása") \
  X(COMMON_BATTERY, "Akku") \
  X(MENU_BATTERY_CHECK, "Akku ellenőrzés") \
  X(MENU_LANGUAGE, "Nyelv \xC2\xB7 Language") \
  X(MENU_ENCODER, "Kódoló típusa") \
  X(MENU_DIRECTION, "Kódoló iránya") \
  X(MENU_ENCODER_GROUP, "Kódoló") \
  X(MENU_KEY_BEEPS, "Gombhang") \
  X(MENU_BAND_EDGE_BEEP, "Sávhatár hang") \
  X(MENU_START_CHIME, "Indítási hang") \
  X(MENU_KEYPAD_TIMEOUT, "Billentyűzet időkorlát") \
  X(MENU_TOUCH, "Érintés") \
  X(MENU_CALIBRATE_TOUCH, "Érintés kalibrálása") \
  X(MENU_CLOCK_FROM_NETWORK, "Hálózati idő") \
  X(MENU_NEW_PIN, "Új PIN") \
  X(MENU_FMT_DIGIT_OF, "Számjegy %u/%u") \
  X(MENU_STATUS, "Kapcsolat állapota") \
  X(MENU_WEB_SERVER, "Webszerver") \
  X(MENU_FMT_PC_CONNECTED, "%u csatlakozva") \
  X(MENU_WEB_ADDRESS, "Webcím") \
  X(MENU_IP_ADDRESS, "IP-cím") \
  X(MENU_WI_FI_NAME, "Wi-Fi hálózat") \
  X(MENU_HOTSPOT_NAME, "Hotspot neve") \
  X(MENU_MAC, "MAC-cím") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi jel") \
  X(MENU_NETWORK_INFO, "Hálózati infó") \
  X(MENU_RESTART, "Újraindítás") \
  X(MENU_AUTO_OFF, "Auto kikapcsolás") \
  X(MENU_UPDATE_CHECK, "Frissítés keresése") \
  X(MENU_FIRMWARE_UPDATE, "Firmware-frissítés") \
  X(MENU_FMT_UPDATE_TO, "Frissítés: %s") \
  X(MENU_UPDATE_TITLE, "Frissítés elérhető") \
  X(MENU_UPDATE_THIS_RADIO, "Ez a rádió") \
  X(MENU_UPDATE_NEW_VERSION, "Új verzió") \
  X(MENU_UPDATE_DOWNLOAD, "Letöltés") \
  X(COMMON_SETTINGS, "Beállítások") \
  X(COMMON_KEPT, "Megmarad") \
  X(MENU_STATION, "Adó") \
  X(MENU_UPDATE_NOW, "Frissítés") \
  X(MENU_LATER, "Később") \
  X(MENU_UPDATE_UP_TO_DATE, "Naprakész") \
  X(MENU_UPDATE_CHECKING, "Ellenőrzés") \
  X(MENU_UPDATE_NOT_CHECKED, "Nem ellenőrzött") \
  X(MENU_UPDATE_CHECK_FAILED, "Ellenőrzési hiba") \
  X(MENU_NOTE_NO_UPDATE, "Nincs frissítés") \
  X(MENU_VERSION, "Firmware verzió") \
  X(MENU_RUNNING_FROM, "Indítási forrás") \
  X(MENU_UPTIME, "Üzemidő") \
  X(MENU_BATTERY_VOLTAGE, "Akkufeszültség") \
  X(MENU_RESET_REASON, "Újraindítás oka") \
  X(MENU_CPU_CORE_0, "CPU mag 0") \
  X(MENU_CPU_CORE_1, "CPU mag 1") \
  X(MENU_FREE_HEAP, "Szabad heap") \
  X(MENU_LOWEST_HEAP, "Legkisebb heap") \
  X(MENU_LARGEST_BLOCK, "Legnagyobb blokk") \
  X(MENU_FLASH_SIZE, "Flash méret") \
  X(RESET_POWER, "Bekapcsolás") \
  X(RESET_SOFTWARE, "Szoftver") \
  X(RESET_PANIC, "Összeomlás") \
  X(RESET_DEEP_SLEEP, "Mély alvás") \
  X(RESET_BROWNOUT, "Feszültségesés") \
  X(RESET_PIN, "Reset láb") \
  X(RESET_UNKNOWN, "Ismeretlen") \
  X(RESET_ASKED, "Kért újraindítás") \
  X(RESET_BOOT_WATCHDOG, "Indítási watchdog") \
  X(RESET_ROLLBACK, "Visszaállítás") \
  X(RESET_DISPLAY, "Kijelzőhiba") \
  X(RESET_UPDATE, "Frissítés") \
  X(MENU_DEVELOPER, "Fejlesztő") \
  X(MENU_LICENSE, "Licenc") \
  X(ABOUT_BUILD_UNKNOWN, "ismeretlen") \
  X(MENU_GO_TO, "Ugrás") \
  X(MENU_GO_TO_BANDWIDTH, "Sávszélesség") \
  X(MENU_GO_TO_DX, "DX mód") \
  X(MENU_GO_TO_SCOPE, "Sávspektrum") \
  X(MENU_GO_TO_BAND, "Következő sáv") \
  X(MENU_GO_TO_LOG, "Adó naplózása") \
  X(MENU_GO_TO_SAVE, "Adó mentése") \
  X(MENU_FMT_SAVED_AS, "Mentve: P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Már mentve: P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Minden hely foglalt") \
  X(MENU_GO_TO_SLEEP, "Alvás") \
  X(MENU_STATIONS, "Adók") \
  X(MENU_MEMORY, "Memóriák") \
  X(MENU_AUDIO, "Hang") \
  X(MENU_FM_SETUP, "FM vétel") \
  X(MENU_AM_SETUP, "AM vétel") \
  X(MENU_DX_SETUP, "DX szkenner") \
  X(MENU_DISPLAY, "Kijelző") \
  X(MENU_CONTROLS, "Kezelés") \
  X(MENU_CONNECTIVITY, "Kapcsolatok") \
  X(MENU_SYSTEM, "Rendszer") \
  X(MENU_DIAGNOSTICS, "Diagnosztika") \
  X(MENU_ABOUT, "Névjegy") \
  X(MENU_REGION_FULL, "Teljes") \
  X(MENU_REGION_JAPAN, "Japán") \
  X(MENU_REGION_WIDE, "Széles") \
  X(MENU_REGION_WORLD, "Világ") \
  X(MENU_RDS_REGION_EUROPE, "Európa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Észak-Amerika") \
  X(MENU_WIFI_JOINED, "Csatlakozva") \
  X(MENU_WIFI_JOINING, "Csatlakozás") \
  X(COMMON_OFF, "Ki") \
  X(MENU_FMT_MINUTES, "%u perc") \
  X(MENU_FMT_HOURS_MINUTES, "%u ó %u p") \
  X(MENU_FMT_KB_OF_KB, "%u / %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V induláskor") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, %u perce") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, %u órája") \
  X(MENU_FMT_AGC_GAIN_NOW, "Erősítés most %+d dB") \
  X(MENU_STATION_LOG, "Adónapló") \
  X(COMMON_NONE, "Nincs") \
  X(MENU_FMT_SCAN_PROGRESS, "Szkennel %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Mentve: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Leállt, %u mentve") \
  X(MENU_FMT_SCAN_NO_ROOM, "Tele, %u kimaradt") \
  X(COMMON_YES, "Igen") \
  X(COMMON_NO, "Nem") \
  X(MENU_UNKNOWN_VALUE, "Ismeretlen") \
  X(COMMON_ROTATION_NORMAL, "Normál") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Fejjel lefelé") \
  X(MENU_CUT_ONLY, "Csak vágás") \
  X(MENU_NEVER, "Soha") \
  X(MENU_BATTERY_PER_CENT, "Százalék") \
  X(MENU_BATTERY_VOLTS, "Volt") \
  X(MENU_KEYS, "Csak billentyűzet") \
  X(MENU_KEYS_LONG, "Rövid és hosszú") \
  X(MENU_EVERY_PRESS, "Minden gomb") \
  X(MENU_STANDARD, "Szabványos") \
  X(MENU_OPTICAL, "Optikai") \
  X(MENU_NORMAL, "Normál") \
  X(MENU_REVERSED, "Fordított") \
  X(MENU_SQUELCH_MAN, "Kézi") \
  X(MENU_ANY_PI, "Bármely PI") \
  X(MENU_STOP_NEVER, "Soha") \
  X(MENU_NEW_ONLY, "Csak új adók") \
  X(MENU_WHOLE_BAND, "Teljes sáv") \
  X(MENU_MEMORY_ONLY, "Csak memóriák") \
  X(MENU_BAND_MEMORY, "Sáv + memóriák") \
  X(COMMON_ON, "Be") \
  X(MENU_HINT_RESTART, "Újraindítás után érvényes") \
  X(MENU_TITLE, "Menü") \
  X(COMMON_FM_ONLY, "Csak FM") \
  X(MENU_NOTE_OFF_PLAN, "Nincs a sávtervben") \
  X(MENU_NOTE_SWITCH_TO_FM, "Előbb válts FM-re") \
  X(MENU_NOTE_RADIO_BUSY, "Foglalt - próbáld újra") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Kapcsold be az RDS-t") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Frissítés próbaüzemben - várj") \
  X(MENU_NOTE_NOT_STORED, "Nincs mentve - visszaállítva") \
  X(MENU_NOTE_NOT_PUT_BACK, "Foglalt - nincs visszaállítva") \
  X(MENU_NOTE_NOT_SAVED, "Nincs mentve") \
  X(MENU_SAVE, "Mentés") \
  X(RADIO_PRESS_TO_START, "Indításhoz nyomd meg") \
  X(COMMON_STILL_TUNING, "Még hangol - várj") \
  X(COMMON_SCAN_RUNNING, "Szkennelés fut - várj") \
  X(RADIO_FMT_NO_BAND, "%s nincs sávban") \
  X(COMMON_NOT_LOGGED, "Nincs naplózva") \
  X(RADIO_FMT_LOGGED, "Naplózva: %s") \
  X(RADIO_CHECKING_UPDATES, "FRISSÍTÉS KERESÉSE\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Szkennelés %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Sikertelen frissítés") \
  X(RADIO_GOING_TO_SLEEP, "Alvó mód") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Ébresztés: nyomd meg a forgógombot") \
  X(RADIO_OLD_IMAGE_KEPT, "Az előző firmware maradt") \
  X(RDS_STEREO, "Sztereó") \
  X(RDS_MONO, "Monó") \
  X(RDS_SPEECH, "Beszéd") \
  X(RDS_MUSIC, "Zene") \
  X(COMMON_ALREADY_LOGGED, "Már naplózva") \
  X(DX_RDS_IS_OFF, "RDS kikapcsolva") \
  X(DX_SCANNING, "Szkennelés") \
  X(DX_SCAN_FINISHED, "Szkennelés kész") \
  X(DX_NOTHING_TO_SCAN, "Nincs mit szkennelni") \
  X(DX_CHECKING_FOR_UPDATES, "Frissítés keresése") \
  X(DX_A_SCAN_IS_RUNNING, "Szkennelés már fut") \
  X(DX_CAUGHT_LIST_UNREAD, "Fogáslista nem elérhető") \
  X(DX_NO_MEMORY, "Memória üres") \
  X(DX_NOT_STARTED, "Nincs elindítva") \
  X(DX_SWEEPING, "Söprés") \
  X(COMMON_FMT_TUNE_TYPED, "Ugrás %s") \
  X(RADIO_SQL_OFF, "KI") \
  X(RADIO_TUNE_MANUAL, "KÉZI") \
  X(DX_FMT_ROWS_OF, "%u-%u / %u") \
  X(DX_FMT_FOUND, "%u találat") \
  X(DX_MODE_SWEEPING, "SÖPRÉS") \
  X(DX_MODE_LEARN_LOCALS, "HELYI ADÓK") \
  X(DX_FMT_MODE_MEMORY, "MEMÓRIA %u-%u") \
  X(DX_MODE_WHOLE_BAND, "TELJES SÁV") \
  X(DX_MODE_BAND_MEMORY, "SÁV + MEMÓRIA") \
  X(DX_RULE_NO_STOP, "NE ÁLLJ MEG") \
  X(DX_RULE_STOP_ON_PI, "ÁLLJ PI-NÉL") \
  X(DX_RULE_STOP_ON_NEW, "ÁLLJ ÚJNÁL") \
  X(DX_FMT_NOT_PRESET, "nem P%02d") \
  X(DX_CONTEXT_SWEEPING, "söprés") \
  X(DX_PRESS_TO_SWEEP, "Nyomd meg a söpréshez") \
  X(SCOPE_FMT_TITLE, "%s spektrum") \
  X(SCOPE_FULL, "Teljes") \
  X(DX_FIXED, "FIX") \
  X(DX_FMT_MEDIAN_OF, "MEDIÁN (%u)") \
  X(DX_FMT_FLOOR, "ZAJ %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Bezár") \
  X(RECOVERY_PRESS_AGAIN, "Nyomd újra") \
  X(PAGE_TUNE, "Hangolás") \
  X(PAGE_DELETE, "Törlés") \
  X(PAGE_LEVEL, "Szint") \
  X(PAGE_NOISE, "Zaj") \
  X(PAGE_MULTIPATH, "Többutas") \
  X(PAGE_COCHANNEL, "Zavaró adó") \
  X(PAGE_STEREO, "Sztereó") \
  X(PAGE_WIDTH, "Szűrő") \
  X(PAGE_BAND, "Sáv") \
  X(PAGE_NOTE_CHANGED, "Megnyitás óta megváltozott") \
  X(PAGE_NOTE_NOT_DELETED, "Nincs törölve") \
  X(PAGE_FMT_SAVED, "Mentve: %s") \
  X(PAGE_TIME_UNKNOWN, "Ismeretlen idő") \
  X(BW_TITLE, "Sávszélesség") \
  X(KEYPAD_TITLE, "Frekvencia") \
  X(KEYPAD_CANCEL, "Mégse") \
  X(DX_CATCHES_EMPTY_TITLE, "Még nincs fogás") \
  X(DX_CATCHES_EMPTY_NOTE, "Az e menetben megerősített adók itt jelennek meg") \
  X(DX_TITLE_CATCHES, "Fogások") \
  X(DX_NEW, "ÚJ") \
  X(DX_READY, "Kész") \
  X(DX_DWELL, "IDŐ") \
  X(DX_STOPPED, "LEÁLLÍTVA") \
  X(DX_TITLE_SCANNER, "Szkenner") \
  X(DX_TITLE_SCOPE, "Spektrum") \
  X(DX_LEVEL, "SZINT") \
  X(DX_RISE, "DIFF") \
  X(DX_NO_STATION_YET, "Még nincs adó") \
  X(DX_NO_RDS_YET, "Még nincs RDS") \
  X(DX_NO_ID, "NINCS ID") \
  X(RDS_TITLE_TEXT, "Rádiószöveg") \
  X(RDS_TITLE_NETWORKS, "Hálózatok") \
  X(RDS_TITLE_DECODER, "Dekóder") \
  X(RDS_NO_COUNTRY, "nincs ECC") \
  X(RDS_MS_UNKNOWN, "Z/B") \
  X(RDS_RTPLUS_NONE, "Ettől az adótól nem jött RT+") \
  X(RDS_RTPLUS_WAIT, "Még nincs RT+ címke e szöveghez") \
  X(RDS_AF_TITLE, "ALTERNATÍV FREKVENCIÁK") \
  X(RDS_EON_TITLE, "MÁS ADÓK, EON") \
  X(RDS_NONE_HEARD, "Nem érkezett") \
  X(RDS_SYNC_CAPS, "SZINKR.") \
  X(RDS_GROUPS_S_CAPS, "CSOP./S") \
  X(RDS_LOST, "HIBÁS") \
  X(RDS_EACH_BLOCK, "BLOKKONKÉNT") \
  X(RDS_LEGEND_CLEAN, "tiszta") \
  X(RDS_LEGEND_FIXED, "javított") \
  X(RDS_LEGEND_LOST, "hibás") \
  X(RDS_ALL_CLEAN, "mind tiszta") \
  X(RDS_GROUP_TYPES, "CSOPORTTÍPUSOK") \
  X(RDS_NO_GROUPS, "Az utolsó percben nem jött csoport") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 %u s") \
  X(RDS_COUNTRY_UNLISTED, "nincs listán") \
  X(RDS_NO_TYPES, "Az utolsó percben nem jött tiszta B blokk") \
  X(RDS_FMT_EON_MORE, "+%u további") \
  X(RDS_EON_MORE_HEARD, "+ további") \
  X(RDS_MORE, "további") \
  X(RADIO_VEIL_UPDATING, "FIRMWARE FRISSÍTÉSE") \
  X(RADIO_VEIL_KEEP_POWER, "Ne kapcsold ki!") \
  X(RADIO_MUTE, "NÉMA") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Nincs tuner az I2C buszon") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "A tuner nem lett üzemkész") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Tuner azonosítás sikertelen") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Nincs firmware patch ehhez a tunerhez") \
  X(TUNER_UNKNOWN_TUNER_PART, "Ismeretlen tuner") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "A tuner elutasította az írást") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "A tuner nem válaszolt olvasásra") \
  X(TUNER_OUT_OF_RANGE, "Tartományon kívül") \
  X(TUNER_UNKNOWN, "Ismeretlen") \
  X(BAND_LONG_LW, "Hosszúhullám") \
  X(BAND_LONG_MW, "Középhullám") \
  X(BAND_LONG_SW, "Rövidhullám") \
  X(MENU_FMT_TUNE_TO, "%s melyik sávon?") \
  X(PTY_NONE, "Nincs") \
  X(PTY_NEWS, "Hírek") \
  X(PTY_CURRENT_AFFAIRS, "Aktuális") \
  X(PTY_INFORMATION, "Információ") \
  X(PTY_EDUCATION, "Oktatás") \
  X(PTY_DRAMA, "Dráma") \
  X(PTY_CULTURE, "Kultúra") \
  X(PTY_SCIENCE, "Tudomány") \
  X(PTY_VARIED, "Változatos") \
  X(PTY_POP_MUSIC, "Popzene") \
  X(PTY_ROCK_MUSIC, "Rockzene") \
  X(PTY_EASY_LISTENING, "Könnyűzene") \
  X(PTY_LIGHT_CLASSICAL, "Könnyű klasszikus") \
  X(PTY_SERIOUS_CLASSICAL, "Komolyzene") \
  X(PTY_OTHER_MUSIC, "Egyéb zene") \
  X(PTY_WEATHER, "Időjárás") \
  X(PTY_FINANCE, "Pénzügy") \
  X(PTY_CHILDREN, "Gyermekműsor") \
  X(PTY_SOCIAL_AFFAIRS, "Társadalmi ügyek") \
  X(PTY_RELIGION, "Vallás") \
  X(PTY_PHONE_IN, "Betelefonálós") \
  X(PTY_TRAVEL, "Utazás") \
  X(PTY_LEISURE, "Szabadidő") \
  X(PTY_COUNTRY_MUSIC, "Country zene") \
  X(PTY_NATIONAL_MUSIC, "Nemzeti zene") \
  X(PTY_OLDIES_MUSIC, "Régi slágerek") \
  X(PTY_FOLK_MUSIC, "Népzene") \
  X(PTY_DOCUMENTARY, "Dokumentum") \
  X(PTY_ALARM_TEST, "Riasztásteszt") \
  X(PTY_ALARM, "Riasztás") \
  X(PTY_NA_NEWS, "Hírek") \
  X(PTY_NA_INFORMATION, "Információ") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_TALK, "Beszélgetés") \
  X(PTY_NA_CLASSIC_ROCK, "Klasszikus rock") \
  X(PTY_NA_SOFT_ROCK, "Soft rock") \
  X(PTY_NA_OLDIES, "Régi slágerek") \
  X(PTY_NA_NOSTALGIA, "Nosztalgia") \
  X(PTY_NA_CLASSICAL, "Komolyzene") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Rhythm and blues") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Soft rhythm and blues") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Idegen nyelvű") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Vallási zene") \
  X(PTY_NA_RELIGIOUS_TALK, "Vallási beszélgetés") \
  X(PTY_NA_PERSONALITY, "Személyiség") \
  X(PTY_NA_PUBLIC, "Közszolgálati") \
  X(PTY_NA_COLLEGE, "Egyetemi") \
  X(PTY_NA_SPANISH_TALK, "Spanyol beszélgetés") \
  X(PTY_NA_SPANISH_MUSIC, "Spanyol zene") \
  X(PTY_NA_HIP_HOP, "Hip-hop") \
  X(PTY_NA_UNASSIGNED, "Nincs kiosztva") \
  X(PTY_NA_WEATHER, "Időjárás") \
  X(PTY_NA_EMERGENCY_TEST, "Vészhelyzet teszt") \
  X(PTY_NA_EMERGENCY, "Vészhelyzet") \
  X(COVERAGE_LOCAL, "Helyi") \
  X(COVERAGE_INTERNATIONAL, "Nemzetközi") \
  X(COVERAGE_NATIONAL, "Országos") \
  X(COVERAGE_SUPRA_REGIONAL, "Országrészi") \
  X(COVERAGE_REGIONAL_1, "Régió 1") \
  X(COVERAGE_REGIONAL_2, "Régió 2") \
  X(COVERAGE_REGIONAL_3, "Régió 3") \
  X(COVERAGE_REGIONAL_4, "Régió 4") \
  X(COVERAGE_REGIONAL_5, "Régió 5") \
  X(COVERAGE_REGIONAL_6, "Régió 6") \
  X(COVERAGE_REGIONAL_7, "Régió 7") \
  X(COVERAGE_REGIONAL_8, "Régió 8") \
  X(COVERAGE_REGIONAL_9, "Régió 9") \
  X(COVERAGE_REGIONAL_10, "Régió 10") \
  X(COVERAGE_REGIONAL_11, "Régió 11") \
  X(COVERAGE_REGIONAL_12, "Régió 12") \
  X(RTP_OTHER, "EGYÉB") \
  X(RTP_TITLE, "CÍM") \
  X(RTP_TRACK, "SZÁM") \
  X(RTP_ARTIST, "ELŐADÓ") \
  X(RTP_WORK, "MŰ") \
  X(RTP_MOVEMENT, "TÉTEL") \
  X(RTP_CONDUCTOR, "KARMESTER") \
  X(RTP_COMPOSER, "ZENESZERZŐ") \
  X(RTP_BAND, "ZENEKAR") \
  X(RTP_COMMENT, "MEGJEGYZÉS") \
  X(RTP_GENRE, "MŰFAJ") \
  X(RTP_NEWS, "HÍREK") \
  X(RTP_LOCAL, "HELYI HÍREK") \
  X(RTP_STOCKS, "TŐZSDE") \
  X(RTP_LOTTERY, "LOTTÓ") \
  X(RTP_HOROSCOPE, "HOROSZKÓP") \
  X(RTP_DIVERSION, "SZABADIDŐ") \
  X(RTP_HEALTH, "EGÉSZSÉG") \
  X(RTP_EVENT, "ESEMÉNY") \
  X(RTP_SCENE, "SZCÉNA") \
  X(RTP_CINEMA, "MOZI") \
  X(RTP_FUN, "HUMOR") \
  X(RTP_DATE, "DÁTUM") \
  X(RTP_WEATHER, "IDŐJÁRÁS") \
  X(RTP_TRAFFIC, "KÖZLEKEDÉS") \
  X(RTP_ALARM, "RIASZTÁS") \
  X(RTP_ADVERT, "REKLÁM") \
  X(RTP_STATION, "ADÓ") \
  X(RTP_PROGRAM, "MŰSOR") \
  X(RTP_NEXT, "KÖVETKEZŐ") \
  X(RTP_PART, "RÉSZ") \
  X(RTP_HOST, "HÁZIGAZDA") \
  X(RTP_EDITORS, "SZERKESZTŐK") \
  X(RTP_FREQUENCY, "FREKVENCIA") \
  X(RTP_HOMEPAGE, "HONLAP") \
  X(RTP_CHANNEL, "CSATORNA") \
  X(RTP_HOTLINE, "FORRÓDRÓT") \
  X(RTP_STUDIO, "STÚDIÓ") \
  X(RTP_PHONE, "TELEFON") \
  X(RTP_EMAIL, "E-MAIL") \
  X(RTP_CHAT, "CSEVEGÉS") \
  X(RTP_VOTE, "SZAVAZÁS") \
  X(RTP_PLACE, "HELYSZÍN") \
  X(RTP_APPOINTMENT, "IDŐPONT") \
  X(RTP_ID, "AZONOSÍTÓ") \
  X(RTP_PURCHASE, "VÁSÁRLÁS") \
  X(DAY_SUNDAY, "VASÁRNAP") \
  X(DAY_MONDAY, "HÉTFŐ") \
  X(DAY_TUESDAY, "KEDD") \
  X(DAY_WEDNESDAY, "SZERDA") \
  X(DAY_THURSDAY, "CSÜTÖRTÖK") \
  X(DAY_FRIDAY, "PÉNTEK") \
  X(DAY_SATURDAY, "SZOMBAT") \
  X(MONTH_JANUARY, "január") \
  X(MONTH_FEBRUARY, "február") \
  X(MONTH_MARCH, "március") \
  X(MONTH_APRIL, "április") \
  X(MONTH_MAY, "május") \
  X(MONTH_JUNE, "június") \
  X(MONTH_JULY, "július") \
  X(MONTH_AUGUST, "augusztus") \
  X(MONTH_SEPTEMBER, "szeptember") \
  X(MONTH_OCTOBER, "október") \
  X(MONTH_NOVEMBER, "november") \
  X(MONTH_DECEMBER, "december") \
  X(DATE_SUFFIX_TH, ".") \
  X(DATE_SUFFIX_ST, ".") \
  X(DATE_SUFFIX_ND, ".") \
  X(DATE_SUFFIX_RD, ".") \
  X(DATE_FMT_LINE, "%5$d. %4$s %2$d%3$s, %1$s") \
  X(DATE_TODAY, "Ma") \
  X(DATE_YESTERDAY, "Tegnap") \
  X(DATE_YDAY, "TEGNAP") \
  X(DATE_FMT_WHEN_DATE, "%2$s %1$d. %3$s") \
  X(MONTH_SHORT_JAN, "jan.") \
  X(MONTH_SHORT_FEB, "febr.") \
  X(MONTH_SHORT_MAR, "márc.") \
  X(MONTH_SHORT_APR, "ápr.") \
  X(MONTH_SHORT_MAY, "máj.") \
  X(MONTH_SHORT_JUN, "jún.") \
  X(MONTH_SHORT_JUL, "júl.") \
  X(MONTH_SHORT_AUG, "aug.") \
  X(MONTH_SHORT_SEP, "szept.") \
  X(MONTH_SHORT_OCT, "okt.") \
  X(MONTH_SHORT_NOV, "nov.") \
  X(MONTH_SHORT_DEC, "dec.") \
  X(MONTH_TAG_MAR, "MÁR") \
  X(MONTH_TAG_APR, "ÁPR") \
  X(MONTH_TAG_MAY, "MÁJ") \
  X(MONTH_TAG_JUN, "JÚN") \
  X(MONTH_TAG_JUL, "JÚL") \
  X(MONTH_TAG_SEP, "SZE") \
  X(MONTH_TAG_OCT, "OKT") \
  X(DX_NOW, "most") \
  X(DX_FMT_MIN_AGO, "%u perce") \
  X(DX_FMT_H_AGO, "%u órája") \
  X(DX_FMT_D_AGO, "%u napja") \
  X(TOUCH_CAL_TITLE, "Érintés kalibrálása") \
  X(TOUCH_CAL_HOLD, "Tartsd nyomva a kört, amíg megtelik") \
  X(TOUCH_CAL_CANCEL_HINT, "Mégse: forgasd vagy nyomd meg a forgógombot") \
  X(TOUCH_CAL_CHECK, "Ellenőrzés") \
  X(TOUCH_CAL_TAP_DOT, "Érintsd meg a pontot") \
  X(TOUCH_CAL_SAVED, "Kalibrálás mentve") \
  X(TOUCH_CAL_FMT_LANDED, "Ellenőrzés: %u px eltérés") \
  X(TOUCH_CAL_BACK_HINT, "Vissza: nyomd meg a forgógombot") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Nincs mentve") \
  X(TOUCH_CAL_FMT_MISSED, "Ellenőrzés: %u px eltérés,") \
  X(TOUCH_CAL_NO_FIT, "A jelekből nem lett kalibrálás,") \
  X(TOUCH_CAL_NOT_SAVED, "Nem sikerült kiírni,") \
  X(TOUCH_CAL_OLD_STAYS, "így a régi kalibrálás marad") \
  X(TOUCH_CAL_RETRY_HINT, "Nyomd meg: újra, forgasd: kilépés")
// clang-format on
