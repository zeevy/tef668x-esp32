/*
 * The panel's texts in Croatian. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_HR(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "FM skeniranje") \
  X(MENU_SCAN_MW, "AM skeniranje") \
  X(MENU_SCAN_SW, "SW skeniranje") \
  X(MENU_SCAN_LW, "LW skeniranje") \
  X(MENU_SQUELCH, "Način squelcha") \
  X(MENU_SQUELCH_LEVEL, "Razina squelcha") \
  X(MENU_SQUELCH_FLOOR, "Donja granica") \
  X(MENU_VOLUME_AGC, "AGC glasnoće") \
  X(MENU_AGC_BOOST, "AGC pojačanje") \
  X(MENU_MUTE_RAMP, "Utišavanje") \
  X(MENU_BAND_PLAN, "Plan opsega") \
  X(MENU_TUNING_STEP, "Korak ugađanja") \
  X(MENU_SEEK_SENSITIVITY, "Osjetlj. traženja") \
  X(MENU_DE_EMPHASIS, "Deemfaza") \
  X(MENU_FORCED_MONO, "Uvijek mono") \
  X(MENU_STEREO_BLEND, "Miješanje stereo") \
  X(MENU_ST_HI_BLEND, "Miješanje stereo/visokih") \
  X(MENU_HIGH_CUT, "Rez visokih") \
  X(MENU_NOISE_BLANKER, "Prigušivač šuma") \
  X(MENU_EQUALISER, "Ekvilajzer") \
  X(COMMON_RDS_DECODER, "RDS dekoder") \
  X(MENU_RDS_REGION, "RDS regija") \
  X(MENU_MW_SPACING, "MW korak") \
  X(MENU_FILTER_WIDTH, "Širina filtra") \
  X(MENU_SOFT_MUTE, "Soft mute") \
  X(MENU_DWELL, "Pauza na kanalu") \
  X(MENU_STOP_ON, "Zaustavi na") \
  X(MENU_SCAN, "Raspon") \
  X(MENU_MEMORY_FROM, "Memorija od") \
  X(MENU_MEMORY_TO, "Memorija do") \
  X(MENU_PRESET_RANGE, "Raspon memorije") \
  X(MENU_LOOP_THE_BAND, "U krug") \
  X(MENU_DX_WIDTH, "Širina skeniranja") \
  X(MENU_MUTE_WHILE_SCANNING, "Utišaj pri skeniranju") \
  X(MENU_AUTO_LOG_NEW, "Nove u dnevnik") \
  X(MENU_LOG_RADIO_TEXT, "RT u dnevnik") \
  X(MENU_WATCH_PRESETS, "Prati memoriju") \
  X(MENU_SCOPE_BASELINE, "Referenca spektra") \
  X(MENU_MEDIAN, "Medijan") \
  X(MENU_FMT_SWEEP_N, "Prolaz %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Nauči lokalne postaje") \
  X(MENU_START_SCAN, "Skeniraj") \
  X(MENU_DAY_THEME, "Dnevna tema") \
  X(MENU_NIGHT_THEME, "Noćna tema") \
  X(MENU_THEME, "Tema") \
  X(MENU_BRIGHTNESS, "Svjetlina") \
  X(MENU_DIM_LEVEL, "Prigušeno") \
  X(MENU_DIM_AFTER, "Priguši nakon") \
  X(MENU_ROTATION, "Rotacija zaslona") \
  X(COMMON_BATTERY, "Baterija") \
  X(MENU_BATTERY_CHECK, "Provjera baterije") \
  X(MENU_LANGUAGE, "Jezik \xC2\xB7 Language") \
  X(MENU_ENCODER, "Vrsta enkodera") \
  X(MENU_DIRECTION, "Smjer enkodera") \
  X(MENU_ENCODER_GROUP, "Enkoder") \
  X(MENU_KEY_BEEPS, "Zvuk tipki") \
  X(MENU_BAND_EDGE_BEEP, "Zvuk ruba opsega") \
  X(MENU_START_CHIME, "Zvuk pokretanja") \
  X(MENU_KEYPAD_TIMEOUT, "Čekanje unosa") \
  X(MENU_TOUCH, "Dodir") \
  X(MENU_CALIBRATE_TOUCH, "Kalibracija dodira") \
  X(MENU_CLOCK_FROM_NETWORK, "Vrijeme s mreže") \
  X(MENU_NEW_PIN, "Novi PIN") \
  X(MENU_FMT_DIGIT_OF, "Znamenka %u/%u") \
  X(MENU_STATUS, "Stanje veze") \
  X(MENU_WEB_SERVER, "Web poslužitelj") \
  X(MENU_FMT_PC_CONNECTED, "Spojeno: %u") \
  X(MENU_WEB_ADDRESS, "Web adresa") \
  X(MENU_IP_ADDRESS, "IP adresa") \
  X(MENU_WI_FI_NAME, "Wi-Fi mreža") \
  X(MENU_HOTSPOT_NAME, "Naziv hotspota") \
  X(MENU_MAC, "MAC adresa") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi signal") \
  X(MENU_NETWORK_INFO, "Podaci o mreži") \
  X(MENU_RESTART, "Ponovno pokreni") \
  X(MENU_AUTO_OFF, "Auto isklj.") \
  X(MENU_UPDATE_CHECK, "Provjera ažuriranja") \
  X(MENU_FIRMWARE_UPDATE, "Ažuriranje firmwarea") \
  X(MENU_FMT_UPDATE_TO, "Ažuriraj na %s") \
  X(MENU_UPDATE_TITLE, "Dostupno ažuriranje") \
  X(MENU_UPDATE_THIS_RADIO, "Ovaj radio") \
  X(MENU_UPDATE_NEW_VERSION, "Nova verzija") \
  X(MENU_UPDATE_DOWNLOAD, "Preuzimanje") \
  X(COMMON_SETTINGS, "Postavke") \
  X(COMMON_KEPT, "Ostaju") \
  X(MENU_STATION, "Postaja") \
  X(MENU_UPDATE_NOW, "Ažuriraj") \
  X(MENU_LATER, "Kasnije") \
  X(MENU_UPDATE_UP_TO_DATE, "Ažurno") \
  X(MENU_UPDATE_CHECKING, "Provjera") \
  X(MENU_UPDATE_NOT_CHECKED, "Nije provjereno") \
  X(MENU_UPDATE_CHECK_FAILED, "Greška provjere") \
  X(MENU_NOTE_NO_UPDATE, "Nema ažuriranja") \
  X(MENU_VERSION, "Verzija firmwarea") \
  X(MENU_RUNNING_FROM, "Pokrenuto iz") \
  X(MENU_UPTIME, "Vrijeme rada") \
  X(MENU_BATTERY_VOLTAGE, "Napon baterije") \
  X(MENU_RESET_REASON, "Razlog reseta") \
  X(MENU_CPU_CORE_0, "CPU jezgra 0") \
  X(MENU_CPU_CORE_1, "CPU jezgra 1") \
  X(MENU_FREE_HEAP, "Slobodni heap") \
  X(MENU_LOWEST_HEAP, "Najmanji heap") \
  X(MENU_LARGEST_BLOCK, "Najveći blok") \
  X(MENU_CHIP, "Čip") \
  X(MENU_FLASH_SIZE, "Veličina flasha") \
  X(RESET_POWER, "Uključenje") \
  X(RESET_SOFTWARE, "Softver") \
  X(RESET_PANIC, "Panika") \
  X(RESET_INT_WATCHDOG, "IRQ watchdog") \
  X(RESET_TASK_WATCHDOG, "Watchdog zadatka") \
  X(RESET_DEEP_SLEEP, "Duboki san") \
  X(RESET_BROWNOUT, "Pad napona") \
  X(RESET_PIN, "Reset pin") \
  X(RESET_UNKNOWN, "Nepoznato") \
  X(RESET_ASKED, "Na zahtjev") \
  X(RESET_BOOT_WATCHDOG, "Watchdog pokretanja") \
  X(RESET_ROLLBACK, "Vraćanje") \
  X(RESET_DISPLAY, "Greška zaslona") \
  X(RESET_UPDATE, "Ažuriranje") \
  X(MENU_DEVELOPER, "Autor") \
  X(MENU_LICENSE, "Licenca") \
  X(ABOUT_BUILD_UNKNOWN, "nepoznato") \
  X(MENU_GO_TO, "Idi na") \
  X(MENU_GO_TO_BANDWIDTH, "Širina pojasa") \
  X(MENU_GO_TO_DX, "DX način") \
  X(MENU_GO_TO_SCOPE, "Spektar") \
  X(MENU_GO_TO_BAND, "Idući opseg") \
  X(MENU_GO_TO_LOG, "Upiši u dnevnik") \
  X(MENU_GO_TO_SAVE, "Spremi postaju") \
  X(MENU_FMT_SAVED_AS, "Spremljeno: P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Već u P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Memorija je puna") \
  X(MENU_GO_TO_SLEEP, "Mirovanje") \
  X(MENU_STATIONS, "Postaje") \
  X(MENU_MEMORY, "Memorija") \
  X(MENU_AUDIO, "Zvuk") \
  X(MENU_FM_SETUP, "FM prijem") \
  X(MENU_AM_SETUP, "AM prijem") \
  X(MENU_DX_SETUP, "DX skener") \
  X(MENU_DISPLAY, "Zaslon") \
  X(MENU_CONTROLS, "Upravljanje") \
  X(MENU_CONNECTIVITY, "Povezivost") \
  X(MENU_SYSTEM, "Sustav") \
  X(MENU_DIAGNOSTICS, "Dijagnostika") \
  X(MENU_ABOUT, "O radiju") \
  X(MENU_REGION_FULL, "Puni") \
  X(MENU_REGION_WIDE, "Široki") \
  X(MENU_REGION_WORLD, "Svjetski") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Sjeverna Amerika") \
  X(MENU_WIFI_JOINED, "Spojeno") \
  X(MENU_WIFI_JOINING, "Spajanje") \
  X(COMMON_OFF, "Isklj.") \
  X(MENU_FMT_HOURS_MINUTES, "%u h %u min") \
  X(MENU_FMT_KB_OF_KB, "%u od %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V pri startu") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, prije %u min") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, prije %u h") \
  X(MENU_FMT_AGC_GAIN_NOW, "Pojačanje: %+d dB") \
  X(MENU_STATION_LOG, "Dnevnik") \
  X(COMMON_NONE, "Nema") \
  X(MENU_FMT_SCAN_PROGRESS, "Sken. %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Spremljeno: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Prekid, spremljeno: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Puno, nespremljeno: %u") \
  X(COMMON_YES, "Da") \
  X(COMMON_NO, "Ne") \
  X(MENU_UNKNOWN_VALUE, "Nepoznato") \
  X(COMMON_ROTATION_NORMAL, "Normalno") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Naopako") \
  X(MENU_CUT_ONLY, "Bez pojačanja") \
  X(MENU_NEVER, "Nikad") \
  X(MENU_BATTERY_PER_CENT, "Postotak") \
  X(MENU_BATTERY_VOLTS, "Volti") \
  X(MENU_KEYS, "Samo brojke") \
  X(MENU_KEYS_LONG, "Kratki i dugi pritisak") \
  X(MENU_EVERY_PRESS, "Svaki pritisak") \
  X(MENU_STANDARD, "Standardni") \
  X(MENU_OPTICAL, "Optički") \
  X(MENU_NORMAL, "Normalan") \
  X(MENU_REVERSED, "Obrnut") \
  X(MENU_SQUELCH_MAN, "Ručno") \
  X(MENU_ANY_PI, "Svaki PI") \
  X(MENU_STOP_NEVER, "Nikad") \
  X(MENU_NEW_ONLY, "Samo nove postaje") \
  X(MENU_WHOLE_BAND, "Cijeli opseg") \
  X(MENU_MEMORY_ONLY, "Samo memorija") \
  X(MENU_BAND_MEMORY, "Opseg + memorija") \
  X(COMMON_ON, "Uklj.") \
  X(MENU_HINT_RESTART, "Vrijedi nakon restarta") \
  X(MENU_TITLE, "Izbornik") \
  X(COMMON_FM_ONLY, "Samo FM") \
  X(MENU_NOTE_OFF_PLAN, "Nije u planu opsega") \
  X(MENU_NOTE_SWITCH_TO_FM, "Prvo uključite FM") \
  X(MENU_NOTE_RADIO_BUSY, "Radio zauzet - ponovite") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Prvo uključite RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Probni firmware - čekajte") \
  X(MENU_NOTE_NOT_STORED, "Nije spremljeno - vraćeno") \
  X(MENU_NOTE_NOT_PUT_BACK, "Radio zauzet - nije vraćeno") \
  X(MENU_NOTE_NOT_SAVED, "Nije spremljeno") \
  X(MENU_SAVE, "Spremi") \
  X(RADIO_PRESS_TO_START, "Pritisnite za start") \
  X(COMMON_STILL_TUNING, "Ugađanje - čekajte") \
  X(COMMON_SCAN_RUNNING, "Skeniranje - čekajte") \
  X(RADIO_FMT_NO_BAND, "%s izvan opsega") \
  X(COMMON_NOT_LOGGED, "Nije upisano") \
  X(RADIO_FMT_LOGGED, "Upisano: %s") \
  X(RADIO_CHECKING_UPDATES, "PROVJERA AŽURIRANJA\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Skeniranje %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Greška ažuriranja") \
  X(RADIO_GOING_TO_SLEEP, "Prelazak u mirovanje") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Pritisnite gumb za buđenje") \
  X(RADIO_OLD_IMAGE_KEPT, "Zadržan stari firmware") \
  X(RDS_SPEECH, "Govor") \
  X(RDS_MUSIC, "Glazba") \
  X(COMMON_ALREADY_LOGGED, "Već upisano") \
  X(DX_RDS_IS_OFF, "RDS isključen") \
  X(DX_SCANNING, "Skeniranje") \
  X(DX_SCAN_FINISHED, "Skeniranje gotovo") \
  X(DX_NOTHING_TO_SCAN, "Nema što skenirati") \
  X(DX_CHECKING_FOR_UPDATES, "Provjera ažuriranja") \
  X(DX_A_SCAN_IS_RUNNING, "Skeniranje već traje") \
  X(DX_CAUGHT_LIST_UNREAD, "Ulovi nedostupni") \
  X(DX_NO_MEMORY, "Memorija prazna") \
  X(DX_NOT_STARTED, "Nije pokrenuto") \
  X(DX_SWEEPING, "Prolaz traje") \
  X(COMMON_FMT_TUNE_TYPED, "Ugodi %s") \
  X(RADIO_TUNE_MANUAL, "RUČ") \
  X(DX_FMT_ROWS_OF, "%u-%u od %u") \
  X(DX_FMT_FOUND, "Nađeno: %u") \
  X(DX_MODE_SWEEPING, "PROLAZ") \
  X(DX_MODE_LEARN_LOCALS, "UČI LOKALNE") \
  X(DX_FMT_MODE_MEMORY, "MEM. %u-%u") \
  X(DX_MODE_WHOLE_BAND, "CIJELI OPSEG") \
  X(DX_MODE_BAND_MEMORY, "OPSEG + MEM.") \
  X(DX_RULE_NO_STOP, "NE STAJE") \
  X(DX_RULE_STOP_ON_PI, "STOP NA PI") \
  X(DX_RULE_STOP_ON_NEW, "STOP NA NOVOJ") \
  X(DX_FMT_NOT_PRESET, "nije P%02d") \
  X(DX_FMT_CHANNEL, "KAN %u") \
  X(DX_CONTEXT_SWEEPING, "prolaz") \
  X(DX_PRESS_TO_SWEEP, "Pritisnite za prolaz") \
  X(SCOPE_FMT_TITLE, "%s spektar") \
  X(SCOPE_FULL, "Cijeli") \
  X(DX_FIXED, "FIKS.") \
  X(DX_FMT_VS, "REF %s") \
  X(DX_FMT_MEDIAN_OF, "MEDIJAN OD %u") \
  X(DX_FMT_FLOOR, "ŠUM %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Zatvori") \
  X(RECOVERY_PRESS_AGAIN, "Pritisnite opet") \
  X(PAGE_TUNE, "Ugodi") \
  X(PAGE_DELETE, "Obriši") \
  X(PAGE_LEVEL, "Razina") \
  X(PAGE_NOISE, "Šum") \
  X(PAGE_MULTIPATH, "Višestaznost") \
  X(PAGE_COCHANNEL, "Isti kanal") \
  X(PAGE_WIDTH, "Širina") \
  X(PAGE_BAND, "Opseg") \
  X(PAGE_NOTE_CHANGED, "Promijenjeno nakon otvaranja") \
  X(PAGE_NOTE_NOT_DELETED, "Nije obrisano") \
  X(PAGE_FMT_SAVED, "Spremljeno: %s") \
  X(PAGE_TIME_UNKNOWN, "Vrijeme nepoznato") \
  X(MENU_FMT_TUNER_PATCH, "%s zakrpa %u") \
  X(BW_TITLE, "Širina pojasa") \
  X(KEYPAD_TITLE, "Frekvencija") \
  X(KEYPAD_CANCEL, "Odustani") \
  X(DX_CATCHES_EMPTY_TITLE, "Još nema ulova") \
  X(DX_CATCHES_EMPTY_NOTE, "Ovdje se prikazuju potvrđene postaje ove sesije") \
  X(DX_TITLE_CATCHES, "Ulovi") \
  X(DX_NEW, "NOV") \
  X(DX_READY, "Spremno") \
  X(DX_DWELL, "PAUZA") \
  X(DX_STOPPED, "STOP") \
  X(DX_TITLE_SCANNER, "Skener") \
  X(DX_TITLE_SCOPE, "Spektar") \
  X(DX_LEVEL, "RAZ.") \
  X(DX_RISE, "RAST") \
  X(DX_NO_STATION_YET, "Još nema postaje") \
  X(DX_NO_RDS_YET, "Još nema RDS-a") \
  X(DX_OFFSET, "POMAK") \
  X(DX_NO_ID, "BEZ ID") \
  X(RDS_TITLE_TEXT, "Radiotekst") \
  X(RDS_TITLE_NETWORKS, "Mreže") \
  X(RDS_TITLE_DECODER, "Dekoder") \
  X(RDS_NO_COUNTRY, "nema ECC-a") \
  X(RDS_RTPLUS_NONE, "Nema RT+ s ove postaje") \
  X(RDS_RTPLUS_WAIT, "Još nema RT+ oznaka za ovaj tekst") \
  X(RDS_AF_TITLE, "ALTERNATIVNE FREKVENCIJE") \
  X(RDS_EON_TITLE, "DRUGE POSTAJE, EON") \
  X(RDS_NONE_HEARD, "Nema") \
  X(RDS_SYNC_CAPS, "SINKR.") \
  X(RDS_GROUPS_S_CAPS, "GRUPA/S") \
  X(RDS_LOST, "GUBICI") \
  X(RDS_EACH_BLOCK, "BLOKOVI") \
  X(RDS_LEGEND_CLEAN, "čisto") \
  X(RDS_LEGEND_FIXED, "ispr.") \
  X(RDS_LEGEND_LOST, "izgub.") \
  X(RDS_ALL_CLEAN, "sve čisto") \
  X(RDS_GROUP_TYPES, "VRSTE GRUPA") \
  X(RDS_NO_GROUPS, "Nema grupa u zadnjoj minuti") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 za %u s") \
  X(RDS_COUNTRY_UNLISTED, "nije na popisu") \
  X(RDS_NO_TYPES, "Nijedan čisti blok B u zadnjoj minuti") \
  X(RDS_FMT_EON_MORE, "+%u još") \
  X(RDS_EON_MORE_HEARD, "+ još") \
  X(RDS_MORE, "još") \
  X(RADIO_VEIL_UPDATING, "AŽURIRANJE FIRMWAREA") \
  X(RADIO_VEIL_KEEP_POWER, "Ne isključujte napajanje") \
  X(RADIO_MUTE, "TIHO") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Tuner nije nađen na I2C sabirnici") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tuner nije postao spreman") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Tuner se nije identificirao") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Nema zakrpe za ovaj tuner") \
  X(TUNER_UNKNOWN_TUNER_PART, "Nepoznat tuner") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tuner odbio upis") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tuner ne odgovara na čitanje") \
  X(TUNER_OUT_OF_RANGE, "Izvan raspona") \
  X(TUNER_UNKNOWN, "Nepoznato") \
  X(BAND_LONG_LW, "Dugi val") \
  X(BAND_LONG_MW, "Srednji val") \
  X(BAND_LONG_SW, "Kratki val") \
  X(MENU_FMT_TUNE_TO, "Opseg za %s") \
  X(PTY_NONE, "Nema") \
  X(PTY_NEWS, "Vijesti") \
  X(PTY_CURRENT_AFFAIRS, "Aktualno") \
  X(PTY_INFORMATION, "Informacije") \
  X(PTY_EDUCATION, "Obrazovanje") \
  X(PTY_CULTURE, "Kultura") \
  X(PTY_SCIENCE, "Znanost") \
  X(PTY_VARIED, "Razno") \
  X(PTY_POP_MUSIC, "Pop glazba") \
  X(PTY_ROCK_MUSIC, "Rock glazba") \
  X(PTY_EASY_LISTENING, "Zabavna glazba") \
  X(PTY_LIGHT_CLASSICAL, "Lagana klasika") \
  X(PTY_SERIOUS_CLASSICAL, "Klasika") \
  X(PTY_OTHER_MUSIC, "Ostala glazba") \
  X(PTY_WEATHER, "Vrijeme") \
  X(PTY_FINANCE, "Financije") \
  X(PTY_CHILDREN, "Za djecu") \
  X(PTY_SOCIAL_AFFAIRS, "Društvo") \
  X(PTY_RELIGION, "Religija") \
  X(PTY_PHONE_IN, "Kontakt program") \
  X(PTY_TRAVEL, "Putovanja") \
  X(PTY_LEISURE, "Slobodno vrijeme") \
  X(PTY_COUNTRY_MUSIC, "Country glazba") \
  X(PTY_NATIONAL_MUSIC, "Domaća glazba") \
  X(PTY_OLDIES_MUSIC, "Stari hitovi") \
  X(PTY_FOLK_MUSIC, "Narodna glazba") \
  X(PTY_DOCUMENTARY, "Dokumentarno") \
  X(PTY_ALARM_TEST, "Test alarma") \
  X(PTY_NA_NEWS, "Vijesti") \
  X(PTY_NA_INFORMATION, "Informacije") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_TALK, "Razgovor") \
  X(PTY_NA_CLASSIC_ROCK, "Klasični rock") \
  X(PTY_NA_ADULT_HITS, "Hitovi za odrasle") \
  X(PTY_NA_SOFT_ROCK, "Soft rock") \
  X(PTY_NA_OLDIES, "Stari hitovi") \
  X(PTY_NA_SOFT, "Lagano") \
  X(PTY_NA_NOSTALGIA, "Nostalgija") \
  X(PTY_NA_CLASSICAL, "Klasika") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Ritam i blues") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Lagani ritam i blues") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Strani jezik") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Duhovna glazba") \
  X(PTY_NA_RELIGIOUS_TALK, "Vjerski razgovor") \
  X(PTY_NA_PERSONALITY, "Autorski program") \
  X(PTY_NA_PUBLIC, "Javni") \
  X(PTY_NA_COLLEGE, "Studentski") \
  X(PTY_NA_SPANISH_TALK, "Španjolski govor") \
  X(PTY_NA_SPANISH_MUSIC, "Španjolska glazba") \
  X(PTY_NA_UNASSIGNED, "Nedodijeljeno") \
  X(PTY_NA_WEATHER, "Vrijeme") \
  X(PTY_NA_EMERGENCY_TEST, "Test uzbune") \
  X(PTY_NA_EMERGENCY, "Uzbuna") \
  X(COVERAGE_LOCAL, "Lokalno") \
  X(COVERAGE_INTERNATIONAL, "Međunarodno") \
  X(COVERAGE_NATIONAL, "Nacionalno") \
  X(COVERAGE_SUPRA_REGIONAL, "Nadregionalno") \
  X(COVERAGE_REGIONAL_1, "Regija 1") \
  X(COVERAGE_REGIONAL_2, "Regija 2") \
  X(COVERAGE_REGIONAL_3, "Regija 3") \
  X(COVERAGE_REGIONAL_4, "Regija 4") \
  X(COVERAGE_REGIONAL_5, "Regija 5") \
  X(COVERAGE_REGIONAL_6, "Regija 6") \
  X(COVERAGE_REGIONAL_7, "Regija 7") \
  X(COVERAGE_REGIONAL_8, "Regija 8") \
  X(COVERAGE_REGIONAL_9, "Regija 9") \
  X(COVERAGE_REGIONAL_10, "Regija 10") \
  X(COVERAGE_REGIONAL_11, "Regija 11") \
  X(COVERAGE_REGIONAL_12, "Regija 12") \
  X(RTP_OTHER, "OSTALO") \
  X(RTP_TITLE, "NASLOV") \
  X(RTP_TRACK, "PJESMA") \
  X(RTP_ARTIST, "IZVOĐAČ") \
  X(RTP_WORK, "DJELO") \
  X(RTP_MOVEMENT, "STAVAK") \
  X(RTP_CONDUCTOR, "DIRIGENT") \
  X(RTP_COMPOSER, "SKLADATELJ") \
  X(RTP_BAND, "BEND") \
  X(RTP_COMMENT, "KOMENTAR") \
  X(RTP_GENRE, "ŽANR") \
  X(RTP_NEWS, "VIJESTI") \
  X(RTP_LOCAL, "LOK. VIJESTI") \
  X(RTP_STOCKS, "BURZA") \
  X(RTP_LOTTERY, "LUTRIJA") \
  X(RTP_HOROSCOPE, "HOROSKOP") \
  X(RTP_DIVERSION, "RAZONODA") \
  X(RTP_HEALTH, "ZDRAVLJE") \
  X(RTP_EVENT, "DOGAĐAJ") \
  X(RTP_SCENE, "SCENA") \
  X(RTP_CINEMA, "KINO") \
  X(RTP_FUN, "HUMOR") \
  X(RTP_DATE, "DATUM") \
  X(RTP_WEATHER, "VRIJEME") \
  X(RTP_TRAFFIC, "PROMET") \
  X(RTP_ADVERT, "REKLAMA") \
  X(RTP_STATION, "POSTAJA") \
  X(RTP_PROGRAM, "EMISIJA") \
  X(RTP_NEXT, "SLIJEDI") \
  X(RTP_PART, "DIO") \
  X(RTP_HOST, "VODITELJ") \
  X(RTP_EDITORS, "UREDNICI") \
  X(RTP_FREQUENCY, "FREKVENCIJA") \
  X(RTP_HOMEPAGE, "STRANICA") \
  X(RTP_CHANNEL, "KANAL") \
  X(RTP_PHONE, "TELEFON") \
  X(RTP_VOTE, "GLAS") \
  X(RTP_PLACE, "MJESTO") \
  X(RTP_APPOINTMENT, "TERMIN") \
  X(RTP_PURCHASE, "KUPNJA") \
  X(DAY_SUNDAY, "NEDJELJA") \
  X(DAY_MONDAY, "PONEDJELJAK") \
  X(DAY_TUESDAY, "UTORAK") \
  X(DAY_WEDNESDAY, "SRIJEDA") \
  X(DAY_THURSDAY, "ČETVRTAK") \
  X(DAY_FRIDAY, "PETAK") \
  X(DAY_SATURDAY, "SUBOTA") \
  X(MONTH_JANUARY, "siječnja") \
  X(MONTH_FEBRUARY, "veljače") \
  X(MONTH_MARCH, "ožujka") \
  X(MONTH_APRIL, "travnja") \
  X(MONTH_MAY, "svibnja") \
  X(MONTH_JUNE, "lipnja") \
  X(MONTH_JULY, "srpnja") \
  X(MONTH_AUGUST, "kolovoza") \
  X(MONTH_SEPTEMBER, "rujna") \
  X(MONTH_OCTOBER, "listopada") \
  X(MONTH_NOVEMBER, "studenoga") \
  X(MONTH_DECEMBER, "prosinca") \
  X(DATE_SUFFIX_TH, ".") \
  X(DATE_SUFFIX_ST, ".") \
  X(DATE_SUFFIX_ND, ".") \
  X(DATE_SUFFIX_RD, ".") \
  X(DATE_TODAY, "Danas") \
  X(DATE_YESTERDAY, "Jučer") \
  X(DATE_YDAY, "JUČER") \
  X(DATE_FMT_WHEN_DATE, "%d. %s %s") \
  X(MONTH_SHORT_JAN, "sij") \
  X(MONTH_SHORT_FEB, "velj") \
  X(MONTH_SHORT_MAR, "ožu") \
  X(MONTH_SHORT_APR, "tra") \
  X(MONTH_SHORT_MAY, "svi") \
  X(MONTH_SHORT_JUN, "lip") \
  X(MONTH_SHORT_JUL, "srp") \
  X(MONTH_SHORT_AUG, "kol") \
  X(MONTH_SHORT_SEP, "ruj") \
  X(MONTH_SHORT_OCT, "lis") \
  X(MONTH_SHORT_NOV, "stu") \
  X(MONTH_SHORT_DEC, "pro") \
  X(MONTH_TAG_JAN, "SIJ") \
  X(MONTH_TAG_FEB, "VELJ") \
  X(MONTH_TAG_MAR, "OŽU") \
  X(MONTH_TAG_APR, "TRA") \
  X(MONTH_TAG_MAY, "SVI") \
  X(MONTH_TAG_JUN, "LIP") \
  X(MONTH_TAG_JUL, "SRP") \
  X(MONTH_TAG_AUG, "KOL") \
  X(MONTH_TAG_SEP, "RUJ") \
  X(MONTH_TAG_OCT, "LIS") \
  X(MONTH_TAG_NOV, "STU") \
  X(MONTH_TAG_DEC, "PRO") \
  X(DX_NOW, "sada") \
  X(DX_FMT_MIN_AGO, "prije %u min") \
  X(DX_FMT_H_AGO, "prije %u h") \
  X(DX_FMT_D_AGO, "prije %u d") \
  X(TOUCH_CAL_TITLE, "Kalibracija dodira") \
  X(TOUCH_CAL_HOLD, "Držite krug dok se ne ispuni") \
  X(TOUCH_CAL_CANCEL_HINT, "Okrenite ili pritisnite gumb za prekid") \
  X(TOUCH_CAL_CHECK, "Provjera") \
  X(TOUCH_CAL_TAP_DOT, "Dodirnite točku") \
  X(TOUCH_CAL_SAVED, "Kalibracija spremljena") \
  X(TOUCH_CAL_FMT_LANDED, "Provjera: odstupanje %u px") \
  X(TOUCH_CAL_BACK_HINT, "Pritisnite gumb za povratak") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Nije spremljeno") \
  X(TOUCH_CAL_FMT_MISSED, "Provjera: odstupanje %u px,") \
  X(TOUCH_CAL_NO_FIT, "Iz oznaka nije dobivena kalibracija,") \
  X(TOUCH_CAL_NOT_SAVED, "Zapis nije uspio,") \
  X(TOUCH_CAL_OLD_STAYS, "pa ostaje stara kalibracija") \
  X(TOUCH_CAL_RETRY_HINT, "Pritisnite gumb: ponovno, okrenite: izlaz")
// clang-format on
