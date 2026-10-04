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
 * parts be tested on a PC against real groups captured off air.
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
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Station name, in characters. Fixed by the standard. */
#define RDS_PS_LEN 8

/*
 * How many passes may be stitched into one name, and how long that makes it.
 *
 * Four is well past what any station measured uses. One station splits its
 * name across two, and no other station measured splits at all. The cap
 * exists because a station tickering a sentence through this field would
 * otherwise build a name as long as the sentence, and that is not a name.
 */
#define RDS_PS_JOIN_MAX 4
#define RDS_PS_LONG_LEN (RDS_PS_JOIN_MAX * RDS_PS_LEN)

/* How many candidate pairs are remembered while waiting for a repeat. */
#define RDS_PS_PAIRS 8

/* Radio text, in characters. 64 from a 2A group, 32 from a 2B. */
#define RDS_RT_LEN 64
/* Room for an RT+ StationName.Short of up to sixteen characters, the width
 * RBDS gives a long display, and its terminator. */
#define RDS_STATION_SHORT_LEN 17

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
  /* When the read was taken, in milliseconds on any clock that only goes
   * forward. The last minute counts in RdsInfo are timed by it. */
  uint32_t atMs;
} RdsRead;

/* A block's error level when it came through clean, in `error` above and
 * in RdsInfo's `blockError`. */
#define RDS_BLOCK_CLEAN 0

/*
 * One RT+ tag: which part of the radio text is what. RT+ is the Open Data
 * Application with AID 4BD7, IEC 62106-6 annex B. `type` is the content
 * type, 1 ITEM.TITLE, 4 ITEM.ARTIST and so on, rdsRtPlusLabel names it.
 * `start` and `length` are in characters of the radio text.
 */
typedef struct {
  uint8_t type;
  uint8_t start;
  uint8_t length;
} RdsRtPlusTag;

/* How many tags are kept. A tag group carries two, and a station that
 * sends a title, an artist and an album takes turns with them. */
#define RDS_RTPLUS_MAX 4

/*
 * Another station the same broadcaster runs, from group 14, Enhanced Other
 * Networks. RDS: The Radio Data System (Kopitz and Marks), Section 3.2.1.8.
 * Listed once it has been heard in two groups, so one wrongly corrected
 * block D does not add a station that does not exist.
 */
#define RDS_EON_MAX 4
#define RDS_EON_AF_MAX 4
typedef struct {
  uint16_t pi;
  uint8_t heard; /* Groups it was named in, up to 255. Listed from 2. */
  bool hasPs;    /* All eight characters of its name have arrived. */
  char ps[RDS_PS_LEN + 1];
  bool hasTp;
  bool tp;
  bool hasTa;
  bool ta; /* On air on it now, as the last 14B or variant 13 said. */
  uint8_t afCount;
  bool afMore; /* It named a frequency after `afCode` was full. */
  /* Frequency codes as group 0A sends them, 1 to 204, a byte each since
   * this struct is copied into every snapshot. rdsAfCodeKHz turns one into
   * kHz. */
  uint8_t afCode[RDS_EON_AF_MAX];
} RdsEon;

/*
 * The last minute of groups, for the decoder page: how many, the level of
 * each block, and the group types. Counted the same way as the since the
 * tune counters below, in RdsInfo, over no more than the last 60 s.
 * `spanMs` is how much time the counts cover, less than 60 s just after a
 * tune, so a rate taken from them is groups over this, not over 60 s.
 */
