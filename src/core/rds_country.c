/* The ECC and country identifier table, and North America's call letters
 * and programme type names. */
#include "rds_country.h"

#include <stddef.h>
#include <string.h>

#include "core/strings.h"

/*
 * One row per ECC, one column per country identifier 1 to F.
 *
 * The codes follow the tables in the RDS standard, IEC 62106 and EN
 * 50067:1998 Annex D, and RDS Forum R08/008_7. They are cross-checked
 * against the redsea decoder by Oona Raisanen, MIT licence, and cell by
 * cell against two other lists: the EBU's SPB 485 of 1996 and the RadioDNS
 * country table. Where two of the three agree and the third does not, the
 * two are taken: Nigeria at D1 F, and Tajikistan, Uzbekistan, Turkmenistan
 * and Puerto Rico, which one list leaves empty.
 * Kosovo and DR Congo carry today's codes, XK and CD. An empty cell is one
 * no list agrees on, and it names no country rather than a guessed one.
 */
static const struct {
  uint8_t ecc;
  char code[15][3];
} kCountries[] = {
    {0xA0,
     {"US", "US", "US", "US", "US", "US", "US", "US", "US", "US", "US", "",
      "US", "US", ""}},
    {0xA1,
     {"", "", "", "", "", "", "", "", "", "", "CA", "CA", "CA", "CA", "GL"}},
    {0xA2,
     {"AI", "AG", "EC", "FK", "BB", "BZ", "KY", "CR", "CU", "AR", "BR", "BM",
      "AN", "GP", "BS"}},
    {0xA3,
     {"BO", "CO", "JM", "MQ", "GF", "PY", "NI", "PR", "PA", "DM", "DO", "CL",
      "GD", "TC", "GY"}},
    {0xA4,
     {"GT", "HN", "AW", "", "MS", "TT", "PE", "SR", "UY", "KN", "LC", "SV",
      "HT", "VE", ""}},
    {0xA5,
     {"", "", "", "", "", "", "", "", "", "", "MX", "VC", "MX", "MX", "MX"}},
    {0xA6, {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "PM"}},
    {0xD0,
     {"CM", "CF", "DJ", "MG", "ML", "AO", "GQ", "GA", "GN", "ZA", "BF", "CG",
      "TG", "BJ", "MW"}},
    {0xD1,
     {"NA", "LR", "GH", "MR", "ST", "CV", "SN", "GM", "BI", "", "BW", "KM",
      "TZ", "ET", "NG"}},
    {0xD2,
     {"SL", "ZW", "MZ", "UG", "SZ", "KE", "SO", "NE", "TD", "GW", "CD", "CI",
      "TZ", "ZM", ""}},
    {0xD3,
     {"", "", "EH", "", "RW", "LS", "", "SC", "", "MU", "", "SD", "", "", ""}},
    {0xE0,
     {"DE", "DZ", "AD", "IL", "IT", "BE", "RU", "PS", "AL", "AT", "HU", "MT",
      "DE", "", "EG"}},
    {0xE1,
     {"GR", "CY", "SM", "CH", "JO", "FI", "LU", "BG", "DK", "GI", "IQ", "GB",
      "LY", "RO", "FR"}},
    {0xE2,
     {"MA", "CZ", "PL", "VA", "SK", "SY", "TN", "", "LI", "IS", "MC", "LT",
      "RS", "ES", "NO"}},
    {0xE3,
     {"ME", "IE", "TR", "MK", "TJ", "", "", "NL", "LV", "LB", "AZ", "HR", "KZ",
      "SE", "BY"}},
    {0xE4,
     {"MD", "EE", "KG", "", "", "UA", "XK", "PT", "SI", "AM", "UZ", "GE", "",
      "TM", "BA"}},
    {0xF0,
     {"AU", "AU", "AU", "AU", "AU", "AU", "AU", "AU", "SA", "AF", "MM", "CN",
      "KP", "BH", "MY"}},
    {0xF1,
     {"KI", "BT", "BD", "PK", "FJ", "OM", "NR", "IR", "NZ", "SB", "BN", "LK",
      "TW", "KR", "HK"}},
    {0xF2,
     {"KW", "QA", "KH", "WS", "IN", "MO", "VN", "PH", "JP", "SG", "MV", "ID",
      "AE", "NP", "VU"}},
    {0xF3,
     {"LA", "TH", "TO", "", "", "", "", "", "PG", "", "YE", "", "", "FM",
      "MN"}},
};

