/*
 * The panel's texts in Greek. The ids of en.h, in its order. A text that
 * is the same as the English one is left out, and the English one shows.
 */
#pragma once

// clang-format off
#define STRINGS_EL(X) \
  X(MENU_SCAN_FM_FOR_STATIONS, "Σάρωση FM") \
  X(MENU_SCAN_MW, "Σάρωση AM") \
  X(MENU_SCAN_SW, "Σάρωση SW") \
  X(MENU_SCAN_LW, "Σάρωση LW") \
  X(MENU_SQUELCH, "Τρόπος φίμωσης") \
  X(MENU_SQUELCH_LEVEL, "Στάθμη φίμωσης") \
  X(MENU_SQUELCH_FLOOR, "Κάτω όριο φίμωσης") \
  X(MENU_SQUELCH_GROUP, "Φίμωση") \
  X(MENU_VOLUME_AGC, "AGC έντασης") \
  X(MENU_AGC_BOOST, "Ενίσχυση AGC") \
  X(MENU_MUTE_RAMP, "Ράμπα σίγασης") \
  X(MENU_BAND_PLAN, "Μπάντα FM") \
  X(MENU_TUNING_STEP, "Βήμα συντονισμού") \
  X(MENU_SEEK_SENSITIVITY, "Ευαισθ. αναζήτησης") \
  X(MENU_DE_EMPHASIS, "Αποέμφαση") \
  X(MENU_FORCED_MONO, "Μονοφωνία") \
  X(MENU_STEREO_BLEND, "Μίξη στέρεο") \
  X(MENU_ST_HI_BLEND, "Μίξη στέρεο/υψηλών") \
  X(MENU_STEREO, "Στέρεο") \
  X(MENU_HIGH_CUT, "Αποκοπή υψηλών") \
  X(MENU_NOISE_BLANKER, "Απαλοιφή θορύβου") \
  X(MENU_EQUALISER, "Ισοσταθμιστής") \
  X(COMMON_RDS_DECODER, "Αποκωδικοποιητής RDS") \
  X(MENU_RDS_REGION, "Περιοχή RDS") \
  X(MENU_MW_SPACING, "Βήμα MW") \
  X(MENU_FILTER_WIDTH, "Εύρος φίλτρου") \
  X(MENU_SOFT_MUTE, "Ήπια σίγαση") \
  X(MENU_DWELL, "Χρόνος παύσης") \
  X(MENU_STOP_ON, "Στάση") \
  X(MENU_SCAN, "Περιοχή σάρωσης") \
  X(MENU_MEMORY_FROM, "Πρώτη μνήμη") \
  X(MENU_MEMORY_TO, "Τελευταία μνήμη") \
  X(MENU_PRESET_RANGE, "Περιοχή μνημών") \
  X(MENU_LOOP_THE_BAND, "Κυκλική σάρωση") \
  X(MENU_DX_WIDTH, "Φίλτρο σάρωσης") \
  X(MENU_MUTE_WHILE_SCANNING, "Σίγαση στη σάρωση") \
  X(MENU_AUTO_LOG_NEW, "Αυτόματη καταγραφή") \
  X(MENU_LOG_RADIO_TEXT, "Καταγραφή RT") \
  X(MENU_WATCH_PRESETS, "Επιτήρηση μνημών") \
  X(MENU_SCOPE_BASELINE, "Βάση φάσματος") \
  X(MENU_MEDIAN, "Διάμεσος") \
  X(MENU_FMT_SWEEP_N, "Μέτρηση %u") \
  X(DX_FMT_WATCH_UP, "%s +%d dB") \
  X(MENU_LEARN_LOCALS, "Μάθηση τοπικών") \
  X(MENU_START_SCAN, "Έναρξη σάρωσης") \
  X(MENU_DAY_THEME, "Θέμα ημέρας") \
  X(MENU_NIGHT_THEME, "Θέμα νύχτας") \
  X(MENU_THEME, "Θέμα") \
  X(MENU_BRIGHTNESS, "Φωτεινότητα") \
  X(MENU_DIM_LEVEL, "Χαμήλωμα σε") \
  X(MENU_DIM_AFTER, "Χαμήλωμα μετά") \
  X(MENU_ROTATION, "Περιστροφή οθόνης") \
  X(COMMON_BATTERY, "Μπαταρία") \
  X(MENU_BATTERY_CHECK, "Έλεγχος μπαταρίας") \
  X(MENU_LANGUAGE, "Γλώσσα \xC2\xB7 Language") \
  X(MENU_ENCODER, "Τύπος κωδικοποιητή") \
  X(MENU_DIRECTION, "Φορά κωδικοποιητή") \
  X(MENU_ENCODER_GROUP, "Κωδικοποιητής") \
  X(MENU_KEY_BEEPS, "Ήχος πλήκτρων") \
  X(MENU_BAND_EDGE_BEEP, "Ήχος ορίου μπάντας") \
  X(MENU_START_CHIME, "Ήχος εκκίνησης") \
  X(MENU_KEYPAD_TIMEOUT, "Αναμονή πλήκτρων") \
  X(MENU_TOUCH, "Αφή") \
  X(MENU_CALIBRATE_TOUCH, "Βαθμονόμηση αφής") \
  X(MENU_CLOCK_FROM_NETWORK, "Ώρα δικτύου") \
  X(MENU_WEB_PIN, "PIN web") \
  X(MENU_NEW_PIN, "Νέο PIN") \
  X(MENU_FMT_DIGIT_OF, "Ψηφίο %u από %u") \
  X(MENU_STATUS, "Κατάσταση") \
  X(MENU_WEB_SERVER, "Διακομιστής web") \
  X(MENU_FMT_PC_CONNECTED, "Συνδεδεμένοι: %u") \
  X(MENU_WEB_ADDRESS, "Διεύθυνση web") \
  X(MENU_IP_ADDRESS, "Διεύθυνση IP") \
  X(MENU_WI_FI_NAME, "Δίκτυο Wi-Fi") \
  X(MENU_HOTSPOT_NAME, "Όνομα hotspot") \
  X(MENU_MAC, "Διεύθυνση MAC") \
  X(MENU_WI_FI_SIGNAL, "Σήμα Wi-Fi") \
  X(MENU_NETWORK_INFO, "Στοιχεία δικτύου") \
  X(MENU_RESTART, "Επανεκκίνηση") \
  X(MENU_AUTO_OFF, "Σβήσιμο μετά") \
  X(MENU_UPDATE_CHECK, "Έλεγχος ενημερώσεων") \
  X(MENU_FIRMWARE_UPDATE, "Ενημέρωση") \
  X(MENU_FMT_UPDATE_TO, "Ενημέρωση σε %s") \
  X(MENU_UPDATE_TITLE, "Υπάρχει ενημέρωση") \
  X(MENU_UPDATE_THIS_RADIO, "Τρέχουσα") \
  X(MENU_UPDATE_NEW_VERSION, "Νέα έκδοση") \
  X(MENU_UPDATE_DOWNLOAD, "Μέγεθος") \
  X(COMMON_SETTINGS, "Ρυθμίσεις") \
  X(COMMON_KEPT, "Διατηρούνται") \
  X(MENU_FMT_QUESTION, "%s;") \
  X(MENU_STATION, "Σταθμός") \
  X(MENU_UPDATE_NOW, "Ενημέρωση") \
  X(MENU_LATER, "Αργότερα") \
  X(MENU_UPDATE_UP_TO_DATE, "Ενημερωμένο") \
  X(MENU_UPDATE_CHECKING, "Έλεγχος") \
  X(MENU_UPDATE_NOT_CHECKED, "Δεν ελέγχθηκε") \
  X(MENU_UPDATE_CHECK_FAILED, "Σφάλμα ελέγχου") \
  X(MENU_NOTE_NO_UPDATE, "Καμία ενημέρωση") \
  X(MENU_VERSION, "Έκδοση λογισμικού") \
  X(MENU_RUNNING_FROM, "Πηγή εκκίνησης") \
  X(MENU_UPTIME, "Σε λειτουργία") \
  X(MENU_BATTERY_VOLTAGE, "Τάση μπαταρίας") \
  X(COMMON_TUNER, "Συντονιστής") \
  X(MENU_RESET_REASON, "Αιτία εκκίνησης") \
  X(MENU_CPU_CORE_0, "Πυρήνας CPU 0") \
  X(MENU_CPU_CORE_1, "Πυρήνας CPU 1") \
  X(MENU_FREE_HEAP, "Ελεύθερος σωρός") \
  X(MENU_LOWEST_HEAP, "Ελάχιστος σωρός") \
  X(MENU_LARGEST_BLOCK, "Μέγιστο μπλοκ") \
  X(MENU_LVGL_POOL, "Χώρος LVGL") \
  X(MENU_CHIP, "Τσιπ") \
  X(MENU_FLASH_SIZE, "Μέγεθος flash") \
  X(RESET_POWER, "Άναμμα") \
  X(RESET_SOFTWARE, "Λογισμικό") \
  X(RESET_PANIC, "Πανικός") \
  X(RESET_TASK_WATCHDOG, "Watchdog εργασίας") \
  X(RESET_DEEP_SLEEP, "Βαθύς ύπνος") \
  X(RESET_BROWNOUT, "Πτώση τάσης") \
  X(RESET_PIN, "Πλήκτρο reset") \
  X(RESET_UNKNOWN, "Άγνωστη") \
  X(RESET_ASKED, "Ζητήθηκε") \
  X(RESET_BOOT_WATCHDOG, "Watchdog εκκίνησης") \
  X(RESET_ROLLBACK, "Επαναφορά") \
  X(RESET_DISPLAY, "Σφάλμα οθόνης") \
  X(RESET_UPDATE, "Ενημέρωση") \
  X(MENU_DEVELOPER, "Δημιουργός") \
  X(MENU_LICENSE, "Άδεια") \
  X(ABOUT_BUILD_UNKNOWN, "άγνωστο") \
  X(MENU_GO_TO, "Μετάβαση") \
  X(MENU_GO_TO_BANDWIDTH, "Εύρος ζώνης") \
  X(MENU_GO_TO_DX, "Λειτουργία DX") \
  X(MENU_GO_TO_SCOPE, "Φάσμα μπάντας") \
  X(MENU_GO_TO_BAND, "Επόμενη μπάντα") \
  X(MENU_GO_TO_LOG, "Καταγραφή σταθμού") \
  X(MENU_GO_TO_SAVE, "Αποθήκευση σταθμού") \
  X(MENU_FMT_SAVED_AS, "Στη μνήμη P%02d") \
  X(MENU_FMT_ALREADY_PRESET, "Ήδη P%02d") \
  X(MENU_NOTE_PRESETS_FULL, "Οι μνήμες γέμισαν") \
  X(MENU_GO_TO_SLEEP, "Ύπνος") \
  X(MENU_STATIONS, "Σταθμοί") \
  X(MENU_MEMORY, "Μνήμες") \
  X(MENU_AUDIO, "Ήχος") \
  X(MENU_FM_SETUP, "Λήψη FM") \
  X(MENU_AM_SETUP, "Λήψη AM") \
  X(MENU_DX_SETUP, "Σαρωτής DX") \
  X(MENU_DISPLAY, "Οθόνη") \
  X(MENU_CONTROLS, "Χειριστήρια") \
  X(MENU_CONNECTIVITY, "Συνδεσιμότητα") \
  X(MENU_SYSTEM, "Σύστημα") \
  X(MENU_DIAGNOSTICS, "Διαγνωστικά") \
  X(MENU_ABOUT, "Πληροφορίες") \
  X(MENU_REGION_FULL, "Πλήρης") \
  X(MENU_REGION_JAPAN, "Ιαπωνία") \
  X(MENU_REGION_WIDE, "Ευρεία") \
  X(MENU_REGION_WORLD, "Παγκόσμια") \
  X(MENU_RDS_REGION_EUROPE, "Ευρώπη") \
  X(MENU_RDS_REGION_NORTH_AMERICA, "Β. Αμερική") \
  X(MENU_WIFI_JOINED, "Συνδέθηκε") \
  X(MENU_WIFI_JOINING, "Συνδέεται") \
  X(COMMON_OFF, "Όχι") \
  X(MENU_FMT_KB_OF_KB, "%u από %u KB") \
  X(MENU_FMT_VOLTS_AT_START, "%u.%02u V στο άναμμα") \
  X(MENU_FMT_VOLTS_AGO, "%u.%02u V, πριν %u λεπ.") \
  X(MENU_FMT_VOLTS_AGO_HOURS, "%u.%02u V, πριν %u ώρ.") \
  X(MENU_FMT_AGC_GAIN_NOW, "Κέρδος τώρα %+d dB") \
  X(MENU_STATION_LOG, "Ημερολόγιο") \
  X(COMMON_NONE, "Κανένα") \
  X(MENU_FMT_SCAN_PROGRESS, "Σάρωση %u/%u") \
  X(MENU_FMT_SCAN_SAVED, "Αποθηκεύτηκαν: %u") \
  X(MENU_FMT_SCAN_STOPPED, "Διακόπηκε, αποθ.: %u") \
  X(MENU_FMT_SCAN_NO_ROOM, "Γέμισαν, δεν αποθ.: %u") \
  X(COMMON_YES, "Ναι") \
  X(COMMON_NO, "Όχι") \
  X(MENU_UNKNOWN_VALUE, "Άγνωστο") \
  X(COMMON_ROTATION_NORMAL, "Κανονική") \
  X(COMMON_ROTATION_UPSIDE_DOWN, "Ανάποδα") \
  X(MENU_CUT_ONLY, "Μόνο μείωση") \
  X(MENU_NEVER, "Ποτέ") \
  X(MENU_BATTERY_PER_CENT, "Ποσοστό") \
  X(MENU_BATTERY_VOLTS, "Βολτ") \
  X(MENU_KEYS, "Μόνο ψηφία") \
  X(MENU_KEYS_LONG, "Σύντομο & μακρύ") \
  X(MENU_EVERY_PRESS, "Κάθε πάτημα") \
  X(MENU_STANDARD, "Τυπικός") \
  X(MENU_OPTICAL, "Οπτικός") \
  X(MENU_NORMAL, "Κανονική") \
  X(MENU_REVERSED, "Αντίστροφη") \
  X(MENU_AUTO, "Αυτόματο") \
  X(MENU_SQUELCH_MAN, "Χειροκίνητο") \
  X(MENU_ANY_PI, "Σε κάθε PI") \
  X(MENU_STOP_NEVER, "Ποτέ") \
  X(MENU_NEW_ONLY, "Μόνο σε νέους") \
  X(MENU_WHOLE_BAND, "Όλη η μπάντα") \
  X(MENU_MEMORY_ONLY, "Μόνο μνήμες") \
  X(MENU_BAND_MEMORY, "Μπάντα + μνήμες") \
  X(COMMON_ON, "Ναι") \
  X(MENU_HINT_RESTART, "Θέλει επανεκκίνηση") \
  X(MENU_TITLE, "Μενού") \
  X(COMMON_FM_ONLY, "Μόνο FM") \
  X(MENU_NOTE_OFF_PLAN, "Εκτός μπάντας") \
  X(MENU_NOTE_SWITCH_TO_FM, "Πρώτα επιλέξτε FM") \
  X(MENU_NOTE_RADIO_BUSY, "Απασχολημένο - δοκιμάστε ξανά") \
  X(MENU_NOTE_SWITCH_RDS_ON, "Πρώτα ανοίξτε το RDS") \
  X(MENU_NOTE_UPDATE_ON_TRIAL, "Υπό δοκιμή - περιμένετε") \
  X(MENU_NOTE_NOT_STORED, "Μη αποθηκευμένο - επαναφορά") \
  X(MENU_NOTE_NOT_PUT_BACK, "Απασχολημένο - χωρίς επαναφορά") \
  X(MENU_NOTE_NOT_SAVED, "Μη αποθηκευμένο") \
  X(MENU_SAVE, "Αποθήκευση") \
  X(RADIO_PRESS_TO_START, "Πατήστε για έναρξη") \
  X(COMMON_STILL_TUNING, "Συντονισμός - περιμένετε") \
  X(COMMON_SCAN_RUNNING, "Σάρωση - περιμένετε") \
  X(RADIO_FMT_NO_BAND, "%s εκτός μπαντών") \
  X(COMMON_NOT_LOGGED, "Δεν καταγράφηκε") \
  X(RADIO_FMT_LOGGED, "Καταγράφηκε %s") \
  X(RADIO_CHECKING_UPDATES, "ΕΛΕΓΧΟΣ ΕΝΗΜΕΡΩΣΕΩΝ\xE2\x80\xA6") \
  X(RADIO_FMT_SCANNING, "Σάρωση %s %u/%u") \
  X(RADIO_UPDATE_FAILED, "Αποτυχία ενημέρωσης") \
  X(RADIO_GOING_TO_SLEEP, "Μετάβαση σε ύπνο") \
  X(RADIO_PRESS_KNOB_TO_WAKE, "Πατήστε το κουμπί για αφύπνιση") \
  X(RADIO_OLD_IMAGE_KEPT, "Έμεινε το παλιό λογισμικό") \
  X(RDS_STEREO, "Στέρεο") \
  X(RDS_MONO, "Μονοφ.") \
  X(RDS_SPEECH, "Ομιλία") \
  X(RDS_MUSIC, "Μουσική") \
  X(COMMON_ALREADY_LOGGED, "Υπάρχει ήδη") \
  X(DX_RDS_IS_OFF, "RDS ανενεργό") \
  X(DX_SCANNING, "Σάρωση") \
  X(DX_SCAN_FINISHED, "Τέλος σάρωσης") \
  X(DX_NOTHING_TO_SCAN, "Τίποτα για σάρωση") \
  X(DX_CHECKING_FOR_UPDATES, "Έλεγχος ενημερώσεων") \
  X(DX_A_SCAN_IS_RUNNING, "Σάρωση σε εξέλιξη") \
  X(DX_CAUGHT_LIST_UNREAD, "Ευρήματα μη διαθέσιμα") \
  X(DX_NO_MEMORY, "Καμία μνήμη") \
  X(DX_NOT_STARTED, "Δεν ξεκίνησε") \
  X(DX_SWEEPING, "Μέτρηση") \
  X(COMMON_FMT_TUNE_TYPED, "Προς %s") \
  X(COMMON_AUTO, "ΑΥΤ.") \
  X(RADIO_TUNE_MANUAL, "ΧΕΙΡ.") \
  X(RADIO_TUNE_AUTO, "ΑΥΤ.") \
  X(RADIO_TUNE_MEMORY, "ΜΝΗΜΗ") \
  X(RADIO_TUNE_METER, "ΜΕΤΡΑ") \
  X(DX_FMT_ROWS_OF, "%u-%u από %u") \
  X(DX_FMT_FOUND, "Βρέθηκαν: %u") \
  X(DX_MODE_SWEEPING, "ΜΕΤΡΗΣΗ") \
  X(DX_MODE_LEARN_LOCALS, "ΜΑΘΗΣΗ ΤΟΠΙΚΩΝ") \
  X(DX_FMT_MODE_MEMORY, "ΜΝΗΜΕΣ %u-%u") \
  X(DX_MODE_WHOLE_BAND, "ΟΛΗ ΜΠΑΝΤΑ") \
  X(DX_MODE_BAND_MEMORY, "ΜΠΑΝΤΑ + ΜΝΗΜΕΣ") \
  X(DX_RULE_NO_STOP, "ΣΥΝΕΧΩΣ") \
  X(DX_RULE_STOP_ON_PI, "ΣΤΑΣΗ ΣΕ PI") \
  X(DX_RULE_STOP_ON_NEW, "ΣΤΑΣΗ ΣΕ ΝΕΟ") \
  X(DX_FMT_NOT_PRESET, "όχι P%02d") \
  X(DX_FMT_CHANNEL, "Καν. %u") \
  X(DX_CONTEXT_SWEEPING, "μέτρηση") \
  X(DX_PRESS_TO_SWEEP, "Πατήστε για μέτρηση") \
  X(SCOPE_FMT_TITLE, "Φάσμα %s") \
  X(SCOPE_FULL, "Όλη") \
  X(DX_FIXED, "ΣΤΑΘΕΡΗ") \
  X(DX_FMT_VS, "ΒΑΣΗ %s") \
  X(DX_FMT_MEDIAN_OF, "ΔΙΑΜΕΣΟΣ ΑΠΟ %u") \
  X(DX_FMT_FLOOR, "ΘΟΡ. %s dB\xC2\xB5V") \
  X(BW_CLOSE, "Έξοδος") \
  X(BW_FMT_AUTO_AT, "Αυτόματο: %u kHz") \
  X(RECOVERY_PRESS_AGAIN, "Πατήστε ξανά") \
  X(PAGE_TUNE, "Συντονισμός") \
  X(PAGE_DELETE, "Διαγραφή") \
  X(PAGE_LEVEL, "Στάθμη") \
  X(PAGE_NOISE, "Θόρυβος") \
  X(PAGE_MULTIPATH, "Πολυδιαδρομή") \
  X(PAGE_COCHANNEL, "Ομοκαναλικό") \
  X(PAGE_STEREO, "Στέρεο") \
  X(PAGE_WIDTH, "Εύρος") \
  X(PAGE_BAND, "Μπάντα") \
  X(PAGE_NOTE_CHANGED, "Άλλαξε από τότε που άνοιξε") \
  X(PAGE_NOTE_NOT_DELETED, "Δεν διαγράφηκε") \
  X(PAGE_FMT_SAVED, "Αποθηκεύτηκε: %s") \
  X(PAGE_TIME_UNKNOWN, "Άγνωστη ώρα") \
  X(BW_TITLE, "Εύρος ζώνης") \
  X(KEYPAD_TITLE, "Συχνότητα") \
  X(KEYPAD_CANCEL, "Άκυρο") \
  X(DX_CATCHES_EMPTY_TITLE, "Κανένα εύρημα ακόμη") \
  X(DX_CATCHES_EMPTY_NOTE, "Εδώ μπαίνουν οι επιβεβαιωμένοι σταθμοί") \
  X(DX_TITLE_CATCHES, "Ευρήματα") \
  X(DX_NEW, "ΝΕΟ") \
  X(DX_READY, "Έτοιμο") \
  X(DX_DWELL, "ΠΑΥΣΗ") \
  X(DX_STOPPED, "ΔΙΑΚΟΠΗ") \
  X(DX_TITLE_SCANNER, "Σαρωτής") \
  X(DX_TITLE_SCOPE, "Φάσμα") \
  X(DX_LEVEL, "ΣΗΜΑ") \
  X(DX_RISE, "ΑΥΞ.") \
  X(DX_NO_STATION_YET, "Χωρίς σταθμό ακόμη") \
  X(DX_NO_RDS_YET, "Χωρίς RDS ακόμη") \
  X(DX_OFFSET, "ΟΛΙΣΘ.") \
  X(DX_MOD, "ΔΙΑΜ.") \
  X(DX_NO_ID, "ΧΩΡΙΣ ID") \
  X(RDS_TITLE_TEXT, "Κείμενο") \
  X(RDS_TITLE_NETWORKS, "Δίκτυα") \
  X(RDS_TITLE_DECODER, "Λήψη") \
  X(RDS_NO_COUNTRY, "χωρίς ECC") \
  X(RDS_MS_UNKNOWN, "Μ/Ο") \
  X(RDS_RTPLUS_NONE, "Κανένα RT+ από αυτόν τον σταθμό") \
  X(RDS_RTPLUS_WAIT, "Καμία ετικέτα RT+ για το κείμενο") \
  X(RDS_AF_TITLE, "ΑΛΛΕΣ ΣΥΧΝΟΤΗΤΕΣ") \
  X(RDS_EON_TITLE, "ΑΛΛΟΙ ΣΤΑΘΜΟΙ, EON") \
  X(RDS_NONE_HEARD, "Καμία λήψη") \
  X(RDS_SYNC_CAPS, "ΣΥΓΧΡ.") \
  X(RDS_GROUPS_S_CAPS, "ΟΜΑΔΕΣ/S") \
  X(RDS_LOST, "ΧΑΜΕΝΑ") \
  X(RDS_EACH_BLOCK, "ΑΝΑ ΜΠΛΟΚ") \
  X(RDS_LEGEND_CLEAN, "καθαρά") \
  X(RDS_LEGEND_FIXED, "διορθ.") \
  X(RDS_LEGEND_LOST, "χαμένα") \
  X(RDS_ALL_CLEAN, "όλα καθαρά") \
  X(RDS_GROUP_TYPES, "ΤΥΠΟΙ ΟΜΑΔΩΝ") \
  X(RDS_NO_GROUPS, "Καμία ομάδα το τελευταίο λεπτό") \
  X(RDS_FMT_LAST_MINUTE, "%s \xC2\xB7 %u δευτ.") \
  X(RDS_COUNTRY_UNLISTED, "εκτός λίστας") \
  X(RDS_NO_TYPES, "Κανένα καθαρό μπλοκ B το τελευταίο λεπτό") \
  X(RDS_FMT_EON_MORE, "+%u ακόμη") \
  X(RDS_EON_MORE_HEARD, "+ ακόμη") \
  X(RDS_MORE, "ακόμη") \
  X(RADIO_VEIL_UPDATING, "ΕΝΗΜΕΡΩΣΗ ΛΟΓΙΣΜΙΚΟΥ") \
  X(RADIO_VEIL_KEEP_POWER, "Μην κόψετε το ρεύμα") \
  X(RADIO_MUTE, "ΣΙΓΗ") \
  X(TUNER_NO_TUNER_ON_THE_I2C_BUS, "Δεν βρέθηκε στον δίαυλο I2C") \
  X(TUNER_THE_TUNER_NEVER_BECAME_READY, "Δεν ετοιμάστηκε") \
  X(TUNER_THE_TUNER_WOULD_NOT_IDENTIFY_ITSELF, "Αποτυχία αναγνώρισης") \
  X(TUNER_THIS_BUILD_HAS_NO_PATCH_FOR_THAT_TUNER, "Χωρίς patch για αυτό το μοντέλο") \
  X(TUNER_UNKNOWN_TUNER_PART, "Άγνωστο μοντέλο") \
  X(TUNER_THE_TUNER_DID_NOT_ACCEPT_A_WRITE, "Απόρριψη εγγραφής") \
  X(TUNER_THE_TUNER_DID_NOT_ANSWER_A_READ, "Καμία απάντηση σε ανάγνωση") \
  X(TUNER_OUT_OF_RANGE, "Εκτός ορίων") \
  X(TUNER_UNKNOWN, "Άγνωστο") \
  X(BAND_LONG_LW, "Μακρά κύματα") \
  X(BAND_LONG_MW, "Μεσαία κύματα") \
  X(BAND_LONG_SW, "Βραχέα κύματα") \
  X(MENU_FMT_TUNE_TO, "Μπάντα για %s") \
  X(PTY_NONE, "Κανένα") \
  X(PTY_NEWS, "Ειδήσεις") \
  X(PTY_CURRENT_AFFAIRS, "Επικαιρότητα") \
  X(PTY_INFORMATION, "Πληροφόρηση") \
  X(PTY_SPORT, "Σπορ") \
  X(PTY_EDUCATION, "Εκπαίδευση") \
  X(PTY_DRAMA, "Δράμα") \
  X(PTY_CULTURE, "Πολιτισμός") \
  X(PTY_SCIENCE, "Επιστήμη") \
  X(PTY_VARIED, "Ποικίλο") \
  X(PTY_POP_MUSIC, "Ποπ μουσική") \
  X(PTY_ROCK_MUSIC, "Ροκ μουσική") \
  X(PTY_EASY_LISTENING, "Ελαφριά μουσική") \
  X(PTY_LIGHT_CLASSICAL, "Ελαφρά κλασική") \
  X(PTY_SERIOUS_CLASSICAL, "Κλασική") \
  X(PTY_OTHER_MUSIC, "Άλλη μουσική") \
  X(PTY_WEATHER, "Καιρός") \
  X(PTY_FINANCE, "Οικονομικά") \
  X(PTY_CHILDREN, "Παιδικά") \
  X(PTY_SOCIAL_AFFAIRS, "Κοινωνικά") \
  X(PTY_RELIGION, "Θρησκεία") \
  X(PTY_PHONE_IN, "Ανοιχτή γραμμή") \
  X(PTY_TRAVEL, "Ταξίδια") \
  X(PTY_LEISURE, "Ψυχαγωγία") \
  X(PTY_JAZZ_MUSIC, "Τζαζ") \
  X(PTY_COUNTRY_MUSIC, "Κάντρι") \
  X(PTY_NATIONAL_MUSIC, "Εθνική μουσική") \
  X(PTY_OLDIES_MUSIC, "Ρετρό") \
  X(PTY_FOLK_MUSIC, "Παραδοσιακή") \
  X(PTY_DOCUMENTARY, "Ντοκιμαντέρ") \
  X(PTY_ALARM_TEST, "Δοκιμή συναγερμού") \
  X(PTY_ALARM, "Συναγερμός") \
  X(PTY_NA_NEWS, "Ειδήσεις") \
  X(PTY_NA_INFORMATION, "Πληροφόρηση") \
  X(PTY_NA_SPORTS, "Σπορ") \
  X(PTY_NA_TALK, "Συζήτηση") \
  X(PTY_NA_ROCK, "Ροκ") \
  X(PTY_NA_CLASSIC_ROCK, "Κλασικό ροκ") \
  X(PTY_NA_ADULT_HITS, "Επιτυχίες ενηλίκων") \
  X(PTY_NA_SOFT_ROCK, "Απαλό ροκ") \
  X(PTY_NA_TOP_40, "Τοπ 40") \
  X(PTY_NA_COUNTRY, "Κάντρι") \
  X(PTY_NA_OLDIES, "Ρετρό") \
  X(PTY_NA_SOFT, "Απαλή") \
  X(PTY_NA_NOSTALGIA, "Νοσταλγία") \
  X(PTY_NA_JAZZ, "Τζαζ") \
  X(PTY_NA_CLASSICAL, "Κλασική") \
  X(PTY_NA_RHYTHM_AND_BLUES, "Ρυθμ εν μπλουζ") \
  X(PTY_NA_SOFT_RHYTHM_AND_BLUES, "Απαλό ρυθμ εν μπλουζ") \
  X(PTY_NA_FOREIGN_LANGUAGE, "Ξένη γλώσσα") \
  X(PTY_NA_RELIGIOUS_MUSIC, "Θρησκ. μουσική") \
  X(PTY_NA_RELIGIOUS_TALK, "Θρησκ. συζήτηση") \
  X(PTY_NA_PERSONALITY, "Προσωπικότητες") \
  X(PTY_NA_PUBLIC, "Δημόσιο") \
  X(PTY_NA_COLLEGE, "Φοιτητικό") \
  X(PTY_NA_SPANISH_TALK, "Ισπαν. συζήτηση") \
  X(PTY_NA_SPANISH_MUSIC, "Ισπανική μουσική") \
  X(PTY_NA_HIP_HOP, "Χιπ χοπ") \
  X(PTY_NA_UNASSIGNED, "Μη ορισμένο") \
  X(PTY_NA_WEATHER, "Καιρός") \
  X(PTY_NA_EMERGENCY_TEST, "Δοκιμή συναγερμού") \
  X(PTY_NA_EMERGENCY, "Συναγερμός") \
  X(COVERAGE_LOCAL, "Τοπική") \
  X(COVERAGE_INTERNATIONAL, "Διεθνής") \
  X(COVERAGE_NATIONAL, "Εθνική") \
  X(COVERAGE_SUPRA_REGIONAL, "Υπερπεριφ.") \
  X(COVERAGE_REGIONAL_1, "Περιοχή 1") \
  X(COVERAGE_REGIONAL_2, "Περιοχή 2") \
  X(COVERAGE_REGIONAL_3, "Περιοχή 3") \
  X(COVERAGE_REGIONAL_4, "Περιοχή 4") \
  X(COVERAGE_REGIONAL_5, "Περιοχή 5") \
  X(COVERAGE_REGIONAL_6, "Περιοχή 6") \
  X(COVERAGE_REGIONAL_7, "Περιοχή 7") \
  X(COVERAGE_REGIONAL_8, "Περιοχή 8") \
  X(COVERAGE_REGIONAL_9, "Περιοχή 9") \
  X(COVERAGE_REGIONAL_10, "Περιοχή 10") \
  X(COVERAGE_REGIONAL_11, "Περιοχή 11") \
  X(COVERAGE_REGIONAL_12, "Περιοχή 12") \
  X(RTP_OTHER, "ΑΛΛΟ") \
  X(RTP_TITLE, "ΤΙΤΛΟΣ") \
  X(RTP_ALBUM, "ΑΛΜΠΟΥΜ") \
  X(RTP_TRACK, "ΚΟΜΜΑΤΙ") \
  X(RTP_ARTIST, "ΚΑΛΛΙΤΕΧΝΗΣ") \
  X(RTP_WORK, "ΕΡΓΟ") \
  X(RTP_MOVEMENT, "ΜΕΡΟΣ") \
  X(RTP_CONDUCTOR, "ΜΑΕΣΤΡΟΣ") \
  X(RTP_COMPOSER, "ΣΥΝΘΕΤΗΣ") \
  X(RTP_BAND, "ΣΥΓΚΡΟΤΗΜΑ") \
  X(RTP_COMMENT, "ΣΧΟΛΙΟ") \
  X(RTP_GENRE, "ΕΙΔΟΣ") \
  X(RTP_NEWS, "ΕΙΔΗΣΕΙΣ") \
  X(RTP_LOCAL, "ΤΟΠΙΚΕΣ ΕΙΔ.") \
  X(RTP_STOCKS, "ΜΕΤΟΧΕΣ") \
  X(RTP_SPORT, "ΣΠΟΡ") \
  X(RTP_LOTTERY, "ΛΑΧΕΙΟ") \
  X(RTP_HOROSCOPE, "ΩΡΟΣΚΟΠΙΟ") \
  X(RTP_DIVERSION, "ΑΝΑΨΥΧΗ") \
  X(RTP_HEALTH, "ΥΓΕΙΑ") \
  X(RTP_EVENT, "ΕΚΔΗΛΩΣΗ") \
  X(RTP_SCENE, "ΣΚΗΝΗ") \
  X(RTP_CINEMA, "ΣΙΝΕΜΑ") \
  X(RTP_FUN, "ΧΙΟΥΜΟΡ") \
  X(RTP_DATE, "ΗΜ/ΝΙΑ") \
  X(RTP_WEATHER, "ΚΑΙΡΟΣ") \
  X(RTP_TRAFFIC, "ΚΥΚΛΟΦΟΡΙΑ") \
  X(RTP_ALARM, "ΣΥΝΑΓΕΡΜΟΣ") \
  X(RTP_ADVERT, "ΔΙΑΦΗΜΙΣΗ") \
  X(RTP_STATION, "ΣΤΑΘΜΟΣ") \
  X(RTP_PROGRAM, "ΠΡΟΓΡΑΜΜΑ") \
  X(RTP_NEXT, "ΕΠΟΜΕΝΟ") \
  X(RTP_PART, "ΤΜΗΜΑ") \
  X(RTP_HOST, "ΠΑΡΟΥΣΙΑΣΗ") \
  X(RTP_EDITORS, "ΣΥΝΤΑΞΗ") \
  X(RTP_FREQUENCY, "ΣΥΧΝΟΤΗΤΑ") \
  X(RTP_HOMEPAGE, "ΙΣΤΟΣΕΛΙΔΑ") \
  X(RTP_CHANNEL, "ΚΑΝΑΛΙ") \
  X(RTP_HOTLINE, "ΤΗΛ. ΓΡΑΜΜΗ") \
  X(RTP_STUDIO, "ΣΤΟΥΝΤΙΟ") \
  X(RTP_PHONE, "ΤΗΛΕΦΩΝΟ") \
  X(RTP_CHAT, "ΣΥΝΟΜΙΛΙΑ") \
  X(RTP_VOTE, "ΨΗΦΟΣ") \
  X(RTP_PLACE, "ΤΟΠΟΘΕΣΙΑ") \
  X(RTP_APPOINTMENT, "ΡΑΝΤΕΒΟΥ") \
  X(RTP_PURCHASE, "ΑΓΟΡΑ") \
  X(DAY_SUNDAY, "ΚΥΡΙΑΚΗ") \
  X(DAY_MONDAY, "ΔΕΥΤΕΡΑ") \
  X(DAY_TUESDAY, "ΤΡΙΤΗ") \
  X(DAY_WEDNESDAY, "ΤΕΤΑΡΤΗ") \
  X(DAY_THURSDAY, "ΠΕΜΠΤΗ") \
  X(DAY_FRIDAY, "ΠΑΡΑΣΚΕΥΗ") \
  X(DAY_SATURDAY, "ΣΑΒΒΑΤΟ") \
  X(MONTH_JANUARY, "Ιανουαρίου") \
  X(MONTH_FEBRUARY, "Φεβρουαρίου") \
  X(MONTH_MARCH, "Μαρτίου") \
  X(MONTH_APRIL, "Απριλίου") \
  X(MONTH_MAY, "Μαΐου") \
  X(MONTH_JUNE, "Ιουνίου") \
  X(MONTH_JULY, "Ιουλίου") \
  X(MONTH_AUGUST, "Αυγούστου") \
  X(MONTH_SEPTEMBER, "Σεπτεμβρίου") \
  X(MONTH_OCTOBER, "Οκτωβρίου") \
  X(MONTH_NOVEMBER, "Νοεμβρίου") \
  X(MONTH_DECEMBER, "Δεκεμβρίου") \
  X(DATE_SUFFIX_TH, "") \
  X(DATE_SUFFIX_ST, "") \
  X(DATE_SUFFIX_ND, "") \
  X(DATE_SUFFIX_RD, "") \
  X(DATE_TODAY, "Σήμερα") \
  X(DATE_YESTERDAY, "Χθες") \
  X(DATE_YDAY, "ΧΘΕΣ") \
  X(MONTH_SHORT_JAN, "Ιαν") \
  X(MONTH_SHORT_FEB, "Φεβ") \
  X(MONTH_SHORT_MAR, "Μαρ") \
  X(MONTH_SHORT_APR, "Απρ") \
  X(MONTH_SHORT_MAY, "Μαΐ") \
  X(MONTH_SHORT_JUN, "Ιουν") \
  X(MONTH_SHORT_JUL, "Ιουλ") \
  X(MONTH_SHORT_AUG, "Αυγ") \
  X(MONTH_SHORT_SEP, "Σεπ") \
  X(MONTH_SHORT_OCT, "Οκτ") \
  X(MONTH_SHORT_NOV, "Νοε") \
  X(MONTH_SHORT_DEC, "Δεκ") \
  X(MONTH_TAG_JAN, "ΙΑΝ") \
  X(MONTH_TAG_FEB, "ΦΕΒ") \
  X(MONTH_TAG_MAR, "ΜΑΡ") \
  X(MONTH_TAG_APR, "ΑΠΡ") \
  X(MONTH_TAG_MAY, "ΜΑΪ") \
  X(MONTH_TAG_JUN, "ΙΟΥΝ") \
  X(MONTH_TAG_JUL, "ΙΟΥΛ") \
  X(MONTH_TAG_AUG, "ΑΥΓ") \
  X(MONTH_TAG_SEP, "ΣΕΠ") \
  X(MONTH_TAG_OCT, "ΟΚΤ") \
  X(MONTH_TAG_NOV, "ΝΟΕ") \
  X(MONTH_TAG_DEC, "ΔΕΚ") \
  X(DX_NOW, "τώρα") \
  X(DX_FMT_MIN_AGO, "πριν %u λεπ.") \
  X(DX_FMT_H_AGO, "πριν %u ώρ.") \
  X(DX_FMT_D_AGO, "πριν %u ημ.") \
  X(TOUCH_CAL_TITLE, "Βαθμονόμηση αφής") \
  X(TOUCH_CAL_HOLD, "Κρατήστε τον κύκλο μέχρι να γεμίσει") \
  X(TOUCH_CAL_CANCEL_HINT, "Γυρίστε ή πατήστε για ακύρωση") \
  X(TOUCH_CAL_CHECK, "Έλεγχος") \
  X(TOUCH_CAL_TAP_DOT, "Αγγίξτε την τελεία") \
  X(TOUCH_CAL_SAVED, "Βαθμονόμηση αποθηκεύτηκε") \
  X(TOUCH_CAL_FMT_LANDED, "Ο έλεγχος απείχε %u px") \
  X(TOUCH_CAL_BACK_HINT, "Πατήστε το κουμπί για επιστροφή") \
  X(TOUCH_CAL_NOT_SAVED_TITLE, "Δεν αποθηκεύτηκε") \
  X(TOUCH_CAL_FMT_MISSED, "Ο έλεγχος απείχε %u px,") \
  X(TOUCH_CAL_NO_FIT, "Τα σημάδια δεν έδωσαν βαθμονόμηση,") \
  X(TOUCH_CAL_NOT_SAVED, "Δεν ήταν δυνατή η εγγραφή,") \
  X(TOUCH_CAL_OLD_STAYS, "οπότε μένει η παλιά βαθμονόμηση") \
  X(TOUCH_CAL_RETRY_HINT, "Πατήστε: ξανά, γυρίστε: έξοδος")
// clang-format on
