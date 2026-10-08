/*
 * The XDR protocol: the lines XDR-GTK and FM-DX Webserver speak to a tuner.
 *
 * One message per line, ended by a line feed alone. The first character says
 * what it is and the rest is its value. A PC sends commands, and the radio
 * answers each with the value really in force, an echo; XDR-GTK puts a control
 * back to the last value the radio reported 2 s after a person moved it, so an
 * echo that repeats what was asked rather than what was done shows a person a
 * radio that is not there. The radio also sends, unasked, the signal, the RDS
 * groups and any change made on the panel.
 *
 * Over TCP the radio first sends a 16 character salt; the PC answers with the
 * SHA-1 of the salt and the password, as 40 hex characters. The hashing itself
 * is the caller's: the ESP32 has it in silicon.
 *
 * Nothing here touches the network or the tuner, so it is tested on a PC.
 */
#ifndef CORE_XDR_H
#define CORE_XDR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "band_plan.h"
#include "squelch.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The port xdrd, XDR-GTK and FM-DX Webserver all use. */
#define XDR_PORT 7373

/* The longest line read from a PC, without its line end. The longest real
 * command is a tune or a width, under 12 characters. */
#define XDR_LINE_MAX 32

/* The salt the radio sends before the login, in characters. */
#define XDR_SALT_LEN 16

/* A SHA-1 digest, in bytes and as hex characters. */
#define XDR_DIGEST_LEN 20
#define XDR_DIGEST_HEX (2 * XDR_DIGEST_LEN)

/* The fewest and most milliseconds between two signal lines. 66 is what
 * XDR-GTK assumes when it has not asked; the radio reads the tuner every
 * 100 ms, so a faster line would only repeat a reading. 1000 is the most
 * XDR-GTK's own setting offers. */
#define XDR_INTERVAL_MIN_MS 66
#define XDR_INTERVAL_MAX_MS 1000

/* What a line from a PC asks for. */
typedef enum {
  XDR_IGNORE = 0,  /* Taken, and nothing to do: a bare F, T0, a scan line. */
  XDR_START,       /* x: start a session. */
  XDR_END,         /* X: end this PC's session. */
  XDR_TUNE,        /* T: `value` in kHz. */
  XDR_MODE,        /* M: 0 FM, 1 AM. */
  XDR_WIDTH,       /* W: `value` in Hz, 0 for automatic. */
  XDR_DEEMPHASIS,  /* D: 0 for 50 µs, 1 for 75 µs, 2 for none. */
  XDR_EQ_IMS,      /* G: `value` the equalizer, `value2` iMS, 0 or 1. */
  XDR_MONO,        /* B: 0 stereo allowed, 1 forced mono, 2 the MPX out. */
  XDR_VOLUME,      /* Y: 0 to 100, 0 for silence. */
  XDR_SQUELCH,     /* Q: -1 to 100, 0 for off. */
  XDR_AGC,         /* A: 0 to 3, the RF AGC start, highest first. */
  XDR_ANTENNA,     /* Z: 0 to 3. */
  XDR_ATTENUATION, /* V: 0 to 127. */
  XDR_ROTATOR,     /* C: 0 to 2, an aerial rotator. */
  XDR_INTERVAL,    /* I: `value` ms between signal lines, 0 for the usual. */
} XdrKind;

typedef struct {
  XdrKind kind;
  int32_t value;
  int32_t value2;
} XdrCommand;

/*
 * Read one line from a PC, without its line end; a carriage return at the end
 * is dropped. NULL when it was understood, `out` filled in; otherwise a plain
 * reason, and `out` set to XDR_IGNORE. A letter this radio has no use for is
 * understood and ignored, as xdrd does, so a newer PC program still works.
 */
const char *xdrParse(const char *line, XdrCommand *out);

/*
 * A level in tenths of a dBuV as tenths of a dBf, the unit the protocol
 * carries: dBf is dBuV plus 11.25 at 75 ohms, and XDR-GTK and FM-DX Webserver
 * both take 11.25 back off to show dBuV. Rounded half away from zero.
 */
