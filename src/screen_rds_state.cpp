/* Implementation of the RDS screen's view, built from the decoder. */
#include "screen_rds_state.h"

#include <stdio.h>
#include <string.h>

#include "core/band_plan.h"
#include "core/dx.h"
#include "core/memory.h"
#include "core/rds_country.h"
#include "core/strings.h"

static ScreenRdsFlag flag(bool known, bool yes) {
  return !known ? SCREEN_RDS_UNKNOWN : yes ? SCREEN_RDS_YES : SCREEN_RDS_NO;
}

/* A frequency as the tiles and lists show it, 98.30, the header's own
 * rule. An alternative frequency is always FM. */
static void formatKHz(uint32_t khz, char *out, size_t n) {
  if (!bandFormatFrequency(BAND_FM, khz, out, n)) {
    out[0] = '\0';
  }
}

/* Page 1, the station. */
static void fillStation(const RdsInfo *r, RdsRegion region, ScreenRds *view) {
  static char identifier[8];
  static char ptyNumber[12];
  static char ct[8];
  static char ecc[4];
  static char name[RDS_PS_LONG_LEN + 1];
  const char *sent = r->hasPsLong ? r->psLong : r->hasPs ? r->ps : NULL;
  view->ps = sent != NULL && rdsNameTrim(sent, sizeof(name), name, sizeof(name))
                 ? name
                 : NULL;
  if (r->hasPi) {
    snprintf(identifier, sizeof(identifier), "%04X", r->pi);
    view->pi = identifier;
    view->piSure = true;
    if (rdsPiHasArea(r->pi, region)) {
      view->area = rdsPiOriginName(r->pi);
    }
  } else if (r->piZero) {
    /* What the station sends, rather than the dash that means nothing has
     * been heard. It has no area to name, because it is not an
     * identifier. */
    view->pi = txt(STR_RDS_PI_ZERO);
    view->piSure = true;
  } else if (rdsFormatPiHeard(r, identifier)) {
    view->pi = identifier;
    view->piSure = false;
  }
  if (r->hasPty) {
    snprintf(ptyNumber, sizeof(ptyNumber), txt(STR_RDS_FMT_PTY_NUMBER),
             (unsigned)r->pty);
    view->ptyNumber = ptyNumber;
    view->ptyName = rdsPtyNameIn(r->pty, region);
  }
  if (r->hasEcc) {
    snprintf(ecc, sizeof(ecc), "%02X", (unsigned)r->ecc);
    view->ecc = ecc;
  }
  static DxIdentity id;
  dxStationIdentity(r, NULL, region, 0, false, &id);
  view->country = id.country;
  view->countryNamed = view->country != NULL;
  /* In North America the call letters take the country's place: the
   * station's own RT+ name, or a guess from the PI, drawn dimmed. */
  if (id.call != RDS_CALL_NONE) {
    view->country = id.callText;
    view->countryNamed = true;
    view->countryGuess = id.call == RDS_CALL_GUESS;
  }
  if (view->country == NULL && view->piSure) {
    /* Why there is none, so a blank does not read as a fact. */
    view->country =
        txt(r->hasEcc ? STR_RDS_COUNTRY_UNLISTED : STR_RDS_NO_COUNTRY);
  }
  view->ptyn = r->hasPtyn ? r->ptyn : NULL;
  view->language = r->hasLanguage ? rdsLanguageName(r->language) : NULL;
  if (r->clock.valid) {
    snprintf(ct, sizeof(ct), txt(STR_COMMON_FMT_CLOCK), (unsigned)r->clock.hour,
             (unsigned)r->clock.minute);
    view->ct = ct;
  }
  view->tp = flag(r->hasFlags, r->tp);
  view->ta = flag(r->hasFlags, r->ta);
  view->speech = flag(r->hasFlags, r->speech);
  view->stereo = flag(r->hasDiStereo, r->diStereo);
}

/* Page 2: the radio text, and the RT+ tags that have words in it, in
 * content type order so a title comes before an artist. */
