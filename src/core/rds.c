/* Implementation of the RDS decoder. */
#include "rds.h"

#include "rds_extra.h"

#include <string.h>

#include "core/strings.h"

/*
 * Only a block the tuner reports as clean is decoded. A block it says it
 * corrected is thrown away with the ones it could not.
 *
 * That is stricter than it sounds necessary, and it is measured rather than
 * chosen. With the aerial collapsed, 91.1 and 106.4 read about 10 dBuV instead
 * of the usual 45, and the tuner corrects around 300 blocks in each recording
 * of their groups. Replaying those groups:
 *
 * | Blocks accepted | Radio texts published on 106.4 |
 * |---|---|
 * | clean, corrected and badly corrected | 11, four of them corrupt |
 * | clean and corrected | 7, two of them corrupt |
 * | clean only | 3, which is exactly what the station sent |
 *
 * `MAwQCFM VINTOO...` and a text whose terminator was corrupted so it ran on
 * into padding both came through blocks the tuner had reported as corrected
 * with the smallest error it reports. The correction is checked against the
 * block's own check word, and on a weak signal it lands on the wrong codeword
 * often enough to matter.
 *
 * What it costs is only speed, and only on a weak signal. On those two
 * recordings about a third of the name passes are lost, so a name takes about
 * half as long again to appear. On any signal worth listening to every block
 * is clean and nothing changes at all.
 */
#define BLOCK_CLEAN RDS_BLOCK_CLEAN

/* Where each thing sits in block B, with GROUP_TYPE and GROUP_IS_B_VERSION
 * in rds_extra.h. */
#define GROUP_TP(b) (((b) & 0x0400u) != 0)
#define GROUP_PTY(b) ((uint8_t)(((b) >> 5) & 0x1Fu))

/* Radio text ends here. Everything after it is padding, not text. */
#define RT_END 0x0D

static const StrId kPtyNames[RDS_PTY_COUNT] = {STR_PTY_NONE,
                                               STR_PTY_NEWS,
                                               STR_PTY_CURRENT_AFFAIRS,
                                               STR_PTY_INFORMATION,
                                               STR_PTY_SPORT,
                                               STR_PTY_EDUCATION,
                                               STR_PTY_DRAMA,
                                               STR_PTY_CULTURE,
                                               STR_PTY_SCIENCE,
                                               STR_PTY_VARIED,
                                               STR_PTY_POP_MUSIC,
                                               STR_PTY_ROCK_MUSIC,
                                               STR_PTY_EASY_LISTENING,
                                               STR_PTY_LIGHT_CLASSICAL,
                                               STR_PTY_SERIOUS_CLASSICAL,
                                               STR_PTY_OTHER_MUSIC,
                                               STR_PTY_WEATHER,
                                               STR_PTY_FINANCE,
                                               STR_PTY_CHILDREN,
                                               STR_PTY_SOCIAL_AFFAIRS,
                                               STR_PTY_RELIGION,
                                               STR_PTY_PHONE_IN,
                                               STR_PTY_TRAVEL,
                                               STR_PTY_LEISURE,
                                               STR_PTY_JAZZ_MUSIC,
                                               STR_PTY_COUNTRY_MUSIC,
                                               STR_PTY_NATIONAL_MUSIC,
                                               STR_PTY_OLDIES_MUSIC,
                                               STR_PTY_FOLK_MUSIC,
                                               STR_PTY_DOCUMENTARY,
                                               STR_PTY_ALARM_TEST,
                                               STR_PTY_ALARM};

const char *rdsPtyName(uint8_t pty) {
  if (pty >= RDS_PTY_COUNT) {
    return txt(STR_PTY_NONE);
  }
  return txt(kPtyNames[pty]);
}

/*
 * RDS: The Radio Data System (Kopitz and Marks), Figure 3.4. Index is the
 * PI's own second nibble, 0 to 15.
 */
static const StrId kCoverageNames[16] = {
    STR_COVERAGE_LOCAL,       STR_COVERAGE_INTERNATIONAL,
    STR_COVERAGE_NATIONAL,    STR_COVERAGE_SUPRA_REGIONAL,
    STR_COVERAGE_REGIONAL_1,  STR_COVERAGE_REGIONAL_2,
    STR_COVERAGE_REGIONAL_3,  STR_COVERAGE_REGIONAL_4,
    STR_COVERAGE_REGIONAL_5,  STR_COVERAGE_REGIONAL_6,
    STR_COVERAGE_REGIONAL_7,  STR_COVERAGE_REGIONAL_8,
    STR_COVERAGE_REGIONAL_9,  STR_COVERAGE_REGIONAL_10,
    STR_COVERAGE_REGIONAL_11, STR_COVERAGE_REGIONAL_12};

const char *rdsPiOriginName(uint16_t pi) {
  return txt(kCoverageNames[(pi >> 8) & 0x0Fu]);
}

bool rdsFormatPiHeard(const RdsInfo *info, char *out) {
  if (out == NULL) {
    return false;
  }
  out[0] = '\0';
  if (info == NULL || !info->hasPiHeard) {
    return false;
  }
  static const char kHex[] = "0123456789ABCDEF";
  for (int i = 0; i < 4; i++) {
    int digit = 3 - i;
    out[i] = (info->piUnsureNibbles & (1u << digit)) != 0
                 ? '?'
                 : kHex[(info->piHeard >> (digit * 4)) & 0x0Fu];
  }
  out[4] = '\0';
  return true;
}

