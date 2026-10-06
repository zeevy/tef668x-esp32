/* Implementation of the stored channels. */
#include "memory.h"

#include <string.h>

void memoryInit(MemoryStore *m) {
  if (m == NULL) {
    return;
  }
  memset(m, 0, sizeof(*m));
}

/*
 * The first four bytes of a stored list that carries a header, "SMEM" read
 * as a little endian number. A list without the header starts with slot
 * one's frequency instead, and no band reaches anywhere near this many kHz,
 * so the two cannot be mistaken for each other.
 */
#define MEMORY_BLOB_MAGIC 0x4D454D53UL

/* The bare list the first firmware wrote: 99 channels of 24 bytes. Written
 * out rather than taken from sizeof, because it describes flash already out
 * there, not the struct as it is now. */
#define MEMORY_BLOB_BARE_BYTES 2376u

/* One channel as version 1 and the bare list wrote it, before the PI. */
typedef struct {
  uint32_t freqKHz;
  uint16_t bandwidthKHz;
  uint8_t band;
  char name[MEMORY_NAME_LEN];
} MemoryChannelV1;

/* A version 1 list, `MEMORY_BLOB_BARE_BYTES` of them, into the list as it
 * is now, every PI 0. */
static bool fromVersion1(const uint8_t *body, MemoryStore *out) {
  if (sizeof(MemoryChannelV1) * MEMORY_SLOT_COUNT != MEMORY_BLOB_BARE_BYTES) {
    return false;
  }
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    MemoryChannelV1 old;
    memcpy(&old, body + (size_t)i * sizeof(old), sizeof(old));
    MemoryChannel *c = &out->slot[i];
    memset(c, 0, sizeof(*c));
    c->freqKHz = old.freqKHz;
    c->bandwidthKHz = old.bandwidthKHz;
    c->band = old.band;
    memcpy(c->name, old.name, sizeof(c->name));
  }
  return true;
}

size_t memoryBlobSize(void) {
  return MEMORY_BLOB_HEADER_BYTES + sizeof(MemoryStore);
}

size_t memoryToBlob(const MemoryStore *m, uint8_t *out, size_t cap) {
  if (m == NULL || out == NULL || cap < memoryBlobSize()) {
    return 0;
  }
  const uint32_t magic = MEMORY_BLOB_MAGIC;
  const uint16_t version = MEMORY_BLOB_VERSION;
  const uint16_t body = (uint16_t)sizeof(MemoryStore);
  memcpy(out, &magic, sizeof(magic));
  memcpy(out + 4, &version, sizeof(version));
  memcpy(out + 6, &body, sizeof(body));
  memcpy(out + MEMORY_BLOB_HEADER_BYTES, m, sizeof(MemoryStore));
  return memoryBlobSize();
}

bool memoryFromBlob(const uint8_t *blob, size_t len, MemoryStore *out) {
  if (out == NULL) {
    return false;
  }
  memoryInit(out);
  if (blob == NULL) {
    return false;
  }
  uint32_t magic = 0;
  if (len >= MEMORY_BLOB_HEADER_BYTES) {
    memcpy(&magic, blob, sizeof(magic));
  }
  if (magic != MEMORY_BLOB_MAGIC) {
    /* No header: the bare list, which is version 1's layout exactly. */
    if (len != MEMORY_BLOB_BARE_BYTES) {
      return false;
    }
    return fromVersion1(blob, out);
  }
  uint16_t version = 0;
  uint16_t body = 0;
  memcpy(&version, blob + 4, sizeof(version));
  memcpy(&body, blob + 6, sizeof(body));
  if (len != (size_t)MEMORY_BLOB_HEADER_BYTES + body) {
    return false;
  }
  if (version == 1 && body == MEMORY_BLOB_BARE_BYTES) {
    return fromVersion1(blob + MEMORY_BLOB_HEADER_BYTES, out);
  }
  if (version != MEMORY_BLOB_VERSION || body != sizeof(MemoryStore)) {
    /* A later version, or a header that does not describe what follows. */
    return false;
  }
  memcpy(out, blob + MEMORY_BLOB_HEADER_BYTES, sizeof(MemoryStore));
  return true;
}

