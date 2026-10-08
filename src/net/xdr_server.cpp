/*
 * The PC Link: XDR-GTK and FM-DX Webserver over TCP port 7373.
 *
 * Polled from loop() like the web server, with no task of its own. Every
 * command a PC sends goes to the radio through radioPost, the call the knob
 * and the HTTP API use, so nothing else in the radio knows a PC is there.
 * What the radio is doing is read from its snapshot and sent to every PC:
 * the value really in force after a command, and any change made on the
 * panel or in the browser.
 *
 * Measured on this radio: a short line to a PC every 66 ms does not raise the
 * level the tuner reads, where a web reply can by up to 25 dB; and one send
 * holds the loop about 2.6 ms. So at most XDR_CLIENTS_MAX PCs, everything a
 * pass has for a PC goes in one send, and a send never waits: a PC whose
 * lines do not fit is dropped. The Arduino client's own write would wait up
 * to ten seconds on a full socket, a phone that left the Wi-Fi, with the knob
 * and the panel stopped.
 */
#include "net/xdr_server.h"

#include <Arduino.h>
#include <errno.h>
#include <esp_random.h>
#include <lwip/sockets.h>
#include <stdio.h>
#include <string.h>

#include "core/band_plan.h"
#include "core/xdr.h"
#include "net/web_update.h"
#include "radio_task.h"
#include "scope_task.h"
#include "screen_task.h"
#include "sleep_task.h"

/* How long after a command to send what is then in force. The radio task
 * works through a command within a round, 100 ms at most, and the snapshot is
 * republished each round. */
#define XDR_ECHO_WAIT_MS 150

/* How long to wait before opening the port again when that failed. */
#define XDR_RETRY_MS 5000

/* How long a PC has from connecting to sending x. XDR-GTK and FM-DX
 * Webserver sign in and send it at once; a PC that is still not started by
 * then has gone, and must not keep one of the three places. */
#define XDR_START_WAIT_MS 10000

/* What one pass may send a PC: the echoes, the PI, the RDS groups that came
 * since the last pass, about one a pass at 66 ms, and the signal line. A
 * group line is 20 bytes, so this holds the 45 or so of a 4 s sweep. */
#define XDR_OUT_MAX 1536

/* Room for the login line, 40 hex characters, and a carriage return. */
#define XDR_READ_MAX (XDR_DIGEST_HEX + 4)

typedef struct {
  int fd;      /* The socket, or -1 for a free slot. */
  uint32_t ip; /* Its address, as the socket gives it. */
  bool signedIn;
  bool started;       /* Sent x: it gets the signal and the RDS. */
  uint32_t startPass; /* The pass it started in: it has the state of that. */
  bool overlong;
  uint8_t len;
  uint32_t sinceMs; /* When it connected. */
  /* Its spectral scan: the range and step in kHz, and the width in Hz. */
  int32_t scanFrom;
  int32_t scanTo;
  int32_t scanStep;
  int32_t scanWidthHz;
  char salt[XDR_SALT_LEN + 1];
  char line[XDR_READ_MAX];
} Pc;

/* What every started PC was last told, so a change is sent once, whoever
 * made it. */
typedef struct {
  bool valid;
  uint32_t freqKHz;
  int32_t mode;
  int32_t widthHz;
  int32_t deemphasis;
  bool equalizer;
  bool ims;
  int32_t mono;
  int32_t volume;
  int32_t squelch;
  int32_t interval;
  bool piValid;
  uint16_t pi;
  uint8_t doubt;
  uint32_t rdsNext; /* The number of the next RDS group to send. */
} Told;

typedef struct {
  Pc pc[XDR_CLIENTS_MAX];
  Told told;
  RadioSnapshot snap;
  RadioRdsRaw rds[RADIO_RDS_RAW_DEPTH];
  size_t outLen;
  char out[XDR_OUT_MAX];
} Link;

static const Settings *sSettings = NULL;
static Link *sLink = NULL; /* On the heap only while the port is open. */
static int sListen = -1;
static bool sRetrying = false;
static uint32_t sRetryFromMs = 0;
static uint32_t sRefused = 0;
static uint16_t sIntervalMs = XDR_INTERVAL_MIN_MS;
static uint32_t sNextLineMs = 0;
static bool sEchoDue = false;
static uint32_t sEchoAtMs = 0;
static bool sUsersChanged = false;
/* The radio was muted by a PC's volume of 0, so the last PC to leave lifts
 * it: nobody at the radio asked for silence. */
