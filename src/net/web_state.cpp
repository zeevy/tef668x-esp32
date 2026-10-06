/*
 * The state document: `GET /status.json`, and everything that builds it
 * from a live snapshot.
 */
#include "web_internal.h"

#include "band_scan_task.h"
#include "board/board.h"
#include "build_id.h"
#include "core/band_plan.h"
#include "core/clock.h"
#include "core/dx.h"
#include "core/rds_country.h"
#include "core/squelch.h"
#include "core/version.h"
#include "core/web_text.h"
#include "core/wifi_signal.h"
#include "drivers/battery_adc.h"
#include "drivers/logbook_fs.h"
#include "drivers/tef668x.h"
#include "input_task.h"
#include "lvgl_port.h"
#include "memory_store.h"
#include "net/ntp.h"
#include "net/restart_reason.h"
#include "net/rollback.h"
#include "net/update_check.h"
#include "net/wifi_manager.h"
#include "radio_task.h"
#include "screen_task.h"
#include "settings_task.h"
#include "sleep_task.h"
#include "system_info.h"

#include <WebServer.h>

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

void jsonBool(String &out, const char *key, bool on) {
  out += ",\"";
  out += key;
  out += on ? "\":true" : "\":false";
}

void jsonNum(String &out, const char *key, long long value) {
  out += ",\"";
  out += key;
  out += "\":";
  out += value;
}

String jsonEscape(const char *raw) {
  String out;
  char piece[WEB_ESCAPE_MAX];
  for (const char *p = raw; *p != '\0'; p++) {
    (void)webJsonEscapeChar(*p, piece);
    out += piece;
  }
  return out;
}