#define RDS_MINUTE_MS 60000UL
#define RDS_LEVEL_CLEAN 0
#define RDS_LEVEL_CORRECTED 1
#define RDS_LEVEL_LOST 2
typedef struct {
  uint16_t groups;
  uint16_t blocks[4][3]; /* Block A to D, by RDS_LEVEL_. */
  uint16_t types[16][2]; /* As groupTypeCount. */
  uint32_t spanMs;
} RdsMinute;

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

  /*
   * The station sends 0000, heard in a clean block A twice in a row.
   *
   * The standard keeps 0000 for a station that has no identifier, and some
   * stations measured send it. It is not an identifier, so `hasPi` stays false
   * and nothing compares it. But "sends 0000" and "nothing heard yet" are
   * different answers for a person tuning round, and the DX page and the
   * RDS page show them apart. Never true beside `hasPi`: a real identifier,
   * once confirmed, is the answer, and a 0000 does not disturb it.
   */
  bool piZero;

  /*
   * The last identifier heard in a clean block A, confirmed or not.
   *
   * What the DX page shows while it is not yet sure. Only a clean block A
   * counts, the same as for `pi`: in recorded DX groups a corrected one carries
   * the wrong identifier 138 times in 183, and a clean one never does in 522.
   *
   * `piUnsureNibbles` has bit n set when hex digit n, counted from the
   * right, differed from the hearing before this one. Those are the digits
   * the page shows as `?`. Zero after a single hearing and when the last
   * two agreed.
   */
  bool hasPiHeard;
  uint16_t piHeard;
  uint8_t piUnsureNibbles;

  /*
   * The error level of each block of the last group, A to D, as the tuner
   * reported it: 0 clean, 1 or 2 corrected, 3 not corrected. For the DX
   * page's block meters.
   *
   * False until a group arrives, and false again once the lock is lost, so
   * a station that has gone never shows its last group as though it were
   * still arriving.
   */
  bool hasBlockErrors;
  uint8_t blockError[4];

  /*
   * The extended country code, group 1A variant 0, which with the first hex
   * digit of `pi` names the country: `rdsCountryCode` in rds_country.h.
   * Taken from a clean block C only and published on its second matching
   * reception, like everything else here, and cleared with the station, so
   * a code from a damaged block or from the last station is never shown.
   * No station measured sends group 1A, so this is proved against groups
   * written by hand.
   */
  bool hasEcc;
  uint8_t ecc;

  bool hasPty; /* A programme type has been heard twice. */
  uint8_t pty; /* 0 to 31. */

  /*
   * The station's own eight character description of its programme type,
   * group 10A, twice a group like the station name. A station's own words
   * rather than the fixed table `rdsPtyName` reads: three services all
   * coded SPORT can still tell a listener apart by carrying "Cricket",
   * "Golf" and "Tennis" here.
   *
   * No station measured sends group 10A: they send only 0A, 0B and 2A, so
   * this is proved against a group written by hand rather than against a
   * broadcast, the same risk the alternative frequency list and the clock
   * carry.
   */
  bool hasPtyn;
  char ptyn[RDS_PS_LEN + 1];

  /*
   * The decoder identification bits, RDS: The Radio Data System (Kopitz
   * and Marks), Table 4.3. Four independent flags, sent one at a time in
   * group 0, addressed the same two bits that address the station name's
   * own pair in the same group, each published once its own address has
   * agreed twice.
   *
   * Not the same claim as the tuner's own stereo pilot flag, which is a
   * fact about the radio wave. This is the broadcaster stating what it
   * believes it is sending, and the two can disagree without either
   * being wrong: a mono broadcast on a stereo capable transmitter still
   * carries a pilot tone the tuner reads as stereo.
   */
  bool hasDiStereo;
  bool diStereo; /* True: the broadcaster states this is stereo. */
  bool hasDiArtificialHead;
  bool diArtificialHead;
  bool hasDiCompressed;
  bool diCompressed;
  bool hasDiDynamicPty;
  bool diDynamicPty; /* True: the programme type may change without notice. */

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
  char ps[RDS_PS_LEN + 1]; /* The station name, exactly as it was sent. */

  /*
   * The station name as it arrives, one pair of characters at a time.
   *
   * `ps` waits for two whole passes that agree, which is about a second and
   * a half at the slowest rate a station sends its name, and a meteor ping
   * or a fade can be over before that. This keeps each position as it was
   * last heard, from clean blocks only, so the DX page can show a name
   * building up. `psHeardHave` says which positions have arrived, because a
   * space is a real character.
   *
   * Two names sent in turn can sit side by side here, half of each. That is
   * what was heard, but it is not a name the station sent, so nothing reads
   * this except the DX page, and that only until `ps` arrives.
   */
  char psHeard[RDS_PS_LEN];
  bool psHeardHave[RDS_PS_LEN];

  /*
   * The same name with the pass that follows it stitched on, when the station
   * is plainly splitting one name across two passes.
   *
   * One station measured sends `SAMPLE 9` and then `5       `, which is
   * `SAMPLE 95` cut at the eighth character. Showing those in turn is
   * truthful and unreadable.
   *
   * `ps` is left alone and this is offered beside it, so what the station
   * actually transmitted is still visible in the state document. A caller
   * showing a name to a person wants this one and should fall back to `ps`.
   * False means there is nothing to stitch, which is the normal case: five of
   * the six RDS stations measured send one name and never split it.
   */
  bool hasPsLong;
  char psLong[RDS_PS_LONG_LEN + 1];
  /*
   * The two always agree. `hasPsLong` false means `psLong` is empty, never a
   * name left over from the last time the run came round.
   */

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

  /*
   * How many of each group type and version this station has actually
   * sent, block B read whether or not this decoder does anything with the
   * type. `[0]` is the A version and `[1]` is B, matching how block B's
   * own version bit reads. Counted only from a group whose block B
   * survived, the same block every group type is addressed by.
   *
   * What a field reading as a dash actually means is not always the same
   * thing, and this is how the difference is told apart: a programme type
   * name that has never arrived on a station sending no group 10A at all
   * is the broadcaster's own choice, and the same dash on a station
   * sending 10A nine times in a hundred is a reception problem.
   */
  uint32_t groupTypeCount[16][2];

  /*
   * Programme Item Number, group type 1, block 4: the scheduled start of
   * the item now playing, day of month, hour and minute of the station's
   * own clock. RDS: The Radio Data System (Kopitz and Marks), Section 4.6.
   * Three fields rather than one packed time, because each is checked
   * against its own range and a PIN can be wrong in only one of them.
   *
   * No station measured sends group type 1: they send only 0A, 0B and 2A, so
   * this is proved against a group written by hand, the same risk the
   * alternative frequency list and the clock carry.
   */
  bool hasPin;
  uint8_t pinDay;    /* 1 to 31. */
  uint8_t pinHour;   /* 0 to 23. */
  uint8_t pinMinute; /* 0 to 59. */

  /*
   * The language of the programme, group 1A variant 3, EN 50067 annex J.
   * Heard twice, as the ECC is. 0 is the standard's "unknown", so it is
   * kept as not given. rdsLanguageName names it.
   */
  bool hasLanguage;
  uint8_t language;

  /*
   * RT+. `rtPlus` is that the station has said, twice, which group carries
   * it. The tags are the ones heard twice since the item began; a change of
   * the item toggle, or of the radio text, empties them. `rtPlusRunning` is
   * the station saying an item is playing now. rdsRtPlusText cuts a tag's
   * words out of `rt`.
   */
  bool rtPlus;
  bool rtPlusRunning;
  /* The station has said, twice, that it sends traffic data, TMC, the ODA
   * with AID CD46. North America's rule for call letters needs it: a TMC
   * station may put 1 in place of its PI's first digit, NRSC-4-B D.7.4. */
  bool tmc;
  /*
   * The last RT+ StationName.Short heard whole, content type 31, kept until
   * the station changes. The tags are emptied at every new radio text, and a
   * station may send its name with only some of them; this keeps the name
   * from coming and going between texts. Empty with
   * `hasStationShort` false, and a name longer than sixteen characters is
   * never kept cut short.
   */
  bool hasStationShort;
  char stationShort[RDS_STATION_SHORT_LEN];
  uint8_t rtPlusCount;
  RdsRtPlusTag rtPlusTag[RDS_RTPLUS_MAX];

  /* Enhanced Other Networks, in the order first heard. Only those with
   * `heard` of 2 or more are real, see RdsEon. The first four heard stay;
   * `eonMore` says another was named after the list was full. */
  uint8_t eonCount;
  RdsEon eon[RDS_EON_MAX];
  bool eonMore;

  RdsMinute minute;
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
  bool zeroCandidateSeen; /* The last clean block A heard was 0000. */

  uint8_t ptyCandidate;
  bool ptyCandidateSeen;

  uint8_t eccCandidate;
  bool eccCandidateSeen;

  /* One candidate per DI address, since all four are independent and a
   * group only ever carries one of them. */
  bool diCandidate[4];
  bool diCandidateSeen[4];

  /*
   * The programme type name being assembled, and the pass before it.
   *
   * Two four character segments rather than the station name's four two
   * character ones, since group 10A carries a whole segment a group
   * rather than a pair of characters, but the same whole-pass rule
   * applies: positions confirmed one at a time, rather than as a pass that
   * matches the one before it, can publish half of one name and half of
   * another, as the station name below explains.
   */
  char ptynFrame[RDS_PS_LEN];
  bool ptynFrameHave[RDS_PS_LEN];
  char ptynPrevious[RDS_PS_LEN];
  bool ptynPreviousSeen;

  /* The PIN candidate, confirmed as one unit: a day, an hour and a minute
   * that agree with the previous group are one candidate agreeing, not
   * three fields that happen to agree separately. */
  bool pinCandidateSeen;
  uint8_t pinCandidateDay;
  uint8_t pinCandidateHour;
  uint8_t pinCandidateMinute;

  /*
   * The station name being assembled, and the last complete one before it.
   *
   * A name is published only when two complete passes of all eight positions
   * arrive saying the same thing. Confirming each position on its own is not
   * enough, and the reason is measured: two stations measured send two names
   * in turn, three passes each, scrolling a longer name through the eight
   * characters. Position by position, different positions settle in
   * different passes, and the result can be "5   HI 9", which is half of one
   * name and half of the other and was never transmitted.
   *
   * Whole passes cannot mix. A station that alternates two names shows each
   * of them in turn, which is what it is sending.
   */
  char psFrame[RDS_PS_LEN];     /* The pass being assembled. */
  bool psFrameHave[RDS_PS_LEN]; /* Which of its positions have arrived. */
  char psPrevious[RDS_PS_LEN];  /* The last pass that completed. */
  bool psPreviousSeen;

  /*
   * How a split name is learned.
   *
   * A station holds each name for several passes and then changes, so what
   * matters is the change: the pair of names either side of it. A pair counts
   * as a split when the first ends on a character rather than a space and the
   * second begins on one, because that is what a string cut at eight
   * characters looks like. A name that is centred or padded is a whole name.
   *
   * Measured against recorded groups from six stations that send RDS. On a
   * strong signal the only pair that qualifies is from the one station that
   * splits its name, and it qualifies five times in ten name changes.
   * Another station rotates eleven names and not one pair qualifies, because
   * every one of them is padded.
   *
   * Weak signals are why a pair has to be seen twice. That second station
   * with the aerial collapsed throws up two pairs that qualify, both made by
   * damaged blocks. Each occurs once in forty three changes against the real
   * split's five in ten, so the gap between a real split and a damaged one
   * is wide.
   */
  char psLast[RDS_PS_LEN + 1]; /* The last name published, to see a change. */
  bool psLastSeen;
  /*
   * Qualifying pairs seen once, waiting for a second sighting.
   *
   * A table rather than one slot because a name split across three passes
   * produces two different pairs alternately, and a single slot is overwritten
   * by the second before the first can come round again, so neither ever
   * reaches two.
   *
   * Eight because corruption makes pairs too. A damaged pass often loses its
   * padding, which is exactly what qualifies, so on a weak signal a station
   * that never splits anything still fills this table. One station measured
   * on a weak signal produces two such pairs and a worse signal would
   * produce more.
   * With the table full the oldest is dropped, and a real split whose two
   * sightings are further apart than that is evicted in between and never
   * confirms.
   *
   * That failure is in the safe direction: the panel shows the eight
   * character pass rather than something wrong. Eight is chosen to keep it
   * rare rather than to make it impossible, because making it impossible
   * needs a table the size of the station's whole rotation.
   */
  char psPairA[RDS_PS_PAIRS][RDS_PS_LEN + 1];
  char psPairB[RDS_PS_PAIRS][RDS_PS_LEN + 1];
  uint8_t psPairCount;
  uint8_t psPairNext;                              /* Where the oldest sits. */
  char psJoinSeg[RDS_PS_JOIN_MAX][RDS_PS_LEN + 1]; /* The confirmed run. */
  uint8_t psJoinCount;

  char rtBuffer[RDS_RT_LEN];
  bool rtHave[RDS_RT_LEN];
  bool rtFlag;     /* The A/B flag the current text came in under. */
  bool rtFlagSeen; /* False until the first text segment of a station. */

  /*
   * How the end of a text is found on a station that never marks one.
   *
   * A text shorter than 64 characters is supposed to end with a terminator.
   * One station measured does not: it sends the first four segments, its
   * name and frequency, round and round, and no terminator and no later
   * segment ever arrive. A decoder that waits for the terminator shows that
   * station no text at all, for ever.
   *
   * So the end is taken from the highest segment the station has been seen
   * to send. That station never sends past segment 3, so its text is sixteen
   * characters and it goes out once all sixteen have arrived. A station with
   * more to say sends a higher segment, and the radio then waits for the
   * whole of it.
   *
   * What this must not do is measure the damage instead of the station.
   * Taking the length from the run of characters received in a pass fails on
   * a steady bad signal, because the same blocks fail every pass and two
   * passes then agree on a wrong answer: with block D of segment 0 failing
   * while block C survived, one station measured would publish "RE", the
   * first two characters of a forty four character text. The highest
   * segment cannot be shortened by damage, only delayed by it.
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

  /* Each other network's name as it arrives, and which positions have, a
   * bit each; RdsInfo's `eon` has only the name once it is whole. */
  char eonPsBuild[RDS_EON_MAX][RDS_PS_LEN];
  uint8_t eonPsHave[RDS_EON_MAX];
  /* The last whole pass of each, which the next must match; a bit each in
   * `eonPsPrevSeen`. */
  char eonPsPrev[RDS_EON_MAX][RDS_PS_LEN];
  uint8_t eonPsPrevSeen;

  /* The language code waiting for a second hearing, as the ECC's. */
  uint8_t languageCandidate;
  bool languageCandidateSeen;

  /*
   * RT+. The group 3A that announces it, and a tag group, are each taken on
   * their second identical hearing. `rtPlusGroup` is the group that carries
   * the tags, type times two plus one for the B version, as block B of 3A
   * says it.
   */
  uint8_t rtPlusGroup;
  uint8_t rtPlusAnnounceCandidate;
  bool rtPlusAnnounceSeen;
  bool tmcAnnounceSeen; /* The first of the two hearings `info.tmc` needs. */
  /* The last four tag groups heard: block B's low five bits, then C, D.
   * `rtPlusTagSeen` has a bit for each that holds one. Four, since a
   * station may rotate that many tag groups and each must be heard twice. */
  uint16_t rtPlusTagCandidate[4][3];
  uint8_t rtPlusTagSeen;
  uint8_t rtPlusTagNext;
  bool rtPlusToggle;
  bool rtPlusToggleSeen;

  /*
   * The last minute, in twelve slots of five seconds. `windowSlot` is the
   * one being filled and `windowSlotMs` when it began; RdsInfo's `minute`
   * is kept equal to the sum of the twelve. Bytes are enough: a slot holds
   * five seconds of groups, about 57 at the standard's 11.4 a second.
   */
  uint8_t windowGroups[12];
  uint8_t windowBlocks[12][4][2]; /* Corrected, lost; clean is the rest. */
  uint8_t windowTypes[12][16][2];
  uint8_t windowSlot;
  uint32_t windowSlotMs;
  uint32_t windowSinceMs;
  bool windowStarted;
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
 * The identifier last heard, as four hex digits with `?` for each digit that
 * changed from the hearing before, such as `5?A1`.
 *
 * `out` must hold 5 bytes. Writes an empty string and returns false when no
 * identifier has been heard, or when `info` or `out` is NULL.
 */
