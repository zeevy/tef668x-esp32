/* Implementation of the ILI9341 panel driver. */
#include "display.h"

#include "board/board.h"

#include <Arduino.h>
#include <SPI.h>
#include <driver/gpio.h>

/*
 * How fast the panel is clocked, in hertz.
 *
 * The panel redraws its glass every 12.7 ms, and nothing on this board tells
 * the firmware where that redraw is. A push that takes longer than one redraw
 * is always caught halfway, so the glass shows part of the old picture and
 * part of the new one. At 7.5 MHz, which the ESP32 runs as 7.27, one step of
 * the scrolling radio text, 296 by 18 pixels, took 13.1 ms on this radio.
 *
 * 40 MHz is the ESP32's 80 MHz bus divided by two, and its harmonics are 80
 * and 120 MHz, so none falls inside the FM band, where 7.27 MHz puts one
 * every 7.27 MHz. A panel clocked too fast shows torn or speckled pixels
 * rather than failing, so a change to this rate has to be looked at on the
 * glass before it is trusted.
 */
#define DISPLAY_SPI_HZ 40000000

/*
 * How the panel is turned. 1 and 3 are the two landscape views.
 *
 * Settled by looking at the radio, which is the only way. The wrong values
 * are not subtly wrong: they give a mirror image, a mirror image the other
 * way, or the right image upside down.
 */
#define DISPLAY_ROTATION 1

/* ILI9341 commands, only the ones used here. */
#define ILI9341_SWRESET 0x01 /* Reset the panel in software. */
#define ILI9341_SLPOUT 0x11  /* Come out of sleep. */
#define ILI9341_DISPON 0x29  /* Turn the display on. */
#define ILI9341_CASET 0x2A   /* Set the column range of the next write. */
#define ILI9341_PASET 0x2B   /* Set the row range of the next write. */
#define ILI9341_RAMWR 0x2C   /* The pixels follow. */
#define ILI9341_MADCTL 0x36  /* Scan order and colour order. */
#define ILI9341_PIXFMT 0x3A  /* How many bits a pixel is. */
#define ILI9341_INVON 0x21   /* Invert the colours. */

/* MADCTL bits: how the panel is scanned, and in which colour order. */
#define MADCTL_MY 0x80  /* Mirror the rows. */
#define MADCTL_MX 0x40  /* Mirror the columns. */
#define MADCTL_MV 0x20  /* Swap rows and columns, which is landscape. */
#define MADCTL_BGR 0x08 /* Blue first, not red. */

static SPIClass sSpi(VSPI);
static SPISettings sSettings(DISPLAY_SPI_HZ, MSBFIRST, SPI_MODE0);
static bool sReady = false;
/* Whether the board's own mount is turned the second way, `DISPLAY_ROTATION
 * == 3` at start up, XORed against a stored 180 by `displayRotationSet`. A
 * setting is relative to how the board is actually screwed in, not a
 * replacement for it. */
static bool sFlipped = false;

/*
 * Only two of the four combinations of MX and MY are rotations. Mirroring
 * one axis alone gives a mirror image, which reads backwards. Landscape is
 * MV on its own, or MV with both MX and MY for the same view turned round.
 * Those are the only two combinations that are a rotation.
 */
static uint8_t madctlFor(bool flipped) {
  uint8_t madctl = MADCTL_MV | MADCTL_BGR;
  if (flipped) {
    madctl |= MADCTL_MX | MADCTL_MY;
  }
  return madctl;
}
/* The panel is 240 by 320 the way it is made, and this radio uses it the wide
 * way round, so the drawing area is 320 across by 240 down. The board header
 * already states it that way. */
static uint16_t sWidth = DISPLAY_WIDTH;
static uint16_t sHeight = DISPLAY_HEIGHT;

/* The backlight channel. Channel 0 is free; the tuner uses no PWM. */
#define BACKLIGHT_CHANNEL 0
#define BACKLIGHT_HZ 5000 /* Well above anything an eye can see flicker. */
#define BACKLIGHT_BITS 8  /* 256 steps of brightness. */

