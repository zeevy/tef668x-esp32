/*
 * The panel's texts in Spanish. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_ES(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Escaneo FM") \
  X(MENU_SCAN_MW, "Escaneo AM") \
  X(MENU_SCAN_SW, "Escaneo SW") \
  X(MENU_SCAN_LW, "Escaneo LW") \
  X(MENU_SQUELCH, "Modo de squelch") \
  X(MENU_SQUELCH_LEVEL, "Nivel de squelch") \
  X(MENU_SQUELCH_FLOOR, "Suelo de squelch") \
  X(MENU_VOLUME_AGC, "AGC de volumen") \
  X(MENU_AGC_BOOST, "Refuerzo AGC") \
  X(MENU_MUTE_RAMP, "Rampa de silencio") \
  X(MENU_BAND_PLAN, "Plan de banda") \
  X(MENU_TUNING_STEP, "Paso de sintonía") \
  X(MENU_SEEK_SENSITIVITY, "Sensib. búsqueda") \
  X(MENU_DE_EMPHASIS, "Deénfasis") \
  X(MENU_FORCED_MONO, "Forzar mono") \
  X(MENU_STEREO_BLEND, "Mezcla estéreo") \
  X(MENU_ST_HI_BLEND, "Mezcla estéreo / alta") \
  X(MENU_STEREO, "Estéreo") \
  X(MENU_HIGH_CUT, "Corte alto") \
  X(MENU_NOISE_BLANKER, "Supresor de ruido") \
  X(MENU_EQUALISER, "Ecualizador") \
  X(COMMON_RDS_DECODER, "Decodificador RDS") \
  X(MENU_RDS_REGION, "Región RDS") \
  X(MENU_MW_SPACING, "Paso MW") \
  X(MENU_FILTER_WIDTH, "Ancho de filtro") \
  X(MENU_SOFT_MUTE, "Silencio suave") \
  X(MENU_DWELL, "Espera") \
  X(MENU_STOP_ON, "Parada") \
  X(MENU_SCAN, "Escanear") \
  X(MENU_MEMORY_FROM, "Memoria inicial") \
  X(MENU_MEMORY_TO, "Memoria final") \
  X(MENU_PRESET_RANGE, "Rango de memorias") \
  X(MENU_LOOP_THE_BAND, "Repetir banda") \
  X(MENU_DX_WIDTH, "Ancho de escaneo") \
  X(MENU_MUTE_WHILE_SCANNING, "Silencio al escanear") \
  X(MENU_AUTO_LOG_NEW, "Autorregistro") \
  X(MENU_LOG_RADIO_TEXT, "Registrar radiotexto") \
  X(MENU_WATCH_PRESETS, "Vigilar memorias") \
  X(MENU_SCOPE_BASELINE, "Ref. espectro") \
  X(MENU_MEDIAN, "Mediana") \
  X(MENU_FMT_SWEEP_N, "Barrido %u") \
  X(DX_FMT_WATCH_UP, "%s sube %d dB") \
  X(MENU_LEARN_LOCALS, "Aprender locales") \
  X(MENU_START_SCAN, "Iniciar escaneo") \
  X(MENU_DAY_THEME, "Tema de día") \
  X(MENU_NIGHT_THEME, "Tema de noche") \
  X(MENU_THEME, "Tema") \
  X(MENU_BRIGHTNESS, "Brillo") \
  X(MENU_DIM_LEVEL, "Brillo bajo") \
  X(MENU_DIM_AFTER, "Atenuar tras") \
  X(MENU_ROTATION, "Girar pantalla") \
  X(COMMON_BATTERY, "Batería") \
  X(MENU_BATTERY_CHECK, "Control de batería") \
  X(MENU_LANGUAGE, "Idioma \xC2\xB7 Language") \
  X(MENU_ENCODER, "Tipo de encoder") \
  X(MENU_DIRECTION, "Sentido del encoder") \
  X(MENU_KEY_BEEPS, "Pitido de teclas") \
  X(MENU_BAND_EDGE_BEEP, "Pitido fin de banda") \
  X(MENU_START_CHIME, "Sonido de inicio") \
  X(MENU_KEYPAD_TIMEOUT, "Espera del teclado") \
  X(MENU_TOUCH, "Táctil") \
  X(MENU_CALIBRATE_TOUCH, "Calibrar táctil") \
  X(MENU_CLOCK_FROM_NETWORK, "Hora de red") \
  X(MENU_WEB_PIN, "PIN web") \
  X(MENU_NEW_PIN, "PIN nuevo") \
  X(MENU_FMT_DIGIT_OF, "Dígito %u de %u") \
  X(MENU_STATUS, "Estado de conexión") \
  X(MENU_WEB_SERVER, "Servidor web") \
  X(MENU_FMT_PC_CONNECTED, "Conectados: %u") \
  X(MENU_WEB_ADDRESS, "Dirección web") \
  X(MENU_IP_ADDRESS, "Dirección IP") \
  X(MENU_WI_FI_NAME, "Red Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Nombre hotspot") \
  X(MENU_MAC, "Dirección MAC") \
  X(MENU_WI_FI_SIGNAL, "Señal Wi-Fi") \
  X(MENU_NETWORK_INFO, "Info de red") \
  X(MENU_RESTART, "Reiniciar radio") \
  X(MENU_AUTO_OFF, "Apagar tras") \
  X(MENU_UPDATE_CHECK, "Buscar actualizaciones") \
  X(MENU_FIRMWARE_UPDATE, "Actualizar firmware") \
  X(MENU_FMT_UPDATE_TO, "Actualizar a %s") \
  X(MENU_UPDATE_TITLE, "Actualización disponible") \
  X(MENU_UPDATE_THIS_RADIO, "Esta radio") \
  X(MENU_UPDATE_NEW_VERSION, "Versión nueva") \
  X(MENU_UPDATE_DOWNLOAD, "Descarga") \
  X(COMMON_SETTINGS, "Ajustes") \
  X(COMMON_KEPT, "Intactos") \
  X(MENU_FMT_QUESTION, "¿%s?") \
  X(MENU_STATION, "Emisora") \
  X(MENU_UPDATE_NOW, "Actualizar") \
  X(MENU_LATER, "Más tarde") \
  X(MENU_UPDATE_UP_TO_DATE, "Al día") \
  X(MENU_UPDATE_CHECKING, "Buscando") \
  X(MENU_UPDATE_NOT_CHECKED, "Sin buscar") \
  X(MENU_UPDATE_CHECK_FAILED, "Error al buscar") \
  X(MENU_NOTE_NO_UPDATE, "Sin actualizaciones") \
  X(MENU_VERSION, "Versión de firmware") \
  X(MENU_RUNNING_FROM, "Arranque desde") \
  X(MENU_UPTIME, "Tiempo activo") \
  X(MENU_BATTERY_VOLTAGE, "Voltaje de batería") \
  X(COMMON_TUNER, "Sintonizador") \
  X(MENU_RESET_REASON, "Motivo de reinicio") \
  X(MENU_CPU_CORE_0, "Núcleo CPU 0") \
  X(MENU_CPU_CORE_1, "Núcleo CPU 1") \
  X(MENU_FREE_HEAP, "Heap libre") \
  X(MENU_LOWEST_HEAP, "Heap mínimo") \
  X(MENU_LARGEST_BLOCK, "Bloque mayor") \
  X(MENU_LVGL_POOL, "Pool LVGL") \
  X(MENU_FLASH_SIZE, "Tamaño flash") \
  X(RESET_POWER, "Encendido") \
  X(RESET_PANIC, "Pánico") \
  X(RESET_INT_WATCHDOG, "Watchdog IRQ") \
  X(RESET_TASK_WATCHDOG, "Watchdog tarea") \
  X(RESET_DEEP_SLEEP, "Sueño profundo") \
  X(RESET_BROWNOUT, "Baja tensión") \
  X(RESET_PIN, "Pin de reset") \
  X(RESET_UNKNOWN, "Desconocido") \
  X(RESET_ASKED, "Reinicio pedido") \
  X(RESET_BOOT_WATCHDOG, "Watchdog arranque") \
  X(RESET_ROLLBACK, "Reversión") \
  X(RESET_DISPLAY, "Fallo de pantalla") \
  X(RESET_UPDATE, "Actualización") \
  X(MENU_DEVELOPER, "Desarrollador") \
  X(MENU_BUILD, "Compilación") \
  X(MENU_LICENSE, "Licencia") \
  X(ABOUT_BUILD_UNKNOWN, "desconocida") \
  X(MENU_GO_TO, "Ir a") \
  X(MENU_GO_TO_BANDWIDTH, "Ancho de banda") \
  X(MENU_GO_TO_DX, "Modo DX") \
  X(MENU_GO_TO_SCOPE, "Espectro") \
  X(MENU_GO_TO_BAND, "Banda siguiente") \
  X(MENU_GO_TO_LOG, "Registrar emisora") \
  X(MENU_GO_TO_SAVE, "Guardar emisora") \
  X(MENU_FMT_SAVED_AS, "Guardada en P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Ya en P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Memorias llenas") \
  X(MENU_GO_TO_SLEEP, "Reposo") \
  X(MENU_STATIONS, "Emisoras") \
  X(MENU_MEMORY, "Memorias") \
  X(MENU_FM_SETUP, "Recepción FM") \
  X(MENU_AM_SETUP, "Recepción AM") \
  X(MENU_DX_SETUP, "Escáner DX") \
  X(MENU_DISPLAY, "Pantalla") \
  X(MENU_CONTROLS, "Controles") \
  X(MENU_CONNECTIVITY, "Conectividad") \
  X(MENU_SYSTEM, "Sistema") \
  X(MENU_DIAGNOSTICS, "Diagnóstico") \
  X(MENU_ABOUT, "Acerca de") \
  X(MENU_REGION_FULL, "Completo") \
  X(MENU_REGION_JAPAN, "Japón") \
  X(MENU_REGION_WIDE, "Amplio") \
  X(MENU_REGION_WORLD, "Mundial") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Norteamérica") \
  X(MENU_WIFI_JOINED, "Conectado") \
  X(MENU_WIFI_JOINING, "Conectando") \
  X(COMMON_OFF, "No") \
  X(MENU_FMT_KB_OF_KB, "%u de %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V al inicio") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, hace %u min") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, hace %u h") \
  X(MENU_FMT_AGC_GAIN_NOW, "Ganancia actual %+d dB") \
  X(MENU_STATION_LOG, "Registro") \
  X(COMMON_NONE, "Ninguno") \
  X(MENU_FMT_SCAN_PROGRESS, "Escaneo %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Guardadas: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Parado, guardadas: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Lleno, sin guardar: %u") \
  X(COMMON_YES, "Sí") \
  X(MENU_UNKNOWN_VALUE, "Desconocido") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Boca abajo") \
  X(MENU_CUT_ONLY, "Solo atenuar") \
  X(MENU_NEVER, "Nunca") \
  X(MENU_BATTERY_PER_CENT, "Porcentaje") \
  X(MENU_BATTERY_VOLTS, "Voltios") \
  X(MENU_KEYS, "Solo teclado") \
  X(MENU_KEYS_LONG, "Pulsación corta y larga") \
  X(MENU_EVERY_PRESS, "Cada pulsación") \
  X(MENU_STANDARD, "Estándar") \
  X(MENU_OPTICAL, "Óptico") \
  X(MENU_REVERSED, "Invertido") \
  X(MENU_ANY_PI, "Con PI") \
  X(MENU_STOP_NEVER, "Nunca") \
  X(MENU_NEW_ONLY, "Solo nuevas") \
  X(MENU_WHOLE_BAND, "Banda entera") \
  X(MENU_MEMORY_ONLY, "Solo memorias") \
  X(MENU_BAND_MEMORY, "Banda + memorias") \
  X(COMMON_ON, "Sí") \
  X(MENU_HINT_RESTART, "Se aplica al reiniciar") \
  X(MENU_TITLE, "Menú") \
  X(COMMON_FM_ONLY, "Solo FM") \
  X(MENU_NOTE_OFF_PLAN, "Fuera del plan de banda") \
  X(MENU_NOTE_SWITCH_TO_FM, "Primero cambie a FM") \
  X(MENU_NOTE_RADIO_BUSY, "Radio ocupada - reintente") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Active antes el RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Firmware a prueba - espere") \
  X(MENU_NOTE_NOT_STORED, "No guardado - restaurado") \
  X(MENU_NOTE_NOT_PUT_BACK, "Radio ocupada - no restaurado") \
  X(MENU_NOTE_NOT_SAVED, "Sin guardar") \
  X(MENU_SAVE, "Guardar") \
  X(RADIO_PRESS_TO_START, "Pulse para iniciar") \
  X(COMMON_STILL_TUNING, "Sintonizando - espere") \
  X(COMMON_SCAN_RUNNING, "Escaneando - espere") \
  X(RADIO_FMT_NO_BAND, "%s fuera de banda") \
  X(COMMON_NOT_LOGGED, "Sin registrar") \
  X(RADIO_FMT_LOGGED, "Registrada %s") \
  X(RADIO_CHECKING_UPDATES, "BUSCANDO ACTUALIZACIONES\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Escaneando %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Fallo al actualizar") \
  X(RADIO_GOING_TO_SLEEP, "Entrando en reposo") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Pulse el mando para despertar") \
  X(RADIO_OLD_IMAGE_KEPT, "Sigue el firmware anterior") \
  X(RDS_STEREO, "Estéreo") \
  X(RDS_SPEECH, "Voz") \
  X(RDS_MUSIC, "Música") \
  X(COMMON_ALREADY_LOGGED, "Ya registrada") \
  X(DX_RDS_IS_OFF, "RDS apagado") \
  X(DX_SCANNING, "Escaneando") \
  X(DX_SCAN_FINISHED, "Fin del escaneo") \
  X(DX_NOTHING_TO_SCAN, "Nada que escanear") \
  X(DX_CHECKING_FOR_UPDATES, "Buscando actualizaciones") \
  X(DX_A_SCAN_IS_RUNNING, "Escaneo ya en curso") \
  X(DX_CAUGHT_LIST_UNREAD, "Capturas no disponibles") \
  X(DX_NO_MEMORY, "Sin memorias") \
  X(DX_NOT_STARTED, "Sin iniciar") \
  X(DX_SWEEPING, "Barriendo") \
  X(COMMON_FMT_TUNE_TYPED, "Ir a %s") \
  X(RADIO_SQL_OFF, "NO") \
  X(DX_FMT_ROWS_OF, "%u-%u de %u") \
  X(DX_FMT_FOUND, "Halladas: %u") \
  X(DX_MODE_SWEEPING, "BARRIDO") \
  X(DX_MODE_LEARN_LOCALS, "APRENDER LOCALES") \
  X(DX_FMT_MODE_MEMORY, "MEMORIAS %u-%u") \
  X(DX_MODE_WHOLE_BAND, "BANDA ENTERA") \
  X(DX_MODE_BAND_MEMORY, "BANDA + MEM.") \
  X(DX_RULE_NO_STOP, "SIN PARAR") \
  X(DX_RULE_STOP_ON_PI, "PARAR CON PI") \
  X(DX_RULE_STOP_ON_NEW, "PARAR SI NUEVA") \
  X(DX_FMT_NOT_PRESET, "no P%02d") \
  X(DX_CONTEXT_SWEEPING, "barriendo") \
  X(DX_PRESS_TO_SWEEP, "Pulse para barrer") \
  X(SCOPE_FMT_TITLE, "Espectro %s") \
  X(SCOPE_FULL, "Toda") \
  X(DX_FIXED, "FIJO") \
  X(DX_FMT_MEDIAN_OF, "MEDIANA DE %u") \
  X(DX_FMT_FLOOR, "SUELO %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Cerrar") \
  X(RECOVERY_PRESS_AGAIN, "Pulse otra vez") \
  X(PAGE_TUNE, "Sintonizar") \
  X(PAGE_DELETE, "Borrar") \
  X(PAGE_LEVEL, "Nivel") \
  X(PAGE_NOISE, "Ruido") \
  X(PAGE_MULTIPATH, "Multitrayecto") \
  X(PAGE_COCHANNEL, "Cocanal") \
  X(PAGE_STEREO, "Estéreo") \
  X(PAGE_WIDTH, "Ancho") \
  X(PAGE_BAND, "Banda") \
  X(PAGE_NOTE_CHANGED, "Cambió desde que se abrió") \
  X(PAGE_NOTE_NOT_DELETED, "No se borró") \
  X(PAGE_FMT_SAVED, "Guardada: %s") \
  X(PAGE_TIME_UNKNOWN, "Hora desconocida") \
  X(MENU_FMT_TUNER_PATCH, "%s parche %u") \
  X(BW_TITLE, "Ancho de banda") \
  X(KEYPAD_TITLE, "Frecuencia") \
  X(KEYPAD_CANCEL, "Cancelar") \
  X(DX_CATCHES_EMPTY_TITLE, "Aún sin capturas") \
  X(DX_CATCHES_EMPTY_NOTE, "Las emisoras confirmadas en la sesión salen aquí") \
  X(DX_TITLE_CATCHES, "Capturas") \
  X(DX_NEW, "NUEVA") \
  X(DX_READY, "Listo") \
  X(DX_DWELL, "ESPERA") \
  X(DX_STOPPED, "PARADO") \
  X(DX_TITLE_SCANNER, "Escáner") \
  X(DX_TITLE_SCOPE, "Espectro") \
  X(DX_LEVEL, "NIVEL") \
  X(DX_RISE, "SUBE") \
  X(DX_NO_STATION_YET, "Aún sin emisora") \
  X(DX_NO_RDS_YET, "Aún sin RDS") \
  X(DX_OFFSET, "DESVÍO") \
  X(DX_NO_ID, "SIN ID") \
  X(RDS_TITLE_TEXT, "Radiotexto") \
  X(RDS_TITLE_NETWORKS, "Redes") \
  X(RDS_TITLE_DECODER, "Decodif.") \
  X(RDS_NO_COUNTRY, "sin ECC") \
  X(RDS_MS_UNKNOWN, "M/V") \
  X(RDS_RTPLUS_NONE, "Sin RT+ de esta emisora") \
  X(RDS_RTPLUS_WAIT, "Sin etiquetas RT+ para este texto") \
  X(RDS_AF_TITLE, "FRECUENCIAS ALTERNATIVAS") \
  X(RDS_EON_TITLE, "OTRAS EMISORAS, EON") \
  X(RDS_NONE_HEARD, "Nada recibido") \
  X(RDS_SYNC_CAPS, "SINC") \
  X(RDS_GROUPS_S_CAPS, "GRUPOS/S") \
  X(RDS_LOST, "PERDIDOS") \
  X(RDS_EACH_BLOCK, "POR BLOQUE") \
  X(RDS_LEGEND_CLEAN, "limpio") \
  X(RDS_LEGEND_FIXED, "corregido") \
  X(RDS_LEGEND_LOST, "perdido") \
  X(RDS_ALL_CLEAN, "todo limpio") \
  X(RDS_GROUP_TYPES, "TIPOS DE GRUPO") \
  X(RDS_NO_GROUPS, "Sin grupos en el último minuto") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 últ. %u s") \
  X(RDS_COUNTRY_UNLISTED, "no listado") \
  X(RDS_NO_TYPES, "Ningún bloque B limpio en el último minuto") \
  X(RDS_FMT_EON_MORE, "+%u más") \
  X(RDS_EON_MORE_HEARD, "+ más") \
  X(RDS_MORE, "más") \
  X(RADIO_VEIL_UPDATING, "ACTUALIZANDO FIRMWARE") \
  X(RADIO_VEIL_KEEP_POWER, "No corte la alimentación") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Sin sintonizador en el bus I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "El sintonizador no está listo") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Sintonizador no identificado") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Sin parche para este sintonizador") \
  X(TUNER_UNKNOWN_TUNER_PART, "Modelo desconocido") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Escritura rechazada") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Sin respuesta a la lectura") \
  X(TUNER_OUT_OF_RANGE, "Fuera de rango") \
  X(TUNER_UNKNOWN, "Desconocido") \
  X(BAND_LONG_LW, "Onda larga") \
  X(BAND_LONG_MW, "Onda media") \
  X(BAND_LONG_SW, "Onda corta") \
  X(MENU_FMT_TUNE_TO, "Sintonizar %s en") \
  X(PTY_NONE, "Ninguno") \
  X(PTY_NEWS, "Noticias") \
  X(PTY_CURRENT_AFFAIRS, "Actualidad") \
  X(PTY_INFORMATION, "Información") \
  X(PTY_SPORT, "Deportes") \
  X(PTY_EDUCATION, "Educación") \
  X(PTY_CULTURE, "Cultura") \
  X(PTY_SCIENCE, "Ciencia") \
  X(PTY_VARIED, "Variedades") \
  X(PTY_POP_MUSIC, "Música pop") \
  X(PTY_ROCK_MUSIC, "Música rock") \
  X(PTY_EASY_LISTENING, "Música ligera") \
  X(PTY_LIGHT_CLASSICAL, "Clásica ligera") \
  X(PTY_SERIOUS_CLASSICAL, "Clásica") \
  X(PTY_OTHER_MUSIC, "Otra música") \
  X(PTY_WEATHER, "Tiempo") \
  X(PTY_FINANCE, "Finanzas") \
  X(PTY_CHILDREN, "Infantil") \
  X(PTY_SOCIAL_AFFAIRS, "Sociedad") \
  X(PTY_RELIGION, "Religión") \
  X(PTY_PHONE_IN, "Llamadas") \
  X(PTY_TRAVEL, "Viajes") \
  X(PTY_LEISURE, "Ocio") \
  X(PTY_COUNTRY_MUSIC, "Country") \
  X(PTY_NATIONAL_MUSIC, "Música nacional") \
  X(PTY_OLDIES_MUSIC, "Música de ayer") \
  X(PTY_FOLK_MUSIC, "Folclore") \
  X(PTY_DOCUMENTARY, "Documental") \
  X(PTY_ALARM_TEST, "Prueba de alarma") \
  X(PTY_ALARM, "Alarma") \
  X(PTY_NA_NEWS, "Noticias") \
  X(PTY_NA_INFORMATION, "Información") \
  X(PTY_NA_SPORTS, "Deportes") \
  X(PTY_NA_TALK, "Charla") \
  X(PTY_NA_CLASSIC_ROCK, "Rock clásico") \
  X(PTY_NA_ADULT_HITS, "Éxitos adultos") \
  X(PTY_NA_SOFT_ROCK, "Rock suave") \
  X(PTY_NA_OLDIES, "Música de ayer") \
  X(PTY_NA_SOFT, "Suave") \
  X(PTY_NA_CLASSICAL, "Clásica") \
  X(PTY_NA_RHYTHM_AND_BLUES, "R&B") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "R&B suave") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Idioma extranjero") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Música religiosa") \
  X(PTY_NA_RELIGIOUS_TALK, "Charla religiosa") \
  X(PTY_NA_PERSONALITY, "Personalidades") \
  X(PTY_NA_PUBLIC, "Pública") \
  X(PTY_NA_COLLEGE, "Universitaria") \
  X(PTY_NA_SPANISH_TALK, "Charla en español") \
  X(PTY_NA_SPANISH_MUSIC, "Música en español") \
  X(PTY_NA_UNASSIGNED, "Sin asignar") \
  X(PTY_NA_WEATHER, "Tiempo") \
  X(PTY_NA_EMERGENCY_TEST, "Prueba de emergencia") \
  X(PTY_NA_EMERGENCY, "Emergencia") \
  X(COVERAGE_INTERNATIONAL, "Internacional") \
  X(COVERAGE_NATIONAL, "Nacional") \
  X(COVERAGE_SUPRA_REGIONAL, "Suprarregional") \
  X(RTP_OTHER, "OTRO") \
  X(RTP_TITLE, "TÍTULO") \
  X(RTP_ALBUM, "ÁLBUM") \
  X(RTP_TRACK, "PISTA") \
  X(RTP_ARTIST, "ARTISTA") \
  X(RTP_WORK, "OBRA") \
  X(RTP_MOVEMENT, "MOVIMIENTO") \
  X(RTP_CONDUCTOR, "DIRECTOR") \
  X(RTP_COMPOSER, "COMPOSITOR") \
  X(RTP_BAND, "GRUPO") \
  X(RTP_COMMENT, "COMENTARIO") \
  X(RTP_GENRE, "GÉNERO") \
  X(RTP_NEWS, "NOTICIAS") \
  X(RTP_LOCAL, "NOT. LOCALES") \
  X(RTP_STOCKS, "BOLSA") \
  X(RTP_SPORT, "DEPORTE") \
  X(RTP_LOTTERY, "LOTERÍA") \
  X(RTP_HOROSCOPE, "HORÓSCOPO") \
  X(RTP_DIVERSION, "DIVERSIÓN") \
  X(RTP_HEALTH, "SALUD") \
  X(RTP_EVENT, "EVENTO") \
  X(RTP_SCENE, "ESCENA") \
  X(RTP_CINEMA, "CINE") \
  X(RTP_FUN, "HUMOR") \
  X(RTP_DATE, "FECHA") \
  X(RTP_WEATHER, "TIEMPO") \
  X(RTP_TRAFFIC, "TRÁFICO") \
  X(RTP_ALARM, "ALARMA") \
  X(RTP_ADVERT, "ANUNCIO") \
  X(RTP_STATION, "EMISORA") \
  X(RTP_PROGRAM, "PROGRAMA") \
  X(RTP_NEXT, "PRÓXIMO") \
  X(RTP_PART, "PARTE") \
  X(RTP_HOST, "LOCUTOR") \
  X(RTP_EDITORS, "REDACCIÓN") \
  X(RTP_FREQUENCY, "FRECUENCIA") \
  X(RTP_HOMEPAGE, "WEB") \
  X(RTP_CHANNEL, "CANAL") \
  X(RTP_HOTLINE, "LÍNEA DIRECTA") \
  X(RTP_STUDIO, "ESTUDIO") \
  X(RTP_PHONE, "TELÉFONO") \
  X(RTP_EMAIL, "CORREO") \
  X(RTP_VOTE, "VOTO") \
  X(RTP_PLACE, "LUGAR") \
  X(RTP_APPOINTMENT, "CITA") \
  X(RTP_PURCHASE, "COMPRA") \
  X(DAY_SUNDAY, "DOMINGO") \
  X(DAY_MONDAY, "LUNES") \
  X(DAY_TUESDAY, "MARTES") \
  X(DAY_WEDNESDAY, "MIÉRCOLES") \
  X(DAY_THURSDAY, "JUEVES") \
  X(DAY_FRIDAY, "VIERNES") \
  X(DAY_SATURDAY, "SÁBADO") \
  X(MONTH_JANUARY, "enero") \
  X(MONTH_FEBRUARY, "febrero") \
  X(MONTH_MARCH, "marzo") \
  X(MONTH_APRIL, "abril") \
  X(MONTH_MAY, "mayo") \
  X(MONTH_JUNE, "junio") \
  X(MONTH_JULY, "julio") \
  X(MONTH_AUGUST, "agosto") \
  X(MONTH_SEPTEMBER, "septiembre") \
  X(MONTH_OCTOBER, "octubre") \
  X(MONTH_NOVEMBER, "noviembre") \
  X(MONTH_DECEMBER, "diciembre") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_TODAY, "Hoy") \
  X(DATE_YESTERDAY, "Ayer") \
  X(DATE_YDAY, "AYER") \
  X(MONTH_SHORT_JAN, "ene") \
  X(MONTH_SHORT_FEB, "feb") \
  X(MONTH_SHORT_MAR, "mar") \
  X(MONTH_SHORT_APR, "abr") \
  X(MONTH_SHORT_MAY, "may") \
  X(MONTH_SHORT_JUN, "jun") \
  X(MONTH_SHORT_JUL, "jul") \
  X(MONTH_SHORT_AUG, "ago") \
  X(MONTH_SHORT_SEP, "sep") \
  X(MONTH_SHORT_OCT, "oct") \
  X(MONTH_SHORT_NOV, "nov") \
  X(MONTH_SHORT_DEC, "dic") \
  X(MONTH_TAG_JAN, "ENE") \
  X(MONTH_TAG_APR, "ABR") \
  X(MONTH_TAG_AUG, "AGO") \
  X(MONTH_TAG_DEC, "DIC") \
  X(DX_NOW, "ahora") \
  X(DX_FMT_MIN_AGO, "hace %u min") \
  X(DX_FMT_H_AGO, "hace %u h") \
  X(DX_FMT_D_AGO, "hace %u d") \
  X(TOUCH_CAL_TITLE, "Calibrar táctil") \
  X(TOUCH_CAL_HOLD, "Pulse el círculo hasta que se llene") \
  X(TOUCH_CAL_CANCEL_HINT, "Gire o pulse el mando para cancelar") \
  X(TOUCH_CAL_CHECK, "Prueba") \
  X(TOUCH_CAL_TAP_DOT, "Toque el punto") \
  X(TOUCH_CAL_SAVED, "Calibración guardada") \
  X(TOUCH_CAL_FMT_LANDED, "La prueba se desvió %u px") \
  X(TOUCH_CAL_BACK_HINT, "Pulse el mando para volver") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "No guardada") \
  X(TOUCH_CAL_FMT_MISSED, "La prueba se desvió %u px,") \
  X(TOUCH_CAL_NO_FIT, "Las marcas no dan una calibración,") \
  X(TOUCH_CAL_NOT_SAVED, "No se pudo guardar,") \
  X(TOUCH_CAL_OLD_STAYS, "así que queda la anterior") \
  X(TOUCH_CAL_RETRY_HINT, "Pulse el mando para repetir, gírelo para salir")
// clang-format on
