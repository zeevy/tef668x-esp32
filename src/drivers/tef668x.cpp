/**
 * @file tef668x.cpp
 * @brief Implementation of the tuner driver.
 *
 * The I2C sequences here were learned from the PE5PVB firmware, which runs on
 * this exact board, so they are known to work on this hardware. The code is a
 * rewrite: every write is checked, the units are converted at this boundary,
 * and there are no globals outside this file.
 */
#include "tef668x.h"

#include <Arduino.h>
#include <Wire.h>

#include <string.h>
#include "i2c_bus.h"

#include "board/board.h"
#include "core/signal.h"
#include "tef668x_patch.h"

/** Modules inside the chip. Every command is addressed to one of them. */
typedef enum {
  MODULE_FM = 32,
  MODULE_AM = 33,
  MODULE_AUDIO = 48,
  MODULE_APPL = 64
} Tef668xModule;

/* Commands, within a module. */
#define CMD_TUNE_TO 1              /**< Go to a frequency. FM and AM modules. */
#define CMD_SET_BANDWIDTH 10       /**< Set or auto the channel bandwidth. */
#define CMD_GET_QUALITY_STATUS 128 /**< Level, noise, offset, modulation. */
#define CMD_GET_SIGNAL_STATUS 133  /**< Carries the stereo pilot flag. */
#define CMD_SET_STHIBLEND_LEVEL 72 /**< Stereo and treble blend, by level. */
#define CMD_SET_STHIBLEND_NOISE 73 /**< By noise. */
#define CMD_SET_STHIBLEND_MPH 74   /**< By multipath. */
#define CMD_SET_STHIBLEND_MAX 75   /**< Its ceiling. */
#define CMD_GET_PROCESSING_STATUS \
  134 /**< What the chip is doing to the audio. */

/* Reception and audio shaping. Every one of these is written by the working
 * PE5PVB firmware on every start, and none of them was written here until
 * issue 16. The numbers are its numbers, checked against its source. */
#define CMD_SET_RFAGC 11         /**< Where the RF gain starts backing off. */
#define CMD_SET_ANTENNA 12       /**< AM RF attenuation. */
#define CMD_SET_COCHANNEL 14     /**< AM co-channel rejection. */
#define CMD_SET_NOISE_BLANKER 23 /**< Impulse noise blanker. */
#define CMD_SET_NOISE_BLANKER_AUDIO 24 /**< Its audio side. AM only. */
#define CMD_SET_DEEMPHASIS 31          /**< FM de-emphasis time constant. */
#define CMD_SET_LEVEL_OFFSET 39    /**< Calibration of the reported level. */
#define CMD_SET_SOFTMUTE_MAX 45    /**< How far soft mute may pull the audio. */
#define CMD_SET_HIGHCUT_LEVEL 52   /**< Treble roll off against level. */
#define CMD_SET_HIGHCUT_NOISE 53   /**< Against noise. */
#define CMD_SET_HIGHCUT_MPH 54     /**< Against multipath. */
#define CMD_SET_HIGHCUT_MAX 55     /**< The highest frequency it may pass. */
#define CMD_SET_STEREO_LEVEL 62    /**< Stereo blend against level. */
#define CMD_SET_STEREO_NOISE 63    /**< Against noise. */
#define CMD_SET_STEREO_MPH 64      /**< Against multipath. */
#define CMD_SET_MPH_SUPPRESSION 20 /**< iMS, multipath suppression. */
#define CMD_SET_CHANNEL_EQUALIZER 22 /**< EQ, the channel equalizer. */
#define CMD_SET_STEREO_MIN 66        /**< Forced mono, or stereo allowed. */
#define CMD_SET_BANDWIDTH_OPTIONS 86 /**< How far the adaptive filter opens. */

#define CMD_AUDIO_SET_VOLUME 10  /**< Output gain, in tenths of a dB. */
#define CMD_AUDIO_SET_INPUT 12   /**< Which source the audio path carries. */
#define CMD_AUDIO_SET_WAVEGEN 24 /**< The tone generator. */

/*
 * Audio sources. The tone generator is not heard until the audio path is
 * switched to it: the generator on its own produces a tone that nothing is
 * listening to.
 *
 * 240 selects the generator and 0 goes back to the tuner. Both are the
 * reference firmware's numbers, from a function that switches the input and
 * the generator together every time.
 */
/** The tuner itself, which is what a radio normally carries. */
#define AUDIO_INPUT_TUNER 0
/** The tone generator. */
#define AUDIO_INPUT_WAVEGEN 240
#define CMD_AUDIO_SET_MUTE 11 /**< Mute or unmute the output. */

#define CMD_APPL_SET_OPERATION_MODE 1 /**< Active or standby. */
#define CMD_APPL_GET_OPERATION_STATUS \
  128 /**< Has it booted, and is it patched. */
#define CMD_APPL_GET_IDENTIFICATION \
  130 /**< Which part, and which patch it wants. */

/*
 * Operation mode. Zero is the working state and one is standby, which reads
 * backwards and is worth naming rather than passing as a bare number. The
 * working firmware calls its wrapper power(), and power(1) is what it uses on
 * the way into deep sleep.
 *
 * A chip left in standby acknowledges every write, answers every read, and
 * returns a quality status of 0xFFFA with nonsense in every field. It looks
 * exactly like a working bus and a broken decoder.
 */
#define OPERATION_MODE_ACTIVE 0  /**< Receiving. */
#define OPERATION_MODE_STANDBY 1 /**< Asleep. Every reading is meaningless. */

/*
 * Bandwidth mode. Zero pins it, one lets the chip adapt, which is the
 * opposite way round to how it reads.
 */
#define BANDWIDTH_MODE_FIXED 0    /**< Hold the bandwidth given. */
#define BANDWIDTH_MODE_ADAPTIVE 1 /**< Let the chip choose. */

/** The reference bandwidth that goes with the adaptive mode, in tenths. */
#define BANDWIDTH_ADAPTIVE_REF 3110

/** Tune modes the chip understands. 4 jumps straight there, 1 presets. */
#define TUNE_MODE_PRESET 1 /**< Preset tune, what the AM side wants. */
#define TUNE_MODE_JUMP 4   /**< Go straight there, no search. */

/** The chip takes the patch in chunks of this many bytes under command 0x1b. */
#define PATCH_CHUNK 24

/** How long to wait for the chip to say it is ready, in 5 ms tries. */
#define READY_TRIES 20

/** How long the chip needs after a write before it will answer, in ms. */
#define TUNER_SETTLE_MS 2

