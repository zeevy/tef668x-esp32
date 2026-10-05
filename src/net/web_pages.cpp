/*
 * The seven HTML pages: the dashboard, the radio, FM & RDS, settings, the
 * network page, DX and the system page, plus the field and card builders
 * every one of them shares.
 */
#include "web_chunked.h"
#include "web_internal.h"

#include "board/board.h"
#include "core/band_plan.h"
#include "core/battery.h"
#include "core/clock.h"
#include "core/meter.h"
#include "core/palette.h"
#include "core/rds_country.h"
#include "core/settings_table.h"
#include "core/signal.h"
#include "core/squelch.h"
#include "core/theme_colour.h"
#include "core/update_check.h"
#include "core/version.h"
#include "core/web_text.h"
#include "core/wifi_signal.h"
#include "drivers/battery_adc.h"
#include "drivers/tef668x.h"
#include "net/rollback.h"
#include "net/update_check.h"
#include "net/wifi_manager.h"
#include "radio_task.h"
#include "screen_task.h"
#include "settings_task.h"
#include "system_info.h"
#include "ui/theme.h"

#include <WebServer.h>

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

static String escapeHtml(const char *raw) {
  String out;
  char piece[WEB_ESCAPE_MAX];
  for (const char *p = raw; *p != '\0'; p++) {
    (void)webHtmlEscapeChar(*p, piece);
    out += piece;
  }
  return out;
}

static void pageHead(ChunkedReply &out, const char *title,
                     const char *active = NULL);

/*
 * Pico CSS, self hosted, decides the whole light and dark story. The page
 * does not read the panel's active theme. It keeps Pico's own look, which
 * follows the browser's colour scheme, rather than the radio's.
 *
 * What is left to write here is what Pico does not ship: the tuned
 * readout, the block meter, the segmented pill Pico has no
 * component for, the two column card layout, and the toast. Every colour
 * below is one of Pico's own custom properties, never a literal, so both
 * of its themes still apply without a line of extra work.
 */
static void pageHead(ChunkedReply &out, const char *title, const char *active) {
  out +=
      F("<!doctype html><html lang=en><head>"
        /*
         * Runs synchronously, before the rest of the head even finishes
         * parsing, so there is no flash of a fallback button before this
         * removes it. `.fallback` is a button that still submits a field
         * with no script running, `data-auto`'s own no-JS answer; nobody
         * needs to see it while the script that makes it redundant is
         * actually running, which this is the one thing able to say for
         * certain rather than guessed from whether the script loaded.
         */
        "<script>document.documentElement.className='js'</script>"
        "<meta charset=utf-8>"
        "<meta name=viewport content='width=device-width,initial-scale=1'>"
        "<link rel=stylesheet href=/pico.min.css>"
        "<title>");
  out += title;
  out +=
      F("</title><style>"
        /* An amber accent, the colour of a dial's own backlight, from
           Pico's published amber ramp rather than an invented hex: a mid
           shade for a light ground, a light shade for a dark one, one
           step darker or lighter again for hover.
           Pico's own light rule is `:root:not([data-theme=dark])`, one
           step more specific than a bare `:root`; matching that
           specificity here is what lets the amber win, rather than
           losing to Pico's own blue on a browser with no explicit
           theme choice of its own. */
        ":root:not([data-theme=dark]){--pico-primary:#785800;"
        "--pico-primary-hover:#5b4200;"
        "--pico-primary-focus:rgba(151,112,0,.35);"
        "--pico-primary-inverse:#fff;"
        /* A solid button, `Go`, `Keep these settings`, `Unlock`, reads
           its fill and border off these two, never `--pico-primary`
           itself; left alone they stay Pico's own blue regardless of
           the amber set above. The hover pair the same, for the state
           a resting button does not show. */
        "--pico-primary-background:#785800;--pico-primary-border:#785800;"
        "--pico-primary-hover-background:#5b4200;"
        "--pico-primary-hover-border:#5b4200;"
        /* A dense settings panel wants a steady control size, not
           Pico's own fluid scale-up on a wide monitor: that scaling is
           what let a dropdown's chosen option outgrow its own box. */
        "--pico-spacing:.8rem;--pico-block-spacing-vertical:.8rem;"
        "--pico-block-spacing-horizontal:1rem;"
        "--pico-form-element-spacing-vertical:.4rem;"
        "--pico-form-element-spacing-horizontal:.75rem}"
        "html{font-size:100%}"
        "@media(prefers-color-scheme:dark){:root:not([data-theme]){"
        "--pico-primary:#e8ae01;--pico-primary-hover:#ffbf00;"
        "--pico-primary-focus:rgba(232,174,1,.35);"
        "--pico-primary-inverse:#161003;"
        "--pico-primary-background:#e8ae01;--pico-primary-border:#e8ae01;"
        "--pico-primary-hover-background:#ffbf00;"
        "--pico-primary-hover-border:#ffbf00}}"
        "header.top{padding-block:calc(var(--pico-spacing) * .6) 0}"
        "header.top hgroup{margin-bottom:0}"
        "header.top h1{font-size:1.3rem;letter-spacing:.04em}"
        /* Seven pages do not fit one row on a phone, so the row wraps
           rather than widening the page past the screen. */
        "header.top nav ul{flex-wrap:wrap;gap:.4rem}"
        /* Every field in a card that is not one of the few needing the
           full width, side by side on a desktop window and stacked on a
           phone screen: Pico's own .grid, not a rewrite of it. */
        "article>form,article>p,article>table{margin-top:0}"
        ".readout{font-variant-numeric:tabular-nums;font-size:2rem;"
        "font-weight:700;margin-bottom:.25rem}"
        ".readout .unit{font-size:1rem;font-weight:400;"
        "color:var(--pico-muted-color)}"
        /* A status is a shape, not a sentence: the svg carries the
           meaning, the text stays for anyone reading with a screen
           reader. */
        ".status-pill{display:inline-flex;align-items:center;gap:.35rem;"
        "padding:.3rem .6rem;border-radius:999px;"
        "background:var(--pico-card-background-color);"
        "border:1px solid var(--pico-muted-border-color);font-size:.8rem}"
        ".status-pill.on{border-color:var(--pico-primary);"
        "color:var(--pico-primary)}"
        ".status-pill svg{width:1rem;height:1rem;flex-shrink:0}"
        ".pill-row{display:flex;flex-wrap:wrap;gap:.5rem;"
        "margin-bottom:1rem}"
        /* A short row of related actions, connected into one strip: an
           input or a select carries the row, a short button sits at its
           own width beside it. Pico's own fieldset[role=group] already
           does the connecting; only the uneven share is added here. */
        "fieldset[role=group] input[type=number],"
        "fieldset[role=group] select{flex:3 1 0}"
        /* A row of buttons that keep their own width and wrap onto a
           second line rather than being forced into an equal column.
           Every button in it is `type=submit`, `formaction` only working
           on that type, so Pico's own `button[type=submit]{width:100%}`
           would stack all four full width, one to a line, without the
           override below. */
        ".btnrow{display:flex;flex-wrap:wrap;gap:.5rem;"
        "margin-bottom:var(--pico-spacing)}"
        ".btnrow button{width:auto}"
        /* Every field caption gets its own line above the control, the
           way a plain Pico label already does; the fixed minimum height
           is what keeps a row of captions that wrap differently, "On"
           beside "Multipath suppression", starting their controls on
           the same line. */
        ".grid>label>.cap{display:block;min-height:2.4em;"
        "margin-bottom:.25rem;line-height:1.2}"
        /* Pico's own .grid sizes its columns off the viewport rather
           than the card, so a card that is only half the page's own
           width still gets as many columns as the full page, squeezing
           a select until its chosen option no longer fits beside the
           arrow. */
        "main .grid{grid-template-columns:repeat(auto-fit,minmax(150px,1fr))}"
        "select{min-width:0;white-space:nowrap;overflow:hidden;"
        "text-overflow:ellipsis}"
        /* A plain fieldset of radio inputs, Pico's own look, laid out in
           a row: Band, Squelch and the theme each pick one of a handful
           of names, which reads at a glance as a row of radios and is
           hidden inside a select. Pico has no pill-shaped toggle of its
           own, so this does not invent one. */
        ".inline-radios{border:0;padding:0;margin:0 0 var(--pico-spacing)}"
        ".inline-radios .options{display:flex;flex-wrap:wrap;"
        "gap:.4rem 1.2rem}"
        ".inline-radios label{display:flex;align-items:center;gap:.4rem;"
        "margin:0;width:auto}"
        /* The signal meter is blocks, not a gradient, each
           a fixed pixel width rather than stretched to fill the card.
           .meter-block shrinks to that same width, so the scale below
           the bar marks its own ends rather than the card's. */
        ".meter-block{width:fit-content;max-width:100%}"
        ".meter{display:flex;gap:2px;align-items:flex-end;height:1.3rem;"
        "margin:.5rem 0}"
        ".meter i{display:block;width:11px;flex:0 0 auto;height:100%;"
        "background:var(--pico-muted-border-color);border-radius:2px;"
        "font-style:normal}"
        ".meter i.on{background:var(--pico-ins-color)}"
        ".meter-label{display:flex;justify-content:space-between;"
        "gap:1rem;width:100%;font-size:.78rem;"
        "color:var(--pico-muted-color)}"
        /* Every settings field still applies the moment it changes, with
           htmx; this is only what still applies a card's fields with no
           script running, so it is small rather than the thing the eye
           lands on. */
        ".fallback{padding:.3rem .8rem;font-size:.8rem}"
        ".js .fallback{display:none}"
        /* A fixed two columns, not auto-fit: with as few as three cards
           on a page, auto-fit leaves the odd one alone in a row with
           empty space beside it. Two columns plus a deliberate
           .span-all on whichever card should own the full row reads as
           chosen, not as a gap. */
        ".page-grid{display:grid;grid-template-columns:1fr 1fr;"
        "gap:var(--pico-block-spacing-vertical) var(--pico-spacing);"
        "align-items:start;margin-top:var(--pico-block-spacing-vertical)}"
        ".page-grid>article{margin:0}"
        /* Two cards kept in one grid cell, stacked, so a card the width
           of its neighbour does not have to be the same height too. */
        ".stack>article{margin-bottom:var(--pico-block-spacing-vertical)}"
        ".span-all{grid-column:1/-1}"
        "@media(max-width:700px){.page-grid{grid-template-columns:1fr}}"
        /* An eyebrow over a rule, so a page split into Seek, Sound,
           Display and Advanced reads as four subjects rather than one
           long wall of cards. */
        ".section-head{display:flex;align-items:baseline;gap:.75rem;"
        "margin:1.6rem 0 .6rem}"
        ".section-head:first-of-type{margin-top:0}"
        ".section-head h2{font-size:.78rem;text-transform:uppercase;"
        "letter-spacing:.09em;color:var(--pico-muted-color);margin:0;"
        "white-space:nowrap}"
        ".section-head hr{flex:1;margin:0}"
        /* Now Playing is the one thing the Radio page is for, so it
           gets a stripe of the accent rather than sitting flush with
           every other card at the same weight. */
        ".hero{border-top:3px solid var(--pico-primary)}"
        ".ok{color:var(--pico-ins-color)}.warn{color:var(--pico-del-color)}"
        /* The reply to a form post. Fixed to the bottom of the screen
           rather than sitting at the top of the page, because the forms
           that produce it are most of a screen further down and the
           answer was landing where nobody was looking. */
        "#toast{position:fixed;left:1rem;right:1rem;bottom:1rem;"
        "z-index:50;max-width:40rem;margin-inline:auto;"
        "padding:.75rem 2.5rem .75rem 1rem;"
        "background:var(--pico-card-background-color);"
        "border:1px solid var(--pico-muted-border-color);"
        "border-radius:var(--pico-border-radius);"
        "box-shadow:0 8px 28px rgba(0,0,0,.35);font-size:.9rem;"
        "transform:translateY(.75rem);opacity:0;pointer-events:none;"
        "transition:transform .2s ease,opacity .2s ease}"
        "#toast.show{transform:translateY(0);opacity:1;"
        "pointer-events:auto}"
        "#toast.bad{color:var(--pico-del-color)}"
        "#toast.good{color:var(--pico-ins-color)}"
        "#toastx{position:absolute;top:.4rem;right:.6rem;border:0;"
        "background:none;color:var(--pico-muted-color);font-size:22px;"
        "line-height:1;cursor:pointer;padding:2px 6px}"
        "@media(prefers-reduced-motion:reduce){*{transition:none!important}}"
        "</style></head><body><header class=top><div class=container>"
        "<hgroup><h1>TEF668X</h1><p>");
  out += F(BOARD_NAME_DISPLAY " &middot; V" FIRMWARE_VERSION);
  out += F("</p></hgroup>");
  if (active != NULL) {
    out += F("<nav><ul>");
    static const char *const kPages[][2] = {
        {"/", "Home"},           {"/radio", "Radio"},
        {"/fm", "FM &amp; RDS"}, {"/settings", "Settings"},
        {"/network", "Network"}, {"/dx", "DX"},
        {"/system", "System"}};
    for (size_t i = 0; i < sizeof(kPages) / sizeof(kPages[0]); i++) {
      out += F("<li><a href='");
      out += kPages[i][0];
      out += F("' role=button class=outline");
      if (strcmp(active, kPages[i][0]) == 0) {
        out += F(" aria-current=page");
      }
      out += F(">");
      out += kPages[i][1];
      out += F("</a></li>");
    }
    out += F("</ul></nav>");
  }
  out += F("</div></header><main class=container>");
}