bool memorySlotInRange(int slot) {
  return slot >= 0 && slot < MEMORY_SLOT_COUNT;
}

bool memoryChannelValid(const MemoryChannel *c) {
  if (c == NULL || c->freqKHz == 0 || c->band >= BAND_COUNT) {
    return false;
  }
  /* 0 means let the radio choose the width, on every band. On FM that is
   * already the tuner's adaptive setting. On AM it means the width the band
   * starts on, because a hand written list has no reason to carry one and a
   * line refused for leaving it out would be a channel lost for nothing. */
  if (c->bandwidthKHz != 0 &&
      !bandBandwidthAllowed((BandId)c->band, c->bandwidthKHz)) {
    return false;
  }
  /* The name has to end inside the field and hold nothing but printable
   * ASCII. A name carrying a newline or a control character would split one
   * exported line into two, and the file would then read back as a different
   * list without anything having failed. */
  bool terminated = false;
  for (size_t i = 0; i < MEMORY_NAME_LEN; i++) {
    if (c->name[i] == '\0') {
      terminated = true;
      break;
    }
    if (c->name[i] < 0x20 || c->name[i] > 0x7E) {
      return false;
    }
  }
  return terminated;
}

bool memoryChannelTunable(const MemoryChannel *c, const BandPlanConfig *plan) {
  if (!memoryChannelValid(c) || plan == NULL) {
    return false;
  }
  /* The band the plan would pick for this frequency has to be the band the
   * channel names, not merely a band that contains it. Two bands overlap: the
   * full FM region starts at 65 MHz and covers the whole of OIRT. A channel
   * stored as FM at 70 MHz is inside FM, so a test of containment alone would
   * call it reachable, and tuning it would land the radio on OIRT with OIRT's
   * step size. A slot the radio stops on and a slot it can actually reach
   * have to be the same set. */
  BandId band;
  if (!bandForFrequency(plan, c->freqKHz, &band)) {
    return false;
  }
  return band == (BandId)c->band;
}

int memorySanitize(MemoryStore *m) {
  if (m == NULL) {
    return 0;
  }
  int lost = 0;
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    if (memoryChannelValid(&m->slot[i])) {
      continue;
    }
    if (m->slot[i].freqKHz != 0) {
      lost++;
    }
    memset(&m->slot[i], 0, sizeof(m->slot[i]));
  }
  return lost;
}

bool memorySlotUsed(const MemoryStore *m, int slot) {
  if (m == NULL || !memorySlotInRange(slot)) {
    return false;
  }
  return m->slot[slot].freqKHz != 0;
}

const MemoryChannel *memoryGet(const MemoryStore *m, int slot) {
  if (!memorySlotUsed(m, slot)) {
    return NULL;
  }
  return &m->slot[slot];
}

bool memorySet(MemoryStore *m, int slot, const MemoryChannel *c) {
  if (m == NULL || !memorySlotInRange(slot) || !memoryChannelValid(c)) {
    return false;
  }
  m->slot[slot] = *c;
  /* Everything past the terminator is zeroed rather than copied, so the same
   * list is always the same bytes. What goes to flash is then decided by what
   * the list holds and not by what happened to be in the caller's struct. */
  size_t used = strlen(m->slot[slot].name);
  memset(m->slot[slot].name + used, 0, MEMORY_NAME_LEN - used);
  return true;
}

int memoryNthUsed(const MemoryStore *m, int n) {
  if (m == NULL || n < 0) {
    return MEMORY_NO_SLOT;
  }
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    if (m->slot[i].freqKHz != 0 && n-- == 0) {
      return i;
    }
  }
  return MEMORY_NO_SLOT;
}