/*
 * Turn one code from the broadcast into a character this radio can show.
 *
 * RDS has its own character set. Its printable range matches ASCII, and above
 * that are accented letters and symbols that the fonts here do not carry,
 * since the panel is English only. Those become an underscore rather than a
 * space, because a space says the station sent a space and this did not.
 *
 * An underscore rather than a question mark. Both are stand ins and both are
 * characters a station can legitimately send, so the choice is about which
 * collision is more likely: radio text asks questions all the time and
 * rarely contains an underscore, so a `?` on the panel would be ambiguous
 * every time it appears.
 */
static char displayable(uint8_t code) {
  if (code >= 0x20 && code <= 0x7E) {
    return (char)code;
  }
  return '_';
}

char rdsDisplayable(uint8_t code) {
  return displayable(code);
}

/*
 * Throw away everything that describes a station.
 *
 * Kept apart from rdsReset because a station can change under a dial that has
 * not moved, and then the lock and the frequency are still true while nothing
 * else is.
 */
static void clearStation(Rds *rds) {
  rds->info.hasEcc = false;
  rds->info.ecc = 0;
  rds->eccCandidateSeen = false;
  rds->eccCandidate = 0;

  rds->info.hasPty = false;
  rds->info.pty = 0;
  rds->ptyCandidateSeen = false;
  rds->ptyCandidate = 0;

  rds->info.hasFlags = false;
  rds->info.tp = false;
  rds->info.ta = false;
  rds->info.speech = false;

  rds->info.hasDiStereo = false;
  rds->info.diStereo = false;
  rds->info.hasDiArtificialHead = false;
  rds->info.diArtificialHead = false;
  rds->info.hasDiCompressed = false;
  rds->info.diCompressed = false;
  rds->info.hasDiDynamicPty = false;
  rds->info.diDynamicPty = false;
  memset(rds->diCandidateSeen, 0, sizeof(rds->diCandidateSeen));

  rds->info.hasPtyn = false;
  rds->info.ptyn[0] = '\0';
  memset(rds->ptynFrame, ' ', sizeof(rds->ptynFrame));
  memset(rds->ptynFrameHave, 0, sizeof(rds->ptynFrameHave));
  memset(rds->ptynPrevious, ' ', sizeof(rds->ptynPrevious));
  rds->ptynPreviousSeen = false;

  rds->info.hasPin = false;
  rds->info.pinDay = 0;
  rds->info.pinHour = 0;
  rds->info.pinMinute = 0;
  rds->pinCandidateSeen = false;

  rds->info.hasPs = false;
  rds->info.ps[0] = '\0';
  memset(rds->info.psHeard, ' ', sizeof(rds->info.psHeard));
  memset(rds->info.psHeardHave, 0, sizeof(rds->info.psHeardHave));
  memset(rds->psFrame, ' ', sizeof(rds->psFrame));
  memset(rds->psFrameHave, 0, sizeof(rds->psFrameHave));
  memset(rds->psPrevious, ' ', sizeof(rds->psPrevious));
  rds->psPreviousSeen = false;
  rds->psJoinCount = 0;
  rds->info.hasPsLong = false;
  rds->info.psLong[0] = '\0';
  rds->psLastSeen = false;
  rds->psPairCount = 0;
  rds->psPairNext = 0;

  rds->info.hasRt = false;
  rds->info.rt[0] = '\0';
  memset(rds->rtBuffer, ' ', sizeof(rds->rtBuffer));
  memset(rds->rtHave, 0, sizeof(rds->rtHave));
  rds->rtFlag = false;
  rds->rtFlagSeen = false;
  rds->rtMaxSegment = 0;
  rds->rtMaxSegmentSeen = false;
  rds->rtMaxIsShort = false;
  rds->rtLastSegment = 0;
  rds->rtLastSegmentSeen = false;
  rds->rtWrapped = false;

  rds->info.afCount = 0;
  memset(rds->info.afKHz, 0, sizeof(rds->info.afKHz));

  memset(&rds->info.clock, 0, sizeof(rds->info.clock));

  rds->info.groupsSeen = 0;
  rds->info.groupsUsed = 0;
  rds->info.blocksCorrected = 0;
  rds->info.blocksBad = 0;
  memset(rds->info.groupTypeCount, 0, sizeof(rds->info.groupTypeCount));

  rdsExtraClear(rds);
}

void rdsReset(Rds *rds, uint32_t tunedKHz) {
  if (rds == NULL) {
    return;
  }
  memset(rds, 0, sizeof(*rds));
  rds->tunedKHz = tunedKHz;
  clearStation(rds);
}

/*
 * A field is published on its second matching reception, never its first.
 *
 * The tuner marks a block it could not correct, and those are already gone.
 * What is left is a block it corrected wrongly, which happens, and on the
 * identifier that means the radio decides the station changed and throws
 * away a name it had. Two receptions of the same value cost about a tenth of
 * a second on a station sending group 0 at the usual rate.
 */
static void hearPi(Rds *rds, uint16_t pi) {
  uint8_t unsure = 0;
  if (rds->info.hasPiHeard) {
    uint16_t changed = (uint16_t)(pi ^ rds->info.piHeard);
    for (int digit = 0; digit < 4; digit++) {
      if (((changed >> (digit * 4)) & 0x0Fu) != 0) {
        unsure |= (uint8_t)(1u << digit);
      }
    }
  }
  rds->info.piUnsureNibbles = unsure;
  rds->info.piHeard = pi;
  rds->info.hasPiHeard = true;
}