static bool sMutedByPc = false;
/* Counts the loop's passes, so a PC started in a pass is not sent that
 * pass's batch on top of the whole state. */
static uint32_t sPass = 0;
/* When that mute was asked for: a snapshot from before the radio task took
 * it still shows the radio playing. */
static uint32_t sMutedAtMs = 0;
/* The spectral scan under way: the place of the PC that asked, -1 for none,
 * whether it repeats, whether it waits for another sweep to end, and the band
 * scope's count of sweeps when it started, so a sweep that ended early is
 * told from one that finished. */
static int sScanBy = -1;
static bool sScanRepeat = false;
static bool sScanWaiting = false;
static uint16_t sScanRev = 0;

void xdrServerBegin(const Settings *settings) {
  sSettings = settings;
}

bool xdrServerListening(void) {
  return sListen >= 0;
}

uint8_t xdrServerClients(void) {
  uint8_t n = 0;
  for (int i = 0; sLink != NULL && i < XDR_CLIENTS_MAX; i++) {
    n += sLink->pc[i].signedIn ? 1 : 0;
  }
  return n;
}

bool xdrServerClientAddress(uint8_t index, char *out, size_t cap) {
  for (int i = 0; sLink != NULL && i < XDR_CLIENTS_MAX; i++) {
    const Pc *pc = &sLink->pc[i];
    if (!pc->signedIn) {
      continue;
    }
    if (index-- == 0) {
      struct in_addr at = {pc->ip};
      return inet_ntoa_r(at, out, cap) != NULL;
    }
  }
  return false;
}

uint32_t xdrServerRefused(void) {
  return sRefused;
}

static void post(RadioCommand *cmd) {
  if (radioPost(cmd)) {
    sleepTaskUsed();
  }
}

static void unmuteIfPcMuted(void) {
  if (!sMutedByPc) {
    return;
  }
  sMutedByPc = false;
  RadioCommand cmd = {};
  cmd.kind = RADIO_SET_MUTE;
  cmd.muted = false;
  (void)radioPost(&cmd);
}

static void drop(Pc *pc) {
  if (pc->fd >= 0) {
    close(pc->fd);
  }
  /* The sweep under way still answers the others; it is not repeated. */
  if (sLink != NULL && pc - sLink->pc == sScanBy) {
    sScanRepeat = false;
  }
  if (pc->signedIn) {
    sUsersChanged = true;
  }
  pc->fd = -1;
  pc->signedIn = false;
  pc->started = false;
  pc->overlong = false;
  pc->len = 0;
}

/* Send the whole of `text` now or drop the PC: half a line is worse than
 * none, and waiting holds the loop. */
static void sendAll(Pc *pc, const char *text, size_t len) {
  if (pc->fd >= 0 && len > 0 &&
      send(pc->fd, text, len, MSG_DONTWAIT) != (int)len) {
    drop(pc);
  }
}

void xdrServerSignOutAll(void) {
  for (int i = 0; sLink != NULL && i < XDR_CLIENTS_MAX; i++) {
    drop(&sLink->pc[i]);
  }
}

static bool openPort(void) {
  sLink = static_cast<Link *>(calloc(1, sizeof(Link)));
  if (sLink == NULL) {
    return false;
  }
  for (int i = 0; i < XDR_CLIENTS_MAX; i++) {
    sLink->pc[i].fd = -1;
  }
  const int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  struct sockaddr_in at = {};
  at.sin_family = AF_INET;
  at.sin_port = htons(XDR_PORT);
  at.sin_addr.s_addr = htonl(INADDR_ANY);
  const int one = 1;
  if (fd < 0 ||
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) != 0 ||
      bind(fd, reinterpret_cast<struct sockaddr *>(&at), sizeof(at)) != 0 ||
      listen(fd, XDR_CLIENTS_MAX) != 0 || fcntl(fd, F_SETFL, O_NONBLOCK) != 0) {
    if (fd >= 0) {
      close(fd);
    }
    free(sLink);
    sLink = NULL;
    return false;
  }
  sListen = fd;
  sIntervalMs = XDR_INTERVAL_MIN_MS;
  sNextLineMs = millis();
  Serial.printf("[xdr] listening on %u\n", (unsigned)XDR_PORT);
  return true;
}

