/*
 * A sliding tuning scale: where its marks go around the tuned frequency.
 *
 * The radio screen draws a scale under the line of radio text the way the
 * ATS-20, ATS-25 and ATS-Mini radios do. The tuned frequency stays in the
 * middle of the row and the scale moves under it as the radio tunes. The middle
 * is one amber mark, and on a station the marks nearest it stand up to a peak
 * as tall as the signal; between stations the row stays flat. There is a mark
 * every 100 kHz on the FM bands and every 10 kHz on the AM bands, SCALE_TICK_PX
 * apart, long every tenth one, medium every fifth one half way between, short
 * between those. Each long mark carries its number: whole megahertz on FM,
 * kilohertz on AM. The scale is a loop, because the tuning is: a step up from
 * the band's top channel lands on its bottom one, `bandStepUp`, and a step down
 * from the bottom on the top. So right of 108.0 MHz come 87.5, 87.6 and on.
 * Between the two ends sit SCALE_SEAM_TICKS dummy marks, short and with no
 * number, so the numbers at the two ends never run into each other, and the
 * seam, where the ends meet, is the middle of them.
 *
 * Pure arithmetic on frequencies and pixels, so it is tested on a PC. The
 * panel only draws what it is given.
 */
#ifndef CORE_SCALE_H
#define CORE_SCALE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* What kind of mark a tick is. */
typedef enum {
  SCALE_SHORT = 0, /* Any other tick. */
  SCALE_MEDIUM,    /* Every fifth tick, half way between two long marks. */
  SCALE_LONG,      /* Every tenth tick, with a number. */
} ScaleMark;

/*
 * Pixels between two ticks. 8, the ATS radios' own spacing on the same 320
 * pixel screen: a 100 kHz FM step moves the scale 8 pixels and a 9 kHz MW
 * step 7, so every step is seen to move it.
 */
#define SCALE_TICK_PX 8

/* The gap between two ticks, in kHz, on the FM bands and on the AM bands. */
#define SCALE_FM_TICK_KHZ 100
#define SCALE_AM_TICK_KHZ 10

/*
 * How far past either end of the row ticks are still given, in pixels, so a
 * number on a mark just off the row is drawn in part and scrolls in and out
 * with the scale rather than appearing whole. Wider than half the widest
 * number, 27000.
 */
#define SCALE_EDGE_PX 24

/*
 * The most ticks a row can hold. The widest row this firmware draws is 320
 * pixels, and with SCALE_EDGE_PX either side that is 46 ticks 8 pixels apart.
 */
#define SCALE_MAX_TICKS 46

/*
 * How many dummy marks sit between the band's last tick and its first. Four
 * puts the two ends 40 pixels apart, room for two five digit numbers, 27000
 * and 1700 at the ends of SW, with a gap between. An even count keeps the
 * seam between two dummies rather than on one.
 */
#define SCALE_SEAM_TICKS 4

/* The most seams a row can show: twice for a band whose loop is shorter than
 * the row, which none of the built in bands is. */
#define SCALE_MAX_SEAMS 2

/* One mark of the scale. Six bytes, since the panel keeps a row of them
 * in static RAM, which is the scarcer. */
typedef struct {
  int16_t x;       /* Pixels from the left end of the row. */
  uint16_t number; /* What a long mark says: MHz on FM, kHz on AM, which
                    * tops out at 27000 on SW. 0 for a dummy, which says
                    * nothing. */
  uint8_t mark;    /* How tall it is drawn, a ScaleMark. */
  bool inBand;     /* A real tick of the band. False for a dummy at the seam,
                    * which is always short. */
} ScaleTick;

/*
 * The peak: the four marks nearest the middle on each side stand taller with
 * the signal, each one SCALE_PEAK_STEP_PERCENT shorter than the one inside
 * it, and the nearest reaches SCALE_PEAK_MAX_PX, the scale's full height, at
 * a full reading. These numbers set the look; nothing measured decides them.
 */
#define SCALE_PEAK_RANKS 4
#define SCALE_PEAK_MAX_PX 32
#define SCALE_PEAK_STEP_PERCENT 22

/*
 * Whether a mark `dxPx` pixels from the middle of the row sits under the
 * middle mark, which is 2 pixels wide and always drawn. Such a mark is not
 * drawn, so the middle reads as one mark.
 */
bool scaleUnderMiddle(int32_t dxPx);

/*
 * Which of the peak's marks a mark `dxPx` pixels from the middle is: 0 for
 * the nearest on its side, up to SCALE_PEAK_RANKS - 1, or -1 for one outside
 * the peak or under the middle mark. Marks are a tick apart, so exactly one
 * mark on each side has each rank, wherever the dial sits between two.
 */
int scalePeakRank(int32_t dxPx);

/*
 * The farthest a peak mark sits from the middle, in pixels, the same either
 * side. A mark further out than this never changes with the signal, so a new
 * level only needs the marks within it drawn again.
 */
int32_t scalePeakReachPx(void);

/*
 * How tall a mark of `rank` stands at `levelPercent`, the signal on the signal
 * scale of signalBarPercent, 0 to 100, in pixels: 0 outside the peak and with
 * no signal. Over 100 counts as 100. A mark is drawn at least as tall as its
 * own kind, so a long mark inside a low peak stays long.
 */
int16_t scalePeakPx(int rank, uint8_t levelPercent);

/*
 * Every tick that falls on a row `widthPx` wide with `tunedKHz` at its middle,
 * `widthPx / 2`, left to right: x from -SCALE_EDGE_PX to `widthPx - 2 +
 * SCALE_EDGE_PX`, going round the loop past either end of the band from
 * `lowKHz` to `highKHz`. Writes no more than `max` and returns how many it
 * wrote: 0 for a band with no width or no tick in it, a row with no pixels
 * or no place to write them.
 */
int scaleTicks(uint32_t lowKHz, uint32_t highKHz, uint32_t tunedKHz, bool fm,
               int16_t widthPx, ScaleTick *out, int max);

/*
 * Where the seam falls on the same row, x from 0 to `widthPx - 1`, left to
 * right. Writes no more than `max`, at most SCALE_MAX_SEAMS, and returns how
 * many it wrote: 0 when the seam is out of view, and in every case where
 * scaleTicks gives no ticks.
 */
int scaleSeams(uint32_t lowKHz, uint32_t highKHz, uint32_t tunedKHz, bool fm,
               int16_t widthPx, int16_t *out, int max);

/*
 * The frequency under the middle after a finger dragged the scale `dxPx`
 * pixels, `fromKHz` being under it when the drag began. The scale moves with
 * the finger, so a drag to the left brings higher frequencies to the middle:
 * a tick, 100 kHz on FM and 10 kHz on AM, for every SCALE_TICK_PX, counted
 * towards zero so a finger has to move a whole tick before the dial does.
 * Round the loop as the scale draws it, from `highKHz` on to `lowKHz`; a drag
 * that ends in the gap the scale draws between the two lands on the nearer
 * end.
 * `fromKHz` itself for a band with no tick in it. Not on any band's channel
 * grid: the caller snaps it to that.
 */
uint32_t scaleDragKHz(uint32_t lowKHz, uint32_t highKHz, uint32_t fromKHz,
                      bool fm, int32_t dxPx);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SCALE_H */
