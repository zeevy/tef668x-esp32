#include "reply_times.h"

#define REPLY_WINDOWS 32
typedef struct {
  uint32_t fromMs;
  uint32_t toMs;
} ReplyWindow;
static ReplyWindow sReplies[REPLY_WINDOWS];
static uint8_t sReplyCount = 0;
static uint8_t sReplyNext = 0;

void replyTimesNote(uint32_t fromMs, uint32_t toMs) {
  sReplies[sReplyNext].fromMs = fromMs;
  sReplies[sReplyNext].toMs = toMs;
  sReplyNext = (uint8_t)((sReplyNext + 1) % REPLY_WINDOWS);
  if (sReplyCount < REPLY_WINDOWS) {
    sReplyCount++;
  }
}

int32_t replyTimesGapMs(uint32_t fromMs, uint32_t toMs) {
  /* Once the ring is full the oldest kept is where it next writes. A span
   * that ended before that one began may have had a reply near it that is
   * no longer kept, so there is no saying. */
  if (sReplyCount == REPLY_WINDOWS &&
      (int32_t)(sReplies[sReplyNext].fromMs - toMs) > 0) {
    return INT32_MAX;
  }
  int32_t best = INT32_MAX;
  for (uint8_t i = 0; i < sReplyCount; i++) {
    const ReplyWindow *w = &sReplies[i];
    int32_t gap = 0;
    if ((int32_t)(w->fromMs - toMs) > 0) {
      gap = (int32_t)(w->fromMs - toMs);
    } else if ((int32_t)(fromMs - w->toMs) > 0) {
      gap = (int32_t)(fromMs - w->toMs);
    }
    if (gap < best) {
      best = gap;
    }
  }
  return best;
}
