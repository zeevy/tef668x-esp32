#include "core/ws.h"

#include <string.h>

static const char kBase64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static bool isBase64(char c) {
  return c != '\0' && strchr(kBase64, c) != NULL;
}

bool wsHashInput(const char *key, char *out, size_t cap) {
  if (key == NULL || out == NULL || cap < WS_HASH_INPUT_LEN + 1 ||
      strlen(key) != WS_KEY_LEN) {
    return false;
  }
  for (int i = 0; i < WS_KEY_LEN - 2; i++) {
    if (!isBase64(key[i])) {
      return false;
    }
  }
  if (key[WS_KEY_LEN - 2] != '=' || key[WS_KEY_LEN - 1] != '=') {
    return false;
  }
  memcpy(out, key, WS_KEY_LEN);
  memcpy(out + WS_KEY_LEN, WS_GUID, sizeof(WS_GUID));
  return true;
}

void wsAcceptKey(const uint8_t digest[WS_DIGEST_LEN],
                 char out[WS_ACCEPT_LEN + 1]) {
  int o = 0;
  /* Six whole groups of three bytes, then the last two with one pad. */
  for (int i = 0; i < WS_DIGEST_LEN; i += 3) {
    const uint32_t b = ((uint32_t)digest[i] << 16) |
                       ((uint32_t)digest[i + 1] << 8) |
                       (i + 2 < WS_DIGEST_LEN ? digest[i + 2] : 0u);
    out[o++] = kBase64[(b >> 18) & 63];
    out[o++] = kBase64[(b >> 12) & 63];
    out[o++] = kBase64[(b >> 6) & 63];
    out[o++] = i + 2 < WS_DIGEST_LEN ? kBase64[b & 63] : '=';
  }
  out[o] = '\0';
}

size_t wsTextHeader(size_t len, uint8_t out[WS_HEADER_MAX]) {
  /* FIN set, opcode 1: one whole text frame. Never masked from a server. */
  out[0] = 0x81;
  if (len <= 125) {
    out[1] = (uint8_t)len;
    return 2;
  }
  if (len <= WS_TEXT_MAX) {
    out[1] = 126;
    out[2] = (uint8_t)(len >> 8);
    out[3] = (uint8_t)len;
    return 4;
  }
  return 0;
}

bool wsIsClose(const uint8_t *data, size_t len) {
  return data != NULL && len > 0 && (data[0] & 0x0F) == 0x8;
}
