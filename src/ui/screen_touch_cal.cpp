/*
 * The touch calibration screen.
 *
 * The whole glass, with no header, so the marks can sit near its corners:
 * a ring at the mark to hold, whose disc grows as the mark fills, a tick at
 * each mark done, five dots for how far along it is, and the instruction in
 * the half of the screen away from the mark. Then a dot to tap as a check,
 * then whether the calibration was kept.
 *
 * It draws what it is given, and owns the panel on its own, because the
 * LVGL pool holds one screen at a time.
 */
#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>

#include "../core/strings.h"
#include "draw.h"
#include "fonts.h"
#include "screen.h"
#include "theme.h"

#define CAL_W 320
#define CAL_H 240

/* The ring at a mark: 35 px across, a 3 px band, and a disc inside that
 * grows from 7 px as the mark fills until it fills the ring's 29 px. */
#define RING 35
#define RING_IN 29
#define DISC_MIN 7
#define DISC_MAX RING_IN
/* The check dot. */
#define DOT 17
/* The five progress dots: 11 px, 18 apart. */
#define PIP 11
#define PIP_PITCH 18
/* Where the text block's instruction sits: a little under the middle, or
 * near the top while the middle mark is held, clear of its ring. */
#define TEXT_BASE 112
#define TEXT_BASE_HIGH 66
#define TITLE_ABOVE 30
#define PIPS_BELOW 26
#define HINT_BASE 228
/* The result: its title, and one or two lines under it. */
#define RESULT_BASE 100
#define RESULT_LINE 32
#define RESULT_PITCH 22

typedef struct {
  bool recovery;
  lv_obj_t *root;
  lv_obj_t *ring;
  lv_obj_t *ringIn;
  lv_obj_t *disc;
  lv_obj_t *dot;
  lv_obj_t *tick[SCREEN_TOUCH_CAL_MARKS];
  lv_obj_t *pip[SCREEN_TOUCH_CAL_MARKS];
  lv_obj_t *title;
  lv_obj_t *line1;
  lv_obj_t *line2;
  lv_obj_t *hint;
} CalUi;
static CalUi *sUi;

static const Theme *theme(void) {
  return sUi != NULL && sUi->recovery ? themeAt(0) : themeCurrent();
}

