/*
 * The DX scanner: walk a list of channels one at a time, stay on each for
 * the dwell, and stop where the caller's stop rule says.
 *
 * Pure logic. The caller tunes the radio, reads it back, says whether a PI
 * is heard and whether its rule stops on that PI; this decides where to go
 * next and when to stop. Nothing here waits: each call looks once and
 * returns.
 *
 * There is no signal limit. At a DX width the seek's limits stop on empty
 * channels and miss stations, as measured on this radio, and no measurement yet
 * says where a weak DX station reads, so a limit here would be a guess that
 * skips the stations the scanner is for. Every channel gets the whole dwell
 * unless a PI is heard on it first.
 */
#ifndef CORE_DX_SCAN_H
#define CORE_DX_SCAN_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * How long each channel is listened to once the dial is on it, by default,
 * in milliseconds. 2.5 s, because a PI takes two clean block A, 11.4 groups
 * come a second, and a weak station loses most of them. The DX Scanner menu
 * sets it, 0.5 to 30 s.
 */
#define DX_SCAN_DWELL_MS 2500u

/* The dwell the DX Scanner menu offers, in tenths of a second: 0.5 to
 * 30 s, the range the PE5PVB TEF6686_ESP32 firmware's "Wait time" has. */
#define DX_SCAN_DWELL_MIN_TENTHS 5
#define DX_SCAN_DWELL_MAX_TENTHS 300

/* What a PI heard on a channel does to a scan. */
typedef enum {
  DX_STOP_NEW = 0, /* Stop on a PI never caught before, go on from a known. */
  DX_STOP_ANY_PI,  /* Stop on any PI confirmed as the channel's own. */
  DX_STOP_NEVER,   /* Never stop: catch, log a NEW one, and go on. */
  DX_STOP_COUNT,
} DxStopRule;

/* What a scan walks. */
typedef enum {
  DX_RANGE_BAND_LESS_MEMORY = 0, /* The band, passing over stored channels. */
  DX_RANGE_BAND,                 /* The whole band. */
  DX_RANGE_MEMORY,               /* The memory channels, in slot order. */
  DX_RANGE_COUNT,
} DxScanRange;

typedef enum {
  DX_SCAN_IDLE = 0, /* Not started, or finished the list. */
  DX_SCAN_RUNNING,
  DX_SCAN_STOPPED, /* On a catch, or stopped by a key, or the dial moved. */
} DxScanState;

/* The channel at position `pos` of a walk, or false to pass over it. */
typedef bool (*DxScanChannel)(void *ctx, uint16_t pos, uint32_t *khz);

/* What a scan walks and how. */
typedef struct {
  uint16_t count;        /* Positions in the walk. */
  DxScanChannel channel; /* Never NULL for a walk that starts. */
  void *ctx;
  uint32_t dwellMs;
  bool loop; /* Go round again from the first position, not finish. */
} DxScanPlan;

typedef struct {
  DxScanState state;
  DxScanPlan plan;
  uint16_t pos;      /* The position being listened to, or stopped at. */
  uint32_t atKHz;    /* Its channel. */
  uint32_t fromKHz;  /* Where the dial was when the scan sent it to atKHz. */
  uint32_t startKHz; /* Where the dial was when the scan started. */
  bool landed;       /* The dial has reached atKHz. */
  uint32_t landedMs;
  bool waiting; /* Waiting for the dial to land, since waitMs. */
  uint32_t waitMs;
  uint16_t found; /* How many channels this lap confirmed a PI on. */
  bool counted;   /* This channel's PI is in `found`. */
  bool onCatch;   /* Stopped because of a PI, not by a key. */
  bool finished;  /* The last scan reached the end of its walk. */
} DxScan;

/* What the caller does after a call: tune to `khz`, or nothing. */
typedef struct {
  bool tune;
  uint32_t khz;
} DxScanAction;

void dxScanReset(DxScan *s);

/*
 * Start at the first position of `plan` not passed over, from the dial at
 * `dialKHz`. Running, and a tune to that channel; idle and no tune when
 * every position is passed over, or for a plan with no positions, no dwell
 * or no channel function.
 */
DxScanAction dxScanStart(DxScan *s, const DxScanPlan *plan, uint32_t dialKHz);

/*
 * Go on from a stop, at the position after the one stopped at, wherever the
 * dial has been turned to since. At the end of the walk it finishes, or
 * goes round, as a scan reaching it does. Nothing unless stopped.
 */
DxScanAction dxScanResume(DxScan *s, uint32_t dialKHz);

/* Stop where it is, a key pressed. Nothing unless running. */
void dxScanStop(DxScan *s);

/*
 * One look at the radio: `dialKHz` where it is tuned, `piHeard` whether a
 * PI is heard now as the channel's own (dxPiHeardNow in dx.h), `holdOn`
 * whether the caller wants the rest of the dwell anyway, such as for a NEW
 * station's name, and `stopOnIt` whether its stop rule stops on that PI.
 * Once the dial is on the channel, a PI counts as found, once, and one the
 * rule stops on stops the scan there; any other sends it on at once, since
 * there is nothing more to wait for, unless held, and the dwell running out
 * sends it on. A tune that has not landed within a dwell is given up and
 * the scan goes on. The dial anywhere else means somebody tuned, and the
 * scan stops and leaves it. At the end of a walk that does not loop it
 * finishes and sends the dial back to where the scan started.
 */
DxScanAction dxScanPoll(DxScan *s, uint32_t nowMs, uint32_t dialKHz,
                        bool piHeard, bool holdOn, bool stopOnIt);

/* The dwell left on this channel: the whole of it until the dial arrives,
 * and 0 unless running. */
uint32_t dxScanLeftMs(const DxScan *s, uint32_t nowMs);

/* How many positions lie before the one being listened to, all of them
 * once a scan has finished, and how many there are. Both 0 before a
 * start. */
uint16_t dxScanPassed(const DxScan *s);
uint16_t dxScanTotal(const DxScan *s);

/* Whether a scan passes over a channel of a band walk. NULL passes over
 * none. */
typedef bool (*DxScanSkip)(void *ctx, uint32_t khz);

/* A walk over a band, from `lowKHz` in `stepKHz`, for DxScanPlan's channel
 * function with this as its context. */
typedef struct {
  uint32_t lowKHz;
  uint16_t stepKHz;
  DxScanSkip skip;
  void *skipCtx;
} DxScanBand;

bool dxScanBandChannel(void *ctx, uint16_t pos, uint32_t *khz);

/* How many channels a band walk has from `lowKHz` to `highKHz`, 0 for a
 * band that is not one. */
uint16_t dxScanBandCount(uint32_t lowKHz, uint32_t highKHz, uint16_t stepKHz);

#ifdef __cplusplus
}
#endif

#endif /* CORE_DX_SCAN_H */