static void closePort(void) {
  for (int i = 0; sLink != NULL && i < XDR_CLIENTS_MAX; i++) {
    drop(&sLink->pc[i]);
  }
  if (sListen >= 0) {
    close(sListen);
  }
  sListen = -1;
  free(sLink);
  sLink = NULL;
  sUsersChanged = false;
  /* Nobody is left to answer, and the sweep keeps the radio muted. */
  if (sScanBy >= 0 && !sScanWaiting) {
    radioSweepCancel();
  }
  sScanBy = -1;
  sScanRepeat = false;
  sScanWaiting = false;
  unmuteIfPcMuted();
  Serial.println(F("[xdr] closed"));
}

static void takeNewPcs(void) {
  for (;;) {
    struct sockaddr_in from = {};
    socklen_t fromLen = sizeof(from);
    const int fd =
        accept(sListen, reinterpret_cast<struct sockaddr *>(&from), &fromLen);
    if (fd < 0) {
      return;
    }
    Pc *pc = NULL;
    for (int i = 0; i < XDR_CLIENTS_MAX && pc == NULL; i++) {
      pc = sLink->pc[i].fd < 0 ? &sLink->pc[i] : NULL;
    }
    if (pc == NULL) {
      close(fd);
      sRefused++;
      continue;
    }
    const int one = 1;
    (void)fcntl(fd, F_SETFL, O_NONBLOCK);
    (void)setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    pc->fd = fd;
    pc->ip = from.sin_addr.s_addr;
    pc->sinceMs = millis();
    pc->scanFrom = 0;
    pc->scanTo = 0;
    pc->scanStep = 0;
    pc->scanWidthHz = 0;
    uint8_t random[XDR_SALT_LEN];
    esp_fill_random(random, sizeof(random));
    xdrSalt(random, pc->salt);
    char line[XDR_SALT_LEN + 2];
    memcpy(line, pc->salt, XDR_SALT_LEN);
    line[XDR_SALT_LEN] = '\n';
    sendAll(pc, line, XDR_SALT_LEN + 1);
  }
}

/* Add one line to the pass's output, if it fits. */
static void add(const char *line, size_t len) {
  if (sLink->outLen + len + 1 > XDR_OUT_MAX) {
    return;
  }
  memcpy(sLink->out + sLink->outLen, line, len);
  sLink->outLen += len;
  sLink->out[sLink->outLen++] = '\n';
}

static void addValue(char letter, long value) {
  char line[16];
  const size_t len = xdrValue(line, sizeof(line), letter, value);
  add(line, len);
}

/* What is in force now, in the protocol's terms. */
static Told inForce(const RadioSnapshot *s) {
  const RadioSettings *r = &s->settings;
  Told now = {};
  now.valid = true;
  now.freqKHz = r->freqKHz;
  now.mode = bandModulation(r->band) == MODULATION_AM ? 1 : 0;
  now.widthHz = (int32_t)radioTunerBandwidth(r) * 1000;
  now.deemphasis = xdrDeemphasisCode(r->deemphasisUs);
  now.equalizer = r->equalizer;
  now.ims = r->multipathSuppression;
  now.mono = r->forcedMono ? 1 : 0;
  now.volume = xdrVolumeFromDb(r->volumeDb, r->muted);
  now.squelch =
      xdrSquelchValue(s->squelchMode, bandModulation(r->band) == MODULATION_AM,
                      s->squelchThresholdTenths, sSettings->fmSquelchFloor);
  now.interval = sIntervalMs;
  const RdsInfo *rds = &s->rds;
  now.piValid = rds->hasPi || rds->hasPiHeard;
  now.pi = !now.piValid ? 0 : rds->hasPi ? rds->pi : rds->piHeard;
  now.doubt = rds->hasPi ? 0 : 1;
  return now;
}

/* The echo of everything but the frequency that changed since `was`, or of
 * everything. */
static void addChanges(const Told *now, const Told *was) {
  const bool all = !was->valid;
  char line[16];
  if (all || now->mode != was->mode) {
    addValue('M', now->mode);
  }
  if (all || now->widthHz != was->widthHz) {
    addValue('W', now->widthHz);
  }
  if (all || now->deemphasis != was->deemphasis) {
    addValue('D', now->deemphasis);
  }
  if (all || now->equalizer != was->equalizer || now->ims != was->ims) {
    const size_t len = xdrEqIms(line, sizeof(line), now->equalizer, now->ims);
    add(line, len);
  }
  if (all || now->mono != was->mono) {
    addValue('B', now->mono);
  }
  if (all || now->volume != was->volume) {
    addValue('Y', now->volume);
  }
  if (all || now->squelch != was->squelch) {
    addValue('Q', now->squelch);
  }
  if (all || now->interval != was->interval) {
    addValue('I', now->interval);
  }
}

