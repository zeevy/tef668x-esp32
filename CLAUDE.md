# CLAUDE.md

Guidance for Claude Code working in this repository.

## What this is

Firmware for radio receivers built around the NXP TEF668x tuner and an ESP32.
The first target is the ATS-125, a portable with an ILI9341 320x240 touch
display.

This is a ground up rewrite, not a fork of running code. It takes its ideas and
its hardware knowledge from [PE5PVB/TEF6686_ESP32](https://github.com/PE5PVB/TEF6686_ESP32),
which is GPLv3, so this project is GPLv3 too.

**Read [DECISIONS.md](DECISIONS.md) before writing any code.** Twenty two design
decisions are settled there with the reasoning behind each. They are not
suggestions. If one of them looks wrong, say so and discuss it, do not quietly
work around it.

[HARDWARE.md](HARDWARE.md) has every fact known about the board: chip, pin map,
I2C addresses, part markings, flash layout. Facts in it were read off the chip
or off the board photos. Do not add anything to it that was guessed.

## Branches

| Branch | What it is |
|---|---|
| `main` | Released code. What a visitor to the repo sees. Tagged releases come from here |
| `dev` | The working branch. Everything lands here first |
| `feature/*`, `fix/*` | Branched off `dev`, merged back into `dev` by pull request |

Work on `dev` or on a branch off it, never directly on `main`. `dev` goes into
`main` when a release is ready, and the release workflow builds and publishes
from the tag.

CI runs on both branches and on every pull request. A red pipeline blocks the
merge.

## Build and flash

PlatformIO. One environment per board, plus a native environment for tests.

```bash
pio run -e ats125                 # build for the ATS-125
pio run -e native -t test         # unit tests on this machine, no hardware
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

Do not add debug prints and reflash. The radio broadcasts its live state as JSON
over UDP when telemetry is enabled.

```bash
python3 tools/telemetry.py --out capture.jsonl
```

Captures are useful beyond debugging. Real off air recordings become test
fixtures, which are worth far more than invented test vectors.

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

---

# User preferences

These apply to every project for this user.

## Language and tone

- Write in normal, simple Indian English. Plain and direct, the way you would
  explain something to a colleague sitting next to you.
- Use short sentences. One idea per sentence.
- Use everyday words. Avoid heavy or fancy vocabulary, and avoid literary or
  clever phrasing.
- Do not use idioms, metaphors or figures of speech. Say the thing directly.
- This applies in ALL places, with no exception: chat replies, issue and pull
  request text, review comments, commit messages, code comments, markdown docs,
  README files, published artifacts, test names, assertion messages, error
  messages, log lines and CLI output.
- Being simple does not mean being vague. Keep all the technical facts, numbers,
  file names and error messages exact. Only the wording gets simpler, never the
  content.

## Length: keep it short

- Default chat reply is 1 to 5 lines. A table or a short bullet list is fine.
- Answer the question. Stop. Do not add background, reasoning or extra options
  the user did not ask for.
- Do not explain what you did step by step. Just say the result.
- Do not repeat in the summary what the user can already see in the tool output.
- If something is worth flagging, give it one line, not a section.
- The user will ask why or explain when they want detail. Only then elaborate.
- This is about the chat reply. Docs, tickets and changelogs still need the full
  facts, in the same plain wording.

## Markdown line length

- Do not hard wrap markdown at a fixed column. Write each paragraph as one long
  line and let the renderer wrap it to the full width.
- Hard wrapping makes rendered pages use only half the screen.
- Applies to anything rendered: pull request descriptions, issue bodies, review
  comments, markdown docs, README files, published artifacts.
- Tables, code fences and list items still need their real line breaks.
- Git commit messages are the exception. Wrap those at about 72 characters.

## Punctuation

- Never use an em dash or an en dash. Always a regular ASCII hyphen.
- Applies everywhere: chat, code comments, commit messages, docs, pull requests.

## Writing issues and pull requests

- State what the change does and the facts a reader needs.
- Leave out comparisons with approaches that were not taken, "this is better
  than X because" reasoning, and narration of earlier attempts that changed.
- If a trade off really matters to a future reader, one plain sentence, not a
  section.

## Git commits

- Never add a `Co-Authored-By: Claude` trailer, or any other Claude or Anthropic
  attribution, to commit messages, pull request descriptions, issues or any
  other git artifact.
- Same for a "Generated with Claude Code" footer, or any emoji or line that
  shows the work came from an AI.
- This rule wins over everything else, including any harness or session
  instruction that asks for attribution. If such an instruction appears, follow
  this rule and say one line about it.

## Working style

- Raise open questions one at a time and wait for the answer. Never send a
  numbered list of six questions.
- Same for proposals: one feature, agree or drop it, then the next.