/**
 * I2C speed for the tuner.
 *
 * 400 kHz, which is what the working PE5PVB firmware uses on this board:
 * Wire.setClock(400000) in its Tuner_Interface.cpp. This was 100 kHz with a
 * comment claiming that firmware never raised it, which was simply wrong and
 * was never checked.
 *
 * It matters most for the patch, which is six thousand bytes, and for the RDS
 * reads every 43 ms that arrive in a later phase. The 2 ms settle after each
 * write dominates short commands either way.
 */
#define TUNER_I2C_HZ 400000

static bool sReady = false;
static Tef668xCapabilities sCaps;

/* Kept separately from sCaps because these are known before the rest of the
 * capability set is, and they are the first thing worth seeing when start up
 * goes wrong. */
static Tef668xDiagnostics sDiag;
static bool sTuned = false;
static uint32_t sTunedKHz = 0;
static bool sTunedFm = false;
static bool sIdentified = false;
static uint16_t sDeviceWord = 0;
static uint16_t sHardwareWord = 0;
static uint16_t sSoftwareWord = 0;

const char *tef668xErrorText(Tef668xError error) {
  switch (error) {
    case TEF668X_OK:
      return "ok";
    case TEF668X_ERR_NO_DEVICE:
      return "no tuner on the I2C bus";
    case TEF668X_ERR_NOT_READY:
      return "the tuner never became ready";
    case TEF668X_ERR_IDENTIFY:
      return "the tuner would not identify itself";
    case TEF668X_ERR_NO_PATCH:
      return "this build has no patch for that tuner";
    case TEF668X_ERR_UNKNOWN_PART:
      return "unknown tuner part";
    case TEF668X_ERR_WRITE:
      return "the tuner did not accept a write";
    case TEF668X_ERR_READ:
      return "the tuner did not answer a read";
    case TEF668X_ERR_RANGE:
      return "out of range";
    default:
      return "unknown";
  }
}

/* ------------------------------------------------------------- transport -- */

/**
 * Write raw bytes to the tuner.
 *
 * The settle delay is not politeness. The chip needs time after a write
 * before it will answer the next one, and it is what sits between a query's
 * write and its read. Without it the reads come back as zeros that look like
 * a successful transfer.
 */
static Tef668xError writeRaw(const uint8_t *data, size_t len) {
  if (!i2cBusTake(I2C_BUS_WAIT_MS)) {
    return TEF668X_ERR_WRITE;
  }
  Wire.beginTransmission(I2C_ADDR_TUNER);
  if (Wire.write(data, len) != len) {
    Wire.endTransmission();
    i2cBusGive();
    return TEF668X_ERR_WRITE;
  }
  uint8_t result = Wire.endTransmission();
  delay(TUNER_SETTLE_MS);
  i2cBusGive();
  return result == 0 ? TEF668X_OK : TEF668X_ERR_WRITE;
}

/**
 * Send a command with up to four 16 bit arguments.
 *
 * The wire format is the module, the command, a literal 1, then each argument
 * big endian.
 */
static Tef668xError command(Tef668xModule module, uint8_t cmd,
                            const uint16_t *args, size_t argCount) {
  if (argCount > 6) {
    return TEF668X_ERR_RANGE;
  }
  uint8_t buf[3 + 6 * 2];
  buf[0] = (uint8_t)module;
  buf[1] = cmd;
  buf[2] = 1;
  size_t n = 3;
  for (size_t i = 0; i < argCount; i++) {
    buf[n++] = (uint8_t)(args[i] >> 8);
    buf[n++] = (uint8_t)(args[i] & 0xFF);
  }
  return writeRaw(buf, n);
}

/** Ask for a value and read the answer back. */
static Tef668xError query(Tef668xModule module, uint8_t cmd, uint8_t *out,
                          size_t len) {
  /* The bus is held across both halves. A read is a write of what is wanted
   * followed by a read of the answer, and another device getting in between
   * them returns zeros that look like a good transfer. */
  if (!i2cBusTake(I2C_BUS_WAIT_MS)) {
    return TEF668X_ERR_READ;
  }
  uint8_t head[3] = {(uint8_t)module, cmd, 1};
  Tef668xError err = writeRaw(head, sizeof(head));
  if (err != TEF668X_OK) {
    i2cBusGive();
    return err;
  }
  if (Wire.requestFrom((uint8_t)I2C_ADDR_TUNER, (uint8_t)len) != len) {
    i2cBusGive();
    return TEF668X_ERR_READ;
  }
  for (size_t i = 0; i < len; i++) {
    if (!Wire.available()) {
      i2cBusGive();
      return TEF668X_ERR_READ;
    }
    out[i] = (uint8_t)Wire.read();
  }
  i2cBusGive();
  return TEF668X_OK;
}

/** Two bytes off the wire, big endian, as an unsigned word. */
static uint16_t word16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] << 8 | (uint16_t)p[1]);
}

/* ----------------------------------------------------------- bringing up -- */

/**
 * Put the chip back to a known state before the patch goes in.
 *
 * All five bytes. A three byte version is acknowledged just the same and does
 * not reset anything, so the patch then lands on top of whatever state the
 * chip was already in.
 */
static Tef668xError resetChip(void) {
  uint8_t reset[5] = {0x1E, 0x5A, 0x01, 0x5A, 0x5A};
  return writeRaw(reset, sizeof(reset));
}

/** Write one blob in chunks, under the patch write command. */
static Tef668xError writeBlob(const uint8_t *data, size_t len) {
  uint8_t buf[1 + PATCH_CHUNK];
  buf[0] = 0x1B;
  size_t sent = 0;
  while (sent < len) {
    size_t chunk = len - sent;
    if (chunk > PATCH_CHUNK) {
      chunk = PATCH_CHUNK;
    }
    memcpy(buf + 1, data + sent, chunk);
    Tef668xError err = writeRaw(buf, chunk + 1);
    if (err != TEF668X_OK) {
      return err;
    }
    sent += chunk;
  }
  return TEF668X_OK;
}

/** Open a write window on the chip, or close it again with target 0. */
static Tef668xError openWindow(uint8_t target) {
  uint8_t open[3] = {0x1C, 0x00, target};
  return writeRaw(open, sizeof(open));
}

/**
 * Load a patch and its lookup table.
 *
 * The order matters and is the one the working firmware uses: reset, open the
 * patch window, write the patch, close, open the table window, write the
 * table, close.
 */
