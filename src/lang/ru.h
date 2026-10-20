/*
 * The panel's texts in Russian. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_RU(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Сканирование FM") \
  X(MENU_SCAN_MW, "Сканирование AM") \
  X(MENU_SCAN_SW, "Сканирование SW") \
  X(MENU_SCAN_LW, "Сканирование LW") \
  X(MENU_SQUELCH, "Режим ШП") \
  X(MENU_SQUELCH_LEVEL, "Уровень ШП") \
  X(MENU_SQUELCH_FLOOR, "Минимум ШП") \
  X(MENU_SQUELCH_GROUP, "Шумоподавитель") \
  X(MENU_VOLUME_AGC, "AGC громкости") \
  X(MENU_AGC_BOOST, "Подъём AGC") \
  X(MENU_MUTE_RAMP, "Затухание") \
  X(MENU_BAND_PLAN, "Диапазон") \
  X(MENU_TUNING_STEP, "Шаг настройки") \
  X(MENU_SEEK_SENSITIVITY, "Чувств. поиска") \
  X(MENU_DE_EMPHASIS, "Деэмфазис") \
  X(MENU_FORCED_MONO, "Всегда моно") \
  X(MENU_STEREO_BLEND, "Смешение стерео") \
  X(MENU_ST_HI_BLEND, "Смешение стерео/ВЧ") \
  X(MENU_STEREO, "Стерео") \
  X(MENU_HIGH_CUT, "Срез ВЧ") \
  X(MENU_NOISE_BLANKER, "Подавитель помех") \
  X(MENU_EQUALISER, "Эквалайзер") \
  X(COMMON_RDS_DECODER, "Декодер RDS") \
  X(MENU_RDS_REGION, "Регион RDS") \
  X(MENU_MW_SPACING, "Шаг MW") \
  X(MENU_FILTER_WIDTH, "Полоса фильтра") \
  X(MENU_SOFT_MUTE, "Приглушение") \
  X(MENU_DWELL, "Пауза на канале") \
  X(MENU_STOP_ON, "Остановка") \
  X(MENU_SCAN, "Сканировать") \
  X(MENU_MEMORY_FROM, "Первая ячейка") \
  X(MENU_MEMORY_TO, "Последняя ячейка") \
  X(MENU_PRESET_RANGE, "Ячейки памяти") \
  X(MENU_LOOP_THE_BAND, "По кругу") \
  X(MENU_DX_WIDTH, "Полоса скан.") \
  X(MENU_MUTE_WHILE_SCANNING, "Без звука при скан.") \
  X(MENU_AUTO_LOG_NEW, "Новые в журнал") \
  X(MENU_LOG_RADIO_TEXT, "RT в журнал") \
  X(MENU_WATCH_PRESETS, "Контроль памяти") \
  X(MENU_SCOPE_BASELINE, "База спектра") \
  X(MENU_MEDIAN, "Медиана") \
  X(MENU_FMT_SWEEP_N, "Обзор %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Запомнить местные") \
  X(MENU_START_SCAN, "Начать скан.") \
  X(MENU_DAY_THEME, "Дневная тема") \
  X(MENU_NIGHT_THEME, "Ночная тема") \
  X(MENU_THEME, "Тема") \
  X(MENU_BRIGHTNESS, "Яркость") \
  X(MENU_DIM_LEVEL, "Яркость затемн.") \
  X(MENU_DIM_AFTER, "Затемнять через") \
  X(MENU_ROTATION, "Поворот экрана") \
  X(COMMON_BATTERY, "Батарея") \
  X(MENU_BATTERY_CHECK, "Проверка батареи") \
  X(MENU_LANGUAGE, "Язык \xC2\xB7 Language") \
  X(MENU_ENCODER, "Тип энкодера") \
  X(MENU_DIRECTION, "Направл. энкодера") \
  X(MENU_ENCODER_GROUP, "Энкодер") \
  X(MENU_KEY_BEEPS, "Звук кнопок") \
  X(MENU_BAND_EDGE_BEEP, "Звук края диап.") \
  X(MENU_START_CHIME, "Звук включения") \
  X(MENU_KEYPAD_TIMEOUT, "Ожидание ввода") \
  X(MENU_TOUCH, "Сенсор") \
  X(MENU_CALIBRATE_TOUCH, "Калибровка сенсора") \
  X(MENU_CLOCK_FROM_NETWORK, "Время из сети") \
  X(MENU_WEB_PIN, "Веб-PIN") \
  X(MENU_NEW_PIN, "Новый PIN") \
  X(MENU_FMT_DIGIT_OF, "Цифра %u из %u") \
  X(MENU_STATUS, "Состояние") \
  X(MENU_HOTSPOT, "Точка доступа") \
  X(MENU_WEB_SERVER, "Веб-сервер") \
  X(MENU_FMT_PC_CONNECTED, "Подключено: %u") \
  X(MENU_WEB_ADDRESS, "Веб-адрес") \
  X(MENU_IP_ADDRESS, "IP-адрес") \
  X(MENU_WI_FI_NAME, "Сеть Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Точка доступа") \
  X(MENU_MAC, "MAC-адрес") \
  X(MENU_WI_FI_SIGNAL, "Сигнал Wi-Fi") \
  X(MENU_NETWORK_INFO, "Данные сети") \
  X(MENU_RESTART, "Перезагрузка") \
  X(MENU_AUTO_OFF, "Автовыкл.") \
  X(MENU_UPDATE_CHECK, "Искать обновления") \
  X(MENU_FIRMWARE_UPDATE, "Обновл. прошивки") \
  X(MENU_FMT_UPDATE_TO, "Обновить до %s") \
  X(MENU_UPDATE_TITLE, "Есть обновление") \
  X(MENU_UPDATE_THIS_RADIO, "Текущая") \
  X(MENU_UPDATE_NEW_VERSION, "Новая") \
  X(MENU_UPDATE_DOWNLOAD, "Размер") \
  X(COMMON_SETTINGS, "Настройки") \
  X(COMMON_KEPT, "Сохранятся") \
  X(MENU_STATION, "Станция") \
  X(MENU_UPDATE_NOW, "Обновить") \
  X(MENU_LATER, "Позже") \
  X(MENU_UPDATE_UP_TO_DATE, "Актуальна") \
  X(MENU_UPDATE_CHECKING, "Проверка") \
  X(MENU_UPDATE_NOT_CHECKED, "Не проверено") \
  X(MENU_UPDATE_CHECK_FAILED, "Сбой проверки") \
  X(MENU_NOTE_NO_UPDATE, "Обновлений нет") \
  X(MENU_VERSION, "Версия прошивки") \
  X(MENU_RUNNING_FROM, "Загрузка из") \
  X(MENU_UPTIME, "Время работы") \
  X(MENU_BATTERY_VOLTAGE, "Напряж. батареи") \
  X(COMMON_TUNER, "Тюнер") \
  X(MENU_RESET_REASON, "Причина сброса") \
  X(MENU_CPU_CORE_0, "Ядро CPU 0") \
  X(MENU_CPU_CORE_1, "Ядро CPU 1") \
  X(MENU_FREE_HEAP, "Своб. куча") \
  X(MENU_LOWEST_HEAP, "Мин. куча") \
  X(MENU_LARGEST_BLOCK, "Макс. блок") \
  X(MENU_LVGL_POOL, "Пул LVGL") \
  X(MENU_CHIP, "Чип") \
  X(MENU_FLASH_SIZE, "Объём флеш") \
  X(RESET_POWER, "Включение") \
  X(RESET_SOFTWARE, "Программный") \
  X(RESET_PANIC, "Паника") \
  X(RESET_INT_WATCHDOG, "WDT IRQ") \
  X(RESET_TASK_WATCHDOG, "WDT задачи") \
  X(RESET_WATCHDOG, "WDT") \
  X(RESET_DEEP_SLEEP, "Глубокий сон") \
  X(RESET_BROWNOUT, "Сбой питания") \
  X(RESET_PIN, "Кнопка сброса") \
  X(RESET_UNKNOWN, "Неизвестно") \
  X(RESET_ASKED, "По запросу") \
  X(RESET_BOOT_WATCHDOG, "WDT загрузки") \
  X(RESET_ROLLBACK, "Откат") \
  X(RESET_DISPLAY, "Сбой дисплея") \
  X(RESET_UPDATE, "Обновление") \
  X(MENU_DEVELOPER, "Разработчик") \
  X(MENU_BUILD, "Сборка") \
  X(MENU_LICENSE, "Лицензия") \
  X(ABOUT_BUILD_UNKNOWN, "неизвестно") \
  X(MENU_GO_TO, "Переход") \
  X(MENU_GO_TO_BANDWIDTH, "Полоса") \
  X(MENU_GO_TO_DX, "Режим DX") \
  X(MENU_GO_TO_SCOPE, "Спектр") \
  X(MENU_GO_TO_BAND, "След. диапазон") \
  X(MENU_GO_TO_LOG, "Станцию в журнал") \
  X(MENU_GO_TO_SAVE, "Станцию в память") \
  X(MENU_FMT_SAVED_AS, "Сохранено в P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Уже в P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Вся память занята") \
  X(MENU_GO_TO_SLEEP, "Сон") \
  X(MENU_STATIONS, "Станции") \
  X(MENU_MEMORY, "Память") \
  X(MENU_AUDIO, "Звук") \
  X(MENU_FM_SETUP, "Приём FM") \
  X(MENU_AM_SETUP, "Приём AM") \
  X(MENU_DX_SETUP, "DX-сканер") \
  X(MENU_DISPLAY, "Экран") \
  X(MENU_CONTROLS, "Управление") \
  X(MENU_CONNECTIVITY, "Сеть") \
  X(MENU_SYSTEM, "Система") \
  X(MENU_DIAGNOSTICS, "Диагностика") \
  X(MENU_ABOUT, "О программе") \
  X(MENU_REGION_FULL, "Полный") \
  X(MENU_REGION_JAPAN, "Япония") \
  X(MENU_REGION_WIDE, "Широкий") \
  X(MENU_REGION_WORLD, "Мировой") \
  X(MENU_RDS_REGION_EUROPE, "Европа") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Сев. Америка") \
  X(MENU_WIFI_JOINED, "Подключено") \
  X(MENU_WIFI_JOINING, "Подключение") \
  X(MENU_WIFI_HOTSPOT, "Точка доступа") \
  X(COMMON_OFF, "Выкл.") \
  X(MENU_FMT_KB_OF_KB, "%u из %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V при старте") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, %u мин назад") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, %u ч назад") \
  X(MENU_FMT_AGC_GAIN_NOW, "Усиление: %+d dB") \
  X(MENU_STATION_LOG, "Журнал") \
  X(COMMON_NONE, "Нет") \
  X(MENU_FMT_SCAN_PROGRESS, "Скан. %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Сохранено: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Прервано, сохр.: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Полно, не сохр.: %u") \
  X(COMMON_YES, "Да") \
  X(COMMON_NO, "Нет") \
  X(MENU_UNKNOWN_VALUE, "Неизвестно") \
  X(COMMON_ROTATION_NORMAL, "Обычный") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Перевёрнут") \
  X(MENU_CUT_ONLY, "Без подъёма") \
  X(MENU_NEVER, "Никогда") \
  X(MENU_BATTERY_PER_CENT, "Проценты") \
  X(MENU_BATTERY_VOLTS, "Вольты") \
  X(MENU_KEYS, "Только цифры") \
  X(MENU_KEYS_LONG, "Короткие и долгие") \
  X(MENU_EVERY_PRESS, "Все нажатия") \
  X(MENU_STANDARD, "Стандартный") \
  X(MENU_OPTICAL, "Оптический") \
  X(MENU_NORMAL, "Обычное") \
  X(MENU_REVERSED, "Обратное") \
  X(MENU_AUTO, "Авто") \
  X(MENU_SQUELCH_MAN, "Ручной") \
  X(MENU_ANY_PI, "Любой PI") \
  X(MENU_STOP_NEVER, "Никогда") \
  X(MENU_NEW_ONLY, "Только новые") \
  X(MENU_WHOLE_BAND, "Весь диапазон") \
  X(MENU_MEMORY_ONLY, "Только память") \
  X(MENU_BAND_MEMORY, "Диапазон + память") \
  X(COMMON_ON, "Вкл.") \
  X(MENU_HINT_RESTART, "Нужна перезагрузка") \
  X(MENU_TITLE, "Меню") \
  X(COMMON_FM_ONLY, "Только FM") \
  X(MENU_NOTE_OFF_PLAN, "Вне диапазона") \
  X(MENU_NOTE_SWITCH_TO_FM, "Включите FM") \
  X(MENU_NOTE_RADIO_BUSY, "Занято - повторите") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Включите RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Пробная прошивка - ждите") \
  X(MENU_NOTE_NOT_STORED, "Не сохранено - возврат") \
  X(MENU_NOTE_NOT_PUT_BACK, "Занято - не возвращено") \
  X(MENU_NOTE_NOT_SAVED, "Не сохранено") \
  X(MENU_SAVE, "Сохранить") \
  X(RADIO_PRESS_TO_START, "Нажмите: старт") \
  X(COMMON_STILL_TUNING, "Настройка - ждите") \
  X(COMMON_SCAN_RUNNING, "Сканирование - ждите") \
  X(RADIO_FMT_NO_BAND, "%s вне диапазонов") \
  X(COMMON_NOT_LOGGED, "Не записано") \
  X(RADIO_FMT_LOGGED, "Записано: %s") \
  X(RADIO_CHECKING_UPDATES, "ПРОВЕРКА ОБНОВЛЕНИЙ\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Сканирование %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Сбой обновления") \
  X(RADIO_GOING_TO_SLEEP, "Переход в сон") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Нажмите ручку для включения") \
  X(RADIO_OLD_IMAGE_KEPT, "Прошивка не изменена") \
  X(RDS_STEREO, "Стерео") \
  X(RDS_MONO, "Моно") \
  X(RDS_SPEECH, "Речь") \
  X(RDS_MUSIC, "Музыка") \
  X(COMMON_ALREADY_LOGGED, "Уже в журнале") \
  X(DX_RDS_IS_OFF, "RDS выключен") \
  X(DX_SCANNING, "Сканирование") \
  X(DX_SCAN_FINISHED, "Скан. завершено") \
  X(DX_NOTHING_TO_SCAN, "Нечего сканировать") \
  X(DX_CHECKING_FOR_UPDATES, "Проверка обновлений") \
  X(DX_A_SCAN_IS_RUNNING, "Скан. уже идёт") \
  X(DX_CAUGHT_LIST_UNREAD, "Улов недоступен") \
  X(DX_NO_MEMORY, "Память пуста") \
  X(DX_NOT_STARTED, "Не запущено") \
  X(DX_SWEEPING, "Идёт обзор") \
  X(COMMON_FMT_TUNE_TYPED, "Настроить %s") \
  X(RADIO_SQL_OFF, "ВЫКЛ") \
  X(COMMON_AUTO, "АВТО") \
  X(RADIO_TUNE_MANUAL, "РУЧ") \
  X(RADIO_TUNE_AUTO, "АВТО") \
  X(RADIO_TUNE_MEMORY, "ПАМ") \
  X(RADIO_TUNE_METER, "МЕТР") \
  X(DX_FMT_ROWS_OF, "%u-%u из %u") \
  X(DX_FMT_FOUND, "Найдено: %u") \
  X(DX_MODE_SWEEPING, "ОБЗОР") \
  X(DX_MODE_LEARN_LOCALS, "УЧЁТ МЕСТНЫХ") \
  X(DX_FMT_MODE_MEMORY, "ПАМЯТЬ %u-%u") \
  X(DX_MODE_WHOLE_BAND, "ВЕСЬ ДИАПАЗОН") \
  X(DX_MODE_BAND_MEMORY, "ДИАП. + ПАМЯТЬ") \
  X(DX_RULE_NO_STOP, "БЕЗ СТОПА") \
  X(DX_RULE_STOP_ON_PI, "СТОП НА PI") \
  X(DX_RULE_STOP_ON_NEW, "СТОП НА НОВОЙ") \
  X(DX_FMT_NOT_PRESET, "не P%02d") \
  X(DX_FMT_CHANNEL, "Яч. %u") \
  X(DX_CONTEXT_SWEEPING, "обзор") \
  X(DX_PRESS_TO_SWEEP, "Нажмите для обзора") \
  X(SCOPE_FMT_TITLE, "Спектр %s") \
  X(SCOPE_FULL, "Весь") \
  X(DX_FIXED, "ФИКС.") \
  X(DX_FMT_VS, "БАЗА %s") \
  X(DX_FMT_MEDIAN_OF, "МЕДИАНА ИЗ %u") \
  X(DX_FMT_FLOOR, "ШУМ %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Закрыть") \
  X(BW_FMT_AUTO_AT, "Авто: %u kHz") \
  X(RECOVERY_PRESS_AGAIN, "Нажмите ещё") \
  X(PAGE_TUNE, "Настроить") \
  X(PAGE_DELETE, "Удалить") \
  X(PAGE_LEVEL, "Уровень") \
  X(PAGE_SNR, "С/Ш") \
  X(PAGE_NOISE, "Шум") \
  X(PAGE_MULTIPATH, "Многолуч.") \
  X(PAGE_COCHANNEL, "Совм. канал") \
  X(PAGE_STEREO, "Стерео") \
  X(PAGE_WIDTH, "Полоса") \
  X(PAGE_BAND, "Диапазон") \
  X(PAGE_NOTE_CHANGED, "Изменено после открытия") \
  X(PAGE_NOTE_NOT_DELETED, "Не удалено") \
  X(PAGE_FMT_SAVED, "Сохранено: %s") \
  X(PAGE_TIME_UNKNOWN, "Время неизвестно") \
  X(MENU_FMT_TUNER_PATCH, "%s патч %u") \
  X(BW_TITLE, "Полоса") \
  X(KEYPAD_TITLE, "Частота") \
  X(KEYPAD_CANCEL, "Отмена") \
  X(DX_CATCHES_EMPTY_TITLE, "Улова пока нет") \
  X(DX_CATCHES_EMPTY_NOTE, "Здесь будут подтверждённые станции сеанса") \
  X(DX_TITLE_CATCHES, "Улов") \
  X(DX_NEW, "НОВ.") \
  X(DX_READY, "Готов") \
  X(DX_DWELL, "ПАУЗА") \
  X(DX_STOPPED, "СТОП") \
  X(DX_TITLE_SCANNER, "Сканер") \
  X(DX_TITLE_SCOPE, "Спектр") \
  X(DX_LEVEL, "УРОВ.") \
  X(DX_RISE, "РОСТ") \
  X(DX_NO_STATION_YET, "Станций пока нет") \
  X(DX_NO_RDS_YET, "RDS пока нет") \
  X(DX_OFFSET, "СДВИГ") \
  X(COMMON_BW, "ПП") \
  X(DX_MOD, "МОД") \
  X(DX_NO_ID, "НЕТ ID") \
  X(RDS_TITLE_TEXT, "Радиотекст") \
  X(RDS_TITLE_NETWORKS, "Сети") \
  X(RDS_TITLE_DECODER, "Декодер") \
  X(RDS_NO_COUNTRY, "нет ECC") \
  X(RDS_MS_UNKNOWN, "М/Р") \
  X(RDS_RTPLUS_NONE, "Нет RT+ от этой станции") \
  X(RDS_RTPLUS_WAIT, "Нет тегов RT+ для этого текста") \
  X(RDS_AF_TITLE, "ДРУГИЕ ЧАСТОТЫ") \
  X(RDS_EON_TITLE, "ДРУГИЕ СТАНЦИИ, EON") \
  X(RDS_NONE_HEARD, "Нет данных") \
  X(RDS_SYNC_CAPS, "СИНХР.") \
  X(RDS_GROUPS_S_CAPS, "ГРУПП/С") \
  X(RDS_LOST, "ПОТЕРИ") \
  X(RDS_EACH_BLOCK, "ПО БЛОКАМ") \
  X(RDS_LEGEND_CLEAN, "чисто") \
  X(RDS_LEGEND_FIXED, "испр.") \
  X(RDS_LEGEND_LOST, "потери") \
  X(RDS_ALL_CLEAN, "всё чисто") \
  X(RDS_GROUP_TYPES, "ТИПЫ ГРУПП") \
  X(RDS_NO_GROUPS, "Нет групп за минуту") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 за %u с") \
  X(RDS_COUNTRY_UNLISTED, "нет в списке") \
  X(RDS_NO_TYPES, "Нет чистых блоков B за минуту") \
  X(RDS_FMT_EON_MORE, "+%u ещё") \
  X(RDS_EON_MORE_HEARD, "+ ещё") \
  X(RDS_MORE, "ещё") \
  X(RADIO_VEIL_UPDATING, "ОБНОВЛЕНИЕ ПРОШИВКИ") \
  X(RADIO_VEIL_KEEP_POWER, "Не отключайте питание") \
  X(RADIO_MUTE, "ТИХО") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Тюнер не найден на шине I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Тюнер не готов") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Тюнер не опознан") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Нет патча для этого тюнера") \
  X(TUNER_UNKNOWN_TUNER_PART, "Тюнер неизвестен") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Тюнер отклонил запись") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Тюнер не ответил на чтение") \
  X(TUNER_OUT_OF_RANGE, "Вне пределов") \
  X(TUNER_UNKNOWN, "Неизвестно") \
  X(BAND_LONG_LW, "Длинные волны") \
  X(BAND_LONG_MW, "Средние волны") \
  X(BAND_LONG_SW, "Короткие волны") \
  X(MENU_FMT_TUNE_TO, "Диапазон для %s") \
  X(PTY_NONE, "Нет") \
  X(PTY_NEWS, "Новости") \
  X(PTY_CURRENT_AFFAIRS, "Текущие события") \
  X(PTY_INFORMATION, "Информация") \
  X(PTY_SPORT, "Спорт") \
  X(PTY_EDUCATION, "Образование") \
  X(PTY_DRAMA, "Драма") \
  X(PTY_CULTURE, "Культура") \
  X(PTY_SCIENCE, "Наука") \
  X(PTY_VARIED, "Разное") \
  X(PTY_POP_MUSIC, "Поп-музыка") \
  X(PTY_ROCK_MUSIC, "Рок-музыка") \
  X(PTY_EASY_LISTENING, "Лёгкая музыка") \
  X(PTY_LIGHT_CLASSICAL, "Лёгкая классика") \
  X(PTY_SERIOUS_CLASSICAL, "Классика") \
  X(PTY_OTHER_MUSIC, "Другая музыка") \
  X(PTY_WEATHER, "Погода") \
  X(PTY_FINANCE, "Финансы") \
  X(PTY_CHILDREN, "Для детей") \
  X(PTY_SOCIAL_AFFAIRS, "Общество") \
  X(PTY_RELIGION, "Религия") \
  X(PTY_PHONE_IN, "Звонки в эфир") \
  X(PTY_TRAVEL, "Туризм") \
  X(PTY_LEISURE, "Досуг") \
  X(PTY_JAZZ_MUSIC, "Джаз") \
  X(PTY_COUNTRY_MUSIC, "Кантри") \
  X(PTY_NATIONAL_MUSIC, "Нац. музыка") \
  X(PTY_OLDIES_MUSIC, "Ретро") \
  X(PTY_FOLK_MUSIC, "Народная") \
  X(PTY_DOCUMENTARY, "Документальное") \
  X(PTY_ALARM_TEST, "Тест тревоги") \
  X(PTY_ALARM, "Тревога") \
  X(PTY_NA_NEWS, "Новости") \
  X(PTY_NA_INFORMATION, "Информация") \
  X(PTY_NA_SPORTS, "Спорт") \
  X(PTY_NA_TALK, "Беседы") \
  X(PTY_NA_ROCK, "Рок") \
  X(PTY_NA_CLASSIC_ROCK, "Классич. рок") \
  X(PTY_NA_ADULT_HITS, "Взрослые хиты") \
  X(PTY_NA_SOFT_ROCK, "Софт-рок") \
  X(PTY_NA_TOP_40, "Топ-40") \
  X(PTY_NA_COUNTRY, "Кантри") \
  X(PTY_NA_OLDIES, "Ретро") \
  X(PTY_NA_SOFT, "Лёгкая") \
  X(PTY_NA_NOSTALGIA, "Ностальгия") \
  X(PTY_NA_JAZZ, "Джаз") \
  X(PTY_NA_CLASSICAL, "Классика") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Ритм-н-блюз") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Софт ритм-н-блюз") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Иностр. язык") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Духовная музыка") \
  X(PTY_NA_RELIGIOUS_TALK, "Религ. беседы") \
  X(PTY_NA_PERSONALITY, "Авторские") \
  X(PTY_NA_PUBLIC, "Общественное") \
  X(PTY_NA_COLLEGE, "Студенческое") \
  X(PTY_NA_SPANISH_TALK, "Беседы на исп.") \
  X(PTY_NA_SPANISH_MUSIC, "Испанская музыка") \
  X(PTY_NA_HIP_HOP, "Хип-хоп") \
  X(PTY_NA_UNASSIGNED, "Не назначено") \
  X(PTY_NA_WEATHER, "Погода") \
  X(PTY_NA_EMERGENCY_TEST, "Тест тревоги") \
  X(PTY_NA_EMERGENCY, "Тревога") \
  X(COVERAGE_LOCAL, "Местный") \
  X(COVERAGE_INTERNATIONAL, "Междунар.") \
  X(COVERAGE_NATIONAL, "Общенац.") \
  X(COVERAGE_SUPRA_REGIONAL, "Межрегион.") \
  X(COVERAGE_REGIONAL_1, "Регион 1") \
  X(COVERAGE_REGIONAL_2, "Регион 2") \
  X(COVERAGE_REGIONAL_3, "Регион 3") \
  X(COVERAGE_REGIONAL_4, "Регион 4") \
  X(COVERAGE_REGIONAL_5, "Регион 5") \
  X(COVERAGE_REGIONAL_6, "Регион 6") \
  X(COVERAGE_REGIONAL_7, "Регион 7") \
  X(COVERAGE_REGIONAL_8, "Регион 8") \
  X(COVERAGE_REGIONAL_9, "Регион 9") \
  X(COVERAGE_REGIONAL_10, "Регион 10") \
  X(COVERAGE_REGIONAL_11, "Регион 11") \
  X(COVERAGE_REGIONAL_12, "Регион 12") \
  X(RTP_OTHER, "ДРУГОЕ") \
  X(RTP_TITLE, "НАЗВАНИЕ") \
  X(RTP_ALBUM, "АЛЬБОМ") \
  X(RTP_TRACK, "ТРЕК") \
  X(RTP_ARTIST, "ИСПОЛН.") \
  X(RTP_WORK, "ПРОИЗВ.") \
  X(RTP_MOVEMENT, "ЧАСТЬ") \
  X(RTP_CONDUCTOR, "ДИРИЖЁР") \
  X(RTP_COMPOSER, "КОМПОЗИТОР") \
  X(RTP_BAND, "ГРУППА") \
  X(RTP_COMMENT, "КОММЕНТ.") \
  X(RTP_GENRE, "ЖАНР") \
  X(RTP_NEWS, "НОВОСТИ") \
  X(RTP_LOCAL, "МЕСТН. НОВ.") \
  X(RTP_STOCKS, "БИРЖА") \
  X(RTP_SPORT, "СПОРТ") \
  X(RTP_LOTTERY, "ЛОТЕРЕЯ") \
  X(RTP_HOROSCOPE, "ГОРОСКОП") \
  X(RTP_DIVERSION, "ДОСУГ") \
  X(RTP_HEALTH, "ЗДОРОВЬЕ") \
  X(RTP_EVENT, "СОБЫТИЕ") \
  X(RTP_SCENE, "СЦЕНА") \
  X(RTP_CINEMA, "КИНО") \
  X(RTP_FUN, "ЮМОР") \
  X(RTP_DATE, "ДАТА") \
  X(RTP_WEATHER, "ПОГОДА") \
  X(RTP_TRAFFIC, "ДОРОГИ") \
  X(RTP_ALARM, "ТРЕВОГА") \
  X(RTP_ADVERT, "РЕКЛАМА") \
  X(RTP_STATION, "СТАНЦИЯ") \
  X(RTP_PROGRAM, "ПРОГРАММА") \
  X(RTP_NEXT, "ДАЛЕЕ") \
  X(RTP_PART, "ЭПИЗОД") \
  X(RTP_HOST, "ВЕДУЩИЙ") \
  X(RTP_EDITORS, "РЕДАКЦИЯ") \
  X(RTP_FREQUENCY, "ЧАСТОТА") \
  X(RTP_HOMEPAGE, "САЙТ") \
  X(RTP_CHANNEL, "КАНАЛ") \
  X(RTP_HOTLINE, "ГОР. ЛИНИЯ") \
  X(RTP_STUDIO, "СТУДИЯ") \
  X(RTP_PHONE, "ТЕЛЕФОН") \
  X(RTP_CHAT, "ЧАТ") \
  X(RTP_VOTE, "ОПРОС") \
  X(RTP_PLACE, "МЕСТО") \
  X(RTP_APPOINTMENT, "ВСТРЕЧА") \
  X(RTP_PURCHASE, "ПОКУПКА") \
  X(DAY_SUNDAY, "ВОСКРЕСЕНЬЕ") \
  X(DAY_MONDAY, "ПОНЕДЕЛЬНИК") \
  X(DAY_TUESDAY, "ВТОРНИК") \
  X(DAY_WEDNESDAY, "СРЕДА") \
  X(DAY_THURSDAY, "ЧЕТВЕРГ") \
  X(DAY_FRIDAY, "ПЯТНИЦА") \
  X(DAY_SATURDAY, "СУББОТА") \
  X(MONTH_JANUARY, "января") \
  X(MONTH_FEBRUARY, "февраля") \
  X(MONTH_MARCH, "марта") \
  X(MONTH_APRIL, "апреля") \
  X(MONTH_MAY, "мая") \
  X(MONTH_JUNE, "июня") \
  X(MONTH_JULY, "июля") \
  X(MONTH_AUGUST, "августа") \
  X(MONTH_SEPTEMBER, "сентября") \
  X(MONTH_OCTOBER, "октября") \
  X(MONTH_NOVEMBER, "ноября") \
  X(MONTH_DECEMBER, "декабря") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_TODAY, "Сегодня") \
  X(DATE_YESTERDAY, "Вчера") \
  X(DATE_YDAY, "ВЧЕРА") \
  X(MONTH_SHORT_JAN, "янв") \
  X(MONTH_SHORT_FEB, "фев") \
  X(MONTH_SHORT_MAR, "мар") \
  X(MONTH_SHORT_APR, "апр") \
  X(MONTH_SHORT_MAY, "мая") \
  X(MONTH_SHORT_JUN, "июн") \
  X(MONTH_SHORT_JUL, "июл") \
  X(MONTH_SHORT_AUG, "авг") \
  X(MONTH_SHORT_SEP, "сен") \
  X(MONTH_SHORT_OCT, "окт") \
  X(MONTH_SHORT_NOV, "ноя") \
  X(MONTH_SHORT_DEC, "дек") \
  X(MONTH_TAG_JAN, "ЯНВ") \
  X(MONTH_TAG_FEB, "ФЕВ") \
  X(MONTH_TAG_MAR, "МАР") \
  X(MONTH_TAG_APR, "АПР") \
  X(MONTH_TAG_MAY, "МАЯ") \
  X(MONTH_TAG_JUN, "ИЮН") \
  X(MONTH_TAG_JUL, "ИЮЛ") \
  X(MONTH_TAG_AUG, "АВГ") \
  X(MONTH_TAG_SEP, "СЕН") \
  X(MONTH_TAG_OCT, "ОКТ") \
  X(MONTH_TAG_NOV, "НОЯ") \
  X(MONTH_TAG_DEC, "ДЕК") \
  X(DX_NOW, "сейчас") \
  X(DX_FMT_MIN_AGO, "%u мин назад") \
  X(DX_FMT_H_AGO, "%u ч назад") \
  X(DX_FMT_D_AGO, "%u дн. назад") \
  X(TOUCH_CAL_TITLE, "Калибровка сенсора") \
  X(TOUCH_CAL_HOLD, "Держите круг до заполнения") \
  X(TOUCH_CAL_CANCEL_HINT, "Поверните/нажмите ручку - отмена") \
  X(TOUCH_CAL_CHECK, "Проверка") \
  X(TOUCH_CAL_TAP_DOT, "Коснитесь точки") \
  X(TOUCH_CAL_SAVED, "Калибровка сохранена") \
  X(TOUCH_CAL_FMT_LANDED, "Проверка: отклонение %u пикс.") \
  X(TOUCH_CAL_BACK_HINT, "Нажмите ручку для возврата") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Не сохранено") \
  X(TOUCH_CAL_FMT_MISSED, "Проверка: отклонение %u пикс.,") \
  X(TOUCH_CAL_NO_FIT, "По меткам калибровка не вышла,") \
  X(TOUCH_CAL_NOT_SAVED, "Запись не удалась,") \
  X(TOUCH_CAL_OLD_STAYS, "осталась старая калибровка") \
  X(TOUCH_CAL_RETRY_HINT, "Нажмите ручку - повтор, поверните - выход")
// clang-format on
