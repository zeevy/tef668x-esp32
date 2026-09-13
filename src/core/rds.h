/*
 * The RDS decoder.
 *
 * FM stations send a slow data stream alongside the audio: the station name,
 * what is playing, the programme type, a station identifier, the frequencies
 * the same programme is on elsewhere, and the time. This turns the groups the
 * tuner hands over into those things.
 *
 * No hardware here. The tuner reads a group, the radio task copies it into an
 * RdsRead, and this works out what it means. That is what lets the awkward
 * parts be tested on a PC against real groups captured off air, which is what
 * `test/fixtures/rds/` holds.
 *
 * The rule this decoder is built around is that a broadcast is noisy and a
 * wrong station name is worse than no station name. Every field here is
 * published only once it has been received twice the same way, and only ever
 * out of blocks the tuner reported as clean. So a caller that sees `hasPs`
 * knows the name was heard twice, and a caller that does not see it knows the
 * radio cannot say yet. There is no third state where a half received name is
 * shown as though it were the name.
 */
#ifndef CORE_RDS_H
#define CORE_RDS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Station name, in characters. Fixed by the standard. */
#define RDS_PS_LEN 8

/* Radio text, in characters. 64 from a 2A group, 32 from a 2B. */
#define RDS_RT_LEN 64

/*
 * How many alternative frequencies are kept.
 *
 * The standard lets a station name up to 25 in one list, so a longer list
 * cannot happen and a station is never cut short by this.
 */
#define RDS_AF_MAX 25

/*
 * How many reads without a lock before the decoder is called out of sync.
 *
 * Two, which is 87 ms at the 43 ms group cadence, and is what the reference
 * firmware waits. One read is not enough: the lock flag drops for a single
 * read on a station that is decoding perfectly well.
 */
#define RDS_SYNC_LOSS_READS 2

/* Programme types, 0 to 31. What the number means depends on the region. */
#define RDS_PTY_COUNT 32

/*
 * The date and time a station sent, in the station's own local time.
 *
 * The broadcast carries UTC and the offset as two separate things. The offset
 * is applied here, date included, because it carries the time over midnight
 * and every caller doing that arithmetic again is every caller getting a
 * chance to get it wrong.
 */
typedef struct {
  bool valid;    /* Nothing below means anything until this is set. */
  uint16_t year; /* Four digits. */
  uint8_t month; /* 1 to 12. */
  uint8_t day;   /* 1 to 31. */
  uint8_t hour;  /* 0 to 23, local. */
  uint8_t minute;
  /* How far the local time above is from UTC, in half hours. Already
   * applied, so it is there to be shown, not to be added again. */
  int8_t offsetHalfHours;
} RdsClockTime;

/*
 * One read of the tuner's RDS decoder.
 *
 * The same shape drivers/tef668x.h produces, restated here so that `core/`
 * does not include a driver. The radio task copies one into the other.
 *
 * `haveGroup` false is an ordinary answer, not a failure. Most reads have
 * nothing new in them, because the chip is asked more often than a group
 * arrives.
 */
typedef struct {
  bool synchronised; /* The tuner is locked to an RDS bit stream. */
  bool haveGroup; /* A new group arrived. `block` means nothing without it. */
  uint16_t block[4]; /* A, B, C, D. */
  uint8_t error[4]; /* Per block: 0 clean, 1 or 2 corrected, 3 not corrected. */
} RdsRead;

/*
 * What the decoder knows.
 *
 * Separate from the decoder itself so that this can be copied into a state
 * snapshot without carrying the half assembled buffers with it. A caller that
 * could see those could show a name that is still being received.
 */
typedef struct {
  /* The tuner is locked to a bit stream, so this station carries RDS. */
  bool synchronised;

  bool hasPi;  /* A station identifier has been heard twice. */
  uint16_t pi; /* The identifier itself. Its top nibble is the country. */

  bool hasPty; /* A programme type has been heard twice. */
  uint8_t pty; /* 0 to 31. */

  /*
   * The three flags that ride in every group 0.
   *
   * They are published together because they arrive together and none of
   * them means anything before the first group 0 has been heard. A radio
   * that shows "music" before it has heard anything is stating a fact it
   * does not have.
   */
  bool hasFlags;
  bool tp;     /* This station carries traffic announcements at some point. */
  bool ta;     /* One is on air right now. */
  bool speech; /* True for speech, false for music. */

  bool hasPs;              /* Two complete passes arrived saying the same. */
  char ps[RDS_PS_LEN + 1]; /* The station name. */

  bool hasRt;              /* A complete radio text has been received. */
  char rt[RDS_RT_LEN + 1]; /* What is playing, or whatever else it sends. */

  /*
   * Frequencies carrying the same programme, in kHz.
   *
   * FM entries only. A station may also name a long or medium wave
   * alternative, and those are read past rather than stored, because this
   * radio has nothing that would follow one.
   *
   * The tuned frequency itself is never in this list. Stations send it as
   * part of their own list, and offering a person the station they are
   * already on is not an alternative.
   */
  uint8_t afCount;
  uint32_t afKHz[RDS_AF_MAX];

  RdsClockTime clock;

  /*
   * How the decoding is going. For a diagnostics page, and for judging a
   * station rather than guessing at it.
   *
   * `groupsSeen` counts every group the tuner handed over. `groupsUsed`
   * counts the ones this decoder took something from, so a station whose
   * groups all arrive damaged shows a used count of zero against a rising
   * seen count.
   *
   * `blocksCorrected` counts blocks the tuner said it had put right and
   * `blocksBad` the ones it could not. Both are thrown away here, and they
   * are counted apart because they say different things about the signal: a
   * rising corrected count on a station that still decodes is a signal
   * getting worse, and a rising bad count is one already too weak to use.
   */
  uint32_t groupsSeen;
  uint32_t groupsUsed;
  uint32_t blocksCorrected;
  uint32_t blocksBad;
} RdsInfo;

