/* Implementation of the last minute counts, RT+, EON and the language. */
#include "rds_extra.h"

#include <string.h>

#include "core/strings.h"

/* Twelve slots of five seconds make the minute. */
#define WINDOW_SLOTS 12
#define WINDOW_SLOT_MS (RDS_MINUTE_MS / WINDOW_SLOTS)

/* The AID of RT+, IEC 62106-6 annex B. */
#define RTPLUS_AID 0x4BD7u
/* RDS-TMC, ALERT-C, ISO 14819-1, as the PE5PVB TEF6686_ESP32 firmware reads
 * it. */
#define TMC_AID 0xCD46u

/* ------------------------------------------------------------ the minute */

static void clearSlot(Rds *rds, uint8_t slot) {
  RdsMinute *m = &rds->info.minute;
  const uint8_t groups = rds->windowGroups[slot];
  m->groups = (uint16_t)(m->groups - groups);
  for (int b = 0; b < 4; b++) {
    /* A slot keeps the corrected and lost counts; clean is the rest. */
    const uint8_t fixed = rds->windowBlocks[slot][b][0];
    const uint8_t lost = rds->windowBlocks[slot][b][1];
    m->blocks[b][RDS_LEVEL_CORRECTED] =
        (uint16_t)(m->blocks[b][RDS_LEVEL_CORRECTED] - fixed);
    m->blocks[b][RDS_LEVEL_LOST] =
        (uint16_t)(m->blocks[b][RDS_LEVEL_LOST] - lost);
    m->blocks[b][RDS_LEVEL_CLEAN] =
        (uint16_t)(m->blocks[b][RDS_LEVEL_CLEAN] - (groups - fixed - lost));
  }
  for (int t = 0; t < 16; t++) {
    for (int v = 0; v < 2; v++) {
      m->types[t][v] =
          (uint16_t)(m->types[t][v] - rds->windowTypes[slot][t][v]);
    }
  }
  rds->windowGroups[slot] = 0;
  memset(rds->windowBlocks[slot], 0, sizeof(rds->windowBlocks[slot]));
  memset(rds->windowTypes[slot], 0, sizeof(rds->windowTypes[slot]));
}

static void clearWindow(Rds *rds) {
  memset(rds->windowGroups, 0, sizeof(rds->windowGroups));
  memset(rds->windowBlocks, 0, sizeof(rds->windowBlocks));
  memset(rds->windowTypes, 0, sizeof(rds->windowTypes));
  memset(&rds->info.minute, 0, sizeof(rds->info.minute));
  rds->windowSlot = 0;
  rds->windowStarted = false;
}

void rdsWindowTick(Rds *rds, uint32_t nowMs) {
  if (!rds->windowStarted) {
    rds->windowStarted = true;
    rds->windowSlotMs = nowMs;
    rds->windowSinceMs = nowMs;
    rds->info.minute.spanMs = 0;
    return;
  }
  uint32_t gone = nowMs - rds->windowSlotMs;
  if (gone >= RDS_MINUTE_MS) {
    /* A minute or more with no read at all: every slot is out of date, and
     * the time with no read is not time the counts cover, so the span
     * starts again too. */
    clearWindow(rds);
    rds->windowStarted = true;
    rds->windowSlotMs = nowMs;
    rds->windowSinceMs = nowMs;
    rds->info.minute.spanMs = 0;
    return;
  }
  while (gone >= WINDOW_SLOT_MS) {
    rds->windowSlot = (uint8_t)((rds->windowSlot + 1) % WINDOW_SLOTS);
    clearSlot(rds, rds->windowSlot);
    rds->windowSlotMs += WINDOW_SLOT_MS;
    gone -= WINDOW_SLOT_MS;
  }
  /* The slots cover the eleven before this one whole, and this one so far,
   * but never from before the counting started. */
  const uint32_t covered = (WINDOW_SLOTS - 1) * WINDOW_SLOT_MS + gone;
  const uint32_t since = nowMs - rds->windowSinceMs;
  if (since >= RDS_MINUTE_MS) {
    /* Held a minute back, so the difference never wraps on a radio left
     * on one station for 49 days. */
    rds->windowSinceMs = nowMs - RDS_MINUTE_MS;
  }
  rds->info.minute.spanMs = since < covered ? since : covered;
}