/* The PI, before every batch of RDS groups: XDR-GTK takes RDS as live only
 * for a few signal lines after a PI line, and the version most people run
 * drops every group until it has had one. */
static void addPi(const Told *now) {
  if (now->piValid) {
    char line[16];
    const size_t len = xdrPi(line, sizeof(line), now->pi, now->doubt);
    add(line, len);
  }
}

/*
 * The frequency goes last in a send. FM-DX Webserver drops the lines after a
 * frequency it already shows. It shows its own start frequency before the
 * radio has tuned there. So nothing may come after the frequency in a send.
 * XDR-GTK and FM-DX Webserver both clear their RDS on a frequency line. The
 * next pass sends the PI again with the next RDS groups.
 */
static void addTune(uint32_t khz) {
  addValue('T', (long)khz);
}

static void addRds(Told *told, bool fromNow);

/* What a PC that has just started is told: OK, then the whole state, and the
 * values this radio has no control for, as fixed: the RF AGC at its highest
 * start, one aerial, no attenuation, no rotator. The frequency last. The first
 * PC to start also sets what every PC was last told, so the next pass does
 * not send the frequency again. */
static void start(Pc *pc) {
  if (!radioGetSnapshot(&sLink->snap)) {
    return;
  }
  sLink->outLen = 0;
  add("OK", 2);
  /* First, since XDR-GTK clears its RDS on an aerial line. */
  addValue('A', 0);
  addValue('Z', 0);
  addValue('V', 0);
  addValue('C', 0);
  Told now = inForce(&sLink->snap);
  const Told none = {};
  addChanges(&now, &none);
  addTune(now.freqKHz);
  sendAll(pc, sLink->out, sLink->outLen);
  sLink->outLen = 0;
  if (!sLink->told.valid) {
    now.rdsNext = sLink->told.rdsNext;
    addRds(&now, true);
    sLink->told = now;
  }
  pc->started = true;
  pc->startPass = sPass;
}

static void tune(int32_t khz) {
  BandPlanConfig plan;
  BandId band;
  if (!radioTaskPlan(&plan) || !bandForFrequency(&plan, (uint32_t)khz, &band)) {
    return;
  }
  if (bandModulation(band) == MODULATION_FM) {
    khz = (khz + 5) / 10 * 10; /* The FM side tunes in 10 kHz steps. */
  }
  RadioCommand cmd = {};
  cmd.kind = RADIO_TUNE;
  cmd.freqKHz = (uint32_t)khz;
  post(&cmd);
}

/*
 * Start the scan `pc` set up, of the band the radio is on. One that cannot be
 * swept is answered with an empty U line, so a PC that takes it stops
 * waiting, and its reason goes to the log. While another sweep, a seek or the
 * update check holds the tuner, it waits and starts when they end.
 */
static void startScan(Pc *pc, bool repeat) {
  sScanBy = (int)(pc - sLink->pc);
  sScanRepeat = repeat;
  sScanWaiting = false;
  RadioSettings r;
  BandPlanConfig plan;
  DxSweepRange range;
  const char *why = !radioGetSettings(&r) || !radioTaskPlan(&plan)
                        ? "The radio did not answer."
                        : xdrScanRange(r.band, &plan, pc->scanFrom, pc->scanTo,
                                       pc->scanStep, &range);
  if (why != NULL) {
    Serial.printf("[xdr] scan not started: %s\n", why);
    sendAll(pc, "U\n", 2);
    sScanBy = -1;
    sScanRepeat = false;
    return;
  }
  ScopeView v;
  scopeTaskView(&v);
  sScanRev = v.revision;
  sScanWaiting =
      scopeTaskSweepRange(&range, xdrWidthKHz(BAND_FM, pc->scanWidthHz, 0)) !=
      SCOPE_STARTED;
}

/* The scan's U line to every started PC, in parts the size of the pass's
 * output; a PC that cannot take a part is dropped, as for any line. */
