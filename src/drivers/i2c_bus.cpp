/**
 * @file i2c_bus.cpp
 * @brief Implementation of the shared I2C bus lock.
 */
#include "i2c_bus.h"

#include "board/board.h"

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static SemaphoreHandle_t sLock = NULL;
static bool sStarted = false;

void i2cBusBegin(uint32_t hz) {
  if (sLock == NULL) {
    /* Recursive, because the tuner's read path takes the bus and then calls
     * its own write path, which takes it again. */
    sLock = xSemaphoreCreateRecursiveMutex();
  }
  if (!sStarted) {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (sLock == NULL) {
      Serial.println(F("[i2c] no lock, the bus cannot be shared safely"));
    }
    sStarted = true;
  }
  Wire.setClock(hz);
}

bool i2cBusTake(uint32_t waitMs) {
  if (sLock == NULL) {
    /* Before the bus is started there is only one caller and nothing to share,
     * so saying yes keeps a driver used during setup from failing in a way
     * that looks like broken hardware.
     *
     * After it is started there must be a lock. If the mutex could not be
     * made, every caller would run unlocked and get exactly the interleaved
     * reads this file exists to stop, and those read as a dead band rather
     * than as an error. Better to refuse. */
    return !sStarted;
  }
  return xSemaphoreTakeRecursive(sLock, pdMS_TO_TICKS(waitMs)) == pdTRUE;
}

void i2cBusGive(void) {
  if (sLock != NULL) {
    xSemaphoreGiveRecursive(sLock);
  }
}
