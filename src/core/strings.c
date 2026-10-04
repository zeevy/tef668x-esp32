#include "core/strings.h"

/* The table is the same list as the enum, so an id and its text cannot get
 * out of step. The where note is dropped here; it is for the reader of
 * lang/en.h. */
static const char *const kText[STR_COUNT] = {
#define X(id, text, where) [STR_##id] = text,
    STRINGS(X)
#undef X
};

const char *txt(StrId id) {
  return (unsigned)id < STR_COUNT ? kText[id] : "";
}
