/**
 * @file tef668x_patch.h
 * @brief The tuner's own firmware, which has to be loaded at every power on.
 *
 * The TEF668x has no usable firmware in ROM. Until a patch is written to it
 * over I2C it will not tune anything, so this is not an optional extra, it is
 * what makes the chip a radio.
 *
 * **Where this came from.** The blobs are NXP's, and they reached this project
 * through the PE5PVB firmware, which is GPLv3 and ships the same bytes for the
 * same chip. They are data, not source: there is no other way to obtain them
 * and nothing here was written by hand. The rest of this driver is a rewrite
 * and shares no code with that project.
 *
 * Two versions exist and the chip decides which one it wants. Read the
 * identification and work it out, never assume.
 */
#ifndef DRIVERS_TEF668X_PATCH_H
#define DRIVERS_TEF668X_PATCH_H

#include <stddef.h>
#include <stdint.h>

/** One patch, and the lookup table that goes with it. */
typedef struct {
  uint16_t version;    /**< 102 or 205, as the identification reports it. */
  const uint8_t *data; /**< The patch itself. */
  size_t dataLen;      /**< How many bytes the patch is. */
  const uint8_t *lut;  /**< The lookup table written after the patch. */
  size_t lutLen;       /**< How many bytes the lookup table is. */
} Tef668xPatch;

/**
 * Find the patch for a tuner version.
 *
 * @param version  102 or 205.
 * @return The patch, or NULL when this firmware does not carry that version.
 */
const Tef668xPatch *tef668xPatchFor(uint16_t version);

#endif /* DRIVERS_TEF668X_PATCH_H */