static bool feedPi(Rds *rds, uint16_t pi) {
  hearPi(rds, pi);
  /*
   * Zero is not an identifier. The standard keeps it for a station that has
   * not been given one, and two of the six RDS stations measured send it.
   * Publishing it would state "0000" as though the station had said so, and
   * a caller comparing identifiers would decide two such stations are the
   * same one.
   */
  if (pi == 0) {
    /* Its own state, confirmed the same way, and kept away from the
     * candidate: a 0000 heard between two hearings of a real identifier
     * must not stop those two agreeing. */
    if (rds->zeroCandidateSeen && !rds->info.hasPi) {
      rds->info.piZero = true;
    }
    rds->zeroCandidateSeen = true;
    return false;
  }
  rds->zeroCandidateSeen = false;
  if (rds->piCandidateSeen && rds->piCandidate == pi) {
    if (rds->info.hasPi && rds->info.pi != pi) {
      /* A different station on the same frequency, which is what a fade
       * between two transmitters looks like. Everything held describes the
       * station that has gone. The lock and the frequency are not touched,
       * because both are still true. */
      clearStation(rds);
    }
    rds->info.hasPi = true;
    rds->info.pi = pi;
    rds->info.piZero = false;
  }
  rds->piCandidate = pi;
  rds->piCandidateSeen = true;
  return true;
}

static void feedEcc(Rds *rds, uint8_t ecc) {
  if (rds->eccCandidateSeen && rds->eccCandidate == ecc) {
    rds->info.hasEcc = true;
    rds->info.ecc = ecc;
  }
  rds->eccCandidate = ecc;
  rds->eccCandidateSeen = true;
}

static void feedPty(Rds *rds, uint8_t pty) {
  if (rds->ptyCandidateSeen && rds->ptyCandidate == pty) {
    rds->info.hasPty = true;
    rds->info.pty = pty;
  }
  rds->ptyCandidate = pty;
  rds->ptyCandidateSeen = true;
}

/*
 * One of the four decoder identification bits, addressed the same way the
 * station name's own pair is addressed in the same group. Published once
 * its own address has agreed twice, the same rule every other field here
 * follows.
 *
 * The address-to-meaning table is RDS: The Radio Data System (Kopitz and
 * Marks), Table 4.3: 0 is dynamic PTY, 1 is compressed, 2 is artificial head, 3
 * is mono/stereo, the reverse of the order a first reading of the table
 * suggests. No recorded broadcast can confirm this order, so it rests on the
 * table itself.
 */
static void feedDi(Rds *rds, uint8_t address, bool bit) {
  if (rds->diCandidateSeen[address] && rds->diCandidate[address] == bit) {
    switch (address) {
      case 0:
        rds->info.hasDiDynamicPty = true;
        rds->info.diDynamicPty = bit;
        break;
      case 1:
        rds->info.hasDiCompressed = true;
        rds->info.diCompressed = bit;
        break;
      case 2:
        rds->info.hasDiArtificialHead = true;
        rds->info.diArtificialHead = bit;
        break;
      default:
        rds->info.hasDiStereo = true;
        rds->info.diStereo = bit;
        break;
    }
  }
  rds->diCandidate[address] = bit;
  rds->diCandidateSeen[address] = true;
}

/*
 * Programme Item Number, group type 1 block 4: the scheduled start of the
 * item now playing. Day, hour and minute are confirmed as one candidate
 * agreeing rather than three separately, so a day that repeats while the
 * hour changes under it is not mistaken for a real match.
 */
static void feedPin(Rds *rds, uint16_t blockD) {
  uint8_t day = (uint8_t)((blockD >> 11) & 0x1Fu);
  uint8_t hour = (uint8_t)((blockD >> 6) & 0x1Fu);
  uint8_t minute = (uint8_t)(blockD & 0x3Fu);
  /* RDS: The Radio Data System (Kopitz and Marks), Section 4.6: a valid
   * PIN holds a day from 1 to 31, an hour from 0 to 23 and a minute from
   * 0 to 59. Anything outside those ranges is no PIN being sent, not a
   * wrong one. */
  if (day < 1 || day > 31 || hour > 23 || minute > 59) {
    return;
  }
  if (rds->pinCandidateSeen && rds->pinCandidateDay == day &&
      rds->pinCandidateHour == hour && rds->pinCandidateMinute == minute) {
    rds->info.hasPin = true;
    rds->info.pinDay = day;
    rds->info.pinHour = hour;
    rds->info.pinMinute = minute;
  }
  rds->pinCandidateDay = day;
  rds->pinCandidateHour = hour;
  rds->pinCandidateMinute = minute;
  rds->pinCandidateSeen = true;
}

static void feedPtynChar(Rds *rds, int position, char ch) {
  rds->ptynFrame[position] = ch;
  rds->ptynFrameHave[position] = true;
}

/*
 * Close a programme type name pass once all eight positions have arrived,
 * and publish it once this pass and the one before it agree.
 *
 * No split-name stitching, unlike the station name: the standard forbids
 * using this field for anything but one fixed eight character
 * description, so there is nothing here for a ticker to split across
 * passes of.
 */