int memoryCount(const MemoryStore *m) {
  if (m == NULL) {
    return 0;
  }
  int count = 0;
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    if (m->slot[i].freqKHz != 0) {
      count++;
    }
  }
  return count;
}

int memoryFirstFree(const MemoryStore *m) {
  if (m == NULL) {
    return MEMORY_NO_SLOT;
  }
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    if (m->slot[i].freqKHz == 0) {
      return i;
    }
  }
  return MEMORY_NO_SLOT;
}

int memoryFind(const MemoryStore *m, uint8_t band, uint32_t freqKHz) {
  if (m == NULL || freqKHz == 0) {
    return MEMORY_NO_SLOT;
  }
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    if (m->slot[i].freqKHz == freqKHz && m->slot[i].band == band) {
      return i;
    }
  }
  return MEMORY_NO_SLOT;
}

/* Whether `slot` is in the store and holds this band and frequency. */
static bool slotHolds(const MemoryStore *m, int slot, uint8_t band,
                      uint32_t freqKHz) {
  const MemoryChannel *c = memoryGet(m, slot);
  return c != NULL && c->freqKHz == freqKHz && c->band == band;
}

int memoryPick(const MemoryStore *m, int first, int second, uint8_t band,
               uint32_t freqKHz) {
  if (m == NULL) {
    return MEMORY_NO_SLOT;
  }
  if (slotHolds(m, first, band, freqKHz)) {
    return first;
  }
  if (slotHolds(m, second, band, freqKHz)) {
    return second;
  }
  return memoryFind(m, band, freqKHz);
}

MemorySaveResult memorySaveChannel(MemoryStore *m, uint8_t band,
                                   uint32_t freqKHz, uint16_t bandwidthKHz,
                                   const char *name, int *slot) {
  if (m == NULL || slot == NULL) {
    return MEMORY_SAVE_INVALID;
  }
  int at = *slot;
  if (at == MEMORY_NO_SLOT) {
    at = memoryFirstFree(m);
    if (at == MEMORY_NO_SLOT) {
      return MEMORY_SAVE_FULL;
    }
  }
  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  c.band = band;
  c.freqKHz = freqKHz;
  c.bandwidthKHz = bandwidthKHz;
  if (name != NULL) {
    strncpy(c.name, name, MEMORY_NAME_LEN - 1);
  }
  if (!memorySet(m, at, &c)) {
    return MEMORY_SAVE_INVALID;
  }
  *slot = at;
  return MEMORY_SAVE_OK;
}

int memoryFindNear(const MemoryStore *m, uint8_t band, uint32_t freqKHz,
                   uint32_t toleranceKHz) {
  if (m == NULL || freqKHz == 0) {
    return MEMORY_NO_SLOT;
  }
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    if (m->slot[i].freqKHz == 0 || m->slot[i].band != band) {
      continue;
    }
    const uint32_t diff = m->slot[i].freqKHz > freqKHz
                              ? m->slot[i].freqKHz - freqKHz
                              : freqKHz - m->slot[i].freqKHz;
    if (diff <= toleranceKHz) {
      return i;
    }
  }
  return MEMORY_NO_SLOT;
}

int memoryStepTunable(const MemoryStore *m, const BandPlanConfig *plan,
                      int from, bool up) {
  if (m == NULL) {
    return MEMORY_NO_SLOT;
  }
  /* Anything outside the store starts from beyond the end it is heading away
   * from, so the first step lands on the first slot in that direction. */
  int at = memorySlotInRange(from) ? from : (up ? -1 : MEMORY_SLOT_COUNT);
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    at = up ? at + 1 : at - 1;
    if (at >= MEMORY_SLOT_COUNT) {
      at = 0;
    } else if (at < 0) {
      at = MEMORY_SLOT_COUNT - 1;
    }
    if (memoryChannelTunable(&m->slot[at], plan)) {
      return at;
    }
  }
  return MEMORY_NO_SLOT;
}
