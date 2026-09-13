/* Implementation of the stored channels. */
#include "memory.h"

#include <string.h>

void memoryInit(MemoryStore *m) {
  if (m == NULL) {
    return;
  }
  memset(m, 0, sizeof(*m));
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

bool memoryClear(MemoryStore *m, int slot) {
  if (m == NULL || !memorySlotInRange(slot)) {
    return false;
  }
  memset(&m->slot[slot], 0, sizeof(m->slot[slot]));
  return true;
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

int memoryStep(const MemoryStore *m, int from, bool up) {
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
    if (m->slot[at].freqKHz != 0) {
      return at;
    }
  }
  return MEMORY_NO_SLOT;
}

int memoryStepTunable(const MemoryStore *m, const BandPlanConfig *plan,
                      int from, bool up) {
  if (m == NULL) {
    return MEMORY_NO_SLOT;
  }
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