static void bump(uint8_t *slot, uint16_t *sum) {
  if (*slot < UINT8_MAX) {
    (*slot)++;
    (*sum)++;
  }
}

void rdsWindowGroup(Rds *rds, const RdsRead *read, bool typeKnown, uint8_t type,
                    bool isB) {
  const uint8_t s = rds->windowSlot;
  RdsMinute *m = &rds->info.minute;
  /* A full slot counts nothing more, so its counts stay in step. */
  if (rds->windowGroups[s] == UINT8_MAX) {
    return;
  }
  bump(&rds->windowGroups[s], &m->groups);
  for (int b = 0; b < 4; b++) {
    const uint8_t e = read->error[b];
    if (e == RDS_BLOCK_CLEAN) {
      m->blocks[b][RDS_LEVEL_CLEAN]++;
    } else if (e >= 3) {
      bump(&rds->windowBlocks[s][b][1], &m->blocks[b][RDS_LEVEL_LOST]);
    } else {
      bump(&rds->windowBlocks[s][b][0], &m->blocks[b][RDS_LEVEL_CORRECTED]);
    }
  }
  if (typeKnown) {
    bump(&rds->windowTypes[s][type][isB ? 1 : 0], &m->types[type][isB ? 1 : 0]);
  }
}

/* -------------------------------------------------------------- language */

void rdsFeedLanguage(Rds *rds, uint16_t blockC) {
  /* The code is the low byte; the four bits above it are unassigned, and
   * the table stops at 7F, so anything else is not a language. */
  if ((blockC & 0x0F00u) != 0 || (blockC & 0xFFu) > 0x7Fu) {
    return;
  }
  const uint8_t code = (uint8_t)(blockC & 0x7Fu);
  if (rds->languageCandidateSeen && rds->languageCandidate == code) {
    rds->info.hasLanguage = code != 0;
    rds->info.language = code;
  }
  rds->languageCandidate = code;
  rds->languageCandidateSeen = true;
}

/* EN 50067 annex J. STR_COUNT is a code the standard leaves unassigned. */
static const StrId kLanguages[128] = {
    STR_COUNT,
    STR_LANG_ALBANIAN,
    STR_LANG_BRETON,
    STR_LANG_CATALAN,
    STR_LANG_CROATIAN,
    STR_LANG_WELSH,
    STR_LANG_CZECH,
    STR_LANG_DANISH,
    STR_LANG_GERMAN,
    STR_LANG_ENGLISH,
    STR_LANG_SPANISH,
    STR_LANG_ESPERANTO,
    STR_LANG_ESTONIAN,
    STR_LANG_BASQUE,
    STR_LANG_FAROESE,
    STR_LANG_FRENCH,
    STR_LANG_FRISIAN,
    STR_LANG_IRISH,
    STR_LANG_GAELIC,
    STR_LANG_GALICIAN,
    STR_LANG_ICELANDIC,
    STR_LANG_ITALIAN,
    STR_LANG_LAPPISH,
    STR_LANG_LATIN,
    STR_LANG_LATVIAN,
    STR_LANG_LUXEMBOURGIAN,
    STR_LANG_LITHUANIAN,
    STR_LANG_HUNGARIAN,
    STR_LANG_MALTESE,
    STR_LANG_DUTCH,
    STR_LANG_NORWEGIAN,
    STR_LANG_OCCITAN,
    STR_LANG_POLISH,
    STR_LANG_PORTUGUESE,
    STR_LANG_ROMANIAN,
    STR_LANG_ROMANSH,
    STR_LANG_SERBIAN,
    STR_LANG_SLOVAK,
    STR_LANG_SLOVENE,
    STR_LANG_FINNISH,
    STR_LANG_SWEDISH,
    STR_LANG_TURKISH,
    STR_LANG_FLEMISH,
    STR_LANG_WALLOON,
    /* 2C to 3F unassigned. */
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    /* 40 is background sound, a clean feed, not a language; 41 to 44
     * unassigned. */
    STR_LANG_BACKGROUND,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_COUNT,
    STR_LANG_ZULU,
    STR_LANG_VIETNAMESE,
    STR_LANG_UZBEK,
    STR_LANG_URDU,
    STR_LANG_UKRAINIAN,
    STR_LANG_THAI,
    STR_LANG_TELUGU,
    STR_LANG_TATAR,
    STR_LANG_TAMIL,
    STR_LANG_TAJIK,
    STR_LANG_SWAHILI,
    STR_LANG_SRANAN_TONGO,
    STR_LANG_SOMALI,
    STR_LANG_SINHALA,
    STR_LANG_SHONA,
    STR_LANG_SERBO_CROAT,
    STR_LANG_RUTHENIAN,
    STR_LANG_RUSSIAN,
    STR_LANG_QUECHUA,
    STR_LANG_PASHTO,
    STR_LANG_PUNJABI,
    STR_LANG_PERSIAN,
    STR_LANG_PAPIAMENTO,
    STR_LANG_ORIYA,
    STR_LANG_NEPALI,
    STR_LANG_NDEBELE,
    STR_LANG_MARATHI,
    STR_LANG_MOLDAVIAN,
    STR_LANG_MALAY,
    STR_LANG_MALAGASY,
    STR_LANG_MACEDONIAN,
    STR_LANG_LAO,
    STR_LANG_KOREAN,
    STR_LANG_KHMER,
    STR_LANG_KAZAKH,
    STR_LANG_KANNADA,
    STR_LANG_JAPANESE,
    STR_LANG_INDONESIAN,
    STR_LANG_HINDI,
    STR_LANG_HEBREW,
    STR_LANG_HAUSA,
    STR_LANG_GUARANI,
    STR_LANG_GUJARATI,
    STR_LANG_GREEK,
    STR_LANG_GEORGIAN,
    STR_LANG_FULANI,
    STR_LANG_DARI,
    STR_LANG_CHUVASH,
    STR_LANG_CHINESE,
    STR_LANG_BURMESE,
    STR_LANG_BULGARIAN,
    STR_LANG_BENGALI,
    STR_LANG_BELARUSIAN,
    STR_LANG_BAMBARA,
    STR_LANG_AZERBAIJANI,
    STR_LANG_ASSAMESE,
    STR_LANG_ARMENIAN,
    STR_LANG_ARABIC,
    STR_LANG_AMHARIC,
};