int32_t xdrDbfTenths(int16_t levelTenthsDbuV);

/*
 * The signal line: S, a flag, and the level in dBf with one decimal. The flag
 * is s for a stereo pilot, m for none, and S or M for the same with mono
 * forced. AM has no pilot and no forced mono, so it is always m.
 *
 * The protocol has two more numbers after the level, the co-channel and the
 * adjacent channel bars. This tuner reports neither in the form XDR-GTK draws
 * them, so they are left out, which the protocol allows, rather than filled
 * with numbers that mean something else.
 */
size_t xdrSignal(char *out, size_t cap, int16_t levelTenthsDbuV, bool pilot,
                 bool forcedMono, bool am);

/* P and the PI in 4 hex digits, then `doubt` question marks, 0 to 3: how
 * unsure the PI is. */
size_t xdrPi(char *out, size_t cap, uint16_t pi, uint8_t doubt);

/* R and one RDS group: blocks A to D in hex, then one byte of error levels,
 * two bits a block, A in the top pair, 0 none to 3 not corrected. The tuner
 * packs its own error byte the same way. */
size_t xdrRds(char *out, size_t cap, const uint16_t block[4], uint8_t error);

/* A letter and a number: T87500, Y58, Q-1. */
size_t xdrValue(char *out, size_t cap, char letter, long value);

/* G, then the equalizer and iMS as 0 or 1. */
size_t xdrEqIms(char *out, size_t cap, bool equalizer, bool ims);

/* o, the PCs signed in, a comma, and the guests, which this radio has none of.
 * FM-DX Webserver starts only once it has seen this line. */
size_t xdrUsers(char *out, size_t cap, unsigned users);

/* The salt from 16 random bytes, each mapped onto the 64 characters xdrd
 * uses, so every character is equally likely. */
void xdrSalt(const uint8_t random[XDR_SALT_LEN], char out[XDR_SALT_LEN + 1]);

/* A digest as 40 lower case hex characters. */
void xdrHex(const uint8_t digest[XDR_DIGEST_LEN], char out[XDR_DIGEST_HEX + 1]);

/* Whether a login line is the expected digest: exactly 40 hex characters,
 * either case, compared in a time that does not depend on where they differ. */
bool xdrDigestMatches(const char *line,
                      const char expected[XDR_DIGEST_HEX + 1]);

/* The protocol's volume, 1 to 100, in dB: 100 is 0 dB and each step 0.6 dB
 * down, so 1 is -59 dB. 0 is silence, which the caller does as a mute. */
int8_t xdrVolumeDb(int32_t volume);

/* A volume in dB as the protocol's 0 to 100: 0 when muted, at least 1 when
 * not, and 100 for anything at or above 0 dB, the top of its scale. */
int32_t xdrVolumeFromDb(int8_t db, bool muted);

/*
 * A width the protocol asks for, in Hz, as the nearest one the band offers,
 * in kHz. 0 asks the FM tuner to choose, as the radio's own automatic width.
 * The AM side has no automatic width, so 0 there keeps `current`.
 */
uint16_t xdrWidthKHz(BandId band, int32_t hz, uint16_t current);

/*
 * The squelch as the protocol's number: 0 for Off, else the level it opens at
 * in whole dBf, Manual's threshold or Auto's FM level floor, 1 to 100. Auto
 * with no level in it, on AM or with the floor off, is 1: the lowest number
 * that still says the squelch is on.
 */
int32_t xdrSquelchValue(SquelchMode mode, bool am, int16_t manualTenths,
                        uint8_t fmFloorDbuV);

/* The protocol's de-emphasis, 0 to 2, in µs, and back: 50, 75, or 0 for none. */
uint16_t xdrDeemphasisUs(int32_t code);
int32_t xdrDeemphasisCode(uint16_t us);

#ifdef __cplusplus
}
#endif

#endif /* CORE_XDR_H */
