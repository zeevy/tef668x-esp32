/* Implementation of the status page, the setup form and the uploader. */
#include "web_update.h"

#include "board/board.h"
#include "core/access_pin.h"
#include "core/backlight.h"
#include "core/band_plan.h"
#include "core/input.h"
#include "core/signal.h"
#include "core/squelch.h"
#include "core/version.h"
#include "drivers/settings_nvs.h"
#include "drivers/tef668x.h"
#include "input_task.h"
#include "memory_store.h"
#include "net/rollback.h"
#include "net/wifi_manager.h"
#include "radio_task.h"
#include "screen_task.h"
#include "settings_task.h"

#include <Update.h>
#include <WebServer.h>
#include <esp_random.h>
#include <esp_system.h>

static WebServer sServer(80);
static Settings *sSettings = NULL;
static uint32_t sAccessPin = 0;
static AccessPinGate sGate;
static uint32_t sRequests = 0;

/* Hex session token, or empty when nobody is signed in. */
static char sSessionToken[33] = "";

/* Millisecond count the session stops being accepted at. */
static uint32_t sSessionExpiresMs = 0;

/* Set while an upload is running and the client was not signed in. */
static bool sUploadRejected = false;

/* Set once a multipart part carrying a file has actually been seen. */
static bool sUploadStarted = false;

/* Set on the first write failure, so the rest of the body is taken quietly. */
static bool sUploadFailed = false;

/* Reboot once the reply has gone out, rather than cutting it off. */
static bool sRebootAfterReply = false;

/* ------------------------------------------------------------------ helpers */

static bool inSetupMode(void) {
  return wifiState() == WIFI_STATE_ACCESS_POINT;
}

static String escapeHtml(const char *raw) {
  String out;
  for (const char *p = raw; *p != '\0'; p++) {
    switch (*p) {
      case '&':
        out += F("&amp;");
        break;
      case '<':
        out += F("&lt;");
        break;
      case '>':
        out += F("&gt;");
        break;
      case '"':
        out += F("&quot;");
        break;
      case '\'':
        out += F("&#39;");
        break;
      default:
        out += *p;
        break;
    }
  }
  return out;
}

/*
 * Escape a string so it can go inside a JSON string.
 *
 * A network name is chosen by whoever runs the network, so it can hold a
 * quote or a backslash and there is nothing wrong with that. Without this the
 * whole document stops parsing, which is how a missing quote broke every
 * reader of /status.json once already.
 */
static String jsonEscape(const char *raw) {
  String out;
  for (const char *p = raw; *p != '\0'; p++) {
    unsigned char c = (unsigned char)*p;
    if (c == '"' || c == '\\') {
      out += '\\';
      out += (char)c;
    } else if (c < 0x20) {
      /* Control characters have no place in a name, but a corrupt NVS blob
       * can still hold one, and it has to come out as valid JSON. */
      char esc[7];
      snprintf(esc, sizeof(esc), "\\u%04x", c);
      out += esc;
    } else {
      out += (char)c;
    }
  }
  return out;
}

static void newSession(void) {
  static const char kHex[] = "0123456789abcdef";
  for (int i = 0; i < 32; i += 8) {
    uint32_t chunk = esp_random();
    for (int b = 0; b < 8; b++) {
      sSessionToken[i + b] = kHex[(chunk >> (b * 4)) & 0x0F];
    }
  }
  sSessionToken[32] = '\0';
  sSessionExpiresMs = millis() + WEB_SESSION_TTL_SECONDS * 1000UL;
}

static void dropSession(void) {
  sSessionToken[0] = '\0';
  sSessionExpiresMs = 0;
}

static bool constantTimeEqual(const char *a, const char *b, size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; i++) {
    diff |= (uint8_t)(a[i] ^ b[i]);
  }
  return diff == 0;
}

static bool signedIn(void) {
  if (sSessionToken[0] == '\0') {
    return false;
  }
  if ((int32_t)(millis() - sSessionExpiresMs) >= 0) {
    dropSession();
    return false;
  }
  if (!sServer.hasHeader("Cookie")) {
    return false;
  }
  String cookie = sServer.header("Cookie");
  /* Match the name at the start of the header or after a separator, so a
   * cookie called mytefsid is not mistaken for ours. */
  int at = cookie.startsWith("tefsid=") ? 0 : cookie.indexOf("; tefsid=");
  if (at < 0) {
    return false;
  }
  if (at > 0) {
    at += 2;
  }
  String token = cookie.substring(at + 7);
  int end = token.indexOf(';');
  if (end >= 0) {
    token = token.substring(0, end);
  }
  if (token.length() != 32) {
    return false;
  }
  return constantTimeEqual(token.c_str(), sSessionToken, 32);
}

static bool requireAuth(bool allowInSetupMode) {
  if (allowInSetupMode && inSetupMode()) {
    return true;
  }
  if (signedIn()) {
    return true;
  }
  sServer.send(403, "text/plain", "Enter the access PIN first.\n");
  return false;
}

/* --------------------------------------------------------------- the pages */

/*
 * Shared page head.
 *
 * Bootstrap 5 comes from a CDN, which is what makes this readable on a phone
 * without shipping a stylesheet. The catch is that the setup page is served
 * on the radio's own access point, where there is no internet and the CDN
 * cannot load. So the small stylesheet below is not decoration: it is what
 * the page falls back to, and it has to be enough on its own. Phase 5 moves
 * Bootstrap onto the filesystem and drops the CDN.
 */
static String pageHead(const char *title, const char *active = NULL);

static String navLink(const char *path, const char *label, const char *active) {
  bool here = active != NULL && strcmp(path, active) == 0;
  String out = F("<a href='");
  out += path;
  out += here ? F("' class=here>") : F("'>");
  out += label;
  out += F("</a>");
  return out;
}

static String pageHead(const char *title, const char *active) {
  String out;
  out.reserve(1500);
  out +=
      F("<!doctype html><html lang=en data-bs-theme=dark><head>"
        "<meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>");
  out += title;
  out +=
      F("</title>"
        "<link rel=stylesheet crossorigin=anonymous "
        "href='https://cdn.jsdelivr.net/npm/bootstrap@" BOOTSTRAP_VERSION
        "/dist/css/bootstrap.min.css'>"
        "<style>"
        /* Enough on its own when the CDN cannot be reached. */
        "body{background:#0b0f13;color:#e6e6e6;"
        "font:15px/1.5 system-ui,sans-serif;margin:0;padding:16px}"
        ".wrap{max-width:680px;margin:0 auto}"
        "h1{font-size:22px}h2{font-size:16px;color:#ffb200;margin-top:26px}"
        "table{width:100%;border-collapse:collapse}"
        "td{padding:7px 0;border-bottom:1px solid #24313d}"
        "input,button{font:inherit}"
        /* Bootstrap is themed to match the radio rather than the other
            * way round, so the two look the same either side of a CDN
            * failure. */
        ":root{--bs-body-bg:#0b0f13;--bs-body-color:#e6e6e6;"
        "--bs-border-color:#24313d;--bs-primary:#ffb200;"
        "--bs-link-color:#4ac2ee;--bs-link-hover-color:#7bd4f3}"
        ".card{background:#121a22;border-color:#24313d}"
        ".btn-primary{--bs-btn-bg:#ffb200;--bs-btn-border-color:#ffb200;"
        "--bs-btn-color:#0b0f13;--bs-btn-hover-bg:#ffc340;"
        "--bs-btn-hover-border-color:#ffc340;--bs-btn-hover-color:#0b0f13;"
        "--bs-btn-active-bg:#e6a000;--bs-btn-active-border-color:#e6a000}"
        ".form-control{background:#121a22;border-color:#24313d;"
        "color:#e6e6e6}"
        ".form-control:focus{background:#121a22;color:#e6e6e6;"
        "border-color:#ffb200;box-shadow:none}"
        ".table{--bs-table-color:#e6e6e6;--bs-table-bg:transparent;"
        "--bs-table-border-color:#24313d}"
        "h2{font-size:16px;color:#ffb200;letter-spacing:.02em}"
        /* The nav. Plain links so it still works when the CDN is gone,
           which is exactly the case on the radio's own access point. */
        "nav.pages{display:flex;flex-wrap:wrap;gap:4px;margin:14px 0 18px}"
        "nav.pages a{padding:6px 12px;border-radius:6px;text-decoration:none;"
        "color:#9fb3c8;border:1px solid #24313d;font-size:14px}"
        "nav.pages a:hover{color:#e6e6e6;border-color:#3a4b5c}"
        "nav.pages a.here{color:#0b0f13;background:#ffb200;"
        "border-color:#ffb200;font-weight:600}"
        /* The form controls, named short because they are repeated for every
           field on the Radio page. The long Bootstrap class lists came to
           2131 bytes of a 14 KB page. */
        ".lbl{display:block;margin:0 0 4px;font-size:.875em;color:#9fb3c8}"
        ".sel,.num{display:block;width:100%;padding:4px 8px;font-size:14px;"
        "line-height:1.5;border-radius:6px;background:#121a22;"
        "border:1px solid #24313d;color:#e6e6e6}"
        ".sel:focus,.num:focus{outline:0;border-color:#ffb200}"
        /* Inside an input group the box has to share the row with the
           buttons either side of it, so it cannot be a full width block. */
        ".input-group>.num{flex:1 1 auto;width:1%;min-width:0}"
        /* The slider, so it is the radio's amber rather than the browser's
           default blue. */
        "input[type=range]{accent-color:#ffb200}"
        /* The reply to a button press. Fixed to the bottom of the screen
           rather than sitting at the top of the page, because the buttons
           that produce it are most of a screen further down and the answer
           was landing where nobody was looking. */
        "#rmsg{position:fixed;left:16px;right:16px;bottom:16px;z-index:50;"
        "margin:0;padding:10px 40px 10px 14px;border-radius:8px;"
        "background:#121a22;border:1px solid #24313d;"
        "box-shadow:0 6px 24px rgba(0,0,0,.5);max-width:648px;"
        "margin-inline:auto}"
        "#rmsg[hidden]{display:none}"
        /* The close button. Nothing closes itself: a message that vanishes
           on a timer is one somebody was still reading. */
        "#rmsgx{position:absolute;top:4px;right:8px;border:0;background:none;"
        "color:#9fb3c8;font-size:22px;line-height:1;cursor:pointer;"
        "padding:2px 6px}"
        "#rmsgx:hover{color:#e6e6e6}"
        "</style></head><body><div class='wrap container-sm py-3'>");

  if (active != NULL) {
    out +=
        F("<h1 class='h4 mb-0' style='letter-spacing:.04em'>TEF668X</h1>"
          "<p class='text-secondary small mb-0' "
          "style='letter-spacing:.04em'>");
    out += F(BOARD_NAME_DISPLAY " &middot; V" FIRMWARE_VERSION);
    out += F("</p><nav class=pages>");
    out += navLink("/", "Home", active);
    out += navLink("/radio", "Radio", active);
    out += navLink("/network", "Network", active);
    out += navLink("/system", "System", active);
    out += F("</nav>");
  }
  return out;
}

static const char *pageTail(bool scripted = false) {
  if (!scripted) {
    /* Only the Radio page has controls that need it, and shipping it on the
     * other three cost every one of them 1.75 KB of heap per request. */
    return "</div></body></html>";
  }
  /*
   * Everything on the Radio page posts to the same API a script would use,
   * so the checks and the wording of every refusal live in one place.
   *
   * Three behaviours, and no framework:
   *
   *   data-post   a button that posts once, with data-args, or with the
   * value of the field named by data-from
   *   data-now    a control that posts the moment it changes. data-send=label
   * sends the option's text rather than its value, because
   * /api/band and /api/squelch take names
   *   data-apply  a button that gathers every data-api field in its own card
   * and posts one request per endpoint
   *
   * The reply is plain text and goes into the line at the top, as text, never
   * as markup. A hostile reply would be shown, not run.
   */
  return "<script>"
         /* q is querySelectorAll, g is getElementById. Both are used enough
            times that the names are worth the two lines. */
         "var q=function(s){return document.querySelectorAll(s)},"
         "g=function(i){return document.getElementById(i)},"
         "S=function(t,c){var m=g('rmsg'),x=g('rmsgt');if(!m||!x)return;"
         "m.className='small '+c;x.textContent=t.trim();m.hidden=false},"
         "M=function(t,o){S(t,o?'text-success':'text-danger')},"
         "W=function(){S('Working...','text-secondary')},"
         /* Update the frequency line from the state document. */
         "R=function(){fetch('/api/state').then(function(r){return r.json()})"
         ".then(function(d){var t=d.tun;if(!t)return;"
         "var e=function(i,v){var n=g(i);if(n)n.textContent=v};"
         "e('npf',t.f);e('npu',t.unt||'');e('npb',t.bnd);"
         "var s=[];if(t.sig!==undefined)s.push((t.sig/10).toFixed(1)+' dBuV');"
         "if(t.st)s.push('stereo');s.push('squelch '+(t.sqo?'open':'shut'));"
         "if(t.mut)s.push('muted');e('nps',s.join(' · '));"
         "var k=g('khz');if(k)k.value=t.khz}).catch(function(){})},"
         "P=function(u,b){W();return fetch(u,{method:'POST',body:b})"
         ".then(function(r){return r.text().then(function(t){"
         "M(t,r.ok);R();return r.ok})})"
         ".catch(function(){M('The radio did not answer.',false);"
         "return false})},"
         /* A seek answers as soon as it has started, so the frequency line
            keeps up with it on its own until it stops. */
         "F=function(){var n=0,t=setInterval(function(){"
         "fetch('/api/state').then(function(r){return r.json()}).then("
         "function(d){R();if(!d.tun||!d.tun.skg||++n>200)clearInterval(t)})"
         ".catch(function(){clearInterval(t)})},250)};"
         "q('[data-post]').forEach(function(b){"
         "b.addEventListener('click',function(e){e.preventDefault();"
         "var p=new URLSearchParams(b.dataset.args||'');"
         "if(b.dataset.from){var f=g(b.dataset.from);if(f)p.append(f.name,"
         "f.value)}"
         "P(b.dataset.post,p).then(function(o){"
         "if(o&&b.dataset.quiet!==undefined)F()})})});"
         "q('[data-now]').forEach(function(l){"
         "l.addEventListener('change',function(){"
         "var v=l.dataset.send==='label'&&l.options?"
         "l.options[l.selectedIndex].text:l.value;"
         "if(l.dataset.echo){var e=g(l.dataset.echo);if(e)e.textContent="
         "l.value}"
         "var p=new URLSearchParams();p.append(l.name,v);"
         /* A band change rebuilds which settings apply, so reload. */
         "P(l.dataset.now,p).then(function(){if(l.name==='bnd')"
         "location.reload()})});"
         "if(l.dataset.echo)l.addEventListener('input',function(){"
         "var e=g(l.dataset.echo);if(e)e.textContent=l.value})});"
         "q('[data-apply]').forEach(function(b){"
         "b.addEventListener('click',async function(e){e.preventDefault();"
         "var c=b.closest('[data-card]');if(!c)return;var m={};"
         "c.querySelectorAll('[data-api]').forEach(function(l){"
         "if(l.disabled)return;var u=l.dataset.api;"
         "if(!m[u])m[u]=new URLSearchParams();m[u].append(l.name,l.value)});"
         "for(var u in m){var o=await P(u,m[u]);if(!o)return}})});"
         "document.addEventListener('click',function(e){"
         "if(e.target&&e.target.id==='rmsgx')g('rmsg').hidden=true});"
         "</script></div></body></html>";
}

static String formSelect(const char *name, const char *label,
                         const char *const *options, const long *values,
                         int count, long current, const char *attrs) {
  String out;
  out += F("<div class='col-6 col-md-4'><label class=lbl>");
  out += label;
  out += F("</label><select class=sel name=");
  out += name;
  out += F(" ");
  out += attrs;
  out += F(">");
  for (int i = 0; i < count; i++) {
    out += F("<option value=");
    out += String(values[i]);
    if (values[i] == current) {
      out += F(" selected");
    }
    out += F(">");
    out += options[i];
    out += F("</option>");
  }
  out += F("</select></div>");
  return out;
}

static String formNumber(const char *name, const char *label, long low,
                         long high, long current, const char *attrs) {
  String out;
  out += F("<div class='col-6 col-md-4'><label class=lbl>");
  out += label;
  out +=
      F("</label><input class=num type=number "
        "name=");
  out += name;
  out += F(" min=");
  out += String(low);
  out += F(" max=");
  out += String(high);
  out += F(" value=");
  out += String(current);
  out += F(" ");
  out += attrs;
  out += F("></div>");
  return out;
}