/*
 * The RDS decoder, as the `rds` object. FM only.
 *
 * A field that is not here is one the radio cannot answer yet. That is the
 * whole shape of this block: an empty station name and a station name that
 * has not been received are different things, and sending `"ps":""` for the
 * second makes them look the same. So `ps`, `rt`, `pi`, `pty` and `ct` appear
 * only once they have been heard.
 *
 *   `off`  true when the decoder is switched off. Nothing else is here then.
 *   `syn`  The tuner is locked to an RDS bit stream.
 *   `pi`   Programme identifier, four hex digits.
 *   `piz`  The station sends 0000 as its PI, which names no station.
 *   `ecc`  Extended country code, two hex digits.
 *   `cty`  The station's country, two letters, from the ECC and the PI.
 *   `cs`   North American call letters, from RT+ or worked out from the PI.
 *   `csg`  true when `cs` was worked out from the PI and is a guess.
 *   `pih`  The PI last heard but not yet sure, `?` for a digit that changed.
 *   `psp`  The PI stored with the tuned preset, four hex digits.
 *   `pim`  "match" when this is that preset's station, "other" when not.
 *   `blk`  Error level of blocks A to D of the last group: 0 clean, 1 or 2
 *          corrected, 3 not corrected.
 *   `pty`  Programme type, 0 to 31.
 *   `ptn`  What that number is called, in the region the settings name.
 *   `tp`   The station carries traffic announcements at some point.
 *   `ta`   One is on air right now.
 *   `ms`   "speech" or "music".
 *   `ps`   Station name, the eight characters as transmitted.
 *   `psh`  The station name as it builds up, eight characters.
 *   `psm`  Which positions of `psh` have arrived, one bit each.
 *   `psl`  The name stitched back together, when the station splits one
 *          across two passes. The panel shows this in place of `ps`.
 *   `rt`   Radio text.
 *   `af`   Alternative frequencies, in kHz.
 *   `ct`   Date and time the station sent, in its own local time.
 *   `lng`  Programme language code.
 *   `lgn`  The name of that language, when it is known.
 *   `rtp`  RT+: `on` the station sends it, `run` an item is playing, and
 *          `tags`, each with its type `t`, label `l` and words `s`.
 *   `eon`  Other networks heard in two groups or more: `pi`, `ps`, `tp`,
 *          `ta`, `af` in kHz, and `afm`, more frequencies than were kept.
 *   `eom`  Another network was named after the list was full.
 *   `min`  The last minute: `ms` it covers, `grp` groups, `blk` the clean,
 *          corrected and lost counts of blocks A to D, and `typ` the count
 *          of each group type, such as "0A".
 *   `grp`  Groups the tuner has handed over on this station.
 *   `use`  How many of those something was decoded from.
 *   `cor`  Blocks the tuner said it had corrected. Not used, only counted.
 *   `bad`  Blocks the tuner could not correct.
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
  if (r->piZero) {
    out += F(",\"piz\":true");
  }
  if (r->hasEcc) {
    snprintf(small, sizeof(small), ",\"ecc\":\"%02X\"", (unsigned)r->ecc);
    out += small;
  }
  /* Who the station is, by the rule the RDS screen shows it by. */
  const uint16_t stored = memoryStorePi(snap.memorySlot);
  const SeekReading reading =
      radioSeekReading(&snap.quality, snap.qualityValid);
  DxIdentity id;
  dxStationIdentity(r, &reading, (RdsRegion)sWeb->settings->rdsRegion, stored,
                    false, &id);
  if (id.country != NULL) {
    snprintf(small, sizeof(small), ",\"cty\":\"%s\"", id.country);
    out += small;
  }
  /* North America's station name: its own RT+ short name, or
   * call letters worked out from the PI, which `csg` says is a guess.
   * Escaped, since the RT+ name is the station's text. */
  if (id.call != RDS_CALL_NONE) {
    out += F(",\"cs\":\"");
    out += jsonEscape(id.callText);
    out += F("\",\"csg\":");
    out += id.call == RDS_CALL_GUESS ? F("true") : F("false");
  }
  /* The one still being made sure of, left out once it is the answer. */
  char heard[5];
  if (rdsFormatPiHeard(r, heard) &&
      !(r->hasPi && r->piHeard == r->pi && r->piUnsureNibbles == 0) &&
      !(r->piZero && r->piHeard == 0)) {
    snprintf(small, sizeof(small), ",\"pih\":\"%s\"", heard);
    out += small;
  }
  /* The preset's stored PI, and whether this is its station, as the DX page
   * and RDS page 1 say it. */
  if (stored != 0) {
    snprintf(small, sizeof(small), ",\"psp\":\"%04X\"", (unsigned)stored);
    out += small;
    if (id.preset != DX_PRESET_NONE) {
      out += id.preset == DX_PRESET_MATCH ? F(",\"pim\":\"match\"")
                                          : F(",\"pim\":\"other\"");
    }
  }
  if (r->hasBlockErrors) {
    snprintf(small, sizeof(small), ",\"blk\":[%u,%u,%u,%u]",
             (unsigned)r->blockError[0], (unsigned)r->blockError[1],
             (unsigned)r->blockError[2], (unsigned)r->blockError[3]);
    out += small;
  }
  if (r->hasPty) {
    snprintf(small, sizeof(small), ",\"pty\":%u", (unsigned)r->pty);
    out += small;
    out += F(",\"ptn\":\"");
    out +=
        jsonEscape(rdsPtyNameIn(r->pty, (RdsRegion)sWeb->settings->rdsRegion));
    out += F("\"");
  }
  if (r->hasFlags) {
    jsonBool(out, "tp", r->tp);
    jsonBool(out, "ta", r->ta);
    out += F(",\"ms\":\"");
    out += r->speech ? F("speech") : F("music");
    out += F("\"");
  }
  if (r->hasPs) {
    out += F(",\"ps\":\"");
    out += jsonEscape(r->ps);
    out += F("\"");
  }
  /* The name as it builds, with the positions heard as a bit mask, because
   * a space is a real character and cannot also mean "not heard". */
  unsigned heardMask = 0;
  for (int i = 0; i < RDS_PS_LEN; i++) {
    if (r->psHeardHave[i]) {
      heardMask |= 1u << i;
    }
  }
  if (heardMask != 0) {
    char name[RDS_PS_LEN + 1];
    memcpy(name, r->psHeard, RDS_PS_LEN);
    name[RDS_PS_LEN] = '\0';
    out += F(",\"psh\":\"");
    out += jsonEscape(name);
    snprintf(small, sizeof(small), "\",\"psm\":%u", heardMask);
    out += small;
  }
  /* Both, when they differ. `ps` stays exactly the eight characters that were
   * transmitted, so what the station is doing is still visible, and `psl` is
   * what the panel shows. Reporting only the stitched one would hide the
   * stitching from the one place it can be checked. */
  if (r->hasPsLong) {
    out += F(",\"psl\":\"");
    out += jsonEscape(r->psLong);
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
  if (r->hasLanguage) {
    snprintf(small, sizeof(small), ",\"lng\":%u", (unsigned)r->language);
    out += small;
    const char *name = rdsLanguageName(r->language);
    if (name != NULL) {
      out += F(",\"lgn\":\"");
      out += jsonEscape(name);
      out += F("\"");
    }
  }
  /* RT+: whether the station announced it, whether an item is
   * playing, and each tag with its words, which are cut from `rt`. */
  if (r->rtPlus || r->rtPlusCount > 0) {
    out += F(",\"rtp\":{\"on\":");
    out += r->rtPlus ? F("true") : F("false");
    jsonBool(out, "run", r->rtPlusRunning);
    out += F(",\"tags\":[");
    for (uint8_t i = 0; i < r->rtPlusCount; i++) {
      char words[RDS_RT_LEN + 1];
      const bool have = rdsRtPlusText(r, i, words, sizeof(words));
      snprintf(
          small, sizeof(small), "%s{\"t\":%u,\"l\":\"%s\",\"s\":", i ? "," : "",
          (unsigned)r->rtPlusTag[i].type, rdsRtPlusLabel(r->rtPlusTag[i].type));
      out += small;
      if (have) {
        out += F("\"");
        out += jsonEscape(words);
        out += F("\"}");
      } else {
        out += F("null}");
      }
    }
    out += F("]}");
  }
  /* The other networks heard in two groups or more, as the panel lists
   * them. */
  bool anyEon = false;
  for (uint8_t i = 0; i < r->eonCount; i++) {
    const RdsEon *e = &r->eon[i];
    if (e->heard < 2) {
      continue;
    }
    out += anyEon ? F(",") : F(",\"eon\":[");
    anyEon = true;
    snprintf(small, sizeof(small), "{\"pi\":\"%04X\",\"ps\":", e->pi);
    out += small;
    if (e->hasPs) {
      out += F("\"");
      out += jsonEscape(e->ps);
      out += F("\"");
    } else {
      out += F("null");
    }
    out += F(",\"tp\":");
    out += !e->hasTp ? F("null") : e->tp ? F("true") : F("false");
    out += F(",\"ta\":");
    out += !e->hasTa ? F("null") : e->ta ? F("true") : F("false");
    jsonBool(out, "afm", e->afMore);
    out += F(",\"af\":[");
    for (uint8_t f = 0; f < e->afCount; f++) {
      snprintf(small, sizeof(small), "%s%u", f ? "," : "",
               (unsigned)rdsAfCodeKHz(e->afCode[f]));
      out += small;
    }
    out += F("]}");
  }
  if (anyEon) {
    out += F("]");
  }
  if (r->eonMore) {
    out += F(",\"eom\":true");
  }
  /* The last minute the decoder page shows: how long it covers, the groups,
   * each block's clean, corrected and lost counts, and the group types. */
  const RdsMinute *m = &r->minute;
  snprintf(small, sizeof(small), ",\"min\":{\"ms\":%u,\"grp\":%u,\"blk\":[",
           (unsigned)m->spanMs, (unsigned)m->groups);
  out += small;
  for (int b = 0; b < 4; b++) {
    snprintf(small, sizeof(small), "%s[%u,%u,%u]", b ? "," : "",
             (unsigned)m->blocks[b][RDS_LEVEL_CLEAN],
             (unsigned)m->blocks[b][RDS_LEVEL_CORRECTED],
             (unsigned)m->blocks[b][RDS_LEVEL_LOST]);
    out += small;
  }
  out += F("],\"typ\":{");
  bool anyType = false;
  for (int t = 0; t < 16; t++) {
    for (int v = 0; v < 2; v++) {
      if (m->types[t][v] == 0) {
        continue;
      }
      snprintf(small, sizeof(small), "%s\"%u%c\":%u", anyType ? "," : "",
               (unsigned)t, v ? 'B' : 'A', (unsigned)m->types[t][v]);
      out += small;
      anyType = true;
    }
  }
  out += F("}}");
  snprintf(small, sizeof(small),
           ",\"grp\":%u,\"use\":%u,\"cor\":%u,\"bad\":%u}",
           (unsigned)r->groupsSeen, (unsigned)r->groupsUsed,
           (unsigned)r->blocksCorrected, (unsigned)r->blocksBad);
  out += small;
}