static const char *pageTail(bool scripted = false) {
  if (!scripted) {
    /* Only the Radio, FM & RDS and Settings pages, and the Network page
     * when signed in, have controls that need it. The other pages have
     * plain forms that call none of it, so they are sent without it. */
    return "</main></body></html>";
  }
  /*
   * htmx does the posting and the swap into the toast: every field that
   * wants to apply itself carries its own `hx-post` to the endpoint its
   * form already uses and `hx-trigger=change`, which is what still reads
   * as one submission of the whole enclosing form, `FormData`'s own rule
   * for an element inside one. What is left to write by hand is what
   * htmx has no opinion about: showing the toast itself, `data-reload`
   * for the one field that changes what the rest of the card offers, and
   * `data-seek` polling `/api/state` until a seek stops, since a seek
   * answers as soon as it starts rather than once it has stopped.
   *
   * A refused or lost post is written into the toast by hand too, because
   * htmx swaps in only a 2xx reply. Its form then goes back to what the
   * radio last took, so a choice that was not saved does not stay on the
   * page looking saved: a good post makes the form's values its new
   * defaults, and a failed one resets the form to them.
   */
  return "<script src=/htmx.min.js defer></script><script>"
         "var g=function(i){return document.getElementById(i)},skg=false,"
         "toast=function(bad){var m=g('toast');if(!m)return;"
         "m.className=bad?'bad show':'good show'},"
         "keep=function(f,ok){if(!f)return;if(!ok){f.reset();return}"
         "for(var i=0;i<f.elements.length;i++){var x=f.elements[i];"
         "if(x.type=='checkbox'||x.type=='radio')x.defaultChecked=x.checked;"
         "else if(x.options)for(var j=0;j<x.options.length;j++)"
         "x.options[j].defaultSelected=x.options[j].selected;"
         "else if('defaultValue' in x)x.defaultValue=x.value}};"
         /* Returns the fetch itself, so seekFollow can wait for this
            round's answer before deciding whether to poll again, rather
            than reading skg as it stood before this call went out. */
         /* Only a page that shows the tuned station asks for it. */
         "refresh=function(){if(!g('npf')&&!g('khz'))"
         "return Promise.resolve();"
         "return fetch('/api/state').then(function(r){"
         "return r.json()}).then(function(d){var t=d.tun||{};"
         "skg=!!t.skg;"
         "var e=function(i,v){var n=g(i);if(n&&v!==undefined)"
         "n.textContent=v};"
         "e('npf',t.f);e('npu',t.unt);e('npb',t.bnd);"
         "var k=g('khz');if(k&&document.activeElement!==k)k.value=t.khz})"
         ".catch(function(){})},"
         "seekFollow=function(){var n=0,t=setInterval(function(){"
         "refresh().then(function(){if(!skg||++n>200)clearInterval(t)})}"
         ",250)};"
         "document.body.addEventListener('htmx:afterRequest',function(e){"
         "var d=e.detail,elt=d.elt,m=g('toastMsg');"
         "if(!d.successful&&m)m.textContent="
         "(d.xhr&&d.xhr.responseText)||'No answer from the radio.';"
         "keep(elt.form,d.successful);"
         "toast(!d.successful);"
         "if(d.successful&&elt.closest('[data-reload]')){"
         "location.reload();return}"
         "refresh();"
         "if(d.successful&&elt.hasAttribute('data-seek'))seekFollow()});"
         "var tx=g('toastx');if(tx)tx.addEventListener('click',function(){"
         "g('toast').classList.remove('show')});"
         "refresh();"
         "</script></main></body></html>";
}

/*
 * `hx-post` to the same endpoint a field's own form already posts to, and
 * `hx-trigger=change`: htmx reads the whole enclosing form for an element
 * that has no form of its own, `FormData`'s own rule, so this is one
 * submission of the whole card the moment the field changes, the same
 * thing `data-auto`'s old `form.requestSubmit()` did. `hx-target` and
 * `hx-swap` say where the plain text reply lands: the toast, replacing
 * what was there.
 */
static String autoAttrs(const char *endpoint) {
  String out = F("hx-post='");
  out += endpoint;
  out += F("' hx-trigger=change hx-target=#toastMsg hx-swap=innerHTML");
  return out;
}

/*
 * The field helpers all return one `<label>` wrapping its input: the label
 * needs no `for` and the input no `id`, so nothing here has to invent one.
 * `attrs` carries whatever the caller wants added to the input itself,
 * usually nothing, sometimes `autoAttrs`'s own `hx-post`, which submits
 * the enclosing form the moment the value changes.
 *
 * The caption is wrapped in its own `<span class=cap>`, a fixed minimum
 * height set in `pageHead`'s stylesheet, so a caption that wraps to two
 * lines and one that fits on one still start their control on the same
 * line, side by side in the same `.grid` row.
 */
static String formSelect(const char *name, const char *label,
                         const char *const *options, const long *values,
                         int count, long current, const String &attrs) {
  String out = F("<label><span class=cap>");
  out += label;
  out += F("</span><select name=");
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
  out += F("</select></label>");
  return out;
}

/*
 * A small set of mutually exclusive choices, as a plain row of radio
 * inputs rather than the one row a select hides them behind. Pico's own
 * look for a radio, only laid out sideways by `.inline-radios` in
 * `pageHead`; there is no pill of this project's own invention here.
 * Only worth it for a handful of options a person wants to compare at a
 * glance, which is what a theme is; a list with dozens of rows still
 * wants a select.
 */
static String formRadioGroup(const char *name, const char *label,
                             const char *const *options, const long *values,
                             int count, long current, const String &attrs) {
  String out = F("<fieldset class=inline-radios>");
  if (label[0] != '\0') {
    out += F("<legend>");
    out += label;
    out += F("</legend>");
  }
  out += F("<div class=options>");
  for (int i = 0; i < count; i++) {
    String id = String(name) + String(values[i]);
    out += F("<label><input type=radio id='");
    out += id;
    out += F("' name=");
    out += name;
    out += F(" value=");
    out += String(values[i]);
    if (values[i] == current) {
      out += F(" checked");
    }
    out += F(" ");
    out += attrs;
    out += F("> ");
    out += options[i];
    out += F("</label>");
  }
  out += F("</div></fieldset>");
  return out;
}

/*
 * The same row of radios, but by name rather than by index: the option's
 * own text is what a plain form submits, which is what `/api/band`
 * expects. A radio needs no trailing Set button the way a select or a
 * range does, since a tap both picks the value and is the submit.
 */
static String formRadioGroupByName(const char *name, const char *label,
                                   const char *const *options, int count,
                                   const char *current, const String &attrs) {
  String out = F("<fieldset class=inline-radios>");
  if (label[0] != '\0') {
    out += F("<legend>");
    out += label;
    out += F("</legend>");
  }
  out += F("<div class=options>");
  for (int i = 0; i < count; i++) {
    String id = String(name) + String(i);
    out += F("<label><input type=radio id='");
    out += id;
    out += F("' name=");
    out += name;
    out += F(" value='");
    out += options[i];
    out += F("'");
    if (strcmp(options[i], current) == 0) {
      out += F(" checked");
    }
    out += F(" ");
    out += attrs;
    out += F("> ");
    out += options[i];
    out += F("</label>");
  }
  out += F("</div></fieldset>");
  return out;
}

static String formNumber(const char *name, const char *label, long low,
                         long high, long current, const String &attrs) {
  String out = F("<label><span class=cap>");
  out += label;
  out += F("</span><input type=number name=");
  out += name;
  out += F(" min=");
  out += String(low);
  out += F(" max=");
  out += String(high);
  out += F(" value=");
  out += String(current);
  out += F(" ");
  out += attrs;
  out += F("></label>");
  return out;
}

/* A number field for one stored setting, its range and its value from the
 * settings table, the one the API checks against, so the page cannot offer
 * a value the save refuses. */
static String formSetting(const Settings *st, const char *key,
                          const char *label, const String &attrs) {
  const SettingRow *row = settingsTableFind(key);
  if (row == NULL) {
    return String();
  }
  return formNumber(key, label, row->low, row->high, settingsTableGet(st, row),
                    attrs);
}

/*
 * A short text field.
 *
 * Only the UTC offset uses this. It is written the way everybody writes an
 * offset, "+05:30", rather than as 330 minutes, because a person setting up a
 * radio knows their offset in hours and minutes and nobody knows it in
 * minutes.
 *
 * No `pattern` attribute. htmx reads the value straight off the element,
 * which skips the browser's own constraint checking entirely, so a pattern
 * here would look like validation and do nothing. `clockParseOffset` on the
 * radio is the only check there is, and it answers with a message saying
 * what the field should look like.
 */
static String formText(const char *name, const char *label, const char *value,
                       const char *placeholder, const String &attrs) {
  String out = F("<label><span class=cap>");
  out += label;
  out += F("</span><input type=text name=");
  out += name;
  out += F(" value='");
  out += value;
  out += F("' placeholder='");
  out += placeholder;
  out += F("' ");
  out += attrs;
  out += F("></label>");
  return out;
}

/*
 * A colour wheel, `<input type=color>`, which every browser already gives a
 * picker for. No `pattern` and no manual hex entry: the element only ever
 * hands back six hex digits itself, so there is nothing here for a person to
 * mistype.
 */
static String formColour(const char *name, const char *label, const char *value,
                         const String &attrs) {
  String out = F("<label>");
  out += label;
  out += F("<input type=color name=");
  out += name;
  out += F(" value='");
  out += value;
  out += F("' ");
  out += attrs;
  out += F("></label>");
  return out;
}

static String cardOpen(const char *title, const char *cardClass = "") {
  String out = F("<article");
  if (cardClass[0] != '\0') {
    /* Quoted: an unquoted attribute ends at the first space, so a two
       word class such as "hero span-all" silently lost span-all, and
       with it the hero card's claim to both grid columns. */
    out += F(" class='");
    out += cardClass;
    out += F("'");
  }
  out += F("><header>");
  out += title;
  out += F("</header>");
  return out;
}

static const char *cardClose(void) {
  return "</article>";
}

/*
 * One form, one endpoint. `endpoint` is empty for a form whose buttons
 * carry their own `formaction`, such as the dial's mix of `/api/tune`,
 * `/api/step` and `/api/seek`. It posts and reloads on its own with no
 * script at all; `hx-post` is what htmx intercepts that same submit with
 * instead. A card can hold more than one of these where its fields do not
 * all answer to the same endpoint, Reception's own `/api/fm` fields beside
 * its `/api/bandwidth` and `/api/step-size` rows being why this is a form
 * rather than something the card itself provides.
 *
 * `attrs` is for the one form that still wants the reload htmx otherwise
 * skips: changing the band changes which fields Reception shows,
 * `data-reload` says so.
 */
static String formOpen(const char *endpoint = "", const char *attrs = "") {
  String out = F("<form method=post");
  if (endpoint[0] != '\0') {
    out += F(" action='");
    out += endpoint;
    out += F("' hx-post='");
    out += endpoint;
    out += F("'");
  }
  out += F(" hx-target=#toastMsg hx-swap=innerHTML ");
  out += attrs;
  out += F(">");
  return out;
}

static const char *formClose(void) {
  return "</form>";
}

/* The reply to a form post, and the placeholder htmx's own swap fills in. */
static String pageToast(void) {
  return F(
      "<p id=toast><span id=toastMsg></span>"
      "<button id=toastx type=button aria-label=Close>"
      "&times;</button></p>");
}