static String cardOpen(const char *title) {
  String out =
      F("<div class='card mb-3' data-card><div class='card-body "
        "p-3'><h2 class='mt-0 mb-3'>");
  out += title;
  out += F("</h2>");
  return out;
}

static const char *cardClose(void) {
  return "</div></div>";
}

/*
 * The Radio page.
 *
 * Four cards. The first works the radio, which is what a page called Radio is
 * for. The second holds how it receives, behind one Apply. The third holds
 * the ones that need a reboot, kept apart so that "needs a reboot" is a
 * property of a card rather than small print somebody has to notice. The
 * fourth measures the knob.
 *
 * Every control posts to the same API a script would use, so there is one set
 * of checks and one set of refusals, and no second path with a weaker check
 * on it.
 */
static String radioForms(void) {
  RadioSnapshot now;
  bool live = radioGetSnapshot(&now);
  const Settings *st = sSettings;

  static const char *offOn[] = {"Off", "On"};
  static const long zeroOne[] = {0, 1};
  String out;
  out.reserve(5500);

  out +=
      F("<p id=rmsg class=small hidden><span id=rmsgt></span>"
        "<button id=rmsgx type=button aria-label=Close>&times;</button>"
        "</p>");

  if (!live) {
    out +=
        F("<div class='alert alert-danger py-2 px-3 small'>The radio task "
          "is not running, so there is nothing to set.</div>");
    return out;
  }

  bool onFm = bandModulation(now.settings.band) == MODULATION_FM;
  char freqText[16];
  if (!bandFormatFrequency(now.settings.band, now.settings.freqKHz, freqText,
                           sizeof(freqText))) {
    freqText[0] = '\0';
  }

  /* ------------------------------------------------------------ the dial */
  out += cardOpen("Listening to");
  out +=
      F("<div class='d-flex align-items-baseline gap-2 mb-1'>"
        "<span id=npf style='font-size:30px;color:#ffb200;"
        "font-variant-numeric:tabular-nums'>");
  out += freqText;
  out += F("</span><span id=npu class='text-secondary'>");
  out += bandFrequencyUnit(now.settings.band);
  out += F("</span><span id=npb class='text-secondary'>");
  out += bandName(now.settings.band);
  out += F("</span></div><p id=nps class='small text-secondary mb-3'>");
  if (now.qualityValid) {
    char level[12];
    signalFormatLevel(now.quality.levelDbuVTenths, level, sizeof(level));
    out += level;
    out += F(" dBuV");
    if (now.quality.stereo && !now.settings.forcedMono) {
      out += F(" &middot; stereo");
    }
  }
  out += now.squelchOpen ? F(" &middot; squelch open")
                         : F(" &middot; squelch shut");
  if (now.settings.muted) {
    out += F(" &middot; muted");
  }
  out += F("</p>");

  /* Step buttons either side of a box to type in: the knob and the keypad,
   * which are the two ways the radio itself is tuned. */
  out +=
      F("<div class='row g-2 align-items-end'>"
        "<div class='col-12 col-md-7'>"
        "<label class=lbl>Frequency, "
        "kHz</label><div class='input-group input-group-sm'>"
        "<button class='btn btn-outline-secondary' data-post='/api/step' "
        "data-args='stp=-1' title='Down one step'>&#8249;</button>"
        "<input class=num id=khz name=khz type=number value=");
  out += String(now.settings.freqKHz);
  out +=
      F("><button class='btn btn-outline-secondary' data-post='/api/step' "
        "data-args='stp=1' title='Up one step'>&#8250;</button>"
        "<button class='btn btn-primary' data-post='/api/tune' "
        "data-from=khz>Go</button></div></div>");

  out +=
      F("<div class='col-6 col-md-5'><label class=lbl>Band</label>"
        "<select class=sel name=bnd "
        "data-now='/api/band' data-send=label>");
  for (int b = 0; b < BAND_COUNT; b++) {
    out += F("<option");
    if (b == (int)now.settings.band) {
      out += F(" selected");
    }
    out += F(">");
    out += bandName((BandId)b);
    out += F("</option>");
  }
  out += F("</select></div>");

  out +=
      F("<div class='col-12 col-md-8'><label class=lbl>Volume, dB <span "
        "id=volnow>");
  out += String(now.settings.volumeDb);
  out += F("</span></label><input class=form-range type=range name=db min=");
  out += String(RADIO_VOLUME_MIN);
  out += F(" max=0 value=");
  out += String(now.settings.volumeDb);
  out += F(" data-now='/api/volume' data-echo=volnow></div>");

  out +=
      F("<div class='col-6 col-md-4'><label class=lbl>Squelch</label>"
        "<select class=sel name=mod "
        "data-now='/api/squelch' data-send=label>");
  for (int m = 0; m < SQUELCH_MODE_COUNT; m++) {
    out += F("<option");
    if (m == (int)now.squelchMode) {
      out += F(" selected");
    }
    out += F(">");
    out += squelchModeName((SquelchMode)m);
    out += F("</option>");
  }
  out += F("</select></div>");

  /* Seek sits with the dial, because it is a way of moving the dial. It
   * answers as soon as it has started, so the button comes back at once and
   * the frequency line above follows it until it stops. */
  out +=
      F("<div class='col-12 mt-1'>"
        "<button class='btn btn-outline-secondary btn-sm me-2' "
        "data-post='/api/seek' data-args='dir=down' data-quiet>"
        "&#8249;&#8249; Seek</button>"
        "<button class='btn btn-outline-secondary btn-sm' "
        "data-post='/api/seek' data-args='dir=up' data-quiet>"
        "Seek &#8250;&#8250;</button></div>");
  out +=
      F("<div class='col-12 mt-2'>"
        "<button class='btn btn-outline-secondary btn-sm me-2' "
        "data-post='/api/cycle' data-args='wht=mute'>Mute or unmute"
        "</button>"
        "<button class='btn btn-primary btn-sm' data-post='/api/save'>"
        "Keep these settings</button></div></div>"
        "<p class='small text-secondary mt-2 mb-0'>Keep these settings "
        "stores the station and everything below, so the radio comes up "
        "this way. The volume follows the knob, except in manual squelch "
        "where the knob is the squelch.</p>");
  out += cardClose();

  /* ----------------------------------------------------------- reception */
  out += cardOpen("Reception");
  out += F("<div class='row g-2'>");
  static const char *deemphNames[] = {"50 us", "75 us", "Off"};
  static const long deemphValues[] = {50, 75, 0};
  out += formSelect("dem", "De-emphasis", deemphNames, deemphValues, 3,
                    now.settings.deemphasisUs, "data-api='/api/fm'");
  if (!onFm) {
    static const char *widthNames[] = {"3 kHz", "4 kHz", "6 kHz", "8 kHz"};
    static const long widthValues[] = {3, 4, 6, 8};
    out += formSelect("khz", "Bandwidth", widthNames, widthValues, 4,
                      now.settings.bandwidthKHz, "data-api='/api/bandwidth'");
  } else {
    out += formSelect("ims", "Multipath suppression", offOn, zeroOne, 2,
                      now.settings.multipathSuppression ? 1 : 0,
                      "data-api='/api/fm'");
    out += formSelect("eq", "Channel equalizer", offOn, zeroOne, 2,
                      now.settings.equalizer ? 1 : 0, "data-api='/api/fm'");
    static const char *stereoMono[] = {"Stereo", "Mono"};
    out += formSelect("mno", "Stereo", stereoMono, zeroOne, 2,
                      now.settings.forcedMono ? 1 : 0, "data-api='/api/fm'");
  }
  out += F("</div>");

  /* The expert half, folded away. Nobody sets a stereo blend start level by
   * accident, and leaving it open made the page look harder than it is. */
  out +=
      F("<details class='mt-3'><summary class='small text-secondary' "
        "style='cursor:pointer'>Weak signal and noise blankers</summary>"
        "<div class='row g-2 mt-1'>");
  if (onFm) {
    out += formNumber("cut", "High cut from, dBuV", 0, 60,
                      now.settings.highCutStart, "data-api='/api/fm'");
    out += formNumber("bld", "Stereo blend from, dBuV", 0, 60,
                      now.settings.stereoBlendStart, "data-api='/api/fm'");
    out += formNumber("hbl", "Both from, dBuV", 0, 60,
                      now.settings.stHiBlendStart, "data-api='/api/fm'");
  }
  out += formNumber("fnb", "FM noise blanker, per cent", 0, 150,
                    now.settings.fmNoiseBlankerStart, "data-api='/api/fm'");
  out += formNumber("anb", "AM noise blanker, per cent", 0, 150,
                    now.settings.amNoiseBlankerStart, "data-api='/api/fm'");
  out +=
      F("</div><p class='small text-secondary mt-2 mb-0'>The levels are 0 "
        "to switch off, or 20 to 60. The blankers are 0, or 50 to 150."
        "</p></details>");

  /* How fussy seek is. Here rather than beside the seek buttons, because it
   * is a setting and they are an action. */
  out +=
      F("<details class='mt-2'><summary class='small text-secondary' "
        "style='cursor:pointer'>Seek sensitivity</summary>"
        "<div class='row g-2 mt-1'>");
  {
    static const char *sensNames[] = {
        "1, strong only", "2", "3",
        "4, the default", "5", "6, finds weak ones"};
    static const long sensValues[] = {1, 2, 3, 4, 5, 6};
    out += formSelect("fsn", "FM", sensNames, sensValues, 6,
                      st->fmScanSensitivity, "data-api='/api/settings'");
    out += formSelect("asn", "AM", sensNames, sensValues, 6,
                      st->amScanSensitivity, "data-api='/api/settings'");
    out += formNumber("sqf", "Squelch floor, dBuV", 0,
                      SQUELCH_FM_LEVEL_FLOOR_MAX_DBUV, st->fmSquelchFloor,
                      "data-api='/api/settings'");
    out += formSelect("rds", "RDS decoder", offOn, zeroOne, 2, st->rdsEnabled,
                      "data-api='/api/settings'");
  }
  out +=
      F("</div><p class='small text-secondary mt-2 mb-0'>Higher settles "
        "for a weaker signal and stops more often on things that are not "
        "stations. This one takes effect at once, with no reboot.</p>"
        "<p class='small text-secondary mt-2 mb-0'>The squelch floor is what "
        "keeps the automatic squelch shut on the channel beside a strong "
        "station, which looks like a station to everything except the "
        "level. 0 switches it off. Raise it if the radio opens on nothing "
        "next to a strong station, lower it if it mutes a weak station you "
        "can hear.</p></details>");

  /* The polish. Folded away because none of it changes what the radio
   * receives, and because a beep is the sort of thing somebody switches on
   * once and never looks at again. */
  out +=
      F("<details class='mt-2'><summary class='small text-secondary' "
        "style='cursor:pointer'>Sounds and fades</summary>"
        "<div class='row g-2 mt-1'>");
  {
    static const char *rampNames[] = {"Off, cut at once", "60 ms", "120 ms",
                                      "250 ms", "500 ms"};
    static const long rampValues[] = {0, 60, 120, 250, 500};
    out += formSelect("smu", "Mute and squelch ramp", rampNames, rampValues, 5,
                      st->softMuteMs, "data-api='/api/settings'");
    static const char *beepNames[] = {"Off", "Keypad only",
                                      "Keypad and long presses", "Every press"};
    static const long beepValues[] = {0, 1, 2, 3};
    out += formSelect("bpk", "Beep on", beepNames, beepValues, 4, st->beepKey,
                      "data-api='/api/settings'");
    out += formSelect("bpe", "Band edge beep", offOn, zeroOne, 2, st->beepEdge,
                      "data-api='/api/settings'");
    out += formSelect("bps", "Chime at start up", offOn, zeroOne, 2,
                      st->beepStart, "data-api='/api/settings'");
  }
  out +=
      F("</div><p class='small text-secondary mt-2 mb-0'>The ramp takes "
        "the click off a mute, the squelch closing and a bandwidth "
        "change. It also runs before a reboot or a firmware update. The "
        "beeps are off unless you want them.</p></details>");

  /* The panel light. Beside the sounds because it is the same kind of thing:
   * none of it changes what the radio receives. */
  out +=
      F("<details class='mt-2'><summary class='small text-secondary' "
        "style='cursor:pointer'>The panel light</summary>"
        "<div class='row g-2 mt-1'>");
  out += formNumber("blt", "Brightness, per cent", BACKLIGHT_MIN_AWAKE, 100,
                    st->backlightPercent, "data-api='/api/settings'");
  out += formNumber("bdm", "Dimmed, per cent", 0, 100, st->backlightDimPercent,
                    "data-api='/api/settings'");
  out += formNumber("bds", "Dim after, seconds", 0, BACKLIGHT_DIM_AFTER_MAX_S,
                    st->backlightDimAfterS, "data-api='/api/settings'");
  out += formSelect("blf", "Fade up at start", offOn, zeroOne, 2,
                    st->backlightFade, "data-api='/api/settings'");
  out +=
      F("</div><p class='small text-secondary mt-2 mb-0'>Brightness "
        "changes as soon as you apply it, so you can look at the panel "
        "and pick one. Dim after 0 never dims. The knob, a button and a "
        "key all bring it straight back.</p></details>");

  if (!onFm) {
    out +=
        F("<p class='small text-secondary mt-3 mb-0'>The FM settings "
          "appear when the radio is on FM. The tuner has nowhere to put "
          "them on an AM band.</p>");
  }
  out +=
      F("<div class='mt-3'><button class='btn btn-primary btn-sm' "
        "data-apply>Apply</button></div>");
  out += cardClose();

  /* ----------------------------------------------------------- band plan */
  out += cardOpen("Band plan and knob");
  out +=
      F("<p class='small text-secondary mb-3'>These are read when the "
        "radio starts, so they need a reboot. They decide which "
        "frequencies exist, which is not a thing to change under a radio "
        "that is tuned to one of them.</p><div class='row g-2'>");
  static const char *regionNames[] = {"65 to 108", "Japan, 76 to 95",
                                      "76 to 108", "87 to 108", "87.5 to 108"};
  static const long regionValues[] = {0, 1, 2, 3, 4};
  out += formSelect("rgn", "FM band, MHz", regionNames, regionValues, 5,
                    st->fmRegion, "data-api='/api/settings'");
  static const char *spacingNames[] = {"9 kHz", "10 kHz"};
  out += formSelect("spc", "Medium wave steps", spacingNames, zeroOne, 2,
                    st->mwSpacing, "data-api='/api/settings'");
  static const char *encoderNames[] = {"Standard", "Optical"};
  out += formSelect("enc", "Encoder", encoderNames, zeroOne, 2, st->encoderKind,
                    "data-api='/api/settings'");
  static const char *directionNames[] = {"Normal", "Reversed"};
  out += formSelect("edr", "Knob direction", directionNames, zeroOne, 2,
                    st->encoderDirection, "data-api='/api/settings'");
  out +=
      F("</div><div class='mt-3'><button class='btn btn-primary btn-sm' "
        "data-apply>Save band plan</button></div>");
  out += cardClose();

  /* ------------------------------------------------------------- the knob */
  out += cardOpen("The volume knob");
  out += F("<p class='small text-secondary mb-2'>");
  if (st->potRawMax != 0) {
    out += F("This knob has been measured: it runs ");
    out += String(st->potRawMin);
    out += F(" to ");
    out += String(st->potRawMax);
    out += F(".");
  } else {
    out +=
        F("Not measured. The radio is using figures taken from another "
          "unit, which may not be this knob's travel.");
  }
  out +=
      F(" Press Measure, turn the knob all the way to both ends, then "
        "press Done. While measuring the knob changes nothing, so you can "
        "sweep it to the loud end without the volume following.</p>"
        "<button class='btn btn-outline-secondary btn-sm me-2' "
        "data-post='/api/pot' data-args='act=start'>Measure</button>"
        "<button class='btn btn-primary btn-sm me-2' "
        "data-post='/api/pot' data-args='act=finish'>Done</button>"
        "<button class='btn btn-outline-secondary btn-sm' "
        "data-post='/api/pot' data-args='act=cancel'>Cancel</button>");
  out += cardClose();
  return out;
}
static String signInForm(const char *next) {
  String out = cardOpen("Access PIN");
  out +=
      F("<p class='text-secondary small'>Six digits. A new radio is on "
        "000000. Five wrong tries locks this for a minute.</p>"
        "<form method=post action='/auth'>"
        "<input type=hidden name=nxt value='");
  out += next;
  out +=
      F("'><label class=lbl>PIN</label>"
        "<input class=num name=pin inputmode=numeric "
        "pattern='[0-9]{6}' maxlength=6 required>"
        "<button class='btn btn-primary mt-3' type=submit>Unlock</button>"
        "</form>");
  out += cardClose();
  return out;
}

