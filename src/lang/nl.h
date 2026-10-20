/*
 * The panel's texts in Dutch. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_NL(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "FM-zenderscan") \
  X(MENU_SCAN_MW, "AM-zenderscan") \
  X(MENU_SCAN_SW, "SW-zenderscan") \
  X(MENU_SCAN_LW, "LW-zenderscan") \
  X(MENU_SQUELCH, "Squelchmodus") \
  X(MENU_SQUELCH_LEVEL, "Squelchniveau") \
  X(MENU_SQUELCH_FLOOR, "Squelchvloer") \
  X(MENU_VOLUME_AGC, "Volume-AGC") \
  X(MENU_AGC_BOOST, "AGC-boost") \
  X(MENU_MUTE_RAMP, "Demptijd") \
  X(MENU_BAND_PLAN, "Bandplan") \
  X(MENU_TUNING_STEP, "Afstemstap") \
  X(MENU_SEEK_SENSITIVITY, "Zoekgevoeligheid") \
  X(MENU_FORCED_MONO, "Altijd mono") \
  X(MENU_STEREO_BLEND, "Stereomenging") \
  X(MENU_ST_HI_BLEND, "Stereo / hoogmenging") \
  X(MENU_HIGH_CUT, "Hoogafval") \
  X(MENU_NOISE_BLANKER, "Ruisfilter") \
  X(COMMON_RDS_DECODER, "RDS-decoder") \
  X(MENU_RDS_REGION, "RDS-regio") \
  X(MENU_MW_SPACING, "MW-stap") \
  X(MENU_FILTER_WIDTH, "Filterbandbreedte") \
  X(MENU_SOFT_MUTE, "Soft mute") \
  X(MENU_DWELL, "Wachttijd") \
  X(MENU_STOP_ON, "Stoppen bij") \
  X(MENU_SCAN, "Scanbereik") \
  X(MENU_MEMORY_FROM, "Eerste") \
  X(MENU_MEMORY_TO, "Laatste") \
  X(MENU_PRESET_RANGE, "Geheugenbereik") \
  X(MENU_LOOP_THE_BAND, "Band herhalen") \
  X(MENU_DX_WIDTH, "Scanbandbreedte") \
  X(MENU_MUTE_WHILE_SCANNING, "Dempen bij scan") \
  X(MENU_AUTO_LOG_NEW, "Automatisch loggen") \
  X(MENU_LOG_RADIO_TEXT, "Radiotekst loggen") \
  X(MENU_WATCH_PRESETS, "Geheugen bewaken") \
  X(MENU_SCOPE_BASELINE, "Scope-basislijn") \
  X(MENU_MEDIAN, "Mediaan") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Lokale zenders leren") \
  X(MENU_START_SCAN, "Scan starten") \
  X(MENU_DAY_THEME, "Dagthema") \
  X(MENU_NIGHT_THEME, "Nachtthema") \
  X(MENU_THEME, "Thema") \
  X(MENU_BRIGHTNESS, "Helderheid") \
  X(MENU_DIM_LEVEL, "Dimniveau") \
  X(MENU_DIM_AFTER, "Dimmen na") \
  X(MENU_ROTATION, "Schermrotatie") \
  X(COMMON_BATTERY, "Batterij") \
  X(MENU_BATTERY_CHECK, "Batterijcontrole") \
  X(MENU_LANGUAGE, "Taal \xC2\xB7 Language") \
  X(MENU_ENCODER, "Encodertype") \
  X(MENU_DIRECTION, "Encoderrichting") \
  X(MENU_KEY_BEEPS, "Toetsgeluid") \
  X(MENU_BAND_EDGE_BEEP, "Piep op bandeinde") \
  X(MENU_START_CHIME, "Startgeluid") \
  X(MENU_KEYPAD_TIMEOUT, "Time-out toetsen") \
  X(MENU_CALIBRATE_TOUCH, "Touch kalibreren") \
  X(MENU_CLOCK_FROM_NETWORK, "Netwerktijd") \
  X(MENU_WEB_PIN, "Web-PIN") \
  X(MENU_NEW_PIN, "Nieuwe PIN") \
  X(MENU_FMT_DIGIT_OF, "Cijfer %u van %u") \
  X(MENU_STATUS, "Verbinding") \
  X(MENU_WEB_SERVER, "Webserver") \
  X(MENU_FMT_PC_CONNECTED, "%u verbonden") \
  X(MENU_WEB_ADDRESS, "Webadres") \
  X(MENU_IP_ADDRESS, "IP-adres") \
  X(MENU_WI_FI_NAME, "Wi-Fi-netwerk") \
  X(MENU_HOTSPOT_NAME, "Hotspotnaam") \
  X(MENU_MAC, "MAC-adres") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi-signaal") \
  X(MENU_NETWORK_INFO, "Netwerkinfo") \
  X(MENU_RESTART, "Radio herstarten") \
  X(MENU_AUTO_OFF, "Auto uit") \
  X(MENU_UPDATE_CHECK, "Updatecontrole") \
  X(MENU_FIRMWARE_UPDATE, "Firmware-update") \
  X(MENU_FMT_UPDATE_TO, "Update naar %s") \
  X(MENU_UPDATE_TITLE, "Update beschikbaar") \
  X(MENU_UPDATE_THIS_RADIO, "Deze radio") \
  X(MENU_UPDATE_NEW_VERSION, "Nieuwe versie") \
  X(COMMON_SETTINGS, "Instellingen") \
  X(COMMON_KEPT, "Blijven") \
  X(MENU_STATION, "Zender") \
  X(MENU_UPDATE_NOW, "Updaten") \
  X(MENU_UPDATE_UP_TO_DATE, "Actueel") \
  X(MENU_UPDATE_CHECKING, "Controleren") \
  X(MENU_UPDATE_NOT_CHECKED, "Niet gecontroleerd") \
  X(MENU_UPDATE_CHECK_FAILED, "Controle mislukt") \
  X(MENU_NOTE_NO_UPDATE, "Geen update") \
  X(MENU_VERSION, "Firmwareversie") \
  X(MENU_RUNNING_FROM, "Opstartbron") \
  X(MENU_UPTIME, "Looptijd") \
  X(MENU_BATTERY_VOLTAGE, "Batterijspanning") \
  X(MENU_RESET_REASON, "Resetoorzaak") \
  X(MENU_CPU_CORE_0, "CPU-kern 0") \
  X(MENU_CPU_CORE_1, "CPU-kern 1") \
  X(MENU_FREE_HEAP, "Vrije heap") \
  X(MENU_LOWEST_HEAP, "Laagste heap") \
  X(MENU_LARGEST_BLOCK, "Grootste blok") \
  X(MENU_LVGL_POOL, "LVGL-pool") \
  X(MENU_FLASH_SIZE, "Flashgrootte") \
  X(RESET_POWER, "Inschakelen") \
  X(RESET_PANIC, "Paniek") \
  X(RESET_TASK_WATCHDOG, "Taak-watchdog") \
  X(RESET_DEEP_SLEEP, "Diepe slaap") \
  X(RESET_BROWNOUT, "Spanningsdip") \
  X(RESET_PIN, "Resetpin") \
  X(RESET_UNKNOWN, "Onbekend") \
  X(RESET_ASKED, "Herstart gevraagd") \
  X(RESET_BOOT_WATCHDOG, "Opstart-watchdog") \
  X(RESET_ROLLBACK, "Teruggezet") \
  X(RESET_DISPLAY, "Schermfout") \
  X(MENU_DEVELOPER, "Ontwikkelaar") \
  X(MENU_LICENSE, "Licentie") \
  X(ABOUT_BUILD_UNKNOWN, "onbekend") \
  X(MENU_GO_TO, "Ga naar") \
  X(MENU_GO_TO_BANDWIDTH, "Bandbreedte") \
  X(MENU_GO_TO_DX, "DX-modus") \
  X(MENU_GO_TO_SCOPE, "Bandscope") \
  X(MENU_GO_TO_BAND, "Volgende band") \
  X(MENU_GO_TO_LOG, "Zender loggen") \
  X(MENU_GO_TO_SAVE, "Zender bewaren") \
  X(MENU_FMT_SAVED_AS, "Bewaard: P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Al op P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Geheugen is vol") \
  X(MENU_GO_TO_SLEEP, "Slapen") \
  X(MENU_STATIONS, "Zenders") \
  X(MENU_MEMORY, "Geheugen") \
  X(MENU_FM_SETUP, "FM-ontvangst") \
  X(MENU_AM_SETUP, "AM-ontvangst") \
  X(MENU_DX_SETUP, "DX-scanner") \
  X(MENU_DISPLAY, "Scherm") \
  X(MENU_CONTROLS, "Bediening") \
  X(MENU_CONNECTIVITY, "Verbindingen") \
  X(MENU_SYSTEM, "Systeem") \
  X(MENU_DIAGNOSTICS, "Diagnose") \
  X(MENU_ABOUT, "Over") \
  X(MENU_REGION_FULL, "Volledig") \
  X(MENU_REGION_WIDE, "Breed") \
  X(MENU_REGION_WORLD, "Wereldwijd") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Noord-Amerika") \
  X(MENU_WIFI_JOINED, "Verbonden") \
  X(MENU_WIFI_JOINING, "Verbinden") \
  X(COMMON_OFF, "Uit") \
  X(MENU_FMT_KB_OF_KB, "%u van %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V bij start") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, %u min oud") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, %u h oud") \
  X(MENU_FMT_AGC_GAIN_NOW, "Nu %+d dB") \
  X(MENU_STATION_LOG, "Logboek") \
  X(COMMON_NONE, "Geen") \
  X(MENU_FMT_SCAN_SAVED, "%u bewaard") \
  X(MENU_FMT_SCAN_STOPPED, "Gestopt, %u bewaard") \
  X(MENU_FMT_SCAN_NO_ROOM, "Vol, %u niet bewaard") \
  X(COMMON_YES, "Ja") \
  X(COMMON_NO, "Nee") \
  X(MENU_UNKNOWN_VALUE, "Onbekend") \
  X(COMMON_ROTATION_NORMAL, "Normaal") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Ondersteboven") \
  X(MENU_CUT_ONLY, "Geen boost") \
  X(MENU_NEVER, "Nooit") \
  X(MENU_BATTERY_PER_CENT, "Procent") \
  X(MENU_BATTERY_VOLTS, "Volt") \
  X(MENU_KEYS, "Alleen cijfers") \
  X(MENU_KEYS_LONG, "Kort & lang drukken") \
  X(MENU_EVERY_PRESS, "Elke druk") \
  X(MENU_STANDARD, "Standaard") \
  X(MENU_OPTICAL, "Optisch") \
  X(MENU_NORMAL, "Normaal") \
  X(MENU_REVERSED, "Omgekeerd") \
  X(MENU_SQUELCH_MAN, "Handmatig") \
  X(MENU_ANY_PI, "Elke PI") \
  X(MENU_STOP_NEVER, "Nooit") \
  X(MENU_NEW_ONLY, "Nieuwe zenders") \
  X(MENU_WHOLE_BAND, "Hele band") \
  X(MENU_MEMORY_ONLY, "Alleen geheugen") \
  X(MENU_BAND_MEMORY, "Band + geheugen") \
  X(COMMON_ON, "Aan") \
  X(MENU_HINT_RESTART, "Actief na herstart") \
  X(COMMON_FM_ONLY, "Alleen FM") \
  X(MENU_NOTE_OFF_PLAN, "Niet in het bandplan") \
  X(MENU_NOTE_SWITCH_TO_FM, "Eerst naar FM") \
  X(MENU_NOTE_RADIO_BUSY, "Radio bezig - opnieuw") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Zet eerst RDS aan") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Update op proef - wacht") \
  X(MENU_NOTE_NOT_STORED, "Niet bewaard - hersteld") \
  X(MENU_NOTE_NOT_PUT_BACK, "Radio bezig - niet hersteld") \
  X(MENU_NOTE_NOT_SAVED, "Niet bewaard") \
  X(MENU_SAVE, "Bewaar") \
  X(RADIO_PRESS_TO_START, "Druk voor start") \
  X(COMMON_STILL_TUNING, "Afstemmen - wacht") \
  X(COMMON_SCAN_RUNNING, "Scan bezig - wacht") \
  X(RADIO_FMT_NO_BAND, "%s: geen band") \
  X(COMMON_NOT_LOGGED, "Niet gelogd") \
  X(RADIO_FMT_LOGGED, "%s gelogd") \
  X(RADIO_CHECKING_UPDATES, "UPDATES CONTROLEREN\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Scannen %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Update mislukt") \
  X(RADIO_GOING_TO_SLEEP, "Gaat slapen") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Wekken: druk op de knop") \
  X(RADIO_OLD_IMAGE_KEPT, "Oude firmware blijft") \
  X(RDS_SPEECH, "Spraak") \
  X(RDS_MUSIC, "Muziek") \
  X(COMMON_ALREADY_LOGGED, "Al gelogd") \
  X(DX_RDS_IS_OFF, "RDS is uit") \
  X(DX_SCANNING, "Scannen") \
  X(DX_SCAN_FINISHED, "Scan klaar") \
  X(DX_NOTHING_TO_SCAN, "Niets te scannen") \
  X(DX_CHECKING_FOR_UPDATES, "Updates controleren") \
  X(DX_A_SCAN_IS_RUNNING, "Scan loopt al") \
  X(DX_CAUGHT_LIST_UNREAD, "Vangstlijst onleesbaar") \
  X(DX_NO_MEMORY, "Geheugen leeg") \
  X(DX_NOT_STARTED, "Niet gestart") \
  X(DX_SWEEPING, "Sweepen") \
  X(COMMON_FMT_TUNE_TYPED, "Naar %s") \
  X(RADIO_SQL_OFF, "UIT") \
  X(DX_FMT_ROWS_OF, "%u-%u van %u") \
  X(DX_FMT_FOUND, "%u gevonden") \
  X(DX_MODE_SWEEPING, "SWEEPEN") \
  X(DX_MODE_LEARN_LOCALS, "LOKALE LEREN") \
  X(DX_FMT_MODE_MEMORY, "GEHEUGEN %u-%u") \
  X(DX_MODE_WHOLE_BAND, "HELE BAND") \
  X(DX_MODE_BAND_MEMORY, "BAND + GEHEUGEN") \
  X(DX_RULE_NO_STOP, "GEEN STOP") \
  X(DX_RULE_STOP_ON_PI, "STOP BIJ PI") \
  X(DX_RULE_STOP_ON_NEW, "STOP BIJ NIEUW") \
  X(DX_FMT_NOT_PRESET, "niet P%02d") \
  X(DX_CONTEXT_SWEEPING, "sweepen") \
  X(DX_PRESS_TO_SWEEP, "Druk voor sweep") \
  X(SCOPE_FMT_TITLE, "%s-scope") \
  X(SCOPE_FULL, "Alles") \
  X(DX_FIXED, "VAST") \
  X(DX_FMT_MEDIAN_OF, "MEDIAAN VAN %u") \
  X(DX_FMT_FLOOR, "VLOER %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Sluiten") \
  X(RECOVERY_PRESS_AGAIN, "Nog eens") \
  X(PAGE_TUNE, "Afstemmen") \
  X(PAGE_DELETE, "Wissen") \
  X(PAGE_LEVEL, "Niveau") \
  X(PAGE_NOISE, "Ruis") \
  X(PAGE_COCHANNEL, "Co-kanaal") \
  X(PAGE_WIDTH, "Breedte") \
  X(PAGE_NOTE_CHANGED, "Intussen gewijzigd") \
  X(PAGE_NOTE_NOT_DELETED, "Niet gewist") \
  X(PAGE_FMT_SAVED, "Bewaard %s") \
  X(PAGE_TIME_UNKNOWN, "Tijd onbekend") \
  X(BW_TITLE, "Bandbreedte") \
  X(KEYPAD_TITLE, "Frequentie") \
  X(KEYPAD_CANCEL, "Annuleren") \
  X(DX_CATCHES_EMPTY_TITLE, "Nog geen vangsten") \
  X(DX_CATCHES_EMPTY_NOTE, "Bevestigde zenders van deze sessie staan hier") \
  X(DX_TITLE_CATCHES, "Vangsten") \
  X(DX_NEW, "NIEUW") \
  X(DX_READY, "Klaar") \
  X(DX_DWELL, "WACHT") \
  X(DX_STOPPED, "GESTOPT") \
  X(DX_LEVEL, "NIVEAU") \
  X(DX_RISE, "BOVEN") \
  X(DX_NO_STATION_YET, "Nog geen zender") \
  X(DX_NO_RDS_YET, "Nog geen RDS") \
  X(DX_NO_ID, "GEEN ID") \
  X(RDS_TITLE_TEXT, "Radiotekst") \
  X(RDS_TITLE_NETWORKS, "Netwerken") \
  X(RDS_NO_COUNTRY, "geen ECC") \
  X(RDS_RTPLUS_NONE, "Geen RT+ van deze zender") \
  X(RDS_RTPLUS_WAIT, "Nog geen RT+ voor deze tekst") \
  X(RDS_AF_TITLE, "ALTERNATIEVE FREQUENTIES") \
  X(RDS_EON_TITLE, "ANDERE ZENDERS, EON") \
  X(RDS_NONE_HEARD, "Niets gehoord") \
  X(RDS_GROUPS_S_CAPS, "GROEP/S") \
  X(RDS_LOST, "WEG") \
  X(RDS_EACH_BLOCK, "PER BLOK") \
  X(RDS_LEGEND_CLEAN, "schoon") \
  X(RDS_LEGEND_FIXED, "hersteld") \
  X(RDS_LEGEND_LOST, "weg") \
  X(RDS_ALL_CLEAN, "alles schoon") \
  X(RDS_GROUP_TYPES, "GROEPTYPES") \
  X(RDS_NO_GROUPS, "Geen groepen in laatste minuut") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 %u s") \
  X(RDS_COUNTRY_UNLISTED, "niet in lijst") \
  X(RDS_NO_TYPES, "Geen schoon blok B in laatste minuut") \
  X(RDS_FMT_EON_MORE, "+%u meer") \
  X(RDS_EON_MORE_HEARD, "+ meer") \
  X(RDS_MORE, "meer") \
  X(RADIO_VEIL_UPDATING, "FIRMWARE BIJWERKEN") \
  X(RADIO_VEIL_KEEP_POWER, "Stroom niet onderbreken") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Geen tuner op I2C-bus") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tuner werd niet gereed") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Tuner-identificatie mislukt") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Geen patch voor deze tuner") \
  X(TUNER_UNKNOWN_TUNER_PART, "Onbekende tuner") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Schrijven geweigerd") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Geen antwoord bij lezen") \
  X(TUNER_OUT_OF_RANGE, "Buiten bereik") \
  X(TUNER_UNKNOWN, "Onbekend") \
  X(BAND_LONG_LW, "Langegolf") \
  X(BAND_LONG_MW, "Middengolf") \
  X(BAND_LONG_SW, "Kortegolf") \
  X(MENU_FMT_TUNE_TO, "Band voor %s") \
  X(PTY_NONE, "Geen") \
  X(PTY_NEWS, "Nieuws") \
  X(PTY_CURRENT_AFFAIRS, "Actualiteit") \
  X(PTY_INFORMATION, "Informatie") \
  X(PTY_EDUCATION, "Educatie") \
  X(PTY_CULTURE, "Cultuur") \
  X(PTY_SCIENCE, "Wetenschap") \
  X(PTY_VARIED, "Varia") \
  X(PTY_POP_MUSIC, "Popmuziek") \
  X(PTY_ROCK_MUSIC, "Rockmuziek") \
  X(PTY_EASY_LISTENING, "Melodie") \
  X(PTY_LIGHT_CLASSICAL, "Licht klassiek") \
  X(PTY_SERIOUS_CLASSICAL, "Klassiek") \
  X(PTY_OTHER_MUSIC, "Overige muziek") \
  X(PTY_WEATHER, "Weer") \
  X(PTY_FINANCE, "Economie") \
  X(PTY_CHILDREN, "Kinderen") \
  X(PTY_SOCIAL_AFFAIRS, "Maatschappij") \
  X(PTY_RELIGION, "Religie") \
  X(PTY_PHONE_IN, "Doe mee") \
  X(PTY_TRAVEL, "Reizen") \
  X(PTY_LEISURE, "Vrije tijd") \
  X(PTY_COUNTRY_MUSIC, "Countrymuziek") \
  X(PTY_NATIONAL_MUSIC, "Nationale muziek") \
  X(PTY_OLDIES_MUSIC, "Gouwe ouwe") \
  X(PTY_FOLK_MUSIC, "Volksmuziek") \
  X(PTY_DOCUMENTARY, "Documentaire") \
  X(PTY_ALARM_TEST, "Alarmtest") \
  X(PTY_NA_NEWS, "Nieuws") \
  X(PTY_NA_INFORMATION, "Informatie") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_CLASSIC_ROCK, "Classic rock") \
  X(PTY_NA_ADULT_HITS, "Adult hits") \
  X(PTY_NA_SOFT_ROCK, "Soft rock") \
  X(PTY_NA_OLDIES, "Gouwe ouwe") \
  X(PTY_NA_NOSTALGIA, "Nostalgie") \
  X(PTY_NA_CLASSICAL, "Klassiek") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Rhythm-and-blues") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Soft rhythm-and-blues") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Vreemde taal") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Religieuze muziek") \
  X(PTY_NA_RELIGIOUS_TALK, "Religieuze talk") \
  X(PTY_NA_PERSONALITY, "Persoonlijkheid") \
  X(PTY_NA_PUBLIC, "Publiek") \
  X(PTY_NA_SPANISH_TALK, "Spaanse talk") \
  X(PTY_NA_SPANISH_MUSIC, "Spaanse muziek") \
  X(PTY_NA_HIP_HOP, "Hiphop") \
  X(PTY_NA_UNASSIGNED, "Ongebruikt") \
  X(PTY_NA_WEATHER, "Weer") \
  X(PTY_NA_EMERGENCY_TEST, "Noodtest") \
  X(PTY_NA_EMERGENCY, "Noodgeval") \
  X(COVERAGE_LOCAL, "Lokaal") \
  X(COVERAGE_INTERNATIONAL, "Internationaal") \
  X(COVERAGE_NATIONAL, "Nationaal") \
  X(COVERAGE_SUPRA_REGIONAL, "Bovenregionaal") \
  X(COVERAGE_REGIONAL_1, "Regionaal 1") \
  X(COVERAGE_REGIONAL_2, "Regionaal 2") \
  X(COVERAGE_REGIONAL_3, "Regionaal 3") \
  X(COVERAGE_REGIONAL_4, "Regionaal 4") \
  X(COVERAGE_REGIONAL_5, "Regionaal 5") \
  X(COVERAGE_REGIONAL_6, "Regionaal 6") \
  X(COVERAGE_REGIONAL_7, "Regionaal 7") \
  X(COVERAGE_REGIONAL_8, "Regionaal 8") \
  X(COVERAGE_REGIONAL_9, "Regionaal 9") \
  X(COVERAGE_REGIONAL_10, "Regionaal 10") \
  X(COVERAGE_REGIONAL_11, "Regionaal 11") \
  X(COVERAGE_REGIONAL_12, "Regionaal 12") \
  X(RTP_OTHER, "OVERIG") \
  X(RTP_TITLE, "TITEL") \
  X(RTP_TRACK, "NUMMER") \
  X(RTP_ARTIST, "ARTIEST") \
  X(RTP_WORK, "WERK") \
  X(RTP_MOVEMENT, "DEEL") \
  X(RTP_CONDUCTOR, "DIRIGENT") \
  X(RTP_COMPOSER, "COMPONIST") \
  X(RTP_COMMENT, "OPMERKING") \
  X(RTP_NEWS, "NIEUWS") \
  X(RTP_LOCAL, "LOKAAL NIEUWS") \
  X(RTP_STOCKS, "BEURS") \
  X(RTP_LOTTERY, "LOTERIJ") \
  X(RTP_HOROSCOPE, "HOROSCOOP") \
  X(RTP_DIVERSION, "VERMAAK") \
  X(RTP_HEALTH, "GEZONDHEID") \
  X(RTP_EVENT, "EVENEMENT") \
  X(RTP_CINEMA, "BIOSCOOP") \
  X(RTP_FUN, "HUMOR") \
  X(RTP_DATE, "DATUM") \
  X(RTP_WEATHER, "WEER") \
  X(RTP_TRAFFIC, "VERKEER") \
  X(RTP_ADVERT, "RECLAME") \
  X(RTP_STATION, "ZENDER") \
  X(RTP_PROGRAM, "PROGRAMMA") \
  X(RTP_NEXT, "STRAKS") \
  X(RTP_PART, "GEDEELTE") \
  X(RTP_EDITORS, "REDACTIE") \
  X(RTP_FREQUENCY, "FREQUENTIE") \
  X(RTP_CHANNEL, "KANAAL") \
  X(RTP_PHONE, "TELEFOON") \
  X(RTP_EMAIL, "E-MAIL") \
  X(RTP_VOTE, "STEMMING") \
  X(RTP_PLACE, "PLAATS") \
  X(RTP_APPOINTMENT, "AFSPRAAK") \
  X(RTP_PURCHASE, "AANKOOP") \
  X(DAY_SUNDAY, "ZONDAG") \
  X(DAY_MONDAY, "MAANDAG") \
  X(DAY_TUESDAY, "DINSDAG") \
  X(DAY_WEDNESDAY, "WOENSDAG") \
  X(DAY_THURSDAY, "DONDERDAG") \
  X(DAY_FRIDAY, "VRIJDAG") \
  X(DAY_SATURDAY, "ZATERDAG") \
  X(MONTH_JANUARY, "januari") \
  X(MONTH_FEBRUARY, "februari") \
  X(MONTH_MARCH, "maart") \
  X(MONTH_APRIL, "april") \
  X(MONTH_MAY, "mei") \
  X(MONTH_JUNE, "juni") \
  X(MONTH_JULY, "juli") \
  X(MONTH_AUGUST, "augustus") \
  X(MONTH_SEPTEMBER, "september") \
  X(MONTH_OCTOBER, "oktober") \
  X(MONTH_NOVEMBER, "november") \
  X(MONTH_DECEMBER, "december") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_FMT_LINE, "%s %d%s %s %d") \
  X(DATE_TODAY, "Vandaag") \
  X(DATE_YESTERDAY, "Gisteren") \
  X(DATE_YDAY, "GISTER") \
  X(MONTH_SHORT_JAN, "jan") \
  X(MONTH_SHORT_FEB, "feb") \
  X(MONTH_SHORT_MAR, "mrt") \
  X(MONTH_SHORT_APR, "apr") \
  X(MONTH_SHORT_MAY, "mei") \
  X(MONTH_SHORT_JUN, "jun") \
  X(MONTH_SHORT_JUL, "jul") \
  X(MONTH_SHORT_AUG, "aug") \
  X(MONTH_SHORT_SEP, "sep") \
  X(MONTH_SHORT_OCT, "okt") \
  X(MONTH_SHORT_NOV, "nov") \
  X(MONTH_SHORT_DEC, "dec") \
  X(MONTH_TAG_MAR, "MRT") \
  X(MONTH_TAG_MAY, "MEI") \
  X(MONTH_TAG_OCT, "OKT") \
  X(DX_NOW, "nu") \
  X(DX_FMT_MIN_AGO, "%u min oud") \
  X(DX_FMT_H_AGO, "%u h oud") \
  X(DX_FMT_D_AGO, "%u d oud") \
  X(TOUCH_CAL_TITLE, "Touch kalibreren") \
  X(TOUCH_CAL_HOLD, "Houd de cirkel vast tot hij vol is") \
  X(TOUCH_CAL_CANCEL_HINT, "Draai of druk de knop: annuleren") \
  X(TOUCH_CAL_CHECK, "Controle") \
  X(TOUCH_CAL_TAP_DOT, "Tik op de stip") \
  X(TOUCH_CAL_SAVED, "Kalibratie bewaard") \
  X(TOUCH_CAL_FMT_LANDED, "De controle lag %u px ernaast") \
  X(TOUCH_CAL_BACK_HINT, "Druk op de knop: terug") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Niet bewaard") \
  X(TOUCH_CAL_FMT_MISSED, "De controle lag %u px ernaast,") \
  X(TOUCH_CAL_NO_FIT, "De punten gaven geen kalibratie,") \
  X(TOUCH_CAL_NOT_SAVED, "Bewaren is mislukt,") \
  X(TOUCH_CAL_OLD_STAYS, "dus de oude kalibratie blijft") \
  X(TOUCH_CAL_RETRY_HINT, "Druk op de knop: opnieuw, draai: verlaten")
// clang-format on