/* Every page below starts the same way: the toast, then nothing more if
 * the radio task is not up to read from. */
static bool liveOrExcuse(ChunkedReply &out, bool live) {
  out += pageToast();
  if (!live) {
    out +=
        F("<p><mark>The radio task is not running, so there is nothing "
          "to set.</mark></p>");
  }
  return live;
}

/*
 * How many blocks the web page's own signal meter is drawn from.
 *
 * The panel's own block count comes from a pixel width on a screen this
 * page does not have; a browser window has no fixed width to divide, so
 * this picks one instead, same as `page-grid` already picks two columns
 * rather than asking the viewport how many it would like.
 */
#define WEB_METER_BLOCKS 24

/*
 * The signal meter as blocks, using the same
 * `meterSegmentsLit` the panel bar does so a reading rounds the same way
 * in both places. No peak mark: that needs a state that survives between
 * requests, which a page built fresh on every load does not keep.
 */

/* The level as a person is shown it, with the band's offset.
 * The bar is drawn from it too, as on the panel. */
static int16_t shownLevel(const RadioSnapshot &now) {
  return signalShownTenths(now.quality.levelDbuVTenths,
                           screenTaskLevelOffsetDb(now.settings.band));
}

static String signalMeter(const RadioSnapshot &now) {
  if (!now.qualityValid) {
    return String();
  }
  bool onFm = bandModulation(now.settings.band) == MODULATION_FM;
  uint8_t fullDbuV = onFm ? SIGNAL_FULL_FM_DBUV : SIGNAL_FULL_AM_DBUV;
  uint8_t percent = signalBarPercent(shownLevel(now), fullDbuV);
  uint8_t lit = meterSegmentsLit(percent, WEB_METER_BLOCKS);

  String out;
  out.reserve(400);
  out += F("<div class=meter-block><div class=meter aria-hidden=true>");
  for (int i = 0; i < WEB_METER_BLOCKS; i++) {
    out += i < lit ? F("<i class=on></i>") : F("<i></i>");
  }
  char level[12];
  signalFormatLevel(shownLevel(now), level, sizeof(level));
  out += F("</div><div class=meter-label><span>0</span><span>Signal, ");
  out += level;
  out += F(" dBuV</span><span>");
  out += String(fullDbuV);
  out += F("</span></div></div>");
  return out;
}

/*
 * What the RDS decoder knows, in this order: the identifier, the programme
 * type, the traffic flags, the group and block counts, the alternative
 * frequency count and the radio text. A dash for a field that has not
 * arrived twice the same way yet, never a blank, so a station with nothing
 * to say and one this radio has not heard from yet do not look alike.
 */
static String rdsCard(const RadioSnapshot &now, const Settings *st) {
  String out = cardOpen("RDS");
  out.reserve(700);
  if (!st->rdsEnabled) {
    out +=
        F("<p>The RDS decoder is off. Switch it on under "
          "<a href='/fm'>FM &amp; RDS</a>.</p>");
    out += cardClose();
    return out;
  }
  const RdsInfo *r = &now.rds;
  out += F("<table><tr><td>Identifier</td><td>");
  if (r->hasPi) {
    char pi[8];
    snprintf(pi, sizeof(pi), "%04X", r->pi);
    out += pi;
  } else if (r->piZero) {
    out += F("0000, the station sends none");
  } else {
    out += F("&ndash;");
  }
  out += F("</td></tr><tr><td>Programme type</td><td>");
  if (r->hasPty) {
    out += String((unsigned)r->pty);
    out += F(" ");
    out += rdsPtyNameIn(r->pty, (RdsRegion)st->rdsRegion);
  } else {
    out += F("&ndash;");
  }
  out += F("</td></tr><tr><td>Traffic</td><td>");
  if (r->hasFlags) {
    if (r->tp) {
      out += F("TP ");
    }
    if (r->ta) {
      out += F("TA ");
    }
    out += r->speech ? F("speech") : F("music");
  } else {
    out += F("&ndash;");
  }
  out += F("</td></tr><tr><td>Groups seen</td><td>");
  out += String(r->groupsSeen);
  out += F("</td></tr><tr><td>Groups used</td><td>");
  out += String(r->groupsUsed);
  out += F("</td></tr><tr><td>Blocks fixed</td><td>");
  out += String(r->blocksCorrected);
  out += F("</td></tr><tr><td>Blocks bad</td><td>");
  out += String(r->blocksBad);
  out += F("</td></tr><tr><td>Alternative frequencies</td><td>");
  out += r->afCount == 0 ? F("none") : String((unsigned)r->afCount);
  out += F("</td></tr></table>");
  if (r->hasRt) {
    out += F("<p><small>Radio text: &ldquo;");
    out += escapeHtml(r->rt);
    out += F("&rdquo;</small></p>");
  } else if (!r->synchronised) {
    out +=
        F("<p><small>No RDS heard on this station "
          "yet.</small></p>");
  }
  out += cardClose();
  return out;
}

/* Signal and what the volume AGC is doing with it: a working AGC and one
 * that decided to do nothing look the same from outside, so this says
 * which. */
static String receptionLevelsCard(const RadioSnapshot &now) {
  String out = cardOpen("Reception levels");
  out.reserve(300);
  out += F("<table><tr><td>Signal</td><td>");
  if (now.qualityValid) {
    char level[12];
    signalFormatLevel(shownLevel(now), level, sizeof(level));
    out += level;
    out += F(" dBuV");
  } else {
    out += F("&ndash;");
  }
  out += F("</td></tr><tr><td>Volume AGC</td><td>");
  if (now.agcOn) {
    out += String((int)now.agcGainDb);
    out += F(" dB, ");
    out += now.agcSettled ? F("settled") : F("still settling");
  } else {
    out += F("off");
  }
  out += F("</td></tr></table>");
  out += cardClose();
  return out;
}

/*
 * Battery, Wi-Fi and the panel light: not what the radio is tuned to, but
 * whether it is well.
 *
 * The battery figure is the one reading from start up: GPIO
 * 13 is ADC2 and Wi-Fi owns ADC2 for as long as it runs, so this is not a
 * live number and says so rather than implying one.
 */
static String radioHealthCard(void) {
  String out = cardOpen("Radio health");
  out.reserve(400);
  out += F("<table><tr><td>Battery</td><td>");
  uint16_t bootMv = 0;
  if (batteryAdcFitted() && batteryAdcAtBoot(&bootMv)) {
    Battery b;
    batteryReset(&b);
    batteryFeed(&b, bootMv, true);
    char text[BATTERY_TEXT_LEN];
    if (batteryFormat(&b, BATTERY_SHOW_PERCENT, text, sizeof(text))) {
      out += text;
      out += F(", read at start up");
    } else {
      out += F("&ndash;");
    }
  } else {
    out += F("no battery fitted");
  }
  out += F("</td></tr><tr><td>Wi-Fi signal</td><td>");
  int8_t rssi = 0;
  if (wifiRssiDbm(&rssi)) {
    out += String((int)rssi);
    out += F(" dBm, ");
    out += String((unsigned)wifiSignalBars(rssi));
    out += F(" bars");
  } else {
    out += F("&ndash;");
  }
  out += F("</td></tr><tr><td>Panel light</td><td>");
  uint8_t lit = 0;
  bool dimmed = screenTaskBacklightState(&lit);
  out += String((unsigned)lit);
  out += F("%, ");
  out += dimmed ? F("dimmed") : F("awake");
  out += F("</td></tr></table>");
  out += cardClose();
  return out;
}

/*
 * The Radio page's second half: what the tuner and the radio itself are
 * doing, read only. RDS gets the left column to itself; reception levels
 * and radio health share the right, since they read closer together
 * than either reads against RDS.
 */
static void radioTelemetryCards(ChunkedReply &out, const RadioSnapshot &now,
                                const Settings *st) {
  out += F("<div class=page-grid>");
  out += rdsCard(now, st);
  out += F("<div class=stack>");
  out += receptionLevelsCard(now);
  out += radioHealthCard();
  out += F("</div></div>");
}

/*
 * The Radio page's first half: tuning, live, the one thing a page called
 * Radio is for.
 */
static void radioDialForms(ChunkedReply &out, const RadioSnapshot &now,
                           const Settings *st) {
  char freqText[16];
  if (!bandFormatFrequency(now.settings.band, now.settings.freqKHz, freqText,
                           sizeof(freqText))) {
    freqText[0] = '\0';
  }

  /* "Tuning", not "Radio": Home already carries that header for its own,
   * read only glance at the same reading, and the two sitting one click
   * apart with the same title read as the same card twice. */
  out += cardOpen("Tuning", "hero span-all");
  out += F("<div class=readout><span id=npf>");
  out += freqText;
  out += F("</span> <span class=unit id=npu>");
  out += bandFrequencyUnit(now.settings.band);
  out += F("</span> <span id=npb>");
  out += bandName(now.settings.band);
  out += F("</span></div>");
  /* A status is a shape, not a sentence: the svg carries the meaning, the
   * text beside it is what keeps a screen reader and a person skimming
   * the source both able to tell what it means. */
  out += F("<div class=pill-row>");
  if (now.qualityValid) {
    char level[12];
    signalFormatLevel(shownLevel(now), level, sizeof(level));
    out += F("<span class=status-pill><small>");
    out += level;
    out += F(" dBuV</small></span>");
    if (now.quality.stereo && !now.settings.forcedMono) {
      out +=
          F("<span class='status-pill on' title='Receiving in stereo'>"
            "<svg viewBox='0 0 24 24' fill=none stroke=currentColor "
            "stroke-width=2><circle cx=8 cy=12 r=4/>"
            "<circle cx=16 cy=12 r=4/></svg>Stereo</span>");
    }
  }
  out += now.squelchOpen
             ? F("<span class='status-pill on' title='Squelch is open, "
                 "audio is passing'><svg viewBox='0 0 24 24' fill=none "
                 "stroke=currentColor stroke-width=2><path d='M4 12h4l3-7 "
                 "4 14 3-7h2'/></svg>Squelch open</span>")
             : F("<span class=status-pill title='Squelch is shut, audio "
                 "is muted'>Squelch shut</span>");
  if (now.settings.muted) {
    out +=
        F("<span class='status-pill on' title='Muted'>"
          "<svg viewBox='0 0 24 24' fill=none stroke=currentColor "
          "stroke-width=2><path d='M4 9v6h4l5 5V4L8 9H4z'/>"
          "<line x1=17 y1=9 x2=22 y2=15/><line x1=22 y1=9 x2=17 "
          "y2=15/></svg>Muted</span>");
  }
  out += F("</div>");
  out += signalMeter(now);

  /* One form: the tune box and Go default to /api/tune, the step and seek
   * buttons override it with their own formaction and a matching
   * `hx-post`, htmx's own way of reading that instead of the form's.
   * Each button's own name and value, rather than a hidden field, is what
   * says which direction. Seek answers as soon as it starts, so its two
   * buttons carry data-seek, which tells the page's own script to keep
   * polling until it stops. */
  out += formOpen("/api/tune");
  /*
   * Only Go and Keep these settings are the solid, primary colour: a row
   * where every button is that colour has no way to say which one
   * actually matters. Step, seek and mute all move the dial without
   * settling on anything, so they stay the quiet, outlined kind.
   */
  out +=
      F("<fieldset role=group>"
        "<button type=submit formaction='/api/step' hx-post='/api/step' "
        "name=stp value=-1 title='Down one step' "
        "class=secondary>&#8249;</button>"
        "<input type=number name=khz value=");
  out += String(now.settings.freqKHz);
  out +=
      F("><button type=submit formaction='/api/step' hx-post='/api/step' "
        "name=stp value=1 title='Up one step' "
        "class=secondary>&#8250;</button>"
        "<button type=submit>Go</button></fieldset>");
  /*
   * A plain wrapping row rather than `role=group`: the connected group's
   * own rule gives every child an equal share of the width, which four
   * buttons this different in length, "Mute" against "Keep these
   * settings", turns into a row of boxes too narrow for their own label.
   * A button sized to its own text, free to drop to a second line, is
   * what a phone screen needs
   * here instead.
   */
  out +=
      F("<div class=btnrow>"
        "<button type=submit formaction='/api/seek' hx-post='/api/seek' "
        "name=dir value=down data-seek class=secondary>"
        "&#8249;&#8249; Seek</button>"
        "<button type=submit formaction='/api/seek' hx-post='/api/seek' "
        "name=dir value=up data-seek class=secondary>"
        "Seek &#8250;&#8250;</button>"
        "<button type=submit formaction='/api/cycle' hx-post='/api/cycle' "
        "name=wht value=mute class=secondary>Mute</button>"
        "<button type=submit formaction='/api/save' "
        "hx-post='/api/save'>Keep these settings</button></div>");
  out += formClose();
  out +=
      F("<p><small>Keep these settings stores the station and "
        "everything below, so the radio comes up this way. The volume "
        "follows the knob, except in manual squelch where the knob is "
        "the squelch.</small></p>");

  /* Three instant fields, each its own tiny form so a change can submit
   * itself. The volume's Set button is not decoration, it is what still
   * sets the slider with no script running at all. Band and Squelch are
   * radio rows, which need no button. */
  out += formOpen("/api/band", "data-reload");
  {
    static const char *bandNames[BAND_COUNT];
    for (int b = 0; b < BAND_COUNT; b++) {
      bandNames[b] = bandName((BandId)b);
    }
    out += formRadioGroupByName("bnd", "Band", bandNames, BAND_COUNT,
                                bandName(now.settings.band),
                                autoAttrs("/api/band"));
  }
  out += formClose();

  /* `volout` is a local echo of the slider's own drag, not the state
   * refresh: a range input only fires `change` on release, and the
   * number beside it needs to move with the thumb while it is still
   * moving. A native `<output>` bound to the slider by its form and
   * `oninput` is what still says so with no script running, since it
   * reads the slider's own value with no code of this page's own. */
  out += formOpen("/api/volume");
  out += F("<label>Volume, dB <output name=volout>");
  out += String(now.settings.volumeDb);
  out +=
      F("</output></label><fieldset role=group style='align-items:end'>"
        "<input type=range name=db min=");
  out += String(RADIO_VOLUME_MIN);
  out += F(" max=0 value=");
  out += String(now.settings.volumeDb);
  out += F(" ");
  out += autoAttrs("/api/volume");
  out += F(
      " oninput='this.form.volout.value=this.value'>"
      "<button type=submit class='secondary fallback'>Set</button></fieldset>");
  out += formClose();

  out += formOpen("/api/squelch");
  {
    static const char *squelchNames[SQUELCH_MODE_COUNT];
    for (int m = 0; m < SQUELCH_MODE_COUNT; m++) {
      squelchNames[m] = squelchModeName((SquelchMode)m);
    }
    out += formRadioGroupByName(
        "mod", "Squelch", squelchNames, SQUELCH_MODE_COUNT,
        squelchModeName(now.squelchMode), autoAttrs("/api/squelch"));
  }
  out += formClose();
  out += cardClose();
}

