# Building from Source

How to build the firmware, run the tests and the checks, and draw the screen pictures, on your own computer. To put the firmware on a radio, see [First Install over USB](First-Install-over-USB.md) and [Updating](Updating.md).

## Tools

You need Python 3, git and [PlatformIO](https://platformio.org/). The checks also need gcovr and clang-format, at these exact versions:

```bash
pip install platformio==6.2.0 gcovr==8.6 clang-format==23.1.1
```

clang-format must be 23.1.1 and no other. Other versions format this code differently, and the format check refuses them. The one that comes with Xcode on macOS is not accepted.

Every library, tool and platform is pinned to an exact version in `platformio.ini` and the CI workflow, so a build on your computer and a build in CI use the same code.

## Build and test

```bash
git clone https://github.com/zeevy/tef668x-esp32.git
cd tef668x-esp32
pio run -e ats125                 # build the firmware for the ATS-125
pio test -e native                # run the unit tests on your computer, no radio needed
pio test -e native -f test_rds    # run one test folder
```

The firmware is written to `.pio/build/ats125/firmware.bin`.

There are two build environments: `ats125` for the radio, and `native` for the tests. The `native` build holds the parts of the code that touch no hardware: `src/core/`, which is plain C and has all the radio's logic, the screen state builders that turn a radio reading into what a screen shows, the decoding of the tuner's replies, and the web server's record of its reply times. Many of the tests replay real readings taken off a radio: RDS groups, band sweeps and signal readings.

## Every check in one command

```bash
tools/check.sh            # every gate
tools/check.sh format     # one gate by name
```

| Gate | What it checks |
|---|---|
| `build` | The firmware builds. Any compiler warning from `src/` fails it |
| `test` | The unit tests pass |
| `coverage` | The tests cover at least 95 % of the lines in the native build |
| `analysis` | Static analysis with cppcheck, through `pio check` |
| `format` | The code matches `.clang-format`, with clang-format 23.1.1 |
| `fonts` | No font is stored compressed, since this build cannot draw a compressed one |
| `size` | Prints the size of the firmware image |

Every gate runs even when an earlier one fails, so one run shows everything that is wrong. CI runs the same script on every push and every pull request to `master`. Coverage is counted a little differently on macOS and on Linux, and the CI number is the one that counts.

## After changing `src/ui/lv_conf.h`

PlatformIO does not rebuild LVGL when `src/ui/lv_conf.h` changes. The build passes with the old values and no warning. Delete the build folder first:

```bash
rm -rf .pio/build/ats125 && pio run -e ats125
```

## Draw the screen pictures

The screen pictures in `assets/`, also used on these wiki pages, come from the firmware's own drawing code, run on your computer. So they show exactly what the radio draws. The web page pictures in the wiki are taken from the real web page in a browser.

```bash
pio run -e ats125                                 # once, so LVGL is downloaded
tools/screenshot.sh && python3 tools/make_assets.py
```

`tools/screenshot.sh` draws every screen into `.pio/shots/`. It exits with 1, and names the picture and the character, when a text holds a character its font does not have, since that draws as nothing and is easy to miss by eye. `tools/make_assets.py` turns the drawings into the PNG files in `assets/`.

## Making a release

Releases are made by the release workflow when the owner pushes a version tag. Only the owner can push a tag.

1. Raise `FIRMWARE_VERSION` in `src/core/version.h`, as `x.y.z`, in a pull request, and merge it.
2. Push the tag `v` + that version on the merged commit, for example `v0.2.0`.
3. The workflow checks that the tag matches `FIRMWARE_VERSION` and is on `master`, runs every gate, builds, and makes a **draft** release with the notes from the merged pull requests and three files: `firmware-ats125.bin`, `firmware-ats125-full.bin` and `manifest-ats125.json`.
4. Flash the draft's `firmware-ats125.bin` to a radio and run the checks that CI cannot do: the display, the tuner and the audio.
5. Publish the draft: `gh release edit v0.2.0 --draft=false --latest`.

A tag with a suffix, such as `v0.2.0-rc.1`, makes a prerelease instead: public at once, for testing, and never marked Latest. Each file has an attestation of the workflow and commit that built it, which you can check with `gh attestation verify firmware-ats125.bin -R zeevy/tef668x-esp32`.

## Where things are

| Folder | What it holds |
|---|---|
| `src/board/` | One header per radio: the pin map, the screen size and the feature flags |
| `src/drivers/` | The tuner, the screen, the knobs and keys, the battery, and the flash stores |
| `src/core/` | The radio's logic in plain C: band plan, RDS decoder, presets, settings, menu, seek, squelch, AGC, DX. No hardware, so it runs on a PC |
| `src/ui/` | The LVGL screens and panels |
| `src/net/` | Wi-Fi, the web server and API, updates and rollback, network time |
| `src/lang/en.h` | Every text the screen shows, one line each |
| `test/` | The unit tests, one folder per part |
| `tools/` | `check.sh`, the screenshot tool and the build scripts |

The README's [Firmware architecture](https://github.com/zeevy/tef668x-esp32#firmware-architecture) section has diagrams of the layers, the two tasks, the start up and the update.
