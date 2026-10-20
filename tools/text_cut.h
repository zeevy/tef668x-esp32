/*
 * For the screenshot tool only: every snprintf goes through
 * textCutSnprintf, which keeps the place and the text of any write that did
 * not fit its buffer, so a text a longer language cuts short is found from
 * the captures. Given to the compiler with -include, after <stdio.h>, so the
 * library's own declaration is read first.
 */
#ifndef TOOLS_TEXT_CUT_H
#define TOOLS_TEXT_CUT_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
int textCutSnprintf(const char *file, int line, char *out, size_t cap,
                    const char *format, ...);
#ifdef __cplusplus
}
#endif

#define snprintf(out, cap, ...) \
  textCutSnprintf(__FILE__, __LINE__, out, cap, __VA_ARGS__)

#endif
