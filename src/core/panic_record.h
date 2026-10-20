/*
 * Where the last crash happened, kept across the restart it causes.
 *
 * The radio has no serial cable, so the backtrace the chip prints when it
 * panics goes nowhere. The panic handler writes this record into RTC memory
 * instead, which a panic's own restart keeps, and the next start copies it
 * to flash, which a power cycle keeps too. GET /api/state shows it, and the
 * addresses name the line with addr2line and the ELF of the build it names.
 *
 * The sealing and the checking are here, away from the hardware, because the
 * way this goes wrong is a quiet one: RTC memory holds noise after a power
 * on, and noise read as a crash sends someone looking for a fault that never
 * happened.
 */
#ifndef CORE_PANIC_RECORD_H
#define CORE_PANIC_RECORD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The backtrace kept: the place that faulted, then its return addresses. */
#define PANIC_RECORD_FRAMES 16
/* The panic's own words, such as "LoadProhibited", or an assert's with its
 * file and line. */
#define PANIC_RECORD_WHY_LEN 96
/* The task that crashed, as FreeRTOS names it, 16 with its end. */
#define PANIC_RECORD_TASK_LEN 16
/* The build it happened on, FIRMWARE_BUILD, a short hash and maybe a +. */
#define PANIC_RECORD_BUILD_LEN 16
/* Room for panicRecordJson with every field full. */
#define PANIC_RECORD_JSON_LEN 768

typedef struct {
  uint32_t marker; /* Set by panicRecordSeal, with `check`. */
  uint32_t pc;     /* Where it faulted. */
  uint32_t frames[PANIC_RECORD_FRAMES];
  uint32_t uptimeS; /* Seconds since that start. */
  uint8_t frameCount;
  uint8_t core;
  char why[PANIC_RECORD_WHY_LEN];
  char task[PANIC_RECORD_TASK_LEN];
  char build[PANIC_RECORD_BUILD_LEN];
  uint32_t check; /* A sum of everything before it. */
} PanicRecord;

/* Mark a filled record as written: end both texts and set the marker and the
 * check. Nothing else in it is touched. */
void panicRecordSeal(PanicRecord *r);

/* Whether `r` was sealed and has not changed since. False for NULL, for the
 * noise RTC memory holds after a power on, and for a cleared record. */
bool panicRecordValid(const PanicRecord *r);

/* Make `r` read as no record. */
void panicRecordClear(PanicRecord *r);

/*
 * The record as a JSON object for GET /api/state:
 * {"why":"LoadProhibited","pc":"0x400d1234","bt":["0x400d1234",...],
 *  "core":1,"task":"loopTask","up":3600,"bld":"0586e2f"}.
 * Writes "null" for a record that is not valid. Returns the length written,
 * 0 when `out` is too small, which then holds an empty string.
 */
size_t panicRecordJson(const PanicRecord *r, char *out, size_t cap);

#ifdef __cplusplus
}
#endif

#endif