/*
 * Where a sign in may send the browser afterwards.
 *
 * Only the four pages this firmware serves. Anything else, including an
 * absolute URL somebody put in the form, comes back as the home page.
 */
static const char *safeNext(const String &want) {
  static const char *kPages[] = {"/", "/radio", "/network", "/system"};
  for (size_t i = 0; i < sizeof(kPages) / sizeof(kPages[0]); i++) {
    if (want == kPages[i]) {
      return kPages[i];
    }
  }
  return "/";
}

static String defaultPinBanner(void) {
  if (!accessPinIsDefault(sAccessPin)) {
    return String();
  }
  return F(
      "<div class='alert alert-danger py-2 px-3 small' role=alert "
      "style='background:#3a1410;border:1px solid #7a2a1e;"
      "color:#ffb4a2'><strong>This radio is on the default access PIN, "
      "000000.</strong> Anyone who can reach it on the network can change "
      "its settings and replace its firmware. Set your own PIN on the "
      "<a href='/network' style='color:#ffb4a2'>Network</a> page.</div>");
}

static String wifiForm(void) {
  String out = cardOpen("Wi-Fi");
  if (inSetupMode()) {
    out +=
        F("<p class='text-secondary small'>The radio could not join a "
          "network, so it is serving this page on its own. Enter the "
          "details and it will try again.</p>");
  }
  out +=
      F("<form method=post action='/wifi'>"
        "<label class=lbl>Network name"
        "</label><input class=num name=sid maxlength=32 required "
        "value='");
  /* The stored name only to somebody who has signed in, or on the access
   * point, where there is no network yet and the form is open by design.
   * Filling it in for anyone who can reach the radio hands out the name of
   * the home network to the whole of it. */
  if (signedIn() || inSetupMode()) {
    out += escapeHtml(sSettings->wifiSsid);
  }
  out +=
      F("'><label class='lbl mt-2'>Passphrase, "
        "leave empty for an open network</label>"
        "<input class=num name=pwd type=password maxlength=64>"
        "<button class='btn btn-primary mt-3' type=submit>Save and join"
        "</button></form>");
  return out;
}

/*
 * Home. What the radio is and where it is, and nothing that changes it.
 *
 * The four pages exist because one page held the lot, and that page was the
 * largest String the web server ever built. Splitting them cuts the peak
 * heap for a request as well as the scrolling.
 */
static void handleRoot(void) {
  sRequests++;
  String out = pageHead("TEF668X", "/");
  out += defaultPinBanner();

  /* What it is receiving, read once. A page that refreshes itself belongs on
   * a websocket, which is phase 5. */
  RadioSnapshot now;
  if (radioGetSnapshot(&now)) {
    out += cardOpen("Radio");
    char freq[16];
    if (bandFormatFrequency(now.settings.band, now.settings.freqKHz, freq,
                            sizeof(freq))) {
      out +=
          F("<div class='d-flex align-items-baseline gap-2 mb-1'>"
            "<span style='font-size:30px;color:#ffb200;"
            "font-variant-numeric:tabular-nums'>");
      out += freq;
      out += F("</span><span class='text-secondary'>");
      out += bandFrequencyUnit(now.settings.band);
      out += F("</span><span class='text-secondary'>");
      out += bandName(now.settings.band);
      out += F("</span></div>");
    }
    out += F("<p class='small text-secondary mb-2'>");
    if (now.qualityValid) {
      char level[12];
      signalFormatLevel(now.quality.levelDbuVTenths, level, sizeof(level));
      out += level;
      out += F(" dBuV");
      if (now.quality.stereo && !now.settings.forcedMono) {
        out += F(" &middot; stereo");
      }
    }
    out += F(" &middot; squelch ");
    out += squelchModeName(now.squelchMode);
    out += now.squelchOpen ? F(", open") : F(", shut");
    if (now.settings.muted) {
      out += F(" &middot; muted");
    }
    out += F("</p>");
    if (signedIn()) {
      out +=
          F("<a class='btn btn-outline-secondary btn-sm' href='/radio'>"
            "Tune and settings</a>");
    }
    out += cardClose();
  }

  out += cardOpen("Network");
  out += F("<table class='table table-sm align-middle mb-0'>");
  out += "<tr><td class='text-secondary'>Joined</td><td>" +
         escapeHtml(wifiNetworkName()) + "</td></tr>";
  out += "<tr><td class='text-secondary'>Address</td><td>" +
         String(wifiAddress()) + "</td></tr>";
  out += F("<tr><td class='text-secondary'>Mode</td><td>");
  out += inSetupMode() ? F("access point, setup") : F("joined a network");
  out += F("</td></tr></table>");
  out += cardClose();

  if (inSetupMode()) {
    /* On the access point there is no network yet, so the form that fixes
     * that goes here as well rather than one page away. */
    out += wifiForm();
  }
  if (!signedIn()) {
    out += signInForm("/");
  }
  out += pageTail();
  sServer.send(200, "text/html", out);
}

static void handleRadioPage(void) {
  sRequests++;
  String out = pageHead("Radio", "/radio");
  if (!signedIn()) {
    out += signInForm("/radio");
    out += pageTail();
    sServer.send(200, "text/html", out);
    return;
  }
  out += radioForms();
  out += pageTail(true);
  sServer.send(200, "text/html", out);
}

static void handleNetworkPage(void) {
  sRequests++;
  String out = pageHead("Network", "/network");
  out += defaultPinBanner();
  out += wifiForm(); /* Open on the access point, which is the point of it. */
  if (!signedIn()) {
    out += signInForm("/network");
  } else {
    out += cardOpen("Access PIN");
    out +=
        F("<p class='text-secondary small'>Six digits. Changing it takes "
          "effect at once and signs every browser out, this one included."
          "</p>"
          "<form method=post action='/setpin'>"
          "<label class=lbl>New PIN</label>"
          "<input class=num name=pin inputmode=numeric "
          "pattern='[0-9]{6}' maxlength=6 required>"
          "<button class='btn btn-primary mt-3' type=submit>Change PIN"
          "</button></form>");
    out += cardClose();
  }
  out += pageTail();
  sServer.send(200, "text/html", out);
}

static void handleSystemPage(void) {
  sRequests++;
  String out = pageHead("System", "/system");
  if (!signedIn()) {
    out += signInForm("/system");
    out += pageTail();
    sServer.send(200, "text/html", out);
    return;
  }

  out += cardOpen("This image");
  out += F("<table class='table table-sm align-middle mb-0'>");
  out += "<tr><td class='text-secondary'>Firmware</td><td>V" +
         String(FIRMWARE_VERSION) + "</td></tr>";
  out += "<tr><td class='text-secondary'>Running from</td><td>" +
         String(rollbackRunningPartition()) + "</td></tr>";
  out += "<tr><td class='text-secondary'>Image</td><td class=";
  out += rollbackPending() ? F("warn>") : F("ok>");
  out += String(rollbackStateText()) + "</td></tr>";
  const Tef668xCapabilities *tuner = tef668xCapabilities();
  if (tuner != NULL) {
    out += "<tr><td class='text-secondary'>Tuner</td><td>" +
           String(tuner->part) + ", patch v" +
           String((unsigned)tuner->patchVersion) + "</td></tr>";
  } else {
    out += "<tr><td class='text-secondary'>Tuner</td><td class=warn>" +
           String(tef668xErrorText(tunerStartError())) + "</td></tr>";
  }
  out += F("</table>");
  out += cardClose();

  out += cardOpen("How it is doing");
  out += F("<table class='table table-sm align-middle mb-0'>");
  out += "<tr><td class='text-secondary'>Free heap</td><td>" +
         String(ESP.getFreeHeap()) + " bytes</td></tr>";
  out += "<tr><td class='text-secondary'>Up for</td><td>" +
         String(millis() / 1000UL) + " seconds</td></tr>";
  out += "<tr><td class='text-secondary'>Requests served</td><td>" +
         String(sRequests) + "</td></tr>";
  out += F("</table>");
  out += cardClose();

  out += cardOpen("Firmware");
  out +=
      F("<p class='text-secondary small'>Pick the .bin from "
        ".pio/build/ats125/firmware.bin. The radio reboots into it and "
        "puts the old one back on its own if it does not come up.</p>"
        "<form method=post action='/update' enctype='multipart/form-data'>"
        "<input class=num type=file name=firmware accept='.bin' "
        "required>"
        "<button class='btn btn-primary mt-3' type=submit>Upload and "
        "reboot</button></form>");
  out += cardClose();

  out += cardOpen("Reboot");
  out +=
      F("<p class='text-secondary small'>The band plan and the knob "
        "settings are read at start up, so this is how they take "
        "effect.</p>"
        "<form method=post action='/reboot'>"
        "<button class='btn btn-outline-secondary' type=submit>"
        "Reboot now</button></form>");
  out += cardClose();

  out += pageTail();
  sServer.send(200, "text/html", out);
}

static void sendResult(int code, const char *title, const char *message,
                       bool bad) {
  String out = pageHead(title);
  out += F("<h1 class=h5>");
  out += title;
  out += F("</h1><p style='color:");
  out += bad ? F("#ff8a72'>") : F("#31c29c'>");
  out += message;
  out += F("</p><p><a href='/' style='color:#4ac2ee'>Back</a></p>");
  out += pageTail();
  sServer.send(code, "text/html", out);
}

static void handleAuth(void) {
  sRequests++;
  uint32_t now = millis();

  if (accessPinGateLocked(&sGate, now)) {
    uint32_t waitMs = accessPinGateRetryAfterMs(&sGate, now);
    sServer.sendHeader("Retry-After", String((waitMs + 999UL) / 1000UL));
    sendResult(429, "Too many tries",
               "Too many wrong PINs. Try again in a minute.", true);
    return;
  }

  uint32_t given = 0;
  if (!sServer.hasArg("pin") ||
      !accessPinParse(sServer.arg("pin").c_str(), &given)) {
    /* A malformed PIN still counts, or the six digit check is free to skip. */
    accessPinGateCheck(&sGate, sAccessPin, sAccessPin + 1, now);
    sendResult(400, "Wrong PIN", "A PIN is six digits.", true);
    return;
  }

  if (!accessPinGateCheck(&sGate, sAccessPin, given, now)) {
    Serial.printf("[web] wrong PIN from %s\n",
                  sServer.client().remoteIP().toString().c_str());
    sendResult(403, "Wrong PIN", "That PIN is not right.", true);
    return;
  }

  newSession();
  String cookie = "tefsid=";
  cookie += sSessionToken;
  cookie += "; Path=/; Max-Age=" + String(WEB_SESSION_TTL_SECONDS) +
            "; HttpOnly; SameSite=Strict";
  sServer.sendHeader("Set-Cookie", cookie);
  /* Back to the page the form was on, checked against the pages this
   * firmware serves. An unchecked target out of the request would send the
   * browser wherever the poster liked. */
  sServer.sendHeader(
      "Location",
      safeNext(sServer.hasArg("nxt") ? sServer.arg("nxt") : String("/")));
  sServer.send(303, "text/plain", "");
}

static void handleWifi(void) {
  sRequests++;
  if (!requireAuth(true)) {
    return;
  }
  if (!sServer.hasArg("sid")) {
    sendResult(400, "Nothing saved", "A network name is needed.", true);
    return;
  }

  String ssid = sServer.arg("sid");
  String pass = sServer.hasArg("pwd") ? sServer.arg("pwd") : String("");

  Settings pending = *sSettings;
  if (!settingsSetWifi(&pending, ssid.c_str(), pass.c_str())) {
    sendResult(400, "Nothing saved",
               "That network name or passphrase is too long.", true);
    return;
  }
  if (!settingsNvsSave(&pending)) {
    sendResult(500, "Nothing saved", "The settings could not be written.",
               true);
    return;
  }
  *sSettings = pending;
  settingsTaskSaved();

  Serial.printf("[web] new credentials saved for %s\n", pending.wifiSsid);
  sendResult(200, "Saved",
             "Saved. The radio is trying that network now. If it works it "
             "will be on tef668x.local, and this access point will stop.",
             false);

  /* Answer first, then move the radio, or the reply never reaches the browser
   * that is connected to the access point being torn down. NetworkClient has
   * no send buffer to flush in this core, so closing the socket is what puts
   * the bytes on the wire. */
  sServer.client().stop();
  delay(200);
  wifiRetryNow(sSettings);
}

static void handleUploadData(void) {
  /* WebServer calls this same callback for two different things. For a
   * multipart upload it fills _currentUpload first. For any other POST body it
   * takes the raw path instead, leaves _currentUpload null, and calls this
   * anyway, so sServer.upload() dereferences a null pointer and the radio
   * panics. There is no accessor that says which one happened, so the content
   * type is what tells them apart. */
  if (!sServer.header("Content-Type").startsWith("multipart/")) {
    return;
  }

  HTTPUpload &upload = sServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    sUploadRejected = !signedIn();
    sUploadStarted = true;
    sUploadFailed = false;
    if (sUploadRejected) {
      Serial.printf("[web] upload refused, no PIN, from %s\n",
                    sServer.client().remoteIP().toString().c_str());
      return;
    }
    Serial.printf("[web] upload started: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      Update.printError(Serial);
      sUploadFailed = true;
    }
    return;
  }

  if (sUploadRejected) {
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (sUploadFailed) {
      /* Already dead. Take the rest of the body quietly rather than printing
       * an error line for every chunk of a large file. */
      return;
    }
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
      sUploadFailed = true;
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("[web] upload finished, %u bytes written\n",
                    (unsigned)upload.totalSize);
    } else {
      Update.printError(Serial);
    }
    return;
  }

  if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    Serial.println("[web] upload aborted");
  }
}

static void handleUploadDone(void) {
  sRequests++;
  bool rejected = sUploadRejected;
  bool sawImage = sUploadStarted;
  sUploadRejected = false;
  sUploadStarted = false;

  /* The upload handler only runs for a multipart part that carries a
   * filename. A POST with no file part never reaches it, so the PIN has to be
   * checked here as well or an unauthenticated request reboots the radio. */
  if (rejected || !signedIn()) {
    sServer.send(403, "text/plain", "Enter the access PIN first.\n");
    return;
  }

  if (!sawImage) {
    sendResult(400, "Nothing uploaded", "No firmware file was sent.", true);
    return;
  }

  if (Update.hasError()) {
    sendResult(500, "Update failed",
               "The image was not accepted. The radio is still running the "
               "old one.",
               true);
    return;
  }

  sendResult(200, "Update written",
             "The radio is rebooting into the new image. If it does not come "
             "up and pass its self check, the old image comes back on its "
             "own.",
             false);
  sRebootAfterReply = true;
}

static void handleSetPin(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }

  uint32_t wanted = 0;
  if (!sServer.hasArg("pin") ||
      !accessPinParse(sServer.arg("pin").c_str(), &wanted)) {
    sendResult(400, "PIN not changed", "A PIN is six digits.", true);
    return;
  }

  Settings pending = *sSettings;
  pending.accessPin = wanted;
  if (!settingsNvsSave(&pending)) {
    sendResult(500, "PIN not changed", "The settings could not be written.",
               true);
    return;
  }
  *sSettings = pending;
  settingsTaskSaved();
  sAccessPin = wanted;

  /* The old session was opened with the old PIN, so it goes. */
  dropSession();
  accessPinGateReset(&sGate);
  Serial.println("[web] the access PIN was changed");

  sendResult(200, "PIN changed",
             accessPinIsDefault(wanted)
                 ? "That is the default PIN, so the radio is still open to "
                   "anyone on the network. Sign in again."
                 : "Sign in again with the new PIN.",
             accessPinIsDefault(wanted));
}

static void handleReboot(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  sendResult(200, "Rebooting", "The radio is restarting.", false);
  sRebootAfterReply = true;
}

