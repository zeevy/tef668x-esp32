/**
 * @file band_plan.c
 * @brief The band table and the arithmetic that moves around it.
 */
#include "band_plan.h"

#include <stdio.h>
#include <string.h>

/* Step sizes each band offers, in kHz, lowest first. */
static const uint16_t kStepsLw[] = {1, 9};
static const uint16_t kStepsMw9[] = {1, 9};
static const uint16_t kStepsMw10[] = {1, 10};
static const uint16_t kStepsSw[] = {1, 5};
static const uint16_t kStepsOirt[] = {10, 30};
static const uint16_t kStepsFm[] = {50, 100, 200};

/** One row of the band table, before the regional config is applied. */
typedef struct {
  const char *name;        /**< Short name shown on screen. */
  uint32_t lowKHz;         /**< Low edge before the region is applied. */
  uint32_t highKHz;        /**< High edge before the region is applied. */
  const uint16_t *steps;   /**< Allowed step sizes in kHz, lowest first. */
  size_t stepCount;        /**< How many entries steps has. */
  uint16_t defaultStepKHz; /**< The step used until someone changes it. */
  Modulation modulation;   /**< AM or FM. */
} BandRow;

/*
 * The table. Edges and steps come from src/constants.h in the PE5PVB
 * firmware, which runs on this radio, cross checked against the vendor
 * listing recorded in HARDWARE.md.
 *
 * The 1 kHz step on the AM bands is not a channel spacing, it is the fine
 * tune the old firmware offers so a station tuned slightly off centre can be
 * reached. Selective fading on AM makes that useful rather than fussy.
 */
static const BandRow kBands[BAND_COUNT] = {
    [BAND_LW] = {"LW", 144, 513, kStepsLw, 2, 9, MODULATION_AM},
    /* Medium wave edges depend on the spacing, so these are the 9 kHz ones
     * and bandLimits swaps them for 10 kHz. */
    [BAND_MW] = {"MW", 522, 1791, kStepsMw9, 2, 9, MODULATION_AM},
    [BAND_SW] = {"SW", 1700, 27000, kStepsSw, 2, 5, MODULATION_AM},
    [BAND_OIRT] = {"OIRT", 65000, 74000, kStepsOirt, 2, 30, MODULATION_FM},
    /* FM edges depend on the region, so these are the widest and bandLimits
     * narrows them. */
    [BAND_FM] = {"FM", 65000, 108000, kStepsFm, 3, 100, MODULATION_FM},
};

/* The FM regional band plans, in kHz. */
static const uint32_t kFmRegionLow[FM_REGION_COUNT] = {
    [FM_REGION_FULL] = 65000,  [FM_REGION_JAPAN] = 76000,
    [FM_REGION_WIDE] = 76000,  [FM_REGION_87_108] = 87000,
    [FM_REGION_WORLD] = 87500,
};
static const uint32_t kFmRegionHigh[FM_REGION_COUNT] = {
    [FM_REGION_FULL] = 108000,  [FM_REGION_JAPAN] = 95000,
    [FM_REGION_WIDE] = 108000,  [FM_REGION_87_108] = 108000,
    [FM_REGION_WORLD] = 108000,
};

/*
 * Shortwave meter bands. From src/constants.h in the PE5PVB firmware, which
 * cites short-wave.info. Ordered by frequency, which is the order the Meter
 * band tuning mode steps through them.
 *
 * 160 metres is an amateur band rather than a broadcast one, but the firmware
 * this replaces steps through it, so leaving it out would take something away
 * from a radio that has it today. The list is kept as that firmware has it.
 */
static const SwMeterBand kMeterBands[] = {
    {160, 1800, 2000},  {120, 2300, 2495},  {90, 3200, 3400},
    {75, 3900, 4000},   {60, 4750, 4995},   {49, 5900, 6200},
    {41, 7200, 7450},   {31, 9400, 9900},   {25, 11600, 12100},
    {22, 13570, 13870}, {19, 15100, 15800}, {16, 17480, 17900},
    {15, 18900, 19020}, {13, 21450, 21850}, {11, 25670, 26100},
};

/** How many shortwave meter bands the table holds. */
#define METER_BAND_COUNT (sizeof(kMeterBands) / sizeof(kMeterBands[0]))

/** The config to use when the caller passed NULL. */
static BandPlanConfig defaultConfig(void) {
  BandPlanConfig c;
  bandPlanDefaults(&c);
  return c;
}

/** True when this is a real band. */
static bool validBand(BandId band) {
  return band >= 0 && band < BAND_COUNT;
}

void bandPlanDefaults(BandPlanConfig *config) {
  if (config == NULL) {
    return;
  }
  config->fmRegion = FM_REGION_WORLD;
  config->mwSpacing = MW_SPACING_9K;
}

const char *bandName(BandId band) {
  return validBand(band) ? kBands[band].name : "";
}

Modulation bandModulation(BandId band) {
  return validBand(band) ? kBands[band].modulation : MODULATION_AM;
}