const char *rdsLanguageName(uint8_t code) {
  if (code >= 128 || kLanguages[code] == STR_COUNT) {
    return NULL;
  }
  return txt(kLanguages[code]);
}

/* ------------------------------------------------------------------ RT+ */

/* RT+ needs blocks C and D both, so only an A version group can carry it,
 * and not one that carries data of its own: 0A, 1A, 2A, 3A, which only
 * announces, 4A, 10A, 14A and 15A. */
static bool odaGroupAllowed(uint8_t code) {
  const uint8_t type = (uint8_t)(code >> 1);
  const bool isB = (code & 1u) != 0;
  return !isB && type > 4 && type != 10 && type != 14 && type != 15;
}

void rdsFeedOdaAnnounce(Rds *rds, uint16_t blockB, uint16_t blockD) {
  if (blockD == TMC_AID) {
    /* On its second hearing, as RT+'s announcement is taken. */
    rds->info.tmc = rds->info.tmc || rds->tmcAnnounceSeen;
    rds->tmcAnnounceSeen = true;
    return;
  }
  if (blockD != RTPLUS_AID) {
    return;
  }
  const uint8_t code = (uint8_t)(blockB & 0x1Fu);
  if (!odaGroupAllowed(code)) {
    return;
  }
  if (rds->rtPlusAnnounceSeen && rds->rtPlusAnnounceCandidate == code) {
    rds->rtPlusGroup = code;
    rds->info.rtPlus = true;
  }
  rds->rtPlusAnnounceCandidate = code;
  rds->rtPlusAnnounceSeen = true;
}

bool rdsIsRtPlusGroup(const Rds *rds, uint16_t blockB) {
  const uint8_t code = (uint8_t)((GROUP_TYPE(blockB) << 1) |
                                 (GROUP_IS_B_VERSION(blockB) ? 1u : 0u));
  return rds->info.rtPlus && code == rds->rtPlusGroup;
}

static void clearTags(Rds *rds) {
  rds->info.rtPlusCount = 0;
  memset(rds->info.rtPlusTag, 0, sizeof(rds->info.rtPlusTag));
}

