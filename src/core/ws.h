/*
 * The WebSocket a browser page keeps open for the radio's state, RFC 6455:
 * the handshake's accept key, the header of a frame the radio sends, and
 * what a frame from the browser is.
 *
 * The radio only sends, the state as text. The page sends nothing, so the
 * one frame from a browser that matters is a close; anything else is read
 * and thrown away.
 *
 * The accept key is the base 64 of the SHA-1 of the browser's key with a
 * fixed GUID after it. The hashing itself is the caller's: the ESP32 has it
 * in silicon.
 *
 * Nothing here touches the network, so it is tested on a PC.
 */
#ifndef CORE_WS_H
#define CORE_WS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A browser's key: 16 random bytes in base 64, two of them padding. */
#define WS_KEY_LEN 24
/* What follows the key before it is hashed, the same for every server. */
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_HASH_INPUT_LEN (WS_KEY_LEN + sizeof(WS_GUID) - 1)
/* A SHA-1 digest, and the accept key made from it: 20 bytes in base 64. */
#define WS_DIGEST_LEN 20
#define WS_ACCEPT_LEN 28
/* The header of a text frame the radio sends: 2 bytes up to 125 bytes of
 * text, 4 up to 65535. The state is about 1.8 KB. */
#define WS_HEADER_MAX 4
#define WS_TEXT_MAX 65535

/* The text to hash, the browser's key with the GUID after it, into `out` of
 * `cap` bytes. False for a key that is not 22 base 64 characters and two
 * padding characters, and when `out` cannot hold the text and its end. */
bool wsHashInput(const char *key, char *out, size_t cap);

/* The accept key for the digest of that text, in `out`, with its end. */
void wsAcceptKey(const uint8_t digest[WS_DIGEST_LEN],
                 char out[WS_ACCEPT_LEN + 1]);

/* The header of one whole, unmasked text frame of `len` bytes, in `out`.
 * Its length, 2 or 4, or 0 for more than WS_TEXT_MAX bytes. */
size_t wsTextHeader(size_t len, uint8_t out[WS_HEADER_MAX]);

/* Whether bytes read from a browser start a close frame. */
bool wsIsClose(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CORE_WS_H */
