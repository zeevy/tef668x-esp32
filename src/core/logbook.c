/* Implementation of the logbook ring, record format and CSV lines. */
#include "logbook.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "band_plan.h"
#include "bytes.h"
#include "core/strings.h"
#include "csv.h"

void logbookRingInit(LogbookRing *ring) {
  if (ring == NULL) {
    return;
  }
  ring->head = 0;
  ring->count = 0;
}

bool logbookSameStation(const LogbookEntry *a, const LogbookEntry *b) {
  if (a == NULL || b == NULL || a->band != b->band ||
      a->freqKHz != b->freqKHz) {
    return false;
  }
  return !(a->hasPi && b->hasPi) || a->pi == b->pi;
}

uint16_t logbookRingAppend(LogbookRing *ring) {
  if (ring == NULL) {
    return 0;
  }
  uint16_t slot = (uint16_t)((ring->head + ring->count) % LOGBOOK_MAX_ENTRIES);
  if (ring->count < LOGBOOK_MAX_ENTRIES) {
    ring->count++;
  } else {
    ring->head = (uint16_t)((ring->head + 1) % LOGBOOK_MAX_ENTRIES);
  }
  return slot;
}

uint16_t logbookRingSlotAt(const LogbookRing *ring, uint16_t i) {
  if (ring == NULL || i >= ring->count) {
    return LOGBOOK_MAX_ENTRIES;
  }
  return (uint16_t)((ring->head + i) % LOGBOOK_MAX_ENTRIES);
}

size_t logbookEncode(const LogbookEntry *e, uint8_t *out, size_t cap) {
  if (e == NULL || out == NULL || cap < LOGBOOK_RECORD_SIZE) {
    return 0;
  }
  uint8_t *p = out;
  *p++ = e->timeKnown ? 1 : 0;
  putU32(p, e->timeValue);
  p += 4;
  *p++ = e->band;
  putU32(p, e->freqKHz);
  p += 4;
  putU16(p, (uint16_t)e->levelDbuVTenths);
  p += 2;
  putU16(p, e->usnTenths);
  p += 2;
  putU16(p, e->multipathTenths);
  p += 2;
  putU16(p, e->coChannelTenths);
  p += 2;
  *p++ = (uint8_t)e->snrDb;
  *p++ = e->stereo ? 1 : 0;
  putU16(p, e->bandwidthKHz);
  p += 2;
  *p++ = e->hasName ? 1 : 0;
  /* Copied whole, including whatever trails the terminator, so encoding the
   * same entry twice always produces the same bytes. A caller that filled
   * `name` with snprintf already has it zero padded; one that did not is
   * not this module's problem to guess at. */
  memcpy(p, e->name, LOGBOOK_NAME_LEN);
  p += LOGBOOK_NAME_LEN;
  *p++ = e->hasPi ? 1 : 0;
  putU16(p, e->pi);
  p += 2;
  /* Copied whole, as the name is. */
  *p++ = e->hasRt ? 1 : 0;
  memcpy(p, e->rt, LOGBOOK_RT_LEN);
  p += LOGBOOK_RT_LEN;
  return (size_t)(p - out);
}

bool logbookDecode(const uint8_t *in, size_t len, LogbookEntry *out) {
  if (in == NULL || out == NULL || len < LOGBOOK_RECORD_SIZE) {
    return false;
  }
  const uint8_t *p = in;
  out->timeKnown = *p++ != 0;
  out->timeValue = getU32(p);
  p += 4;
  out->band = *p++;
  out->freqKHz = getU32(p);
  p += 4;
  out->levelDbuVTenths = (int16_t)getU16(p);
  p += 2;
  out->usnTenths = getU16(p);
  p += 2;
  out->multipathTenths = getU16(p);
  p += 2;
  out->coChannelTenths = getU16(p);
  p += 2;
  out->snrDb = (int8_t)*p++;
  out->stereo = *p++ != 0;
  out->bandwidthKHz = getU16(p);
  p += 2;
  out->hasName = *p++ != 0;
  memcpy(out->name, p, LOGBOOK_NAME_LEN);
  out->name[LOGBOOK_NAME_LEN - 1] = '\0';
  p += LOGBOOK_NAME_LEN;
  out->hasPi = *p++ != 0;
  out->pi = getU16(p);
  p += 2;
  out->hasRt = *p++ != 0;
  memcpy(out->rt, p, LOGBOOK_RT_LEN);
  out->rt[LOGBOOK_RT_LEN - 1] = '\0';
  return true;
}

