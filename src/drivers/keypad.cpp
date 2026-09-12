/**
 * @file keypad.cpp
 * @brief Implementation of the PCA9555 keypad.
 */
#include "keypad.h"

#include "board/board.h"

#include <Arduino.h>
#include <Wire.h>

#include "i2c_bus.h"

/** Input port 0. Reading from here gives both bytes, low byte first. */
#define PCA9555_INPUT_0 0x00

/** Configuration port 0. A one makes a line an input. */
#define PCA9555_CONFIG_0 0x06

/**
 * Which key each line carries.
 *
 * Read off the working PE5PVB firmware, which runs on this board, with one
 * correction. That firmware's keypad routine skips line 2, so copying it left
 * the DX key dead. Line 2 is the DX key, confirmed on this radio by watching
 * the raw lines while it was pressed: it pulls line 2 low and no other.
 *
 * Lines 12 to 15 are not wired to anything and are listed as KEYPAD_NONE
 * rather than as a digit. Mapping an unused line to a key would turn a line
 * stuck low into a digit that types itself.
 */
static const int8_t kKeyForLine[16] = {
    2, 3, KEYPAD_DX, 5, 6,           0,           9,           KEYPAD_ENTER,
    8, 7, 4,         1, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE};

static bool sPresent = false;

/** Which key is being held, so one press is reported once. */
static int8_t sHeld = KEYPAD_NONE;

bool keypadBegin(void) {
  sPresent = false;
  sHeld = KEYPAD_NONE;

  pinMode(PIN_KEYPAD_IRQ, INPUT_PULLUP);

  if (!i2cBusTake(I2C_BUS_WAIT_MS)) {
    return false;
  }
  Wire.beginTransmission(I2C_ADDR_KEYPAD);
  Wire.write(PCA9555_CONFIG_0);
  Wire.write(0xFF);
  Wire.write(0xFF);
  sPresent = Wire.endTransmission() == 0;
  i2cBusGive();
  return sPresent;
}

bool keypadPresent(void) {
  return sPresent;
}

bool keypadRawLines(uint16_t *bits) {
  if (!sPresent || bits == NULL) {
    return false;
  }
  /* Held across the write and the read together. The tuner is on this same
   * bus and its reads come back as zeros if this one lands in the middle of
   * them. */
  if (!i2cBusTake(I2C_BUS_WAIT_MS)) {
    return false;
  }
  Wire.beginTransmission(I2C_ADDR_KEYPAD);
  Wire.write(PCA9555_INPUT_0);
  if (Wire.endTransmission() != 0) {
    i2cBusGive();
    return false;
  }
  if (Wire.requestFrom((int)I2C_ADDR_KEYPAD, 2) != 2) {
    i2cBusGive();
    return false;
  }
  uint16_t low = (uint16_t)(Wire.read() & 0xFF);
  uint16_t high = (uint16_t)(Wire.read() & 0xFF);
  i2cBusGive();
  *bits = (uint16_t)(low | (high << 8));
  return true;
}

bool keypadRead(int8_t *key, uint16_t *lines) {
  if (key == NULL) {
    return false;
  }
  uint16_t bits = 0;
  if (!keypadRawLines(&bits)) {
    return false;
  }
  if (lines != NULL) {
    *lines = bits;
  }

  /* A line reads low while its key is held. Count them, because two at once
   * cannot be turned into one answer and guessing would type the wrong
   * digit. */
  int8_t found = KEYPAD_NONE;
  int downCount = 0;
  for (int line = 0; line < 16; line++) {
    if ((bits & (1u << line)) == 0 && kKeyForLine[line] != KEYPAD_NONE) {
      found = kKeyForLine[line];
      downCount++;
    }
  }

  if (downCount != 1) {
    if (downCount == 0) {
      /* Everything is up, so the next press is a new one. */
      sHeld = KEYPAD_NONE;
    }
    *key = KEYPAD_NONE;
    return true;
  }

  if (sHeld == found) {
    /* Still holding the same key. One press is one press. */
    *key = KEYPAD_NONE;
    return true;
  }

  sHeld = found;
  *key = found;
  return true;
}
