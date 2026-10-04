/*
 * The drawing every panel shares.
 *
 * Colours, text placed by its baseline, boxes, and the one thing LVGL will not
 * tell you: how wide a string actually is. Nothing here knows what a radio is.
 */
#ifndef UI_DRAW_H
#define UI_DRAW_H

#include <lvgl.h>
#include <stdbool.h>
#include <stdint.h>

#include "fonts.h"
#include "theme.h"

/*
 * The spacing system of every screen, and the whole of it.
 *
 * Every gap on the panel is one of these. Nothing is spaced by eye, and
 * nothing is nudged to look right in one place at the cost of not matching
 * anywhere else.
 */
#define UI_MARGIN 12  /* Screen edge to a panel, a tile or loose text. */
#define UI_PAD 12     /* A panel's edge to its content. */
#define UI_GAP 8      /* Between groups, between tiles, between header items. */
#define UI_TIGHT 4    /* A label to its value. */
#define UI_UNIT_GAP 6 /* A big number to its unit. */
#define UI_RADIUS 10  /* The amber panel. */
#define UI_TILE_R 6   /* A tile or a menu row. */

/* The one glyph that ends a string cut short, U+2026, as UTF-8. */
#define UI_ELLIPSIS "\xE2\x80\xA6"

lv_color_t uiColour(ThemeColour c);

/* A label with no size of its own. Position it with uiBaseline. */
lv_obj_t *uiLabel(lv_obj_t *parent, const lv_font_t *font, ThemeColour c);

/* A filled rectangle. */
lv_obj_t *uiBlock(lv_obj_t *parent, ThemeColour c, int16_t x, int16_t y,
                  int16_t w, int16_t h);

/* A filled rectangle with round corners: a tile, a menu row, the panel. */
lv_obj_t *uiRound(lv_obj_t *parent, ThemeColour c, int16_t x, int16_t y,
                  int16_t w, int16_t h, int16_t radius);

/*
 * One filled rectangle inside an object's own draw event, placed against the
 * object's top left. For the parts drawn as one object rather than as many,
 * the meter's segments and the scale's marks, which keeps them out of the
 * LVGL pool.
 */
void uiFillRect(lv_layer_t *layer, const lv_area_t *obj, int16_t x, int16_t y,
                int16_t w, int16_t h, lv_color_t c);

/*
 * Put a label by its baseline.
 *
 * Every piece of text is placed by its baseline, because that is what a row of
 * different sizes lines up on. LVGL places by the top left, so this works the
 * top out from the font rather than from a number typed in here, and a change
 * of face or size stays lined up.
 *
 * `x` is the left edge of the text. Nothing here touches the alignment style
 * or the width, on purpose: `lv_obj_set_local_style_prop` does not compare the
 * value it is given, so setting an alignment that has not changed still
 * invalidates the object and marks the layout dirty. `lv_obj_set_pos` does
 * compare, so a label that has not moved costs nothing. These run on about
 * thirty labels twenty five times a second.
 */
void uiBaseline(lv_obj_t *label, const lv_font_t *font, int16_t x,
                int16_t baselineY);

/*
 * The same, with `x` as the right edge of the text.
 *
 * The label is moved rather than being made into a wide box with the text
 * pushed to one end of it. A label right aligned inside a box that reaches
 * the screen edge invalidates the whole box when its text changes, so a 30
 * pixel number would redraw a 268 pixel strip.
 */
void uiBaselineRight(lv_obj_t *label, const lv_font_t *font, int16_t x,
                     int16_t baselineY);

/*
 * How wide a label's text actually is.
 *
 * Not `lv_obj_get_width`. A right aligned label is given the whole width to
 * the left of its anchor, so its width is the box and not the letters.
 */
int16_t uiTextWidth(lv_obj_t *label, const lv_font_t *font);

/*
 * `text` as it fits in `room` pixels: unchanged when it fits, otherwise the
 * longest start of it that fits with an ellipsis after it, written into
 * `out`. Only the 13, 15, 17 and 21 pixel faces carry the ellipsis glyph.
 *
 * Done here rather than with LVGL's own dot mode, which writes the dots into
 * the label's text. The label's text then never matches the string it was
 * given, so `uiSetText` would set it again, and redraw it, on every poll.
 */
const char *uiFitText(const char *text, const lv_font_t *font, int16_t room,
                      char *out, size_t cap);

/*
 * `text` with a newline wherever a line would pass `width`, breaking only at
 * spaces, written into `out`.
 *
 * LVGL's own wrapping also breaks after a full stop, `LV_TXT_BREAK_CHARS` in
 * lv_conf.h, which cuts a frequency like 93.50 in two across lines and reads
 * as two numbers. Lines packed here always fit, so LVGL never has to break
 * one. Stops early, cleanly between two words, when `out` is full.
 */
