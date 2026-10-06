/*
 * How a meter bar is divided into blocks, and how its peak mark falls back.
 *
 * No drawing here and no LVGL. The radio screen's modulation meter and the
 * web page's signal meter ask how many blocks are lit, and this answers in
 * plain numbers, so the arithmetic that decides whether a reading shows at
 * all can be tested on a PC.
 *
 * That matters more than it looks. A bar of blocks quantises, so a real
 * reading can round down to nothing, and an empty bar already means "no
 * reading" on this panel. Those two must never look the same, so where the
 * line sits is settled here and not in the draw code.
 */
#ifndef CORE_METER_H
#define CORE_METER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A block and the gap after it, in pixels, as the radio ships.
 *
 * Three lit and two dark, the shape the settings struct stores and checks.
 */
#define METER_SEG_W 3
#define METER_SEG_GAP 2

/* The range settingsValid accepts for the unused meterSegW and meterSegGap
 * bytes in the settings struct. */
#define METER_SEG_W_MIN 1
#define METER_SEG_W_MAX 10
#define METER_SEG_GAP_MIN 1
#define METER_SEG_GAP_MAX 5

/*
 * The shape of a block: how wide it is, and the dark gap after it, in pixels.
 *
 * A narrower block gives more of them, so the bar reads finer and moves in
 * smaller steps. Taken far enough the divisions stop being visible at all.
 * There is no measurement that picks this: it is judged on the panel.
 *
 * No screen draws with it: the radio screen's meter has a fixed shape. The
 * range stays because the settings struct still holds the two bytes and
 * checks them, so a stored blob keeps its layout and still loads.
 */
/*
 * How many blocks are lit at this reading.
 *
 * Truncates, so 99 per cent of a 40 block bar is 39 and only a full reading
 * fills it. The one exception is at the bottom: any reading above zero lights
 * at least one block. Without that a real but small reading rounds down to an
 * empty bar, and an empty bar on this panel means the radio has nothing to
 * report, so a quiet station and a dead tuner would look identical.
 */
uint8_t meterSegmentsLit(uint8_t percent, uint8_t count);

/*
 * The bar itself, damped.
 *
 * The reading moves faster than a person can read it. Measured on this radio
 * over 300 samples of speech on 101.90 and 300 of music on 106.40: the
 * modulation reading moves a mean of 17.0 and 17.6 points of a hundred between
 * one reading and the next. A bar following that shows every transient and
 * settles on nothing, so a loud passage and a single peak look the same.
 *
 * So the bar rises to a reading at once and falls back slowly, which is what a
 * level meter has always done. Rising at once matters: over both runs, with an
 * instant rise no peak in the reading fails to reach the bar at any fall time
 * tested, and a rise taken over 300 ms starts losing them.
 *
 * The value is not capped at a hundred here. Modulation past reference
 * deviation is real and routine: 122 of 572 recorded AGC samples read over
 * 100 and the loudest read 153. A caller that shows only a bar, as the radio
 * screen does, caps the reading at 100 before feeding it, so the bar does
 * not stay full while it falls back from the excess.
 */
typedef struct {
  uint16_t percent; /* Where the bar is now. May be past 100. */
  bool valid;       /* False until a reading has arrived. */
  uint32_t fellMs;  /* When the fall last took a step. */
} MeterBar;

/* Forget the bar, for a reading that stopped arriving. */
void meterBarReset(MeterBar *b);

/*
 * Feed one reading, and say how long the bar should be drawn.
 *
 * `fallFullMs` is how long the bar takes to fall a hundred, so the speed is a
 * time and not a step per call. That is not a nicety here: the radio task
 * reads the tuner every 100 ms nominally and in the same runs the gap
 * between readings measures 48 to 254 ms, so a step per call would make the
 * bar's speed follow whatever else the task was doing. A value above a
 * hundred falls at that same rate rather than faster.
 */
uint16_t meterBarFeed(MeterBar *b, uint16_t percent, uint32_t nowMs,
                      uint32_t fallFullMs);

/*
 * The peak mark on the modulation bar.
 *
 * It jumps to a new peak at once, sits still for a while, then falls back at
 * a steady rate. Every peak programme meter ever built works this way: the
 * jump is the measurement, the hold is what makes it readable, and the fall
 * is what stops it reading as a mark that has stuck.
 */
typedef struct {
  MeterBar bar;    /* The mark, which falls as the bar does once held. */
  uint32_t heldMs; /* When the mark was last pushed up. */
} MeterPeak;

/* Forget the mark. A reading that stopped arriving must not leave a level on
 * screen that nothing is measuring any more. */
void meterPeakReset(MeterPeak *p);

/*
 * Feed one reading, and say where the mark should be drawn.
 *
 * `holdMs` is how long the mark sits still after a peak. `fallFullMs` is how
 * long it takes to fall the whole bar, so the speed is a time and not a step
 * per call: the caller's poll rate can change without changing how the meter
 * behaves. A `fallFullMs` of 0 makes the mark drop as soon as the hold ends.
 *
 * Both are times rather than dB rates because this bar is linear in
 * modulation per cent. IEC 60268-10 specifies 1.7 s for a 20 dB fall on a
 * Type I meter and 2.8 s on a Type II, and those durations are what this
 * borrows. The law is not the same, and nothing here should claim it is.
 */
void meterPeakFeed(MeterPeak *p, uint8_t percent, uint32_t nowMs,
                   uint32_t holdMs, uint32_t fallFullMs);

#ifdef __cplusplus
}
#endif

#endif /* CORE_METER_H */
