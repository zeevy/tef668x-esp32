/*
 * How busy a CPU core was between two readings, in per cent.
 *
 * FreeRTOS counts how long each task has run, on the same microsecond clock
 * the run time stats use. The time a core did not spend in its idle task is
 * the time it was busy. Nothing here reads the scheduler; core/ never does.
 */
#ifndef CORE_CPU_LOAD_H
#define CORE_CPU_LOAD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One reading: the core's idle task run time, and the run time clock read
 * at the same moment. Both are 32 bit microsecond counters that wrap about
 * every 71 minutes. */
typedef struct {
  uint32_t idleUs;
  uint32_t clockUs;
} CpuSample;

/*
 * The share of the time from `before` to `now` the core spent outside its
 * idle task, 0 to 100, rounded to the nearest whole per cent.
 *
 * A counter that wrapped once between the two readings still gives the
 * right answer. The idle counter only moves when the idle task is switched
 * out, so it can run a little behind the clock, and idle time longer than
 * the window reads as 0 rather than as a number past the end.
 *
 * Returns false, and leaves `out` alone, for a NULL or when no time passed
 * between the two readings, since there is then no share to give.
 */
bool cpuBusyPercent(const CpuSample *before, const CpuSample *now,
                    uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif /* CORE_CPU_LOAD_H */