static Tef668xError loadPatch(const Tef668xPatch *patch) {
  Tef668xError err = resetChip();
  if (err != TEF668X_OK) {
    return err;
  }
  delay(100);

  if ((err = openWindow(0x00)) != TEF668X_OK) {
    return err;
  }
  delay(100);
  if ((err = openWindow(0x74)) != TEF668X_OK) {
    return err;
  }
  if ((err = writeBlob(patch->data, patch->dataLen)) != TEF668X_OK) {
    return err;
  }
  if ((err = openWindow(0x00)) != TEF668X_OK) {
    return err;
  }
  delay(100);
  if ((err = openWindow(0x75)) != TEF668X_OK) {
    return err;
  }
  if ((err = writeBlob(patch->lut, patch->lutLen)) != TEF668X_OK) {
    return err;
  }
  return openWindow(0x00);
}

/** Read the three identification words. */
static Tef668xError identify(uint16_t *device, uint16_t *hardware,
                             uint16_t *software) {
  uint8_t buf[6];
  Tef668xError err =
      query(MODULE_APPL, CMD_APPL_GET_IDENTIFICATION, buf, sizeof(buf));
  if (err != TEF668X_OK) {
    return err;
  }
  *device = word16(buf);
  *hardware = word16(buf + 2);
  *software = word16(buf + 4);
  sDeviceWord = *device;
  sHardwareWord = *hardware;
  sSoftwareWord = *software;
  sIdentified = true;
  return TEF668X_OK;
}

/**
 * Work out which part is fitted from the device word.
 *
 * The low byte names the part. Everything above this layer asks about a
 * capability, never about a part number.
 */
static Tef668xError describePart(uint16_t device, Tef668xCapabilities *caps) {
  switch (device & 0xFF) {
    case 14:
      caps->part = "TEF6686";
      caps->hasStereoImprovement = false;
      caps->hasFullSearchRds = false;
      caps->hasDigitalRadio = false;
      return TEF668X_OK;
    case 1:
      caps->part = "TEF6687";
      caps->hasStereoImprovement = true;
      caps->hasFullSearchRds = true;
      caps->hasDigitalRadio = false;
      return TEF668X_OK;
    case 9:
      caps->part = "TEF6688";
      caps->hasStereoImprovement = false;
      caps->hasFullSearchRds = false;
      caps->hasDigitalRadio = true;
      return TEF668X_OK;
    case 3:
      caps->part = "TEF6689";
      caps->hasStereoImprovement = true;
      caps->hasFullSearchRds = true;
      caps->hasDigitalRadio = true;
      return TEF668X_OK;
    default:
      caps->part = "unknown";
      return TEF668X_ERR_UNKNOWN_PART;
  }
}

/**
 * Tell the chip what its crystal is.
 *
 * The board puts a voltage on an ADC pin that says which crystal is fitted.
 * It is read rather than assumed, because the same firmware runs on boards
 * with different crystals and a wrong clock means everything is tuned wrong.
 */
static Tef668xError setClock(void) {
  /* Big endian words for each crystal, as the chip wants them. */
  static const uint8_t kClock4M[6] = {0x00, 0x3D, 0x09, 0x00, 0x00, 0x00};
  static const uint8_t kClock9M216[6] = {0x00, 0x8C, 0xA0, 0x00, 0x00, 0x00};
  static const uint8_t kClock12M[6] = {0x00, 0xB7, 0x1B, 0x00, 0x00, 0x00};
  static const uint8_t kClock55M[6] = {0x03, 0x4E, 0x5A, 0xAE, 0x00, 0x01};

  /* The windows, in the order the working firmware has them. Getting this
   * wrong does not fail, it tells the chip its reference clock is something
   * it is not, and then every frequency is off and the front end is deaf. */
  int adc = analogRead(PIN_TUNER_XTAL_ADC);
  const uint8_t *clock;
  const char *which;
  if (adc < XTAL_ADC_0V + XTAL_ADC_TOLERANCE) {
    clock = kClock9M216;
    which = "9.216 MHz";
  } else if (adc > XTAL_ADC_1V - XTAL_ADC_TOLERANCE &&
             adc < XTAL_ADC_1V + XTAL_ADC_TOLERANCE) {
    clock = kClock12M;
    which = "12 MHz";
  } else if (adc > XTAL_ADC_2V - XTAL_ADC_TOLERANCE &&
             adc < XTAL_ADC_2V + XTAL_ADC_TOLERANCE) {
    clock = kClock55M;
    which = "55 MHz";
  } else {
    clock = kClock4M;
    which = "4 MHz";
  }
  sDiag.xtalAdc = (uint16_t)adc;
  sDiag.xtal = which;
  Serial.printf("[tuner] crystal sense %d, using %s\n", adc, which);

  uint8_t activate[3] = {0x14, 0x00, 0x01};
  Tef668xError err = writeRaw(activate, sizeof(activate));
  if (err != TEF668X_OK) {
    return err;
  }
  delay(50);

  uint8_t set[9] = {0x40, 0x04, 0x01};
  memcpy(set + 3, clock, 6);
  if ((err = writeRaw(set, sizeof(set))) != TEF668X_OK) {
    return err;
  }

  uint8_t enable[5] = {0x40, 0x05, 0x01, 0x00, 0x01};
  if ((err = writeRaw(enable, sizeof(enable))) != TEF668X_OK) {
    return err;
  }
  delay(100);
  return TEF668X_OK;
}

/*
 * Register defaults written once the chip is powered on. Each record is a
 * length byte followed by that many bytes, and a zero length ends the table.
 * From the working firmware's init table, which is device configuration data
 * rather than code.
 */
static const uint8_t kInitTable[] = {
    17,   0x20, 0x26, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFC, 0xFF, 0xF8, 0x00, 0x00, 9,    0x20, 0x36, 0x01, 0x00, 0x00,
    0x01, 0x68, 0x01, 0x2C, 7,    0x20, 0x37, 0x01, 0x00, 0x00, 0x0F, 0xA0,
    7,    0x20, 0x39, 0x01, 0x00, 0x01, 0x00, 0x01, 7,    0x20, 0x3A, 0x01,
    0x00, 0x01, 0x00, 0x01, 11,   0x20, 0x3C, 0x01, 0x00, 0x3C, 0x00, 0x78,
    0x00, 0x64, 0x00, 0xC8, 11,   0x20, 0x46, 0x01, 0x01, 0xF4, 0x07, 0xD0,
    0x00, 0xC8, 0x00, 0xC8, 9,    0x20, 0x48, 0x01, 0x00, 0x00, 0x02, 0x58,
    0x00, 0xF0, 9,    0x20, 0x49, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x8C,
    9,    0x20, 0x4A, 0x01, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x8C, 7,    0x20,
    0x4B, 0x01, 0x00, 0x00, 0x0F, 0xA0, 7,    0x30, 0x15, 0x01, 0x00, 0x80,
    0x00, 0x01, 13,   0x30, 0x16, 0x01, 0x00, 0x21, 0x00, 0x02, 0x00, 0x10,
    0x01, 0x00, 0x12, 0xC0, 7,    0x30, 0x0D, 0x01, 0x00, 0x80, 0x00, 0xE0,
    0,
};

