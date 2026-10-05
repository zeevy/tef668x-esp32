/*
 * The control API: the stored settings: what is stored, and changing it.
 */
#include "web_api_internal.h"
#include "web_internal.h"

#include "core/clock.h"
#include "core/settings_table.h"
#include "core/squelch.h"
#include "core/theme_colour.h"
#include "drivers/settings_nvs.h"
#include "net/wifi_manager.h"
#include "settings_task.h"
#include "ui/theme.h"
#include "web_update.h"

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

/*
 * GET /api/settings. What is stored, without the secrets.
 *
 * Behind the PIN in both directions. The struct holds the Wi-Fi passphrase
 * and the access PIN, so this says whether each one is set and never what it
 * is. A caller that wants to know the passphrase already has to be standing
 * at the radio.
 *
 *   `sid`  The stored network name, empty when there is none.
 *   `pss`  A passphrase is stored. False is an open network.
 *   `dpn`  The access PIN is still 000000.
 *   `ldd`  The stored settings were read back. False means the defaults.
 *   `sql`  The squelch mode this radio comes up in: Off, Auto or Manual.
 *   `sbd`  The band it comes up on, as a BandId number.
 *   `sfq`  The frequency it comes up on, kHz. 0 is the bottom of the band.
 *   `svl`  The volume it comes up at, dB. Read in manual squelch only.
 *   `ims`, `eq`, `mno`  The stored FM features, 0 or 1.
 *   `cut`, `bld`, `hbl`  The stored weak signal start levels, dBuV.
 *   `fnb`, `anb`  The stored noise blanker starts, percent.
 *   `ahc`, `lhc`  The stored AM high cut starts, MW and SW, then LW, dBuV.
 *   `asm`, `lsm`  The stored AM soft mute starts, MW and SW, then LW, dBuV.
 *   `dem`  The stored FM de-emphasis, in microseconds.
 *   `abw`  The width the AM bands come up on, kHz.
 *   `tzo`  The clock's offset from UTC, such as "+05:30".
 *
 * Then every key of the settings table, as a number, the same keys POST
 * /api/settings takes and describes. Last, `cst`, the custom theme's nine
 * colours as "#rrggbb" text.
 *
 * These are what is stored, which is not always what the radio is set to now.
 * /api/state says what it is set to now. POST /api/save makes the two agree.
 */
static void handleApiSettingsGet(void) {
  sWeb->requests++;
  if (!requireAuth(false)) {
    return;
  }
  const Settings *st = sWeb->settings;
  String out;
  out.reserve(512);
  out += F("{\"sid\":\"");
  out += jsonEscape(st->wifiSsid);
  out += F("\",\"pss\":");
  out += st->wifiPass[0] != '\0' ? F("true") : F("false");
  out += F(",\"dpn\":");
  out += webPinIsDefault() ? F("true") : F("false");
  /* Whether these are the stored settings at all. False means the blob could
   * not be read and the radio is running on the defaults, which otherwise
   * only shows as everything having gone back to how it was. */
  out += F(",\"ldd\":");
  out += settingsWereLoaded() ? F("true") : F("false");
  out += F(",\"sql\":\"");
  out += squelchModeName((SquelchMode)st->squelchMode);
  out += F("\"");
  /* What the radio was last tuned to and how it was set, stored with the
   * rest but changed through their own endpoints, not here. */
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
  out += F(",\"ahc\":");
  out += st->amHighCutStart;
  out += F(",\"lhc\":");
  out += st->lwHighCutStart;
  out += F(",\"asm\":");
  out += st->amSoftMuteStart;
  out += F(",\"lsm\":");
  out += st->lwSoftMuteStart;
  out += F(",\"dem\":");
  out += st->fmDeemphasisUs;
  out += F(",\"abw\":");
  out += st->amBandwidthKHz;
  {
    char tzText[8];
    out += F(",\"tzo\":\"");
    if (clockFormatOffset(st->clockOffsetMinutes, tzText, sizeof(tzText))) {
      out += tzText;
    }
    out += F("\"");
  }
  /* Every setting POST takes by number, from the one table it writes
   * through, so the two cannot name a setting differently. As numbers
   * through String, since a signed byte added to a String goes in as a
   * character. */
  for (size_t i = 0; i < settingsTableCount(); i++) {
    const SettingRow *row = settingsTableAt(i);
    out += F(",\"");
    out += row->key;
    out += F("\":");
    out += String((long)settingsTableGet(st, row));
  }
  /* The custom slot's own nine colours, in the order Theme lists its
   * fields, so the web page's colour wheels can be set from whatever was
   * last saved rather than from Nightwatch's defaults every time the page
   * loads. */
  out += F(",\"cst\":[");
  for (int i = 0; i < THEME_CUSTOM_COLOUR_COUNT; i++) {
    char hex[8];
    themeColourFormat(st->customTheme[i][0], st->customTheme[i][1],
                      st->customTheme[i][2], hex, sizeof(hex));
    if (i > 0) {
      out += F(",");
    }
    out += F("\"");
    out += hex;
    out += F("\"");
  }
  out += F("]}");
  sWeb->server.send(200, "application/json", out);
}