/*
 * Take the SPI bus for one operation.
 *
 * Each drawing call takes it and gives it back. Holding it open for the life
 * of the firmware would keep the bus semaphore locked for ever: the touch
 * controller shares this bus, needs a slower clock, and would block on its
 * first read with no timeout.
 */
static void busTake(void) {
  sSpi.beginTransaction(sSettings);
}

static void busGive(void) {
  sSpi.endTransaction();
}

static void writeCommand(uint8_t cmd) {
  digitalWrite(PIN_TFT_DC, LOW);
  digitalWrite(PIN_TFT_CS, LOW);
  sSpi.transfer(cmd);
  digitalWrite(PIN_TFT_CS, HIGH);
}

static void writeCommandData(uint8_t cmd, const uint8_t *data, size_t len) {
  writeCommand(cmd);
  if (len == 0) {
    return;
  }
  digitalWrite(PIN_TFT_DC, HIGH);
  digitalWrite(PIN_TFT_CS, LOW);
  for (size_t i = 0; i < len; i++) {
    sSpi.transfer(data[i]);
  }
  digitalWrite(PIN_TFT_CS, HIGH);
}

/*
 * Say which rectangle the pixels that follow belong to.
 *
 * The panel takes the window first and then a stream of pixels, which is what
 * makes a partial redraw cheap: only the rectangle that changed is sent.
 */
static void setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  uint16_t x1 = (uint16_t)(x + w - 1);
  uint16_t y1 = (uint16_t)(y + h - 1);
  uint8_t cols[4] = {(uint8_t)(x >> 8), (uint8_t)x, (uint8_t)(x1 >> 8),
                     (uint8_t)x1};
  uint8_t rows[4] = {(uint8_t)(y >> 8), (uint8_t)y, (uint8_t)(y1 >> 8),
                     (uint8_t)y1};
  writeCommandData(ILI9341_CASET, cols, sizeof(cols));
  writeCommandData(ILI9341_PASET, rows, sizeof(rows));
  writeCommand(ILI9341_RAMWR);
}

static bool clip(int16_t *x, int16_t *y, uint16_t *w, uint16_t *h) {
  if (*w == 0 || *h == 0) {
    return false;
  }
  int32_t x0 = *x;
  int32_t y0 = *y;
  int32_t x1 = x0 + *w;
  int32_t y1 = y0 + *h;
  if (x0 < 0) {
    x0 = 0;
  }
  if (y0 < 0) {
    y0 = 0;
  }
  if (x1 > sWidth) {
    x1 = sWidth;
  }
  if (y1 > sHeight) {
    y1 = sHeight;
  }
  if (x1 <= x0 || y1 <= y0) {
    return false;
  }
  *x = (int16_t)x0;
  *y = (int16_t)y0;
  *w = (uint16_t)(x1 - x0);
  *h = (uint16_t)(y1 - y0);
  return true;
}

