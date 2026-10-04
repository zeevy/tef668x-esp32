/*
 * The 99 stored channels, and stepping through them.
 *
 * A channel is a band, a frequency, a filter width, a name and the station's
 * PI once one is learnt. The first four are what the web editor lets a
 * person change, and what the radio needs to put itself back on a station.
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
 * 99, matching the PE5PVB TEF6686_ESP32 firmware, so a list moves between the
 * two.
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
  /* The station's PI, learnt the first time one is confirmed on this
   * channel, or 0 for none known. 0000 never names a station, so 0 cannot
   * be a real one. Version 2. */
  uint16_t pi;
} MemoryChannel;

/* Every slot. Zero it before first use, or call memoryInit. */
typedef struct {
  MemoryChannel slot[MEMORY_SLOT_COUNT]; /* Slot 0 is channel 1. */
} MemoryStore;

void memoryInit(MemoryStore *m);

/*
 * The stored form of the list: an eight byte header, then the list.
 *
 * The header is a magic number, the version of the layout the list was
 * written in, and the length of what follows. Without it the only check a
 * load could make was the length, so any change to MemoryChannel would make
 * every stored list unreadable, and the next edit would write an empty list
 * over it. With it, a change to the channel is a new version and a case in
 * memoryFromBlob that reads the old one, the way settings.c handles the
 * settings.
 *
 * Changing MemoryChannel or MEMORY_SLOT_COUNT changes the stored form. Bump
 * MEMORY_BLOB_VERSION and teach memoryFromBlob to read the version before;
 * a unit test on the stored size fails until that is done. Version 2 added
 * `pi`; version 1 and the bare list are read with every PI 0.
 */
#define MEMORY_BLOB_VERSION 2
#define MEMORY_BLOB_HEADER_BYTES 8

/* Bytes memoryToBlob writes, header included. */
size_t memoryBlobSize(void);

/* The list in its stored form. Answers the bytes written, or 0 when `out`
 * is too small or either pointer is NULL. */
size_t memoryToBlob(const MemoryStore *m, uint8_t *out, size_t cap);

/*
 * The list out of its stored form, whichever form that is.
 *
 * A blob with the header is read by its version. One without it is the form
 * the first firmware wrote, the bare list, which is version 1's layout. A
 * version this firmware does not know is refused rather than guessed at,
 * because a newer firmware's channel read as this one's puts every field in
 * the wrong place. On false `out` is left empty. The contents are not
 * checked here: that is memorySanitize's job.
 */
bool memoryFromBlob(const uint8_t *blob, size_t len, MemoryStore *out);

/*
 * Empty every slot that this firmware could not have written, and say how
 * many channels that lost.
 *
 * For a store that arrives from somewhere other than memorySet, which is the
 * only writer that checks what it is given. A slot that fails
 * memoryChannelValid is zeroed rather than kept, because a channel whose name
 * does not end inside its field is read past the field by anything that walks
 * it to the terminator. The count is of slots that held a frequency, so it is
 * the number of channels a person has lost, not the number of bytes tidied.
 */
int memorySanitize(MemoryStore *m);

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

int memoryCount(const MemoryStore *m);

/* The slot of the n-th used slot, n counted from 0 in slot order, or
 * MEMORY_NO_SLOT when fewer than n + 1 are used. */
int memoryNthUsed(const MemoryStore *m, int n);

int memoryFirstFree(const MemoryStore *m);

int memoryFind(const MemoryStore *m, uint8_t band, uint32_t freqKHz);

/*
 * The nearest used slot on `band` within `toleranceKHz` of `freqKHz`, or
 * MEMORY_NO_SLOT if none is that close.
 *
 * For deciding whether a channel a scan just found already has a memory
 * channel near enough that it is the same station: a scan and a person's
 * own earlier save rarely land on the exact frequency memoryFind checks
 * for, since a scan walks a fixed raster and a person can store whatever
 * the dial is on at the time.
 */
int memoryFindNear(const MemoryStore *m, uint8_t band, uint32_t freqKHz,
                   uint32_t toleranceKHz);

/*
 * Which slot the radio is on, for a dial on `band` and `freqKHz`.
 *
 * Two slots can hold one station, and memoryFind gives the lowest of them. So
 * `first`, then `second`, is kept while it holds this band and frequency, and
 * memoryFind is asked only when neither does. The radio passes the slot a
 * person last chose, by a recall or a knob step, then the one it was on.
 */
int memoryPick(const MemoryStore *m, int first, int second, uint8_t band,
               uint32_t freqKHz);

/* What came of storing a channel. */
typedef enum {
  MEMORY_SAVE_OK,         /* Written, into the slot `*slot` names. */
  MEMORY_SAVE_FULL,       /* No slot was named and none is free. */
  MEMORY_SAVE_INVALID,    /* Not a preset: the name, width or frequency. */
  MEMORY_SAVE_UNREACHABLE /* For the store around this: it did not start. */
} MemorySaveResult;

/*
 * Store a channel into `*slot`, or into the lowest free slot when `*slot` is
 * MEMORY_NO_SLOT, with `name`, or no name when it is NULL.
 *
 * It replaces the slot outright: keeping an old name on a different station
 * would be worse than losing it. A name longer than a slot holds is cut to
 * fit. Nothing changes unless the answer is MEMORY_SAVE_OK, and only then does
 * `*slot` name where it went.
 */
MemorySaveResult memorySaveChannel(MemoryStore *m, uint8_t band,
                                   uint32_t freqKHz, uint16_t bandwidthKHz,
                                   const char *name, int *slot);

/*
 * The next filled slot in a direction, wrapping at the ends.
 *
 * Empty slots are stepped over. Starting from outside the store starts from
 * the end it is heading away from, so MEMORY_NO_SLOT going up gives the
 * lowest filled slot.
 *
 * The firmware steps with memoryStepTunable below. Only the unit tests call
 * this one directly.
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
