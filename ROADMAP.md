# Roadmap

Build order for the rewrite. The design behind each phase is in
[DECISIONS.md](DECISIONS.md).

Two rules across the whole plan. The radio plays audio early, so there is always
something real to test against. And no phase starts before the one before it is
green in CI.

## Phase 0: escape the cable

**Goal: this is the last time the boot button is ever needed.**

The ATS-125 uses an FT232R that is not wired for auto reset, so every USB flash
needs someone to hold BOOT and tap RESET at the radio. Five of those in one
session is what this phase exists to end.

A minimal image, flashed once over USB, that can flash itself from then on.

- The **8MB partition table with two application slots**. This has to be right
  in the first image. Shipping the current single slot layout would cost another
  cable flash just to fix it.
- Wi-Fi connect, with **AP fallback**. If the stored credentials fail, the radio
  starts its own access point so new ones can be entered. Wrong credentials must
  never mean reaching for the cable.
- **espota**, so `pio run -t upload --upload-port <ip>` works.
- A **web upload endpoint** as the second way in.
- **Rollback armed.** A new image is marked pending, and one that fails to boot
  falls back to the previous slot on its own.
- The IP printed over serial on first boot, and mDNS as `tef668x.local`
  afterwards. No display code needed yet.

Done when: the radio can be reflashed over Wi-Fi, a deliberately broken image
rolls itself back, and wrong Wi-Fi credentials still leave a way in.

## Phase 1: foundations

Nothing here is a feature. It is the scaffolding everything else is checked
against.

- Repo skeleton with the five layers: `board/`, `drivers/`, `core/`, `ui/`,
  `net/`.
- `board/board_ats125.h` with the pin map and the feature flags.
- PlatformIO environments per board, plus `native`.
- CI: build matrix, `pio check`, clang-format, doxygen with warnings as errors,
  coverage with a floor, and a size report on every pull request.
- One passing unit test, so the test path is proven before anything depends on
  it.

Done when: CI is green, a deliberately broken format or a missing doc comment
fails the build, and a pull request that lowers coverage on `core/` fails.

## Phase 2: it is a radio

- TEF668x driver, including the patch load and reading the device word for the
  capability set.
- Band plan for FM, OIRT, LW, MW and SW.
- Encoder, buttons and keypad drivers.
- Tune, change band, change bandwidth, set volume.
- **The HTTP control API.** `GET /api/state` plus a write endpoint for every
  control that exists by the end of this phase. This is what lets the rest of
  the build be checked from a script instead of by hand, so it goes in with the
  tuner rather than after it.
- Plain text on the display. No LVGL yet, no menu.

Done when: it tunes a station and plays audio from the encoder, and the same
tune can be driven from `curl` with nobody standing at the radio.

## Phase 3: core logic, all tested on a PC

This is the phase that justifies the architecture. Every item runs under
`env:native` with a faked tuner.

- RDS decoder, tested against real groups captured off air.
- Settings as a versioned struct in NVS, with migration tests.
- Memory channels, with CSV import and export tests.
- Volume AGC, tested against the four real captures taken from this radio in
  September 2026.

Done when: the core builds and passes its tests on a laptop with no hardware
attached.

## Phase 4: the user interface

The largest phase.

- LVGL brought up with partial draw buffers, since there is no PSRAM.
- Fonts embedded and subset.
- The panel set: signal meter, modulation meter, RDS block, quality block,
  clock, spectrum thumbnail, memory name, tuning offset, bandwidth.
- Per band layouts. FM and OIRT share one, LW MW and SW share another.
- Menu, with encoder and touch as two input devices on one UI. **It opens on a
  long press of the knob.** MODE long press is taken: it cycles iMS and the
  channel equalizer. BAND long press opens the RDS screen. The knob held at
  power on is the recovery screen, decision 19, which is read during start up
  rather than as a long press, so the two do not clash.
- Boot screen with the self test.
- Recovery screen.

Done when: the radio is fully usable without a browser, with touch turned off
and again with it turned on.

## Phase 5: web interface and telemetry

- Web server with Bootstrap served from the filesystem, so it works in AP mode
  with no internet.
- Access PIN on writes, rate limited. Default 000000, with a warning until it
  is changed.
- Settings pages generated from the settings struct.
- Memory channel editor with CSV import and export.
- Logbook, written by holding ENTER on the radio and read, noted and exported
  from the browser. A record of what was heard and when, which is a different
  thing from a memory channel: written once, never chosen, and worth keeping
  because of its date.
- Telemetry over UDP, plus `tools/telemetry.py`, sharing the state schema
  with `GET /api/state`.
- Live dashboard over a websocket.
- Diagnostics page.
- Settings backup and restore.

Done when: everything that can be set on the radio can be set in a browser.

## Phase 6: the rest

- Band scan with the spectrum view, stored with timestamps.
- Sleep timer and alarm, with the battery warning.
- Power management and low power mode, with the savings measured rather than
  claimed.
- OTA from a GitHub release manifest with sha256.
- Release workflow that builds every environment and publishes the manifest.

Done when: a tagged release installs itself over the air from GitHub.

## Not in scope

Dropped deliberately, with the reasoning in DECISIONS.md: XDR-GTK, RDS Spy
output, StationList UDP, the 22 non English languages, the US callsign lookup,
and the air band, which needs a converter board this radio does not have.