bool rdsFormatPiHeard(const RdsInfo *info, char *out);

/*
 * The name of a programme type, in English. Never NULL.
 *
 * The European table. North America uses a different set of names for the
 * same 32 numbers, and `rdsPtyNameIn` in rds_country.h reads either.
 */
const char *rdsPtyName(uint8_t pty);

/*
 * What a PI's own second nibble says about how far its programme reaches,
 * in English. Never NULL. RDS: The Radio Data System (Kopitz and Marks),
 * Figure 3.4: 0 local, 1 international, 2 national, 3 supra-regional, 4
 * to F regional 1 to 12. Read straight off `RdsInfo.pi`, `(pi >> 8) &
 * 0xF`, so this takes the whole PI rather than a nibble already pulled
 * out, the same way a caller is not asked to shift `rdsPtyName`'s own
 * argument out of a group first.
 *
 * The first nibble, the country, is not named here. Only 15 codes cover
 * some 190 countries, and telling them apart needs the Extended Country
 * Code from group 1A. `rdsCountryCode` in rds_country.h names the country
 * from the PI and the ECC together.
 */
const char *rdsPiOriginName(uint16_t pi);

/* An alternative frequency code, 1 to 204, as kHz, 87600 to 107900. 0 for
 * any other code. */
uint32_t rdsAfCodeKHz(uint8_t code);

