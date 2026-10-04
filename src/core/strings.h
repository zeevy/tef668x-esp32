/*
 * Every text the panel shows, by name.
 *
 * The list itself is lang/en.h. This header only turns it into the StrId enum,
 * one STR_ value per line of that list, and gives txt() to read the text back.
 * A screen says txt(STR_MENU_STATIONS) and never quotes the words, so the words
 * can change, or be swapped for another language, without the screen knowing.
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

/* The text for an id. An id out of range gives an empty string rather than a
 * read past the table. */
const char *txt(StrId id);

#ifdef __cplusplus
}
#endif