/*
 * Append everything about the radio and its tuner, as the `tun` object.
 *
 * One builder, used by both /api/state and /status.json, so the two
 * cannot drift apart. The caller supplies any leading comma.
 *
 * When the tuner started:
 *
 *   `prt`  Which TEF668x is fitted.
 *   `pch`  Tuner firmware version loaded into it.
 *   `xad`  What the crystal sense pin read, raw.
 *   `xtl`  Which crystal that maps to, or "not read".
 *   `fsi`  Has FM stereo improvement.
 *   `frd`  Has full search RDS.
 *   `dr`   Has digital radio.
 *   `bnd`  The band's name.
 *   `khz`  Tuned frequency, kHz.
 *   `f`    The same frequency as the panel writes it, in `unt`.
 *   `unt`  The unit of `f`, MHz or kHz.
 *   `stp`  Tuning step, kHz.
 *   `bws`  Bandwidth the radio is set to, kHz, 0 for automatic on FM. `bw`
 *          is what the tuner actually applies.
 *   `dxw`  DX mode's fixed width while DX mode is open, kHz, 0 when it is
 *          not. It is in force over `bws` on FM and never saved.
 *   `vol`  Volume, dB.
 *   `mut`  Muted, as the person asked for it.
 *   `tmd`  Tuning mode: Manual, Auto, Presets or Meter band.
 *   `pst`  The preset the radio is on, counted from 1, 0 for none.
 *   `seq`  Goes up with every new snapshot. A stalled radio task stops it.
 *   `ims`  Multipath suppression, iMS on the old radio.
 *   `eq`   Channel equalizer.
 *   `mno`  Stereo refused on purpose.
 *   `dem`  FM de-emphasis, microseconds: 50, 75, or 0 for none.
 *   `fnb`  FM noise blanker start, percent, 0 for off.
 *   `anb`  AM noise blanker start, percent, 0 for off.
 *   `ahc`  MW and SW high cut start, dBuV, 0 for off.
 *   `lhc`  LW high cut start, dBuV, 0 for off.
 *   `asm`  MW and SW soft mute start, dBuV.
 *   `lsm`  LW soft mute start, dBuV.
 *   `snr`  Signal to noise, worked out and not read from the chip, dB.
 *   `cut`  The treble roll off the chip is applying now. No known unit.
 *   `bld`  The stereo blend it is applying now. No known unit.
 *   `hbl`  The combined blend. No known unit.
 *   `wid`  The adaptive filter is allowed to open.
 *   `skg`  A seek is running.
 *   `skf`  The last seek found a station.
 *   `bep`  A tone is sounding now.
 *   `sql`  Squelch mode: Off, Auto or Manual.
 *   `sqo`  The squelch is letting sound through.
 *   `hmu`  The mute the tuner was told, against `mut`, which is what was
 *          asked.
 *   `sqa`  The manual squelch threshold, tenths of a dBuV. Manual only.
 *   `per`  What the tuner last refused, in words. Only when it did.
 *   `sig`  Signal level as it came off the chip, tenths of a dBuV.
 *   `sav`  The same level smoothed, tenths of a dBuV.
 *   `usn`  Ultrasonic noise, tenths of a percent.
 *   `wam`  On FM, multipath, what the chip calls weighted AM. On AM,
 *          co-channel. Tenths of a percent.
 *   `off`  How far off centre the station is, tenths of a kHz.
 *   `bw`   Bandwidth the tuner settled on, kHz.
 *   `mod`  Modulation depth, percent.
 *   `st`   You are hearing stereo.
 *   `plt`  The station is transmitting a stereo pilot.
 *   `qst`  The quality status word as the chip sent it, which carries the
 *          time stamp saying whether the readings have settled.
 *   `lvo`  The level offset in force on this band, whole dB.
 *   `agc`  The volume AGC: `on`, `gn`, `avg` and `set`, described where it
 *          is built.
 *   `rds`  The RDS decoder, FM only, described on appendRdsState.
 *   `scn`  A band scan is running.
 *   `scd`  Channels the running scan has looked at so far.
 *   `sct`  Channels the running scan will look at in total.
 *   `scf`  Channels the last finished scan found.
 *   `sca`  Of those, how many were new and written into a free slot.
 *   `scr`  New ones with no free slot left to take them.
 *   `scc`  The last finished scan covered the whole band. False if a tune
 *          stopped it. Null until a scan has finished.
 *
 * When the tuner did not start, so a fault can be read without a serial
 * cable:
 *
 *   `err`  What stopped it, in words.
 *   `dev`, `hwd`, `swd`  The three identification words, hex.
 *   `saw`  Something acknowledged at the I2C address.
 *   `rbt`  The operation status came back.
 *   `bot`  What it said. 0 means not patched yet.
 *   `ptd`  A patch was written this boot.
 *   `try`  Which patch version was written.
 *   `wnt`  Which one the chip then asked for.
 *   `xad`  What the crystal sense pin read, raw.
 *   `xtl`  Which crystal that maps to.
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
    jsonBool(out, "fsi", tuner->hasStereoImprovement);
    jsonBool(out, "frd", tuner->hasFullSearchRds);
    jsonBool(out, "dr", tuner->hasDigitalRadio);
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
                 ",\"stp\":%u,\"bws\":%u,\"dxw\":%u,\"vol\":%d,\"mut\":%s"
                 ",\"tmd\":\"%s\""
                 ",\"pst\":%d,\"seq\":%u",
                 bandName(snap.settings.band), (unsigned)snap.settings.freqKHz,
                 freqText, bandFrequencyUnit(snap.settings.band),
                 (unsigned)snap.settings.stepKHz,
                 (unsigned)snap.settings.bandwidthKHz,
                 (unsigned)snap.settings.dxBandwidthKHz, snap.settings.volumeDb,
                 snap.settings.muted ? "true" : "false",
                 tuneModeName(snap.settings.tuneMode),
                 snap.memorySlot == MEMORY_NO_SLOT ? 0 : snap.memorySlot + 1,
                 (unsigned)snap.sequence);
        out += tuned;
      }
      /* The FM features, none of which turns itself on. */
      jsonBool(out, "ims", snap.settings.multipathSuppression);
      jsonBool(out, "eq", snap.settings.equalizer);
      jsonBool(out, "mno", snap.settings.forcedMono);
      /* The de-emphasis the tuner is set to. It is written on every start, so
       * without it here there is no way to read back what the chip has. */
      jsonNum(out, "dem", snap.settings.deemphasisUs);
      jsonNum(out, "fnb", snap.settings.fmNoiseBlankerStart);
      jsonNum(out, "anb", snap.settings.amNoiseBlankerStart);
      jsonNum(out, "ahc", snap.settings.amHighCutStart);
      jsonNum(out, "lhc", snap.settings.lwHighCutStart);
      jsonNum(out, "asm", snap.settings.amSoftMuteStart);
      jsonNum(out, "lsm", snap.settings.lwSoftMuteStart);
      jsonNum(out, "snr", snap.quality.snrDb);
      if (snap.processingValid) {
        /* What the chip is applying now, not what it was told. */
        jsonNum(out, "cut", snap.processing.highCut);
        jsonNum(out, "bld", snap.processing.stereo);
        jsonNum(out, "hbl", snap.processing.stHiBlend);
      }
      jsonBool(out, "wid", snap.bandwidthWide);
      /* Whether the dial is moving on its own. A caller that cannot tell
       * seeking from a person turning the knob shows the same thing for
       * both. */
      jsonBool(out, "skg", snap.seeking);
      jsonBool(out, "skf", snap.seekFound);
      jsonBool(out, "bep", snap.beeping);

      /* The squelch, so a radio that has gone quiet says why. */
      out += F(",\"sql\":\"");
      out += squelchModeName(snap.squelchMode);
      out += F("\",\"sqo\":");
      out += snap.squelchOpen ? F("true") : F("false");
      jsonBool(out, "hmu", snap.tunerMuted);
      if (snap.squelchMode == SQUELCH_MANUAL) {
        jsonNum(out, "sqa", snap.squelchThresholdTenths);
      }

      /* What the tuner last refused. Without this the page can show a station
       * the radio is not actually on, with nothing to say so. */
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
               ",\"bw\":%u,\"mod\":%d,\"st\":%s,\"plt\":%s,\"qst\":%u",
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
               q.stereo ? "true" : "false", (unsigned)q.status);
      out += sig;
    }
    /* The level offset in force on this band, whole dB. `sig` and `sav`
     * are the radio's own; a page shows them with this added. */
    if (haveSnap) {
      char lvo[16];
      snprintf(lvo, sizeof(lvo), ",\"lvo\":%d",
               (int)screenTaskLevelOffsetDb(snap.settings.band));
      out += lvo;
    }

    /*
     * The volume AGC, as a block of its own.
     *
     * Present whether it is on or off, because "off" is the answer to what it
     * is doing and leaving the block out would make an AGC that is off look
     * like a firmware that does not have one. `gn` is what it is adding to
     * the volume this moment, `avg` the running average of the modulation it
     * works from in tenths of a per cent, and `set` whether that average has
     * seen enough of this station to be worth acting on.
     *
     * This exists because a working AGC and one that decided to do nothing
     * are the same thing from outside.
     */
    if (haveSnap) {
      char agc[80];
      snprintf(agc, sizeof(agc),
               ",\"agc\":{\"on\":%s,\"gn\":%d,\"avg\":%d,\"set\":%s}",
               snap.agcOn ? "true" : "false", snap.agcGainDb,
               snap.agcAverageTenths, snap.agcSettled ? "true" : "false");
      out += agc;
    }

    /* FM only. There is no RDS on the AM side, and an empty block there says
     * nothing that `bnd` does not already say. */
    if (haveSnap && bandModulation(snap.settings.band) == MODULATION_FM) {
      appendRdsState(out, snap);
    }

    /*
     * The band scan: whether one is running, how far it has got, and what
     * the last one to finish found. Always present, the same reasoning as
     * the AGC block above: a scan that has never run and one that found
     * nothing both say `scf 0`, and only having the field at all when a
     * scan is running would make the two impossible to tell apart from a
     * caller that missed the window.
     */
    {
      uint16_t scanDone = 0;
      uint16_t scanTotal = 0;
      bandScanProgress(&scanDone, &scanTotal);
      BandScanResult last;
      const bool scanRan = bandScanLastResult(&last);
      char scan[128];
      snprintf(scan, sizeof(scan),
               ",\"scn\":%s,\"scd\":%u,\"sct\":%u,\"scf\":%u,\"sca\":%u,"
               "\"scr\":%u,\"scc\":%s",
               bandScanActive() ? "true" : "false", (unsigned)scanDone,
               (unsigned)scanTotal, (unsigned)last.found, (unsigned)last.added,
               (unsigned)last.noRoom,
               !scanRan ? "null" : (last.complete ? "true" : "false"));
      out += scan;
    }
    out += F("}");
  } else {
    /* Say why, not just that it failed. Without this the only way to find out
     * is the serial cable. */
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
 * The input layer, as the `inp` object.
 *
 * This is how a dead switch is told from a wrong pin number without opening
 * the radio. A press that shows up here but does nothing to the radio is a
 * mapping problem; a press that never shows up at all is wiring.
 *
 *   `pad`  The keypad expander answered at start up.
 *   `clk`  Knob clicks since boot.
 *   `prs`  Button and key events since boot.
 *   `lst`  The last event in words, such as "BAND long".
 *   `lms`  When that was, ms since boot. 0 for never.
 *   `typ`  Digits keyed and not yet entered.
 *   `lns`  The keypad's sixteen raw lines, a 0 bit for a key held. 65535
 *          until they have been read.
 *   `pot`  The volume knob, 0 to 4095.
 *   `pdb`  The volume that reading was turned into, in dB.
 *   `pcl`  While the knob is being calibrated, the lowest and highest
 *          readings so far, as `min` and `max`. Not there otherwise.
 *   `tch`  The touch chip, on a board that has one. `on` true while the
 *          chip is read: the Touch setting is on, or the calibration screen
 *          is open. False otherwise; then `pen` stays false and nothing
 *          else moves. `pen` true while its pen line says a finger is down,
 *          `dn` times that line went down since boot, `rd` readings taken
 *          since boot, and `x`, `y`, `z1` and `z2` the last reading, raw 0
 *          to 4095, null before the first. `cal` is `stored` when a
 *          calibration a person made is in use, `board` for the board's
 *          own; `px` and `py` are the last steady point in screen pixels
 *          by it, null before the first.
 *          Readings are taken only while a finger is down, at the loop's
 *          own pace, so a tap shorter than one pass of the loop can be
 *          missed by every one of these. The last reading can be one taken
 *          as the finger lifted, far from where it was.
 */
static void appendInputState(String &out) {
  InputStatus in;
  inputStatusGet(&in);
  out += F("\"inp\":{\"pad\":");
  out += in.keypadPresent ? F("true") : F("false");
  jsonNum(out, "clk", in.clicks);
  jsonNum(out, "prs", in.presses);
  out += F(",\"lst\":\"");
  out += jsonEscape(in.lastEvent);
  out += F("\",\"lms\":");
  out += String(in.lastEventMs);
  out += F(",\"typ\":\"");
  out += jsonEscape(in.typed);
  out += F("\",\"lns\":");
  out += String(in.linesOk ? in.lines : 0xFFFF);
  jsonNum(out, "pot", in.pot);
  jsonNum(out, "pdb", in.potDb);
  /* The calibration, while one is running, so the page can show the knob
   * reaching further as it is turned. Without it the person has no sign that
   * turning the knob is doing anything, because during a calibration it
   * deliberately does not change the volume. */
  uint16_t calMin = 0;
  uint16_t calMax = 0;
  if (inputPotCalibrating(&calMin, &calMax)) {
    out += F(",\"pcl\":{\"min\":");
    out += String(calMin);
    jsonNum(out, "max", calMax);
    out += F("}");
  }
#if FEATURE_TOUCH
  out += F(",\"tch\":{\"on\":");
  out += in.touchOn ? F("true") : F("false");
  jsonBool(out, "pen", in.touchPen);
  jsonNum(out, "dn", in.touchDowns);
  jsonNum(out, "rd", in.touchReads);
  out += F(",\"cal\":\"");
  out += in.touchCalStored ? F("stored") : F("board");
  out += F("\"");
  if (in.touchMapped) {
    jsonNum(out, "px", (int)in.touchAt.x);
    jsonNum(out, "py", (int)in.touchAt.y);
  } else {
    out += F(",\"px\":null,\"py\":null");
  }
  if (in.touchReads > 0) {
    char raw[64];
    snprintf(raw, sizeof(raw), ",\"x\":%u,\"y\":%u,\"z1\":%u,\"z2\":%u}",
             (unsigned)in.touch.x, (unsigned)in.touch.y, (unsigned)in.touch.z1,
             (unsigned)in.touch.z2);
    out += raw;
  } else {
    out += F(",\"x\":null,\"y\":null,\"z1\":null,\"z2\":null}");
  }
#endif
  out += F("}");
}

/*
 * Where the network has got to, in a word.
 *
 * All four states, not two. A radio still trying and a radio that has given
 * up and put its own network up look the same from outside, and the
 * difference decides whether waiting is worth anything. `offline` matters for
 * the same reason: it means the access point would not start either, so the
 * radio is on nothing at all and nobody can reach it to find that out.
 */
static const char *netStateText(void) {
  switch (wifiState()) {
    case WIFI_STATE_OFFLINE:
      return "offline";
    case WIFI_STATE_JOINING:
      return "joining";
    case WIFI_STATE_ONLINE:
      return "station";
    case WIFI_STATE_ACCESS_POINT:
      return "ap";
  }
  return "offline";
}

/*
 * Build the whole state document, device and tuner together.
 *
 * One builder for both /status.json and /api/state. They are the same
 * document under two names. Two names for one document is better than two
 * documents, which is what a subset would become the first time a field is
 * added to only one of them.
 *
 * Keys are short on purpose. Every number is an integer. Anything with a
 * fraction is sent in tenths, so nothing has to parse a float and no
 * precision is lost.
 *
 *   `brd`   Board id.
 *   `ver`   Firmware version.
 *   `bid`   The git commit the image was built from, with a + when the
 *           sources had changes not yet committed. Empty when not known.
 *   `slt`   Application partition this image booted from.
 *   `cnf`   The image passed its self check and will not roll back.
 *   `upd`   The update check. `st` is `off`, `wait`, `checking`, `none`,
 *           `found` or `failed`; `ver` and `sz` are the newer release's
 *           version and image size in bytes while `st` is `found`, null
 *           otherwise.
 *   `net`   `offline`, `joining`, `station` or `ap`.
 *   `rssi`  The joined network's signal, dBm. Null unless `net` is
 *           `station`.
 *   `bars`  `rssi` as the header draws it, 0 to 3. Null when `rssi` is.
 *   `rst`   Why the radio last started.
 *   `ip`    Address it can be reached on.
 *   `dpn`   The access PIN is still 000000.
 *   `hep`   Free heap, bytes.
 *   `hmn`   The lowest the free heap has been since boot, bytes.
 *   `hmp`   The lowest it went in the boot before, bytes, when that boot
 *           ended in a restart this firmware made and noted, such as the
 *           one after an update. Null after a power cycle or a crash.
 *   `hlb`   The largest free piece of the heap, the biggest allocation that
 *           can succeed, bytes.
 *   `stk`   Stack bytes each task has never reached since boot: `rad` the
 *           radio task, `lop` the loop task.
 *   `up`    Time since boot, seconds.
 *   `slp`   Seconds until auto off, or null when auto off is off.
 *   `pnl`   The panel, below.
 *   `bat`   The battery, below.
 *   `clk`   The clock, below.
 *   `asv`   The automatic save, below.
 *   `pst`   The preset list, below.
 *   `log`   The logbook, below.
 *   `inp`   The input layer, described on appendInputState.
 *   `tun`   The tuner, described on appendRadioState.
 *
 * Inside `pnl`, the panel:
 *
 *   `lit`  How bright the panel is now, percent.
 *   `dim`  It has been left alone long enough to have dropped.
 *   `sdb`  The signal number on the panel, held still, whole dBuV.
 *   `swp`  How long the last screen change took, ms, 0 if none yet.
 *   `lvu`  LVGL heap in use, bytes, or null when LVGL is not running.
 *   `lvt`  How big that heap is, bytes.
 *   `lvp`  How much of it is in use, percent.
 *   `lvb`  The largest piece still free, which is what fragmentation eats,
 *          bytes.
 *   `lvm`  The most LVGL has had in use since boot, as it counts its
 *          blocks, bytes.
 *   `psh`  The pushes to the panel in the last whole second, or null before
 *          one has passed: `n` how many, `px` their pixels, `us` their time
 *          in all and `max` the longest, in microseconds, the SPI push alone
 *          with the wait for the bus, and `ref` the longest refresh, LVGL
 *          drawing and pushing together.
 *
 * Inside `bat`, the battery. `mv` is null once Wi-Fi owns the converter,
 * ADC2, so `boot` is the only reading that lasts:
 *
 *   `fit`   This board has a sense pin at all.
 *   `mv`    A reading taken now, mV, or null when the converter refused.
 *   `boot`  The one reading taken at start up, mV, or null.
 *
 * Inside `clk`, the time. Null rather than a guess, because this board has
 * no battery backed clock:
 *
 *   `syn`  A server has answered and the time is being kept.
 *   `age`  Seconds since the last answer, or null if none ever came.
 *   `now`  Local time as `HH:MM`, or null when nothing has answered.
 *
 * Inside `asv`, the automatic save. Not to be confused with `sav` in `tun`,
 * which is the smoothed signal level:
 *
 *   `n`    Automatic saves written since boot.
 *   `dif`  What a save would store now is not what is stored. During a seek
 *          or a band scan that is the station it started from.
 *   `due`  How long until a save, ms, 0 when none is waiting. Held at the
 *          full wait for as long as a seek or a DX scan runs, since no
 *          automatic save is made then.
 *   `bad`  The last automatic write was refused or failed.
 *   `idl`  The wait in use, ms. 0 means it is switched off.
 *
 * Inside `pst`, the preset list:
 *
 *   `n`     Presets stored.
 *   `bad`   The last write to the list failed.
 *   `lost`  Presets start up threw away as unreadable.
 *
 * Inside `log`, the logbook:
 *
 *   `fit`  The logbook partition mounted.
 *   `n`    Entries in the log.
 */

String buildState(void) {
  String out;
  out.reserve(768);
  out += F("{\"brd\":\"" BOARD_NAME "\",\"ver\":\"" FIRMWARE_VERSION
           "\",\"bid\":\"" FIRMWARE_BUILD "\",\"slt\":\"");
  out += rollbackRunningPartition();
  out += F("\",\"cnf\":");
  out += rollbackPending() ? F("false") : F("true");
  out += F(",\"upd\":{\"st\":\"");
  out += updateCheckStateName();
  out += F("\",\"ver\":");
  if (updateCheckVersion() != NULL) {
    out += F("\"");
    out += updateCheckVersion();
    out += F("\",\"sz\":");
    out += String((unsigned long)updateCheckSize());
  } else {
    out += F("null,\"sz\":null");
  }
  out += F("}");
  out += F(",\"net\":\"");
  out += netStateText();
  out += F("\",\"rssi\":");
  /*
   * The joined network's signal, in dBm, and the bars the header is
   * actually drawing from it. `null` rather than left out: a station radio
   * and one serving its own access point are both real states, and only one
   * of them has a signal to report, so the field says so rather than
   * disappearing the way a station-only reading would if this just skipped
   * writing it.
   */
  int8_t rssi = 0;
  if (wifiRssiDbm(&rssi)) {
    out += String((int)rssi);
    jsonNum(out, "bars", (unsigned)wifiSignalBars(rssi));
  } else {
    out += F("null,\"bars\":null");
  }
  out += F(",\"rst\":\"");
  /*
   * Why the radio last started. Without it a restart in the field can only be
   * guessed at, and the five the firmware performs itself all look the same
   * to the chip.
   */
  out += restartReasonText();
  out += F("\",\"ip\":\"");
  out += wifiAddress();
  /* Closing the address string, then the separator. Kept here rather than
   * fused onto the front of the next field, so that moving that field into
   * its own builder cannot take the quote with it. */
  out += F("\",\"dpn\":");
  out += webPinIsDefault() ? F("true") : F("false");
  jsonNum(out, "hep", systemHeapFree());
  /* The heap's worst case, not only where it is now: the lowest it has
   * been since boot, and the largest piece left, which is what a big
   * allocation needs and fragmentation eats. */
  jsonNum(out, "hmn", systemHeapLowest());
  /* The boot before's, for an update: the radio answers nothing while one
   * is written and restarts at once, so its low point shows only here. */
  uint32_t lastLowest = 0;
  out += F(",\"hmp\":");
  out +=
      restartReasonLastHeap(&lastLowest) ? String(lastLowest) : String("null");
  jsonNum(out, "hlb", systemHeapLargest());
  /* The bytes of each task's stack it has never reached since boot. The
   * web server's requests run in the loop task, so the loop's own mark is
   * the one this request is running on. */
  out += F(",\"stk\":{\"rad\":");
  out += String(radioTaskStackFree());
  jsonNum(out, "lop", (uint32_t)uxTaskGetStackHighWaterMark(NULL));
  out += F("}");
  jsonNum(out, "up", millis() / 1000UL);
  /* Seconds until auto off sends the radio to sleep, and null when it is
   * off, so never is not read as no time left. */
  uint32_t sleepLeftS = 0;
  out += F(",\"slp\":");
  out += sleepTaskLeftS(&sleepLeftS) ? String(sleepLeftS) : String("null");
  /* What the panel light is doing. A dim and a wake are both silent, so
   * without this the only way to check either is to sit and watch the
   * radio. */
  uint8_t lit = 0;
  bool dimmed = screenTaskBacklightState(&lit);
  out += F(",\"pnl\":{\"lit\":");
  out += String((int)lit);
  jsonBool(out, "dim", dimmed);
  /* What the panel is actually showing for the signal, which is not the
   * smoothed level: the screen holds its number until the level moves a
   * whole dB away. Without this the only way to check that it sits still is
   * to stand at the radio and watch. */
  jsonNum(out, "sdb", (int)screenTaskSignalShown());
  /*
   * LVGL's own heap, which is not the ESP32 heap above.
   *
   * There is no way to look at the panel from here, so this is how a screen
   * that has quietly run out of memory is told apart from one that is fine.
   * LVGL stops drawing when its pool is full, and a frozen screen on a radio
   * that still answers every request looks exactly like dead hardware.
   */
  uint32_t lvUsed = 0;
  uint32_t lvTotal = 0;
  uint8_t lvPct = 0;
  uint32_t lvBiggest = 0;
  uint32_t lvPeak = 0;
  if (lvglPortMemory(&lvUsed, &lvTotal, &lvPct, &lvBiggest, &lvPeak)) {
    char lv[96];
    snprintf(lv, sizeof(lv),
             ",\"swp\":%u,\"lvu\":%u,\"lvt\":%u,\"lvp\":%u,\"lvb\":%u,"
             "\"lvm\":%u",
             (unsigned)screenTaskSwapMs(), (unsigned)lvUsed, (unsigned)lvTotal,
             (unsigned)lvPct, (unsigned)lvBiggest, (unsigned)lvPeak);
    out += lv;
  } else {
    /* Said outright. An absent block would read as a radio with no panel,
     * and this radio has one. */
    out += F(",\"lvu\":null");
  }
  /* The pushes to the panel over the last whole second. The loop that runs
   * the screen also runs this web server and waits for every push, so this
   * is how a screen that keeps it busy is seen from here. Null until a whole
   * second has passed, which is not the same as a second with no push. */
  PushSecond pushes;
  if (lvglPortPushes(&pushes)) {
    char ps[96];
    snprintf(ps, sizeof(ps),
             ",\"psh\":{\"n\":%u,\"px\":%u,\"us\":%u,\"max\":%u,"
             "\"ref\":%u}",
             (unsigned)pushes.pushes, (unsigned)pushes.pixels,
             (unsigned)pushes.busyUs, (unsigned)pushes.longestUs,
             (unsigned)pushes.longestRefreshUs);
    out += ps;
  } else {
    out += F(",\"psh\":null");
  }
  out += F("}");
  /*
   * The battery, whatever the display setting says.
   *
   * Reported here even when the panel is told not to show it, because this is
   * where it gets checked. The divider ratio has never been measured off this
   * unit, and the only way to find out is to read what the radio thinks
   * against what a meter says. `mv` null means no reading: no battery, or the
   * Wi-Fi driver had the converter, and nothing here can tell those apart.
   */
  out += F(",\"bat\":{\"fit\":");
  out += batteryAdcFitted() ? F("true") : F("false");
  out += F(",\"mv\":");
  {
    uint16_t mv = 0;
    if (batteryAdcRead(&mv)) {
      out += String((unsigned)mv);
    } else {
      out += F("null");
    }
  }
  out += F(",\"boot\":");
  {
    uint16_t bootMv = 0;
    if (batteryAdcAtBoot(&bootMv)) {
      out += String((unsigned)bootMv);
    } else {
      out += F("null");
    }
  }
  out += F("}");
  /*
   * The clock.
   *
   * `syn` is whether the radio has the time now, `age` is how many seconds
   * since a server last answered, and `now` is what the panel is showing.
   * The age is the one worth having: the ESP32 keeps counting on its own
   * after Wi-Fi drops, so a clock that nothing has corrected for six hours
   * looks exactly like one that was set a second ago, and the panel cannot
   * tell the difference either. `age` is null when no server has answered
   * during this run, which is not the same as an age of zero.
   */
  out += F(",\"clk\":{\"syn\":");
  out += ntpSynchronised() ? F("true") : F("false");
  out += F(",\"age\":");
  if (ntpEverSynced()) {
    out += String(ntpSecondsSinceSync());
  } else {
    out += F("null");
  }
  out += F(",\"now\":");
  {
    char nowText[CLOCK_TEXT_LEN];
    if (clockFormat(ntpLocalTime(), nowText, sizeof(nowText))) {
      out += F("\"");
      out += nowText;
      out += F("\"");
    } else {
      out += F("null");
    }
  }
  out += F("}");
  /* What the automatic save is doing. A save that never fires and one that
   * fires constantly both look the same from outside, and the second only
   * shows up years later as a worn out sector. */
  SettingsSaveStatus save;
  settingsTaskStatus(&save);
  out += F(",\"asv\":{\"n\":");
  out += String(save.saves);
  jsonBool(out, "dif", save.differs);
  jsonNum(out, "due", save.dueInMs);
  jsonBool(out, "bad", save.lastFailed);
  jsonNum(out, "idl", save.idleMs);
  out += F("}");
  /* The channel list. A write that quietly fails leaves a radio that looks
   * normal and forgets every channel at the next power cycle. */
  out += F(",\"pst\":{\"n\":");
  out += String(memoryStoreCount());
  jsonBool(out, "bad", memoryStoreFailed());
  /* Channels start up threw away. A list that came back short says nothing
   * about itself, so without this the count above is the only sign and it
   * reads as a list that was always that length. */
  jsonNum(out, "lost", memoryStoreCleared());
  out += F("}");
  /* The logbook. `fit` false is the one condition GET /api/log cannot say
   * on its own: an empty log and a partition that never mounted both read
   * as zero entries there, so this is the only place a person can tell
   * them apart. */
  out += F(",\"log\":{\"fit\":");
  out += logbookFsPresent() ? F("true") : F("false");
  jsonNum(out, "n", logbookFsCount());
  out += F("}");
  out += F(",");
  appendInputState(out);
  out += F(",");
  appendRadioState(out);
  out += F("}");
  return out;
}

/* The radio's state as JSON, for scripts and for checks made by hand. */
static void handleStatusJson(void) {
  sWeb->server.send(200, "application/json", buildState());
}

/* Everything this file answers `sWeb->server.on` for. */
void webStateRegisterRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/status.json", HTTP_GET, handleStatusJson);
}