static void publishPtyn(Rds *rds) {
  for (int i = 0; i < RDS_PS_LEN; i++) {
    if (!rds->ptynFrameHave[i]) {
      return;
    }
  }
  if (rds->ptynPreviousSeen &&
      memcmp(rds->ptynFrame, rds->ptynPrevious, RDS_PS_LEN) == 0) {
    memcpy(rds->info.ptyn, rds->ptynFrame, RDS_PS_LEN);
    rds->info.ptyn[RDS_PS_LEN] = '\0';
    rds->info.hasPtyn = true;
  }
  memcpy(rds->ptynPrevious, rds->ptynFrame, RDS_PS_LEN);
  rds->ptynPreviousSeen = true;
  memset(rds->ptynFrameHave, 0, sizeof(rds->ptynFrameHave));
}

/*
 * The position always fits. It is the two bit segment address doubled, plus
 * nothing or one, so it runs from 0 to 7 and cannot leave the frame.
 */
static void feedPsChar(Rds *rds, int position, char ch) {
  rds->psFrame[position] = ch;
  rds->psFrameHave[position] = true;
  rds->info.psHeard[position] = ch;
  rds->info.psHeardHave[position] = true;
}

/* Whether a name runs right up to its last character rather than padding. */
static bool psRunsOn(const char *name) {
  return name[RDS_PS_LEN - 1] != ' ';
}

/* Whether a name starts on a character rather than being padded or centred. */
static bool psStartsFlush(const char *name) {
  return name[0] != ' ';
}

/*
 * Build the long name out of the confirmed run.
 *
 * Padding at either end goes, because it is padding: a second pass of `5`
 * and seven spaces would make `SAMPLE 95` followed by seven spaces, which
 * is aligned nowhere in particular. Spaces inside are kept, because they are
 * part of what was sent.
 */
static void buildPsLong(Rds *rds) {
  char built[RDS_PS_LONG_LEN + 1];
  size_t n = 0;
  for (uint8_t i = 0; i < rds->psJoinCount; i++) {
    for (int j = 0; j < RDS_PS_LEN && n < RDS_PS_LONG_LEN; j++) {
      built[n++] = rds->psJoinSeg[i][j];
    }
  }
  /* Both ends. Only the second piece of a pair has to start flush, so the
   * first can be padded or centred, and a name drawn one space in sits
   * against nothing while every other label lines up. */
  size_t start = 0;
  while (start < n && built[start] == ' ') {
    start++;
  }
  while (n > start && built[n - 1] == ' ') {
    n--;
  }
  memcpy(rds->info.psLong, built + start, n - start);
  rds->info.psLong[n - start] = '\0';
}

/*
 * Whether the run held is a whole name rather than part of one.
 *
 * A name cut into eight character pieces ends where the string ends, so its
 * last piece has room left in it. Every piece before the last is full, which
 * is what made it a piece.
 *
 * This is what stops a chain of confirmed pairs being published as a name.
 * A station rotating four names that all fill the field produces four
 * qualifying pairs, each one extends the run, and the panel is handed
 * `AAAAAAAABBBBBBBBCCCCCCCC...`, which the station never sent as one thing.
 * Requiring the last piece to have room refuses all of them, because a
 * ticker of full names never has a last piece.
 */
static bool psRunIsWhole(const Rds *rds) {
  return rds->psJoinCount >= 2 &&
         !psRunsOn(rds->psJoinSeg[rds->psJoinCount - 1]);
}

/* Forget the run. Used when it turns out not to be a name. */
static void dropPsRun(Rds *rds) {
  rds->psJoinCount = 0;
  rds->info.hasPsLong = false;
  rds->info.psLong[0] = '\0';
}

/*
 * Take a pair that has now been seen twice and make it the run.
 *
 * A pair that carries on from the run already held extends it, which is what
 * a name split across three passes looks like. Anything else starts again:
 * two unrelated splits in one rotation are two names, not one longer one.
 *
 * A run that wants to grow past the cap is dropped rather than trimmed or
 * restarted. Both of those publish a piece of something instead of the thing,
 * and a station that splits a name across more than four passes is sending
 * something that is not a name. So it is left alone, and the panel shows the
 * eight character pass.
 */
static void confirmPsPair(Rds *rds, const char *a, const char *b) {
  const bool carriesOn =
      rds->psJoinCount > 0 &&
      memcmp(rds->psJoinSeg[rds->psJoinCount - 1], a, RDS_PS_LEN) == 0;
  if (carriesOn) {
    if (rds->psJoinCount >= RDS_PS_JOIN_MAX) {
      dropPsRun(rds);
      return;
    }
    memcpy(rds->psJoinSeg[rds->psJoinCount], b, RDS_PS_LEN + 1);
    rds->psJoinCount++;
  } else {
    memcpy(rds->psJoinSeg[0], a, RDS_PS_LEN + 1);
    memcpy(rds->psJoinSeg[1], b, RDS_PS_LEN + 1);
    rds->psJoinCount = 2;
  }
}

/* Whether a name is one of the passes the confirmed run is made of. */
static bool psInJoin(const Rds *rds, const char *name) {
  for (uint8_t i = 0; i < rds->psJoinCount; i++) {
    if (memcmp(rds->psJoinSeg[i], name, RDS_PS_LEN) == 0) {
      return true;
    }
  }
  return false;
}

/*
 * Watch the name change, and learn a split when the same change happens
 * twice.
 *
 * Called with every name this decoder publishes, including the same one
 * again. A name that has not changed is not a change, and counting it as one
 * would confirm a pair off a single transmission.
 */
