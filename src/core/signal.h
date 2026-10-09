/*
 * Numbers worked out from a tuner reading, rather than read from it.
 *
 * The tuner reports a level and a noise figure. It does not report a signal
 * to noise ratio, and it does not smooth anything. Both of those are done
 * here, where they can be tested, because they decide when a feature switches
 * itself on, and a threshold that is silently wrong switches a feature off
 * with no sign of it.
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
 * The limits for the wider FM filter: a ratio above 15 dB and a level above
 * 30.0 dBuV.
 *
 * The reference firmware's own, `CN > 15 && SStatus > 300` in
 * TEF6686_ESP32.ino, which runs on this same board. They
 * hold here unchanged because the ratio they are compared with is
 * signalSnrDb's, the same line the reference works its C/N out with, and the
 * level is on the same tenths of a dBuV scale.
 */
#define SIGNAL_WIDE_MIN_SNR_DB 15
#define SIGNAL_WIDE_MIN_LEVEL_TENTHS 300

/*
 * Whether a signal is strong and clean enough for the wider FM filter,
 * which gives more treble and better stereo separation. Both above their
 * limits, strictly. Give it the smoothed level and ratio: one reading of
 * this tuner jumps far more than the signal does, and the filter would open
 * and shut several times a second on a bare one.
 */
bool signalWantsWideBandwidth(int16_t levelTenths, int16_t snrDb);

/*
 * Write a level in tenths of a dBuV as text.
 *
 * One place, so every caller prints it the same way. Dividing minus
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
 * Measured on this radio, on FM 106.40 at about 24 dBuV over twenty four
 * seconds. Replaying those readings, a screen showing whole dB off the smoothed
 * level changes twelve times with no hysteresis and once with this. A longer
 * average does worse: three seconds gives two changes and costs up to three
 * seconds of lag.
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
 * Where the top of the signal scale sits, in dBuV: a level this strong fills
 * the tuning scale's peak and the browser page's signal meter.
 *
 * The FM figures come from a 206 channel sweep of the FM band on this radio.
 * The six real stations run 26.7 to 47.5 dBuV and the 200 empty channels reach
 * 21.8 at the worst, a shoulder. Against a top of 60 the strongest reads 79 per
 * cent, so the bar spends its travel where a person can read it. Both ends of
 * the range fail on the radio: 30 makes every listenable station peg, and 70
 * leaves nothing past 68 per cent, so the top third never moves.
 *
 * The AM figures come from a 120 channel medium wave sweep, taken at night, at
 * about 01:00 local time. The strongest channel reads 35.4 dBuV and the median
 * 14.3, so against a top of 45 the strongest reads 78 per cent, within a point
 * of where 60 puts the strongest FM station.
 *
 * Two figures and not one, because the bands are about 12 dB apart: a top of
 * 60 leaves the strongest medium wave signal measured at 59 per cent and the
 * median channel at 24.
 *
 * The medium wave figure is a night one. After dark the band fills with
 * skywave from hundreds of miles away; by day the same channels carry
 * groundwave only and read far lower.
 */
#define SIGNAL_FULL_FM_DBUV 60
#define SIGNAL_FULL_AM_DBUV 45

/*
 * How far along the bar a level sits, 0 to 100.
 *
 * `fullDbuV` is the top of the scale. A level at or below zero gives nothing,
 * because the bar starts at zero dBuV and a negative reading is a dead band
 * rather than a signal pointing the other way. A level at or past the top
 * gives a hundred and no more, so the bar stops at the end of its track.
 *
 * A `fullDbuV` of zero would be a scale with no length. It answers zero
 * rather than dividing, so a caller that has not been given a scale draws an
 * empty bar instead of faulting.
 */
uint8_t signalBarPercent(int16_t levelTenths, uint8_t fullDbuV);

/*
 * A reading held still enough to read. The DX page uses it for the tuning
 * offset and the ultrasonic noise it prints.
 *
 * Shown raw, these readings cannot be read: measured on this radio over
 * four recorded runs of 300 samples each, the tuning offset takes 120 to 272
 * different values in thirty seconds, the ultrasonic noise 19 to 128 and the
 * filter width 146 to 260. The signal level has the same fault and has an
 * average and a hysteresis for it; this is that treatment for the rest.
 *
 * Both parts are needed and that is measured rather than assumed. Replaying the
 * four runs through this code, hysteresis on its own leaves up to 240 changes
 * in thirty seconds, because a noisy reading crosses the band again and again.
 * With the average in front of it the same thresholds give 0 to 21.
 *
 * `quantum` is how many raw units make one unit on the screen, so a reading
 * kept in tenths and printed whole passes 10. `hysteresis` is in raw units
 * and is how far the smoothed reading has to sit from what is shown before
 * the shown value follows.
 */
typedef struct {
  SignalAverage avg;
  int16_t shown; /* In raw units, already rounded to a whole quantum. */
  bool started;
} ReadingHold;

void readingHoldReset(ReadingHold *h);

int16_t readingHoldFeed(ReadingHold *h, int16_t sample, int16_t quantum,
                        int16_t hysteresis);

/*
 * How far each reading has to move before the screen follows it.
 *
 * One figure per reading, because they are different quantities with
 * different noise and a single constant across all three would be a guess,
 * not a measurement. Each is the smallest value in the sweep that brings
 * the two cleanly received stations in those runs to about five changes in
 * thirty seconds or fewer, which is the rate the signal level already sits
 * at. Replayed on those two: the offset changes 0 and 5 times, the noise 3
 * and 2, the width 4 and 10.
 *
 * The two stations that still change more than that, at 21 and 20, are
 * weak ones whose offset genuinely wanders over twenty four kilohertz
 * because they are weak and off tune. That movement is real and is not damped
 * away.
 *
 * The offset is also printed whole rather than to a tenth. At a tenth it
 * takes 120 to 272 values in thirty seconds and no honest dead band fixes it:
 * half a kilohertz of hysteresis still leaves 31 to 111 changes. A tenth of a
 * kilohertz is below anything a person acts on, which is the argument that
 * took the level to whole dB.
 */
#define READING_OFFSET_QUANTUM_TENTHS 10
#define READING_OFFSET_HYSTERESIS_TENTHS 15
#define READING_USN_QUANTUM_TENTHS 10
#define READING_USN_HYSTERESIS_TENTHS 20

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
