/*
 * Every text the panel shows, by name, in every language the panel speaks.
 *
 * The list itself is lang/en.h. This header only turns it into the StrId enum,
 * one STR_ value per line of that list, and gives txt() to read the text back.
 * A screen says txt(STR_MENU_STATIONS) and never quotes the words, so the words
 * can change, or be swapped for another language, without the screen knowing.
 *
 * txt() answers in the panel's language. Whatever leaves the radio as data,
 * through the web server, the PC Link or a file, asks for English by name
 * with txtIn(TXT_ENGLISH, ...), because the web page and the CSV import read
 * some of those words back as values.
 */
#pragma once

#include "lang/en.h"

#ifdef __cplusplus
extern "C" {
#endif

// clang-format off
typedef enum {
#define X(id, text, where) STR_##id,
  STRINGS(X)
#undef X
  STR_COUNT
} StrId;
// clang-format on

/*
 * The languages the panel can speak, in the order they were added. The number
 * is stored in every radio's settings, so an entry never moves and a new
 * language goes at the end, before TXT_LANGUAGE_COUNT.
 */
typedef enum {
  TXT_ENGLISH,
  TXT_CHINESE,
  TXT_GERMAN,
  TXT_RUSSIAN,
  TXT_DUTCH,
  TXT_FRENCH,
  TXT_SPANISH,
  TXT_ITALIAN,
  TXT_PORTUGUESE,
  TXT_POLISH,
  TXT_CZECH,
  TXT_CROATIAN,
  TXT_SERBIAN,
  TXT_ROMANIAN,
  TXT_HUNGARIAN,
  TXT_DANISH,
  TXT_NORWEGIAN,
  TXT_SWEDISH,
  TXT_FINNISH,
  TXT_IRISH,
  TXT_LANGUAGE_COUNT
} TxtLanguage;

/* The text for an id in the panel's language. A text the language leaves out
 * gives the English one, and an id out of range an empty string rather than a
 * read past the table. */
const char *txt(StrId id);

/* The same in a given language. A language out of range gives English. */
const char *txtIn(TxtLanguage language, StrId id);

/* The panel's language from now on. A number that is no language sets
 * English. Set once, as the boot screen hands over to the radio screen. */
void txtSetLanguage(TxtLanguage language);

/* A language's own name in its own script, such as "Deutsch", the same in
 * every language. English for a number that is no language. */
StrId txtLanguageName(TxtLanguage language);

/* A language's code, such as "de", or NULL for a number that is no
 * language. */
const char *txtLanguageCode(TxtLanguage language);

#ifdef __cplusplus
}
#endif
