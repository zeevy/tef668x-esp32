/*
 * The panel's texts in Italian. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_IT(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Scansione FM") \
  X(MENU_SCAN_MW, "Scansione AM") \
  X(MENU_SCAN_SW, "Scansione SW") \
  X(MENU_SCAN_LW, "Scansione LW") \
  X(MENU_SQUELCH, "Modo squelch") \
  X(MENU_SQUELCH_LEVEL, "Livello squelch") \
  X(MENU_SQUELCH_FLOOR, "Squelch minimo") \
  X(MENU_VOLUME_AGC, "AGC volume") \
  X(MENU_AGC_BOOST, "Boost AGC") \
  X(MENU_MUTE_RAMP, "Rampa mute") \
  X(MENU_BAND_PLAN, "Piano banda") \
  X(MENU_TUNING_STEP, "Passo sintonia") \
  X(MENU_SEEK_SENSITIVITY, "Sensibilità ricerca") \
  X(MENU_DE_EMPHASIS, "De-enfasi") \
  X(MENU_FORCED_MONO, "Forza mono") \
  X(MENU_STEREO_BLEND, "Fusione stereo") \
  X(MENU_ST_HI_BLEND, "Fusione stereo / alti") \
  X(MENU_HIGH_CUT, "Taglio alti") \
  X(MENU_NOISE_BLANKER, "Antidisturbo") \
  X(MENU_EQUALISER, "Equalizzatore") \
  X(COMMON_RDS_DECODER, "Decoder RDS") \
  X(MENU_RDS_REGION, "Regione RDS") \
  X(MENU_MW_SPACING, "Passo MW") \
  X(MENU_FILTER_WIDTH, "Larghezza filtro") \
  X(MENU_SOFT_MUTE, "Soft mute") \
  X(MENU_DWELL, "Tempo sosta") \
  X(MENU_STOP_ON, "Stop su") \
  X(MENU_SCAN, "Scansiona") \
  X(MENU_MEMORY_FROM, "Prima memoria") \
  X(MENU_MEMORY_TO, "Ultima memoria") \
  X(MENU_PRESET_RANGE, "Campo memorie") \
  X(MENU_LOOP_THE_BAND, "Ripeti banda") \
  X(MENU_DX_WIDTH, "Largh. scansione") \
  X(MENU_MUTE_WHILE_SCANNING, "Mute in scansione") \
  X(MENU_AUTO_LOG_NEW, "Registro automatico") \
  X(MENU_LOG_RADIO_TEXT, "Registra RT") \
  X(MENU_WATCH_PRESETS, "Sorveglia memorie") \
  X(MENU_SCOPE_BASELINE, "Rif. spettro") \
  X(MENU_MEDIAN, "Mediana") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Impara stazioni locali") \
  X(MENU_START_SCAN, "Avvia scansione") \
  X(MENU_DAY_THEME, "Tema giorno") \
  X(MENU_NIGHT_THEME, "Tema notte") \
  X(MENU_THEME, "Tema") \
  X(MENU_BRIGHTNESS, "Luminosità") \
  X(MENU_DIM_LEVEL, "Luce ridotta") \
  X(MENU_DIM_AFTER, "Riduci dopo") \
  X(MENU_ROTATION, "Rotazione display") \
  X(COMMON_BATTERY, "Batteria") \
  X(MENU_BATTERY_CHECK, "Controllo batteria") \
  X(MENU_LANGUAGE, "Lingua \xC2\xB7 Language") \
  X(MENU_ENCODER, "Tipo encoder") \
  X(MENU_DIRECTION, "Senso encoder") \
  X(MENU_KEY_BEEPS, "Beep tasti") \
  X(MENU_BAND_EDGE_BEEP, "Beep fine banda") \
  X(MENU_START_CHIME, "Suono avvio") \
  X(MENU_KEYPAD_TIMEOUT, "Timeout tastiera") \
  X(MENU_CALIBRATE_TOUCH, "Calibra touch") \
  X(MENU_CLOCK_FROM_NETWORK, "Ora da rete") \
  X(MENU_WEB_PIN, "PIN web") \
  X(MENU_NEW_PIN, "Nuovo PIN") \
  X(MENU_FMT_DIGIT_OF, "Cifra %u di %u") \
  X(MENU_STATUS, "Stato connessione") \
  X(MENU_WEB_SERVER, "Server web") \
  X(MENU_FMT_PC_CONNECTED, "Connessi: %u") \
  X(MENU_WEB_ADDRESS, "Indirizzo web") \
  X(MENU_IP_ADDRESS, "Indirizzo IP") \
  X(MENU_WI_FI_NAME, "Rete Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Nome hotspot") \
  X(MENU_MAC, "Indirizzo MAC") \
  X(MENU_WI_FI_SIGNAL, "Segnale Wi-Fi") \
  X(MENU_NETWORK_INFO, "Info rete") \
  X(MENU_RESTART, "Riavvia radio") \
  X(MENU_AUTO_OFF, "Spegni dopo") \
  X(MENU_UPDATE_CHECK, "Cerca aggiornamenti") \
  X(MENU_FIRMWARE_UPDATE, "Aggiorna firmware") \
  X(MENU_FMT_UPDATE_TO, "Aggiorna a %s") \
  X(MENU_UPDATE_TITLE, "Nuovo firmware") \
  X(MENU_UPDATE_THIS_RADIO, "Questa radio") \
  X(MENU_UPDATE_NEW_VERSION, "Nuova versione") \
  X(COMMON_SETTINGS, "Impostazioni") \
  X(COMMON_KEPT, "Invariate") \
  X(MENU_STATION, "Stazione") \
  X(MENU_UPDATE_NOW, "Aggiorna") \
  X(MENU_LATER, "Dopo") \
  X(MENU_UPDATE_UP_TO_DATE, "Aggiornato") \
  X(MENU_UPDATE_CHECKING, "In verifica") \
  X(MENU_UPDATE_NOT_CHECKED, "Non verificato") \
  X(MENU_UPDATE_CHECK_FAILED, "Verifica fallita") \
  X(MENU_NOTE_NO_UPDATE, "Nessun aggiornamento") \
  X(MENU_VERSION, "Versione firmware") \
  X(MENU_RUNNING_FROM, "Avvio da") \
  X(MENU_UPTIME, "Acceso da") \
  X(MENU_BATTERY_VOLTAGE, "Tensione batteria") \
  X(MENU_RESET_REASON, "Causa reset") \
  X(MENU_CPU_CORE_0, "Core CPU 0") \
  X(MENU_CPU_CORE_1, "Core CPU 1") \
  X(MENU_FREE_HEAP, "Heap libero") \
  X(MENU_LOWEST_HEAP, "Heap minimo") \
  X(MENU_LARGEST_BLOCK, "Blocco max") \
  X(MENU_LVGL_POOL, "Pool LVGL") \
  X(MENU_FLASH_SIZE, "Flash") \
  X(RESET_POWER, "Accensione") \
  X(RESET_INT_WATCHDOG, "Watchdog IRQ") \
  X(RESET_TASK_WATCHDOG, "Watchdog task") \
  X(RESET_DEEP_SLEEP, "Sospensione") \
  X(RESET_BROWNOUT, "Calo tensione") \
  X(RESET_PIN, "Pin reset") \
  X(RESET_UNKNOWN, "Sconosciuta") \
  X(RESET_ASKED, "Riavvio richiesto") \
  X(RESET_BOOT_WATCHDOG, "Watchdog avvio") \
  X(RESET_ROLLBACK, "Ripristino") \
  X(RESET_DISPLAY, "Errore display") \
  X(RESET_UPDATE, "Aggiornamento") \
  X(MENU_DEVELOPER, "Autore") \
  X(MENU_LICENSE, "Licenza") \
  X(ABOUT_BUILD_UNKNOWN, "sconosciuta") \
  X(MENU_GO_TO, "Vai a") \
  X(MENU_GO_TO_BANDWIDTH, "Largh. banda") \
  X(MENU_GO_TO_DX, "Modo DX") \
  X(MENU_GO_TO_SCOPE, "Spettro banda") \
  X(MENU_GO_TO_BAND, "Banda succ.") \
  X(MENU_GO_TO_LOG, "Registra stazione") \
  X(MENU_GO_TO_SAVE, "Salva stazione") \
  X(MENU_FMT_SAVED_AS, "Salvata in P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Già in P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Memorie piene") \
  X(MENU_GO_TO_SLEEP, "Sospendi") \
  X(MENU_STATIONS, "Stazioni") \
  X(MENU_MEMORY, "Memorie") \
  X(MENU_FM_SETUP, "Ricezione FM") \
  X(MENU_AM_SETUP, "Ricezione AM") \
  X(MENU_DX_SETUP, "Scanner DX") \
  X(MENU_CONTROLS, "Comandi") \
  X(MENU_CONNECTIVITY, "Connettività") \
  X(MENU_SYSTEM, "Sistema") \
  X(MENU_DIAGNOSTICS, "Diagnostica") \
  X(MENU_ABOUT, "Info") \
  X(MENU_REGION_FULL, "Intero") \
  X(MENU_REGION_JAPAN, "Giappone") \
  X(MENU_REGION_WIDE, "Esteso") \
  X(MENU_REGION_WORLD, "Mondiale") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Nord America") \
  X(MENU_WIFI_JOINED, "Connesso") \
  X(MENU_WIFI_JOINING, "In connessione") \
  X(MENU_FMT_KB_OF_KB, "%u di %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V all'avvio") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, %u min fa") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, %u h fa") \
  X(MENU_FMT_AGC_GAIN_NOW, "Guadagno ora %+d dB") \
  X(MENU_STATION_LOG, "Registro") \
  X(COMMON_NONE, "Nessuno") \
  X(MENU_FMT_SCAN_PROGRESS, "Scans. %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Salvate: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Fermata, salvate: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Piene, perse: %u") \
  X(COMMON_YES, "Sì") \
  X(MENU_UNKNOWN_VALUE, "Sconosciuto") \
  X(COMMON_ROTATION_NORMAL, "Normale") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Capovolto") \
  X(MENU_CUT_ONLY, "Solo taglio") \
  X(MENU_NEVER, "Mai") \
  X(MENU_BATTERY_PER_CENT, "Percentuale") \
  X(MENU_BATTERY_VOLTS, "Volt") \
  X(MENU_KEYS, "Solo tastiera") \
  X(MENU_KEYS_LONG, "Breve e lunga") \
  X(MENU_EVERY_PRESS, "Ogni tasto") \
  X(MENU_OPTICAL, "Ottico") \
  X(MENU_NORMAL, "Normale") \
  X(MENU_REVERSED, "Invertito") \
  X(MENU_SQUELCH_MAN, "Manuale") \
  X(MENU_ANY_PI, "Ogni PI") \
  X(MENU_STOP_NEVER, "Mai") \
  X(MENU_NEW_ONLY, "Solo stazioni nuove") \
  X(MENU_WHOLE_BAND, "Banda intera") \
  X(MENU_MEMORY_ONLY, "Solo memorie") \
  X(MENU_BAND_MEMORY, "Banda + memorie") \
  X(MENU_HINT_RESTART, "Attivo dopo il riavvio") \
  X(COMMON_FM_ONLY, "Solo FM") \
  X(MENU_NOTE_OFF_PLAN, "Fuori piano banda") \
  X(MENU_NOTE_SWITCH_TO_FM, "Passa prima a FM") \
  X(MENU_NOTE_RADIO_BUSY, "Radio occupata - riprova") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Attiva prima RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Firmware in prova - attendi") \
  X(MENU_NOTE_NOT_STORED, "Non salvato - ripristinato") \
  X(MENU_NOTE_NOT_PUT_BACK, "Radio occupata - non riprist.") \
  X(MENU_NOTE_NOT_SAVED, "Non salvato") \
  X(MENU_SAVE, "Salva") \
  X(RADIO_PRESS_TO_START, "Premi per avviare") \
  X(COMMON_STILL_TUNING, "Attendi fine sintonia") \
  X(COMMON_SCAN_RUNNING, "Attendi fine scansione") \
  X(RADIO_FMT_NO_BAND, "%s fuori banda") \
  X(COMMON_NOT_LOGGED, "Non registrata") \
  X(RADIO_FMT_LOGGED, "Registrata %s") \
  X(RADIO_CHECKING_UPDATES, "VERIFICA AGGIORNAMENTI\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Scansione %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Aggiornamento fallito") \
  X(RADIO_GOING_TO_SLEEP, "Sospensione") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Premi la manopola per riattivare") \
  X(RADIO_OLD_IMAGE_KEPT, "Firmware precedente mantenuto") \
  X(RDS_SPEECH, "Parlato") \
  X(RDS_MUSIC, "Musica") \
  X(COMMON_ALREADY_LOGGED, "Già registrata") \
  X(DX_RDS_IS_OFF, "RDS spento") \
  X(DX_SCANNING, "In scansione") \
  X(DX_SCAN_FINISHED, "Fine scansione") \
  X(DX_NOTHING_TO_SCAN, "Nulla da scansionare") \
  X(DX_CHECKING_FOR_UPDATES, "Verifica aggiornamenti") \
  X(DX_A_SCAN_IS_RUNNING, "Scansione già in corso") \
  X(DX_CAUGHT_LIST_UNREAD, "Catture non disponibili") \
  X(DX_NO_MEMORY, "Memorie vuote") \
  X(DX_NOT_STARTED, "Non avviata") \
  X(DX_SWEEPING, "Sweep in corso") \
  X(COMMON_FMT_TUNE_TYPED, "Vai a %s") \
  X(DX_FMT_ROWS_OF, "%u-%u di %u") \
  X(DX_FMT_FOUND, "Trovate: %u") \
  X(DX_MODE_SWEEPING, "SWEEP") \
  X(DX_MODE_LEARN_LOCALS, "IMPARA LOCALI") \
  X(DX_FMT_MODE_MEMORY, "MEMORIE %u-%u") \
  X(DX_MODE_WHOLE_BAND, "BANDA INTERA") \
  X(DX_MODE_BAND_MEMORY, "BANDA + MEMORIE") \
  X(DX_RULE_STOP_ON_PI, "STOP SU PI") \
  X(DX_RULE_STOP_ON_NEW, "STOP SU NUOVE") \
  X(DX_FMT_NOT_PRESET, "non P%02d") \
  X(DX_CONTEXT_SWEEPING, "sweep") \
  X(DX_PRESS_TO_SWEEP, "Premi per lo sweep") \
  X(SCOPE_FMT_TITLE, "Spettro %s") \
  X(SCOPE_FULL, "Intera") \
  X(DX_FIXED, "FISSO") \
  X(DX_FMT_MEDIAN_OF, "MEDIANA DI %u") \
  X(DX_FMT_FLOOR, "FONDO %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Chiudi") \
  X(RECOVERY_PRESS_AGAIN, "Premi ancora") \
  X(PAGE_TUNE, "Sintonizza") \
  X(PAGE_DELETE, "Elimina") \
  X(PAGE_LEVEL, "Livello") \
  X(PAGE_NOISE, "Rumore") \
  X(PAGE_COCHANNEL, "Co-canale") \
  X(PAGE_WIDTH, "Filtro") \
  X(PAGE_BAND, "Banda") \
  X(PAGE_NOTE_CHANGED, "Cambiato nel frattempo") \
  X(PAGE_NOTE_NOT_DELETED, "Non eliminato") \
  X(PAGE_FMT_SAVED, "Salvata %s") \
  X(PAGE_TIME_UNKNOWN, "Ora sconosciuta") \
  X(BW_TITLE, "Largh. banda") \
  X(KEYPAD_TITLE, "Frequenza") \
  X(KEYPAD_CANCEL, "Annulla") \
  X(DX_CATCHES_EMPTY_TITLE, "Nessuna cattura") \
  X(DX_CATCHES_EMPTY_NOTE, "Qui le stazioni confermate in questa sessione") \
  X(DX_TITLE_CATCHES, "Catture") \
  X(DX_READY, "Pronto") \
  X(DX_DWELL, "SOSTA") \
  X(DX_STOPPED, "FERMO") \
  X(DX_TITLE_SCOPE, "Spettro") \
  X(DX_LEVEL, "LIV.") \
  X(DX_RISE, "DIFF") \
  X(DX_NO_STATION_YET, "Nessuna stazione") \
  X(DX_NO_RDS_YET, "Nessun RDS") \
  X(RDS_TITLE_NETWORKS, "Reti") \
  X(RDS_NO_COUNTRY, "nessun ECC") \
  X(RDS_MS_UNKNOWN, "M/P") \
  X(RDS_RTPLUS_NONE, "Nessun RT+ da questa stazione") \
  X(RDS_RTPLUS_WAIT, "Nessun tag RT+ per questo testo") \
  X(RDS_AF_TITLE, "FREQUENZE ALTERNATIVE") \
  X(RDS_EON_TITLE, "ALTRE STAZIONI, EON") \
  X(RDS_NONE_HEARD, "Nessuna") \
  X(RDS_GROUPS_S_CAPS, "GRUPPI/S") \
  X(RDS_LOST, "PERSI") \
  X(RDS_EACH_BLOCK, "PER BLOCCO") \
  X(RDS_LEGEND_CLEAN, "integri") \
  X(RDS_LEGEND_FIXED, "corretti") \
  X(RDS_LEGEND_LOST, "persi") \
  X(RDS_ALL_CLEAN, "tutti integri") \
  X(RDS_GROUP_TYPES, "TIPI DI GRUPPO") \
  X(RDS_NO_GROUPS, "Nessun gruppo nell'ultimo minuto") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 %u s") \
  X(RDS_COUNTRY_UNLISTED, "non elencato") \
  X(RDS_NO_TYPES, "Nessun blocco B integro nell'ultimo minuto") \
  X(RDS_FMT_EON_MORE, "+%u altre") \
  X(RDS_EON_MORE_HEARD, "+ altre") \
  X(RDS_MORE, "altre") \
  X(RADIO_VEIL_UPDATING, "AGGIORNAMENTO FIRMWARE") \
  X(RADIO_VEIL_KEEP_POWER, "Non spegnere la radio") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Tuner non trovato sul bus I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tuner non pronto") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Identificazione tuner fallita") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Nessuna patch per questo tuner") \
  X(TUNER_UNKNOWN_TUNER_PART, "Tuner sconosciuto") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tuner: scrittura rifiutata") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tuner: lettura senza risposta") \
  X(TUNER_OUT_OF_RANGE, "Fuori limiti") \
  X(TUNER_UNKNOWN, "Sconosciuto") \
  X(BAND_LONG_LW, "Onde lunghe") \
  X(BAND_LONG_MW, "Onde medie") \
  X(BAND_LONG_SW, "Onde corte") \
  X(MENU_FMT_TUNE_TO, "Banda per %s") \
  X(PTY_NONE, "Nessuno") \
  X(PTY_NEWS, "Notizie") \
  X(PTY_CURRENT_AFFAIRS, "Attualità") \
  X(PTY_INFORMATION, "Informazione") \
  X(PTY_EDUCATION, "Educazione") \
  X(PTY_DRAMA, "Teatro") \
  X(PTY_CULTURE, "Cultura") \
  X(PTY_SCIENCE, "Scienza") \
  X(PTY_VARIED, "Varietà") \
  X(PTY_POP_MUSIC, "Musica pop") \
  X(PTY_ROCK_MUSIC, "Musica rock") \
  X(PTY_EASY_LISTENING, "Musica leggera") \
  X(PTY_LIGHT_CLASSICAL, "Classica leggera") \
  X(PTY_SERIOUS_CLASSICAL, "Classica") \
  X(PTY_OTHER_MUSIC, "Altra musica") \
  X(PTY_WEATHER, "Meteo") \
  X(PTY_FINANCE, "Finanza") \
  X(PTY_CHILDREN, "Bambini") \
  X(PTY_SOCIAL_AFFAIRS, "Società") \
  X(PTY_RELIGION, "Religione") \
  X(PTY_PHONE_IN, "Telefonate") \
  X(PTY_TRAVEL, "Viaggi") \
  X(PTY_LEISURE, "Svago") \
  X(PTY_COUNTRY_MUSIC, "Musica country") \
  X(PTY_NATIONAL_MUSIC, "Musica nazionale") \
  X(PTY_FOLK_MUSIC, "Musica folk") \
  X(PTY_DOCUMENTARY, "Documentari") \
  X(PTY_ALARM_TEST, "Test allarme") \
  X(PTY_ALARM, "Allarme") \
  X(PTY_NA_NEWS, "Notizie") \
  X(PTY_NA_INFORMATION, "Informazione") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_TALK, "Parlato") \
  X(PTY_NA_CLASSIC_ROCK, "Rock classico") \
  X(PTY_NA_CLASSICAL, "Classica") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Lingua straniera") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Musica religiosa") \
  X(PTY_NA_RELIGIOUS_TALK, "Parlato religioso") \
  X(PTY_NA_PERSONALITY, "Personalità") \
  X(PTY_NA_PUBLIC, "Pubblica") \
  X(PTY_NA_SPANISH_TALK, "Parlato spagnolo") \
  X(PTY_NA_SPANISH_MUSIC, "Musica spagnola") \
  X(PTY_NA_UNASSIGNED, "Non assegnato") \
  X(PTY_NA_WEATHER, "Meteo") \
  X(PTY_NA_EMERGENCY_TEST, "Test emergenza") \
  X(PTY_NA_EMERGENCY, "Emergenza") \
  X(COVERAGE_LOCAL, "Locale") \
  X(COVERAGE_INTERNATIONAL, "Internazionale") \
  X(COVERAGE_NATIONAL, "Nazionale") \
  X(COVERAGE_SUPRA_REGIONAL, "Sovraregionale") \
  X(COVERAGE_REGIONAL_1, "Regionale 1") \
  X(COVERAGE_REGIONAL_2, "Regionale 2") \
  X(COVERAGE_REGIONAL_3, "Regionale 3") \
  X(COVERAGE_REGIONAL_4, "Regionale 4") \
  X(COVERAGE_REGIONAL_5, "Regionale 5") \
  X(COVERAGE_REGIONAL_6, "Regionale 6") \
  X(COVERAGE_REGIONAL_7, "Regionale 7") \
  X(COVERAGE_REGIONAL_8, "Regionale 8") \
  X(COVERAGE_REGIONAL_9, "Regionale 9") \
  X(COVERAGE_REGIONAL_10, "Regionale 10") \
  X(COVERAGE_REGIONAL_11, "Regionale 11") \
  X(COVERAGE_REGIONAL_12, "Regionale 12") \
  X(RTP_OTHER, "ALTRO") \
  X(RTP_TITLE, "TITOLO") \
  X(RTP_TRACK, "BRANO") \
  X(RTP_ARTIST, "ARTISTA") \
  X(RTP_WORK, "OPERA") \
  X(RTP_MOVEMENT, "MOVIMENTO") \
  X(RTP_CONDUCTOR, "DIRETTORE") \
  X(RTP_COMPOSER, "COMPOSITORE") \
  X(RTP_BAND, "GRUPPO") \
  X(RTP_COMMENT, "COMMENTO") \
  X(RTP_GENRE, "GENERE") \
  X(RTP_NEWS, "NOTIZIE") \
  X(RTP_LOCAL, "NOTIZIE LOCALI") \
  X(RTP_STOCKS, "BORSA") \
  X(RTP_LOTTERY, "LOTTERIA") \
  X(RTP_HOROSCOPE, "OROSCOPO") \
  X(RTP_DIVERSION, "SVAGO") \
  X(RTP_HEALTH, "SALUTE") \
  X(RTP_EVENT, "EVENTO") \
  X(RTP_SCENE, "SCENA") \
  X(RTP_FUN, "SPASSO") \
  X(RTP_DATE, "DATA") \
  X(RTP_WEATHER, "METEO") \
  X(RTP_TRAFFIC, "TRAFFICO") \
  X(RTP_ALARM, "ALLARME") \
  X(RTP_ADVERT, "SPOT") \
  X(RTP_STATION, "STAZIONE") \
  X(RTP_PROGRAM, "PROGRAMMA") \
  X(RTP_NEXT, "SEGUE") \
  X(RTP_PART, "PARTE") \
  X(RTP_HOST, "CONDUTTORE") \
  X(RTP_EDITORS, "REDAZIONE") \
  X(RTP_FREQUENCY, "FREQUENZA") \
  X(RTP_CHANNEL, "CANALE") \
  X(RTP_PHONE, "TEL.") \
  X(RTP_VOTE, "VOTO") \
  X(RTP_PLACE, "LUOGO") \
  X(RTP_APPOINTMENT, "APPUNT.") \
  X(RTP_PURCHASE, "ACQUISTO") \
  X(DAY_SUNDAY, "DOMENICA") \
  X(DAY_MONDAY, "LUNEDÌ") \
  X(DAY_TUESDAY, "MARTEDÌ") \
  X(DAY_WEDNESDAY, "MERCOLEDÌ") \
  X(DAY_THURSDAY, "GIOVEDÌ") \
  X(DAY_FRIDAY, "VENERDÌ") \
  X(DAY_SATURDAY, "SABATO") \
  X(MONTH_JANUARY, "gennaio") \
  X(MONTH_FEBRUARY, "febbraio") \
  X(MONTH_MARCH, "marzo") \
  X(MONTH_APRIL, "aprile") \
  X(MONTH_MAY, "maggio") \
  X(MONTH_JUNE, "giugno") \
  X(MONTH_JULY, "luglio") \
  X(MONTH_AUGUST, "agosto") \
  X(MONTH_SEPTEMBER, "settembre") \
  X(MONTH_OCTOBER, "ottobre") \
  X(MONTH_NOVEMBER, "novembre") \
  X(MONTH_DECEMBER, "dicembre") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_FMT_LINE, "%s %d%s %s %d") \
  X(DATE_TODAY, "Oggi") \
  X(DATE_YESTERDAY, "Ieri") \
  X(DATE_YDAY, "IERI") \
  X(MONTH_SHORT_JAN, "gen") \
  X(MONTH_SHORT_FEB, "feb") \
  X(MONTH_SHORT_MAR, "mar") \
  X(MONTH_SHORT_APR, "apr") \
  X(MONTH_SHORT_MAY, "mag") \
  X(MONTH_SHORT_JUN, "giu") \
  X(MONTH_SHORT_JUL, "lug") \
  X(MONTH_SHORT_AUG, "ago") \
  X(MONTH_SHORT_SEP, "set") \
  X(MONTH_SHORT_OCT, "ott") \
  X(MONTH_SHORT_NOV, "nov") \
  X(MONTH_SHORT_DEC, "dic") \
  X(MONTH_TAG_JAN, "GEN") \
  X(MONTH_TAG_MAY, "MAG") \
  X(MONTH_TAG_JUN, "GIU") \
  X(MONTH_TAG_JUL, "LUG") \
  X(MONTH_TAG_AUG, "AGO") \
  X(MONTH_TAG_SEP, "SET") \
  X(MONTH_TAG_OCT, "OTT") \
  X(MONTH_TAG_DEC, "DIC") \
  X(DX_NOW, "ora") \
  X(DX_FMT_MIN_AGO, "%u min fa") \
  X(DX_FMT_H_AGO, "%u h fa") \
  X(DX_FMT_D_AGO, "%u g fa") \
  X(TOUCH_CAL_TITLE, "Calibra touch") \
  X(TOUCH_CAL_HOLD, "Tieni premuto il cerchio finché è pieno") \
  X(TOUCH_CAL_CANCEL_HINT, "Gira o premi la manopola per annullare") \
  X(TOUCH_CAL_CHECK, "Verifica") \
  X(TOUCH_CAL_TAP_DOT, "Tocca il punto") \
  X(TOUCH_CAL_SAVED, "Calibrazione salvata") \
  X(TOUCH_CAL_FMT_LANDED, "Verifica: scarto di %u px") \
  X(TOUCH_CAL_BACK_HINT, "Premi la manopola per tornare") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Non salvata") \
  X(TOUCH_CAL_FMT_MISSED, "Verifica: scarto di %u px,") \
  X(TOUCH_CAL_NO_FIT, "I punti non danno una calibrazione,") \
  X(TOUCH_CAL_NOT_SAVED, "Scrittura fallita,") \
  X(TOUCH_CAL_OLD_STAYS, "resta la vecchia calibrazione") \
  X(TOUCH_CAL_RETRY_HINT, "Premi la manopola per riprovare, gira per uscire")
// clang-format on
