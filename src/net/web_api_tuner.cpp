/*
 * The control API: the tuner's own controls and readings: squelch, the FM
 * processing, the seek settle readings and raw RDS.
 */
#include "web_api_internal.h"
#include "web_internal.h"

#include "radio_task.h"

/* The server, the settings and the counts, handed over as the routes are
 * registered. */
static WebContext *sWeb = NULL;

/*
 * POST /api/seek/settle. What a seek would decide on, at a given settle time.
 *
 * Takes `khz` and `ms`, and optionally `n` readings `gap` milliseconds apart,
 * one by default. Tunes there, waits that long, takes the readings off the
 * tuner and returns them in `r`. That is exactly the sequence a seek runs for
 * every channel it visits, so this is the only way to see what a seek is
 * actually judging: `GET /api/state` serves the last polled reading, which is
 * up to a poll interval old and often describes the channel before this one.
 * More than one reading answers a different question from the settle time:
 * whether a channel that looks like a station on one reading still does on
 * the next.
 *
 * A refusal is a 400, a tuner that did not answer is a 502, and a radio task
 * that never got to it is a 503. They are told apart because they send a
 * person looking in three different places.
 *
 * A write, because it moves the dial, and it **leaves the dial where it put
 * it**, the way a sweep does. Tune back afterwards.
 *
 * `mvd` says whether the dial actually moved. Asking about the frequency the
 * radio is already on settles nothing: there was no retune, so what comes
 * back is a fully settled reading wearing whatever settle time was asked for.
 * Anything measuring settling has to park the dial elsewhere first and check
 * this field.
 *
 * It shows what a seek decides at its own 50 ms settle against what a longer
 * settle, such as 400 ms, gives on the same channel, so seek thresholds can
 * be checked against the readings the seek actually sees.
 */
static void handleApiSeekSettle(void) {
  if (!requireAuth(false)) {
    return;
  }
  long khz = 0;
  long ms = 0;
  if (!apiNumber("khz", &khz, 1, 300000)) {
    return;
  }
  if (!apiNumber("ms", &ms, 1, RADIO_PROBE_MAX_MS)) {
    return;
  }

  long reads = 1;
  long gap = 50;
  if (sWeb->server.hasArg("n") &&
      !apiNumber("n", &reads, 1, RADIO_PROBE_MAX_READS)) {
    return;
  }
  if (sWeb->server.hasArg("gap") &&
      !apiNumber("gap", &gap, 1, RADIO_PROBE_MAX_MS)) {
    return;
  }

  Tef668xQuality q[RADIO_PROBE_MAX_READS];
  memset(q, 0, sizeof(q));
  bool moved = false;
  RadioProbeResult probed = radioSettleProbe(
      (uint32_t)khz, (uint16_t)ms, (uint8_t)reads, (uint16_t)gap, 0, q, &moved);
  /* Told apart on purpose. A tuner that did not answer is a different thing
   * from a request the radio would not take, and answering both with the
   * same sentence sends somebody chasing their frequency when the bus is
   * dead. */
  if (probed == RADIO_PROBE_REFUSED) {
    apiFail(400,
            "Not a probe this radio will take. The frequency has to be in "
            "the band it is on, a seek must not be running, it must not be "
            "on its way down for a restart, and ms plus the gaps must come "
            "to no more than 3000, because the radio task waits it out and "
            "stops reading the tuner while it does.");
    return;
  }
  if (probed == RADIO_PROBE_NO_READ) {
    apiFail(502, "The dial moved and the tuner did not answer the read.");
    return;
  }
  if (probed != RADIO_PROBE_OK) {
    apiFail(503, "The radio did not get to it. Try again in a moment.");
    return;
  }

  RadioSnapshot snap;
  bool haveSnap = radioGetSnapshot(&snap);
  bool fm = haveSnap && bandModulation(snap.settings.band) == MODULATION_FM;

  String out;
  char head[80];
  snprintf(head, sizeof(head),
           "{\"khz\":%u,\"ms\":%u,\"gap\":%u,\"mvd\":%s,\"r\":[", (unsigned)khz,
           (unsigned)ms, (unsigned)gap, moved ? "true" : "false");
  out += head;
  for (long i = 0; i < reads; i++) {
    char one[176];
    snprintf(
        one, sizeof(one),
        "%s{\"sig\":%d,\"usn\":%u,\"wam\":%u,\"off\":%d"
        ",\"mod\":%d,\"st\":%s,\"snr\":%d,\"bw\":%u,\"qst\":%u}",
        i ? "," : "", q[i].levelDbuVTenths, (unsigned)q[i].usnTenths,
        fm ? (unsigned)q[i].multipathTenths : (unsigned)q[i].coChannelTenths,
        q[i].offsetKHzTenths, q[i].modulationPercent,
        q[i].stereo ? "true" : "false", q[i].snrDb, (unsigned)q[i].bandwidthKHz,
        (unsigned)q[i].status);
    out += one;
  }
  out += F("]}");
  sWeb->server.send(200, "application/json", out);
}