static void notePsPublished(Rds *rds, const char *name) {
  if (rds->psLastSeen && memcmp(rds->psLast, name, RDS_PS_LEN) != 0) {
    if (psRunsOn(rds->psLast) && psStartsFlush(name)) {
      bool seenBefore = false;
      for (uint8_t i = 0; i < rds->psPairCount; i++) {
        if (memcmp(rds->psPairA[i], rds->psLast, RDS_PS_LEN) == 0 &&
            memcmp(rds->psPairB[i], name, RDS_PS_LEN) == 0) {
          seenBefore = true;
          break;
        }
      }
      if (seenBefore) {
        confirmPsPair(rds, rds->psLast, name);
      } else {
        memcpy(rds->psPairA[rds->psPairNext], rds->psLast, RDS_PS_LEN + 1);
        memcpy(rds->psPairB[rds->psPairNext], name, RDS_PS_LEN + 1);
        rds->psPairNext = (uint8_t)((rds->psPairNext + 1) % RDS_PS_PAIRS);
        if (rds->psPairCount < RDS_PS_PAIRS) {
          rds->psPairCount++;
        }
      }
    }
  }
  memcpy(rds->psLast, name, RDS_PS_LEN + 1);
  rds->psLastSeen = true;

  /*
   * Offered only while the name on screen is part of a run that is a whole
   * name. A station that rotates a split name among unrelated ones goes back
   * to showing the pass when it moves on to one of those.
   *
   * Built here rather than when the run is confirmed, so the string and the
   * flag can never disagree. Setting the flag from the run and the string
   * somewhere else can leave `hasPsLong` true beside an empty `psLong` when
   * the rotation comes back round, and an empty name is drawn as no name at
   * all, so the station name would vanish from the panel for a whole pass.
   */
  if (psRunIsWhole(rds) && psInJoin(rds, name)) {
    buildPsLong(rds);
    rds->info.hasPsLong = true;
  } else {
    rds->info.psLong[0] = '\0';
    rds->info.hasPsLong = false;
  }
}

/*
 * Close a pass once all eight positions of it have arrived, and publish the
 * name when this pass and the one before it agree.
 */
static void publishPs(Rds *rds) {
  for (int i = 0; i < RDS_PS_LEN; i++) {
    if (!rds->psFrameHave[i]) {
      return;
    }
  }
  if (rds->psPreviousSeen &&
      memcmp(rds->psFrame, rds->psPrevious, RDS_PS_LEN) == 0) {
    memcpy(rds->info.ps, rds->psFrame, RDS_PS_LEN);
    rds->info.ps[RDS_PS_LEN] = '\0';
    rds->info.hasPs = true;
    notePsPublished(rds, rds->info.ps);
  }
  memcpy(rds->psPrevious, rds->psFrame, RDS_PS_LEN);
  rds->psPreviousSeen = true;
  memset(rds->psFrameHave, 0, sizeof(rds->psFrameHave));
}

/*
 * Remember the highest segment the station has sent in this text.
 *
 * That is what says how long the text is on a station that never sends a
 * terminator. It is taken from the segment address, which rides in block B
 * and is only read from a clean one, so damage can delay it but cannot make
 * it smaller.
 */
static void feedRtSegment(Rds *rds, uint8_t segment, bool shortSegments) {
  /* Going backwards means the station has finished a pass and started
   * again, which is what makes the highest segment a length rather than
   * simply how far this pass has got. */
  if (rds->rtLastSegmentSeen && segment <= rds->rtLastSegment) {
    rds->rtWrapped = true;
  }
  rds->rtLastSegment = segment;
  rds->rtLastSegmentSeen = true;

  if (!rds->rtMaxSegmentSeen || segment > rds->rtMaxSegment) {
    rds->rtMaxSegment = segment;
  }
  rds->rtMaxIsShort = shortSegments;
  rds->rtMaxSegmentSeen = true;
}

/* How long the text is, from the highest segment sent. 0 means not known. */
static int rtLengthFromSegments(const Rds *rds) {
  if (!rds->rtMaxSegmentSeen || !rds->rtWrapped) {
    return 0;
  }
  int perSegment = rds->rtMaxIsShort ? 2 : 4;
  int length = ((int)rds->rtMaxSegment + 1) * perSegment;
  return length > RDS_RT_LEN ? RDS_RT_LEN : length;
}

/*
 * Publish the radio text once it is known where it ends.
 *
 * Three things can say where. The terminator, which is what a station is
 * supposed to send. All 64 characters having arrived, which is a text that
 * fills the field. Or two passes having ended at the same length on a station
 * that sends neither, which is what feedRtSegment works out.
 *
 * Segments do not arrive in order, so a buffer with holes in it is normal on
 * the way to a complete one. Publishing one puts spaces in the middle of a
 * sentence, and those look exactly like spaces the station sent.
 */
