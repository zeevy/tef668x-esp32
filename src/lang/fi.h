/*
 * The panel's texts in Finnish. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_FI(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "FM-skannaus") \
  X(MENU_SCAN_MW, "AM-skannaus") \
  X(MENU_SCAN_SW, "SW-skannaus") \
  X(MENU_SCAN_LW, "LW-skannaus") \
  X(MENU_SQUELCH, "Salpatila") \
  X(MENU_SQUELCH_LEVEL, "Salpataso") \
  X(MENU_SQUELCH_FLOOR, "Alaraja") \
  X(MENU_SQUELCH_GROUP, "Kohinasalpa") \
  X(MENU_VOLUME_AGC, "Äänen AGC") \
  X(MENU_AGC_BOOST, "AGC-korotus") \
  X(MENU_MUTE_RAMP, "Mykistysliuku") \
  X(MENU_BAND_PLAN, "Taajuusalue") \
  X(MENU_TUNING_STEP, "Viritysaskel") \
  X(MENU_SEEK_SENSITIVITY, "Hakuherkkyys") \
  X(MENU_DE_EMPHASIS, "Deemfaasi") \
  X(MENU_FORCED_MONO, "Pakota mono") \
  X(MENU_STEREO_BLEND, "Stereohäivytys") \
  X(MENU_ST_HI_BLEND, "Stereo-/ylähäivytys") \
  X(MENU_HIGH_CUT, "Alipäästö") \
  X(MENU_NOISE_BLANKER, "Häiriönpoisto") \
  X(MENU_EQUALISER, "Taajuuskorjain") \
  X(COMMON_RDS_DECODER, "RDS-dekooderi") \
  X(MENU_RDS_REGION, "RDS-alue") \
  X(MENU_MW_SPACING, "MW-askel") \
  X(MENU_FILTER_WIDTH, "Suodatinkaista") \
  X(MENU_SOFT_MUTE, "Pehmeä mykistys") \
  X(MENU_DWELL, "Kuunteluaika") \
  X(MENU_STOP_ON, "Pysähdysehto") \
  X(MENU_SCAN, "Skannausalue") \
  X(MENU_MEMORY_FROM, "Muistista") \
  X(MENU_MEMORY_TO, "Muistiin") \
  X(MENU_PRESET_RANGE, "Muistialue") \
  X(MENU_LOOP_THE_BAND, "Kierrä alue") \
  X(MENU_DX_WIDTH, "Skannauskaista") \
  X(MENU_MUTE_WHILE_SCANNING, "Mykistä skannatessa") \
  X(MENU_AUTO_LOG_NEW, "Autom. kirjaus") \
  X(MENU_LOG_RADIO_TEXT, "Kirjaa radioteksti") \
  X(MENU_WATCH_PRESETS, "Vahdi muisteja") \
  X(MENU_SCOPE_BASELINE, "Spektrin vertailu") \
  X(MENU_MEDIAN, "Mediaani") \
  X(MENU_FMT_SWEEP_N, "Pyyhkäisy %u") \
  X(DX_FMT_WATCH_UP, "%s nousi %d dB") \
  X(MENU_LEARN_LOCALS, "Opettele paikalliset") \
  X(MENU_START_SCAN, "Skannaa") \
  X(MENU_DAY_THEME, "Päiväteema") \
  X(MENU_NIGHT_THEME, "Yöteema") \
  X(MENU_THEME, "Teema") \
  X(MENU_BRIGHTNESS, "Kirkkaus") \
  X(MENU_DIM_LEVEL, "Himmennystaso") \
  X(MENU_DIM_AFTER, "Himmennysviive") \
  X(MENU_ROTATION, "Näytön kierto") \
  X(COMMON_BATTERY, "Akku") \
  X(MENU_BATTERY_CHECK, "Akun tarkistus") \
  X(MENU_LANGUAGE, "Kieli · Language") \
  X(MENU_ENCODER, "Kooderin tyyppi") \
  X(MENU_DIRECTION, "Kiertosuunta") \
  X(MENU_ENCODER_GROUP, "Kooderi") \
  X(MENU_KEY_BEEPS, "Näppäinäänet") \
  X(MENU_BAND_EDGE_BEEP, "Alueen reunaääni") \
  X(MENU_START_CHIME, "Käynnistysääni") \
  X(MENU_KEYPAD_TIMEOUT, "Syötön aikaraja") \
  X(MENU_TOUCH, "Kosketus") \
  X(MENU_CALIBRATE_TOUCH, "Kalibroi kosketus") \
  X(MENU_CLOCK_FROM_NETWORK, "Verkkoaika") \
  X(MENU_WEB_PIN, "Web-PIN") \
  X(MENU_NEW_PIN, "Uusi PIN") \
  X(MENU_FMT_DIGIT_OF, "Numero %u/%u") \
  X(MENU_STATUS, "Yhteyden tila") \
  X(MENU_WEB_SERVER, "Web-palvelin") \
  X(MENU_FMT_PC_CONNECTED, "%u yhdistetty") \
  X(MENU_WEB_ADDRESS, "Verkko-osoite") \
  X(MENU_IP_ADDRESS, "IP-osoite") \
  X(MENU_WI_FI_NAME, "Wi-Fi-verkko") \
  X(MENU_HOTSPOT_NAME, "Hotspotin nimi") \
  X(MENU_MAC, "MAC-osoite") \
  X(MENU_WI_FI_SIGNAL, "Wi-Fi-signaali") \
  X(MENU_NETWORK_INFO, "Verkon tiedot") \
  X(MENU_RESTART, "Käynnistä uudelleen") \
  X(MENU_AUTO_OFF, "Sammutusviive") \
  X(MENU_UPDATE_CHECK, "Päivitystarkistus") \
  X(MENU_FIRMWARE_UPDATE, "Ohjelmistopäivitys") \
  X(MENU_FMT_UPDATE_TO, "Päivitys %s") \
  X(MENU_UPDATE_TITLE, "Päivitys saatavilla") \
  X(MENU_UPDATE_THIS_RADIO, "Tämä radio") \
  X(MENU_UPDATE_NEW_VERSION, "Uusi versio") \
  X(MENU_UPDATE_DOWNLOAD, "Lataus") \
  X(COMMON_SETTINGS, "Asetukset") \
  X(COMMON_KEPT, "Säilyvät") \
  X(MENU_STATION, "Asema") \
  X(MENU_UPDATE_NOW, "Päivitä") \
  X(MENU_LATER, "Myöhemmin") \
  X(MENU_UPDATE_UP_TO_DATE, "Ajan tasalla") \
  X(MENU_UPDATE_CHECKING, "Tarkistetaan") \
  X(MENU_UPDATE_NOT_CHECKED, "Ei tarkistettu") \
  X(MENU_UPDATE_CHECK_FAILED, "Tarkistusvirhe") \
  X(MENU_NOTE_NO_UPDATE, "Ei päivitystä") \
  X(MENU_VERSION, "Ohjelmistoversio") \
  X(MENU_RUNNING_FROM, "Käynnistysosio") \
  X(MENU_UPTIME, "Käyntiaika") \
  X(MENU_BATTERY_VOLTAGE, "Akun jännite") \
  X(COMMON_TUNER, "Viritin") \
  X(MENU_RESET_REASON, "Käynnistyssyy") \
  X(MENU_CPU_CORE_0, "CPU-ydin 0") \
  X(MENU_CPU_CORE_1, "CPU-ydin 1") \
  X(MENU_FREE_HEAP, "Vapaa keko") \
  X(MENU_LOWEST_HEAP, "Pienin keko") \
  X(MENU_LARGEST_BLOCK, "Suurin lohko") \
  X(MENU_LVGL_POOL, "LVGL-muisti") \
  X(MENU_CHIP, "Piiri") \
  X(MENU_FLASH_SIZE, "Flash-koko") \
  X(RESET_POWER, "Virta päälle") \
  X(RESET_SOFTWARE, "Ohjelmisto") \
  X(RESET_PANIC, "Kaatuminen") \
  X(RESET_INT_WATCHDOG, "IRQ-watchdog") \
  X(RESET_TASK_WATCHDOG, "Tehtävä-watchdog") \
  X(RESET_DEEP_SLEEP, "Syvä lepo") \
  X(RESET_BROWNOUT, "Alijännite") \
  X(RESET_PIN, "Reset-nasta") \
  X(RESET_UNKNOWN, "Tuntematon") \
  X(RESET_ASKED, "Pyydetty") \
  X(RESET_BOOT_WATCHDOG, "Boot-watchdog") \
  X(RESET_ROLLBACK, "Palautus") \
  X(RESET_DISPLAY, "Näyttövika") \
  X(RESET_UPDATE, "Päivitys") \
  X(MENU_DEVELOPER, "Kehittäjä") \
  X(MENU_BUILD, "Käännös") \
  X(MENU_LICENSE, "Lisenssi") \
  X(ABOUT_BUILD_UNKNOWN, "tuntematon") \
  X(MENU_GO_TO, "Siirry") \
  X(MENU_GO_TO_BANDWIDTH, "Kaista") \
  X(MENU_GO_TO_DX, "DX-tila") \
  X(MENU_GO_TO_SCOPE, "Spektri") \
  X(MENU_GO_TO_BAND, "Seuraava alue") \
  X(MENU_GO_TO_LOG, "Kirjaa asema") \
  X(MENU_GO_TO_SAVE, "Tallenna asema") \
  X(MENU_FMT_SAVED_AS, "Tallessa P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "On jo P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Muistit täynnä") \
  X(MENU_GO_TO_SLEEP, "Lepotila") \
  X(MENU_STATIONS, "Asemat") \
  X(MENU_MEMORY, "Muistit") \
  X(MENU_AUDIO, "Ääni") \
  X(MENU_FM_SETUP, "FM-vastaanotto") \
  X(MENU_AM_SETUP, "AM-vastaanotto") \
  X(MENU_DX_SETUP, "DX-skanneri") \
  X(MENU_DISPLAY, "Näyttö") \
  X(MENU_CONTROLS, "Ohjaimet") \
  X(MENU_CONNECTIVITY, "Yhteydet") \
  X(MENU_SYSTEM, "Järjestelmä") \
  X(MENU_DIAGNOSTICS, "Diagnostiikka") \
  X(MENU_ABOUT, "Tietoja") \
  X(MENU_REGION_FULL, "Täysi") \
  X(MENU_REGION_JAPAN, "Japani") \
  X(MENU_REGION_WIDE, "Laaja") \
  X(MENU_REGION_WORLD, "Maailma") \
  X(MENU_RDS_REGION_EUROPE, "Eurooppa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Pohjois-Amerikka") \
  X(MENU_WIFI_JOINED, "Yhdistetty") \
  X(MENU_WIFI_JOINING, "Yhdistetään") \
  X(COMMON_OFF, "Pois") \
  X(MENU_FMT_HOURS_MINUTES, "%u h %u min") \
  X(MENU_FMT_KB_OF_KB, "%u / %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V alussa") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, %u min sitten") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, %u h sitten") \
  X(MENU_FMT_AGC_GAIN_NOW, "Vahvistus nyt %+d dB") \
  X(MENU_STATION_LOG, "Asemaloki") \
  X(COMMON_NONE, "Puuttuu") \
  X(MENU_FMT_SCAN_PROGRESS, "Skannaus %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "%u tallessa") \
  X(MENU_FMT_SCAN_STOPPED, "Keskeytyi, %u tallessa") \
  X(MENU_FMT_SCAN_NO_ROOM, "Täynnä, %u jäi pois") \
  X(COMMON_YES, "Kyllä") \
  X(COMMON_NO, "Ei") \
  X(MENU_UNKNOWN_VALUE, "Tuntematon") \
  X(COMMON_ROTATION_NORMAL, "Normaali") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Ylösalaisin") \
  X(MENU_CUT_ONLY, "Vain vaimennus") \
  X(MENU_NEVER, "Ei koskaan") \
  X(MENU_BATTERY_PER_CENT, "Prosentti") \
  X(MENU_BATTERY_VOLTS, "Voltit") \
  X(MENU_KEYS, "Vain numerot") \
  X(MENU_KEYS_LONG, "Lyhyet ja pitkät") \
  X(MENU_EVERY_PRESS, "Joka painallus") \
  X(MENU_STANDARD, "Vakio") \
  X(MENU_OPTICAL, "Optinen") \
  X(MENU_NORMAL, "Normaali") \
  X(MENU_REVERSED, "Käänteinen") \
  X(MENU_SQUELCH_MAN, "Käsin") \
  X(MENU_ANY_PI, "Jokin PI") \
  X(MENU_STOP_NEVER, "Ei koskaan") \
  X(MENU_NEW_ONLY, "Vain uudet") \
  X(MENU_WHOLE_BAND, "Koko alue") \
  X(MENU_MEMORY_ONLY, "Vain muistit") \
  X(MENU_BAND_MEMORY, "Alue + muistit") \
  X(COMMON_ON, "Päällä") \
  X(MENU_HINT_RESTART, "Voimaan käynnistyksessä") \
  X(MENU_TITLE, "Valikko") \
  X(COMMON_FM_ONLY, "Vain FM") \
  X(MENU_NOTE_OFF_PLAN, "Ei taajuusalueella") \
  X(MENU_NOTE_SWITCH_TO_FM, "Vaihda ensin FM:lle") \
  X(MENU_NOTE_RADIO_BUSY, "Varattu - yritä uudelleen") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Kytke ensin RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Päivitys testissä - odota") \
  X(MENU_NOTE_NOT_STORED, "Ei tallessa - palautettu") \
  X(MENU_NOTE_NOT_PUT_BACK, "Varattu - ei palautettu") \
  X(MENU_NOTE_NOT_SAVED, "Ei tallessa") \
  X(MENU_SAVE, "Tallenna") \
  X(RADIO_PRESS_TO_START, "Aloita painamalla") \
  X(COMMON_STILL_TUNING, "Viritys kesken - odota") \
  X(COMMON_SCAN_RUNNING, "Skannaus käy - odota") \
  X(RADIO_FMT_NO_BAND, "Ei aluetta: %s") \
  X(COMMON_NOT_LOGGED, "Ei kirjattu") \
  X(RADIO_FMT_LOGGED, "Kirjattu %s") \
  X(RADIO_CHECKING_UPDATES, "PÄIVITYSTARKISTUS…") \
  X(RADIO_FMT_SCANNING, "Skannataan %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Päivitys epäonnistui") \
  X(RADIO_GOING_TO_SLEEP, "Lepotilaan") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Herätä painamalla nuppia") \
  X(RADIO_OLD_IMAGE_KEPT, "Edellinen ohjelmisto säilyi") \
  X(RDS_SPEECH, "Puhe") \
  X(RDS_MUSIC, "Musiikki") \
  X(COMMON_ALREADY_LOGGED, "Jo kirjattu") \
  X(DX_RDS_IS_OFF, "RDS on pois") \
  X(DX_SCANNING, "Skannataan") \
  X(DX_SCAN_FINISHED, "Skannaus valmis") \
  X(DX_NOTHING_TO_SCAN, "Ei skannattavaa") \
  X(DX_CHECKING_FOR_UPDATES, "Tarkistetaan päivityksiä") \
  X(DX_A_SCAN_IS_RUNNING, "Skannaus jo käynnissä") \
  X(DX_CAUGHT_LIST_UNREAD, "Löytölista ei saatavilla") \
  X(DX_NO_MEMORY, "Ei muisteja") \
  X(DX_NOT_STARTED, "Ei aloitettu") \
  X(DX_SWEEPING, "Pyyhkäistään") \
  X(COMMON_FMT_TUNE_TYPED, "Viritä %s") \
  X(RADIO_SQL_OFF, "POIS") \
  X(RADIO_TUNE_MANUAL, "KÄSI") \
  X(DX_FMT_ROWS_OF, "%u-%u / %u") \
  X(DX_FMT_FOUND, "%u löydetty") \
  X(DX_MODE_SWEEPING, "PYYHKÄISY") \
  X(DX_MODE_LEARN_LOCALS, "PAIKALLISET") \
  X(DX_FMT_MODE_MEMORY, "MUISTIT %u-%u") \
  X(DX_MODE_WHOLE_BAND, "KOKO ALUE") \
  X(DX_MODE_BAND_MEMORY, "ALUE + MUISTIT") \
  X(DX_RULE_NO_STOP, "EI PYSÄHDY") \
  X(DX_RULE_STOP_ON_PI, "SEIS: PI") \
  X(DX_RULE_STOP_ON_NEW, "SEIS: UUSI") \
  X(DX_FMT_NOT_PRESET, "ei P%02d") \
  X(DX_CONTEXT_SWEEPING, "pyyhkäisy") \
  X(DX_PRESS_TO_SWEEP, "Pyyhkäise painamalla") \
  X(SCOPE_FMT_TITLE, "%s-spektri") \
  X(SCOPE_FULL, "Koko") \
  X(DX_FIXED, "KIINTEÄ") \
  X(DX_FMT_MEDIAN_OF, "MEDIAANI (%u)") \
  X(DX_FMT_FLOOR, "KOHINA %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Sulje") \
  X(RECOVERY_PRESS_AGAIN, "Paina uudelleen") \
  X(PAGE_TUNE, "Viritä") \
  X(PAGE_DELETE, "Poista") \
  X(PAGE_LEVEL, "Taso") \
  X(PAGE_NOISE, "Kohina") \
  X(PAGE_MULTIPATH, "Monitie") \
  X(PAGE_COCHANNEL, "Samakanava") \
  X(PAGE_WIDTH, "Kaista") \
  X(PAGE_BAND, "Alue") \
  X(PAGE_NOTE_CHANGED, "Muuttunut avaamisen jälkeen") \
  X(PAGE_NOTE_NOT_DELETED, "Ei poistettu") \
  X(PAGE_FMT_SAVED, "Tallennettu %s") \
  X(PAGE_TIME_UNKNOWN, "Aika ei tiedossa") \
  X(BW_TITLE, "Kaista") \
  X(KEYPAD_TITLE, "Taajuus") \
  X(KEYPAD_CANCEL, "Peruuta") \
  X(DX_CATCHES_EMPTY_TITLE, "Ei löytöjä vielä") \
  X(DX_CATCHES_EMPTY_NOTE, "Tämän istunnon varmistetut asemat näkyvät tässä") \
  X(DX_TITLE_CATCHES, "Löydöt") \
  X(DX_NEW, "UUSI") \
  X(DX_READY, "Valmis") \
  X(DX_DWELL, "AIKA") \
  X(DX_STOPPED, "SEIS") \
  X(DX_TITLE_SCANNER, "Skanneri") \
  X(DX_TITLE_SCOPE, "Spektri") \
  X(DX_LEVEL, "TASO") \
  X(DX_RISE, "ERO") \
  X(DX_NO_STATION_YET, "Ei asemaa vielä") \
  X(DX_NO_RDS_YET, "Ei RDS:ää vielä") \
  X(DX_NO_ID, "EI ID") \
  X(RDS_TITLE_TEXT, "Radioteksti") \
  X(RDS_TITLE_NETWORKS, "Verkot") \
  X(RDS_TITLE_DECODER, "Dekooderi") \
  X(RDS_NO_COUNTRY, "ECC ei kuultu") \
  X(RDS_RTPLUS_NONE, "Asemalta ei kuultu RT+:aa") \
  X(RDS_RTPLUS_WAIT, "Tälle tekstille ei vielä RT+:aa") \
  X(RDS_AF_TITLE, "VAIHTOEHTOISET TAAJUUDET") \
  X(RDS_EON_TITLE, "MUUT ASEMAT, EON") \
  X(RDS_NONE_HEARD, "Ei kuultu") \
  X(RDS_GROUPS_S_CAPS, "RYHMÄ/S") \
  X(RDS_LOST, "HÄVIÖ") \
  X(RDS_EACH_BLOCK, "LOHKOT") \
  X(RDS_LEGEND_CLEAN, "ehjä") \
  X(RDS_LEGEND_FIXED, "korjattu") \
  X(RDS_LEGEND_LOST, "hävinnyt") \
  X(RDS_ALL_CLEAN, "kaikki ehjiä") \
  X(RDS_GROUP_TYPES, "RYHMÄTYYPIT") \
  X(RDS_NO_GROUPS, "Ei ryhmiä viime minuuttina") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 %u s") \
  X(RDS_COUNTRY_UNLISTED, "ei listassa") \
  X(RDS_NO_TYPES, "Ei ehjää B-lohkoa viime minuuttina") \
  X(RDS_FMT_EON_MORE, "+%u lisää") \
  X(RDS_EON_MORE_HEARD, "+ lisää") \
  X(RDS_MORE, "lisää") \
  X(RADIO_VEIL_UPDATING, "PÄIVITETÄÄN OHJELMISTOA") \
  X(RADIO_VEIL_KEEP_POWER, "Älä katkaise virtaa") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Viritintä ei löydy I2C-väylältä") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Viritin ei tullut valmiiksi") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Virittimen tunnistus epäonnistui") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Tälle virittimelle ei ole patchia") \
  X(TUNER_UNKNOWN_TUNER_PART, "Tuntematon viritin") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Viritin hylkäsi kirjoituksen") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Viritin ei vastannut lukuun") \
  X(TUNER_OUT_OF_RANGE, "Alueen ulkopuolella") \
  X(TUNER_UNKNOWN, "Tuntematon") \
  X(BAND_LONG_LW, "Pitkät aallot") \
  X(BAND_LONG_MW, "Keskiaallot") \
  X(BAND_LONG_SW, "Lyhyet aallot") \
  X(MENU_FMT_TUNE_TO, "Viritä %s") \
  X(PTY_NONE, "Ei mitään") \
  X(PTY_NEWS, "Uutiset") \
  X(PTY_CURRENT_AFFAIRS, "Ajankohtaista") \
  X(PTY_INFORMATION, "Tietoa") \
  X(PTY_SPORT, "Urheilu") \
  X(PTY_EDUCATION, "Opetus") \
  X(PTY_DRAMA, "Draama") \
  X(PTY_CULTURE, "Kulttuuri") \
  X(PTY_SCIENCE, "Tiede") \
  X(PTY_VARIED, "Viihde") \
  X(PTY_POP_MUSIC, "Pop-musiikki") \
  X(PTY_ROCK_MUSIC, "Rock-musiikki") \
  X(PTY_EASY_LISTENING, "Kevyt musiikki") \
  X(PTY_LIGHT_CLASSICAL, "Kevyt klassinen") \
  X(PTY_SERIOUS_CLASSICAL, "Klassinen") \
  X(PTY_OTHER_MUSIC, "Muu musiikki") \
  X(PTY_WEATHER, "Sää") \
  X(PTY_FINANCE, "Talous") \
  X(PTY_CHILDREN, "Lapset") \
  X(PTY_SOCIAL_AFFAIRS, "Yhteiskunta") \
  X(PTY_RELIGION, "Uskonto") \
  X(PTY_PHONE_IN, "Puhelinohjelma") \
  X(PTY_TRAVEL, "Matkailu") \
  X(PTY_LEISURE, "Vapaa-aika") \
  X(PTY_COUNTRY_MUSIC, "Country-musiikki") \
  X(PTY_NATIONAL_MUSIC, "Kotimainen musiikki") \
  X(PTY_FOLK_MUSIC, "Kansanmusiikki") \
  X(PTY_DOCUMENTARY, "Dokumentti") \
  X(PTY_ALARM_TEST, "Hälytystesti") \
  X(PTY_ALARM, "Hälytys") \
  X(PTY_NA_NEWS, "Uutiset") \
  X(PTY_NA_INFORMATION, "Tietoa") \
  X(PTY_NA_SPORTS, "Urheilu") \
  X(PTY_NA_TALK, "Puhe") \
  X(PTY_NA_CLASSICAL, "Klassinen") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Vieraskielinen") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Hengellinen musiikki") \
  X(PTY_NA_RELIGIOUS_TALK, "Hengellinen puhe") \
  X(PTY_NA_PERSONALITY, "Juontajaohjelma") \
  X(PTY_NA_PUBLIC, "Julkinen radio") \
  X(PTY_NA_COLLEGE, "Opiskelijaradio") \
  X(PTY_NA_SPANISH_TALK, "Puhe espanjaksi") \
  X(PTY_NA_SPANISH_MUSIC, "Musiikki espanjaksi") \
  X(PTY_NA_UNASSIGNED, "Määrittämätön") \
  X(PTY_NA_WEATHER, "Sää") \
  X(PTY_NA_EMERGENCY_TEST, "Hätätesti") \
  X(PTY_NA_EMERGENCY, "Hätätiedote") \
  X(COVERAGE_LOCAL, "Paikallinen") \
  X(COVERAGE_INTERNATIONAL, "Kansainvälinen") \
  X(COVERAGE_NATIONAL, "Kansallinen") \
  X(COVERAGE_SUPRA_REGIONAL, "Ylialueellinen") \
  X(COVERAGE_REGIONAL_1, "Alueellinen 1") \
  X(COVERAGE_REGIONAL_2, "Alueellinen 2") \
  X(COVERAGE_REGIONAL_3, "Alueellinen 3") \
  X(COVERAGE_REGIONAL_4, "Alueellinen 4") \
  X(COVERAGE_REGIONAL_5, "Alueellinen 5") \
  X(COVERAGE_REGIONAL_6, "Alueellinen 6") \
  X(COVERAGE_REGIONAL_7, "Alueellinen 7") \
  X(COVERAGE_REGIONAL_8, "Alueellinen 8") \
  X(COVERAGE_REGIONAL_9, "Alueellinen 9") \
  X(COVERAGE_REGIONAL_10, "Alueellinen 10") \
  X(COVERAGE_REGIONAL_11, "Alueellinen 11") \
  X(COVERAGE_REGIONAL_12, "Alueellinen 12") \
  X(RTP_OTHER, "MUU") \
  X(RTP_TITLE, "NIMI") \
  X(RTP_ALBUM, "ALBUMI") \
  X(RTP_TRACK, "KAPPALE") \
  X(RTP_ARTIST, "ESITTÄJÄ") \
  X(RTP_WORK, "TEOS") \
  X(RTP_MOVEMENT, "OSA") \
  X(RTP_CONDUCTOR, "DIRIGENTTI") \
  X(RTP_COMPOSER, "SÄVELTÄJÄ") \
  X(RTP_BAND, "YHTYE") \
  X(RTP_COMMENT, "KOMMENTTI") \
  X(RTP_NEWS, "UUTISET") \
  X(RTP_LOCAL, "LÄHIUUTISET") \
  X(RTP_STOCKS, "PÖRSSI") \
  X(RTP_SPORT, "URHEILU") \
  X(RTP_LOTTERY, "ARVONTA") \
  X(RTP_HOROSCOPE, "HOROSKOOPPI") \
  X(RTP_DIVERSION, "AJANVIETE") \
  X(RTP_HEALTH, "TERVEYS") \
  X(RTP_EVENT, "TAPAHTUMA") \
  X(RTP_SCENE, "MENOT") \
  X(RTP_CINEMA, "ELOKUVAT") \
  X(RTP_FUN, "HUUMORI") \
  X(RTP_DATE, "PÄIVÄYS") \
  X(RTP_WEATHER, "SÄÄ") \
  X(RTP_TRAFFIC, "LIIKENNE") \
  X(RTP_ALARM, "HÄLYTYS") \
  X(RTP_ADVERT, "MAINOS") \
  X(RTP_STATION, "ASEMA") \
  X(RTP_PROGRAM, "OHJELMA") \
  X(RTP_NEXT, "SEURAAVA") \
  X(RTP_PART, "JAKSO") \
  X(RTP_HOST, "JUONTAJA") \
  X(RTP_EDITORS, "TOIMITUS") \
  X(RTP_FREQUENCY, "TAAJUUS") \
  X(RTP_HOMEPAGE, "KOTISIVU") \
  X(RTP_CHANNEL, "KANAVA") \
  X(RTP_PHONE, "PUHELIN") \
  X(RTP_EMAIL, "SÄHKÖPOSTI") \
  X(RTP_VOTE, "ÄÄNESTYS") \
  X(RTP_PLACE, "PAIKKA") \
  X(RTP_APPOINTMENT, "TAPAAMINEN") \
  X(RTP_ID, "TUNNUS") \
  X(RTP_PURCHASE, "OSTO") \
  X(DAY_SUNDAY, "SUNNUNTAI") \
  X(DAY_MONDAY, "MAANANTAI") \
  X(DAY_TUESDAY, "TIISTAI") \
  X(DAY_WEDNESDAY, "KESKIVIIKKO") \
  X(DAY_THURSDAY, "TORSTAI") \
  X(DAY_FRIDAY, "PERJANTAI") \
  X(DAY_SATURDAY, "LAUANTAI") \
  X(MONTH_JANUARY, "tammikuuta") \
  X(MONTH_FEBRUARY, "helmikuuta") \
  X(MONTH_MARCH, "maaliskuuta") \
  X(MONTH_APRIL, "huhtikuuta") \
  X(MONTH_MAY, "toukokuuta") \
  X(MONTH_JUNE, "kesäkuuta") \
  X(MONTH_JULY, "heinäkuuta") \
  X(MONTH_AUGUST, "elokuuta") \
  X(MONTH_SEPTEMBER, "syyskuuta") \
  X(MONTH_OCTOBER, "lokakuuta") \
  X(MONTH_NOVEMBER, "marraskuuta") \
  X(MONTH_DECEMBER, "joulukuuta") \
  X(DATE_SUFFIX_TH, ".") \
  X(DATE_SUFFIX_ST, ".") \
  X(DATE_SUFFIX_ND, ".") \
  X(DATE_SUFFIX_RD, ".") \
  X(DATE_TODAY, "Tänään") \
  X(DATE_YESTERDAY, "Eilen") \
  X(DATE_YDAY, "EILEN") \
  X(DATE_FMT_WHEN_DATE, "%d.%s %s") \
  X(MONTH_SHORT_JAN, "1.") \
  X(MONTH_SHORT_FEB, "2.") \
  X(MONTH_SHORT_MAR, "3.") \
  X(MONTH_SHORT_APR, "4.") \
  X(MONTH_SHORT_MAY, "5.") \
  X(MONTH_SHORT_JUN, "6.") \
  X(MONTH_SHORT_JUL, "7.") \
  X(MONTH_SHORT_AUG, "8.") \
  X(MONTH_SHORT_SEP, "9.") \
  X(MONTH_SHORT_OCT, "10.") \
  X(MONTH_SHORT_NOV, "11.") \
  X(MONTH_SHORT_DEC, "12.") \
  X(MONTH_TAG_JAN, "1.") \
  X(MONTH_TAG_FEB, "2.") \
  X(MONTH_TAG_MAR, "3.") \
  X(MONTH_TAG_APR, "4.") \
  X(MONTH_TAG_MAY, "5.") \
  X(MONTH_TAG_JUN, "6.") \
  X(MONTH_TAG_JUL, "7.") \
  X(MONTH_TAG_AUG, "8.") \
  X(MONTH_TAG_SEP, "9.") \
  X(MONTH_TAG_OCT, "10.") \
  X(MONTH_TAG_NOV, "11.") \
  X(MONTH_TAG_DEC, "12.") \
  X(DX_NOW, "nyt") \
  X(DX_FMT_MIN_AGO, "%u min sitten") \
  X(DX_FMT_H_AGO, "%u h sitten") \
  X(DX_FMT_D_AGO, "%u pv sitten") \
  X(TOUCH_CAL_TITLE, "Kalibroi kosketus") \
  X(TOUCH_CAL_HOLD, "Pidä ympyrää, kunnes se täyttyy") \
  X(TOUCH_CAL_CANCEL_HINT, "Peru: käännä tai paina nuppia") \
  X(TOUCH_CAL_CHECK, "Tarkistus") \
  X(TOUCH_CAL_TAP_DOT, "Napauta pistettä") \
  X(TOUCH_CAL_SAVED, "Kalibrointi tallennettu") \
  X(TOUCH_CAL_FMT_LANDED, "Tarkistus osui %u px ohi") \
  X(TOUCH_CAL_BACK_HINT, "Palaa painamalla nuppia") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Ei tallennettu") \
  X(TOUCH_CAL_FMT_MISSED, "Tarkistus osui %u px ohi,") \
  X(TOUCH_CAL_NO_FIT, "Merkeistä ei saatu kalibrointia,") \
  X(TOUCH_CAL_NOT_SAVED, "Sitä ei voitu tallentaa,") \
  X(TOUCH_CAL_OLD_STAYS, "joten vanha kalibrointi säilyy") \
  X(TOUCH_CAL_RETRY_HINT, "Paina nuppia: uusi yritys, käännä: poistu")
// clang-format on
