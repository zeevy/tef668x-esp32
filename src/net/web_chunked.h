/*
 * A page sent in pieces, never built whole in heap first.
 *
 * `WebServer` can send a response as chunks: a `Content-Length` of
 * `CONTENT_LENGTH_UNKNOWN`, `send` with an empty body to open the reply,
 * then `sendContent` called as many times as needed, and a final empty
 * `sendContent` to close it. This wraps that behind the same `+=` a
 * `String` takes, so a page builder writes `out += F("...")`. Each 512 bytes
 * are handed to the client and the buffer behind `out` is reused for the
 * next 512, rather than one allocation growing to the size of the whole
 * page.
 *
 * ESP32 only. `F()` and `__FlashStringHelper` exist for API compatibility
 * with AVR's separate flash address space; on this chip flash is memory
 * mapped, so an `__FlashStringHelper*` is read with a plain `strlen` and
 * `memcpy`, the same as the ESP32 Arduino core's own `String` class does
 * internally. Built for AVR, that same code would read past the end of
 * whatever the pointer's low bytes happened to alias in RAM.
 */
#ifndef NET_WEB_CHUNKED_H
#define NET_WEB_CHUNKED_H

#include <Arduino.h>
#include <WebServer.h>
#include <string.h>

class ChunkedReply {
 public:
  explicit ChunkedReply(WebServer &server) : mServer(server), mBuf{} {}

  /* Flushes whatever is left and closes the reply, so a page builder that
   * returns early, the sign in form in place of the page it was asked
   * for, still ends the response correctly. */
  ~ChunkedReply() {
    end();
  }

  /* Opens the reply. `sendHeader` calls belong before this. */
  void begin(int code, const char *contentType) {
    mServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
    mServer.send(code, contentType, "");
    mOpen = true;
  }

  ChunkedReply &operator+=(const char *text) {
    append(text, strlen(text));
    return *this;
  }

  ChunkedReply &operator+=(const __FlashStringHelper *text) {
    const char *p = reinterpret_cast<const char *>(text);
    append(p, strlen(p));
    return *this;
  }

  ChunkedReply &operator+=(const String &text) {
    append(text.c_str(), text.length());
    return *this;
  }

 private:
  static const size_t kBufLen = 512;

  void append(const char *data, size_t len) {
    while (len > 0) {
      size_t room = kBufLen - mFilled;
      size_t take = len < room ? len : room;
      memcpy(mBuf + mFilled, data, take);
      mFilled += take;
      data += take;
      len -= take;
      if (mFilled == kBufLen) {
        flush();
      }
    }
  }

  void flush() {
    if (mFilled > 0 && mOpen) {
      /* A long page to a slow phone can outlast one feed of the task
       * watchdog per pass of the loop, so each chunk feeds it. */
      feedLoopWDT();
      mServer.sendContent(mBuf, mFilled);
      mFilled = 0;
    }
  }

  /* Flushes what is left, then the empty chunk that tells the client the
   * reply is complete. Safe to call once whether or not `begin` ever ran:
   * a page that signs somebody out before writing anything still needs
   * this to be a no-op rather than a reply with no `Content-Length` and
   * nothing ever sent to close it. */
  void end() {
    if (!mOpen) {
      return;
    }
    flush();
    mServer.sendContent("");
    mOpen = false;
  }

  WebServer &mServer;
  char mBuf[kBufLen];
  size_t mFilled = 0;
  bool mOpen = false;
};

#endif /* NET_WEB_CHUNKED_H */