static void sendScan(const DxSweep *s) {
  uint16_t next = 0;
  size_t len = 0;
  while ((len = xdrScanPart(sLink->out, XDR_OUT_MAX, s, &next)) > 0) {
    for (int i = 0; i < XDR_CLIENTS_MAX; i++) {
      if (sLink->pc[i].started) {
        sendAll(&sLink->pc[i], sLink->out, len);
      }
    }
  }
}

/*
 * A scan that has ended: answered, and started again at once when it repeats,
 * so no signal line goes out between two sweeps and XDR-GTK keeps its scan
 * on. One ended early, by an empty line, a key or a tune, is not answered
 * and not repeated.
 */
static bool anyStarted(void);

static void scanPoll(void) {
  if (sScanBy < 0) {
    return;
  }
  if (!anyStarted()) {
    /* Nobody left to answer, and the sweep keeps the radio muted. */
    if (!sScanWaiting) {
      radioSweepCancel();
    }
    sScanBy = -1;
    sScanRepeat = false;
    sScanWaiting = false;
    return;
  }
  Pc *by = &sLink->pc[sScanBy];
  if (sScanWaiting) {
    if (!by->started) {
      sScanBy = -1;
    } else if (!radioSweepBusy()) {
      startScan(by, sScanRepeat);
    }
    return;
  }
  ScopeView v;
  scopeTaskView(&v);
  if (v.running) {
    return;
  }
  const bool finished = v.revision != sScanRev && v.latest != NULL;
  if (finished) {
    sendScan(v.latest);
  }
  if (finished && sScanRepeat && by->started) {
    startScan(by, true);
    return;
  }
  sScanBy = -1;
  sScanRepeat = false;
}

/* Carry out one understood command. The answer is the echo the next pass
 * sends, from what the radio then has. */
static void act(Pc *pc, const XdrCommand *c, const RadioSettings *r) {
  RadioCommand cmd = {};
  const bool am = bandModulation(r->band) == MODULATION_AM;
  switch (c->kind) {
    case XDR_SCAN_FROM:
      pc->scanFrom = c->value;
      return;
    case XDR_SCAN_TO:
      pc->scanTo = c->value;
      return;
    case XDR_SCAN_STEP:
      pc->scanStep = c->value;
      return;
    case XDR_SCAN_WIDTH:
      pc->scanWidthHz = c->value;
      return;
    case XDR_SCAN_RUN:
      if (sScanBy < 0) {
        startScan(pc, c->value == 1);
      } else if (pc - sLink->pc == sScanBy) {
        /* Its own scan under way: this only says whether it goes on. Another
         * PC's answers this one too. */
        sScanRepeat = c->value == 1;
      }
      return;
    case XDR_SCAN_STOP:
      if (sScanBy >= 0 && pc - sLink->pc == sScanBy) {
        sScanRepeat = false;
        if (sScanWaiting) {
          sScanBy = -1;
          sScanWaiting = false;
        } else {
          radioSweepCancel();
        }
      }
      return;
    case XDR_START:
      start(pc);
      return;
    case XDR_END:
      drop(pc);
      return;
    case XDR_TUNE:
      tune(c->value);
      break;
    case XDR_MODE:
      if ((c->value == 1) != am) {
        cmd.kind = RADIO_SET_BAND;
        cmd.band = c->value == 1 ? BAND_MW : BAND_FM;
        post(&cmd);
      }
      break;
    case XDR_WIDTH:
      cmd.bandwidthKHz = xdrWidthKHz(r->band, c->value, r->bandwidthKHz);
      if (screenTaskDxWidth() != 0) {
        /* DX mode's own fixed width is in force, so the PC sets that, as the
         * bandwidth page does there. Automatic is refused: DX mode is a
         * fixed width, and the echo says so. */
        if (screenTaskDxSetWidth(cmd.bandwidthKHz)) {
          sleepTaskUsed();
        }
        break;
      }
      cmd.kind = RADIO_SET_BANDWIDTH;
      post(&cmd);
      break;
    case XDR_DEEMPHASIS:
      cmd.kind = RADIO_SET_DEEMPHASIS;
      cmd.deemphasisUs = xdrDeemphasisUs(c->value);
      post(&cmd);
      break;
    case XDR_EQ_IMS:
      cmd.kind = RADIO_SET_EQUALIZER;
      cmd.on = c->value != 0;
      post(&cmd);
      cmd.kind = RADIO_SET_MPH_SUPPRESSION;
      cmd.on = c->value2 != 0;
      post(&cmd);
      break;
    case XDR_MONO:
      /* 2 asks for the MPX output, which this radio does not have. */
      if (c->value < 2) {
        cmd.kind = RADIO_SET_MONO;
        cmd.on = c->value == 1;
        post(&cmd);
      }
      break;
    case XDR_VOLUME:
      if (c->value == 0) {
        sMutedByPc = sMutedByPc || !r->muted;
        sMutedAtMs = millis();
        cmd.kind = RADIO_SET_MUTE;
        cmd.muted = true;
        post(&cmd);
        break;
      }
      cmd.kind = RADIO_SET_VOLUME;
      cmd.volumeDb = xdrVolumeDb(c->value);
      post(&cmd);
      if (r->muted) {
        cmd.kind = RADIO_SET_MUTE;
        cmd.muted = false;
        post(&cmd);
      }
      break;
    case XDR_SQUELCH:
      /* 0 turns it off, and a level turns Off into Auto. A squelch already
       * on stays as it is: Manual gives the knob to the squelch, and XDR-GTK
       * sends its own slider's level each time it connects. -1 asks for a
       * stereo squelch, which this radio does not have. */
      if (c->value == 0 ||
          (c->value > 0 && radioSquelchMode(NULL) == SQUELCH_OFF)) {
        radioSetSquelchMode(c->value == 0 ? SQUELCH_OFF : SQUELCH_AUTO);
        sleepTaskUsed();
      }
      break;
    case XDR_INTERVAL: {
      /* 0 asks for the usual, which is also the fastest. */
      sIntervalMs =
          (uint16_t)(c->value < XDR_INTERVAL_MIN_MS ? XDR_INTERVAL_MIN_MS
                                                    : c->value);
      break;
    }
    default:
      /* A, Z, V and C: nothing here to set. The values in force went out
       * when the PC started, and its control goes back to them. */
      break;
  }
  sEchoDue = true;
  sEchoAtMs = millis() + XDR_ECHO_WAIT_MS;
}

