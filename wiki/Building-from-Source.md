# Building from Source

How to build the firmware, run the tests and the checks, and draw the screen pictures, on your own computer. To put the firmware on a radio, see [First Install over USB](First-Install-over-USB.md) and [Updating](Updating.md).

## Tools

You need Python 3, git and [PlatformIO](https://platformio.org/). The checks also need gcovr and clang-format, at these exact versions:

```bash
pip install platformio==6.2.0 gcovr==8.6 clang-format==23.1.1
```

clang-format must be 23.1.1 and no other. Other versions format this code differently, and the format check refuses them. The one that comes with Xcode on macOS is not accepted.

The web page's check, and any change to the web page, also need [Node](https://nodejs.org/) 24.21.0, the version CI uses. Building the firmware alone does not need Node.

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

## The web page

The radio's [web page](Web-Page.md) is built from `web/`, with Preact and Tailwind CSS, into one file, `web/dist/index.html`, with its script and styles inside it. `tools/web_page.py` gzips that file and writes it into `src/net/web_page.h`, which is committed, so the firmware build uses it as it is. The radio stores it gzipped in flash and sends it as it is.

After a change in `web/`, build the page and write it into the firmware:

```bash
npm --prefix web ci                                       # once, and after web/package.json changes
npm --prefix web run build && python3 tools/web_page.py   # prints the page's gzipped size
```

While you work on the page, `npm --prefix web run dev` serves it on your computer with answers recorded from a radio, so no radio is needed. `npm --prefix web run dev:radio` serves it with every call sent to the radio at `http://tef668x.lan:8080`; set `RADIO` for another address, for example `RADIO=http://192.168.1.40:8080`.

`npm --prefix web test` runs the page's browser tests, at a phone size and a laptop size, against the recorded answers. They need their browser once: `cd web && npx playwright install --only-shell chromium`.

## The first build takes longer

The firmware changes one setting inside the framework's own ESP-IDF libraries: HTTPS keeps 4 KB for what it sends instead of 16 KB, which leaves about 12 KB more memory free while the radio checks for an update. `custom_sdkconfig` in `platformio.ini` asks for this, and the platform then rebuilds those libraries once, on the first build. That build downloads ESP-IDF and about 30 of its components and takes a few minutes more. Later builds reuse the rebuilt libraries.

`tools/pin_components.py` keeps every component at the version the framework's own libraries were built with, and stops the build if one comes out different. The rebuild writes `.dummy/`, `managed_components/`, `sdkconfig.defaults` and `sdkconfig.ats125` in the project folder, and git ignores them.

The rebuilt libraries replace the stock ones in PlatformIO's package folder. Another project on the same computer that uses the same platform without these settings puts the stock libraries back by itself, and the next build here rebuilds them again.

## Build environments

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
| `fonts` | The fonts hold every letter the language files use, and only the faces of letters past ASCII and of Chinese are stored packed, which LVGL then unpacks |
| `english` | Nothing under `src/net/` asks for a text in the screen's language: the web page, the API and the files are English |
| `web` | The web page builds, its browser tests pass, and `src/net/web_page.h` is what the page builds to now. Needs Node 24.21.0 |
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

To draw every screen in another language, give its code. The boot and recovery screens stay English, as on the radio:

```bash
SHOT_LANGUAGE=de tools/screenshot.sh /tmp/de
tools/screenshot.sh /tmp/en
python3 tools/text_fit.py /tmp/de --base /tmp/en
```

In any language the screenshot tool also fails on a letter drawn from a font of another size, and on a text cut short by the buffer it was written into, naming the file and line. Beside each picture it writes what the screen shows, as `GET /api/screen` gives it on the radio. `tools/text_fit.py` reads those, or answers saved from the radio, and lists every text that runs past its box, off the screen or into another text, or that the screen had to cut short with `…`, leaving out what English does too.

The pictures in `assets/` and on this wiki are English only.

## The fonts

The screen's fonts are generated C files in `src/ui/fonts/`, made by `tools/make_lvgl_font.py` from Roboto Condensed, Noto Sans SC (for Chinese) and Material Symbols. The tool's own help names the exact files and where to get them; it stops when the Roboto Condensed or Noto Sans SC file is not the one the fonts were made from. Each text face falls back to a face of the extra letters at its own size, and that one to a Chinese face, so a letter English does not use comes from a font of the same size, except that 15 px text takes its Chinese at 13 px. The tool reads every language file and puts every letter they use into the faces. Run it after changing a language file, then `clang-format` over the fonts:

```bash
python3 tools/make_lvgl_font.py --font "RobotoCondensed[wght].ttf" \
    --icons "MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf" --cjk "NotoSansSC[wght].ttf"
clang-format -i src/ui/fonts/*.c
```

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
| `web/` | The web page: its screens, its browser tests in `web/tests/`, and the radio's answers it is tested against in `web/mock/` |
| `src/lang/en.h` | Every text the screen shows, one line each |
| `src/lang/<code>.h` | The same texts in another language |
| `test/` | The unit tests, one folder per part |
| `tools/` | `check.sh`, the screenshot tool, `web_page.py` and the build scripts |

The README's [Firmware architecture](https://github.com/zeevy/tef668x-esp32#firmware-architecture) section has diagrams of the layers, the two tasks, the start up and the update.
