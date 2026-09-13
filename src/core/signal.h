/*
 * Numbers worked out from a tuner reading, rather than read from it.
 *
 * The tuner reports a level and a noise figure. It does not report a signal
 * to noise ratio, and it does not smooth anything. Both of those are done
 * here, where they can be tested, because they decide when a feature switches
 * itself on and a threshold that is silently wrong is the failure this
 * project keeps finding.
 */
#ifndef CORE_SIGNAL_H
#define CORE_SIGNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Signal to noise, in dB, from a level and an ultrasonic noise reading.
 *
 * The chip does not report this. The working PE5PVB firmware computes it in
 * its own driver with a fitted straight line, and this is that line:
 *
 *     snr = 0.46222375 * level - 0.082495118 * noise + 10
 *
 * with both inputs in dB and per cent rather than tenths. Those constants are
 * somebody's fit against measurements, not a formula from the datasheet. They
 * are copied rather than re-derived because the thresholds that use the
 * answer were chosen against this exact line, and changing one without the
 * other moves the point where features switch on with nothing to show for it.
 *
 * The level is clamped the way that firmware clamps it, to -20 and 120 dBuV,
 * before the line is applied.
 *
 * The AM side reports its noise on a different scale, and the reference
 * divides that field by fifty before putting it through the same line. Which
 * band the reading came from therefore has to be said.
 */
int8_t signalSnrDb(int16_t levelTenths, uint16_t noiseTenths, bool fm);

/*
 * Write a level in tenths of a dBuV as text.
 *
 * One place, because there were three and they did not agree. Dividing minus
 * five tenths by ten gives zero, so a level just below zero shows as "0.0"
 * unless the sign is taken before the value is split. A dead band really does
 * read a little below zero, so this matters on exactly the readings a person
 * is squinting at.
 */
void signalFormatLevel(int16_t tenths, char *out, size_t outLen);

/*
 * A running average. Zero it before first use.
 *
 * One reading of this tuner jumps about far more than the signal does, and a
 * feature that switches on a bare reading switches on and off several times a
 * second. This is the same smoothing the reference firmware applies before it
 * makes any such decision.
 */
typedef struct {
  int32_t accumulator; /* Ten times the smoothed value. */
  bool started;        /* A first sample has been taken. */
} SignalAverage;

/*
 * Add a sample and get the smoothed value.
 *
 * The first sample is taken as the answer rather than being averaged up to
 * from zero. That only helps if the average is started again when the old
 * readings stop meaning anything, which is what signalAverageReset is for:
 * a band change makes every reading before it irrelevant, and without the
 * reset the smoothing carries the old band's numbers for about two seconds.
 */
int16_t signalAverage(SignalAverage *avg, int16_t sample);

/*
 * How far the level has to move from the number on the screen before that
 * number changes, in tenths of a dB.
 *
 * Measured on this radio on 13 September 2026, on FM 106.40 at about 24 dBuV
 * over twenty four seconds. Replaying that capture, a screen showing whole dB
 * off the smoothed level changed twelve times with no hysteresis and once
 * with this. A longer average was tried against it and did worse: three
 * seconds gave two changes and cost up to three seconds of lag.
 *
 * It is the distance from the shown value, so with the half dB of rounding
 * the level has to move a whole dB before the digit follows.
 */
#define SIGNAL_DISPLAY_HYSTERESIS_TENTHS 5

/*
 * The signal level as a person reads it. Zero it before first use.
 *
 * Smoothing the value is not enough on its own to make a screen sit still.
 * The smoothed level still moves a tenth or two between readings, and a
 * screen printing a decimal place changes the last digit ten times a second,
 * which reads as flicker whatever the number underneath is doing. So the
 * screen is given whole dB, and the whole dB is held until the level has
 * moved clear of it.
 *
 * A tenth of a dB is below anything a person acts on. The tenths are still
 * in the state document, where a script wants them.
 */
typedef struct {
  int16_t shownDb; /* The whole dBuV currently on the screen. */
  bool started;    /* A first reading has been taken. */
  /*
   * The dial has moved and the reading has not caught up yet.
   *
   * The two do not move together. The dial moves as soon as the command is
   * worked through; the reading keeps its own cadence. So there is a moment
   * where the frequency is the new one and the level is still the old
   * station's, and starting again on the frequency alone would latch the
   * station that was just left.
   */
  bool waitingForStation;
} SignalDisplay;

int16_t signalDisplayLevel(SignalDisplay *d, int16_t smoothedTenths,
                           bool fresh);

/*
 * The dial has moved.
 *
 * The number on the screen is kept until a reading arrives from where the
 * dial now is, and that reading is then taken as it stands rather than
 * having to climb out of the old station's hysteresis band. Two stations a
 * dB apart would otherwise leave the screen showing the one you left.
 */
void signalDisplayStationChanged(SignalDisplay *d);

/*
 * Forget what is on the screen, so the next reading is taken as it stands.
 *
 * For a fresh start rather than a change of station, such as the screen
 * coming up. signalDisplayStationChanged is the one to use when the dial
 * moves, because it waits for the reading to catch up.
 */
void signalDisplayReset(SignalDisplay *d);

/*
 * Forget everything this average has seen.
 *
 * For when the readings stop meaning what they did, such as a change of band
 * or a retune. The next sample is then taken as the answer rather than
 * averaged in with readings from somewhere else on the dial.
 */
void signalAverageReset(SignalAverage *avg);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SIGNAL_H */