/* One tag into the list: in place of the same type, else added, else in
 * place of the oldest. Type 0 is the standard's empty tag. */
static void keepTag(Rds *rds, uint8_t type, uint8_t start, uint8_t length) {
  if (type == 0 || start + length > RDS_RT_LEN) {
    return;
  }
  RdsInfo *info = &rds->info;
  for (uint8_t i = 0; i < info->rtPlusCount; i++) {
    if (info->rtPlusTag[i].type == type) {
      info->rtPlusTag[i].start = start;
      info->rtPlusTag[i].length = length;
      return;
    }
  }
  if (info->rtPlusCount == RDS_RTPLUS_MAX) {
    memmove(&info->rtPlusTag[0], &info->rtPlusTag[1],
            (RDS_RTPLUS_MAX - 1) * sizeof(RdsRtPlusTag));
    info->rtPlusCount--;
  }
  RdsRtPlusTag *t = &info->rtPlusTag[info->rtPlusCount++];
  t->type = type;
  t->start = start;
  t->length = length;
}

/* How many recent tag groups a new one is matched against. */
#define TAG_CANDIDATES 4

/* Whether this tag group matches one of the last four heard, and remember
 * it in place of the oldest of them. Four, because a station may send
 * several tag groups in turn, and each must still be heard twice. */
static bool tagHeardBefore(Rds *rds, uint16_t low, uint16_t c, uint16_t d) {
  for (uint8_t i = 0; i < TAG_CANDIDATES; i++) {
    const uint16_t *k = rds->rtPlusTagCandidate[i];
    if ((rds->rtPlusTagSeen & (1u << i)) && k[0] == low && k[1] == c &&
        k[2] == d) {
      return true;
    }
  }
  const uint8_t slot = rds->rtPlusTagNext;
  rds->rtPlusTagCandidate[slot][0] = low;
  rds->rtPlusTagCandidate[slot][1] = c;
  rds->rtPlusTagCandidate[slot][2] = d;
  rds->rtPlusTagSeen |= (uint8_t)(1u << slot);
  rds->rtPlusTagNext = (uint8_t)((slot + 1u) % TAG_CANDIDATES);
  return false;
}

void rdsFeedRtPlusTags(Rds *rds, uint16_t blockB, uint16_t blockC,
                       uint16_t blockD) {
  if (!tagHeardBefore(rds, (uint16_t)(blockB & 0x1Fu), blockC, blockD)) {
    return;
  }
  /* IEC 62106-6 annex B: block B bit 4 is the item toggle, bit 3 item
   * running, bits 2 to 0 the top of the first content type. */
  const bool toggle = (blockB & 0x10u) != 0;
  if (rds->rtPlusToggleSeen && toggle != rds->rtPlusToggle) {
    clearTags(rds);
  }
  rds->rtPlusToggle = toggle;
  rds->rtPlusToggleSeen = true;
  rds->info.rtPlusRunning = (blockB & 0x08u) != 0;

  const uint8_t type1 = (uint8_t)(((blockB & 0x07u) << 3) | (blockC >> 13));
  const uint8_t start1 = (uint8_t)((blockC >> 7) & 0x3Fu);
  const uint8_t length1 = (uint8_t)(((blockC >> 1) & 0x3Fu) + 1u);
  const uint8_t type2 = (uint8_t)(((blockC & 0x01u) << 5) | (blockD >> 11));
  const uint8_t start2 = (uint8_t)((blockD >> 5) & 0x3Fu);
  const uint8_t length2 = (uint8_t)((blockD & 0x1Fu) + 1u);
  keepTag(rds, type1, start1, length1);
  keepTag(rds, type2, start2, length2);
  rdsRtPlusKeepStationShort(rds);
}

/* IEC 62106-6 annex B: STATIONNAME.SHORT. */
#define RTPLUS_STATION_SHORT 31