static void publishRt(Rds *rds) {
  int end = RDS_RT_LEN;
  for (int i = 0; i < RDS_RT_LEN; i++) {
    if (rds->rtHave[i] && rds->rtBuffer[i] == RT_END) {
      end = i;
      break;
    }
  }
  if (end == RDS_RT_LEN) {
    int fromSegments = rtLengthFromSegments(rds);
    if (fromSegments > 0) {
      end = fromSegments;
    }
  }
  for (int i = 0; i < end; i++) {
    if (!rds->rtHave[i]) {
      return;
    }
  }
  /* Trailing spaces are padding to the segment boundary, not text. */
  while (end > 0 && rds->rtBuffer[end - 1] == ' ') {
    end--;
  }
  if (end == 0) {
    /* A terminator in the first position, or a text that is all padding. The
     * station is saying it has nothing to show, which happens between songs.
     * Published as nothing rather than as an empty string, because a reader
     * cannot tell an empty string from a text that has not arrived. Holding
     * the last song's title instead would be worse still: the station has
     * just said that is no longer what is playing. */
    rds->info.hasRt = false;
    rds->info.rt[0] = '\0';
    /* The tags pointed into the text that has gone. */
    rdsRtPlusTextChanged(rds);
    return;
  }
  char text[RDS_RT_LEN + 1];
  for (int i = 0; i < end; i++) {
    text[i] = displayable((uint8_t)rds->rtBuffer[i]);
  }
  text[end] = '\0';
  if (rds->info.hasRt && strcmp(text, rds->info.rt) != 0) {
    /* A new text under the same A/B flag, which a station may send: the
     * RT+ tags pointed into the old one. */
    rdsRtPlusTextChanged(rds);
  }
  memcpy(rds->info.rt, text, (size_t)end + 1);
  rds->info.hasRt = true;
  /* Tags heard before the text was whole name nothing until now. */
  rdsRtPlusKeepStationShort(rds);
}

/*
 * The position always fits. It is the four bit segment address times four
 * plus up to three in an A group, and times two plus one in a B group, so at
 * most 63 either way.
 */
static void feedRtChar(Rds *rds, int position, uint8_t code) {
  rds->rtBuffer[position] = (char)code;
  rds->rtHave[position] = true;
}

static void feedRtFlag(Rds *rds, bool flag) {
  if (rds->rtFlagSeen && rds->rtFlag == flag) {
    return;
  }
  if (rds->rtFlagSeen) {
    /* The station toggled the flag, which is how it says the text has
     * changed. Keeping the old characters would leave the tail of the
     * previous message on the end of the new one. */
    memset(rds->rtBuffer, ' ', sizeof(rds->rtBuffer));
    memset(rds->rtHave, 0, sizeof(rds->rtHave));
    rds->info.hasRt = false;
    rds->info.rt[0] = '\0';
    rdsRtPlusTextChanged(rds);
    /* The new text may be a different length, so how far the last one went
     * says nothing about how far this one does. */
    rds->rtMaxSegment = 0;
    rds->rtMaxSegmentSeen = false;
    rds->rtMaxIsShort = false;
    rds->rtLastSegmentSeen = false;
    rds->rtWrapped = false;
  }
  rds->rtFlag = flag;
  rds->rtFlagSeen = true;
}

static void addAf(Rds *rds, uint8_t code) {
  const uint32_t khz = rdsAfCodeKHz(code);
  if (khz == 0 || khz == rds->tunedKHz) {
    return;
  }
  for (uint8_t i = 0; i < rds->info.afCount; i++) {
    if (rds->info.afKHz[i] == khz) {
      return;
    }
  }
  if (rds->info.afCount >= RDS_AF_MAX) {
    return;
  }
  rds->info.afKHz[rds->info.afCount++] = khz;
}

static void feedAf(Rds *rds, uint16_t blockC) {
  uint8_t first = (uint8_t)(blockC >> 8);
  uint8_t second = (uint8_t)(blockC & 0xFF);
  if (first == AF_LOW_BAND) {
    /* The pair names a long or medium wave frequency. Read past it: this
     * radio would have to change band to follow one, and taking the second
     * byte as an FM code would put a frequency in the list that the station
     * never named. */
    return;
  }
  addAf(rds, first);
  addAf(rds, second);
}

/*
 * Turn a modified Julian day into a date.
 *
 * The inverse formula from the RDS standard itself, which is where the date
 * comes from, so the two agree by construction. Integer only.
 */
static bool mjdToDate(uint32_t mjd, uint16_t *year, uint8_t *month,
                      uint8_t *day) {
  /* The formula starts at 1 March 1900, which is modified Julian day 15079.
   * Below that its divisions go negative and the answer is meaningless. */
  if (mjd < 15079u) {
    return false;
  }
  /* yPrime is int((mjd - 15078.2) / 365.25), scaled by twenty so the .2 and
   * the .25 both come out whole. */
  uint32_t yPrime = (mjd * 20u - 301564u) / 7305u;
  uint32_t yDays = yPrime * 36525u / 100u; /* int(yPrime * 365.25) */
  uint32_t t = mjd - 14956u - yDays;
  /* mPrime is int((t - 0.1) / 30.6001), scaled by ten thousand. */
  uint32_t mPrime = (t * 10000u - 1000u) / 306001u;
  uint32_t d = t - mPrime * 306001u / 10000u; /* int(mPrime * 30.6001) */
  uint32_t k = (mPrime == 14u || mPrime == 15u) ? 1u : 0u;
  *year = (uint16_t)(yPrime + k + 1900u);
  *month = (uint8_t)(mPrime - 1u - k * 12u);
  *day = (uint8_t)d;
  return true;
}

