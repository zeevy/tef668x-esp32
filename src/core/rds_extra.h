/*
 * The parts of the RDS decoder kept out of rds.c for size: the last minute
 * of counts, RT+, Enhanced Other Networks and the programme language. Only
 * rds.c calls these; everything a caller reads is in RdsInfo, rds.h.
 */
#ifndef CORE_RDS_EXTRA_H
#define CORE_RDS_EXTRA_H

#include "rds.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Where each thing sits in block B, for both files. */
#define GROUP_TYPE(b) ((uint8_t)((b) >> 12))
#define GROUP_IS_B_VERSION(b) (((b) & 0x0800u) != 0)

/* Alternative frequency codes, group 0A and group 14 alike. */
#define AF_FM_FIRST 1   /* 87.6 MHz. */
#define AF_FM_LAST 204  /* 107.9 MHz. */
#define AF_LOW_BAND 250 /* The other byte of the pair is long or medium wave. */

/* rds.c's own character rule, so a name from group 14 is shown the same
 * way as one from group 0. */
char rdsDisplayable(uint8_t code);

/* Forget everything these parts know about the station. */
void rdsExtraClear(Rds *rds);

/* Move the last minute on to `nowMs`, dropping slots that are older. */
void rdsWindowTick(Rds *rds, uint32_t nowMs);

/* Count one group's four block levels, and its type when block B came
 * through clean. */
void rdsWindowGroup(Rds *rds, const RdsRead *read, bool typeKnown, uint8_t type,
                    bool isB);

/* Group 1A variant 3: the language code, the low byte of block C. */
void rdsFeedLanguage(Rds *rds, uint16_t blockC);

/* Group 3A, which says which group carries an Open Data Application. */
void rdsFeedOdaAnnounce(Rds *rds, uint16_t blockB, uint16_t blockD);

/* Whether a group with this block B is the one that carries RT+. */
bool rdsIsRtPlusGroup(const Rds *rds, uint16_t blockB);

/* An RT+ tag group whose blocks B, C and D all came through clean. */
void rdsFeedRtPlusTags(Rds *rds, uint16_t blockB, uint16_t blockC,
                       uint16_t blockD);

/* The radio text changed, so the tags that pointed into it mean nothing. */
void rdsRtPlusTextChanged(Rds *rds);

/* The tags or the radio text just changed: keep the StationName.Short the
 * two now give whole, if they give one, in `info.stationShort`. */
void rdsRtPlusKeepStationShort(Rds *rds);

/* Group 14. `cOk` says whether block C came through clean; block D, the
 * other station's PI, must have. */
void rdsFeedEon(Rds *rds, uint16_t blockB, uint16_t blockC, bool cOk,
                uint16_t blockD);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RDS_EXTRA_H */