bool bandLimits(BandId band, const BandPlanConfig *config, uint32_t *lowKHz,
                uint32_t *highKHz) {
  if (!validBand(band) || lowKHz == NULL || highKHz == NULL) {
    return false;
  }
  BandPlanConfig c = config ? *config : defaultConfig();

  uint32_t lo = kBands[band].lowKHz;
  uint32_t hi = kBands[band].highKHz;

  if (band == BAND_FM && c.fmRegion < FM_REGION_COUNT) {
    lo = kFmRegionLow[c.fmRegion];
    hi = kFmRegionHigh[c.fmRegion];
  } else if (band == BAND_MW && c.mwSpacing == MW_SPACING_10K) {
    lo = 520;
    hi = 1720;
  }

  *lowKHz = lo;
  *highKHz = hi;
  return true;
}

/** The step list for a band, which medium wave changes with its spacing. */
static const uint16_t *stepsFor(BandId band, const BandPlanConfig *c,
                                size_t *count) {
  if (band == BAND_MW && c->mwSpacing == MW_SPACING_10K) {
    *count = 2;
    return kStepsMw10;
  }
  *count = kBands[band].stepCount;
  return kBands[band].steps;
}

size_t bandStepCount(BandId band, const BandPlanConfig *config) {
  if (!validBand(band)) {
    return 0;
  }
  BandPlanConfig c = config ? *config : defaultConfig();
  size_t count = 0;
  stepsFor(band, &c, &count);
  return count;
}

uint16_t bandStepAt(BandId band, const BandPlanConfig *config, size_t index) {
  if (!validBand(band)) {
    return 0;
  }
  BandPlanConfig c = config ? *config : defaultConfig();
  size_t count = 0;
  const uint16_t *steps = stepsFor(band, &c, &count);
  return index < count ? steps[index] : 0;
}

uint16_t bandDefaultStep(BandId band, const BandPlanConfig *config) {
  if (!validBand(band)) {
    return 0;
  }
  BandPlanConfig c = config ? *config : defaultConfig();
  if (band == BAND_MW && c.mwSpacing == MW_SPACING_10K) {
    return 10;
  }
  return kBands[band].defaultStepKHz;
}

bool bandStepAllowed(BandId band, const BandPlanConfig *config,
                     uint16_t stepKHz) {
  if (!validBand(band) || stepKHz == 0) {
    return false;
  }
  BandPlanConfig c = config ? *config : defaultConfig();
  size_t count = 0;
  const uint16_t *steps = stepsFor(band, &c, &count);
  for (size_t i = 0; i < count; i++) {
    if (steps[i] == stepKHz) {
      return true;
    }
  }
  return false;
}

bool bandContains(BandId band, const BandPlanConfig *config, uint32_t freqKHz) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (!bandLimits(band, config, &lo, &hi)) {
    return false;
  }
  return freqKHz >= lo && freqKHz <= hi;
}

uint32_t bandClamp(BandId band, const BandPlanConfig *config,
                   uint32_t freqKHz) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (!bandLimits(band, config, &lo, &hi)) {
    return freqKHz;
  }
  if (freqKHz < lo) {
    return lo;
  }
  if (freqKHz > hi) {
    return hi;
  }
  return freqKHz;
}

uint32_t bandTopChannel(BandId band, const BandPlanConfig *config,
                        uint16_t stepKHz) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (!bandStepAllowed(band, config, stepKHz) ||
      !bandLimits(band, config, &lo, &hi)) {
    return 0;
  }
  return lo + ((hi - lo) / stepKHz) * stepKHz;
}

uint32_t bandStepUp(BandId band, const BandPlanConfig *config, uint32_t freqKHz,
                    uint16_t stepKHz) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (!bandStepAllowed(band, config, stepKHz) ||
      !bandLimits(band, config, &lo, &hi)) {
    return freqKHz;
  }
  /* Anything at or above the top edge wraps. Doing this before the arithmetic
   * is not only the right answer, it is what keeps the arithmetic from
   * overflowing: a frequency near the top of the range would otherwise make
   * lo + (offset / step + 1) * step wrap around and land back inside the
   * band at a frequency that is not on the grid. */
  if (freqKHz < lo || freqKHz >= hi) {
    return lo;
  }
  /* The grid starts at the low edge, so a frequency that is not on it moves
   * to the next one that is rather than carrying the offset along. */
  uint32_t next = lo + ((freqKHz - lo) / stepKHz + 1) * stepKHz;
  return next > hi ? lo : next;
}

uint32_t bandStepDown(BandId band, const BandPlanConfig *config,
                      uint32_t freqKHz, uint16_t stepKHz) {
  uint32_t lo = 0;
  uint32_t hi = 0;
  if (!bandStepAllowed(band, config, stepKHz) ||
      !bandLimits(band, config, &lo, &hi)) {
    return freqKHz;
  }
  uint32_t top = lo + ((hi - lo) / stepKHz) * stepKHz;
  if (freqKHz <= lo) {
    return top;
  }
  if (freqKHz > hi) {
    return top;
  }
  uint32_t offset = freqKHz - lo;
  uint32_t steps = offset / stepKHz;
  if (offset % stepKHz != 0) {
    /* Off the grid, so come back down onto it rather than below it. */
    return lo + steps * stepKHz;
  }
  return steps == 0 ? top : lo + (steps - 1) * stepKHz;
}

