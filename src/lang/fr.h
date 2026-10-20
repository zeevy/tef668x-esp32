/*
 * The panel's texts in French. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_FR(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Scan stations FM") \
  X(MENU_SCAN_MW, "Scan stations AM") \
  X(MENU_SCAN_SW, "Scan stations SW") \
  X(MENU_SCAN_LW, "Scan stations LW") \
  X(MENU_SQUELCH, "Mode squelch") \
  X(MENU_SQUELCH_LEVEL, "Niveau squelch") \
  X(MENU_SQUELCH_FLOOR, "Squelch min.") \
  X(MENU_VOLUME_AGC, "AGC volume") \
  X(MENU_AGC_BOOST, "Boost AGC") \
  X(MENU_MUTE_RAMP, "Rampe sourdine") \
  X(MENU_BAND_PLAN, "Plan de bande") \
  X(MENU_TUNING_STEP, "Pas d'accord") \
  X(MENU_SEEK_SENSITIVITY, "Sensib. recherche") \
  X(MENU_DE_EMPHASIS, "Désaccentuation") \
  X(MENU_FORCED_MONO, "Forcer mono") \
  X(MENU_STEREO_BLEND, "Mélange stéréo") \
  X(MENU_ST_HI_BLEND, "Mélange st. / aigus") \
  X(MENU_STEREO, "Stéréo") \
  X(MENU_HIGH_CUT, "Coupe-haut") \
  X(MENU_NOISE_BLANKER, "Anti-parasites") \
  X(MENU_EQUALISER, "Égaliseur") \
  X(COMMON_RDS_DECODER, "Décodeur RDS") \
  X(MENU_RDS_REGION, "Région RDS") \
  X(MENU_MW_SPACING, "Pas MW") \
  X(MENU_FILTER_WIDTH, "Bande passante") \
  X(MENU_SOFT_MUTE, "Sourdine douce") \
  X(MENU_DWELL, "Pause scan") \
  X(MENU_STOP_ON, "Arrêt sur") \
  X(MENU_SCAN, "Plage scan") \
  X(MENU_MEMORY_FROM, "Mém. début") \
  X(MENU_MEMORY_TO, "Mém. fin") \
  X(MENU_PRESET_RANGE, "Plage mém.") \
  X(MENU_LOOP_THE_BAND, "En boucle") \
  X(MENU_DX_WIDTH, "Bande pass. scan") \
  X(MENU_MUTE_WHILE_SCANNING, "Sourdine en scan") \
  X(MENU_AUTO_LOG_NEW, "Journal auto") \
  X(MENU_LOG_RADIO_TEXT, "Noter radiotexte") \
  X(MENU_WATCH_PRESETS, "Surveiller mém.") \
  X(MENU_SCOPE_BASELINE, "Référence spectre") \
  X(MENU_MEDIAN, "Médiane") \
  X(MENU_FMT_SWEEP_N, "Balayage %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Apprendre locales") \
  X(MENU_START_SCAN, "Lancer scan") \
  X(MENU_DAY_THEME, "Thème jour") \
  X(MENU_NIGHT_THEME, "Thème nuit") \
  X(MENU_THEME, "Thème") \
  X(MENU_BRIGHTNESS, "Luminosité") \
  X(MENU_DIM_LEVEL, "Lum. réduite") \
  X(MENU_DIM_AFTER, "Réduire après") \
  X(MENU_ROTATION, "Rotation écran") \
  X(COMMON_BATTERY, "Batterie") \
  X(MENU_BATTERY_CHECK, "Suivi batterie") \
  X(MENU_LANGUAGE, "Langue \xC2\xB7 Language") \
  X(MENU_ENCODER, "Type encodeur") \
  X(MENU_DIRECTION, "Sens encodeur") \
  X(MENU_ENCODER_GROUP, "Encodeur") \
  X(MENU_KEY_BEEPS, "Bips touches") \
  X(MENU_BAND_EDGE_BEEP, "Bip fin de bande") \
  X(MENU_START_CHIME, "Son démarrage") \
  X(MENU_KEYPAD_TIMEOUT, "Délai clavier") \
  X(MENU_TOUCH, "Tactile") \
  X(MENU_CALIBRATE_TOUCH, "Calibrer tactile") \
  X(MENU_CLOCK_FROM_NETWORK, "Heure réseau") \
  X(MENU_WEB_PIN, "PIN web") \
  X(MENU_NEW_PIN, "Nouveau PIN") \
  X(MENU_FMT_DIGIT_OF, "Chiffre %u/%u") \
  X(MENU_STATUS, "État connexion") \
  X(MENU_WEB_SERVER, "Serveur web") \
  X(MENU_FMT_PC_CONNECTED, "Connectés: %u") \
  X(MENU_WEB_ADDRESS, "Adresse web") \
  X(MENU_IP_ADDRESS, "Adresse IP") \
  X(MENU_WI_FI_NAME, "Réseau Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Nom du hotspot") \
  X(MENU_MAC, "Adresse MAC") \
  X(MENU_WI_FI_SIGNAL, "Signal Wi-Fi") \
  X(MENU_NETWORK_INFO, "Infos réseau") \
  X(MENU_RESTART, "Redémarrer") \
  X(MENU_AUTO_OFF, "Arrêt auto") \
  X(MENU_UPDATE_CHECK, "Vérifier MAJ") \
  X(MENU_FIRMWARE_UPDATE, "MAJ firmware") \
  X(MENU_FMT_UPDATE_TO, "MAJ vers %s") \
  X(MENU_UPDATE_TITLE, "MAJ disponible") \
  X(MENU_UPDATE_THIS_RADIO, "Cette radio") \
  X(MENU_UPDATE_NEW_VERSION, "Nouvelle version") \
  X(MENU_UPDATE_DOWNLOAD, "Taille") \
  X(COMMON_SETTINGS, "Réglages") \
  X(COMMON_KEPT, "Gardés") \
  X(MENU_UPDATE_NOW, "Installer") \
  X(MENU_LATER, "Plus tard") \
  X(MENU_FMT_MEGABYTES, "%s Mo") \
  X(MENU_UPDATE_UP_TO_DATE, "À jour") \
  X(MENU_UPDATE_CHECKING, "En cours") \
  X(MENU_UPDATE_NOT_CHECKED, "Non vérifié") \
  X(MENU_UPDATE_CHECK_FAILED, "Échec vérif.") \
  X(MENU_NOTE_NO_UPDATE, "Aucune MAJ") \
  X(MENU_VERSION, "Version firmware") \
  X(MENU_RUNNING_FROM, "Source boot") \
  X(MENU_UPTIME, "En marche") \
  X(MENU_BATTERY_VOLTAGE, "Tension batterie") \
  X(MENU_RESET_REASON, "Cause reset") \
  X(MENU_CPU_CORE_0, "Cœur CPU 0") \
  X(MENU_CPU_CORE_1, "Cœur CPU 1") \
  X(MENU_FREE_HEAP, "Tas libre") \
  X(MENU_LOWEST_HEAP, "Tas min") \
  X(MENU_LARGEST_BLOCK, "Bloc max") \
  X(MENU_LVGL_POOL, "Pool LVGL") \
  X(MENU_CHIP, "Puce") \
  X(MENU_FLASH_SIZE, "Taille flash") \
  X(RESET_POWER, "Allumage") \
  X(RESET_SOFTWARE, "Logiciel") \
  X(RESET_PANIC, "Panique") \
  X(RESET_INT_WATCHDOG, "Watchdog IRQ") \
  X(RESET_TASK_WATCHDOG, "Watchdog tâche") \
  X(RESET_DEEP_SLEEP, "Veille profonde") \
  X(RESET_BROWNOUT, "Sous-tension") \
  X(RESET_PIN, "Broche reset") \
  X(RESET_UNKNOWN, "Inconnue") \
  X(RESET_ASKED, "Sur demande") \
  X(RESET_BOOT_WATCHDOG, "Watchdog boot") \
  X(RESET_ROLLBACK, "Restauration") \
  X(RESET_DISPLAY, "Défaut écran") \
  X(RESET_UPDATE, "MAJ") \
  X(MENU_DEVELOPER, "Développeur") \
  X(MENU_LICENSE, "Licence") \
  X(ABOUT_BUILD_UNKNOWN, "inconnu") \
  X(MENU_GO_TO, "Aller à") \
  X(MENU_GO_TO_BANDWIDTH, "Bande passante") \
  X(MENU_GO_TO_DX, "Mode DX") \
  X(MENU_GO_TO_SCOPE, "Spectre") \
  X(MENU_GO_TO_BAND, "Bande suivante") \
  X(MENU_GO_TO_LOG, "Noter la station") \
  X(MENU_GO_TO_SAVE, "Mémoriser la station") \
  X(MENU_FMT_SAVED_AS, "Mém. en P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Déjà en P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Mémoires pleines") \
  X(MENU_GO_TO_SLEEP, "Veille") \
  X(MENU_MEMORY, "Mémoires") \
  X(MENU_FM_SETUP, "Réception FM") \
  X(MENU_AM_SETUP, "Réception AM") \
  X(MENU_DX_SETUP, "Scanner DX") \
  X(MENU_DISPLAY, "Affichage") \
  X(MENU_CONTROLS, "Commandes") \
  X(MENU_CONNECTIVITY, "Connectivité") \
  X(MENU_SYSTEM, "Système") \
  X(MENU_DIAGNOSTICS, "Diagnostic") \
  X(MENU_ABOUT, "À propos") \
  X(MENU_REGION_FULL, "Complet") \
  X(MENU_REGION_JAPAN, "Japon") \
  X(MENU_REGION_WIDE, "Large") \
  X(MENU_REGION_WORLD, "Monde") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Amérique du Nord") \
  X(MENU_WIFI_JOINED, "Connecté") \
  X(MENU_WIFI_JOINING, "Connexion") \
  X(COMMON_OFF, "Arrêt") \
  X(MENU_FMT_KB, "%u Ko") \
  X(MENU_FMT_KB_OF_KB, "%u/%u Ko") \
  X(MENU_FMT_MB, "%u Mo") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V au départ") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, il y a %u min") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, il y a %u h") \
  X(MENU_FMT_AGC_GAIN_NOW, "Gain actuel %+d dB") \
  X(MENU_STATION_LOG, "Journal") \
  X(COMMON_NONE, "Aucun") \
  X(MENU_FMT_SCAN_SAVED, "%u en mém.") \
  X(MENU_FMT_SCAN_STOPPED, "Arrêté, %u en mém.") \
  X(MENU_FMT_SCAN_NO_ROOM, "Plein, %u en trop") \
  X(COMMON_YES, "Oui") \
  X(COMMON_NO, "Non") \
  X(MENU_UNKNOWN_VALUE, "Inconnu") \
  X(COMMON_ROTATION_NORMAL, "Normale") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Inversée") \
  X(MENU_CUT_ONLY, "Baisse seule") \
  X(MENU_NEVER, "Jamais") \
  X(MENU_BATTERY_PER_CENT, "Pour cent") \
  X(MENU_KEYS, "Clavier seul") \
  X(MENU_KEYS_LONG, "Appui court/long") \
  X(MENU_EVERY_PRESS, "Chaque appui") \
  X(MENU_OPTICAL, "Optique") \
  X(MENU_REVERSED, "Inversé") \
  X(MENU_SQUELCH_MAN, "Manuel") \
  X(MENU_ANY_PI, "Tout PI") \
  X(MENU_STOP_NEVER, "Jamais") \
  X(MENU_NEW_ONLY, "Nouvelles seules") \
  X(MENU_WHOLE_BAND, "Bande entière") \
  X(MENU_MEMORY_ONLY, "Mém. seules") \
  X(MENU_BAND_MEMORY, "Bande + mém.") \
  X(COMMON_ON, "Marche") \
  X(MENU_HINT_RESTART, "Effet au redémarrage") \
  X(COMMON_FM_ONLY, "FM seule") \
  X(MENU_NOTE_OFF_PLAN, "Hors plan de bande") \
  X(MENU_NOTE_SWITCH_TO_FM, "Passez en FM") \
  X(MENU_NOTE_RADIO_BUSY, "Occupé - réessayez") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Activez le RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "MAJ en test - attendez") \
  X(MENU_NOTE_NOT_STORED, "Non sauvé - rétabli") \
  X(MENU_NOTE_NOT_PUT_BACK, "Occupé - non rétabli") \
  X(MENU_NOTE_NOT_SAVED, "Non sauvé") \
  X(MENU_SAVE, "Valider") \
  X(RADIO_PRESS_TO_START, "Lancez le scan") \
  X(COMMON_STILL_TUNING, "Accord en cours") \
  X(COMMON_SCAN_RUNNING, "Scan en cours") \
  X(RADIO_FMT_NO_BAND, "%s hors bande") \
  X(COMMON_NOT_LOGGED, "Non noté") \
  X(RADIO_FMT_LOGGED, "%s noté") \
  X(RADIO_CHECKING_UPDATES, "VÉRIFICATION DES MAJ\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Scan %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Échec MAJ") \
  X(RADIO_GOING_TO_SLEEP, "Mise en veille") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Réveil: appui molette") \
  X(RADIO_OLD_IMAGE_KEPT, "Ancien firmware gardé") \
  X(RDS_STEREO, "Stéréo") \
  X(RDS_SPEECH, "Parole") \
  X(RDS_MUSIC, "Musique") \
  X(COMMON_ALREADY_LOGGED, "Déjà noté") \
  X(DX_RDS_IS_OFF, "RDS arrêté") \
  X(DX_SCANNING, "Scan lancé") \
  X(DX_SCAN_FINISHED, "Scan terminé") \
  X(DX_NOTHING_TO_SCAN, "Rien à scanner") \
  X(DX_CHECKING_FOR_UPDATES, "Vérification des MAJ") \
  X(DX_A_SCAN_IS_RUNNING, "Scan déjà en cours") \
  X(DX_CAUGHT_LIST_UNREAD, "Captures illisibles") \
  X(DX_NO_MEMORY, "Pas de mém.") \
  X(DX_NOT_STARTED, "Non lancé") \
  X(DX_SWEEPING, "Balayage") \
  X(COMMON_FMT_TUNE_TYPED, "Accorder %s") \
  X(RADIO_TUNE_MEMORY, "MÉM") \
  X(DX_FMT_ROWS_OF, "%u-%u/%u") \
  X(DX_FMT_FOUND, "%u trouv.") \
  X(DX_MODE_SWEEPING, "BALAYAGE") \
  X(DX_MODE_LEARN_LOCALS, "APPR. LOCALES") \
  X(DX_FMT_MODE_MEMORY, "MÉM. %u-%u") \
  X(DX_MODE_WHOLE_BAND, "BANDE ENTIÈRE") \
  X(DX_MODE_BAND_MEMORY, "BANDE + MÉM.") \
  X(DX_RULE_NO_STOP, "NON-STOP") \
  X(DX_RULE_STOP_ON_PI, "ARRÊT SI PI") \
  X(DX_RULE_STOP_ON_NEW, "ARRÊT SI NOUV.") \
  X(DX_FMT_NOT_PRESET, "pas P%02d") \
  X(DX_CONTEXT_SWEEPING, "balayage") \
  X(DX_PRESS_TO_SWEEP, "Appuyez pour balayer") \
  X(SCOPE_FMT_TITLE, "Spectre %s") \
  X(SCOPE_FULL, "Tout") \
  X(DX_FIXED, "FIXE") \
  X(DX_FMT_MEDIAN_OF, "MÉDIANE DE %u") \
  X(DX_FMT_FLOOR, "FOND %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Fermer") \
  X(RECOVERY_PRESS_AGAIN, "Appuyez encore") \
  X(PAGE_TUNE, "Accorder") \
  X(PAGE_DELETE, "Effacer") \
  X(PAGE_LEVEL, "Niveau") \
  X(PAGE_SNR, "S/B") \
  X(PAGE_NOISE, "Bruit") \
  X(PAGE_MULTIPATH, "Multitrajet") \
  X(PAGE_COCHANNEL, "Co-canal") \
  X(PAGE_STEREO, "Stéréo") \
  X(PAGE_WIDTH, "Largeur") \
  X(PAGE_BAND, "Bande") \
  X(PAGE_NOTE_CHANGED, "Modifié depuis l'ouverture") \
  X(PAGE_NOTE_NOT_DELETED, "Non effacé") \
  X(PAGE_FMT_SAVED, "Mémorisée %s") \
  X(PAGE_TIME_UNKNOWN, "Heure inconnue") \
  X(BW_TITLE, "Bande passante") \
  X(KEYPAD_TITLE, "Fréquence") \
  X(KEYPAD_CANCEL, "Annuler") \
  X(DX_CATCHES_EMPTY_TITLE, "Aucune capture") \
  X(DX_CATCHES_EMPTY_NOTE, "Les stations confirmées de la session s'affichent ici") \
  X(DX_TITLE_CATCHES, "Captures") \
  X(DX_NEW, "NOUV") \
  X(DX_READY, "Prêt") \
  X(DX_DWELL, "PAUSE") \
  X(DX_STOPPED, "ARRÊTÉ") \
  X(DX_TITLE_SCOPE, "Spectre") \
  X(DX_LEVEL, "NIV.") \
  X(DX_RISE, "DIFF") \
  X(DX_NO_STATION_YET, "Aucune station") \
  X(DX_NO_RDS_YET, "Pas de RDS") \
  X(DX_OFFSET, "ÉCART") \
  X(COMMON_BW, "BP") \
  X(DX_NO_ID, "SANS ID") \
  X(RDS_TITLE_TEXT, "Radiotexte") \
  X(RDS_TITLE_NETWORKS, "Réseaux") \
  X(RDS_TITLE_DECODER, "Décodeur") \
  X(RDS_NO_COUNTRY, "ECC non reçu") \
  X(RDS_MS_UNKNOWN, "M/P") \
  X(RDS_RTPLUS_NONE, "Aucun RT+ de cette station") \
  X(RDS_RTPLUS_WAIT, "Aucun tag RT+ pour ce texte") \
  X(RDS_AF_TITLE, "FRÉQUENCES ALTERNATIVES") \
  X(RDS_EON_TITLE, "AUTRES RÉSEAUX, EON") \
  X(RDS_NONE_HEARD, "Rien reçu") \
  X(RDS_GROUPS_S_CAPS, "GRP/S") \
  X(RDS_LOST, "PERDUS") \
  X(RDS_EACH_BLOCK, "PAR BLOC") \
  X(RDS_LEGEND_CLEAN, "bons") \
  X(RDS_LEGEND_FIXED, "corrigés") \
  X(RDS_LEGEND_LOST, "perdus") \
  X(RDS_ALL_CLEAN, "tous bons") \
  X(RDS_GROUP_TYPES, "TYPES DE GROUPES") \
  X(RDS_NO_GROUPS, "Aucun groupe depuis 1 min") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 %u s") \
  X(RDS_COUNTRY_UNLISTED, "hors liste") \
  X(RDS_NO_TYPES, "Aucun bloc B bon depuis 1 min") \
  X(RDS_FMT_EON_MORE, "+%u de plus") \
  X(RDS_EON_MORE_HEARD, "+ autres") \
  X(RDS_MORE, "etc.") \
  X(RADIO_VEIL_UPDATING, "MAJ DU FIRMWARE") \
  X(RADIO_VEIL_KEEP_POWER, "N'éteignez pas") \
  X(RADIO_MUTE, "MUET") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Tuner absent du bus I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Le tuner n'est jamais prêt") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Tuner non identifié") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Aucun patch pour ce tuner") \
  X(TUNER_UNKNOWN_TUNER_PART, "Tuner inconnu") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Écriture refusée") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Pas de réponse en lecture") \
  X(TUNER_OUT_OF_RANGE, "Hors limites") \
  X(TUNER_UNKNOWN, "Inconnu") \
  X(BAND_LONG_LW, "Grandes ondes") \
  X(BAND_LONG_MW, "Ondes moyennes") \
  X(BAND_LONG_SW, "Ondes courtes") \
  X(MENU_FMT_TUNE_TO, "Bande pour %s") \
  X(PTY_NONE, "Aucun") \
  X(PTY_NEWS, "Infos") \
  X(PTY_CURRENT_AFFAIRS, "Actualités") \
  X(PTY_EDUCATION, "Éducation") \
  X(PTY_DRAMA, "Fiction") \
  X(PTY_VARIED, "Divers") \
  X(PTY_POP_MUSIC, "Pop") \
  X(PTY_ROCK_MUSIC, "Rock") \
  X(PTY_EASY_LISTENING, "Variétés") \
  X(PTY_LIGHT_CLASSICAL, "Classique léger") \
  X(PTY_SERIOUS_CLASSICAL, "Classique") \
  X(PTY_OTHER_MUSIC, "Autre musique") \
  X(PTY_WEATHER, "Météo") \
  X(PTY_CHILDREN, "Enfants") \
  X(PTY_SOCIAL_AFFAIRS, "Société") \
  X(PTY_PHONE_IN, "Ligne ouverte") \
  X(PTY_TRAVEL, "Voyage") \
  X(PTY_LEISURE, "Loisirs") \
  X(PTY_COUNTRY_MUSIC, "Country") \
  X(PTY_NATIONAL_MUSIC, "Musique nationale") \
  X(PTY_OLDIES_MUSIC, "Rétro") \
  X(PTY_FOLK_MUSIC, "Folklore") \
  X(PTY_DOCUMENTARY, "Documentaire") \
  X(PTY_ALARM_TEST, "Test alarme") \
  X(PTY_ALARM, "Alarme") \
  X(PTY_NA_NEWS, "Infos") \
  X(PTY_NA_TALK, "Parlé") \
  X(PTY_NA_CLASSIC_ROCK, "Rock classique") \
  X(PTY_NA_ADULT_HITS, "Hits adultes") \
  X(PTY_NA_SOFT_ROCK, "Rock doux") \
  X(PTY_NA_OLDIES, "Rétro") \
  X(PTY_NA_SOFT, "Doux") \
  X(PTY_NA_NOSTALGIA, "Nostalgie") \
  X(PTY_NA_CLASSICAL, "Classique") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Rhythm and blues") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "R&B doux") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Langue étrangère") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Musique religieuse") \
  X(PTY_NA_RELIGIOUS_TALK, "Religieux parlé") \
  X(PTY_NA_PERSONALITY, "Personnalité") \
  X(PTY_NA_COLLEGE, "Campus") \
  X(PTY_NA_SPANISH_TALK, "Espagnol parlé") \
  X(PTY_NA_SPANISH_MUSIC, "Musique espagnole") \
  X(PTY_NA_HIP_HOP, "Hip-hop") \
  X(PTY_NA_UNASSIGNED, "Non attribué") \
  X(PTY_NA_WEATHER, "Météo") \
  X(PTY_NA_EMERGENCY_TEST, "Test d'urgence") \
  X(PTY_NA_EMERGENCY, "Urgence") \
  X(COVERAGE_SUPRA_REGIONAL, "Suprarégional") \
  X(COVERAGE_REGIONAL_1, "Régional 1") \
  X(COVERAGE_REGIONAL_2, "Régional 2") \
  X(COVERAGE_REGIONAL_3, "Régional 3") \
  X(COVERAGE_REGIONAL_4, "Régional 4") \
  X(COVERAGE_REGIONAL_5, "Régional 5") \
  X(COVERAGE_REGIONAL_6, "Régional 6") \
  X(COVERAGE_REGIONAL_7, "Régional 7") \
  X(COVERAGE_REGIONAL_8, "Régional 8") \
  X(COVERAGE_REGIONAL_9, "Régional 9") \
  X(COVERAGE_REGIONAL_10, "Régional 10") \
  X(COVERAGE_REGIONAL_11, "Régional 11") \
  X(COVERAGE_REGIONAL_12, "Régional 12") \
  X(RTP_OTHER, "AUTRE") \
  X(RTP_TITLE, "TITRE") \
  X(RTP_TRACK, "PISTE") \
  X(RTP_ARTIST, "ARTISTE") \
  X(RTP_WORK, "ŒUVRE") \
  X(RTP_MOVEMENT, "MOUVEMENT") \
  X(RTP_CONDUCTOR, "DIRECTION") \
  X(RTP_COMPOSER, "COMPOSITEUR") \
  X(RTP_BAND, "GROUPE") \
  X(RTP_COMMENT, "COMMENTAIRE") \
  X(RTP_NEWS, "INFOS") \
  X(RTP_LOCAL, "INFOS LOCALES") \
  X(RTP_STOCKS, "BOURSE") \
  X(RTP_LOTTERY, "LOTERIE") \
  X(RTP_DIVERSION, "DÉTENTE") \
  X(RTP_HEALTH, "SANTÉ") \
  X(RTP_EVENT, "ÉVÉNEMENT") \
  X(RTP_SCENE, "SCÈNE") \
  X(RTP_CINEMA, "CINÉMA") \
  X(RTP_FUN, "HUMOUR") \
  X(RTP_WEATHER, "MÉTÉO") \
  X(RTP_TRAFFIC, "TRAFIC") \
  X(RTP_ALARM, "ALARME") \
  X(RTP_ADVERT, "PUB") \
  X(RTP_PROGRAM, "ÉMISSION") \
  X(RTP_NEXT, "À SUIVRE") \
  X(RTP_PART, "PARTIE") \
  X(RTP_HOST, "ANIMATEUR") \
  X(RTP_EDITORS, "RÉDACTION") \
  X(RTP_FREQUENCY, "FRÉQUENCE") \
  X(RTP_HOMEPAGE, "SITE WEB") \
  X(RTP_CHANNEL, "CANAL") \
  X(RTP_PHONE, "TÉL.") \
  X(RTP_PLACE, "LIEU") \
  X(RTP_APPOINTMENT, "RENDEZ-VOUS") \
  X(RTP_PURCHASE, "ACHAT") \
  X(DAY_SUNDAY, "DIMANCHE") \
  X(DAY_MONDAY, "LUNDI") \
  X(DAY_TUESDAY, "MARDI") \
  X(DAY_WEDNESDAY, "MERCREDI") \
  X(DAY_THURSDAY, "JEUDI") \
  X(DAY_FRIDAY, "VENDREDI") \
  X(DAY_SATURDAY, "SAMEDI") \
  X(MONTH_JANUARY, "janvier") \
  X(MONTH_FEBRUARY, "février") \
  X(MONTH_MARCH, "mars") \
  X(MONTH_APRIL, "avril") \
  X(MONTH_MAY, "mai") \
  X(MONTH_JUNE, "juin") \
  X(MONTH_JULY, "juillet") \
  X(MONTH_AUGUST, "août") \
  X(MONTH_SEPTEMBER, "septembre") \
  X(MONTH_OCTOBER, "octobre") \
  X(MONTH_NOVEMBER, "novembre") \
  X(MONTH_DECEMBER, "décembre") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_FMT_LINE, "%s %d%s %s %d") \
  X(DATE_TODAY, "Aujourd'hui") \
  X(DATE_YESTERDAY, "Hier") \
  X(DATE_YDAY, "HIER") \
  X(MONTH_SHORT_JAN, "jan") \
  X(MONTH_SHORT_FEB, "fév") \
  X(MONTH_SHORT_MAR, "mars") \
  X(MONTH_SHORT_APR, "avr") \
  X(MONTH_SHORT_MAY, "mai") \
  X(MONTH_SHORT_JUN, "juin") \
  X(MONTH_SHORT_JUL, "juil") \
  X(MONTH_SHORT_AUG, "août") \
  X(MONTH_SHORT_SEP, "sep") \
  X(MONTH_SHORT_OCT, "oct") \
  X(MONTH_SHORT_NOV, "nov") \
  X(MONTH_SHORT_DEC, "déc") \
  X(MONTH_TAG_FEB, "FÉV") \
  X(MONTH_TAG_APR, "AVR") \
  X(MONTH_TAG_MAY, "MAI") \
  X(MONTH_TAG_JUN, "JUIN") \
  X(MONTH_TAG_JUL, "JUIL") \
  X(MONTH_TAG_AUG, "AOÛT") \
  X(MONTH_TAG_DEC, "DÉC") \
  X(DX_NOW, "à l'instant") \
  X(DX_FMT_MIN_AGO, "il y a %u min") \
  X(DX_FMT_H_AGO, "il y a %u h") \
  X(DX_FMT_D_AGO, "il y a %u j") \
  X(TOUCH_CAL_TITLE, "Calibrage tactile") \
  X(TOUCH_CAL_HOLD, "Restez sur le cercle jusqu'au plein") \
  X(TOUCH_CAL_CANCEL_HINT, "Tournez ou appuyez pour annuler") \
  X(TOUCH_CAL_CHECK, "Test") \
  X(TOUCH_CAL_TAP_DOT, "Touchez le point") \
  X(TOUCH_CAL_SAVED, "Calibrage sauvé") \
  X(TOUCH_CAL_FMT_LANDED, "Écart du test: %u px") \
  X(TOUCH_CAL_BACK_HINT, "Appuyez pour revenir") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Non sauvé") \
  X(TOUCH_CAL_FMT_MISSED, "Écart du test: %u px,") \
  X(TOUCH_CAL_NO_FIT, "Repères insuffisants pour calibrer,") \
  X(TOUCH_CAL_NOT_SAVED, "Écriture impossible,") \
  X(TOUCH_CAL_OLD_STAYS, "l'ancien calibrage reste") \
  X(TOUCH_CAL_RETRY_HINT, "Appuyez: réessayer. Tournez: quitter")
// clang-format on
