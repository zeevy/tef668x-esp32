# CLAUDE.md

Guidance for Claude Code working in this repository.

## What this is

Firmware for radio receivers built around the NXP TEF668x tuner and an ESP32.
The first target is the ATS-125, a portable with an ILI9341 320x240 touch
display.

**Status: it works as a radio.** Phases 0 to 2 are done and phase 3 is well
under way. The radio joins Wi-Fi, updates itself over the air with rollback,
brings the TEF6686 up with its patch, tunes FM and AM, shows what it is doing
on the panel, and is worked from the knob, the keypad and the volume pot as
well as from a browser and the HTTP control API. It seeks for stations, holds a
squelch, and keeps its settings across a power cycle.

Still to come: touch, LVGL, RDS, memory channels, the volume AGC, telemetry,
the spectrum and the clock. Those are phases 4 to 6 in
[ROADMAP.md](ROADMAP.md).

This is a ground up rewrite, not a fork of running code. It takes its ideas and
its hardware knowledge from [PE5PVB/TEF6686_ESP32](https://github.com/PE5PVB/TEF6686_ESP32),
which is GPLv3, so this project is GPLv3 too.

**Read [DECISIONS.md](DECISIONS.md) before writing any code.** Twenty six design
decisions are settled there with the reasoning behind each. They are not
suggestions. If one of them looks wrong, say so and discuss it, do not quietly
work around it.

[HARDWARE.md](HARDWARE.md) has every fact known about the board: chip, pin map,
I2C addresses, part markings, flash layout. Facts in it were read off the chip
or off the board photos. Do not add anything to it that was guessed.

## Where things are

| | |
|---|---|
| This repo | The working directory of your session |
| Reference firmware | `../TEF6686_ESP32`, a sibling directory. PE5PVB's firmware, which runs on this radio today |
| Serial port | `/dev/cu.usbserial-A5069RR4`, an FT232R with no auto reset |
| Test fixtures | `test/fixtures/`, real captures taken off air from this radio |
| Working rules | [RULES.md](RULES.md). How to work here. Read it first |
| Every gate | `tools/check.sh`. Build, tests, coverage, analysis, format, docs, size |
| Build order | [ROADMAP.md](ROADMAP.md), seven phases. Do not start a phase before the one before it is green |
| Screen design | [docs/design.html](docs/design.html), the proposed screens at 320x240, the type scale and the palette. Also published as an artifact |
| Manual test checklist | [docs/test-checklist.md](docs/test-checklist.md). Numbered checks a person runs on the radio, because CI cannot. Add to it with every build |
| Security audit | [docs/security/](docs/security/), appended to by each phase, never replaced |

The reference firmware is worth reading when a hardware detail is unclear. It is
working code on the same board, so its pin numbers, I2C sequences and tuner
patch blobs are known good. **Do not copy code from it.** This project is a
rewrite and shares no source with it. Read it to learn the hardware, then write
the thing properly.

## How work is done

**Taking a ticket? Use the `build-ticket` skill.** It is in
`.claude/skills/build-ticket/`, and it is the way of working that produced
phases 0 to 3: plan against the decisions, measure thresholds off the radio
instead of guessing them, write the logic in `core/` with its tests, run every
gate, run `/code-review` and `/security-audit`, check the memory and the
structure, flash it, walk the user through the manual checks one at a time,
and make the documents true again before finishing.

**[RULES.md](RULES.md) has the working rules and they are not optional.** Read
it before doing anything. In short, and not as a substitute for reading it:

- Work on `dev`. No feature branches. Nothing is pushed until the user says so.
- Before any code is offered: tests, `/code-review`, `/security-audit`, fix, test again.
- Never commit until the user says so, then one short commit for the whole change.
- `tools/check.sh` runs every gate. CI runs the same script.
- Every library, tool and action pins to its latest stable version, exactly, never a range.
- Every build ships with a numbered manual test checklist.
- One question at a time.

## Build and flash

PlatformIO. One environment per board, plus a native environment for tests.

```bash
pio run -e ats125                 # build for the ATS-125
pio run -e native -t test         # unit tests on this machine, no hardware
pio test -e native -f <filter>    # one test folder, by name
pio run -e ats125 -t upload       # flash over USB, needs the boot button
pio run -e ats125 -t upload --upload-port <ip>   # flash over Wi-Fi
pio check                         # static analysis
```
### The first flash needs the boot button

The ATS-125 uses an FT232R that is not wired for auto reset. A USB upload fails
with `Failed to connect to ESP32: No serial data received` unless the board is
put into download mode by hand: hold BOOT, tap RESET, release BOOT. Ask the user
to do it, wait for them, then upload. Retrying on its own never works.

After the first flash, use over the air. It needs no cable and no person
standing at the radio.

### Reading state off a running radio

Do not add debug prints and reflash. There is no serial cable on this radio, so
ask it over HTTP instead. `GET /api/state` needs no PIN and holds everything:
which slot booted, the tuner identification, the band, the frequency, and the
signal readings.

```bash
curl -s http://tef668x.local/api/state
```

The same document is served at `/status.json`, which is the name the scripts in
`tools/` use. Writes need the PIN. See the control API section in the README.

A JSON parse failure in a polling script looks exactly like a radio that hung.
Print the raw body before deciding the firmware crashed. That mistake cost an
hour once.

Telemetry over UDP, and `tools/telemetry.py` with it, arrives in phase 5.

Captures are useful beyond debugging. Real off air recordings become test
fixtures, which are worth far more than invented test vectors.
`test/fixtures/agc/` already holds five of them, taken from this radio in
September 2026, with a README explaining the format and the cases they cover.
## Architecture

Five layers. The rule that holds it together: **the core must not know a screen
exists.**

```
board/     One header per board. Pin map, display driver, which inputs and
           peripherals exist. Picked by a build flag, one PlatformIO env each.
drivers/   tef668x, display, touch, encoder, keypad, rtc, battery.
           Each behind an interface. No globals.
core/      Tuner state machine, RDS decoder, band plan, memory channels,
           settings, volume AGC. No hardware, no UI. Builds and runs on a PC.
ui/        LVGL screens and menus. Talks to core through an API only.
net/       Wi-Fi, web server, OTA, NTP, telemetry.
```

Two FreeRTOS tasks. Radio on core 0 owns the tuner and the 43 ms RDS cadence.
UI and network on core 1. They talk through a queue each way and a state
snapshot behind a lock. **Nothing mutable is shared without a lock, and no third
task gets added because something feels slow.**

### Things that must not creep back in

These are the failure modes of the firmware this replaces. Each one is why a
decision in DECISIONS.md exists.

- **No global mutable state shared across modules.** The old `gui.cpp` read and
  wrote radio globals directly, which is why none of it could be reused.
- **No settings at hand written byte offsets.** Settings are one versioned
  struct. Adding a setting is adding a field, never editing an address map.
- **No string lookups by index.** Strings have named IDs. Inserting a string
  must never shift another one.
- **No screen drawn twice**, once for encoder and once for touch. LVGL takes
  both as input devices on one UI.
- **No feature deleted to strip the build.** Optional subsystems sit behind
  build flags in the board header.
- **No guessed thresholds.** Signal, modulation and noise limits come from
  measured data. A guessed threshold silently switches a feature off instead of
  failing, and that is hard to spot. This already cost a week on the volume AGC.
