/*
 * The panel's texts in Norwegian Bokmal. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_NB(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "FM-skanning") \
  X(MENU_SCAN_MW, "AM-skanning") \
  X(MENU_SCAN_SW, "SW-skanning") \
  X(MENU_SCAN_LW, "LW-skanning") \
  X(MENU_SQUELCH, "Squelch-modus") \
  X(MENU_SQUELCH_LEVEL, "Squelch-nivå") \
  X(MENU_SQUELCH_FLOOR, "Nedre grense") \
  X(MENU_VOLUME_AGC, "Volum-AGC") \
  X(MENU_AGC_BOOST, "AGC-løft") \
  X(MENU_MUTE_RAMP, "Dempetid") \
  X(MENU_BAND_PLAN, "Båndplan") \
  X(MENU_TUNING_STEP, "Trinnstørrelse") \
  X(MENU_SEEK_SENSITIVITY, "Søkefølsomhet") \
  X(MENU_DE_EMPHASIS, "De-emfase") \
  X(MENU_FORCED_MONO, "Tving mono") \
  X(MENU_STEREO_BLEND, "Stereoblanding") \
  X(MENU_ST_HI_BLEND, "Stereo/diskantblanding") \
  X(MENU_HIGH_CUT, "Diskantkutt") \
  X(MENU_NOISE_BLANKER, "Støydemper") \
  X(COMMON_RDS_DECODER, "RDS-dekoder") \
  X(MENU_RDS_REGION, "RDS-region") \
  X(MENU_MW_SPACING, "MW-trinn") \
  X(MENU_FILTER_WIDTH, "Filterbåndbredde") \
  X(MENU_SOFT_MUTE, "Softmute") \
  X(MENU_DWELL, "Ventetid") \
  X(MENU_STOP_ON, "Stopp ved") \
  X(MENU_SCAN, "Skanneområde") \
  X(MENU_MEMORY_FROM, "Minne fra") \
  X(MENU_MEMORY_TO, "Minne til") \
  X(MENU_PRESET_RANGE, "Minneområde") \
  X(MENU_LOOP_THE_BAND, "Gjenta bånd") \
  X(MENU_DX_WIDTH, "Skannebåndbredde") \
  X(MENU_MUTE_WHILE_SCANNING, "Demp ved skanning") \
  X(MENU_AUTO_LOG_NEW, "Autologg") \
  X(MENU_LOG_RADIO_TEXT, "Logg radiotekst") \
  X(MENU_WATCH_PRESETS, "Overvåk minne") \
  X(MENU_SCOPE_BASELINE, "Scope-referanse") \
  X(MENU_FMT_SWEEP_N, "Sveip %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Lær lokale stasjoner") \
  X(MENU_START_SCAN, "Start skanning") \
  X(MENU_DAY_THEME, "Dagtema") \
  X(MENU_NIGHT_THEME, "Natt-tema") \
  X(MENU_THEME, "Tema") \
  X(MENU_BRIGHTNESS, "Lysstyrke") \
  X(MENU_DIM_LEVEL, "Dimmenivå") \
  X(MENU_DIM_AFTER, "Dim etter") \
  X(MENU_ROTATION, "Skjermrotasjon") \
  X(COMMON_BATTERY, "Batteri") \
  X(MENU_BATTERY_CHECK, "Batterisjekk") \
  X(MENU_LANGUAGE, "Språk \xC2\xB7 Language") \
  X(MENU_ENCODER, "Enkodertype") \
  X(MENU_DIRECTION, "Dreieretning") \
  X(MENU_ENCODER_GROUP, "Enkoder") \
  X(MENU_KEY_BEEPS, "Tastelyd") \
  X(MENU_BAND_EDGE_BEEP, "Pip ved båndkant") \
  X(MENU_START_CHIME, "Oppstartslyd") \
  X(MENU_KEYPAD_TIMEOUT, "Tastaturtid") \
  X(MENU_TOUCH, "Berøring") \
  X(MENU_CALIBRATE_TOUCH, "Kalibrer berøring") \
  X(MENU_CLOCK_FROM_NETWORK, "Nettverkstid") \
  X(MENU_WEB_PIN, "Web-PIN") \
  X(MENU_NEW_PIN, "Ny PIN") \
  X(MENU_FMT_DIGIT_OF, "Siffer %u/%u") \
  X(MENU_STATUS, "Tilkoblingsstatus") \
  X(MENU_WEB_SERVER, "Webserver") \
  X(MENU_FMT_PC_CONNECTED, "%u tilkoblet") \
  X(MENU_WEB_ADDRESS, "Webadresse") \
  X(MENU_IP_ADDRESS, "IP-adresse") \
  X(MENU_WI_FI_NAME, "Wi-Fi-nett") \
  X(MENU_HOTSPOT_NAME, "Hotspot-navn") \
  X(MENU_MAC, "MAC-adresse") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi-signal") \
  X(MENU_NETWORK_INFO, "Nettverksinfo") \
  X(MENU_RESTART, "Omstart") \
  X(MENU_AUTO_OFF, "Auto av") \
  X(MENU_UPDATE_CHECK, "Sjekk oppdateringer") \
  X(MENU_FIRMWARE_UPDATE, "Oppdatering") \
  X(MENU_FMT_UPDATE_TO, "Oppdater til %s") \
  X(MENU_UPDATE_TITLE, "Oppdatering klar") \
  X(MENU_UPDATE_THIS_RADIO, "Installert") \
  X(MENU_UPDATE_NEW_VERSION, "Ny versjon") \
  X(MENU_UPDATE_DOWNLOAD, "Nedlasting") \
  X(COMMON_SETTINGS, "Innstillinger") \
  X(COMMON_KEPT, "Beholdt") \
  X(MENU_STATION, "Stasjon") \
  X(MENU_UPDATE_NOW, "Oppdater") \
  X(MENU_LATER, "Senere") \
  X(MENU_UPDATE_UP_TO_DATE, "Oppdatert") \
  X(MENU_UPDATE_CHECKING, "Sjekker") \
  X(MENU_UPDATE_NOT_CHECKED, "Ikke sjekket") \
  X(MENU_UPDATE_CHECK_FAILED, "Sjekk feilet") \
  X(MENU_NOTE_NO_UPDATE, "Ingen oppdatering") \
  X(MENU_VERSION, "Fastvareversjon") \
  X(MENU_RUNNING_FROM, "Oppstartskilde") \
  X(MENU_UPTIME, "Oppetid") \
  X(MENU_BATTERY_VOLTAGE, "Batterispenning") \
  X(MENU_RESET_REASON, "Reset-årsak") \
  X(MENU_CPU_CORE_0, "CPU-kjerne 0") \
  X(MENU_CPU_CORE_1, "CPU-kjerne 1") \
  X(MENU_FREE_HEAP, "Ledig heap") \
  X(MENU_LOWEST_HEAP, "Laveste heap") \
  X(MENU_LARGEST_BLOCK, "Største blokk") \
  X(MENU_LVGL_POOL, "LVGL-pool") \
  X(MENU_CHIP, "Brikke") \
  X(MENU_FLASH_SIZE, "Flashminne") \
  X(RESET_POWER, "Påslåing") \
  X(RESET_SOFTWARE, "Programvare") \
  X(RESET_PANIC, "Panikk") \
  X(RESET_INT_WATCHDOG, "IRQ-watchdog") \
  X(RESET_TASK_WATCHDOG, "Task-watchdog") \
  X(RESET_DEEP_SLEEP, "Dyp dvale") \
  X(RESET_BROWNOUT, "Lav spenning") \
  X(RESET_PIN, "Reset-pinne") \
  X(RESET_UNKNOWN, "Ukjent") \
  X(RESET_ASKED, "Omstart valgt") \
  X(RESET_BOOT_WATCHDOG, "Boot-watchdog") \
  X(RESET_ROLLBACK, "Rullet tilbake") \
  X(RESET_DISPLAY, "Skjermfeil") \
  X(RESET_UPDATE, "Oppdatering") \
  X(MENU_DEVELOPER, "Utvikler") \
  X(MENU_BUILD, "Bygg") \
  X(MENU_LICENSE, "Lisens") \
  X(ABOUT_BUILD_UNKNOWN, "ukjent") \
  X(MENU_GO_TO, "Gå til") \
  X(MENU_GO_TO_BANDWIDTH, "Båndbredde") \
  X(MENU_GO_TO_DX, "DX-modus") \
  X(MENU_GO_TO_SCOPE, "Båndscope") \
  X(MENU_GO_TO_BAND, "Neste bånd") \
  X(MENU_GO_TO_LOG, "Logg stasjonen") \
  X(MENU_GO_TO_SAVE, "Lagre stasjonen") \
  X(MENU_FMT_SAVED_AS, "Lagret som P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Allerede P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Minnet er fullt") \
  X(MENU_GO_TO_SLEEP, "Dvale") \
  X(MENU_STATIONS, "Stasjoner") \
  X(MENU_MEMORY, "Minne") \
  X(MENU_AUDIO, "Lyd") \
  X(MENU_FM_SETUP, "FM-mottak") \
  X(MENU_AM_SETUP, "AM-mottak") \
  X(MENU_DX_SETUP, "DX-skanner") \
  X(MENU_DISPLAY, "Skjerm") \
  X(MENU_CONTROLS, "Betjening") \
  X(MENU_CONNECTIVITY, "Tilkobling") \
  X(MENU_DIAGNOSTICS, "Diagnostikk") \
  X(MENU_ABOUT, "Om") \
  X(MENU_REGION_WIDE, "Bred") \
  X(MENU_REGION_WORLD, "Verden") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Nord-Amerika") \
  X(MENU_WIFI_JOINED, "Tilkoblet") \
  X(MENU_WIFI_JOINING, "Kobler til") \
  X(COMMON_OFF, "Av") \
  X(MENU_FMT_HOURS_MINUTES, "%u t %u min") \
  X(MENU_FMT_KB_OF_KB, "%u av %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V ved start") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, %u min siden") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, %u t siden") \
  X(MENU_FMT_AGC_GAIN_NOW, "Nå %+d dB") \
  X(MENU_STATION_LOG, "Loggbok") \
  X(COMMON_NONE, "Ingen") \
  X(MENU_FMT_SCAN_PROGRESS, "Skanner %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Lagret %u") \
  X(MENU_FMT_SCAN_STOPPED, "Stoppet, %u lagret") \
  X(MENU_FMT_SCAN_NO_ROOM, "Fullt, %u ikke lagret") \
  X(COMMON_YES, "Ja") \
  X(COMMON_NO, "Nei") \
  X(MENU_UNKNOWN_VALUE, "Ukjent") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Opp ned") \
  X(MENU_CUT_ONLY, "Kun kutt") \
  X(MENU_NEVER, "Aldri") \
  X(MENU_BATTERY_PER_CENT, "Prosent") \
  X(MENU_BATTERY_VOLTS, "Volt") \
  X(MENU_KEYS, "Kun tastatur") \
  X(MENU_KEYS_LONG, "Kort og langt trykk") \
  X(MENU_EVERY_PRESS, "Hvert trykk") \
  X(MENU_OPTICAL, "Optisk") \
  X(MENU_REVERSED, "Omvendt") \
  X(MENU_SQUELCH_MAN, "Manuell") \
  X(MENU_ANY_PI, "Alle PI") \
  X(MENU_STOP_NEVER, "Aldri") \
  X(MENU_NEW_ONLY, "Kun nye stasjoner") \
  X(MENU_WHOLE_BAND, "Hele båndet") \
  X(MENU_MEMORY_ONLY, "Kun minne") \
  X(MENU_BAND_MEMORY, "Bånd + minne") \
  X(COMMON_ON, "På") \
  X(MENU_HINT_RESTART, "Gjelder etter omstart") \
  X(MENU_TITLE, "Meny") \
  X(COMMON_FM_ONLY, "Kun FM") \
  X(MENU_NOTE_OFF_PLAN, "Ikke i båndplanen") \
  X(MENU_NOTE_SWITCH_TO_FM, "Bytt til FM først") \
  X(MENU_NOTE_RADIO_BUSY, "Opptatt - prøv igjen") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Slå på RDS først") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Ny fastvare testes - vent") \
  X(MENU_NOTE_NOT_STORED, "Ikke lagret - angret") \
  X(MENU_NOTE_NOT_PUT_BACK, "Opptatt - ikke angret") \
  X(MENU_NOTE_NOT_SAVED, "Ikke lagret") \
  X(MENU_SAVE, "Lagre") \
  X(RADIO_PRESS_TO_START, "Trykk for start") \
  X(COMMON_STILL_TUNING, "Søker - vent") \
  X(COMMON_SCAN_RUNNING, "Skanner - vent") \
  X(RADIO_FMT_NO_BAND, "%s: intet bånd") \
  X(COMMON_NOT_LOGGED, "Ikke logget") \
  X(RADIO_FMT_LOGGED, "Logget %s") \
  X(RADIO_CHECKING_UPDATES, "SJEKKER OPPDATERINGER\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Skanner %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Oppdatering feilet") \
  X(RADIO_GOING_TO_SLEEP, "Går i dvale") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Trykk knotten for å vekke") \
  X(RADIO_OLD_IMAGE_KEPT, "Forrige fastvare beholdt") \
  X(RDS_SPEECH, "Tale") \
  X(RDS_MUSIC, "Musikk") \
  X(COMMON_ALREADY_LOGGED, "Allerede logget") \
  X(DX_RDS_IS_OFF, "RDS er av") \
  X(DX_SCANNING, "Skanner") \
  X(DX_SCAN_FINISHED, "Skanning ferdig") \
  X(DX_NOTHING_TO_SCAN, "Ingenting å skanne") \
  X(DX_CHECKING_FOR_UPDATES, "Sjekker oppdateringer") \
  X(DX_A_SCAN_IS_RUNNING, "Skanner allerede") \
  X(DX_CAUGHT_LIST_UNREAD, "Fangstliste mangler") \
  X(DX_NO_MEMORY, "Tomt minne") \
  X(DX_NOT_STARTED, "Ikke startet") \
  X(DX_SWEEPING, "Sveiper") \
  X(COMMON_FMT_TUNE_TYPED, "Til %s") \
  X(RADIO_SQL_OFF, "AV") \
  X(DX_FMT_ROWS_OF, "%u-%u av %u") \
  X(DX_FMT_FOUND, "%u funnet") \
  X(DX_MODE_SWEEPING, "SVEIPER") \
  X(DX_MODE_LEARN_LOCALS, "LÆR LOKALE") \
  X(DX_FMT_MODE_MEMORY, "MINNE %u-%u") \
  X(DX_MODE_WHOLE_BAND, "HELE BÅNDET") \
  X(DX_MODE_BAND_MEMORY, "BÅND + MINNE") \
  X(DX_RULE_NO_STOP, "NONSTOP") \
  X(DX_RULE_STOP_ON_PI, "STOPP PÅ PI") \
  X(DX_RULE_STOP_ON_NEW, "STOPP PÅ NY") \
  X(DX_FMT_NOT_PRESET, "ikke P%02d") \
  X(DX_CONTEXT_SWEEPING, "sveiper") \
  X(DX_PRESS_TO_SWEEP, "Trykk for sveip") \
  X(SCOPE_FMT_TITLE, "%s-scope") \
  X(SCOPE_FULL, "Alt") \
  X(DX_FIXED, "FAST") \
  X(DX_FMT_MEDIAN_OF, "MEDIAN AV %u") \
  X(DX_FMT_FLOOR, "GULV %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Lukk") \
  X(RECOVERY_PRESS_AGAIN, "Trykk igjen") \
  X(PAGE_TUNE, "Lytt") \
  X(PAGE_DELETE, "Slett") \
  X(PAGE_LEVEL, "Nivå") \
  X(PAGE_NOISE, "Støy") \
  X(PAGE_MULTIPATH, "Flerveis") \
  X(PAGE_COCHANNEL, "Samkanal") \
  X(PAGE_WIDTH, "Filter") \
  X(PAGE_BAND, "Bånd") \
  X(PAGE_NOTE_CHANGED, "Endret etter åpning") \
  X(PAGE_NOTE_NOT_DELETED, "Ikke slettet") \
  X(PAGE_FMT_SAVED, "Lagret %s") \
  X(PAGE_TIME_UNKNOWN, "Ukjent tid") \
  X(BW_TITLE, "Båndbredde") \
  X(KEYPAD_TITLE, "Frekvens") \
  X(KEYPAD_CANCEL, "Avbryt") \
  X(DX_CATCHES_EMPTY_TITLE, "Ingen fangster ennå") \
  X(DX_CATCHES_EMPTY_NOTE, "Bekreftede stasjoner fra denne økten vises her") \
  X(DX_TITLE_CATCHES, "Fangster") \
  X(DX_NEW, "NY") \
  X(DX_READY, "Klar") \
  X(DX_DWELL, "TID") \
  X(DX_STOPPED, "STOPPET") \
  X(DX_TITLE_SCANNER, "Skanner") \
  X(DX_LEVEL, "NIVÅ") \
  X(DX_RISE, "DIFF") \
  X(DX_NO_STATION_YET, "Ingen stasjon ennå") \
  X(DX_NO_RDS_YET, "Ingen RDS ennå") \
  X(DX_NO_ID, "UTEN ID") \
  X(RDS_TITLE_TEXT, "Radiotekst") \
  X(RDS_TITLE_NETWORKS, "Nettverk") \
  X(RDS_TITLE_DECODER, "Dekoder") \
  X(RDS_NO_COUNTRY, "ingen ECC") \
  X(RDS_MS_UNKNOWN, "M/T") \
  X(RDS_RTPLUS_NONE, "Ingen RT+ fra denne stasjonen") \
  X(RDS_RTPLUS_WAIT, "Ingen RT+ for teksten ennå") \
  X(RDS_AF_TITLE, "ALTERNATIVE FREKVENSER") \
  X(RDS_EON_TITLE, "ANDRE NETT, EON") \
  X(RDS_NONE_HEARD, "Ingen hørt") \
  X(RDS_SYNC_CAPS, "SYNK") \
  X(RDS_GROUPS_S_CAPS, "GRP/S") \
  X(RDS_LOST, "TAPT") \
  X(RDS_EACH_BLOCK, "HVER BLOKK") \
  X(RDS_LEGEND_CLEAN, "ren") \
  X(RDS_LEGEND_FIXED, "rettet") \
  X(RDS_LEGEND_LOST, "tapt") \
  X(RDS_ALL_CLEAN, "alt rent") \
  X(RDS_GROUP_TYPES, "GRUPPETYPER") \
  X(RDS_NO_GROUPS, "Ingen grupper siste minutt") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 %u s") \
  X(RDS_COUNTRY_UNLISTED, "ukjent") \
  X(RDS_NO_TYPES, "Ingen blokk B lest feilfritt siste minutt") \
  X(RDS_FMT_EON_MORE, "+%u flere") \
  X(RDS_EON_MORE_HEARD, "+ flere") \
  X(RDS_MORE, "flere") \
  X(RADIO_VEIL_UPDATING, "OPPDATERER FASTVARE") \
  X(RADIO_VEIL_KEEP_POWER, "Ikke slå av strømmen") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Ingen tuner på I2C-bussen") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tuner ble aldri klar") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Tuner ikke identifisert") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Ingen patch for denne tuneren") \
  X(TUNER_UNKNOWN_TUNER_PART, "Ukjent tuner") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tuner avviste skriving") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tuner svarte ikke på lesing") \
  X(TUNER_OUT_OF_RANGE, "Utenfor område") \
  X(TUNER_UNKNOWN, "Ukjent") \
  X(BAND_LONG_LW, "Langbølge") \
  X(BAND_LONG_MW, "Mellombølge") \
  X(BAND_LONG_SW, "Kortbølge") \
  X(MENU_FMT_TUNE_TO, "Bånd for %s") \
  X(PTY_NONE, "Ingen") \
  X(PTY_NEWS, "Nyheter") \
  X(PTY_CURRENT_AFFAIRS, "Aktualiteter") \
  X(PTY_INFORMATION, "Informasjon") \
  X(PTY_EDUCATION, "Utdanning") \
  X(PTY_CULTURE, "Kultur") \
  X(PTY_SCIENCE, "Vitenskap") \
  X(PTY_VARIED, "Variert") \
  X(PTY_POP_MUSIC, "Popmusikk") \
  X(PTY_ROCK_MUSIC, "Rockmusikk") \
  X(PTY_EASY_LISTENING, "Lett musikk") \
  X(PTY_LIGHT_CLASSICAL, "Lett klassisk") \
  X(PTY_SERIOUS_CLASSICAL, "Klassisk") \
  X(PTY_OTHER_MUSIC, "Annen musikk") \
  X(PTY_WEATHER, "Vær") \
  X(PTY_FINANCE, "Økonomi") \
  X(PTY_CHILDREN, "Barn") \
  X(PTY_SOCIAL_AFFAIRS, "Samfunn") \
  X(PTY_PHONE_IN, "Ring inn") \
  X(PTY_TRAVEL, "Reise") \
  X(PTY_LEISURE, "Fritid") \
  X(PTY_COUNTRY_MUSIC, "Countrymusikk") \
  X(PTY_NATIONAL_MUSIC, "Nasjonal musikk") \
  X(PTY_FOLK_MUSIC, "Folkemusikk") \
  X(PTY_DOCUMENTARY, "Dokumentar") \
  X(PTY_ALARM_TEST, "Alarmtest") \
  X(PTY_NA_NEWS, "Nyheter") \
  X(PTY_NA_INFORMATION, "Informasjon") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_TALK, "Prat") \
  X(PTY_NA_NOSTALGIA, "Nostalgi") \
  X(PTY_NA_CLASSICAL, "Klassisk") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Rhythm & blues") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Soft rhythm & blues") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Fremmedspråk") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Religiøs musikk") \
  X(PTY_NA_RELIGIOUS_TALK, "Religiøs prat") \
  X(PTY_NA_PERSONALITY, "Personlighet") \
  X(PTY_NA_PUBLIC, "Offentlig") \
  X(PTY_NA_SPANISH_TALK, "Spansk prat") \
  X(PTY_NA_SPANISH_MUSIC, "Spansk musikk") \
  X(PTY_NA_UNASSIGNED, "Ikke tildelt") \
  X(PTY_NA_WEATHER, "Vær") \
  X(PTY_NA_EMERGENCY_TEST, "Nødtest") \
  X(PTY_NA_EMERGENCY, "Nødvarsel") \
  X(COVERAGE_LOCAL, "Lokal") \
  X(COVERAGE_INTERNATIONAL, "Internasjonal") \
  X(COVERAGE_NATIONAL, "Nasjonal") \
  X(COVERAGE_SUPRA_REGIONAL, "Overregional") \
  X(RTP_OTHER, "ANNET") \
  X(RTP_TITLE, "TITTEL") \
  X(RTP_TRACK, "SPOR") \
  X(RTP_WORK, "VERK") \
  X(RTP_MOVEMENT, "SATS") \
  X(RTP_CONDUCTOR, "DIRIGENT") \
  X(RTP_COMPOSER, "KOMPONIST") \
  X(RTP_COMMENT, "MERKNAD") \
  X(RTP_GENRE, "SJANGER") \
  X(RTP_NEWS, "NYTT") \
  X(RTP_LOCAL, "LOKALNYTT") \
  X(RTP_STOCKS, "AKSJER") \
  X(RTP_LOTTERY, "LOTTERI") \
  X(RTP_HOROSCOPE, "HOROSKOP") \
  X(RTP_DIVERSION, "TIDSFORDRIV") \
  X(RTP_HEALTH, "HELSE") \
  X(RTP_CINEMA, "KINO") \
  X(RTP_FUN, "GØY") \
  X(RTP_DATE, "DATO") \
  X(RTP_WEATHER, "VÆR") \
  X(RTP_TRAFFIC, "TRAFIKK") \
  X(RTP_ADVERT, "REKLAME") \
  X(RTP_STATION, "STASJON") \
  X(RTP_NEXT, "NESTE") \
  X(RTP_PART, "DEL") \
  X(RTP_HOST, "VERT") \
  X(RTP_EDITORS, "RED.") \
  X(RTP_FREQUENCY, "FREKVENS") \
  X(RTP_HOMEPAGE, "NETTSIDE") \
  X(RTP_CHANNEL, "KANAL") \
  X(RTP_PHONE, "TLF.") \
  X(RTP_EMAIL, "E-POST") \
  X(RTP_VOTE, "STEM") \
  X(RTP_PLACE, "STED") \
  X(RTP_APPOINTMENT, "AVTALE") \
  X(RTP_PURCHASE, "KJØP") \
  X(DAY_SUNDAY, "SØNDAG") \
  X(DAY_MONDAY, "MANDAG") \
  X(DAY_TUESDAY, "TIRSDAG") \
  X(DAY_WEDNESDAY, "ONSDAG") \
  X(DAY_THURSDAY, "TORSDAG") \
  X(DAY_FRIDAY, "FREDAG") \
  X(DAY_SATURDAY, "LØRDAG") \
  X(MONTH_JANUARY, "januar") \
  X(MONTH_FEBRUARY, "februar") \
  X(MONTH_MARCH, "mars") \
  X(MONTH_APRIL, "april") \
  X(MONTH_MAY, "mai") \
  X(MONTH_JUNE, "juni") \
  X(MONTH_JULY, "juli") \
  X(MONTH_AUGUST, "august") \
  X(MONTH_SEPTEMBER, "september") \
  X(MONTH_OCTOBER, "oktober") \
  X(MONTH_NOVEMBER, "november") \
  X(MONTH_DECEMBER, "desember") \
  X(DATE_SUFFIX_TH, ".") \
  X(DATE_SUFFIX_ST, ".") \
  X(DATE_SUFFIX_ND, ".") \
  X(DATE_SUFFIX_RD, ".") \
  X(DATE_FMT_LINE, "%s %d%s %s %d") \
  X(DATE_TODAY, "I dag") \
  X(DATE_YESTERDAY, "I går") \
  X(DATE_YDAY, "I GÅR") \
  X(DATE_FMT_WHEN_DATE, "%d. %s %s") \
  X(MONTH_SHORT_JAN, "jan") \
  X(MONTH_SHORT_FEB, "feb") \
  X(MONTH_SHORT_MAR, "mar") \
  X(MONTH_SHORT_APR, "apr") \
  X(MONTH_SHORT_MAY, "mai") \
  X(MONTH_SHORT_JUN, "jun") \
  X(MONTH_SHORT_JUL, "jul") \
  X(MONTH_SHORT_AUG, "aug") \
  X(MONTH_SHORT_SEP, "sep") \
  X(MONTH_SHORT_OCT, "okt") \
  X(MONTH_SHORT_NOV, "nov") \
  X(MONTH_SHORT_DEC, "des") \
  X(MONTH_TAG_MAY, "MAI") \
  X(MONTH_TAG_OCT, "OKT") \
  X(MONTH_TAG_DEC, "DES") \
  X(DX_NOW, "nå") \
  X(DX_FMT_MIN_AGO, "%u min siden") \
  X(DX_FMT_H_AGO, "%u t siden") \
  X(DX_FMT_D_AGO, "%u d siden") \
  X(TOUCH_CAL_TITLE, "Kalibrer berøring") \
  X(TOUCH_CAL_HOLD, "Hold på sirkelen til den fylles") \
  X(TOUCH_CAL_CANCEL_HINT, "Vri eller trykk knotten: avbryt") \
  X(TOUCH_CAL_CHECK, "Kontroll") \
  X(TOUCH_CAL_TAP_DOT, "Trykk på prikken") \
  X(TOUCH_CAL_SAVED, "Kalibrering lagret") \
  X(TOUCH_CAL_FMT_LANDED, "Kontrollen traff %u px unna") \
  X(TOUCH_CAL_BACK_HINT, "Trykk knotten: tilbake") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Ikke lagret") \
  X(TOUCH_CAL_FMT_MISSED, "Kontrollen traff %u px unna,") \
  X(TOUCH_CAL_NO_FIT, "Punktene ga ingen kalibrering,") \
  X(TOUCH_CAL_NOT_SAVED, "Lagring feilet,") \
  X(TOUCH_CAL_OLD_STAYS, "så den gamle beholdes") \
  X(TOUCH_CAL_RETRY_HINT, "Trykk knotten: prøv igjen, vri: gå ut")
// clang-format on