/*
 * The FM & RDS page: reception quality and the RDS decoder's own
 * settings, kept apart from Radio's dial and telemetry and from the
 * broader Settings page, because both cards here are about what the
 * tuner does with a signal rather than how the radio behaves.
 */
static void fmRdsForms(ChunkedReply &out, const RadioSnapshot &now,
                       const Settings *st) {
  bool onFm = bandModulation(now.settings.band) == MODULATION_FM;
  static const char *offOn[] = {"Off", "On"};
  static const long zeroOne[] = {0, 1};
  String kAuto;
  out += F("<div class=page-grid>");

  /* ----------------------------------------------------------- reception */
  out += cardOpen("Reception");
  out += formOpen("/api/fm");
  kAuto = autoAttrs("/api/fm");
  out += F("<div class=grid>");
  static const char *deemphNames[] = {"50 us", "75 us", "Off"};
  static const long deemphValues[] = {50, 75, 0};
  out += formSelect("dem", "De-emphasis", deemphNames, deemphValues, 3,
                    now.settings.deemphasisUs, kAuto);
  if (onFm) {
    out += formSelect("ims", "Multipath suppression", offOn, zeroOne, 2,
                      now.settings.multipathSuppression ? 1 : 0, kAuto);
    out += formSelect("eq", "Channel equalizer", offOn, zeroOne, 2,
                      now.settings.equalizer ? 1 : 0, kAuto);
    static const char *stereoMono[] = {"Stereo", "Mono"};
    out += formSelect("mno", "Stereo", stereoMono, zeroOne, 2,
                      now.settings.forcedMono ? 1 : 0, kAuto);
  }
  out += F("</div>");
  /* The expert half, folded away in the same form: nobody sets a stereo
   * blend start level by accident, and leaving it open made the page look
   * harder than it is. */
  out +=
      F("<details><summary>Weak signal and noise blankers</summary>"
        "<div class=grid>");
  if (onFm) {
    out += formNumber("cut", "High cut from, dBuV", 0, 60,
                      now.settings.highCutStart, kAuto);
    out += formNumber("bld", "Stereo blend from, dBuV", 0, 60,
                      now.settings.stereoBlendStart, kAuto);
    out += formNumber("hbl", "Both from, dBuV", 0, 60,
                      now.settings.stHiBlendStart, kAuto);
  }
  out += formNumber("fnb", "FM noise blanker, per cent", 0, 150,
                    now.settings.fmNoiseBlankerStart, kAuto);
  out += formNumber("anb", "AM noise blanker, per cent", 0, 150,
                    now.settings.amNoiseBlankerStart, kAuto);
  out += formNumber("ahc", "AM high cut from, MW and SW, dBuV", 0, 60,
                    now.settings.amHighCutStart, kAuto);
  out += formNumber("lhc", "AM high cut from, LW, dBuV", 0, 60,
                    now.settings.lwHighCutStart, kAuto);
  out += formNumber("asm", "AM soft mute from, MW and SW, dBuV", 0, 50,
                    now.settings.amSoftMuteStart, kAuto);
  out += formNumber("lsm", "AM soft mute from, LW, dBuV", 0, 50,
                    now.settings.lwSoftMuteStart, kAuto);
  out +=
      F("</div><p><small>The levels are 0 to switch off, or 20 to 60. "
        "The AM soft mute levels are 0 to 50 and have no off. "
        "The blankers are 0, or 50 to 150.</small></p></details>");
  out += F("<button type=submit class='secondary fallback'>Apply</button>");
  out += formClose();
  if (!onFm) {
    out += formOpen("/api/bandwidth");
    static const char *widthNames[] = {"3 kHz", "4 kHz", "6 kHz", "8 kHz"};
    static const long widthValues[] = {3, 4, 6, 8};
    out += F("<fieldset role=group style='align-items:end'>");
    out += formSelect("khz", "Bandwidth", widthNames, widthValues, 4,
                      now.settings.bandwidthKHz, autoAttrs("/api/bandwidth"));
    out +=
        F("<button type=submit class='secondary "
          "fallback'>Set</button></fieldset>");
    out += formClose();
  }
  /*
   * The tuning step choices are not written here: FM allows three steps
   * and an AM band one of two, and which pair depends on which AM band
   * this is, so the list is read off bandStepCount/bandStepAt for the band
   * that is actually tuned. A list kept here as well would be a second
   * copy of the driver's own answer, free to drift from it the day a step
   * is added on one side and not the other.
   */
  {
    BandPlanConfig plan;
    if (radioTaskPlan(&plan)) {
      size_t stepCount = bandStepCount(now.settings.band, &plan);
      if (stepCount > BAND_STEP_MAX_COUNT) {
        stepCount = BAND_STEP_MAX_COUNT; /* Never happens on this radio;
                                             caught rather than overrunning. */
      }
      static char stepNames[BAND_STEP_MAX_COUNT][16];
      static const char *stepNamePtrs[BAND_STEP_MAX_COUNT];
      static long stepValues[BAND_STEP_MAX_COUNT];
      for (size_t i = 0; i < stepCount; i++) {
        uint16_t khz = bandStepAt(now.settings.band, &plan, i);
        snprintf(stepNames[i], sizeof(stepNames[i]), "%u kHz", (unsigned)khz);
        stepNamePtrs[i] = stepNames[i];
        stepValues[i] = khz;
      }
      out += formOpen("/api/step-size");
      out += F("<fieldset role=group style='align-items:end'>");
      out += formSelect("khz", "Tuning step", stepNamePtrs, stepValues,
                        (int)stepCount, now.settings.stepKHz,
                        autoAttrs("/api/step-size"));
      out +=
          F("<button type=submit class='secondary "
            "fallback'>Set</button></fieldset>");
      out += formClose();
    }
  }
  if (!onFm) {
    out +=
        F("<p><small>The FM settings appear when the radio is on FM. "
          "The tuner has nowhere to put them on an AM band."
          "</small></p>");
  }
  out += cardClose();

  /* --------------------------------------------------------- rds, clock */
  out += cardOpen("RDS, network time and battery");
  out += formOpen("/api/settings");
  kAuto = autoAttrs("/api/settings");
  out += F("<div class=grid>");
  out += formSelect("rds", "RDS decoder", offOn, zeroOne, 2, st->rdsEnabled,
                    kAuto);
  {
    static const char *const regionNames[] = {"Europe", "North America"};
    static const long regionValues[] = {RDS_REGION_EUROPE,
                                        RDS_REGION_NORTH_AMERICA};
    static_assert(
        sizeof(regionValues) / sizeof(regionValues[0]) == RDS_REGION_COUNT,
        "a region with no option on the settings page");
    out += formSelect("rrg", "RDS region", regionNames, regionValues, 2,
                      st->rdsRegion, kAuto);
  }
  out += formSelect("ntp", "Network time", offOn, zeroOne, 2, st->ntpEnabled,
                    kAuto);
  {
    static const char *const batNames[] = {"off", "per cent", "volts"};
    static const long batValues[] = {0, 1, 2};
    out += formSelect("bat", "Battery", batNames, batValues, 3, st->batteryShow,
                      kAuto);
  }
  {
    char tzText[8];
    if (!clockFormatOffset(st->clockOffsetMinutes, tzText, sizeof(tzText))) {
      tzText[0] = '\0';
    }
    out += formText("tzo", "UTC offset", tzText, "+05:30", kAuto);
  }
  out += F("</div>");
  out += F("<button type=submit class='secondary fallback'>Save</button>");
  out += formClose();
  out += cardClose();

  out += F("</div>");
}

/*
 * The Settings page: everything about how the radio behaves rather than
 * what it is tuned to. Seek, Sound, Display and Advanced, four groups
 * rather than one flat stack of cards, so the one wanted is easy to find.
 */