/*
 * The decoder: the answer, plus what it takes to work the answer out.
 *
 * The radio task owns one of these outright. There is no allocation here.
 */
typedef struct {
  RdsInfo info; /* The answer. The only part worth copying out. */

  uint8_t syncMissed; /* Reads in a row with no lock. */

  uint16_t piCandidate; /* Last identifier heard, waiting to be confirmed. */
  bool piCandidateSeen;

  uint8_t ptyCandidate;
  bool ptyCandidateSeen;

  /*
   * The station name being assembled, and the last complete one before it.
   *
   * A name is published only when two complete passes of all eight positions
   * arrive saying the same thing. Confirming each position on its own is not
   * enough, and the reason is measured: 95.0 and 91.1 here send two names in
   * turn, three passes each, scrolling a longer name through the eight
   * characters. Position by position, different positions settle in
   * different passes, and 95.0 then published "5   HI 9", which is half of
   * one name and half of the other and was never transmitted.
   *
   * Whole passes cannot mix. A station that alternates two names shows each
   * of them in turn, which is what it is sending.
   */
  char psFrame[RDS_PS_LEN];     /* The pass being assembled. */
  bool psFrameHave[RDS_PS_LEN]; /* Which of its positions have arrived. */
  char psPrevious[RDS_PS_LEN];  /* The last pass that completed. */
  bool psPreviousSeen;

  char rtBuffer[RDS_RT_LEN];
  bool rtHave[RDS_RT_LEN];
  bool rtFlag;     /* The A/B flag the current text came in under. */
  bool rtFlagSeen; /* False until the first text segment of a station. */

  /*
   * How the end of a text is found on a station that never marks one.
   *
   * A text shorter than 64 characters is supposed to end with a terminator.
   * 94.3 here does not: it sends the first four segments, "FEVER 94.3 FM",
   * round and round, and no terminator and no later segment ever arrive. A
   * decoder that waits for the terminator shows that station no text at all,
   * for ever.
   *
   * So the end is taken from the highest segment the station has been seen to
   * send. 94.3 never sends past segment 3, so its text is sixteen characters
   * and it goes out once all sixteen have arrived. A station with more to say
   * sends a higher segment, and the radio then waits for the whole of it.
   *
   * What this must not do is measure the damage instead of the station. An
   * earlier version took the length from the run of characters received in a
   * pass and believed it once two passes agreed. On a steady bad signal the
   * same blocks fail every pass, so two passes agree on a wrong answer: with
   * block D of segment 0 failing while block C survived, 93.5 published "RE",
   * which is the first two characters of a forty four character text. The
   * highest segment cannot be shortened by damage, only delayed by it.
   */
  uint8_t rtMaxSegment;
  bool rtMaxSegmentSeen;
  /* The B version carries two characters a segment rather than four, so the
   * same segment number means a different length. */
  bool rtMaxIsShort;
  uint8_t rtLastSegment;
  bool rtLastSegmentSeen;
  /*
   * The station has been seen to start its segments again.
   *
   * Until it has, the highest segment so far is only how far this pass has
   * got, not how long the text is, and taking a length from it would put a
   * text on show that fills in as a person watches.
   */
  bool rtWrapped;

  /* What the radio is tuned to, so the station's own frequency is kept out
   * of its list of alternatives. */
  uint32_t tunedKHz;
} Rds;

/*
 * Start again, as though nothing had ever been heard.
 *
 * Called on every retune. Nothing about the station just left is true of the
 * one being tuned, and a name left over from the last station is the worst
 * kind of wrong answer: it is a real name, on the wrong station, and there is
 * no way to tell by looking.
 *
 * `tunedKHz` is where the radio now is. A station lists its own frequency
 * among its alternatives, and that one entry is dropped, so this has to be
 * told rather than worked out.
 */
void rdsReset(Rds *rds, uint32_t tunedKHz);

/*
 * Take one read from the tuner.
 *
 * Call it for every read, including the ones with no group in them, because
 * that is how the lock is followed. Safe with a NULL read, which is counted
 * as a read with no lock and no group.
 */
void rdsFeed(Rds *rds, const RdsRead *read);

/*
 * The name of a programme type, in English. Never NULL.
 *
 * The European table, which India follows. North America uses a different set
 * of names for the same 32 numbers. That table earns its place when a North
 * American band plan does, because a radio on the European FM plan cannot be
 * receiving an RBDS station.
 */
const char *rdsPtyName(uint8_t pty);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RDS_H */
