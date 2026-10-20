/*
 * The panel's texts in Romanian. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_RO(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Scanare FM") \
  X(MENU_SCAN_MW, "Scanare AM") \
  X(MENU_SCAN_SW, "Scanare SW") \
  X(MENU_SCAN_LW, "Scanare LW") \
  X(MENU_SQUELCH, "Mod squelch") \
  X(MENU_SQUELCH_LEVEL, "Nivel squelch") \
  X(MENU_SQUELCH_FLOOR, "Squelch minim") \
  X(MENU_VOLUME_AGC, "AGC volum") \
  X(MENU_AGC_BOOST, "Amplif. AGC") \
  X(MENU_MUTE_RAMP, "Rampă sonor") \
  X(MENU_BAND_PLAN, "Plan bandă") \
  X(MENU_TUNING_STEP, "Pas acord") \
  X(MENU_SEEK_SENSITIVITY, "Sensib. căutare") \
  X(MENU_DE_EMPHASIS, "Deaccentuare") \
  X(MENU_FORCED_MONO, "Mono forțat") \
  X(MENU_STEREO_BLEND, "Amestec stereo") \
  X(MENU_ST_HI_BLEND, "Amestec st. / înalte") \
  X(MENU_HIGH_CUT, "Tăiere înalte") \
  X(MENU_NOISE_BLANKER, "Anti-paraziți") \
  X(MENU_EQUALISER, "Egalizator") \
  X(COMMON_RDS_DECODER, "Decodor RDS") \
  X(MENU_RDS_REGION, "Regiune RDS") \
  X(MENU_MW_SPACING, "Pas MW") \
  X(MENU_FILTER_WIDTH, "Lățime filtru") \
  X(MENU_SOFT_MUTE, "Atenuare lină") \
  X(MENU_DWELL, "Pauză scanare") \
  X(MENU_STOP_ON, "Oprire la") \
  X(MENU_SCAN, "Interval scanare") \
  X(MENU_MEMORY_FROM, "Prima memorie") \
  X(MENU_MEMORY_TO, "Ultima memorie") \
  X(MENU_PRESET_RANGE, "Interval memorii") \
  X(MENU_LOOP_THE_BAND, "Bandă în buclă") \
  X(MENU_DX_WIDTH, "Lățime scanare") \
  X(MENU_MUTE_WHILE_SCANNING, "Fără sonor la scanare") \
  X(MENU_AUTO_LOG_NEW, "Jurnal automat") \
  X(MENU_LOG_RADIO_TEXT, "Notează radiotext") \
  X(MENU_WATCH_PRESETS, "Urmărire memorii") \
  X(MENU_SCOPE_BASELINE, "Referință spectru") \
  X(MENU_MEDIAN, "Mediană") \
  X(MENU_FMT_SWEEP_N, "Baleiaj %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Învață posturi locale") \
  X(MENU_START_SCAN, "Scanează") \
  X(MENU_DAY_THEME, "Temă zi") \
  X(MENU_NIGHT_THEME, "Temă noapte") \
  X(MENU_THEME, "Temă") \
  X(MENU_BRIGHTNESS, "Luminozitate") \
  X(MENU_DIM_LEVEL, "Lumină redusă") \
  X(MENU_DIM_AFTER, "Reducere după") \
  X(MENU_ROTATION, "Rotire ecran") \
  X(COMMON_BATTERY, "Baterie") \
  X(MENU_BATTERY_CHECK, "Verificare baterie") \
  X(MENU_LANGUAGE, "Limbă \xC2\xB7 Language") \
  X(MENU_ENCODER, "Tip encoder") \
  X(MENU_DIRECTION, "Sens encoder") \
  X(MENU_KEY_BEEPS, "Bipuri taste") \
  X(MENU_BAND_EDGE_BEEP, "Bip capăt bandă") \
  X(MENU_START_CHIME, "Sunet pornire") \
  X(MENU_KEYPAD_TIMEOUT, "Expirare tastatură") \
  X(MENU_TOUCH, "Tactil") \
  X(MENU_CALIBRATE_TOUCH, "Calibrare tactil") \
  X(MENU_CLOCK_FROM_NETWORK, "Ora din rețea") \
  X(MENU_WEB_PIN, "PIN web") \
  X(MENU_NEW_PIN, "PIN nou") \
  X(MENU_FMT_DIGIT_OF, "Cifra %u din %u") \
  X(MENU_STATUS, "Stare conexiune") \
  X(MENU_WEB_SERVER, "Server web") \
  X(MENU_FMT_PC_CONNECTED, "Conectați: %u") \
  X(MENU_WEB_ADDRESS, "Adresă web") \
  X(MENU_IP_ADDRESS, "Adresă IP") \
  X(MENU_WI_FI_NAME, "Rețea Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Nume hotspot") \
  X(MENU_MAC, "Adresă MAC") \
  X(MENU_WI_FI_SIGNAL, "Semnal Wi-Fi") \
  X(MENU_NETWORK_INFO, "Info rețea") \
  X(MENU_RESTART, "Repornire") \
  X(MENU_AUTO_OFF, "Oprire auto") \
  X(MENU_UPDATE_CHECK, "Caută actualizări") \
  X(MENU_FIRMWARE_UPDATE, "Actualizare firmware") \
  X(MENU_FMT_UPDATE_TO, "Actualizare la %s") \
  X(MENU_UPDATE_TITLE, "Actualizare disponibilă") \
  X(MENU_UPDATE_THIS_RADIO, "Acest radio") \
  X(MENU_UPDATE_NEW_VERSION, "Versiune nouă") \
  X(MENU_UPDATE_DOWNLOAD, "Descărcare") \
  X(COMMON_SETTINGS, "Setări") \
  X(COMMON_KEPT, "Păstrate") \
  X(MENU_STATION, "Post") \
  X(MENU_UPDATE_NOW, "Actualizează") \
  X(MENU_LATER, "Mai târziu") \
  X(MENU_UPDATE_UP_TO_DATE, "La zi") \
  X(MENU_UPDATE_CHECKING, "Se verifică") \
  X(MENU_UPDATE_NOT_CHECKED, "Neverificat") \
  X(MENU_UPDATE_CHECK_FAILED, "Eroare verificare") \
  X(MENU_NOTE_NO_UPDATE, "Nicio actualizare") \
  X(MENU_VERSION, "Versiune firmware") \
  X(MENU_RUNNING_FROM, "Sursă boot") \
  X(MENU_UPTIME, "Pornit de") \
  X(MENU_BATTERY_VOLTAGE, "Tensiune baterie") \
  X(MENU_RESET_REASON, "Cauză reset") \
  X(MENU_CPU_CORE_0, "Nucleu CPU 0") \
  X(MENU_CPU_CORE_1, "Nucleu CPU 1") \
  X(MENU_FREE_HEAP, "Heap liber") \
  X(MENU_LOWEST_HEAP, "Heap minim") \
  X(MENU_LARGEST_BLOCK, "Bloc maxim") \
  X(MENU_LVGL_POOL, "Pool LVGL") \
  X(MENU_CHIP, "Cip") \
  X(MENU_FLASH_SIZE, "Mărime flash") \
  X(RESET_POWER, "Pornire") \
  X(RESET_PANIC, "Panică") \
  X(RESET_INT_WATCHDOG, "Watchdog IRQ") \
  X(RESET_TASK_WATCHDOG, "Watchdog task") \
  X(RESET_DEEP_SLEEP, "Repaus adânc") \
  X(RESET_BROWNOUT, "Subtensiune") \
  X(RESET_PIN, "Pin reset") \
  X(RESET_UNKNOWN, "Necunoscut") \
  X(RESET_ASKED, "Repornire cerută") \
  X(RESET_BOOT_WATCHDOG, "Watchdog boot") \
  X(RESET_ROLLBACK, "Revenire") \
  X(RESET_DISPLAY, "Eroare ecran") \
  X(RESET_UPDATE, "Actualizare") \
  X(MENU_DEVELOPER, "Dezvoltator") \
  X(MENU_LICENSE, "Licență") \
  X(ABOUT_BUILD_UNKNOWN, "necunoscut") \
  X(MENU_GO_TO, "Mergi la") \
  X(MENU_GO_TO_BANDWIDTH, "Lățime bandă") \
  X(MENU_GO_TO_DX, "Mod DX") \
  X(MENU_GO_TO_SCOPE, "Spectru bandă") \
  X(MENU_GO_TO_BAND, "Banda următoare") \
  X(MENU_GO_TO_LOG, "Notează postul") \
  X(MENU_GO_TO_SAVE, "Memorează postul") \
  X(MENU_FMT_SAVED_AS, "Memorat în P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Deja în P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Memorii pline") \
  X(MENU_GO_TO_SLEEP, "Repaus") \
  X(MENU_STATIONS, "Posturi") \
  X(MENU_MEMORY, "Memorii") \
  X(MENU_FM_SETUP, "Recepție FM") \
  X(MENU_AM_SETUP, "Recepție AM") \
  X(MENU_DX_SETUP, "Scaner DX") \
  X(MENU_DISPLAY, "Ecran") \
  X(MENU_CONTROLS, "Comenzi") \
  X(MENU_CONNECTIVITY, "Conectivitate") \
  X(MENU_SYSTEM, "Sistem") \
  X(MENU_DIAGNOSTICS, "Diagnoză") \
  X(MENU_ABOUT, "Despre") \
  X(MENU_REGION_FULL, "Complet") \
  X(MENU_REGION_JAPAN, "Japonia") \
  X(MENU_REGION_WIDE, "Larg") \
  X(MENU_REGION_WORLD, "Mondial") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "America de Nord") \
  X(MENU_WIFI_JOINED, "Conectat") \
  X(MENU_WIFI_JOINING, "Se conectează") \
  X(COMMON_OFF, "Nu") \
  X(MENU_FMT_KB_OF_KB, "%u din %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V la pornire") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, acum %u min") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, acum %u h") \
  X(MENU_FMT_AGC_GAIN_NOW, "Câștig acum %+d dB") \
  X(MENU_STATION_LOG, "Jurnal") \
  X(COMMON_NONE, "Niciunul") \
  X(MENU_FMT_SCAN_PROGRESS, "Scanare %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Memorate: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Oprit, memorate: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Plin, nememorate: %u") \
  X(COMMON_YES, "Da") \
  X(COMMON_NO, "Nu") \
  X(MENU_UNKNOWN_VALUE, "Necunoscut") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Inversat") \
  X(MENU_CUT_ONLY, "Doar atenuare") \
  X(MENU_NEVER, "Niciodată") \
  X(MENU_BATTERY_PER_CENT, "Procente") \
  X(MENU_BATTERY_VOLTS, "Volți") \
  X(MENU_KEYS, "Doar tastatura") \
  X(MENU_KEYS_LONG, "Apăsare scurtă/lungă") \
  X(MENU_EVERY_PRESS, "Orice apăsare") \
  X(MENU_OPTICAL, "Optic") \
  X(MENU_REVERSED, "Inversat") \
  X(MENU_ANY_PI, "Orice PI") \
  X(MENU_STOP_NEVER, "Niciodată") \
  X(MENU_NEW_ONLY, "Doar posturi noi") \
  X(MENU_WHOLE_BAND, "Toată banda") \
  X(MENU_MEMORY_ONLY, "Doar memorii") \
  X(MENU_BAND_MEMORY, "Bandă + memorii") \
  X(COMMON_ON, "Da") \
  X(MENU_HINT_RESTART, "Activ după repornire") \
  X(MENU_TITLE, "Meniu") \
  X(COMMON_FM_ONLY, "Doar FM") \
  X(MENU_NOTE_OFF_PLAN, "Nu e în planul benzii") \
  X(MENU_NOTE_SWITCH_TO_FM, "Treceți întâi pe FM") \
  X(MENU_NOTE_RADIO_BUSY, "Ocupat - reîncercați") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Porniți întâi RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Firmware în test - așteptați") \
  X(MENU_NOTE_NOT_STORED, "Nesalvat - restabilit") \
  X(MENU_NOTE_NOT_PUT_BACK, "Ocupat - nerestabilit") \
  X(MENU_NOTE_NOT_SAVED, "Nesalvat") \
  X(MENU_SAVE, "Salvează") \
  X(RADIO_PRESS_TO_START, "Apăsați pentru start") \
  X(COMMON_STILL_TUNING, "Se acordă - așteptați") \
  X(COMMON_SCAN_RUNNING, "Se scanează - așteptați") \
  X(RADIO_FMT_NO_BAND, "%s nu e în nicio bandă") \
  X(COMMON_NOT_LOGGED, "Nu s-a notat") \
  X(RADIO_FMT_LOGGED, "%s în jurnal") \
  X(RADIO_CHECKING_UPDATES, "SE CAUTĂ ACTUALIZĂRI\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Scanare %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Actualizare eșuată") \
  X(RADIO_GOING_TO_SLEEP, "Intrare în repaus") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Apăsați butonul pentru trezire") \
  X(RADIO_OLD_IMAGE_KEPT, "Firmware vechi păstrat") \
  X(RDS_SPEECH, "Vorbire") \
  X(RDS_MUSIC, "Muzică") \
  X(COMMON_ALREADY_LOGGED, "Deja în jurnal") \
  X(DX_RDS_IS_OFF, "RDS oprit") \
  X(DX_SCANNING, "Se scanează") \
  X(DX_SCAN_FINISHED, "Scanare încheiată") \
  X(DX_NOTHING_TO_SCAN, "Nimic de scanat") \
  X(DX_CHECKING_FOR_UPDATES, "Se caută actualizări") \
  X(DX_A_SCAN_IS_RUNNING, "Scanare deja pornită") \
  X(DX_CAUGHT_LIST_UNREAD, "Lista capturi indisponibilă") \
  X(DX_NO_MEMORY, "Fără memorii") \
  X(DX_NOT_STARTED, "Nepornit") \
  X(DX_SWEEPING, "Baleiaj") \
  X(COMMON_FMT_TUNE_TYPED, "Acord %s") \
  X(RADIO_SQL_OFF, "NU") \
  X(DX_FMT_ROWS_OF, "%u-%u din %u") \
  X(DX_FMT_FOUND, "Găsite: %u") \
  X(DX_MODE_SWEEPING, "BALEIAJ") \
  X(DX_MODE_LEARN_LOCALS, "ÎNVAȚĂ LOCALE") \
  X(DX_FMT_MODE_MEMORY, "MEMORII %u-%u") \
  X(DX_MODE_WHOLE_BAND, "TOATĂ BANDA") \
  X(DX_MODE_BAND_MEMORY, "BANDĂ + MEMORII") \
  X(DX_RULE_NO_STOP, "NON-STOP") \
  X(DX_RULE_STOP_ON_PI, "STOP LA PI") \
  X(DX_RULE_STOP_ON_NEW, "STOP LA NOU") \
  X(DX_FMT_NOT_PRESET, "nu e P%02d") \
  X(DX_CONTEXT_SWEEPING, "baleiaj") \
  X(DX_PRESS_TO_SWEEP, "Apăsați pentru baleiaj") \
  X(SCOPE_FMT_TITLE, "Spectru %s") \
  X(SCOPE_FULL, "Tot") \
  X(DX_FIXED, "FIX") \
  X(DX_FMT_MEDIAN_OF, "MEDIANA DIN %u") \
  X(DX_FMT_FLOOR, "FOND %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Închide") \
  X(RECOVERY_PRESS_AGAIN, "Apăsați din nou") \
  X(PAGE_TUNE, "Acord") \
  X(PAGE_DELETE, "Șterge") \
  X(PAGE_LEVEL, "Nivel") \
  X(PAGE_SNR, "S/Z") \
  X(PAGE_NOISE, "Zgomot") \
  X(PAGE_MULTIPATH, "Multicale") \
  X(PAGE_COCHANNEL, "Co-canal") \
  X(PAGE_WIDTH, "Lățime") \
  X(PAGE_BAND, "Bandă") \
  X(PAGE_NOTE_CHANGED, "Modificat între timp") \
  X(PAGE_NOTE_NOT_DELETED, "Neșters") \
  X(PAGE_FMT_SAVED, "Memorat %s") \
  X(PAGE_TIME_UNKNOWN, "Oră necunoscută") \
  X(BW_TITLE, "Lățime bandă") \
  X(KEYPAD_TITLE, "Frecvență") \
  X(KEYPAD_CANCEL, "Anulează") \
  X(DX_CATCHES_EMPTY_TITLE, "Nicio captură încă") \
  X(DX_CATCHES_EMPTY_NOTE, "Posturile confirmate în această sesiune apar aici") \
  X(DX_TITLE_CATCHES, "Capturi") \
  X(DX_NEW, "NOU") \
  X(DX_READY, "Gata") \
  X(DX_DWELL, "PAUZĂ") \
  X(DX_STOPPED, "OPRIT") \
  X(DX_TITLE_SCANNER, "Scaner") \
  X(DX_TITLE_SCOPE, "Spectru") \
  X(DX_LEVEL, "NIVEL") \
  X(DX_RISE, "DIF") \
  X(DX_NO_STATION_YET, "Niciun post încă") \
  X(DX_NO_RDS_YET, "Fără RDS încă") \
  X(DX_OFFSET, "DECAL.") \
  X(COMMON_BW, "LB") \
  X(DX_NO_ID, "FĂRĂ ID") \
  X(RDS_TITLE_TEXT, "Radiotext") \
  X(RDS_TITLE_NETWORKS, "Rețele") \
  X(RDS_TITLE_DECODER, "Decodor") \
  X(RDS_NO_COUNTRY, "fără ECC") \
  X(RDS_MS_UNKNOWN, "M/V") \
  X(RDS_RTPLUS_NONE, "Niciun RT+ de la acest post") \
  X(RDS_RTPLUS_WAIT, "Încă nicio etichetă RT+ pentru text") \
  X(RDS_AF_TITLE, "FRECVENȚE ALTERNATIVE") \
  X(RDS_EON_TITLE, "ALTE POSTURI, EON") \
  X(RDS_NONE_HEARD, "Nimic recepționat") \
  X(RDS_SYNC_CAPS, "SINC") \
  X(RDS_GROUPS_S_CAPS, "GRUP/S") \
  X(RDS_LOST, "LIPSĂ") \
  X(RDS_EACH_BLOCK, "PE BLOC") \
  X(RDS_LEGEND_CLEAN, "bune") \
  X(RDS_LEGEND_FIXED, "corectate") \
  X(RDS_LEGEND_LOST, "lipsă") \
  X(RDS_ALL_CLEAN, "toate bune") \
  X(RDS_GROUP_TYPES, "TIPURI DE GRUPURI") \
  X(RDS_NO_GROUPS, "Niciun grup în ultimul minut") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 pe %u s") \
  X(RDS_COUNTRY_UNLISTED, "nelistat") \
  X(RDS_NO_TYPES, "Niciun bloc B curat în ultimul minut") \
  X(RDS_FMT_EON_MORE, "+%u altele") \
  X(RDS_EON_MORE_HEARD, "+ altele") \
  X(RDS_MORE, "etc.") \
  X(RADIO_VEIL_UPDATING, "ACTUALIZARE FIRMWARE") \
  X(RADIO_VEIL_KEEP_POWER, "Nu opriți alimentarea") \
  X(RADIO_MUTE, "MUT") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Tuner absent pe magistrala I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tunerul nu e gata") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Identificare tuner eșuată") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Niciun patch pentru acest tuner") \
  X(TUNER_UNKNOWN_TUNER_PART, "Tuner necunoscut") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tunerul a refuzat scrierea") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tunerul nu răspunde la citire") \
  X(TUNER_OUT_OF_RANGE, "În afara limitelor") \
  X(TUNER_UNKNOWN, "Necunoscut") \
  X(BAND_LONG_LW, "Unde lungi") \
  X(BAND_LONG_MW, "Unde medii") \
  X(BAND_LONG_SW, "Unde scurte") \
  X(MENU_FMT_TUNE_TO, "Bandă pentru %s") \
  X(PTY_NONE, "Niciunul") \
  X(PTY_NEWS, "Știri") \
  X(PTY_CURRENT_AFFAIRS, "Actualități") \
  X(PTY_INFORMATION, "Informații") \
  X(PTY_EDUCATION, "Educație") \
  X(PTY_DRAMA, "Teatru") \
  X(PTY_CULTURE, "Cultură") \
  X(PTY_SCIENCE, "Știință") \
  X(PTY_VARIED, "Varietăți") \
  X(PTY_POP_MUSIC, "Muzică pop") \
  X(PTY_ROCK_MUSIC, "Muzică rock") \
  X(PTY_EASY_LISTENING, "Muzică ușoară") \
  X(PTY_LIGHT_CLASSICAL, "Clasică ușoară") \
  X(PTY_SERIOUS_CLASSICAL, "Clasică") \
  X(PTY_OTHER_MUSIC, "Altă muzică") \
  X(PTY_WEATHER, "Meteo") \
  X(PTY_FINANCE, "Finanțe") \
  X(PTY_CHILDREN, "Copii") \
  X(PTY_SOCIAL_AFFAIRS, "Social") \
  X(PTY_RELIGION, "Religie") \
  X(PTY_TRAVEL, "Călătorii") \
  X(PTY_LEISURE, "Timp liber") \
  X(PTY_COUNTRY_MUSIC, "Muzică country") \
  X(PTY_NATIONAL_MUSIC, "Muzică națională") \
  X(PTY_OLDIES_MUSIC, "Retro") \
  X(PTY_FOLK_MUSIC, "Folclor") \
  X(PTY_DOCUMENTARY, "Documentar") \
  X(PTY_ALARM_TEST, "Test alarmă") \
  X(PTY_ALARM, "Alarmă") \
  X(PTY_NA_NEWS, "Știri") \
  X(PTY_NA_INFORMATION, "Informații") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_TALK, "Discuții") \
  X(PTY_NA_CLASSIC_ROCK, "Rock clasic") \
  X(PTY_NA_ADULT_HITS, "Hituri adulți") \
  X(PTY_NA_SOFT_ROCK, "Rock soft") \
  X(PTY_NA_OLDIES, "Retro") \
  X(PTY_NA_SOFT, "Lejer") \
  X(PTY_NA_NOSTALGIA, "Nostalgie") \
  X(PTY_NA_CLASSICAL, "Clasică") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "R&B lejer") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Limbă străină") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Muzică religioasă") \
  X(PTY_NA_RELIGIOUS_TALK, "Discuții religioase") \
  X(PTY_NA_PERSONALITY, "Personalitate") \
  X(PTY_NA_COLLEGE, "Universitar") \
  X(PTY_NA_SPANISH_TALK, "Discuții în spaniolă") \
  X(PTY_NA_SPANISH_MUSIC, "Muzică spaniolă") \
  X(PTY_NA_UNASSIGNED, "Nealocat") \
  X(PTY_NA_WEATHER, "Meteo") \
  X(PTY_NA_EMERGENCY_TEST, "Test urgență") \
  X(PTY_NA_EMERGENCY, "Urgență") \
  X(COVERAGE_INTERNATIONAL, "Internațional") \
  X(COVERAGE_NATIONAL, "Național") \
  X(COVERAGE_SUPRA_REGIONAL, "Supraregional") \
  X(RTP_OTHER, "ALTELE") \
  X(RTP_TITLE, "TITLU") \
  X(RTP_TRACK, "PIESĂ") \
  X(RTP_WORK, "OPERĂ") \
  X(RTP_MOVEMENT, "MIȘCARE") \
  X(RTP_CONDUCTOR, "DIRIJOR") \
  X(RTP_COMPOSER, "COMPOZITOR") \
  X(RTP_BAND, "FORMAȚIE") \
  X(RTP_COMMENT, "COMENTARIU") \
  X(RTP_GENRE, "GEN") \
  X(RTP_NEWS, "ȘTIRI") \
  X(RTP_LOCAL, "ȘTIRI LOCALE") \
  X(RTP_STOCKS, "BURSĂ") \
  X(RTP_LOTTERY, "LOTO") \
  X(RTP_HOROSCOPE, "HOROSCOP") \
  X(RTP_DIVERSION, "RELAXARE") \
  X(RTP_HEALTH, "SĂNĂTATE") \
  X(RTP_EVENT, "EVENIMENT") \
  X(RTP_SCENE, "SCENĂ") \
  X(RTP_FUN, "UMOR") \
  X(RTP_DATE, "DATA") \
  X(RTP_WEATHER, "METEO") \
  X(RTP_TRAFFIC, "TRAFIC") \
  X(RTP_ALARM, "ALARMĂ") \
  X(RTP_ADVERT, "RECLAMĂ") \
  X(RTP_STATION, "POST") \
  X(RTP_PROGRAM, "EMISIUNE") \
  X(RTP_NEXT, "URMEAZĂ") \
  X(RTP_PART, "PARTE") \
  X(RTP_HOST, "PREZENTATOR") \
  X(RTP_EDITORS, "REDACȚIE") \
  X(RTP_FREQUENCY, "FRECVENȚĂ") \
  X(RTP_HOMEPAGE, "SITE WEB") \
  X(RTP_CHANNEL, "CANAL") \
  X(RTP_HOTLINE, "LINIE DIRECTĂ") \
  X(RTP_PHONE, "TELEFON") \
  X(RTP_VOTE, "VOT") \
  X(RTP_PLACE, "LOC") \
  X(RTP_APPOINTMENT, "ÎNTÂLNIRE") \
  X(RTP_PURCHASE, "CUMPĂRARE") \
  X(DAY_SUNDAY, "DUMINICĂ") \
  X(DAY_MONDAY, "LUNI") \
  X(DAY_TUESDAY, "MARȚI") \
  X(DAY_WEDNESDAY, "MIERCURI") \
  X(DAY_THURSDAY, "JOI") \
  X(DAY_FRIDAY, "VINERI") \
  X(DAY_SATURDAY, "SÂMBĂTĂ") \
  X(MONTH_JANUARY, "ianuarie") \
  X(MONTH_FEBRUARY, "februarie") \
  X(MONTH_MARCH, "martie") \
  X(MONTH_APRIL, "aprilie") \
  X(MONTH_MAY, "mai") \
  X(MONTH_JUNE, "iunie") \
  X(MONTH_JULY, "iulie") \
  X(MONTH_AUGUST, "august") \
  X(MONTH_SEPTEMBER, "septembrie") \
  X(MONTH_OCTOBER, "octombrie") \
  X(MONTH_NOVEMBER, "noiembrie") \
  X(MONTH_DECEMBER, "decembrie") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_TODAY, "Azi") \
  X(DATE_YESTERDAY, "Ieri") \
  X(DATE_YDAY, "IERI") \
  X(MONTH_SHORT_JAN, "ian") \
  X(MONTH_SHORT_FEB, "feb") \
  X(MONTH_SHORT_MAR, "mar") \
  X(MONTH_SHORT_APR, "apr") \
  X(MONTH_SHORT_MAY, "mai") \
  X(MONTH_SHORT_JUN, "iun") \
  X(MONTH_SHORT_JUL, "iul") \
  X(MONTH_SHORT_AUG, "aug") \
  X(MONTH_SHORT_SEP, "sep") \
  X(MONTH_SHORT_OCT, "oct") \
  X(MONTH_SHORT_NOV, "nov") \
  X(MONTH_SHORT_DEC, "dec") \
  X(MONTH_TAG_JAN, "IAN") \
  X(MONTH_TAG_MAY, "MAI") \
  X(MONTH_TAG_JUN, "IUN") \
  X(MONTH_TAG_JUL, "IUL") \
  X(DX_NOW, "acum") \
  X(DX_FMT_MIN_AGO, "acum %u min") \
  X(DX_FMT_H_AGO, "acum %u h") \
  X(DX_FMT_D_AGO, "acum %u z") \
  X(TOUCH_CAL_TITLE, "Calibrare tactil") \
  X(TOUCH_CAL_HOLD, "Țineți pe cerc până se umple") \
  X(TOUCH_CAL_CANCEL_HINT, "Rotiți sau apăsați butonul: anulare") \
  X(TOUCH_CAL_CHECK, "Test") \
  X(TOUCH_CAL_TAP_DOT, "Atingeți punctul") \
  X(TOUCH_CAL_SAVED, "Calibrare salvată") \
  X(TOUCH_CAL_FMT_LANDED, "Abatere test: %u px") \
  X(TOUCH_CAL_BACK_HINT, "Apăsați butonul: revenire") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Nesalvat") \
  X(TOUCH_CAL_FMT_MISSED, "Abatere test: %u px,") \
  X(TOUCH_CAL_NO_FIT, "Marcajele nu au dat o calibrare,") \
  X(TOUCH_CAL_NOT_SAVED, "Nu s-a putut scrie,") \
  X(TOUCH_CAL_OLD_STAYS, "deci rămâne calibrarea veche") \
  X(TOUCH_CAL_RETRY_HINT, "Apăsați butonul: reîncercare. Rotiți: ieșire")
// clang-format on
