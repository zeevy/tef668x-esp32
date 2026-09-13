/*
 * The stored channels as text, so a list can be kept off the radio.
 *
 * One line per channel:
 *
 * ```
 * slot,band,frequency,bandwidth,name
 * 1,FM,92700,0,Radio City
 * 2,MW,1071,6,"Vividh Bharati, Hyderabad"
 * ```
 *
 * The slot is counted from 1, the way it is shown on the panel. Frequency and
 * bandwidth are in kHz on every band, which is the unit the whole of core/
 * uses. A bandwidth of 0 means let the radio choose. The band is its name
 * rather than a number, so a file stays readable and a band added later cannot
 * silently change what an old file means.
 *
 * A name holding a comma or a quote is wrapped in quotes, and a quote inside
 * one is doubled, which is what a spreadsheet writes and reads.
 *
 * Reading skips blank lines, a line starting with `#`, and a header line, so a
 * file exported from here goes straight back in.
 *
 * Nothing here touches hardware, so it builds and is tested on a PC.
 */
#ifndef CORE_MEMORY_CSV_H
#define CORE_MEMORY_CSV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The longest line this writes, with its newline and terminator. */
#define MEMORY_CSV_LINE_MAX 64

const char *memoryCsvHeader(void);

/*
 * Write one slot as a line of CSV.
 *
 * A line at a time, so a handler sending a full list never holds four
 * kilobytes of it at once.
 */
size_t memoryCsvLine(const MemoryStore *m, int slot, char *out, size_t cap);

/* What an import does with what is already stored. */
typedef enum {
  /*
   * Fill empty slots only and leave the rest alone.
   *
   * A line naming a slot that already holds a channel is counted in `kept`
   * and changes nothing. A line that cannot be read is counted in `skipped`
   * and the rest of the file still lands, because nothing that was already
   * stored can be lost this way.
   */
  MEMORY_IMPORT_MERGE = 0,
  /*
   * Throw the list away and load the file.
   *
   * All or nothing. Every line is read before anything is written, and one
   * line that cannot be read means the store is left exactly as it was, with
   * `firstBadLine` saying which line to look at. Wiping a list and then
   * stopping part way through a bad file would lose channels that the person
   * had no way of getting back.
   */
  MEMORY_IMPORT_REPLACE
} MemoryImportMode;

/* What an import did. */
typedef struct {
  uint16_t lines;    /* Data lines seen. Blanks and headers are not lines. */
  uint16_t imported; /* Channels written. */
  uint16_t skipped;  /* Data lines that could not be read. */
  uint16_t kept;     /* Merge only: lines whose slot was already filled. */
  /*
   * Names too long for a slot, which were cut short rather than refused.
   *
   * Losing a channel over a long name would cost more than it saves, but a
   * name that came back shorter than it went in is worth saying.
   */
  uint16_t truncated;
  /* Which line was refused first, counted from 1 over the whole file. */
  uint16_t firstBadLine;
} MemoryImportResult;

bool memoryImportCsv(MemoryStore *m, MemoryImportMode mode, const char *text,
                     size_t len, MemoryImportResult *out);

#ifdef __cplusplus
}
#endif

#endif /* CORE_MEMORY_CSV_H */
