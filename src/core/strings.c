#include "core/strings.h"

#include <stddef.h>
#include <stdint.h>

#include "lang/de.h"
#include "lang/es.h"
#include "lang/fr.h"
#include "lang/it.h"
#include "lang/nl.h"
#include "lang/pt.h"
#include "lang/ru.h"
#include "lang/zh.h"

/* The table is the same list as the enum, so an id and its text cannot get
 * out of step. The where note is dropped here; it is for the reader of
 * lang/en.h. */
static const char *const kText[STR_COUNT] = {
#define X(id, text, where) [STR_##id] = text,
    STRINGS(X)
#undef X
};

/*
 * Every other language keeps its texts in one block of bytes and finds each by
 * its offset, two bytes a text where a pointer would be four: about 1.6 KB
 * less a language. The block is a struct of character arrays, one per text,
 * which the compiler lays end to end with no gap since a character array needs
 * no alignment. An offset is stored plus one, so the 0 of a text the language
 * leaves out says "the English one". A language that holds an id twice, or an
 * id en.h does not have, does not compile.
 *
 * TEXT_AT names the struct through LANGUAGE, which each language below sets
 * before it is expanded.
 */
#define TEXT_FIELD(id, text) char f_##id[sizeof(text)];
#define TEXT_VALUE(id, text) text,
#define TEXT_AT(id, text) \
  [STR_##id] = (uint16_t)(offsetof(LANGUAGE, f_##id) + 1),

#define LANGUAGE struct TextsZh
LANGUAGE{STRINGS_ZH(TEXT_FIELD)};
static const LANGUAGE kZh = {STRINGS_ZH(TEXT_VALUE)};
static const uint16_t kZhAt[STR_COUNT] = {STRINGS_ZH(TEXT_AT)};
_Static_assert(sizeof(kZh) < UINT16_MAX,
               "Chinese is past what an offset holds");
#undef LANGUAGE

#define LANGUAGE struct TextsDe
LANGUAGE{STRINGS_DE(TEXT_FIELD)};
static const LANGUAGE kDe = {STRINGS_DE(TEXT_VALUE)};
static const uint16_t kDeAt[STR_COUNT] = {STRINGS_DE(TEXT_AT)};
_Static_assert(sizeof(kDe) < UINT16_MAX, "German is past what an offset holds");
#undef LANGUAGE

#define LANGUAGE struct TextsRu
LANGUAGE{STRINGS_RU(TEXT_FIELD)};
static const LANGUAGE kRu = {STRINGS_RU(TEXT_VALUE)};
static const uint16_t kRuAt[STR_COUNT] = {STRINGS_RU(TEXT_AT)};
_Static_assert(sizeof(kRu) < UINT16_MAX,
               "Russian is past what an offset holds");
#undef LANGUAGE

#define LANGUAGE struct TextsNl
LANGUAGE{STRINGS_NL(TEXT_FIELD)};
static const LANGUAGE kNl = {STRINGS_NL(TEXT_VALUE)};
static const uint16_t kNlAt[STR_COUNT] = {STRINGS_NL(TEXT_AT)};
_Static_assert(sizeof(kNl) < UINT16_MAX, "Dutch is past what an offset holds");
#undef LANGUAGE

#define LANGUAGE struct TextsFr
LANGUAGE{STRINGS_FR(TEXT_FIELD)};
static const LANGUAGE kFr = {STRINGS_FR(TEXT_VALUE)};
static const uint16_t kFrAt[STR_COUNT] = {STRINGS_FR(TEXT_AT)};
_Static_assert(sizeof(kFr) < UINT16_MAX, "French is past what an offset holds");
#undef LANGUAGE

#define LANGUAGE struct TextsEs
LANGUAGE{STRINGS_ES(TEXT_FIELD)};
static const LANGUAGE kEs = {STRINGS_ES(TEXT_VALUE)};
static const uint16_t kEsAt[STR_COUNT] = {STRINGS_ES(TEXT_AT)};
_Static_assert(sizeof(kEs) < UINT16_MAX,
               "Spanish is past what an offset holds");
#undef LANGUAGE

#define LANGUAGE struct TextsIt
LANGUAGE{STRINGS_IT(TEXT_FIELD)};
static const LANGUAGE kIt = {STRINGS_IT(TEXT_VALUE)};
static const uint16_t kItAt[STR_COUNT] = {STRINGS_IT(TEXT_AT)};
_Static_assert(sizeof(kIt) < UINT16_MAX,
               "Italian is past what an offset holds");
#undef LANGUAGE

#define LANGUAGE struct TextsPt
LANGUAGE{STRINGS_PT(TEXT_FIELD)};
static const LANGUAGE kPt = {STRINGS_PT(TEXT_VALUE)};
static const uint16_t kPtAt[STR_COUNT] = {STRINGS_PT(TEXT_AT)};
_Static_assert(sizeof(kPt) < UINT16_MAX,
               "Portuguese is past what an offset holds");
#undef LANGUAGE

typedef struct {
  const char *code;
  StrId name;
  const char *texts; /* NULL for English. */
  const uint16_t *at;
} Language;

static const Language kLanguages[TXT_LANGUAGE_COUNT] = {
    [TXT_ENGLISH] = {"en", STR_LANGUAGE_EN, NULL, NULL},
    [TXT_CHINESE] = {"zh", STR_LANGUAGE_ZH, (const char *)&kZh, kZhAt},
    [TXT_GERMAN] = {"de", STR_LANGUAGE_DE, (const char *)&kDe, kDeAt},
    [TXT_RUSSIAN] = {"ru", STR_LANGUAGE_RU, (const char *)&kRu, kRuAt},
    [TXT_DUTCH] = {"nl", STR_LANGUAGE_NL, (const char *)&kNl, kNlAt},
    [TXT_FRENCH] = {"fr", STR_LANGUAGE_FR, (const char *)&kFr, kFrAt},
    [TXT_SPANISH] = {"es", STR_LANGUAGE_ES, (const char *)&kEs, kEsAt},
    [TXT_ITALIAN] = {"it", STR_LANGUAGE_IT, (const char *)&kIt, kItAt},
    [TXT_PORTUGUESE] = {"pt", STR_LANGUAGE_PT, (const char *)&kPt, kPtAt},
};

static TxtLanguage sLanguage = TXT_ENGLISH;

const char *txtIn(TxtLanguage language, StrId id) {
  if ((unsigned)id >= STR_COUNT) {
    return "";
  }
  if ((unsigned)language >= TXT_LANGUAGE_COUNT ||
      kLanguages[language].texts == NULL) {
    return kText[id];
  }
  const uint16_t at = kLanguages[language].at[id];
  return at != 0 ? kLanguages[language].texts + at - 1 : kText[id];
}

const char *txt(StrId id) {
  return txtIn(sLanguage, id);
}

void txtSetLanguage(TxtLanguage language) {
  sLanguage = (unsigned)language < TXT_LANGUAGE_COUNT ? language : TXT_ENGLISH;
}

StrId txtLanguageName(TxtLanguage language) {
  return kLanguages[(unsigned)language < TXT_LANGUAGE_COUNT ? language
                                                            : TXT_ENGLISH]
      .name;
}

const char *txtLanguageCode(TxtLanguage language) {
  return (unsigned)language < TXT_LANGUAGE_COUNT ? kLanguages[language].code
                                                 : NULL;
}