/*
 * GET /api/rds/raw. The groups the tuner handed over, as they arrived.
 *
 * This is the route for capturing real RDS off the radio. There is no serial
 * cable on this radio, so real groups cannot be read off it any other way, and
 * a decoder tested only against invented groups is tested against a broadcast
 * nobody transmits. A script can poll this and save the groups.
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
  if (!radioRdsEnabled()) {
    /* Said outright. An empty list would read as a station sending nothing,
     * which is a different thing from a decoder nobody has switched on. */
    sWeb->server.send(
        200, "text/plain",
        "# the RDS decoder is switched off, so no groups are read\n");
    return;
  }
  /* On the heap for the one request, not a static: static RAM is the
   * scarcer, and this page is asked for only while capturing. */
  RadioRdsRaw *groups =
      (RadioRdsRaw *)malloc(sizeof(RadioRdsRaw) * RADIO_RDS_RAW_DEPTH);
  if (groups == NULL) {
    apiFail(503, "No memory to copy the groups out. Try again.");
    return;
  }
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

  sWeb->server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  sWeb->server.send(200, "text/plain", "");
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
  sWeb->server.sendContent(line);
  for (uint16_t i = 0; i < held; i++) {
    snprintf(line, sizeof(line), "%u %04X %04X %04X %04X %02X\n",
             (unsigned)(first + i), groups[i].block[0], groups[i].block[1],
             groups[i].block[2], groups[i].block[3], groups[i].error);
    sWeb->server.sendContent(line);
  }
  sWeb->server.sendContent("");
  free(groups);
}

/*
 * POST /api/squelch. What decides whether the audio is open.
 *
 * Takes `mod`: `off`, `auto` or `manual`.
 *
 * The mode also decides what the pot on the front does. There is one knob, so
 * it is the volume control or the squelch control and never both: off and
 * auto leave it as the volume, manual takes it for the squelch.
 *
 * There is deliberately no way to set the manual threshold here. In manual the
 * knob is the threshold, and the knob would replace any number set here a
 * fraction of a second later. Read the threshold back from `sqa` and turn the
 * knob to change it.
 */