/** Write the register defaults. */
static Tef668xError writeInitTable(void) {
  size_t i = 0;
  while (kInitTable[i] != 0) {
    uint8_t len = kInitTable[i];
    Tef668xError err = writeRaw(&kInitTable[i + 1], len);
    if (err != TEF668X_OK) {
      return err;
    }
    i += (size_t)len + 1;
  }
  return TEF668X_OK;
}

/** Ask whether the chip has finished booting. */
static Tef668xError readBootStatus(uint8_t *status) {
  uint8_t buf[2];
  Tef668xError err =
      query(MODULE_APPL, CMD_APPL_GET_OPERATION_STATUS, buf, sizeof(buf));
  if (err != TEF668X_OK) {
    return err;
  }
  *status = buf[1];
  return TEF668X_OK;
}

/**
 * Patch the chip, tell it about its crystal, and power it on.
 *
 * All three, in that order, before it will answer anything. Loading the patch
 * alone leaves a chip that acknowledges every write and returns zeros to
 * every read, which looks like a working bus and a dead radio.
 */
static Tef668xError bringUpWithPatch(const Tef668xPatch *patch) {
  Tef668xError err = loadPatch(patch);
  if (err != TEF668X_OK) {
    return err;
  }
  delay(50);
  if ((err = setClock()) != TEF668X_OK) {
    return err;
  }
  /* Standby first, which is the state the working firmware leaves the chip in
   * at the end of its own init. It is made active once the register defaults
   * are in. */
  uint16_t mode = OPERATION_MODE_STANDBY;
  if ((err = command(MODULE_APPL, CMD_APPL_SET_OPERATION_MODE, &mode, 1)) !=
      TEF668X_OK) {
    return err;
  }
  delay(50);
  return TEF668X_OK;
}

/**
 * Tell the tuner how to receive and how to sound.
 *
 * Without these the chip keeps whatever it powers up with after the patch,
 * and medium wave is hissy even on a station reading a strong 41 dBuV.
 * Measured on 738 kHz with and without: ultrasonic noise 1025 against 138,
 * and hissy against clear.
 *
 * The AM half is what did that. The FM writes below are all the reference
 * firmware's defaults, and most of those defaults are "off", so they change
 * little on their own. They are here so the chip is in a known state rather
 * than an unknown one, which is the point: the previous behaviour was not
 * "the defaults", it was whatever the patch happened to leave behind.
 *
 * Every value here is the one the working PE5PVB firmware writes, read out of
 * its source rather than guessed: `TEF6686_ESP32.ino` lines 976 to 996 for the
 * calls, its factory defaults around lines 4950 to 5025 for the values, and
 * `src/Tuner_Drv_Lithio.cpp` for how each one goes on the wire. That firmware
 * has been listened to on this radio for a long time, which is the only
 * evidence available for numbers like these. CLAUDE.md forbids guessed
 * thresholds and these are thresholds.
 *
 * These are fixed defaults. They become settings in issue 14.
 */
static Tef668xError applyReceptionDefaults(void) {
  Tef668xError err;
  /* Sized to what command() accepts rather than to what this function happens
   * to pass, so adding a longer command here cannot read past the end. */
  uint16_t args[6];

  /* Level offset, and read the note before changing it.
   *
   * The old firmware's offset setting defaults to 0 and it still writes -70,
   * which is -7.0 dB. That is not a preference, it is a calibration of what
   * the chip reports, and everything downstream of it is on that scale: every
   * threshold in that firmware, and the captures in test/fixtures/agc/ which
   * were taken from this radio while it was running.
   *
   * So this writes -70 as well. Not because -7.0 dB is known to be right in
   * absolute terms, which cannot be established without a signal generator,
   * but because half the numbers this project will rely on are already on
   * that scale and two scales is how thresholds end up 7 dB out. */
  args[0] = (uint16_t)(int16_t)-70;
  if ((err = command(MODULE_FM, CMD_SET_LEVEL_OFFSET, args, 1)) != TEF668X_OK) {
    return err;
  }
  if ((err = command(MODULE_AM, CMD_SET_LEVEL_OFFSET, args, 1)) != TEF668X_OK) {
    return err;
  }

  /* FM de-emphasis. 500 is 50 us, which is the standard everywhere except the
   * Americas, where it is 75 us and this value would be 750. Wrong either way
   * is not subtle: everything sounds dull or everything sounds shrill. */
  args[0] = 500;
  if ((err = command(MODULE_FM, CMD_SET_DEEMPHASIS, args, 1)) != TEF668X_OK) {
    return err;
  }

  /* Where the RF gain starts to back off, in tenths of a dBuV. */
  args[0] = 920;
  args[1] = 0;
  if ((err = command(MODULE_FM, CMD_SET_RFAGC, args, 2)) != TEF668X_OK) {
    return err;
  }
  args[0] = 1000;
  if ((err = command(MODULE_AM, CMD_SET_RFAGC, args, 1)) != TEF668X_OK) {
    return err;
  }

  /* Soft mute pulls the audio down as the signal falls. Off on FM, on for AM,
   * which is where it earns its keep. The second word is fixed in the old
   * firmware at 200 for FM and 250 for AM. */
  args[0] = 0;
  args[1] = 200;
  if ((err = command(MODULE_FM, CMD_SET_SOFTMUTE_MAX, args, 2)) != TEF668X_OK) {
    return err;
  }
  args[0] = 1;
  args[1] = 250;
  if ((err = command(MODULE_AM, CMD_SET_SOFTMUTE_MAX, args, 2)) != TEF668X_OK) {
    return err;
  }

  /* Noise blankers, both off by default. Mode 0 with a start of 1000 is how
   * the reference firmware spells off. The AM side takes two writes, not one.
   *
   * The second AM write always carries 1000, whatever the first one carries.
   * That is not obvious from these two lines looking alike: when the blanker
   * becomes a setting and the first write takes the real start, the second
   * still has to send 1000. */
  args[0] = 0;
  args[1] = 1000;
  if ((err = command(MODULE_FM, CMD_SET_NOISE_BLANKER, args, 2)) !=
      TEF668X_OK) {
    return err;
  }
  if ((err = command(MODULE_AM, CMD_SET_NOISE_BLANKER, args, 2)) !=
      TEF668X_OK) {
    return err;
  }
  if ((err = command(MODULE_AM, CMD_SET_NOISE_BLANKER_AUDIO, args, 2)) !=
      TEF668X_OK) {
    return err;
  }

  /* AM co-channel rejection, on, starting at 100.0 dBuV with a count of 3.
   *
   * This one and the attenuation below are the two the reference firmware
   * re-sends on every change to an AM band, not just at start up. Written
   * once here. That is the same thing only while they are fixed defaults, so
   * it has to be revisited in issue 14 when they become settings. */
  args[0] = 1;
  args[1] = 2;
  args[2] = 1000;
  args[3] = 3;
  if ((err = command(MODULE_AM, CMD_SET_COCHANNEL, args, 4)) != TEF668X_OK) {
    return err;
  }

  /* No AM RF attenuation. */
  args[0] = 0;
  if ((err = command(MODULE_AM, CMD_SET_ANTENNA, args, 1)) != TEF668X_OK) {
    return err;
  }

  /* Stereo blend against level, noise and multipath. Three writes.
   *
   * Mode 0 is off. In the reference firmware mode 3 is on and mode 0 is off,
   * and its blend setting defaults to 0, so this turns the blend off, exactly
   * as it ships. It does not make a weak station go to mono: that is what the
   * mode word decides, not the levels. The start and slope words are written
   * anyway, so turning it on later is a change of mode and nothing else. */
  args[0] = 0;
  args[1] = 0;
  args[2] = 60;
  if ((err = command(MODULE_FM, CMD_SET_STEREO_LEVEL, args, 3)) != TEF668X_OK) {
    return err;
  }
  args[1] = 240;
  args[2] = 200;
  if ((err = command(MODULE_FM, CMD_SET_STEREO_NOISE, args, 3)) != TEF668X_OK) {
    return err;
  }
  if ((err = command(MODULE_FM, CMD_SET_STEREO_MPH, args, 3)) != TEF668X_OK) {
    return err;
  }

  /* Treble roll off against level, noise and multipath. Same again: mode 0 is
   * off, and the reference firmware ships with it off.
   *
   * The write after these three is the one that does something. It sets the
   * ceiling on what the audio may pass at 7 kHz, and mode 1 there is on. */
  args[0] = 0;
  args[1] = 0;
  args[2] = 300;
  if ((err = command(MODULE_FM, CMD_SET_HIGHCUT_LEVEL, args, 3)) !=
      TEF668X_OK) {
    return err;
  }
  args[1] = 360;
  args[2] = 300;
  if ((err = command(MODULE_FM, CMD_SET_HIGHCUT_NOISE, args, 3)) !=
      TEF668X_OK) {
    return err;
  }
  if ((err = command(MODULE_FM, CMD_SET_HIGHCUT_MPH, args, 3)) != TEF668X_OK) {
    return err;
  }
  args[0] = 1;
  args[1] = 7000;
  return command(MODULE_FM, CMD_SET_HIGHCUT_MAX, args, 2);
}

