/**
 * @file agc.h
 * @brief Evening out how loud one station is against the next.
 *
 * Stations are transmitted at different modulation depths, so one sounds
 * louder than the next at the same volume setting. The tuner reports the
 * modulation depth as a percentage, and a slow average of that is a good
 * measure of how loud a station is. This works out how many dB to add or take
 * away to bring them all towards one target.
 *
 * It decides a number and nothing else. Whether that number reaches the chip,
 * and how it sits alongside the volume the knob asked for, is somebody else's
 * job. That is what keeps the awkward parts testable on a PC: the settling
 * after a tune, the readings that have to be thrown away, and the release when
 * there is nothing measurable to work from.
 *
 * The design and every constant here come from a prototype built on the
 * working PE5PVB firmware and run on this radio in September 2026, and the
 * captures it produced are in `test/fixtures/agc/`.
 *
 * Not every constant is verified by those captures, and the ones that are not
 * say so on themselves. The captures were taken on real stations, so they
 * carry nothing about an empty channel and nothing about a reading that has
 * wrapped. Saying which is which matters more than a tidy claim that they are
 * all measured.
 */
#ifndef CORE_AGC_H
#define CORE_AGC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Quietest target a person may ask for, in modulation percent. */
#define AGC_TARGET_MIN 30

/** Loudest. Above this there is nothing left to bring down. */
#define AGC_TARGET_MAX 80

/** The most the AGC will ever turn a station down, in dB. */
#define AGC_MAX_CUT (-12)

/** The most boost a person may ask for, in dB. */
#define AGC_BOOST_MAX 8

/**
 * Below this the station is silent and the reading says nothing.
 *
 * A speech pause is not a quiet station, and averaging silence in would drag
 * the gain up and then leave it there when the talking starts again. What is
 * wanted is how loud a station is while it is playing.
 */
#define AGC_MOD_MIN 10

/**
 * Above this the reading is not believable.
 *
 * Real over modulation reaches 153 per cent in the captures, so this is well
 * clear of anything genuine. It exists because the chip reports modulation in
 * an unsigned word and a negative reading comes back as a number in the
 * thousands. The driver already turns that back into the small negative it
 * was, which `AGC_MOD_MIN` then rejects, so this is the second line rather
 * than the first. No capture contains a reading above 153, so this guard is
 * justified by what the chip does and not by the fixtures.
 */
#define AGC_MOD_MAX 200

/** The signal has to reach this, in dBuV, before a reading is measured. */
#define AGC_MIN_SIGNAL 8

/**
 * Above this ultrasonic noise the reading is noise, not audio. FM only.
 *
 * In tenths of a per cent, matching the driver. Noise reads as heavy
 * modulation, so a channel with no station on it would otherwise have the AGC
 * measure the noise and work a gain out from it.
 *
 * Measured from `test/fixtures/squelch/shoulder-2026-09-13.log`, which is the
 * only capture here that holds real stations, the channel beside a strong
 * station, and an empty channel, all sampled the same way. Over its 360
 * station readings the highest noise is 133 tenths, so 140 is the largest
 * limit that rejects none of them.
 *
 * It is a second line and not the first. Of the 180 shoulder and empty
 * readings in that capture, 74 get past the level gate, and this rejects 34
 * of those. The level gate does the rest and more. A limit of 120 would
 * reject 47 of the 74 but would also throw away two real station readings,
 * and rejecting a station is the side worth avoiding even though the cost
 * here is only a skipped sample.
 *
 * The prototype's own figure was ten times this and rejected nothing in any
 * capture in this repository, so the gate it appeared to provide was not
 * there. That is why this number is measured here rather than inherited.
 */
#define AGC_MAX_NOISE_TENTHS 140

/** Updates with nothing measurable before the gain is walked back to zero. */
#define AGC_IDLE_TICKS 50

/** Updates needed before the gain is allowed to move at all. */
#define AGC_MIN_TICKS 5

/** Updates after a tune that use the fast average. */
#define AGC_SETTLE_TICKS 20

/** Fast average weight, one part in this. */
#define AGC_FAST_DIV 4

/** Slow average weight, one part in this. */
#define AGC_SLOW_DIV 64

/** dB the gain moves per update. */
#define AGC_STEP_DB 1

/**
 * How far out the gain has to be before it moves, in dB.
 *
 * Wider than one step on purpose. Without it the gain toggles by a dB every
 * update whenever the average sits on a rounding boundary, which is heard as
 * tremolo and writes to the chip on every tick.
 */
#define AGC_DEADBAND_DB 2

/** What the AGC is aiming for and how far it may go. */
typedef struct {
  /**
   * The modulation depth to bring every station towards, as a percentage.
   *
   * 0 switches the AGC off. A lower target evens the stations out more,
   * because every station above it is brought down and only the ones below
   * are left alone, but it also makes everything quieter. That trade belongs
   * to the person listening, which is why it is a number and not a switch.
   */
  uint8_t targetPercent;
  /**
   * The most the AGC may add to a quiet station, in dB. 0 is cut only.
   *
   * Cutting alone brings the loud stations down to the target and leaves the
   * quiet ones where they are, so they stay quieter than the rest. Boost
   * brings those up as well, and it is off by default because it clips a
   * station with a low average and full peaks, and it lifts the noise on a
   * weak signal. The price of cut only is that everything plays a little
   * quieter, so the amplifier is turned up once and left alone.
   */
  uint8_t boostDb;
} AgcConfig;