bool bandForFrequency(const BandPlanConfig *config, uint32_t freqKHz,
                      BandId *band) {
  if (band == NULL) {
    return false;
  }
  /* Lowest band first, and OIRT before FM, because the full FM region starts
   * at 65 MHz and would otherwise swallow the whole OIRT band. */
  static const BandId order[] = {BAND_LW, BAND_MW, BAND_SW, BAND_OIRT, BAND_FM};
  for (size_t i = 0; i < sizeof(order) / sizeof(order[0]); i++) {
    if (bandContains(order[i], config, freqKHz)) {
      *band = order[i];
      return true;
    }
  }
  return false;
}

size_t swMeterBandCount(void) {
  return METER_BAND_COUNT;
}

const SwMeterBand *swMeterBandAt(size_t index) {
  return index < METER_BAND_COUNT ? &kMeterBands[index] : NULL;
}

const SwMeterBand *swMeterBandFor(uint32_t freqKHz) {
  for (size_t i = 0; i < METER_BAND_COUNT; i++) {
    if (freqKHz >= kMeterBands[i].lowKHz && freqKHz <= kMeterBands[i].highKHz) {
      return &kMeterBands[i];
    }
  }
  return NULL;
}

/**
 * Which meter band a frequency sits in, by index.
 *
 * @return The index, or METER_BAND_COUNT when it is between two bands.
 */
static size_t meterBandIndexFor(uint32_t freqKHz) {
  for (size_t i = 0; i < METER_BAND_COUNT; i++) {
    if (freqKHz >= kMeterBands[i].lowKHz && freqKHz <= kMeterBands[i].highKHz) {
      return i;
    }
  }
  return METER_BAND_COUNT;
}

uint32_t swMeterBandNext(uint32_t freqKHz) {
  size_t here = meterBandIndexFor(freqKHz);
  if (here != METER_BAND_COUNT) {
    /* Inside a band, so move past it rather than to its own start. */
    size_t next = (here + 1) % METER_BAND_COUNT;
    return kMeterBands[next].lowKHz;
  }
  for (size_t i = 0; i < METER_BAND_COUNT; i++) {
    if (kMeterBands[i].lowKHz > freqKHz) {
      return kMeterBands[i].lowKHz;
    }
  }
  return kMeterBands[0].lowKHz;
}

uint32_t swMeterBandPrevious(uint32_t freqKHz) {
  size_t here = meterBandIndexFor(freqKHz);
  if (here != METER_BAND_COUNT) {
    /* Inside a band. Step to the one below, not to this one's own start,
     * otherwise leaving a band takes two presses. */
    size_t prev = here == 0 ? METER_BAND_COUNT - 1 : here - 1;
    return kMeterBands[prev].lowKHz;
  }
  for (size_t i = METER_BAND_COUNT; i > 0; i--) {
    if (kMeterBands[i - 1].lowKHz < freqKHz) {
      return kMeterBands[i - 1].lowKHz;
    }
  }
  return kMeterBands[METER_BAND_COUNT - 1].lowKHz;
}

const char *bandFrequencyUnit(BandId band) {
  return bandModulation(band) == MODULATION_FM ? "MHz" : "kHz";
}

bool bandFormatFrequency(BandId band, uint32_t freqKHz, char *out,
                         size_t outLen) {
  if (out == NULL || outLen == 0) {
    return false;
  }
  out[0] = '\0';
  if (!validBand(band)) {
    return false;
  }

  char buf[24];
  int written;

  if (bandModulation(band) == MODULATION_FM) {
    /* Two decimals of megahertz, so 104000 kHz reads 104.00. */
    written = snprintf(buf, sizeof(buf), "%u.%02u", (unsigned)(freqKHz / 1000),
                       (unsigned)((freqKHz % 1000) / 10));
  } else {
    /* Kilohertz, grouped in threes from the right: 9420 reads "9 420". */
    /* Ten digits is the widest a uint32_t gets, so the buffer cannot be too
     * small. Checking that at compile time rather than at run time keeps out
     * a branch that no test could ever reach. */
    char digits[12];
    _Static_assert(sizeof(digits) >= 11,
                   "a uint32_t needs ten digits and a terminator");
    int n = snprintf(digits, sizeof(digits), "%u", (unsigned)freqKHz);
    size_t outPos = 0;
    for (int i = 0; i < n; i++) {
      if (i > 0 && (n - i) % 3 == 0) {
        buf[outPos++] = ' ';
      }
      buf[outPos++] = digits[i];
    }
    buf[outPos] = '\0';
    written = (int)outPos;
  }

  if (written < 0 || (size_t)written >= outLen) {
    return false;
  }
  memcpy(out, buf, (size_t)written + 1);
  return true;
}