/*
 * The name of a programme language, EN 50067 annex J, in English, or NULL
 * for a code the standard leaves unassigned, and for 0, which is unknown.
 */
const char *rdsLanguageName(uint8_t code);

/*
 * A short name for an RT+ content type, in capitals, such as TITLE or
 * ARTIST. Never NULL: a type the standard keeps for later is OTHER.
 */
const char *rdsRtPlusLabel(uint8_t type);

/*
 * The words of tag `index` of the radio text, spaces taken off both ends,
 * into `out`. A tag that reaches past the end of the text is cut at the
 * end, since the text lost its padding when it was trimmed. False, and an
 * empty `out`, when there is no such tag, no radio text, the tag starts
 * past the end of the text, or the words are all spaces.
 */
bool rdsRtPlusText(const RdsInfo *info, uint8_t index, char *out, size_t cap);

/*
 * A station name as a screen shows it: the first `len` characters of `name`,
 * or fewer at a terminator, without the spaces at either end, into `out`.
 * Stations pad an eight character name with spaces to centre it, " MAGIC  ",
 * and drawn as sent it sits a space to the right of the text under it.
 * Spaces inside the name stay. False, and an empty `out`, when nothing but
 * spaces is left.
 */
bool rdsNameTrim(const char *name, size_t len, char *out, size_t cap);

/*
 * The last minute as the decoder page shows it. Shares are in whole per
 * cent of the minute's groups, rounded to nearest, and the lost share also
 * in tenths, since a lost block in a thousand is worth seeing. The group
 * types are the ones sent, most first, a tie in type order, up to
 * `maxGroups` and never more than RDS_MINUTE_GROUPS_MAX.
 */
#define RDS_MINUTE_GROUPS_MAX 8
typedef struct {
  bool known;     /* Any group at all in the minute. */
  bool rateKnown; /* The minute covers a second or more. */
  uint16_t rateTenths;
  uint16_t blerTenths;
  uint16_t lostBlocks;
  uint16_t totalBlocks;
  uint8_t clean[4];
  uint8_t fixed[4];
  uint8_t lost[4];
  uint16_t fixedTenths[4];
  uint16_t lostTenths[4];
  uint8_t groupCount;
  uint8_t groupType[RDS_MINUTE_GROUPS_MAX];
  bool groupIsB[RDS_MINUTE_GROUPS_MAX];
  uint8_t groupPercent[RDS_MINUTE_GROUPS_MAX];
} RdsMinuteStats;

void rdsMinuteStats(const RdsMinute *m, uint8_t maxGroups, RdsMinuteStats *out);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RDS_H */
