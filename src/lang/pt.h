/*
 * The panel's texts in Portuguese. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_PT(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Pesquisa FM") \
  X(MENU_SCAN_MW, "Pesquisa AM") \
  X(MENU_SCAN_SW, "Pesquisa SW") \
  X(MENU_SCAN_LW, "Pesquisa LW") \
  X(MENU_SQUELCH, "Modo de squelch") \
  X(MENU_SQUELCH_LEVEL, "Nível de squelch") \
  X(MENU_SQUELCH_FLOOR, "Mínimo de squelch") \
  X(MENU_VOLUME_AGC, "AGC de volume") \
  X(MENU_AGC_BOOST, "Reforço AGC") \
  X(MENU_MUTE_RAMP, "Rampa de mudo") \
  X(MENU_BAND_PLAN, "Plano de banda") \
  X(MENU_TUNING_STEP, "Passo de sintonia") \
  X(MENU_SEEK_SENSITIVITY, "Sensib. da busca") \
  X(MENU_DE_EMPHASIS, "De-ênfase") \
  X(MENU_FORCED_MONO, "Forçar mono") \
  X(MENU_STEREO_BLEND, "Mistura estéreo") \
  X(MENU_ST_HI_BLEND, "Mistura estéreo / agudos") \
  X(MENU_STEREO, "Estéreo") \
  X(MENU_HIGH_CUT, "Corte de agudos") \
  X(MENU_NOISE_BLANKER, "Supressor de ruído") \
  X(MENU_EQUALISER, "Equalizador") \
  X(COMMON_RDS_DECODER, "Descodificador RDS") \
  X(MENU_RDS_REGION, "Região RDS") \
  X(MENU_MW_SPACING, "Passo MW") \
  X(MENU_FILTER_WIDTH, "Largura do filtro") \
  X(MENU_SOFT_MUTE, "Mudo suave") \
  X(MENU_DWELL, "Espera") \
  X(MENU_STOP_ON, "Quando parar") \
  X(MENU_SCAN, "Pesquisar em") \
  X(MENU_MEMORY_FROM, "Memória inicial") \
  X(MENU_MEMORY_TO, "Memória final") \
  X(MENU_PRESET_RANGE, "Gama de memórias") \
  X(MENU_LOOP_THE_BAND, "Repetir banda") \
  X(MENU_DX_WIDTH, "Largura de banda") \
  X(MENU_MUTE_WHILE_SCANNING, "Mudo na pesquisa") \
  X(MENU_AUTO_LOG_NEW, "Anotação automática") \
  X(MENU_LOG_RADIO_TEXT, "Anotar radiotexto") \
  X(MENU_WATCH_PRESETS, "Vigiar memórias") \
  X(MENU_SCOPE_BASELINE, "Base do espectro") \
  X(MENU_MEDIAN, "Mediana") \
  X(MENU_FMT_SWEEP_N, "Varrimento %u") \
  X(DX_FMT_WATCH_UP, "%s subiu %d dB") \
  X(MENU_LEARN_LOCALS, "Aprender estações locais") \
  X(MENU_START_SCAN, "Iniciar pesquisa") \
  X(MENU_DAY_THEME, "Tema de dia") \
  X(MENU_NIGHT_THEME, "Tema de noite") \
  X(MENU_THEME, "Tema") \
  X(MENU_BRIGHTNESS, "Brilho") \
  X(MENU_DIM_LEVEL, "Brilho reduzido") \
  X(MENU_DIM_AFTER, "Reduzir após") \
  X(MENU_ROTATION, "Rotação do visor") \
  X(COMMON_BATTERY, "Bateria") \
  X(MENU_BATTERY_CHECK, "Verificar bateria") \
  X(MENU_LANGUAGE, "Idioma \xC2\xB7 Language") \
  X(MENU_ENCODER, "Tipo de encoder") \
  X(MENU_DIRECTION, "Sentido do encoder") \
  X(MENU_KEY_BEEPS, "Bip das teclas") \
  X(MENU_BAND_EDGE_BEEP, "Bip no fim da banda") \
  X(MENU_START_CHIME, "Som ao ligar") \
  X(MENU_KEYPAD_TIMEOUT, "Espera do teclado") \
  X(MENU_TOUCH, "Toque") \
  X(MENU_CALIBRATE_TOUCH, "Calibrar toque") \
  X(MENU_CLOCK_FROM_NETWORK, "Hora da rede") \
  X(MENU_WEB_PIN, "PIN web") \
  X(MENU_NEW_PIN, "Novo PIN") \
  X(MENU_FMT_DIGIT_OF, "Dígito %u de %u") \
  X(MENU_STATUS, "Estado da conexão") \
  X(MENU_WEB_SERVER, "Servidor web") \
  X(MENU_FMT_PC_CONNECTED, "Conectados: %u") \
  X(MENU_WEB_ADDRESS, "Endereço web") \
  X(MENU_IP_ADDRESS, "Endereço IP") \
  X(MENU_WI_FI_NAME, "Rede Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Nome do hotspot") \
  X(MENU_MAC, "Endereço MAC") \
  X(MENU_WI_FI_SIGNAL, "Sinal Wi-Fi") \
  X(MENU_NETWORK_INFO, "Info. de rede") \
  X(MENU_RESTART, "Reiniciar rádio") \
  X(MENU_AUTO_OFF, "Desligar auto.") \
  X(MENU_UPDATE_CHECK, "Verificar atualizações") \
  X(MENU_FIRMWARE_UPDATE, "Atualizar firmware") \
  X(MENU_FMT_UPDATE_TO, "Atualizar para %s") \
  X(MENU_UPDATE_TITLE, "Atualização disponível") \
  X(MENU_UPDATE_THIS_RADIO, "Este rádio") \
  X(MENU_UPDATE_NEW_VERSION, "Nova versão") \
  X(MENU_UPDATE_DOWNLOAD, "Tamanho") \
  X(COMMON_SETTINGS, "Ajustes") \
  X(COMMON_KEPT, "Mantidos") \
  X(MENU_STATION, "Estação") \
  X(MENU_UPDATE_NOW, "Atualizar") \
  X(MENU_LATER, "Depois") \
  X(MENU_UPDATE_UP_TO_DATE, "Atualizado") \
  X(MENU_UPDATE_CHECKING, "A verificar") \
  X(MENU_UPDATE_NOT_CHECKED, "Não verificado") \
  X(MENU_UPDATE_CHECK_FAILED, "Verif. falhou") \
  X(MENU_NOTE_NO_UPDATE, "Sem atualizações") \
  X(MENU_VERSION, "Versão do firmware") \
  X(MENU_RUNNING_FROM, "Iniciado de") \
  X(MENU_UPTIME, "Tempo ligado") \
  X(MENU_BATTERY_VOLTAGE, "Tensão da bateria") \
  X(COMMON_TUNER, "Sintonizador") \
  X(MENU_RESET_REASON, "Motivo do reinício") \
  X(MENU_CPU_CORE_0, "Núcleo CPU 0") \
  X(MENU_CPU_CORE_1, "Núcleo CPU 1") \
  X(MENU_FREE_HEAP, "Heap livre") \
  X(MENU_LOWEST_HEAP, "Heap mínimo") \
  X(MENU_LARGEST_BLOCK, "Maior bloco") \
  X(MENU_LVGL_POOL, "Pool LVGL") \
  X(MENU_FLASH_SIZE, "Flash") \
  X(RESET_POWER, "Ao ligar") \
  X(RESET_PANIC, "Pânico") \
  X(RESET_INT_WATCHDOG, "Watchdog IRQ") \
  X(RESET_TASK_WATCHDOG, "Watchdog tarefa") \
  X(RESET_DEEP_SLEEP, "Sono profundo") \
  X(RESET_BROWNOUT, "Subtensão") \
  X(RESET_PIN, "Pino reset") \
  X(RESET_UNKNOWN, "Desconhecido") \
  X(RESET_ASKED, "Reinício pedido") \
  X(RESET_BOOT_WATCHDOG, "Watchdog início") \
  X(RESET_ROLLBACK, "Reversão") \
  X(RESET_DISPLAY, "Falha do visor") \
  X(RESET_UPDATE, "Atualização") \
  X(MENU_DEVELOPER, "Autor") \
  X(MENU_BUILD, "Compilação") \
  X(MENU_LICENSE, "Licença") \
  X(ABOUT_BUILD_UNKNOWN, "desconhecida") \
  X(MENU_GO_TO, "Ir para") \
  X(MENU_GO_TO_BANDWIDTH, "Largura de banda") \
  X(MENU_GO_TO_DX, "Modo DX") \
  X(MENU_GO_TO_SCOPE, "Espectro") \
  X(MENU_GO_TO_BAND, "Próxima banda") \
  X(MENU_GO_TO_LOG, "Anotar estação atual") \
  X(MENU_GO_TO_SAVE, "Guardar estação atual") \
  X(MENU_FMT_SAVED_AS, "Guardada em P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Já é P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Memórias cheias") \
  X(MENU_GO_TO_SLEEP, "Repouso") \
  X(MENU_STATIONS, "Estações") \
  X(MENU_MEMORY, "Memórias") \
  X(MENU_AUDIO, "Áudio") \
  X(MENU_FM_SETUP, "Recepção FM") \
  X(MENU_AM_SETUP, "Recepção AM") \
  X(MENU_DX_SETUP, "Scanner DX") \
  X(MENU_DISPLAY, "Visor") \
  X(MENU_CONTROLS, "Comandos") \
  X(MENU_CONNECTIVITY, "Conectividade") \
  X(MENU_SYSTEM, "Sistema") \
  X(MENU_DIAGNOSTICS, "Diagnóstico") \
  X(MENU_ABOUT, "Sobre") \
  X(MENU_REGION_FULL, "Total") \
  X(MENU_REGION_JAPAN, "Japão") \
  X(MENU_REGION_WIDE, "Amplo") \
  X(MENU_REGION_WORLD, "Mundial") \
  X(MENU_RDS_REGION_EUROPE, "Europa") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "América do Norte") \
  X(MENU_WIFI_JOINED, "Conectado") \
  X(MENU_WIFI_JOINING, "A conectar") \
  X(COMMON_OFF, "Desl.") \
  X(MENU_FMT_KB_OF_KB, "%u de %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V ao ligar") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, há %u min") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, há %u h") \
  X(MENU_FMT_AGC_GAIN_NOW, "Ganho atual %+d dB") \
  X(MENU_STATION_LOG, "Diário") \
  X(COMMON_NONE, "Nenhum") \
  X(MENU_FMT_SCAN_PROGRESS, "Pesq. %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Guardadas: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Parou, guardadas: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Cheio, %u sem lugar") \
  X(COMMON_YES, "Sim") \
  X(COMMON_NO, "Não") \
  X(MENU_UNKNOWN_VALUE, "Desconhecido") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Invertido") \
  X(MENU_CUT_ONLY, "Só corte") \
  X(MENU_NEVER, "Nunca") \
  X(MENU_BATTERY_PER_CENT, "Por cento") \
  X(MENU_KEYS, "Só teclado") \
  X(MENU_KEYS_LONG, "Curtas e longas") \
  X(MENU_EVERY_PRESS, "Todas as teclas") \
  X(MENU_STANDARD, "Padrão") \
  X(MENU_OPTICAL, "Óptico") \
  X(MENU_REVERSED, "Invertido") \
  X(MENU_ANY_PI, "Qualquer PI") \
  X(MENU_STOP_NEVER, "Nunca") \
  X(MENU_NEW_ONLY, "Só estações novas") \
  X(MENU_WHOLE_BAND, "Banda toda") \
  X(MENU_MEMORY_ONLY, "Só memórias") \
  X(MENU_BAND_MEMORY, "Banda + memórias") \
  X(COMMON_ON, "Lig.") \
  X(MENU_HINT_RESTART, "Aplica-se ao reiniciar") \
  X(COMMON_FM_ONLY, "Só FM") \
  X(MENU_NOTE_OFF_PLAN, "Fora do plano de banda") \
  X(MENU_NOTE_SWITCH_TO_FM, "Mude para FM") \
  X(MENU_NOTE_RADIO_BUSY, "Rádio ocupado - repita") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Ligue o RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Versão em teste - aguarde") \
  X(MENU_NOTE_NOT_STORED, "Não guardado - reposto") \
  X(MENU_NOTE_NOT_PUT_BACK, "Rádio ocupado - não reposto") \
  X(MENU_NOTE_NOT_SAVED, "Não guardado") \
  X(MENU_SAVE, "Guardar") \
  X(RADIO_PRESS_TO_START, "Pressione para iniciar") \
  X(COMMON_STILL_TUNING, "A sintonizar - aguarde") \
  X(COMMON_SCAN_RUNNING, "A pesquisar - aguarde") \
  X(RADIO_FMT_NO_BAND, "%s fora das bandas") \
  X(COMMON_NOT_LOGGED, "Não anotado") \
  X(RADIO_FMT_LOGGED, "Anotado %s") \
  X(RADIO_CHECKING_UPDATES, "A VERIFICAR ATUALIZAÇÕES\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "A pesquisar %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Atualização falhou") \
  X(RADIO_GOING_TO_SLEEP, "A adormecer") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Pressione o botão para acordar") \
  X(RADIO_OLD_IMAGE_KEPT, "Firmware anterior mantido") \
  X(RDS_STEREO, "Estéreo") \
  X(RDS_SPEECH, "Fala") \
  X(RDS_MUSIC, "Música") \
  X(COMMON_ALREADY_LOGGED, "Já anotado") \
  X(DX_RDS_IS_OFF, "RDS desligado") \
  X(DX_SCANNING, "A pesquisar") \
  X(DX_SCAN_FINISHED, "Fim da pesquisa") \
  X(DX_NOTHING_TO_SCAN, "Nada a pesquisar") \
  X(DX_CHECKING_FOR_UPDATES, "A verificar atualizações") \
  X(DX_A_SCAN_IS_RUNNING, "Pesquisa já em curso") \
  X(DX_CAUGHT_LIST_UNREAD, "Capturas indisponíveis") \
  X(DX_NO_MEMORY, "Sem memórias") \
  X(DX_NOT_STARTED, "Não iniciado") \
  X(DX_SWEEPING, "A varrer") \
  X(COMMON_FMT_TUNE_TYPED, "Ir para %s") \
  X(RADIO_SQL_OFF, "DESL.") \
  X(DX_FMT_ROWS_OF, "%u-%u de %u") \
  X(DX_FMT_FOUND, "Achadas: %u") \
  X(DX_MODE_SWEEPING, "A VARRER") \
  X(DX_MODE_LEARN_LOCALS, "APRENDER LOCAIS") \
  X(DX_FMT_MODE_MEMORY, "MEM %u-%u") \
  X(DX_MODE_WHOLE_BAND, "BANDA TODA") \
  X(DX_MODE_BAND_MEMORY, "BANDA + MEM") \
  X(DX_RULE_NO_STOP, "SEM PARAR") \
  X(DX_RULE_STOP_ON_PI, "PARAR: PI") \
  X(DX_RULE_STOP_ON_NEW, "PARAR: NOVAS") \
  X(DX_FMT_NOT_PRESET, "não P%02d") \
  X(DX_CONTEXT_SWEEPING, "a varrer") \
  X(DX_PRESS_TO_SWEEP, "Pressione para varrer") \
  X(SCOPE_FMT_TITLE, "Espectro %s") \
  X(SCOPE_FULL, "Total") \
  X(DX_FIXED, "FIXO") \
  X(DX_FMT_MEDIAN_OF, "MEDIANA DE %u") \
  X(DX_FMT_FLOOR, "FUNDO %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Fechar") \
  X(RECOVERY_PRESS_AGAIN, "Confirmar") \
  X(PAGE_TUNE, "Sintonizar") \
  X(PAGE_DELETE, "Apagar") \
  X(PAGE_LEVEL, "Nível") \
  X(PAGE_NOISE, "Ruído") \
  X(PAGE_COCHANNEL, "Cocanal") \
  X(PAGE_STEREO, "Estéreo") \
  X(PAGE_WIDTH, "Largura") \
  X(PAGE_BAND, "Banda") \
  X(PAGE_NOTE_CHANGED, "Alterado desde que abriu") \
  X(PAGE_NOTE_NOT_DELETED, "Não apagado") \
  X(PAGE_FMT_SAVED, "Guardada %s") \
  X(PAGE_TIME_UNKNOWN, "Hora desconhecida") \
  X(BW_TITLE, "Larg. de banda") \
  X(KEYPAD_TITLE, "Frequência") \
  X(KEYPAD_CANCEL, "Cancelar") \
  X(DX_CATCHES_EMPTY_TITLE, "Ainda sem capturas") \
  X(DX_CATCHES_EMPTY_NOTE, "As estações confirmadas nesta sessão aparecem aqui") \
  X(DX_TITLE_CATCHES, "Capturas") \
  X(DX_NEW, "NOVA") \
  X(DX_READY, "Pronto") \
  X(DX_DWELL, "ESPERA") \
  X(DX_STOPPED, "PARADO") \
  X(DX_TITLE_SCOPE, "Espectro") \
  X(DX_LEVEL, "NÍVEL") \
  X(DX_RISE, "SOBE") \
  X(DX_NO_STATION_YET, "Ainda sem estação") \
  X(DX_NO_RDS_YET, "Ainda sem RDS") \
  X(DX_OFFSET, "DESVIO") \
  X(DX_NO_ID, "SEM ID") \
  X(RDS_TITLE_TEXT, "Radiotexto") \
  X(RDS_TITLE_NETWORKS, "Redes") \
  X(RDS_TITLE_DECODER, "Descodif.") \
  X(RDS_NO_COUNTRY, "sem ECC") \
  X(RDS_MS_UNKNOWN, "M/F") \
  X(RDS_RTPLUS_NONE, "Sem RT+ desta estação") \
  X(RDS_RTPLUS_WAIT, "Sem etiquetas RT+ para este texto") \
  X(RDS_AF_TITLE, "FREQUÊNCIAS ALTERNATIVAS") \
  X(RDS_EON_TITLE, "OUTRAS ESTAÇÕES, EON") \
  X(RDS_NONE_HEARD, "Nada ouvido") \
  X(RDS_SYNC_CAPS, "SINC") \
  X(RDS_GROUPS_S_CAPS, "GRUPOS/S") \
  X(RDS_LOST, "PERDAS") \
  X(RDS_EACH_BLOCK, "CADA BLOCO") \
  X(RDS_LEGEND_CLEAN, "limpo") \
  X(RDS_LEGEND_FIXED, "corrigido") \
  X(RDS_LEGEND_LOST, "perdido") \
  X(RDS_ALL_CLEAN, "tudo limpo") \
  X(RDS_GROUP_TYPES, "TIPOS DE GRUPO") \
  X(RDS_NO_GROUPS, "Sem grupos no último minuto") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 %u s") \
  X(RDS_COUNTRY_UNLISTED, "fora da lista") \
  X(RDS_NO_TYPES, "Nenhum bloco B limpo no último minuto") \
  X(RDS_FMT_EON_MORE, "+%u mais") \
  X(RDS_EON_MORE_HEARD, "+ mais") \
  X(RDS_MORE, "mais") \
  X(RADIO_VEIL_UPDATING, "A ATUALIZAR FIRMWARE") \
  X(RADIO_VEIL_KEEP_POWER, "Não corte a alimentação") \
  X(RADIO_MUTE, "MUDO") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Sem sintonizador no bus I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Sintonizador não ficou pronto") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Sintonizador não identificado") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Sem patch para este sintonizador") \
  X(TUNER_UNKNOWN_TUNER_PART, "Sintonizador desconhecido") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Sintonizador recusou a escrita") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Sintonizador não respondeu à leitura") \
  X(TUNER_OUT_OF_RANGE, "Fora da gama") \
  X(TUNER_UNKNOWN, "Desconhecido") \
  X(BAND_LONG_LW, "Onda longa") \
  X(BAND_LONG_MW, "Onda média") \
  X(BAND_LONG_SW, "Onda curta") \
  X(MENU_FMT_TUNE_TO, "Sintonizar %s em") \
  X(PTY_NONE, "Nenhum") \
  X(PTY_NEWS, "Notícias") \
  X(PTY_CURRENT_AFFAIRS, "Atualidade") \
  X(PTY_INFORMATION, "Informação") \
  X(PTY_SPORT, "Desporto") \
  X(PTY_EDUCATION, "Educação") \
  X(PTY_DRAMA, "Teatro") \
  X(PTY_CULTURE, "Cultura") \
  X(PTY_SCIENCE, "Ciência") \
  X(PTY_VARIED, "Variedades") \
  X(PTY_POP_MUSIC, "Música pop") \
  X(PTY_ROCK_MUSIC, "Música rock") \
  X(PTY_EASY_LISTENING, "Música ligeira") \
  X(PTY_LIGHT_CLASSICAL, "Clássica ligeira") \
  X(PTY_SERIOUS_CLASSICAL, "Clássica") \
  X(PTY_OTHER_MUSIC, "Outras músicas") \
  X(PTY_WEATHER, "Meteorologia") \
  X(PTY_FINANCE, "Economia") \
  X(PTY_CHILDREN, "Infantil") \
  X(PTY_SOCIAL_AFFAIRS, "Sociedade") \
  X(PTY_RELIGION, "Religião") \
  X(PTY_TRAVEL, "Viagens") \
  X(PTY_LEISURE, "Lazer") \
  X(PTY_COUNTRY_MUSIC, "Música country") \
  X(PTY_NATIONAL_MUSIC, "Música nacional") \
  X(PTY_OLDIES_MUSIC, "Música antiga") \
  X(PTY_FOLK_MUSIC, "Folclore") \
  X(PTY_DOCUMENTARY, "Documentário") \
  X(PTY_ALARM_TEST, "Teste de alarme") \
  X(PTY_ALARM, "Alarme") \
  X(PTY_NA_NEWS, "Notícias") \
  X(PTY_NA_INFORMATION, "Informação") \
  X(PTY_NA_SPORTS, "Desporto") \
  X(PTY_NA_TALK, "Conversa") \
  X(PTY_NA_CLASSIC_ROCK, "Rock clássico") \
  X(PTY_NA_ADULT_HITS, "Hits adultos") \
  X(PTY_NA_SOFT_ROCK, "Rock suave") \
  X(PTY_NA_OLDIES, "Música antiga") \
  X(PTY_NA_SOFT, "Suave") \
  X(PTY_NA_CLASSICAL, "Clássica") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "R&B suave") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Língua estrangeira") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Música religiosa") \
  X(PTY_NA_RELIGIOUS_TALK, "Conversa religiosa") \
  X(PTY_NA_PERSONALITY, "Personalidade") \
  X(PTY_NA_PUBLIC, "Pública") \
  X(PTY_NA_COLLEGE, "Universitária") \
  X(PTY_NA_SPANISH_TALK, "Conversa em espanhol") \
  X(PTY_NA_SPANISH_MUSIC, "Música espanhola") \
  X(PTY_NA_UNASSIGNED, "Não atribuído") \
  X(PTY_NA_WEATHER, "Meteorologia") \
  X(PTY_NA_EMERGENCY_TEST, "Teste de emergência") \
  X(PTY_NA_EMERGENCY, "Emergência") \
  X(COVERAGE_INTERNATIONAL, "Internacional") \
  X(COVERAGE_NATIONAL, "Nacional") \
  X(COVERAGE_SUPRA_REGIONAL, "Suprarregional") \
  X(RTP_OTHER, "OUTRO") \
  X(RTP_TITLE, "TÍTULO") \
  X(RTP_ALBUM, "ÁLBUM") \
  X(RTP_TRACK, "FAIXA") \
  X(RTP_ARTIST, "ARTISTA") \
  X(RTP_WORK, "OBRA") \
  X(RTP_MOVEMENT, "ANDAMENTO") \
  X(RTP_CONDUCTOR, "MAESTRO") \
  X(RTP_COMPOSER, "COMPOSITOR") \
  X(RTP_BAND, "BANDA") \
  X(RTP_COMMENT, "COMENTÁRIO") \
  X(RTP_GENRE, "ESTILO") \
  X(RTP_NEWS, "NOTÍCIAS") \
  X(RTP_LOCAL, "INFO LOCAL") \
  X(RTP_STOCKS, "BOLSA") \
  X(RTP_SPORT, "DESPORTO") \
  X(RTP_LOTTERY, "SORTEIO") \
  X(RTP_HOROSCOPE, "HORÓSCOPO") \
  X(RTP_DIVERSION, "DIVERSÃO") \
  X(RTP_HEALTH, "SAÚDE") \
  X(RTP_EVENT, "EVENTO") \
  X(RTP_SCENE, "CENA") \
  X(RTP_FUN, "HUMOR") \
  X(RTP_DATE, "DATA") \
  X(RTP_WEATHER, "METEO") \
  X(RTP_TRAFFIC, "TRÂNSITO") \
  X(RTP_ALARM, "ALARME") \
  X(RTP_ADVERT, "ANÚNCIO") \
  X(RTP_STATION, "ESTAÇÃO") \
  X(RTP_PROGRAM, "PROGRAMA") \
  X(RTP_NEXT, "PRÓXIMO") \
  X(RTP_PART, "PARTE") \
  X(RTP_HOST, "LOCUTOR") \
  X(RTP_EDITORS, "EDITORES") \
  X(RTP_FREQUENCY, "FREQUÊNCIA") \
  X(RTP_HOMEPAGE, "SITE") \
  X(RTP_CHANNEL, "CANAL") \
  X(RTP_STUDIO, "ESTÚDIO") \
  X(RTP_PHONE, "TELEFONE") \
  X(RTP_EMAIL, "E-MAIL") \
  X(RTP_VOTE, "VOTO") \
  X(RTP_PLACE, "LOCAL") \
  X(RTP_APPOINTMENT, "COMPROMISSO") \
  X(RTP_PURCHASE, "COMPRA") \
  X(DAY_SUNDAY, "DOMINGO") \
  X(DAY_MONDAY, "SEGUNDA") \
  X(DAY_TUESDAY, "TERÇA") \
  X(DAY_WEDNESDAY, "QUARTA") \
  X(DAY_THURSDAY, "QUINTA") \
  X(DAY_FRIDAY, "SEXTA") \
  X(DAY_SATURDAY, "SÁBADO") \
  X(MONTH_JANUARY, "janeiro") \
  X(MONTH_FEBRUARY, "fevereiro") \
  X(MONTH_MARCH, "março") \
  X(MONTH_APRIL, "abril") \
  X(MONTH_MAY, "maio") \
  X(MONTH_JUNE, "junho") \
  X(MONTH_JULY, "julho") \
  X(MONTH_AUGUST, "agosto") \
  X(MONTH_SEPTEMBER, "setembro") \
  X(MONTH_OCTOBER, "outubro") \
  X(MONTH_NOVEMBER, "novembro") \
  X(MONTH_DECEMBER, "dezembro") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_FMT_LINE, "%s, %d%s de %s de %d") \
  X(DATE_TODAY, "Hoje") \
  X(DATE_YESTERDAY, "Ontem") \
  X(DATE_YDAY, "ONTEM") \
  X(MONTH_SHORT_JAN, "jan") \
  X(MONTH_SHORT_FEB, "fev") \
  X(MONTH_SHORT_MAR, "mar") \
  X(MONTH_SHORT_APR, "abr") \
  X(MONTH_SHORT_MAY, "mai") \
  X(MONTH_SHORT_JUN, "jun") \
  X(MONTH_SHORT_JUL, "jul") \
  X(MONTH_SHORT_AUG, "ago") \
  X(MONTH_SHORT_SEP, "set") \
  X(MONTH_SHORT_OCT, "out") \
  X(MONTH_SHORT_NOV, "nov") \
  X(MONTH_SHORT_DEC, "dez") \
  X(MONTH_TAG_FEB, "FEV") \
  X(MONTH_TAG_APR, "ABR") \
  X(MONTH_TAG_MAY, "MAI") \
  X(MONTH_TAG_AUG, "AGO") \
  X(MONTH_TAG_SEP, "SET") \
  X(MONTH_TAG_OCT, "OUT") \
  X(MONTH_TAG_DEC, "DEZ") \
  X(DX_NOW, "agora") \
  X(DX_FMT_MIN_AGO, "há %u min") \
  X(DX_FMT_H_AGO, "há %u h") \
  X(DX_FMT_D_AGO, "há %u d") \
  X(TOUCH_CAL_TITLE, "Calibrar toque") \
  X(TOUCH_CAL_HOLD, "Segure o círculo até encher") \
  X(TOUCH_CAL_CANCEL_HINT, "Gire ou pressione para cancelar") \
  X(TOUCH_CAL_CHECK, "Teste") \
  X(TOUCH_CAL_TAP_DOT, "Toque no ponto") \
  X(TOUCH_CAL_SAVED, "Calibração guardada") \
  X(TOUCH_CAL_FMT_LANDED, "Teste: desvio de %u px") \
  X(TOUCH_CAL_BACK_HINT, "Pressione o botão para voltar") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Não guardada") \
  X(TOUCH_CAL_FMT_MISSED, "Teste: desvio de %u px,") \
  X(TOUCH_CAL_NO_FIT, "As marcas não deram calibração,") \
  X(TOUCH_CAL_NOT_SAVED, "Não foi possível gravar,") \
  X(TOUCH_CAL_OLD_STAYS, "e fica a calibração antiga") \
  X(TOUCH_CAL_RETRY_HINT, "Pressione para repetir, gire para sair")
// clang-format on
