/*
 * Draw the real screens on this machine and write them out as images.
 *
 * `ui/` is built so that it knows about LVGL and nothing else: no tuner, no
 * driver, no Arduino. That is what makes this tool possible. The same
 * screen.cpp that runs on the radio is linked here against LVGL's software
 * renderer, handed a state, and rendered into memory rather than into a panel.
 * What comes out is what the panel would show.
 *
 * It exists because the panel is the one part of this firmware that cannot be
 * checked over HTTP. Everything else answers a question about itself; a
 * screen only looks right or wrong, and without this tool the only way to
 * see it is to look at the radio.
 *
 * What it does not check: the real panel's colours, its gamma, its refresh,
 * the mura on black, or how any of it looks in a room. Those still need eyes.
 * This checks layout, and layout is most of what goes wrong.
 *
 *     tools/screenshot.sh            build it and draw every scene
 *     tools/screenshot.sh /tmp/x       somewhere else
 */
#include <execinfo.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board/board.h"
#include "core/version.h"
#include "lvgl.h"
/* For the draw unit and the text decoder that the glyph check below uses. */
#include "src/lvgl_private.h"
#include "ui/draw.h"
#include "ui/screen.h"
#include "ui/theme.h"

#include "../test/test_dx/captures.h"
#include "../test/test_rds/captures.h"
/* The RDS captures count their own list with this name, and the level sweep
 * below uses it for its channel count. Nothing here reads the first. */
#undef CAPTURE_COUNT
#include "../test/test_dx_sweep/capture.h"
#include "core/band_plan.h"
#include "core/battery.h"
#include "core/bw_page.h"
#include "core/dx.h"
#include "core/dx_catch.h"
#include "core/dx_sweep.h"
#include "core/palette.h"
#include "core/radio.h"
#include "core/rds.h"
#include "core/rds_country.h"
#include "core/settings.h"
#include "core/signal.h"
#include "core/strings.h"
#include "dx_task.h"
#include "lvgl_port.h"
#include "screen_bw_state.h"
#include "screen_dx_state.h"
#include "screen_rds_state.h"
#include "screen_state.h"

/*
 * What LVGL calls when an assertion fails.
 *
 * The radio's version restarts the chip. Here there is nothing to restart, so
 * it says what happened and stops, which is what a build machine wants.
 */
extern "C" void lvglAssertFailed(void) {
  fprintf(stderr, "lvgl assertion failed\n");
  /* Where, since LV_USE_LOG is 0 and the radio has no serial cable to turn it
   * on for. The trace is the whole value of hitting this here rather than on
   * the hardware. */
  void *frames[32];
  int n = backtrace(frames, 32);
  backtrace_symbols_fd(frames, n, 2);
  exit(2);
}

/*
 * A character a face does not carry draws as nothing, with no error and no
 * log line. So every text LVGL is asked to draw is checked here, against the
 * font it is drawn in, before the software renderer draws it. The check sits in
 * the draw step rather than on the labels, so a text a panel draws itself is
 * checked too. A miss is kept until the capture it was drawn in is written, so
 * the report can name the capture.
 */
static uint32_t sMissing[8];
static int sMissingCount = 0;
static int sMissingCaptures = 0;

static void noteMissing(uint32_t letter) {
  for (int i = 0; i < sMissingCount; i++) {
    if (sMissing[i] == letter) {
      return;
    }
  }
  /* Past eight different characters the capture fails just the same, only
   * the list in the report stops growing. */
  if (sMissingCount < (int)(sizeof(sMissing) / sizeof(sMissing[0]))) {
    sMissing[sMissingCount++] = letter;
  }
}

static int32_t checkGlyphs(lv_draw_unit_t *unit, lv_draw_task_t *task) {
  (void)unit;
  if (lv_draw_task_get_type(task) != LV_DRAW_TASK_TYPE_LABEL) {
    return 0;
  }
  const lv_draw_label_dsc_t *d = lv_draw_task_get_label_dsc(task);
  if (d->text == NULL) {
    return 0;
  }
  uint32_t i = 0;
  while (i < d->text_length && d->text[i] != '\0') {
    const uint32_t letter = lv_text_encoded_next(d->text, &i);
    lv_font_glyph_dsc_t g;
    if (!lv_text_is_marker(letter) &&
        !lv_font_get_glyph_dsc(d->font, &g, letter, 0)) {
      noteMissing(letter);
    }
  }
  return 0;
}

/* The checking unit takes no task, so the software renderer draws them all. */
static int32_t takeNothing(lv_draw_unit_t *unit, lv_layer_t *layer) {
  (void)unit;
  (void)layer;
  return LV_DRAW_UNIT_IDLE;
}

static void reportMissing(const char *where) {
  if (sMissingCount == 0) {
    return;
  }
  /* So the report does not land in the middle of a path still buffered. */
  fflush(stdout);
  fprintf(stderr, "  %s: no glyph for", where);
  for (int i = 0; i < sMissingCount; i++) {
    fprintf(stderr, " U+%04X", (unsigned)sMissing[i]);
  }
  fputc('\n', stderr);
  sMissingCount = 0;
  sMissingCaptures++;
}

#define W 320
#define H 240

static uint16_t sFrame[W * H];
static uint16_t sDrawBuf[W * LVGL_DRAW_LINES];

/*
 * Take the piece LVGL has finished and put it in the frame.
 *
 * The same job the real flush does, into memory instead of down the SPI bus.
 * LVGL draws in the radio's own format, each pixel's bytes high first as the
 * panel takes them, so they are turned back here for the image.
 */
static void flush(lv_display_t *display, const lv_area_t *area,
                  uint8_t *pixels) {
  const uint16_t *src = (const uint16_t *)pixels;
  for (int32_t y = area->y1; y <= area->y2; y++) {
    for (int32_t x = area->x1; x <= area->x2; x++) {
      if (x >= 0 && x < W && y >= 0 && y < H) {
        sFrame[y * W + x] = (uint16_t)((*src >> 8) | (*src << 8));
      }
      src++;
    }
  }
  lv_display_flush_ready(display);
}

/* LVGL wants a clock. Nothing here animates, so a counter is enough. */
static uint32_t sTick = 0;
static uint32_t tick(void) {
  sTick += 5;
  return sTick;
}

/* GET /api/screen needs no PIN, so a text there that is one digit while the
 * PIN editor is up is the PIN. Counts the stars it should be instead. */
static void countPinTexts(void *ctx, const UiText *t) {
  int *count = (int *)ctx;
  if (t->text[0] >= '0' && t->text[0] <= '9' && t->text[1] == '\0') {
    count[1]++;
  } else if (strcmp(t->text, "*") == 0) {
    count[0]++;
  }
}

/*
 * A bottom up 24 bit BMP, because it is the one image format that is a header
 * and then the pixels. Writing a PNG would mean a compressor, and nothing
 * here is worth a dependency.
 */
