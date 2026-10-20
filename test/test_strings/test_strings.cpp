/*
 * Tests for the string tables. Runs on a PC.
 *
 * The enum and the English table come from the same list in lang/en.h, so
 * they cannot drift, and a language file that holds an id twice or an id
 * en.h does not have does not compile. What is left to check is a hole in
 * the list, an id past the end, that each table lines up with the enum, that
 * every language's formats are ones snprintf fills the same way as the
 * English ones, that every text is whole UTF-8, and that the texts which
 * stay English do so in every language.
 */
#include <stdio.h>
#include <string.h>
#include <unity.h>

#include "core/strings.h"

void setUp(void) {
  txtSetLanguage(TXT_ENGLISH);
}
void tearDown(void) {}

static const char *const kIdNames[STR_COUNT] = {
#define X(id, text, where) #id,
    STRINGS(X)
#undef X
};

static void every_id_has_text(void) {
  for (unsigned i = 0; i < STR_COUNT; i++) {
    TEST_ASSERT_NOT_NULL(txt((StrId)i));
    TEST_ASSERT_TRUE_MESSAGE(txt((StrId)i)[0] != '\0', "an id has empty text");
  }
}

static void an_id_past_the_end_gives_an_empty_string(void) {
  TEST_ASSERT_EQUAL_STRING("", txt(STR_COUNT));
  TEST_ASSERT_EQUAL_STRING("", txt((StrId)-1));
  txtSetLanguage(TXT_GERMAN);
  TEST_ASSERT_EQUAL_STRING("", txt(STR_COUNT));
  TEST_ASSERT_EQUAL_STRING("", txtIn(TXT_CHINESE, (StrId)-1));
}

/* One known text, so a table shifted by one is caught. */
static void the_text_matches_the_id(void) {
  TEST_ASSERT_EQUAL_STRING("Volume AGC", txt(STR_MENU_VOLUME_AGC));
  TEST_ASSERT_EQUAL_STRING("Alarm", txt(STR_PTY_ALARM));
}

/* What snprintf reads for each argument of a text, by argument number: the
 * conversion letter with its length letters before it ("d", "lu", "s"). A
 * `%%` reads nothing. Width, precision and flags do not change what is read,
 * so they are left out. A language may number its arguments, "%2$s", to put
 * them in another order; then every one must be numbered and none skipped,
 * since snprintf cannot step over one it does not know the type of. False
 * for a text that mixes the two ways, skips one, reads one two ways or
 * numbers one past MAX_ARGS. */
#define MAX_ARGS 8
typedef struct {
  char conv[MAX_ARGS][4];
  int count;
} Reads;

static bool readsOf(const char *text, Reads *r) {
  memset(r, 0, sizeof(*r));
  int numbered = -1; /* Not known yet. */
  int next = 0;
  for (const char *p = text; *p != '\0'; p++) {
    if (*p != '%') {
      continue;
    }
    p++;
    if (*p == '%') {
      continue;
    }
    if (*p == '\0') {
      break; /* A text that is only the unit, "%", is not a format. */
    }
    int at = next++;
    const char *digits = p;
    int n = 0;
    while (*p >= '0' && *p <= '9') {
      n = n * 10 + (*p - '0');
      p++;
    }
    const bool hasNumber = *p == '$' && p > digits;
    if (hasNumber) {
      at = n - 1;
      p++;
    } else {
      p = digits;
    }
    if (numbered == -1) {
      numbered = hasNumber ? 1 : 0;
    } else if (numbered != (hasNumber ? 1 : 0)) {
      return false;
    }
    if (at < 0 || at >= MAX_ARGS) {
      return false;
    }
    while (*p != '\0' && strchr("-+ #0123456789.*", *p) != NULL) {
      p++;
    }
    char conv[4] = {0};
    size_t k = 0;
    while (*p != '\0' && strchr("hljztL", *p) != NULL && k < 2) {
      conv[k++] = *p++;
    }
    if (*p == '\0') {
      return false;
    }
    conv[k] = *p;
    if (r->conv[at][0] != '\0') {
      return false; /* One argument read twice. */
    }
    memcpy(r->conv[at], conv, sizeof(conv));
    if (at + 1 > r->count) {
      r->count = at + 1;
    }
  }
  for (int i = 0; i < r->count; i++) {
    if (r->conv[i][0] == '\0') {
      return false; /* An argument skipped. */
    }
  }
  return true;
}

/* A format snprintf reads differently from the English one reads a number as
 * a pointer, or past the arguments the caller gave. */
