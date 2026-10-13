/* Implementation of a saved station's page lines. */
#include "station_page.h"

#include <stdio.h>
#include <string.h>

#include "band_plan.h"
#include "clock.h"
#include "core/strings.h"

/* Between the parts of the line under the station. The middle dot is in
 * the face that line is drawn in. */
#define SEP " \xC2\xB7 "

/* Add `part` to `out` after a separator, unless it is the first. */
static void addPart(char *out, size_t len, const char *part) {
  const size_t used = strlen(out);
  if (part == NULL || part[0] == '\0' || used >= len) {
    return;
  }
  snprintf(out + used, len - used, "%s%s", used > 0 ? SEP : "", part);
}

/* A name with the spaces a station pads it with taken off both ends. */
static void trimmed(const char *in, char *out, size_t len) {
  while (*in == ' ') {
    in++;
  }
  snprintf(out, len, "%s", in);
  size_t n = strlen(out);
  while (n > 0 && out[n - 1] == ' ') {
    out[--n] = '\0';
  }
}

static void clearPage(StationPage *out) {
  memset(out, 0, sizeof(*out));
}

/* Two spaces between the frequency and the name, the one gap the title
 * has, so the two read as apart. */
static void title(StationPage *out, const char *freq, const char *name) {
  snprintf(out->title, sizeof(out->title), name[0] != '\0' ? "%s  %s" : "%s",
           freq, name);
}

static void tile(StationPage *out, uint8_t *n, StrId label) {
  out->label[*n] = txt(label);
  (*n)++;
}

/* A tenth of a dB written with its sign: -0.5, 45.2. */
static void tenths(int16_t v, char *out, size_t len) {
  const unsigned a = (unsigned)(v < 0 ? -v : v);
  snprintf(out, len, "%s%u.%u", v < 0 ? "-" : "", a / 10, a % 10);
}

/* When it was, as the lists write it. With the radio's own clock not set,
 * a time of 0 for now writes the day and time, which is how a moment after
 * now is written; one made before the clock was ever set says so. */
static void whenText(bool known, uint32_t at, bool nowKnown, uint32_t nowUtc,
                     int16_t offsetMinutes, char *out, size_t len) {
  out[0] = '\0';
  if (!known) {
    snprintf(out, len, "%s", txt(STR_PAGE_TIME_UNKNOWN));
  } else {
    (void)clockFormatWhen(at, nowKnown ? nowUtc : 0, offsetMinutes,
                          CLOCK_WHEN_ROW, out, len);
  }
}

/* The PI, the country and the programme type, each when known, on the line
 * under the title. */
static void identity(StationPage *out, uint16_t pi, const char *country,
                     uint8_t pty, RdsRegion region) {
  if (pi != 0) {
    char text[12];
    snprintf(text, sizeof(text), txt(STR_LOG_FMT_PI), (unsigned)pi);
    addPart(out->sub, sizeof(out->sub), text);
    addPart(out->sub, sizeof(out->sub), country);
  }
  if (pty > 0) {
    addPart(out->sub, sizeof(out->sub), rdsPtyNameIn(pty, region));
  }
}