static void feedClock(Rds *rds, uint16_t blockB, uint16_t blockC,
                      uint16_t blockD) {
  uint32_t mjd = ((uint32_t)(blockB & 0x0003u) << 15) | (uint32_t)(blockC >> 1);
  uint8_t utcHour = (uint8_t)(((blockC & 0x0001u) << 4) | (blockD >> 12));
  uint8_t utcMinute = (uint8_t)((blockD >> 6) & 0x3Fu);
  int8_t offset = (int8_t)(blockD & 0x1Fu);
  if ((blockD & 0x0020u) != 0) {
    offset = (int8_t)(-offset);
  }

  if (mjd == 0 || utcHour > 23 || utcMinute > 59) {
    return;
  }

  /*
   * The group carries the time in UTC and how far the station's own time is
   * from it, as two separate things. What a person wants is the second one,
   * so the offset is applied here rather than left to every caller. At
   * +5:30, for example, a clock that skipped this would be half a working
   * day out and would still look like a working clock.
   *
   * The offset carries the time over midnight often enough to matter, so the
   * date moves with it. It is five bits of half hours, at most 15.5 hours, so
   * the day can only ever move by one and no loop is needed.
   */
  int32_t minutes =
      (int32_t)utcHour * 60 + (int32_t)utcMinute + (int32_t)offset * 30;
  int32_t dayShift = 0;
  if (minutes < 0) {
    minutes += 24 * 60;
    dayShift = -1;
  } else if (minutes >= 24 * 60) {
    minutes -= 24 * 60;
    dayShift = 1;
  }
  uint8_t hour = (uint8_t)(minutes / 60);
  uint8_t minute = (uint8_t)(minutes % 60);

  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  if (!mjdToDate((uint32_t)((int32_t)mjd + dayShift), &year, &month, &day)) {
    return;
  }
  /* The formula is only defined from 1900, and a station sending a date this
   * radio cannot have been switched on for is sending a wrong one. Checking
   * it here is what stops a caller being handed a confident wrong date. */
  if (year < 1900 || year > 2099 || month < 1 || month > 12 || day < 1 ||
      day > 31) {
    return;
  }

  rds->info.clock.valid = true;
  rds->info.clock.year = year;
  rds->info.clock.month = month;
  rds->info.clock.day = day;
  rds->info.clock.hour = hour;
  rds->info.clock.minute = minute;
  rds->info.clock.offsetHalfHours = offset;
}

/*
 * Count one group, after it has been decoded and not before.
 *
 * The order matters. A confirmed change of identifier means a different
 * station on the same frequency, and clearStation zeroes these counters
 * because they described the station that has gone. Counting the group on the
 * way in would put the count back to zero underneath itself, and the group
 * that changed the station would come out as one used out of none seen.
 */
static void countGroup(Rds *rds, const RdsRead *read, bool used) {
  const bool bOk = read->error[1] == BLOCK_CLEAN;
  rdsWindowGroup(rds, read, bOk, GROUP_TYPE(read->block[1]),
                 GROUP_IS_B_VERSION(read->block[1]));
  rds->info.groupsSeen++;
  if (used) {
    rds->info.groupsUsed++;
  }
  for (int i = 0; i < 4; i++) {
    if (read->error[i] == BLOCK_CLEAN) {
      continue;
    }
    if (read->error[i] >= 3) {
      rds->info.blocksBad++;
    } else {
      rds->info.blocksCorrected++;
    }
  }
}

