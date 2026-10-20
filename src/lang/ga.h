/*
 * The panel's texts in Irish. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_GA(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Scanadh FM") \
  X(MENU_SCAN_MW, "Scanadh AM") \
  X(MENU_SCAN_SW, "Scanadh SW") \
  X(MENU_SCAN_LW, "Scanadh LW") \
  X(MENU_SQUELCH, "Mód squelch") \
  X(MENU_SQUELCH_LEVEL, "Leibhéal squelch") \
  X(MENU_SQUELCH_FLOOR, "Urlár squelch") \
  X(MENU_VOLUME_AGC, "AGC fuaime") \
  X(MENU_AGC_BOOST, "Treisiú AGC") \
  X(MENU_MUTE_RAMP, "Rampa ciúnaithe") \
  X(MENU_BAND_PLAN, "Plean banda") \
  X(MENU_TUNING_STEP, "Céim tiúnta") \
  X(MENU_SEEK_SENSITIVITY, "Íogaireacht cuardaigh") \
  X(MENU_DE_EMPHASIS, "Dí-bhéim") \
  X(MENU_FORCED_MONO, "Mono i gcónaí") \
  X(MENU_STEREO_BLEND, "Cumasc steiréó") \
  X(MENU_ST_HI_BLEND, "Cumasc steiréó / ard") \
  X(MENU_STEREO, "Steiréó") \
  X(MENU_HIGH_CUT, "High cut") \
  X(MENU_NOISE_BLANKER, "Noise blanker") \
  X(MENU_EQUALISER, "Cothromóir") \
  X(COMMON_RDS_DECODER, "Díchódóir RDS") \
  X(MENU_RDS_REGION, "Réigiún RDS") \
  X(MENU_MW_SPACING, "Céim MW") \
  X(MENU_FILTER_WIDTH, "Leithead scagaire") \
  X(MENU_SOFT_MUTE, "Ciúnú bog") \
  X(MENU_DWELL, "Fanacht") \
  X(MENU_STOP_ON, "Stad ar") \
  X(MENU_SCAN, "Raon scanta") \
  X(MENU_MEMORY_FROM, "Cuimhne ó") \
  X(MENU_MEMORY_TO, "Cuimhne go") \
  X(MENU_PRESET_RANGE, "Raon cuimhne") \
  X(MENU_LOOP_THE_BAND, "Lúb an banda") \
  X(MENU_DX_WIDTH, "Leithead scanta") \
  X(MENU_MUTE_WHILE_SCANNING, "Ciúin ag scanadh") \
  X(MENU_AUTO_LOG_NEW, "Uathlogáil stáisiún") \
  X(MENU_LOG_RADIO_TEXT, "Logáil téacs raidió") \
  X(MENU_WATCH_PRESETS, "Faire ar chuimhne") \
  X(MENU_SCOPE_BASELINE, "Bunlíne scóip") \
  X(MENU_MEDIAN, "Airmheán") \
  X(MENU_FMT_SWEEP_N, "Scuabadh %u") \
  X(DX_FMT_WATCH_UP, "%s suas %d dB") \
  X(MENU_LEARN_LOCALS, "Foghlaim stáisiúin áitiúla") \
  X(MENU_START_SCAN, "Tosaigh scanadh") \
  X(MENU_DAY_THEME, "Téama lae") \
  X(MENU_NIGHT_THEME, "Téama oíche") \
  X(MENU_THEME, "Téama") \
  X(MENU_BRIGHTNESS, "Gile") \
  X(MENU_DIM_LEVEL, "Gile íseal") \
  X(MENU_DIM_AFTER, "Ísligh tar éis") \
  X(MENU_ROTATION, "Rothlú scáileáin") \
  X(COMMON_BATTERY, "Ceallra") \
  X(MENU_BATTERY_CHECK, "Seiceáil ceallra") \
  X(MENU_LANGUAGE, "Teanga \xC2\xB7 Language") \
  X(MENU_ENCODER, "Cineál") \
  X(MENU_DIRECTION, "Treo") \
  X(MENU_ENCODER_GROUP, "Ionchódóir") \
  X(MENU_KEY_BEEPS, "Bípeanna") \
  X(MENU_BAND_EDGE_BEEP, "Bíp imeall banda") \
  X(MENU_START_CHIME, "Fuaim tosaithe") \
  X(MENU_KEYPAD_TIMEOUT, "Feitheamh eochrach") \
  X(MENU_TOUCH, "Tadhall") \
  X(MENU_CALIBRATE_TOUCH, "Calabraigh tadhall") \
  X(MENU_CLOCK_FROM_NETWORK, "Am líonra") \
  X(MENU_WEB_PIN, "PIN gréasáin") \
  X(MENU_NEW_PIN, "PIN nua") \
  X(MENU_FMT_DIGIT_OF, "Digit %u as %u") \
  X(MENU_STATUS, "Stádas ceangail") \
  X(MENU_WEB_SERVER, "Freastalaí gréasáin") \
  X(MENU_FMT_PC_CONNECTED, "%u ceangailte") \
  X(MENU_WEB_ADDRESS, "Seoladh gréasáin") \
  X(MENU_IP_ADDRESS, "Seoladh IP") \
  X(MENU_WI_FI_NAME, "Líonra Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Ainm hotspot") \
  X(MENU_MAC, "Seoladh MAC") \
  X(MENU_WI_FI_SIGNAL, "Neart Wi-Fi") \
  X(MENU_NETWORK_INFO, "Eolas líonra") \
  X(MENU_RESTART, "Atosaigh") \
  X(MENU_AUTO_OFF, "Múch tar éis") \
  X(MENU_UPDATE_CHECK, "Lorg nuashonruithe") \
  X(MENU_FIRMWARE_UPDATE, "Nuashonrú") \
  X(MENU_FMT_UPDATE_TO, "Nuashonrú %s") \
  X(MENU_UPDATE_TITLE, "Nuashonrú ar fáil") \
  X(MENU_UPDATE_THIS_RADIO, "An raidió seo") \
  X(MENU_UPDATE_NEW_VERSION, "Leagan nua") \
  X(MENU_UPDATE_DOWNLOAD, "Íoslódáil") \
  X(COMMON_SETTINGS, "Socruithe") \
  X(COMMON_KEPT, "Coinnithe") \
  X(MENU_STATION, "Stáisiún") \
  X(MENU_UPDATE_NOW, "Suiteáil") \
  X(MENU_LATER, "Ar ball") \
  X(MENU_UPDATE_UP_TO_DATE, "Cothrom le dáta") \
  X(MENU_UPDATE_CHECKING, "Á sheiceáil") \
  X(MENU_UPDATE_NOT_CHECKED, "Gan seiceáil") \
  X(MENU_UPDATE_CHECK_FAILED, "Theip ar sheiceáil") \
  X(MENU_NOTE_NO_UPDATE, "Gan nuashonrú nua") \
  X(MENU_VERSION, "Leagan bogearraí") \
  X(MENU_RUNNING_FROM, "Foinse tosaithe") \
  X(MENU_UPTIME, "Am ar siúl") \
  X(MENU_BATTERY_VOLTAGE, "Voltas ceallra") \
  X(COMMON_TUNER, "Tiúnóir") \
  X(MENU_RESET_REASON, "Cúis atosaithe") \
  X(MENU_CPU_CORE_0, "Croí CPU 0") \
  X(MENU_CPU_CORE_1, "Croí CPU 1") \
  X(MENU_FREE_HEAP, "Carn saor") \
  X(MENU_LOWEST_HEAP, "Carn is ísle") \
  X(MENU_LARGEST_BLOCK, "Bloc is mó") \
  X(MENU_LVGL_POOL, "Linn LVGL") \
  X(MENU_CHIP, "Slis") \
  X(MENU_FLASH_SIZE, "Méid flash") \
  X(RESET_POWER, "Cumhacht air") \
  X(RESET_SOFTWARE, "Bogearraí") \
  X(RESET_PANIC, "Scaoll") \
  X(RESET_INT_WATCHDOG, "Watchdog IRQ") \
  X(RESET_TASK_WATCHDOG, "Watchdog taisc") \
  X(RESET_DEEP_SLEEP, "Codladh domhain") \
  X(RESET_BROWNOUT, "Ísliú voltais") \
  X(RESET_PIN, "Biorán RST") \
  X(RESET_UNKNOWN, "Anaithnid") \
  X(RESET_ASKED, "Atosú iarrtha") \
  X(RESET_BOOT_WATCHDOG, "Watchdog tosaithe") \
  X(RESET_ROLLBACK, "Rolladh siar") \
  X(RESET_DISPLAY, "Fabht scáileáin") \
  X(RESET_UPDATE, "Nuashonrú") \
  X(MENU_DEVELOPER, "Forbróir") \
  X(MENU_BUILD, "Tógáil") \
  X(MENU_LICENSE, "Ceadúnas") \
  X(ABOUT_BUILD_UNKNOWN, "anaithnid") \
  X(MENU_GO_TO, "Téigh go") \
  X(MENU_GO_TO_BANDWIDTH, "Bandaleithead") \
  X(MENU_GO_TO_DX, "Mód DX") \
  X(MENU_GO_TO_SCOPE, "Scóp banda") \
  X(MENU_GO_TO_BAND, "Banda eile") \
  X(MENU_GO_TO_LOG, "Logáil an stáisiún") \
  X(MENU_GO_TO_SAVE, "Sábháil an stáisiún") \
  X(MENU_FMT_SAVED_AS, "Sábháilte: P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "P%02d cheana") \
  X(MENU_NOTE_PRESETS_FULL, "Cuimhne lán") \
  X(MENU_GO_TO_SLEEP, "Codladh") \
  X(MENU_STATIONS, "Stáisiúin") \
  X(MENU_MEMORY, "Cuimhne") \
  X(MENU_AUDIO, "Fuaim") \
  X(MENU_FM_SETUP, "Glacadh FM") \
  X(MENU_AM_SETUP, "Glacadh AM") \
  X(MENU_DX_SETUP, "Scanóir DX") \
  X(MENU_DISPLAY, "Scáileán") \
  X(MENU_CONTROLS, "Rialtáin") \
  X(MENU_CONNECTIVITY, "Nascacht") \
  X(MENU_SYSTEM, "Córas") \
  X(MENU_DIAGNOSTICS, "Diagnóisic") \
  X(MENU_ABOUT, "Eolas") \
  X(MENU_REGION_FULL, "Iomlán") \
  X(MENU_REGION_JAPAN, "An tSeapáin") \
  X(MENU_REGION_WIDE, "Leathan") \
  X(MENU_REGION_WORLD, "Domhanda") \
  X(MENU_RDS_REGION_EUROPE, "An Eoraip") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Meiriceá Thuaidh") \
  X(MENU_WIFI_JOINED, "Ceangailte") \
  X(MENU_WIFI_JOINING, "Ag ceangal") \
  X(COMMON_OFF, "As") \
  X(MENU_FMT_KB_OF_KB, "%u as %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V ag tosú") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, %u nóim. ó shin") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, %u uair ó shin") \
  X(MENU_FMT_AGC_GAIN_NOW, "Anois %+d dB") \
  X(MENU_STATION_LOG, "Logleabhar") \
  X(COMMON_NONE, "Dada") \
  X(MENU_FMT_SCAN_PROGRESS, "Scanadh %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "%u sábháilte") \
  X(MENU_FMT_SCAN_STOPPED, "Stadta, %u sábháilte") \
  X(MENU_FMT_SCAN_NO_ROOM, "Lán, %u gan sábháil") \
  X(COMMON_YES, "Tá") \
  X(COMMON_NO, "Níl") \
  X(MENU_UNKNOWN_VALUE, "Anaithnid") \
  X(COMMON_ROTATION_NORMAL, "Gnáth") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Bun os cionn") \
  X(MENU_CUT_ONLY, "Gan treisiú") \
  X(MENU_NEVER, "Riamh") \
  X(MENU_BATTERY_PER_CENT, "Céatadán") \
  X(MENU_BATTERY_VOLTS, "Voltaí") \
  X(MENU_KEYS, "Digití amháin") \
  X(MENU_KEYS_LONG, "Brú gearr & fada") \
  X(MENU_EVERY_PRESS, "Gach brú") \
  X(MENU_STANDARD, "Caighdeán") \
  X(MENU_OPTICAL, "Optúil") \
  X(MENU_NORMAL, "Gnáth") \
  X(MENU_REVERSED, "Droim ar ais") \
  X(MENU_SQUELCH_MAN, "De láimh") \
  X(MENU_ANY_PI, "Aon PI") \
  X(MENU_STOP_NEVER, "Riamh") \
  X(MENU_NEW_ONLY, "Stáisiúin nua amháin") \
  X(MENU_WHOLE_BAND, "Banda iomlán") \
  X(MENU_MEMORY_ONLY, "Cuimhne amháin") \
  X(MENU_BAND_MEMORY, "Banda + cuimhne") \
  X(COMMON_ON, "Air") \
  X(MENU_HINT_RESTART, "I bhfeidhm tar éis atosú") \
  X(MENU_TITLE, "Roghchlár") \
  X(COMMON_FM_ONLY, "FM amháin") \
  X(MENU_NOTE_OFF_PLAN, "As an bplean banda") \
  X(MENU_NOTE_SWITCH_TO_FM, "Téigh go FM ar dtús") \
  X(MENU_NOTE_RADIO_BUSY, "Gnóthach - triail arís") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Cuir RDS air ar dtús") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Nuashonrú ar triail - fan") \
  X(MENU_NOTE_NOT_STORED, "Gan sábháil - seanluach") \
  X(MENU_NOTE_NOT_PUT_BACK, "Gnóthach - gan aischur") \
  X(MENU_NOTE_NOT_SAVED, "Gan sábháil") \
  X(MENU_SAVE, "Sábháil") \
  X(RADIO_PRESS_TO_START, "Brúigh le tosú") \
  X(COMMON_STILL_TUNING, "Ag tiúnadh - fan") \
  X(COMMON_SCAN_RUNNING, "Scanadh ar siúl - fan") \
  X(RADIO_FMT_NO_BAND, "%s: gan bhanda") \
  X(COMMON_NOT_LOGGED, "Níor logáladh") \
  X(RADIO_FMT_LOGGED, "%s logáilte") \
  X(RADIO_CHECKING_UPDATES, "AG LORG NUASHONRUITHE\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Ag scanadh %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Theip ar nuashonrú") \
  X(RADIO_GOING_TO_SLEEP, "Ag dul a chodladh") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Brúigh an cnaipe le múscailt") \
  X(RADIO_OLD_IMAGE_KEPT, "Seanleagan coinnithe") \
  X(RDS_STEREO, "Steiréó") \
  X(RDS_SPEECH, "Caint") \
  X(RDS_MUSIC, "Ceol") \
  X(COMMON_ALREADY_LOGGED, "Logáilte cheana") \
  X(DX_RDS_IS_OFF, "RDS as") \
  X(DX_SCANNING, "Ag scanadh") \
  X(DX_SCAN_FINISHED, "Scanadh déanta") \
  X(DX_NOTHING_TO_SCAN, "Dada le scanadh") \
  X(DX_CHECKING_FOR_UPDATES, "Ag lorg nuashonruithe") \
  X(DX_A_SCAN_IS_RUNNING, "Scanadh ar siúl cheana") \
  X(DX_CAUGHT_LIST_UNREAD, "Liosta ar iarraidh") \
  X(DX_NO_MEMORY, "Cuimhne folamh") \
  X(DX_NOT_STARTED, "Níor tosaíodh") \
  X(DX_SWEEPING, "Ag scuabadh") \
  X(COMMON_FMT_TUNE_TYPED, "Tiúin %s") \
  X(RADIO_SQL_OFF, "AS") \
  X(DX_FMT_ROWS_OF, "%u-%u as %u") \
  X(DX_FMT_FOUND, "%u aimsithe") \
  X(DX_MODE_SWEEPING, "SCUABADH") \
  X(DX_MODE_LEARN_LOCALS, "FOGHLAIM ÁITIÚLA") \
  X(DX_FMT_MODE_MEMORY, "CUIMHNE %u-%u") \
  X(DX_MODE_WHOLE_BAND, "BANDA IOMLÁN") \
  X(DX_MODE_BAND_MEMORY, "BANDA + CUIMHNE") \
  X(DX_RULE_NO_STOP, "GAN STAD") \
  X(DX_RULE_STOP_ON_PI, "STAD AR PI") \
  X(DX_RULE_STOP_ON_NEW, "STAD AR NUA") \
  X(DX_FMT_NOT_PRESET, "ní P%02d") \
  X(DX_CONTEXT_SWEEPING, "ag scuabadh") \
  X(DX_PRESS_TO_SWEEP, "Brúigh le scuabadh") \
  X(SCOPE_FMT_TITLE, "Scóp %s") \
  X(SCOPE_FULL, "Iomlán") \
  X(DX_FIXED, "SEASTA") \
  X(DX_FMT_MEDIAN_OF, "AIRMHEÁN AS %u") \
  X(DX_FMT_FLOOR, "URLÁR %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Dún") \
  X(RECOVERY_PRESS_AGAIN, "Brúigh arís") \
  X(PAGE_TUNE, "Tiúin") \
  X(PAGE_DELETE, "Scrios") \
  X(PAGE_LEVEL, "Neart") \
  X(PAGE_NOISE, "Torann") \
  X(PAGE_MULTIPATH, "Ilchonair") \
  X(PAGE_COCHANNEL, "Comhchainéal") \
  X(PAGE_STEREO, "Steiréó") \
  X(PAGE_WIDTH, "Leithead") \
  X(PAGE_BAND, "Banda") \
  X(PAGE_NOTE_CHANGED, "Athraithe ó osclaíodh é") \
  X(PAGE_NOTE_NOT_DELETED, "Níor scriosadh") \
  X(PAGE_FMT_SAVED, "Sábháilte %s") \
  X(PAGE_TIME_UNKNOWN, "Am anaithnid") \
  X(MENU_FMT_TUNER_PATCH, "%s paiste %u") \
  X(BW_TITLE, "Bandaleithead") \
  X(KEYPAD_TITLE, "Minicíocht") \
  X(KEYPAD_CANCEL, "Cealaigh") \
  X(DX_CATCHES_EMPTY_TITLE, "Gan ghabháil fós") \
  X(DX_CATCHES_EMPTY_NOTE, "Feicfear anseo stáisiúin dheimhnithe an tseisiúin seo") \
  X(DX_TITLE_CATCHES, "Gabhálacha") \
  X(DX_NEW, "NUA") \
  X(DX_READY, "Réidh") \
  X(DX_DWELL, "FANACHT") \
  X(DX_STOPPED, "STADTA") \
  X(DX_TITLE_SCANNER, "Scanóir") \
  X(DX_TITLE_SCOPE, "Scóp") \
  X(DX_LEVEL, "NEART") \
  X(DX_RISE, "ARDÚ") \
  X(DX_NO_STATION_YET, "Gan stáisiún fós") \
  X(DX_NO_RDS_YET, "Gan RDS fós") \
  X(DX_NO_ID, "GAN ID") \
  X(RDS_TITLE_TEXT, "Téacs raidió") \
  X(RDS_TITLE_NETWORKS, "Líonraí") \
  X(RDS_TITLE_DECODER, "Díchódóir") \
  X(RDS_NO_COUNTRY, "gan ECC fós") \
  X(RDS_RTPLUS_NONE, "Gan RT+ ón stáisiún seo") \
  X(RDS_RTPLUS_WAIT, "Gan RT+ don téacs seo fós") \
  X(RDS_AF_TITLE, "MINICÍOCHTAÍ EILE") \
  X(RDS_EON_TITLE, "STÁISIÚIN EILE, EON") \
  X(RDS_NONE_HEARD, "Dada cloiste") \
  X(RDS_GROUPS_S_CAPS, "GRÚPA/S") \
  X(RDS_LOST, "CAILLTE") \
  X(RDS_EACH_BLOCK, "GACH BLOC") \
  X(RDS_LEGEND_CLEAN, "glan") \
  X(RDS_LEGEND_FIXED, "deisithe") \
  X(RDS_LEGEND_LOST, "caillte") \
  X(RDS_ALL_CLEAN, "glan ar fad") \
  X(RDS_GROUP_TYPES, "CINEÁLACHA GRÚPA") \
  X(RDS_NO_GROUPS, "Gan ghrúpa le nóiméad") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 le %u s") \
  X(RDS_COUNTRY_UNLISTED, "tír anaithnid") \
  X(RDS_NO_TYPES, "Níor léadh bloc B glan le nóiméad") \
  X(RDS_FMT_EON_MORE, "+%u eile") \
  X(RDS_EON_MORE_HEARD, "+ eile") \
  X(RDS_MORE, "eile") \
  X(RADIO_VEIL_UPDATING, "Á NUASHONRÚ") \
  X(RADIO_VEIL_KEEP_POWER, "Ná bain an chumhacht") \
  X(RADIO_MUTE, "CIÚIN") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Tiúnóir ar iarraidh ar bhus I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Ní raibh an tiúnóir réidh") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Theip ar aithint an tiúnóra") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Gan phaiste don tiúnóir seo") \
  X(TUNER_UNKNOWN_TUNER_PART, "Tiúnóir anaithnid") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Scríobh diúltaithe") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Níor fhreagair an tiúnóir léamh") \
  X(TUNER_OUT_OF_RANGE, "As raon") \
  X(TUNER_UNKNOWN, "Anaithnid") \
  X(BAND_LONG_LW, "Tonn fhada") \
  X(BAND_LONG_MW, "Meántonn") \
  X(BAND_LONG_SW, "Gearrthonn") \
  X(MENU_FMT_TUNE_TO, "Tiúin %s ar") \
  X(PTY_NONE, "Dada") \
  X(PTY_NEWS, "Nuacht") \
  X(PTY_CURRENT_AFFAIRS, "Cúrsaí reatha") \
  X(PTY_INFORMATION, "Eolas") \
  X(PTY_SPORT, "Spórt") \
  X(PTY_EDUCATION, "Oideachas") \
  X(PTY_DRAMA, "Dráma") \
  X(PTY_CULTURE, "Cultúr") \
  X(PTY_SCIENCE, "Eolaíocht") \
  X(PTY_VARIED, "Éagsúil") \
  X(PTY_POP_MUSIC, "Popcheol") \
  X(PTY_ROCK_MUSIC, "Rac-cheol") \
  X(PTY_EASY_LISTENING, "Ceol éasca") \
  X(PTY_LIGHT_CLASSICAL, "Clasaic éadrom") \
  X(PTY_SERIOUS_CLASSICAL, "Clasaiceach") \
  X(PTY_OTHER_MUSIC, "Ceol eile") \
  X(PTY_WEATHER, "Aimsir") \
  X(PTY_FINANCE, "Airgeadas") \
  X(PTY_CHILDREN, "Páistí") \
  X(PTY_SOCIAL_AFFAIRS, "Cúrsaí sóisialta") \
  X(PTY_RELIGION, "Creideamh") \
  X(PTY_PHONE_IN, "Glaonna") \
  X(PTY_TRAVEL, "Taisteal") \
  X(PTY_LEISURE, "Fóillíocht") \
  X(PTY_JAZZ_MUSIC, "Snagcheol") \
  X(PTY_COUNTRY_MUSIC, "Ceol tuaithe") \
  X(PTY_NATIONAL_MUSIC, "Ceol náisiúnta") \
  X(PTY_OLDIES_MUSIC, "Seancheol") \
  X(PTY_FOLK_MUSIC, "Ceol tíre") \
  X(PTY_DOCUMENTARY, "Clár faisnéise") \
  X(PTY_ALARM_TEST, "Tástáil aláraim") \
  X(PTY_ALARM, "Aláram") \
  X(PTY_NA_NEWS, "Nuacht") \
  X(PTY_NA_INFORMATION, "Eolas") \
  X(PTY_NA_SPORTS, "Spórt") \
  X(PTY_NA_TALK, "Caint") \
  X(PTY_NA_ROCK, "Rac") \
  X(PTY_NA_CLASSIC_ROCK, "Rac clasaiceach") \
  X(PTY_NA_ADULT_HITS, "Amais aosacha") \
  X(PTY_NA_SOFT_ROCK, "Rac bog") \
  X(PTY_NA_COUNTRY, "Ceol tuaithe") \
  X(PTY_NA_OLDIES, "Seancheol") \
  X(PTY_NA_SOFT, "Bog") \
  X(PTY_NA_NOSTALGIA, "Cumha") \
  X(PTY_NA_JAZZ, "Snagcheol") \
  X(PTY_NA_CLASSICAL, "Clasaiceach") \
  X(PTY_NA_RHYTHM_AND_BLUES, "R&B") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "R&B bog") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Teanga iasachta") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Ceol creidimh") \
  X(PTY_NA_RELIGIOUS_TALK, "Caint chreidimh") \
  X(PTY_NA_PERSONALITY, "Pearsantacht") \
  X(PTY_NA_PUBLIC, "Poiblí") \
  X(PTY_NA_COLLEGE, "Coláiste") \
  X(PTY_NA_SPANISH_TALK, "Caint Spáinnise") \
  X(PTY_NA_SPANISH_MUSIC, "Ceol Spáinneach") \
  X(PTY_NA_HIP_HOP, "Hip-hop") \
  X(PTY_NA_UNASSIGNED, "Gan sannadh") \
  X(PTY_NA_WEATHER, "Aimsir") \
  X(PTY_NA_EMERGENCY_TEST, "Tástáil éigeandála") \
  X(PTY_NA_EMERGENCY, "Éigeandáil") \
  X(COVERAGE_LOCAL, "Áitiúil") \
  X(COVERAGE_INTERNATIONAL, "Idirnáisiúnta") \
  X(COVERAGE_NATIONAL, "Náisiúnta") \
  X(COVERAGE_SUPRA_REGIONAL, "Osréigiúnach") \
  X(COVERAGE_REGIONAL_1, "Réigiúnach 1") \
  X(COVERAGE_REGIONAL_2, "Réigiúnach 2") \
  X(COVERAGE_REGIONAL_3, "Réigiúnach 3") \
  X(COVERAGE_REGIONAL_4, "Réigiúnach 4") \
  X(COVERAGE_REGIONAL_5, "Réigiúnach 5") \
  X(COVERAGE_REGIONAL_6, "Réigiúnach 6") \
  X(COVERAGE_REGIONAL_7, "Réigiúnach 7") \
  X(COVERAGE_REGIONAL_8, "Réigiúnach 8") \
  X(COVERAGE_REGIONAL_9, "Réigiúnach 9") \
  X(COVERAGE_REGIONAL_10, "Réigiúnach 10") \
  X(COVERAGE_REGIONAL_11, "Réigiúnach 11") \
  X(COVERAGE_REGIONAL_12, "Réigiúnach 12") \
  X(RTP_OTHER, "EILE") \
  X(RTP_TITLE, "TEIDEAL") \
  X(RTP_ALBUM, "ALBAM") \
  X(RTP_TRACK, "RIAN") \
  X(RTP_ARTIST, "EALAÍONTÓIR") \
  X(RTP_WORK, "SAOTHAR") \
  X(RTP_MOVEMENT, "GLUAISEACHT") \
  X(RTP_CONDUCTOR, "STIÚRTHÓIR") \
  X(RTP_COMPOSER, "CUMADÓIR") \
  X(RTP_BAND, "BANNA") \
  X(RTP_COMMENT, "NÓTA") \
  X(RTP_GENRE, "SEÁNRA") \
  X(RTP_NEWS, "NUACHT") \
  X(RTP_LOCAL, "NUACHT ÁITIÚIL") \
  X(RTP_STOCKS, "STOC") \
  X(RTP_SPORT, "SPÓRT") \
  X(RTP_LOTTERY, "CRANNCHUR") \
  X(RTP_HOROSCOPE, "HORASCÓP") \
  X(RTP_DIVERSION, "SIAMSA") \
  X(RTP_HEALTH, "SLÁINTE") \
  X(RTP_EVENT, "IMEACHT") \
  X(RTP_SCENE, "RADHARC") \
  X(RTP_CINEMA, "CINEAMA") \
  X(RTP_FUN, "SPRAOI") \
  X(RTP_DATE, "DÁTA") \
  X(RTP_WEATHER, "AIMSIR") \
  X(RTP_TRAFFIC, "TRÁCHT") \
  X(RTP_ALARM, "ALÁRAM") \
  X(RTP_ADVERT, "FÓGRA") \
  X(RTP_STATION, "STÁISIÚN") \
  X(RTP_PROGRAM, "CLÁR") \
  X(RTP_NEXT, "CHUGAINN") \
  X(RTP_PART, "CUID") \
  X(RTP_HOST, "LÁITHREOIR") \
  X(RTP_EDITORS, "EAGARTHÓIRÍ") \
  X(RTP_FREQUENCY, "MINICÍOCHT") \
  X(RTP_HOMEPAGE, "SUÍOMH GRÉASÁIN") \
  X(RTP_CHANNEL, "CAINÉAL") \
  X(RTP_HOTLINE, "BEOLÍNE") \
  X(RTP_STUDIO, "STIÚIDEO") \
  X(RTP_PHONE, "FÓN") \
  X(RTP_EMAIL, "R-PHOST") \
  X(RTP_CHAT, "COMHRÁ") \
  X(RTP_VOTE, "VÓTA") \
  X(RTP_PLACE, "ÁIT") \
  X(RTP_APPOINTMENT, "COINNE") \
  X(RTP_PURCHASE, "CEANNACH") \
  X(DAY_SUNDAY, "DÉ DOMHNAIGH") \
  X(DAY_MONDAY, "DÉ LUAIN") \
  X(DAY_TUESDAY, "DÉ MÁIRT") \
  X(DAY_WEDNESDAY, "DÉ CÉADAOIN") \
  X(DAY_THURSDAY, "DÉARDAOIN") \
  X(DAY_FRIDAY, "DÉ hAOINE") \
  X(DAY_SATURDAY, "DÉ SATHAIRN") \
  X(MONTH_JANUARY, "Eanáir") \
  X(MONTH_FEBRUARY, "Feabhra") \
  X(MONTH_MARCH, "Márta") \
  X(MONTH_APRIL, "Aibreán") \
  X(MONTH_MAY, "Bealtaine") \
  X(MONTH_JUNE, "Meitheamh") \
  X(MONTH_JULY, "Iúil") \
  X(MONTH_AUGUST, "Lúnasa") \
  X(MONTH_SEPTEMBER, "Meán Fómhair") \
  X(MONTH_OCTOBER, "Deireadh Fómhair") \
  X(MONTH_NOVEMBER, "Samhain") \
  X(MONTH_DECEMBER, "Nollaig") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_TODAY, "Inniu") \
  X(DATE_YESTERDAY, "Inné") \
  X(DATE_YDAY, "INNÉ") \
  X(MONTH_SHORT_JAN, "Ean") \
  X(MONTH_SHORT_FEB, "Feabh") \
  X(MONTH_SHORT_MAR, "Márta") \
  X(MONTH_SHORT_APR, "Aib") \
  X(MONTH_SHORT_MAY, "Beal") \
  X(MONTH_SHORT_JUN, "Meith") \
  X(MONTH_SHORT_JUL, "Iúil") \
  X(MONTH_SHORT_AUG, "Lún") \
  X(MONTH_SHORT_SEP, "MFómh") \
  X(MONTH_SHORT_OCT, "DFómh") \
  X(MONTH_SHORT_NOV, "Samh") \
  X(MONTH_SHORT_DEC, "Noll") \
  X(MONTH_TAG_JAN, "EAN") \
  X(MONTH_TAG_FEB, "FEABH") \
  X(MONTH_TAG_MAR, "MÁRTA") \
  X(MONTH_TAG_APR, "AIB") \
  X(MONTH_TAG_MAY, "BEAL") \
  X(MONTH_TAG_JUN, "MEITH") \
  X(MONTH_TAG_JUL, "IÚIL") \
  X(MONTH_TAG_AUG, "LÚN") \
  X(MONTH_TAG_SEP, "MFÓMH") \
  X(MONTH_TAG_OCT, "DFÓMH") \
  X(MONTH_TAG_NOV, "SAMH") \
  X(MONTH_TAG_DEC, "NOLL") \
  X(DX_NOW, "anois") \
  X(DX_FMT_MIN_AGO, "%u nóim. ó shin") \
  X(DX_FMT_H_AGO, "%u uair ó shin") \
  X(DX_FMT_D_AGO, "%u lá ó shin") \
  X(TOUCH_CAL_TITLE, "Calabraigh tadhall") \
  X(TOUCH_CAL_HOLD, "Brúigh an ciorcal go líonfaidh sé") \
  X(TOUCH_CAL_CANCEL_HINT, "Cas nó brúigh an cnaipe le cealú") \
  X(TOUCH_CAL_CHECK, "Seiceáil") \
  X(TOUCH_CAL_TAP_DOT, "Tapáil an ponc") \
  X(TOUCH_CAL_SAVED, "Calabrú sábháilte") \
  X(TOUCH_CAL_FMT_LANDED, "Bhí an tseiceáil %u px as") \
  X(TOUCH_CAL_BACK_HINT, "Brúigh an cnaipe le dul siar") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Gan sábháil") \
  X(TOUCH_CAL_FMT_MISSED, "Bhí an tseiceáil %u px as,") \
  X(TOUCH_CAL_NO_FIT, "Ní dhearna na marcanna calabrú,") \
  X(TOUCH_CAL_NOT_SAVED, "Theip ar scríobh,") \
  X(TOUCH_CAL_OLD_STAYS, "agus fanann an seanchalabrú") \
  X(TOUCH_CAL_RETRY_HINT, "Brúigh an cnaipe arís, nó cas é le himeacht")
// clang-format on
