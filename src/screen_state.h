/*
 * The radio screen's state, built from the radio's snapshot.
 *
 * A part of `screen_task.cpp` kept in its own file. The screen task reads what
 * the state is built from, the snapshot and everything the other tasks and
 * drivers know, into `ScreenInputs`, and this turns it into a `ScreenState`:
 * which words, which numbers, what is held still and what is left out. It
 * reaches nothing itself, no driver, no lock and no clock, so the renderer in
 * `tools/screenshot.cpp` can run the same code on a PC. Glue like the task
 * files, not a layer of its own.
 */
#ifndef SCREEN_STATE_H
#define SCREEN_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "core/band_plan.h"
#include "core/battery.h"
#include "core/memory.h"
#include "core/meter.h"
#include "core/rds.h"
#include "core/signal.h"
#include "input_task.h"
#include "radio_task.h"
#include "ui/screen.h"

/* Read one stored channel, the way `memoryStoreRead` does: false for an
 * empty or unreadable slot. */
typedef bool (*ScreenChannelRead)(int slot, MemoryChannel *out);

/* Everything the state is built from, read by the caller. */
typedef struct {
  const RadioSnapshot *snap;     /* What the radio is doing. Never NULL. */
  bool planValid;                /* The band plan below could be read. */
  BandPlanConfig plan;           /* The band limits the dial tunes against. */
  ScreenChannelRead readChannel; /* The stored channels. Never NULL. */
  /* Goes up whenever a stored channel changes, so the channel name read
   * from `readChannel` is read again only then. */
  uint32_t memoryGeneration;
  const char *typed; /* Digits typed on the keypad so far, or NULL. */
  bool touchOn;      /* Touch is On and the chip answered: a finger can act. */
  bool pcLink;       /* A PC is signed in over the PC Link. */
  ScreenWifi wifi;   /* What the network is doing. */
  bool rssiValid;    /* The link's strength below was read. */
  int8_t rssiDbm;    /* The link's strength, in dBm. */
  const Battery *battery;  /* The battery readings, or NULL. */
  BatteryShow batteryShow; /* How the battery is shown, if at all. */
  const char *clock;       /* "14:05", or NULL until a server has answered. */
  const char *date;        /* The date line, or NULL the same way. */
  const char *fault;       /* What the tuner last refused, or NULL. */
  const char *logConfirm;  /* "Logged 106.40" while it holds, or NULL. */
  const char *notice;      /* The line under the panel's message, or NULL. */
  uint32_t nowMs;          /* The time, for the modulation meter's fall. */
} ScreenInputs;

/*
 * What the builder keeps from one build to the next: the readings it holds
 * still, the lists it reads only when they change, and the text the state
 * points at, which has to outlive the call that shows it. One per screen,
 * cleared with screenStateReset.
 */
typedef struct {
  /*
   * The signal number on the screen, and what it was last shown for.
   *
   * Held still rather than redrawn from every reading. The band and
   * frequency are kept beside it so a change of station starts it again,
   * instead of the old station's number being held until the new one has
   * moved a whole dB away from it.
   */
  SignalDisplay signalDisplay;
  /* The modulation meter's bar and peak mark, which fall over time, and the
   * count of tuner reads the last one was fed from. */
  MeterBar modulationBar;
  MeterPeak modulationPeak;
  uint32_t modulationReads;
  uint32_t shownFreqKHz;
  BandId shownBand;
  bool shownStationKnown;
  /* The name of the stored channel the radio is on, and what it was last
   * read for. */
  int namedSlot;
  uint32_t namedGeneration;
  char channelName[MEMORY_NAME_LEN];
  /* The station name without its padding. */
  char stationName[RDS_PS_LONG_LEN + 1];
  /* The short strings the state points at. */
  char frequency[16];
  char meterBand[8];
  char memory[8];
  char filterText[8];
  char volumeText[12];
  char squelchText[12];
  char batteryText[BATTERY_TEXT_LEN];
  char typing[INPUT_DIGITS_MAX + 2];
} ScreenBuild;

/* A build with nothing held yet, for start up. */
void screenStateReset(ScreenBuild *b);

/*
 * Build the radio screen's state from `in` into `out`. The strings in `out`
 * point into `b` and stay valid until the next build with the same `b`.
 */
void screenStateBuild(ScreenBuild *b, const ScreenInputs *in, ScreenState *out);

/* A number being typed as the radio screen and the headers draw it: the
 * digits and a dash for the next, and no dash once INPUT_DIGITS_MAX are
 * typed. The keypad draws the digits alone. */
void screenTypedText(const char *typed, char *out, size_t len);

#endif /* SCREEN_STATE_H */
