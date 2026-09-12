/**
 * @file band_plan.h
 * @brief Which frequencies exist, what steps are allowed, and what happens at
 *        the edges.
 *
 * Everything in here is in **kilohertz**, on every band. The firmware this
 * replaces mixes units: FM is in tenths of a kilohertz so 8750 means 87.50 MHz,
 * while AM is in kilohertz. That is a standing invitation to a bug that only
 * shows on one band, so this layer picks one unit and the tuner driver converts
 * to whatever the chip wants.
 *
 * Nothing in here touches hardware, so it builds and is tested on a PC.
 *
 * Sources for the numbers, none of them guessed:
 * - Band edges and steps: `src/constants.h` in the PE5PVB firmware, which runs
 *   on this radio, cross checked against the vendor listing in HARDWARE.md.
 * - Shortwave meter bands: the same file, which cites short-wave.info.
 */
#ifndef CORE_BAND_PLAN_H
#define CORE_BAND_PLAN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The bands this radio has. */
typedef enum {
  BAND_LW = 0, /**< Long wave. */
  BAND_MW,     /**< Medium wave. */
  BAND_SW,     /**< Short wave. */
  BAND_OIRT,   /**< The eastern European FM band, 65 to 74 MHz. */
  BAND_FM,     /**< FM broadcast. */
  BAND_COUNT   /**< How many bands there are. Not a band. */
} BandId;

/** How a band is modulated, which decides what the tuner is told to do. */
typedef enum {
  MODULATION_AM, /**< LW, MW and SW. */
  MODULATION_FM  /**< OIRT and FM. */
} Modulation;

/**
 * Which slice of FM is in use.
 *
 * These are regional band plans, not different hardware. The tuner covers the
 * whole 65 to 108 MHz range whichever one is picked.
 */
typedef enum {
  FM_REGION_FULL = 0, /**< 65.0 to 108.0 MHz, everything the tuner can do. */
  FM_REGION_JAPAN,    /**< 76.0 to 95.0 MHz. */
  FM_REGION_WIDE,     /**< 76.0 to 108.0 MHz. */
  FM_REGION_87_108,   /**< 87.0 to 108.0 MHz. */
  FM_REGION_WORLD,    /**< 87.5 to 108.0 MHz. Most of the world, the default. */
  FM_REGION_COUNT     /**< How many regions there are. Not a region. */
} FmRegion;

/**
 * Medium wave channel spacing.
 *
 * The two do not cover the same range, so this changes the band edges as well
 * as the step.
 */
typedef enum {
  MW_SPACING_9K = 0, /**< 9 kHz, 522 to 1791 kHz. Europe, Africa, Asia. */
  MW_SPACING_10K     /**< 10 kHz, 520 to 1720 kHz. The Americas. */
} MwSpacing;

/** The regional choices that change what a band looks like. */
typedef struct {
  FmRegion fmRegion;   /**< Which slice of FM. */
  MwSpacing mwSpacing; /**< Medium wave channel spacing. */
} BandPlanConfig;

/**
 * A shortwave meter band, the ones the Meter band tuning mode steps by.
 *
 * Mostly broadcast bands, plus 160 metres, which is amateur but which the
 * firmware this replaces steps through.
 */
typedef struct {
  uint16_t metres;  /**< 49 for the 49 metre band, and so on. */
  uint32_t lowKHz;  /**< First frequency in the band. */
  uint32_t highKHz; /**< Last frequency in the band. */
} SwMeterBand;

/**
 * Fill a config with what a radio leaves the factory with.
 *
 * @param config  Receives the defaults.
 */
void bandPlanDefaults(BandPlanConfig *config);

/**
 * The short name of a band.
 *
 * @param band  Which band.
 * @return "FM", "SW" and so on. An empty string for a band that is not real.
 *         Never NULL.
 */
const char *bandName(BandId band);

/**
 * Whether a band is amplitude or frequency modulated.
 *
 * @param band  Which band.
 * @return The modulation. AM for a band that is not real.
 */
Modulation bandModulation(BandId band);

/**
 * The edges of a band, taking the regional config into account.
 *
 * @param band    Which band.
 * @param config  The regional choices. NULL means the defaults.
 * @param lowKHz  Receives the first frequency in the band.
 * @param highKHz Receives the last frequency in the band.
 * @return false when the band id is not a real band, in which case nothing is
 *         written.
 */
bool bandLimits(BandId band, const BandPlanConfig *config, uint32_t *lowKHz,
                uint32_t *highKHz);

/**
 * How many step sizes a band offers.
 *
 * @param band    Which band.
 * @param config  The regional choices. NULL means the defaults.
 * @return The count, or 0 when the band id is not a real band.
 */
size_t bandStepCount(BandId band, const BandPlanConfig *config);