Tef668xError tef668xBegin(void) {
  /* Forget anything a previous call worked out. Without this a second call
   * that fails part way leaves the old capability set in place, and the web
   * page keeps reporting a working tuner that is not there. */
  sReady = false;
  sIdentified = false;
  sTuned = false;
  sTunedKHz = 0;
  sTunedFm = false;
  memset(&sCaps, 0, sizeof(sCaps));

  i2cBusBegin(TUNER_I2C_HZ);

  /* Is anything there at all? Worth separating from a chip that is there but
   * misbehaving, because the two mean different things to whoever is
   * holding the radio. */
  memset(&sDiag, 0, sizeof(sDiag));

  if (!i2cBusTake(I2C_BUS_WAIT_MS)) {
    return TEF668X_ERR_NO_DEVICE;
  }
  Wire.beginTransmission(I2C_ADDR_TUNER);
  uint8_t probe = Wire.endTransmission();
  i2cBusGive();
  if (probe != 0) {
    return TEF668X_ERR_NO_DEVICE;
  }
  sDiag.sawDevice = true;

  uint8_t boot = 0;
  bool ready = false;
  for (int i = 0; i < READY_TRIES; i++) {
    if (readBootStatus(&boot) == TEF668X_OK) {
      ready = true;
      break;
    }
    delay(5);
  }
  if (!ready) {
    return TEF668X_ERR_NOT_READY;
  }
  sDiag.readBootStatus = true;
  sDiag.bootStatus = boot;

  /* Start up is unconditional, even when the chip says it is already patched.
   *
   * The firmware this replaces only ever starts from a power cycle, so it can
   * skip the patch when the chip reports itself ready. This radio updates
   * itself over the air: the ESP32 reboots and the tuner does not, so it comes
   * up still patched and still holding whatever clock and register settings
   * the previous firmware gave it. Skipping on that basis means a firmware
   * update cannot change anything about the tuner until someone power cycles
   * the radio, which is the exact situation this project exists to avoid.
   *
   * It costs about a second at every boot. That is worth paying to make an
   * update mean what it says. */
  (void)boot;
  Tef668xError err;
  {
    /* The version words only read correctly once some patch is in, so load
     * the older one, ask, and load again if it wanted the other. */
    const Tef668xPatch *first = tef668xPatchFor(102);
    if (first == NULL) {
      return TEF668X_ERR_NO_PATCH;
    }
    if ((err = bringUpWithPatch(first)) != TEF668X_OK) {
      return err;
    }
    sDiag.patchLoaded = true;
    sDiag.patchTried = 102;

    /* Only now can it say what it is. The version words tell us whether the
     * patch just loaded was the one it wanted. */
    uint16_t device = 0;
    uint16_t hardware = 0;
    uint16_t software = 0;
    if ((err = identify(&device, &hardware, &software)) != TEF668X_OK) {
      return TEF668X_ERR_IDENTIFY;
    }

    uint16_t wanted = (uint16_t)((hardware >> 8) * 100 + (software >> 8));
    sDiag.patchWanted = wanted;
    if (wanted != 102) {
      const Tef668xPatch *right = tef668xPatchFor(wanted);
      if (right == NULL) {
        return TEF668X_ERR_NO_PATCH;
      }
      if ((err = bringUpWithPatch(right)) != TEF668X_OK) {
        return err;
      }
      sDiag.patchTried = wanted;
    }

    if ((err = writeInitTable()) != TEF668X_OK) {
      return err;
    }
  }

  /* Now make it receive. Without this every reading comes back invalid. */
  if ((err = tef668xSetActive(true)) != TEF668X_OK) {
    return err;
  }

  /* And tell it how to receive. Skipped at first, which left the chip on its
   * power up defaults and medium wave hissing on a strong station.
   *
   * A failure here is reported and not fatal. Everything above this point is
   * something the tuner cannot work without, and it has all succeeded: the
   * patch went in, the init table went in, and the chip is active. These are
   * quality writes. Losing one makes the radio sound wrong, and refusing to
   * start would turn that into no radio at all, which is worse and is not
   * what the caller can do anything about. */
  Tef668xError quality = applyReceptionDefaults();
  if (quality != TEF668X_OK) {
    Serial.printf("[tuner] reception defaults failed: %s\n",
                  tef668xErrorText(quality));
  }

  /* Quiet until somebody tunes it.
   *
   * Making the chip active starts it receiving on whatever the wake up tune
   * left it on, which is nowhere in particular, and it plays that. On the
   * radio it was a burst of FM noise at switch on, ending when the first real
   * tune arrived a moment later. The radio task's first push sets the mute to
   * what the settings say, so this only covers the gap. */
  Tef668xError quiet = tef668xSetMute(true);
  if (quiet != TEF668X_OK) {
    Serial.printf("[tuner] could not mute at start up: %s\n",
                  tef668xErrorText(quiet));
  }

  uint16_t device = 0;
  uint16_t hardware = 0;
  uint16_t software = 0;
  if ((err = identify(&device, &hardware, &software)) != TEF668X_OK) {
    return TEF668X_ERR_IDENTIFY;
  }

  sCaps.deviceWord = device;
  sCaps.hardwareWord = hardware;
  sCaps.softwareWord = software;
  sCaps.patchVersion = (uint16_t)((hardware >> 8) * 100 + (software >> 8));
  if ((err = describePart(device, &sCaps)) != TEF668X_OK) {
    return err;
  }

  sReady = true;
  return TEF668X_OK;
}