bool logbookUpgradeV1(const uint8_t *in, uint8_t *out, size_t cap) {
  if (in == NULL || out == NULL || cap < LOGBOOK_RECORD_SIZE) {
    return false;
  }
  memcpy(out, in, LOGBOOK_RECORD_SIZE_V1);
  memset(out + LOGBOOK_RECORD_SIZE_V1, 0,
         LOGBOOK_RECORD_SIZE - LOGBOOK_RECORD_SIZE_V1);
  return true;
}

const char *logbookCsvHeader(void) {
  return "time,real,band,khz,level_dbuv,usn,multipath,cochannel,snr,stereo,"
         "bandwidth_khz,name,pi,rt\n";
}

size_t logbookCsvLine(const LogbookEntry *e, int16_t offsetMinutes, char *out,
                      size_t cap) {
  if (e == NULL || out == NULL) {
    return 0;
  }
  /* Wide enough for the worst case gmtime_r's tm_year could hand back, not
   * just a real date, or the compiler warns that a genuine format could
   * truncate even though every date this radio will ever log fits in a
   * quarter of this. */
  char timeText[40];
  if (e->timeKnown) {
    /* The offset is applied to the epoch itself, not to hours and minutes
     * the way the panel clock does it, so a shift that crosses midnight
     * moves the calendar day along with it rather than landing on the wrong
     * date at the right time. */
    time_t local = (time_t)e->timeValue + (time_t)offsetMinutes * 60;
    struct tm tmLocal;
    if (gmtime_r(&local, &tmLocal) != NULL) {
      snprintf(timeText, sizeof(timeText), "%04d-%02d-%02d %02d:%02d",
               tmLocal.tm_year + 1900, tmLocal.tm_mon + 1, tmLocal.tm_mday,
               tmLocal.tm_hour, tmLocal.tm_min);
    } else {
      timeText[0] = '\0';
    }
  } else {
    /* Not a calendar time at all, so it is written as what it actually is:
     * how long the radio had been on when this was held. */
    snprintf(timeText, sizeof(timeText), "+%ums", (unsigned)e->timeValue);
  }

  /* The longest a quoted name gets: every character a doubled quote, inside
   * the two quotes that wrap the field. */
  char nameField[2 + (LOGBOOK_NAME_LEN - 1) * 2 + 1];
  nameField[0] = '\0';
  if (e->hasName) {
    csvQuote(e->name, LOGBOOK_NAME_LEN - 1, nameField, sizeof(nameField));
  }

  char piField[8];
  piField[0] = '\0';
  if (e->hasPi) {
    snprintf(piField, sizeof(piField), "%04X", e->pi);
  }

  char rtField[2 + (LOGBOOK_RT_LEN - 1) * 2 + 1];
  rtField[0] = '\0';
  if (e->hasRt) {
    csvQuote(e->rt, LOGBOOK_RT_LEN - 1, rtField, sizeof(rtField));
  }

  return (size_t)snprintf(
      out, cap, "%s,%d,%s,%u,%d,%u,%u,%u,%d,%d,%u,%s,%s,%s\n", timeText,
      e->timeKnown ? 1 : 0, bandName((BandId)e->band), (unsigned)e->freqKHz,
      (int)e->levelDbuVTenths, (unsigned)e->usnTenths,
      (unsigned)e->multipathTenths, (unsigned)e->coChannelTenths, (int)e->snrDb,
      e->stereo ? 1 : 0, (unsigned)e->bandwidthKHz, nameField, piField,
      rtField);
}

void logbookEntryLabel(const LogbookEntry *e, char *out, size_t len) {
  if (out == NULL || len == 0) {
    return;
  }
  out[0] = '\0';
  if (e == NULL) {
    return;
  }
  if (e->hasName) {
    /* RDS sends a name as eight characters, padded with spaces to centre it
     * or to fill it. */
    size_t start = 0;
    size_t end = strnlen(e->name, sizeof(e->name));
    while (start < end && e->name[start] == ' ') {
      start++;
    }
    while (end > start && e->name[end - 1] == ' ') {
      end--;
    }
    if (end > start) {
      snprintf(out, len, "%.*s", (int)(end - start), e->name + start);
      return;
    }
  }
  if (e->hasPi) {
    snprintf(out, len, txt(STR_LOG_FMT_PI), (unsigned)e->pi);
    return;
  }
  snprintf(out, len, "%s",
           e->band < BAND_COUNT ? bandName((BandId)e->band) : "?");
}

bool logbookEntryFrequency(const LogbookEntry *e, char *out, size_t len) {
  if (out == NULL || len == 0) {
    return false;
  }
  out[0] = '\0';
  if (e == NULL || e->band >= BAND_COUNT) {
    return false;
  }
  return bandFormatWithUnit((BandId)e->band, e->freqKHz, out, len);
}
