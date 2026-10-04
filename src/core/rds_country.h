/*
 * Which country an RDS station is in, from its PI and its ECC.
 *
 * The first hex digit of the PI is the country identifier, but there are
 * only fifteen of them for every country in the world, so each is shared:
 * a PI starting with 5 is Italy, Jordan, Slovakia or India, among others.
 * The extended country code, sent now and then in group 1A, says which. So
 * a country is named only once both are known, and never from the PI alone.
 */
#ifndef CORE_RDS_COUNTRY_H
#define CORE_RDS_COUNTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rds.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The ISO 3166 two letter code of the country `pi` and `ecc` name together,
 * such as "IN" for a PI starting with 5 and an ECC of F2.
 *
 * NULL when the pair names no country: a country identifier of 0, which is
 * not one, an ECC outside the table, or a cell the standard leaves empty.
 */
const char *rdsCountryCode(uint16_t pi, uint8_t ecc);

/*
 * Where the radio is listening, for the RDS rules that differ by region.
 * North America's RBDS works a station's call letters into its PI, and
 * nowhere else does, so a PI is read as call letters only on North America.
 */
typedef enum {
  RDS_REGION_EUROPE = 0, /* And the rest of the world: IEC 62106. */
  RDS_REGION_NORTH_AMERICA,
  RDS_REGION_COUNT,
} RdsRegion;

/*
 * The call letters a North American PI is worked out from, NRSC-4-B annex
 * D.7: K or W and three letters, such as "KGTB" for 21C7, or one of the
 * three letter calls of table D.7, such as "KEX" for 9950. `cap` must hold
 * five bytes.
 *
 * False, with `out` empty, for a PI that names no call letters: 0000 to
 * 0FFF, which are reserved; B, D and E, networks linked across stations;
 * C, Canada; F, Mexico; 9950 to 9EFF where the table lists none; and a PI
 * with a second digit of 0 or ending 00 inside 1000 to 994F, since the
 * standard sends every one of those moved into the A block.
 *
 * Only ever a guess. NRSC-G300 says many stations do not send the PI their
 * call letters give, and a station may put 1 in place of the PI's first
 * digit for traffic data, which the letters cannot show.
 */
bool rdsCallSign(uint16_t pi, char *out, size_t cap);

/* Room for what `rdsStationCall` writes: an RT+ name of up to sixteen
 * characters, the width RBDS gives a long display, or four call letters,
 * and the terminator. */
#define RDS_CALL_TEXT_LEN RDS_STATION_SHORT_LEN

/* What `rdsStationCall` found. */
typedef enum {
  RDS_CALL_NONE = 0, /* Nothing to show. */
  RDS_CALL_GUESS, /* Worked out from the PI, which may not be the right one. */
  RDS_CALL_SENT,  /* Sent by the station as its RT+ StationName.Short. */
} RdsCall;

/*
 * A North American station's name, for the slot a country takes elsewhere:
 * its RT+ StationName.Short, content type 31, when it sends one that fits
 * `out` whole, since those are the station's own words, and otherwise the
 * call letters its confirmed PI gives, a guess. The short name is often a
 * slogan, "HOT 97", rather than call letters.
 *
 * RDS_CALL_NONE, with `out` empty, on any region but North America; with
 * neither; and for a PI starting 1 from a station sending TMC, which
 * NRSC-4-B D.7.4 says may have had its first digit swapped, so its letters
 * cannot be worked out.
 */
RdsCall rdsStationCall(const RdsInfo *info, RdsRegion region, char *out,
                       size_t cap);

/*
 * Whether `pi` carries a coverage area to name, National, Local and so on.
 * Always in Europe. In North America only in the B, D and E blocks,
 * NRSC-4-B D.7.3: a PI worked out from call letters carries none.
 */
bool rdsPiHasArea(uint16_t pi, RdsRegion region);

/*
 * The programme type's name: RBDS's own on North America, NRSC-4-B table
 * F.2, such as Rock for 5, and IEC 62106's everywhere else, Education for
 * the same 5. Never NULL.
 */
const char *rdsPtyNameIn(uint8_t pty, RdsRegion region);

#ifdef __cplusplus
}
#endif

#endif /* CORE_RDS_COUNTRY_H */