const char *uiWrapAtSpaces(const char *text, const lv_font_t *font,
                           int16_t width, char *out, size_t cap);

/* The top of a label whose baseline is `baselineY`, for text drawn inside
 * a draw event rather than placed with uiBaseline. */
int16_t uiRowTop(const lv_font_t *font, int16_t baselineY);

/* The top of a symbol's box beside text whose baseline is `baselineY`. The
 * box's middle sits 4 rows above the baseline, which centres the ink on the
 * capitals of the words beside it. */
int16_t uiIconTop(int16_t baselineY);

/*
 * Set a label, but only when the text has actually changed.
 *
 * `lv_label_set_text` does not compare. It frees the old string, mallocs a new
 * one and invalidates the object, every single time. This runs on about thirty
 * labels twenty five times a second out of a 24 KB pool, and dirties most of
 * the screen on every pass. The compare is the whole reason this exists.
 */
void uiSetText(lv_obj_t *o, const char *text);

/* Set a label, or hide it when there is nothing to say. A blank label and a
 * label the radio cannot fill are different things, and an empty string on
 * screen looks like the first. */
void uiSetOrHide(lv_obj_t *o, const char *text);

void uiShowIf(lv_obj_t *o, bool on);

/*
 * Style setters that compare before they write.
 *
 * `lv_obj_set_local_style_prop` does not compare the value it is given: it
 * always refreshes the style, which invalidates the object, and the layout
 * properties mark the screen layout dirty as well. Setting a colour to the
 * colour it already has therefore costs a redraw. These run on about thirty
 * objects twenty five times a second, and almost nothing changes between one
 * pass and the next, so the compare is what keeps the panel still.
 *
 * `lv_obj_set_pos` and `lv_obj_set_width` already compare, so those are used
 * directly.
 */
void uiSetColour(lv_obj_t *o, ThemeColour c);
void uiSetBgColour(lv_obj_t *o, ThemeColour c);
void uiSetBorderColour(lv_obj_t *o, ThemeColour c);
void uiSetFont(lv_obj_t *o, const lv_font_t *font);

/*
 * Point a label at a string it does not own, but only when it changed.
 *
 * For the icon glyphs, which are literals. `lv_label_set_text_static` does not
 * compare either, and always ends in a re-layout and an invalidation.
 */
void uiSetTextStatic(lv_obj_t *o, const char *text);

/*
 * The frame every full screen shares: a title on the left of the header, a
 * run of context, page position and clock ending at the right margin, and
 * the two hints along the bottom. Any part left NULL is left out and the run
 * closes up.
 */
#define UI_HEAD_TITLE_BASE 21
#define UI_HEAD_RUN_BASE 20
/*
 * A Material Symbol's box, which the glyph fills, on every screen. Its ink
 * sits a row above the middle of the box, measured off the renderer, so a
 * symbol centred on a line has its box UI_ICON_INK_DROP rows lower. In the
 * header the line is 14 rows down, so the box starts at row 7.
 */
#define UI_ICON_SIZE 16
#define UI_ICON_INK_DROP 1
#define UI_HEAD_ICON_TOP 7
#define UI_HINT_BASE 228

typedef struct {
  lv_obj_t *title;
  lv_obj_t *context;
  lv_obj_t *position;
  lv_obj_t *clock;
  lv_obj_t *hintLeft;
  lv_obj_t *hintRight;
  lv_obj_t *sleep; /* The sleep mark, left of the run. */
  int16_t w;       /* The parent's width, read once when it was built. */
} UiFrame;

void uiFrameBegin(UiFrame *f, lv_obj_t *parent, const Theme *t,
                  ThemeColour title);

/*
 * The person in bed every header shows while auto off is on: grey, and in
 * radio for the last five minutes before the radio sleeps, when a key keeps
 * it awake. One setting for every screen, set by the sleep task, because
 * which screen is up does not change whether auto off is on.
 */
typedef enum {
  UI_SLEEP_NONE = 0, /* Auto off is off. */
  UI_SLEEP_ON,       /* On, more than five minutes to go. */
  UI_SLEEP_SOON      /* The last five minutes. */
} UiSleepMark;

void uiSetSleepMark(UiSleepMark mark);
UiSleepMark uiSleepMark(void);

/* Show or hide one screen's copy of the mark, in its colour. True when it is
 * shown, so the caller places it. */
bool uiShowSleepMark(lv_obj_t *o);
void uiFrameShow(UiFrame *f, const char *title, const char *context,
                 const char *position, const char *clock, const char *hintLeft,
                 const char *hintRight);

