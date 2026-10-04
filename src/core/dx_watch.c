/* The preset watch. */
#include "dx_watch.h"

#include <stddef.h>
#include <string.h>

void dxWatchSetChannels(DxWatch *w, const uint32_t *khz, uint8_t count) {
  if (w == NULL) {
    return;
  }
  if (khz == NULL) {
    count = 0;
  }
  if (count > DX_WATCH_MAX) {
    count = DX_WATCH_MAX;
  }
  const uint32_t due = w->count > 0 ? w->ch[w->cursor].khz : 0;
  /* In place, so no second list is needed: the channels before `i` are the
   * new list, and the old ones not yet claimed sit from `i` to `old`. */
  uint8_t old = w->count;
  for (uint8_t i = 0; i < count; i++) {
    uint8_t j = i;
    while (j < old && w->ch[j].khz != khz[i]) {
      j++;
    }
    if (j < old) {
      const DxWatchChannel t = w->ch[i];
      w->ch[i] = w->ch[j];
      w->ch[j] = t;
      continue;
    }
    if (i < old) {
      /* A new channel takes an old one's place; the old one moves to the
       * end, where a later channel can still claim it, while there is room
       * there. */
      if (old < DX_WATCH_MAX) {
        w->ch[old++] = w->ch[i];
      }
    } else {
      old = (uint8_t)(i + 1);
    }
    memset(&w->ch[i], 0, sizeof(w->ch[i]));
    w->ch[i].khz = khz[i];
  }
  const uint8_t n = count;
  w->count = n;
  /* The channel that was due stays due, so a list given again does not
   * start the round over, or drop a confirmation. */
  w->cursor = 0;
  bool found = false;
  for (uint8_t i = 0; i < n; i++) {
    if (w->ch[i].khz == due) {
      w->cursor = i;
      found = true;
      break;
    }
  }
  if (!found) {
    w->confirming = false;
  }
}

bool dxWatchNext(const DxWatch *w, uint32_t *khz) {
  if (w == NULL || khz == NULL || w->count == 0) {
    return false;
  }
  *khz = w->ch[w->cursor].khz;
  return true;
}

/* The median of what a channel holds, which must be at least one. */
static int16_t floorOf(const DxWatchChannel *c) {
  int16_t v[DX_WATCH_HISTORY];
  memcpy(v, c->recent, sizeof(v[0]) * c->held);
  for (uint8_t i = 1; i < c->held; i++) {
    for (uint8_t j = i; j > 0 && v[j - 1] > v[j]; j--) {
      const int16_t t = v[j];
      v[j] = v[j - 1];
      v[j - 1] = t;
    }
  }
  return v[c->held / 2];
}

static void keep(DxWatchChannel *c, int16_t level) {
  c->recent[c->next] = level;
  c->next = (uint8_t)((c->next + 1) % DX_WATCH_HISTORY);
  if (c->held < DX_WATCH_HISTORY) {
    c->held++;
  }
}

static void advance(DxWatch *w) {
  w->confirming = false;
  w->cursor = (uint8_t)((w->cursor + 1) % w->count);
}

DxWatchResult dxWatchFeed(DxWatch *w, uint32_t khz, int16_t levelTenths,
                          bool ok, int16_t *rise) {
  if (rise != NULL) {
    *rise = 0;
  }
  if (w == NULL || w->count == 0 || w->ch[w->cursor].khz != khz) {
    return DX_WATCH_QUIET;
  }
  DxWatchChannel *c = &w->ch[w->cursor];
  if (!ok) {
    advance(w);
    return DX_WATCH_QUIET;
  }
  if (c->held < DX_WATCH_HISTORY) {
    keep(c, levelTenths);
    advance(w);
    return DX_WATCH_QUIET;
  }
  const int16_t by = (int16_t)(levelTenths - floorOf(c));
  if (rise != NULL) {
    *rise = by;
  }
  if (by <= DX_WATCH_RISE_TENTHS) {
    keep(c, levelTenths);
    advance(w);
    return DX_WATCH_QUIET;
  }
  if (!w->confirming) {
    w->confirming = true;
    return DX_WATCH_SUSPECT;
  }
  /* Up. Its floor is now where it is, so it is flagged again only if it
   * rises as far a second time. */
  for (uint8_t i = 0; i < DX_WATCH_HISTORY; i++) {
    c->recent[i] = levelTenths;
  }
  c->held = DX_WATCH_HISTORY;
  c->next = 0;
  advance(w);
  return DX_WATCH_UP;
}
