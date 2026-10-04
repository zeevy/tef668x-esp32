/*
 * The tuner's own firmware, which has to be loaded at every power on.
 *
 * The TEF668x has no usable firmware in ROM. Until a patch is written to it
 * over I2C it will not tune anything, so this is not an optional extra, it is
 * what makes the chip a radio.
 *
 * **Where this came from.** The bytes are NXP's firmware for the tuner. NXP
 * has not published terms for them that this project knows of. They are
 * carried here as the PE5PVB TEF6686_ESP32 firmware carries them, and that
 * firmware is GPLv3. They are data, not source, and nothing in them was
 * written by hand. The rest of this driver is a rewrite and shares no code
 * with that project.
 *
 * Two versions exist and the chip decides which one it wants. Read the
 * identification and work it out, never assume.
 */
#ifndef DRIVERS_TEF668X_PATCH_H
#define DRIVERS_TEF668X_PATCH_H

#include <stddef.h>
#include <stdint.h>

/* One patch, and the lookup table that goes with it. */
typedef struct {
  uint16_t version;    /* 102 or 205, as the identification reports it. */
  const uint8_t *data; /* The patch itself. */
  size_t dataLen;      /* How many bytes the patch is. */
  const uint8_t *lut;  /* The lookup table written after the patch. */
  size_t lutLen;       /* How many bytes the lookup table is. */
} Tef668xPatch;

const Tef668xPatch *tef668xPatchFor(uint16_t version);

#endif /* DRIVERS_TEF668X_PATCH_H */