/*
 * The RDS decoder, as one JSON field. FM only.
 *
 * A field that is not here is one the radio cannot answer yet. That is the
 * whole shape of this block: an empty station name and a station name that
 * has not been received are different things, and sending `"ps":""` for the
 * second makes them look the same. So `ps`, `rt`, `pi`, `pty` and `ct` appear
 * only once they have been heard.
 *
 * | Key | Full name |
 * |---|---|
 * | `off` | The decoder is switched off. Nothing else in the block is here |
 * | `syn` | The tuner is locked to an RDS bit stream |
 * | `pi` | Programme identifier, four hex digits |
 * | `pty` | Programme type, 0 to 31 |
 * | `ptn` | What that number is called |
 * | `tp` | The station carries traffic announcements at some point |
 * | `ta` | One is on air right now |
 * | `ms` | "speech" or "music" |
 * | `ps` | Station name, eight characters |
 * | `rt` | Radio text |
 * | `af` | Alternative frequencies, in kHz |
 * | `ct` | Date and time the station sent, in its own local time |
 * | `grp` | Groups the tuner has handed over on this station |
 * | `use` | How many of those something was decoded from |
 * | `cor` | Blocks the tuner said it had corrected. Not used, only counted |
 * | `bad` | Blocks the tuner could not correct |
 */
static void appendRdsState(String &out, const RadioSnapshot &snap) {
  const RdsInfo *r = &snap.rds;
  if (!radioRdsEnabled()) {
    /* Said outright rather than left to look like a station with no RDS. The
     * two are the same picture otherwise, and somebody who switched it off
     * and forgot would have no way to tell which they were looking at. */
    out += F(",\"rds\":{\"off\":true}");
    return;
  }
  out += F(",\"rds\":{\"syn\":");
  out += r->synchronised ? F("true") : F("false");

  char small[96];
  if (r->hasPi) {
    snprintf(small, sizeof(small), ",\"pi\":\"%04X\"", r->pi);
    out += small;
  }
  if (r->hasPty) {
    snprintf(small, sizeof(small), ",\"pty\":%u", (unsigned)r->pty);
    out += small;
    out += F(",\"ptn\":\"");
    out += jsonEscape(rdsPtyName(r->pty));
    out += F("\"");
  }
  if (r->hasFlags) {
    out += F(",\"tp\":");
    out += r->tp ? F("true") : F("false");
    out += F(",\"ta\":");
    out += r->ta ? F("true") : F("false");
    out += F(",\"ms\":\"");
    out += r->speech ? F("speech") : F("music");
    out += F("\"");
  }
  if (r->hasPs) {
    out += F(",\"ps\":\"");
    out += jsonEscape(r->ps);
    out += F("\"");
  }
  if (r->hasRt) {
    out += F(",\"rt\":\"");
    out += jsonEscape(r->rt);
    out += F("\"");
  }
  if (r->afCount > 0) {
    out += F(",\"af\":[");
    for (uint8_t i = 0; i < r->afCount; i++) {
      if (i > 0) {
        out += F(",");
      }
      out += String((unsigned)r->afKHz[i]);
    }
    out += F("]");
  }
  if (r->clock.valid) {
    int minutes = r->clock.offsetHalfHours * 30;
    char sign = minutes < 0 ? '-' : '+';
    int magnitude = minutes < 0 ? -minutes : minutes;
    snprintf(small, sizeof(small),
             ",\"ct\":\"%04u-%02u-%02uT%02u:%02u%c%02d:%02d\"",
             (unsigned)r->clock.year, (unsigned)r->clock.month,
             (unsigned)r->clock.day, (unsigned)r->clock.hour,
             (unsigned)r->clock.minute, sign, magnitude / 60, magnitude % 60);
    out += small;
  }
  snprintf(small, sizeof(small),
           ",\"grp\":%u,\"use\":%u,\"cor\":%u,\"bad\":%u}",
           (unsigned)r->groupsSeen, (unsigned)r->groupsUsed,
           (unsigned)r->blocksCorrected, (unsigned)r->blocksBad);
  out += small;
}

/*
 * Append everything about the radio and its tuner, as JSON fields.
 *
 * One builder, used by both /api/state and /status.json, so the two
 * cannot drift apart and telemetry in a later phase has one shape to
 * match. The caller supplies the braces and any leading comma.
 *
 * The field names are documented on handleStatusJson.
 */
static void appendRadioState(String &out) {
  const Tef668xCapabilities *tuner = tef668xCapabilities();
  out += F("\"tun\":");
  if (tuner != NULL) {
    out += F("{\"prt\":\"");
    out += tuner->part;
    out += F("\",\"pch\":");
    out += String(tuner->patchVersion);
    /* The crystal is reported whether start up worked or not. A wrong choice
     * here does not fail, it just makes the radio deaf, so it has to be
     * visible on a working radio too. */
    const Tef668xDiagnostics *dg = tef668xDiagnostics();
    char xt[72];
    snprintf(xt, sizeof(xt), ",\"xad\":%u,\"xtl\":\"%s\"",
             (unsigned)dg->xtalAdc, dg->xtal ? dg->xtal : "not read");
    out += xt;
    out += F(",\"fsi\":");
    out += tuner->hasStereoImprovement ? F("true") : F("false");
    out += F(",\"frd\":");
    out += tuner->hasFullSearchRds ? F("true") : F("false");
    out += F(",\"dr\":");
    out += tuner->hasDigitalRadio ? F("true") : F("false");
    /* Which band we are on decides which module the quality comes from and
     * how the frequency reads, so it goes out too. */
    /* Everything live comes from one snapshot, so the frequency and the
     * readings under it always describe the same moment. Reading the tuner
     * from here would also mean two tasks on one I2C bus. */
    RadioSnapshot snap;
    bool haveSnap = radioGetSnapshot(&snap);
    if (haveSnap) {
      char freqText[16];
      if (bandFormatFrequency(snap.settings.band, snap.settings.freqKHz,
                              freqText, sizeof(freqText))) {
        char tuned[160];
        snprintf(tuned, sizeof(tuned),
                 ",\"bnd\":\"%s\",\"khz\":%u,\"f\":\"%s\",\"unt\":\"%s\""
                 ",\"stp\":%u,\"vol\":%d,\"mut\":%s,\"tmd\":\"%s\""
                 ",\"mem\":%d,\"seq\":%u",
                 bandName(snap.settings.band), (unsigned)snap.settings.freqKHz,
                 freqText, bandFrequencyUnit(snap.settings.band),
                 (unsigned)snap.settings.stepKHz, snap.settings.volumeDb,
                 snap.settings.muted ? "true" : "false",
                 tuneModeName(snap.settings.tuneMode),
                 snap.memorySlot == MEMORY_NO_SLOT ? 0 : snap.memorySlot + 1,
                 (unsigned)snap.sequence);
        out += tuned;
      }
      /* What the tuner last refused. Without this the page can show a station
       * the radio is not actually on, with nothing to say so. */
      /* The FM features, none of which turns itself on. */
      out += F(",\"ims\":");
      out += snap.settings.multipathSuppression ? F("true") : F("false");
      out += F(",\"eq\":");
      out += snap.settings.equalizer ? F("true") : F("false");
      out += F(",\"mno\":");
      out += snap.settings.forcedMono ? F("true") : F("false");
      /* The de-emphasis the tuner is set to. It is written on every start, so
       * without it here there is no way to read back what the chip has. */
      out += F(",\"dem\":");
      out += String(snap.settings.deemphasisUs);
      out += F(",\"fnb\":");
      out += String(snap.settings.fmNoiseBlankerStart);
      out += F(",\"anb\":");
      out += String(snap.settings.amNoiseBlankerStart);
      out += F(",\"snr\":");
      out += String(snap.quality.snrDb);
      if (snap.processingValid) {
        /* What the chip is applying now, not what it was told. */
        out += F(",\"cut\":");
        out += String(snap.processing.highCut);
        out += F(",\"bld\":");
        out += String(snap.processing.stereo);
        out += F(",\"hbl\":");
        out += String(snap.processing.stHiBlend);
      }
      out += F(",\"wid\":");
      out += snap.bandwidthWide ? F("true") : F("false");
      /* Whether the dial is moving on its own. A caller that cannot tell
       * seeking from a person turning the knob shows the same thing for
       * both. */
      out += F(",\"skg\":");
      out += snap.seeking ? F("true") : F("false");
      out += F(",\"skf\":");
      out += snap.seekFound ? F("true") : F("false");
      out += F(",\"bep\":");
      out += snap.beeping ? F("true") : F("false");

      /* The squelch, so a radio that has gone quiet says why. */
      out += F(",\"sql\":\"");
      out += squelchModeName(snap.squelchMode);
      out += F("\",\"sqo\":");
      out += snap.squelchOpen ? F("true") : F("false");
      out += F(",\"hmu\":");
      out += snap.tunerMuted ? F("true") : F("false");
      if (snap.squelchMode == SQUELCH_MANUAL) {
        out += F(",\"sqa\":");
        out += String(snap.squelchThresholdTenths);
      }

      if (snap.lastError != TEF668X_OK) {
        out += F(",\"per\":\"");
        out += tef668xErrorText(snap.lastError);
        out += F("\"");
      }
    }

    if (haveSnap && snap.qualityValid) {
      Tef668xQuality q = snap.quality;
      char sig[200];
      /* Tenths go out as tenths, not as a decimal string, so nothing has to
       * parse a float and no precision is lost on the way. */
      snprintf(sig, sizeof(sig),
               ",\"sig\":%d,\"sav\":%d,\"usn\":%u,\"wam\":%u,\"off\":%d"
               ",\"bw\":%u,\"mod\":%d,\"st\":%s,\"plt\":%s",
               q.levelDbuVTenths, snap.levelSmoothedTenths,
               (unsigned)q.usnTenths,
               bandModulation(snap.settings.band) == MODULATION_FM
                   ? (unsigned)q.multipathTenths
                   : (unsigned)q.coChannelTenths,
               q.offsetKHzTenths, (unsigned)q.bandwidthKHz, q.modulationPercent,
               /* st is what comes out of the speaker, pilot is the chip's raw
                * flag. Forcing mono leaves the pilot where it was, so the two
                * differ, and a reader that wants to know whether the station
                * is transmitting stereo still has it. */
               (q.stereo && !snap.settings.forcedMono) ? "true" : "false",
               q.stereo ? "true" : "false");
      out += sig;
    }

    /* FM only. There is no RDS on the AM side, and an empty block there says
     * nothing that `bnd` does not already say. */
    if (haveSnap && bandModulation(snap.settings.band) == MODULATION_FM) {
      appendRdsState(out, snap);
    }
    out += F("}");
  } else {
    /* Say why, not just that it failed. Without this the only way to find out
     * is the cable, and the whole point of this phase is not needing one. */
    out += F("{\"err\":\"");
    out += tef668xErrorText(tunerStartError());
    out += F("\"");
    uint16_t dev = 0;
    uint16_t hw = 0;
    uint16_t sw = 0;
    if (tef668xLastIdentification(&dev, &hw, &sw)) {
      char words[64];
      snprintf(words, sizeof(words),
               ",\"dev\":\"%04X\",\"hwd\":\"%04X\",\"swd\":\"%04X\"", dev, hw,
               sw);
      out += words;
    }
    const Tef668xDiagnostics *d = tef668xDiagnostics();
    char diag[176];
    snprintf(diag, sizeof(diag),
             ",\"saw\":%s,\"rbt\":%s,\"bot\":%u"
             ",\"ptd\":%s,\"try\":%u,\"wnt\":%u",
             d->sawDevice ? "true" : "false",
             d->readBootStatus ? "true" : "false", (unsigned)d->bootStatus,
             d->patchLoaded ? "true" : "false", (unsigned)d->patchTried,
             (unsigned)d->patchWanted);
    out += diag;
    char xt[64];
    snprintf(xt, sizeof(xt), ",\"xad\":%u,\"xtl\":\"%s\"", (unsigned)d->xtalAdc,
             d->xtal ? d->xtal : "");
    out += xt;
    out += F("}");
  }
}

/*
 * The input layer, as one JSON field.
 *
 * There is no display yet, so this is the only way to tell a dead switch from
 * a wrong pin number. A press that shows up here but does nothing to the
 * radio is a mapping problem; a press that never shows up at all is wiring.
 *
 * | Key | Full name |
 * |---|---|
 * | `pad` | The keypad expander answered at start up |
 * | `clk` | Knob clicks since boot |
 * | `prs` | Button and key events since boot |
 * | `lst` | The last event in words, such as "BAND long" |
 * | `lms` | When that was, ms since boot. 0 for never |
 * | `typ` | Digits keyed and not yet entered |
 * | `pot` | The volume knob, 0 to 4095 |
 * | `pdb` | The volume that reading was turned into, in dB |
 */
static void appendInputState(String &out) {
  InputStatus in;
  inputStatusGet(&in);
  out += F("\"inp\":{\"pad\":");
  out += in.keypadPresent ? F("true") : F("false");
  out += F(",\"clk\":");
  out += String(in.clicks);
  out += F(",\"prs\":");
  out += String(in.presses);
  out += F(",\"lst\":\"");
  out += jsonEscape(in.lastEvent);
  out += F("\",\"lms\":");
  out += String(in.lastEventMs);
  out += F(",\"typ\":\"");
  out += jsonEscape(in.typed);
  out += F("\",\"lns\":");
  out += String(in.linesOk ? in.lines : 0xFFFF);
  out += F(",\"pot\":");
  out += String(in.pot);
  out += F(",\"pdb\":");
  out += String(in.potDb);
  /* The calibration, while one is running, so the page can show the knob
   * reaching further as it is turned. Without it the person has no sign that
   * turning the knob is doing anything, because during a calibration it
   * deliberately does not change the volume. */
  uint16_t calMin = 0;
  uint16_t calMax = 0;
  if (inputPotCalibrating(&calMin, &calMax)) {
    out += F(",\"pcl\":{\"min\":");
    out += String(calMin);
    out += F(",\"max\":");
    out += String(calMax);
    out += F("}");
  }
  out += F("}");
}

/*
 * Build the whole state document, device and tuner together.
 *
 * One builder for both /status.json and /api/state. They are the same
 * document under two names: /status.json is what phase 0 called it and what
 * the scripts in tools/ read, /api/state is the name the control API uses.
 * Two names for one document is better than two documents, which is what a
 * subset would become the first time a field is added to only one of them.
 */
static String buildState(void) {
  String out;
  out.reserve(768);
  out += F("{\"brd\":\"" BOARD_NAME "\",\"ver\":\"" FIRMWARE_VERSION
           "\",\"slt\":\"");
  out += rollbackRunningPartition();
  out += F("\",\"cnf\":");
  out += rollbackPending() ? F("false") : F("true");
  out += F(",\"net\":\"");
  out += inSetupMode() ? F("ap") : F("station");
  out += F("\",\"ip\":\"");
  out += wifiAddress();
  /* Closing the address string, then the separator. Kept here rather than
   * fused onto the front of the next field, so that moving that field into
   * its own builder cannot take the quote with it. */
  out += F("\",\"dpn\":");
  out += accessPinIsDefault(sAccessPin) ? F("true") : F("false");
  out += F(",\"hep\":");
  out += String(ESP.getFreeHeap());
  out += F(",\"up\":");
  out += String(millis() / 1000UL);
  /* What the panel light is doing. A dim and a wake are both silent, so
   * without this the only way to check either is to sit and watch the
   * radio. */
  uint8_t lit = 0;
  bool dimmed = screenTaskBacklightState(&lit);
  out += F(",\"pnl\":{\"lit\":");
  out += String((int)lit);
  out += F(",\"dim\":");
  out += dimmed ? F("true") : F("false");
  /* What the panel is actually showing for the signal, which is not the
   * smoothed level: the screen holds its number until the level moves a
   * whole dB away. Without this the only way to check that it sits still is
   * to stand at the radio and watch. */
  out += F(",\"sdb\":");
  out += String((int)screenTaskSignalShown());
  out += F("}");
  /* What the automatic save is doing. A save that never fires and one that
   * fires constantly both look the same from outside, and the second only
   * shows up years later as a worn out sector. */
  SettingsSaveStatus save;
  settingsTaskStatus(&save);
  out += F(",\"asv\":{\"n\":");
  out += String(save.saves);
  out += F(",\"dif\":");
  out += save.differs ? F("true") : F("false");
  out += F(",\"due\":");
  out += String(save.dueInMs);
  out += F(",\"bad\":");
  out += save.lastFailed ? F("true") : F("false");
  out += F(",\"idl\":");
  out += String(save.idleMs);
  out += F("}");
  /* The channel list. A write that quietly fails leaves a radio that looks
   * normal and forgets every channel at the next power cycle. */
  out += F(",\"chn\":{\"n\":");
  out += String(memoryStoreCount());
  out += F(",\"bad\":");
  out += memoryStoreFailed() ? F("true") : F("false");
  out += F("}");
  out += F(",");
  appendInputState(out);
  out += F(",");
  appendRadioState(out);
  out += F("}");
  return out;
}