static void handleLine(Pc *pc, const char *line) {
  if (!pc->signedIn) {
    if (!webAuthXdrLogin(pc->salt, line)) {
      Serial.println(F("[xdr] wrong PIN"));
      sendAll(pc, "a0\n", 3);
      drop(pc);
      return;
    }
    pc->signedIn = true;
    sUsersChanged = true;
    char ip[16];
    struct in_addr at = {pc->ip};
    Serial.printf("[xdr] %s signed in\n", inet_ntoa_r(at, ip, sizeof(ip)));
    return;
  }
  XdrCommand c;
  if (xdrParse(line, &c) != NULL || c.kind == XDR_IGNORE) {
    return;
  }
  RadioSettings r;
  if (!radioGetSettings(&r)) {
    return;
  }
  act(pc, &c, &r);
}

static void readPc(Pc *pc) {
  char buf[64];
  for (int round = 0; round < 4 && pc->fd >= 0; round++) {
    const int n = recv(pc->fd, buf, sizeof(buf), MSG_DONTWAIT);
    if (n == 0 || (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN)) {
      drop(pc);
      return;
    }
    if (n < 0) {
      return;
    }
    for (int i = 0; i < n && pc->fd >= 0; i++) {
      if (buf[i] == '\n') {
        pc->line[pc->len] = '\0';
        if (!pc->overlong) {
          handleLine(pc, pc->line);
        }
        pc->len = 0;
        pc->overlong = false;
      } else if (pc->len < XDR_READ_MAX - 1) {
        pc->line[pc->len++] = buf[i];
      } else {
        pc->overlong = true;
      }
    }
  }
}

static void tellUsers(void) {
  sUsersChanged = false;
  const uint8_t users = xdrServerClients();
  char line[16];
  size_t len = xdrUsers(line, sizeof(line) - 1, users);
  line[len++] = '\n';
  for (int i = 0; i < XDR_CLIENTS_MAX; i++) {
    if (sLink->pc[i].signedIn) {
      sendAll(&sLink->pc[i], line, len);
    }
  }
  if (users == 0) {
    unmuteIfPcMuted();
    sIntervalMs = XDR_INTERVAL_MIN_MS;
  }
}

/* The RDS groups that arrived since the last pass, oldest first, with the PI
 * before them. A retune empties the ring and starts its count again, so a
 * count below the last one is a new station, sent from its first group; the
 * first pass a PC is there starts from now rather than sending what came
 * before. Nothing is changed when the ring could not be read. */