static void handleApiSquelch(void) {
  if (!requireAuth(false)) {
    return;
  }

  if (sWeb->server.hasArg("thr")) {
    apiFail(400,
            "The knob sets the threshold in manual, so it cannot be set "
            "here. Read it back from sqa.");
    return;
  }
  if (!sWeb->server.hasArg("mod")) {
    apiFail(400, "Give mod, one of off auto manual.");
    return;
  }

  const String want = sWeb->server.arg("mod");
  SquelchMode mode = SQUELCH_MODE_COUNT;
  for (int m = 0; m < SQUELCH_MODE_COUNT; m++) {
    if (want.equalsIgnoreCase(squelchModeName((SquelchMode)m))) {
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
  sWeb->server.send(200, "text/plain", said + "\n");
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
 * | `ahc` | dBuV, 0 for off, or 20 to 60 | AM high cut start, MW and SW |
 * | `lhc` | dBuV, 0 for off, or 20 to 60 | AM high cut start, LW |
 * | `asm` | dBuV, 0 to 50 | AM soft mute start, MW and SW |
 * | `lsm` | dBuV, 0 to 50 | AM soft mute start, LW |
 * | `dem` | 50, 75 or 0 | FM de-emphasis, in microseconds |
 *
 * `cut`, `bld` and `hbl` go to the chip together, and so do `anb` and `fnb`,
 * and so do the four AM levels. Whichever of a group is not given keeps the
 * value it has, so changing one does not switch off the others.
 *
 * The first six are FM ideas and are refused on the AM bands, where the chip
 * has nowhere to put them. The blankers and the AM levels can be set on
 * both, and so can `dem`, which belongs to the country the radio is in
 * rather than to the band it happens to be on.
 *
 * `dem` is 50 everywhere except the Americas, which use 75. Wrong either
 * way is not subtle: everything sounds dull, or everything sounds shrill.
 *
 * `ims` is multipath suppression, which the old radio badges as iMS and which
 * is what makes a station suffering reflections listenable. `eq` is the
 * channel equalizer. `mno` refuses stereo on purpose, which is not the same
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
         now.settings.fmNoiseBlankerStart + ", am high cut " +
         now.settings.amHighCutStart + " lw " + now.settings.lwHighCutStart +
         ", am soft mute " + now.settings.amSoftMuteStart + " lw " +
         now.settings.lwSoftMuteStart + ", de-emphasis " +
         now.settings.deemphasisUs + " us";
}

static void handleApiFm(void) {
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
    if (!sWeb->server.hasArg(features[i].name)) {
      continue;
    }
    if (!apiNumber(features[i].name, &values[i], 0, 1)) {
      return;
    }
    given[i] = true;
    count++;
  }
  /* The three weak signal start levels, in dBuV, 0 for off. Each one given
   * is a member of one command, and the radio keeps the ones not given as
   * they are: asking to change one thing must not quietly change two
   * others. */
  long weak[3] = {0, 0, 0};
  uint8_t weakMembers = 0;
  {
    const char *names[3] = {"cut", "bld", "hbl"};
    for (int i = 0; i < 3; i++) {
      if (!sWeb->server.hasArg(names[i])) {
        continue;
      }
      if (!apiNumber(names[i], &weak[i], 0, 60)) {
        return;
      }
      weakMembers |= RADIO_MEMBER(i);
      /* The reference's own menu offers 0 or 20 to 60 dBuV. Below 20 the
       * mechanism starts at a level no signal reaches, so it is switched on
       * and does nothing, which is a silent no-op. */
      if (weak[i] != 0 && weak[i] < 20) {
        apiFail(400, String(names[i]) +
                         " is a level in dBuV: 0 to switch it off, or 20 to "
                         "60.");
        return;
      }
    }
  }

  /* The noise blankers, which take impulse noise out rather than hiss. The
   * AM one is the lever for medium wave and shortwave. Members the same
   * way. */
  long blanker[2] = {0, 0};
  uint8_t blankerMembers = 0;
  {
    const char *names[2] = {"anb", "fnb"};
    for (int i = 0; i < 2; i++) {
      if (!sWeb->server.hasArg(names[i])) {
        continue;
      }
      if (!apiNumber(names[i], &blanker[i], 0, 150)) {
        return;
      }
      blankerMembers |= RADIO_MEMBER(i);
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

  /* The AM weak signal start levels, members the same way as the FM three
   * above. The high cuts are 0 for off or 20 to 60 dBuV, the soft mutes 0 to
   * 50 dBuV with no off. */
  const char *amNames[4] = {"ahc", "lhc", "asm", "lsm"};
  long amWeak[4] = {0, 0, 0, 0};
  uint8_t amWeakMembers = 0;
  {
    for (int i = 0; i < 4; i++) {
      if (!sWeb->server.hasArg(amNames[i])) {
        continue;
      }
      if (!apiNumber(amNames[i], &amWeak[i], 0, i < 2 ? 60 : 50)) {
        return;
      }
      amWeakMembers |= RADIO_MEMBER(i);
      if (i < 2 && amWeak[i] != 0 && amWeak[i] < 20) {
        apiFail(400, String(amNames[i]) +
                         " is a level in dBuV: 0 to switch it off, or 20 to "
                         "60.");
        return;
      }
    }
  }

  /* De-emphasis, in microseconds. Only the two real standards and off: any
   * other number is a guess, and the chip would take it and sound wrong. */
  bool wantDeemph = sWeb->server.hasArg("dem");
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

  if (count == 0 && weakMembers == 0 && blankerMembers == 0 && !wantDeemph &&
      amWeakMembers == 0) {
    apiFail(400,
            "Give ims, eq or mno as 0 or 1, cut, bld or hbl as a level in "
            "dBuV, anb or fnb as a percentage, ahc, lhc, asm or lsm as a "
            "level in dBuV, or dem as 50, 75 or 0.");
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
        reason = why == RADIO_ERR_TUNER ? String(radioErrorText(why))
                                        : String(F("the radio refused it: ")) +
                                              radioErrorText(why);
      }
      /* Which one failed, and what the radio is actually set to now. An
       * earlier feature in the same request may already have been applied,
       * and a reply that only said no would be describing a radio that had
       * changed underneath it. */
      apiFail(posted == RADIO_POST_DONE ? apiCommandStatus(why) : 503,
              String(features[i].name) + ": " + reason + ". It is now " +
                  apiFmState());
      return;
    }
  }

  if (blankerMembers != 0) {
    RadioCommand cmd = {};
    cmd.kind = RADIO_SET_NOISE_BLANKER;
    cmd.blanker[0] = (uint8_t)blanker[0];
    cmd.blanker[1] = (uint8_t)blanker[1];
    cmd.members = blankerMembers;
    RadioError why = RADIO_OK;
    if (radioPostAndSettle(&cmd, API_SETTLE_MS, &why) != RADIO_POST_DONE ||
        why != RADIO_OK) {
      apiFail(apiCommandStatus(why),
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
      apiFail(apiCommandStatus(why),
              String("de-emphasis: ") +
                  (why != RADIO_OK ? radioErrorText(why) : "not confirmed") +
                  ". It is now " + apiFmState());
      return;
    }
  }

  if (weakMembers != 0) {
    RadioCommand cmd = {};
    cmd.kind = RADIO_SET_WEAK_SIGNAL;
    cmd.weak[0] = (uint8_t)weak[0];
    cmd.weak[1] = (uint8_t)weak[1];
    cmd.weak[2] = (uint8_t)weak[2];
    cmd.members = weakMembers;
    RadioError why = RADIO_OK;
    if (radioPostAndSettle(&cmd, API_SETTLE_MS, &why) != RADIO_POST_DONE ||
        why != RADIO_OK) {
      apiFail(apiCommandStatus(why),
              String("weak signal: ") +
                  (why != RADIO_OK ? radioErrorText(why) : "not confirmed") +
                  ". It is now " + apiFmState());
      return;
    }
  }

  if (amWeakMembers != 0) {
    RadioCommand cmd = {};
    cmd.kind = RADIO_SET_AM_WEAK_SIGNAL;
    for (int i = 0; i < 4; i++) {
      cmd.amWeak[i] = (uint8_t)amWeak[i];
    }
    cmd.members = amWeakMembers;
    RadioError why = RADIO_OK;
    if (radioPostAndSettle(&cmd, API_SETTLE_MS, &why) != RADIO_POST_DONE ||
        why != RADIO_OK) {
      apiFail(apiCommandStatus(why),
              String("am weak signal: ") +
                  (why != RADIO_OK ? radioErrorText(why) : "not confirmed") +
                  ". It is now " + apiFmState());
      return;
    }
  }

  String said = apiFmState();
  Serial.printf("[api] %s\n", said.c_str());
  sWeb->server.send(200, "text/plain", said + "\n");
}

void webApiTunerRoutes(WebContext *web) {
  sWeb = web;
  sWeb->server.on("/api/squelch", HTTP_POST, handleApiSquelch);
  sWeb->server.on("/api/fm", HTTP_POST, handleApiFm);
  sWeb->server.on("/api/rds/raw", HTTP_GET, handleApiRdsRaw);
  sWeb->server.on("/api/seek/settle", HTTP_POST, handleApiSeekSettle);
}