/*
 * The radio's state as JSON, for scripts and for the test checklist.
 *
 * Keys are short on purpose. They are the names already used on the radio's
 * own screen and in `test/fixtures/agc/`, so a telemetry capture, a test
 * fixture and this endpoint all use one vocabulary. Decision 24 asks for one
 * schema across the API and telemetry, and this is it.
 *
 * Every number is an integer. Anything with a fraction is sent in tenths, so
 * nothing has to parse a float and no precision is lost.
 *
 * | Key | Full name | Unit |
 * |---|---|---|
 * | `brd` | Board id | |
 * | `ver` | Firmware version | |
 * | `slt` | Application partition this image booted from | |
 * | `cnf` | Image passed its self check and will not roll back | |
 * | `net` | `station` on a network, `ap` on its own access point | |
 * | `ip` | Address it can be reached on | |
 * | `dpn` | Access PIN is still 000000 | |
 * | `hep` | Free heap | bytes |
 * | `up` | Time since boot | seconds |
 *
 * Inside `pnl`, the panel light:
 *
 * | Key | Full name | Unit |
 * |---|---|---|
 * | `lit` | How bright the panel is now | percent |
 * | `dim` | It has been left alone long enough to have dropped | |
 * | `sdb` | The signal number on the panel, held still | whole dBuV |
 *
 * Inside `asv`, the automatic save. Not to be confused with `sav` above,
 * which is the smoothed signal level:
 *
 * | Key | Full name | Unit |
 * |---|---|---|
 * | `n` | Automatic saves written since boot | |
 * | `dif` | What the radio is set to is not what is stored | |
 * | `due` | How long until a save, 0 when none is waiting | ms |
 * | `bad` | The last automatic write was refused or failed | |
 * | `idl` | The wait in use. 0 means it is switched off | ms |
 *
 * Inside `tuner`, when the tuner started:
 *
 * | Key | Full name | Unit |
 * |---|---|---|
 * | `prt` | Which TEF668x is fitted | |
 * | `pch` | Tuner firmware version loaded into it | |
 * | `fsi` | Has FM stereo improvement | |
 * | `frd` | Has full search RDS | |
 * | `dr` | Has digital radio | |
 * | `sig` | Signal level, the reading as it came off the chip | tenths of a dBuV |
 * | `sav` | The same level smoothed, which is what the panel shows | tenths of a dBuV |
 * | `usn` | Ultrasonic noise | tenths of a percent |
 * | `wam` | Multipath, what the chip calls weighted AM | tenths of a percent |
 * | `off` | How far off centre the station is | tenths of a kHz |
 * | `bw` | Bandwidth the tuner settled on | kHz |
 * | `mod` | Modulation depth | percent |
 * | `st` | You are hearing stereo | |
 * | `plt` | The station is transmitting a stereo pilot | |
 * | `ims` | Multipath suppression, iMS on the old radio | |
 * | `eq` | Channel equalizer | |
 * | `mno` | Stereo refused on purpose | |
 * | `snr` | Signal to noise, worked out and not read from the chip | dB |
 * | `wid` | The adaptive filter is allowed to open | |
 * | `cut` | The treble roll off the chip is applying now | no known unit |
 * | `bld` | The stereo blend it is applying now | no known unit |
 * | `hbl` | The combined blend | no known unit |
 * | `sql` | Off, Auto or Manual | |
 * | `sqo` | The squelch is letting sound through | |
 * | `sqa` | The manual threshold. Manual only | tenths of a dBuV |
 * | `hmu` | What the tuner was told, against `mute` which is what was asked | |
 *
 * Inside `tuner` when it did not start, so a fault can be read without a
 * serial cable:
 *
 * | Key | Full name |
 * |---|---|
 * | `err` | What stopped it, in words |
 * | `device`, `hw`, `sw` | The three identification words, hex |
 * | `saw` | Something acknowledged at the I2C address |
 * | `rbt` | The operation status came back |
 * | `bot` | What it said. 0 means not patched yet |
 * | `ptd` | A patch was written this boot |
 * | `try` | Which patch version was written |
 * | `wnt` | Which one the chip then asked for |
 */
static void handleStatusJson(void) {
  sRequests++;
  sServer.send(200, "application/json", buildState());
}

/* ------------------------------------------------------------ control API -
 *
 * Decision 24: every command the radio can carry out is reachable over HTTP,
 * and the screen is one caller of the same queue. Nothing here talks to the
 * tuner. A request becomes a RadioCommand and goes on the queue, which is the
 * same path the buttons will use.
 *
 * Reads are open. Writes need the PIN.
 * ------------------------------------------------------------------------ */

static void apiFail(int code, const String &why) {
  sServer.send(code, "text/plain", why + "\n");
}

static String apiDescribe(const RadioSettings *s) {
  char text[16];
  bandFormatFrequency(s->band, s->freqKHz, text, sizeof(text));
  return String(bandName(s->band)) + " " + text + " " +
         bandFrequencyUnit(s->band);
}

/* Which part of the settled state a reply should describe. */
typedef enum {
  API_SAY_TEXT = 0,  /* Whatever the caller passed in. */
  API_SAY_TUNE,      /* The band and frequency it reached. */
  API_SAY_BANDWIDTH, /* The bandwidth it reached. */
  API_SAY_MODE,      /* The tuning mode it reached. */
  API_SAY_MUTE,      /* Whether it is muted. */
  API_SAY_FEATURES   /* Which of the four iMS and EQ states it reached. */
} ApiSay;

/* How long a request waits for the radio task to carry a command out. */
#define API_SETTLE_MS 500

/*
 * Check a command, carry it out, and say what happened.
 *
 * The one place a request turns into a command. Every endpoint below ends
 * here, so they cannot drift apart in how they validate or what they report.
 *
 * It waits for the radio to finish. Without that wait a script that sends two
 * requests back to back gets the second one judged against the state before
 * the first, which refuses things that are allowed and reports frequencies the
 * radio is not on.
 */
static void apiSubmit(const RadioCommand *command, const String &said,
                      ApiSay say = API_SAY_TEXT) {
  RadioError why = RADIO_OK;
  RadioPostResult posted = radioPostAndSettle(command, API_SETTLE_MS, &why);
  if (posted == RADIO_POST_BUSY) {
    apiFail(503, "The radio is busy. Try again in a moment.");
    return;
  }
  if (posted == RADIO_POST_SLOW) {
    /* 202, not an error. The command is on the queue and will be carried out.
     * Calling this a failure would have the caller send it again, and the
     * radio would do it twice. */
    apiFail(202,
            "The radio took it but has not confirmed yet. Read "
            "/api/state to see where it got to.");
    return;
  }
  /* The radio's own verdict on this exact command, not a guess made before it
   * was sent. Judging it beforehand against the last published state meant a
   * command refused by the radio still answered 200. */
  if (why != RADIO_OK) {
    apiFail(400, String("The radio refused it: ") + radioErrorText(why));
    return;
  }

  String answer = said;
  RadioSnapshot now;
  if (say != API_SAY_TEXT && radioGetSnapshot(&now)) {
    switch (say) {
      case API_SAY_TUNE:
        answer = apiDescribe(&now.settings);
        break;
      case API_SAY_BANDWIDTH:
        answer =
            now.settings.bandwidthKHz == 0
                ? String("bandwidth automatic")
                : String("bandwidth ") + now.settings.bandwidthKHz + " kHz";
        break;
      case API_SAY_MODE:
        answer = String("mode ") + tuneModeName(now.settings.tuneMode);
        break;
      case API_SAY_MUTE:
        answer = now.settings.muted ? String("muted") : String("unmuted");
        break;
      case API_SAY_FEATURES:
        /* Both named, on or off, so the reply says which of the four states
         * it landed in rather than only that something moved. */
        answer = String("iMS ") +
                 (now.settings.multipathSuppression ? "on" : "off") + ", EQ " +
                 (now.settings.equalizer ? "on" : "off");
        break;
      case API_SAY_TEXT:
      default:
        break;
    }
  }
  Serial.printf("[api] %s\n", answer.c_str());
  sServer.send(200, "text/plain", answer + "\n");
}

static bool apiNumber(const char *name, long *out, long low, long high) {
  if (!sServer.hasArg(name)) {
    apiFail(400, String("Give ") + name + ".");
    return false;
  }
  String raw = sServer.arg(name);
  char *end = NULL;
  long value = strtol(raw.c_str(), &end, 10);
  if (raw.length() == 0 || end == NULL || *end != '\0') {
    apiFail(400, String(name) + " has to be a whole number.");
    return false;
  }
  if (value < low || value > high) {
    apiFail(400,
            String(name) + " has to be between " + low + " and " + high + ".");
    return false;
  }
  *out = value;
  return true;
}

/*
 * GET /api/state. The whole of the radio, open to read.
 *
 * The same document as /status.json, from the same builder.
 */
static void handleApiState(void) {
  sRequests++;
  sServer.send(200, "application/json", buildState());
}

