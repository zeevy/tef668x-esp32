/*
 * The boot screen: what came up, and what answered.
 *
 * It is the one screen a person sees before the radio exists, so everything
 * on it is something `setup` has actually been told. The rows are the
 * questions it can answer by the time it draws them, and a row that has not
 * answered yet shows a dash rather than a tick.
 *
 * The frame every screen uses: the product on the left of the header and the
 * board on its right. The tuner the chip said it is sits on the tuner panel,
 * the way the radio screen puts what it is tuned to there. The self tests are
 * tiles, six in two columns and Touch across both under them, and the
 * bar under them fills as each one answers.
 *
 * It is built on the active screen on its own, before the radio layout
 * exists, and deleted whole when the radio takes over.
 */
#include <lvgl.h>
#include <stdio.h>

#include "../core/strings.h"
#include "draw.h"
#include "fonts.h"
#include "screen.h"
#include "theme.h"

#define BOOT_W 320
#define BOOT_H 240

/* The tuner panel. */
#define PANEL_Y 32
#define PANEL_H 36
#define PANEL_BASE 58

/* The tests: two columns of three tiles, then one across both, 28 high on a
 * 32 pitch. */
#define TEST_ROWS 3
#define TEST_TOP 76
#define TEST_PITCH 32
#define TEST_H 28
#define TEST_BASE 20

/* The bar under them, thin enough to leave the hint line its room. */
#define BAR_Y 204
#define BAR_H 8
#define BAR_R 4

static lv_obj_t *sBoot;
static UiFrame sFrame;
static lv_obj_t *sTuner;
static lv_obj_t *sTile[SCREEN_BOOT_STEPS];
static lv_obj_t *sName[SCREEN_BOOT_STEPS];
static lv_obj_t *sMark[SCREEN_BOOT_STEPS];
static lv_obj_t *sBarFill;
static char sCountText[12];

static int16_t tileW(void) {
  return (int16_t)((BOOT_W - 2 * UI_MARGIN - UI_GAP) / 2);
}

/* Tests past the two columns are a row each across both. */
static bool across(uint8_t i) {
  return i >= 2 * TEST_ROWS;
}

static int16_t tileWidth(uint8_t i) {
  return across(i) ? (int16_t)(BOOT_W - 2 * UI_MARGIN) : tileW();
}

static int16_t tileX(uint8_t i) {
  return (int16_t)(UI_MARGIN +
                   (!across(i) && i >= TEST_ROWS ? tileW() + UI_GAP : 0));
}

static int16_t tileY(uint8_t i) {
  const uint8_t row = across(i) ? (uint8_t)(i - TEST_ROWS) : i % TEST_ROWS;
  return (int16_t)(TEST_TOP + row * TEST_PITCH);
}

bool screenBootBegin(void) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sBoot != NULL) {
    return true;
  }
  const Theme *t = themeCurrent();

  /*
   * The root is set up here as well as in `screenBegin`, because this screen
   * comes first and the radio layout is not built until it goes. Only one
   * screen exists at a time: the LVGL pool does not hold two, and an
   * allocation that fails this early restarts the radio before the image
   * can mark itself good.
   */
  uiScreenRoot(root, t->ground);

  sBoot = uiBlock(root, t->ground, 0, 0, BOOT_W, BOOT_H);
  uiFrameBegin(&sFrame, sBoot, t, t->radio);

  uiRound(sBoot, t->radio, UI_MARGIN, PANEL_Y, BOOT_W - 2 * UI_MARGIN, PANEL_H,
          UI_RADIUS);
  sTuner = uiLabel(sBoot, &roboto_menu, t->ground);

  for (uint8_t i = 0; i < SCREEN_BOOT_STEPS; i++) {
    sTile[i] = uiRound(sBoot, t->rule, tileX(i), tileY(i), tileWidth(i), TEST_H,
                       UI_TILE_R);
    sName[i] = uiLabel(sBoot, &roboto_small, t->measurement);
    sMark[i] = uiLabel(sBoot, &roboto_icons, t->dead);
    uiShowIf(sTile[i], false);
    uiShowIf(sName[i], false);
    uiShowIf(sMark[i], false);
  }

  uiRound(sBoot, t->rule, UI_MARGIN, BAR_Y, BOOT_W - 2 * UI_MARGIN, BAR_H,
          BAR_R);
  sBarFill = uiRound(sBoot, t->radio, UI_MARGIN, BAR_Y, BAR_H, BAR_H, BAR_R);
  uiShowIf(sBarFill, false);
  return true;
}