/*
 * POST /api/settings. The things that are stored and not tuned.
 *
 * Takes `sid` with an optional `pwd`, and `pin`, and the settings that are
 * stored rather than tuned. Every row below that is a number is one row of
 * the settings table in core/settings_table.c, which the handler reads and
 * writes through:
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
 * | `ntp` | 0 or 1 | at once | Ask the network for the time. Off takes the clock off the panel |
 * | `bat` | 0 to 2 | at once | The battery in the header: 0 off, 1 per cent, 2 volts |
 * | `agt` | 0, or 30 to 80 | at once | Volume AGC target modulation, per cent. 0 is off |
 * | `agb` | 0 to 8 | at once | The most the AGC may add to a quiet station, dB. 0 is cut only |
 * | `thm` | 0 to `THEME_COUNT` - 1 | at once | The theme drawn by day, 06:00 to 17:59 local time and whenever the time is not known, by saved index: 0 Nightwatch, 1 Daylight, 2 Red Night, 3 Phosphor, 4 Clear, 5 Custom, 6 Slate, 7 Paper, 8 LCD, 9 Ember, 10 Clear Day, 11 High Contrast, 12 Mono, 13 Hi-Fi, 14 Violet, 15 Blossom |
 * | `thn` | 0 to `THEME_COUNT` - 1 | at once | The theme drawn at night, 18:00 to 05:59 local time, by the same index as `thm`. Both the same means the theme never changes with the hour |
 * | `hsp` | 0 to 2 | at once | The radio's own hotspot: 0 Auto, served when the stored network cannot be joined; 1 On, served in place of it; 2 Off, never served. New network details given in `sid` set On back to Auto. A change can take the radio off the network this reply goes out on |
 * | `web` | 0 or 1 | at once | The web server: 0 stops the pages, this API and updates over Wi-Fi, and only the panel's menu turns it on again |
 * | `wif` | 0 or 1 | at once | Wi-Fi at all: 0 leaves the radio on no network and serves no hotspot, and only the panel's menu turns it on again |
 * | `slp` | 0 to 600 | at once | Auto off: minutes alone before the radio sleeps, 0 for never. A new time starts the count again |
 * | `upc` | 0 or 1 | at once | Look on GitHub for a newer release once the radio is on the network, once each start. Turned on, it looks in this start too |
 * | `tof` | 0 or 1 | at once | Touch off: 1 stops the radio reading the touch screen, 0, the default, reads it. Kept the other way round from the other switches so the 0 an older blob holds reads as on |
 * | `rot` | 0 or 180 | at once | The display rotation, in degrees from the way the board is fitted |
 * | `dst` | 0 to 2 | at once | What stops the DX scanner: 0 a NEW PI, 1 any PI, 2 nothing |
 * | `dsc` | 0 to 2 | next scan | What it walks: 0 the band less the stored channels, 1 the whole band, 2 the memory channels |
 * | `dmf` | 1 to 99 | next scan | The memory walk's first channel |
 * | `dml` | `dmf` to 99 | next scan | Its last |
 * | `dlp` | 0 or 1 | at once | Go round the band again rather than stop at the top |
 * | `dmu` | 0 or 1 | at once | Mute between channels |
 * | `dal` | 0 or 1 | at once | Write a NEW catch to the log without being asked |
 * | `ddw` | 5 to 300 | at once | Tenths of a second on each channel |
 * | `dbw` | one of the tuner's FM widths | next DX mode | The width DX mode opens with, kHz |
 * | `rrg` | 0 or 1 | at once | The RDS region: 0 Europe and the rest of the world, 1 North America, where the PI is read as call letters |
 * | `drt` | 0 or 1 | at once | Put the station's radio text in each log entry |
 * | `dwt` | 0 or 1 | at once | Watch the presets in DX mode's range |
 * | `fof`, `aof` | -25 to 15 | at once | FM and AM level offset, whole dB added to every level shown or exported |
 * | `tzo` | `-12:00` to `+14:00` | at once | How far local time is from UTC, written `+05:30`. Not in the table: it is text, so it is read on its own |
 * | `tc0` to `tc8` | `#rrggbb` | at once if Custom is the active theme | The Custom slot's own nine colours, in the order ui/theme.h's Theme struct lists them. Not in the table either, for the same reason `tzo` is not |
 *
 * `tzo` is refused rather than rounded when it cannot be read or is outside
 * that range. An offset quietly moved to the nearest legal one gives a clock
 * that is exactly right for somewhere else, and nobody would spot it. A
 * `tc*` field that does not parse as six hex digits is refused the same way.
 *
 * The table in this comment and the settings table have to agree on the
 * ranges, and nothing checks that they do; the settings table is the one the
 * radio goes by.
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
  sWeb->requests++;
  if (!requireAuth(false)) {
    return;
  }

  bool wantWifi = sWeb->server.hasArg("sid");
  bool wantPin = sWeb->server.hasArg("pin");

  /* Written into a copy, so a request refused part way stores nothing. */
  Settings pending = *sWeb->settings;
  bool wantStored = false;
  bool wantNow = false;
  bool wantReboot = false;
  /* The walk of a DX scan and the width of DX mode are fixed when each
   * starts, so a change to them waits for the next one, and the reply says
   * so rather than "in use now". */
  bool wantDxNext = false;
  for (size_t i = 0; i < settingsTableCount(); i++) {
    const SettingRow *row = settingsTableAt(i);
    if (!sWeb->server.hasArg(row->key)) {
      continue;
    }
    /* Only ui/theme.h knows how many themes ship. */
    const bool theme =
        strcmp(row->key, "thm") == 0 || strcmp(row->key, "thn") == 0;
    long value = 0;
    if (!apiNumber(row->key, &value, row->low,
                   theme ? (long)THEME_COUNT - 1 : (long)row->high)) {
      return;
    }
    /* Named, because `settingsValid` would refuse 90 with a message about
     * the AGC target, which is the one hole it names. */
    if (strcmp(row->key, "rot") == 0 && value != 0 && value != 180) {
      apiFail(400, "The rotation is 0 or 180.");
      return;
    }
    if (strcmp(row->key, "dbw") == 0 &&
        !bandBandwidthAllowed(BAND_FM, (uint16_t)value)) {
      apiFailFmWidth("dbw");
      return;
    }
    settingsTableSet(&pending, row, (int32_t)value);
    wantStored = true;
    if (row->acts == SETTING_ACTS_AT_START) {
      wantReboot = true;
    } else if (row->acts == SETTING_ACTS_NEXT_DX) {
      wantDxNext = true;
    } else {
      wantNow = true;
    }
  }

  /*
   * The UTC offset. Text rather than a number, so it does not fit the table
   * above. Refused outright rather than clamped: an offset silently moved to
   * the nearest legal one would show a clock that is exactly right for
   * somewhere else, which nobody would spot.
   *
   * Read here, before the check below, so that setting only the offset is a
   * complete request rather than an empty one.
   */
  bool wantClock = false;
  int16_t pendingOffset = 0;
  if (sWeb->server.hasArg("tzo")) {
    String tz = sWeb->server.arg("tzo");
    if (!clockParseOffset(tz.c_str(), &pendingOffset)) {
      apiFail(400,
              "The UTC offset has to be written like +05:30 or -08:00, "
              "between -12:00 and +14:00.");
      return;
    }
    wantClock = true;
    wantStored = true;
    wantNow = true;
  }

  /*
   * The Custom slot's nine colours. Text, like `tzo`, so none of them fit
   * the numeric table above, and each is independent: a page that only
   * changed one colour wheel should not have to resend the other eight.
   * In the order ui/theme.h's Theme struct lists its own fields, which is
   * also the order core/settings.h's customTheme array expects them in.
   */
  static const char *const kThemeColourNames[THEME_CUSTOM_COLOUR_COUNT] = {
      "tc0", "tc1", "tc2", "tc3", "tc4", "tc5", "tc6", "tc7", "tc8"};
  bool wantCustom = false;
  uint8_t pendingCustom[13][3];
  memcpy(pendingCustom, sWeb->settings->customTheme, sizeof(pendingCustom));
  for (int i = 0; i < THEME_CUSTOM_COLOUR_COUNT; i++) {
    if (!sWeb->server.hasArg(kThemeColourNames[i])) {
      continue;
    }
    String c = sWeb->server.arg(kThemeColourNames[i]);
    if (!themeColourParse(c.c_str(), &pendingCustom[i][0], &pendingCustom[i][1],
                          &pendingCustom[i][2])) {
      apiFail(400, String(kThemeColourNames[i]) +
                       " has to be a colour written like #rrggbb.");
      return;
    }
    wantCustom = true;
    wantStored = true;
    wantNow = true;
  }

  if (!wantWifi && !wantPin && !wantStored) {
    /* Built from the table this handler validates against, not written out
     * beside it. A list typed by hand drifts every time a setting is added,
     * and then a person correcting a typo against it is told a name the
     * endpoint accepts is not one it takes. `pwd` is accepted with `sid` and
     * is left out on purpose, being half of one answer rather than a
     * setting. */
    String names = F("sid, pin, tzo, tc0..tc8");
    for (size_t i = 0; i < settingsTableCount(); i++) {
      names += F(", ");
      names += settingsTableAt(i)->key;
    }
    apiFail(400, "Give " + names + ", or any mix of them.");
    return;
  }

  uint32_t newPin = 0;

  /* Named, for the same reason as the rotation: settingsValid would refuse
   * it with a message about the AGC target. */
  if (pending.dxMemFirst > pending.dxMemLast) {
    apiFail(400, "dmf, the first preset, has to be at most dml.");
    return;
  }
  if (wantClock) {
    pending.clockOffsetMinutes = pendingOffset;
  }
  if (wantCustom) {
    memcpy(pending.customTheme, pendingCustom, sizeof(pendingCustom));
  }

  if (wantWifi) {
    String ssid = sWeb->server.arg("sid");
    String pass =
        sWeb->server.hasArg("pwd") ? sWeb->server.arg("pwd") : String("");
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
    if (!accessPinParse(sWeb->server.arg("pin").c_str(), &newPin)) {
      apiFail(400, "A PIN is six digits.");
      return;
    }
    pending.accessPin = newPin;
  }

  /* The whole struct, not only what this request touched. A blob that fails
   * this is one the radio would refuse to load at its next start, and that
   * is a radio that comes up on the defaults with no explanation. */
  if (!settingsValid(&pending)) {
    /*
     * Named, because the only field with a hole in its range is the one
     * somebody will trip over. Every other value here is refused by the
     * bounds check above, so a set that reaches this line and fails is almost
     * always an AGC target between 1 and the minimum.
     */
    apiFail(400,
            "Those settings are not a set this radio can use. The volume AGC "
            "target is 0 for off, or between 30 and 80.");
    return;
  }
  /* Read before the store, which makes `pending` the live settings. Any of
   * the three can take the radio off the network this reply goes out on. */
  const bool hotspotMoved = pending.hotspot != sWeb->settings->hotspot ||
                            pending.webEnabled != sWeb->settings->webEnabled ||
                            pending.wifiEnabled != sWeb->settings->wifiEnabled;
  /*
   * One call, shared with the menu: it checks, writes, applies and counts.
   *
   * Two ways into a setting that write it differently is how a radio ends up
   * behaving one way from the panel and another from a browser. The count
   * matters on its own: `asv.n` is what says how many times a 20 KB flash
   * partition has been written, so a write that goes round this call is a
   * write nobody can see.
   */
  if (!settingsTaskStore(&pending)) {
    apiFail(500, "The settings could not be written. Nothing changed.");
    return;
  }

  String said;
  if (wantNow) {
    said += F("Saved and in use now.");
  }
  if (wantDxNext) {
    said += said.length() > 0 ? F(" ") : F("Saved. ");
    said +=
        F("What the DX scanner walks takes effect at its next scan, and "
          "the DX width the next time DX mode opens.");
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
    webPinChanged(newPin);
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
  if (hotspotMoved && !wantWifi) {
    if (said.length() > 0) {
      said += F(" ");
    }
    said += F("Network setting changed, so this address may stop answering.");
  }
  sWeb->server.send(200, "text/plain", said + "\n");

  if (wantWifi || hotspotMoved) {
    /* Answer first, then move the radio, or the reply never reaches a caller
     * on the network being left. Closing the socket is what puts the bytes
     * on the wire. A hotspot change is acted on by the next pass of the
     * Wi-Fi loop. */
    sWeb->server.client().stop();
    delay(200);
  }
  if (wantWifi) {
    wifiRetryNow();
  }
}

void webApiSettingsRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/api/settings", HTTP_GET, handleApiSettingsGet);
  sWeb->server.on("/api/settings", HTTP_POST, handleApiSettingsPost);
}
