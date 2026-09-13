/**
 * @file seek.h
 * @brief Deciding whether the radio has landed on a station.
 *
 * Pure logic, no hardware, so the decision can be replayed against real
 * readings on a PC. That matters more here than anywhere else in this
 * project: a seek that is slightly too fussy skips the station you wanted and
 * looks like a broken band, and one that is slightly too loose stops on the
 * shoulder of the station it just left. Neither fails, so neither shows up
 * except by listening.
 *
 * The thresholds come from a sweep of the whole FM band taken off this radio,
 * kept in `test/fixtures/seek/`. They are not picked. What that sweep shows:
 *
 * | | Real stations | Everything else |
 * |---|---|---|
 * | Ultrasonic noise | 6 to 31 | 52 and up |
 * | Multipath | 29 to 36 | 160 and up |
 * | Signal level | 267 to 475 | up to 218 |
 *
 * So noise and multipath separate cleanly, with a wide empty gap that nothing
 * sits in, and signal level does not: the shoulders either side of a strong
 * station read higher than a real but quieter station does. A seek built on
 * level alone stops on the edges of the station it just left.
 *
 * Level still earns a gate of its own, as the third of three, because it
 * catches what the other two cannot. Running the first version of this on the
 * radio it stopped on 102.0 MHz, which sits beside the strongest station on
 * the band. Its noise reads 25 to 87 and its multipath under 130, both well
 * inside the limits, because there is real signal there: the sidebands of
 * 101.9 next door. What it does not have is level, at -1.8 dBuV against 19.6
 * for the weakest real station.
 *
 * Each gate rejects a different thing, and none of the three is enough alone:
 *
 * | Gate | Rejects |
 * |---|---|
 * | Noise | an empty channel, which is all hiss |
 * | Multipath | a channel that is mostly reflections |
 * | Level | the shoulder of a strong station, which is quiet but clean |
 *
 * The multipath limit is the loosest of the three, and deliberately. A real
 * but weak station with reflections is the case a seek must never skip, and
 * on this radio 95.0 MHz is one: it read a multipath of 36 in the sweep and
 * 204 to 279 the next day, on both days a station. Level is what rejects the
 * things that are not stations, so multipath does not have to.
 */
#ifndef CORE_SEEK_H
#define CORE_SEEK_H

#include <stdbool.h>
#include <stdint.h>

#include "band_plan.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The fussiest sensitivity the radio offers. */
#define SEEK_SENSITIVITY_MIN 1
/** The loosest. Finds weak stations, and stops on a lot that is not one. */
#define SEEK_SENSITIVITY_MAX 6

/**
 * The default, which is the reference firmware's default.
 *
 * On the sweep in `test/fixtures/seek/` this stops on all six stations that
 * were on air and on nothing else, with room to spare on both gates: the
 * worst station uses 31 of the 120 it is allowed for noise and 36 of the 200
 * for multipath.
 */
#define SEEK_SENSITIVITY_DEFAULT 4

/**
 * How far off centre a carrier may read and still count, in tenths of a kHz.
 *
 * Wide, and deliberately. This radio reports every FM station as 5 to 7 kHz
 * high, because its tuner crystal runs about 50 ppm fast. That is measured
 * and written up in HARDWARE.md, and it is the same on the firmware this
 * project replaces.
 *
 * The reference firmware's seek allows plus or minus 80 here. Against a bias
 * of plus 42 to plus 84 that has almost no margin left, and in the sweep
 * 106.4 MHz read plus 84 and would have been skipped. A gate that quietly
 * skips a real station is exactly the failure this project keeps finding, so
 * this window is wide enough that the bias cannot reach it, and the real work
 * is left to noise and multipath.
 *
 * The AM window stays tight. The same 50 ppm at 738 kHz is 0.04 kHz, far too
 * small to see, so there is no bias to allow for on that side.
 */
#define SEEK_OFFSET_FM_TENTHS 200
/** The same on the AM side, where there is no crystal bias to allow for. */
#define SEEK_OFFSET_AM_TENTHS 20

/** What a seek has to beat to stop. */
typedef struct {
  uint8_t fmSensitivity; /**< 1 to 6. Higher stops on weaker signals. */
  uint8_t amSensitivity; /**< 1 to 6. Separate, as the old firmware has. */
} SeekConfig;

/**
 * The sensitivities this radio ships with.
 *
 * @param out  Receives the defaults.
 */
void seekDefaults(SeekConfig *out);

/** One reading, as much of it as the stop decision cares about. */
typedef struct {
  bool valid;               /**< The reading came back. False never stops. */
  int16_t levelTenths;      /**< Signal level, tenths of a dBuV. */
  uint16_t noiseTenths;     /**< Ultrasonic noise. */
  uint16_t multipathTenths; /**< Multipath. FM only, ignored on AM. */
  int16_t offsetTenths;     /**< How far off centre, tenths of a kHz. */
} SeekReading;

/**
 * Whether the radio should stop here.
 *
 * @param cfg      The sensitivities. NULL means the defaults.
 * @param band     Which band, which decides the rule and the offset window.
 * @param reading  What the tuner says.
 * @return true when this looks like a station.
 */
bool seekShouldStop(const SeekConfig *cfg, BandId band,
                    const SeekReading *reading);

/**
 * The noise limit a sensitivity works out to, in tenths of a per cent.
 *
 * Exposed so the web page and the tests can show the number rather than the
 * sensitivity, because "1 to 6" says nothing on its own.
 *
 * @param sensitivity  1 to 6. Anything else is clamped.
 * @return The limit a reading has to come in under.
 */
uint16_t seekNoiseLimit(uint8_t sensitivity);

/**
 * The multipath limit a sensitivity works out to.
 *
 * @param sensitivity  1 to 6. Anything else is clamped.
 * @return The limit a reading has to come in under.
 */
uint16_t seekMultipathLimit(uint8_t sensitivity);

/**
 * The lowest level a sensitivity will stop on, in tenths of a dBuV.
 *
 * This one goes the other way: a higher sensitivity accepts a weaker signal.
 *
 * @param sensitivity  1 to 6. Anything else is clamped.
 * @return The level a reading has to reach.
 */
int16_t seekLevelFloor(uint8_t sensitivity);

#ifdef __cplusplus
}
#endif

#endif /* CORE_SEEK_H */