static void settingsForms(ChunkedReply &out, const Settings *st) {
  static const char *offOn[] = {"Off", "On"};
  static const long zeroOne[] = {0, 1};
  String kAuto;

  out += F("<div class=section-head><h2>Seek</h2><hr></div>");
  out += F("<div class=page-grid>");
  out += cardOpen("Seek", "span-all");
  out += formOpen("/api/settings");
  /* Every remaining card on this page but the volume knob posts to the same
   * endpoint, so `kAuto` is set once here and carries through Volume AGC,
   * Sounds and fades, the panel light, the theme, Custom's own colours, and
   * Band plan and knob below. */
  kAuto = autoAttrs("/api/settings");
  out += F("<div class=grid>");
  {
    static const char *sensNames[] = {
        "1, strong only", "2", "3",
        "4, the default", "5", "6, finds weak ones"};
    static const long sensValues[] = {1, 2, 3, 4, 5, 6};
    out += formSelect("fsn", "FM", sensNames, sensValues, 6,
                      st->fmScanSensitivity, kAuto);
    out += formSelect("asn", "AM", sensNames, sensValues, 6,
                      st->amScanSensitivity, kAuto);
    out += formSetting(st, "sqf", "Squelch floor, dBuV", kAuto);
  }
  out +=
      F("</div><p><small>Higher settles for a weaker signal and stops "
        "more often on things that are not stations. This one takes "
        "effect at once, with no reboot. The squelch floor is what "
        "keeps the automatic squelch shut on the channel beside a "
        "strong station, which looks like a station to everything "
        "except the level. 0 switches it off. Raise it if the radio "
        "opens on nothing next to a strong station, lower it if it "
        "mutes a weak station you can hear.</small></p>");
  out += F("<button type=submit class='secondary fallback'>Save</button>");
  out += formClose();
  out += cardClose();
  out += F("</div>");

  out += F("<div class=section-head><h2>Sound</h2><hr></div>");
  out += F("<div class=page-grid>");
  /* ---------------------------------------------------------- volume agc */
  out += cardOpen("Volume AGC");
  out += formOpen("/api/settings");
  out += F("<div class=grid>");
  /*
   * The target is the on switch, so a target of 0 and an AGC that is off
   * are the same row rather than two that can disagree. `settingsValid`
   * refuses everything between 0 and the minimum, which is why the hint
   * says so rather than the field offering it.
   */
  out += formSetting(st, "agt", "Target, 0 for off", kAuto);
  out += formSetting(st, "agb", "Boost, dB, 0 for cut only", kAuto);
  out +=
      F("</div><p><small>Levels one station against the next by "
        "changing the gain the tuner is given, never the volume the "
        "knob asked for.</small></p>");
  out += F("<button type=submit class='secondary fallback'>Save</button>");
  out += formClose();
  out += cardClose();

  /* ------------------------------------------------------- sounds, panel */
  out += cardOpen("Sounds and fades");
  out += formOpen("/api/settings");
  out += F("<div class=grid>");
  {
    /* A number in steps of 10, the same range and step as the menu row. A
     * list here could not show a value the menu set between its entries,
     * and the card posts every field on any change, so it would send the
     * list's first entry in its place. */
    out += formSetting(st, "smu", "Mute and squelch ramp, ms, 0 for off",
                       kAuto + F(" step=10"));
    static const char *beepNames[] = {"Off", "Keypad only",
                                      "Short &amp; long press", "Every press"};
    static const long beepValues[] = {0, 1, 2, 3};
    out += formSelect("bpk", "Beep on", beepNames, beepValues, 4, st->beepKey,
                      kAuto);
    out += formSelect("bpe", "Band edge beep", offOn, zeroOne, 2, st->beepEdge,
                      kAuto);
    out += formSelect("bps", "Chime at start up", offOn, zeroOne, 2,
                      st->beepStart, kAuto);
  }
  out +=
      F("</div><p><small>The ramp takes the click off a mute, the "
        "squelch closing and a bandwidth change. It also runs before a "
        "reboot or a firmware update. The beeps are off unless you "
        "want them.</small></p>");
  out += F("<button type=submit class='secondary fallback'>Save</button>");
  out += formClose();
  out += cardClose();
  out += F("</div>");

  out += F("<div class=section-head><h2>Display</h2><hr></div>");
  out += F("<div class=page-grid>");
  out += cardOpen("The panel light");
  out += formOpen("/api/settings");
  out += F("<div class=grid>");
  out += formSetting(st, "blt", "Brightness, per cent", kAuto);
  out += formSetting(st, "bdm", "Dimmed, per cent", kAuto);
  out += formSetting(st, "bds", "Dim after, seconds", kAuto);
  out += formSelect("blf", "Fade up at start", offOn, zeroOne, 2,
                    st->backlightFade, kAuto);
  out +=
      F("</div><p><small>Dim after 0 never dims. The knob, a button "
        "and a key all bring it straight back.</small></p>");
  out += F("<button type=submit class='secondary fallback'>Save</button>");
  out += formClose();
  out += cardClose();

  /* The same switch as Controls > Touch and recovery's Touch row. */
  out += cardOpen("Touch screen");
  out += formOpen("/api/settings");
  out += F("<div class=grid>");
  /* Stored as off, so On is 0; listed Off first, as every other switch. */
  static const long touchValues[] = {1, 0};
  out += formSelect("tof", "Touch", offOn, touchValues, 2, st->touchOff, kAuto);
  out +=
      F("</div><p><small>Off, the radio stops reading the touch screen. "
        "A touch does nothing on the radio yet.</small></p>");
  out += F("<button type=submit class='secondary fallback'>Save</button>");
  out += formClose();
  out += cardClose();

  /*
   * The theme, and the one custom slot. Only Custom's own colours are ever
   * set here; the other named themes are fixed palettes in core/palette.c,
   * not something a person can lose by mistake while trying out their own.
   * A native `<details>` holds the eight colours a screen draws with,
   * `open` when Custom is the active theme, since a person can still
   * expand it to set Custom up before ever switching to it. One theme for
   * the day and one for the night, by the panel's own clock.
   */
  bool customActive =
      (st->theme == THEME_CUSTOM || st->nightTheme == THEME_CUSTOM);
  out += cardOpen("Theme", "span-all");
  out += formOpen("/api/settings");
  {
    static const char *themeNames[THEME_COUNT];
    static long themeValues[THEME_COUNT];
    /* In the menu's order, each sending its saved index. */
    for (int i = 0; i < THEME_COUNT; i++) {
      const uint8_t theme = paletteThemeAt((uint8_t)i);
      themeNames[i] = themeAt(theme)->name;
      themeValues[i] = theme;
    }
    out += formRadioGroup("thm", "Day, 6 AM to 6 PM", themeNames, themeValues,
                          THEME_COUNT, st->theme, kAuto);
    out += formRadioGroup("thn", "Night, 6 PM to 6 AM", themeNames, themeValues,
                          THEME_COUNT, st->nightTheme, kAuto);
  }
  out += formClose();
  out += F("<details");
  if (customActive) {
    out += F(" open");
  }
  out += F("><summary>Custom's colours</summary>");
  out += formOpen("/api/settings");
  out +=
      F("<p><small>Changing one here only ever changes what Custom looks "
        "like.</small></p>"
        "<div class=grid>");
  {
    static const char *const kThemeColourNames[THEME_CUSTOM_COLOUR_COUNT] = {
        "tc0", "tc1", "tc2", "tc3", "tc4", "tc5", "tc6", "tc7", "tc8"};
    static const char *const kThemeColourLabels[THEME_CUSTOM_COLOUR_COUNT] = {
        "Background",  "Header", "Tile and row", "Radio", "Broadcast",
        "Measurement", "Good",   "Fault",        "Dead"};
    for (int i = 0; i < THEME_CUSTOM_COLOUR_COUNT; i++) {
      /* No screen draws with the header colour, so there is nothing to see
       * when it changes. It stays stored, so a saved Custom theme keeps its
       * layout. */
      if (i == PALETTE_HEADER) {
        continue;
      }
      char hex[8];
      themeColourFormat(st->customTheme[i][0], st->customTheme[i][1],
                        st->customTheme[i][2], hex, sizeof(hex));
      out +=
          formColour(kThemeColourNames[i], kThemeColourLabels[i], hex, kAuto);
    }
  }
  out += F("</div>");
  out += F("<button type=submit class='secondary fallback'>Save</button>");
  out += formClose();
  out += F("</details>");
  out += cardClose();
  out += F("</div>");

  out += F("<div class=section-head><h2>Updates</h2><hr></div>");
  out += F("<div class=page-grid>");
  out += cardOpen("From GitHub releases");
  out += formOpen("/api/settings");
  out += F("<div class=grid>");
  out += formSelect("upc", "Check for updates", offOn, zeroOne, 2,
                    st->updateCheck, kAuto);
  out +=
      F("</div><p><small>On, the radio looks for a newer release once "
        "it is on the network, once each start, and offers it on its "
        "screen. Nothing is installed until somebody says yes. The "
        "<a href=/system>System</a> page shows what it found.</small></p>");
  out += F("<button type=submit class='secondary fallback'>Save</button>");
  out += formClose();
  out += cardClose();
  out += F("</div>");

  out +=
      F("<div class=section-head><h2>Advanced, needs a reboot</h2><hr>"
        "</div>");
  out += F("<div class=page-grid>");
  /* ----------------------------------------------------------- band plan */
  out += cardOpen("Band plan and knob");
  out +=
      F("<p><small>These are read when the radio starts, so they need "
        "a reboot. They decide which frequencies exist, which is not "
        "a thing to change under a radio that is tuned to one of them."
        "</small></p>");
  out += formOpen("/api/settings");
  out += F("<div class=grid>");
  static const char *regionNames[] = {"65 to 108", "Japan, 76 to 95",
                                      "76 to 108", "87 to 108", "87.5 to 108"};
  static const long regionValues[] = {0, 1, 2, 3, 4};
  out += formSelect("rgn", "FM band, MHz", regionNames, regionValues, 5,
                    st->fmRegion, kAuto);
  static const char *spacingNames[] = {"9 kHz", "10 kHz"};
  out += formSelect("spc", "Medium wave steps", spacingNames, zeroOne, 2,
                    st->mwSpacing, kAuto);
  static const char *encoderNames[] = {"Standard", "Optical"};
  out += formSelect("enc", "Encoder", encoderNames, zeroOne, 2, st->encoderKind,
                    kAuto);
  static const char *directionNames[] = {"Normal", "Reversed"};
  out += formSelect("edr", "Knob direction", directionNames, zeroOne, 2,
                    st->encoderDirection, kAuto);
  out += F("</div>");
  out +=
      F("<button type=submit class='secondary fallback'>Save band "
        "plan</button>");
  out += formClose();
  out += cardClose();

  /* ------------------------------------------------------------- the knob */
  out += cardOpen("The volume knob");
  out += F("<p>");
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
        "press Done. While measuring the knob changes nothing, so you "
        "can sweep it to the loud end without the volume following.</p>");
  out += formOpen();
  out +=
      F("<fieldset role=group>"
        "<button type=submit formaction='/api/pot' hx-post='/api/pot' "
        "name=act value=start>Measure</button>"
        "<button type=submit formaction='/api/pot' hx-post='/api/pot' "
        "name=act value=finish>Done</button>"
        "<button type=submit formaction='/api/pot' hx-post='/api/pot' "
        "name=act value=cancel class=secondary>Cancel</button></fieldset>");
  out += formClose();
  out += cardClose();
  out += F("</div>");
}

static String signInForm(const char *next) {
  String out = cardOpen("Access PIN");
  out +=
      F("<p>Six digits. A new radio is on its default PIN. Five wrong "
        "tries locks this for a minute.</p>"
        "<form method=post action='/auth'>"
        "<input type=hidden name=nxt value='");
  out += next;
  out +=
      F("'><label>PIN<input name=pin inputmode=numeric "
        "pattern='[0-9]{6}' maxlength=6 required></label>"
        "<button type=submit>Unlock</button>"
        "</form>");
  out += cardClose();
  return out;
}

/*
 * Where a sign in may send the browser afterwards.
 *
 * Only the seven pages this firmware serves. Anything else, including an
 * absolute URL somebody put in the form, comes back as the home page.
 */
const char *safeNext(const String &want) {
  static const char *kPages[] = {"/",        "/radio", "/fm",    "/settings",
                                 "/network", "/dx",    "/system"};
  for (size_t i = 0; i < sizeof(kPages) / sizeof(kPages[0]); i++) {
    if (want == kPages[i]) {
      return kPages[i];
    }
  }
  return "/";
}

static String defaultPinBanner(void) {
  if (!webPinIsDefault()) {
    return String();
  }
  return F(
      "<mark>This radio is still on its default access PIN.</mark> "
      "Anyone who can reach it on the network can change its settings "
      "and replace its firmware. Set your own PIN on the "
      "<a href='/network'>Network</a> page.");
}

static String wifiForm(void) {
  String out = cardOpen("Wi-Fi");
  if (inSetupMode()) {
    out +=
        F("<p>The radio could not join a network, so it is serving this "
          "page on its own. Enter the details and it will try again.</p>");
  } else if (wifiState() == WIFI_STATE_ACCESS_POINT) {
    out +=
        F("<p>The hotspot is set On, so the radio is serving this page on "
          "its own. Saving a network here sets the hotspot back to Auto and "
          "joins that network.</p>");
  }
  out +=
      F("<form method=post action='/wifi'>"
        "<label>Network name<input name=sid maxlength=32 required "
        "value='");
  /* The stored name only to somebody who has signed in, or on the access
   * point, where there is no network yet and the form is open by design.
   * Filling it in for anyone who can reach the radio hands out the name of
   * the home network to the whole of it. */
  if (signedIn() || inSetupMode()) {
    out += escapeHtml(sWeb->settings->wifiSsid);
  }
  out +=
      F("'></label><label>Passphrase, leave empty for an open network"
        "<input name=pwd type=password maxlength=64></label>"
        "<button type=submit>Save and join</button></form>");
  out += cardClose();
  return out;
}