void screenBootShow(const ScreenBoot *boot) {
  if (sBoot == NULL || boot == NULL) {
    return;
  }
  const Theme *t = themeCurrent();

  const uint8_t total = boot->total != 0 ? boot->total : SCREEN_BOOT_STEPS;
  const uint8_t done = boot->done > total ? total : boot->done;
  snprintf(sCountText, sizeof(sCountText), "%u/%u", (unsigned)done,
           (unsigned)total);
  uiFrameShow(&sFrame, boot->product, boot->board, NULL, NULL,
              txt(STR_BOOT_SELF_TEST), sCountText);

  /* What the tuner said it is, once it has said it, and "---" until then,
   * the way the radio screen marks a name that has not arrived. */
  uiSetText(sTuner,
            boot->tuner != NULL ? boot->tuner : txt(STR_COMMON_NO_VALUE));
  /* Centred on the panel, which spans the screen between the margins. */
  const int16_t tw = uiTextWidth(sTuner, &roboto_menu);
  uiBaseline(sTuner, &roboto_menu, (int16_t)((sFrame.w - tw) / 2), PANEL_BASE);

  for (uint8_t i = 0; i < SCREEN_BOOT_STEPS; i++) {
    const ScreenBootStep *step = &boot->steps[i];
    const bool on = step->name != NULL;
    uiShowIf(sTile[i], on);
    uiShowIf(sName[i], on);
    uiShowIf(sMark[i], on);
    if (!on) {
      continue;
    }
    const int16_t base = (int16_t)(tileY(i) + TEST_BASE);
    const int16_t right = (int16_t)(tileX(i) + tileWidth(i) - UI_PAD);
    uiSetText(sName[i], step->name);
    uiBaseline(sName[i], &roboto_small, (int16_t)(tileX(i) + UI_PAD), base);
    /*
     * A number wins over a tick. "99" says the channel store answered and
     * says what it holds; a tick beside it would say the first of those
     * twice and the second not at all.
     */
    if (step->value != NULL) {
      uiSetFont(sMark[i], &roboto_small);
      uiSetText(sMark[i], step->value);
      uiSetColour(sMark[i],
                  step->mark == SCREEN_BOOT_FAILED ? t->fault : t->measurement);
      uiBaselineRight(sMark[i], &roboto_small, right, base);
    } else if (step->mark == SCREEN_BOOT_WAITING) {
      /* The dash is not an icon, so it is set in the face the row is in. */
      uiSetFont(sMark[i], &roboto_small);
      uiSetText(sMark[i], txt(STR_COMMON_DASH));
      uiSetColour(sMark[i], t->dead);
      uiBaselineRight(sMark[i], &roboto_small, right, base);
    } else {
      uiSetFont(sMark[i], &roboto_icons);
      uiSetTextStatic(sMark[i],
                      step->mark == SCREEN_BOOT_OK ? ICON_TICK : ICON_CROSS);
      uiSetColour(sMark[i], step->mark == SCREEN_BOOT_OK ? t->good : t->fault);
      /* Placed by its box, centred on the tile, like every icon on the
       * radio screen. */
      lv_obj_set_pos(
          sMark[i], (int16_t)(right - UI_ICON_SIZE),
          (int16_t)(tileY(i) + (TEST_H - UI_ICON_SIZE) / 2 + UI_ICON_INK_DROP));
    }
  }

  /* The bar is the count, not a timer. It moves when something answers, so
   * a bar that stops is a step that did not come back, and the tiles above
   * say which. At least its own height once anything has answered, so its
   * rounded end shows. */
  const int16_t barW = (int16_t)(BOOT_W - 2 * UI_MARGIN);
  int16_t fill = (int16_t)((int32_t)barW * done / total);
  if (fill > 0 && fill < BAR_H) {
    fill = BAR_H;
  }
  uiShowIf(sBarFill, fill > 0);
  lv_obj_set_width(sBarFill, fill > 0 ? fill : BAR_H);
}

void screenBootEnd(void) {
  if (sBoot == NULL) {
    return;
  }
  /* Checked, because anything that rebuilds a layout cleans the root and
   * takes this with it. A dangling delete is a reboot rather than a mark on
   * the glass, so it is worth the one call. */
  uiDropRoot(&sBoot);
}
