/**
 * @file display.cpp
 * @brief Implementation of the ILI9341 panel driver.
 */
#include "display.h"

#include "board/board.h"

#include <Arduino.h>
#include <SPI.h>

/**
 * How fast the panel is clocked, in hertz.
 *
 * 7.5 MHz is what the working PE5PVB firmware uses on this board, recorded in
 * HARDWARE.md. It is slower than the chip can go, and the ribbon on a portable
 * is the reason a board runs below the datasheet figure. Raising it is a
 * measurement, not a guess: a panel clocked too fast shows torn or speckled
 * pixels rather than failing, so it has to be looked at before it is trusted.
 */
#define DISPLAY_SPI_HZ 7500000

/**
 * How the panel is turned. 1 and 3 are the two landscape views.
 *
 * Settled by looking at the radio, which is the only way. Three of the four
 * tries were wrong in a different way each time: mirrored, mirrored the other
 * way, then the right way round but upside down. This is the fourth.
 */
#define DISPLAY_ROTATION 1

/* ILI9341 commands, only the ones used here. */
#define ILI9341_SWRESET 0x01 /**< Reset the panel in software. */
#define ILI9341_SLPOUT 0x11  /**< Come out of sleep. */
#define ILI9341_DISPON 0x29  /**< Turn the display on. */
#define ILI9341_CASET 0x2A   /**< Set the column range of the next write. */
#define ILI9341_PASET 0x2B   /**< Set the row range of the next write. */
#define ILI9341_RAMWR 0x2C   /**< The pixels follow. */
#define ILI9341_MADCTL 0x36  /**< Scan order and colour order. */
#define ILI9341_PIXFMT 0x3A  /**< How many bits a pixel is. */
#define ILI9341_INVON 0x21   /**< Invert the colours. */

/* MADCTL bits: how the panel is scanned, and in which colour order. */
#define MADCTL_MY 0x80  /**< Mirror the rows. */
#define MADCTL_MX 0x40  /**< Mirror the columns. */
#define MADCTL_MV 0x20  /**< Swap rows and columns, which is landscape. */
#define MADCTL_BGR 0x08 /**< Blue first, not red. */

static SPIClass sSpi(VSPI);
static SPISettings sSettings(DISPLAY_SPI_HZ, MSBFIRST, SPI_MODE0);
static bool sReady = false;
/* The panel is 240 by 320 the way it is made, and this radio uses it the wide
 * way round, so the drawing area is 320 across by 240 down. The board header
 * already states it that way. Having these two the other way round drew
 * everything into a 240 wide box and pushed it to one side of the glass. */
static uint16_t sWidth = DISPLAY_WIDTH;
static uint16_t sHeight = DISPLAY_HEIGHT;

/** The backlight channel. Channel 0 is free; the tuner uses no PWM. */
#define BACKLIGHT_CHANNEL 0
#define BACKLIGHT_HZ 5000 /**< Well above anything an eye can see flicker. */
#define BACKLIGHT_BITS 8  /**< 256 steps of brightness. */

/**
 * A scratch buffer, big enough for one glyph of the largest font.
 *
 * One SPI transaction per glyph rather than one per row. Static rather than on
 * the stack because the loop task's stack is not large and a 5KB frame in a
 * drawing call is how a stack overflow arrives later, in some unrelated place.
 */
#define GLYPH_MAX_W 64 /**< The widest glyph either font has. */
#define GLYPH_MAX_H 40 /**< The tallest. The large font is 31. */
static Colour sGlyph[GLYPH_MAX_W * GLYPH_MAX_H];

/**
 * Take the SPI bus for one operation.
 *
 * Each drawing call takes it and gives it back. Holding it open for the life
 * of the firmware, which is what this did at first, keeps the bus semaphore
 * locked for ever: the touch controller in phase 4 shares this bus, needs a
 * slower clock, and would block on its first read with no timeout.
 */
static void busTake(void) {
  sSpi.beginTransaction(sSettings);
}

/** Give the bus back. One for every busTake. */
static void busGive(void) {
  sSpi.endTransaction();
}

/** Send one command byte. */
static void writeCommand(uint8_t cmd) {
  digitalWrite(PIN_TFT_DC, LOW);
  digitalWrite(PIN_TFT_CS, LOW);
  sSpi.transfer(cmd);
  digitalWrite(PIN_TFT_CS, HIGH);
}

/** Send a command and its data bytes. */
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