bool screenTouchCalBegin(bool recovery) {
  lv_obj_t *root = lv_screen_active();
  if (root == NULL) {
    return false;
  }
  if (sUi != NULL) {
    return true;
  }
  sUi = (CalUi *)calloc(1, sizeof(*sUi));
  if (sUi == NULL) {
    return false;
  }
  sUi->recovery = recovery;
  const Theme *t = theme();
  lv_obj_remove_style_all(root);
  lv_obj_set_style_bg_color(root, uiColour(t->ground), 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

  sUi->root = uiBlock(root, t->ground, 0, 0, CAL_W, CAL_H);
  sUi->ring = uiRound(sUi->root, t->radio, 0, 0, RING, RING, RING / 2 + 1);
  sUi->ringIn =
      uiRound(sUi->root, t->ground, 0, 0, RING_IN, RING_IN, RING_IN / 2 + 1);
  sUi->disc = uiRound(sUi->root, t->radio, 0, 0, DISC_MIN, DISC_MIN, DISC_MIN);
  sUi->dot = uiRound(sUi->root, t->measurement, 0, 0, DOT, DOT, DOT / 2 + 1);
  for (uint8_t i = 0; i < SCREEN_TOUCH_CAL_MARKS; i++) {
    sUi->tick[i] = uiLabel(sUi->root, &roboto_icons, t->good);
    uiSetText(sUi->tick[i], ICON_TICK);
    sUi->pip[i] = uiRound(sUi->root, t->dead, 0, 0, PIP, PIP, PIP / 2 + 1);
  }
  sUi->title = uiLabel(sUi->root, &roboto_title, t->radio);
  sUi->line1 = uiLabel(sUi->root, &roboto_text, t->measurement);
  sUi->line2 = uiLabel(sUi->root, &roboto_text, t->measurement);
  sUi->hint = uiLabel(sUi->root, &roboto_label, t->dead);
  return true;
}

/* `o` centred across the screen on `baseline`, in `font`. */
static void centre(lv_obj_t *o, const lv_font_t *font, int16_t baseline) {
  const int16_t w = uiTextWidth(o, font);
  uiBaseline(o, font, (int16_t)((CAL_W - w) / 2), baseline);
}

/* A round object `size` across, centred on (x, y). */
static void around(lv_obj_t *o, int16_t size, int16_t x, int16_t y) {
  lv_obj_set_size(o, size, size);
  lv_obj_set_style_radius(o, size / 2 + 1, 0);
  lv_obj_set_pos(o, (int16_t)(x - size / 2), (int16_t)(y - size / 2));
}

static void showMarks(const ScreenTouchCal *s) {
  const bool marking = s->step == SCREEN_TOUCH_CAL_MARK;
  const uint8_t done = s->mark;
  const bool checking = s->step == SCREEN_TOUCH_CAL_CHECK;
  for (uint8_t i = 0; i < SCREEN_TOUCH_CAL_MARKS; i++) {
    uiShowIf(sUi->tick[i], marking && i < done);
    if (i < done) {
      uiBaseline(sUi->tick[i], &roboto_icons, (int16_t)(s->markX[i] - 8),
                 (int16_t)(s->markY[i] + 6));
    }
  }
  uiShowIf(sUi->ring, marking);
  uiShowIf(sUi->ringIn, marking);
  uiShowIf(sUi->disc, marking);
  if (marking) {
    const int16_t x = s->markX[s->mark];
    const int16_t y = s->markY[s->mark];
    const int16_t pct = s->fillPct > 100 ? 100 : s->fillPct;
    around(sUi->ring, RING, x, y);
    around(sUi->ringIn, RING_IN, x, y);
    around(sUi->disc, (int16_t)(DISC_MIN + (DISC_MAX - DISC_MIN) * pct / 100),
           x, y);
  }
  uiShowIf(sUi->dot, checking);
  if (checking) {
    around(sUi->dot, DOT, s->dotX, s->dotY);
  }
}

static void showPips(const ScreenTouchCal *s, const Theme *t, bool shown,
                     int16_t baseline) {
  const int16_t x0 = (int16_t)(CAL_W / 2 - 2 * PIP_PITCH - PIP / 2);
  for (uint8_t i = 0; i < SCREEN_TOUCH_CAL_MARKS; i++) {
    uiShowIf(sUi->pip[i], shown);
    if (!shown) {
      continue;
    }
    uiSetBgColour(sUi->pip[i], i < s->mark    ? t->good
                               : i == s->mark ? t->radio
                                              : t->dead);
    lv_obj_set_pos(sUi->pip[i], (int16_t)(x0 + i * PIP_PITCH),
                   (int16_t)(baseline - PIP + 1));
  }
}

void screenTouchCalShow(const ScreenTouchCal *s) {
  if (sUi == NULL || s == NULL) {
    return;
  }
  const Theme *t = theme();
  showMarks(s);
  char line[48];
  switch (s->step) {
    case SCREEN_TOUCH_CAL_MARK: {
      const int16_t base =
          s->mark == SCREEN_TOUCH_CAL_MARKS - 1 ? TEXT_BASE_HIGH : TEXT_BASE;
      uiSetColour(sUi->title, t->radio);
      uiSetText(sUi->title, txt(STR_TOUCH_CAL_TITLE));
      centre(sUi->title, &roboto_title, (int16_t)(base - TITLE_ABOVE));
      uiSetText(sUi->line1, txt(STR_TOUCH_CAL_HOLD));
      centre(sUi->line1, &roboto_text, base);
      uiShowIf(sUi->line2, false);
      showPips(s, t, true, (int16_t)(base + PIPS_BELOW));
      uiSetText(sUi->hint, txt(STR_TOUCH_CAL_CANCEL_HINT));
      break;
    }
    case SCREEN_TOUCH_CAL_CHECK:
      uiSetColour(sUi->title, t->radio);
      uiSetText(sUi->title, txt(STR_TOUCH_CAL_CHECK));
      centre(sUi->title, &roboto_title, (int16_t)(TEXT_BASE + 20));
      uiSetText(sUi->line1, txt(STR_TOUCH_CAL_TAP_DOT));
      centre(sUi->line1, &roboto_text, (int16_t)(TEXT_BASE + 48));
      uiShowIf(sUi->line2, false);
      showPips(s, t, false, 0);
      uiSetText(sUi->hint, txt(STR_TOUCH_CAL_CANCEL_HINT));
      break;
    case SCREEN_TOUCH_CAL_KEPT:
      uiSetColour(sUi->title, t->good);
      uiSetText(sUi->title, txt(STR_TOUCH_CAL_KEPT));
      centre(sUi->title, &roboto_title, RESULT_BASE);
      snprintf(line, sizeof(line), txt(STR_TOUCH_CAL_FMT_LANDED),
               (unsigned)s->offPx);
      uiSetText(sUi->line1, line);
      centre(sUi->line1, &roboto_text, (int16_t)(RESULT_BASE + RESULT_LINE));
      uiShowIf(sUi->line2, false);
      showPips(s, t, false, 0);
      uiSetText(sUi->hint, txt(STR_TOUCH_CAL_BACK_HINT));
      break;
    default:
      uiSetColour(sUi->title, t->fault);
      uiSetText(sUi->title, txt(STR_TOUCH_CAL_NOT_KEPT));
      centre(sUi->title, &roboto_title, RESULT_BASE);
      if (s->step == SCREEN_TOUCH_CAL_MISSED) {
        snprintf(line, sizeof(line), txt(STR_TOUCH_CAL_FMT_MISSED),
                 (unsigned)s->offPx);
        uiSetText(sUi->line1, line);
      } else if (s->step == SCREEN_TOUCH_CAL_NOT_SAVED) {
        uiSetText(sUi->line1, txt(STR_TOUCH_CAL_NOT_SAVED));
      } else {
        uiSetText(sUi->line1, txt(STR_TOUCH_CAL_NO_FIT));
      }
      centre(sUi->line1, &roboto_text, (int16_t)(RESULT_BASE + RESULT_LINE));
      uiShowIf(sUi->line2, true);
      uiSetText(sUi->line2, txt(STR_TOUCH_CAL_OLD_STAYS));
      centre(sUi->line2, &roboto_text,
             (int16_t)(RESULT_BASE + RESULT_LINE + RESULT_PITCH));
      showPips(s, t, false, 0);
      uiSetText(sUi->hint, txt(STR_TOUCH_CAL_RETRY_HINT));
      break;
  }
  uiShowIf(sUi->line1, true);
  centre(sUi->hint, &roboto_label, HINT_BASE);
}

void screenTouchCalEnd(void) {
  if (sUi == NULL) {
    return;
  }
  if (lv_obj_is_valid(sUi->root)) {
    lv_obj_del(sUi->root);
  }
  free(sUi);
  sUi = NULL;
}
