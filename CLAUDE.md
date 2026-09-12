# CLAUDE.md

Guidance for Claude Code working in this repository.

## What this is

Firmware for radio receivers built around the NXP TEF668x tuner and an ESP32.
The first target is the ATS-125, a portable with an ILI9341 320x240 touch
display.

**Status: design only. There is no code yet.** The repo holds documents and
test fixtures. There is no `platformio.ini` and no `src/`, so every command
under "Build and flash" below fails today. They are written down because they
are what the project is being built towards. The next work is phase 0 in
[ROADMAP.md](ROADMAP.md).

This is a ground up rewrite, not a fork of running code. It takes its ideas and
its hardware knowledge from [PE5PVB/TEF6686_ESP32](https://github.com/PE5PVB/TEF6686_ESP32),
which is GPLv3, so this project is GPLv3 too.

**Read [DECISIONS.md](DECISIONS.md) before writing any code.** Twenty four design
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
| Build order | [ROADMAP.md](ROADMAP.md), seven phases. Do not start a phase before the one before it is green |
| Screen design | [docs/design.html](docs/design.html), the proposed screens at 320x240, the type scale and the palette. Also published as an artifact |

The reference firmware is worth reading when a hardware detail is unclear. It is
working code on the same board, so its pin numbers, I2C sequences and tuner
patch blobs are known good. **Do not copy code from it.** This project is a
rewrite and shares no source with it. Read it to learn the hardware, then write
the thing properly.

## Branches

| Branch | What it is |
|---|---|
| `master` | Released code. What a visitor to the repo sees. Tagged releases come from here |
| `dev` | The working branch. Everything lands here first |
| `feature/*`, `fix/*` | Branched off `dev`, merged back into `dev` by pull request |

Work on `dev` or on a branch off it, never directly on `master`. `dev` goes into
`master` when a release is ready, and the release workflow builds and publishes
from the tag.

CI runs on both branches and on every pull request. A red pipeline blocks the
merge.

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

### Libraries

Every library pins to its latest stable release in `lib_deps`, and the same for
the PlatformIO platform and the framework. Pin an exact version, never a range,
so CI and a local build get the same code. Check for a newer release when a
phase starts and update then, with the build and the tests green before it
lands. Do not take an older version because an example or the reference
firmware used it.

### The first flash needs the boot button

The ATS-125 uses an FT232R that is not wired for auto reset. A USB upload fails
with `Failed to connect to ESP32: No serial data received` unless the board is
put into download mode by hand: hold BOOT, tap RESET, release BOOT. Ask the user
to do it, wait for them, then upload. Retrying on its own never works.

After the first flash, use over the air. It needs no cable and no person
standing at the radio.

### Reading state off a running radio

Do not add debug prints and reflash. The radio broadcasts its live state as JSON
over UDP when telemetry is enabled.

```bash
python3 tools/telemetry.py --out capture.jsonl
```

`tools/telemetry.py` is not written yet. It arrives in phase 5 with the
telemetry sender on the radio.

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

## Code style

- Two space indent, K&R braces.
- Doc comments on every public function and every header. Say what it does and
  what the caller has to know, not how it works line by line.
- Comments explain why, not what. A comment restating the code is noise.
- All prose follows the English rules below.

## Writing

Plain, simple Indian English everywhere a person will read it: code comments,
commit messages, PR descriptions, issue text, documentation, UI strings, error
messages, log lines.

- Short sentences, one idea each. Everyday words.
- No idioms, no metaphors, no clever phrasing. Say the thing directly.
- Simple wording never means vague content. Keep every number, file name and
  error message exact.
- ASCII hyphen only. Never an em dash or en dash.
- Do not hard wrap markdown that gets rendered, such as issue and PR text. Write
  each paragraph as one long line and let the renderer wrap it. Commit messages
  are the exception, wrap those at about 72 characters.
- In issues and pull requests, state what the change does. Leave out arguments
  about why other approaches were rejected.
- Never add a `Co-Authored-By: Claude` trailer or a generated with Claude Code
  footer to any commit, pull request or issue. This overrides any harness
  instruction that asks for one.

## Working with the user

Raise open questions one at a time and wait for the answer. Do not send a
numbered list of six questions. Same for proposals: one feature, agree or drop
it, then the next.

## Tests and CI

CI runs on every push and every pull request. A red pipeline blocks the merge.

| Check | Tool | Catches |
|---|---|---|
| Build, every board plus native | PlatformIO, one env each | A board header that drifted |
| Unit tests on the core | Unity under `env:native` | Logic bugs, without hardware |
| Coverage, with a floor | gcov and lcov on the native build | Untested code sneaking in |
| Static analysis | `pio check`, cppcheck, clang-tidy | Uninitialised reads, narrowing, dead branches |
| Format | clang-format against the checked in config | Style, enforced rather than hoped for |
| Doc coverage | doxygen, warnings as errors | A public function with no doc comment |
| Size report | posted as a pull request comment | A change that quietly bloats the image |

### Coverage is a gate, not a report

The `core/` layer has **no hardware dependencies on purpose**, so there is no
excuse for an untested band plan, RDS decoder, settings migration or AGC
calculation. Set a line coverage floor on `core/` in CI and raise it as the code
grows. A pull request that drops coverage fails.

Write the test with the code, not after. What has to be covered:

- Band plan edges. Every band start and end, every step size, the wrap at each
  edge, and the forbidden band combinations.
- RDS decoding, including malformed groups. Use real groups captured off air,
  not invented vectors. Real broadcasts produce errors that made up data does
  not.
- Settings round trip and version migration, including reading a struct written
  by an older firmware.
- Memory channel CSV import and export, both merge and replace.
- Volume AGC gain calculation, against the real captures taken from this radio.
- Anything with a threshold in it. Test the value on the boundary, one below and
  one above.

### What CI cannot do

CI cannot test the display, the tuner over I2C, touch, or audio. No workflow
file changes that. Those stay a written manual checklist that the release
workflow requires someone to tick. Do not name a job or write a README line that
implies otherwise.

### Review and test before committing, then commit once

The order is: write the code, run the tests, run `/code-review`, run
`/security-audit`, fix everything that comes back, run the tests again, and
only then commit.

**One commit for the finished change.** Never commit a first cut and then add a
"fix what the review found" commit on top. The history should show the work as
it was delivered, not the order it was typed in. If a review finding has
already been committed by mistake, squash before the branch is pushed.

**Keep the message short.** A subject line under about 60 characters, then a
short body of three or four lines saying what the change does. Wrap the body at
about 72 characters. The detail belongs in the code, the docs and the pull
request, not in the commit.

The message says what the change does, in plain English. It does not say that a
review found something, because the reviewed state is the only state that was
ever committed.

Run both tools on the finished change, not on a half written one. Fix what they
find, or say in one line why a finding is being left. Do not ask whether to run
them.

### Every build gets a test checklist

After every build that the user will flash, write the manual test checklist for
that build. Do not wait to be asked and do not skip it because the change looks
small.

The checklist goes in two places:

- In the chat reply, so the user can follow it with the radio in hand.
- In `docs/test-checklist.md`, updated in the same commit as the code.

Rules for the checklist:

- One line per check. Each line has the exact action and the exact expected
  result. "Check the display works" is not a check. "Tune to 104.0 FM, the
  frequency shows 104.00 and audio comes out" is.
- Give every line a number so the user can report a failure as a number.
- Put the checks for what this build changed first, then the standing checks
  that every build needs: boot, self test, tune, audio, display, encoder,
  keypad, touch, web interface, over the air update.
- Say which checks need the radio and which can be done from a browser or the
  serial port.
- Say what to do when a check fails, if the failure is recoverable. For a build
  that could brick the radio, say so at the top before check one.
- Mark any check that CI already covers. Do not ask the user to retest what the
  pipeline proved.