void rdsRtPlusKeepStationShort(Rds *rds) {
  RdsInfo *info = &rds->info;
  char name[RDS_RT_LEN + 1];
  for (uint8_t i = 0; i < info->rtPlusCount && i < RDS_RTPLUS_MAX; i++) {
    /* Whole or not at all: a name cut short and kept as the station's own
     * words would be a name the station never sent. */
    if (info->rtPlusTag[i].type == RTPLUS_STATION_SHORT &&
        rdsRtPlusText(info, i, name, sizeof(name)) &&
        strlen(name) < sizeof(info->stationShort)) {
      memcpy(info->stationShort, name, strlen(name) + 1);
      info->hasStationShort = true;
      return;
    }
  }
}

void rdsRtPlusTextChanged(Rds *rds) {
  clearTags(rds);
  /* A tag group heard before the change pointed into the old text, so it
   * must be heard twice again. */
  rds->rtPlusTagSeen = 0;
}

/* IEC 62106-6 annex B, table of content types, as short labels. */
static const StrId kRtPlusLabels[64] = {
    STR_RTP_OTHER,       STR_RTP_TITLE,     STR_RTP_ALBUM,
    STR_RTP_TRACK,       STR_RTP_ARTIST,    STR_RTP_WORK,
    STR_RTP_MOVEMENT,    STR_RTP_CONDUCTOR, STR_RTP_COMPOSER,
    STR_RTP_BAND,        STR_RTP_COMMENT,   STR_RTP_GENRE,
    STR_RTP_NEWS,        STR_RTP_LOCAL,     STR_RTP_STOCKS,
    STR_RTP_SPORT,       STR_RTP_LOTTERY,   STR_RTP_HOROSCOPE,
    STR_RTP_DIVERSION,   STR_RTP_HEALTH,    STR_RTP_EVENT,
    STR_RTP_SCENE,       STR_RTP_CINEMA,    STR_RTP_FUN,
    STR_RTP_DATE,        STR_RTP_WEATHER,   STR_RTP_TRAFFIC,
    STR_RTP_ALARM,       STR_RTP_ADVERT,    STR_RTP_URL,
    STR_RTP_OTHER,       STR_RTP_STATION,   STR_RTP_STATION,
    STR_RTP_PROGRAM,     STR_RTP_NEXT,      STR_RTP_PART,
    STR_RTP_HOST,        STR_RTP_EDITORS,   STR_RTP_FREQUENCY,
    STR_RTP_HOMEPAGE,    STR_RTP_CHANNEL,   STR_RTP_HOTLINE,
    STR_RTP_STUDIO,      STR_RTP_PHONE,     STR_RTP_SMS,
    STR_RTP_SMS,         STR_RTP_EMAIL,     STR_RTP_EMAIL,
    STR_RTP_EMAIL,       STR_RTP_MMS,       STR_RTP_CHAT,
    STR_RTP_CHAT,        STR_RTP_VOTE,      STR_RTP_VOTE,
    STR_RTP_OTHER,       STR_RTP_OTHER,     STR_RTP_OTHER,
    STR_RTP_OTHER,       STR_RTP_OTHER,     STR_RTP_PLACE,
    STR_RTP_APPOINTMENT, STR_RTP_ID,        STR_RTP_PURCHASE,
    STR_RTP_OTHER,
};

const char *rdsRtPlusLabel(uint8_t type) {
  return txt(kRtPlusLabels[type & 0x3Fu]);
}

bool rdsRtPlusText(const RdsInfo *info, uint8_t index, char *out, size_t cap) {
  if (out == NULL || cap == 0) {
    return false;
  }
  out[0] = '\0';
  if (info == NULL || index >= info->rtPlusCount || !info->hasRt) {
    return false;
  }
  const RdsRtPlusTag *t = &info->rtPlusTag[index];
  const size_t have = strlen(info->rt);
  if (t->start >= have) {
    return false;
  }
  /* A tag may reach into the padding the text lost when it was trimmed. */
  size_t end = (size_t)t->start + t->length;
  if (end > have) {
    end = have;
  }
  size_t from = t->start;
  while (from < end && info->rt[from] == ' ') {
    from++;
  }
  while (end > from && info->rt[end - 1] == ' ') {
    end--;
  }
  if (end == from) {
    return false;
  }
  size_t n = end - from;
  if (n >= cap) {
    n = cap - 1;
  }
  memcpy(out, &info->rt[from], n);
  out[n] = '\0';
  return true;
}

/* ------------------------------------------------------------------ EON */

