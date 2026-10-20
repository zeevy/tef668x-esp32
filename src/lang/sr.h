/*
 * The panel's texts in Serbian (Latin script). The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_SR(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Skeniranje FM") \
  X(MENU_SCAN_MW, "Skeniranje AM") \
  X(MENU_SCAN_SW, "Skeniranje SW") \
  X(MENU_SCAN_LW, "Skeniranje LW") \
  X(MENU_SQUELCH, "SQL režim") \
  X(MENU_SQUELCH_LEVEL, "SQL nivo") \
  X(MENU_SQUELCH_FLOOR, "SQL minimum") \
  X(MENU_SQUELCH_GROUP, "Blokada šuma") \
  X(MENU_VOLUME_AGC, "AGC jačine") \
  X(MENU_AGC_BOOST, "AGC podizanje") \
  X(MENU_MUTE_RAMP, "Utišavanje") \
  X(MENU_BAND_PLAN, "Plan opsega") \
  X(MENU_TUNING_STEP, "Korak") \
  X(MENU_SEEK_SENSITIVITY, "Osetljivost pretrage") \
  X(MENU_DE_EMPHASIS, "De-emfaza") \
  X(MENU_FORCED_MONO, "Uvek mono") \
  X(MENU_STEREO_BLEND, "Stereo mešanje") \
  X(MENU_ST_HI_BLEND, "Mešanje stereo/visokih") \
  X(MENU_HIGH_CUT, "Rez visokih") \
  X(MENU_NOISE_BLANKER, "Noise blanker") \
  X(MENU_EQUALISER, "Ekvilajzer") \
  X(COMMON_RDS_DECODER, "RDS dekoder") \
  X(MENU_RDS_REGION, "RDS region") \
  X(MENU_MW_SPACING, "MW raster") \
  X(MENU_FILTER_WIDTH, "Širina filtera") \
  X(MENU_SOFT_MUTE, "Soft mute") \
  X(MENU_DWELL, "Pauza na kanalu") \
  X(MENU_STOP_ON, "Zaustavljanje") \
  X(MENU_SCAN, "Opseg skeniranja") \
  X(MENU_MEMORY_FROM, "Od pozicije") \
  X(MENU_MEMORY_TO, "Do pozicije") \
  X(MENU_PRESET_RANGE, "Opseg memorije") \
  X(MENU_LOOP_THE_BAND, "Ponavljaj opseg") \
  X(MENU_DX_WIDTH, "Širina skeniranja") \
  X(MENU_MUTE_WHILE_SCANNING, "Utišaj pri skeniranju") \
  X(MENU_AUTO_LOG_NEW, "Auto upis stanica") \
  X(MENU_LOG_RADIO_TEXT, "Upis radio teksta") \
  X(MENU_WATCH_PRESETS, "Prati memoriju") \
  X(MENU_SCOPE_BASELINE, "Referenca spektra") \
  X(MENU_MEDIAN, "Medijana") \
  X(MENU_FMT_SWEEP_N, "Prolaz %u") \
  X(DX_FMT_WATCH_UP, "%s raste %d dB") \
  X(MENU_LEARN_LOCALS, "Nauči lokalne stanice") \
  X(MENU_START_SCAN, "Skeniraj") \
  X(MENU_DAY_THEME, "Dnevna tema") \
  X(MENU_NIGHT_THEME, "Noćna tema") \
  X(MENU_THEME, "Tema") \
  X(MENU_BRIGHTNESS, "Osvetljenost") \
  X(MENU_DIM_LEVEL, "Zatamnjeno") \
  X(MENU_DIM_AFTER, "Zatamni posle") \
  X(MENU_ROTATION, "Rotacija ekrana") \
  X(COMMON_BATTERY, "Baterija") \
  X(MENU_BATTERY_CHECK, "Provera baterije") \
  X(MENU_LANGUAGE, "Jezik \xC2\xB7 Language") \
  X(MENU_ENCODER, "Tip enkodera") \
  X(MENU_DIRECTION, "Smer enkodera") \
  X(MENU_ENCODER_GROUP, "Enkoder") \
  X(MENU_KEY_BEEPS, "Zvuk tastera") \
  X(MENU_BAND_EDGE_BEEP, "Zvuk kraja opsega") \
  X(MENU_START_CHIME, "Zvuk pri startu") \
  X(MENU_KEYPAD_TIMEOUT, "Vreme za unos") \
  X(MENU_TOUCH, "Dodir") \
  X(MENU_CALIBRATE_TOUCH, "Kalibracija dodira") \
  X(MENU_CLOCK_FROM_NETWORK, "Mrežno vreme") \
  X(MENU_NEW_PIN, "Novi PIN") \
  X(MENU_FMT_DIGIT_OF, "Cifra %u od %u") \
  X(MENU_STATUS, "Status veze") \
  X(MENU_WEB_SERVER, "Web server") \
  X(MENU_FMT_PC_CONNECTED, "Povezano: %u") \
  X(MENU_WEB_ADDRESS, "Web adresa") \
  X(MENU_IP_ADDRESS, "IP adresa") \
  X(MENU_WI_FI_NAME, "Wi-Fi mreža") \
  X(MENU_HOTSPOT_NAME, "Ime hotspota") \
  X(MENU_MAC, "MAC adresa") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi signal") \
  X(MENU_NETWORK_INFO, "Podaci o mreži") \
  X(MENU_RESTART, "Restartuj radio") \
  X(MENU_AUTO_OFF, "Auto gašenje") \
  X(MENU_UPDATE_CHECK, "Provera ažuriranja") \
  X(MENU_FIRMWARE_UPDATE, "Ažuriranje firmvera") \
  X(MENU_FMT_UPDATE_TO, "Ažuriraj na %s") \
  X(MENU_UPDATE_TITLE, "Dostupno ažuriranje") \
  X(MENU_UPDATE_THIS_RADIO, "Ovaj radio") \
  X(MENU_UPDATE_NEW_VERSION, "Nova verzija") \
  X(MENU_UPDATE_DOWNLOAD, "Preuzimanje") \
  X(COMMON_SETTINGS, "Podešavanja") \
  X(COMMON_KEPT, "Ostaju") \
  X(MENU_STATION, "Stanica") \
  X(MENU_UPDATE_NOW, "Ažuriraj") \
  X(MENU_LATER, "Kasnije") \
  X(MENU_UPDATE_UP_TO_DATE, "Ažurno") \
  X(MENU_UPDATE_CHECKING, "Proverava se") \
  X(MENU_UPDATE_NOT_CHECKED, "Nije provereno") \
  X(MENU_UPDATE_CHECK_FAILED, "Greška provere") \
  X(MENU_NOTE_NO_UPDATE, "Nema ažuriranja") \
  X(MENU_VERSION, "Verzija firmvera") \
  X(MENU_RUNNING_FROM, "Pokrenut sa") \
  X(MENU_UPTIME, "Vreme rada") \
  X(MENU_BATTERY_VOLTAGE, "Napon baterije") \
  X(COMMON_TUNER, "Tjuner") \
  X(MENU_RESET_REASON, "Razlog reseta") \
  X(MENU_CPU_CORE_0, "CPU jezgro 0") \
  X(MENU_CPU_CORE_1, "CPU jezgro 1") \
  X(MENU_FREE_HEAP, "Slobodan heap") \
  X(MENU_LOWEST_HEAP, "Najmanji heap") \
  X(MENU_LARGEST_BLOCK, "Najveći blok") \
  X(MENU_LVGL_POOL, "LVGL pool") \
  X(MENU_CHIP, "Čip") \
  X(MENU_FLASH_SIZE, "Flash") \
  X(RESET_POWER, "Uključenje") \
  X(RESET_SOFTWARE, "Softverski") \
  X(RESET_PANIC, "Panika") \
  X(RESET_INT_WATCHDOG, "IRQ watchdog") \
  X(RESET_TASK_WATCHDOG, "Watchdog zadatka") \
  X(RESET_DEEP_SLEEP, "Duboki san") \
  X(RESET_BROWNOUT, "Pad napona") \
  X(RESET_PIN, "Reset pin") \
  X(RESET_UNKNOWN, "Nepoznato") \
  X(RESET_ASKED, "Na zahtev") \
  X(RESET_BOOT_WATCHDOG, "Watchdog starta") \
  X(RESET_ROLLBACK, "Vraćanje") \
  X(RESET_DISPLAY, "Greška ekrana") \
  X(RESET_UPDATE, "Ažuriranje") \
  X(MENU_DEVELOPER, "Autor") \
  X(MENU_LICENSE, "Licenca") \
  X(ABOUT_BUILD_UNKNOWN, "nepoznato") \
  X(MENU_GO_TO, "Idi na") \
  X(MENU_GO_TO_BANDWIDTH, "Širina opsega") \
  X(MENU_GO_TO_DX, "DX režim") \
  X(MENU_GO_TO_SCOPE, "Spektar") \
  X(MENU_GO_TO_BAND, "Sledeći opseg") \
  X(MENU_GO_TO_LOG, "Upiši u dnevnik") \
  X(MENU_GO_TO_SAVE, "Sačuvaj stanicu") \
  X(MENU_FMT_SAVED_AS, "Sačuvano u P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Već je P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Memorija puna") \
  X(MENU_GO_TO_SLEEP, "Spavanje") \
  X(MENU_STATIONS, "Stanice") \
  X(MENU_MEMORY, "Memorija") \
  X(MENU_AUDIO, "Zvuk") \
  X(MENU_FM_SETUP, "FM prijem") \
  X(MENU_AM_SETUP, "AM prijem") \
  X(MENU_DX_SETUP, "DX skener") \
  X(MENU_DISPLAY, "Ekran") \
  X(MENU_CONTROLS, "Kontrole") \
  X(MENU_CONNECTIVITY, "Mreža") \
  X(MENU_SYSTEM, "Sistem") \
  X(MENU_DIAGNOSTICS, "Dijagnostika") \
  X(MENU_ABOUT, "O uređaju") \
  X(MENU_REGION_FULL, "Pun") \
  X(MENU_REGION_WIDE, "Širok") \
  X(MENU_REGION_WORLD, "Ceo svet") \
  X(MENU_RDS_REGION_EUROPE, "Evropa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Severna Amerika") \
  X(MENU_WIFI_JOINED, "Povezano") \
  X(MENU_WIFI_JOINING, "Povezivanje") \
  X(COMMON_OFF, "Isklj.") \
  X(MENU_FMT_HOURS_MINUTES, "%u h %u min") \
  X(MENU_FMT_KB_OF_KB, "%u od %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V na startu") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, pre %u min") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, pre %u h") \
  X(MENU_FMT_AGC_GAIN_NOW, "Trenutno %+d dB") \
  X(MENU_STATION_LOG, "Dnevnik") \
  X(COMMON_NONE, "Nema") \
  X(MENU_FMT_SCAN_PROGRESS, "Sken. %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Sačuvano: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Prekid, sačuvano: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Puno, nesačuvano: %u") \
  X(COMMON_YES, "Da") \
  X(COMMON_NO, "Ne") \
  X(MENU_UNKNOWN_VALUE, "Nepoznato") \
  X(COMMON_ROTATION_NORMAL, "Normalno") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Naopako") \
  X(MENU_CUT_ONLY, "Bez podizanja") \
  X(MENU_NEVER, "Nikad") \
  X(MENU_BATTERY_PER_CENT, "Procenti") \
  X(MENU_BATTERY_VOLTS, "Volti") \
  X(MENU_KEYS, "Samo cifre") \
  X(MENU_KEYS_LONG, "Kratak i dug pritisak") \
  X(MENU_EVERY_PRESS, "Svaki pritisak") \
  X(MENU_STANDARD, "Standardni") \
  X(MENU_OPTICAL, "Optički") \
  X(MENU_NORMAL, "Normalan") \
  X(MENU_REVERSED, "Obrnut") \
  X(MENU_SQUELCH_MAN, "Ručno") \
  X(MENU_ANY_PI, "Svaki PI") \
  X(MENU_STOP_NEVER, "Nikad") \
  X(MENU_NEW_ONLY, "Samo nove stanice") \
  X(MENU_WHOLE_BAND, "Ceo opseg") \
  X(MENU_MEMORY_ONLY, "Samo memorija") \
  X(MENU_BAND_MEMORY, "Opseg + memorija") \
  X(COMMON_ON, "Uklj.") \
  X(MENU_HINT_RESTART, "Važi posle restarta") \
  X(MENU_TITLE, "Meni") \
  X(COMMON_FM_ONLY, "Samo FM") \
  X(MENU_NOTE_OFF_PLAN, "Nije u planu opsega") \
  X(MENU_NOTE_SWITCH_TO_FM, "Prvo pređite na FM") \
  X(MENU_NOTE_RADIO_BUSY, "Zauzeto - pokušajte opet") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Prvo uključite RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Probni firmver - čekajte") \
  X(MENU_NOTE_NOT_STORED, "Nije sačuvano - vraćeno") \
  X(MENU_NOTE_NOT_PUT_BACK, "Zauzeto - nije vraćeno") \
  X(MENU_NOTE_NOT_SAVED, "Nije sačuvano") \
  X(MENU_SAVE, "Sačuvaj") \
  X(RADIO_PRESS_TO_START, "Pritisnite za start") \
  X(COMMON_STILL_TUNING, "Pretraga - čekajte") \
  X(COMMON_SCAN_RUNNING, "Skeniranje - čekajte") \
  X(RADIO_FMT_NO_BAND, "%s van opsega") \
  X(COMMON_NOT_LOGGED, "Nije upisano") \
  X(RADIO_FMT_LOGGED, "Upisano %s") \
  X(RADIO_CHECKING_UPDATES, "PROVERA AŽURIRANJA\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Skeniranje %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Greška ažuriranja") \
  X(RADIO_GOING_TO_SLEEP, "Uspavljivanje") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Pritisnite točkić za buđenje") \
  X(RADIO_OLD_IMAGE_KEPT, "Zadržan prethodni firmver") \
  X(RDS_SPEECH, "Govor") \
  X(RDS_MUSIC, "Muzika") \
  X(COMMON_ALREADY_LOGGED, "Već upisano") \
  X(DX_RDS_IS_OFF, "RDS isključen") \
  X(DX_SCANNING, "Skeniranje") \
  X(DX_SCAN_FINISHED, "Skeniranje gotovo") \
  X(DX_NOTHING_TO_SCAN, "Ništa za skeniranje") \
  X(DX_CHECKING_FOR_UPDATES, "Provera ažuriranja") \
  X(DX_A_SCAN_IS_RUNNING, "Skeniranje već traje") \
  X(DX_CAUGHT_LIST_UNREAD, "Ulovi nedostupni") \
  X(DX_NO_MEMORY, "Memorija prazna") \
  X(DX_NOT_STARTED, "Nije pokrenuto") \
  X(DX_SWEEPING, "Prolaz u toku") \
  X(COMMON_FMT_TUNE_TYPED, "Podesi %s") \
  X(RADIO_SQL_OFF, "ISKLJ") \
  X(RADIO_TUNE_MANUAL, "RUČ") \
  X(DX_FMT_ROWS_OF, "%u-%u od %u") \
  X(DX_FMT_FOUND, "Nađeno: %u") \
  X(DX_MODE_SWEEPING, "PROLAZ") \
  X(DX_MODE_LEARN_LOCALS, "UČI LOKALNE") \
  X(DX_FMT_MODE_MEMORY, "MEM. %u-%u") \
  X(DX_MODE_WHOLE_BAND, "CEO OPSEG") \
  X(DX_MODE_BAND_MEMORY, "OPSEG + MEM.") \
  X(DX_RULE_NO_STOP, "NE STAJE") \
  X(DX_RULE_STOP_ON_PI, "STOP: PI") \
  X(DX_RULE_STOP_ON_NEW, "STOP: NOVA") \
  X(DX_FMT_NOT_PRESET, "nije P%02d") \
  X(DX_FMT_CHANNEL, "Poz. %u") \
  X(DX_CONTEXT_SWEEPING, "prolaz") \
  X(DX_PRESS_TO_SWEEP, "Pritisnite za prolaz") \
  X(SCOPE_FMT_TITLE, "%s spektar") \
  X(SCOPE_FULL, "Ceo") \
  X(DX_FIXED, "FIKS.") \
  X(DX_FMT_VS, "REF %s") \
  X(DX_FMT_MEDIAN_OF, "MEDIJANA %u") \
  X(DX_FMT_FLOOR, "ŠUM %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Zatvori") \
  X(RECOVERY_PRESS_AGAIN, "Još jednom") \
  X(PAGE_TUNE, "Podesi") \
  X(PAGE_DELETE, "Obriši") \
  X(PAGE_LEVEL, "Nivo") \
  X(PAGE_SNR, "S/Š") \
  X(PAGE_NOISE, "Šum") \
  X(PAGE_COCHANNEL, "Isti kanal") \
  X(PAGE_WIDTH, "Širina") \
  X(PAGE_BAND, "Opseg") \
  X(PAGE_NOTE_CHANGED, "Promenjeno u međuvremenu") \
  X(PAGE_NOTE_NOT_DELETED, "Nije obrisano") \
  X(PAGE_FMT_SAVED, "Sačuvano: %s") \
  X(PAGE_TIME_UNKNOWN, "Vreme nepoznato") \
  X(MENU_FMT_TUNER_PATCH, "%s zakrpa %u") \
  X(BW_TITLE, "Širina opsega") \
  X(KEYPAD_TITLE, "Frekvencija") \
  X(KEYPAD_CANCEL, "Otkaži") \
  X(DX_CATCHES_EMPTY_TITLE, "Još nema ulova") \
  X(DX_CATCHES_EMPTY_NOTE, "Ovde se prikazuju potvrđene stanice iz ove sesije") \
  X(DX_TITLE_CATCHES, "Ulovi") \
  X(DX_NEW, "NOV") \
  X(DX_READY, "Spremno") \
  X(DX_DWELL, "PAUZA") \
  X(DX_STOPPED, "STOP") \
  X(DX_TITLE_SCANNER, "Skener") \
  X(DX_TITLE_SCOPE, "Spektar") \
  X(DX_LEVEL, "NIVO") \
  X(DX_RISE, "RAST") \
  X(DX_NO_STATION_YET, "Još nema stanice") \
  X(DX_NO_RDS_YET, "Još nema RDS-a") \
  X(DX_OFFSET, "OFSET") \
  X(DX_NO_ID, "BEZ ID") \
  X(RDS_TITLE_TEXT, "Radio tekst") \
  X(RDS_TITLE_NETWORKS, "Mreže") \
  X(RDS_TITLE_DECODER, "Dekoder") \
  X(RDS_NO_COUNTRY, "nema ECC") \
  X(RDS_RTPLUS_NONE, "Od ove stanice nema RT+") \
  X(RDS_RTPLUS_WAIT, "Još nema RT+ oznaka za tekst") \
  X(RDS_AF_TITLE, "ALTERNATIVNE FREKVENCIJE") \
  X(RDS_EON_TITLE, "DRUGE STANICE, EON") \
  X(RDS_NONE_HEARD, "Nije primljeno") \
  X(RDS_SYNC_CAPS, "SINH") \
  X(RDS_GROUPS_S_CAPS, "GRUPA/S") \
  X(RDS_LOST, "GUBICI") \
  X(RDS_EACH_BLOCK, "PO BLOKU") \
  X(RDS_LEGEND_CLEAN, "čisto") \
  X(RDS_LEGEND_FIXED, "ispr.") \
  X(RDS_LEGEND_LOST, "izgub.") \
  X(RDS_ALL_CLEAN, "sve čisto") \
  X(RDS_GROUP_TYPES, "TIPOVI GRUPA") \
  X(RDS_NO_GROUPS, "Nema grupa u poslednjem minutu") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 za %u s") \
  X(RDS_COUNTRY_UNLISTED, "nije na listi") \
  X(RDS_NO_TYPES, "Nijedan čist blok B u poslednjem minutu") \
  X(RDS_FMT_EON_MORE, "još %u") \
  X(RDS_EON_MORE_HEARD, "i još") \
  X(RDS_MORE, "još") \
  X(RADIO_VEIL_UPDATING, "AŽURIRANJE FIRMVERA") \
  X(RADIO_VEIL_KEEP_POWER, "Ne isključujte napajanje") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Tjuner nije na I2C magistrali") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tjuner nije spreman") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Tjuner se ne identifikuje") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Nema zakrpe za ovaj tjuner") \
  X(TUNER_UNKNOWN_TUNER_PART, "Nepoznat tjuner") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tjuner odbio upis") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tjuner ne odgovara na čitanje") \
  X(TUNER_OUT_OF_RANGE, "Van opsega") \
  X(TUNER_UNKNOWN, "Nepoznato") \
  X(BAND_LONG_LW, "Dugi talasi") \
  X(BAND_LONG_MW, "Srednji talasi") \
  X(BAND_LONG_SW, "Kratki talasi") \
  X(MENU_FMT_TUNE_TO, "Opseg za %s") \
  X(PTY_NONE, "Nema") \
  X(PTY_NEWS, "Vesti") \
  X(PTY_CURRENT_AFFAIRS, "Aktuelnosti") \
  X(PTY_INFORMATION, "Informacije") \
  X(PTY_EDUCATION, "Obrazovanje") \
  X(PTY_CULTURE, "Kultura") \
  X(PTY_SCIENCE, "Nauka") \
  X(PTY_VARIED, "Razno") \
  X(PTY_POP_MUSIC, "Pop muzika") \
  X(PTY_ROCK_MUSIC, "Rok muzika") \
  X(PTY_EASY_LISTENING, "Lagana muzika") \
  X(PTY_LIGHT_CLASSICAL, "Laka klasika") \
  X(PTY_SERIOUS_CLASSICAL, "Klasika") \
  X(PTY_OTHER_MUSIC, "Ostala muzika") \
  X(PTY_WEATHER, "Prognoza") \
  X(PTY_FINANCE, "Finansije") \
  X(PTY_CHILDREN, "Za decu") \
  X(PTY_SOCIAL_AFFAIRS, "Društvo") \
  X(PTY_RELIGION, "Religija") \
  X(PTY_PHONE_IN, "Kontakt program") \
  X(PTY_TRAVEL, "Putovanja") \
  X(PTY_LEISURE, "Slobodno vreme") \
  X(PTY_JAZZ_MUSIC, "Džez") \
  X(PTY_COUNTRY_MUSIC, "Kantri muzika") \
  X(PTY_NATIONAL_MUSIC, "Narodna muzika") \
  X(PTY_OLDIES_MUSIC, "Stari hitovi") \
  X(PTY_FOLK_MUSIC, "Folk muzika") \
  X(PTY_DOCUMENTARY, "Dokumentarni") \
  X(PTY_ALARM_TEST, "Test alarma") \
  X(PTY_NA_NEWS, "Vesti") \
  X(PTY_NA_INFORMATION, "Informacije") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_TALK, "Razgovor") \
  X(PTY_NA_ROCK, "Rok") \
  X(PTY_NA_CLASSIC_ROCK, "Klasični rok") \
  X(PTY_NA_ADULT_HITS, "Hitovi za odrasle") \
  X(PTY_NA_SOFT_ROCK, "Lagani rok") \
  X(PTY_NA_COUNTRY, "Kantri") \
  X(PTY_NA_OLDIES, "Stari hitovi") \
  X(PTY_NA_SOFT, "Lagano") \
  X(PTY_NA_NOSTALGIA, "Nostalgija") \
  X(PTY_NA_JAZZ, "Džez") \
  X(PTY_NA_CLASSICAL, "Klasika") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Ritam i bluz") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Lagani ritam i bluz") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Strani jezik") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Duhovna muzika") \
  X(PTY_NA_RELIGIOUS_TALK, "Verski razgovor") \
  X(PTY_NA_PERSONALITY, "Ličnosti") \
  X(PTY_NA_PUBLIC, "Javni servis") \
  X(PTY_NA_COLLEGE, "Studentski") \
  X(PTY_NA_SPANISH_TALK, "Španski govor") \
  X(PTY_NA_SPANISH_MUSIC, "Španska muzika") \
  X(PTY_NA_HIP_HOP, "Hip-hop") \
  X(PTY_NA_UNASSIGNED, "Nedodeljeno") \
  X(PTY_NA_WEATHER, "Prognoza") \
  X(PTY_NA_EMERGENCY_TEST, "Test alarma") \
  X(PTY_NA_EMERGENCY, "Alarm") \
  X(COVERAGE_LOCAL, "Lokalno") \
  X(COVERAGE_INTERNATIONAL, "Međunarodno") \
  X(COVERAGE_NATIONAL, "Nacionalno") \
  X(COVERAGE_SUPRA_REGIONAL, "Nadregionalno") \
  X(COVERAGE_REGIONAL_1, "Region 1") \
  X(COVERAGE_REGIONAL_2, "Region 2") \
  X(COVERAGE_REGIONAL_3, "Region 3") \
  X(COVERAGE_REGIONAL_4, "Region 4") \
  X(COVERAGE_REGIONAL_5, "Region 5") \
  X(COVERAGE_REGIONAL_6, "Region 6") \
  X(COVERAGE_REGIONAL_7, "Region 7") \
  X(COVERAGE_REGIONAL_8, "Region 8") \
  X(COVERAGE_REGIONAL_9, "Region 9") \
  X(COVERAGE_REGIONAL_10, "Region 10") \
  X(COVERAGE_REGIONAL_11, "Region 11") \
  X(COVERAGE_REGIONAL_12, "Region 12") \
  X(RTP_OTHER, "DRUGO") \
  X(RTP_TITLE, "NAZIV") \
  X(RTP_TRACK, "PESMA") \
  X(RTP_ARTIST, "IZVOĐAČ") \
  X(RTP_WORK, "DELO") \
  X(RTP_MOVEMENT, "STAV") \
  X(RTP_CONDUCTOR, "DIRIGENT") \
  X(RTP_COMPOSER, "KOMPOZITOR") \
  X(RTP_BAND, "BEND") \
  X(RTP_COMMENT, "KOMENTAR") \
  X(RTP_GENRE, "ŽANR") \
  X(RTP_NEWS, "VESTI") \
  X(RTP_LOCAL, "LOK. VESTI") \
  X(RTP_STOCKS, "BERZA") \
  X(RTP_LOTTERY, "LUTRIJA") \
  X(RTP_HOROSCOPE, "HOROSKOP") \
  X(RTP_DIVERSION, "RAZONODA") \
  X(RTP_HEALTH, "ZDRAVLJE") \
  X(RTP_EVENT, "DOGAĐAJ") \
  X(RTP_SCENE, "SCENA") \
  X(RTP_CINEMA, "BIOSKOP") \
  X(RTP_FUN, "ŠALA") \
  X(RTP_DATE, "DATUM") \
  X(RTP_WEATHER, "PROGNOZA") \
  X(RTP_TRAFFIC, "SAOBRAĆAJ") \
  X(RTP_ADVERT, "REKLAMA") \
  X(RTP_STATION, "STANICA") \
  X(RTP_NEXT, "SLEDI") \
  X(RTP_PART, "DEO") \
  X(RTP_HOST, "VODITELJ") \
  X(RTP_EDITORS, "UREDNICI") \
  X(RTP_FREQUENCY, "FREKVENCIJA") \
  X(RTP_HOMEPAGE, "SAJT") \
  X(RTP_CHANNEL, "KANAL") \
  X(RTP_HOTLINE, "KONTAKT") \
  X(RTP_PHONE, "TEL.") \
  X(RTP_CHAT, "ČET") \
  X(RTP_VOTE, "GLAS") \
  X(RTP_PLACE, "MESTO") \
  X(RTP_APPOINTMENT, "TERMIN") \
  X(RTP_PURCHASE, "KUPOVINA") \
  X(DAY_SUNDAY, "NEDELJA") \
  X(DAY_MONDAY, "PONEDELJAK") \
  X(DAY_TUESDAY, "UTORAK") \
  X(DAY_WEDNESDAY, "SREDA") \
  X(DAY_THURSDAY, "ČETVRTAK") \
  X(DAY_FRIDAY, "PETAK") \
  X(DAY_SATURDAY, "SUBOTA") \
  X(MONTH_JANUARY, "januar") \
  X(MONTH_FEBRUARY, "februar") \
  X(MONTH_MARCH, "mart") \
  X(MONTH_APRIL, "april") \
  X(MONTH_MAY, "maj") \
  X(MONTH_JUNE, "jun") \
  X(MONTH_JULY, "jul") \
  X(MONTH_AUGUST, "avgust") \
  X(MONTH_SEPTEMBER, "septembar") \
  X(MONTH_OCTOBER, "oktobar") \
  X(MONTH_NOVEMBER, "novembar") \
  X(MONTH_DECEMBER, "decembar") \
  X(DATE_SUFFIX_TH, ".") \
  X(DATE_SUFFIX_ST, ".") \
  X(DATE_SUFFIX_ND, ".") \
  X(DATE_SUFFIX_RD, ".") \
  X(DATE_TODAY, "Danas") \
  X(DATE_YESTERDAY, "Juče") \
  X(DATE_YDAY, "JUČE") \
  X(DATE_FMT_WHEN_DATE, "%d. %s %s") \
  X(MONTH_SHORT_JAN, "jan") \
  X(MONTH_SHORT_FEB, "feb") \
  X(MONTH_SHORT_MAR, "mar") \
  X(MONTH_SHORT_APR, "apr") \
  X(MONTH_SHORT_MAY, "maj") \
  X(MONTH_SHORT_JUN, "jun") \
  X(MONTH_SHORT_JUL, "jul") \
  X(MONTH_SHORT_AUG, "avg") \
  X(MONTH_SHORT_SEP, "sep") \
  X(MONTH_SHORT_OCT, "okt") \
  X(MONTH_SHORT_NOV, "nov") \
  X(MONTH_SHORT_DEC, "dec") \
  X(MONTH_TAG_MAY, "MAJ") \
  X(MONTH_TAG_AUG, "AVG") \
  X(MONTH_TAG_OCT, "OKT") \
  X(DX_NOW, "sada") \
  X(DX_FMT_MIN_AGO, "pre %u min") \
  X(DX_FMT_H_AGO, "pre %u h") \
  X(DX_FMT_D_AGO, "pre %u d") \
  X(TOUCH_CAL_TITLE, "Kalibracija dodira") \
  X(TOUCH_CAL_HOLD, "Držite krug dok se ne popuni") \
  X(TOUCH_CAL_CANCEL_HINT, "Okrenite ili pritisnite točkić za otkaz") \
  X(TOUCH_CAL_CHECK, "Provera") \
  X(TOUCH_CAL_TAP_DOT, "Dodirnite tačku") \
  X(TOUCH_CAL_SAVED, "Kalibracija sačuvana") \
  X(TOUCH_CAL_FMT_LANDED, "Provera: odstupanje %u px") \
  X(TOUCH_CAL_BACK_HINT, "Pritisnite točkić za povratak") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Nije sačuvano") \
  X(TOUCH_CAL_FMT_MISSED, "Provera: odstupanje %u px,") \
  X(TOUCH_CAL_NO_FIT, "Oznake nisu dale kalibraciju,") \
  X(TOUCH_CAL_NOT_SAVED, "Nije mogla da se upiše,") \
  X(TOUCH_CAL_OLD_STAYS, "pa ostaje stara kalibracija") \
  X(TOUCH_CAL_RETRY_HINT, "Pritisnite za novi pokušaj, okrenite za izlaz")
// clang-format on