static void handleApiTune(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  long khz = 0;
  if (!apiNumber("khz", &khz, 1, 30000000L)) {
    return;
  }

  BandPlanConfig plan;
  if (!radioTaskPlan(&plan)) {
    apiFail(503, "The radio is not running.");
    return;
  }
  BandId band;
  if (!bandForFrequency(&plan, (uint32_t)khz, &band)) {
    apiFail(400, "That frequency is in no band.");
    return;
  }
  /* FM tunes in steps of 10 kHz on this chip, so anything finer is the
   * caller's mistake and never reaches the radio task. */
  if (bandModulation(band) == MODULATION_FM && (khz % 10) != 0) {
    apiFail(400, "The tuner cannot reach that. FM tunes in steps of 10 kHz.");
    return;
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_TUNE;
  cmd.freqKHz = (uint32_t)khz;

  char text[16];
  bandFormatFrequency(band, (uint32_t)khz, text, sizeof(text));
  apiSubmit(&cmd,
            String(bandName(band)) + " " + text + " " + bandFrequencyUnit(band),
            API_SAY_TUNE);
}

static void handleApiStep(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  long steps = 0;
  if (!apiNumber("stp", &steps, -1000, 1000)) {
    return;
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_STEP;
  cmd.steps = (int16_t)steps;

  /* Say where it landed, not just that it moved, so a script can check the
   * answer without a second request. */
  apiSubmit(&cmd, String("stepped ") + steps, API_SAY_TUNE);
}

static void handleApiBand(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  if (!sServer.hasArg("bnd")) {
    apiFail(400, "Give bnd, one of LW MW SW OIRT FM.");
    return;
  }
  String want = sServer.arg("bnd");
  want.toUpperCase();

  BandId band = BAND_COUNT;
  for (int b = 0; b < BAND_COUNT; b++) {
    if (want.equals(bandName((BandId)b))) {
      band = (BandId)b;
      break;
    }
  }
  if (band == BAND_COUNT) {
    apiFail(400, "That is not a band. Use one of LW MW SW OIRT FM.");
    return;
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_BAND;
  cmd.band = band;
  apiSubmit(&cmd, String("band ") + bandName(band), API_SAY_TUNE);
}

static void handleApiBandwidth(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  long khz = 0;
  if (!apiNumber("khz", &khz, 0, 6000)) {
    return;
  }
  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_BANDWIDTH;
  cmd.bandwidthKHz = (uint16_t)khz;
  apiSubmit(&cmd,
            khz == 0 ? String("bandwidth automatic")
                     : String("bandwidth ") + khz + " kHz",
            API_SAY_BANDWIDTH);
}

static void handleApiStepSize(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  long khz = 0;
  if (!apiNumber("khz", &khz, 1, 1000)) {
    return;
  }
  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_STEP;
  cmd.stepKHz = (uint16_t)khz;
  apiSubmit(&cmd, String("step ") + khz + " kHz");
}

static void handleApiVolume(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  long db = 0;
  if (!apiNumber("db", &db, RADIO_VOLUME_MIN, RADIO_VOLUME_MAX)) {
    return;
  }
  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_VOLUME;
  cmd.volumeDb = (int8_t)db;
  apiSubmit(&cmd, String("volume ") + db + " dB");
}

static void handleApiMute(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  long on = 0;
  if (!apiNumber("on", &on, 0, 1)) {
    return;
  }
  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_MUTE;
  cmd.muted = on != 0;
  apiSubmit(&cmd, on ? String("muted") : String("unmuted"), API_SAY_MUTE);
}

static void handleApiMode(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  if (!sServer.hasArg("mod")) {
    apiFail(400, "Give mod, one of Manual Auto Memory MeterBand.");
    return;
  }
  String want = sServer.arg("mod");
  want.toLowerCase();
  want.replace(" ", "");

  TuneMode mode = TUNE_MODE_COUNT;
  for (int m = 0; m < TUNE_MODE_COUNT; m++) {
    String name = tuneModeName((TuneMode)m);
    name.toLowerCase();
    name.replace(" ", "");
    if (want.equals(name)) {
      mode = (TuneMode)m;
      break;
    }
  }
  if (mode == TUNE_MODE_COUNT) {
    apiFail(400,
            "That is not a tuning mode. Use Manual, Auto, Memory or "
            "MeterBand.");
    return;
  }

  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_TUNE_MODE;
  cmd.tuneMode = mode;
  apiSubmit(&cmd, String("mode ") + tuneModeName(mode), API_SAY_MODE);
}

/*
 * GET /api/settings. What is stored, without the secrets.
 *
 * Behind the PIN in both directions. The struct holds the Wi-Fi passphrase
 * and the access PIN, so this says whether each one is set and never what it
 * is. A caller that wants to know the passphrase already has to be standing
 * at the radio.
 *
 * | Key | Full name |
 * |---|---|
 * | `sid` | The stored network name, empty when there is none |
 * | `pss` | A passphrase is stored. False is an open network |
 * | `dpn` | The access PIN is still 000000 |
 * | `ldd` | The stored settings were read back. False means the defaults |
 * | `rgn` | Which slice of the FM band, 0 to 4 |
 * | `spc` | Medium wave channel spacing, 0 for 9 kHz and 1 for 10 |
 * | `enc` | Which encoder is fitted, 0 standard and 1 optical |
 * | `edr` | 0 normal, 1 reversed |
 * | `sql` | The squelch mode this radio comes up in |
 * | `startBand`, `startFreqKHz` | Where it comes up |
 * | `svl` | The volume it comes up at, in manual squelch only |
 * | `fmsens`, `amsens` | How fussy seek is, per band |
 * | `ims`, `eq`, `mono` | The stored FM features |
 * | `cut`, `blend`, `hiblend` | The stored weak signal start levels |
 * | `fmnb`, `amnb` | The stored noise blanker percentages |
 * | `dem` | The stored FM de-emphasis, in microseconds |
 * | `abw` | The width the AM bands come up on |
 * | `blt`, `bdm`, `bds`, `blf` | The panel light |
 * | `sqf` | The auto squelch level floor in dBuV, 0 for off |
 *
 * These are what is stored, which is not always what the radio is set to now.
 * /api/state says what it is set to now. POST /api/save makes the two agree.
 */
static void handleApiSettingsGet(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  const Settings *st = sSettings;
  String out;
  out.reserve(512);
  out += F("{\"sid\":\"");
  out += jsonEscape(st->wifiSsid);
  out += F("\",\"pss\":");
  out += st->wifiPass[0] != '\0' ? F("true") : F("false");
  out += F(",\"dpn\":");
  out += accessPinIsDefault(sAccessPin) ? F("true") : F("false");
  /* Whether these are the stored settings at all. False means the blob could
   * not be read and the radio is running on the defaults, which otherwise
   * only shows as everything having gone back to how it was. */
  out += F(",\"ldd\":");
  out += settingsWereLoaded() ? F("true") : F("false");
  out += F(",\"sql\":\"");
  out += squelchModeName((SquelchMode)st->squelchMode);
  out += F("\"");
  out += F(",\"rgn\":");
  out += st->fmRegion;
  out += F(",\"spc\":");
  out += st->mwSpacing;
  out += F(",\"enc\":");
  out += st->encoderKind;
  out += F(",\"edr\":");
  out += st->encoderDirection;
  out += F(",\"sbd\":");
  out += st->startBand;
  out += F(",\"sfq\":");
  out += st->startFreqKHz;
  out += F(",\"svl\":");
  /* Through String, not straight in. This one is signed, and String appends a
   * signed char as the character it stands for rather than as a number. */
  out += String((int)st->startVolumeDb);
  out += F(",\"ims\":");
  out += st->fmMultipathSuppression;
  out += F(",\"eq\":");
  out += st->fmEqualizer;
  out += F(",\"mno\":");
  out += st->fmForcedMono;
  out += F(",\"cut\":");
  out += st->fmHighCutStart;
  out += F(",\"bld\":");
  out += st->fmStereoBlendStart;
  out += F(",\"hbl\":");
  out += st->fmStHiBlendStart;
  out += F(",\"fnb\":");
  out += st->fmNoiseBlankerStart;
  out += F(",\"anb\":");
  out += st->amNoiseBlankerStart;
  out += F(",\"dem\":");
  out += st->fmDeemphasisUs;
  out += F(",\"abw\":");
  out += st->amBandwidthKHz;
  out += F(",\"fsn\":");
  out += st->fmScanSensitivity;
  out += F(",\"asn\":");
  out += st->amScanSensitivity;
  out += F(",\"smu\":");
  out += st->softMuteMs;
  out += F(",\"bpk\":");
  out += st->beepKey;
  out += F(",\"bpe\":");
  out += st->beepEdge;
  out += F(",\"bps\":");
  out += st->beepStart;
  out += F(",\"sqf\":");
  out += st->fmSquelchFloor;
  out += F(",\"rds\":");
  out += st->rdsEnabled;
  out += F(",\"blt\":");
  out += st->backlightPercent;
  out += F(",\"bdm\":");
  out += st->backlightDimPercent;
  out += F(",\"bds\":");
  out += st->backlightDimAfterS;
  out += F(",\"blf\":");
  out += st->backlightFade;
  out += F("}");
  sServer.send(200, "application/json", out);
}

/*
 * POST /api/settings. The things that are stored and not tuned.
 *
 * Takes `sid` with an optional `pwd`, and `pin`, and the settings that are
 * stored rather than tuned. Every row below is one entry of `stored[]` in
 * the handler:
 *
 * | Argument | Range | Acts | What it is |
 * |---|---|---|---|
 * | `rgn` | 0 to 4 | at start | Which slice of the FM band |
 * | `spc` | 0 or 1 | at start | Medium wave channels, 9 kHz or 10 kHz |
 * | `enc` | 0 or 1 | at start | Which encoder is fitted, standard or optical |
 * | `edr` | 0 or 1 | at start | Normal, or reversed |
 * | `bps` | 0 or 1 | at start | Chime when the radio comes on |
 * | `sqf` | 0 to 40 | at once | Auto squelch FM level floor, dBuV. 0 is off |
 * | `fsn` | 1 to 6 | at once | How fussy seek is on FM. Higher finds weaker |
 * | `asn` | 1 to 6 | at once | The same on the AM bands |
 * | `smu` | 0 to 500 | at once | The mute and squelch ramp, in ms. 0 is off |
 * | `bpk` | 0 to 3 | at once | Which presses beep |
 * | `bpe` | 0 or 1 | at once | Beep at a band edge |
 * | `blt` | 5 to 100 | at once | Panel brightness, per cent |
 * | `bdm` | 0 to 100 | at once | Panel brightness once left alone |
 * | `bds` | 0 to 240 | at once | Seconds before it dims. 0 never |
 * | `blf` | 0 or 1 | at start | Fade the panel up rather than snap it on |
 * | `rds` | 0 or 1 | at once | Run the RDS decoder. Off gives the radio task back two thirds of its wakeups on FM |
 *
 * The table above and `stored[]` in the handler have to agree. The handler
 * counts its own field list against that table and refuses the request if
 * the two ever disagree, but nothing checks this comment.
 *
 * Everything given is checked before anything is written, and then one save
 * puts the lot in NVS. A half applied change, say a new PIN stored against
 * the old network, is worse than no change at all.
 *
 * The reply says which of the two happened, and says both when a request
 * mixes them. The band plan decides which frequencies exist, and changing
 * that under a radio that is tuned to one of them is a change with no right
 * answer, which is why those wait for a restart. The rest of
 * the radio's settings are not here: they are changed with /api/fm,
 * /api/squelch and the like, which act at once, and kept with /api/save.
 *
 * Changing the PIN ends the session, the same as the form does. Changing the
 * network moves the radio off whatever it is on, so the reply goes out first.
 */
static void handleApiSettingsPost(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }

  bool wantWifi = sServer.hasArg("sid");
  bool wantPin = sServer.hasArg("pin");

  /* The four that only take effect at the next start. Each is a number with
   * a fixed set of values, and settingsValid checks the lot again below
   * against the enums they name. */
  /* `atStart` says whether the radio can only act on it when it next starts,
   * which is what the reply has to tell the caller. Carried on the row rather
   * than worked out from the index, so reordering the table cannot silently
   * change which settings claim to need a reboot. */
  struct {
    const char *name;
    long low;
    long high;
    bool atStart;
  } stored[] = {
      {"rgn", 0, (long)FM_REGION_COUNT - 1, true},
      {"spc", 0, (long)MW_SPACING_10K, true},
      {"enc", 0, (long)ENCODER_OPTICAL, true},
      {"edr", 0, (long)ENCODER_REVERSED, true},
      {"fsn", SEEK_SENSITIVITY_MIN, SEEK_SENSITIVITY_MAX, false},
      {"asn", SEEK_SENSITIVITY_MIN, SEEK_SENSITIVITY_MAX, false},
      {"smu", 0, 500, false},
      {"bpk", 0, (long)BEEP_MODE_COUNT - 1, false},
      {"bpe", 0, 1, false},
      {"bps", 0, 1, true},
      {"sqf", 0, SQUELCH_FM_LEVEL_FLOOR_MAX_DBUV, false},
      {"blt", BACKLIGHT_MIN_AWAKE, 100, false},
      {"bdm", 0, 100, false},
      {"bds", 0, BACKLIGHT_DIM_AFTER_MAX_S, false},
      /* At start, because the only thing it changes is how the panel comes
       * up, and that has already happened by the time anybody can set it. */
      {"blf", 0, 1, true},
      {"rds", 0, 1, false},
  };
  /* Sized from the table, not from a number written beside it. A seventh row
   * would otherwise run off the end of all three of these with no warning. */
  const int kStored = (int)(sizeof(stored) / sizeof(stored[0]));
  long values[sizeof(stored) / sizeof(stored[0])] = {0};
  bool given[sizeof(stored) / sizeof(stored[0])] = {false};
  bool wantStored = false;
  bool wantNow = false;
  bool wantReboot = false;
  for (int i = 0; i < kStored; i++) {
    if (!sServer.hasArg(stored[i].name)) {
      continue;
    }
    if (!apiNumber(stored[i].name, &values[i], stored[i].low, stored[i].high)) {
      return;
    }
    given[i] = true;
    wantStored = true;
    if (stored[i].atStart) {
      wantReboot = true;
    } else {
      wantNow = true;
    }
  }

  if (!wantWifi && !wantPin && !wantStored) {
    apiFail(400,
            "Give sid, pin, rgn, spc, enc, edr, fsn, asn, smu, bpk, bpe, "
            "bps, sqf, blt, bdm, bds, blf or rds, or any mix of them.");
    return;
  }

  Settings pending = *sSettings;
  uint32_t newPin = sAccessPin;

  /* One entry per row of `stored`, sized from the same expression, so a row
   * added there cannot walk off the end of this. `smu` is two bytes wide and
   * has no slot here, so it is written below. */
  uint8_t *fields[sizeof(stored) / sizeof(stored[0])] = {
      &pending.fmRegion,
      &pending.mwSpacing,
      &pending.encoderKind,
      &pending.encoderDirection,
      &pending.fmScanSensitivity,
      &pending.amScanSensitivity,
      NULL,
      &pending.beepKey,
      &pending.beepEdge,
      &pending.beepStart,
      &pending.fmSquelchFloor,
      &pending.backlightPercent,
      &pending.backlightDimPercent,
      &pending.backlightDimAfterS,
      &pending.backlightFade,
      &pending.rdsEnabled};
  /* The compiler checks the two tables are the same length. An entry left
   * out is otherwise value initialised to NULL and silently does nothing,
   * which is how the chime came to be unswitchable. */
  static_assert(
      sizeof(fields) / sizeof(fields[0]) == sizeof(stored) / sizeof(stored[0]),
      "every row of stored needs a slot in fields");
  for (int i = 0; i < kStored; i++) {
    if (given[i] && fields[i] != NULL) {
      *fields[i] = (uint8_t)values[i];
    }
  }
  /* Found by name, not by a number written here. A row inserted before it in
   * the table would otherwise put somebody else's value into softMuteMs, with
   * nothing to say so. */
  for (int i = 0; i < kStored; i++) {
    if (given[i] && strcmp(stored[i].name, "smu") == 0) {
      pending.softMuteMs = (uint16_t)values[i];
    }
  }

  if (wantWifi) {
    String ssid = sServer.arg("sid");
    String pass = sServer.hasArg("pwd") ? sServer.arg("pwd") : String("");
    if (ssid.length() == 0) {
      apiFail(400, "A network name is needed.");
      return;
    }
    if (!settingsSetWifi(&pending, ssid.c_str(), pass.c_str())) {
      apiFail(400, "That network name or passphrase is too long.");
      return;
    }
  }

  if (wantPin) {
    if (!accessPinParse(sServer.arg("pin").c_str(), &newPin)) {
      apiFail(400, "A PIN is six digits.");
      return;
    }
    pending.accessPin = newPin;
  }

  /* The whole struct, not only what this request touched. A blob that fails
   * this is one the radio would refuse to load at its next start, and that
   * is a radio that comes up on the defaults with no explanation. */
  if (!settingsValid(&pending)) {
    apiFail(400, "Those settings are not a set this radio can use.");
    return;
  }
  if (!settingsNvsSave(&pending)) {
    apiFail(500, "The settings could not be written. Nothing changed.");
    return;
  }
  *sSettings = pending;
  settingsTaskSaved();

  /* The seek sensitivities are the one part of this endpoint that acts at
   * once. They are not read at start up like the band plan: nothing is tuned
   * to them, so there is nothing for a change to be unfair to. */
  if (wantNow) {
    SeekConfig seekCfg;
    seekDefaults(&seekCfg);
    seekCfg.fmSensitivity = pending.fmScanSensitivity;
    seekCfg.amSensitivity = pending.amScanSensitivity;
    radioSetSeekConfig(&seekCfg);
    /* The polish acts at once too. Nothing is tuned to it, so there is
     * nothing for a change to be unfair to. */
    radioSetSoftMuteMs(pending.softMuteMs);
    radioSetEdgeBeep(pending.beepEdge != 0);
    radioSetSquelchFloor(pending.fmSquelchFloor);
    radioSetRdsEnabled(pending.rdsEnabled != 0);
    inputSetBeeps((BeepMode)pending.beepKey);
    /* The panel light changes while the person is looking at it, which is the
     * only way a brightness can be chosen. */
    BacklightConfig backlight;
    backlightFromSettings(&pending, &backlight);
    screenTaskSetBacklight(&backlight);
  }

  String said;
  if (wantNow) {
    said += F("Saved and in use now.");
  }
  if (wantReboot) {
    if (said.length() > 0) {
      said += F(" ");
    }
    said += F("Read at start up, so reboot for that to take effect.");
  }
  if (wantPin) {
    if (said.length() > 0) {
      said += F(" ");
    }
    sAccessPin = newPin;
    /* The session was opened with the old PIN, so it goes. */
    dropSession();
    accessPinGateReset(&sGate);
    Serial.println("[web] the access PIN was changed");
    said += accessPinIsDefault(newPin)
                ? F("PIN changed to the default, so the radio is open to "
                    "anyone on the network. Sign in again.")
                : F("PIN changed. Sign in again.");
  }
  if (wantWifi) {
    Serial.printf("[web] new credentials saved for %s\n", pending.wifiSsid);
    if (said.length() > 0) {
      said += F(" ");
    }
    said +=
        F("Network saved. The radio is trying it now, so this address "
          "may stop answering.");
  }
  sServer.send(200, "text/plain", said + "\n");

  if (wantWifi) {
    /* Answer first, then move the radio, or the reply never reaches a caller
     * on the access point that is being torn down. Closing the socket is what
     * puts the bytes on the wire. */
    sServer.client().stop();
    delay(200);
    wifiRetryNow(sSettings);
  }
}

/*
 * POST /api/pot. Learn how far this unit's volume knob turns.
 *
 * Takes `action`: `start`, `finish` or `cancel`.
 *
 * The built in ends of travel, 120 to 4000, are one radio's numbers taken
 * from the reference firmware. This unit reaches 0 and 4095, so they are not
 * wrong here, but a knob that read 200 to 3800 would lose travel at both ends
 * with nothing to say so.
 *
 * Between `start` and `finish` the knob sets neither the volume nor the
 * squelch. It only records how far it goes, because finding the loud end stop
 * should not mean sweeping the volume to full on the way. `GET /api/state`
 * reports `potCal` while it runs, with the lowest and highest seen so far.
 *
 * `finish` refuses a sweep narrower than a quarter of the converter, because
 * a knob that barely moved was not swept end to end and storing what it saw
 * would leave the radio with almost no usable travel.
 */
static void handleApiPot(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  if (!sServer.hasArg("act")) {
    apiFail(400, "Give act, one of start finish cancel.");
    return;
  }
  String want = sServer.arg("act");
  want.toLowerCase();

  if (want == "start") {
    inputPotCalibrateStart();
    sServer.send(
        200, "text/plain",
        "Turn the volume knob all the way to each end, then finish.\n");
    return;
  }

  if (want == "cancel") {
    inputPotCalibrateCancel();
    sServer.send(200, "text/plain", "Nothing changed.\n");
    return;
  }

  if (want != "finish") {
    apiFail(400, "That is not an action. Use start, finish or cancel.");
    return;
  }

  uint16_t rawMin = 0;
  uint16_t rawMax = 0;
  if (!inputPotCalibrateFinish(&rawMin, &rawMax)) {
    apiFail(400, String("The knob only moved from ") + rawMin + " to " +
                     rawMax +
                     ", which is not a full sweep. Nothing changed. Start "
                     "again and turn it all the way to both ends.");
    return;
  }

  Settings pending = *sSettings;
  pending.potRawMin = rawMin;
  pending.potRawMax = rawMax;
  if (!settingsValid(&pending) || !settingsNvsSave(&pending)) {
    /* The knob is already using the new travel, because that is what made it
     * worth telling the person about. Say plainly that it will not survive a
     * power cycle rather than implying the whole thing failed. */
    apiFail(500, String("The knob now runs ") + rawMin + " to " + rawMax +
                     ", but it could not be stored, so it goes back at the "
                     "next start.");
    return;
  }
  *sSettings = pending;
  settingsTaskSaved();

  String said =
      String("The knob runs ") + rawMin + " to " + rawMax + ", stored.";
  Serial.printf("[api] %s\n", said.c_str());
  sServer.send(200, "text/plain", said + "\n");
}

/*
 * POST /api/beep. Sound the tuner's own tone generator.
 *
 * Takes `ms`, 1 to 3000, and optionally `hz` and `hz2`, each 100 to 15000.
 * The radio beeps for that long and stops on its own. Two different pitches
 * are what a DTMF pair needs, if the generator's two slots are two tones
 * rather than the two output channels.
 *
 * This is here because the tone generator is the one piece of audio hardware
 * on this radio that is not the tuner receiving something, and a beep of
 * fifty milliseconds is not long enough to tell "it did not sound" from "it
 * sounded and I missed it". A long tone answers that in one press.
 *
 * A muted radio stays silent. The tone goes through the same output mute as
 * everything else.
 */
static void handleApiBeep(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  long ms = 0;
  if (!apiNumber("ms", &ms, 1, 3000)) {
    return;
  }
  long hz = 2000;
  long hz2 = 0;
  if (sServer.hasArg("hz") && !apiNumber("hz", &hz, 100, 15000)) {
    return;
  }
  if (sServer.hasArg("hz2") && !apiNumber("hz2", &hz2, 100, 15000)) {
    return;
  }
  if (hz2 == 0) {
    hz2 = hz;
  }
  if (!radioBeepAt((uint16_t)ms, (uint16_t)hz, (uint16_t)hz2)) {
    apiFail(503, "The radio is busy. Try again in a moment.");
    return;
  }
  sServer.send(200, "text/plain", String("beeping for ") + ms + " ms\n");
}

/* -------------------------------------------------------- memory channels */

static bool apiSlot(int *out, bool required) {
  if (!required && !sServer.hasArg("slot")) {
    *out = MEMORY_NO_SLOT;
    return true;
  }
  long slot = 0;
  if (!apiNumber("slot", &slot, 1, MEMORY_SLOT_COUNT)) {
    return false;
  }
  *out = (int)slot - 1;
  return true;
}

static void handleApiMemoryCsv(void) {
  sRequests++;
  sServer.sendHeader("Content-Disposition",
                     "attachment; filename=\"channels.csv\"");
  sServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sServer.send(200, "text/csv", "");
  sServer.sendContent(memoryCsvHeader());
  char line[MEMORY_CSV_LINE_MAX];
  for (int i = 0; i < MEMORY_SLOT_COUNT; i++) {
    size_t need = memoryStoreLine(i, line, sizeof(line));
    if (need == 0 || need >= sizeof(line)) {
      continue;
    }
    sServer.sendContent(line);
  }
  sServer.sendContent("");
}

/*
 * GET /api/rds/raw. The groups the tuner handed over, as they arrived.
 *
 * This is the capture route for RDS test fixtures. There is no serial cable
 * on this radio, so real groups cannot be read off it any other way, and a
 * decoder tested only against invented groups is tested against a broadcast
 * nobody transmits. `tools/rdscap.py` polls this and writes the log that
 * `test/fixtures/rds/` holds.
 *
 * One line per group, oldest first:
 *
 *     seq A B C D err
 *     4213 5241 0408 E0CD 2020 00
 *
 * `seq` counts groups since the radio tuned this station, so a script polling
 * this can see whether it missed any between two calls. The four blocks are
 * hex. `err` is the tuner's own confidence, two bits per block with block A in
 * the top pair: 0 clean, 1 or 2 corrected, 3 not corrected.
 *
 * The first line is a comment giving the frequency and how many groups have
 * arrived, so a saved capture says which station it came from. `stat` and
 * `read` come back as a question mark when the radio task held the lock and
 * they could not be found out, which is not the same as a read that has not
 * happened.
 *
 * Open to read, like the rest of the state. It carries nothing that is not
 * being broadcast to the whole city.
 */
static void handleApiRdsRaw(void) {
  sRequests++;
  if (!radioRdsEnabled()) {
    /* Said outright. An empty list would read as a station sending nothing,
     * which is a different thing from a decoder nobody has switched on. */
    sServer.send(200, "text/plain",
                 "# the RDS decoder is switched off, so no groups are read\n");
    return;
  }
  static RadioRdsRaw groups[RADIO_RDS_RAW_DEPTH];
  uint32_t first = 0;
  uint32_t total = 0;
  uint32_t dropped = 0;
  uint16_t held =
      radioRdsRaw(groups, RADIO_RDS_RAW_DEPTH, &first, &total, &dropped);

  RadioSnapshot snap;
  uint32_t khz = 0;
  if (radioGetSnapshot(&snap)) {
    khz = snap.settings.freqKHz;
  }

  sServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sServer.send(200, "text/plain", "");
  char line[64];
  uint16_t status = 0;
  bool statusRead = false;
  if (radioRdsStatus(&status, &statusRead)) {
    snprintf(line, sizeof(line),
             "# khz=%u total=%u held=%u lost=%u stat=%04X read=%d\n",
             (unsigned)khz, (unsigned)total, (unsigned)held, (unsigned)dropped,
             status, statusRead ? 1 : 0);
  } else {
    /* The radio task had the lock. `read=0` here would say the chip has not
     * been read, which is what the AM side truthfully says, so it has to be
     * a third answer rather than that one. */
    snprintf(line, sizeof(line),
             "# khz=%u total=%u held=%u lost=%u stat=? read=?\n", (unsigned)khz,
             (unsigned)total, (unsigned)held, (unsigned)dropped);
  }
  sServer.sendContent(line);
  for (uint16_t i = 0; i < held; i++) {
    snprintf(line, sizeof(line), "%u %04X %04X %04X %04X %02X\n",
             (unsigned)(first + i), groups[i].block[0], groups[i].block[1],
             groups[i].block[2], groups[i].block[3], groups[i].error);
    sServer.sendContent(line);
  }
  sServer.sendContent("");
}

/*
 * POST /api/memory. What to do comes in `do`.
 *
 * | `do` | What it does |
 * |---|---|
 * | `store` | Write what the radio is playing into a slot, or the first free one |
 * | `set` | Write a slot from `khz`, and `bw` and `name` if given |
 * | `clear` | Empty one slot |
 * | `recall` | Tune to a slot |
 * | `wipe` | Empty every slot |
 */
static void handleApiMemoryPost(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  if (!sServer.hasArg("do")) {
    apiFail(400, "Give do=store, set, clear, recall or wipe.");
    return;
  }
  String what = sServer.arg("do");
  /* Checked before anything else is read, so an action nobody recognises is
   * answered with that rather than with a complaint about a missing slot. */
  if (what != "store" && what != "set" && what != "clear" && what != "recall" &&
      what != "wipe") {
    apiFail(400, "do has to be store, set, clear, recall or wipe.");
    return;
  }

  if (what == "wipe") {
    if (!memoryStoreWipe()) {
      apiFail(503, "The channel list could not be reached.");
      return;
    }
    sServer.send(200, "text/plain", "Every channel cleared.\n");
    return;
  }

  int slot = MEMORY_NO_SLOT;
  if (!apiSlot(&slot, what != "store")) {
    return;
  }

  if (what == "clear") {
    if (!memoryStoreClearSlot(slot)) {
      apiFail(503, "The channel list could not be reached.");
      return;
    }
    sServer.send(200, "text/plain",
                 String("Channel ") + (slot + 1) + " cleared.\n");
    return;
  }

  if (what == "recall") {
    MemoryChannel c;
    if (!memoryStoreRead(slot, &c)) {
      apiFail(404, String("Channel ") + (slot + 1) + " is empty.");
      return;
    }
    BandPlanConfig plan;
    if (radioTaskPlan(&plan) && !memoryChannelTunable(&c, &plan)) {
      apiFail(409, String("Channel ") + (slot + 1) + " says " +
                       bandName((BandId)c.band) +
                       " but that frequency is not on that band here. Change "
                       "the channel or the band plan.");
      return;
    }
    RadioCommand cmd = {};
    cmd.kind = RADIO_RECALL;
    cmd.memorySlot = (int16_t)slot;
    apiSubmit(&cmd, String("recalled ") + (slot + 1), API_SAY_TUNE);
    return;
  }

  MemoryChannel c;
  memset(&c, 0, sizeof(c));
  /* What is in the slot now, so a request that gives only the frequency keeps
   * the name and the width that are already there. Writing a fresh channel
   * would wipe the name of anybody correcting a frequency. */
  bool had = slot != MEMORY_NO_SLOT && memoryStoreRead(slot, &c);

  if (what == "store") {
    RadioSnapshot snap;
    if (!radioGetSnapshot(&snap)) {
      apiFail(503, "The radio is not running.");
      return;
    }
    /* Storing what is playing replaces the slot outright. The frequency, the
     * band and the width all come off the radio, and keeping the old name on
     * a different station would be worse than losing it. */
    memset(&c, 0, sizeof(c));
    had = false;
    c.band = (uint8_t)snap.settings.band;
    c.freqKHz = snap.settings.freqKHz;
    c.bandwidthKHz = snap.settings.bandwidthKHz;
    if (slot == MEMORY_NO_SLOT) {
      slot = memoryStoreFirstFree();
      if (slot == MEMORY_NO_SLOT) {
        apiFail(409, "Every channel is taken. Name a slot to overwrite.");
        return;
      }
    }
  } else if (what == "set") {
    long khz = 0;
    if (!apiNumber("khz", &khz, 1, 30000000L)) {
      return;
    }
    BandPlanConfig plan;
    if (!radioTaskPlan(&plan)) {
      apiFail(503, "The radio is not running.");
      return;
    }
    BandId band;
    if (!bandForFrequency(&plan, (uint32_t)khz, &band)) {
      apiFail(400, "That frequency is in no band.");
      return;
    }
    if (!had || c.band != (uint8_t)band) {
      /* A frequency that moves the channel to another band cannot keep the
       * old band's filter width, and there is no width the new band is known
       * to want, so it goes back to letting the radio choose. */
      c.bandwidthKHz = 0;
    }
    c.band = (uint8_t)band;
    c.freqKHz = (uint32_t)khz;
    if (sServer.hasArg("bw")) {
      long bw = 0;
      if (!apiNumber("bw", &bw, 0, 400)) {
        return;
      }
      c.bandwidthKHz = (uint16_t)bw;
    }
  }

  if (sServer.hasArg("name")) {
    String name = sServer.arg("name");
    /* Checked before it is stored so the reason can name the character. The
     * validity check refuses this too, but it cannot say which of four rules
     * the name broke. */
    strncpy(c.name, name.c_str(), MEMORY_NAME_LEN - 1);
    c.name[MEMORY_NAME_LEN - 1] = '\0';
  }

  if (!memoryChannelValid(&c)) {
    /* Only the name and the width can still be wrong here, since the band and
     * the frequency were checked above. Saying which two saves the caller
     * guessing among four fields. */
    apiFail(400,
            "That channel cannot be stored. The name has to be plain text, "
            "and the bandwidth has to be one this band offers.");
    return;
  }
  if (!memoryStoreWrite(slot, &c)) {
    apiFail(503, "The channel list could not be reached.");
    return;
  }
  char text[16];
  bandFormatFrequency((BandId)c.band, c.freqKHz, text, sizeof(text));
  sServer.send(200, "text/plain",
               String("Channel ") + (slot + 1) + " is " +
                   bandName((BandId)c.band) + " " + text + " " +
                   bandFrequencyUnit((BandId)c.band) + "\n");
}

/*
 * POST /api/memory/import. The file is the body, the mode is in the query.
 *
 * `mode=merge` fills the empty slots only. `mode=replace` throws the list
 * away and loads the file, and refuses the whole file if one line cannot be
 * read, so a bad file never leaves a half loaded list.
 */
static void handleApiMemoryImport(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  MemoryImportMode mode = MEMORY_IMPORT_MERGE;
  if (sServer.hasArg("mode")) {
    String text = sServer.arg("mode");
    if (text == "replace") {
      mode = MEMORY_IMPORT_REPLACE;
    } else if (text != "merge") {
      apiFail(400, "mode has to be merge or replace.");
      return;
    }
  }
  if (!sServer.hasArg("plain")) {
    /* The server only fills `plain` when the content type is not
     * application/x-www-form-urlencoded, which is what curl -d sends unless
     * it is told otherwise. Saying so here saves the caller working out why
     * a file they did send looks missing. */
    apiFail(400,
            "Send the CSV as the body of the request, with a content type of "
            "text/csv.");
    return;
  }
  String body = sServer.arg("plain");
  MemoryImportResult result;
  if (!memoryStoreImport(mode, body.c_str(), body.length(), &result)) {
    apiFail(400, String("Line ") + result.firstBadLine +
                     " could not be read, so nothing was changed.");
    return;
  }
  String said = String(result.imported) + " of " + result.lines +
                " lines imported, " + result.skipped + " refused, " +
                result.kept + " already taken, " + result.truncated +
                " names cut short";
  Serial.printf("[api] %s\n", said.c_str());
  sServer.send(200, "text/plain", said + "\n");
}

/*
 * POST /api/seek. Hunt for the next station.
 *
 * Takes `dir`: `up` or `down`. Returns as soon as the seek has started, not
 * when it has finished, because a pass of the FM band takes about ten seconds
 * and holding an HTTP request open for that would tie up the one connection
 * this server has.
 *
 * Watch `seeking` in `GET /api/state` to see when it stops, and `seekFound`
 * to see whether it found anything. Any other command stops it.
 */
static void handleApiSeek(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  if (!sServer.hasArg("dir")) {
    apiFail(400, "Give dir, up or down.");
    return;
  }
  String want = sServer.arg("dir");
  want.toLowerCase();
  bool up = false;
  if (want == "up") {
    up = true;
  } else if (want != "down") {
    apiFail(400, "That is not a direction. Use up or down.");
    return;
  }

  if (!radioSeek(up)) {
    apiFail(503, "The radio is busy. Try again in a moment.");
    return;
  }
  String said = String("seeking ") + (up ? "up" : "down");
  Serial.printf("[api] %s\n", said.c_str());
  sServer.send(200, "text/plain", said + "\n");
}

/*
 * POST /api/save. Keep what the radio is set to now.
 *
 * Takes nothing. It reads the radio's own state and writes the parts worth
 * keeping into NVS: the band and frequency, the FM features, the weak signal
 * levels, the noise blankers, the de-emphasis, the AM width and the squelch
 * mode. The radio comes up on all of it next time.
 *
 * Why this rather than a stored copy of every setting alongside the live one:
 * two copies of the same thing drift, and then a person has to know which of
 * the two a given page is showing. There is one set of live values, changed
 * through the endpoints that act at once, and this puts them somewhere they
 * survive a power cycle.
 *
 * The volume is not kept. It belongs to the knob, and a stored volume would
 * argue with the knob at every start up.
 */
static void handleApiSave(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }

  /* Built by the same call the automatic save uses, so the two can never
   * write different subsets of what the radio is set to.
   *
   * One snapshot, not two. Taking a second one for the reply lets the dial
   * move in between, and the message then names a station that was not the
   * one written. */
  Settings pending;
  if (!settingsBuildCandidate(sSettings, &pending, NULL)) {
    apiFail(503, "The radio is busy. Nothing was saved.");
    return;
  }

  /* The radio can reach states the stored form has no room for, and the AM
   * width on an FM band is one of them. Refusing here beats writing a blob
   * the next start would throw away without saying so. */
  if (!settingsValid(&pending)) {
    apiFail(500, "The radio is in a state that cannot be stored.");
    return;
  }
  if (!settingsNvsSave(&pending)) {
    apiFail(500, "The settings could not be written. Nothing changed.");
    return;
  }
  *sSettings = pending;
  /* So an automatic save is not left due the moment this one lands. */
  settingsTaskSaved();

  /* From what was written, not from a fresh look at the radio. The reply has
   * to name the station that went into NVS. */
  BandId saved = (BandId)pending.startBand;
  char text[16];
  bandFormatFrequency(saved, pending.startFreqKHz, text, sizeof(text));
  String said = String("Saved. It will come up on ") + text + " " +
                bandFrequencyUnit(saved) + ", squelch " +
                squelchModeName((SquelchMode)pending.squelchMode) + ".";
  Serial.printf("[api] %s\n", said.c_str());
  sServer.send(200, "text/plain", said + "\n");
}

/*
 * POST /api/cycle. The next one, whatever it is now.
 *
 * Takes `wht`: `band`, `bandwidth`, `mode`, `mute` or `features`. This is
 * what the BAND, BW and MODE buttons and the push on the knob send, so a
 * script can drive the radio the way a hand does. Decision 24: if the panel
 * can do it, the API can do it.
 *
 * `features` walks the four combinations of iMS and the channel equalizer,
 * which is the MODE long press.
 *
 * The radio works out the next value from its own state rather than being
 * told one. A caller that read the state, worked out the next value and sent
 * that would leave a gap for the state to move in.
 */
static void handleApiCycle(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }
  if (!sServer.hasArg("wht")) {
    apiFail(400, "Give wht, one of band bandwidth mode mute features.");
    return;
  }
  String want = sServer.arg("wht");
  want.toLowerCase();

  RadioCommand cmd = {};
  ApiSay say = API_SAY_TEXT;
  if (want == "band") {
    cmd.kind = RADIO_CYCLE_BAND;
    say = API_SAY_TUNE;
  } else if (want == "bandwidth") {
    cmd.kind = RADIO_CYCLE_BANDWIDTH;
    say = API_SAY_BANDWIDTH;
  } else if (want == "features") {
    cmd.kind = RADIO_CYCLE_FM_FEATURES;
    say = API_SAY_FEATURES;
  } else if (want == "mode") {
    cmd.kind = RADIO_CYCLE_TUNE_MODE;
    say = API_SAY_MODE;
  } else if (want == "mute") {
    cmd.kind = RADIO_TOGGLE_MUTE;
    say = API_SAY_MUTE;
  } else {
    apiFail(400,
            "That is not something to cycle. Use band, bandwidth, mode, "
            "mute or features.");
    return;
  }
  apiSubmit(&cmd, String("cycled ") + want, say);
}