const char *rdsCountryCode(uint16_t pi, uint8_t ecc) {
  unsigned ci = (pi >> 12) & 0x0Fu;
  if (ci == 0) {
    return NULL;
  }
  for (size_t i = 0; i < sizeof(kCountries) / sizeof(kCountries[0]); i++) {
    if (kCountries[i].ecc == ecc) {
      const char *code = kCountries[i].code[ci - 1];
      return code[0] != '\0' ? code : NULL;
    }
  }
  return NULL;
}

/*
 * The three letter calls, NRSC-4-B table D.7, in the order of their PI.
 * Taken from the standard's own table.
 */
static const struct {
  uint16_t pi;
  char call[4];
} kThreeLetter[] = {
    {0x9950, "KEX"}, {0x9951, "KFH"}, {0x9952, "KFI"}, {0x9953, "KGA"},
    {0x9954, "KGO"}, {0x9955, "KGU"}, {0x9956, "KGW"}, {0x9957, "KGY"},
    {0x9958, "KID"}, {0x9959, "KIT"}, {0x995A, "KJR"}, {0x995B, "KLO"},
    {0x995C, "KLZ"}, {0x995D, "KMA"}, {0x995E, "KMJ"}, {0x995F, "KNX"},
    {0x9960, "KOA"}, {0x9964, "KQV"}, {0x9965, "KSL"}, {0x9966, "KUJ"},
    {0x9967, "KVI"}, {0x9968, "KWG"}, {0x996B, "KYW"}, {0x996D, "WBZ"},
    {0x996E, "WDZ"}, {0x996F, "WEW"}, {0x9971, "WGL"}, {0x9972, "WGN"},
    {0x9973, "WGR"}, {0x9975, "WHA"}, {0x9976, "WHB"}, {0x9977, "WHK"},
    {0x9978, "WHO"}, {0x997A, "WIP"}, {0x997B, "WJR"}, {0x997C, "WKY"},
    {0x997D, "WLS"}, {0x997E, "WLW"}, {0x9981, "WOC"}, {0x9983, "WOL"},
    {0x9984, "WOR"}, {0x9988, "WWJ"}, {0x9989, "WWL"}, {0x9990, "KDB"},
    {0x9991, "KGB"}, {0x9992, "KOY"}, {0x9993, "KPQ"}, {0x9994, "KSD"},
    {0x9995, "KUT"}, {0x9996, "KXL"}, {0x9997, "KXO"}, {0x9999, "WBT"},
    {0x999A, "WGH"}, {0x999B, "WGY"}, {0x999C, "WHP"}, {0x999D, "WIL"},
    {0x999E, "WMC"}, {0x999F, "WMT"}, {0x99A0, "WOI"}, {0x99A1, "WOW"},
    {0x99A2, "WRR"}, {0x99A3, "WSB"}, {0x99A4, "WSM"}, {0x99A5, "KBW"},
    {0x99A6, "KCY"}, {0x99A7, "KDF"}, {0x99AA, "KHQ"}, {0x99AB, "KOB"},
    {0x99B3, "WIS"}, {0x99B4, "WJW"}, {0x99B5, "WJZ"}, {0x99B9, "WRC"},
};

/* The K and W blocks: 4096 and 21672 plus 676, 26 and 1 for each letter. */
#define CALL_K_FIRST 0x1000
#define CALL_W_FIRST 0x54A8
#define CALL_W_LAST 0x994F
#define CALL_LETTERS 26

/*
 * The PI call letters worth `pi` are sent as, annex D.7.1's exceptions 1 and
 * 2 in that order: P1 0 P3 P4 as A P1 P3 P4, then P1 P2 0 0 as A F P1 P2.
 * 1000 to 9000 take both, so 1000 goes to A100 and then AFA1.
 */
static uint16_t sentAs(uint16_t pi) {
  if ((pi & 0x0F00) == 0) {
    pi = (uint16_t)(0xA000 | ((pi & 0xF000) >> 4) | (pi & 0x00FF));
  }
  if ((pi & 0x00FF) == 0) {
    pi = (uint16_t)(0xAF00 | (pi >> 8));
  }
  return pi;
}

