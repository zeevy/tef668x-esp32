/*
 * The control API: the panel as a script sees it and presses it: what it
 * shows, its keys and its touch screen.
 */
#include "web_api_internal.h"
#include "web_internal.h"

#include "board/board.h"
#include "input_task.h"
#include "screen_task.h"
#include "ui/draw.h"

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

/* A theme colour as #RRGGBB, into `out`, which holds 8. */
static void hexColour(ThemeColour c, char *out) {
  snprintf(out, 8, "#%06lX",
           (unsigned long)(lv_color_to_u32(uiColour(c)) & 0xFFFFFFUL));
}

static void sendText(void *ctx, const UiText *t) {
  (void)ctx;
  /* Room for a role naming all nine, 62 characters, which a Custom theme
   * with one colour for every role gives. */
  char head[192];
  char colour[8];
  hexColour(t->colour, colour);
  snprintf(head, sizeof(head),
           "{\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d,"
           "\"c\":\"%s\",\"role\":\"%s\",\"font\":\"%s\","
           "\"text\":",
           (int)t->x, (int)t->y, (int)t->w, (int)t->h, colour, t->role,
           t->font);
  sWeb->server.sendContent(head);
  sWeb->server.sendContent("\"" + jsonEscape(t->text) + "\"}\n");
}

/* A box's line has `fill` and `fillRole` when its background shows, and
 * `border`, its width, with `borderC` and `borderRole` when its border
 * does. No `text`, which is how a reader tells the two kinds apart. */
static void sendBox(void *ctx, const UiBox *b) {
  (void)ctx;
  /* Room for both roles naming all nine, as for a text, and each part is
   * added only while there is room, so a line can come out short but never
   * runs past the end. */
  char line[320];
  size_t at = (size_t)snprintf(line, sizeof(line),
                               "{\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d",
                               (int)b->x, (int)b->y, (int)b->w, (int)b->h);
  char colour[8];
  if (b->filled && at < sizeof(line)) {
    hexColour(b->fill, colour);
    at += (size_t)snprintf(line + at, sizeof(line) - at,
                           ",\"fill\":\"%s\",\"fillRole\":\"%s\"", colour,
                           b->fillRole);
  }
  if (b->border > 0 && at < sizeof(line)) {
    hexColour(b->borderColour, colour);
    at += (size_t)snprintf(
        line + at, sizeof(line) - at,
        ",\"border\":%u,\"borderC\":\"%s\",\"borderRole\":\"%s\"",
        (unsigned)b->border, colour, b->borderRole);
  }
  if (at < sizeof(line)) {
    snprintf(line + at, sizeof(line) - at, "}\n");
  }
  sWeb->server.sendContent(line);
}

/*
 * GET /api/screen. What the panel shows now, one compact JSON object a
 * line: first which screen and page, whether the panel is dimmed and the
 * theme, then one line for each text and each box, read off LVGL's own
 * objects by uiReadPanel: a text with where it is, its colour, the theme
 * roles that colour is and its face, and a box with where it is and the
 * colours and roles of its fill and its border. So a check of the panel can
 * be run from here, without a person reading it. Open to read, like
 * `/api/state`: the panel shows readings and settings, never the Wi-Fi
 * passphrase, and the PIN, which the menu shows on its Web PIN row and
 * editor, comes here as stars.
 */
static void handleApiScreenGet(void) {
  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "application/x-ndjson", "");
  uint8_t page = 0;
  const char *screen = screenTaskShowing(&page);
  char line[160];
  snprintf(line, sizeof(line),
           "{\"screen\":\"%s\",\"page\":%u,\"dim\":%s,\"theme\":\"%s\"}\n",
           screen, (unsigned)page,
           screenTaskBacklightState(NULL) ? "true" : "false",
           themeCurrent()->name);
  sWeb->server.sendContent(line);
  /* Then the parts a touch acts on, before what is drawn. */
  InputZone zones[16];
  const int n = inputScreenZones(zones, 16);
  for (int i = 0; i < n; i++) {
    snprintf(line, sizeof(line),
             "{\"zone\":\"%s\",\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d}\n",
             zones[i].name, zones[i].zone.x, zones[i].zone.y, zones[i].zone.w,
             zones[i].zone.h);
    sWeb->server.sendContent(line);
  }
  uiReadPanel(sendText, sendBox, NULL);
  sWeb->server.sendContent("");
}

/*
 * POST /api/key. A press or a turn, as if made at the panel:
 * `k` is BAND, BW, MODE, PUSH, ENTER, DX or a digit 0 to 9, and `e` short,
 * the default, or long; or `turn` is the knob's clicks, -20 to 20 and not 0.
 * It is handled at the next input poll the way the real one would be, and
 * the answer says only that it was taken; GET /api/screen and
 * `inp.lst`, which starts "api ", say what it did.
 */
