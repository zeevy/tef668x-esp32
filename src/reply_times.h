/*
 * When the web server's last replies went out.
 *
 * The radio's Wi-Fi transmitting raises the level read at that moment,
 * as measured on this radio, so a DX reading taken near a reply can be told
 * from one that was not. The web server notes each reply here, and the DX
 * session and the API ask how far a reading was from the nearest. Kept apart
 * from both, so neither has to include the other for it.
 *
 * Loop task only: the web server's replies and the DX session both run
 * there.
 */
#ifndef REPLY_TIMES_H
#define REPLY_TIMES_H

#include <stdint.h>

/* A reply was served from `fromMs` to `toMs`: from just before the call that
 * served it to just after, which is all the loop task knows of it. */
void replyTimesNote(uint32_t fromMs, uint32_t toMs);

/*
 * How far the span `fromMs` to `toMs` was from the nearest of the last 32
 * replies, in milliseconds: 0 when they overlap, and INT32_MAX with none
 * kept, or when the span is older than the oldest kept once 32 are held,
 * since a reply near it may have been dropped.
 */
int32_t replyTimesGapMs(uint32_t fromMs, uint32_t toMs);

#endif /* REPLY_TIMES_H */