static void addRds(Told *told, bool fromNow) {
  uint32_t first = 0;
  const uint16_t n =
      radioRdsRaw(sLink->rds, RADIO_RDS_RAW_DEPTH, &first, NULL, NULL);
  if (n == 0) {
    return;
  }
  const uint32_t from = fromNow                     ? first + n
                        : first + n < told->rdsNext ? first
                                                    : told->rdsNext;
  if (first + n > from) {
    addPi(told);
  }
  char line[24];
  for (uint16_t i = 0; i < n; i++) {
    if (first + i >= from) {
      const size_t len =
          xdrRds(line, sizeof(line), sLink->rds[i].block, sLink->rds[i].error);
      add(line, len);
    }
  }
  told->rdsNext = first + n;
}

static bool anyStarted(void) {
  for (int i = 0; i < XDR_CLIENTS_MAX; i++) {
    if (sLink->pc[i].started) {
      return true;
    }
  }
  return false;
}

/* The echoes, the RDS, the signal when due, and a new frequency last, to
 * every started PC in one send each. */
static void sendDue(bool lineDue) {
  if (!radioGetSnapshot(&sLink->snap)) {
    return;
  }
  const RadioSnapshot *s = &sLink->snap;
  if (!s->settings.muted && millis() - sMutedAtMs >= XDR_ECHO_WAIT_MS) {
    sMutedByPc = false;
  }
  const bool am = bandModulation(s->settings.band) == MODULATION_AM;
  sLink->outLen = 0;
  Told now = inForce(s);
  addChanges(&now, &sLink->told);
  now.rdsNext = sLink->told.rdsNext;
  if (!am) {
    addRds(&now, !sLink->told.valid);
  }
  if (lineDue && s->levelSmoothedValid && s->qualityValid) {
    char line[24];
    const size_t len = xdrSignal(line, sizeof(line), s->levelSmoothedTenths,
                                 s->quality.stereo, s->settings.forcedMono, am);
    add(line, len);
  }
  if (!sLink->told.valid || now.freqKHz != sLink->told.freqKHz) {
    addTune(now.freqKHz);
  }
  sLink->told = now;
  for (int i = 0; i < XDR_CLIENTS_MAX; i++) {
    Pc *pc = &sLink->pc[i];
    if (pc->started && pc->startPass != sPass) {
      sendAll(pc, sLink->out, sLink->outLen);
    }
  }
}

void xdrServerLoop(void) {
  const uint32_t now = millis();
  sPass++;
  const bool wanted =
      sSettings != NULL && sSettings->pcLink != 0 && sSettings->wifiEnabled;
  if (!wanted) {
    if (sListen >= 0) {
      closePort();
    }
    return;
  }
  if (sListen < 0) {
    if (sRetrying && now - sRetryFromMs < XDR_RETRY_MS) {
      return;
    }
    sRetrying = !openPort();
    sRetryFromMs = now;
    if (sRetrying) {
      return;
    }
  }
  takeNewPcs();
  for (int i = 0; i < XDR_CLIENTS_MAX; i++) {
    Pc *pc = &sLink->pc[i];
    if (pc->fd >= 0) {
      readPc(pc);
    }
    if (pc->fd >= 0 && !pc->started &&
        millis() - pc->sinceMs > XDR_START_WAIT_MS) {
      drop(pc);
    }
  }
  if (sUsersChanged) {
    tellUsers();
  }
  scanPoll();
  /* Nothing goes out while a level sweep reads the band: a send there would
   * raise the channel being read, as a web reply does. */
  if (radioSweepBusy()) {
    return;
  }
  const bool lineDue = (int32_t)(now - sNextLineMs) >= 0;
  const bool echoDue = sEchoDue && (int32_t)(now - sEchoAtMs) >= 0;
  if (!lineDue && !echoDue) {
    return;
  }
  if (lineDue) {
    sNextLineMs += sIntervalMs;
    /* Behind by more than a line, after a sweep or a slow pass: start the
     * count from now rather than send a burst to catch up. */
    if ((int32_t)(now - sNextLineMs) >= 0) {
      sNextLineMs = now + sIntervalMs;
    }
  }
  sEchoDue = false;
  if (!anyStarted()) {
    sLink->told.valid = false;
    return;
  }
  sendDue(lineDue);
}