/** One reading, as much of it as the AGC cares about. */
typedef struct {
  bool valid;                /**< The reading came back. */
  int16_t modulationPercent; /**< Modulation depth. Signed: see AGC_MOD_MAX. */
  int16_t levelTenths;       /**< Signal level, tenths of a dBuV. */
  uint16_t noiseTenths;      /**< Ultrasonic noise, tenths of a per cent. FM. */
  bool fm;                   /**< The noise test only means something on FM. */
  /**
   * The audio is playing and the dial is still.
   *
   * A seek walking the band, a shut squelch and a deliberate mute all mean
   * the reading describes nothing anybody is listening to. The caller knows
   * about all three and the AGC does not, so it is told.
   */
  bool listening;
  /**
   * This reading came off the chip, rather than being the last one again.
   *
   * The AGC runs on a steady cadence and the tuner is not always read at the
   * same rate: on a weak FM signal it is read every 300 ms while the AGC
   * ticks every 100 ms, so the same value arrives three times over. Averaged
   * in three times it would make the settling guard pass on a third of the
   * evidence it asks for.
   *
   * The caller says, rather than the AGC comparing this reading against the
   * last one. A station that genuinely holds the same whole per cent for a
   * while is indistinguishable from a repeat by value, and guessing would
   * stall the AGC on exactly the steady station it handles best. A caller
   * with no way to tell passes true and accepts the triple counting.
   */
  bool fresh;
} AgcReading;

/** What the AGC has seen. Zero it before first use. */
typedef struct {
  /**
   * The running average of the modulation, in thousandths of a per cent.
   *
   * Slow on purpose, or the gain would follow the music and pump. For the
   * first AGC_SETTLE_TICKS after a tune it runs fast, so a new station
   * settles in about two seconds, and then it goes slow and stays there.
   *
   * Thousandths rather than tenths, and that is not spare precision. The
   * average moves by a divided difference, and with the slow divisor of 64 a
   * difference held in tenths divides to nothing whenever it is under 6.4
   * modulation per cent. The average then freezes and the gain never moves
   * again, which is the whole feature stopped. In thousandths the same
   * division only loses a difference under 0.064 per cent, which is far below
   * anything the chip reports.
   */
  int32_t averageMilli;
  uint8_t ticks; /**< Usable updates since the last tune, held at the cap. */
  uint8_t idle;  /**< Updates with nothing to measure, held at the cap. */
  int8_t gainDb; /**< What the AGC is adding now. */
} Agc;

/**
 * Start an AGC, adding nothing.
 *
 * @param a  The AGC. Cleared.
 */
void agcInit(Agc *a);

/**
 * The dial has moved, so everything measured before it says nothing.
 *
 * The average starts again and the settling runs fast for a while, which is
 * what makes a new station reach its level in about two seconds. The gain
 * itself is left where it is: walking it to zero on every retune would be
 * heard as the volume jumping about as the dial crosses the band.
 *
 * @param a  The AGC.
 */
void agcRetuned(Agc *a);

/**
 * Take one reading and say what gain to apply.
 *
 * Call it on a steady cadence. The settling, the release and the step rate
 * are all counted in calls, not in milliseconds, so a caller that changes its
 * rate changes how fast the AGC moves.
 *
 * @param a    The AGC.
 * @param cfg  The target and the boost. NULL, or a target of 0, means off.
 * @param r    The reading. NULL counts as nothing measurable.
 * @return The gain to add to the volume, in dB. 0 when the AGC is off.
 */
int8_t agcUpdate(Agc *a, const AgcConfig *cfg, const AgcReading *r);

/**
 * The gain the AGC is adding now, without taking a reading.
 *
 * @param a  The AGC.
 * @return The gain in dB. 0 for NULL.
 */
int8_t agcGain(const Agc *a);

/**
 * The gain the average is asking for, before the step rate and the deadband.
 *
 * Exposed because it is the number the whole thing turns on, and a caller
 * that shows only the gain cannot tell a station that has settled from one
 * that is still moving towards its level.
 *
 * @param a    The AGC.
 * @param cfg  The target and the boost.
 * @return The wanted gain in dB, or 0 when there is no usable average yet.
 */
int8_t agcWantedGain(const Agc *a, const AgcConfig *cfg);

/**
 * The running average of the modulation, in tenths of a per cent.
 *
 * The stored form is finer than this, for reasons on the field itself. This
 * is the number a person or a capture would recognise.
 *
 * @param a  The AGC.
 * @return The average in tenths of a per cent. 0 for NULL.
 */
int16_t agcAverageTenths(const Agc *a);

/**
 * Whether the average is worth acting on yet.
 *
 * A couple of samples are not a loudness measurement, and acting on them
 * means a loud or quiet moment at the instant of tuning sends the gain the
 * wrong way, which is heard as a swoop.
 *
 * @param a  The AGC.
 * @return true once enough usable readings have arrived.
 */
bool agcSettled(const Agc *a);

#ifdef __cplusplus
}
#endif

#endif /* CORE_AGC_H */
