/*
 * Recovery: the one screen that ignores the saved theme.
 *
 * Reached by holding the rotary button at power on. The setting that put the
 * radio here may be the theme itself, so every colour is the compiled
 * Nightwatch palette, `themeAt(0)`, and never the theme that is saved or the
 * Custom slot. `drivers/display.cpp` is not asked to apply the stored
 * rotation either, for the same reason.
 *
 * It must read when the display's colours are wrong, so what separates each
 * thing from what is behind it is brightness and not hue. The title is in
 * `measurement`, white, which stays above 11:1 with red and blue swapped or
 * with colours inverted, where a red title falls to 2.7:1. The word itself
 * says it is not the normal menu. The rows are the menu's own rows.
 */
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include "../core/strings.h"
#include "draw.h"
#include "fonts.h"
#include "screen.h"
#include "theme.h"

#define RECOVERY_W 320
#define RECOVERY_H 240

static lv_obj_t *sRoot;
static UiFrame sFrame;
static UiRow sRows[SCREEN_RECOVERY_VISIBLE];

/* Nightwatch, compiled: index 0 of the palettes, never the Custom slot. */
static const Theme *fixedTheme(void) {
  return themeAt(0);
}

bool screenRecoveryBegin(void) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sRoot != NULL) {
    return true;
  }
  const Theme *t = fixedTheme();

  lv_obj_remove_style_all(root);
  lv_obj_set_style_bg_color(root, uiColour(t->ground), 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

  sRoot = uiBlock(root, t->ground, 0, 0, RECOVERY_W, RECOVERY_H);
  uiFrameBegin(&sFrame, sRoot, t, t->measurement);
  for (uint8_t i = 0; i < SCREEN_RECOVERY_VISIBLE; i++) {
    uiRowBegin(&sRows[i], sRoot, t, UI_ROW_H, UI_ROW_PITCH);
  }
  return true;
}

void screenRecoveryShow(const ScreenRecovery *recovery) {
  if (sRoot == NULL || recovery == NULL) {
    return;
  }
  const Theme *t = fixedTheme();
  char pos[12];
  snprintf(pos, sizeof(pos), "%u/%u", (unsigned)(recovery->cursor + 1),
           (unsigned)SCREEN_RECOVERY_ROWS);
  uiFrameShow(&sFrame, txt(STR_RECOVERY_TITLE), txt(STR_RECOVERY_SAFE_DISPLAY),
              pos, NULL,
              recovery->hint != NULL ? recovery->hint : txt(STR_RECOVERY_HINT),
              NULL);

  /* Five rows at a time, the window moving only when the cursor would leave
   * it. With five rows it never moves; it is here so a sixth row can be added
   * without changing the drawing. */
  uint8_t top = 0;
  if (recovery->cursor >= SCREEN_RECOVERY_VISIBLE) {
    top = (uint8_t)(recovery->cursor - SCREEN_RECOVERY_VISIBLE + 1);
  }
  if (top > SCREEN_RECOVERY_ROWS - SCREEN_RECOVERY_VISIBLE) {
    top = SCREEN_RECOVERY_ROWS - SCREEN_RECOVERY_VISIBLE;
  }
  for (uint8_t i = 0; i < SCREEN_RECOVERY_VISIBLE; i++) {
    const uint8_t at = (uint8_t)(top + i);
    const ScreenRecoveryRow *row = &recovery->rows[at];
    if (row->name == NULL) {
      uiRowHide(&sRows[i]);
      continue;
    }
    UiRowView view;
    memset(&view, 0, sizeof(view));
    view.slot = i;
    view.name = row->name;
    view.value = row->value;
    view.cursor = at == recovery->cursor;
    uiRowShow(&sRows[i], t, &view);
  }
}

void screenRecoveryEnd(void) {
  if (sRoot == NULL) {
    return;
  }
  if (lv_obj_is_valid(sRoot)) {
    lv_obj_del(sRoot);
  }
  sRoot = NULL;
}
