/*
 * The band scope's sweeps, outside DX mode: the whole band the radio is on,
 * or a span round the dial. Read through DX mode's width, the width the sweep
 * was measured at, so a level means here what it means on DX mode's Scope
 * page.
 *
 * Loop task only.
 */
#ifndef SCOPE_TASK_H
#define SCOPE_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/dx_sweep.h"
#include "core/settings.h"

/* Keep the live settings, for DX mode's width. */
void scopeTaskBegin(const Settings *settings);

typedef enum {
  SCOPE_STARTED = 0,
  SCOPE_NO_MEMORY, /* No room on the heap for the two sweeps. */
  SCOPE_REFUSED,   /* The radio cannot sweep now, see radioSweepStart. */
} ScopeStart;

/* Start a sweep of the band the radio is on: `spanKHz` 0 for the whole band,
 * or that span round the dial. */
ScopeStart scopeTaskSweep(uint32_t spanKHz);

typedef struct {
  const DxSweep *latest; /* The last sweep that finished, or NULL. */
  bool whole;            /* `latest` is the whole band, not a span. */
  bool running;          /* A sweep is waiting or running. */
  bool abandoned;        /* The last one asked for ended early, by a key or a
                          * tune, and `latest` is the one before it. */
  uint16_t revision;     /* Moves whenever `latest` changes. */
} ScopeView;

/* The band scope as it stands, with a sweep that has just finished taken
 * first. */
void scopeTaskView(ScopeView *out);

/* Take a finished sweep from the radio task. From loop(). */
void scopeTaskPoll(void);

#endif /* SCOPE_TASK_H */