bool displayBegin(void) {
  pinMode(PIN_TFT_CS, OUTPUT);
  pinMode(PIN_TFT_DC, OUTPUT);
  pinMode(PIN_TFT_RST, OUTPUT);
  digitalWrite(PIN_TFT_CS, HIGH);
  digitalWrite(PIN_TFT_DC, HIGH);

  /* The touch controller shares this bus and must not answer while the panel
   * is being set up. Its chip select is driven high here even though touch
   * is not used yet. */
  pinMode(PIN_TOUCH_CS, OUTPUT);
  digitalWrite(PIN_TOUCH_CS, HIGH);

  /* MISO is deliberately not given a pin. Nothing reads from this panel, and
   * the pin the bus would use is the standby LED. Touch does have to read, so
   * it will have to attach MISO and settle that pin first. */
  sSpi.begin(PIN_SPI_SCK, -1, PIN_SPI_MOSI, -1);
  /* The weakest drive, about 5 mA, as the reference firmware sets. Softer
   * edges put less noise into the tuner next to these lines. Edges too soft
   * for the clock above show as speckled pixels, so the two are judged
   * together, on the glass. Set after begin, which is what routes these
   * pins. */
  const gpio_num_t spiPins[] = {
      (gpio_num_t)PIN_SPI_SCK, (gpio_num_t)PIN_SPI_MOSI, (gpio_num_t)PIN_TFT_CS,
      (gpio_num_t)PIN_TFT_DC, (gpio_num_t)PIN_TFT_RST};
  for (gpio_num_t pin : spiPins) {
    gpio_set_drive_capability(pin, GPIO_DRIVE_CAP_0);
  }
  busTake();

  digitalWrite(PIN_TFT_RST, HIGH);
  delay(5);
  digitalWrite(PIN_TFT_RST, LOW);
  delay(20);
  digitalWrite(PIN_TFT_RST, HIGH);
  delay(150);

  writeCommand(ILI9341_SWRESET);
  delay(150);

  /* Power and gamma, from the ILI9341 datasheet's own recommended values.
   *
   * These are not optional decoration. Without them the panel powers up with a
   * gamma curve that lifts the bottom of the scale hard: the background colour
   * this radio uses is red 1, green 3 and blue 2 out of 31, 63 and 31, which
   * is all but black, and it comes out as a solid medium blue. Pure red,
   * green, blue, black and white all look right without them, which makes it
   * read as a colour order problem rather than a gamma one.
   *
   * Each line is a command and the bytes that follow it. */
  static const uint8_t kInit[] = {
      0xEF, 3,    0x03, 0x80, 0x02,                   /* Undocumented, and in */
      0xCF, 3,    0x00, 0xC1, 0x30,                   /* every vendor init. */
      0xED, 4,    0x64, 0x03, 0x12, 0x81,             /* Power on sequence. */
      0xE8, 3,    0x85, 0x00, 0x78,                   /* Driver timing A. */
      0xCB, 5,    0x39, 0x2C, 0x00, 0x34, 0x02,       /* Power control A. */
      0xF7, 1,    0x20,                               /* Pump ratio. */
      0xEA, 2,    0x00, 0x00,                         /* Driver timing B. */
      0xC0, 1,    0x23,                               /* Power control 1. */
      0xC1, 1,    0x10,                               /* Power control 2. */
      0xC5, 2,    0x3E, 0x28,                         /* VCOM 1. */
      0xC7, 1,    0x86,                               /* VCOM 2. */
      0xB1, 2,    0x00, 0x18,                         /* Frame rate. */
      0xB6, 3,    0x08, 0x82, 0x27,                   /* Display function. */
      0xF2, 1,    0x00,                               /* 3 gamma off. */
      0x26, 1,    0x01,                               /* Gamma curve 1. */
      0xE0, 15,   0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, /* Positive gamma. */
      0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
      0xE1, 15,   0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, /* Negative gamma. */
      0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
  };
  for (size_t i = 0; i < sizeof(kInit);) {
    uint8_t cmd = kInit[i++];
    uint8_t len = kInit[i++];
    writeCommandData(cmd, &kInit[i], len);
    i += len;
  }

  uint8_t pixfmt = 0x55; /* 16 bits per pixel. */
  writeCommandData(ILI9341_PIXFMT, &pixfmt, 1);

  /* Landscape, and BGR because this panel is wired that way. The working
   * firmware's setup calls this colour order RGB, which is the setting name
   * there for this bit being set. A wrong choice here does
   * not fail, it swaps red and blue, so it is checked by eye. */
  sFlipped = (DISPLAY_ROTATION == 3);
  uint8_t madctl = madctlFor(sFlipped);
  writeCommandData(ILI9341_MADCTL, &madctl, 1);

  /* This panel is fitted the inverted way round. Without this the near black
   * background comes out white and the amber comes out blue. Inversion is a
   * property of the glass, not of the controller, so it cannot be worked out
   * from the datasheet and has to be looked at. */
  writeCommand(ILI9341_INVON);
  delay(10);

  writeCommand(ILI9341_SLPOUT);
  delay(120);
  writeCommand(ILI9341_DISPON);
  delay(20);

  busGive();
  sReady = true;

  /* The backlight is attached once, here. Attaching it again on every
   * brightness change logs an error from the core each time. */
  ledcAttachChannel(PIN_BACKLIGHT_PWM, BACKLIGHT_HZ, BACKLIGHT_BITS,
                    BACKLIGHT_CHANNEL);

  displayFill(0, 0, sWidth, sHeight, 0);
  /* Left dark. The panel is cleared first so there is nothing to see, and
   * whoever started it decides how the light comes up. Turning it on here
   * would put a frame of whatever the controller powered up holding on the
   * glass before the fade had a chance to start. */
  displayBacklight(0);
  return true;
}