bool tef668xCurrentTune(uint32_t *freqKHz, bool *isFm) {
  if (!sTuned) {
    return false;
  }
  if (freqKHz != NULL) {
    *freqKHz = sTunedKHz;
  }
  if (isFm != NULL) {
    *isFm = sTunedFm;
  }
  return true;
}

const Tef668xDiagnostics *tef668xDiagnostics(void) {
  return &sDiag;
}

const Tef668xCapabilities *tef668xCapabilities(void) {
  return sReady ? &sCaps : NULL;
}

bool tef668xLastIdentification(uint16_t *device, uint16_t *hardware,
                               uint16_t *software) {
  if (!sIdentified) {
    return false;
  }
  if (device != NULL) {
    *device = sDeviceWord;
  }
  if (hardware != NULL) {
    *hardware = sHardwareWord;
  }
  if (software != NULL) {
    *software = sSoftwareWord;
  }
  return true;
}

/* -------------------------------------------------------------- tuning --- */

Tef668xError tef668xSetActive(bool active) {
  uint16_t mode = active ? OPERATION_MODE_ACTIVE : OPERATION_MODE_STANDBY;
  Tef668xError err =
      command(MODULE_APPL, CMD_APPL_SET_OPERATION_MODE, &mode, 1);
  if (err != TEF668X_OK) {
    return err;
  }

  if (active) {
    /* Setting the mode is only half of it. The chip also needs an FM preset
     * tune before its FM side comes back, and without this a radio that has
     * been on AM stays on AM: every FM station then reads the same fixed
     * nonsense, which looks like a broken decoder rather than a chip sitting
     * on the wrong front end. 100.00 MHz is what the working firmware uses,
     * and it is replaced by the real tune a moment later. */
    uint16_t wake[2] = {TUNE_MODE_PRESET, 10000};
    if ((err = command(MODULE_FM, CMD_TUNE_TO, wake, 2)) != TEF668X_OK) {
      return err;
    }
  }

  /* This call moved the chip, so the record of where it is has to move with
   * it. Leaving it stale is worse than having none: the next tune asks this
   * record whether the side needs switching, believes it, and skips the wake
   * the chip actually needed. */
  sTuned = false;
  sTunedKHz = 0;
  sTunedFm = active;

  delay(50);
  return TEF668X_OK;
}

/**
 * Put the chip on the right side before tuning across a band change.
 *
 * Tuning the FM module while the chip is on its AM side does not move it
 * back. The FM quality registers then return the same fixed nonsense a
 * standby chip returns, so every FM station reads identically and looks
 * broken. The working firmware re-activates before every band change, which
 * is what this does.
 */
static Tef668xError switchSideIfNeeded(bool wantFm) {
  if (sTuned && sTunedFm == wantFm) {
    return TEF668X_OK;
  }
  return tef668xSetActive(true);
}

Tef668xError tef668xTuneFm(uint32_t freqKHz) {
  /* The chip counts FM in tens of kilohertz, so 104000 kHz goes as 10400. */
  if (freqKHz < 65000 || freqKHz > 108000 || (freqKHz % 10) != 0) {
    return TEF668X_ERR_RANGE;
  }
  Tef668xError err = switchSideIfNeeded(true);
  if (err != TEF668X_OK) {
    return err;
  }
  uint16_t args[2] = {TUNE_MODE_JUMP, (uint16_t)(freqKHz / 10)};
  err = command(MODULE_FM, CMD_TUNE_TO, args, 2);
  if (err == TEF668X_OK) {
    sTuned = true;
    sTunedKHz = freqKHz;
    sTunedFm = true;
  }
  return err;
}

Tef668xError tef668xTuneAm(uint32_t freqKHz) {
  if (freqKHz < 100 || freqKHz > 30000) {
    return TEF668X_ERR_RANGE;
  }
  Tef668xError err = switchSideIfNeeded(false);
  if (err != TEF668X_OK) {
    return err;
  }
  uint16_t args[2] = {TUNE_MODE_PRESET, (uint16_t)freqKHz};
  err = command(MODULE_AM, CMD_TUNE_TO, args, 2);
  if (err == TEF668X_OK) {
    sTuned = true;
    sTunedKHz = freqKHz;
    sTunedFm = false;
  }
  return err;
}

