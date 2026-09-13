/*
 * The 99 stored channels, and stepping through them.
 *
 * A channel is a band, a frequency, a filter width and a name. That is what
 * decision 13 says the web editor lets a person change, and it is what the
 * radio needs to put itself back on a station.
 *
 * Nothing in here touches hardware or NVS, so it builds and is tested on a PC.
 * The NVS side lives in drivers/memory_nvs.h and the text side in
 * memory_csv.h.
 *
 * An empty slot is a frequency of 0. No band starts at 0 kHz, so there is no
 * real channel this could be confused with, and a store that has been zeroed
 * is a store with no channels in it rather than 99 channels on 0 kHz.
 */
#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "band_plan.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * How many channels the radio stores.
 *
 * 99, matching the firmware this replaces, so a list moves between the two.
 * The whole list is kept in RAM, which is what makes the knob step to the
 * next channel without a flash read.
 */
#define MEMORY_SLOT_COUNT 99

/* Room for a 16 character name and its terminator. */
#define MEMORY_NAME_LEN 17

/* What a caller gets back when there is no slot to give. */
#define MEMORY_NO_SLOT (-1)

/* One stored channel. */
typedef struct {
  uint32_t freqKHz; /* Where it is. 0 means the slot is empty. */
  /* Filter width in kHz. 0 lets the radio choose, on either band. */
  uint16_t bandwidthKHz;
  uint8_t band;               /* See BandId. */
  char name[MEMORY_NAME_LEN]; /* What to call it. May be empty. */
} MemoryChannel;

/* Every slot. Zero it before first use, or call memoryInit. */
typedef struct {
  MemoryChannel slot[MEMORY_SLOT_COUNT]; /* Slot 0 is channel 1. */
} MemoryStore;

void memoryInit(MemoryStore *m);

bool memorySlotInRange(int slot);

/*
 * Whether a channel could have been written by this firmware.
 *
 * Not whether it can be tuned now: a band plan change moves the edges under a
 * channel that was correct when it was stored. See memoryChannelTunable.
 */
bool memoryChannelValid(const MemoryChannel *c);

bool memoryChannelTunable(const MemoryChannel *c, const BandPlanConfig *plan);

bool memorySlotUsed(const MemoryStore *m, int slot);

const MemoryChannel *memoryGet(const MemoryStore *m, int slot);

bool memorySet(MemoryStore *m, int slot, const MemoryChannel *c);

bool memoryClear(MemoryStore *m, int slot);

int memoryCount(const MemoryStore *m);

int memoryFirstFree(const MemoryStore *m);

int memoryFind(const MemoryStore *m, uint8_t band, uint32_t freqKHz);

/*
 * The next filled slot in a direction, wrapping at the ends.
 *
 * Empty slots are stepped over. Starting from outside the store starts from
 * the end it is heading away from, so MEMORY_NO_SLOT going up gives the
 * lowest filled slot.
 */
int memoryStep(const MemoryStore *m, int from, bool up);

/*
 * The next filled slot that can actually be tuned, wrapping at the ends.
 *
 * The same as memoryStep, but a channel the band plan in force cannot reach
 * is stepped over as though the slot were empty. Changing the FM region or
 * the medium wave spacing can put a stored channel outside its band, and
 * without this the knob would stop on it and the radio would refuse to move.
 */
int memoryStepTunable(const MemoryStore *m, const BandPlanConfig *plan,
                      int from, bool up);

#ifdef __cplusplus
}
#endif

#endif /* CORE_MEMORY_H */