static void fillText(const RdsInfo *r, ScreenRds *view) {
  static char words[SCREEN_RDS_TAGS][RDS_RT_LEN + 1];
  view->text = r->hasRt ? r->rt : NULL;
  bool used[RDS_RTPLUS_MAX] = {false};
  while (view->tagCount < SCREEN_RDS_TAGS) {
    int best = -1;
    for (uint8_t i = 0; i < r->rtPlusCount; i++) {
      if (!used[i] &&
          (best < 0 || r->rtPlusTag[i].type < r->rtPlusTag[best].type)) {
        best = i;
      }
    }
    if (best < 0) {
      break;
    }
    used[best] = true;
    const uint8_t n = view->tagCount;
    if (rdsRtPlusText(r, (uint8_t)best, words[n], sizeof(words[n]))) {
      view->tag[n].label = rdsRtPlusLabel(r->rtPlusTag[best].type);
      view->tag[n].text = words[n];
      view->tagCount++;
    }
  }
  view->rtPlusRunning = r->rtPlusRunning;
  view->rtPlusNote = txt(r->rtPlus ? STR_RDS_RTPLUS_WAIT : STR_RDS_RTPLUS_NONE);
}

/*
 * Page 3: the alternative frequencies and the other networks.
 *
 * `RdsInfo.afKHz` and the EON list are decoded and tested, but like the
 * rest of this page they have not been checked against a live broadcast.
 */
static void fillNetworks(const RdsInfo *r, ScreenRds *view) {
  view->eonMoreHeard = r->eonMore;
  static char af[SCREEN_RDS_AF][16];
  static char eonPi[SCREEN_RDS_EON][8];
  static char eonPs[SCREEN_RDS_EON][RDS_PS_LEN + 1];
  static char eonFreqs[SCREEN_RDS_EON][40];
  const uint8_t n = r->afCount;
  const uint8_t shown = n > SCREEN_RDS_AF ? SCREEN_RDS_AF - 1 : n;
  for (uint8_t i = 0; i < shown; i++) {
    formatKHz(r->afKHz[i], af[i], sizeof(af[i]));
    view->af[i] = af[i];
  }
  view->afCount = shown;
  if (n > SCREEN_RDS_AF) {
    view->afMore = (uint8_t)(n - shown);
    snprintf(af[shown], sizeof(af[shown]), txt(STR_RDS_FMT_MORE),
             (unsigned)view->afMore);
    view->af[shown] = af[shown];
    view->afCount = SCREEN_RDS_AF;
  }
  for (uint8_t i = 0; i < r->eonCount; i++) {
    const RdsEon *e = &r->eon[i];
    /* One group naming it could be a block D corrected wrongly. */
    if (e->heard < 2) {
      continue;
    }
    if (view->eonCount == SCREEN_RDS_EON) {
      view->eonMore++;
      continue;
    }
    const uint8_t k = view->eonCount++;
    snprintf(eonPi[k], sizeof(eonPi[k]), "%04X", e->pi);
    view->eon[k].pi = eonPi[k];
    view->eon[k].ps = e->hasPs && rdsNameTrim(e->ps, sizeof(eonPs[k]), eonPs[k],
                                              sizeof(eonPs[k]))
                          ? eonPs[k]
                          : NULL;
    view->eon[k].ta = flag(e->hasTa, e->ta);
    /* Two whole frequencies and a count of the rest, so a row never
     * ends in half a number. */
    size_t at = 0;
    eonFreqs[k][0] = '\0';
    const uint8_t freqShown = e->afCount > 2 ? 2 : e->afCount;
    for (uint8_t f = 0; f < freqShown; f++) {
      char one[16];
      formatKHz(rdsAfCodeKHz(e->afCode[f]), one, sizeof(one));
      int w = snprintf(eonFreqs[k] + at, sizeof(eonFreqs[k]) - at, "%s%s",
                       f ? txt(STR_RDS_EON_SEPARATOR) : "", one);
      if (w < 0 || (size_t)w >= sizeof(eonFreqs[k]) - at) {
        break;
      }
      at += (size_t)w;
    }
    if ((e->afCount > freqShown || e->afMore) && at < sizeof(eonFreqs[k])) {
      /* A count only when it is the whole of the rest. */
      char more[8];
      if (e->afMore) {
        snprintf(more, sizeof(more), "%s", txt(STR_RDS_MORE));
      } else {
        snprintf(more, sizeof(more), txt(STR_RDS_FMT_MORE),
                 (unsigned)(e->afCount - freqShown));
      }
      snprintf(eonFreqs[k] + at, sizeof(eonFreqs[k]) - at, "%s%s",
               txt(STR_RDS_EON_SEPARATOR), more);
      at = strlen(eonFreqs[k]);
    }
    view->eon[k].freqs = at > 0 ? eonFreqs[k] : NULL;
  }
}