/* Home. What the radio is and where it is, and nothing that changes it. */
static void handleRoot(void) {
  sWeb->requests++;
  /* Every one of these pages is built fresh from what the radio is doing
   * right now; a browser serving one from its own cache on a plain
   * reload would show a station, a theme or a layout that changed since,
   * indistinguishable from a page that never updated at all. */
  sWeb->server.sendHeader("Cache-Control", "no-store");
  ChunkedReply out(sWeb->server);
  out.begin(200, "text/html");
  pageHead(out, "TEF668X", "/");
  out += defaultPinBanner();
  out += F("<div class=page-grid>");

  /* What it is receiving, read once. A page that refreshes itself belongs on
   * a websocket, which this firmware does not have. */
  RadioSnapshot now;
  if (radioGetSnapshot(&now)) {
    out += cardOpen("Radio", "hero span-all");
    char freq[16];
    if (bandFormatFrequency(now.settings.band, now.settings.freqKHz, freq,
                            sizeof(freq))) {
      out += F("<div class=readout>");
      out += freq;
      out += F("<span class=unit> ");
      out += bandFrequencyUnit(now.settings.band);
      out += F(" &middot; ");
      out += bandName(now.settings.band);
      out += F("</span></div>");
    }
    out += F("<div class=pill-row>");
    if (now.qualityValid) {
      char level[12];
      signalFormatLevel(shownLevel(now), level, sizeof(level));
      out += F("<span class=status-pill><small>");
      out += level;
      out += F(" dBuV</small></span>");
      if (now.quality.stereo && !now.settings.forcedMono) {
        out +=
            F("<span class='status-pill on' title='Receiving in stereo'>"
              "<svg viewBox='0 0 24 24' fill=none stroke=currentColor "
              "stroke-width=2><circle cx=8 cy=12 r=4/>"
              "<circle cx=16 cy=12 r=4/></svg>Stereo</span>");
      }
    }
    out += F("<span class='status-pill");
    out += now.squelchOpen ? F(" on") : F("");
    out += F("'>Squelch ");
    out += now.squelchOpen ? F("open") : F("shut");
    out += F("</span>");
    if (now.settings.muted) {
      out += F("<span class='status-pill on'>Muted</span>");
    }
    out += F("</div>");
    if (signedIn()) {
      out +=
          F("<a role=button class=secondary href='/radio'>Tune and "
            "settings</a>");
    }
    out += cardClose();
  }

  out += cardOpen("Network");
  out += F("<table>");
  out +=
      "<tr><td>Joined</td><td>" + escapeHtml(wifiNetworkName()) + "</td></tr>";
  out += "<tr><td>Address</td><td>" + String(wifiAddress()) + "</td></tr>";
  out += F("<tr><td>Mode</td><td>");
  out += wifiState() == WIFI_STATE_ACCESS_POINT ? F("its own hotspot")
                                                : F("joined a network");
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
  out += F("</div>");
  out += pageTail();
}

/*
 * The Radio page: the dial, then what the tuner and the radio itself are
 * doing. Reception's own settings and the RDS decoder's settings are on
 * `/fm`, since this page tunes and reports and does not also configure.
 */
static void handleRadioPage(void) {
  sWeb->requests++;
  sWeb->server.sendHeader("Cache-Control", "no-store");
  ChunkedReply out(sWeb->server);
  out.begin(200, "text/html");
  pageHead(out, "Radio", "/radio");
  if (!signedIn()) {
    out += signInForm("/radio");
    out += pageTail();
    return;
  }
  RadioSnapshot now;
  bool live = radioGetSnapshot(&now);
  if (liveOrExcuse(out, live)) {
    const Settings *st = sWeb->settings;
    out += F("<div class=page-grid>");
    radioDialForms(out, now, st);
    out += F("</div>");
    radioTelemetryCards(out, now, st);
  }
  out += pageTail(true);
}

/* The FM & RDS page: reception quality and the RDS decoder's own settings. */
static void handleFmPage(void) {
  sWeb->requests++;
  sWeb->server.sendHeader("Cache-Control", "no-store");
  ChunkedReply out(sWeb->server);
  out.begin(200, "text/html");
  pageHead(out, "FM &amp; RDS", "/fm");
  if (!signedIn()) {
    out += signInForm("/fm");
    out += pageTail();
    return;
  }
  RadioSnapshot now;
  bool live = radioGetSnapshot(&now);
  if (liveOrExcuse(out, live)) {
    fmRdsForms(out, now, sWeb->settings);
  }
  out += pageTail(true);
}

/*
 * The DX page's chart, drawn by the browser from
 * GET /api/dx/sweep: the level as a line over a shaded area, the baseline
 * as a line and the peak hold dotted, the floor, the rise strip under it
 * and the dial's mark from GET /api/state. Each is one path, not a shape a
 * channel, so the page stays small in the browser. The mouse over a channel shows its
 * readings and a click tunes there. Its buttons post to /api/dx, as the
 * panel's keys call the same functions.
 */
static const char kDxPageBody[] =
    "<style>"
    ".rd{font-variant-numeric:tabular-nums;margin-bottom:.4rem;"
    "min-height:1.6em}.rd b{font-size:1.25rem}"
    ".rd .up{color:var(--pico-primary)}.rd .dn,.meta{color:"
    "var(--pico-muted-color)}.meta{font-size:.85rem}"
    "#chart,#strip{width:100%;height:auto;display:block;cursor:crosshair}"
    "#chart text,#strip text{fill:var(--pico-muted-color);font-size:13px}"
    ".mono,.live b{font-family:var(--pico-font-family-monospace);"
    "white-space:pre}.live{display:flex;flex-wrap:wrap;gap:.2rem 1.4rem}"
    ".live small{display:block;font-size:.72rem;text-transform:uppercase;"
    "letter-spacing:.09em;color:var(--pico-muted-color)}"
    ".live b{font-size:2rem;line-height:1.1}.live .pi{color:"
    "var(--pico-primary)}.live .part{color:var(--pico-muted-color)}"
    ".blk{display:flex;gap:.35rem;margin:.8rem 0 .3rem}.blk span{width:3.2rem;"
    "text-align:center;border-radius:.3rem;font-weight:600;font-size:.85rem;"
    "border:1px solid var(--pico-primary);color:var(--pico-primary)}"
    ".blk .e0{background:var(--pico-primary);color:var(--pico-primary-inverse)}"
    ".blk .e3{border-color:var(--pico-muted-border-color);color:"
    "var(--pico-muted-color);text-decoration:line-through}"
    ".chips{display:flex;flex-wrap:wrap;gap:.4rem;margin:.6rem 0}"
    ".chips span{font-size:.78rem;padding:0 .5rem;border-radius:1rem;"
    "border:1px solid var(--pico-muted-border-color);color:"
    "var(--pico-muted-color)}.chips .on{border-color:var(--pico-primary);"
    "color:var(--pico-primary)}.rt{overflow-wrap:anywhere;white-space:normal}"
    ".wide{overflow-x:auto}.wide table{font-variant-numeric:tabular-nums}"
    ".wide td,.wide th{white-space:nowrap}.wide button{width:auto;margin:0;"
    "padding:.1rem .7rem;font-size:.8rem}.new{font-size:.68rem;padding:0 .4rem;"
    "margin-left:.3rem;border-radius:.3rem;background:var(--pico-primary);"
    "color:var(--pico-primary-inverse)}.tick{color:var(--pico-primary)}"
    ".section-head a{margin-left:auto;font-size:.85rem;width:auto}"
    "</style>"
    "<div class=section-head><h2>RDS now</h2><hr></div>"
    "<article class=hero><p id=rmsg>Reading the radio...</p><div id=rbody "
    "hidden><div id=rtop></div><div id=rblk></div><p class=meta id=rcnt></p>"
    "<div class=chips "
    "id=rchips></div><p class='mono rt' id=rrt></p><details><summary "
    "class=meta id=rhs></summary><table><thead><tr><th>UTC</th><th>PI</th>"
    "<th>Name</th></tr></thead><tbody id=rht></tbody></table></details></div>"
    "</article>"
    "<div class=section-head><h2>Level sweep</h2><hr></div>"
    "<article class=hero><div class=rd id=rd></div>"
    "<svg id=chart viewBox='0 0 1000 300'></svg>"
    "<svg id=strip viewBox='0 0 1000 110'></svg>"
    "<p class=meta id=meta></p><p class=meta id=serr></p><div class=btnrow>"
    "<button id=sweep type=button>Sweep now</button>"
    "<button id=open type=button class=secondary hidden>Open DX mode"
    "</button><button id=fix type=button class=secondary>Fix this as the "
    "baseline</button><button id=auto type=button class=outline>Back to "
    "the median</button></div></article>"
    "<div class=section-head><h2>Catches</h2><hr><a href=/api/dx.csv "
    "role=button class=outline download>Download CSV</a></div>"
    "<article><div class=wide id=catches></div><p class=meta id=gone></p>"
    "<p class=meta>Newest first, with the time each was last heard, in UTC. "
    "The CSV has each catch's strongest hearing that had a time, oldest "
    "first, "
    "in the TEF logbook format that the FMLIST converter CSVtoURDS takes as "
    "it is.</p></article>";

