/* Implementation of the plain text screen. */
#include "screen.h"

#include "core/signal.h"

#include "drivers/display.h"
#include "ui/font_large.h"
#include "ui/font_small.h"

#include <stdio.h>
#include <string.h>

/* The palette, from docs/design.html. Only the few colours this screen uses.
 * The rest of that design is phase 4.
 *
 * The background is pure black, not the design's #0B0F13. That colour is red
 * 1, green 3 and blue 2 once it is squeezed into the panel's 5, 6 and 5 bits,
 * and this panel does not render the bottom of its scale as anything like a
 * near black: it came out a solid medium blue, against a proper black for
 * zero. Measured with a bar of each on the glass. */
static const Colour kBackground = displayColour(0, 0, 0);
static const Colour kPrimary = displayColour(0xFF, 0xB2, 0x00);
static const Colour kText = displayColour(0xE6, 0xE6, 0xE6);
static const Colour kDim = displayColour(0x6C, 0x7A, 0x89);
static const Colour kGood = displayColour(0x31, 0xC2, 0x9C);
static const Colour kBad = displayColour(0xFF, 0x8A, 0x72);

/* Gap from the edge of the panel, in pixels. */
#define MARGIN 12

/*
 * How long a remembered line can be, with its terminator.
 *
 * Longer than the longest string any field can hold. Shorter than that and a
 * field is stored truncated, never compares equal to what it is asked to
 * draw, and so is redrawn on every pass: constant SPI traffic and a
 * visible flicker. The tuner fault messages are the long ones.
 */
#define LINE_MAX 64

/*
 * One field on the screen.
 *
 * Each owns a fixed box. Redrawing means painting the whole box over and then
 * writing into it, which is what stops anything of the old text surviving.
 * Clearing only the difference in width leaves the tail of a longer string
 * behind whenever a field moves, and the unit after the frequency moves every
 * time the number changes width. On the radio that shows as "kHz MHzzHz".
 */
typedef struct {
  int16_t x;            /* Left edge of the box. */
  int16_t y;            /* Top edge. */
  uint16_t w;           /* Width. */
  const Font *font;     /* Which font. */
  bool alignRight;      /* Sit the text against the right of the box. */
  char drawn[LINE_MAX]; /* What is on the panel now. */
  bool valid;           /* Something has been drawn in this box. */
} Field;

/* Every field, in the order they appear down the screen. */
enum {
  FIELD_BAND = 0,
  FIELD_MODE,
  FIELD_FREQUENCY,
  FIELD_SIGNAL,
  FIELD_FLAGS,
  FIELD_MUTE,
  FIELD_FAULT,
  FIELD_COUNT
};

static Field sFields[FIELD_COUNT];

static void place(int which, int16_t x, int16_t y, uint16_t w, const Font *font,
                  bool alignRight) {
  Field *f = &sFields[which];
  memset(f, 0, sizeof(*f));
  f->x = x;
  f->y = y;
  f->w = w;
  f->font = font;
  f->alignRight = alignRight;
}

static void layout(void) {
  uint16_t panel = displayWidth();
  uint16_t small = kSmallFont.height;
  uint16_t large = kLargeFont.height;
  uint16_t full = (uint16_t)(panel - 2 * MARGIN);
  uint16_t half = (uint16_t)(full / 2);
  int16_t row = 70;

  place(FIELD_BAND, MARGIN, 14, half, &kSmallFont, false);
  place(FIELD_MODE, (int16_t)(MARGIN + half), 14, half, &kSmallFont, true);
  place(FIELD_FREQUENCY, MARGIN, row, full, &kLargeFont, false);
  row = (int16_t)(row + large + 24);
  place(FIELD_SIGNAL, MARGIN, row, full, &kSmallFont, false);
  row = (int16_t)(row + small + 8);
  /* Mute has a box of its own on the same row, at the right. It is drawn in
   * the fault colour, and sharing a box would paint iMS and EQ the same, so a
   * muted radio would look as though those had gone wrong. */
  place(FIELD_FLAGS, MARGIN, row, half, &kSmallFont, false);
  place(FIELD_MUTE, (int16_t)(MARGIN + half), row, half, &kSmallFont, true);
  row = (int16_t)(row + small + 8);
  place(FIELD_FAULT, MARGIN, row, full, &kSmallFont, false);
}

/*
 * Draw a field, if what it says has changed.
 *
 * Text wider than its box is cut to fit. Two fields share the top row, so a
 * string that overran would write into its neighbour's box and leave half a
 * word of the wrong field on the screen, and one that reached the edge of the
 * panel would lose whole glyphs without saying so: displayPush refuses a
 * rectangle that crosses the edge rather than clipping it.
 */