static int writeBmp(const char *path) {
  reportMissing(path);
  FILE *f = fopen(path, "wb");
  if (f == NULL) {
    return 0;
  }
  const uint32_t rowBytes = (uint32_t)(W * 3 + 3) & ~3u;
  const uint32_t pixelBytes = rowBytes * H;
  const uint32_t fileBytes = 54 + pixelBytes;
  uint8_t head[54];
  memset(head, 0, sizeof(head));
  head[0] = 'B';
  head[1] = 'M';
  memcpy(head + 2, &fileBytes, 4);
  const uint32_t offset = 54;
  memcpy(head + 10, &offset, 4);
  const uint32_t dib = 40;
  memcpy(head + 14, &dib, 4);
  const int32_t w = W;
  const int32_t h = H;
  memcpy(head + 18, &w, 4);
  memcpy(head + 22, &h, 4);
  head[26] = 1;
  head[28] = 24;
  memcpy(head + 34, &pixelBytes, 4);
  fwrite(head, 1, sizeof(head), f);

  uint8_t *row = (uint8_t *)calloc(rowBytes, 1);
  for (int y = H - 1; y >= 0; y--) {
    for (int x = 0; x < W; x++) {
      const uint16_t c = sFrame[y * W + x];
      /* RGB565 out to eight bits each, with the top bits repeated into the
       * bottom so that full scale stays full scale. */
      const uint8_t r = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
      const uint8_t g = (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
      const uint8_t b = (uint8_t)((c & 0x1F) * 255 / 31);
      row[x * 3 + 0] = b;
      row[x * 3 + 1] = g;
      row[x * 3 + 2] = r;
    }
    fwrite(row, 1, rowBytes, f);
  }
  free(row);
  fclose(f);
  return 1;
}

/* Draws the panel, writes it to the path the format gives, and prints the
 * path. A picture that could not be written ends the run with a failure,
 * so an old copy is never taken for a new one. */
static void saveShot(const char *format, ...) {
  char path[512];
  va_list args;
  va_start(args, format);
  vsnprintf(path, sizeof(path), format, args);
  va_end(args);
  lv_refr_now(NULL);
  if (!writeBmp(path)) {
    fprintf(stderr, "could not write %s\n", path);
    exit(1);
  }
  printf("  %s\n", path);
}

/*
 * A text made from one of the table's printf formats, so a scene shows the
 * radio's own wording for a number. A scene holds several of these at once,
 * so the buffers rotate rather than being one.
 */
static const char *fmt(StrId id, ...) {
  static char ring[32][40];
  static unsigned next;
  char *out = ring[next++ % 32];
  va_list args;
  va_start(args, id);
  vsnprintf(out, sizeof(ring[0]), txt(id), args);
  va_end(args);
  return out;
}

/* A number with its unit after it, the way the menu prints the dwell. */
static const char *withUnit(const char *number, StrId unit) {
  static char ring[4][16];
  static unsigned next;
  char *out = ring[next++ % 4];
  const int n = snprintf(out, sizeof(ring[0]), "%s", number);
  snprintf(out + n, sizeof(ring[0]) - (size_t)n, txt(STR_MENU_FMT_UNIT_AFTER),
           txt(unit));
  return out;
}

/*
 * The scenes.
 *
 * Each is a state the radio can really be in, with the numbers taken from
 * what this radio reads on those stations rather than made up, so a layout
 * that only works for short strings shows itself.
 */
static void sceneFm(ScreenState *s) {
  s->fm = true;
  s->band = txt(STR_BAND_FM);
  s->frequency = "106.40";
  s->sweepLowKHz = 87500;
  s->sweepSpanKHz = 20500;
  s->sweepKHz = 106400;
  s->unit = txt(STR_COMMON_UNIT_MHZ);
  s->signalDbuV = 38;
  s->signalValid = true;
  s->signalPercent = 63;
  s->modulationValid = true;
  s->modulationPercent = 62;
  s->modulationPeakValid = true;
  s->modulationPeakPercent = 84;
  s->stationName = "MAGIC";
  s->radioText = "CHEPPANA NA KADHA - BHARAYALU JAGARTHA";
  s->memory = fmt(STR_RADIO_FMT_MEMORY_SLOT, 12);
  s->wifi = SCREEN_WIFI_JOINED;
  s->wifiBars = 3;
  s->batteryValid = true;
  s->batteryPercent = 78;
  s->batteryText = NULL;
  s->tuneMode = tuneModeShort(TUNE_MODE_MANUAL);
  s->squelchMode = txt(STR_COMMON_AUTO);
  s->filter = txt(STR_RADIO_BW_DYN);
  s->volume = fmt(STR_RADIO_FMT_VOL_DB, -6);
  s->clock = "05:09";
  s->date = fmt(STR_DATE_FMT_LINE, txt(STR_DAY_WEDNESDAY), 30,
                txt(STR_DATE_SUFFIX_TH), txt(STR_MONTH_SEPTEMBER), 2026);
  s->tunerReady = true;
}

/* FM with a PC signed in over the PC Link. */
static void scenePcLink(ScreenState *s) {
  sceneFm(s);
  s->pcMark = true;
}

/* A hold of ENTER, mid confirmation: the name line says so. */
static void sceneFmLogged(ScreenState *s) {
  sceneFm(s);
  s->logConfirm = fmt(STR_RADIO_FMT_LOGGED, "106.40");
}

/* The update check running: the line under the panel says so in place of
 * the radio text. */
static void sceneFmCheckingUpdates(ScreenState *s) {
  sceneFm(s);
  s->notice = txt(STR_RADIO_CHECKING_UPDATES);
}

/* The longest name the stitching can produce. */
static void sceneFmLongName(ScreenState *s) {
  sceneFm(s);
  s->frequency = "95.00";
  s->sweepKHz = 95000;
  s->stationName = "MIRCHI 95 AND THEN SOME MORE TEXT";
  s->radioText = "MIRCHI 95.0MHZ";
  s->memory = NULL;
}

/* A station with no RDS at all: the name line reads "---" and the line under
 * the panel carries the date. */
static void sceneFmBare(ScreenState *s) {
  sceneFm(s);
  s->frequency = "101.90";
  s->sweepKHz = 101900;
  s->signalDbuV = 34;
  s->signalPercent = 0;
  s->signalValid = true;
  s->stationName = NULL;
  s->radioText = NULL;
  s->squelchMode = txt(STR_RADIO_SQL_OFF);
  s->filter = fmt(STR_RADIO_FMT_BW_K, 84u);
}

/* No RDS, but the channel is stored and a person named it. */
static void sceneFmMemory(ScreenState *s) {
  sceneFmBare(s);
  s->signalPercent = 56;
  s->memoryName = "MORNING COMMUTE";
}

/* The battery shown as volts: the upright battery, then the number. */
static void sceneFmVolts(ScreenState *s) {
  sceneFm(s);
  s->batteryText = fmt(STR_COMMON_FMT_VOLTS_TENTHS, 4u, 1u);
  s->batteryPercent = 85;
}

static void sceneFmVoltsLow(ScreenState *s) {
  sceneFm(s);
  s->batteryText = fmt(STR_COMMON_FMT_VOLTS_TENTHS, 3u, 4u);
  s->batteryPercent = 12;
}

/* Typing a frequency on the keypad, mid entry. */
static void sceneFmTyping(ScreenState *s) {
  sceneFm(s);
  s->typing = fmt(STR_RADIO_FMT_TYPED, "104");
}

/* The squelch holding the audio down on a shoulder next to a strong
 * station, which is what the measured 15 dBuV floor exists for. */
static void sceneFmSquelched(ScreenState *s) {
  sceneFmBare(s);
  s->signalDbuV = 12;
  s->signalPercent = 20;
  s->squelchMode = fmt(STR_RADIO_FMT_SQL_DB, 15);
  s->audio = SCREEN_AUDIO_SQUELCHED;
}

/* No network, so no clock and no date, and nothing names the station. */
static void sceneFmNoClock(ScreenState *s) {
  sceneFmBare(s);
  s->wifi = SCREEN_WIFI_NONE;
  s->memory = NULL;
  s->clock = NULL;
  s->date = NULL;
}

/* Touch Off: no menu symbol, and the band name back at the margin. */
static void sceneFmTouchOff(ScreenState *s) {
  sceneFm(s);
  s->menuMark = false;
}

/* A shortwave scene tuned to `khz`: the frequency, the scale and the metre
 * band all from the one number, through core's own rules, so a scene cannot
 * show a metre band the radio would not. */
static void tuneSw(ScreenState *s, uint32_t khz) {
  static char frequency[16];
  static char meterBand[8];
  s->frequency = bandFormatFrequency(BAND_SW, khz, frequency, sizeof(frequency))
                     ? frequency
                     : NULL;
  s->sweepKHz = khz;
  s->meterBand = swMeterBandFormat(BAND_SW, khz, meterBand, sizeof(meterBand))
                     ? meterBand
                     : NULL;
}

/* Shortwave, on a stored channel a person named. The battery is a percentage,
 * like every other scene: it is read once at boot, so a voltage would read as a
 * live measurement of something that has not moved since the radio came on. */
static void sceneSw(ScreenState *s) {
  s->band = txt(STR_BAND_SW);
  s->wifi = SCREEN_WIFI_JOINED;
  s->wifiBars = 2;
  s->batteryValid = true;
  s->batteryPercent = 76;
  s->batteryText = NULL;
  s->tuneMode = tuneModeShort(TUNE_MODE_AUTO);
  s->squelchMode = txt(STR_COMMON_AUTO);
  s->filter = fmt(STR_RADIO_FMT_BW_K, 4u);
  s->volume = fmt(STR_RADIO_FMT_VOL_DB, -6);
  tuneSw(s, 9420);
  s->sweepLowKHz = 1700;
  s->sweepSpanKHz = 25300;
  s->unit = txt(STR_COMMON_UNIT_KHZ);
  s->signalDbuV = 41;
  s->signalValid = true;
  s->signalPercent = 91;
  s->modulationValid = true;
  s->modulationPercent = 41;
  s->modulationPeakValid = true;
  s->modulationPeakPercent = 58;
  s->memory = fmt(STR_RADIO_FMT_MEMORY_SLOT, 7);
  s->memoryName = "RADIO ROMANIA";
  s->clock = "05:09";
  s->date = fmt(STR_DATE_FMT_LINE, txt(STR_DAY_WEDNESDAY), 30,
                txt(STR_DATE_SUFFIX_TH), txt(STR_MONTH_SEPTEMBER), 2026);
  s->tunerReady = true;
}

/* Shortwave at the very top of the band, where the scale's two ends meet. Both
 * 27000 and 1700 are long marks, so this is the case the dummy gap is for. */
static void sceneSwTop(ScreenState *s) {
  sceneSw(s);
  /* Above 11 m, the last metre band, so the header shows none. */
  tuneSw(s, 27000);
}

/* Medium wave, muted, on a nearly flat battery, trying to join a network:
 * the design's third state. */
static void sceneMw(ScreenState *s) {
  s->band = txt(STR_BAND_MW);
  s->frequency = "738";
  s->sweepLowKHz = 522;
  s->sweepSpanKHz = 1188;
  s->sweepKHz = 738;
  s->unit = txt(STR_COMMON_UNIT_KHZ);
  s->signalDbuV = 22;
  s->signalValid = true;
  s->signalPercent = 37;
  s->modulationValid = true;
  s->modulationPercent = 28;
  s->modulationPeakValid = true;
  s->modulationPeakPercent = 45;
  s->audio = SCREEN_AUDIO_MUTED;
  s->wifi = SCREEN_WIFI_NONE;
  s->batteryValid = true;
  s->batteryPercent = 12;
  s->batteryText = NULL;
  s->tuneMode = tuneModeShort(TUNE_MODE_MEMORY);
  s->squelchMode = txt(STR_RADIO_SQL_OFF);
  s->filter = fmt(STR_RADIO_FMT_BW_K, 6u);
  s->volume = fmt(STR_RADIO_FMT_VOL_DB, -6);
  s->clock = "05:09";
  s->date = fmt(STR_DATE_FMT_LINE, txt(STR_DAY_WEDNESDAY), 30,
                txt(STR_DATE_SUFFIX_TH), txt(STR_MONTH_SEPTEMBER), 2026);
  s->tunerReady = true;
}

/* The DX key or a BAND hold on medium wave: the name line says why nothing
 * opened. */
static void sceneMwFmFirst(ScreenState *s) {
  sceneMw(s);
  s->logConfirm = txt(STR_MENU_NOTE_SWITCH_TO_FM);
}

/* ENTER on a typed number that no band holds. */
static void sceneFmNoBand(ScreenState *s) {
  sceneFm(s);
  s->logConfirm = fmt(STR_RADIO_FMT_NO_BAND, "123");
}

/* Medium wave on 10 kHz spacing, where the band ends at 1710. */
static void sceneMw10k(ScreenState *s) {
  sceneMw(s);
  s->sweepLowKHz = 520;
  s->sweepSpanKHz = 1190;
  s->wifi = SCREEN_WIFI_TRYING;
}

/* OIRT, the eastern European FM band, on the same layout as FM. */
static void sceneOirt(ScreenState *s) {
  sceneFm(s);
  s->band = txt(STR_BAND_OIRT);
  s->frequency = "70.30";
  s->sweepLowKHz = 65800;
  s->sweepSpanKHz = 8200;
  s->sweepKHz = 70300;
  s->stationName = "RADIO 1";
  s->signalDbuV = 29;
  s->signalPercent = 48;
  s->modulationValid = true;
  s->modulationPercent = 55;
  s->modulationPeakValid = true;
  s->modulationPeakPercent = 70;
  s->memory = NULL;
  s->batteryPercent = 64;
  s->batteryText = NULL;
  s->tuneMode = tuneModeShort(TUNE_MODE_AUTO);
  s->squelchMode = txt(STR_RADIO_SQL_OFF);
  s->volume = fmt(STR_RADIO_FMT_VOL_DB, -4);
  s->radioText = "RADIO 1 - THE MORNING SHOW";
  s->wifi = SCREEN_WIFI_AP;
}

/* Nothing readable at all: no signal, no clock, no name, no battery. */
static void sceneNothing(ScreenState *s) {
  s->fm = true;
  s->band = txt(STR_BAND_FM);
  s->frequency = "88.00";
  s->sweepLowKHz = 87500;
  s->sweepSpanKHz = 20500;
  s->sweepKHz = 88000;
  s->unit = txt(STR_COMMON_UNIT_MHZ);
  s->signalValid = false;
  s->tuneMode = tuneModeShort(TUNE_MODE_MANUAL);
  s->squelchMode = txt(STR_RADIO_SQL_OFF);
  s->filter = txt(STR_RADIO_BW_DYN);
  s->volume = fmt(STR_RADIO_FMT_VOL_DB, -6);
  s->tunerReady = true;
}

/*
 * The smallest target a touch is given: every tap measured on this glass
 * landed inside a target 28 pixels tall, and all but one inside one 38 wide.
 */
#define ZONE_MIN_W 38
#define ZONE_MIN_H 28

/* The run ends with a failure when two of a screen's zones share a pixel,
 * when one is smaller than the targets measured to hold a tap, or when one
 * reaches past the panel. `name` says which screen. */
static void checkZones(const TouchZone *zones, int n, const char *name) {
  bool bad = !touchZonesValid(zones, n);
  for (int i = 0; i < n; i++) {
    const TouchZone *z = &zones[i];
    if (z->w < ZONE_MIN_W || z->h < ZONE_MIN_H || z->x < 0 || z->y < 0 ||
        z->x + z->w > W || z->y + z->h > H) {
      fprintf(stderr, "%s: zone %d at %d,%d is %d by %d\n", name, z->id, z->x,
              z->y, z->w, z->h);
      bad = true;
    }
  }
  if (bad) {
    fprintf(stderr, "%s: the zones overlap or are too small\n", name);
    exit(1);
  }
}

/* A screen's touch zones, checked, then outlined over what is on the panel
 * now and written to `path`. */
static void saveZones(const TouchZone *zones, int n, const char *path) {
  checkZones(zones, n, path);
  /* The outlines are drawn on a copy of the frame and the frame put back,
   * since the next picture only redraws what changed. */
  static uint16_t keep[W * H];
  memcpy(keep, sFrame, sizeof(keep));
  for (int i = 0; i < n; i++) {
    const TouchZone *z = &zones[i];
    const uint16_t magenta = 0xF81F;
    for (int x = z->x; x < z->x + z->w && x < W; x++) {
      sFrame[z->y * W + x] = magenta;
      sFrame[(z->y + z->h - 1) * W + x] = magenta;
    }
    for (int y = z->y; y < z->y + z->h && y < H; y++) {
      sFrame[y * W + z->x] = magenta;
      sFrame[y * W + z->x + z->w - 1] = magenta;
    }
  }
  if (!writeBmp(path)) {
    fprintf(stderr, "could not write %s\n", path);
    exit(1);
  }
  printf("  %s\n", path);
  memcpy(sFrame, keep, sizeof(keep));
}

static void render(void (*scene)(ScreenState *), const char *path) {
  ScreenState s;
  memset(&s, 0, sizeof(s));
  s.menuMark = true; /* Touch On, as on a new radio. */
  scene(&s);
  screenShow(&s);
  saveShot("%s", path);
}

/* ------------------------------------------------ scenes from real captures */

/*
 * The scenes above are made by hand, to put a layout's edge cases on the
 * screen: the longest name, a reading at its widest. These are built from
 * readings taken off the radio, and go through screenStateBuild, the code the
 * radio itself runs to turn its snapshot into a screen. So a mistake in how
 * the state is built shows in these pictures, not only on the radio.
 *
 * FM only. The AM layout is covered by the hand made scenes. The FM sweep has
 * no filter width, which the builder leaves out rather than printing as 0, so
 * these scenes have no BW value.
 */

/* Three channels from a band sweep taken off the radio, as the tuner
 * reported them. */
static const struct {
  uint32_t khz;
  int16_t levelTenths;
  uint16_t usnTenths;
  uint16_t multipathTenths;
  int16_t offsetTenths;
  int16_t modulation;
  bool stereo;
  int8_t snr;
} kSweepRows[] = {
    {93500, 330, 14, 35, 59, 79, true, 25},
    {105300, -5, 205, 259, 130, 19, false, 9},
    {106400, 367, 23, 32, 84, 57, true, 26},
};

/* One FM channel from that sweep. False when the sweep has no row for it. */
static bool captureFmQuality(uint32_t khz, Tef668xQuality *q) {
  for (size_t i = 0; i < sizeof(kSweepRows) / sizeof(kSweepRows[0]); i++) {
    if (kSweepRows[i].khz != khz) {
      continue;
    }
    memset(q, 0, sizeof(*q));
    q->levelDbuVTenths = kSweepRows[i].levelTenths;
    q->usnTenths = kSweepRows[i].usnTenths;
    q->multipathTenths = kSweepRows[i].multipathTenths;
    q->offsetKHzTenths = kSweepRows[i].offsetTenths;
    q->modulationPercent = kSweepRows[i].modulation;
    q->stereo = kSweepRows[i].stereo;
    q->snrDb = kSweepRows[i].snr;
    return true;
  }
  return false;
}

/* Every group of an RDS capture, fed through the decoder in order, the way
 * the radio task feeds it. The captures are the ones the RDS tests replay;
 * `weak` picks the one taken with a weak signal. False when there is none for
 * the channel: a scene drawn without it would look like a station that sends
 * no RDS. */
static bool captureRds(uint32_t khz, bool weak, RdsInfo *out) {
  static Rds rds;
  rdsReset(&rds, khz);
  for (size_t c = 0; c < sizeof(kCaptures) / sizeof(kCaptures[0]); c++) {
    const Capture *cap = &kCaptures[c];
    if (cap->khz != khz || (strstr(cap->name, "-weak") != NULL) != weak) {
      continue;
    }
    for (uint16_t n = 0; n < cap->count; n++) {
      const CaptureGroup *g = &cap->groups[n];
      RdsRead read;
      memset(&read, 0, sizeof(read));
      /* At the standard's 11.4 groups a second, so the decoder's last minute
       * holds what a minute on air would. */
      read.atMs = (uint32_t)(n * 1000u / 11.4);
      read.synchronised = true;
      read.haveGroup = true;
      for (int i = 0; i < 4; i++) {
        read.block[i] = g->block[i];
        read.error[i] = (uint8_t)((g->error >> (6 - 2 * i)) & 3u);
      }
      rdsFeed(&rds, &read);
    }
    *out = rds.info;
    return true;
  }
  return false;
}

/*
 * A made up station that sends every field the RDS pages show, written by
 * hand, since none of the captures has all of them. For layout only: PS, the PTY, TP,
 * the radio text and its RT+ tags, the alternative frequencies, three other
 * networks, a language and a country, and a last minute of groups shaped
 * like a station that sends all of it.
 */
static void rdsExampleStation(RdsInfo *r) {
  memset(r, 0, sizeof(*r));
  r->synchronised = true;
  r->hasPi = true;
  r->pi = 0xC241;
  r->hasPs = true;
  snprintf(r->ps, sizeof(r->ps), "%s", "RADIO 1 ");
  r->hasPty = true;
  r->pty = 10;
  r->hasPtyn = true;
  snprintf(r->ptyn, sizeof(r->ptyn), "%s", "POP HITS");
  r->hasEcc = true;
  r->ecc = 0xE1;
  r->hasLanguage = true;
  r->language = 0x09;
  r->clock.valid = true;
  r->clock.hour = 21;
  r->clock.minute = 4;
  r->hasFlags = true;
  r->tp = true;
  r->ta = false;
  r->speech = true; /* Speech, the wider of its two words. */
  r->hasDiStereo = true;
  r->diStereo = true;
  r->hasRt = true;
  snprintf(r->rt, sizeof(r->rt), "%s",
           "NOW PLAYING: DREAMS BY FLEETWOOD MAC ON RADIO 1, YOUR HIT MUSIC");
  r->rtPlus = true;
  r->rtPlusRunning = true;
  r->rtPlusCount = 3;
  r->rtPlusTag[0] = {33, 54, 9}; /* PROGRAM, HIT MUSIC. */
  r->rtPlusTag[1] = {4, 23, 13}; /* ARTIST, FLEETWOOD MAC. */
  r->rtPlusTag[2] = {1, 13, 6};  /* TITLE, DREAMS. */
  const uint8_t af[4] = {6, 102, 113, 174};
  r->afCount = 4;
  for (int i = 0; i < 4; i++) {
    r->afKHz[i] = rdsAfCodeKHz(af[i]);
  }
  const struct {
    uint16_t pi;
    const char *ps;
    uint8_t af[2];
    uint8_t afCount;
    bool ta;
  } eon[3] = {
      {0xC242, "RADIO 2 ", {24, 31}, 2, false},
      {0xC243, "RADIO 3 ", {38, 0}, 1, false},
      {0xC204, "TRAFFIC ", {146, 0}, 1, true},
  };
  r->eonCount = 3;
  for (int i = 0; i < 3; i++) {
    RdsEon *e = &r->eon[i];
    e->pi = eon[i].pi;
    e->heard = 9;
    e->hasPs = true;
    snprintf(e->ps, sizeof(e->ps), "%s", eon[i].ps);
    e->hasTa = true;
    e->ta = eon[i].ta;
    e->afCount = eon[i].afCount;
    e->afCode[0] = eon[i].af[0];
    e->afCode[1] = eon[i].af[1];
  }
  RdsMinute *m = &r->minute;
  m->groups = 684;
  m->spanMs = 60000;
  for (int b = 0; b < 4; b++) {
    m->blocks[b][RDS_LEVEL_CLEAN] = 684;
  }
  m->types[0][0] = 274;
  m->types[2][0] = 205;
  m->types[11][0] = 68;
  m->types[14][0] = 68;
  m->types[3][0] = 35;
  m->types[1][0] = 34;
}

/*
 * The example station with every field as long as it can be: twelve
 * alternative frequencies, four other networks, the longest RT+ label, a
 * tenth of every block lost, and a minute only twelve seconds old. What
 * this checks is that nothing runs off the screen or over its neighbour,
 * and that what does not fit is counted.
 */
static void rdsLongStation(RdsInfo *r) {
  rdsExampleStation(r);
  r->hasPs = true;
  snprintf(r->ps, sizeof(r->ps), "%s", "WWWWWWWW");
  r->hasEcc = true;
  r->ecc = 0xE3;
  r->hasPi = true;
  r->pi = 0x5241;
  r->afCount = 12;
  for (int i = 0; i < 12; i++) {
    r->afKHz[i] = rdsAfCodeKHz((uint8_t)(170 + i * 3));
  }
  r->rtPlusTag[1] = {60, 23, 13}; /* APPOINTMENT, FLEETWOOD MAC. */
  r->eonCount = 4;
  r->eon[3] = r->eon[0];
  r->eon[3].pi = 0xC245;
  snprintf(r->eon[1].ps, sizeof(r->eon[1].ps), "%s", "WWWWWWWW");
  r->eonMore = true;
  r->eon[0].afMore = true;
  r->eon[0].afCount = 4;
  r->eon[0].afCode[2] = 200;
  r->eon[0].afCode[3] = 204;
  RdsMinute *m = &r->minute;
  m->groups = 684;
  m->spanMs = 12000;
  for (int b = 0; b < 4; b++) {
    m->blocks[b][RDS_LEVEL_CLEAN] = 400;
    m->blocks[b][RDS_LEVEL_CORRECTED] = 253;
    m->blocks[b][RDS_LEVEL_LOST] = 31;
  }
}

/* No channels stored: the capture scenes are not on a memory channel. */
static bool captureNoChannel(int slot, MemoryChannel *out) {
  (void)slot;
  (void)out;
  return false;
}

/* The RDS decoder fed the first `groups` groups heard on a channel. */
static void feedDx(Rds *rds, const DxChannel *c, uint16_t groups) {
  rdsReset(rds, c->khz);
  for (uint16_t i = 0; i < groups && i < c->groupCount; i++) {
    RdsRead r;
    memset(&r, 0, sizeof(r));
    r.synchronised = true;
    r.haveGroup = true;
    for (int b = 0; b < 4; b++) {
      r.block[b] = c->groups[i].block[b];
      r.error[b] = (uint8_t)((c->groups[i].error >> (6 - b * 2)) & 0x03);
    }
    rdsFeed(rds, &r);
  }
}

/* The snapshot on a captured channel: its settled reading, and the RDS
 * decoder fed the first `groups` groups heard on it. */
static void snapFor(const DxChannel *c, uint16_t groups, RadioSnapshot *snap) {
  memset(snap, 0, sizeof(*snap));
  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  radioDefaults(&snap->settings, &plan);
  snap->settings.band = BAND_FM;
  snap->settings.freqKHz = c->khz;
  snap->qualityValid = true;
  snap->quality.levelDbuVTenths = c->tuned.levelTenths;
  snap->quality.usnTenths = c->tuned.noiseTenths;
  snap->quality.multipathTenths = c->tuned.multipathTenths;
  snap->quality.offsetKHzTenths = c->tuned.offsetTenths;
  snap->quality.bandwidthKHz = c->bandwidthKHz;
  snap->quality.modulationPercent = c->modulation;
  snap->quality.stereo = c->stereo;
  static Rds rds;
  feedDx(&rds, c, groups);
  snap->rds = rds.info;
}

/*
 * One DX page from the DX captures the unit tests carry, through the real view
 * builder: the snapshot is the channel's settled reading, the RDS decoder is
 * fed the first `groups` groups heard on it, and the history is every
 * reading taken while it listened, a second and a half apart as they were.
 * The clock reads 07:17, local time at +05:30 when the captures were taken.
 */
static void renderDx(const char *dir, const char *name, const DxChannel *c,
                     uint16_t groups, RdsRegion region = RDS_REGION_EUROPE,
                     uint16_t pi = 0, int16_t presetSlot = MEMORY_NO_SLOT,
                     uint16_t presetPi = 0) {
  static RadioSnapshot snap;
  snapFor(c, groups, &snap);
  snap.memorySlot = presetSlot;
  /* A PI put in place of the capture's, for a region this radio never
   * hears. */
  if (pi != 0) {
    snap.rds.pi = pi;
  }

  static ScreenDxKeep keep;
  screenDxStateReset(&keep);
  ScreenDxInputs in;
  memset(&in, 0, sizeof(in));
  in.snap = &snap;
  in.rdsEnabled = true;
  in.clock = "07:17";
  in.pages = SCREEN_DX_PAGES;
  in.region = region;
  in.presetPi = presetPi;
  uint32_t ms = 1000000;
  static ScreenDx view;
  for (uint16_t q = 0; q < c->readingCount; q++) {
    snap.quality.levelDbuVTenths = c->readings[q].levelTenths;
    in.nowMs = ms;
    screenDxStateBuild(&in, &keep, &view);
    ms += 1500;
  }
  snap.quality.levelDbuVTenths = c->tuned.levelTenths;
  in.nowMs = ms;
  screenDxStateBuild(&in, &keep, &view);
  screenDxShow(&view);
  saveShot("%s/dx-%s.bmp", dir, name);
}

/*
 * The Catches page, from the band sweep the DX unit tests carry:
 * every channel whose PI `dxPiConfirmed` takes, added to the list in the
 * order the sweep reached it, through the real list and view builder. The
 * sweep ran from 01:46 UTC for 23 minutes over 206 channels, so
 * each catch gets the time its channel was reached. The first `fresh` are
 * marked NEW, as if the rest had been caught on an earlier night.
 */
static void buildCatches(DxCatches *list, uint8_t fresh) {
  dxCatchesReset(list);
  static Rds rds;
  uint8_t added = 0;
  for (size_t i = 0; i < DX_SWEEP_COUNT; i++) {
    const DxChannel *c = &kDxSweep[i];
    feedDx(&rds, c, UINT16_MAX);
    SeekReading reading;
    memset(&reading, 0, sizeof(reading));
    reading.valid = true;
    reading.offsetTenths = c->tuned.offsetTenths;
    if (!dxPiConfirmed(&rds.info, &reading)) {
      continue;
    }
    DxHearing h;
    memset(&h, 0, sizeof(h));
    h.khz = c->khz;
    h.band = (uint8_t)BAND_FM;
    h.pi = rds.info.pi;
    h.ps = rds.info.hasPs ? rds.info.ps : NULL;
    h.country =
        rds.info.hasEcc ? rdsCountryCode(rds.info.pi, rds.info.ecc) : NULL;
    h.readings.levelDbuVTenths = c->tuned.levelTenths;
    h.readings.usnTenths = c->tuned.noiseTenths;
    h.readings.multipathTenths = c->tuned.multipathTenths;
    h.readings.bandwidthKHz = c->bandwidthKHz;
    h.readings.stereo = c->stereo;
    h.at.known = true;
    h.at.value = 1790473560u + (uint32_t)(i * 1380u / DX_SWEEP_COUNT);
    (void)dxCatchesAdd(list, &h, added < fresh);
    added++;
  }
}

/* The UTC offset the DX captures were taken at, +05:30, so the Catches
 * times read as the radio showed them. */
static const int16_t kCaptureOffsetMinutes = 330;

static void renderCatches(const char *dir, const char *name,
                          const DxCatches *list, uint8_t cursor,
                          const char *confirm) {
  static ScreenCatchesKeep keep;
  static ScreenCatches view;
  ScreenCatchesInputs in;
  memset(&in, 0, sizeof(in));
  in.list = list;
  in.cursor = cursor;
  in.page = SCREEN_DX_PAGE_CATCHES;
  in.pages = SCREEN_DX_PAGES;
  in.clock = "07:39";
  in.offsetMinutes = kCaptureOffsetMinutes;
  in.confirm = confirm;
  screenCatchesStateBuild(&in, &keep, &view);
  screenCatchesShow(&view);
  saveShot("%s/dx-%s.bmp", dir, name);
}

static const DxChannel *dxChannel(const DxChannel *list, size_t count,
                                  uint32_t khz);

/* A pass of the settle capture as the level sweep takes it: the mean of the
 * four readings on each channel. */
static void sweepFromCapture(DxSweep *s, const int16_t (*pass)[CAPTURE_READS],
                             uint32_t at) {
  memset(s, 0, sizeof(*s));
  s->timeKnown = true;
  s->at = at;
  s->lowKHz = CAPTURE_LOW_KHZ;
  s->stepKHz = CAPTURE_STEP_KHZ;
  s->count = CAPTURE_COUNT;
  s->widthKHz = 114;
  for (uint16_t i = 0; i < CAPTURE_COUNT; i++) {
    s->level[i] = dxSweepMean(pass[i], CAPTURE_READS);
  }
}

static void renderScopeView(const char *dir, const char *name,
                            const ScreenScopeInputs *in) {
  static ScreenScopeKeep keep;
  ScreenScope view;
  /* The header's marks follow the same Touch state as the foot row. */
  uiSetTouchMarks(in->touchOn);
  screenScopeStateBuild(in, &keep, &view);
  screenScopeShow(&view);
  saveShot("%s/%s.bmp", dir, name);
  uiSetTouchMarks(true);
}

/*
 * The Scope page, through the real view builder over the two passes of the
 * settle capture, the second the latest sweep and the first the one it is held
 * against, taken three minutes before. The dial on 106.4; the cursor on 98.3, a
 * station, and on 99.5, an empty channel; sweeping; the first sweep, with
 * nothing to hold it against; and before any sweep.
 */
static void renderScopes(const char *dir) {
  static DxSweep live;
  static DxSweep earlier;
  static DxSweep base;
  static DxSweep peak;
  const uint32_t now = 1790511689u;
  sweepFromCapture(&live, kPass1, now - 180);
  sweepFromCapture(&earlier, kPass0, now - 480);
  const uint8_t baseN = dxSweepMedian(&earlier, 1, &live, &base);
  memset(&peak, 0, sizeof(peak));
  dxSweepPeak(&peak, &earlier);
  dxSweepPeak(&peak, &live);

  ScreenScopeInputs in;
  memset(&in, 0, sizeof(in));
  in.live = &live;
  in.base = &base;
  in.baseN = baseN;
  in.peak = &peak;
  in.revision = 1;
  in.cursor = (uint16_t)dxSweepChannelOf(&live, 98300);
  in.dialKHz = 106400;
  in.nowKnown = true;
  in.nowUtc = now;
  in.page = SCREEN_DX_PAGE_SCOPE;
  in.pages = SCREEN_DX_PAGES;
  in.clock = "17:43";
  renderScopeView(dir, "dx-scope", &in);
  {
    /* With Touch On: the buttons in the foot row, the rise up in the chart,
     * and the touch zones over it; then the same while a sweep runs. */
    in.touchOn = true;
    renderScopeView(dir, "dx-scope-touch", &in);
    char path[512];
    TouchZone zones[TOUCH_ZONES_MAX];
    snprintf(path, sizeof(path), "%s/touch-scope.bmp", dir);
    saveZones(zones, screenScopeZones(zones, TOUCH_ZONES_MAX), path);
    in.sweeping = true;
    in.revision++;
    renderScopeView(dir, "dx-scope-touch-sweeping", &in);
    in.sweeping = false;
    in.touchOn = false;
    in.revision++;
  }
  uiSetSleepMark(UI_SLEEP_SOON);
  renderScopeView(dir, "dx-scope-sleep", &in);
  uiSetSleepMark(UI_SLEEP_NONE);
  in.cursor = (uint16_t)dxSweepChannelOf(&live, 99500);
  in.revision++;
  renderScopeView(dir, "dx-scope-empty-channel", &in);
  in.cursor = (uint16_t)dxSweepChannelOf(&live, 98300);
  in.sweeping = true;
  in.revision++;
  renderScopeView(dir, "dx-scope-sweeping", &in);
  in.sweeping = false;
  in.base = NULL;
  in.baseN = 0;
  in.peak = NULL;
  in.revision++;
  renderScopeView(dir, "dx-scope-first", &in);
  in.live = NULL;
  in.revision++;
  renderScopeView(dir, "dx-scope-none", &in);
}

/* The channels of `s` that the `n` frequencies at `khz` fall on, into `out`;
 * returns how many. */
static uint8_t channelsOf(const DxSweep *s, const uint32_t *khz, size_t n,
                          uint16_t *out) {
  uint8_t found = 0;
  for (size_t i = 0; i < n; i++) {
    const int16_t at = dxSweepChannelOf(s, khz[i]);
    if (at >= 0) {
      out[found++] = (uint16_t)at;
    }
  }
  return found;
}

/*
 * The band scope, the same page over the radio screen, on the same sweep: the
 * whole band with the stored channels and two DX catches marked, then the span
 * round the dial, cut from it the way a span sweep reads it, then the same
 * with Touch On.
 */
static void renderBandScopes(const char *dir) {
  static DxSweep whole;
  static DxSweep span;
  const uint32_t now = 1790511689u;
  sweepFromCapture(&whole, kPass1, now - 60);
  static const uint32_t kPresetKHz[] = {91100,  93500,  94300, 98300,
                                        101900, 102800, 106400};
  static const uint32_t kCatchKHz[] = {96000, 104000, 107800};
  const size_t presets = sizeof(kPresetKHz) / sizeof(kPresetKHz[0]);
  const size_t caught = sizeof(kCatchKHz) / sizeof(kCatchKHz[0]);
  uint16_t marks[presets];
  uint16_t catches[caught];
  ScreenScopeInputs in;
  memset(&in, 0, sizeof(in));
  in.live = &whole;
  in.revision = 1;
  in.cursor = (uint16_t)dxSweepChannelOf(&whole, 98300);
  in.dialKHz = 106400;
  in.nowKnown = true;
  in.nowUtc = now;
  in.clock = "17:43";
  in.title = txt(STR_SCOPE_TITLE_FM);
  in.position = txt(STR_SCOPE_FULL);
  in.marks = marks;
  in.markCount = channelsOf(&whole, kPresetKHz, presets, marks);
  in.catches = catches;
  in.catchCount = channelsOf(&whole, kCatchKHz, caught, catches);
  renderScopeView(dir, "band-scope", &in);

  BandPlanConfig plan;
  bandPlanDefaults(&plan);
  DxSweepRange r;
  if (!dxSweepRange(BAND_FM, &plan, 106400, 3600, &r)) {
    return;
  }
  span = whole;
  span.lowKHz = r.lowKHz;
  span.stepKHz = r.stepKHz;
  span.count = r.count;
  for (uint16_t i = 0; i < r.count; i++) {
    const int16_t at = dxSweepChannelOf(&whole, r.lowKHz + i * r.stepKHz);
    span.level[i] = at >= 0 ? whole.level[at] : DX_SWEEP_NO_READING;
  }
  in.markCount = channelsOf(&span, kPresetKHz, presets, marks);
  in.catchCount = channelsOf(&span, kCatchKHz, caught, catches);
  in.live = &span;
  in.span = true;
  char spanText[16];
  snprintf(spanText, sizeof(spanText), txt(STR_SCOPE_FMT_SPAN), 3ul, 6ul);
  in.position = spanText;
  in.cursor = (uint16_t)dxSweepChannelOf(&span, 106400);
  in.revision++;
  renderScopeView(dir, "band-scope-span", &in);
  in.touchOn = true;
  in.revision++;
  renderScopeView(dir, "band-scope-touch", &in);
}

/* The sweep a fresh scan starts with, running. */
static bool sScanSweeping = false;

static void renderScanView(const char *dir, const char *name,
                           const DxScan *scan, const RadioSnapshot *snap,
                           const DxCatches *catches, uint32_t nowMs) {
  static ScreenScanKeep keep;
  static ScreenScan view;
  ScreenScanInputs in;
  memset(&in, 0, sizeof(in));
  in.scan = scan;
  in.snap = snap;
  in.catches = catches;
  in.range = DX_RANGE_BAND_LESS_MEMORY;
  in.memFirst = 1;
  in.memLast = MEMORY_SLOT_COUNT;
  in.stop = DX_STOP_NEW;
  in.sweeping = sScanSweeping;
  in.dwellMs = DX_SCAN_DWELL_MS;
  in.lowKHz = 87500;
  in.highKHz = 108000;
  in.page = SCREEN_DX_PAGE_SCAN;
  in.pages = SCREEN_DX_PAGES;
  in.clock = "07:17";
  in.nowMs = nowMs;
  screenScanStateBuild(&in, &keep, &view);
  screenScanShow(&view);
  saveShot("%s/dx-%s.bmp", dir, name);
}

/*
 * The Scanner page, through the real scanner and view builder over the band
 * sweep the DX unit tests carry. Each channel's PI is taken as `dxPiConfirmed`
 * gives it from every group the sweep heard there, about 3 s of them, not as
 * the firmware's `dxPiHeardNow` does inside a 2.5 s dwell, so these show the
 * page's layout and states, not which channels a scan would stop on. Every PI
 * but 98.3's counts as caught on an earlier night. Idle; running on 96.4, 0.8 s
 * into its dwell, having gone past 93.5 at once on its known PI; and stopped on
 * 98.3, whose PI is caught for the first time.
 */
static void renderScans(const char *dir) {
  static RadioSnapshot snap;
  static DxScan scan;
  static DxCatches catches;
  dxCatchesReset(&catches);
  snapFor(dxChannel(kDxSweep, DX_SWEEP_COUNT, 106400), 0, &snap);
  dxScanReset(&scan);
  renderScanView(dir, "scan-idle", &scan, &snap, &catches, 0);

  uint32_t now = 0;
  static DxScanBand walk = {87500, 100, NULL, NULL};
  DxScanPlan plan;
  memset(&plan, 0, sizeof(plan));
  plan.count = dxScanBandCount(87500, 108000, 100);
  plan.channel = dxScanBandChannel;
  plan.ctx = &walk;
  plan.dwellMs = DX_SCAN_DWELL_MS;
  (void)dxScanStart(&scan, &plan, 106400);
  /* Started, its first channel waiting for the sweep. */
  sScanSweeping = true;
  renderScanView(dir, "scan-sweeping", &scan, &snap, &catches, 0);
  sScanSweeping = false;
  const uint32_t runAt = 96400;
  const uint32_t stopAt = 98300;
  while (scan.state == DX_SCAN_RUNNING) {
    const DxChannel *c = dxChannel(kDxSweep, DX_SWEEP_COUNT, scan.atKHz);
    snapFor(c, UINT16_MAX, &snap);
    SeekReading reading;
    memset(&reading, 0, sizeof(reading));
    reading.valid = true;
    reading.offsetTenths = c->tuned.offsetTenths;
    const bool heard = dxPiConfirmed(&snap.rds, &reading);
    (void)dxScanPoll(&scan, now, c->khz, false, false,
                     false); /* The dial lands. */
    if (c->khz == runAt) {
      static RadioSnapshot early;
      snapFor(c, 0, &early);
      renderScanView(dir, "scan-run", &scan, &early, &catches, now + 800);
      {
        /* The page's touch zones over it. */
        char path[512];
        TouchZone zones[TOUCH_ZONES_MAX];
        snprintf(path, sizeof(path), "%s/touch-scan.bmp", dir);
        saveZones(zones, screenScanZones(zones, TOUCH_ZONES_MAX), path);
      }
    }
    /* Caught on an earlier night, all but 98.3. */
    (void)dxScanPoll(&scan, now, c->khz, heard, false, c->khz == stopAt);
    if (scan.state == DX_SCAN_STOPPED) {
      if (c->khz == stopAt) {
        DxHearing h;
        memset(&h, 0, sizeof(h));
        h.khz = c->khz;
        h.band = (uint8_t)BAND_FM;
        h.pi = snap.rds.pi;
        h.readings.levelDbuVTenths = c->tuned.levelTenths;
        (void)dxCatchesAdd(&catches, &h, true);
        renderScanView(dir, "scan-stop", &scan, &snap, &catches, now);
        return;
      }
      (void)dxScanResume(&scan, c->khz);
      continue;
    }
    now += DX_SCAN_DWELL_MS;
    if (scan.state == DX_SCAN_RUNNING && scan.atKHz == c->khz) {
      (void)dxScanPoll(&scan, now, c->khz, false, false,
                       false); /* The dwell runs out. */
    }
  }
}

static const DxChannel *dxChannel(const DxChannel *list, size_t count,
                                  uint32_t khz) {
  for (size_t i = 0; i < count; i++) {
    if (list[i].khz == khz) {
      return &list[i];
    }
  }
  fprintf(stderr, "no DX capture for %lu kHz\n", (unsigned long)khz);
  exit(1);
}

/*
 * Build one scene from captures, the way screenTaskPoll does on the radio,
 * and draw it. The Wi-Fi strength, -52 dBm, and the battery, 3754 mV, are
 * readings taken off the radio. The clock and the date are the hand scenes'
 * own, so the two sets line up.
 */
static bool renderCapture(const char *path, uint32_t khz, bool withRds) {
  static RadioSnapshot snap;
  memset(&snap, 0, sizeof(snap));
  ScreenInputs in;
  memset(&in, 0, sizeof(in));
  bandPlanDefaults(&in.plan);
  in.planValid = true;
  radioDefaults(&snap.settings, &in.plan);
  snap.settings.band = BAND_FM;
  snap.settings.freqKHz = khz;
  snap.settings.stepKHz = bandDefaultStep(BAND_FM, &in.plan);
  if (!captureFmQuality(khz, &snap.quality)) {
    fprintf(stderr, "no capture for %lu kHz\n", (unsigned long)khz);
    return false;
  }
  snap.qualityValid = true;
  snap.qualityReads = 1;
  snap.levelSmoothedTenths = snap.quality.levelDbuVTenths;
  snap.levelSmoothedValid = true;
  if (withRds && !captureRds(khz, false, &snap.rds)) {
    fprintf(stderr, "no RDS capture for %lu kHz\n", (unsigned long)khz);
    return false;
  }
  snap.tunerReady = true;
  snap.squelchMode = SQUELCH_AUTO;
  snap.squelchOpen = true;
  snap.memorySlot = MEMORY_NO_SLOT;
  snap.lastError = TEF668X_OK;

  static Battery battery;
  batteryReset(&battery);
  batteryFeed(&battery, 3754, true);
  in.snap = &snap;
  in.readChannel = captureNoChannel;
  in.memoryGeneration = 1;
  in.wifi = SCREEN_WIFI_JOINED;
  in.rssiValid = true;
  in.rssiDbm = -52;
  in.battery = &battery;
  in.batteryShow = BATTERY_SHOW_PERCENT;
  in.clock = "05:09";
  in.touchOn = true;
  in.date = fmt(STR_DATE_FMT_LINE, txt(STR_DAY_WEDNESDAY), 30,
                txt(STR_DATE_SUFFIX_TH), txt(STR_MONTH_SEPTEMBER), 2026);
  in.nowMs = 0;

  static ScreenBuild build;
  screenStateReset(&build);
  ScreenState s;
  screenStateBuild(&build, &in, &s);
  screenShow(&s);
  saveShot("%s", path);
  return true;
}

int main(int argc, char **argv) {
  const char *dir = argc > 1 ? argv[1] : ".";

  lv_init();
  lv_draw_unit_t *glyphs =
      (lv_draw_unit_t *)lv_draw_create_unit(sizeof(lv_draw_unit_t));
  glyphs->evaluate_cb = checkGlyphs;
  glyphs->dispatch_cb = takeNothing;
  lv_tick_set_cb(tick);
  lv_display_t *d = lv_display_create(W, H);
  lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565_SWAPPED);
  lv_display_set_flush_cb(d, flush);
  lv_display_set_buffers(d, sDrawBuf, NULL, sizeof(sDrawBuf),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  /* Custom's colours as main.cpp gives them at start up, from the default
   * settings, so the theme list has its Custom row. */
  {
    Settings defaults;
    settingsDefaults(&defaults);
    themeSetCustomColours(defaults.customTheme);
  }
  /* Touch On, as on a new radio: the headers carry their touch marks. */
  uiSetTouchMarks(true);
  if (!screenBegin()) {
    fprintf(stderr, "screenBegin failed\n");
    return 1;
  }

  {
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    fprintf(stderr, "  lvgl pool %u total, %u free, biggest %u\n",
            (unsigned)m.total_size, (unsigned)m.free_size,
            (unsigned)m.free_biggest_size);
  }

  char path[512];
  struct {
    const char *name;
    void (*scene)(ScreenState *);
  } scenes[] = {
      {"fm", sceneFm},
      {"fm-logged", sceneFmLogged},
      {"fm-checking-updates", sceneFmCheckingUpdates},
      {"fm-no-band", sceneFmNoBand},
      {"mw-fm-first", sceneMwFmFirst},
      {"fm-longname", sceneFmLongName},
      {"fm-bare", sceneFmBare},
      {"fm-memory", sceneFmMemory},
      {"fm-typing", sceneFmTyping},
      {"fm-squelched", sceneFmSquelched},
      {"fm-volts", sceneFmVolts},
      {"fm-volts-low", sceneFmVoltsLow},
      {"fm-noclock", sceneFmNoClock},
      {"fm-touch-off", sceneFmTouchOff},
      {"sw", sceneSw},
      {"sw-top", sceneSwTop},
      {"mw", sceneMw},
      {"mw-10k", sceneMw10k},
      {"oirt", sceneOirt},
      {"nothing", sceneNothing},
  };
  for (size_t i = 0; i < sizeof(scenes) / sizeof(scenes[0]); i++) {
    snprintf(path, sizeof(path), "%s/%s.bmp", dir, scenes[i].name);
    render(scenes[i].scene, path);
    if (i == 0) {
      /* The radio screen's touch zones over the FM picture. */
      TouchZone zones[TOUCH_ZONES_MAX];
      const int n = screenRadioZones(zones, TOUCH_ZONES_MAX);
      snprintf(path, sizeof(path), "%s/touch-radio.bmp", dir);
      saveZones(zones, n, path);
    }
  }
  /* Auto off on: the sleep mark left of the speaker, grey, and in radio for
   * the last five minutes. */
  uiSetSleepMark(UI_SLEEP_ON);
  snprintf(path, sizeof(path), "%s/fm-auto-off.bmp", dir);
  render(sceneFm, path);
  uiSetSleepMark(UI_SLEEP_SOON);
  snprintf(path, sizeof(path), "%s/fm-sleep.bmp", dir);
  render(sceneFm, path);
  uiSetSleepMark(UI_SLEEP_NONE);
  /* A PC signed in over the PC Link: the laptop left of the Wi-Fi symbol. */
  snprintf(path, sizeof(path), "%s/fm-pc-link.bmp", dir);
  render(scenePcLink, path);

  /* FM with a station, FM bare and medium wave in every theme but Custom,
   * by saved index. The rebuild a theme change triggers is the same one the
   * radio does. */
  {
    static const char *const kThemeFile[THEME_COUNT] = {
        "nightwatch", "daylight", "redNight", "phosphor",
        "clear",      NULL,       "slate",    "paper",
        "lcd",        "ember",    "clearDay", "highContrast",
        "mono",       "hiFi",     "violet",   "blossom"};
    static const struct {
      const char *name;
      void (*scene)(ScreenState *);
    } kStates[] = {{"fm", sceneFm}, {"fm-bare", sceneFmBare}, {"mw", sceneMw}};
    for (uint8_t t = 0; t < THEME_COUNT; t++) {
      if (t == THEME_CUSTOM) {
        continue;
      }
      themeSet(t);
      for (size_t i = 0; i < sizeof(kStates) / sizeof(kStates[0]); i++) {
        snprintf(path, sizeof(path), "%s/theme-%s-%s.bmp", dir, kThemeFile[t],
                 kStates[i].name);
        render(kStates[i].scene, path);
      }
    }
    themeSet(0);
  }

  /* From captures, through the radio's own state builder. */
  struct {
    const char *name;
    uint32_t khz;
    bool rds;
  } captures[] = {
      {"cap-fm-106400", 106400, true},
      {"cap-fm-93500", 93500, true},
      {"cap-fm-105300", 105300, false},
  };
  for (size_t i = 0; i < sizeof(captures) / sizeof(captures[0]); i++) {
    snprintf(path, sizeof(path), "%s/%s.bmp", dir, captures[i].name);
    if (!renderCapture(path, captures[i].khz, captures[i].rds)) {
      return 1;
    }
  }

  /* The message, which is its own call rather than a state. */
  screenMessage(txt(STR_COMMON_TUNER), txt(STR_TUNER_NO_TUNER_ON_THE_I2C_BUS));
  saveShot("%s/message.bmp", dir);

  /* The moment before the radio sleeps, which is the last chance to say how
   * it wakes. */
  screenMessage(txt(STR_RADIO_GOING_TO_SLEEP),
                txt(STR_RADIO_PRESS_KNOB_TO_WAKE));
  saveShot("%s/sleeping.bmp", dir);

  /* What is left when a firmware write fails, in screen_task.cpp's words.
   * The write itself is the veil, veil-start.bmp and veil.bmp below. */
  screenMessage(txt(STR_RADIO_UPDATE_FAILED), txt(STR_RADIO_OLD_IMAGE_KEPT));
  saveShot("%s/update-failed.bmp", dir);

  /*
   * The boot screen, caught part way through, which is the only interesting
   * moment it has: two rows still waiting, one keypad that did not answer,
   * and a bar that is not full. A capture of the finished screen would be six
   * ticks and say nothing about what the screen is for.
   */
  ScreenBoot boot;
  memset(&boot, 0, sizeof(boot));
  boot.product = txt(STR_RADIO_PRODUCT_NAME);
  /* Built from the same two headers screen_task.cpp builds it from, rather
   * than written out here. These screens are captures of the real renderer,
   * so anything on them that names the firmware has to come from the
   * firmware's own source or it can say something the radio does not. */
  char version[40];
  snprintf(version, sizeof(version), "%s", BOARD_NAME_DISPLAY);
  boot.board = version;
  boot.tuner = fmt(STR_BOOT_FMT_TUNER_VERSION, "TEF6686", FIRMWARE_VERSION);
  static const StrId names[SCREEN_BOOT_STEPS] = {
      STR_BOOT_SETTINGS, STR_COMMON_TUNER,   STR_BOOT_RADIO, STR_BOOT_CHANNELS,
      STR_BOOT_KEYPAD,   STR_COMMON_BATTERY, STR_BOOT_TOUCH,
  };
  for (int i = 0; i < SCREEN_BOOT_STEPS; i++) {
    boot.steps[i].name = txt(names[i]);
    boot.steps[i].mark = SCREEN_BOOT_WAITING;
  }
  boot.steps[0].mark = SCREEN_BOOT_OK;
  boot.steps[1].mark = SCREEN_BOOT_OK;
  boot.steps[3].mark = SCREEN_BOOT_OK;
  boot.steps[3].value = "12";
  boot.steps[4].mark = SCREEN_BOOT_FAILED;
  boot.steps[5].mark = SCREEN_BOOT_OK;
  boot.steps[5].value = fmt(STR_COMMON_FMT_VOLTS, 3u, 91u);
  boot.steps[6].mark = SCREEN_BOOT_OK;
  boot.done = 6;
  boot.total = SCREEN_BOOT_STEPS;
  if (screenBootBegin()) {
    screenBootShow(&boot);
    saveShot("%s/boot.bmp", dir);
    screenBootEnd();
  }
  /* The menu, both levels. The group list says which groups have anything in
   * them, and the group screen is caught with an edit running, because the
   * one thing a person has to know before turning the knob is whether it
   * moves the cursor or changes the number. */
  screenEnd();
  if (screenMenuBegin()) {
    ScreenMenu menu;
    memset(&menu, 0, sizeof(menu));
    /* The menu screens take their texts from the same table menu_task.cpp
     * reads, so a renamed row renames here too. Which rows go in which
     * group, and the hints each screen sets, still follow that file by
     * hand: it needs the radio task and the settings to link. */
    menu.title = txt(STR_MENU_TITLE);
    static const StrId kNames[] = {STR_MENU_GO_TO,    STR_MENU_STATIONS,
                                   STR_MENU_AUDIO,    STR_MENU_FM_SETUP,
                                   STR_MENU_AM_SETUP, STR_MENU_DX_SETUP};
    static const char *const kCounts[] = {"6", "6", "4", "10", "7", "12"};
    for (int i = 0; i < SCREEN_MENU_ROWS; i++) {
      menu.rows[i].name = txt(kNames[i]);
      menu.rows[i].value = kCounts[i];
      menu.rows[i].selected = i == 0;
      menu.rows[i].opens = true;
    }
    menu.total = 12; /* Every group, six shown. */
    screenMenuShow(&menu);
    saveShot("%s/menu-groups.bmp", dir);
    /* The list's touch zones over it. */
    TouchZone zones[TOUCH_ZONES_MAX];
    snprintf(path, sizeof(path), "%s/touch-menu.bmp", dir);
    saveZones(zones, screenMenuZones(zones, TOUCH_ZONES_MAX), path);

    memset(&menu, 0, sizeof(menu));
    menu.title = txt(STR_MENU_DISPLAY);
    static const StrId kRows[] = {STR_MENU_THEME,     STR_MENU_BRIGHTNESS,
                                  STR_MENU_DIM_LEVEL, STR_MENU_DIM_AFTER,
                                  STR_MENU_ROTATION,  STR_COMMON_BATTERY};
    const char *const kValues[] = {"2",
                                   fmt(STR_MENU_FMT_PERCENT, 70),
                                   fmt(STR_MENU_FMT_PERCENT, 20),
                                   txt(STR_MENU_NEVER),
                                   txt(STR_COMMON_ROTATION_NORMAL),
                                   txt(STR_MENU_BATTERY_PER_CENT)};
    for (int i = 0; i < SCREEN_MENU_ROWS; i++) {
      menu.rows[i].name = txt(kRows[i]);
      menu.rows[i].value = kValues[i];
      menu.rows[i].selected = i == 1;
      menu.rows[i].opens = i == 0;
    }
    menu.total = 8; /* Display's rows, six shown. */
    screenMenuShow(&menu);
    saveShot("%s/menu-screen.bmp", dir);
    /* With Touch Off: no back mark, the title at the margin. */
    uiSetTouchMarks(false);
    screenMenuShow(&menu);
    saveShot("%s/menu-screen-touch-off.bmp", dir);
    uiSetTouchMarks(true);

    /* Back from a brightness turned to and not saved: its row says so. */
    menu.note = txt(STR_MENU_NOTE_NOT_SAVED);
    screenMenuShow(&menu);
    saveShot("%s/menu-not-saved.bmp", dir);
    menu.note = NULL;

    /* A sub-group, Display's Theme: the group it was opened from, then its
     * own name, as the title. */
    memset(&menu, 0, sizeof(menu));
    menu.title =
        fmt(STR_MENU_FMT_PATH, txt(STR_MENU_DISPLAY), txt(STR_MENU_THEME));
    menu.rows[0].name = txt(STR_MENU_DAY_THEME);
    menu.rows[0].value = txt(STR_THEME_NIGHTWATCH);
    menu.rows[1].name = txt(STR_MENU_NIGHT_THEME);
    menu.rows[1].value = txt(STR_THEME_RED_NIGHT);
    menu.rows[1].selected = true;
    screenMenuShow(&menu);
    saveShot("%s/menu-sub.bmp", dir);

    /* A note on the row it answers: DX Mode pressed on AM. */
    memset(&menu, 0, sizeof(menu));
    menu.title = txt(STR_MENU_GO_TO);
    menu.note = txt(STR_MENU_NOTE_SWITCH_TO_FM);
    menu.rows[0].name = txt(STR_MENU_GO_TO_BAND);
    menu.rows[1].name = txt(STR_MENU_GO_TO_BANDWIDTH);
    menu.rows[2].name = txt(STR_MENU_GO_TO_RDS);
    menu.rows[3].name = txt(STR_MENU_GO_TO_DX);
    menu.rows[3].selected = true;
    menu.rows[4].name = txt(STR_MENU_GO_TO_LOG);
    menu.rows[5].name = txt(STR_MENU_GO_TO_SLEEP);
    screenMenuShow(&menu);
    saveShot("%s/menu-note.bmp", dir);

    /* The Station Log, newest first, six stations from this radio's own
     * logbook, one entry each as the log keeps them: the band where a station
     * gave no name or PI, a name, a PI where there was no name, and the
     * frequency with its unit. */
    memset(&menu, 0, sizeof(menu));
    menu.title = fmt(STR_MENU_FMT_PATH, txt(STR_MENU_STATIONS),
                     txt(STR_MENU_STATION_LOG));
    static const char *const kLogNames[] = {"MW",     "SW",      "MAGIC",
                                            "MIRCHI", "PI 0935", "PI 3712"};
    static const char *const kLogValues[] = {"747 kHz",    "9810 kHz",
                                             "106.40 MHz", "98.30 MHz",
                                             "93.50 MHz",  "91.10 MHz"};
    for (int i = 0; i < SCREEN_MENU_ROWS; i++) {
      menu.rows[i].name = kLogNames[i];
      menu.rows[i].value = kLogValues[i];
      menu.rows[i].selected = i == 1;
    }
    screenMenuShow(&menu);
    saveShot("%s/menu-station-log.bmp", dir);

    /* Stations > Presets: saved presets in slot order, with a name on slot 3
     * as long as a preset name can be, sixteen characters, beside the widest
     * frequency a row shows. */
    memset(&menu, 0, sizeof(menu));
    menu.title =
        fmt(STR_MENU_FMT_PATH, txt(STR_MENU_STATIONS), txt(STR_MENU_MEMORY));
    static const char *const kPresetNames[] = {
        "P01", "P02", "P03 Morning Classics", "P04", "P05", "P15"};
    static const char *const kPresetValues[] = {"91.10 MHz", "92.70 MHz",
                                                "93.50 MHz", "94.30 MHz",
                                                "95.00 MHz", "21550 kHz"};
    for (int i = 0; i < SCREEN_MENU_ROWS; i++) {
      menu.rows[i].name = kPresetNames[i];
      menu.rows[i].value = kPresetValues[i];
      menu.rows[i].selected = i == 2;
    }
    screenMenuShow(&menu);
    saveShot("%s/menu-presets.bmp", dir);

    /* The longest list title, Network Info under Connectivity. */
    memset(&menu, 0, sizeof(menu));
    menu.title = fmt(STR_MENU_FMT_PATH, txt(STR_MENU_CONNECTIVITY),
                     txt(STR_MENU_NETWORK_INFO));
    static const StrId kNetRows[] = {
        STR_MENU_STATUS,     STR_MENU_WEB_ADDRESS,  STR_MENU_IP_ADDRESS,
        STR_MENU_WI_FI_NAME, STR_MENU_WI_FI_SIGNAL, STR_MENU_MAC};
    const char *const kNetValues[] = {
        txt(STR_MENU_WIFI_JOINED),
        fmt(STR_MENU_FMT_NAME_ADDRESS, "tef668x", 8080),
        "255.255.255.255",
        "Home",
        "-54 dBm",
        "02:12:34:56:78:9A"};
    for (int i = 0; i < 6; i++) {
      menu.rows[i].name = txt(kNetRows[i]);
      menu.rows[i].value = kNetValues[i];
      menu.rows[i].selected = i == 1;
    }
    screenMenuShow(&menu);
    saveShot("%s/menu-network-info.bmp", dir);

    /* The choice of bands for 123 typed on FM, which FM does not hold:
     * medium wave and shortwave, the cursor on the first. */
    {
      static const BandTypedReading kChoice[] = {{BAND_MW, 1230},
                                                 {BAND_SW, 12300}};
      static char freq[2][16];
      memset(&menu, 0, sizeof(menu));
      menu.title = fmt(STR_MENU_FMT_TUNE_TO, "123");
      for (int i = 0; i < 2; i++) {
        bandFormatWithUnit(kChoice[i].band, kChoice[i].freqKHz, freq[i],
                           sizeof(freq[i]));
        menu.rows[i].name = bandLongName(kChoice[i].band);
        menu.rows[i].value = freq[i];
        menu.rows[i].selected = i == 0;
      }
      screenMenuShow(&menu);
      saveShot("%s/menu-typed-choice.bmp", dir);
    }

    /* The offer of a newer release found at start, the knob on Update. The
     * versions and the size are samples. */
    {
      static char size[16];
      snprintf(size, sizeof(size), txt(STR_MENU_FMT_MEGABYTES), "1.9");
      ScreenMenuDialog dialog;
      memset(&dialog, 0, sizeof(dialog));
      dialog.icon = ICON_NEW;
      dialog.title = txt(STR_MENU_UPDATE_TITLE);
      dialog.label[0] = txt(STR_MENU_UPDATE_THIS_RADIO);
      dialog.value[0] = "0.1.0";
      dialog.label[1] = txt(STR_MENU_UPDATE_NEW_VERSION);
      dialog.value[1] = "0.2.0";
      dialog.label[2] = txt(STR_MENU_UPDATE_DOWNLOAD);
      dialog.value[2] = size;
      dialog.label[3] = txt(STR_COMMON_SETTINGS);
      dialog.value[3] = txt(STR_COMMON_KEPT);
      dialog.button[0] = txt(STR_MENU_UPDATE_NOW);
      dialog.button[1] = txt(STR_MENU_LATER);
      screenMenuDialogShow(&dialog);
      saveShot("%s/menu-update-offer.bmp", dir);

      /* Restart Radio's question: no symbol, No on the left where the knob
       * starts, and the two answers' touch zones over it. */
      static char question[32];
      snprintf(question, sizeof(question), txt(STR_MENU_FMT_QUESTION),
               txt(STR_MENU_RESTART));
      memset(&dialog, 0, sizeof(dialog));
      dialog.title = question;
      dialog.label[0] = txt(STR_COMMON_SETTINGS);
      dialog.value[0] = txt(STR_COMMON_KEPT);
      dialog.label[1] = txt(STR_MENU_STATION);
      dialog.value[1] = "FM 106.40 MHz";
      dialog.button[0] = txt(STR_COMMON_NO);
      dialog.button[1] = txt(STR_COMMON_YES);
      dialog.taps = true;
      screenMenuDialogShow(&dialog);
      saveShot("%s/menu-restart.bmp", dir);
      TouchZone zones[TOUCH_ZONES_MAX];
      snprintf(path, sizeof(path), "%s/touch-question.bmp", dir);
      saveZones(zones, screenMenuZones(zones, TOUCH_ZONES_MAX), path);
    }

    /* The About group, the last one, with nothing to edit. The build is a
     * sample hash: the real one is written by the firmware build, which this
     * tool is not. */
    memset(&menu, 0, sizeof(menu));
    menu.title = txt(STR_MENU_ABOUT);
    static const StrId kAboutRows[] = {STR_MENU_VERSION, STR_MENU_BUILD,
                                       STR_MENU_DEVELOPER, STR_MENU_LICENSE,
                                       STR_MENU_GITHUB};
    const char *const kAboutValues[] = {
        FIRMWARE_VERSION, "f62ad2a", txt(STR_ABOUT_DEVELOPER),
        txt(STR_ABOUT_LICENSE), txt(STR_ABOUT_GITHUB)};
    for (int i = 0; i < 5; i++) {
      menu.rows[i].name = txt(kAboutRows[i]);
      menu.rows[i].value = kAboutValues[i];
      menu.rows[i].selected = i == 4;
    }
    screenMenuShow(&menu);
    saveShot("%s/menu-about.bmp", dir);

    /* The Diagnostics group's readings, with the widest value each row can
     * show: the battery read at start up, a reset reason of 13 letters, and
     * both cores flat out. */
    memset(&menu, 0, sizeof(menu));
    menu.title = txt(STR_MENU_DIAGNOSTICS);
    static const StrId kSystemRows[] = {
        STR_MENU_UPTIME,       STR_MENU_BATTERY_VOLTAGE, STR_COMMON_TUNER,
        STR_MENU_RESET_REASON, STR_MENU_CPU_CORE_0,      STR_MENU_CPU_CORE_1};
    const char *const kSystemValues[] = {
        fmt(STR_MENU_FMT_HOURS_MINUTES, 23u, 59u),
        fmt(STR_MENU_FMT_VOLTS_AT_START, 3u, 69u),
        "TEF6686 patch 102",
        txt(STR_RESET_BOOT_WATCHDOG),
        fmt(STR_MENU_FMT_PERCENT, 100),
        fmt(STR_MENU_FMT_PERCENT, 100)};
    for (int i = 0; i < SCREEN_MENU_ROWS; i++) {
      menu.rows[i].name = txt(kSystemRows[i]);
      menu.rows[i].value = kSystemValues[i];
      menu.rows[i].selected = i == 1;
    }
    screenMenuShow(&menu);
    saveShot("%s/menu-diagnostics.bmp", dir);

    /* The same group at its last row. */
    static const StrId kSystemEnd[] = {
        STR_MENU_FREE_HEAP, STR_MENU_LOWEST_HEAP, STR_MENU_LARGEST_BLOCK,
        STR_MENU_LVGL_POOL, STR_MENU_CHIP,        STR_MENU_FLASH_SIZE};
    const char *const kSystemEndValues[] = {
        fmt(STR_MENU_FMT_KB, 143),
        fmt(STR_MENU_FMT_KB, 118),
        fmt(STR_MENU_FMT_KB, 110),
        fmt(STR_MENU_FMT_KB_OF_KB, 22, 22),
        fmt(STR_MENU_FMT_CHIP, "ESP32-D0WD-V3", 3, 1),
        fmt(STR_MENU_FMT_MB, 8)};
    for (int i = 0; i < SCREEN_MENU_ROWS; i++) {
      menu.rows[i].name = txt(kSystemEnd[i]);
      menu.rows[i].value = kSystemEndValues[i];
      menu.rows[i].selected = i == 5;
    }
    screenMenuShow(&menu);
    saveShot("%s/menu-diagnostics-end.bmp", dir);

    /* The System group, Auto Off at 30 minutes and the restart. */
    memset(&menu, 0, sizeof(menu));
    menu.title = txt(STR_MENU_SYSTEM);
    menu.rows[0].name = txt(STR_MENU_AUTO_OFF);
    menu.rows[0].value = fmt(STR_MENU_FMT_MINUTES, 30u);
    menu.rows[0].selected = true;
    menu.rows[1].name = txt(STR_MENU_RESTART);
    screenMenuShow(&menu);
    saveShot("%s/menu-system.bmp", dir);

    /* The DX Scanner group at its defaults. */
    memset(&menu, 0, sizeof(menu));
    menu.title = txt(STR_MENU_DX_SETUP);
    static const StrId kDxRows[] = {STR_MENU_START_SCAN,   STR_MENU_DWELL,
                                    STR_MENU_STOP_ON,      STR_MENU_SCAN,
                                    STR_MENU_PRESET_RANGE, STR_MENU_DX_WIDTH};
    const char *const kDxValues[] = {NULL,
                                     withUnit("2.5", STR_COMMON_UNIT_S),
                                     txt(STR_MENU_NEW_ONLY),
                                     txt(STR_MENU_BAND_MEMORY),
                                     "2",
                                     withUnit("114", STR_COMMON_UNIT_KHZ)};
    for (int i = 0; i < SCREEN_MENU_ROWS; i++) {
      menu.rows[i].name = txt(kDxRows[i]);
      menu.rows[i].value = kDxValues[i];
      menu.rows[i].selected = i == 1;
      menu.rows[i].opens = i == 4;
    }
    screenMenuShow(&menu);
    saveShot("%s/menu-dx.bmp", dir);

    ScreenMenuValue dwell;
    memset(&dwell, 0, sizeof(dwell));
    dwell.buttons = true; /* Touch On, as on a new radio. */
    dwell.name =
        fmt(STR_MENU_FMT_PATH, txt(STR_MENU_DX_SETUP), txt(STR_MENU_DWELL));
    dwell.value = "2.5";
    dwell.unit = txt(STR_COMMON_UNIT_S);
    dwell.hasRange = true;
    dwell.min = 5;
    dwell.max = 300;
    dwell.at = 25;
    dwell.minText = withUnit("0.5", STR_COMMON_UNIT_S);
    dwell.maxText = withUnit("30.0", STR_COMMON_UNIT_S);
    screenMenuValueShow(&dwell);
    saveShot("%s/menu-dx-dwell.bmp", dir);

    /* One setting on its own, which is what a press on a row opens. The bar
     * is the thing the list could never show: where this value sits between
     * its limits. */
    ScreenMenuValue one;
    memset(&one, 0, sizeof(one));
    one.buttons = true; /* Touch On, as on a new radio. */
    one.name =
        fmt(STR_MENU_FMT_PATH, txt(STR_MENU_DISPLAY), txt(STR_MENU_BRIGHTNESS));
    one.value = "70";
    one.unit = txt(STR_COMMON_UNIT_PERCENT);
    one.hasRange = true;
    one.min = 5;
    one.max = 100;
    one.at = 70;
    one.minText =
        fmt(STR_MENU_FMT_NUMBER_UNIT, 5, txt(STR_COMMON_UNIT_PERCENT));
    one.maxText =
        fmt(STR_MENU_FMT_NUMBER_UNIT, 100, txt(STR_COMMON_UNIT_PERCENT));
    screenMenuValueShow(&one);
    saveShot("%s/menu-value.bmp", dir);
    {
      /* The value editor's touch zones over it. */
      TouchZone zones[TOUCH_ZONES_MAX];
      snprintf(path, sizeof(path), "%s/touch-value.bmp", dir);
      saveZones(zones, screenMenuZones(zones, TOUCH_ZONES_MAX), path);
    }

    /* The same once the knob has moved it: not saved until the press. */
    one.note = txt(STR_MENU_NOTE_NOT_SAVED);
    screenMenuValueShow(&one);
    saveShot("%s/menu-value-not-saved.bmp", dir);
    one.note = NULL;

    /* Touch Off: no buttons, and the note back on the bottom line. */
    one.buttons = false;
    one.note = txt(STR_MENU_NOTE_NOT_SAVED);
    uiSetTouchMarks(false);
    screenMenuValueShow(&one);
    saveShot("%s/menu-value-touch-off.bmp", dir);
    uiSetTouchMarks(true);
    one.note = NULL;
    one.buttons = true;

    /* Auto Off at 30 minutes, on a bar from Off to ten hours in five minute
     * steps. */
    ScreenMenuValue sleep;
    memset(&sleep, 0, sizeof(sleep));
    sleep.buttons = true; /* Touch On, as on a new radio. */
    sleep.name =
        fmt(STR_MENU_FMT_PATH, txt(STR_MENU_SYSTEM), txt(STR_MENU_AUTO_OFF));
    sleep.value = "30";
    sleep.unit = txt(STR_COMMON_UNIT_MIN);
    sleep.hasRange = true;
    sleep.min = 0;
    sleep.max = 600;
    sleep.at = 30;
    sleep.minText = txt(STR_COMMON_OFF);
    sleep.maxText = fmt(STR_MENU_FMT_MINUTES, 600u);
    screenMenuValueShow(&sleep);
    saveShot("%s/menu-value-auto-off.bmp", dir);

    /* The Network Time row: a signed offset, with the mark at zero,
     * and the one setting value that carries a colon, on a bar whose left
     * end is Off. West of Greenwich, then east, since the two signs are two
     * different characters the numeric face has to carry, then Off. */
    ScreenMenuValue offset;
    memset(&offset, 0, sizeof(offset));
    offset.buttons = true; /* Touch On, as on a new radio. */
    offset.name = fmt(STR_MENU_FMT_PATH, txt(STR_MENU_CONNECTIVITY),
                      txt(STR_MENU_CLOCK_FROM_NETWORK));
    offset.value = "-05:30";
    offset.hasRange = true;
    offset.min = -735;
    offset.max = 840;
    offset.at = -330;
    offset.minText = txt(STR_COMMON_OFF);
    offset.maxText = "+14:00";
    screenMenuValueShow(&offset);
    saveShot("%s/menu-value-colon.bmp", dir);

    offset.value = "+05:30";
    offset.at = 330;
    screenMenuValueShow(&offset);
    saveShot("%s/menu-value-plus.bmp", dir);

    offset.value = txt(STR_COMMON_OFF);
    offset.at = -735;
    screenMenuValueShow(&offset);
    saveShot("%s/menu-value-off.bmp", dir);

    /* The Web PIN, set a digit at a time, in the words menu_task.cpp gives
     * it: at the first digit, at the third, and at the last, where the
     * press saves. */
    {
      static const struct {
        const char *file;
        const char *digits;
        uint8_t at;
      } kPin[] = {{"menu-pin-first", "000000", 0},
                  {"menu-pin-third", "482000", 2},
                  {"menu-pin-last", "482917", 5}};
      for (size_t i = 0; i < sizeof(kPin) / sizeof(kPin[0]); i++) {
        static char place[24];
        snprintf(place, sizeof(place), txt(STR_MENU_FMT_DIGIT_OF),
                 (unsigned)(kPin[i].at + 1), 6u);
        ScreenMenuValue pin;
        memset(&pin, 0, sizeof(pin));
        pin.name = fmt(STR_MENU_FMT_PATH, txt(STR_MENU_CONNECTIVITY),
                       txt(STR_MENU_WEB_PIN));
        pin.isDigits = true;
        pin.digits = kPin[i].digits;
        pin.digitAt = kPin[i].at;
        pin.digitPlace = place;
        pin.label = txt(STR_MENU_NEW_PIN);
        pin.buttons = true; /* Touch On, as on a new radio. */
        screenMenuValueShow(&pin);
        lv_refr_now(NULL);
        int count[2] = {0, 0};
        uiReadPanel(countPinTexts, NULL, count);
        if (count[0] != 6 || count[1] != 0) {
          fprintf(stderr, "%s: %d stars and %d digits read back, not 6 and 0\n",
                  kPin[i].file, count[0], count[1]);
          exit(1);
        }
        saveShot("%s/%s.bmp", dir, kPin[i].file);
        if (i == 1) {
          /* Its keys' touch zones, and the same PIN with Touch Off. */
          TouchZone zones[TOUCH_ZONES_MAX];
          snprintf(path, sizeof(path), "%s/touch-pin.bmp", dir);
          saveZones(zones, screenMenuZones(zones, TOUCH_ZONES_MAX), path);
          pin.buttons = false;
          uiSetTouchMarks(false);
          screenMenuValueShow(&pin);
          lv_refr_now(NULL);
          int off[2] = {0, 0};
          uiReadPanel(countPinTexts, NULL, off);
          if (off[0] != 6 || off[1] != 0) {
            fprintf(stderr,
                    "menu-pin-touch-off: %d stars and %d digits read back, not "
                    "6 and 0\n",
                    off[0], off[1]);
            exit(1);
          }
          saveShot("%s/menu-pin-touch-off.bmp", dir);
          uiSetTouchMarks(true);
        }
      }
    }

    /* The theme picker: a named choice shown rather than a number scrubbed on a
     * bar, its own swatches read from the same theme table the panel itself is
     * drawn from. */
    ScreenMenuValue theme;
    memset(&theme, 0, sizeof(theme));
    theme.name =
        fmt(STR_MENU_FMT_PATH, txt(STR_MENU_THEME), txt(STR_MENU_DAY_THEME));
    theme.isPicker = true;
    theme.isTheme = true;
    theme.pickerTotal = THEME_COUNT;
    /* Every page of the list, in the menu's order, the cursor on the second
     * row of each and the saved choice, Nightwatch, on the first page. */
    for (int page = 0; page * SCREEN_MENU_PICKER_ROWS < THEME_COUNT; page++) {
      for (int i = 0; i < SCREEN_MENU_PICKER_ROWS; i++) {
        const int place = page * SCREEN_MENU_PICKER_ROWS + i;
        if (place >= THEME_COUNT) {
          theme.picker[i].name = NULL;
          continue;
        }
        const uint8_t index = paletteThemeAt((uint8_t)place);
        theme.picker[i].name = themeAt(index)->name;
        theme.picker[i].isCursor = i == 1;
        theme.picker[i].isSaved = place == 0;
        theme.picker[i].themeIndex = index;
      }
      theme.pickerTop = (uint8_t)(page * SCREEN_MENU_PICKER_ROWS);
      screenMenuValueShow(&theme);
      if (page == 0) {
        saveShot("%s/menu-theme.bmp", dir);
        /* A picker's touch zones over it. */
        TouchZone zones[TOUCH_ZONES_MAX];
        snprintf(path, sizeof(path), "%s/touch-picker.bmp", dir);
        saveZones(zones, screenMenuZones(zones, TOUCH_ZONES_MAX), path);
      } else {
        saveShot("%s/menu-theme-%d.bmp", dir, page + 1);
      }
    }

    /* The plain enum picker, the same screen with no swatches. Four options
     * here, so the list simply ends rather than stretching to fill the
     * fifth row. */
    ScreenMenuValue beep;
    memset(&beep, 0, sizeof(beep));
    beep.name =
        fmt(STR_MENU_FMT_PATH, txt(STR_MENU_CONTROLS), txt(STR_MENU_KEY_BEEPS));
    beep.isPicker = true;
    static const StrId kBeepNames[] = {STR_COMMON_OFF, STR_MENU_KEYS,
                                       STR_MENU_KEYS_LONG,
                                       STR_MENU_EVERY_PRESS};
    for (int i = 0; i < 4; i++) {
      beep.picker[i].name = txt(kBeepNames[i]);
      beep.picker[i].isCursor = i == 2;
      beep.picker[i].isSaved = i == 1;
    }
    screenMenuValueShow(&beep);
    saveShot("%s/menu-enum.bmp", dir);

    /* The Rotation row's own picker, the cursor on the choice not yet kept. */
    ScreenMenuValue rotation;
    memset(&rotation, 0, sizeof(rotation));
    rotation.name =
        fmt(STR_MENU_FMT_PATH, txt(STR_MENU_DISPLAY), txt(STR_MENU_ROTATION));
    rotation.isPicker = true;
    for (int i = 0; i < 2; i++) {
      rotation.picker[i].name = txt(i == 0 ? STR_COMMON_ROTATION_NORMAL
                                           : STR_COMMON_ROTATION_UPSIDE_DOWN);
      rotation.picker[i].isCursor = i == 1;
      rotation.picker[i].isSaved = i == 0;
    }
    screenMenuValueShow(&rotation);
    saveShot("%s/menu-rotation.bmp", dir);
    screenMenuEnd();
  }

  /* The RDS screen's four pages, through the same screen_rds_state.cpp the
   * radio runs. From real captures: Magic FM 106.4 as it was heard, and the
   * same with a weak signal, for the decoder page. Then a made up station
   * with every field set, since none of the captures sends AF, EON, RT+, a
   * language, a PIN or PTYN, to check that nothing runs off the screen or over
   * its neighbour. */
  if (screenRdsBegin()) {
    static RdsInfo strong, weak, example, longest;
    const bool haveStrong = captureRds(106400, false, &strong);
    const bool haveWeak = captureRds(106400, true, &weak);
    rdsExampleStation(&example);
    rdsLongStation(&longest);
    /* The example read on North America, with the station's own RT+
     * StationName.Short over "RADIO 1," in its text: eight characters, the
     * most the slot takes, and shown without the dimming a guess gets. */
    static RdsInfo exampleNa, exampleNaSent;
    exampleNa = example;
    exampleNa.pi = 0x21C7;
    exampleNa.hasEcc = false;
    exampleNaSent = exampleNa;
    exampleNaSent.rtPlusTag[exampleNaSent.rtPlusCount++] = {31, 40, 8};
    /* Kept as the decoder keeps it, from that tag. */
    exampleNaSent.hasStationShort = rdsRtPlusText(
        &exampleNaSent, (uint8_t)(exampleNaSent.rtPlusCount - 1),
        exampleNaSent.stationShort, sizeof(exampleNaSent.stationShort));
    static const char *const kPageNames[SCREEN_RDS_PAGES] = {
        "station", "text", "networks", "decoder"};
    const struct {
      const char *suffix;
      const RdsInfo *info;
      bool have;
      const char *frequency;
      const char *sync;
      uint8_t firstPage;
      uint8_t lastPage;
      RdsRegion region;
    } scenes[] = {
        {"", &strong, haveStrong, "106.40", "41m10s", 0, 3, RDS_REGION_EUROPE},
        {"-weak", &weak, haveWeak, "106.40", "2m04s", 3, 3, RDS_REGION_EUROPE},
        {"-example", &example, true, "94.80", "4m12s", 0, 3, RDS_REGION_EUROPE},
        {"-long", &longest, true, "107.90", "16h40m", 0, 3, RDS_REGION_EUROPE},
        /* Read on North America. The example on PI 21C7, which works out as
         * KGTB, a guess, with no area and RBDS's programme type names; then the
         * same sending its own short name over it. 106.4's own 1064 gives no
         * call letters, since the standard sends it moved, so it shows the
         * reason there is no country, as in Europe. */
        {"-na", &exampleNa, true, "94.80", "4m12s", 0, 0,
         RDS_REGION_NORTH_AMERICA},
        {"-na-sent", &exampleNaSent, true, "94.80", "4m12s", 0, 0,
         RDS_REGION_NORTH_AMERICA},
        {"-na-1064", &strong, haveStrong, "106.40", "41m10s", 0, 0,
         RDS_REGION_NORTH_AMERICA},
    };
    for (size_t k = 0; k < sizeof(scenes) / sizeof(scenes[0]); k++) {
      if (!scenes[k].have) {
        fprintf(stderr, "no RDS capture for rds scene rds-%s%s\n",
                kPageNames[scenes[k].firstPage], scenes[k].suffix);
        continue;
      }
      for (uint8_t page = scenes[k].firstPage;
           page <= scenes[k].lastPage && page < SCREEN_RDS_PAGES; page++) {
        ScreenRdsInputs in;
        memset(&in, 0, sizeof(in));
        in.rds = scenes[k].info;
        in.page = page;
        in.region = scenes[k].region;
        in.frequency = fmt(STR_COMMON_FMT_TWO_WORDS, scenes[k].frequency,
                           txt(STR_COMMON_UNIT_MHZ));
        in.clock = "06:31";
        in.sync = scenes[k].sync;
        in.syncGood = true;
        ScreenRds view;
        screenRdsStateBuild(&in, &view);
        screenRdsShow(&view);
        saveShot("%s/rds-%s%s.bmp", dir, kPageNames[page], scenes[k].suffix);
        if (k == 0 && page == 0) {
          /* The screen's touch zones over its first page. */
          TouchZone zones[TOUCH_ZONES_MAX];
          snprintf(path, sizeof(path), "%s/touch-rds.bmp", dir);
          saveZones(zones, screenRdsZones(zones, TOUCH_ZONES_MAX), path);
          /* The same with the sleep mark of the last minutes before auto off. */
          uiSetSleepMark(UI_SLEEP_SOON);
          screenRdsShow(&view);
          saveShot("%s/rds-station-sleep.bmp", dir);
          uiSetSleepMark(UI_SLEEP_NONE);
        }
      }
    }
    /* Page 1 of 106.4 on preset 3: its stored PI, then another's, read on
     * the channel's own carrier. */
    if (haveStrong) {
      static const struct {
        const char *file;
        uint16_t pi;
      } kPreset[] = {{"rds-station-preset", 0x1064},
                     {"rds-station-preset-other", 0x26FF}};
      for (size_t i = 0; i < sizeof(kPreset) / sizeof(kPreset[0]); i++) {
        ScreenRdsInputs in;
        memset(&in, 0, sizeof(in));
        in.rds = &strong;
        in.page = 0;
        in.region = RDS_REGION_EUROPE;
        in.frequency =
            fmt(STR_COMMON_FMT_TWO_WORDS, "106.40", txt(STR_COMMON_UNIT_MHZ));
        in.clock = "06:31";
        in.sync = "41m10s";
        in.syncGood = true;
        in.reading.valid = true;
        in.reading.offsetTenths = 30;
        in.presetSlot = 2;
        in.presetPi = kPreset[i].pi;
        ScreenRds view;
        screenRdsStateBuild(&in, &view);
        screenRdsShow(&view);
        saveShot("%s/%s.bmp", dir, kPreset[i].file);
      }
      /* Digits typed over page 1: the header says what ENTER will tune to,
       * since the page has no frequency of its own to show them in. */
      ScreenRdsInputs in;
      memset(&in, 0, sizeof(in));
      in.rds = &strong;
      in.page = 0;
      in.region = RDS_REGION_EUROPE;
      in.frequency =
          fmt(STR_COMMON_FMT_TWO_WORDS, "106.40", txt(STR_COMMON_UNIT_MHZ));
      in.clock = "06:31";
      in.sync = "41m10s";
      in.syncGood = true;
      in.reading.valid = true;
      in.presetSlot = MEMORY_NO_SLOT;
      in.message = fmt(STR_COMMON_FMT_TUNE_TYPED, "104-");
      ScreenRds view;
      screenRdsStateBuild(&in, &view);
      screenRdsShow(&view);
      saveShot("%s/rds-station-typed.bmp", dir);
    }
    screenRdsEnd();
  }

  /* The DX page, from real captures. A station with its name and PI confirmed;
   * one sending 0000; the channel beside Red FM, which hears Red FM's PI but is
   * not its channel, over a minute; the same a few groups in, with the name
   * half heard; noise; and the one channel where noise gave a clean block A. */
  if (screenDxBegin()) {
    renderDx(dir, "station", dxChannel(kDxSweep, DX_SWEEP_COUNT, 106400),
             UINT16_MAX);
    {
      /* The DX page's touch zones over it, and the other pages' checked. */
      TouchZone zones[TOUCH_ZONES_MAX];
      checkZones(zones, screenDxZones(zones, TOUCH_ZONES_MAX, false),
                 "dx pages");
      snprintf(path, sizeof(path), "%s/touch-dx.bmp", dir);
      saveZones(zones, screenDxZones(zones, TOUCH_ZONES_MAX, true), path);
    }
    uiSetSleepMark(UI_SLEEP_ON);
    renderDx(dir, "auto-off", dxChannel(kDxSweep, DX_SWEEP_COUNT, 106400),
             UINT16_MAX);
    uiSetSleepMark(UI_SLEEP_SOON);
    renderDx(dir, "sleep", dxChannel(kDxSweep, DX_SWEEP_COUNT, 106400),
             UINT16_MAX);
    uiSetSleepMark(UI_SLEEP_NONE);
    /* The same capture read on North America on PI 21C7: KGTB, with the
     * help mark. */
    renderDx(dir, "station-na", dxChannel(kDxSweep, DX_SWEEP_COUNT, 106400),
             UINT16_MAX, RDS_REGION_NORTH_AMERICA, 0x21C7);
    /* 106.4 on preset 3: its stored PI, and then another station's stored
     * there, so 1064 is not that preset's. */
    renderDx(dir, "preset", dxChannel(kDxSweep, DX_SWEEP_COUNT, 106400),
             UINT16_MAX, RDS_REGION_EUROPE, 0, 2, 0x1064);
    renderDx(dir, "preset-other", dxChannel(kDxSweep, DX_SWEEP_COUNT, 106400),
             UINT16_MAX, RDS_REGION_EUROPE, 0, 2, 0x26FF);
    renderDx(dir, "zero", dxChannel(kDxSweep, DX_SWEEP_COUNT, 94300),
             UINT16_MAX);
    renderDx(dir, "shoulder", &kDxMinute[1], UINT16_MAX);
    renderDx(dir, "building", &kDxMinute[1], 8);
    renderDx(dir, "noise", dxChannel(kDxSweep, DX_SWEEP_COUNT, 89400),
             UINT16_MAX);
    renderDx(dir, "heard", dxChannel(kDxSweep, DX_SWEEP_COUNT, 90700),
             UINT16_MAX);
    screenDxEnd();
  }

  if (screenScopeBegin()) {
    renderScopes(dir);
    renderBandScopes(dir);
    screenScopeEnd();
  }

  /* The bandwidth page: FM on automatic with the cursor on it and on 114; the
   * cursor on the iMS switch, on; medium wave on 4 kHz with the cursor on 6;
   * and over the DX page at 114, with no automatic. */
  if (screenBwBegin()) {
    static BwTile tiles[BW_PAGE_MAX];
    static ScreenBwKeep keep;
    struct {
      const char *name;
      BandId band;
      bool dx;
      uint16_t cursorKHz;
      int cursor; /* -1: the cursorKHz tile; else this tile. */
      uint16_t width;
      bool ims;
    } shots[] = {{"bw-fm", BAND_FM, false, 0, -1, 0, true},
                 {"bw-fm-114", BAND_FM, false, 114, -1, 0, true},
                 {"bw-fm-ims", BAND_FM, false, 0, 17, 114, true},
                 {"bw-mw", BAND_MW, false, 6, -1, 4, false},
                 {"bw-dx", BAND_FM, true, 114, -1, 114, false}};
    for (size_t i = 0; i < sizeof(shots) / sizeof(shots[0]); i++) {
      ScreenBwInputs in;
      memset(&in, 0, sizeof(in));
      in.count = bwPageTiles(shots[i].band, shots[i].dx, tiles, BW_PAGE_MAX);
      in.tiles = tiles;
      in.cursor = shots[i].cursor >= 0
                      ? (uint8_t)shots[i].cursor
                      : bwPageStart(tiles, in.count, shots[i].cursorKHz);
      in.band = shots[i].band;
      in.dxMode = shots[i].dx;
      in.widthKHz = shots[i].width;
      in.chipKnown = true;
      in.chipKHz = shots[i].width == 0 ? 217 : shots[i].width;
      in.ims = shots[i].ims;
      in.clock = "19:32";
      ScreenBw view;
      screenBwStateBuild(&in, &keep, &view);
      screenBwShow(&view);
      saveShot("%s/%s.bmp", dir, shots[i].name);
      /* The page's touch zones checked on every page, and drawn over the
       * FM one. */
      TouchZone zones[TOUCH_ZONES_MAX];
      const int n = screenBwZones(zones, TOUCH_ZONES_MAX);
      checkZones(zones, n, shots[i].name);
      if (i == 0) {
        snprintf(path, sizeof(path), "%s/touch-bw.bmp", dir);
        saveZones(zones, n, path);
      }
    }
    screenBwEnd();
  }

  if (screenKeypadBegin()) {
    ScreenKeypad pad;
    pad.context = "FM \xC2\xB7 MHz";
    pad.clock = "19:32";
    pad.typed = "104";
    screenKeypadShow(&pad);
    saveShot("%s/keypad.bmp", dir);
    TouchZone zones[TOUCH_ZONES_MAX];
    snprintf(path, sizeof(path), "%s/touch-keypad.bmp", dir);
    saveZones(zones, screenKeypadZones(zones, TOUCH_ZONES_MAX), path);
    screenKeypadEnd();
  }

  if (screenScanBegin()) {
    renderScans(dir);
    screenScanEnd();
  }

  /* The Catches page: the sweep's catches with the cursor on the newest and on
   * the oldest, a hold's confirmation, and the empty page. The sweep has three,
   * so no capture has a second screen of rows. */
  if (screenCatchesBegin()) {
    static DxCatches list;
    buildCatches(&list, 2);
    renderCatches(dir, "catches", &list, 0, NULL);
    {
      /* The page's touch zones over it. */
      TouchZone zones[TOUCH_ZONES_MAX];
      snprintf(path, sizeof(path), "%s/touch-catches.bmp", dir);
      saveZones(zones, screenCatchesZones(zones, TOUCH_ZONES_MAX), path);
    }
    renderCatches(dir, "catches-logged", &list, 0,
                  fmt(STR_RADIO_FMT_LOGGED, "106.40"));
    renderCatches(dir, "catches-oldest", &list,
                  (uint8_t)(list.count > 0 ? list.count - 1 : 0), NULL);
    dxCatchesReset(&list);
    renderCatches(dir, "catches-empty", &list, 0, NULL);
    screenCatchesEnd();
  }

  /* The update veil: a firmware write in progress, the one screen with no
   * header and no footer, the percentage the only thing that moves. First
   * as a write starts, before its size is known, then part way. */
  if (screenBegin()) {
    screenUpdateVeilShow(-1);
    saveShot("%s/veil-start.bmp", dir);
    screenUpdateVeilShow(62);
    saveShot("%s/veil.bmp", dir);
    screenEnd();
  }

  /* Recovery: no theme, fault red, the cursor on the first row, the saved
   * rotation and the Touch switch shown as values. */
  if (screenRecoveryBegin()) {
    ScreenRecovery recovery;
    memset(&recovery, 0, sizeof(recovery));
    recovery.cursor = 0;
    static const StrId kRecoveryNames[SCREEN_RECOVERY_ROWS] = {
        STR_RECOVERY_ROTATE_DISPLAY,       STR_RECOVERY_TOUCH,
        STR_RECOVERY_CALIBRATE_TOUCH,      STR_RECOVERY_START_HOTSPOT,
        STR_RECOVERY_ROLL_BACK_FIRMWARE,   STR_RECOVERY_ERASE_SETTINGS,
        STR_RECOVERY_EXIT_AND_START_RADIO,
    };
    for (int i = 0; i < SCREEN_RECOVERY_ROWS; i++) {
      recovery.rows[i].name = txt(kRecoveryNames[i]);
    }
    recovery.rows[0].value = txt(STR_COMMON_ROTATION_NORMAL);
    recovery.rows[1].value = txt(STR_COMMON_ON);
    screenRecoveryShow(&recovery);
    saveShot("%s/recovery.bmp", dir);

    /* Erase Settings after a write that failed, which says so and stays
     * rather than restarting on the old settings. */
    recovery.cursor = 5;
    recovery.rows[5].value = txt(STR_RECOVERY_FAILED);
    screenRecoveryShow(&recovery);
    saveShot("%s/recovery-failed.bmp", dir);

    /* Erase Settings pressed once: it waits for a second press, and the foot
     * line says what that will do. */
    recovery.rows[5].value = txt(STR_RECOVERY_PRESS_AGAIN);
    recovery.hint = txt(STR_RECOVERY_ASK_ERASE);
    screenRecoveryShow(&recovery);
    saveShot("%s/recovery-ask.bmp", dir);

    /* Touch pressed once, on a radio with touch on, the second press to turn
     * it off. */
    recovery.rows[5].value = NULL;
    recovery.cursor = 1;
    recovery.rows[1].value = txt(STR_RECOVERY_PRESS_AGAIN);
    recovery.hint = txt(STR_RECOVERY_ASK_TOUCH_OFF);
    screenRecoveryShow(&recovery);
    saveShot("%s/recovery-touch-ask.bmp", dir);

    /* The cursor on the last row, which scrolls the list by one. */
    recovery.rows[1].value = txt(STR_COMMON_ON);
    recovery.hint = NULL;
    recovery.cursor = SCREEN_RECOVERY_ROWS - 1;
    screenRecoveryShow(&recovery);
    saveShot("%s/recovery-exit.bmp", dir);
    screenRecoveryEnd();
  }

  /* The touch calibration screen: the second mark part filled, the middle
   * mark, the check, and the three results. */
  if (screenTouchCalBegin(false)) {
    ScreenTouchCal cal;
    memset(&cal, 0, sizeof(cal));
    static const int16_t kX[5] = {32, 287, 287, 32, 160};
    static const int16_t kY[5] = {32, 32, 207, 207, 120};
    for (int i = 0; i < 5; i++) {
      cal.markX[i] = kX[i];
      cal.markY[i] = kY[i];
    }
    cal.dotX = 224;
    cal.dotY = 90;
    static const struct {
      ScreenTouchCalStep step;
      uint8_t mark, fill;
      uint16_t off;
      const char *name;
    } kShots[] = {
        {SCREEN_TOUCH_CAL_MARK, 1, 60, 0, "touch-cal-mark2"},
        {SCREEN_TOUCH_CAL_MARK, 4, 0, 0, "touch-cal-mark5"},
        {SCREEN_TOUCH_CAL_CHECK, 4, 0, 0, "touch-cal-check"},
        {SCREEN_TOUCH_CAL_SAVED, 4, 0, 3, "touch-cal-saved"},
        {SCREEN_TOUCH_CAL_MISSED, 4, 0, 31, "touch-cal-missed"},
        {SCREEN_TOUCH_CAL_NO_FIT, 4, 0, 0, "touch-cal-no-fit"},
        {SCREEN_TOUCH_CAL_NOT_SAVED, 4, 0, 0, "touch-cal-not-saved"},
    };
    for (const auto &shot : kShots) {
      cal.step = shot.step;
      cal.mark = shot.mark;
      cal.fillPct = shot.fill;
      cal.offPx = shot.off;
      screenTouchCalShow(&cal);
      saveShot("%s/%s.bmp", dir, shot.name);
    }
    screenTouchCalEnd();
  }

  /* A text drawn after the last capture was written still counts. */
  reportMissing("after the last capture");
  if (sMissingCaptures > 0) {
    fprintf(stderr, "%d captures draw a character their face does not carry\n",
            sMissingCaptures);
    return 1;
  }
  return 0;
}