static const char kDxPageScript[] = R"JS(<script>
(function(){
var D=null,DIAL=0,cur=null,W=1000,H=300,L=44,R=8,T=10,B=26,PW=W-L-R,PH=H-T-B,lo=0,hi=1;
function g(i){return document.getElementById(i)}
function toast(text,bad){g('toastMsg').textContent=text;g('toast').className=bad?'bad show':'good show'}
g('toastx').onclick=function(){g('toast').classList.remove('show')};
function post(path,args){return fetch(path,{method:'POST',body:new URLSearchParams(args)}).then(function(r){return r.text().then(function(t){toast(t.trim(),!r.ok);return r})})}
function khz(i){return D.low+i*D.step}
function mhz(k){return (k/1000).toFixed(2)}
function db(t){return t===null||t===undefined?'-':(t/10).toFixed(1)}
/* A level as shown, with the FM level offset the radio gives;
 * db alone is for a difference, which the offset does not move. */
var LVO=0;
function lv(t){return t===null||t===undefined?'-':(t/10+LVO).toFixed(1)}
function X(i){return L+i*PW/D.count}
function Y(t){t=Math.max(lo,Math.min(hi,t));return T+PH-(t-lo)*PH/(hi-lo)}
function scale(){var top=-1000;D.level.concat(D.peak||[]).forEach(function(v){if(v!==null&&v>top)top=v});
 var f=D.floor===null?-100:D.floor;lo=Math.floor((f-50)/100)*100;hi=Math.max(lo+100,Math.ceil((top+30)/100)*100)}
function draw(){
 var c=g('chart'),s=g('strip');
 if(!D||!D.count){c.innerHTML='';s.innerHTML='';g('rd').textContent=D&&D.running?'Sweeping...':'No sweep yet. Open DX mode and press Sweep now.';return}
 var o='',i,v,bw=PW/D.count,w=Math.max(bw-.6,.6);
 for(v=lo;v<=hi;v+=100)o+='<line x1='+L+' x2='+(W-R)+' y1='+Y(v)+' y2='+Y(v)+' stroke="var(--pico-muted-border-color)"/><text x='+(L-6)+' y='+(Y(v)+4)+' text-anchor=end>'+(v/10+LVO)+'</text>';
 o+='<text x='+(L+4)+' y='+(T+12)+'>dB&micro;V</text>';
 var first=Math.ceil(D.low/2000)*2,last=Math.floor(khz(D.count-1)/1000);
 for(var m=first;m<=last;m+=2)o+='<text x='+(X((m*1000-D.low)/D.step)+bw/2)+' y='+(H-6)+' text-anchor=middle>'+m+'</text>';
 if(D.floor!==null)o+='<line x1='+L+' x2='+(W-R)+' y1='+Y(D.floor)+' y2='+Y(D.floor)+' stroke="var(--pico-muted-color)" stroke-dasharray="4 4"/>';
 var bar=D.running?'var(--pico-muted-color)':'var(--pico-primary)';
 function line(a){var d='',up=false;for(var j=0;j<D.count;j++){if(!a||a[j]===null){up=false;continue}d+=(up?'L':'M')+(X(j)+bw/2).toFixed(1)+','+Y(a[j]).toFixed(1);up=true}return d}
 var lv=line(D.level);
 if(lv)o+='<path d="M'+L+','+(T+PH)+lv.replace(/M/g,'L')+'L'+(W-R)+','+(T+PH)+'Z" fill="'+bar+'" fill-opacity=.25 />';
 if(D.peak)o+='<path d="'+line(D.peak)+'" fill=none stroke="var(--pico-muted-color)" stroke-dasharray="2 3"/>';
 if(D.baseline_level)o+='<path d="'+line(D.baseline_level)+'" fill=none stroke="var(--pico-color)" stroke-width=1.2 />';
 o+='<path d="'+lv+'" fill=none stroke="'+bar+'" stroke-width=1.6 />';
 var di=(DIAL-D.low)/D.step;
 if(di>=0&&di<D.count&&di%1===0)o+='<path d="M'+(X(di)+bw/2-6)+','+(T+PH)+' l6,-9 l6,9 z" fill="var(--pico-ins-color)"/>';
 if(cur!==null)o+='<line x1='+(X(cur)+bw/2)+' x2='+(X(cur)+bw/2)+' y1='+T+' y2='+(T+PH)+' stroke="var(--pico-color)"/>';
 c.innerHTML=o;
 var h=110,z=h/2,r='<line x1='+L+' x2='+(W-R)+' y1='+z+' y2='+z+' stroke="var(--pico-muted-color)"/><text x='+(L-6)+' y=14 text-anchor=end>+15</text><text x='+(L-6)+' y='+(z+4)+' text-anchor=end>0</text><text x='+(L-6)+' y='+(h-4)+' text-anchor=end>-15</text>';
 var pu='',pd='';
 if(D.rise)for(i=0;i<D.count;i++){var q=D.rise[i];if(q===null||q===0)continue;var ph=Math.min(z-2,Math.abs(q)*z/150).toFixed(1),bx=X(i).toFixed(1);
  if(q>0)pu+='M'+bx+','+z+'v-'+ph+'h'+w.toFixed(1)+'v'+ph+'z';else pd+='M'+bx+','+(z+1)+'v'+ph+'h'+w.toFixed(1)+'v-'+ph+'z'}
 if(pu)r+='<path d="'+pu+'" fill="'+bar+'"/>';
 if(pd)r+='<path d="'+pd+'" fill="var(--pico-muted-color)"/>';
 if(cur!==null)r+='<line x1='+(X(cur)+bw/2)+' x2='+(X(cur)+bw/2)+' y1=0 y2='+h+' stroke="var(--pico-color)"/>';
 s.innerHTML=r;
 var k=cur!==null?cur:(di>=0&&di<D.count&&di%1===0?di:null);
 if(k===null){g('rd').textContent='The dial is not on the sweep. Point at a channel.';return}
 var rs=D.rise?D.rise[k]:null;
 g('rd').innerHTML='<b>'+mhz(khz(k))+'</b> MHz &middot; <b>'+lv(D.level[k])+'</b> dB&micro;V'+(rs===null?'':' &middot; rise <span class='+(rs>0?'up':'dn')+'>'+(rs>0?'+':'')+db(rs)+' dB</span>')+(D.peak?' &middot; peak '+lv(D.peak[k]):'')+(D.baseline_level?' &middot; base '+lv(D.baseline_level[k]):'')+' <small class=meta>'+(cur===null?'the dial':'click to tune')+'</small>'}
function meta(){if(!D||!D.count){g('meta').textContent=D&&D.abandoned?'The last sweep was stopped before the top.':'';return}
 var p=[];
 if(D.running)p.push('Sweeping now, this is the last whole sweep');
 if(D.real){var a=Math.floor(Date.now()/1000)-D.time,t=new Date(D.time*1000).toLocaleTimeString([],{hour:'2-digit',minute:'2-digit'});
  p.push('Swept at '+t+(a>=0?(a<60?' (now)':a<3600?' ('+Math.floor(a/60)+' min ago)':a<86400?' ('+Math.floor(a/3600)+' h ago)':' ('+Math.floor(a/86400)+' d ago)'):''))}
 else p.push('Swept with the clock unset');
 p.push('width '+D.width+' kHz','took '+(D.took_ms/1000).toFixed(1)+' s');
 p.push(D.baseline==='fixed'?'baseline: fixed by hand':D.baseline==='median'?'baseline: median of '+D.baseline_sweeps+' sweeps':'no baseline yet');
 if(D.floor!==null)p.push('floor '+lv(D.floor)+' dBµV');
 if(D.abandoned&&!D.running)p.push('the last sweep was stopped before the top');
 g('meta').textContent=p.join(' · ')}
function load(){return get('/api/dx/sweep',true).then(function(a){
 D=a;LVO=a.lvo||0;buttons();
 g('serr').textContent='';if(D.count)scale();draw();meta();return D}).catch(function(e){g('serr').textContent=why(e)+(D?' The chart is the last sweep it gave.':'')})}
/* One loop at a time. Each round waits for the last, and a failed round is
 * tried again after 2 s. It ends on an answer that says no sweep is running,
 * or after five failed rounds in a row; the Catches poll starts it again
 * when the sweep has moved on or none was ever read. Nothing is asked while
 * the tab is hidden. */
var FOLLOW=false;
function follow(){if(!FOLLOW){FOLLOW=true;round(0)}}
function round(miss){try{if(document.hidden){setTimeout(function(){round(miss)},1000);return}
 load().then(function(d){if(d&&d.running)setTimeout(function(){round(0)},500);else if(!d&&miss<4)setTimeout(function(){round(miss+1)},2000);else FOLLOW=false})}catch(e){FOLLOW=false}}
function at(e,svg){var r=svg.getBoundingClientRect(),x=(e.clientX-r.left)*W/r.width,i=Math.floor((x-L)*D.count/PW);return i<0||i>=D.count?null:i}
['chart','strip'].forEach(function(id){var s=g(id);
 s.addEventListener('mousemove',function(e){if(D&&D.count){cur=at(e,s);draw()}});
 s.addEventListener('mouseleave',function(){cur=null;if(D)draw()});
 s.addEventListener('click',function(e){if(!D||!D.count)return;var i=at(e,s);if(i===null)return;
  post('/api/tune',{khz:khz(i)}).then(function(r){if(r.ok){DIAL=khz(i);draw()}})})});
g('sweep').onclick=function(){post('/api/dx',{sweep:1}).then(function(r){if(r.ok)follow()})};
g('open').onclick=function(){post('/api/dx',{on:1}).then(function(){follow();catches()})};
g('fix').onclick=function(){post('/api/dx',{baseline:'now'}).then(follow)};
g('auto').onclick=function(){post('/api/dx',{baseline:'auto'}).then(follow)};
function esc(t){return String(t).replace(/[&<>"']/g,function(c){return '&#'+c.charCodeAt(0)+';'})}
function two(n){return (n<10?'0':'')+n}
/* The body too, not only the headers: a reply cut off halfway would
 * otherwise never settle and the next round would never be asked for. */
function get(p,json){var c=new AbortController(),t=setTimeout(function(){c.abort()},4000);return fetch(p,{signal:c.signal}).then(function(r){
 if(!r.ok)return r.text().then(function(x){var e=new Error(x.trim()||'The radio said '+r.status+'.');e.said=true;throw e});return json?r.json():r.text()}).finally(function(){clearTimeout(t)})}
/* What to say for a failed ask: the radio's own reason when it gave one. */
function why(e){return e&&e.said?e.message:'The radio did not answer.'}
function put(id,h){var e=g(id);if(e._h!==h){e._h=h;e.innerHTML=h}}
var MON=['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'],HEARD=[],HKHZ=0,LAST='';
function msg(t){g('rmsg').textContent=t;g('rmsg').hidden=false;g('rbody').hidden=true}
function rdsBox(t){
 var r=t.rds,i;
 if(!t.bnd){msg('The radio could not be read. Trying again.');return}
 if(t.khz!==HKHZ){HKHZ=t.khz;HEARD=[];LAST=''}
 if(r&&r.off){msg('The RDS decoder is switched off in FM & RDS.');return}
 if(!r||!(r.pi||r.piz||r.pih||r.psh)){msg(t.bnd==='FM'||t.bnd==='OIRT'?'No RDS on '+t.f+' '+t.unt+'.':'RDS is only on FM.');return}
 g('rmsg').hidden=true;g('rbody').hidden=false;
 var sure=r.pi||(r.piz?'0000':''),pi=sure||r.pih||'----',ps=r.ps;
 if(ps===undefined){ps='';for(i=0;i<8;i++)ps+=(r.psm>>i)&1?(r.psh||'').charAt(i):'·'}
 if(sure&&sure!==LAST){LAST=sure;var d=new Date();HEARD.unshift([two(d.getUTCHours())+':'+two(d.getUTCMinutes())+':'+two(d.getUTCSeconds()),sure,r.ps]);HEARD.length=Math.min(HEARD.length,8)}
 else if(sure&&r.ps!==undefined)HEARD[0][2]=r.ps;
 var o='<div class=live><div><small>PI</small><b class="pi'+(sure?'':' part')+'">'+esc(pi)+'</b></div><div><small>Station name</small><b'+(r.ps===undefined?' class=part':'')+'>'+esc(ps)+'</b></div></div>';
 put('rtop',o);o='';
 if(r.blk){o+='<div class=blk title="Error level of each block. Filled is clean, outlined was corrected, struck through was lost.">';
  for(i=0;i<4;i++)o+='<span class=e'+(r.blk[i]===0?0:r.blk[i]>=3?3:1)+'>'+'ABCD'.charAt(i)+'</span>';o+='</div>'}
 put('rblk',o);
 put('rcnt',r.grp+' groups, '+(r.grp?Math.round(r.use*100/r.grp):0)+'% used &middot; '+r.cor+' blocks corrected &middot; '+r.bad+' blocks lost');
 o='';
 if(r.ptn)o+='<span class=on>'+esc(r.ptn)+'</span>';
 if(r.cty)o+='<span class=on>'+esc(r.cty)+'</span>';
 if(r.cs)o+='<span class=on title="'+(r.csg?'Call letters worked out from the PI, a guess':'The name the station sends as its RT+ short name')+'">'+esc(r.cs)+(r.csg?'?':'')+'</span>';
 if(r.tp!==undefined)o+='<span'+(r.tp?' class=on':'')+'>TP</span><span'+(r.ta?' class=on':'')+'>TA</span><span class=on>'+(r.ms==='speech'?'Speech':'Music')+'</span>';
 put('rchips',o);
 put('rrt',r.rt?esc(r.rt):'');g('rrt').hidden=!r.rt;
 put('rhs','Heard on '+t.f+' '+t.unt+' while this page was in view');
 o='';HEARD.forEach(function(h){o+='<tr><td>'+h[0]+'</td><td>'+esc(h[1])+'</td><td class=mono>'+(h[2]===undefined?'-':esc(h[2]))+'</td></tr>'});
 put('rht',o)}
/* Hidden, the page asks nothing, so a retune away and back is not seen. */
document.addEventListener('visibilitychange',function(){if(!document.hidden)HKHZ=0});
function live(){if(document.hidden){setTimeout(live,1000);return}
 get('/api/state',true).then(function(d){var t=d.tun||{};if(t.khz&&t.khz!==DIAL){DIAL=t.khz;if(D)draw()}rdsBox(t)}).catch(function(e){msg(why(e))}).then(function(){setTimeout(live,1000)})}
var DX=null;
function buttons(){if(DX){g('open').hidden=!!DX.on;g('sweep').disabled=!DX.on||!!(D&&D.running)}}
function dxList(txt){var lines=txt.split('\n'),dx=null,rows=[],bad=false;try{dx=JSON.parse(lines[0])}catch(e){}
 lines.slice(1).forEach(function(l){if(l)try{rows.push(JSON.parse(l))}catch(e){bad=true}});
 if(!dx||bad){missed({said:true,message:'The last answer was cut short.'});return}
 DX=dx;LVO=dx.lvo||0;buttons();
 /* A sweep started on the panel or over the API, or a baseline changed
  * there, is followed too, and a chart never read is tried again. The
  * number compared is the one the loaded sweep came with, so a load that
  * failed or was served before the change is caught on the next poll. */
 if(dx.sweeping||!D||D.rev!==dx.sweep_rev||D.running)follow();
 /* The age under the chart moves on even when the sweep does not. */
 else meta()
 var n=dx.dropped;
 g('gone').textContent=n===1?'1 older catch was dropped to make room. It is not in this list or in the CSV.':n?n+' older catches were dropped to make room. They are not in this list or in the CSV.':'';
 var o='<p>No catches this session. Open DX mode on the radio, then tune or scan.</p>';
 if(rows.length){o='<table><thead><tr><th>MHz</th><th>PI</th><th>Name</th><th>Country</th><th>dB&micro;V</th><th>Heard</th><th>Last</th><th></th></tr></thead><tbody>';
 rows.forEach(function(k){var d=new Date(k.time*1000);
  o+='<tr><td>'+mhz(k.khz)+'</td><td>'+k.pi+(k.new?'<span class=new>NEW</span>':'')+'</td><td class=mono>'+(k.name===null?'-':esc(k.name))+'</td><td>'+(k.country===null?'-':esc(k.country))+'</td><td>'+lv(k.level_dbuv)+'</td><td>&times;'+k.count+'</td><td>'+(k.real?two(d.getUTCDate())+' '+MON[d.getUTCMonth()]+' '+two(d.getUTCHours())+':'+two(d.getUTCMinutes()):'clock unset')+'</td><td>'+(k.logged?'<span class=tick>&#10003; logged</span> ':'')+(!k.logged||k.due?'<button type=button class=outline data-id='+k.id+(SENT[k.id]?' disabled':'')+'>'+(k.logged?'Log again':'Log')+'</button>':'')+'</td></tr>'});
  o+='</tbody></table>'}
 put('catches',o)}
/* A button is off while its log is on the way, so a double click writes
 * one entry; SENT keeps that through a redraw of the table. */
var SENT={};
g('catches').addEventListener('click',function(e){var bt=e.target.closest('button');if(!bt||bt.disabled)return;
 var id=bt.getAttribute('data-id');SENT[id]=true;bt.disabled=true;g('catches')._h=null;
 post('/api/dx',{logid:id}).catch(function(){}).then(function(){delete SENT[id];catches()})});
/* A missed answer leaves the last table and its buttons, and says so. */
function missed(e){g('gone').textContent=why(e)+(g('catches').innerHTML?' The table is what it said before.':' Trying again.')}
function catches(){return get('/api/dx').then(dxList).catch(missed)}
function poll(){(document.hidden?Promise.resolve():catches()).then(function(){setTimeout(poll,5000)})}
live();poll();
follow();
})();
</script>)JS";

static void handleDxPage(void) {
  sWeb->requests++;
  sWeb->server.sendHeader("Cache-Control", "no-store");
  ChunkedReply out(sWeb->server);
  out.begin(200, "text/html");
  pageHead(out, "DX", "/dx");
  if (!signedIn()) {
    out += signInForm("/dx");
    out += pageTail();
    return;
  }
  out += pageToast();
  out += kDxPageBody;
  out += kDxPageScript;
  out += pageTail();
}

/* The Settings page: everything about how the radio behaves. */
static void handleSettingsPage(void) {
  sWeb->requests++;
  sWeb->server.sendHeader("Cache-Control", "no-store");
  ChunkedReply out(sWeb->server);
  out.begin(200, "text/html");
  pageHead(out, "Settings", "/settings");
  if (!signedIn()) {
    out += signInForm("/settings");
    out += pageTail();
    return;
  }
  out += pageToast();
  settingsForms(out, sWeb->settings);
  out += pageTail(true);
}

static void handleNetworkPage(void) {
  sWeb->requests++;
  sWeb->server.sendHeader("Cache-Control", "no-store");
  ChunkedReply out(sWeb->server);
  out.begin(200, "text/html");
  pageHead(out, "Network", "/network");
  out += defaultPinBanner();
  /* Signed in, the Hotspot choices save the moment one is chosen, so the
   * page needs the toast for the radio's answer and the script that
   * posts. Signed out it has only plain forms, and neither. */
  const bool pinOk = signedIn();
  if (pinOk) {
    out += pageToast();
  }
  out += F("<div class=page-grid>");
  out += wifiForm(); /* Open on the access point, which is the point of it. */
  if (!pinOk) {
    out += signInForm("/network");
  } else {
    /* The hotspot, behind the PIN like every other setting. */
    static const char *const kHotspotNames[] = {"Auto", "On", "Off"};
    static const long kHotspotValues[] = {WIFI_HOTSPOT_AUTO, WIFI_HOTSPOT_ON,
                                          WIFI_HOTSPOT_OFF};
    out += cardOpen("Hotspot");
    out +=
        F("<p>The radio's own network, at <code>http://192.168.4.1:8080/</"
          "code>. Auto serves it when the radio cannot join your Wi-Fi. On "
          "serves it in place of your Wi-Fi, so this page stops answering "
          "here. Off never serves it; the recovery screen's Start Hotspot "
          "turns it back on.</p>");
    out += formOpen("/api/settings");
    out += formRadioGroup("hsp", "", kHotspotNames, kHotspotValues,
                          WIFI_HOTSPOT_COUNT, sWeb->settings->hotspot,
                          autoAttrs("/api/settings"));
    out += formClose();
    out += cardClose();
    out += cardOpen("Access PIN");
    out +=
        F("<p>Six digits. Changing it takes effect at once and signs "
          "every browser out, this one included.</p>"
          "<form method=post action='/setpin'>"
          "<label>New PIN<input name=pin inputmode=numeric "
          "pattern='[0-9]{6}' maxlength=6 required></label>"
          "<button type=submit>Change PIN</button></form>");
    out += cardClose();
  }
  out += F("</div>");
  out += pageTail(pinOk);
}

/*
 * What the update check found, for the System page, with an Install button
 * that asks first while a newer release is known. A plain form and a native
 * dialog, as the Reboot card has: this page loads no script.
 */
static String updateCard(void) {
  String out;
  switch (updateCheckState()) {
    case UPDATE_STATE_FOUND: {
      char mb[12];
      updateFormatMegabytes(updateCheckSize(), mb, sizeof(mb));
      out += F("<p>Version <strong>");
      out += updateCheckVersion();
      out += F("</strong> is available, ");
      out += mb;
      out +=
          F(" MB.</p><button type=button "
            "onclick='confirmUpdate.showModal()'>Install</button>"
            "<dialog id=confirmUpdate><article>"
            "<header>Install version ");
      out += updateCheckVersion();
      out +=
          F("?</header><p>The radio downloads it from GitHub, writes it "
            "and restarts into it. It keeps the version it has now, and "
            "goes back to it by itself if the new one does not come "
            "up.</p><footer>"
            "<button type=button class=secondary "
            "onclick='confirmUpdate.close()'>Cancel</button>"
            "<form method=post action='/update/install' "
            "style='display:inline'>"
            "<button type=submit>Install</button></form>"
            "</footer></article></dialog>");
      return out;
    }
    case UPDATE_STATE_NONE:
      out +=
          F("<p>Up to date: no newer release at the check of this "
            "start.</p>");
      return out;
    case UPDATE_STATE_WAITING:
      out +=
          F("<p>Not checked yet. The radio looks once it is on the "
            "network.</p>");
      return out;
    case UPDATE_STATE_CHECKING:
      out +=
          F("<p>Looking on GitHub now. Reload the page in a few "
            "seconds.</p>");
      return out;
    case UPDATE_STATE_FAILED:
      out +=
          F("<p class=warn>The check could not be made. It is tried "
            "again at the next start.</p>");
      return out;
    case UPDATE_STATE_OFF:
    default:
      out +=
          F("<p>Check for updates is off. Turn it on in "
            "<a href=/settings>Settings</a>.</p>");
      return out;
  }
}

static void handleSystemPage(void) {
  sWeb->requests++;
  sWeb->server.sendHeader("Cache-Control", "no-store");
  ChunkedReply out(sWeb->server);
  out.begin(200, "text/html");
  pageHead(out, "System", "/system");
  if (!signedIn()) {
    out += signInForm("/system");
    out += pageTail();
    return;
  }

  out += F("<div class=page-grid>");
  out += cardOpen("This image");
  out += F("<table>");
  out += "<tr><td>Firmware</td><td>V" + String(FIRMWARE_VERSION) + "</td></tr>";
  out += "<tr><td>Running from</td><td>" + String(rollbackRunningPartition()) +
         "</td></tr>";
  out += "<tr><td>Image</td><td class=";
  out += rollbackPending() ? F("warn>") : F("ok>");
  out += String(rollbackStateText()) + "</td></tr>";
  const Tef668xCapabilities *tuner = tef668xCapabilities();
  if (tuner != NULL) {
    out += "<tr><td>Tuner</td><td>" + String(tuner->part) + ", patch v" +
           String((unsigned)tuner->patchVersion) + "</td></tr>";
  } else {
    out += "<tr><td>Tuner</td><td class=warn>" +
           String(tef668xErrorText(tunerStartError())) + "</td></tr>";
  }
  out += F("</table>");
  out += cardClose();

  out += cardOpen("How it is doing");
  out += F("<table>");
  out += "<tr><td>Free heap</td><td>" + String(systemHeapFree()) +
         " bytes</td></tr>";
  out += "<tr><td>Up for</td><td>" + String(millis() / 1000UL) +
         " seconds</td></tr>";
  out += "<tr><td>Requests served</td><td>" + String(sWeb->requests) +
         "</td></tr>";
  out += F("</table>");
  out += cardClose();

  out += cardOpen("Firmware");
  out +=
      F("<p>Pick the firmware .bin file. The radio "
        "reboots into it and puts the old one back on its own if it does "
        "not come up.</p>"
        "<form method=post action='/update' enctype='multipart/form-data'>"
        "<input type=file name=firmware accept='.bin' required>"
        "<button type=submit>Upload and reboot</button></form>");
  out += cardClose();

  out += cardOpen("From GitHub releases");
  out += updateCard();
  out += cardClose();

  out += cardOpen("Reboot");
  out +=
      F("<p>The band plan and the knob settings are read at start up, so "
        "this is how they take effect.</p>"
        "<button type=button class=secondary "
        "onclick='confirmReboot.showModal()'>Reboot now</button>"
        /* A native <dialog>, not a script driven confirm(): the browser
         * already has a modal, and `showModal` is a plain method call
         * from the button's own inline handler, no script tier needed
         * for it the way the Radio page's htmx forms are. */
        "<dialog id=confirmReboot><article>"
        "<header>Reboot the radio now?</header>"
        "<p>It rejoins Wi-Fi and comes back up on its own, usually in "
        "under a minute.</p><footer>"
        "<button type=button class=secondary "
        "onclick='confirmReboot.close()'>Cancel</button>"
        "<form method=post action='/reboot' style='display:inline'>"
        "<button type=submit>Reboot</button></form>"
        "</footer></article></dialog>");
  out += cardClose();
  out += F("</div>");

  out += pageTail();
}

void sendResult(int code, const char *title, const char *message, bool bad) {
  sWeb->server.sendHeader("Cache-Control", "no-store");
  ChunkedReply out(sWeb->server);
  out.begin(code, "text/html");
  pageHead(out, title);
  out += F("<h3>");
  out += title;
  out += F("</h3><p class=");
  out += bad ? F("warn>") : F("ok>");
  out += message;
  out += F("</p><p><a href='/'>Back</a></p>");
  out += pageTail();
}

static void handleWifi(void) {
  sWeb->requests++;
  if (!requireAuth(true)) {
    return;
  }
  if (!sWeb->server.hasArg("sid")) {
    sendResult(400, "Nothing saved", "A network name is needed.", true);
    return;
  }

  String ssid = sWeb->server.arg("sid");
  String pass =
      sWeb->server.hasArg("pwd") ? sWeb->server.arg("pwd") : String("");

  Settings pending = *sWeb->settings;
  if (!settingsSetWifi(&pending, ssid.c_str(), pass.c_str())) {
    sendResult(400, "Nothing saved",
               "That network name or passphrase is too long.", true);
    return;
  }
  if (!settingsTaskStore(&pending)) {
    sendResult(500, "Nothing saved", "The settings could not be written.",
               true);
    return;
  }

  Serial.printf("[web] new credentials saved for %s\n", pending.wifiSsid);
  sendResult(200, "Saved",
             "Saved. The radio is trying that network now. If it works it "
             "will be on tef668x.local:8080, and this access point will "
             "stop.",
             false);

  /* Answer first, then move the radio, or the reply never reaches the browser
   * that is connected to the access point being torn down. NetworkClient has
   * no send buffer to flush in this core, so closing the socket is what puts
   * the bytes on the wire. */
  sWeb->server.client().stop();
  delay(200);
  wifiRetryNow();
}

/*
 * Everything this file answers `sWeb->server.on` for. Called once, from
 * `webBegin`, which owns the server the way it owns every other route.
 */
void webPagesRegisterRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/", HTTP_GET, handleRoot);
  sWeb->server.on("/radio", HTTP_GET, handleRadioPage);
  sWeb->server.on("/fm", HTTP_GET, handleFmPage);
  sWeb->server.on("/settings", HTTP_GET, handleSettingsPage);
  sWeb->server.on("/network", HTTP_GET, handleNetworkPage);
  sWeb->server.on("/dx", HTTP_GET, handleDxPage);
  sWeb->server.on("/system", HTTP_GET, handleSystemPage);
  sWeb->server.on("/wifi", HTTP_POST, handleWifi);
}