/**
 * One of the step sizes a band offers, in kHz.
 *
 * @param band    Which band.
 * @param config  The regional choices. NULL means the defaults.
 * @param index   0 to bandStepCount() - 1.
 * @return The step in kHz, or 0 when the band or the index is wrong.
 */
uint16_t bandStepAt(BandId band, const BandPlanConfig *config, size_t index);

/**
 * The step a band uses until someone changes it, in kHz.
 *
 * @param band    Which band.
 * @param config  The regional choices. NULL means the defaults.
 * @return The step in kHz, or 0 when the band id is not a real band.
 */
uint16_t bandDefaultStep(BandId band, const BandPlanConfig *config);

/**
 * Whether a band allows a given step size.
 *
 * @param band    Which band.
 * @param config  The regional choices. NULL means the defaults.
 * @param stepKHz The step to check, in kHz.
 * @return true when that band offers that step.
 */
bool bandStepAllowed(BandId band, const BandPlanConfig *config,
                     uint16_t stepKHz);

/**
 * Whether a frequency falls inside a band.
 *
 * This only checks the edges. A frequency between two channels is still inside
 * the band.
 *
 * @param band    Which band.
 * @param config  The regional choices. NULL means the defaults.
 * @param freqKHz The frequency to check, in kHz.
 * @return true when it is between the edges, inclusive.
 */
bool bandContains(BandId band, const BandPlanConfig *config, uint32_t freqKHz);

/**
 * Move a frequency to the nearest one inside the band.
 *
 * @param band    Which band.
 * @param config  The regional choices. NULL means the defaults.
 * @param freqKHz The frequency to clamp, in kHz.
 * @return The clamped frequency, or freqKHz when the band id is not real.
 */
uint32_t bandClamp(BandId band, const BandPlanConfig *config, uint32_t freqKHz);

/**
 * The channel nearest a frequency, on this band's grid.
 *
 * Being inside a band and being on one of its channels are two different
 * questions, and most of this file only asks the first. They come apart when
 * the grid moves under a frequency that was stored earlier: 738 kHz is a real
 * medium wave channel at 9 kHz spacing and is not one at 10 kHz, but it is
 * inside the band either way. A radio that comes up there is slightly off
 * every station until somebody moves it, with nothing to say why.
 *
 * A frequency exactly between two channels goes up, which is arbitrary but
 * has to be decided somewhere.
 *
 * @param band     Which band.
 * @param config   The regional choices. NULL for the defaults.
 * @param freqKHz  The frequency to snap.
 * @param stepKHz  The channel spacing to snap onto.
 * @return The nearest channel, or freqKHz unchanged when the band or the step
 *         is not one this radio has.
 */
uint32_t bandNearestChannel(BandId band, const BandPlanConfig *config,
                            uint32_t freqKHz, uint16_t stepKHz);

/**
 * The next channel up, wrapping round to the bottom at the top edge.
 *
 * The channel grid starts at the band's low edge, so a frequency that is not
 * on the grid moves to the next one that is rather than staying off it.
 *
 * @param band    Which band.
 * @param config  The regional choices. NULL means the defaults.
 * @param freqKHz Where it is now, in kHz.
 * @param stepKHz The step to move by, in kHz. Must be one the band allows.
 * @return The new frequency, or freqKHz when the band or the step is wrong.
 */
uint32_t bandStepUp(BandId band, const BandPlanConfig *config, uint32_t freqKHz,
                    uint16_t stepKHz);

/**
 * The next channel down, wrapping round to the top at the bottom edge.
 *
 * @param band    Which band.
 * @param config  The regional choices. NULL means the defaults.
 * @param freqKHz Where it is now, in kHz.
 * @param stepKHz The step to move by, in kHz. Must be one the band allows.
 * @return The new frequency, or freqKHz when the band or the step is wrong.
 */
uint32_t bandStepDown(BandId band, const BandPlanConfig *config,
                      uint32_t freqKHz, uint16_t stepKHz);

/**
 * The highest channel on the grid inside a band.
 *
 * This is not always the top edge. With a 9 kHz step from 522 kHz the last
 * channel is 1791, but a step that does not divide the range evenly stops
 * short.
 *
 * @param band    Which band.
 * @param config  The regional choices. NULL means the defaults.
 * @param stepKHz The step in kHz.
 * @return The highest channel, or 0 when the band or the step is wrong.
 */
uint32_t bandTopChannel(BandId band, const BandPlanConfig *config,
                        uint16_t stepKHz);

/**
 * Which band a frequency belongs to.
 *
 * @param config  The regional choices. NULL means the defaults.
 * @param freqKHz The frequency in kHz.
 * @param band    Receives the band when one matches.
 * @return false when the frequency is in no band, in which case nothing is
 *         written.
 */
bool bandForFrequency(const BandPlanConfig *config, uint32_t freqKHz,
                      BandId *band);

/**
 * How many shortwave meter bands there are.
 *
 * @return The count.
 */
size_t swMeterBandCount(void);