/*
 * POST /api/squelch. What decides whether the audio is open.
 *
 * Takes `mode`: `off`, `auto` or `manual`.
 *
 * The mode also decides what the pot on the front does. There is one knob, so
 * it is the volume control or the squelch control and never both: off and
 * auto leave it as the volume, manual takes it for the squelch.
 *
 * There is deliberately no way to set the manual threshold here. In manual the
 * knob is the threshold, and a second way to set it is a second owner for one
 * value: setting it here answered with the number asked for and the knob
 * replaced it a fraction of a second later, so the reply was untrue before the
 * caller had read it. Read the threshold back from `sqlAt` and turn the knob
 * to change it.
 */
static void handleApiSquelch(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }

  if (sServer.hasArg("thr")) {
    apiFail(400,
            "The knob sets the threshold in manual, so it cannot be set "
            "here. Read it back from sqlAt.");
    return;
  }
  if (!sServer.hasArg("mod")) {
    apiFail(400, "Give mod, one of off auto manual.");
    return;
  }

  String want = sServer.arg("mod");
  want.toLowerCase();
  SquelchMode mode = SQUELCH_MODE_COUNT;
  for (int m = 0; m < SQUELCH_MODE_COUNT; m++) {
    String name = squelchModeName((SquelchMode)m);
    name.toLowerCase();
    if (want.equals(name)) {
      mode = (SquelchMode)m;
      break;
    }
  }
  if (mode == SQUELCH_MODE_COUNT) {
    apiFail(400, "That is not a squelch mode. Use off, auto or manual.");
    return;
  }
  radioSetSquelchMode(mode);

  /* Read back rather than repeat what was asked for, and read it from where
   * it is kept rather than from the snapshot, which is only republished ten
   * times a second and would still hold the value from before this call.
   *
   * The threshold is not reported here even in manual. The knob has not been
   * read since the mode changed, so what is stored is still the old value and
   * saying it would be the same lie as setting it was. */
  SquelchMode now = radioSquelchMode(NULL);
  String said = String("squelch ") + squelchModeName(now);
  if (now == SQUELCH_MANUAL) {
    said += F(", the knob sets the threshold");
  }
  Serial.printf("[api] %s\n", said.c_str());
  sServer.send(200, "text/plain", said + "\n");
}