static void every_format_reads_the_same_arguments(void) {
  for (int l = 1; l < TXT_LANGUAGE_COUNT; l++) {
    for (unsigned i = 0; i < STR_COUNT; i++) {
      Reads en;
      Reads other;
      const char *english = txtIn(TXT_ENGLISH, (StrId)i);
      TEST_ASSERT_TRUE_MESSAGE(readsOf(english, &en), english);
      const char *text = txtIn((TxtLanguage)l, (StrId)i);
      const bool same = readsOf(text, &other) && other.count == en.count &&
                        memcmp(other.conv, en.conv, sizeof(en.conv)) == 0;
      if (!same) {
        char msg[240];
        snprintf(msg, sizeof(msg),
                 "%s, %s: \"%s\" does not read what \"%s\" reads",
                 txtLanguageCode((TxtLanguage)l), kIdNames[i], text, english);
        TEST_FAIL_MESSAGE(msg);
      }
    }
  }
}

/* The date line with the year first and the day name last, through the
 * numbered arguments the radio's snprintf takes. */
static void a_numbered_format_reorders(void) {
  char out[32];
  snprintf(out, sizeof(out), "%5$d-%4$s-%2$d%3$s %1$s", "MON", 20, "th", "Oct",
           2026);
  TEST_ASSERT_EQUAL_STRING("2026-Oct-20th MON", out);
  Reads a;
  Reads b;
  TEST_ASSERT_TRUE(readsOf("%s, %d%s %s %d", &a));
  TEST_ASSERT_TRUE(readsOf("%5$d-%4$s-%2$d%3$s %1$s", &b));
  TEST_ASSERT_EQUAL_INT(5, b.count);
  TEST_ASSERT_EQUAL_MEMORY(a.conv, b.conv, sizeof(a.conv));
  TEST_ASSERT_FALSE(readsOf("%2$s %s", &b));   /* Mixed. */
  TEST_ASSERT_FALSE(readsOf("%3$d %1$s", &b)); /* The second skipped. */
  TEST_ASSERT_FALSE(readsOf("%1$d %1$s", &b)); /* Read twice. */
  TEST_ASSERT_FALSE(readsOf("%9$d", &b));      /* Past MAX_ARGS. */
  TEST_ASSERT_FALSE(readsOf("%l", &b));        /* No conversion. */
  TEST_ASSERT_TRUE(readsOf("100%% %lu", &b));
  TEST_ASSERT_EQUAL_STRING("lu", b.conv[0]);
}

/* Whole UTF-8 characters only, so no text ends in half a character the font
 * cannot draw. */
static void every_text_is_whole_utf8(void) {
  for (int l = 0; l < TXT_LANGUAGE_COUNT; l++) {
    for (unsigned i = 0; i < STR_COUNT; i++) {
      const unsigned char *p =
          (const unsigned char *)txtIn((TxtLanguage)l, (StrId)i);
      while (*p != '\0') {
        int more = *p < 0x80             ? 0
                   : (*p & 0xE0) == 0xC0 ? 1
                   : (*p & 0xF0) == 0xE0 ? 2
                   : (*p & 0xF8) == 0xF0 ? 3
                                         : -1;
        TEST_ASSERT_TRUE_MESSAGE(more >= 0, kIdNames[i]);
        p++;
        for (; more > 0; more--, p++) {
          TEST_ASSERT_TRUE_MESSAGE((*p & 0xC0) == 0x80, kIdNames[i]);
        }
      }
    }
  }
}

/* The texts that are the same in every language: the boot and recovery
 * screens', drawn before the language is set, the RDS programme languages,
 * the theme and language names, the units, the band codes, the clock and
 * the metre band. The web page, the API and the files send some of them, and
 * the CSV import reads the band codes back. */
static bool staysEnglish(const char *id) {
  static const char *const kPrefixes[] = {"BOOT_", "LANG_", "THEME_",
                                          "LANGUAGE_", "COMMON_UNIT_"};
  static const char *const kIds[] = {"BAND_LW",
                                     "BAND_MW",
                                     "BAND_SW",
                                     "BAND_OIRT",
                                     "BAND_FM",
                                     "COMMON_FMT_CLOCK",
                                     "RADIO_FMT_METER_BAND"};
  for (size_t i = 0; i < sizeof(kPrefixes) / sizeof(kPrefixes[0]); i++) {
    if (strncmp(id, kPrefixes[i], strlen(kPrefixes[i])) == 0) {
      return true;
    }
  }
  for (size_t i = 0; i < sizeof(kIds) / sizeof(kIds[0]); i++) {
    if (strcmp(id, kIds[i]) == 0) {
      return true;
    }
  }
  return strncmp(id, "RECOVERY_", 9) == 0 &&
         strcmp(id, "RECOVERY_PRESS_AGAIN") != 0;
}