/* Page 4: the decoder's last minute. */
static void fillDecoder(const RdsInfo *r, ScreenRds *view) {
  static char rate[12];
  static char bler[12];
  static char lost[20];
  static char blockText[4][24];
  static char groupLabel[SCREEN_RDS_GROUPS][6];

  RdsMinuteStats m;
  rdsMinuteStats(&r->minute, SCREEN_RDS_GROUPS, &m);
  if (m.rateKnown) {
    snprintf(rate, sizeof(rate), txt(STR_RDS_FMT_RATE),
             (double)m.rateTenths / 10.0);
    view->rate = rate;
  }
  if (m.known) {
    snprintf(bler, sizeof(bler), txt(STR_RDS_FMT_BLER),
             (double)m.blerTenths / 10.0);
    view->bler = bler;
    snprintf(lost, sizeof(lost), txt(STR_RDS_FMT_BAD_BLOCKS),
             (unsigned)m.lostBlocks, (unsigned)m.totalBlocks);
    view->lost = lost;
  }
  view->blocksKnown = m.known;
  for (uint8_t b = 0; b < 4; b++) {
    view->block[b].fixedTenths = m.fixedTenths[b];
    view->block[b].lostTenths = m.lostTenths[b];
    const bool clean = r->minute.blocks[b][RDS_LEVEL_CORRECTED] == 0 &&
                       r->minute.blocks[b][RDS_LEVEL_LOST] == 0;
    if (m.known && !clean) {
      snprintf(blockText[b], sizeof(blockText[b]), txt(STR_RDS_FMT_BLOCK),
               (unsigned)m.fixed[b], (unsigned)(m.lostTenths[b] / 10),
               (unsigned)(m.lostTenths[b] % 10));
      view->block[b].text = blockText[b];
    }
  }
  for (uint8_t i = 0; i < m.groupCount; i++) {
    snprintf(groupLabel[i], sizeof(groupLabel[i]),
             txt(m.groupIsB[i] ? STR_RDS_FMT_GROUP_B : STR_RDS_FMT_GROUP_A),
             (unsigned)m.groupType[i]);
    view->group[i].label = groupLabel[i];
    view->group[i].percent = m.groupPercent[i];
  }
  view->groupCount = m.groupCount;
  if (m.groupCount == 0) {
    view->groupNote = txt(m.known ? STR_RDS_NO_TYPES : STR_RDS_NO_GROUPS);
  }
}

/* On a preset with a stored PI, the line under the PI names the preset, the
 * way the DX page does: its own station, shown as the stored PI from the
 * first matching block, or another one, confirmed, in red. */
static void fillPreset(const ScreenRdsInputs *in, ScreenRds *view) {
  if (in->presetSlot == MEMORY_NO_SLOT) {
    return;
  }
  DxIdentity id;
  dxStationIdentity(in->rds, &in->reading, in->region, in->presetPi, false,
                    &id);
  const DxPresetPi preset = id.preset;
  if (preset == DX_PRESET_NONE) {
    return;
  }
  static char line[16];
  snprintf(line, sizeof(line),
           txt(preset == DX_PRESET_MATCH ? STR_DX_FMT_PRESET
                                         : STR_DX_FMT_NOT_PRESET),
           (int)in->presetSlot + 1);
  view->country = line;
  view->countryNamed = true;
  view->countryGuess = false;
  view->countryFault = preset == DX_PRESET_OTHER;
  /* Always the stored PI on a match, since the decoder's own may still be an
   * older station's while the block just heard already matches. */
  if (preset == DX_PRESET_MATCH) {
    static char stored[8];
    snprintf(stored, sizeof(stored), "%04X", (unsigned)in->presetPi);
    view->pi = stored;
    view->piSure = true;
  }
}

void screenRdsStateBuild(const ScreenRdsInputs *in, ScreenRds *out) {
  if (out == NULL) {
    return;
  }
  /* Cleared first, so a caller that draws it anyway draws dashes. */
  memset(out, 0, sizeof(*out));
  if (in == NULL || in->rds == NULL) {
    return;
  }
  out->page = in->page;
  out->frequency = in->frequency;
  out->clock = in->clock;
  out->message = in->message;
  out->sync = in->sync;
  out->syncGood = in->syncGood;
  /* Page 4's header: how many seconds the counts cover, which is less than
   * a minute just after a tune. */
  static char span[48];
  if (in->frequency != NULL) {
    const uint32_t seconds = in->rds->minute.spanMs / 1000u;
    snprintf(span, sizeof(span), txt(STR_RDS_FMT_LAST_MINUTE), in->frequency,
             (unsigned)(seconds > 60u ? 60u : seconds));
    out->span = span;
  }
  fillStation(in->rds, in->region, out);
  fillPreset(in, out);
  fillText(in->rds, out);
  fillNetworks(in->rds, out);
  fillDecoder(in->rds, out);
}