bool rdsCallSign(uint16_t pi, char *out, size_t cap) {
  if (out == NULL || cap < 5) {
    return false;
  }
  out[0] = '\0';
  /* Annex D.7.1's exceptions 1 and 2 moved a PI that would read as a local
   * station in Europe: P1 P2 0 0 to A F P1 P2, and P1 0 P3 P4 to A P1 P3
   * P4. Undone in that order, since 1000 to 9000 took both. */
  const uint16_t sent = pi;
  if ((pi & 0xFF00) == 0xAF00) {
    pi = (uint16_t)((pi & 0x00FF) << 8);
  }
  if ((pi & 0xF000) == 0xA000) {
    pi = (uint16_t)(((pi & 0x0F00) << 4) | (pi & 0x00FF));
  }
  /* Only the one form the standard sends is a station's: 1064 comes from no
   * station, since KADW sends A164, and A100 from none, since KAAA sends
   * AFA1. */
  if (pi >= CALL_K_FIRST && pi <= CALL_W_LAST && sentAs(pi) == sent) {
    const bool k = pi < CALL_W_FIRST;
    unsigned v = (unsigned)pi - (k ? CALL_K_FIRST : CALL_W_FIRST);
    out[0] = k ? 'K' : 'W';
    out[3] = (char)('A' + v % CALL_LETTERS);
    v /= CALL_LETTERS;
    out[2] = (char)('A' + v % CALL_LETTERS);
    out[1] = (char)('A' + v / CALL_LETTERS);
    out[4] = '\0';
    return true;
  }
  for (size_t i = 0; i < sizeof(kThreeLetter) / sizeof(kThreeLetter[0]); i++) {
    if (kThreeLetter[i].pi == pi) {
      for (size_t c = 0; c < 4; c++) {
        out[c] = kThreeLetter[i].call[c];
      }
      return true;
    }
  }
  return false;
}

RdsCall rdsStationCall(const RdsInfo *info, RdsRegion region, char *out,
                       size_t cap) {
  if (out == NULL || cap == 0) {
    return RDS_CALL_NONE;
  }
  out[0] = '\0';
  if (info == NULL || region != RDS_REGION_NORTH_AMERICA) {
    return RDS_CALL_NONE;
  }
  /* The last StationName.Short the decoder heard whole from this station,
   * kept across radio texts, since a station may send it with only some of
   * them. Shown only if it fits whole. */
  const size_t len =
      info->hasStationShort ? strnlen(info->stationShort, cap) : cap;
  if (len < cap) {
    memcpy(out, info->stationShort, len + 1);
    return RDS_CALL_SENT;
  }
  if (!info->hasPi || ((info->pi >> 12) == 1 && info->tmc)) {
    return RDS_CALL_NONE;
  }
  return rdsCallSign(info->pi, out, cap) ? RDS_CALL_GUESS : RDS_CALL_NONE;
}

bool rdsPiHasArea(uint16_t pi, RdsRegion region) {
  if (region != RDS_REGION_NORTH_AMERICA) {
    return true;
  }
  const uint16_t block = (uint16_t)(pi >> 12);
  return block == 0xB || block == 0xD || block == 0xE;
}

/* NRSC-4-B table F.2, the "Program type" column. 27 and 28 are unassigned. */
static const StrId kRbdsPtyNames[RDS_PTY_COUNT] = {
    STR_PTY_NONE,
    STR_PTY_NA_NEWS,
    STR_PTY_NA_INFORMATION,
    STR_PTY_NA_SPORTS,
    STR_PTY_NA_TALK,
    STR_PTY_NA_ROCK,
    STR_PTY_NA_CLASSIC_ROCK,
    STR_PTY_NA_ADULT_HITS,
    STR_PTY_NA_SOFT_ROCK,
    STR_PTY_NA_TOP_40,
    STR_PTY_NA_COUNTRY,
    STR_PTY_NA_OLDIES,
    STR_PTY_NA_SOFT,
    STR_PTY_NA_NOSTALGIA,
    STR_PTY_NA_JAZZ,
    STR_PTY_NA_CLASSICAL,
    STR_PTY_NA_RHYTHM_AND_BLUES,
    STR_PTY_NA_SOFT_RHYTHM_AND_BLUES,
    STR_PTY_NA_FOREIGN_LANGUAGE,
    STR_PTY_NA_RELIGIOUS_MUSIC,
    STR_PTY_NA_RELIGIOUS_TALK,
    STR_PTY_NA_PERSONALITY,
    STR_PTY_NA_PUBLIC,
    STR_PTY_NA_COLLEGE,
    STR_PTY_NA_SPANISH_TALK,
    STR_PTY_NA_SPANISH_MUSIC,
    STR_PTY_NA_HIP_HOP,
    STR_PTY_NA_UNASSIGNED,
    STR_PTY_NA_UNASSIGNED,
    STR_PTY_NA_WEATHER,
    STR_PTY_NA_EMERGENCY_TEST,
    STR_PTY_NA_EMERGENCY};

const char *rdsPtyNameIn(uint8_t pty, RdsRegion region) {
  if (region != RDS_REGION_NORTH_AMERICA) {
    return rdsPtyName(pty);
  }
  return pty < RDS_PTY_COUNT ? txt(kRbdsPtyNames[pty]) : txt(STR_PTY_NONE);
}
