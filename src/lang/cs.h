/*
 * The panel's texts in Czech. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_CS(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Sken stanic FM") \
  X(MENU_SCAN_MW, "Sken stanic AM") \
  X(MENU_SCAN_SW, "Sken stanic SW") \
  X(MENU_SCAN_LW, "Sken stanic LW") \
  X(MENU_SQUELCH, "Režim SQL") \
  X(MENU_SQUELCH_LEVEL, "Úroveň SQL") \
  X(MENU_SQUELCH_FLOOR, "Minimum SQL") \
  X(MENU_VOLUME_AGC, "AGC hlasitosti") \
  X(MENU_AGC_BOOST, "Zesílení AGC") \
  X(MENU_MUTE_RAMP, "Rampa ztlumení") \
  X(MENU_BAND_PLAN, "Rozsah pásma") \
  X(MENU_TUNING_STEP, "Krok ladění") \
  X(MENU_SEEK_SENSITIVITY, "Citlivost hledání") \
  X(MENU_DE_EMPHASIS, "Deemfáze") \
  X(MENU_FORCED_MONO, "Vždy mono") \
  X(MENU_STEREO_BLEND, "Prolínání sterea") \
  X(MENU_ST_HI_BLEND, "Prolínání sterea/výšek") \
  X(MENU_HIGH_CUT, "Útlum výšek") \
  X(MENU_NOISE_BLANKER, "Tlumič šumu") \
  X(MENU_EQUALISER, "Ekvalizér") \
  X(COMMON_RDS_DECODER, "Dekodér RDS") \
  X(MENU_RDS_REGION, "Region RDS") \
  X(MENU_MW_SPACING, "Krok MW") \
  X(MENU_FILTER_WIDTH, "Šířka filtru") \
  X(MENU_SOFT_MUTE, "Softmute") \
  X(MENU_DWELL, "Pauza na kanálu") \
  X(MENU_STOP_ON, "Zastavení") \
  X(MENU_SCAN, "Rozsah skenu") \
  X(MENU_MEMORY_FROM, "Předvolba od") \
  X(MENU_MEMORY_TO, "Předvolba do") \
  X(MENU_PRESET_RANGE, "Rozsah předvoleb") \
  X(MENU_LOOP_THE_BAND, "Pásmo dokola") \
  X(MENU_DX_WIDTH, "Šířka skenu") \
  X(MENU_MUTE_WHILE_SCANNING, "Ztlumit při skenu") \
  X(MENU_AUTO_LOG_NEW, "Autozápis stanic") \
  X(MENU_LOG_RADIO_TEXT, "Zápis radiotextu") \
  X(MENU_WATCH_PRESETS, "Hlídat předvolby") \
  X(MENU_SCOPE_BASELINE, "Reference spektra") \
  X(MENU_MEDIAN, "Medián") \
  X(MENU_FMT_SWEEP_N, "Snímek %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Naučit místní") \
  X(MENU_START_SCAN, "Spustit sken") \
  X(MENU_DAY_THEME, "Denní motiv") \
  X(MENU_NIGHT_THEME, "Noční motiv") \
  X(MENU_THEME, "Motiv") \
  X(MENU_BRIGHTNESS, "Jas") \
  X(MENU_DIM_LEVEL, "Jas po ztmavení") \
  X(MENU_DIM_AFTER, "Ztmavit po") \
  X(MENU_ROTATION, "Otočení displeje") \
  X(COMMON_BATTERY, "Baterie") \
  X(MENU_BATTERY_CHECK, "Kontrola baterie") \
  X(MENU_LANGUAGE, "Jazyk \xC2\xB7 Language") \
  X(MENU_ENCODER, "Typ enkodéru") \
  X(MENU_DIRECTION, "Směr enkodéru") \
  X(MENU_ENCODER_GROUP, "Enkodér") \
  X(MENU_KEY_BEEPS, "Tóny tlačítek") \
  X(MENU_BAND_EDGE_BEEP, "Tón konce pásma") \
  X(MENU_START_CHIME, "Tón při startu") \
  X(MENU_KEYPAD_TIMEOUT, "Čas zadání") \
  X(MENU_TOUCH, "Dotyk") \
  X(MENU_CALIBRATE_TOUCH, "Kalibrace dotyku") \
  X(MENU_CLOCK_FROM_NETWORK, "Čas ze sítě") \
  X(MENU_WEB_PIN, "Webový PIN") \
  X(MENU_NEW_PIN, "Nový PIN") \
  X(MENU_FMT_DIGIT_OF, "Číslice %u z %u") \
  X(MENU_STATUS, "Stav připojení") \
  X(MENU_WEB_SERVER, "Webový server") \
  X(MENU_FMT_PC_CONNECTED, "Připojeno: %u") \
  X(MENU_WEB_ADDRESS, "Webová adresa") \
  X(MENU_IP_ADDRESS, "IP adresa") \
  X(MENU_WI_FI_NAME, "Síť Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Název hotspotu") \
  X(MENU_MAC, "MAC adresa") \
  X(MENU_WI_FI_SIGNAL, "Signál Wi-Fi") \
  X(MENU_NETWORK_INFO, "Info o síti") \
  X(MENU_RESTART, "Restart rádia") \
  X(MENU_AUTO_OFF, "Auto vypnutí") \
  X(MENU_UPDATE_CHECK, "Hledat aktualizace") \
  X(MENU_FIRMWARE_UPDATE, "Aktualizace") \
  X(MENU_FMT_UPDATE_TO, "Aktualizovat na %s") \
  X(MENU_UPDATE_TITLE, "Dostupná aktualizace") \
  X(MENU_UPDATE_THIS_RADIO, "Toto rádio") \
  X(MENU_UPDATE_NEW_VERSION, "Nová verze") \
  X(MENU_UPDATE_DOWNLOAD, "Velikost") \
  X(COMMON_SETTINGS, "Nastavení") \
  X(COMMON_KEPT, "Zůstane") \
  X(MENU_STATION, "Stanice") \
  X(MENU_UPDATE_NOW, "Aktualizovat") \
  X(MENU_LATER, "Později") \
  X(MENU_UPDATE_UP_TO_DATE, "Aktuální") \
  X(MENU_UPDATE_CHECKING, "Kontroluji") \
  X(MENU_UPDATE_NOT_CHECKED, "Neověřeno") \
  X(MENU_UPDATE_CHECK_FAILED, "Kontrola selhala") \
  X(MENU_NOTE_NO_UPDATE, "Žádná aktualizace") \
  X(MENU_VERSION, "Verze firmwaru") \
  X(MENU_RUNNING_FROM, "Spuštěno z") \
  X(MENU_UPTIME, "Doba běhu") \
  X(MENU_BATTERY_VOLTAGE, "Napětí baterie") \
  X(MENU_RESET_REASON, "Důvod resetu") \
  X(MENU_CPU_CORE_0, "Jádro CPU 0") \
  X(MENU_CPU_CORE_1, "Jádro CPU 1") \
  X(MENU_FREE_HEAP, "Volná halda") \
  X(MENU_LOWEST_HEAP, "Min. halda") \
  X(MENU_LARGEST_BLOCK, "Max. blok") \
  X(MENU_LVGL_POOL, "Paměť LVGL") \
  X(MENU_CHIP, "Čip") \
  X(MENU_FLASH_SIZE, "Velikost flash") \
  X(RESET_POWER, "Zapnutí") \
  X(RESET_PANIC, "Panika") \
  X(RESET_INT_WATCHDOG, "Watchdog IRQ") \
  X(RESET_TASK_WATCHDOG, "Watchdog úlohy") \
  X(RESET_DEEP_SLEEP, "Hluboký spánek") \
  X(RESET_BROWNOUT, "Podpětí") \
  X(RESET_PIN, "Pin reset") \
  X(RESET_UNKNOWN, "Neznámý") \
  X(RESET_ASKED, "Na žádost") \
  X(RESET_BOOT_WATCHDOG, "Watchdog startu") \
  X(RESET_ROLLBACK, "Vrácení verze") \
  X(RESET_DISPLAY, "Chyba displeje") \
  X(RESET_UPDATE, "Aktualizace") \
  X(MENU_DEVELOPER, "Vývojář") \
  X(MENU_BUILD, "Sestavení") \
  X(MENU_LICENSE, "Licence") \
  X(ABOUT_BUILD_UNKNOWN, "neznámé") \
  X(MENU_GO_TO, "Přejít") \
  X(MENU_GO_TO_BANDWIDTH, "Šířka pásma") \
  X(MENU_GO_TO_DX, "Režim DX") \
  X(MENU_GO_TO_SCOPE, "Spektrum pásma") \
  X(MENU_GO_TO_BAND, "Další pásmo") \
  X(MENU_GO_TO_LOG, "Zapsat do deníku") \
  X(MENU_GO_TO_SAVE, "Uložit stanici") \
  X(MENU_FMT_SAVED_AS, "Uloženo na P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Už na P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Předvolby jsou plné") \
  X(MENU_GO_TO_SLEEP, "Spánek") \
  X(MENU_STATIONS, "Stanice") \
  X(MENU_MEMORY, "Předvolby") \
  X(MENU_AUDIO, "Zvuk") \
  X(MENU_FM_SETUP, "Příjem FM") \
  X(MENU_AM_SETUP, "Příjem AM") \
  X(MENU_DX_SETUP, "DX skener") \
  X(MENU_DISPLAY, "Displej") \
  X(MENU_CONTROLS, "Ovládání") \
  X(MENU_CONNECTIVITY, "Připojení") \
  X(MENU_SYSTEM, "Systém") \
  X(MENU_DIAGNOSTICS, "Diagnostika") \
  X(MENU_ABOUT, "O rádiu") \
  X(MENU_REGION_FULL, "Plný") \
  X(MENU_REGION_JAPAN, "Japonsko") \
  X(MENU_REGION_WIDE, "Široký") \
  X(MENU_REGION_WORLD, "Celosvětový") \
  X(MENU_RDS_REGION_EUROPE, "Evropa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Sev. Amerika") \
  X(MENU_WIFI_JOINED, "Připojeno") \
  X(MENU_WIFI_JOINING, "Připojování") \
  X(COMMON_OFF, "Vyp") \
  X(MENU_FMT_KB_OF_KB, "%u z %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V při startu") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, před %u min") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, před %u h") \
  X(MENU_FMT_AGC_GAIN_NOW, "Zisk nyní %+d dB") \
  X(MENU_STATION_LOG, "Deník stanic") \
  X(COMMON_NONE, "Žádný") \
  X(MENU_FMT_SCAN_PROGRESS, "Sken %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Uloženo: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Stop, uloženo: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Plno, neuloženo: %u") \
  X(COMMON_YES, "Ano") \
  X(COMMON_NO, "Ne") \
  X(MENU_UNKNOWN_VALUE, "Neznámý") \
  X(COMMON_ROTATION_NORMAL, "Normální") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Vzhůru nohama") \
  X(MENU_CUT_ONLY, "Jen útlum") \
  X(MENU_NEVER, "Nikdy") \
  X(MENU_BATTERY_PER_CENT, "Procenta") \
  X(MENU_BATTERY_VOLTS, "Volty") \
  X(MENU_KEYS, "Jen klávesnice") \
  X(MENU_KEYS_LONG, "Krátký i dlouhý stisk") \
  X(MENU_EVERY_PRESS, "Každý stisk") \
  X(MENU_STANDARD, "Standardní") \
  X(MENU_OPTICAL, "Optický") \
  X(MENU_NORMAL, "Normální") \
  X(MENU_REVERSED, "Obrácený") \
  X(MENU_SQUELCH_MAN, "Ruční") \
  X(MENU_ANY_PI, "Jakýkoli PI") \
  X(MENU_STOP_NEVER, "Nikdy") \
  X(MENU_NEW_ONLY, "Jen nové stanice") \
  X(MENU_WHOLE_BAND, "Celé pásmo") \
  X(MENU_MEMORY_ONLY, "Jen předvolby") \
  X(MENU_BAND_MEMORY, "Pásmo + předvolby") \
  X(COMMON_ON, "Zap") \
  X(MENU_HINT_RESTART, "Platí po restartu") \
  X(COMMON_FM_ONLY, "Jen FM") \
  X(MENU_NOTE_OFF_PLAN, "Mimo rozsah pásma") \
  X(MENU_NOTE_SWITCH_TO_FM, "Přepněte na FM") \
  X(MENU_NOTE_RADIO_BUSY, "Pracuji - zkuste znovu") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Zapněte RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Zkušební verze - čekejte") \
  X(MENU_NOTE_NOT_STORED, "Neuloženo - obnoveno") \
  X(MENU_NOTE_NOT_PUT_BACK, "Pracuji - neobnoveno") \
  X(MENU_NOTE_NOT_SAVED, "Neuloženo") \
  X(MENU_SAVE, "Uložit") \
  X(RADIO_PRESS_TO_START, "Stiskem spustit") \
  X(COMMON_STILL_TUNING, "Ladím - čekejte") \
  X(COMMON_SCAN_RUNNING, "Skenuji - čekejte") \
  X(RADIO_FMT_NO_BAND, "%s je mimo pásma") \
  X(COMMON_NOT_LOGGED, "Nezapsáno") \
  X(RADIO_FMT_LOGGED, "Zapsáno %s") \
  X(RADIO_CHECKING_UPDATES, "HLEDÁM AKTUALIZACE\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Skenuji %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Aktualizace selhala") \
  X(RADIO_GOING_TO_SLEEP, "Přechod do spánku") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Probuzení stiskem knoflíku") \
  X(RADIO_OLD_IMAGE_KEPT, "Původní firmware zachován") \
  X(RDS_SPEECH, "Řeč") \
  X(RDS_MUSIC, "Hudba") \
  X(COMMON_ALREADY_LOGGED, "Už v deníku") \
  X(DX_RDS_IS_OFF, "RDS vypnuto") \
  X(DX_SCANNING, "Skenuji") \
  X(DX_SCAN_FINISHED, "Sken dokončen") \
  X(DX_NOTHING_TO_SCAN, "Není co skenovat") \
  X(DX_CHECKING_FOR_UPDATES, "Hledám aktualizace") \
  X(DX_A_SCAN_IS_RUNNING, "Sken už běží") \
  X(DX_CAUGHT_LIST_UNREAD, "Úlovky nedostupné") \
  X(DX_NO_MEMORY, "Žádné předvolby") \
  X(DX_NOT_STARTED, "Nespuštěno") \
  X(DX_SWEEPING, "Snímám") \
  X(COMMON_FMT_TUNE_TYPED, "Naladit %s") \
  X(RADIO_SQL_OFF, "VYP") \
  X(RADIO_TUNE_MANUAL, "RUČ") \
  X(DX_FMT_ROWS_OF, "%u-%u z %u") \
  X(DX_FMT_FOUND, "Nalezeno: %u") \
  X(DX_MODE_SWEEPING, "SNÍMÁNÍ") \
  X(DX_MODE_LEARN_LOCALS, "UČÍM MÍSTNÍ") \
  X(DX_FMT_MODE_MEMORY, "PŘEDV. %u-%u") \
  X(DX_MODE_WHOLE_BAND, "CELÉ PÁSMO") \
  X(DX_MODE_BAND_MEMORY, "PÁSMO + PŘEDV.") \
  X(DX_RULE_NO_STOP, "NONSTOP") \
  X(DX_RULE_STOP_ON_PI, "STOP U PI") \
  X(DX_RULE_STOP_ON_NEW, "STOP U NOVÉ") \
  X(DX_FMT_NOT_PRESET, "ne P%02d") \
  X(DX_CONTEXT_SWEEPING, "snímání") \
  X(DX_PRESS_TO_SWEEP, "Stiskem nasnímat") \
  X(SCOPE_FMT_TITLE, "Spektrum %s") \
  X(SCOPE_FULL, "Celé") \
  X(DX_FIXED, "FIX") \
  X(DX_FMT_MEDIAN_OF, "MEDIÁN Z %u") \
  X(DX_FMT_FLOOR, "ŠUM %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Zavřít") \
  X(RECOVERY_PRESS_AGAIN, "Stiskněte znovu") \
  X(PAGE_TUNE, "Naladit") \
  X(PAGE_DELETE, "Smazat") \
  X(PAGE_LEVEL, "Síla") \
  X(PAGE_NOISE, "Šum") \
  X(PAGE_MULTIPATH, "Odrazy") \
  X(PAGE_COCHANNEL, "Soukanál") \
  X(PAGE_WIDTH, "Šířka") \
  X(PAGE_BAND, "Pásmo") \
  X(PAGE_NOTE_CHANGED, "Změněno od otevření") \
  X(PAGE_NOTE_NOT_DELETED, "Nesmazáno") \
  X(PAGE_FMT_SAVED, "Uloženo %s") \
  X(PAGE_TIME_UNKNOWN, "Čas neznámý") \
  X(BW_TITLE, "Šířka pásma") \
  X(KEYPAD_TITLE, "Frekvence") \
  X(KEYPAD_CANCEL, "Zrušit") \
  X(DX_CATCHES_EMPTY_TITLE, "Zatím žádné úlovky") \
  X(DX_CATCHES_EMPTY_NOTE, "Zde budou potvrzené stanice této relace") \
  X(DX_TITLE_CATCHES, "Úlovky") \
  X(DX_NEW, "NOVÁ") \
  X(DX_READY, "Připraveno") \
  X(DX_DWELL, "PAUZA") \
  X(DX_STOPPED, "ZASTAVENO") \
  X(DX_TITLE_SCANNER, "Skener") \
  X(DX_TITLE_SCOPE, "Spektrum") \
  X(DX_LEVEL, "SÍLA") \
  X(DX_RISE, "RŮST") \
  X(DX_NO_STATION_YET, "Zatím žádná stanice") \
  X(DX_NO_RDS_YET, "Zatím bez RDS") \
  X(DX_NO_ID, "BEZ ID") \
  X(RDS_TITLE_TEXT, "Radiotext") \
  X(RDS_TITLE_NETWORKS, "Sítě") \
  X(RDS_TITLE_DECODER, "Dekodér") \
  X(RDS_NO_COUNTRY, "ECC nepřijato") \
  X(RDS_MS_UNKNOWN, "H/Ř") \
  X(RDS_RTPLUS_NONE, "Od této stanice žádné RT+") \
  X(RDS_RTPLUS_WAIT, "Zatím žádné značky RT+ k textu") \
  X(RDS_AF_TITLE, "ALTERNATIVNÍ FREKVENCE") \
  X(RDS_EON_TITLE, "DALŠÍ STANICE, EON") \
  X(RDS_NONE_HEARD, "Nic nepřijato") \
  X(RDS_GROUPS_S_CAPS, "SKUPIN/S") \
  X(RDS_LOST, "ZTRÁTY") \
  X(RDS_EACH_BLOCK, "PO BLOCÍCH") \
  X(RDS_LEGEND_CLEAN, "čisté") \
  X(RDS_LEGEND_FIXED, "opravené") \
  X(RDS_LEGEND_LOST, "ztracené") \
  X(RDS_ALL_CLEAN, "vše čisté") \
  X(RDS_GROUP_TYPES, "TYPY SKUPIN") \
  X(RDS_NO_GROUPS, "Za minutu žádné skupiny") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 za %u s") \
  X(RDS_COUNTRY_UNLISTED, "neuvedeno") \
  X(RDS_NO_TYPES, "Za minutu žádný čistý blok B") \
  X(RDS_FMT_EON_MORE, "+%u další") \
  X(RDS_EON_MORE_HEARD, "+ další") \
  X(RDS_MORE, "další") \
  X(RADIO_VEIL_UPDATING, "AKTUALIZACE FIRMWARU") \
  X(RADIO_VEIL_KEEP_POWER, "Neodpojujte napájení") \
  X(RADIO_MUTE, "TICHO") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Tuner nenalezen na sběrnici I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tuner není připraven") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Identifikace tuneru selhala") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Pro tento tuner chybí patch") \
  X(TUNER_UNKNOWN_TUNER_PART, "Neznámý tuner") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tuner odmítl zápis") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tuner neodpověděl na čtení") \
  X(TUNER_OUT_OF_RANGE, "Mimo rozsah") \
  X(TUNER_UNKNOWN, "Neznámý") \
  X(BAND_LONG_LW, "Dlouhé vlny") \
  X(BAND_LONG_MW, "Střední vlny") \
  X(BAND_LONG_SW, "Krátké vlny") \
  X(MENU_FMT_TUNE_TO, "Pásmo pro %s") \
  X(PTY_NONE, "Žádný") \
  X(PTY_NEWS, "Zprávy") \
  X(PTY_CURRENT_AFFAIRS, "Publicistika") \
  X(PTY_INFORMATION, "Informace") \
  X(PTY_EDUCATION, "Vzdělávání") \
  X(PTY_CULTURE, "Kultura") \
  X(PTY_SCIENCE, "Věda") \
  X(PTY_VARIED, "Různé") \
  X(PTY_POP_MUSIC, "Popová hudba") \
  X(PTY_ROCK_MUSIC, "Rocková hudba") \
  X(PTY_EASY_LISTENING, "Oddychová hudba") \
  X(PTY_LIGHT_CLASSICAL, "Lehká klasika") \
  X(PTY_SERIOUS_CLASSICAL, "Vážná hudba") \
  X(PTY_OTHER_MUSIC, "Jiná hudba") \
  X(PTY_WEATHER, "Počasí") \
  X(PTY_CHILDREN, "Pro děti") \
  X(PTY_SOCIAL_AFFAIRS, "Společnost") \
  X(PTY_RELIGION, "Náboženství") \
  X(PTY_PHONE_IN, "Telefonáty") \
  X(PTY_TRAVEL, "Cestování") \
  X(PTY_LEISURE, "Volný čas") \
  X(PTY_COUNTRY_MUSIC, "Country") \
  X(PTY_NATIONAL_MUSIC, "Národní hudba") \
  X(PTY_FOLK_MUSIC, "Lidová hudba") \
  X(PTY_DOCUMENTARY, "Dokumenty") \
  X(PTY_ALARM_TEST, "Test poplachu") \
  X(PTY_ALARM, "Poplach") \
  X(PTY_NA_NEWS, "Zprávy") \
  X(PTY_NA_INFORMATION, "Informace") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_TALK, "Mluvené slovo") \
  X(PTY_NA_CLASSIC_ROCK, "Klasický rock") \
  X(PTY_NA_ADULT_HITS, "Hity pro dospělé") \
  X(PTY_NA_SOFT_ROCK, "Soft rock") \
  X(PTY_NA_NOSTALGIA, "Nostalgie") \
  X(PTY_NA_CLASSICAL, "Vážná hudba") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Rhythm and blues") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Soft rhythm and blues") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Cizí jazyk") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Duchovní hudba") \
  X(PTY_NA_RELIGIOUS_TALK, "Duchovní slovo") \
  X(PTY_NA_PERSONALITY, "Osobnosti") \
  X(PTY_NA_PUBLIC, "Veřejné") \
  X(PTY_NA_COLLEGE, "Studentské") \
  X(PTY_NA_SPANISH_TALK, "Španělské slovo") \
  X(PTY_NA_SPANISH_MUSIC, "Španělská hudba") \
  X(PTY_NA_HIP_HOP, "Hip-hop") \
  X(PTY_NA_UNASSIGNED, "Nepřiřazeno") \
  X(PTY_NA_WEATHER, "Počasí") \
  X(PTY_NA_EMERGENCY_TEST, "Test poplachu") \
  X(PTY_NA_EMERGENCY, "Poplach") \
  X(COVERAGE_LOCAL, "Místní") \
  X(COVERAGE_INTERNATIONAL, "Mezinárodní") \
  X(COVERAGE_NATIONAL, "Celostátní") \
  X(COVERAGE_SUPRA_REGIONAL, "Nadregionální") \
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
  X(RTP_OTHER, "JINÉ") \
  X(RTP_TITLE, "NÁZEV") \
  X(RTP_TRACK, "STOPA") \
  X(RTP_ARTIST, "INTERPRET") \
  X(RTP_WORK, "DÍLO") \
  X(RTP_MOVEMENT, "VĚTA") \
  X(RTP_CONDUCTOR, "DIRIGENT") \
  X(RTP_COMPOSER, "SKLADATEL") \
  X(RTP_BAND, "KAPELA") \
  X(RTP_COMMENT, "KOMENTÁŘ") \
  X(RTP_GENRE, "ŽÁNR") \
  X(RTP_NEWS, "ZPRÁVY") \
  X(RTP_LOCAL, "MÍST. ZPRÁVY") \
  X(RTP_STOCKS, "BURZA") \
  X(RTP_LOTTERY, "LOTERIE") \
  X(RTP_HOROSCOPE, "HOROSKOP") \
  X(RTP_DIVERSION, "ZÁBAVA") \
  X(RTP_HEALTH, "ZDRAVÍ") \
  X(RTP_EVENT, "UDÁLOST") \
  X(RTP_SCENE, "SCÉNA") \
  X(RTP_CINEMA, "KINO") \
  X(RTP_FUN, "HUMOR") \
  X(RTP_DATE, "DATUM") \
  X(RTP_WEATHER, "POČASÍ") \
  X(RTP_TRAFFIC, "DOPRAVA") \
  X(RTP_ALARM, "POPLACH") \
  X(RTP_ADVERT, "REKLAMA") \
  X(RTP_STATION, "STANICE") \
  X(RTP_PROGRAM, "POŘAD") \
  X(RTP_NEXT, "DALŠÍ") \
  X(RTP_PART, "DÍL") \
  X(RTP_HOST, "MODERÁTOR") \
  X(RTP_EDITORS, "REDAKCE") \
  X(RTP_FREQUENCY, "FREKVENCE") \
  X(RTP_HOMEPAGE, "WEB") \
  X(RTP_CHANNEL, "KANÁL") \
  X(RTP_PHONE, "TELEFON") \
  X(RTP_VOTE, "ANKETA") \
  X(RTP_PLACE, "MÍSTO") \
  X(RTP_APPOINTMENT, "SCHŮZKA") \
  X(RTP_PURCHASE, "NÁKUP") \
  X(DAY_SUNDAY, "NEDĚLE") \
  X(DAY_MONDAY, "PONDĚLÍ") \
  X(DAY_TUESDAY, "ÚTERÝ") \
  X(DAY_WEDNESDAY, "STŘEDA") \
  X(DAY_THURSDAY, "ČTVRTEK") \
  X(DAY_FRIDAY, "PÁTEK") \
  X(DAY_SATURDAY, "SOBOTA") \
  X(MONTH_JANUARY, "ledna") \
  X(MONTH_FEBRUARY, "února") \
  X(MONTH_MARCH, "března") \
  X(MONTH_APRIL, "dubna") \
  X(MONTH_MAY, "května") \
  X(MONTH_JUNE, "června") \
  X(MONTH_JULY, "července") \
  X(MONTH_AUGUST, "srpna") \
  X(MONTH_SEPTEMBER, "září") \
  X(MONTH_OCTOBER, "října") \
  X(MONTH_NOVEMBER, "listopadu") \
  X(MONTH_DECEMBER, "prosince") \
  X(DATE_SUFFIX_TH, ".") \
  X(DATE_SUFFIX_ST, ".") \
  X(DATE_SUFFIX_ND, ".") \
  X(DATE_SUFFIX_RD, ".") \
  X(DATE_TODAY, "Dnes") \
  X(DATE_YESTERDAY, "Včera") \
  X(DATE_YDAY, "VČERA") \
  X(DATE_FMT_WHEN_DATE, "%d. %s %s") \
  X(MONTH_SHORT_JAN, "led") \
  X(MONTH_SHORT_FEB, "úno") \
  X(MONTH_SHORT_MAR, "bře") \
  X(MONTH_SHORT_APR, "dub") \
  X(MONTH_SHORT_MAY, "kvě") \
  X(MONTH_SHORT_JUN, "čvn") \
  X(MONTH_SHORT_JUL, "čvc") \
  X(MONTH_SHORT_AUG, "srp") \
  X(MONTH_SHORT_SEP, "zář") \
  X(MONTH_SHORT_OCT, "říj") \
  X(MONTH_SHORT_NOV, "lis") \
  X(MONTH_SHORT_DEC, "pro") \
  X(MONTH_TAG_JAN, "LED") \
  X(MONTH_TAG_FEB, "ÚNO") \
  X(MONTH_TAG_MAR, "BŘE") \
  X(MONTH_TAG_APR, "DUB") \
  X(MONTH_TAG_MAY, "KVĚ") \
  X(MONTH_TAG_JUN, "ČVN") \
  X(MONTH_TAG_JUL, "ČVC") \
  X(MONTH_TAG_AUG, "SRP") \
  X(MONTH_TAG_SEP, "ZÁŘ") \
  X(MONTH_TAG_OCT, "ŘÍJ") \
  X(MONTH_TAG_NOV, "LIS") \
  X(MONTH_TAG_DEC, "PRO") \
  X(DX_NOW, "nyní") \
  X(DX_FMT_MIN_AGO, "před %u min") \
  X(DX_FMT_H_AGO, "před %u h") \
  X(DX_FMT_D_AGO, "před %u d") \
  X(TOUCH_CAL_TITLE, "Kalibrace dotyku") \
  X(TOUCH_CAL_HOLD, "Držte kruh, dokud se nezaplní") \
  X(TOUCH_CAL_CANCEL_HINT, "Otočte/stiskněte knoflík - zrušit") \
  X(TOUCH_CAL_CHECK, "Kontrola") \
  X(TOUCH_CAL_TAP_DOT, "Klepněte na tečku") \
  X(TOUCH_CAL_SAVED, "Kalibrace uložena") \
  X(TOUCH_CAL_FMT_LANDED, "Kontrola: odchylka %u px") \
  X(TOUCH_CAL_BACK_HINT, "Stiskem knoflíku zpět") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Neuloženo") \
  X(TOUCH_CAL_FMT_MISSED, "Kontrola: odchylka %u px,") \
  X(TOUCH_CAL_NO_FIT, "Ze značek nelze kalibrovat,") \
  X(TOUCH_CAL_NOT_SAVED, "Kalibraci nelze zapsat,") \
  X(TOUCH_CAL_OLD_STAYS, "proto zůstává stará kalibrace") \
  X(TOUCH_CAL_RETRY_HINT, "Stiskem knoflíku znovu, otočením konec")
// clang-format on