bool stationPageLog(const LogbookEntry *e, bool nowKnown, uint32_t nowUtc,
                    int16_t offsetMinutes, RdsRegion region, StationPage *out) {
  if (e == NULL || out == NULL) {
    return false;
  }
  clearPage(out);
  char freq[24];
  char name[LOGBOOK_NAME_LEN];
  (void)logbookEntryFrequency(e, freq, sizeof(freq));
  trimmed(e->hasName ? e->name : "", name, sizeof(name));
  title(out, freq, name);

  char when[CLOCK_WHEN_LEN];
  whenText(e->timeKnown, e->timeValue, nowKnown, nowUtc, offsetMinutes, when,
           sizeof(when));
  addPart(out->sub, sizeof(out->sub), when);
  identity(out, e->hasPi ? e->pi : 0, logbookEntryCountry(e),
           e->hasPty ? e->pty : 0, region);

  const bool fm =
      e->band < BAND_COUNT && bandModulation((BandId)e->band) == MODULATION_FM;
  uint8_t n = 0;
  char level[12];
  tenths(e->levelDbuVTenths, level, sizeof(level));
  snprintf(out->value[n], sizeof(out->value[n]), txt(STR_PAGE_FMT_DBUV), level);
  tile(out, &n, STR_PAGE_LEVEL);
  snprintf(out->value[n], sizeof(out->value[n]), txt(STR_PAGE_FMT_DB),
           (int)e->snrDb);
  tile(out, &n, STR_PAGE_SNR);
  /* Noise and multipath are FM readings; on AM the co-channel one stands
   * in, as the radio's own screens have it. */
  if (fm) {
    snprintf(out->value[n], sizeof(out->value[n]), txt(STR_PAGE_FMT_PERCENT),
             (unsigned)((e->usnTenths + 5u) / 10u));
    tile(out, &n, STR_PAGE_NOISE);
    snprintf(out->value[n], sizeof(out->value[n]), txt(STR_PAGE_FMT_PERCENT),
             (unsigned)((e->multipathTenths + 5u) / 10u));
    tile(out, &n, STR_PAGE_MULTIPATH);
    snprintf(out->value[n], sizeof(out->value[n]), "%s",
             txt(e->stereo ? STR_COMMON_YES : STR_COMMON_NO));
    tile(out, &n, STR_PAGE_STEREO);
  } else {
    snprintf(out->value[n], sizeof(out->value[n]), txt(STR_PAGE_FMT_PERCENT),
             (unsigned)((e->coChannelTenths + 5u) / 10u));
    tile(out, &n, STR_PAGE_COCHANNEL);
  }
  snprintf(out->value[n], sizeof(out->value[n]), txt(STR_MENU_FMT_KHZ),
           (int)e->bandwidthKHz);
  tile(out, &n, STR_PAGE_WIDTH);

  if (e->hasRt) {
    trimmed(e->rt, out->text, sizeof(out->text));
  }
  trimmed(e->note, out->note, sizeof(out->note));
  return true;
}

bool stationPagePreset(const MemoryChannel *c, uint8_t slot, bool nowKnown,
                       uint32_t nowUtc, int16_t offsetMinutes, RdsRegion region,
                       StationPage *out) {
  if (c == NULL || out == NULL || c->freqKHz == 0 || c->band >= BAND_COUNT) {
    return false;
  }
  clearPage(out);
  const BandId band = (BandId)c->band;
  char freq[24];
  char name[MEMORY_NAME_LEN];
  (void)bandFormatWithUnit(band, c->freqKHz, freq, sizeof(freq));
  trimmed(c->name, name, sizeof(name));
  title(out, freq, name);

  char tag[8];
  snprintf(tag, sizeof(tag), txt(STR_RADIO_FMT_MEMORY_SLOT), (int)slot);
  addPart(out->sub, sizeof(out->sub), tag);
  /* A country only from the PI and the ECC together, never a guess. */
  identity(out, c->pi,
           c->pi != 0 && c->ecc != 0 ? rdsCountryCode(c->pi, c->ecc) : NULL,
           c->pty, region);
  /* When it was saved, in the place a log entry's radio text takes; nothing
   * for a preset saved before the radio kept the time. */
  if (c->savedUtc != 0) {
    char when[CLOCK_WHEN_LEN];
    whenText(true, c->savedUtc, nowKnown, nowUtc, offsetMinutes, when,
             sizeof(when));
    snprintf(out->text, sizeof(out->text), txt(STR_PAGE_FMT_SAVED), when);
  }
  uint8_t n = 0;
  snprintf(out->value[n], sizeof(out->value[n]), "%s", bandName(band));
  tile(out, &n, STR_PAGE_BAND);
  if (c->bandwidthKHz != 0) {
    snprintf(out->value[n], sizeof(out->value[n]), txt(STR_MENU_FMT_KHZ),
             (int)c->bandwidthKHz);
  } else {
    snprintf(out->value[n], sizeof(out->value[n]), "%s", txt(STR_MENU_AUTO));
  }
  tile(out, &n, STR_PAGE_WIDTH);
  return true;
}
