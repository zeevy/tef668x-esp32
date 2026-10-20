/*
 * The panel's texts in Polish. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_PL(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Skanowanie FM") \
  X(MENU_SCAN_MW, "Skanowanie AM") \
  X(MENU_SCAN_SW, "Skanowanie SW") \
  X(MENU_SCAN_LW, "Skanowanie LW") \
  X(MENU_SQUELCH, "Tryb blokady") \
  X(MENU_SQUELCH_LEVEL, "Poziom blokady") \
  X(MENU_SQUELCH_FLOOR, "Dolny próg") \
  X(MENU_SQUELCH_GROUP, "Blokada szumów") \
  X(MENU_VOLUME_AGC, "AGC głośności") \
  X(MENU_AGC_BOOST, "Podbicie AGC") \
  X(MENU_MUTE_RAMP, "Czas wyciszania") \
  X(MENU_BAND_PLAN, "Plan pasma") \
  X(MENU_TUNING_STEP, "Krok strojenia") \
  X(MENU_SEEK_SENSITIVITY, "Czułość szukania") \
  X(MENU_DE_EMPHASIS, "Deemfaza") \
  X(MENU_FORCED_MONO, "Wymuś mono") \
  X(MENU_STEREO_BLEND, "Mieszanie stereo") \
  X(MENU_ST_HI_BLEND, "Mieszanie stereo/wys.") \
  X(MENU_HIGH_CUT, "Cięcie wysokich") \
  X(MENU_NOISE_BLANKER, "Wygaszacz zakłóceń") \
  X(MENU_EQUALISER, "Korektor") \
  X(COMMON_RDS_DECODER, "Dekoder RDS") \
  X(MENU_RDS_REGION, "Region RDS") \
  X(MENU_MW_SPACING, "Krok MW") \
  X(MENU_FILTER_WIDTH, "Szerokość filtra") \
  X(MENU_SOFT_MUTE, "Łagodne wycisz.") \
  X(MENU_DWELL, "Czas na kanale") \
  X(MENU_STOP_ON, "Warunek stopu") \
  X(MENU_SCAN, "Zakres skanu") \
  X(MENU_MEMORY_FROM, "Pamięć od") \
  X(MENU_MEMORY_TO, "Pamięć do") \
  X(MENU_PRESET_RANGE, "Zakres pamięci") \
  X(MENU_LOOP_THE_BAND, "Zapętl pasmo") \
  X(MENU_DX_WIDTH, "Szer. pasma skanu") \
  X(MENU_MUTE_WHILE_SCANNING, "Wycisz przy skanie") \
  X(MENU_AUTO_LOG_NEW, "Nowe do dziennika") \
  X(MENU_LOG_RADIO_TEXT, "RT do dziennika") \
  X(MENU_WATCH_PRESETS, "Nadzór pamięci") \
  X(MENU_SCOPE_BASELINE, "Baza widma") \
  X(MENU_MEDIAN, "Mediana") \
  X(MENU_FMT_SWEEP_N, "Pomiar %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Zapamiętaj lokalne") \
  X(MENU_START_SCAN, "Start skanu") \
  X(MENU_DAY_THEME, "Motyw dzienny") \
  X(MENU_NIGHT_THEME, "Motyw nocny") \
  X(MENU_THEME, "Motyw") \
  X(MENU_BRIGHTNESS, "Jasność") \
  X(MENU_DIM_LEVEL, "Jasność przyciem.") \
  X(MENU_DIM_AFTER, "Przyciemnij po") \
  X(MENU_ROTATION, "Obrót ekranu") \
  X(COMMON_BATTERY, "Bateria") \
  X(MENU_BATTERY_CHECK, "Kontrola baterii") \
  X(MENU_LANGUAGE, "Język \xC2\xB7 Language") \
  X(MENU_ENCODER, "Typ enkodera") \
  X(MENU_DIRECTION, "Kierunek enkodera") \
  X(MENU_ENCODER_GROUP, "Enkoder") \
  X(MENU_KEY_BEEPS, "Dźwięk klawiszy") \
  X(MENU_BAND_EDGE_BEEP, "Dźwięk końca pasma") \
  X(MENU_START_CHIME, "Dźwięk startu") \
  X(MENU_KEYPAD_TIMEOUT, "Czas wpisywania") \
  X(MENU_TOUCH, "Dotyk") \
  X(MENU_CALIBRATE_TOUCH, "Kalibracja dotyku") \
  X(MENU_CLOCK_FROM_NETWORK, "Czas z sieci") \
  X(MENU_WEB_PIN, "PIN WWW") \
  X(MENU_NEW_PIN, "Nowy PIN") \
  X(MENU_FMT_DIGIT_OF, "Cyfra %u z %u") \
  X(MENU_STATUS, "Stan połączenia") \
  X(MENU_WEB_SERVER, "Serwer WWW") \
  X(MENU_FMT_PC_CONNECTED, "Połączone: %u") \
  X(MENU_WEB_ADDRESS, "Adres WWW") \
  X(MENU_IP_ADDRESS, "Adres IP") \
  X(MENU_WI_FI_NAME, "Sieć Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Nazwa hotspotu") \
  X(MENU_MAC, "Adres MAC") \
  X(MENU_WI_FI_SIGNAL, "Sygnał Wi-Fi") \
  X(MENU_NETWORK_INFO, "Dane sieci") \
  X(MENU_RESTART, "Restart radia") \
  X(MENU_AUTO_OFF, "Autowyłączanie") \
  X(MENU_UPDATE_CHECK, "Szukaj aktualizacji") \
  X(MENU_FIRMWARE_UPDATE, "Aktualizacja") \
  X(MENU_FMT_UPDATE_TO, "Aktualizuj do %s") \
  X(MENU_UPDATE_TITLE, "Dostępna aktualizacja") \
  X(MENU_UPDATE_THIS_RADIO, "To radio") \
  X(MENU_UPDATE_NEW_VERSION, "Nowa wersja") \
  X(MENU_UPDATE_DOWNLOAD, "Do pobrania") \
  X(COMMON_SETTINGS, "Ustawienia") \
  X(COMMON_KEPT, "Zachowane") \
  X(MENU_STATION, "Stacja") \
  X(MENU_UPDATE_NOW, "Aktualizuj") \
  X(MENU_LATER, "Później") \
  X(MENU_UPDATE_UP_TO_DATE, "Aktualne") \
  X(MENU_UPDATE_CHECKING, "Sprawdzanie") \
  X(MENU_UPDATE_NOT_CHECKED, "Nie sprawdzono") \
  X(MENU_UPDATE_CHECK_FAILED, "Błąd sprawdzania") \
  X(MENU_NOTE_NO_UPDATE, "Brak aktualizacji") \
  X(MENU_VERSION, "Wersja oprogramowania") \
  X(MENU_RUNNING_FROM, "Źródło startu") \
  X(MENU_UPTIME, "Czas pracy") \
  X(MENU_BATTERY_VOLTAGE, "Napięcie baterii") \
  X(MENU_RESET_REASON, "Przyczyna resetu") \
  X(MENU_CPU_CORE_0, "Rdzeń CPU 0") \
  X(MENU_CPU_CORE_1, "Rdzeń CPU 1") \
  X(MENU_FREE_HEAP, "Wolna sterta") \
  X(MENU_LOWEST_HEAP, "Min. sterta") \
  X(MENU_LARGEST_BLOCK, "Największy blok") \
  X(MENU_LVGL_POOL, "Pula LVGL") \
  X(MENU_CHIP, "Układ") \
  X(MENU_FLASH_SIZE, "Rozmiar flash") \
  X(RESET_POWER, "Włączenie") \
  X(RESET_SOFTWARE, "Programowy") \
  X(RESET_PANIC, "Panika") \
  X(RESET_INT_WATCHDOG, "Watchdog IRQ") \
  X(RESET_TASK_WATCHDOG, "Watchdog zadania") \
  X(RESET_DEEP_SLEEP, "Głęboki sen") \
  X(RESET_BROWNOUT, "Spadek napięcia") \
  X(RESET_PIN, "Pin resetu") \
  X(RESET_UNKNOWN, "Nieznany") \
  X(RESET_ASKED, "Na żądanie") \
  X(RESET_BOOT_WATCHDOG, "Watchdog startu") \
  X(RESET_ROLLBACK, "Wycofanie") \
  X(RESET_DISPLAY, "Błąd ekranu") \
  X(RESET_UPDATE, "Aktualizacja") \
  X(MENU_DEVELOPER, "Autor") \
  X(MENU_BUILD, "Kompilacja") \
  X(MENU_LICENSE, "Licencja") \
  X(ABOUT_BUILD_UNKNOWN, "nieznana") \
  X(MENU_GO_TO, "Przejdź") \
  X(MENU_GO_TO_BANDWIDTH, "Szerokość pasma") \
  X(MENU_GO_TO_DX, "Tryb DX") \
  X(MENU_GO_TO_SCOPE, "Widmo pasma") \
  X(MENU_GO_TO_BAND, "Następne pasmo") \
  X(MENU_GO_TO_LOG, "Stacja do dziennika") \
  X(MENU_GO_TO_SAVE, "Stacja do pamięci") \
  X(MENU_FMT_SAVED_AS, "Zapisano w P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Już w P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Pamięć pełna") \
  X(MENU_GO_TO_SLEEP, "Uśpienie") \
  X(MENU_STATIONS, "Stacje") \
  X(MENU_MEMORY, "Pamięć") \
  X(MENU_AUDIO, "Dźwięk") \
  X(MENU_FM_SETUP, "Odbiór FM") \
  X(MENU_AM_SETUP, "Odbiór AM") \
  X(MENU_DX_SETUP, "Skaner DX") \
  X(MENU_DISPLAY, "Ekran") \
  X(MENU_CONTROLS, "Sterowanie") \
  X(MENU_CONNECTIVITY, "Łączność") \
  X(MENU_DIAGNOSTICS, "Diagnostyka") \
  X(MENU_ABOUT, "O radiu") \
  X(MENU_REGION_FULL, "Pełny") \
  X(MENU_REGION_JAPAN, "Japonia") \
  X(MENU_REGION_WIDE, "Szeroki") \
  X(MENU_REGION_WORLD, "Światowy") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Ameryka Płn.") \
  X(MENU_WIFI_JOINED, "Połączono") \
  X(MENU_WIFI_JOINING, "Łączenie") \
  X(COMMON_OFF, "Wył.") \
  X(MENU_FMT_HOURS_MINUTES, "%u h %u min") \
  X(MENU_FMT_KB_OF_KB, "%u z %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V przy starcie") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, %u min temu") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, %u h temu") \
  X(MENU_FMT_AGC_GAIN_NOW, "Wzm. teraz %+d dB") \
  X(MENU_STATION_LOG, "Dziennik stacji") \
  X(COMMON_NONE, "Brak") \
  X(MENU_FMT_SCAN_PROGRESS, "Skan %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Zapisano: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Stop, zapisano: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Pełno, bez zapisu: %u") \
  X(COMMON_YES, "Tak") \
  X(COMMON_NO, "Nie") \
  X(MENU_UNKNOWN_VALUE, "Nieznany") \
  X(COMMON_ROTATION_NORMAL, "Normalny") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Odwrócony") \
  X(MENU_CUT_ONLY, "Bez podbicia") \
  X(MENU_NEVER, "Nigdy") \
  X(MENU_BATTERY_PER_CENT, "Procent") \
  X(MENU_BATTERY_VOLTS, "Wolty") \
  X(MENU_KEYS, "Tylko klawiatura") \
  X(MENU_KEYS_LONG, "Krótkie i długie") \
  X(MENU_EVERY_PRESS, "Każde naciśnięcie") \
  X(MENU_STANDARD, "Standardowy") \
  X(MENU_OPTICAL, "Optyczny") \
  X(MENU_NORMAL, "Normalny") \
  X(MENU_REVERSED, "Odwrócony") \
  X(MENU_SQUELCH_MAN, "Ręczny") \
  X(MENU_ANY_PI, "Dowolny PI") \
  X(MENU_STOP_NEVER, "Nigdy") \
  X(MENU_NEW_ONLY, "Tylko nowe stacje") \
  X(MENU_WHOLE_BAND, "Całe pasmo") \
  X(MENU_MEMORY_ONLY, "Tylko pamięć") \
  X(MENU_BAND_MEMORY, "Pasmo + pamięć") \
  X(COMMON_ON, "Wł.") \
  X(MENU_HINT_RESTART, "Działa po restarcie") \
  X(COMMON_FM_ONLY, "Tylko FM") \
  X(MENU_NOTE_OFF_PLAN, "Poza planem pasma") \
  X(MENU_NOTE_SWITCH_TO_FM, "Najpierw włącz FM") \
  X(MENU_NOTE_RADIO_BUSY, "Radio zajęte - ponów") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Najpierw włącz RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Test aktualizacji - czekaj") \
  X(MENU_NOTE_NOT_STORED, "Nie zapisano - cofnięto") \
  X(MENU_NOTE_NOT_PUT_BACK, "Radio zajęte - nie cofnięto") \
  X(MENU_NOTE_NOT_SAVED, "Nie zapisano") \
  X(MENU_SAVE, "Zapisz") \
  X(RADIO_PRESS_TO_START, "Naciśnij: start") \
  X(COMMON_STILL_TUNING, "Strojenie - czekaj") \
  X(COMMON_SCAN_RUNNING, "Skanowanie - czekaj") \
  X(RADIO_FMT_NO_BAND, "%s poza pasmami") \
  X(COMMON_NOT_LOGGED, "Nie dodano") \
  X(RADIO_FMT_LOGGED, "Dodano %s") \
  X(RADIO_CHECKING_UPDATES, "SZUKAM AKTUALIZACJI\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Skanowanie %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Błąd aktualizacji") \
  X(RADIO_GOING_TO_SLEEP, "Usypianie") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Naciśnij pokrętło, by wybudzić") \
  X(RADIO_OLD_IMAGE_KEPT, "Stara wersja zostaje") \
  X(RDS_SPEECH, "Mowa") \
  X(RDS_MUSIC, "Muzyka") \
  X(COMMON_ALREADY_LOGGED, "Już w dzienniku") \
  X(DX_RDS_IS_OFF, "RDS wył.") \
  X(DX_SCANNING, "Skanowanie") \
  X(DX_SCAN_FINISHED, "Skan zakończony") \
  X(DX_NOTHING_TO_SCAN, "Nic do skanowania") \
  X(DX_CHECKING_FOR_UPDATES, "Szukam aktualizacji") \
  X(DX_A_SCAN_IS_RUNNING, "Skan już trwa") \
  X(DX_CAUGHT_LIST_UNREAD, "Brak listy złapanych") \
  X(DX_NO_MEMORY, "Pamięć pusta") \
  X(DX_NOT_STARTED, "Nie rozpoczęto") \
  X(DX_SWEEPING, "Trwa pomiar") \
  X(COMMON_FMT_TUNE_TYPED, "Dostrój %s") \
  X(RADIO_SQL_OFF, "WYŁ") \
  X(RADIO_TUNE_MANUAL, "RĘCZ") \
  X(RADIO_TUNE_MEMORY, "PAM") \
  X(DX_FMT_ROWS_OF, "%u-%u z %u") \
  X(DX_FMT_FOUND, "Stacje: %u") \
  X(DX_MODE_SWEEPING, "POMIAR") \
  X(DX_MODE_LEARN_LOCALS, "LOKALNE") \
  X(DX_FMT_MODE_MEMORY, "PAMIĘĆ %u-%u") \
  X(DX_MODE_WHOLE_BAND, "CAŁE PASMO") \
  X(DX_MODE_BAND_MEMORY, "PASMO + PAMIĘĆ") \
  X(DX_RULE_NO_STOP, "NON-STOP") \
  X(DX_RULE_STOP_ON_PI, "STOP: PI") \
  X(DX_RULE_STOP_ON_NEW, "STOP: NOWA") \
  X(DX_FMT_NOT_PRESET, "nie P%02d") \
  X(DX_CONTEXT_SWEEPING, "pomiar") \
  X(DX_PRESS_TO_SWEEP, "Naciśnij, by zmierzyć") \
  X(SCOPE_FMT_TITLE, "Widmo %s") \
  X(SCOPE_FULL, "Całe") \
  X(DX_FIXED, "STAŁY") \
  X(DX_FMT_MEDIAN_OF, "MEDIANA Z %u") \
  X(DX_FMT_FLOOR, "TŁO %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Zamknij") \
  X(RECOVERY_PRESS_AGAIN, "Naciśnij znów") \
  X(PAGE_TUNE, "Dostrój") \
  X(PAGE_DELETE, "Usuń") \
  X(PAGE_LEVEL, "Poziom") \
  X(PAGE_NOISE, "Szum") \
  X(PAGE_MULTIPATH, "Odbicia") \
  X(PAGE_COCHANNEL, "Współkan.") \
  X(PAGE_WIDTH, "Filtr") \
  X(PAGE_BAND, "Pasmo") \
  X(PAGE_NOTE_CHANGED, "Zmienione od otwarcia") \
  X(PAGE_NOTE_NOT_DELETED, "Nie usunięto") \
  X(PAGE_FMT_SAVED, "Zapisano %s") \
  X(PAGE_TIME_UNKNOWN, "Czas nieznany") \
  X(MENU_FMT_TUNER_PATCH, "%s łatka %u") \
  X(BW_TITLE, "Szer. pasma") \
  X(KEYPAD_TITLE, "Częstotliwość") \
  X(KEYPAD_CANCEL, "Anuluj") \
  X(DX_CATCHES_EMPTY_TITLE, "Nic nie złapano") \
  X(DX_CATCHES_EMPTY_NOTE, "Tu pojawią się stacje potwierdzone w tej sesji") \
  X(DX_TITLE_CATCHES, "Złapane") \
  X(DX_NEW, "NOWA") \
  X(DX_READY, "Gotowy") \
  X(DX_DWELL, "CZAS") \
  X(DX_STOPPED, "STOP") \
  X(DX_TITLE_SCANNER, "Skaner") \
  X(DX_TITLE_SCOPE, "Widmo") \
  X(DX_LEVEL, "POZ.") \
  X(DX_RISE, "WZR.") \
  X(DX_NO_STATION_YET, "Brak stacji") \
  X(DX_NO_RDS_YET, "Brak RDS") \
  X(DX_NO_ID, "BEZ ID") \
  X(RDS_TITLE_TEXT, "Radiotekst") \
  X(RDS_TITLE_NETWORKS, "Sieci") \
  X(RDS_TITLE_DECODER, "Dekoder") \
  X(RDS_NO_COUNTRY, "brak ECC") \
  X(RDS_RTPLUS_NONE, "Brak RT+ z tej stacji") \
  X(RDS_RTPLUS_WAIT, "Brak tagów RT+ dla tego tekstu") \
  X(RDS_AF_TITLE, "INNE CZĘSTOTLIWOŚCI") \
  X(RDS_EON_TITLE, "INNE STACJE, EON") \
  X(RDS_NONE_HEARD, "Nie odebrano") \
  X(RDS_GROUPS_S_CAPS, "GRUPY/S") \
  X(RDS_LOST, "STRATY") \
  X(RDS_EACH_BLOCK, "KAŻDY BLOK") \
  X(RDS_LEGEND_CLEAN, "czyste") \
  X(RDS_LEGEND_FIXED, "poprawione") \
  X(RDS_LEGEND_LOST, "stracone") \
  X(RDS_ALL_CLEAN, "bez błędów") \
  X(RDS_GROUP_TYPES, "TYPY GRUP") \
  X(RDS_NO_GROUPS, "Brak grup w ostatniej minucie") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 ost. %u s") \
  X(RDS_COUNTRY_UNLISTED, "spoza listy") \
  X(RDS_NO_TYPES, "Brak czystych bloków B w ostatniej minucie") \
  X(RDS_FMT_EON_MORE, "+%u więcej") \
  X(RDS_EON_MORE_HEARD, "+ więcej") \
  X(RDS_MORE, "więcej") \
  X(RADIO_VEIL_UPDATING, "AKTUALIZACJA OPROGRAMOWANIA") \
  X(RADIO_VEIL_KEEP_POWER, "Nie odłączaj zasilania") \
  X(RADIO_MUTE, "CISZA") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Brak tunera na szynie I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Tuner nie jest gotowy") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Błąd identyfikacji tunera") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Brak łatki dla tego tunera") \
  X(TUNER_UNKNOWN_TUNER_PART, "Nieznany tuner") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Tuner odrzucił zapis") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Tuner nie odpowiada na odczyt") \
  X(TUNER_OUT_OF_RANGE, "Poza zakresem") \
  X(TUNER_UNKNOWN, "Nieznany") \
  X(BAND_LONG_LW, "Fale długie") \
  X(BAND_LONG_MW, "Fale średnie") \
  X(BAND_LONG_SW, "Fale krótkie") \
  X(MENU_FMT_TUNE_TO, "Pasmo dla %s") \
  X(PTY_NONE, "Brak") \
  X(PTY_NEWS, "Wiadomości") \
  X(PTY_CURRENT_AFFAIRS, "Publicystyka") \
  X(PTY_INFORMATION, "Informacje") \
  X(PTY_EDUCATION, "Edukacja") \
  X(PTY_DRAMA, "Słuchowisko") \
  X(PTY_CULTURE, "Kultura") \
  X(PTY_SCIENCE, "Nauka") \
  X(PTY_VARIED, "Różne") \
  X(PTY_POP_MUSIC, "Muzyka pop") \
  X(PTY_ROCK_MUSIC, "Muzyka rock") \
  X(PTY_EASY_LISTENING, "Muzyka lekka") \
  X(PTY_LIGHT_CLASSICAL, "Lekka klasyka") \
  X(PTY_SERIOUS_CLASSICAL, "Klasyka") \
  X(PTY_OTHER_MUSIC, "Inna muzyka") \
  X(PTY_WEATHER, "Pogoda") \
  X(PTY_FINANCE, "Finanse") \
  X(PTY_CHILDREN, "Dla dzieci") \
  X(PTY_SOCIAL_AFFAIRS, "Sprawy społeczne") \
  X(PTY_RELIGION, "Religia") \
  X(PTY_PHONE_IN, "Rozmowy tel.") \
  X(PTY_TRAVEL, "Podróże") \
  X(PTY_LEISURE, "Czas wolny") \
  X(PTY_COUNTRY_MUSIC, "Muzyka country") \
  X(PTY_NATIONAL_MUSIC, "Muzyka narodowa") \
  X(PTY_OLDIES_MUSIC, "Stare przeboje") \
  X(PTY_FOLK_MUSIC, "Muzyka ludowa") \
  X(PTY_DOCUMENTARY, "Dokument") \
  X(PTY_ALARM_TEST, "Test alarmu") \
  X(PTY_NA_NEWS, "Wiadomości") \
  X(PTY_NA_INFORMATION, "Informacje") \
  X(PTY_NA_SPORTS, "Sport") \
  X(PTY_NA_TALK, "Rozmowy") \
  X(PTY_NA_CLASSIC_ROCK, "Klasyczny rock") \
  X(PTY_NA_SOFT_ROCK, "Soft rock") \
  X(PTY_NA_OLDIES, "Stare przeboje") \
  X(PTY_NA_SOFT, "Łagodna") \
  X(PTY_NA_CLASSICAL, "Klasyka") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Rhythm and blues") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Łagodny R&B") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Języki obce") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Muzyka religijna") \
  X(PTY_NA_RELIGIOUS_TALK, "Rozmowy religijne") \
  X(PTY_NA_PERSONALITY, "Osobowości") \
  X(PTY_NA_PUBLIC, "Publiczne") \
  X(PTY_NA_COLLEGE, "Studenckie") \
  X(PTY_NA_SPANISH_TALK, "Rozmowy po hiszp.") \
  X(PTY_NA_SPANISH_MUSIC, "Muzyka hiszp.") \
  X(PTY_NA_UNASSIGNED, "Nieprzypisany") \
  X(PTY_NA_WEATHER, "Pogoda") \
  X(PTY_NA_EMERGENCY_TEST, "Test alarmu") \
  X(PTY_NA_EMERGENCY, "Alarm") \
  X(COVERAGE_LOCAL, "Lokalny") \
  X(COVERAGE_INTERNATIONAL, "Międzynar.") \
  X(COVERAGE_NATIONAL, "Krajowy") \
  X(COVERAGE_SUPRA_REGIONAL, "Ponadregion.") \
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
  X(RTP_OTHER, "INNE") \
  X(RTP_TITLE, "TYTUŁ") \
  X(RTP_TRACK, "UTWÓR") \
  X(RTP_ARTIST, "WYKONAWCA") \
  X(RTP_WORK, "DZIEŁO") \
  X(RTP_MOVEMENT, "CZĘŚĆ") \
  X(RTP_CONDUCTOR, "DYRYGENT") \
  X(RTP_COMPOSER, "KOMPOZYTOR") \
  X(RTP_BAND, "ZESPÓŁ") \
  X(RTP_COMMENT, "KOMENTARZ") \
  X(RTP_GENRE, "GATUNEK") \
  X(RTP_NEWS, "WIADOMOŚCI") \
  X(RTP_LOCAL, "WIAD. LOK.") \
  X(RTP_STOCKS, "GIEŁDA") \
  X(RTP_LOTTERY, "LOTERIA") \
  X(RTP_HOROSCOPE, "HOROSKOP") \
  X(RTP_DIVERSION, "ROZRYWKA") \
  X(RTP_HEALTH, "ZDROWIE") \
  X(RTP_EVENT, "WYDARZENIE") \
  X(RTP_SCENE, "SCENA") \
  X(RTP_CINEMA, "KINO") \
  X(RTP_FUN, "HUMOR") \
  X(RTP_DATE, "DATA") \
  X(RTP_WEATHER, "POGODA") \
  X(RTP_TRAFFIC, "DROGI") \
  X(RTP_ADVERT, "REKLAMA") \
  X(RTP_STATION, "STACJA") \
  X(RTP_PROGRAM, "AUDYCJA") \
  X(RTP_NEXT, "NASTĘPNIE") \
  X(RTP_PART, "ODCINEK") \
  X(RTP_HOST, "PROWADZĄCY") \
  X(RTP_EDITORS, "REDAKCJA") \
  X(RTP_FREQUENCY, "CZĘSTOTL.") \
  X(RTP_HOMEPAGE, "STRONA WWW") \
  X(RTP_CHANNEL, "KANAŁ") \
  X(RTP_HOTLINE, "INFOLINIA") \
  X(RTP_PHONE, "TELEFON") \
  X(RTP_EMAIL, "E-MAIL") \
  X(RTP_CHAT, "CZAT") \
  X(RTP_VOTE, "SONDA") \
  X(RTP_PLACE, "MIEJSCE") \
  X(RTP_APPOINTMENT, "SPOTKANIE") \
  X(RTP_PURCHASE, "ZAKUP") \
  X(DAY_SUNDAY, "NIEDZIELA") \
  X(DAY_MONDAY, "PONIEDZIAŁEK") \
  X(DAY_TUESDAY, "WTOREK") \
  X(DAY_WEDNESDAY, "ŚRODA") \
  X(DAY_THURSDAY, "CZWARTEK") \
  X(DAY_FRIDAY, "PIĄTEK") \
  X(DAY_SATURDAY, "SOBOTA") \
  X(MONTH_JANUARY, "stycznia") \
  X(MONTH_FEBRUARY, "lutego") \
  X(MONTH_MARCH, "marca") \
  X(MONTH_APRIL, "kwietnia") \
  X(MONTH_MAY, "maja") \
  X(MONTH_JUNE, "czerwca") \
  X(MONTH_JULY, "lipca") \
  X(MONTH_AUGUST, "sierpnia") \
  X(MONTH_SEPTEMBER, "września") \
  X(MONTH_OCTOBER, "października") \
  X(MONTH_NOVEMBER, "listopada") \
  X(MONTH_DECEMBER, "grudnia") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_TODAY, "Dziś") \
  X(DATE_YESTERDAY, "Wczoraj") \
  X(DATE_YDAY, "WCZ.") \
  X(MONTH_SHORT_JAN, "sty") \
  X(MONTH_SHORT_FEB, "lut") \
  X(MONTH_SHORT_MAR, "mar") \
  X(MONTH_SHORT_APR, "kwi") \
  X(MONTH_SHORT_MAY, "maj") \
  X(MONTH_SHORT_JUN, "cze") \
  X(MONTH_SHORT_JUL, "lip") \
  X(MONTH_SHORT_AUG, "sie") \
  X(MONTH_SHORT_SEP, "wrz") \
  X(MONTH_SHORT_OCT, "paź") \
  X(MONTH_SHORT_NOV, "lis") \
  X(MONTH_SHORT_DEC, "gru") \
  X(MONTH_TAG_JAN, "STY") \
  X(MONTH_TAG_FEB, "LUT") \
  X(MONTH_TAG_APR, "KWI") \
  X(MONTH_TAG_MAY, "MAJ") \
  X(MONTH_TAG_JUN, "CZE") \
  X(MONTH_TAG_JUL, "LIP") \
  X(MONTH_TAG_AUG, "SIE") \
  X(MONTH_TAG_SEP, "WRZ") \
  X(MONTH_TAG_OCT, "PAŹ") \
  X(MONTH_TAG_NOV, "LIS") \
  X(MONTH_TAG_DEC, "GRU") \
  X(DX_NOW, "teraz") \
  X(DX_FMT_MIN_AGO, "%u min temu") \
  X(DX_FMT_H_AGO, "%u h temu") \
  X(DX_FMT_D_AGO, "%u dn. temu") \
  X(TOUCH_CAL_TITLE, "Kalibracja dotyku") \
  X(TOUCH_CAL_HOLD, "Trzymaj kółko, aż się wypełni") \
  X(TOUCH_CAL_CANCEL_HINT, "Anuluj: obróć lub naciśnij pokrętło") \
  X(TOUCH_CAL_CHECK, "Test") \
  X(TOUCH_CAL_TAP_DOT, "Dotknij kropki") \
  X(TOUCH_CAL_SAVED, "Kalibracja zapisana") \
  X(TOUCH_CAL_FMT_LANDED, "Odchyłka testu: %u px") \
  X(TOUCH_CAL_BACK_HINT, "Naciśnij pokrętło, by wrócić") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Nie zapisano") \
  X(TOUCH_CAL_FMT_MISSED, "Odchyłka testu: %u px,") \
  X(TOUCH_CAL_NO_FIT, "Z punktów nie wyszła kalibracja,") \
  X(TOUCH_CAL_NOT_SAVED, "Nie udało się jej zapisać,") \
  X(TOUCH_CAL_OLD_STAYS, "więc zostaje stara kalibracja") \
  X(TOUCH_CAL_RETRY_HINT, "Naciśnij pokrętło - ponów, obróć - wyjdź")
// clang-format on
