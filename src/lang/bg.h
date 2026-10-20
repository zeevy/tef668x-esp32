/*
 * The panel's texts in Bulgarian. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_BG(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Сканиране FM") \
  X(MENU_SCAN_MW, "Сканиране AM") \
  X(MENU_SCAN_SW, "Сканиране SW") \
  X(MENU_SCAN_LW, "Сканиране LW") \
  X(MENU_SQUELCH, "Режим скуелч") \
  X(MENU_SQUELCH_LEVEL, "Ниво скуелч") \
  X(MENU_SQUELCH_FLOOR, "Минимум скуелч") \
  X(MENU_SQUELCH_GROUP, "Скуелч") \
  X(MENU_VOLUME_AGC, "AGC на звука") \
  X(MENU_AGC_BOOST, "Усилване AGC") \
  X(MENU_MUTE_RAMP, "Затихване") \
  X(MENU_BAND_PLAN, "Обхват") \
  X(MENU_TUNING_STEP, "Стъпка") \
  X(MENU_SEEK_SENSITIVITY, "Чувств. търсене") \
  X(MENU_DE_EMPHASIS, "Деемфаза") \
  X(MENU_FORCED_MONO, "Винаги моно") \
  X(MENU_STEREO_BLEND, "Смесване стерео") \
  X(MENU_ST_HI_BLEND, "Смесване стерео/ВЧ") \
  X(MENU_STEREO, "Стерео") \
  X(MENU_HIGH_CUT, "Отрез ВЧ") \
  X(MENU_NOISE_BLANKER, "Шумозаглушител") \
  X(MENU_EQUALISER, "Еквалайзер") \
  X(COMMON_RDS_DECODER, "RDS декодер") \
  X(MENU_RDS_REGION, "RDS регион") \
  X(MENU_MW_SPACING, "Стъпка MW") \
  X(MENU_FILTER_WIDTH, "Лента на филтъра") \
  X(MENU_SOFT_MUTE, "Меко заглуш.") \
  X(MENU_DWELL, "Пауза") \
  X(MENU_STOP_ON, "Спиране") \
  X(MENU_SCAN, "Сканиране") \
  X(MENU_MEMORY_FROM, "От канал") \
  X(MENU_MEMORY_TO, "До канал") \
  X(MENU_PRESET_RANGE, "Канали в паметта") \
  X(MENU_LOOP_THE_BAND, "Циклично") \
  X(MENU_DX_WIDTH, "Лента при скан.") \
  X(MENU_MUTE_WHILE_SCANNING, "Без звук при скан.") \
  X(MENU_AUTO_LOG_NEW, "Нови в дневника") \
  X(MENU_LOG_RADIO_TEXT, "RT в дневника") \
  X(MENU_WATCH_PRESETS, "Следи паметта") \
  X(MENU_SCOPE_BASELINE, "База на спектъра") \
  X(MENU_MEDIAN, "Медиана") \
  X(MENU_FMT_SWEEP_N, "Обхождане %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Научи местните") \
  X(MENU_START_SCAN, "Сканирай") \
  X(MENU_DAY_THEME, "Дневна тема") \
  X(MENU_NIGHT_THEME, "Нощна тема") \
  X(MENU_THEME, "Тема") \
  X(MENU_BRIGHTNESS, "Яркост") \
  X(MENU_DIM_LEVEL, "Яркост затъмн.") \
  X(MENU_DIM_AFTER, "Затъмни след") \
  X(MENU_ROTATION, "Завъртане екран") \
  X(COMMON_BATTERY, "Батерия") \
  X(MENU_BATTERY_CHECK, "Проверка батерия") \
  X(MENU_LANGUAGE, "Език \xC2\xB7 Language") \
  X(MENU_ENCODER, "Тип енкодер") \
  X(MENU_DIRECTION, "Посока енкодер") \
  X(MENU_ENCODER_GROUP, "Енкодер") \
  X(MENU_KEY_BEEPS, "Звук бутони") \
  X(MENU_BAND_EDGE_BEEP, "Звук край обхват") \
  X(MENU_START_CHIME, "Звук при старт") \
  X(MENU_KEYPAD_TIMEOUT, "Изчакване цифри") \
  X(MENU_TOUCH, "Сензор") \
  X(MENU_CALIBRATE_TOUCH, "Калибриране") \
  X(MENU_CLOCK_FROM_NETWORK, "Час от мрежата") \
  X(MENU_WEB_PIN, "Уеб PIN") \
  X(MENU_NEW_PIN, "Нов PIN") \
  X(MENU_FMT_DIGIT_OF, "Цифра %u от %u") \
  X(MENU_STATUS, "Състояние") \
  X(MENU_HOTSPOT, "Точка за достъп") \
  X(MENU_WEB_SERVER, "Уеб сървър") \
  X(MENU_FMT_PC_CONNECTED, "Свързани: %u") \
  X(MENU_WEB_ADDRESS, "Уеб адрес") \
  X(MENU_IP_ADDRESS, "IP адрес") \
  X(MENU_WI_FI_NAME, "Wi-Fi мрежа") \
  X(MENU_HOTSPOT_NAME, "Име на точката") \
  X(MENU_MAC, "MAC адрес") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi сигнал") \
  X(MENU_NETWORK_INFO, "Инфо мрежа") \
  X(MENU_RESTART, "Рестарт") \
  X(MENU_AUTO_OFF, "Автоизкл.") \
  X(MENU_UPDATE_CHECK, "Търси нова версия") \
  X(MENU_FIRMWARE_UPDATE, "Обновяване") \
  X(MENU_FMT_UPDATE_TO, "Обнови до %s") \
  X(MENU_UPDATE_TITLE, "Има нова версия") \
  X(MENU_UPDATE_THIS_RADIO, "Текуща") \
  X(MENU_UPDATE_NEW_VERSION, "Нова") \
  X(MENU_UPDATE_DOWNLOAD, "Размер") \
  X(COMMON_SETTINGS, "Настройки") \
  X(COMMON_KEPT, "Остават") \
  X(MENU_STATION, "Станция") \
  X(MENU_UPDATE_NOW, "Обнови") \
  X(MENU_LATER, "После") \
  X(MENU_UPDATE_UP_TO_DATE, "Актуален") \
  X(MENU_UPDATE_CHECKING, "Проверка") \
  X(MENU_UPDATE_NOT_CHECKED, "Непроверено") \
  X(MENU_UPDATE_CHECK_FAILED, "Грешка") \
  X(MENU_NOTE_NO_UPDATE, "Няма нова версия") \
  X(MENU_VERSION, "Версия фърмуер") \
  X(MENU_RUNNING_FROM, "Зареждане от") \
  X(MENU_UPTIME, "Работи от") \
  X(MENU_BATTERY_VOLTAGE, "Напреж. батерия") \
  X(COMMON_TUNER, "Тунер") \
  X(MENU_RESET_REASON, "Причина рестарт") \
  X(MENU_CPU_CORE_0, "CPU ядро 0") \
  X(MENU_CPU_CORE_1, "CPU ядро 1") \
  X(MENU_FREE_HEAP, "Своб. RAM") \
  X(MENU_LOWEST_HEAP, "Мин. RAM") \
  X(MENU_LARGEST_BLOCK, "Макс. блок") \
  X(MENU_LVGL_POOL, "LVGL пул") \
  X(MENU_CHIP, "Чип") \
  X(MENU_FLASH_SIZE, "Флаш памет") \
  X(RESET_POWER, "Включване") \
  X(RESET_SOFTWARE, "Софтуерен") \
  X(RESET_PANIC, "Паника") \
  X(RESET_INT_WATCHDOG, "WDT IRQ") \
  X(RESET_TASK_WATCHDOG, "WDT задача") \
  X(RESET_WATCHDOG, "WDT") \
  X(RESET_DEEP_SLEEP, "Дълбок сън") \
  X(RESET_BROWNOUT, "Спад напр.") \
  X(RESET_PIN, "Бутон рестарт") \
  X(RESET_UNKNOWN, "Неизвестна") \
  X(RESET_ASKED, "По заявка") \
  X(RESET_BOOT_WATCHDOG, "WDT зареждане") \
  X(RESET_ROLLBACK, "Връщане") \
  X(RESET_DISPLAY, "Грешка екран") \
  X(RESET_UPDATE, "Обновяване") \
  X(MENU_DEVELOPER, "Разработчик") \
  X(MENU_BUILD, "Билд") \
  X(MENU_LICENSE, "Лиценз") \
  X(ABOUT_BUILD_UNKNOWN, "неизвестен") \
  X(MENU_GO_TO, "Бърз достъп") \
  X(MENU_GO_TO_BANDWIDTH, "Лента") \
  X(MENU_GO_TO_DX, "DX режим") \
  X(MENU_GO_TO_SCOPE, "Спектър") \
  X(MENU_GO_TO_BAND, "Следващ обхват") \
  X(MENU_GO_TO_LOG, "Запиши в дневника") \
  X(MENU_GO_TO_SAVE, "Запиши в паметта") \
  X(MENU_FMT_SAVED_AS, "Записано в P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Вече в P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Паметта е пълна") \
  X(MENU_GO_TO_SLEEP, "Сън") \
  X(MENU_STATIONS, "Станции") \
  X(MENU_MEMORY, "Памет") \
  X(MENU_AUDIO, "Звук") \
  X(MENU_FM_SETUP, "FM приемане") \
  X(MENU_AM_SETUP, "AM приемане") \
  X(MENU_DX_SETUP, "DX скенер") \
  X(MENU_DISPLAY, "Екран") \
  X(MENU_CONTROLS, "Управление") \
  X(MENU_CONNECTIVITY, "Мрежа") \
  X(MENU_SYSTEM, "Система") \
  X(MENU_DIAGNOSTICS, "Диагностика") \
  X(MENU_ABOUT, "Относно") \
  X(MENU_REGION_FULL, "Пълен") \
  X(MENU_REGION_JAPAN, "Япония") \
  X(MENU_REGION_WIDE, "Широк") \
  X(MENU_REGION_WORLD, "Световен") \
  X(MENU_RDS_REGION_EUROPE, "Европа") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Сев. Америка") \
  X(MENU_WIFI_JOINED, "Свързан") \
  X(MENU_WIFI_JOINING, "Свързване") \
  X(MENU_WIFI_HOTSPOT, "Точка за достъп") \
  X(COMMON_OFF, "Изкл") \
  X(MENU_FMT_KB_OF_KB, "%u от %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V при старт") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, преди %u мин") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, преди %u ч") \
  X(MENU_FMT_AGC_GAIN_NOW, "Усилване: %+d dB") \
  X(MENU_STATION_LOG, "Дневник") \
  X(COMMON_NONE, "Няма") \
  X(MENU_FMT_SCAN_PROGRESS, "Скан. %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Записани: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Спряно, записани: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Няма място: %u") \
  X(COMMON_YES, "Да") \
  X(COMMON_NO, "Не") \
  X(MENU_UNKNOWN_VALUE, "Неизвестен") \
  X(COMMON_ROTATION_NORMAL, "Нормално") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Обърнато") \
  X(MENU_CUT_ONLY, "Без усилване") \
  X(MENU_NEVER, "Никога") \
  X(MENU_BATTERY_PER_CENT, "Процент") \
  X(MENU_BATTERY_VOLTS, "Волтове") \
  X(MENU_KEYS, "Само цифри") \
  X(MENU_KEYS_LONG, "Кратко и дълго") \
  X(MENU_EVERY_PRESS, "Всички") \
  X(MENU_STANDARD, "Стандартен") \
  X(MENU_OPTICAL, "Оптичен") \
  X(MENU_NORMAL, "Нормална") \
  X(MENU_REVERSED, "Обратна") \
  X(MENU_AUTO, "Авто") \
  X(MENU_SQUELCH_MAN, "Ръчен") \
  X(MENU_ANY_PI, "Всеки PI") \
  X(MENU_STOP_NEVER, "Никога") \
  X(MENU_NEW_ONLY, "Само нови") \
  X(MENU_WHOLE_BAND, "Целия обхват") \
  X(MENU_MEMORY_ONLY, "Само паметта") \
  X(MENU_BAND_MEMORY, "Обхват + памет") \
  X(COMMON_ON, "Вкл") \
  X(MENU_HINT_RESTART, "Важи след рестарт") \
  X(MENU_TITLE, "Меню") \
  X(COMMON_FM_ONLY, "Само FM") \
  X(MENU_NOTE_OFF_PLAN, "Извън обхвата") \
  X(MENU_NOTE_SWITCH_TO_FM, "Изберете FM") \
  X(MENU_NOTE_RADIO_BUSY, "Заето - опитайте пак") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Включете RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Пробна версия - чакайте") \
  X(MENU_NOTE_NOT_STORED, "Незаписано - върнато") \
  X(MENU_NOTE_NOT_PUT_BACK, "Заето - не е върнато") \
  X(MENU_NOTE_NOT_SAVED, "Незаписано") \
  X(MENU_SAVE, "Запиши") \
  X(RADIO_PRESS_TO_START, "Натиснете за старт") \
  X(COMMON_STILL_TUNING, "Търсене - чакайте") \
  X(COMMON_SCAN_RUNNING, "Сканиране - чакайте") \
  X(RADIO_FMT_NO_BAND, "%s: няма обхват") \
  X(COMMON_NOT_LOGGED, "Незаписано") \
  X(RADIO_FMT_LOGGED, "В дневника: %s") \
  X(RADIO_CHECKING_UPDATES, "ПРОВЕРКА ЗА НОВА ВЕРСИЯ\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Сканиране %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Неуспешно обновяване") \
  X(RADIO_GOING_TO_SLEEP, "Заспиване") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Натиснете копчето за събуждане") \
  X(RADIO_OLD_IMAGE_KEPT, "Старият фърмуер остава") \
  X(RDS_STEREO, "Стерео") \
  X(RDS_MONO, "Моно") \
  X(RDS_SPEECH, "Говор") \
  X(RDS_MUSIC, "Музика") \
  X(COMMON_ALREADY_LOGGED, "Вече в дневника") \
  X(DX_RDS_IS_OFF, "RDS изкл.") \
  X(DX_SCANNING, "Сканиране") \
  X(DX_SCAN_FINISHED, "Скан. завърши") \
  X(DX_NOTHING_TO_SCAN, "Нищо за сканиране") \
  X(DX_CHECKING_FOR_UPDATES, "Проверка за нова версия") \
  X(DX_A_SCAN_IS_RUNNING, "Вече сканира") \
  X(DX_CAUGHT_LIST_UNREAD, "Уловът е недостъпен") \
  X(DX_NO_MEMORY, "Празна памет") \
  X(DX_NOT_STARTED, "Не е пуснато") \
  X(DX_SWEEPING, "Обхождане") \
  X(COMMON_FMT_TUNE_TYPED, "Към %s") \
  X(RADIO_SQL_OFF, "ИЗКЛ") \
  X(COMMON_AUTO, "АВТО") \
  X(RADIO_TUNE_MANUAL, "РЪЧ") \
  X(RADIO_TUNE_AUTO, "АВТО") \
  X(RADIO_TUNE_MEMORY, "ПАМ") \
  X(RADIO_TUNE_METER, "МТР") \
  X(DX_FMT_ROWS_OF, "%u-%u от %u") \
  X(DX_FMT_FOUND, "Открити: %u") \
  X(DX_MODE_SWEEPING, "ОБХОЖДАНЕ") \
  X(DX_MODE_LEARN_LOCALS, "УЧИ МЕСТНИ") \
  X(DX_FMT_MODE_MEMORY, "ПАМЕТ %u-%u") \
  X(DX_MODE_WHOLE_BAND, "ЦЕЛИЯ ОБХВАТ") \
  X(DX_MODE_BAND_MEMORY, "ОБХВАТ + ПАМЕТ") \
  X(DX_RULE_NO_STOP, "БЕЗ СТОП") \
  X(DX_RULE_STOP_ON_PI, "СТОП ПРИ PI") \
  X(DX_RULE_STOP_ON_NEW, "СТОП ПРИ НОВА") \
  X(DX_FMT_NOT_PRESET, "не P%02d") \
  X(DX_FMT_CHANNEL, "Кан. %u") \
  X(DX_CONTEXT_SWEEPING, "обхождане") \
  X(DX_PRESS_TO_SWEEP, "Натиснете за обхождане") \
  X(SCOPE_FMT_TITLE, "%s спектър") \
  X(SCOPE_FULL, "Цял") \
  X(DX_FIXED, "ФИКС.") \
  X(DX_FMT_VS, "БАЗА %s") \
  X(DX_FMT_MEDIAN_OF, "МЕДИАНА ОТ %u") \
  X(DX_FMT_FLOOR, "ШУМ %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Затвори") \
  X(BW_FMT_AUTO_AT, "Авто: %u kHz") \
  X(RECOVERY_PRESS_AGAIN, "Натиснете пак") \
  X(PAGE_TUNE, "Настрой") \
  X(PAGE_DELETE, "Изтрий") \
  X(PAGE_LEVEL, "Ниво") \
  X(PAGE_SNR, "С/Ш") \
  X(PAGE_NOISE, "Шум") \
  X(PAGE_MULTIPATH, "Многолъч.") \
  X(PAGE_COCHANNEL, "Съканални") \
  X(PAGE_STEREO, "Стерео") \
  X(PAGE_WIDTH, "Лента") \
  X(PAGE_BAND, "Обхват") \
  X(PAGE_NOTE_CHANGED, "Променено след отваряне") \
  X(PAGE_NOTE_NOT_DELETED, "Не е изтрито") \
  X(PAGE_FMT_SAVED, "Записано %s") \
  X(PAGE_TIME_UNKNOWN, "Неизвестен час") \
  X(MENU_FMT_TUNER_PATCH, "%s пач %u") \
  X(BW_TITLE, "Лента") \
  X(KEYPAD_TITLE, "Честота") \
  X(KEYPAD_CANCEL, "Отказ") \
  X(DX_CATCHES_EMPTY_TITLE, "Още няма улов") \
  X(DX_CATCHES_EMPTY_NOTE, "Тук ще са потвърдените станции от сесията") \
  X(DX_TITLE_CATCHES, "Улов") \
  X(DX_NEW, "НОВ") \
  X(DX_READY, "Готов") \
  X(DX_DWELL, "ПАУЗА") \
  X(DX_STOPPED, "СТОП") \
  X(DX_TITLE_SCANNER, "Скенер") \
  X(DX_TITLE_SCOPE, "Спектър") \
  X(DX_LEVEL, "НИВО") \
  X(DX_RISE, "РЪСТ") \
  X(DX_NO_STATION_YET, "Няма станция") \
  X(DX_NO_RDS_YET, "Няма RDS") \
  X(DX_OFFSET, "ОТКЛ.") \
  X(DX_MOD, "МОД") \
  X(DX_NO_ID, "БЕЗ ID") \
  X(RDS_TITLE_TEXT, "Радиотекст") \
  X(RDS_TITLE_NETWORKS, "Мрежи") \
  X(RDS_TITLE_DECODER, "Декодер") \
  X(RDS_NO_COUNTRY, "няма ECC") \
  X(RDS_MS_UNKNOWN, "М/Г") \
  X(RDS_RTPLUS_NONE, "Няма RT+ от тази станция") \
  X(RDS_RTPLUS_WAIT, "Още няма RT+ тагове за текста") \
  X(RDS_AF_TITLE, "АЛТЕРНАТИВНИ ЧЕСТОТИ") \
  X(RDS_EON_TITLE, "ДРУГИ СТАНЦИИ, EON") \
  X(RDS_NONE_HEARD, "Няма данни") \
  X(RDS_SYNC_CAPS, "СИНХР.") \
  X(RDS_GROUPS_S_CAPS, "ГРУПИ/С") \
  X(RDS_LOST, "ЗАГУБИ") \
  X(RDS_EACH_BLOCK, "ПО БЛОКОВЕ") \
  X(RDS_LEGEND_CLEAN, "чисти") \
  X(RDS_LEGEND_FIXED, "попр.") \
  X(RDS_LEGEND_LOST, "загуби") \
  X(RDS_ALL_CLEAN, "чисто") \
  X(RDS_GROUP_TYPES, "ТИПОВЕ ГРУПИ") \
  X(RDS_NO_GROUPS, "Няма групи за минута") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 за %u с") \
  X(RDS_COUNTRY_UNLISTED, "непозната") \
  X(RDS_NO_TYPES, "Няма чист блок B за последната минута") \
  X(RDS_FMT_EON_MORE, "+%u още") \
  X(RDS_EON_MORE_HEARD, "+ още") \
  X(RDS_MORE, "още") \
  X(RADIO_VEIL_UPDATING, "ОБНОВЯВАНЕ") \
  X(RADIO_VEIL_KEEP_POWER, "Не изключвайте") \
  X(RADIO_MUTE, "ТИХО") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Няма тунер на шината I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Тунерът не е готов") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Тунерът не е разпознат") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Няма пач за този тунер") \
  X(TUNER_UNKNOWN_TUNER_PART, "Непознат тунер") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Тунерът отказа запис") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Тунерът не отговори при четене") \
  X(TUNER_OUT_OF_RANGE, "Извън границите") \
  X(TUNER_UNKNOWN, "Неизвестен") \
  X(BAND_LONG_LW, "Дълги вълни") \
  X(BAND_LONG_MW, "Средни вълни") \
  X(BAND_LONG_SW, "Къси вълни") \
  X(MENU_FMT_TUNE_TO, "Обхват за %s") \
  X(PTY_NONE, "Няма") \
  X(PTY_NEWS, "Новини") \
  X(PTY_CURRENT_AFFAIRS, "Актуално") \
  X(PTY_INFORMATION, "Информация") \
  X(PTY_SPORT, "Спорт") \
  X(PTY_EDUCATION, "Образование") \
  X(PTY_DRAMA, "Драма") \
  X(PTY_CULTURE, "Култура") \
  X(PTY_SCIENCE, "Наука") \
  X(PTY_VARIED, "Разни") \
  X(PTY_POP_MUSIC, "Поп музика") \
  X(PTY_ROCK_MUSIC, "Рок музика") \
  X(PTY_EASY_LISTENING, "Лека музика") \
  X(PTY_LIGHT_CLASSICAL, "Лека класика") \
  X(PTY_SERIOUS_CLASSICAL, "Класика") \
  X(PTY_OTHER_MUSIC, "Друга музика") \
  X(PTY_WEATHER, "Времето") \
  X(PTY_FINANCE, "Финанси") \
  X(PTY_CHILDREN, "Детски") \
  X(PTY_SOCIAL_AFFAIRS, "Общество") \
  X(PTY_RELIGION, "Религия") \
  X(PTY_PHONE_IN, "Обаждания") \
  X(PTY_TRAVEL, "Туризъм") \
  X(PTY_LEISURE, "Отдих") \
  X(PTY_JAZZ_MUSIC, "Джаз") \
  X(PTY_COUNTRY_MUSIC, "Кънтри") \
  X(PTY_NATIONAL_MUSIC, "Нац. музика") \
  X(PTY_OLDIES_MUSIC, "Ретро") \
  X(PTY_FOLK_MUSIC, "Народна") \
  X(PTY_DOCUMENTARY, "Документални") \
  X(PTY_ALARM_TEST, "Тест аларма") \
  X(PTY_ALARM, "Аларма") \
  X(PTY_NA_NEWS, "Новини") \
  X(PTY_NA_INFORMATION, "Информация") \
  X(PTY_NA_SPORTS, "Спорт") \
  X(PTY_NA_TALK, "Говор") \
  X(PTY_NA_ROCK, "Рок") \
  X(PTY_NA_CLASSIC_ROCK, "Класически рок") \
  X(PTY_NA_ADULT_HITS, "Хитове за възрастни") \
  X(PTY_NA_SOFT_ROCK, "Софт рок") \
  X(PTY_NA_TOP_40, "Топ 40") \
  X(PTY_NA_COUNTRY, "Кънтри") \
  X(PTY_NA_OLDIES, "Ретро") \
  X(PTY_NA_SOFT, "Лека") \
  X(PTY_NA_NOSTALGIA, "Носталгия") \
  X(PTY_NA_JAZZ, "Джаз") \
  X(PTY_NA_CLASSICAL, "Класика") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Ритъм и блус") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Лек ритъм и блус") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Чужд език") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Религ. музика") \
  X(PTY_NA_RELIGIOUS_TALK, "Религ. разговори") \
  X(PTY_NA_PERSONALITY, "Личности") \
  X(PTY_NA_PUBLIC, "Обществено") \
  X(PTY_NA_COLLEGE, "Студентско") \
  X(PTY_NA_SPANISH_TALK, "Исп. разговори") \
  X(PTY_NA_SPANISH_MUSIC, "Испанска музика") \
  X(PTY_NA_HIP_HOP, "Хип-хоп") \
  X(PTY_NA_UNASSIGNED, "Незададен") \
  X(PTY_NA_WEATHER, "Времето") \
  X(PTY_NA_EMERGENCY_TEST, "Тест аларма") \
  X(PTY_NA_EMERGENCY, "Аларма") \
  X(COVERAGE_LOCAL, "Местен") \
  X(COVERAGE_INTERNATIONAL, "Междунар.") \
  X(COVERAGE_NATIONAL, "Национален") \
  X(COVERAGE_SUPRA_REGIONAL, "Надрегион.") \
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
  X(RTP_OTHER, "ДРУГО") \
  X(RTP_TITLE, "ЗАГЛАВИЕ") \
  X(RTP_ALBUM, "АЛБУМ") \
  X(RTP_TRACK, "ПИСТА") \
  X(RTP_ARTIST, "ИЗПЪЛНИТЕЛ") \
  X(RTP_WORK, "ТВОРБА") \
  X(RTP_MOVEMENT, "ЧАСТ") \
  X(RTP_CONDUCTOR, "ДИРИГЕНТ") \
  X(RTP_COMPOSER, "КОМПОЗИТОР") \
  X(RTP_BAND, "ГРУПА") \
  X(RTP_COMMENT, "КОМЕНТАР") \
  X(RTP_GENRE, "ЖАНР") \
  X(RTP_NEWS, "НОВИНИ") \
  X(RTP_LOCAL, "МЕСТНИ НОВ.") \
  X(RTP_STOCKS, "БОРСА") \
  X(RTP_SPORT, "СПОРТ") \
  X(RTP_LOTTERY, "ЛОТАРИЯ") \
  X(RTP_HOROSCOPE, "ХОРОСКОП") \
  X(RTP_DIVERSION, "ОТДИХ") \
  X(RTP_HEALTH, "ЗДРАВЕ") \
  X(RTP_EVENT, "СЪБИТИЕ") \
  X(RTP_SCENE, "СЦЕНА") \
  X(RTP_CINEMA, "КИНО") \
  X(RTP_FUN, "ХУМОР") \
  X(RTP_DATE, "ДАТА") \
  X(RTP_WEATHER, "ВРЕМЕТО") \
  X(RTP_TRAFFIC, "ТРАФИК") \
  X(RTP_ALARM, "АЛАРМА") \
  X(RTP_ADVERT, "РЕКЛАМА") \
  X(RTP_STATION, "СТАНЦИЯ") \
  X(RTP_PROGRAM, "ПРЕДАВАНЕ") \
  X(RTP_NEXT, "СЛЕДВА") \
  X(RTP_PART, "ЕПИЗОД") \
  X(RTP_HOST, "ВОДЕЩ") \
  X(RTP_EDITORS, "РЕДАКЦИЯ") \
  X(RTP_FREQUENCY, "ЧЕСТОТА") \
  X(RTP_HOMEPAGE, "САЙТ") \
  X(RTP_CHANNEL, "КАНАЛ") \
  X(RTP_HOTLINE, "ГОР. ЛИНИЯ") \
  X(RTP_STUDIO, "СТУДИО") \
  X(RTP_PHONE, "ТЕЛЕФОН") \
  X(RTP_EMAIL, "ИМЕЙЛ") \
  X(RTP_CHAT, "ЧАТ") \
  X(RTP_VOTE, "ГЛАСУВАНЕ") \
  X(RTP_PLACE, "МЯСТО") \
  X(RTP_APPOINTMENT, "СРЕЩА") \
  X(RTP_PURCHASE, "ПОКУПКА") \
  X(DAY_SUNDAY, "НЕДЕЛЯ") \
  X(DAY_MONDAY, "ПОНЕДЕЛНИК") \
  X(DAY_TUESDAY, "ВТОРНИК") \
  X(DAY_WEDNESDAY, "СРЯДА") \
  X(DAY_THURSDAY, "ЧЕТВЪРТЪК") \
  X(DAY_FRIDAY, "ПЕТЪК") \
  X(DAY_SATURDAY, "СЪБОТА") \
  X(MONTH_JANUARY, "януари") \
  X(MONTH_FEBRUARY, "февруари") \
  X(MONTH_MARCH, "март") \
  X(MONTH_APRIL, "април") \
  X(MONTH_MAY, "май") \
  X(MONTH_JUNE, "юни") \
  X(MONTH_JULY, "юли") \
  X(MONTH_AUGUST, "август") \
  X(MONTH_SEPTEMBER, "септември") \
  X(MONTH_OCTOBER, "октомври") \
  X(MONTH_NOVEMBER, "ноември") \
  X(MONTH_DECEMBER, "декември") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_TODAY, "Днес") \
  X(DATE_YESTERDAY, "Вчера") \
  X(DATE_YDAY, "ВЧЕРА") \
  X(MONTH_SHORT_JAN, "яну") \
  X(MONTH_SHORT_FEB, "фев") \
  X(MONTH_SHORT_MAR, "мар") \
  X(MONTH_SHORT_APR, "апр") \
  X(MONTH_SHORT_MAY, "май") \
  X(MONTH_SHORT_JUN, "юни") \
  X(MONTH_SHORT_JUL, "юли") \
  X(MONTH_SHORT_AUG, "авг") \
  X(MONTH_SHORT_SEP, "сеп") \
  X(MONTH_SHORT_OCT, "окт") \
  X(MONTH_SHORT_NOV, "ное") \
  X(MONTH_SHORT_DEC, "дек") \
  X(MONTH_TAG_JAN, "ЯНУ") \
  X(MONTH_TAG_FEB, "ФЕВ") \
  X(MONTH_TAG_MAR, "МАР") \
  X(MONTH_TAG_APR, "АПР") \
  X(MONTH_TAG_MAY, "МАЙ") \
  X(MONTH_TAG_JUN, "ЮНИ") \
  X(MONTH_TAG_JUL, "ЮЛИ") \
  X(MONTH_TAG_AUG, "АВГ") \
  X(MONTH_TAG_SEP, "СЕП") \
  X(MONTH_TAG_OCT, "ОКТ") \
  X(MONTH_TAG_NOV, "НОЕ") \
  X(MONTH_TAG_DEC, "ДЕК") \
  X(DX_NOW, "сега") \
  X(DX_FMT_MIN_AGO, "%u мин") \
  X(DX_FMT_H_AGO, "%u ч") \
  X(DX_FMT_D_AGO, "%u дн.") \
  X(TOUCH_CAL_TITLE, "Калибриране") \
  X(TOUCH_CAL_HOLD, "Задръжте кръга до запълване") \
  X(TOUCH_CAL_CANCEL_HINT, "Завъртете/натиснете копчето - отказ") \
  X(TOUCH_CAL_CHECK, "Проверка") \
  X(TOUCH_CAL_TAP_DOT, "Докоснете точката") \
  X(TOUCH_CAL_SAVED, "Калибрирането е записано") \
  X(TOUCH_CAL_FMT_LANDED, "Проверка: отклонение %u px") \
  X(TOUCH_CAL_BACK_HINT, "Натиснете копчето за връщане") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Незаписано") \
  X(TOUCH_CAL_FMT_MISSED, "Проверка: отклонение %u px,") \
  X(TOUCH_CAL_NO_FIT, "Точките не дават калибриране,") \
  X(TOUCH_CAL_NOT_SAVED, "Записът не успя,") \
  X(TOUCH_CAL_OLD_STAYS, "остава старото калибриране") \
  X(TOUCH_CAL_RETRY_HINT, "Натиснете копчето - нов опит, завъртете - изход")
// clang-format on
