/*
 * The panel's texts in German. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_DE(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "FM-Senderscan") \
  X(MENU_SCAN_MW, "AM-Senderscan") \
  X(MENU_SCAN_SW, "SW-Senderscan") \
  X(MENU_SCAN_LW, "LW-Senderscan") \
  X(MENU_SQUELCH, "Sperrmodus") \
  X(MENU_SQUELCH_LEVEL, "Sperrpegel") \
  X(MENU_SQUELCH_FLOOR, "Untergrenze") \
  X(MENU_SQUELCH_GROUP, "Rauschsperre") \
  X(MENU_VOLUME_AGC, "Lautstärke-AGC") \
  X(MENU_AGC_BOOST, "AGC-Anhebung") \
  X(MENU_MUTE_RAMP, "Stummrampe") \
  X(MENU_BAND_PLAN, "Bandplan") \
  X(MENU_TUNING_STEP, "Schrittweite") \
  X(MENU_SEEK_SENSITIVITY, "Suchschwelle") \
  X(MENU_DE_EMPHASIS, "De-Emphasis") \
  X(MENU_FORCED_MONO, "Immer Mono") \
  X(MENU_STEREO_BLEND, "Stereo-Blend") \
  X(MENU_ST_HI_BLEND, "Stereo/Höhen-Blend") \
  X(MENU_HIGH_CUT, "Tiefpass") \
  X(MENU_NOISE_BLANKER, "Störaustaster") \
  X(COMMON_RDS_DECODER, "RDS-Decoder") \
  X(MENU_RDS_REGION, "RDS-Region") \
  X(MENU_MW_SPACING, "MW-Raster") \
  X(MENU_FILTER_WIDTH, "Filterbandbreite") \
  X(MENU_SOFT_MUTE, "Soft-Mute") \
  X(MENU_DWELL, "Verweilzeit") \
  X(MENU_STOP_ON, "Stopp bei") \
  X(MENU_SCAN, "Scanbereich") \
  X(MENU_MEMORY_FROM, "Speicher von") \
  X(MENU_MEMORY_TO, "Speicher bis") \
  X(MENU_PRESET_RANGE, "Speicherbereich") \
  X(MENU_LOOP_THE_BAND, "Endlos-Scan") \
  X(MENU_DX_WIDTH, "Scan-Bandbreite") \
  X(MENU_MUTE_WHILE_SCANNING, "Stumm beim Scan") \
  X(MENU_AUTO_LOG_NEW, "Auto-Logbuch") \
  X(MENU_LOG_RADIO_TEXT, "Radiotext loggen") \
  X(MENU_WATCH_PRESETS, "Speicher überw.") \
  X(MENU_SCOPE_BASELINE, "Scope-Referenz") \
  X(MENU_FMT_SWEEP_N, "Durchlauf %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Ortssender lernen") \
  X(MENU_START_SCAN, "Scan starten") \
  X(MENU_DAY_THEME, "Tagesdesign") \
  X(MENU_NIGHT_THEME, "Nachtdesign") \
  X(MENU_THEME, "Design") \
  X(MENU_BRIGHTNESS, "Helligkeit") \
  X(MENU_DIM_LEVEL, "Dimmstufe") \
  X(MENU_DIM_AFTER, "Dimmzeit") \
  X(MENU_ROTATION, "Anzeige drehen") \
  X(COMMON_BATTERY, "Akku") \
  X(MENU_BATTERY_CHECK, "Akkuprüfung") \
  X(MENU_LANGUAGE, "Sprache \xC2\xB7 Language") \
  X(MENU_ENCODER, "Drehgeber-Typ") \
  X(MENU_DIRECTION, "Drehrichtung") \
  X(MENU_ENCODER_GROUP, "Drehgeber") \
  X(MENU_KEY_BEEPS, "Tastentöne") \
  X(MENU_BAND_EDGE_BEEP, "Bandende-Ton") \
  X(MENU_START_CHIME, "Startton") \
  X(MENU_KEYPAD_TIMEOUT, "Eingabezeit") \
  X(MENU_CALIBRATE_TOUCH, "Touch kalibrieren") \
  X(MENU_CLOCK_FROM_NETWORK, "Netzwerkzeit") \
  X(MENU_WEB_PIN, "Web-PIN") \
  X(MENU_NEW_PIN, "Neue PIN") \
  X(MENU_FMT_DIGIT_OF, "Ziffer %u/%u") \
  X(MENU_STATUS, "Verbindungsstatus") \
  X(MENU_WEB_SERVER, "Webserver") \
  X(MENU_FMT_PC_CONNECTED, "%u verbunden") \
  X(MENU_WEB_ADDRESS, "Webadresse") \
  X(MENU_IP_ADDRESS, "IP-Adresse") \
  X(MENU_WI_FI_NAME, "Wi-Fi-Netz") \
  X(MENU_HOTSPOT_NAME, "Hotspot-Name") \
  X(MENU_MAC, "MAC-Adresse") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi-Signal") \
  X(MENU_NETWORK_INFO, "Netzwerk-Info") \
  X(MENU_RESTART, "Neustart") \
  X(MENU_AUTO_OFF, "Auto-Aus") \
  X(MENU_UPDATE_CHECK, "Update-Suche") \
  X(MENU_FIRMWARE_UPDATE, "Firmware-Update") \
  X(MENU_FMT_UPDATE_TO, "Update auf %s") \
  X(MENU_UPDATE_TITLE, "Update verfügbar") \
  X(MENU_UPDATE_THIS_RADIO, "Installiert") \
  X(MENU_UPDATE_NEW_VERSION, "Neue Version") \
  X(COMMON_SETTINGS, "Einst.") \
  X(COMMON_KEPT, "Bleiben") \
  X(MENU_STATION, "Sender") \
  X(MENU_LATER, "Später") \
  X(MENU_UPDATE_UP_TO_DATE, "Aktuell") \
  X(MENU_UPDATE_CHECKING, "Prüfe") \
  X(MENU_UPDATE_NOT_CHECKED, "Nicht geprüft") \
  X(MENU_UPDATE_CHECK_FAILED, "Prüffehler") \
  X(MENU_NOTE_NO_UPDATE, "Kein Update") \
  X(MENU_VERSION, "Firmware-Version") \
  X(MENU_RUNNING_FROM, "Bootquelle") \
  X(MENU_UPTIME, "Laufzeit") \
  X(MENU_BATTERY_VOLTAGE, "Akkuspannung") \
  X(MENU_RESET_REASON, "Reset-Grund") \
  X(MENU_CPU_CORE_0, "CPU-Kern 0") \
  X(MENU_CPU_CORE_1, "CPU-Kern 1") \
  X(MENU_FREE_HEAP, "Heap frei") \
  X(MENU_LOWEST_HEAP, "Min. Heap") \
  X(MENU_LARGEST_BLOCK, "Größter Block") \
  X(MENU_LVGL_POOL, "LVGL-Pool") \
  X(MENU_FLASH_SIZE, "Flash-Größe") \
  X(RESET_POWER, "Einschalten") \
  X(RESET_PANIC, "Absturz") \
  X(RESET_INT_WATCHDOG, "IRQ-Watchdog") \
  X(RESET_TASK_WATCHDOG, "Task-Watchdog") \
  X(RESET_DEEP_SLEEP, "Tiefschlaf") \
  X(RESET_BROWNOUT, "Unterspannung") \
  X(RESET_PIN, "Reset-Pin") \
  X(RESET_UNKNOWN, "Unbekannt") \
  X(RESET_ASKED, "Auf Wunsch") \
  X(RESET_BOOT_WATCHDOG, "Boot-Watchdog") \
  X(RESET_DISPLAY, "Anzeigefehler") \
  X(MENU_DEVELOPER, "Entwickler") \
  X(MENU_LICENSE, "Lizenz") \
  X(ABOUT_BUILD_UNKNOWN, "unbekannt") \
  X(MENU_GO_TO, "Gehe zu") \
  X(MENU_GO_TO_BANDWIDTH, "Bandbreite") \
  X(MENU_GO_TO_DX, "DX-Modus") \
  X(MENU_GO_TO_SCOPE, "Bandscope") \
  X(MENU_GO_TO_BAND, "Band weiter") \
  X(MENU_GO_TO_LOG, "Sender loggen") \
  X(MENU_GO_TO_SAVE, "Sender speichern") \
  X(MENU_FMT_SAVED_AS, "Gespeichert: P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Schon P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Speicher voll") \
  X(MENU_GO_TO_SLEEP, "Standby") \
  X(MENU_STATIONS, "Sender") \
  X(MENU_MEMORY, "Speicher") \
  X(MENU_FM_SETUP, "FM-Empfang") \
  X(MENU_AM_SETUP, "AM-Empfang") \
  X(MENU_DX_SETUP, "DX-Scanner") \
  X(MENU_DISPLAY, "Anzeige") \
  X(MENU_CONTROLS, "Bedienung") \
  X(MENU_CONNECTIVITY, "Netzwerk") \
  X(MENU_DIAGNOSTICS, "Diagnose") \
  X(MENU_ABOUT, "Info") \
  X(MENU_REGION_FULL, "Voll") \
  X(MENU_REGION_WIDE, "Erweitert") \
  X(MENU_REGION_WORLD, "Weltweit") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Nordamerika") \
  X(MENU_WIFI_JOINED, "Verbunden") \
  X(MENU_WIFI_JOINING, "Verbinde") \
  X(COMMON_OFF, "Aus") \
  X(MENU_FMT_KB_OF_KB, "%u/%u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V (Start)") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, vor %u min") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, vor %u h") \
  X(MENU_FMT_AGC_GAIN_NOW, "Aktuell %+d dB") \
  X(MENU_STATION_LOG, "Logbuch") \
  X(COMMON_NONE, "Fehlt") \
  X(MENU_FMT_SCAN_SAVED, "%u gesp.") \
  X(MENU_FMT_SCAN_STOPPED, "Gestoppt, %u gesp.") \
  X(MENU_FMT_SCAN_NO_ROOM, "Voll, %u nicht gesp.") \
  X(COMMON_YES, "Ja") \
  X(COMMON_NO, "Nein") \
  X(MENU_UNKNOWN_VALUE, "Unbekannt") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Kopfüber") \
  X(MENU_CUT_ONLY, "Nur senken") \
  X(MENU_NEVER, "Nie") \
  X(MENU_BATTERY_PER_CENT, "Prozent") \
  X(MENU_BATTERY_VOLTS, "Volt") \
  X(MENU_KEYS, "Nur Ziffern") \
  X(MENU_KEYS_LONG, "Kurz- & Langdruck") \
  X(MENU_EVERY_PRESS, "Jede Taste") \
  X(MENU_OPTICAL, "Optisch") \
  X(MENU_REVERSED, "Umgekehrt") \
  X(MENU_SQUELCH_MAN, "Manuell") \
  X(MENU_ANY_PI, "Jeder PI") \
  X(MENU_STOP_NEVER, "Nie") \
  X(MENU_NEW_ONLY, "Nur neue Sender") \
  X(MENU_WHOLE_BAND, "Ganzes Band") \
  X(MENU_MEMORY_ONLY, "Nur Speicher") \
  X(MENU_BAND_MEMORY, "Band + Speicher") \
  X(COMMON_ON, "Ein") \
  X(MENU_HINT_RESTART, "Gilt nach Neustart") \
  X(MENU_TITLE, "Menü") \
  X(COMMON_FM_ONLY, "Nur FM") \
  X(MENU_NOTE_OFF_PLAN, "Nicht im Bandplan") \
  X(MENU_NOTE_SWITCH_TO_FM, "Erst FM wählen") \
  X(MENU_NOTE_RADIO_BUSY, "Radio belegt - nochmal") \
  X(MENU_NOTE_SWITCH_RDS_ON, "RDS einschalten") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Update-Test - warten") \
  X(MENU_NOTE_NOT_STORED, "Nicht gesp. - alter Wert") \
  X(MENU_NOTE_NOT_PUT_BACK, "Radio belegt - nicht zurück") \
  X(MENU_NOTE_NOT_SAVED, "Nicht gesp.") \
  X(MENU_SAVE, "Speichern") \
  X(RADIO_PRESS_TO_START, "Zum Start drücken") \
  X(COMMON_STILL_TUNING, "Suchlauf - warten") \
  X(COMMON_SCAN_RUNNING, "Scan läuft - warten") \
  X(RADIO_FMT_NO_BAND, "%s: kein Band") \
  X(COMMON_NOT_LOGGED, "Nicht geloggt") \
  X(RADIO_FMT_LOGGED, "%s geloggt") \
  X(RADIO_CHECKING_UPDATES, "SUCHE NACH UPDATES\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Scan %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Update-Fehler") \
  X(RADIO_GOING_TO_SLEEP, "Standby") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Wecken: Knopf drücken") \
  X(RADIO_OLD_IMAGE_KEPT, "Alte Firmware bleibt") \
  X(RDS_SPEECH, "Sprache") \
  X(RDS_MUSIC, "Musik") \
  X(COMMON_ALREADY_LOGGED, "Schon geloggt") \
  X(DX_RDS_IS_OFF, "RDS aus") \
  X(DX_SCANNING, "Scan läuft") \
  X(DX_SCAN_FINISHED, "Scan beendet") \
  X(DX_NOTHING_TO_SCAN, "Nichts zu scannen") \
  X(DX_CHECKING_FOR_UPDATES, "Suche nach Updates") \
  X(DX_A_SCAN_IS_RUNNING, "Scan läuft schon") \
  X(DX_CAUGHT_LIST_UNREAD, "Trefferliste fehlt") \
  X(DX_NO_MEMORY, "Speicher leer") \
  X(DX_NOT_STARTED, "Nicht gestartet") \
  X(DX_SWEEPING, "Durchlauf") \
  X(COMMON_FMT_TUNE_TYPED, "Wahl %s") \
  X(RADIO_SQL_OFF, "AUS") \
  X(DX_FMT_ROWS_OF, "%u-%u von %u") \
  X(DX_FMT_FOUND, "%u gefunden") \
  X(DX_MODE_SWEEPING, "DURCHLAUF") \
  X(DX_MODE_LEARN_LOCALS, "ORTSSENDER") \
  X(DX_FMT_MODE_MEMORY, "SPEICHER %u-%u") \
  X(DX_MODE_WHOLE_BAND, "GANZES BAND") \
  X(DX_MODE_BAND_MEMORY, "BAND + SPEICHER") \
  X(DX_RULE_NO_STOP, "KEIN STOPP") \
  X(DX_RULE_STOP_ON_PI, "STOPP BEI PI") \
  X(DX_RULE_STOP_ON_NEW, "STOPP BEI NEU") \
  X(DX_FMT_NOT_PRESET, "nicht P%02d") \
  X(DX_CONTEXT_SWEEPING, "Durchlauf") \
  X(DX_PRESS_TO_SWEEP, "Drücken für Durchlauf") \
  X(SCOPE_FMT_TITLE, "%s-Scope") \
  X(SCOPE_FULL, "Voll") \
  X(DX_FIXED, "FEST") \
  X(DX_FMT_MEDIAN_OF, "MEDIAN AUS %u") \
  X(DX_FMT_FLOOR, "BODEN %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Zurück") \
  X(RECOVERY_PRESS_AGAIN, "Erneut drücken") \
  X(PAGE_TUNE, "Hören") \
  X(PAGE_DELETE, "Löschen") \
  X(PAGE_LEVEL, "Pegel") \
  X(PAGE_NOISE, "Rauschen") \
  X(PAGE_MULTIPATH, "Mehrweg") \
  X(PAGE_COCHANNEL, "Gleichkanal") \
  X(PAGE_WIDTH, "Bandbreite") \
  X(PAGE_NOTE_CHANGED, "Seit dem Öffnen geändert") \
  X(PAGE_NOTE_NOT_DELETED, "Nicht gelöscht") \
  X(PAGE_FMT_SAVED, "Gespeichert %s") \
  X(PAGE_TIME_UNKNOWN, "Zeit unbekannt") \
  X(MENU_FMT_TUNER_PATCH, "%s Patch %u") \
  X(BW_TITLE, "Bandbreite") \
  X(KEYPAD_TITLE, "Frequenz") \
  X(KEYPAD_CANCEL, "Abbruch") \
  X(DX_CATCHES_EMPTY_TITLE, "Noch keine Treffer") \
  X(DX_CATCHES_EMPTY_NOTE, "Bestätigte Sender dieser Sitzung erscheinen hier") \
  X(DX_TITLE_CATCHES, "Treffer") \
  X(DX_NEW, "NEU") \
  X(DX_READY, "Bereit") \
  X(DX_DWELL, "ZEIT") \
  X(DX_STOPPED, "GESTOPPT") \
  X(DX_LEVEL, "PEGEL") \
  X(DX_RISE, "DIFF") \
  X(DX_NO_STATION_YET, "Noch kein Sender") \
  X(DX_NO_RDS_YET, "Noch kein RDS") \
  X(DX_NO_ID, "KEINE ID") \
  X(RDS_TITLE_TEXT, "Radiotext") \
  X(RDS_TITLE_NETWORKS, "Netze") \
  X(RDS_NO_COUNTRY, "kein ECC") \
  X(RDS_RTPLUS_NONE, "Kein RT+ von diesem Sender") \
  X(RDS_RTPLUS_WAIT, "Noch keine RT+ Tags zum Text") \
  X(RDS_AF_TITLE, "ALTERNATIVFREQUENZEN") \
  X(RDS_EON_TITLE, "ANDERE SENDER, EON") \
  X(RDS_NONE_HEARD, "Keine") \
  X(RDS_GROUPS_S_CAPS, "GRUPPEN/S") \
  X(RDS_LOST, "VERLUST") \
  X(RDS_EACH_BLOCK, "JEDER BLOCK") \
  X(RDS_LEGEND_CLEAN, "sauber") \
  X(RDS_LEGEND_FIXED, "korr.") \
  X(RDS_LEGEND_LOST, "fehlt") \
  X(RDS_ALL_CLEAN, "alles sauber") \
  X(RDS_GROUP_TYPES, "GRUPPENTYPEN") \
  X(RDS_NO_GROUPS, "Seit 1 min keine Gruppen") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 seit %u s") \
  X(RDS_COUNTRY_UNLISTED, "unbekannt") \
  X(RDS_NO_TYPES, "Seit 1 min kein Block B fehlerfrei") \
  X(RDS_FMT_EON_MORE, "+%u mehr") \
  X(RDS_EON_MORE_HEARD, "+ mehr") \
  X(RDS_MORE, "mehr") \
  X(RADIO_VEIL_UPDATING, "FIRMWARE-UPDATE") \
  X(RADIO_VEIL_KEEP_POWER, "Strom nicht trennen") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Kein Tuner am I2C-Bus") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tuner wurde nicht bereit") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Tuner nicht identifiziert") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Kein Patch für diesen Tuner") \
  X(TUNER_UNKNOWN_TUNER_PART, "Tuner unbekannt") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tuner: Schreibfehler") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tuner: Lesefehler") \
  X(TUNER_OUT_OF_RANGE, "Außer Bereich") \
  X(TUNER_UNKNOWN, "Unbekannt") \
  X(BAND_LONG_LW, "Langwelle") \
  X(BAND_LONG_MW, "Mittelwelle") \
  X(BAND_LONG_SW, "Kurzwelle") \
  X(MENU_FMT_TUNE_TO, "Band für %s") \
  X(PTY_NONE, "Keine") \
  X(PTY_NEWS, "Nachrichten") \
  X(PTY_CURRENT_AFFAIRS, "Aktuelles") \
  X(PTY_EDUCATION, "Bildung") \
  X(PTY_DRAMA, "Hörspiel") \
  X(PTY_CULTURE, "Kultur") \
  X(PTY_SCIENCE, "Wissenschaft") \
  X(PTY_VARIED, "Unterhaltung") \
  X(PTY_POP_MUSIC, "Pop-Musik") \
  X(PTY_ROCK_MUSIC, "Rock-Musik") \
  X(PTY_EASY_LISTENING, "Unterhaltungsmusik") \
  X(PTY_LIGHT_CLASSICAL, "Leichte Klassik") \
  X(PTY_SERIOUS_CLASSICAL, "Klassik") \
  X(PTY_OTHER_MUSIC, "Andere Musik") \
  X(PTY_WEATHER, "Wetter") \
  X(PTY_FINANCE, "Wirtschaft") \
  X(PTY_CHILDREN, "Kinder") \
  X(PTY_SOCIAL_AFFAIRS, "Soziales") \
  X(PTY_PHONE_IN, "Anrufsendung") \
  X(PTY_TRAVEL, "Reise") \
  X(PTY_LEISURE, "Freizeit") \
  X(PTY_COUNTRY_MUSIC, "Country-Musik") \
  X(PTY_NATIONAL_MUSIC, "Landesmusik") \
  X(PTY_FOLK_MUSIC, "Folklore") \
  X(PTY_DOCUMENTARY, "Dokumentation") \
  X(PTY_ALARM_TEST, "Alarmtest") \
  X(PTY_NA_NEWS, "Nachrichten") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_NOSTALGIA, "Nostalgie") \
  X(PTY_NA_CLASSICAL, "Klassik") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Rhythm & Blues") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Soft Rhythm & Blues") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Fremdsprachig") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Religiöse Musik") \
  X(PTY_NA_RELIGIOUS_TALK, "Religiöser Talk") \
  X(PTY_NA_PERSONALITY, "Persönlichkeit") \
  X(PTY_NA_PUBLIC, "Öffentlich") \
  X(PTY_NA_SPANISH_TALK, "Talk Spanisch") \
  X(PTY_NA_SPANISH_MUSIC, "Spanische Musik") \
  X(PTY_NA_UNASSIGNED, "Nicht belegt") \
  X(PTY_NA_WEATHER, "Wetter") \
  X(PTY_NA_EMERGENCY_TEST, "Notfalltest") \
  X(PTY_NA_EMERGENCY, "Notfall") \
  X(COVERAGE_LOCAL, "Lokal") \
  X(COVERAGE_SUPRA_REGIONAL, "Überregional") \
  X(RTP_OTHER, "ANDERE") \
  X(RTP_TITLE, "TITEL") \
  X(RTP_TRACK, "STÜCK") \
  X(RTP_ARTIST, "KÜNSTLER") \
  X(RTP_WORK, "WERK") \
  X(RTP_MOVEMENT, "SATZ") \
  X(RTP_CONDUCTOR, "DIRIGENT") \
  X(RTP_COMPOSER, "KOMPONIST") \
  X(RTP_COMMENT, "KOMMENTAR") \
  X(RTP_NEWS, "NACHRICHTEN") \
  X(RTP_LOCAL, "LOKALES") \
  X(RTP_STOCKS, "BÖRSE") \
  X(RTP_LOTTERY, "LOTTO") \
  X(RTP_HOROSCOPE, "HOROSKOP") \
  X(RTP_DIVERSION, "TAGESTIPP") \
  X(RTP_HEALTH, "GESUNDHEIT") \
  X(RTP_EVENT, "EREIGNIS") \
  X(RTP_SCENE, "SZENE") \
  X(RTP_CINEMA, "KINO") \
  X(RTP_FUN, "SPASS") \
  X(RTP_DATE, "DATUM") \
  X(RTP_WEATHER, "WETTER") \
  X(RTP_TRAFFIC, "VERKEHR") \
  X(RTP_ADVERT, "WERBUNG") \
  X(RTP_STATION, "SENDER") \
  X(RTP_PROGRAM, "SENDUNG") \
  X(RTP_NEXT, "DANACH") \
  X(RTP_PART, "TEIL") \
  X(RTP_HOST, "MODERATOR") \
  X(RTP_EDITORS, "REDAKTION") \
  X(RTP_FREQUENCY, "FREQUENZ") \
  X(RTP_CHANNEL, "KANAL") \
  X(RTP_PHONE, "TELEFON") \
  X(RTP_EMAIL, "E-MAIL") \
  X(RTP_VOTE, "ABSTIMMUNG") \
  X(RTP_PLACE, "ORT") \
  X(RTP_APPOINTMENT, "TERMIN") \
  X(RTP_PURCHASE, "KAUF") \
  X(DAY_SUNDAY, "SONNTAG") \
  X(DAY_MONDAY, "MONTAG") \
  X(DAY_TUESDAY, "DIENSTAG") \
  X(DAY_WEDNESDAY, "MITTWOCH") \
  X(DAY_THURSDAY, "DONNERSTAG") \
  X(DAY_FRIDAY, "FREITAG") \
  X(DAY_SATURDAY, "SAMSTAG") \
  X(MONTH_JANUARY, "Januar") \
  X(MONTH_FEBRUARY, "Februar") \
  X(MONTH_MARCH, "März") \
  X(MONTH_MAY, "Mai") \
  X(MONTH_JUNE, "Juni") \
  X(MONTH_JULY, "Juli") \
  X(MONTH_OCTOBER, "Oktober") \
  X(MONTH_DECEMBER, "Dezember") \
  X(DATE_SUFFIX_TH, ".") \
  X(DATE_SUFFIX_ST, ".") \
  X(DATE_SUFFIX_ND, ".") \
  X(DATE_SUFFIX_RD, ".") \
  X(DATE_TODAY, "Heute") \
  X(DATE_YESTERDAY, "Gestern") \
  X(DATE_YDAY, "GEST.") \
  X(DATE_FMT_WHEN_DATE, "%d. %s %s") \
  X(MONTH_SHORT_MAR, "Mär") \
  X(MONTH_SHORT_MAY, "Mai") \
  X(MONTH_SHORT_OCT, "Okt") \
  X(MONTH_SHORT_DEC, "Dez") \
  X(MONTH_TAG_MAR, "MÄR") \
  X(MONTH_TAG_MAY, "MAI") \
  X(MONTH_TAG_OCT, "OKT") \
  X(MONTH_TAG_DEC, "DEZ") \
  X(DX_NOW, "jetzt") \
  X(DX_FMT_MIN_AGO, "vor %u min") \
  X(DX_FMT_H_AGO, "vor %u h") \
  X(DX_FMT_D_AGO, "vor %u d") \
  X(TOUCH_CAL_TITLE, "Touch kalibrieren") \
  X(TOUCH_CAL_HOLD, "Kreis halten, bis er voll ist") \
  X(TOUCH_CAL_CANCEL_HINT, "Knopf drehen/drücken: Abbruch") \
  X(TOUCH_CAL_CHECK, "Test") \
  X(TOUCH_CAL_TAP_DOT, "Punkt antippen") \
  X(TOUCH_CAL_SAVED, "Gespeichert") \
  X(TOUCH_CAL_FMT_LANDED, "Der Test lag %u px daneben") \
  X(TOUCH_CAL_BACK_HINT, "Knopf drücken: zurück") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Nicht gespeichert") \
  X(TOUCH_CAL_FMT_MISSED, "Der Test lag %u px daneben,") \
  X(TOUCH_CAL_NO_FIT, "Die Punkte ergaben keine Kalibrierung,") \
  X(TOUCH_CAL_NOT_SAVED, "Speichern misslang,") \
  X(TOUCH_CAL_OLD_STAYS, "die alte Kalibrierung bleibt") \
  X(TOUCH_CAL_RETRY_HINT, "Knopf drücken: neu, drehen: verlassen")
// clang-format on