/*
 * Turn the panel between the board's own mount and 180 from it, live.
 *
 * `degrees` is a stored setting: 0 is however the board is actually
 * screwed in, the orientation `displayBegin` sets up, and 180 XORs the same
 * MX/MY pair into whatever that already is. Nothing else is legal;
 * `settingsValid` refuses a stored value that is not one of these two before
 * it ever reaches here.
 */
bool displayRotationSet(uint16_t degrees) {
  const bool flipped = (DISPLAY_ROTATION == 3) != (degrees == 180);
  if (!sReady || flipped == sFlipped) {
    return false;
  }
  sFlipped = flipped;
  uint8_t madctl = madctlFor(sFlipped);
  busTake();
  writeCommandData(ILI9341_MADCTL, &madctl, 1);
  busGive();
  return true;
}

void displayBacklight(uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }
  ledcWrite(PIN_BACKLIGHT_PWM, (uint32_t)percent * 255 / 100);
}

uint16_t displayWidth(void) {
  return sWidth;
}

uint16_t displayHeight(void) {
  return sHeight;
}

/*
 * How many pixels are sent per burst.
 *
 * One byte at a time costs about two microseconds each, so clearing the panel
 * takes the best part of a second and nothing else on the loop task runs
 * meanwhile. Sending a block at a time lets the driver fill the hardware
 * queue instead of stopping between every byte.
 */
#define CHUNK_PIXELS 128
static uint8_t sChunk[CHUNK_PIXELS * 2];

void displayFill(int16_t x, int16_t y, uint16_t w, uint16_t h, Colour colour) {
  if (!sReady || !clip(&x, &y, &w, &h)) {
    return;
  }
  busTake();
  setWindow((uint16_t)x, (uint16_t)y, w, h);

  /* The panel wants the high byte first, whichever way round this processor
   * stores a uint16_t, so the bytes are laid out by hand. */
  for (uint16_t i = 0; i < CHUNK_PIXELS; i++) {
    sChunk[i * 2] = (uint8_t)(colour >> 8);
    sChunk[i * 2 + 1] = (uint8_t)colour;
  }

  uint32_t left = (uint32_t)w * h;
  digitalWrite(PIN_TFT_DC, HIGH);
  digitalWrite(PIN_TFT_CS, LOW);
  while (left > 0) {
    uint32_t n = left > CHUNK_PIXELS ? CHUNK_PIXELS : left;
    sSpi.writeBytes(sChunk, n * 2);
    left -= n;
  }
  digitalWrite(PIN_TFT_CS, HIGH);
  busGive();
}

void displayPush(int16_t x, int16_t y, uint16_t w, uint16_t h,
                 const uint8_t *bytes) {
  if (!sReady || bytes == NULL) {
    return;
  }
  /* Clipping would mean skipping rows and columns of the source, and nothing
   * here draws off the edge, so an off screen rectangle is a caller's mistake
   * and is refused rather than half drawn. */
  if (x < 0 || y < 0 || w == 0 || h == 0 || x + w > sWidth || y + h > sHeight) {
    return;
  }
  busTake();
  setWindow((uint16_t)x, (uint16_t)y, w, h);
  digitalWrite(PIN_TFT_DC, HIGH);
  digitalWrite(PIN_TFT_CS, LOW);

  sSpi.writeBytes(bytes, (uint32_t)w * h * 2);
  digitalWrite(PIN_TFT_CS, HIGH);
  busGive();
}
