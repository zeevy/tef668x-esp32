/*
 * The web page's live state: GET /api/live becomes a WebSocket on the web
 * server's own port, and the loop sends each open page the same JSON as
 * GET /api/state every LIVE_EVERY_MS.
 *
 * Asking for the state costs the radio a whole request each time, about 85
 * to 280 ms of the loop whatever the size of the answer, so a page that asks
 * once a second makes the panel stutter. Here the connection stays open and
 * a pass is one send. The state carries the up time and the live readings,
 * so it differs from one pass to the next and is sent each time.
 *
 * Read only, as GET /api/state is, so it needs no PIN. At most
 * LIVE_PAGES_MAX pages. A send never waits, as the PC Link's do not: a page
 * whose message does not fit is dropped, and it opens a new socket. Each
 * send is a web reply to the rest of the radio: marked while it goes out and
 * noted after, so the DX session leaves out a reading taken during one.
 *
 * The web server's own handler answers the handshake and keeps a copy of
 * the connection. The server only lets go of its copy once the handler
 * returns, so the socket stays open while this file holds one.
 */
#include <Arduino.h>
#include <errno.h>
#include <lwip/sockets.h>
#include <mbedtls/sha1.h>
#include <string.h>

#include "core/ws.h"
#include "net/web_internal.h"
#include "radio_task.h"
#include "reply_times.h"

/* Two pages, the owner's choice: a phone and a laptop. Each costs about
 * 2 KB while open, the connection's receive buffer and its socket. */
#define LIVE_PAGES_MAX 2
/* The radio reads the tuner every 100 ms, and a person reads a meter no
 * faster than this. */
#define LIVE_EVERY_MS 500

static WebContext *sWeb = NULL;
/* A free place holds a client with no socket. */
static NetworkClient sPages[LIVE_PAGES_MAX];
static uint32_t sRefused = 0;
static uint32_t sLongestMs = 0;
static uint32_t sNextMs = 0;

static bool isOpen(const NetworkClient &page) {
  return page.fd() >= 0;
}

static void drop(NetworkClient &page) {
  page.stop();
  page = NetworkClient();
}

/* A page that closed, or said it is closing, gives its place back. The page
 * sends nothing else, so anything else it sends is read and let go. */
static void readPage(NetworkClient &page) {
  uint8_t in[64];
  const int n = recv(page.fd(), in, sizeof(in), MSG_DONTWAIT);
  if (n == 0 || (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN) ||
      wsIsClose(in, n > 0 ? (size_t)n : 0)) {
    drop(page);
  }
}

/* Reads every open page; how many are still open. */
static int readPages(void) {
  int open = 0;
  for (NetworkClient &page : sPages) {
    if (isOpen(page)) {
      readPage(page);
    }
    open += isOpen(page) ? 1 : 0;
  }
  return open;
}

static void handleLive(void) {
  WebServer &server = sWeb->server;
  char input[WS_HASH_INPUT_LEN + 1];
  if (server.header("Sec-WebSocket-Version") != "13" ||
      !wsHashInput(server.header("Sec-WebSocket-Key").c_str(), input,
                   sizeof(input))) {
    server.send(400, "text/plain",
                "A WebSocket key and version 13 are needed.\n");
    return;
  }
  /* A page reloaded during a level sweep closed its old socket while
   * nothing was read: read it now, so its place is free for the new one. */
  readPages();
  int place = -1;
  for (int i = 0; i < LIVE_PAGES_MAX && place < 0; i++) {
    if (!isOpen(sPages[i])) {
      place = i;
    }
  }
  if (place < 0) {
    sRefused++;
    server.send(503, "text/plain",
                "Two pages already have the live state. Close one.\n");
    return;
  }
  uint8_t digest[WS_DIGEST_LEN];
  if (mbedtls_sha1(reinterpret_cast<const unsigned char *>(input),
                   strlen(input), digest) != 0) {
    server.send(500, "text/plain", "The handshake could not be made.\n");
    return;
  }
  char accept[WS_ACCEPT_LEN + 1];
  wsAcceptKey(digest, accept);
  char reply[160];
  const int len =
      snprintf(reply, sizeof(reply),
               "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
               "Connection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n",
               accept);
  NetworkClient page = server.client();
  if (send(page.fd(), reply, len, MSG_DONTWAIT) != len) {
    return;
  }
  sPages[place] = page;
}

/* The whole frame now, or the page goes: half a state is worse than none,
 * and waiting holds the loop. */
static void sendFrame(NetworkClient &page, uint8_t *head, size_t headLen,
                      const String &text) {
  struct iovec parts[2] = {
      {head, headLen},
      {const_cast<char *>(text.c_str()), text.length()},
  };
  struct msghdr msg = {};
  msg.msg_iov = parts;
  msg.msg_iovlen = 2;
  const int want = (int)(headLen + text.length());
  if (sendmsg(page.fd(), &msg, MSG_DONTWAIT) != want) {
    drop(page);
  }
}

void webLiveRegisterRoutes(WebContext *web) {
  sWeb = web;
  web->server.on("/api/live", HTTP_GET, handleLive);
}

void webLiveCloseAll(void) {
  for (NetworkClient &page : sPages) {
    drop(page);
  }
}

void webLiveLoop(void) {
  const uint32_t now = millis();
  /* A level sweep the request just served asked for may start reading at
   * once: nothing goes out until it has finished. */
  if (readPages() == 0 || radioSweepBusy() || (int32_t)(now - sNextMs) < 0) {
    return;
  }
  sNextMs = now + LIVE_EVERY_MS;
  const uint32_t startUs = micros();
  const String state = buildState();
  uint8_t head[WS_HEADER_MAX];
  const size_t headLen = wsTextHeader(state.length(), head);
  if (headLen == 0) {
    return;
  }
  radioSetNetServing(true);
  for (NetworkClient &page : sPages) {
    if (isOpen(page)) {
      sendFrame(page, head, headLen, state);
    }
  }
  radioSetNetServing(false);
  replyTimesNote(now, millis());
  const uint32_t tookMs = (micros() - startUs + 999u) / 1000u;
  if (tookMs > sLongestMs) {
    sLongestMs = tookMs;
  }
}

uint8_t webLivePages(void) {
  uint8_t n = 0;
  for (const NetworkClient &page : sPages) {
    n += isOpen(page) ? 1 : 0;
  }
  return n;
}

uint32_t webLiveRefused(void) {
  return sRefused;
}

uint32_t webLiveLongestMs(void) {
  return sLongestMs;
}