/*
 * POST /api/fm. The FM features the tuner has and nothing turns on by itself.
 *
 * Takes any of:
 *
 * | Argument | Range | What it is |
 * |---|---|---|
 * | `ims` | 0 or 1 | Multipath suppression. FM only |
 * | `eq` | 0 or 1 | Channel equalizer. FM only |
 * | `mno` | 0 or 1 | Refuse stereo on purpose. FM only |
 * | `cut` | dBuV, 0 for off | Roll the treble off below this. FM only |
 * | `bld` | dBuV, 0 for off | Blend towards mono below this. FM only |
 * | `hbl` | dBuV, 0 for off | Do both together below this. FM only |
 * | `anb` | per cent, 0 or 50 to 150 | AM impulse noise blanker |
 * | `fnb` | per cent, 0 or 50 to 150 | FM impulse noise blanker |
 * | `dem` | 50, 75 or 0 | FM de-emphasis, in microseconds |
 *
 * `cut`, `blend` and `hiblend` go to the chip together, and so do `amnb` and
 * `fmnb`. Whichever of a group is not given keeps the value it has, so
 * changing one does not switch off the others.
 *
 * The first six are FM ideas and are refused on the AM bands, where the chip
 * has nowhere to put them. The blankers work on both, and so does `deemph`,
 * which belongs to the country the radio is in rather than to the band it
 * happens to be on.
 *
 * `deemph` is 50 everywhere except the Americas, which use 75. Wrong either
 * way is not subtle: everything sounds dull, or everything sounds shrill.
 *
 * `ims` is multipath suppression, which the old radio badges as iMS and which
 * is what makes a station suffering reflections listenable. `eq` is the
 * channel equalizer. `mono` refuses stereo on purpose, which is not the same
 * as the automatic blend that drops to mono as a signal weakens.
 *
 * The bandwidth extension is not here. It follows the signal on its own.
 */
static String apiFmState(void) {
  RadioSnapshot now;
  if (!radioGetSnapshot(&now)) {
    return String("unknown, the radio is not running");
  }
  return String("iMS ") + (now.settings.multipathSuppression ? "on" : "off") +
         ", EQ " + (now.settings.equalizer ? "on" : "off") + ", " +
         (now.settings.forcedMono ? "mono" : "stereo") + ", weak signal cut " +
         now.settings.highCutStart + " blend " + now.settings.stereoBlendStart +
         " hiblend " + now.settings.stHiBlendStart + ", blanker am " +
         now.settings.amNoiseBlankerStart + " fm " +
         now.settings.fmNoiseBlankerStart + ", de-emphasis " +
         now.settings.deemphasisUs + " us";
}

static void handleApiFm(void) {
  sRequests++;
  if (!requireAuth(false)) {
    return;
  }

  struct {
    const char *name;
    RadioCommandKind kind;
  } features[] = {
      {"ims", RADIO_SET_MPH_SUPPRESSION},
      {"eq", RADIO_SET_EQUALIZER},
      {"mno", RADIO_SET_MONO},
  };

  /* Every argument is read and checked before any command is sent, so a
   * request with one bad argument changes nothing.
   *
   * That is not the same as the whole handler being all or nothing. Three
   * features are three commands, and if the second is refused the first has
   * already been applied, so every failure below reports the state the radio
   * actually reached rather than implying nothing happened. */
  long values[3];
  bool given[3] = {false, false, false};
  int count = 0;
  for (int i = 0; i < 3; i++) {
    if (!sServer.hasArg(features[i].name)) {
      continue;
    }
    if (!apiNumber(features[i].name, &values[i], 0, 1)) {
      return;
    }
    given[i] = true;
    count++;
  }
  /* The three weak signal start levels, in dBuV, 0 for off. All three move
   * together, because sending one without the others would mean remembering
   * the rest here. */
  bool wantWeak =
      sServer.hasArg("cut") || sServer.hasArg("bld") || sServer.hasArg("hbl");
  long weak[3] = {0, 0, 0};
  if (wantWeak) {
    /* Started from what the radio is set to, not from zero. These three go to
     * the chip together, so filling the missing ones with zero would switch
     * off whichever the caller did not mention. Asking to change one thing
     * must not quietly change two others. */
    RadioSnapshot now;
    if (!radioGetSnapshot(&now)) {
      /* Without the current values there is nothing to seed the untouched
       * members from, and sending zeros would switch them off. Refusing is
       * the only honest answer. */
      apiFail(503, "The radio is busy. Try again in a moment.");
      return;
    }
    weak[0] = now.settings.highCutStart;
    weak[1] = now.settings.stereoBlendStart;
    weak[2] = now.settings.stHiBlendStart;
    const char *names[3] = {"cut", "bld", "hbl"};
    for (int i = 0; i < 3; i++) {
      if (!sServer.hasArg(names[i])) {
        continue;
      }
      if (!apiNumber(names[i], &weak[i], 0, 60)) {
        return;
      }
      /* The reference's own menu offers 0 or 20 to 60 dBuV. Below 20 the
       * mechanism starts at a level no signal reaches, so it is switched on
       * and does nothing, which is the silent no-op the rules warn about. */
      if (weak[i] != 0 && weak[i] < 20) {
        apiFail(400, String(names[i]) +
                         " is a level in dBuV: 0 to switch it off, or 20 to "
                         "60.");
        return;
      }
    }
  }

  /* The noise blankers, which take impulse noise out rather than hiss. The
   * AM one is the lever for medium wave and shortwave. */
  bool wantBlanker = sServer.hasArg("anb") || sServer.hasArg("fnb");
  long blanker[2] = {0, 0};
  if (wantBlanker) {
    /* The same, and the same reason. */
    RadioSnapshot now;
    if (!radioGetSnapshot(&now)) {
      apiFail(503, "The radio is busy. Try again in a moment.");
      return;
    }
    blanker[0] = now.settings.amNoiseBlankerStart;
    blanker[1] = now.settings.fmNoiseBlankerStart;
    const char *names[2] = {"anb", "fnb"};
    for (int i = 0; i < 2; i++) {
      if (!sServer.hasArg(names[i])) {
        continue;
      }
      if (!apiNumber(names[i], &blanker[i], 0, 150)) {
        return;
      }
      /* A percentage, and the chip's usable range starts at 50. Anything
       * between 1 and 49 is not off and not usable either, so it is refused
       * rather than accepted into doing nothing. */
      if (blanker[i] != 0 && blanker[i] < 50) {
        apiFail(400, String(names[i]) +
                         " is a percentage: 0 to switch it off, or 50 to 150.");
        return;
      }
    }
  }

  /* De-emphasis, in microseconds. Only the two real standards and off: any
   * other number is a guess, and the chip would take it and sound wrong. */
  bool wantDeemph = sServer.hasArg("dem");
  long deemph = 0;
  if (wantDeemph) {
    if (!apiNumber("dem", &deemph, 0, 75)) {
      return;
    }
    if (deemph != 0 && deemph != 50 && deemph != 75) {
      apiFail(400,
              "dem is a time constant in microseconds: 50 here, 75 in the "
              "Americas, or 0 to switch it off.");
      return;
    }
  }

  if (count == 0 && !wantWeak && !wantBlanker && !wantDeemph) {
    apiFail(400,
            "Give ims, eq or mno as 0 or 1, cut, bld or hbl as a level in "
            "dBuV, anb or fnb as a percentage, or dem as 50, 75 or 0.");
    return;
  }

  for (int i = 0; i < 3; i++) {
    if (!given[i]) {
      continue;
    }
    RadioCommand cmd = {};
    cmd.kind = features[i].kind;
    cmd.on = values[i] != 0;
    RadioError why = RADIO_OK;
    RadioPostResult posted = radioPostAndSettle(&cmd, API_SETTLE_MS, &why);
    if (posted != RADIO_POST_DONE || why != RADIO_OK) {
      String reason;
      if (posted == RADIO_POST_BUSY) {
        reason = F("the radio is busy");
      } else if (posted == RADIO_POST_SLOW) {
        reason = F("the radio has not confirmed it");
      } else {
        reason = String(F("the radio refused it: ")) + radioErrorText(why);
      }
      /* Which one failed, and what the radio is actually set to now. An
       * earlier feature in the same request may already have been applied,
       * and a reply that only said no would be describing a radio that had
       * changed underneath it. */
      apiFail(posted == RADIO_POST_DONE ? 400 : 503,
              String(features[i].name) + ": " + reason + ". It is now " +
                  apiFmState());
      return;
    }
  }

  if (wantBlanker) {
    RadioCommand cmd = {};
    cmd.kind = RADIO_SET_NOISE_BLANKER;
    cmd.blanker[0] = (uint8_t)blanker[0];
    cmd.blanker[1] = (uint8_t)blanker[1];
    RadioError why = RADIO_OK;
    if (radioPostAndSettle(&cmd, API_SETTLE_MS, &why) != RADIO_POST_DONE ||
        why != RADIO_OK) {
      apiFail(400,
              String("noise blanker: ") +
                  (why != RADIO_OK ? radioErrorText(why) : "not confirmed") +
                  ". It is now " + apiFmState());
      return;
    }
  }

  if (wantDeemph) {
    RadioCommand cmd = {};
    cmd.kind = RADIO_SET_DEEMPHASIS;
    cmd.deemphasisUs = (uint16_t)deemph;
    RadioError why = RADIO_OK;
    if (radioPostAndSettle(&cmd, API_SETTLE_MS, &why) != RADIO_POST_DONE ||
        why != RADIO_OK) {
      apiFail(400,
              String("de-emphasis: ") +
                  (why != RADIO_OK ? radioErrorText(why) : "not confirmed") +
                  ". It is now " + apiFmState());
      return;
    }
  }

  if (wantWeak) {
    RadioCommand cmd = {};
    cmd.kind = RADIO_SET_WEAK_SIGNAL;
    cmd.weak[0] = (uint8_t)weak[0];
    cmd.weak[1] = (uint8_t)weak[1];
    cmd.weak[2] = (uint8_t)weak[2];
    RadioError why = RADIO_OK;
    if (radioPostAndSettle(&cmd, API_SETTLE_MS, &why) != RADIO_POST_DONE ||
        why != RADIO_OK) {
      apiFail(400,
              String("weak signal: ") +
                  (why != RADIO_OK ? radioErrorText(why) : "not confirmed") +
                  ". It is now " + apiFmState());
      return;
    }
  }

  String said = apiFmState();
  Serial.printf("[api] %s\n", said.c_str());
  sServer.send(200, "text/plain", said + "\n");
}

static void handleNotFound(void) {
  sRequests++;
  sServer.sendHeader("Location", "/");
  sServer.send(302, "text/plain", "");
}

/* ----------------------------------------------------------------- the API */

void webBegin(Settings *settings, uint32_t accessPin) {
  sSettings = settings;
  sAccessPin = accessPin;
  accessPinGateReset(&sGate);
  dropSession();

  /* Content-Type is collected because handleUploadData needs it to tell a
   * real upload from a raw POST body. */
  const char *keep[] = {"Cookie", "Content-Type"};
  sServer.collectHeaders(keep, 2);

  sServer.on("/", HTTP_GET, handleRoot);
  sServer.on("/radio", HTTP_GET, handleRadioPage);
  sServer.on("/network", HTTP_GET, handleNetworkPage);
  sServer.on("/system", HTTP_GET, handleSystemPage);
  sServer.on("/status.json", HTTP_GET, handleStatusJson);
  sServer.on("/auth", HTTP_POST, handleAuth);
  sServer.on("/wifi", HTTP_POST, handleWifi);
  sServer.on("/update", HTTP_POST, handleUploadDone, handleUploadData);
  sServer.on("/api/state", HTTP_GET, handleApiState);
  sServer.on("/api/tune", HTTP_POST, handleApiTune);
  sServer.on("/api/step", HTTP_POST, handleApiStep);
  sServer.on("/api/band", HTTP_POST, handleApiBand);
  sServer.on("/api/bandwidth", HTTP_POST, handleApiBandwidth);
  sServer.on("/api/step-size", HTTP_POST, handleApiStepSize);
  sServer.on("/api/volume", HTTP_POST, handleApiVolume);
  sServer.on("/api/mute", HTTP_POST, handleApiMute);
  sServer.on("/api/mode", HTTP_POST, handleApiMode);
  sServer.on("/api/cycle", HTTP_POST, handleApiCycle);
  sServer.on("/api/squelch", HTTP_POST, handleApiSquelch);
  sServer.on("/api/fm", HTTP_POST, handleApiFm);
  sServer.on("/api/settings", HTTP_GET, handleApiSettingsGet);
  sServer.on("/api/settings", HTTP_POST, handleApiSettingsPost);
  sServer.on("/api/save", HTTP_POST, handleApiSave);
  sServer.on("/api/seek", HTTP_POST, handleApiSeek);
  sServer.on("/api/beep", HTTP_POST, handleApiBeep);
  sServer.on("/api/pot", HTTP_POST, handleApiPot);
  sServer.on("/api/memory", HTTP_POST, handleApiMemoryPost);
  sServer.on("/api/memory.csv", HTTP_GET, handleApiMemoryCsv);
  sServer.on("/api/rds/raw", HTTP_GET, handleApiRdsRaw);
  sServer.on("/api/memory/import", HTTP_POST, handleApiMemoryImport);
  sServer.on("/setpin", HTTP_POST, handleSetPin);
  sServer.on("/reboot", HTTP_POST, handleReboot);
  sServer.onNotFound(handleNotFound);
  sServer.begin();
}

void webLoop(void) {
  sServer.handleClient();

  if (sRebootAfterReply) {
    sRebootAfterReply = false;
    Serial.println("[web] rebooting on request");
    Serial.flush();
    /* Down and muted first, so a reboot and an update do not end in a click.
     * This covers the firmware upload too: that sets the same flag. */
    radioHush();
    delay(200);
    ESP.restart();
  }
}

bool webHasServed(void) {
  return sRequests > 0;
}

uint32_t webRequestCount(void) {
  return sRequests;
}