/**
 * One shortwave meter band, ordered from the lowest frequency upwards.
 *
 * @param index 0 to swMeterBandCount() - 1.
 * @return The band, or NULL when the index is out of range.
 */
const SwMeterBand *swMeterBandAt(size_t index);

/**
 * The meter band a frequency falls in.
 *
 * @param freqKHz The frequency in kHz.
 * @return The band, or NULL when the frequency is between two of them. Most of
 *         shortwave is between two of them, so NULL is the normal answer.
 */
const SwMeterBand *swMeterBandFor(uint32_t freqKHz);

/**
 * The start of the next meter band above a frequency, wrapping at the top.
 *
 * @param freqKHz Where it is now, in kHz.
 * @return The first frequency of the next meter band.
 */
uint32_t swMeterBandNext(uint32_t freqKHz);

/**
 * The start of the next meter band below a frequency, wrapping at the bottom.
 *
 * @param freqKHz Where it is now, in kHz.
 * @return The first frequency of that meter band.
 */
uint32_t swMeterBandPrevious(uint32_t freqKHz);

/**
 * How many bandwidths this band offers.
 *
 * @param band  Which band.
 * @return The count, or 0 for a band that is not real.
 */
size_t bandBandwidthCount(BandId band);

/**
 * One of the bandwidths a band offers, in kHz.
 *
 * The FM list starts at 0, which means the tuner picks the width itself from
 * how much interference it can see. There is no such mode on the AM side, so
 * that list has no 0 in it.
 *
 * @param band   Which band.
 * @param index  Which one, from 0.
 * @return The bandwidth in kHz, or 0 when the index is past the end. A 0 that
 *         means "past the end" and a 0 that means "automatic" are told apart
 *         by checking the index against bandBandwidthCount first.
 */
uint16_t bandBandwidthAt(BandId band, size_t index);

/**
 * Whether a band offers this bandwidth.
 *
 * The filter is built from what the silicon can do, so the two lists are not
 * round numbers and they do not overlap: the AM widths are 3 to 8 kHz and the
 * FM ones 56 to 311. A width from the wrong list is not a near miss. Asking
 * the FM side for 4 pins its filter at 4.0 kHz, which is narrower than a
 * station, and the radio then reports no stereo pilot and no signal and looks
 * exactly like one with no aerial.
 *
 * @param band  Which band.
 * @param khz   The width being asked for.
 * @return true when that band offers it.
 */
bool bandBandwidthAllowed(BandId band, uint16_t khz);

/**
 * The next bandwidth after this one, wrapping at the end.
 *
 * For the BW button, which walks the list. A current value that is not in the
 * list starts again at the beginning rather than getting stuck.
 *
 * @param band     Which band.
 * @param current  The bandwidth in use now, in kHz.
 * @return The next one in kHz.
 */
uint16_t bandBandwidthNext(BandId band, uint16_t current);

/**
 * Work out what a typed number means.
 *
 * Somebody keying 1028 on the pad means 102.8 MHz, and keying 738 means
 * 738 kHz. The digits alone do not say which, so the number is multiplied by
 * ten until it lands inside a band. That is the rule the working PE5PVB
 * firmware uses, so it is the one people already expect from this radio.
 *
 * The band in use is tried first, so a number that could be read two ways
 * stays on the band the radio is already on. 1000 is both 1000 kHz on medium
 * wave and 100.0 MHz on FM, and someone on medium wave typing it means the
 * medium wave station.
 *
 * @param config   The band plan.
 * @param typed    The digits as one number, so 1028 for 102.8 MHz.
 * @param prefer   The band to try first. BAND_COUNT for no preference.
 * @param freqKHz  Receives the frequency in kHz.
 * @param band     Receives which band it turned out to be.
 * @return false when no amount of multiplying lands it in a band, in which
 *         case nothing is written.
 */
bool bandFromTypedNumber(const BandPlanConfig *config, uint32_t typed,
                         BandId prefer, uint32_t *freqKHz, BandId *band);

/**
 * Write a frequency the way it is shown on screen, without the unit.
 *
 * FM and OIRT come out as megahertz with two decimals, "104.00". AM bands come
 * out as plain kilohertz, "9420" and "1377", with nothing between the digits.
 *
 * @param band    Which band, which decides the format.
 * @param freqKHz The frequency in kHz.
 * @param out     Buffer for the text.
 * @param outLen  How big that buffer is. 16 is always enough.
 * @return true when it fitted.
 */
bool bandFormatFrequency(BandId band, uint32_t freqKHz, char *out,
                         size_t outLen);

/**
 * The unit a band's frequency is shown in.
 *
 * @param band  Which band.
 * @return "MHz" for FM and OIRT, "kHz" for the AM bands. Never NULL.
 */
const char *bandFrequencyUnit(BandId band);

#ifdef __cplusplus
}
#endif

#endif /* CORE_BAND_PLAN_H */
