/*
 * Auto off: the radio goes to sleep once it has been left alone for a set
 * time.
 *
 * Every key, knob turn or write from the browser starts the count again.
 * While it is on the header shows a person in bed, which turns to `radio`
 * for the last five minutes; in the last thirty seconds the sound fades
 * down, and at the end the radio sleeps. Any use before then puts it back to awake at
 * once.
 *
 * This decides which of those it is and nothing else. What counts as use,
 * the fade itself and the sleep are the caller's.
 */
#ifndef CORE_AUTO_OFF_H
#define CORE_AUTO_OFF_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* How long before the radio sleeps the sleep mark turns to `radio`. Five
 * minutes is long enough to be seen across a room before the fade starts. */
#define AUTO_OFF_WARN_MS 300000UL

/*
 * How long the sound takes to fade down before the radio sleeps.
 *
 * Long enough to be heard as a fade and not as the sound being cut, which is
 * what makes a sleep timer unpleasant, and short enough that a person who
 * wants to keep listening is still awake to press a key.
 */
#define AUTO_OFF_FADE_MS 30000UL

/* The longest it may be set to, in minutes: ten hours. Any whole minute up
 * to it is taken, so one minute can be set to try it out. */
#define AUTO_OFF_MAX_MINUTES 600

typedef enum {
  AUTO_OFF_AWAKE = 0, /* Off, or more than five minutes to go. */
  AUTO_OFF_WARN,      /* The last five minutes: the sleep mark is in `radio`. */
  AUTO_OFF_FADE,      /* The last thirty seconds: the sound fades too. */
  AUTO_OFF_SLEEP      /* Time is up. */
} AutoOffPhase;

typedef struct {
  uint16_t minutes;   /* 0 for off. */
  uint32_t lastUseMs; /* When the count last started. */
} AutoOff;

/* Off, or 1 to AUTO_OFF_MAX_MINUTES. Anything longer is refused. */
bool autoOffMinutesOk(uint16_t minutes);

void autoOffInit(AutoOff *a, uint16_t minutes, uint32_t nowMs);

/*
 * Change the time, and start the count again.
 *
 * Choosing a time is using the radio, and a change from 60 to 15 minutes
 * after forty minutes alone must not send it to sleep the moment it is made.
 */
void autoOffSetMinutes(AutoOff *a, uint16_t minutes, uint32_t nowMs);

/* Somebody used the radio. */
void autoOffUsed(AutoOff *a, uint32_t nowMs);

AutoOffPhase autoOffPhase(const AutoOff *a, uint32_t nowMs);

/*
 * How long until the radio sleeps, in milliseconds, into `out`.
 *
 * False when auto off is off, so "no time left" and "never" cannot be read
 * as the same zero.
 */
bool autoOffLeftMs(const AutoOff *a, uint32_t nowMs, uint32_t *out);

#ifdef __cplusplus
}
#endif

#endif /* CORE_AUTO_OFF_H */