void rdsFeed(Rds *rds, const RdsRead *read) {
  if (rds == NULL) {
    return;
  }
  if (read != NULL) {
    rdsWindowTick(rds, read->atMs);
  }

  if (read == NULL || !read->synchronised) {
    if (rds->syncMissed < RDS_SYNC_LOSS_READS) {
      rds->syncMissed++;
    }
    if (rds->syncMissed >= RDS_SYNC_LOSS_READS) {
      rds->info.synchronised = false;
      rds->info.hasBlockErrors = false;
    }
  } else {
    rds->syncMissed = 0;
    rds->info.synchronised = true;
  }

  if (read == NULL || !read->haveGroup) {
    return;
  }

  rds->info.hasBlockErrors = true;
  memcpy(rds->info.blockError, read->error, sizeof(rds->info.blockError));

  bool aOk = read->error[0] == BLOCK_CLEAN;
  bool bOk = read->error[1] == BLOCK_CLEAN;
  bool cOk = read->error[2] == BLOCK_CLEAN;
  bool dOk = read->error[3] == BLOCK_CLEAN;
  bool used = false;

  if (aOk && feedPi(rds, read->block[0])) {
    used = true;
  }

  /* Everything else is addressed by block B, so a group whose block B did not
   * survive cannot be decoded at all: there is no way to know what it was. */
  if (!bOk) {
    countGroup(rds, read, used);
    return;
  }

  uint16_t b = read->block[1];
  feedPty(rds, GROUP_PTY(b));
  used = true;

  /* Every group type this station actually sends, whether or not the
   * switch below does anything with it. */
  rds->info.groupTypeCount[GROUP_TYPE(b)][GROUP_IS_B_VERSION(b) ? 1 : 0]++;

  /* The group the station said carries RT+, whichever number it is. */
  if (rdsIsRtPlusGroup(rds, b)) {
    if (cOk && dOk) {
      rdsFeedRtPlusTags(rds, b, read->block[2], read->block[3]);
    }
    countGroup(rds, read, used);
    return;
  }

  switch (GROUP_TYPE(b)) {
    case 0: {
      rds->info.hasFlags = true;
      rds->info.tp = GROUP_TP(b);
      rds->info.ta = (b & 0x0010u) != 0;
      /* The standard's bit is set for music, so speech is its opposite. */
      rds->info.speech = (b & 0x0008u) == 0;

      int segment = (int)(b & 0x0003u);
      /* The DI bit itself lives in block B, so it needs neither block C
       * nor D to survive. RDS: The Radio Data System, Section 4.5: the
       * segment address is shared between the station name pair and this
       * bit in the same group. */
      feedDi(rds, (uint8_t)segment, (b & 0x0004u) != 0);
      if (dOk) {
        feedPsChar(rds, segment * 2,
                   displayable((uint8_t)(read->block[3] >> 8)));
        feedPsChar(rds, segment * 2 + 1,
                   displayable((uint8_t)(read->block[3] & 0xFF)));
        publishPs(rds);
      }
      /* Only the A version carries alternative frequencies. In a 0B group
       * block C is the identifier again, and reading it as two frequency
       * codes invents two stations. */
      if (cOk && !GROUP_IS_B_VERSION(b)) {
        feedAf(rds, read->block[2]);
      }
      break;
    }
    case 2: {
      feedRtFlag(rds, (b & 0x0010u) != 0);
      int segment = (int)(b & 0x000Fu);
      feedRtSegment(rds, (uint8_t)segment, GROUP_IS_B_VERSION(b));
      if (!GROUP_IS_B_VERSION(b)) {
        if (cOk) {
          feedRtChar(rds, segment * 4, (uint8_t)(read->block[2] >> 8));
          feedRtChar(rds, segment * 4 + 1, (uint8_t)(read->block[2] & 0xFF));
        }
        if (dOk) {
          feedRtChar(rds, segment * 4 + 2, (uint8_t)(read->block[3] >> 8));
          feedRtChar(rds, segment * 4 + 3, (uint8_t)(read->block[3] & 0xFF));
        }
      } else if (dOk) {
        /* The B version sends half as much text and repeats the identifier
         * in block C, so its segments are two characters wide. */
        feedRtChar(rds, segment * 2, (uint8_t)(read->block[3] >> 8));
        feedRtChar(rds, segment * 2 + 1, (uint8_t)(read->block[3] & 0xFF));
      }
      publishRt(rds);
      break;
    }
    case 4: {
      /* 4B is not clock time. It has no defined use, so decoding it as a
       * date would turn whatever a station puts there into a wrong clock. */
      if (!GROUP_IS_B_VERSION(b) && cOk && dOk) {
        feedClock(rds, b, read->block[2], read->block[3]);
      }
      break;
    }
    case 1: {
      /* Both versions carry the PIN in block 4, RDS: The Radio Data
       * System, Section 4.6; only the A version's block 3 differs, and
       * this radio does not read block 3 either version. */
      if (dOk) {
        feedPin(rds, read->block[3]);
      }
      /* The ECC is the low eight bits of block C in the A version, when
       * the variant code in bits 12 to 14 is 0 (EN 50067, group 1A). The
       * other variants carry paging, the language and so on in the same
       * place, and the B version has the PI in block C. */
      if (!GROUP_IS_B_VERSION(b) && cOk &&
          ((read->block[2] >> 12) & 0x07u) == 0) {
        feedEcc(rds, (uint8_t)(read->block[2] & 0xFFu));
      }
      /* Variant 3 carries the programme language in the same place. */
      if (!GROUP_IS_B_VERSION(b) && cOk &&
          ((read->block[2] >> 12) & 0x07u) == 3) {
        rdsFeedLanguage(rds, read->block[2]);
      }
      break;
    }
    case 3: {
      /* 3A says which group an Open Data Application rides in; only RT+
       * and TMC are read. 3B is itself an ODA group. */
      if (!GROUP_IS_B_VERSION(b) && dOk) {
        rdsFeedOdaAnnounce(rds, b, read->block[3]);
      }
      break;
    }
    case 14: {
      /* Block D is the other station's PI in both versions. */
      if (dOk) {
        rdsFeedEon(rds, b, read->block[2], cOk, read->block[3]);
      }
      break;
    }
    case 10: {
      /* Only the A version. 10B carries something the standard leaves
       * undefined for this group number, the same reasoning group 4B
       * already gets above. */
      if (!GROUP_IS_B_VERSION(b) && cOk && dOk) {
        int segment = (int)(b & 0x0001u);
        feedPtynChar(rds, segment * 4,
                     displayable((uint8_t)(read->block[2] >> 8)));
        feedPtynChar(rds, segment * 4 + 1,
                     displayable((uint8_t)(read->block[2] & 0xFF)));
        feedPtynChar(rds, segment * 4 + 2,
                     displayable((uint8_t)(read->block[3] >> 8)));
        feedPtynChar(rds, segment * 4 + 3,
                     displayable((uint8_t)(read->block[3] & 0xFF)));
        publishPtyn(rds);
      }
      break;
    }
    default:
      /* Every other group carries something this radio does not use. The
       * programme type and the traffic flag have already been taken from
       * block B, which every group carries. */
      break;
  }

  countGroup(rds, read, used);
}

bool rdsNameTrim(const char *name, size_t len, char *out, size_t cap) {
  if (out == NULL || cap == 0) {
    return false;
  }
  out[0] = '\0';
  if (name == NULL) {
    return false;
  }
  size_t end = 0;
  while (end < len && name[end] != '\0') {
    end++;
  }
  size_t from = 0;
  while (from < end && name[from] == ' ') {
    from++;
  }
  while (end > from && name[end - 1] == ' ') {
    end--;
  }
  size_t n = end - from;
  if (n > cap - 1) {
    n = cap - 1;
  }
  memcpy(out, name + from, n);
  out[n] = '\0';
  return n > 0;
}