static void draw(int which, const char *text, Colour colour) {
  Field *f = &sFields[which];
  if (text == NULL) {
    text = "";
  }
  if (f->valid && strncmp(f->drawn, text, LINE_MAX) == 0) {
    return;
  }

  /* Cut to what the box holds, a character at a time from the end. Cutting by
   * an average character width would be wrong on a proportional font. */
  char fitted[LINE_MAX];
  snprintf(fitted, sizeof(fitted), "%s", text);
  size_t len = strlen(fitted);
  while (len > 0 && displayTextWidth(f->font, fitted) > f->w) {
    fitted[--len] = '\0';
  }

  displayFill(f->x, f->y, f->w, f->font->height, kBackground);

  int16_t at = f->x;
  uint16_t width = displayTextWidth(f->font, fitted);
  if (width < f->w && f->alignRight) {
    at = (int16_t)(f->x + f->w - width);
  }
  displayText(at, f->y, f->font, fitted, colour, kBackground);

  /* What was asked for, not what fitted, so a field whose text changes only
   * past the cut is still redrawn when the box or the font changes. */
  snprintf(f->drawn, LINE_MAX, "%s", text);
  f->valid = true;
}

bool screenBegin(void) {
  if (!displayBegin()) {
    return false;
  }
  layout();
  /* displayBegin has already cleared the panel, and a full clear is the most
   * expensive thing this driver does, so it is not done again here. */
  return true;
}

void screenMessage(const char *line1, const char *line2) {
  displayFill(0, 0, displayWidth(), displayHeight(), kBackground);
  layout();

  int16_t y = (int16_t)(displayHeight() / 2 - kSmallFont.height);
  if (line1 != NULL) {
    uint16_t w = displayTextWidth(&kSmallFont, line1);
    displayText((int16_t)((displayWidth() - w) / 2), y, &kSmallFont, line1,
                kPrimary, kBackground);
  }
  if (line2 != NULL) {
    uint16_t w = displayTextWidth(&kSmallFont, line2);
    displayText((int16_t)((displayWidth() - w) / 2),
                (int16_t)(y + kSmallFont.height + 6), &kSmallFont, line2, kText,
                kBackground);
  }
}

static void formatSignal(const ScreenState *state, char *out, size_t outLen) {
  if (!state->tunerReady) {
    snprintf(out, outLen, "no tuner");
    return;
  }
  if (!state->signalValid) {
    snprintf(out, outLen, "no reading");
    return;
  }
  snprintf(out, outLen, "%d dBuV", (int)state->signalDbuV);
}

void screenShow(const ScreenState *state) {
  if (state == NULL) {
    return;
  }

  /* The first draw after a message paints over whatever it left. */
  if (!sFields[FIELD_FREQUENCY].valid) {
    displayFill(0, 0, displayWidth(), displayHeight(), kBackground);
  }

  draw(FIELD_BAND, state->band, kPrimary);
  draw(FIELD_MODE, state->mode, kDim);

  /* The frequency and its unit share one box. The unit sits after the number,
   * so it moves whenever the number changes width, and giving it a box of its
   * own is what left three units on the screen at once. */
  char frequency[LINE_MAX];
  snprintf(frequency, sizeof(frequency), "%s %s",
           state->frequency != NULL ? state->frequency : "",
           state->unit != NULL ? state->unit : "");
  Field *f = &sFields[FIELD_FREQUENCY];
  if (!f->valid || strncmp(f->drawn, frequency, LINE_MAX) != 0) {
    displayFill(f->x, f->y, f->w, f->font->height, kBackground);
    int16_t end = displayText(f->x, f->y, &kLargeFont,
                              state->frequency != NULL ? state->frequency : "",
                              kText, kBackground);
    displayText((int16_t)(end + 8),
                (int16_t)(f->y + kLargeFont.height - kSmallFont.height),
                &kSmallFont, state->unit != NULL ? state->unit : "", kDim,
                kBackground);
    snprintf(f->drawn, LINE_MAX, "%s", frequency);
    f->valid = true;
  }

  char signal[LINE_MAX];
  formatSignal(state, signal, sizeof(signal));
  draw(FIELD_SIGNAL, signal, state->tunerReady ? kText : kBad);

  char flags[LINE_MAX];
  snprintf(flags, sizeof(flags), "%s%s%s", state->ims ? "iMS " : "",
           state->eq ? "EQ " : "", state->stereo ? "stereo" : "");
  draw(FIELD_FLAGS, flags, kGood);
  draw(FIELD_MUTE, state->muted ? "muted" : "", kBad);

  draw(FIELD_FAULT, state->fault, kBad);
}
