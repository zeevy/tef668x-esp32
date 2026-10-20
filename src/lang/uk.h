/*
 * The panel's texts in Ukrainian. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_UK(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Сканування FM") \
  X(MENU_SCAN_MW, "Сканування AM") \
  X(MENU_SCAN_SW, "Сканування SW") \
  X(MENU_SCAN_LW, "Сканування LW") \
  X(MENU_SQUELCH, "Режим ШП") \
  X(MENU_SQUELCH_LEVEL, "Рівень ШП") \
  X(MENU_SQUELCH_FLOOR, "Мінімум ШП") \
  X(MENU_SQUELCH_GROUP, "Шумоподавлювач") \
  X(MENU_VOLUME_AGC, "AGC гучності") \
  X(MENU_AGC_BOOST, "Підйом AGC") \
  X(MENU_MUTE_RAMP, "Згасання") \
  X(MENU_BAND_PLAN, "Діапазон") \
  X(MENU_TUNING_STEP, "Крок налаштування") \
  X(MENU_SEEK_SENSITIVITY, "Чутливість пошуку") \
  X(MENU_DE_EMPHASIS, "Деемфазис") \
  X(MENU_FORCED_MONO, "Завжди моно") \
  X(MENU_STEREO_BLEND, "Змішування стерео") \
  X(MENU_ST_HI_BLEND, "Змішування стерео/ВЧ") \
  X(MENU_STEREO, "Стерео") \
  X(MENU_HIGH_CUT, "Зріз ВЧ") \
  X(MENU_NOISE_BLANKER, "Подавлювач завад") \
  X(MENU_EQUALISER, "Еквалайзер") \
  X(COMMON_RDS_DECODER, "Декодер RDS") \
  X(MENU_RDS_REGION, "Регіон RDS") \
  X(MENU_MW_SPACING, "Крок MW") \
  X(MENU_FILTER_WIDTH, "Смуга фільтра") \
  X(MENU_SOFT_MUTE, "Приглушення") \
  X(MENU_DWELL, "Пауза на каналі") \
  X(MENU_STOP_ON, "Зупинка") \
  X(MENU_SCAN, "Сканувати") \
  X(MENU_MEMORY_FROM, "Перший канал") \
  X(MENU_MEMORY_TO, "Останній канал") \
  X(MENU_PRESET_RANGE, "Канали пам'яті") \
  X(MENU_LOOP_THE_BAND, "По колу") \
  X(MENU_DX_WIDTH, "Смуга скан.") \
  X(MENU_MUTE_WHILE_SCANNING, "Без звуку при скан.") \
  X(MENU_AUTO_LOG_NEW, "Нові в журнал") \
  X(MENU_LOG_RADIO_TEXT, "RT в журнал") \
  X(MENU_WATCH_PRESETS, "Контроль пам'яті") \
  X(MENU_SCOPE_BASELINE, "База спектра") \
  X(MENU_MEDIAN, "Медіана") \
  X(MENU_FMT_SWEEP_N, "Огляд %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Запам'ятати місцеві") \
  X(MENU_START_SCAN, "Почати скан.") \
  X(MENU_DAY_THEME, "Денна тема") \
  X(MENU_NIGHT_THEME, "Нічна тема") \
  X(MENU_THEME, "Тема") \
  X(MENU_BRIGHTNESS, "Яскравість") \
  X(MENU_DIM_LEVEL, "Яскравість затемн.") \
  X(MENU_DIM_AFTER, "Затемнити через") \
  X(MENU_ROTATION, "Поворот екрана") \
  X(COMMON_BATTERY, "Батарея") \
  X(MENU_BATTERY_CHECK, "Перевірка батареї") \
  X(MENU_LANGUAGE, "Мова \xC2\xB7 Language") \
  X(MENU_ENCODER, "Тип енкодера") \
  X(MENU_DIRECTION, "Напрям енкодера") \
  X(MENU_ENCODER_GROUP, "Енкодер") \
  X(MENU_KEY_BEEPS, "Звук кнопок") \
  X(MENU_BAND_EDGE_BEEP, "Звук краю діап.") \
  X(MENU_START_CHIME, "Звук увімкнення") \
  X(MENU_KEYPAD_TIMEOUT, "Очікування вводу") \
  X(MENU_TOUCH, "Сенсор") \
  X(MENU_CALIBRATE_TOUCH, "Калібрування сенсора") \
  X(MENU_CLOCK_FROM_NETWORK, "Час з мережі") \
  X(MENU_WEB_PIN, "Веб-PIN") \
  X(MENU_NEW_PIN, "Новий PIN") \
  X(MENU_FMT_DIGIT_OF, "Цифра %u з %u") \
  X(MENU_STATUS, "Стан з'єднання") \
  X(MENU_HOTSPOT, "Точка доступу") \
  X(MENU_WEB_SERVER, "Веб-сервер") \
  X(MENU_FMT_PC_CONNECTED, "Підключено: %u") \
  X(MENU_WEB_ADDRESS, "Веб-адреса") \
  X(MENU_IP_ADDRESS, "IP-адреса") \
  X(MENU_WI_FI_NAME, "Мережа Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Точка доступу") \
  X(MENU_MAC, "MAC-адреса") \
  X(MENU_WI_FI_SIGNAL, "Сигнал Wi-Fi") \
  X(MENU_NETWORK_INFO, "Дані мережі") \
  X(MENU_RESTART, "Перезапуск") \
  X(MENU_AUTO_OFF, "Автовимк.") \
  X(MENU_UPDATE_CHECK, "Шукати оновлення") \
  X(MENU_FIRMWARE_UPDATE, "Оновл. прошивки") \
  X(MENU_FMT_UPDATE_TO, "Оновити до %s") \
  X(MENU_UPDATE_TITLE, "Є оновлення") \
  X(MENU_UPDATE_THIS_RADIO, "Поточна") \
  X(MENU_UPDATE_NEW_VERSION, "Нова") \
  X(MENU_UPDATE_DOWNLOAD, "Розмір") \
  X(COMMON_SETTINGS, "Налаштування") \
  X(COMMON_KEPT, "Без змін") \
  X(MENU_STATION, "Станція") \
  X(MENU_UPDATE_NOW, "Оновити") \
  X(MENU_LATER, "Пізніше") \
  X(MENU_UPDATE_UP_TO_DATE, "Актуальна") \
  X(MENU_UPDATE_CHECKING, "Перевірка") \
  X(MENU_UPDATE_NOT_CHECKED, "Не перевірено") \
  X(MENU_UPDATE_CHECK_FAILED, "Збій перевірки") \
  X(MENU_NOTE_NO_UPDATE, "Оновлень немає") \
  X(MENU_VERSION, "Версія прошивки") \
  X(MENU_RUNNING_FROM, "Завантаження з") \
  X(MENU_UPTIME, "Час роботи") \
  X(MENU_BATTERY_VOLTAGE, "Напруга батареї") \
  X(COMMON_TUNER, "Тюнер") \
  X(MENU_RESET_REASON, "Причина скидання") \
  X(MENU_CPU_CORE_0, "Ядро CPU 0") \
  X(MENU_CPU_CORE_1, "Ядро CPU 1") \
  X(MENU_FREE_HEAP, "Вільна купа") \
  X(MENU_LOWEST_HEAP, "Мін. купа") \
  X(MENU_LARGEST_BLOCK, "Макс. блок") \
  X(MENU_LVGL_POOL, "Пул LVGL") \
  X(MENU_CHIP, "Чип") \
  X(MENU_FLASH_SIZE, "Обсяг флеш") \
  X(RESET_POWER, "Увімкнення") \
  X(RESET_SOFTWARE, "Програмний") \
  X(RESET_PANIC, "Паніка") \
  X(RESET_INT_WATCHDOG, "WDT IRQ") \
  X(RESET_TASK_WATCHDOG, "WDT задачі") \
  X(RESET_WATCHDOG, "WDT") \
  X(RESET_DEEP_SLEEP, "Глибокий сон") \
  X(RESET_BROWNOUT, "Збій живлення") \
  X(RESET_PIN, "Кнопка скидання") \
  X(RESET_UNKNOWN, "Невідомо") \
  X(RESET_ASKED, "За запитом") \
  X(RESET_BOOT_WATCHDOG, "WDT запуску") \
  X(RESET_ROLLBACK, "Відкат") \
  X(RESET_DISPLAY, "Збій екрана") \
  X(RESET_UPDATE, "Оновлення") \
  X(MENU_DEVELOPER, "Розробник") \
  X(MENU_BUILD, "Збірка") \
  X(MENU_LICENSE, "Ліцензія") \
  X(ABOUT_BUILD_UNKNOWN, "невідомо") \
  X(MENU_GO_TO, "Перехід") \
  X(MENU_GO_TO_BANDWIDTH, "Смуга") \
  X(MENU_GO_TO_DX, "Режим DX") \
  X(MENU_GO_TO_SCOPE, "Спектр") \
  X(MENU_GO_TO_BAND, "Наст. діапазон") \
  X(MENU_GO_TO_LOG, "Станцію в журнал") \
  X(MENU_GO_TO_SAVE, "Станцію в пам'ять") \
  X(MENU_FMT_SAVED_AS, "Збережено в P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Вже в P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Уся пам'ять зайнята") \
  X(MENU_GO_TO_SLEEP, "Сон") \
  X(MENU_STATIONS, "Станції") \
  X(MENU_MEMORY, "Пам'ять") \
  X(MENU_AUDIO, "Звук") \
  X(MENU_FM_SETUP, "Прийом FM") \
  X(MENU_AM_SETUP, "Прийом AM") \
  X(MENU_DX_SETUP, "DX-сканер") \
  X(MENU_DISPLAY, "Екран") \
  X(MENU_CONTROLS, "Керування") \
  X(MENU_CONNECTIVITY, "Мережа") \
  X(MENU_SYSTEM, "Система") \
  X(MENU_DIAGNOSTICS, "Діагностика") \
  X(MENU_ABOUT, "Про програму") \
  X(MENU_REGION_FULL, "Повний") \
  X(MENU_REGION_JAPAN, "Японія") \
  X(MENU_REGION_WIDE, "Широкий") \
  X(MENU_REGION_WORLD, "Світовий") \
  X(MENU_RDS_REGION_EUROPE, "Європа") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Пн. Америка") \
  X(MENU_WIFI_JOINED, "Підключено") \
  X(MENU_WIFI_JOINING, "Підключення") \
  X(MENU_WIFI_HOTSPOT, "Точка доступу") \
  X(COMMON_OFF, "Вимк.") \
  X(MENU_FMT_KB_OF_KB, "%u з %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V на старті") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, %u хв тому") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, %u год тому") \
  X(MENU_FMT_AGC_GAIN_NOW, "Підсилення: %+d dB") \
  X(MENU_STATION_LOG, "Журнал") \
  X(COMMON_NONE, "Немає") \
  X(MENU_FMT_SCAN_PROGRESS, "Скан. %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Збережено: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Перервано, збер.: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Повно, не збер.: %u") \
  X(COMMON_YES, "Так") \
  X(COMMON_NO, "Ні") \
  X(MENU_UNKNOWN_VALUE, "Невідомо") \
  X(COMMON_ROTATION_NORMAL, "Звичайний") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Перевернутий") \
  X(MENU_CUT_ONLY, "Без підйому") \
  X(MENU_NEVER, "Ніколи") \
  X(MENU_BATTERY_PER_CENT, "Відсотки") \
  X(MENU_BATTERY_VOLTS, "Вольти") \
  X(MENU_KEYS, "Лише цифри") \
  X(MENU_KEYS_LONG, "Короткі й довгі") \
  X(MENU_EVERY_PRESS, "Усі натискання") \
  X(MENU_STANDARD, "Стандартний") \
  X(MENU_OPTICAL, "Оптичний") \
  X(MENU_NORMAL, "Звичайний") \
  X(MENU_REVERSED, "Зворотний") \
  X(MENU_AUTO, "Авто") \
  X(MENU_SQUELCH_MAN, "Ручний") \
  X(MENU_ANY_PI, "Будь-який PI") \
  X(MENU_STOP_NEVER, "Ніколи") \
  X(MENU_NEW_ONLY, "Лише нові") \
  X(MENU_WHOLE_BAND, "Весь діапазон") \
  X(MENU_MEMORY_ONLY, "Лише пам'ять") \
  X(MENU_BAND_MEMORY, "Діапазон + пам'ять") \
  X(COMMON_ON, "Увімк.") \
  X(MENU_HINT_RESTART, "Діє після перезапуску") \
  X(MENU_TITLE, "Меню") \
  X(COMMON_FM_ONLY, "Лише FM") \
  X(MENU_NOTE_OFF_PLAN, "Поза діапазоном") \
  X(MENU_NOTE_SWITCH_TO_FM, "Перейдіть на FM") \
  X(MENU_NOTE_RADIO_BUSY, "Зайнято - повторіть") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Увімкніть RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Пробна прошивка - чекайте") \
  X(MENU_NOTE_NOT_STORED, "Не збережено - повернуто") \
  X(MENU_NOTE_NOT_PUT_BACK, "Зайнято - не повернуто") \
  X(MENU_NOTE_NOT_SAVED, "Не збережено") \
  X(MENU_SAVE, "Зберегти") \
  X(RADIO_PRESS_TO_START, "Натисніть: старт") \
  X(COMMON_STILL_TUNING, "Пошук - чекайте") \
  X(COMMON_SCAN_RUNNING, "Сканування - чекайте") \
  X(RADIO_FMT_NO_BAND, "%s поза діапазонами") \
  X(COMMON_NOT_LOGGED, "Не записано") \
  X(RADIO_FMT_LOGGED, "Записано: %s") \
  X(RADIO_CHECKING_UPDATES, "ПЕРЕВІРКА ОНОВЛЕНЬ\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Сканування %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Збій оновлення") \
  X(RADIO_GOING_TO_SLEEP, "Перехід у сон") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Натисніть ручку, щоб увімкнути") \
  X(RADIO_OLD_IMAGE_KEPT, "Прошивку не змінено") \
  X(RDS_STEREO, "Стерео") \
  X(RDS_MONO, "Моно") \
  X(RDS_SPEECH, "Розмова") \
  X(RDS_MUSIC, "Музика") \
  X(COMMON_ALREADY_LOGGED, "Вже в журналі") \
  X(DX_RDS_IS_OFF, "RDS вимкнено") \
  X(DX_SCANNING, "Сканування") \
  X(DX_SCAN_FINISHED, "Скан. завершено") \
  X(DX_NOTHING_TO_SCAN, "Нічого сканувати") \
  X(DX_CHECKING_FOR_UPDATES, "Перевірка оновлень") \
  X(DX_A_SCAN_IS_RUNNING, "Скан. вже йде") \
  X(DX_CAUGHT_LIST_UNREAD, "Улов недоступний") \
  X(DX_NO_MEMORY, "Пам'ять порожня") \
  X(DX_NOT_STARTED, "Не запущено") \
  X(DX_SWEEPING, "Йде огляд") \
  X(COMMON_FMT_TUNE_TYPED, "Налашт. %s") \
  X(RADIO_SQL_OFF, "ВИМК") \
  X(COMMON_AUTO, "АВТО") \
  X(RADIO_TUNE_MANUAL, "РУЧ") \
  X(RADIO_TUNE_AUTO, "АВТО") \
  X(RADIO_TUNE_MEMORY, "ПАМ") \
  X(RADIO_TUNE_METER, "МЕТР") \
  X(DX_FMT_ROWS_OF, "%u-%u з %u") \
  X(DX_FMT_FOUND, "Знайдено: %u") \
  X(DX_MODE_SWEEPING, "ОГЛЯД") \
  X(DX_MODE_LEARN_LOCALS, "ОБЛІК МІСЦ.") \
  X(DX_FMT_MODE_MEMORY, "ПАМ'ЯТЬ %u-%u") \
  X(DX_MODE_WHOLE_BAND, "ВЕСЬ ДІАП.") \
  X(DX_MODE_BAND_MEMORY, "ДІАП. + ПАМ'ЯТЬ") \
  X(DX_RULE_NO_STOP, "НОН-СТОП") \
  X(DX_RULE_STOP_ON_PI, "СТОП: PI") \
  X(DX_RULE_STOP_ON_NEW, "СТОП: НОВА") \
  X(DX_FMT_NOT_PRESET, "не P%02d") \
  X(DX_FMT_CHANNEL, "Кан. %u") \
  X(DX_CONTEXT_SWEEPING, "огляд") \
  X(DX_PRESS_TO_SWEEP, "Натисніть для огляду") \
  X(SCOPE_FMT_TITLE, "Спектр %s") \
  X(SCOPE_FULL, "Весь") \
  X(DX_FIXED, "ФІКС.") \
  X(DX_FMT_VS, "БАЗА %s") \
  X(DX_FMT_MEDIAN_OF, "МЕДІАНА З %u") \
  X(DX_FMT_FLOOR, "ШУМ %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Закрити") \
  X(BW_FMT_AUTO_AT, "Авто: %u kHz") \
  X(RECOVERY_PRESS_AGAIN, "Натисніть ще") \
  X(PAGE_TUNE, "Налаштувати") \
  X(PAGE_DELETE, "Видалити") \
  X(PAGE_LEVEL, "Рівень") \
  X(PAGE_SNR, "С/Ш") \
  X(PAGE_NOISE, "Шум") \
  X(PAGE_MULTIPATH, "Багатопр.") \
  X(PAGE_COCHANNEL, "Співканал") \
  X(PAGE_STEREO, "Стерео") \
  X(PAGE_WIDTH, "Смуга") \
  X(PAGE_BAND, "Діапазон") \
  X(PAGE_NOTE_CHANGED, "Змінено після відкриття") \
  X(PAGE_NOTE_NOT_DELETED, "Не видалено") \
  X(PAGE_FMT_SAVED, "Збережено: %s") \
  X(PAGE_TIME_UNKNOWN, "Час невідомий") \
  X(MENU_FMT_TUNER_PATCH, "%s патч %u") \
  X(BW_TITLE, "Смуга") \
  X(KEYPAD_TITLE, "Частота") \
  X(KEYPAD_CANCEL, "Скасувати") \
  X(DX_CATCHES_EMPTY_TITLE, "Улову ще немає") \
  X(DX_CATCHES_EMPTY_NOTE, "Тут будуть підтверджені станції сеансу") \
  X(DX_TITLE_CATCHES, "Улов") \
  X(DX_NEW, "НОВ.") \
  X(DX_READY, "Готовий") \
  X(DX_DWELL, "ПАУЗА") \
  X(DX_STOPPED, "СТОП") \
  X(DX_TITLE_SCANNER, "Сканер") \
  X(DX_TITLE_SCOPE, "Спектр") \
  X(DX_LEVEL, "РІВ.") \
  X(DX_RISE, "РІСТ") \
  X(DX_NO_STATION_YET, "Станцій ще немає") \
  X(DX_NO_RDS_YET, "RDS ще немає") \
  X(DX_OFFSET, "ЗСУВ") \
  X(COMMON_BW, "СП") \
  X(DX_MOD, "МОД") \
  X(DX_NO_ID, "БЕЗ ID") \
  X(RDS_TITLE_TEXT, "Радіотекст") \
  X(RDS_TITLE_NETWORKS, "Мережі") \
  X(RDS_TITLE_DECODER, "Декодер") \
  X(RDS_NO_COUNTRY, "немає ECC") \
  X(RDS_MS_UNKNOWN, "М/Р") \
  X(RDS_RTPLUS_NONE, "Немає RT+ від цієї станції") \
  X(RDS_RTPLUS_WAIT, "Немає тегів RT+ для тексту") \
  X(RDS_AF_TITLE, "ІНШІ ЧАСТОТИ") \
  X(RDS_EON_TITLE, "ІНШІ СТАНЦІЇ, EON") \
  X(RDS_NONE_HEARD, "Немає даних") \
  X(RDS_SYNC_CAPS, "СИНХР.") \
  X(RDS_GROUPS_S_CAPS, "ГРУП/С") \
  X(RDS_LOST, "ВТРАТИ") \
  X(RDS_EACH_BLOCK, "ПО БЛОКАХ") \
  X(RDS_LEGEND_CLEAN, "чисто") \
  X(RDS_LEGEND_FIXED, "випр.") \
  X(RDS_LEGEND_LOST, "втрати") \
  X(RDS_ALL_CLEAN, "усе чисто") \
  X(RDS_GROUP_TYPES, "ТИПИ ГРУП") \
  X(RDS_NO_GROUPS, "Немає груп за хвилину") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 за %u с") \
  X(RDS_COUNTRY_UNLISTED, "не в списку") \
  X(RDS_NO_TYPES, "Немає чистих блоків B за хвилину") \
  X(RDS_FMT_EON_MORE, "+%u ще") \
  X(RDS_EON_MORE_HEARD, "+ ще") \
  X(RDS_MORE, "ще") \
  X(RADIO_VEIL_UPDATING, "ОНОВЛЕННЯ ПРОШИВКИ") \
  X(RADIO_VEIL_KEEP_POWER, "Не вимикайте живлення") \
  X(RADIO_MUTE, "ТИША") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Тюнер не знайдено на шині I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Тюнер не готовий") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Тюнер не розпізнано") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Немає патча для цього тюнера") \
  X(TUNER_UNKNOWN_TUNER_PART, "Невідомий тюнер") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Тюнер відхилив запис") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Тюнер не відповів на читання") \
  X(TUNER_OUT_OF_RANGE, "Поза межами") \
  X(TUNER_UNKNOWN, "Невідомо") \
  X(BAND_LONG_LW, "Довгі хвилі") \
  X(BAND_LONG_MW, "Середні хвилі") \
  X(BAND_LONG_SW, "Короткі хвилі") \
  X(MENU_FMT_TUNE_TO, "Діапазон для %s") \
  X(PTY_NONE, "Немає") \
  X(PTY_NEWS, "Новини") \
  X(PTY_CURRENT_AFFAIRS, "Поточні події") \
  X(PTY_INFORMATION, "Інформація") \
  X(PTY_SPORT, "Спорт") \
  X(PTY_EDUCATION, "Освіта") \
  X(PTY_DRAMA, "Драма") \
  X(PTY_CULTURE, "Культура") \
  X(PTY_SCIENCE, "Наука") \
  X(PTY_VARIED, "Різне") \
  X(PTY_POP_MUSIC, "Поп-музика") \
  X(PTY_ROCK_MUSIC, "Рок-музика") \
  X(PTY_EASY_LISTENING, "Легка музика") \
  X(PTY_LIGHT_CLASSICAL, "Легка класика") \
  X(PTY_SERIOUS_CLASSICAL, "Класика") \
  X(PTY_OTHER_MUSIC, "Інша музика") \
  X(PTY_WEATHER, "Погода") \
  X(PTY_FINANCE, "Фінанси") \
  X(PTY_CHILDREN, "Для дітей") \
  X(PTY_SOCIAL_AFFAIRS, "Суспільство") \
  X(PTY_RELIGION, "Релігія") \
  X(PTY_PHONE_IN, "Дзвінки в ефір") \
  X(PTY_TRAVEL, "Подорожі") \
  X(PTY_LEISURE, "Дозвілля") \
  X(PTY_JAZZ_MUSIC, "Джаз") \
  X(PTY_COUNTRY_MUSIC, "Кантрі") \
  X(PTY_NATIONAL_MUSIC, "Нац. музика") \
  X(PTY_OLDIES_MUSIC, "Ретро") \
  X(PTY_FOLK_MUSIC, "Фольклор") \
  X(PTY_DOCUMENTARY, "Документальне") \
  X(PTY_ALARM_TEST, "Тест тривоги") \
  X(PTY_ALARM, "Тривога") \
  X(PTY_NA_NEWS, "Новини") \
  X(PTY_NA_INFORMATION, "Інформація") \
  X(PTY_NA_SPORTS, "Спорт") \
  X(PTY_NA_TALK, "Бесіди") \
  X(PTY_NA_ROCK, "Рок") \
  X(PTY_NA_CLASSIC_ROCK, "Класичний рок") \
  X(PTY_NA_ADULT_HITS, "Дорослі хіти") \
  X(PTY_NA_SOFT_ROCK, "Софт-рок") \
  X(PTY_NA_TOP_40, "Топ-40") \
  X(PTY_NA_COUNTRY, "Кантрі") \
  X(PTY_NA_OLDIES, "Ретро") \
  X(PTY_NA_SOFT, "Легка") \
  X(PTY_NA_NOSTALGIA, "Ностальгія") \
  X(PTY_NA_JAZZ, "Джаз") \
  X(PTY_NA_CLASSICAL, "Класика") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Ритм-н-блюз") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Софт ритм-н-блюз") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Іноземна мова") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Духовна музика") \
  X(PTY_NA_RELIGIOUS_TALK, "Реліг. бесіди") \
  X(PTY_NA_PERSONALITY, "Авторські") \
  X(PTY_NA_PUBLIC, "Громадське") \
  X(PTY_NA_COLLEGE, "Студентське") \
  X(PTY_NA_SPANISH_TALK, "Ісп. бесіди") \
  X(PTY_NA_SPANISH_MUSIC, "Іспанська музика") \
  X(PTY_NA_HIP_HOP, "Хіп-хоп") \
  X(PTY_NA_UNASSIGNED, "Не призначено") \
  X(PTY_NA_WEATHER, "Погода") \
  X(PTY_NA_EMERGENCY_TEST, "Тест тривоги") \
  X(PTY_NA_EMERGENCY, "Тривога") \
  X(COVERAGE_LOCAL, "Місцевий") \
  X(COVERAGE_INTERNATIONAL, "Міжнар.") \
  X(COVERAGE_NATIONAL, "Націон.") \
  X(COVERAGE_SUPRA_REGIONAL, "Міжрегіон.") \
  X(COVERAGE_REGIONAL_1, "Регіон 1") \
  X(COVERAGE_REGIONAL_2, "Регіон 2") \
  X(COVERAGE_REGIONAL_3, "Регіон 3") \
  X(COVERAGE_REGIONAL_4, "Регіон 4") \
  X(COVERAGE_REGIONAL_5, "Регіон 5") \
  X(COVERAGE_REGIONAL_6, "Регіон 6") \
  X(COVERAGE_REGIONAL_7, "Регіон 7") \
  X(COVERAGE_REGIONAL_8, "Регіон 8") \
  X(COVERAGE_REGIONAL_9, "Регіон 9") \
  X(COVERAGE_REGIONAL_10, "Регіон 10") \
  X(COVERAGE_REGIONAL_11, "Регіон 11") \
  X(COVERAGE_REGIONAL_12, "Регіон 12") \
  X(RTP_OTHER, "ІНШЕ") \
  X(RTP_TITLE, "НАЗВА") \
  X(RTP_ALBUM, "АЛЬБОМ") \
  X(RTP_TRACK, "ТРЕК") \
  X(RTP_ARTIST, "ВИКОНАВЕЦЬ") \
  X(RTP_WORK, "ТВІР") \
  X(RTP_MOVEMENT, "ЧАСТИНА") \
  X(RTP_CONDUCTOR, "ДИРИГЕНТ") \
  X(RTP_COMPOSER, "КОМПОЗИТОР") \
  X(RTP_BAND, "ГУРТ") \
  X(RTP_COMMENT, "КОМЕНТАР") \
  X(RTP_GENRE, "ЖАНР") \
  X(RTP_NEWS, "НОВИНИ") \
  X(RTP_LOCAL, "МІСЦ. НОВ.") \
  X(RTP_STOCKS, "БІРЖА") \
  X(RTP_SPORT, "СПОРТ") \
  X(RTP_LOTTERY, "ЛОТЕРЕЯ") \
  X(RTP_HOROSCOPE, "ГОРОСКОП") \
  X(RTP_DIVERSION, "РОЗВАГИ") \
  X(RTP_HEALTH, "ЗДОРОВ'Я") \
  X(RTP_EVENT, "ПОДІЯ") \
  X(RTP_SCENE, "СЦЕНА") \
  X(RTP_CINEMA, "КІНО") \
  X(RTP_FUN, "ГУМОР") \
  X(RTP_DATE, "ДАТА") \
  X(RTP_WEATHER, "ПОГОДА") \
  X(RTP_TRAFFIC, "ДОРОГИ") \
  X(RTP_ALARM, "ТРИВОГА") \
  X(RTP_ADVERT, "РЕКЛАМА") \
  X(RTP_STATION, "СТАНЦІЯ") \
  X(RTP_PROGRAM, "ПРОГРАМА") \
  X(RTP_NEXT, "ДАЛІ") \
  X(RTP_PART, "ЕПІЗОД") \
  X(RTP_HOST, "ВЕДУЧИЙ") \
  X(RTP_EDITORS, "РЕДАКЦІЯ") \
  X(RTP_FREQUENCY, "ЧАСТОТА") \
  X(RTP_HOMEPAGE, "САЙТ") \
  X(RTP_CHANNEL, "КАНАЛ") \
  X(RTP_HOTLINE, "ГАР. ЛІНІЯ") \
  X(RTP_STUDIO, "СТУДІЯ") \
  X(RTP_PHONE, "ТЕЛЕФОН") \
  X(RTP_CHAT, "ЧАТ") \
  X(RTP_VOTE, "ОПИТУВАННЯ") \
  X(RTP_PLACE, "МІСЦЕ") \
  X(RTP_APPOINTMENT, "ЗУСТРІЧ") \
  X(RTP_PURCHASE, "ПОКУПКА") \
  X(DAY_SUNDAY, "НЕДІЛЯ") \
  X(DAY_MONDAY, "ПОНЕДІЛОК") \
  X(DAY_TUESDAY, "ВІВТОРОК") \
  X(DAY_WEDNESDAY, "СЕРЕДА") \
  X(DAY_THURSDAY, "ЧЕТВЕР") \
  X(DAY_FRIDAY, "П'ЯТНИЦЯ") \
  X(DAY_SATURDAY, "СУБОТА") \
  X(MONTH_JANUARY, "січня") \
  X(MONTH_FEBRUARY, "лютого") \
  X(MONTH_MARCH, "березня") \
  X(MONTH_APRIL, "квітня") \
  X(MONTH_MAY, "травня") \
  X(MONTH_JUNE, "червня") \
  X(MONTH_JULY, "липня") \
  X(MONTH_AUGUST, "серпня") \
  X(MONTH_SEPTEMBER, "вересня") \
  X(MONTH_OCTOBER, "жовтня") \
  X(MONTH_NOVEMBER, "листопада") \
  X(MONTH_DECEMBER, "грудня") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_TODAY, "Сьогодні") \
  X(DATE_YESTERDAY, "Вчора") \
  X(DATE_YDAY, "ВЧОРА") \
  X(MONTH_SHORT_JAN, "січ") \
  X(MONTH_SHORT_FEB, "лют") \
  X(MONTH_SHORT_MAR, "бер") \
  X(MONTH_SHORT_APR, "кві") \
  X(MONTH_SHORT_MAY, "тра") \
  X(MONTH_SHORT_JUN, "чер") \
  X(MONTH_SHORT_JUL, "лип") \
  X(MONTH_SHORT_AUG, "сер") \
  X(MONTH_SHORT_SEP, "вер") \
  X(MONTH_SHORT_OCT, "жов") \
  X(MONTH_SHORT_NOV, "лис") \
  X(MONTH_SHORT_DEC, "гру") \
  X(MONTH_TAG_JAN, "СІЧ") \
  X(MONTH_TAG_FEB, "ЛЮТ") \
  X(MONTH_TAG_MAR, "БЕР") \
  X(MONTH_TAG_APR, "КВІ") \
  X(MONTH_TAG_MAY, "ТРА") \
  X(MONTH_TAG_JUN, "ЧЕР") \
  X(MONTH_TAG_JUL, "ЛИП") \
  X(MONTH_TAG_AUG, "СЕР") \
  X(MONTH_TAG_SEP, "ВЕР") \
  X(MONTH_TAG_OCT, "ЖОВ") \
  X(MONTH_TAG_NOV, "ЛИС") \
  X(MONTH_TAG_DEC, "ГРУ") \
  X(DX_NOW, "зараз") \
  X(DX_FMT_MIN_AGO, "%u хв тому") \
  X(DX_FMT_H_AGO, "%u год тому") \
  X(DX_FMT_D_AGO, "%u дн. тому") \
  X(TOUCH_CAL_TITLE, "Калібрування сенсора") \
  X(TOUCH_CAL_HOLD, "Тримайте коло до заповнення") \
  X(TOUCH_CAL_CANCEL_HINT, "Поворот/натиск ручки - скасувати") \
  X(TOUCH_CAL_CHECK, "Перевірка") \
  X(TOUCH_CAL_TAP_DOT, "Торкніться точки") \
  X(TOUCH_CAL_SAVED, "Калібрування збережено") \
  X(TOUCH_CAL_FMT_LANDED, "Перевірка: зсув %u px") \
  X(TOUCH_CAL_BACK_HINT, "Натисніть ручку - назад") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Не збережено") \
  X(TOUCH_CAL_FMT_MISSED, "Перевірка: зсув %u px,") \
  X(TOUCH_CAL_NO_FIT, "За мітками калібрування не вийшло,") \
  X(TOUCH_CAL_NOT_SAVED, "Запис не вдався,") \
  X(TOUCH_CAL_OLD_STAYS, "лишилося старе калібрування") \
  X(TOUCH_CAL_RETRY_HINT, "Ручка: натиск - повтор, поворот - вихід")
// clang-format on
