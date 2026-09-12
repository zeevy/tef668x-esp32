/**
 * @file signal.h
 * @brief Numbers worked out from a tuner reading, rather than read from it.
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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
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
 *
 * @param levelTenths  Signal level in tenths of a dBuV.
 * @param noiseTenths  The noise field from the quality reading.
 * @param fm           true for an FM reading, false for AM.
 * @return The ratio in dB.
 */
int8_t signalSnrDb(int16_t levelTenths, uint16_t noiseTenths, bool fm);

/**
 * A running average. Zero it before first use.
 *
 * One reading of this tuner jumps about far more than the signal does, and a
 * feature that switches on a bare reading switches on and off several times a
 * second. This is the same smoothing the reference firmware applies before it
 * makes any such decision.
 */
typedef struct {
  int32_t accumulator; /**< Ten times the smoothed value. */
  bool started;        /**< A first sample has been taken. */
} SignalAverage;

/**
 * Add a sample and get the smoothed value.
 *
 * The first sample is taken as the answer rather than being averaged up to
 * from zero. That only helps if the average is started again when the old
 * readings stop meaning anything, which is what signalAverageReset is for:
 * a band change makes every reading before it irrelevant, and without the
 * reset the smoothing carries the old band's numbers for about two seconds.
 *
 * @param avg     The average.
 * @param sample  The reading.
 * @return The smoothed value, in the same units as the sample.
 */
int16_t signalAverage(SignalAverage *avg, int16_t sample);

/**
 * Forget everything this average has seen.
 *
 * For when the readings stop meaning what they did, such as a change of band
 * or a retune. The next sample is then taken as the answer rather than
 * averaged in with readings from somewhere else on the dial.
 *
 * @param avg  The average.
 */
void signalAverageReset(SignalAverage *avg);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SIGNAL_H */