static RdsEon *eonFor(Rds *rds, uint16_t pi) {
  RdsInfo *info = &rds->info;
  for (uint8_t i = 0; i < info->eonCount; i++) {
    if (info->eon[i].pi == pi) {
      return &info->eon[i];
    }
  }
  if (info->eonCount == RDS_EON_MAX) {
    /* The first four stay. Giving a slot to a newer one would, with five
     * or more networks sent in turn, swap them for ever and list none of
     * the ones swapped; the page says instead that more were named. */
    info->eonMore = true;
    return NULL;
  }
  const uint8_t i = info->eonCount++;
  RdsEon *e = &info->eon[i];
  memset(e, 0, sizeof(*e));
  memset(rds->eonPsBuild[i], ' ', RDS_PS_LEN);
  rds->eonPsHave[i] = 0;
  rds->eonPsPrevSeen &= (uint8_t)~(1u << i);
  e->pi = pi;
  return e;
}

uint32_t rdsAfCodeKHz(uint8_t code) {
  return code >= AF_FM_FIRST && code <= AF_FM_LAST
             ? 87500UL + (uint32_t)code * 100UL
             : 0;
}

/* A code from 1 to 204 is 87.6 to 107.9 MHz, as in group 0A. */
static void eonAf(RdsEon *e, uint8_t code) {
  if (rdsAfCodeKHz(code) == 0) {
    return;
  }
  for (uint8_t i = 0; i < e->afCount; i++) {
    if (e->afCode[i] == code) {
      return;
    }
  }
  if (e->afCount < RDS_EON_AF_MAX) {
    e->afCode[e->afCount++] = code;
  } else {
    e->afMore = true;
  }
}

void rdsFeedEon(Rds *rds, uint16_t blockB, uint16_t blockC, bool cOk,
                uint16_t blockD) {
  /* 0000 is no station, and a station naming itself is not another one. */
  if (blockD == 0 || (rds->info.hasPi && blockD == rds->info.pi)) {
    return;
  }
  RdsEon *e = eonFor(rds, blockD);
  if (e == NULL) {
    return;
  }
  if (e->heard < UINT8_MAX) {
    e->heard++;
  }
  e->hasTp = true;
  e->tp = (blockB & 0x10u) != 0;
  if (GROUP_IS_B_VERSION(blockB)) {
    /* 14B is sent when the other station's TA switches. */
    e->hasTa = true;
    e->ta = (blockB & 0x08u) != 0;
    return;
  }
  if (!cOk) {
    return;
  }
  const uint8_t variant = (uint8_t)(blockB & 0x0Fu);
  if (variant <= 3) {
    /* A name is taken when two whole passes of its four segments agree,
     * the rule the station's own name keeps, so one block C corrected
     * wrongly cannot put half of one name and half of another up. */
    const size_t k = (size_t)(e - rds->info.eon);
    char *build = rds->eonPsBuild[k];
    const uint8_t bits = (uint8_t)(3u << (variant * 2));
    if (rds->eonPsHave[k] & bits) {
      /* A segment again before the pass is whole: a new pass begins. */
      rds->eonPsHave[k] = 0;
    }
    build[variant * 2] = rdsDisplayable((uint8_t)(blockC >> 8));
    build[variant * 2 + 1] = rdsDisplayable((uint8_t)(blockC & 0xFFu));
    rds->eonPsHave[k] |= bits;
    if (rds->eonPsHave[k] == 0xFFu) {
      if ((rds->eonPsPrevSeen & (1u << k)) &&
          memcmp(rds->eonPsPrev[k], build, RDS_PS_LEN) == 0) {
        memcpy(e->ps, build, RDS_PS_LEN);
        e->ps[RDS_PS_LEN] = '\0';
        e->hasPs = true;
      }
      memcpy(rds->eonPsPrev[k], build, RDS_PS_LEN);
      rds->eonPsPrevSeen |= (uint8_t)(1u << k);
      rds->eonPsHave[k] = 0;
    }
  } else if (variant == 4) {
    /* AF method A: two codes. A count code, 225 to 249, falls outside the
     * frequency codes and is left out by eonAf; 250 says the other byte is
     * long or medium wave, not an FM code, as in group 0A. */
    const uint8_t first = (uint8_t)(blockC >> 8);
    if (first != AF_LOW_BAND) {
      eonAf(e, first);
      eonAf(e, (uint8_t)(blockC & 0xFFu));
    }
  } else if (variant <= 8) {
    /* A mapped frequency: this station's in the high byte, the other's in
     * the low one. */
    eonAf(e, (uint8_t)(blockC & 0xFFu));
  } else if (variant == 13) {
    e->hasTa = true;
    e->ta = (blockC & 0x0001u) != 0;
  }
}