/**
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

/**
 * Clip a rectangle to the panel.
 *
 * @return false when nothing of it is on screen, in which case nothing should
 *         be drawn. Without this a negative x wraps into a huge unsigned
 *         window and the panel is asked for a rectangle off the end of itself.
 */
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
   * itself is phase 4. */
  pinMode(PIN_TOUCH_CS, OUTPUT);
  digitalWrite(PIN_TOUCH_CS, HIGH);

  /* MISO is deliberately not given a pin. Nothing reads from this panel, and
   * the pin the bus would use is the standby LED. See decision 26. Touch in
   * phase 4 does have to read, so it will have to attach MISO and settle that
   * pin first. */
  sSpi.begin(PIN_SPI_SCK, -1, PIN_SPI_MOSI, -1);
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
   * is all but black, and it came out as a solid medium blue. Pure red, green,
   * blue, black and white all looked right, which is what made it look like a
   * colour order problem rather than a gamma one.
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

  /* Landscape, and BGR because this panel is wired that way. HARDWARE.md
   * records the colour order as RGB from the working firmware's setup, which
   * is the setting name there for this bit being set. A wrong choice here does
   * not fail, it swaps red and blue, so it is checked by eye. */
  /* Only two of the four combinations of MX and MY are rotations. Mirroring
   * one axis alone gives a mirror image, which reads backwards. Landscape is
   * MV on its own, or MV with both MX and MY for the same view turned round.
   * Setting just one of them was the first two attempts on real hardware, and
   * both came out mirrored. */
  uint8_t madctl = MADCTL_MV | MADCTL_BGR;
  if (DISPLAY_ROTATION == 3) {
    madctl |= MADCTL_MX | MADCTL_MY;
  }
  writeCommandData(ILI9341_MADCTL, &madctl, 1);

  /* This panel is fitted the inverted way round. Without this the near black
   * background comes out white and the amber comes out blue, which is exactly
   * what the first build on real hardware showed. Inversion is a property of
   * the glass, not of the controller, so it cannot be worked out from the
   * datasheet and has to be looked at. */
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
  displayBacklight(100);
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

/**
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
                 const Colour *pixels) {
  if (!sReady || pixels == NULL) {
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

  uint32_t total = (uint32_t)w * h;
  uint32_t done = 0;
  while (done < total) {
    uint32_t n = total - done > CHUNK_PIXELS ? CHUNK_PIXELS : total - done;
    for (uint32_t i = 0; i < n; i++) {
      sChunk[i * 2] = (uint8_t)(pixels[done + i] >> 8);
      sChunk[i * 2 + 1] = (uint8_t)pixels[done + i];
    }
    sSpi.writeBytes(sChunk, n * 2);
    done += n;
  }
  digitalWrite(PIN_TFT_CS, HIGH);
  busGive();
}

/**
 * Which glyph a character is, or -1 when the font does not have it.
 *
 * The large font holds the digits, a full stop and a space, in that order,
 * and nothing else. It says so by having no first and last character.
 */
static int glyphIndex(const Font *font, char c) {
  if (font->first == 0 && font->last == 0) {
    if (c >= '0' && c <= '9') {
      return c - '0';
    }
    if (c == '.') {
      return 10;
    }
    if (c == ' ') {
      return 11;
    }
    return -1;
  }
  if ((uint8_t)c < font->first || (uint8_t)c > font->last) {
    return -1;
  }
  return (uint8_t)c - font->first;
}

uint16_t displayTextWidth(const Font *font, const char *text) {
  if (font == NULL || text == NULL) {
    return 0;
  }
  uint32_t total = 0;
  for (const char *p = text; *p != '\0'; p++) {
    int index = glyphIndex(font, *p);
    if (index >= 0) {
      total += font->glyphs[index].width;
    }
  }
  return total > 0xFFFF ? 0xFFFF : (uint16_t)total;
}

int16_t displayText(int16_t x, int16_t y, const Font *font, const char *text,
                    Colour fg, Colour bg) {
  if (!sReady || font == NULL || text == NULL) {
    return x;
  }
  if (font->height > GLYPH_MAX_H) {
    return x;
  }

  for (const char *p = text; *p != '\0'; p++) {
    int index = glyphIndex(font, *p);
    if (index < 0) {
      continue;
    }
    const FontGlyph *g = &font->glyphs[index];
    uint16_t w = g->width;
    if (w == 0) {
      continue;
    }
    if (w > GLYPH_MAX_W) {
      w = GLYPH_MAX_W;
    }

    /* Unpack one glyph into colours, then push it in one go. A row at a time
     * would be a separate SPI window for every row of every character. */
    uint16_t stride = (uint16_t)((g->width + 7) / 8);
    for (uint16_t row = 0; row < font->height; row++) {
      const uint8_t *src = &font->bits[g->offset + (uint32_t)row * stride];
      for (uint16_t col = 0; col < w; col++) {
        bool on = (src[col / 8] >> (7 - (col % 8))) & 1;
        sGlyph[(uint32_t)row * w + col] = on ? fg : bg;
      }
    }
    displayPush(x, y, w, font->height, sGlyph);
    /* By what was drawn, not by what the glyph claims. They differ only for a
     * glyph wider than the scratch buffer, which no font here has, and a gap
     * after a truncated character is harder to spot than a narrow one. */
    x = (int16_t)(x + w);
  }
  return x;
}