static void handleApiKey(void) {
  if (!requireAuth(false)) {
    return;
  }
  if (sWeb->server.hasArg("turn")) {
    long clicks = 0;
    if (!apiNumber("turn", &clicks, -20, 20)) {
      return;
    }
    if (clicks == 0) {
      apiFail(400, "turn has to be a number of clicks, not 0.");
      return;
    }
    if (!inputTurnFromApi((int32_t)clicks)) {
      apiFail(409, "The last key sent has not been handled yet. Send again.");
      return;
    }
    sWeb->server.send(200, "text/plain", String("turned ") + clicks + "\n");
    return;
  }
  InputKey key;
  if (!inputKeyFromName(sWeb->server.arg("k").c_str(), &key)) {
    apiFail(400, "Give k, one of BAND BW MODE PUSH ENTER DX 0 to 9, or turn.");
    return;
  }
  ButtonEvent event = BUTTON_SHORT;
  if (sWeb->server.hasArg("e") &&
      !buttonEventFromName(sWeb->server.arg("e").c_str(), &event)) {
    apiFail(400, "e has to be short or long.");
    return;
  }
  if (event == BUTTON_LONG && !inputKeyTakesLong(key)) {
    apiFail(400, "That key has no long press.");
    return;
  }
  if (!inputPressFromApi(key, event)) {
    apiFail(409, "The last key sent has not been handled yet. Send again.");
    return;
  }
  sWeb->server.send(200, "text/plain",
                    String("pressed ") + inputKeyName(key) + " " +
                        buttonEventName(event) + "\n");
}

#if FEATURE_TOUCH
/*
 * POST /api/touch. A gesture on the screen, as if made on the glass: `x`
 * and `y` in screen pixels, and `g` tap, hold, swipe-left, swipe-right,
 * swipe-up or swipe-down, where a swipe starts, or drag from there to `x2`,
 * `y2`. Handled at the next input
 * poll like a key; `inp.lst` then says what it did: "api touch tap" and the
 * like, or "api DX scan stopped" or "api typed number cleared" when that is
 * all it did. Refused, with the reason, wherever a finger on the glass would
 * do nothing.
 */
static void handleApiTouch(void) {
  static const struct {
    const char *name;
    TouchGestureEvent event;
  } kGestures[] = {{"tap", TOUCH_TAP},
                   {"hold", TOUCH_HOLD},
                   {"swipe-left", TOUCH_SWIPE_LEFT},
                   {"swipe-right", TOUCH_SWIPE_RIGHT},
                   {"swipe-up", TOUCH_SWIPE_UP},
                   {"swipe-down", TOUCH_SWIPE_DOWN},
                   {"drag", TOUCH_DRAG}};
  if (!requireAuth(false)) {
    return;
  }
  long x = 0;
  long y = 0;
  if (!apiNumber("x", &x, 0, DISPLAY_WIDTH - 1) ||
      !apiNumber("y", &y, 0, DISPLAY_HEIGHT - 1)) {
    return;
  }
  const String want = sWeb->server.arg("g");
  TouchGestureEvent event = TOUCH_NOTHING;
  const char *name = NULL;
  for (const auto &g : kGestures) {
    if (want.equalsIgnoreCase(g.name)) {
      event = g.event;
      name = g.name;
    }
  }
  if (event == TOUCH_NOTHING) {
    apiFail(400,
            "Give g, one of tap hold swipe-left swipe-right swipe-up "
            "swipe-down drag.");
    return;
  }
  long x2 = x;
  long y2 = y;
  if (event == TOUCH_DRAG && (!apiNumber("x2", &x2, 0, DISPLAY_WIDTH - 1) ||
                              !apiNumber("y2", &y2, 0, DISPLAY_HEIGHT - 1))) {
    return;
  }
  const TouchPoint at = {(int16_t)x, (int16_t)y};
  const TouchPoint to = {(int16_t)x2, (int16_t)y2};
  switch (inputTouchFromApi(event, at, to)) {
    case INPUT_TOUCH_TAKEN:
      break;
    case INPUT_TOUCH_BUSY:
      apiFail(409, "The last key sent has not been handled yet. Send again.");
      return;
    case INPUT_TOUCH_OFF:
      apiFail(409, "Touch is Off, so the radio does not act on a touch.");
      return;
    case INPUT_TOUCH_NOT_NOW:
      apiFail(409,
              "The screen is dark, starting up, going to sleep or held by an "
              "update, and a touch does nothing then. A key ends that.");
      return;
    case INPUT_TOUCH_CALIBRATING:
      apiFail(409, "The calibration screen takes only a finger on the glass.");
      return;
    case INPUT_TOUCH_NOT_HERE:
      apiFail(409,
              "A finger cannot make that there. A drag starts on a part that "
              "follows one and moves more than 16 px, and such a part takes "
              "no swipe.");
      return;
  }
  sWeb->server.send(200, "text/plain",
                    String("touched ") + name + " at " + x + "," + y + "\n");
}
#endif

void webApiScreenRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/api/screen", HTTP_GET, handleApiScreenGet);
  sWeb->server.on("/api/key", HTTP_POST, handleApiKey);
#if FEATURE_TOUCH
  sWeb->server.on("/api/touch", HTTP_POST, handleApiTouch);
#endif
}