static void some_texts_stay_english_in_every_language(void) {
  unsigned kept = 0;
  for (unsigned i = 0; i < STR_COUNT; i++) {
    if (!staysEnglish(kIdNames[i])) {
      continue;
    }
    kept++;
    for (int l = 1; l < TXT_LANGUAGE_COUNT; l++) {
      TEST_ASSERT_EQUAL_PTR_MESSAGE(txtIn(TXT_ENGLISH, (StrId)i),
                                    txtIn((TxtLanguage)l, (StrId)i),
                                    kIdNames[i]);
    }
  }
  TEST_ASSERT_TRUE(kept > 100);
}

static void the_language_picks_the_table(void) {
  TEST_ASSERT_EQUAL_STRING("Language", txt(STR_MENU_LANGUAGE));
  txtSetLanguage(TXT_GERMAN);
  TEST_ASSERT_EQUAL_STRING(txtIn(TXT_GERMAN, STR_MENU_LANGUAGE),
                           txt(STR_MENU_LANGUAGE));
  TEST_ASSERT_TRUE(strcmp("Language", txt(STR_MENU_LANGUAGE)) != 0);
  /* The panel's language changes nothing asked for in English. */
  TEST_ASSERT_EQUAL_STRING("Language", txtIn(TXT_ENGLISH, STR_MENU_LANGUAGE));
  /* A text the language leaves out shows the English one. */
  TEST_ASSERT_EQUAL_STRING("FM", txt(STR_BAND_FM));
}

/* The Language row's own name ends with the English word in every language,
 * so a person who cannot read the panel finds the way back. */
static void the_language_row_says_language_in_english(void) {
  for (int l = 0; l < TXT_LANGUAGE_COUNT; l++) {
    const char *name = txtIn((TxtLanguage)l, STR_MENU_LANGUAGE);
    const size_t n = strlen(name);
    TEST_ASSERT_TRUE_MESSAGE(n >= 8 && strcmp(name + n - 8, "Language") == 0,
                             name);
  }
}

static void an_unknown_language_is_english(void) {
  txtSetLanguage(TXT_CHINESE);
  txtSetLanguage(TXT_LANGUAGE_COUNT);
  TEST_ASSERT_EQUAL_STRING("Language", txt(STR_MENU_LANGUAGE));
  txtSetLanguage(TXT_CHINESE);
  txtSetLanguage((TxtLanguage)200);
  TEST_ASSERT_EQUAL_STRING("Language", txt(STR_MENU_LANGUAGE));
  TEST_ASSERT_EQUAL_STRING("Language",
                           txtIn(TXT_LANGUAGE_COUNT, STR_MENU_LANGUAGE));
  TEST_ASSERT_EQUAL(STR_LANGUAGE_EN, txtLanguageName(TXT_LANGUAGE_COUNT));
  TEST_ASSERT_NULL(txtLanguageCode(TXT_LANGUAGE_COUNT));
}

/* Each language has its own code and its own name, and the names read the
 * same whatever the panel shows. */
static void every_language_has_its_own_code_and_name(void) {
  TEST_ASSERT_EQUAL_STRING("en", txtLanguageCode(TXT_ENGLISH));
  TEST_ASSERT_EQUAL_STRING("简体中文", txt(txtLanguageName(TXT_CHINESE)));
  for (int a = 0; a < TXT_LANGUAGE_COUNT; a++) {
    TEST_ASSERT_NOT_NULL(txtLanguageCode((TxtLanguage)a));
    const StrId name = txtLanguageName((TxtLanguage)a);
    TEST_ASSERT_EQUAL_STRING(txtIn(TXT_ENGLISH, name),
                             txtIn(TXT_RUSSIAN, name));
    for (int b = a + 1; b < TXT_LANGUAGE_COUNT; b++) {
      TEST_ASSERT_TRUE(strcmp(txtLanguageCode((TxtLanguage)a),
                              txtLanguageCode((TxtLanguage)b)) != 0);
      TEST_ASSERT_TRUE(name != txtLanguageName((TxtLanguage)b));
    }
  }
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(every_id_has_text);
  RUN_TEST(an_id_past_the_end_gives_an_empty_string);
  RUN_TEST(the_text_matches_the_id);
  RUN_TEST(every_format_reads_the_same_arguments);
  RUN_TEST(a_numbered_format_reorders);
  RUN_TEST(every_text_is_whole_utf8);
  RUN_TEST(some_texts_stay_english_in_every_language);
  RUN_TEST(the_language_picks_the_table);
  RUN_TEST(the_language_row_says_language_in_english);
  RUN_TEST(an_unknown_language_is_english);
  RUN_TEST(every_language_has_its_own_code_and_name);
  return UNITY_END();
}