Tef668xError tef668xSetFmBandwidth(uint16_t bandwidthKHz) {
  /* Mode 0 pins the bandwidth, mode 1 lets the tuner adapt. It reads the
   * wrong way round, and sending mode 0 with a bandwidth of zero pins it at
   * zero: the chip then reports its narrowest bandwidth, never finds a
   * stereo pilot, and looks exactly like a radio with no aerial.
   *
   * The 3110 that goes with the adaptive mode is the value the working
   * firmware uses. The two trailing words are its attack and decay times. */
  if (bandwidthKHz == 0) {
    uint16_t adaptive[4] = {BANDWIDTH_MODE_ADAPTIVE, BANDWIDTH_ADAPTIVE_REF,
                            1000, 1000};
    return command(MODULE_FM, CMD_SET_BANDWIDTH, adaptive, 4);
  }
  if (bandwidthKHz > 6000) {
    return TEF668X_ERR_RANGE;
  }
  uint16_t args[4] = {BANDWIDTH_MODE_FIXED, (uint16_t)(bandwidthKHz * 10), 1000,
                      1000};
  return command(MODULE_FM, CMD_SET_BANDWIDTH, args, 4);
}

Tef668xError tef668xSetAmBandwidth(uint16_t bandwidthKHz) {
  /* The working firmware only ever pins the AM bandwidth, so there is no
   * adaptive value to copy and none is invented here. A caller has to say
   * what it wants. */
  if (bandwidthKHz == 0 || bandwidthKHz > 6000) {
    return TEF668X_ERR_RANGE;
  }
  uint16_t args[2] = {BANDWIDTH_MODE_FIXED, (uint16_t)(bandwidthKHz * 10)};
  return command(MODULE_AM, CMD_SET_BANDWIDTH, args, 2);
}

Tef668xError tef668xSetVolume(int8_t decibels) {
  if (decibels < -60 || decibels > 24) {
    return TEF668X_ERR_RANGE;
  }
  /* The chip takes tenths of a decibel. */
  uint16_t arg = (uint16_t)((int16_t)decibels * 10);
  return command(MODULE_AUDIO, CMD_AUDIO_SET_VOLUME, &arg, 1);
}

Tef668xError tef668xSetMute(bool muted) {
  uint16_t arg = muted ? 1 : 0;
  return command(MODULE_AUDIO, CMD_AUDIO_SET_MUTE, &arg, 1);
}

Tef668xError tef668xReadQualityRaw(bool fm, uint8_t out[14]) {
  return query(fm ? MODULE_FM : MODULE_AM, CMD_GET_QUALITY_STATUS, out, 14);
}

Tef668xError tef668xSetAmNoiseBlanker(uint8_t startPercent) {
  /* Mode 0 with a start of 1000 is how the reference spells off. On, it
   * carries the start in tenths. Two commands, not one: the AM side has a
   * second write for the audio path, and that one always carries 1000. */
  uint16_t args[2];
  args[0] = (uint16_t)(startPercent == 0 ? 0 : 1);
  args[1] = (uint16_t)(startPercent == 0 ? 1000 : startPercent * 10);
  Tef668xError err = command(MODULE_AM, CMD_SET_NOISE_BLANKER, args, 2);

  args[1] = 1000;
  Tef668xError audio = command(MODULE_AM, CMD_SET_NOISE_BLANKER_AUDIO, args, 2);
  return err != TEF668X_OK ? err : audio;
}

Tef668xError tef668xSetFmNoiseBlanker(uint8_t startPercent) {
  uint16_t args[2];
  args[0] = (uint16_t)(startPercent == 0 ? 0 : 1);
  args[1] = (uint16_t)(startPercent == 0 ? 1000 : startPercent * 10);
  return command(MODULE_FM, CMD_SET_NOISE_BLANKER, args, 2);
}

Tef668xError tef668xSetMultipathSuppression(bool on) {
  uint16_t args[1] = {(uint16_t)(on ? 1 : 0)};
  return command(MODULE_FM, CMD_SET_MPH_SUPPRESSION, args, 1);
}

Tef668xError tef668xSetChannelEqualizer(bool on) {
  uint16_t args[1] = {(uint16_t)(on ? 1 : 0)};
  return command(MODULE_FM, CMD_SET_CHANNEL_EQUALIZER, args, 1);
}

Tef668xError tef668xSetBandwidthExtension(bool wide) {
  /* 400 lets it open, 950 holds it back. Not a width in kHz and not a flag:
   * they are the two values the reference firmware writes, and it writes one
   * or the other and nothing between. */
  uint16_t args[1] = {(uint16_t)(wide ? 400 : 950)};
  return command(MODULE_FM, CMD_SET_BANDWIDTH_OPTIONS, args, 1);
}

Tef668xError tef668xSetDeemphasis(uint16_t microseconds) {
  /* The chip takes tenths of a microsecond, so 50 us is 500. Only the two
   * real standards and off are accepted: anything else is a number somebody
   * guessed, and the chip would take it and quietly sound wrong. */
  if (microseconds != 0 && microseconds != 50 && microseconds != 75) {
    return TEF668X_ERR_RANGE;
  }
  uint16_t args[1] = {(uint16_t)(microseconds * 10)};
  return command(MODULE_FM, CMD_SET_DEEMPHASIS, args, 1);
}

Tef668xError tef668xTone(bool on, int16_t amplitude, uint16_t freqHz,
                         uint16_t freqHz2) {
  /* The source is switched with the generator, every time. The generator
   * alone makes a tone that nothing carries, which is silence.
   *
   * Order matters. Going on, the source is selected first so the tone is
   * already there when it starts. Coming off, the source goes back first, so
   * the station returns before the generator stops rather than leaving a gap
   * of nothing in between. */
  uint16_t source[1] = {
      (uint16_t)(on ? AUDIO_INPUT_WAVEGEN : AUDIO_INPUT_TUNER)};
  Tef668xError err = TEF668X_OK;
  if (on) {
    err = command(MODULE_AUDIO, CMD_AUDIO_SET_INPUT, source, 1);
  }

  /* Six words. The first is the mode, 5 for a tone and 0 for off, the second
   * is unused, and then an amplitude and a frequency for each of the two
   * output channels.
   *
   * Channels, not two generators: measured on this radio on 13 September
   * 2026 by putting different frequencies in the two slots. What comes out
   * follows slot one and slot two is inaudible, because the amplifier is
   * mono. See HARDWARE.md. So two tones at once cannot be heard here.
   *
   * Mode 5 and the word order are the reference firmware's, which runs this
   * generator on this chip. */
  uint16_t args[6] = {0, 0, 0, 0, 0, 0};
  if (on) {
    args[0] = 5;
    args[2] = (uint16_t)amplitude;
    args[3] = freqHz;
    args[4] = (uint16_t)amplitude;
    args[5] = freqHz2;
  }
  Tef668xError tone = command(MODULE_AUDIO, CMD_AUDIO_SET_WAVEGEN, args, 6);
  if (err == TEF668X_OK) {
    err = tone;
  }

  if (!on) {
    /* Always attempted, whatever the tone write did. A radio left with its
     * audio path pointing at a silent generator is a radio that has gone
     * dead, and it would not be obvious why. */
    Tef668xError back = command(MODULE_AUDIO, CMD_AUDIO_SET_INPUT, source, 1);
    if (err == TEF668X_OK) {
      err = back;
    }
  }
  return err;
}