/*
 * `text` in the label face, cut to fit a header's run from `left` to the
 * right margin of a screen `w` wide, with room kept for the sleep mark while
 * it shows. For a header's text that can be long, such as a moment's message
 * in place of the page and the clock.
 */
const char *uiFitHeaderText(const char *text, int16_t w, int16_t left,
                            char *out, size_t cap);

/*
 * One list row: a tile of `rule`, 296 wide, from y 32, and `radio` with
 * `ground` type on the row the knob is on. The name is 17 pixels on the left;
 * a run on the right holds, from the edge inwards, a chevron for a row that
 * opens a list, the theme picker's four swatches, a tick for the saved
 * choice, and the value.
 *
 * 30 high and 36 apart, five to a screen, on the recovery screen, which
 * keeps a line of hints under them. The menu and the Catches page have no
 * line under their rows, so they take six, 29 high and 4 apart: the last
 * ends at y 226, 14 above the edge, about the 12 at either side.
 */
#define UI_ROW_TOP 32
#define UI_ROW_H 30
#define UI_ROW_PITCH 36
#define UI_MENU_ROW_H 29
#define UI_MENU_ROW_PITCH 33

typedef struct {
  lv_obj_t *tile;
  lv_obj_t *name;
  lv_obj_t *value;
  lv_obj_t *tick;
  lv_obj_t *chevron;
  lv_obj_t *swatch;
  int16_t w;     /* The parent's width, read once when it was built. */
  int16_t h;     /* The tile's height. */
  int16_t pitch; /* From one row's top to the next one's. */
  uint8_t themeIndex;
  char fit[48];
} UiRow;

void uiRowBegin(UiRow *r, lv_obj_t *parent, const Theme *t, int16_t h,
                int16_t pitch);
void uiRowHide(UiRow *r);

/* What one row shows. Cleared to zero, it is a plain row with no value. */
typedef struct {
  uint8_t slot;      /* Which row, 0 the top. */
  const char *name;  /* The name at the left. */
  const char *value; /* The value at the right, or NULL. */
  bool cursor;       /* The cursor is on it. */
  bool dimValue;     /* The value in the quiet colour. */
  bool opens;        /* It opens a deeper level: a chevron at the right. */
  bool saved;        /* The stored choice in a list: a tick. */
  bool swatch;       /* The colours of theme `swatchTheme` at the right. */
  uint8_t swatchTheme;
} UiRowView;

void uiRowShow(UiRow *r, const Theme *t, const UiRowView *v);

/* One text on the panel as it is drawn, for uiReadPanel. */
typedef struct {
  const char *text;
  int16_t x, y, w, h; /* Where the label sits, in panel pixels. */
  ThemeColour colour; /* Its text colour as drawn. */
  /* Every role of the current theme with that colour, "measurement" or,
   * where a theme gives two roles one colour, "radio|measurement". Empty
   * for a colour that is none of them. */
  const char *role;
  const char *font; /* The face, "small" for roboto_small, or "". */
} UiText;

/*
 * A label that shows the access PIN. GET /api/screen needs no PIN, so
 * uiReadPanel gives such a label as one star for each character, with where
 * it sits, its colour and its face kept.
 */
#define UI_FLAG_SECRET LV_OBJ_FLAG_USER_1

/*
 * One box on the panel as it is drawn, for uiReadPanel: any object whose
 * background or border shows, a label with a fill or a border of its own
 * among them. The roles are as for UiText.
 */
typedef struct {
  int16_t x, y, w, h;
  bool filled; /* Its background shows. */
  ThemeColour fill;
  const char *fillRole;
  uint8_t border; /* Its border's width in pixels, 0 for none. */
  ThemeColour borderColour;
  const char *borderRole;
} UiBox;

/*
 * Every text and every box the panel shows now, read off LVGL's own objects
 * on the active screen and its top layer, in the order LVGL keeps them, for
 * GET /api/screen. Read from what was drawn rather than from what a screen
 * was asked to show, so a text a screen meant to hide and did not is there
 * too, and a box still in the colours of a theme no longer in use. A hidden
 * object and everything under it is left out, and so is an empty label. A
 * label flagged UI_FLAG_SECRET comes as stars. `texts` gets each text and
 * `boxes` each box in turn; either may be NULL.
 *
 * What a draw callback paints, the meter, the scale and the graphs, is not
 * an object and is not read. Each takes the current theme as it draws.
 */
typedef void (*UiTextSink)(void *ctx, const UiText *t);
typedef void (*UiBoxSink)(void *ctx, const UiBox *b);
void uiReadPanel(UiTextSink texts, UiBoxSink boxes, void *ctx);

#endif /* UI_DRAW_H */
