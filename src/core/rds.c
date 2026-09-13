/* Implementation of the RDS decoder. */
#include "rds.h"

#include <string.h>

/*
 * Only a block the tuner reports as clean is decoded. A block it says it
 * corrected is thrown away with the ones it could not.
 *
 * That is stricter than it sounds necessary, and it is measured rather than
 * chosen. Two captures were taken with the aerial collapsed, in
 * test/fixtures/rds/, where 91.1 and 106.4 read about 10 dBuV instead of the
 * usual 45 and the tuner corrected around 300 blocks in each. Replaying them:
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
 * captures about a third of the name passes are lost, so a name takes about
 * half as long again to appear. On any signal worth listening to every block
 * is clean and nothing changes at all.
 */
#define BLOCK_CLEAN 0

/* Where each thing sits in block B. */
#define GROUP_TYPE(b) ((uint8_t)((b) >> 12))
#define GROUP_IS_B_VERSION(b) (((b) & 0x0800u) != 0)
#define GROUP_TP(b) (((b) & 0x0400u) != 0)
#define GROUP_PTY(b) ((uint8_t)(((b) >> 5) & 0x1Fu))

/* Alternative frequency codes that are not an FM frequency. */
#define AF_FM_FIRST 1   /* 87.6 MHz. */
#define AF_FM_LAST 204  /* 107.9 MHz. */
#define AF_LOW_BAND 250 /* The other byte of the pair is long or medium wave. */

/* The bottom of the FM alternative frequency scale, in kHz. */
#define AF_BASE_KHZ 87500

/* Radio text ends here. Everything after it is padding, not text. */
#define RT_END 0x0D

static const char *const kPtyNames[RDS_PTY_COUNT] = {"None",
                                                     "News",
                                                     "Current affairs",
                                                     "Information",
                                                     "Sport",
                                                     "Education",
                                                     "Drama",
                                                     "Culture",
                                                     "Science",
                                                     "Varied",
                                                     "Pop music",
                                                     "Rock music",
                                                     "Easy listening",
                                                     "Light classical",
                                                     "Serious classical",
                                                     "Other music",
                                                     "Weather",
                                                     "Finance",
                                                     "Children",
                                                     "Social affairs",
                                                     "Religion",
                                                     "Phone in",
                                                     "Travel",
                                                     "Leisure",
                                                     "Jazz music",
                                                     "Country music",
                                                     "National music",
                                                     "Oldies music",
                                                     "Folk music",
                                                     "Documentary",
                                                     "Alarm test",
                                                     "Alarm"};

const char *rdsPtyName(uint8_t pty) {
  if (pty >= RDS_PTY_COUNT) {
    return "None";
  }
  return kPtyNames[pty];
}

/*
 * Turn one code from the broadcast into a character this radio can show.
 *
 * RDS has its own character set. Its printable range matches ASCII, and above
 * that are accented letters and symbols that the fonts here do not carry and
 * decision 6 does not ask for. Those become a question mark rather than a
 * space, because a question mark says a character was sent and could not be
 * shown, and a space says the station sent a space.
 */
static char displayable(uint8_t code) {
  if (code >= 0x20 && code <= 0x7E) {
    return (char)code;
  }
  return '?';
}

/*
 * Throw away everything that describes a station.
 *
 * Kept apart from rdsReset because a station can change under a dial that has
 * not moved, and then the lock and the frequency are still true while nothing
 * else is.
 */
static void clearStation(Rds *rds) {
  rds->info.hasPty = false;
  rds->info.pty = 0;
  rds->ptyCandidateSeen = false;
  rds->ptyCandidate = 0;

  rds->info.hasFlags = false;
  rds->info.tp = false;
  rds->info.ta = false;
  rds->info.speech = false;

  rds->info.hasPs = false;
  rds->info.ps[0] = '\0';
  memset(rds->psFrame, ' ', sizeof(rds->psFrame));
  memset(rds->psFrameHave, 0, sizeof(rds->psFrameHave));
  memset(rds->psPrevious, ' ', sizeof(rds->psPrevious));
  rds->psPreviousSeen = false;

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
static bool feedPi(Rds *rds, uint16_t pi) {
  /*
   * Zero is not an identifier. The standard keeps it for a station that has
   * not been given one, and two of the six stations that carry RDS here send
   * it: 94.3 and 95.0, in the captures of 13 September 2026. Publishing it
   * would state "0000" as though the station had said so, and a caller
   * comparing identifiers would decide those two stations are the same one.
   */
  if (pi == 0) {
    return false;
  }
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
  }
  rds->piCandidate = pi;
  rds->piCandidateSeen = true;
  return true;
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
 * The position always fits. It is the two bit segment address doubled, plus
 * nothing or one, so it runs from 0 to 7 and cannot leave the frame.
 */
static void feedPsChar(Rds *rds, int position, char ch) {
  rds->psFrame[position] = ch;
  rds->psFrameHave[position] = true;
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
    return;
  }
  for (int i = 0; i < end; i++) {
    rds->info.rt[i] = displayable((uint8_t)rds->rtBuffer[i]);
  }
  rds->info.rt[end] = '\0';
  rds->info.hasRt = true;
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
  if (code < AF_FM_FIRST || code > AF_FM_LAST) {
    return;
  }
  uint32_t khz = AF_BASE_KHZ + (uint32_t)code * 100u;
  if (khz == rds->tunedKHz) {
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
   * so the offset is applied here rather than left to every caller. In India
   * it is five and a half hours, so a clock that skipped this would be half a
   * working day out and would still look like a working clock.
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
 * way in put the count back to zero underneath itself, and the group that
 * changed the station came out as one used out of none seen.
 */
static void countGroup(Rds *rds, const RdsRead *read, bool used) {
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

  if (read == NULL || !read->synchronised) {
    if (rds->syncMissed < RDS_SYNC_LOSS_READS) {
      rds->syncMissed++;
    }
    if (rds->syncMissed >= RDS_SYNC_LOSS_READS) {
      rds->info.synchronised = false;
    }
  } else {
    rds->syncMissed = 0;
    rds->info.synchronised = true;
  }

  if (read == NULL || !read->haveGroup) {
    return;
  }

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

  switch (GROUP_TYPE(b)) {
    case 0: {
      rds->info.hasFlags = true;
      rds->info.tp = GROUP_TP(b);
      rds->info.ta = (b & 0x0010u) != 0;
      /* The standard's bit is set for music, so speech is its opposite. */
      rds->info.speech = (b & 0x0008u) == 0;

      int segment = (int)(b & 0x0003u);
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
    default:
      /* Every other group carries something this radio does not use. The
       * programme type and the traffic flag have already been taken from
       * block B, which every group carries. */
      break;
  }

  countGroup(rds, read, used);
}