/* ------------------------------------------------------------ the stats */

static uint8_t share(uint32_t part, uint32_t whole) {
  return (uint8_t)((part * 100u + whole / 2u) / whole);
}

void rdsMinuteStats(const RdsMinute *m, uint8_t maxGroups,
                    RdsMinuteStats *out) {
  if (out == NULL) {
    return;
  }
  memset(out, 0, sizeof(*out));
  if (m == NULL || m->groups == 0) {
    return;
  }
  const uint32_t groups = m->groups;
  out->known = true;
  if (m->spanMs >= 1000u) {
    out->rateKnown = true;
    out->rateTenths =
        (uint16_t)((groups * 10000u + m->spanMs / 2u) / m->spanMs);
  }
  uint32_t lost = 0;
  for (int b = 0; b < 4; b++) {
    lost += m->blocks[b][RDS_LEVEL_LOST];
    out->clean[b] = share(m->blocks[b][RDS_LEVEL_CLEAN], groups);
    out->fixed[b] = share(m->blocks[b][RDS_LEVEL_CORRECTED], groups);
    out->lost[b] = share(m->blocks[b][RDS_LEVEL_LOST], groups);
    out->fixedTenths[b] =
        (uint16_t)((m->blocks[b][RDS_LEVEL_CORRECTED] * 1000u + groups / 2u) /
                   groups);
    out->lostTenths[b] =
        (uint16_t)((m->blocks[b][RDS_LEVEL_LOST] * 1000u + groups / 2u) /
                   groups);
  }
  out->lostBlocks = (uint16_t)lost;
  out->totalBlocks = (uint16_t)(groups * 4u);
  out->blerTenths = (uint16_t)((lost * 1000u + groups * 2u) / (groups * 4u));

  if (maxGroups > RDS_MINUTE_GROUPS_MAX) {
    maxGroups = RDS_MINUTE_GROUPS_MAX;
  }
  /* The most sent first: pick the largest left each time, which keeps a
   * tie in type order since the scan goes that way. */
  bool taken[16][2] = {{false}};
  while (out->groupCount < maxGroups) {
    int bestT = -1, bestV = 0;
    uint16_t bestN = 0;
    for (int t = 0; t < 16; t++) {
      for (int v = 0; v < 2; v++) {
        if (!taken[t][v] && m->types[t][v] > bestN) {
          bestN = m->types[t][v];
          bestT = t;
          bestV = v;
        }
      }
    }
    if (bestT < 0) {
      break;
    }
    taken[bestT][bestV] = true;
    const uint8_t i = out->groupCount++;
    out->groupType[i] = (uint8_t)bestT;
    out->groupIsB[i] = bestV != 0;
    out->groupPercent[i] = share(bestN, groups);
  }
}

/* ------------------------------------------------------------ the clear */

void rdsExtraClear(Rds *rds) {
  rds->info.hasLanguage = false;
  rds->info.language = 0;
  rds->languageCandidateSeen = false;

  rds->info.rtPlus = false;
  rds->info.rtPlusRunning = false;
  clearTags(rds);
  rds->rtPlusGroup = 0;
  rds->rtPlusAnnounceSeen = false;
  rds->info.tmc = false;
  rds->tmcAnnounceSeen = false;
  rds->info.hasStationShort = false;
  memset(rds->info.stationShort, 0, sizeof(rds->info.stationShort));
  rds->rtPlusTagSeen = 0;
  rds->rtPlusToggleSeen = false;

  rds->info.eonCount = 0;
  rds->info.eonMore = false;
  memset(rds->info.eon, 0, sizeof(rds->info.eon));
  memset(rds->eonPsHave, 0, sizeof(rds->eonPsHave));
  rds->eonPsPrevSeen = 0;

  clearWindow(rds);
}