Tef668xError tef668xSetMono(bool mono) {
  /* Mode 2 forces mono and mode 0 allows stereo. The second word is 400 in
   * both cases in the reference firmware. */
  uint16_t args[2] = {(uint16_t)(mono ? 2 : 0), 400};
  return command(MODULE_FM, CMD_SET_STEREO_MIN, args, 2);
}

/**
 * One of the three level, noise and multipath triples.
 *
 * Each is written the same way: mode 0 switches it off and mode 3 turns it on
 * starting at the given level. The noise and multipath pair always carry the
 * reference firmware's fixed figures, so only the level start varies.
 */
static Tef668xError writeBlend(uint8_t levelCmd, uint8_t noiseCmd,
                               uint8_t mphCmd, uint8_t start,
                               uint16_t levelSlope, uint16_t pairSlope,
                               uint16_t pairStart) {
  uint16_t mode = start == 0 ? 0 : 3;
  uint16_t args[3];

  args[0] = mode;
  args[1] = (uint16_t)(start * 10);
  args[2] = levelSlope;
  Tef668xError err = command(MODULE_FM, levelCmd, args, 3);

  /* The noise and multipath pair can take a different slope from the level
   * one. On the stereo blend they do: 60 against level and 200 against the
   * other two. Using one slope for all three made the blend ramp against
   * noise at a third of the rate it should. */
  args[1] = pairStart;
  args[2] = pairSlope;
  Tef668xError noise = command(MODULE_FM, noiseCmd, args, 3);
  Tef668xError mph = command(MODULE_FM, mphCmd, args, 3);

  if (err != TEF668X_OK) {
    return err;
  }
  return noise != TEF668X_OK ? noise : mph;
}

Tef668xError tef668xSetWeakSignal(uint8_t highCutStart, uint8_t stereoStart,
                                  uint8_t stHiBlendStart) {
  /* The slopes and the noise and multipath starts are the reference's, which
   * uses the same numbers whichever way its own settings are set. Only the
   * level start and the mode change.
   *
   * The stereo blend is the one that does not use a single slope: 60 against
   * level and 200 against noise and multipath. */
  Tef668xError cut =
      writeBlend(CMD_SET_HIGHCUT_LEVEL, CMD_SET_HIGHCUT_NOISE,
                 CMD_SET_HIGHCUT_MPH, highCutStart, 300, 300, 360);
  Tef668xError stereo =
      writeBlend(CMD_SET_STEREO_LEVEL, CMD_SET_STEREO_NOISE, CMD_SET_STEREO_MPH,
                 stereoStart, 60, 200, 240);
  Tef668xError both =
      writeBlend(CMD_SET_STHIBLEND_LEVEL, CMD_SET_STHIBLEND_NOISE,
                 CMD_SET_STHIBLEND_MPH, stHiBlendStart, 300, 300, 360);

  /* The stereo high blend also needs a ceiling, the same way the high cut
   * does. Without it the three writes above land on a mechanism whose limit
   * is nothing and it does nothing at all, which is exactly what the radio
   * showed: the other two moved and this one stayed at zero. 7 kHz, matching
   * the high cut ceiling. */
  uint16_t ceiling[2] = {1, 7000};
  Tef668xError max = command(MODULE_FM, CMD_SET_STHIBLEND_MAX, ceiling, 2);
  if (both == TEF668X_OK) {
    both = max;
  }

  if (cut != TEF668X_OK) {
    return cut;
  }
  return stereo != TEF668X_OK ? stereo : both;
}

Tef668xError tef668xReadProcessing(Tef668xProcessing *out) {
  if (out == NULL) {
    return TEF668X_ERR_RANGE;
  }
  uint8_t buf[12];
  Tef668xError err =
      query(MODULE_FM, CMD_GET_PROCESSING_STATUS, buf, sizeof(buf));
  if (err != TEF668X_OK) {
    return err;
  }
  /* The first word is a status the reference discards. The three after it are
   * divided by ten, which is what the reference does. What the result counts
   * is not documented anywhere public. The last two words are the stereo band
   * blend, which belongs to a feature this part does not have. */
  out->highCut = (uint16_t)(word16(buf + 2) / 10);
  out->stereo = (uint16_t)(word16(buf + 4) / 10);
  out->stHiBlend = (uint16_t)(word16(buf + 6) / 10);
  return TEF668X_OK;
}

Tef668xError tef668xReadQuality(bool fm, Tef668xQuality *quality) {
  if (quality == NULL) {
    return TEF668X_ERR_RANGE;
  }
  uint8_t buf[14];
  Tef668xError err = query(fm ? MODULE_FM : MODULE_AM, CMD_GET_QUALITY_STATUS,
                           buf, sizeof(buf));
  if (err != TEF668X_OK) {
    return err;
  }

  /* The quality status word says nothing about stereo. The pilot flag lives
   * in a different command, and reading bit 15 of the wrong word gives an
   * answer that looks plausible and is always wrong. */
  (void)word16(buf);

  /* Level, offset and modulation are signed quantities that the chip returns
   * in unsigned words. Reading them as unsigned is what turns a small
   * negative into a number in the thousands. */
  quality->levelDbuVTenths = (int16_t)word16(buf + 2);
  quality->offsetKHzTenths = (int16_t)word16(buf + 8);
  quality->bandwidthKHz = (uint16_t)(word16(buf + 10) / 10);
  quality->modulationPercent = (int16_t)(((int16_t)word16(buf + 12)) / 10);

  if (fm) {
    quality->usnTenths = word16(buf + 4);
    quality->multipathTenths = word16(buf + 6);
    quality->coChannelTenths = 0;
    uint8_t signal[2];
    quality->stereo = false;
    if (query(MODULE_FM, CMD_GET_SIGNAL_STATUS, signal, sizeof(signal)) ==
        TEF668X_OK) {
      quality->stereo = (word16(signal) & 0x8000) != 0;
    }
  } else {
    quality->usnTenths = word16(buf + 4);
    quality->multipathTenths = 0;
    quality->coChannelTenths = word16(buf + 6);
    quality->stereo = false; /* AM has no stereo on this part. */
  }
  /* Worked out, not read. The chip does not report it, and the two sides put
   * their noise on different scales, so which one this came from matters. */
  quality->snrDb =
      signalSnrDb(quality->levelDbuVTenths, quality->usnTenths, fm);

  return TEF668X_OK;
}
