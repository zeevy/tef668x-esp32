# Design decisions

Settled points for the ATS-125 firmware rewrite. Anything not in here is still
open. Hardware facts live in [HARDWARE.md](HARDWARE.md).

## Settled

### 1. Name: tef668x-esp32

Tuner family first, because that is what defines the project. The ESP32 is the
host. `tef668x` rather than `tef6686` says the code covers the family, and keeps
it distinguishable from PE5PVB's `TEF6686_ESP32` while still findable by anyone
searching for that.

Not `esp32u`. The U in ESP32-WROOM-32U only means external antenna connector,
which is a packaging variant and changes no code. Another board in this project
could use a WROOM-32, a WROVER or an ESP32-S3.

This radio is a board inside the project, not the project:

| | |
|---|---|
| Repo | `tef668x-esp32` |
| Board header | `board/board_ats125.h` |
| Build env | `[env:ats125]` |
| Release asset | `tef668x-esp32-ats125-v1.0.0.bin` |
| mDNS and AP name | `tef668x` |

### 2. Modular, board agnostic

The code is split so it can be built for other TEF668x radios later, not just
this one.

```
board/     One header per board. Pin map, display driver, which inputs and
           peripherals exist. Picked by a build flag, one PlatformIO env each.
drivers/   tef668x, display, touch, encoder, keypad, rtc, battery.
           Each behind an interface. No globals.
core/      Tuner state machine, RDS decoder, band plan, memory channels,
           settings, volume AGC. No hardware, no UI. Builds on a PC.
ui/        LVGL screens and menus. Talks to core through an API only.
net/       Wi-Fi, web server, OTA, NTP, remote protocols.
```

The rule that makes it work: **the core must not know a screen exists.** In the
PE5PVB firmware `gui.cpp` reads and writes radio globals directly, which is why
none of it can be reused on another board.

### 3. LVGL for the user interface

The UI is built on LVGL, not hand drawn to TFT_eSPI.

The reason is the encoder and touch split. In the PE5PVB firmware every menu row
is written twice, once in `ShowOneLine()` for the encoder models and once in
`ShowOneButton()` for the touch model. The two volume AGC rows added in
September 2026 were each written twice. That duplication is why the menu is
stuck at a small font and why the touch and non touch paths drift apart.

LVGL treats a rotary encoder and a touchscreen as two input devices feeding one
UI, through focus groups. Touch becomes a setting, not a second code path. The
rest follows: scrollable lists, any TTF at any size with anti aliasing, and real
theming, which is what a redesign needs.

What this costs on this hardware:

| | |
|---|---|
| Flash | About 150KB, against 8MB fitted |
| RAM | No PSRAM, so partial draw buffers. Two of about 20KB each is normal for 320x240 and leaves most of the 320KB free |
| SPI | The panel runs at 7.5 MHz write. Partial redraws are fine, full screen animation will not be smooth |
| Effort | The largest single piece of the rewrite |

The existing drawing code is not ported. It is 6000 lines of absolute
coordinates and it is the thing being replaced.

### 4. Screens are built from panels, and each band gets its own layout

The main screen is not one fixed picture. A set of panels is defined once and
each band's screen is an arrangement of them.

Panels: signal meter, modulation meter, RDS block, quality block, clock,
spectrum thumbnail, memory name, tuning offset, bandwidth.

Why per band layouts: the PE5PVB screen is FM's screen with AM squeezed into it.
`PS:`, `RT:`, `PTY:` and `PI:` take the bottom third and are permanently empty
on AM, while the things that matter on AM are tiny or missing.

What goes on the AM screen with that space back:

| Panel | Why it matters on AM |
|---|---|
| Bandwidth, large | It is changed constantly to fight the adjacent channel. Today it is 20 pixels in a corner |
| Co-channel detector | `getStatusAM()` already returns it and nothing displays it |
| Tuning offset | AM is deliberately tuned off centre for selective fading. Today it is a small corner number |
| RF attenuation | Already a setting, buried in a menu |
| Noise blanker state | Matters far more on AM than on FM |
| Meter band, SW only | 49m, 41m, 31m. The data exists and is shown as an afterthought |
| Clock, large | Shortwave listening is schedule driven |

Shipping arrangement: FM and OIRT share one, LW MW and SW share another with SW
adding the meter band. Two arrangements, not five.

Composition rather than a drawing function per screen means adding a band, or
later letting the user rearrange their own layout, does not mean writing another
500 lines. Same principle as the board headers: describe what goes where, do not
hard code the picture.

### 5. Capabilities are detected, never hard coded

This unit has a TEF6686, which has no FMSI, no full search RDS and no digital
radio. The tuner driver still reads the device word at startup and publishes a
capability set. On a board with a TEF6687 or TEF6689 those features appear. Here
the UI simply does not offer them.

Same idea for the board: the board header says the hardware can have touch, a
setting says whether to use it.

### 6. English only, with the mechanism kept for more

Ship English and nothing else. But build the string system so a language can be
added later without touching a single existing string.

How: strings live in `lang/en.json`. A build script turns that into a header
holding a named enum and a table. UI code refers to `STR_VOLUME_AGC`, never to a
number. Adding a language is adding `lang/xx.json`.

Why not the current approach: the PE5PVB firmware keeps 23 parallel arrays and
every string is found by its index, so a new string has to be inserted at the
same position in all 23 blocks or the entire UI shifts. That is why the volume
AGC menu item added in September 2026 had to be assembled from recycled words
instead of getting a proper label. Named IDs remove that whole class of bug.

A language added later still needs font coverage. Telugu, for example, needs
combining vowel signs that the current per codepoint bitmap fonts cannot
compose. LVGL handles this better but it is still real work.

### 7. Two FreeRTOS tasks, not one cooperative loop

| Task | Core | Job |
|---|---|---|
| Radio | 0 | Tuner I2C, the 43 ms RDS group cadence, signal and modulation reads, the volume AGC |
| UI and net | 1 | LVGL, web server, OTA, telemetry |

They talk through a queue each way and a shared state snapshot behind a lock.
No mutable state is shared without one.

What this replaces: the PE5PVB firmware runs one long `loop()` on `millis()`
timers and guard flags. Its own notes say long work must not block or RDS
decoding, touch and the web server all stall. The effects are visible in the
code. The web server only runs when a menu is closed. `readRds()` is called from
several different places to keep up with the group cadence. Adding the volume
AGC in September 2026 meant checking seven mode flags to decide whether it was
allowed to run at all.

With a separate radio task, RDS keeps its timing no matter what the UI is doing
and a slow web request cannot drop groups.

What it costs: concurrency bugs, which are harder to find than the ones this
design removes. The mitigation is discipline, not cleverness. Exactly two tasks.
One queue in each direction. Nothing shared without a lock. Tasks do not get
added later because something feels slow.

### 8. Settings are one versioned struct, not an address map

Settings live in a single `Settings` struct with a `version` field, stored in
NVS. Adding a setting is adding a field.

```c
struct Settings {
    uint16_t version;
    uint8_t  agcTarget;   // 0 is off, else 30 to 80 percent
    uint8_t  agcBoost;    // 0 is off, else 2 to 8 dB
    int8_t   volume;
    uint8_t  theme;       // index into the theme table
    uint8_t  layout;      // index into the layout table
    // ...
};
```

What this replaces: the PE5PVB firmware maps every setting to a hand written
byte offset in `constants.h`. Adding one means editing six places and bumping
`EE_TOTAL_CNT` in both `HAS_AIR_BAND` branches.

Two failure modes of that design, both hit in September 2026:

- Changing the layout means bumping `EE_CHECKBYTE_VALUE`, which silently wipes
  every user setting on the next boot.
- Turning on `HAS_AIR_BAND` shifts `EE_BYTE_WIFI_STATICIP` from 2287 to 2292, so
  the Wi-Fi settings read back as garbage.

Neither is a coding mistake. They are properties of the design.

Upgrades are handled by the `version` field and a migration function, so a
firmware update never wipes settings.

Three things follow:

1. The web UI is nearly free. The struct serialises straight to JSON, so
   settings read and write over HTTP with no separate mapping layer.
2. Backup and restore is downloading that JSON and uploading it back.
3. It is testable. Round trip and migration tests run under `env:native` with no
   hardware.

The tradeoff: NVS writes are slower than raw EEPROM and use more flash for the
same data. Neither matters here, settings are written once on leaving a menu.

### 9. No XDR-GTK, no RDS Spy, no StationList

All three remote protocols from the PE5PVB firmware are dropped.

XDR-GTK is the one that mattered. It takes over volume, mute and bandwidth, so
every one of those paths in the current firmware carries a
`XDRGTKUSB || XDRGTKTCP` check. The volume AGC added in September 2026 had to
stand down entirely while it was connected. Dropping it removes that
cross-cutting condition from the whole codebase.

RDS Spy output and StationList UDP are cheap on their own but are not used here.

### 10. Telemetry instead of debug builds

The radio broadcasts its live state so it can be watched without a cable and
without a special build.

| | |
|---|---|
| Transport | UDP, LAN broadcast or unicast to a configured address |
| Format | One JSON object per line, with a schema version and a sequence number |
| Contents | Frequency, band, signal, USN, WAM, modulation, AGC average and gain, RDS PI and PS, stereo, battery, free heap, uptime, Wi-Fi RSSI |
| Rate | 1 Hz by default, up to 10 Hz while tuning something |
| Default | Off. Turned on in settings or from the web UI |
| Host tool | `tools/telemetry.py` listens and writes JSONL |

The same data is also served over a websocket for a live dashboard in the web
UI. UDP is for capturing to a file and analysing later, the websocket is for
watching in a browser.

Why this is worth building early: tuning the volume AGC on the PE5PVB firmware
took four flash cycles, each needing a debug `#define`, a manual boot mode
entry, a wired serial capture, and then stripping the debug code back out.
Telemetry makes the shipping firmware observable, so what gets debugged is what
gets released.

Captures also become test data. The four AGC captures taken from this radio in
September 2026 are real off air recordings, and under CI they turn into
regression tests worth far more than invented test vectors.

Two rules for the packet: no credentials in it ever, and the schema version and
sequence number are not optional. Version so old captures stay readable,
sequence so dropped packets show up instead of silently skewing an analysis.

Note that UDP broadcast on a LAN is unauthenticated and unencrypted. Anyone on
the network can see what the radio is tuned to. Default off covers this.

### 11. Flashing and updates

USB with the boot button is the first flash only. Everything after that is over
the air.

| Path | Who uses it | Phase |
|---|---|---|
| USB serial, hold BOOT and tap RESET | First flash of a blank board | Day one |
| espota, `pio run -t upload --upload-port <ip>` | Development | Day one |
| Web upload, pick a `.bin` in the browser | The user | Day one |
| GitHub release manifest with sha256 | Everyone | Later phase |

**Phase 0 of the build is a minimal image that can flash itself.** Wi-Fi, OTA
and the partition table, and nothing else. It goes on over USB once, and after
that the cable and the boot button are never needed again. Wrong Wi-Fi
credentials start an access point rather than forcing a cable flash, so there is
no way back to the cable by accident.

**The first USB image must already carry the OTA partition table.** Two
application slots, `otadata`, and the larger filesystem. Shipping the current
single slot layout would cost a second cable flash just to fix it.

```
nvs       data nvs      0x9000    0x5000
otadata   data ota      0xE000    0x2000
app0      app  ota_0    0x10000   0x300000
app1      app  ota_1    0x310000  0x300000
littlefs  data spiffs   0x610000  0x1F0000
```

Rollback is armed. A new image is marked pending, and if it fails to boot or
fails its self check the bootloader falls back to the previous slot. A bad
update never costs a cable flash.

Why espota matters more than it looks: on the PE5PVB firmware every test cycle
needs someone standing at the radio to hold BOOT and tap RESET, because the
FT232R is not wired for auto reset. That happened five times in one session in
September 2026. Over the air removes the person from the loop.

### 12. Access PIN

A six digit PIN protects anything that changes the radio. It can be changed in
settings, and a factory reset brings the default back.

**The default is 000000, and the radio says so.** A radio still on the default
prints a warning on every boot and shows a red banner on its own web page.
Changing it is the user's call.

The first design derived the default from the MAC address, so that every radio
had a different one and a factory reset restored it without storing it. That was
dropped on 12 September 2026. The MAC is in the header of every frame the radio
sends, and the derivation is eight lines in this repository, so the default was
public while looking private. A default that is obviously public is safer than
one that is secretly public, because nobody is left thinking they are protected
when they are not.

Six digits rather than four: a million combinations against ten thousand, at the
cost of two extra key presses.

**Rate limited.** Five wrong attempts locks the endpoint for a minute. Without
that, six digits fall to a script in minutes.

**Writes only.** The dashboard, the live telemetry and the memory channel list
are open to view without a PIN, so the radio stays a glanceable page on a phone.
The PIN is required to change settings, edit memory channels, upload firmware or
factory reset. A session cookie after entry avoids retyping it.

### 13. Memory channels in the web UI

All 99 slots in a table in the browser. Edit frequency, band, bandwidth and name
inline, reorder by dragging, delete, and import or export CSV.

**The CSV conversion runs on the radio, in `core/memory_csv.c`.** This was the other way round when the decision was first written, with the browser doing the conversion so the ESP32 never had to. It was changed on 13 September 2026, in phase 3, for three reasons. The parser and the writer cost about 1.5 kB of flash and no permanent RAM, which is not the saving it looked like. Everything that decides anything belongs in `core/` where it is tested on a PC, and both RULES.md and the phase 3 ticket ask for exactly that test. And decision 24 says the API can do whatever the screen can do: with the conversion on the radio, `GET /api/memory.csv` and `POST /api/memory/import` load and save a bandplan from `curl` with no browser at all.

The browser still holds the editor. It sends the file and shows what came back.

Import offers both modes, and they refuse differently:

| Mode | Behaviour |
|---|---|
| Merge | Fills empty slots only, leaves existing channels alone. A line that cannot be read is counted and skipped, and the rest of the file still lands, because a merge can only add |
| Replace | Wipes the list and loads the file. Every line is read before anything is written, so one line that cannot be read means nothing is changed and the reply says which line. Wiping a list and then stopping part way through a bad file would lose channels with no way back |

A name longer than the sixteen characters a slot holds is cut short and counted rather than refused. The name is what a person reads and nothing tunes by it, so losing a channel over a long name would cost more than it saves. Counted, because a name that came back shorter than it went in is the kind of quiet difference this firmware is careful about.

A name may hold any printable ASCII character, including the ones a spreadsheet reads as a formula. This radio has one user, who types the name and opens the exported file, so there is nobody for a formula to be aimed at, and refusing `-Fm` as a station name would cost more than it saves. Printable ASCII is the limit because a control character would split one exported line into two and the file would read back as a different list. That also means a name cannot be written in Telugu or Hindi today, which is a real limit and follows the panel font rather than this rule.

Why it earns its place: entering a frequency and a name with a rotary encoder is
the most tedious thing on the radio. A keyboard turns ten minutes into one. CSV
means a bandplan for an area can be built in a spreadsheet and loaded in one go.

It also makes the memories survivable. In the PE5PVB firmware they live in
EEPROM bytes 0 to 2078 with no way to get them off the device.

### 14. Band scan with a spectrum view

The sweep already exists. `doAutoMemory()` and the DX scanner both step across
the band, but all the user sees is a progress bar and the signal reading at each
step is thrown away.

Keep the readings and draw them: signal strength against frequency, the whole
band in one view. A plot on the radio screen, and an interactive chart in the
browser where hovering a peak and tuning to it is one click.

**The last few scans are stored with a timestamp**, so this evening can be
compared with last week. Band openings on FM are exactly what a DX listener
wants to catch, and they do not wait for someone to be watching.

Cost is small. At 100 kHz steps the whole FM band is 205 points.

Worth noting what this would have answered already: in September 2026 the signal
on 104.0 read between 10 and 38 dBuV and swung about. A sweep would have shown
straight away whether that is the station, the antenna, or a neighbour's
interference.

### 15. Sleep timer and alarm

The RTC and NTP are already on the board and the radio can power itself down.

**Sleep timer.** 15, 30, 60 or 90 minutes. The volume fades down over the last
30 seconds, then the radio powers off. The fade is the part that matters.
Cutting audio dead is what makes most sleep timers unpleasant, and fine grained
dB volume control already exists from the AGC work.

**Alarm.** Wake at a set time on chosen days, to a chosen memory channel, at a
chosen volume, for a set duration.

**The battery warning is part of the feature, not an extra.** An alarm only
fires if the radio still has power. When setting one, the estimated runtime is
shown and a warning appears if the battery will not last until the alarm time.

Why this is worth more than it sounds: it makes the radio useful when nobody is
operating it. A shortwave listener with a schedule can have it wake for a
broadcast.

### 16. Boot diagnostics and crash visibility

**Startup self test.** Probe each I2C device, check the tuner device word,
verify the patch loaded, check the filesystem mounts, confirm settings passed
their version check. Anything that fails is named on screen in plain words.

What this replaces: when the tuner does not answer, the PE5PVB firmware prints
"Tuner not detected" and runs `for(;;);`. The radio hangs with no explanation
and nothing is recorded.

**Diagnostics page in the browser.** The same results plus free heap, uptime,
Wi-Fi RSSI, flash usage, which OTA slot is running and what version sits in the
other one.

**Crash visibility.** The ESP32 records a reset reason and can write a core
dump. Capture it, show the last reset reason with a timestamp, and keep the last
few. A radio that reboots itself once a day is invisible today.

This exists because of two other decisions. With a public repo, an issue saying
"it reboots sometimes" is answerable by asking for the diagnostics page instead
of guessing. With OTA, rollback catches an image that will not boot, but not one
that boots and then misbehaves.

### 17. Power and battery

The two task split is itself a power feature. A cooperative loop spins and never
idles. Tasks that block on a queue let the idle task enter automatic light
sleep, which lowers the floor rather than shaving a peak.

Largest consumers first: backlight, Wi-Fi, CPU at 240 MHz, tuner, audio amp.
Anything not aimed at the first three is noise.

**Always on, no setting needed:**

| | |
|---|---|
| CPU down to 80 MHz when idle | Roughly halves CPU current. Only LVGL redraws need 240 MHz |
| `btStop()` at boot | Bluetooth is never used and costs RAM and power |
| Wi-Fi modem sleep when connected | Keeps the connection, cuts much of the radio cost |
| Tuner polled at 10 Hz | The PE5PVB firmware calls `getStatus()` every loop pass on a strong signal. RDS keeps its own 43 ms cadence |

**Low power mode**, a setting that trades features for hours: Wi-Fi off, CPU
pinned at 80 MHz, backlight capped with a shorter timeout, status polling at
4 Hz, telemetry off, spectrum disabled.

**Battery management beyond what exists today:**

- A real Li-ion discharge curve rather than a linear voltage map, which is why
  the percentage jumps around on the current firmware.
- Estimated runtime in hours, not only a percentage. That is what makes the
  alarm battery warning meaningful.
- Charge detection and charge state.
- A clean shutdown at low battery: warn, dim, save settings, power down. A
  brownout during an NVS write can corrupt settings and nothing prevents that
  today.

**The savings are measured, not claimed.** No mA figure goes in the README until
it has been measured. Telemetry already logs battery voltage with a timestamp,
so the method is: run the radio in each mode, capture the discharge curve,
compare. Guessing a threshold is what made the volume AGC do nothing for a week
in September 2026.

### 18. Settings backup and restore over the web

Download the whole configuration as a JSON file, upload it back to restore.

The file carries the schema version, the firmware version and the board name.
Restore refuses a file from a different board and migrates an older schema
rather than rejecting it.

**The Wi-Fi password is masked in the backup.** The file is not a credential
store. On restore the radio keeps its current Wi-Fi password, or asks for one if
it has none.

Nearly free once settings are a struct: the serialiser already exists for the
settings page, so this is two HTTP endpoints and a file picker.

Why it earns its place on this radio: reflashing needs the board opened and the
boot button held. A settings wipe would mean re-entering Wi-Fi credentials, band
edges, memory channels and touch calibration on a 320x240 screen with a rotary
encoder.

### 19. One recovery screen, not seven boot combos

Hold the rotary button at boot to get a plain list, navigated with the encoder:

```
RECOVERY
  Rotary direction          normal
  Rotary type               standard
  Flip screen               off
  Invert colours            off
  Calibrate touch
  Calibrate S-meter
  Show diagnostics
  Roll back firmware
  Factory reset
  Exit and start radio
```

**Recovery mode ignores the stored display settings.** Known rotation, no
inversion, full brightness, large font, encoder only with no touch dependency.
So it stays readable and usable when the settings that caused the trouble are
the broken ones.

Why these belong at boot at all: every one is a recovery action, needed when the
normal UI cannot be used. Touch calibration cannot be fixed through a touch
menu. A flipped or inverted screen cannot be read to reach the menu that fixes
it.

Why not the PE5PVB approach of seven separate combos: nobody remembers that BW
plus Mode is touch calibration, and the moment it is needed is the moment the
README cannot be consulted.

Two entries come from other decisions. **Roll back firmware** returns to the
previous OTA slot when an update boots but misbehaves. **Show diagnostics** puts
the self test results on the device when the web UI is unreachable.

Two things stay outside it:

1. Everything here is also in normal settings, discoverable without knowing any
   trick.
2. **Factory reset keeps a blind combo.** Hold BW and the rotary button for five
   seconds, with a beep and the standby LED confirming, so it works with a dead
   display. It is the only action that must work when nothing can be seen.

The combo list goes on the About screen, so it is findable on the radio rather
than only in a README.

### 20. Optional features are build flags, not deleted code

Every optional subsystem sits behind a flag in the board header.

```c
#define FEATURE_WEB_UI      1
#define FEATURE_TELEMETRY   1
#define FEATURE_SPECTRUM    1
#define FEATURE_ALARM       1
#define FEATURE_AIR_BAND    0
```

A flag at 0 means that code is not compiled and costs nothing. The CI size
report shows what each feature costs in flash and RAM, so "stripped down"
becomes a measured number rather than a feeling.

Deleting code instead would not be reversible and would not be per board. A
future board might have the air band converter, or want something this one does
not.

This is also the correct fix for the `HAS_AIR_BAND` bug in the PE5PVB firmware.
That option is broken precisely because it is a define in one file rather than a
build flag every file sees.

### 21. Boot screen

The splash carries information, not decoration. With OTA in use the most
valuable thing it can say is which firmware is actually running.

```
    /(( TEF668X ))\
    \(( V1.0.0 . ATS125 ))/

    TEF6686 LITHIO . PATCH V102

    Tuner          ok
    Keypad         ok
    Touch          ok
    Clock          ok
    Filesystem     ok
    Settings       ok
```

**The logo is drawn, not stored.** A rounded rectangle encloses the whole
lockup. Its two end caps are the outer arc of each side, joined across the top
and bottom, so the outer ring is one closed shape. Inside it sit two smaller
arcs and a dot on each side, spreading outwards, with the name in type between
them. All of it is LVGL arc and rectangle primitives, so it costs no flash,
scales to any size and stays sharp. Name, version and tuner line are upper case
and centred.

**Each self test line ends in a tick, not the word ok.** The tick is a two
segment path, drawn the same way. A failed line turns the tick into a cross in
the warn colour.

**The animation is the progress indicator.** The inner arcs light one at a time
as the self test passes each stage, then the outer ring, and a bar under the
test list fills with them. On a failure everything stops and the failing line
turns to the warn colour. Nothing on this screen exists only to look busy.

**No author name or callsign on it**, at least for now.

**Not skippable.** It shows for exactly as long as booting takes.

**Target: under two seconds from power to audio**, and measured rather than
assumed. The PE5PVB firmware has a bare `delay(1500)` in `setup()`, which is
dead time on every power on.

### 22. Themes are data, and ten of them ship

Colours are never written into screen code. Every screen, panel and menu draws
from ten named roles, and a theme is one row of ten values.

| Role | Used for |
|---|---|
| `bg` | Screen background |
| `bar` | Status bar and bottom strip background |
| `primary` | Frequency, panel values, the tuned thing |
| `secondary` | Station name, radio text, anything the broadcast sends |
| `muted` | Labels, units, scale numbers |
| `text` | Neutral text such as the clock |
| `ok` | Good state: stereo lock, battery healthy, self test passed, low end of a meter |
| `warn` | Bad state: overload, low battery, a failed self test, high end of a meter |
| `rule` | Dividers and panel edges |
| `track` | The unfilled part of a meter |

The ten that ship: Nightwatch (default), Grayscale, Nightvision, Daylight,
Paper, Blueprint, Vintage, Ice, Terminal and High contrast.

**Every theme has to carry rank.** A theme where the frequency, the station name
and the labels sit at the same brightness reads flat, so a single hue at one
weight is not shipped. Nightvision is the one exception and it earns it. Dim
green with no white and no blue is what keeps night adaptation when the radio is
used outdoors at night, and it still separates its four levels by brightness.
Grayscale carries rank by brightness alone, so it still reads on a panel with a
bad colour cast, and it is what the recovery screen uses.

**The theme is one byte in the settings struct.** Changing it repaints the
screen and touches nothing else. Adding an eleventh theme is adding a row to
the table, never editing a screen.

**A custom theme comes from the web interface**, as the same ten values stored
in settings instead of read from the table. That falls out of the theme being
data, so it costs almost nothing.

### 23. Themes and layouts are settings, not builds

Both the colour theme and the screen layout are picked by the user at run time.
Neither needs a reflash and neither is chosen at compile time.

| | Where it lives | Cost |
|---|---|---|
| Theme | `settings.theme`, one byte | Ten colours per theme, so twelve themes is about 240 bytes of flash |
| Layout | `settings.layout`, one byte | A layout is a table of panel placements, not code. Six of them is about 3KB |

Layouts are already data. The decision on panels says describe what goes where,
do not hard code the picture, so a second layout is a second table and not a
second screen function. LVGL builds only the active layout, so RAM holds one at
a time whatever the setting says.

**Build flags only where a layout needs a subsystem that can be off.** The band
spectrum layout needs `FEATURE_SPECTRUM`. With that flag at 0 the layout is not
compiled and does not appear in the menu. No new flag is added for it, it
follows the flag that already exists.

**The real cost is testing, not flash.** Six layouts across two band groups,
with touch off and again with touch on, is twenty four manual checks per release
instead of four. That is the number to weigh when a seventh layout is proposed.
Ship the six, and add another only when someone asks for it.

Both settings are in the struct, so both come to the web interface for free, and
both go into the settings backup.

### 24. The radio is driven by an HTTP API, and the screen is one of its callers

Every command the radio can carry out is reachable over HTTP. The menu on the
screen does not talk to the radio task directly. It builds the same command any
other caller would and puts it on the same queue.

**This is a testing decision before it is a feature.** CI cannot turn an
encoder, and it cannot hear audio. Without an API, checking that a band edge
wraps correctly on real hardware needs a person standing at the radio. With one,
it is a script. What is left for a person is only what a person has to see or
hear: the display, the audio and the touch panel. Everything else moves off the
manual checklist.

| | |
|---|---|
| `GET /api/state` | The whole state snapshot as JSON. The same snapshot the screen reads |
| `POST /api/tune` | Frequency, or a step up and down |
| `POST /api/band` | Band, and bandwidth |
| `POST /api/volume` | Volume, mute, AGC target and boost |
| `GET, POST /api/memory` | The channel list, and one channel |
| `GET, POST /api/settings` | The settings struct, straight from and to JSON. **Behind the PIN in both directions**, see below |
| `POST /api/scan` | Start a band scan, and read the result |

**One rule holds it together: if the screen can do it, the API can do it.** A
control that exists only as a knob cannot be tested from anywhere else, and that
is the exact gap this decision closes. A pull request that adds a control
without adding its endpoint has not finished.

**One schema, not two.** The API returns the same JSON the telemetry sender
publishes. A capture from `tools/telemetry.py` and a `GET /api/state` describe
the state the same way, so a test fixture recorded from one works against the
other.

**Reads are open, writes need the access PIN.** Same rule as the rest of the web
interface, for the same reason: the radio stays a glanceable page on a phone,
and nothing changes without the PIN.

**With one exception: `/api/settings` needs the PIN to read as well.** The
settings struct holds the Wi-Fi passphrase and the access PIN itself, so an open
read would hand both to anyone on the network. That would also contradict the
access PIN decision above, which lists only the dashboard, the live telemetry
and the memory channels as open to view. If a read only view of the settings is
ever wanted, it returns a redacted copy with those two fields removed, and it
says in the response which fields were removed.

**Errors say what was wrong.** A refused command returns a status code and a
plain reason. Never a bare 500, and never a 200 with the command quietly
dropped, because a test that cannot tell those apart is worse than no test.

This arrives in phase 2 with the tuner. Read endpoints land as soon as there is
state to read, and each write endpoint lands with the control it drives.

### 25. The panel driver is ours, the UI toolkit is not

LVGL draws the user interface, as decision 12 says. What puts pixels on the
ILI9341 underneath it is a driver in this repository, not TFT_eSPI or
Adafruit_ILI9341.

The reason is how little is actually needed. LVGL asks a display driver for one
thing: take a rectangle of pixels and push it to the panel. That is a chip
select, a command, and an SPI write. Everything else those libraries offer,
fonts, shapes, sprites and touch, is work LVGL is already doing, so pulling one
in means carrying a second drawing stack that nothing calls.

What it buys:

| | |
|---|---|
| Dependencies | Stays at zero. The only thing this firmware builds against is the Arduino ESP32 core, pinned exactly |
| Configuration | Pins are in the board header with every other pin. TFT_eSPI wants its own header edited or thirty build flags, and a wrong one there fails as a blank screen |
| Size | About 3KB against roughly 40KB, on top of LVGL's 150KB |

What it costs: the initialisation sequence and the SPI setup are ours to get
right, and those are the parts that fail as a screen that stays dark.

The text drawing that phase 2 uses to prove the panel works is temporary. It
goes when LVGL arrives, and the flush function is the only part that stays.

**Pin 19 is the standby LED, not SPI MISO.** HARDWARE.md recorded it as both and
flagged it as needing a check. Nothing ever reads from this panel, so MISO is
never wired and the conflict does not exist. A driver that reads the panel back
would have to settle it properly first.

### 26. One live copy of the settings, and one call that keeps it

The radio's state lives in one place, the `RadioSettings` the radio task owns. Nothing keeps a second copy of it. The stored `Settings` struct in NVS is not a second copy either: it is where that state goes to survive a power cycle, and it is written from the live one.

So there are three kinds of setting and three ways to reach them:

| Kind | Changed with | When it takes effect |
|---|---|---|
| What the radio is set to now | `/api/fm`, `/api/squelch`, `/api/bandwidth`, the knob, the keypad | At once |
| The same, kept | `POST /api/save` | At the next start |
| What can only be read at start up | `POST /api/settings` | At the next start, and the reply says so |

The last row is the band plan, the medium wave spacing and which encoder is fitted. The band plan decides which frequencies exist, and changing it under a radio that is tuned to one of them is a change with no right answer, so it is not attempted.

The reason for `POST /api/save` rather than a stored value beside every live one: two copies of the same thing drift, and then a person has to know which of the two a page is showing. It also gives "come up on the station I was listening to" for nothing, because that is the same call.

That call now also happens on its own, ten seconds after the radio stops changing, which is what makes "come up where I left it" true for somebody who never presses anything. It writes the same candidate the manual save writes, built by the one function both call, so the two cannot keep different subsets. The manual save stays, because it is still the way to say "this one, now" and it is how a test pins a known state. The delay is measured rather than chosen, and the reasoning is in decision 29.

`radioPlanFromSettings`, `radioFromSettings` and `radioToSettings` in `core/radio.c` are the only three places the two forms are converted. A caller that built its own band plan from the defaults would be a second answer to "what is the band plan", and there was already one of those.

The volume belongs to the knob, and a stored volume arguing with the knob at every start up is exactly what this refuses. There is one exception, and it is the reason the exception is safe: in manual squelch the knob is the squelch control and never touches the volume, so in that one mode nothing on the radio says how loud to be. `startVolumeDb` is stored for that case and read in no other. Without it a radio left in manual squelch comes up at whatever the threshold maps to, which is full volume at one end of the travel and silence at the other.

### 27. The start up chime is on by default, the other beeps are not

Every other beep ships off: key presses, long presses and the band edge. The chime at start up ships on.

That is inconsistent on purpose, and the reason is what each one answers.

The other beeps confirm something a person just did. They already know they pressed the key, so the beep adds confidence rather than information, and whether it is worth hearing every time is a matter of taste. Off is the safe default for anything in that class, because a radio that makes a noise nobody asked for is worse than one that is quiet.

The chime answers something else: whether the radio came on at all. That question is asked by somebody who has pressed the power button and is waiting, and it is the one moment when silence and a fault look the same. It also arrives before the station does, so it is the only sound that says the tuner was patched and made active successfully.

The residual risk, written down because RULES.md asks for it: a radio updated from version 5 starts chiming without anybody having asked. It is one tone of four tenths of a second, it is switchable on the Radio page under Sounds and fades, and `POST /api/settings -d 'bps=0'` turns it off.

Checklist row 283 says that with every one of these switched off the radio behaves as it did before the polish was added. That still holds, but it now means switching four things off rather than three.

### 28. The fade at boot is on by default, the dim is not

The same split as decision 27, applied to the panel instead of to the sound, and for the same reason.

The panel fades up at start up on every radio. The drop to a dim level after the radio is left alone ships off, and a person has to set a number of seconds to switch it on.

A fade up is only noticed when it is missing. Nobody looks at a radio coming on and thinks the light took four tenths of a second, and nobody can misread it, because it ends at the brightness they expect. It is also the one moment when the panel has something to show, the boot banner, and a fade reveals it rather than the panel arriving first and the text filling in after.

A dim is different. It happens when nobody is looking, it looks exactly like a panel that has failed, and the person it happens to did not ask for it. On a portable that is worse, not better: a radio in a bag with a dark screen reads as a flat battery. So it is a thing somebody switches on for themselves, with a delay they choose.

The wake is instant and never a fade, which is the other half of the same decision. Any input brings full brightness back with no ramp: the knob, any of the four buttons, any keypad key, and a real turn of the volume pot. A person who has just pressed a key is already looking at the panel, so a fade there is only a delay between the press and being able to read the answer. Turning the dim off from the web page also brings a panel that had already dropped straight back, because nothing else would: a dimmed panel is only lifted by an input.

Both fades are one second, up and down, and both are even in perceived brightness rather than in duty. Neither number is a taste. The fade up was four tenths of a second first, matching the chime, and on the radio it could not be told from the light switching on in a blind comparison. A feature nobody can see is the thing this ticket's notes warn about, so the length was measured instead: 400 ms reads as a snap, 1000 ms reads as a fade, 2500 ms is obviously slow. The backlight section of HARDWARE.md has the working.

The brightness settings act the moment they are written rather than at the next start. A brightness can only be chosen by looking at the panel set to it, and a setting that could not be seen until the radio had been left alone for a minute could not be chosen at all.

The residual risk, written down because RULES.md asks for it: `backlightPercent` has a floor of 5 per cent and no way to reach 0. A panel driven to nothing while the radio is in use is a panel that looks broken, and the only control for it is the web page that has just gone dark. The dim level has no floor, because that one is left on purpose and any input brings it back.

Checklist row 283 says that with every one of these switched off the radio behaves as it did before the polish was added. That still holds. The panel light adds one more thing to switch off, the fade, and the dim is already off on a radio nobody has told otherwise.

### 29. The radio writes its own settings down, and what belongs to a band stays with it

Two halves of one idea, and the second is what makes the first worth having.

**What belongs to a band stays with that band.** The frequency already did. The filter width, the step size and the tuning mode now do too. A width chosen on medium wave is not a width shortwave wants, 1 kHz steps chosen to pick between two crowded medium wave stations are not what FM wants, and meter band stepping only exists on shortwave at all. Each is stored per band and each is checked against the band before it is restored, because the band plan can move under a stored value: a step that was legal before a medium wave spacing change need not be one the band offers after it.

A zero entry means that band has never been set and takes its own default. Zero is not a real step or tuning mode, and a zero width is the FM automatic setting, which is what an FM band defaults to anyway, so zero means the same thing in all four.

This replaced a defect rather than adding a feature. `settleAfterBandChange` reset the width to a fixed default on every band change and never read the stored one, which was only applied at boot. Confirmed on the radio on 13 September 2026: 6 kHz set and saved on 738 kHz, then a trip to FM and back, and the radio was running 4 while `GET /api/settings` still reported 6. A setting that is accepted, reported back and quietly ignored is the failure this project keeps finding.

**The radio writes itself down ten seconds after it stops changing.** Nothing was kept before unless somebody asked, so a person who tuned a station and switched the radio off lost it, which is not what a radio does.

Ten seconds is measured, not chosen. Over 154 seconds of ordinary use the gaps between one input and the next had a median of 0.43 s and a 99th percentile of 6.1 s, so ten is about 1.6 times the longest gap inside a burst of tuning. A burst of hunting across a band comes out as one write rather than forty, which was confirmed on the radio: twenty retunes in ten seconds cost exactly one. Fifteen seconds would have saved one further write per session and left a longer window in which switching off loses the station just found, and closing that window is the point. The working is in the usage section of HARDWARE.md.

Three things keep the flash safe, and all three are checked rather than assumed. It only writes when what the radio is set to actually differs from what is stored, so the IDF's own skip-identical behaviour never has to be relied on. It never writes while a seek is running, because the dial moves every fifty milliseconds during one and none of those channels is a station anybody chose. And a write, from here or from the manual save, starts the wait again, so the two cannot take turns writing.

The residual risk, written down because RULES.md asks for it: a station tuned and then switched off within about ten seconds is still lost. That is the deliberate trade against flash wear rather than an oversight, and checklist row 330 exists so it is a known limit rather than a surprise.

### 30. The channel list is its own key in NVS, and the knob walks it

**Not part of the settings struct.** Ninety nine channels are 2376 bytes against 340 for the settings, and the settings are written every time a station is left alone for ten seconds, which is decision 29. So the list has its own NVS key, written only when a channel changes and only once a run of edits has gone quiet.

A stored blob of any other length is from a firmware whose channel struct was a different shape, and is ignored rather than read with the fields in the wrong places.

**A channel is band, frequency, filter width and name.** Sixteen characters of name. A width of 0 means the channel has no opinion and the band keeps whatever it is set to, which is what a list written by hand in a spreadsheet will carry.

**Stored and tunable are different questions.** A channel is checked for structure when it is stored, and against the band plan only when it is used, because changing the FM region moves the band edges under a channel that was correct when it was written. Memory mode steps over a channel it cannot reach rather than stopping on it and refusing to move.

**In memory mode the knob walks the list, not the dial.** It skips empty slots, wraps at both ends, and carries the band and the width with it. The step becomes a tune inside the radio task, because only the radio knows which slot it is on. Recalling a slot over HTTP goes through the same code, so the knob and the API cannot come to mean different things. The tuning mode is kept across the tune: a list runs across the bands, and decision 29's per band mode would otherwise drop the radio out of memory mode on the first channel that is not on this band.

**The slot the radio reports is worked out from the band and the frequency**, so tuning to a stored station with the keypad shows its slot. A recall reports the slot that was asked for, because two slots may hold the same station.

**Nothing writes to flash from the radio task**, the same split decision 29 uses. A write that fails is retried and `chn.bad` in `GET /api/state` says so.

The residual risk: the list sits in RAM for the life of the radio, 2376 bytes whether or not anything is stored. That is the price of the knob stepping to the next channel without a flash read.

### 31. The RDS decoder says nothing it has not heard twice

**A field is published only once it has been received twice the same way, and a field that has not is missing rather than empty.** The station name, the radio text, the identifier and the programme type each have a flag saying whether the radio can answer at all, and `GET /api/state` leaves the field out when it cannot. An empty station name and a station name that has not arrived are different things, and sending `"ps":""` for the second makes them look the same.

**The name is confirmed as a whole pass, not position by position.** Eight characters arrive two at a time over four groups, and a name is published only when two complete passes say the same thing.

This is measured and not a preference. The first version confirmed each of the eight positions on its own. Replayed against the capture from Mirchi 95, it published `5   HI 9`. That station rotates two names, `MIRCHI 9` and `5       `, three passes each, scrolling a longer name through the field, and different positions settled in different passes. Radio City 91.1 rotates five names the same way. Whole passes cannot mix, so a station that rotates names shows each in turn, which is what it is sending. The captures and the working are in `test/fixtures/rds/README.md`.

**Only a block the tuner reports as clean is decoded.** A block it says it corrected is thrown away with the ones it could not.

That is measured and it is stricter than the reference firmware, which accepts a corrected block. Two captures taken with the aerial collapsed, at about 10 dBuV instead of the usual 45, published five corrupt radio texts under the looser rule, including `MAwQCFM VINTOO...` and one whose terminator had been corrupted so the text ran on into padding. Both came through blocks the chip reported as corrected with the smallest error it reports. Clean blocks only publishes exactly the three texts those two stations actually sent. It costs about a third of the name passes on a weak signal and nothing at all on a signal worth listening to.

**An identifier of zero is no identifier.** The standard keeps `0000` for a station that has not been given one, and two of the six stations that carry RDS here send it in every group. Publishing it would state `0000` as a fact, and anything comparing identifiers would decide those two stations are the same one.

**A radio text with no terminator ends at the highest segment the station sends.** A short text is supposed to end with character `0D`. Fever FM 94.3 sends four segments round and round and never one, so a decoder that waits for it shows that station no text at all, for ever, with no way to tell that from a fault. So the segment address going backwards is taken as the pass having finished, and the length is then the highest segment that pass reached.

The length must come from the segment number and not from how many characters actually arrived. Taking it from the run of characters received, and believing it once two passes agreed, was tried and it failed on the radio: on 93.5 at 20 dBuV, with block D of segment 0 failing while block C survived, the run stalled at two characters on every pass, the two passes agreed, and the radio published `GA` as a song title. On a steady bad signal the same blocks fail every pass, so two passes agreeing proves nothing. A segment number only ever grows within one text, so damage can delay a text but cannot shorten it.

**The raw groups stay in the shipping firmware.** `GET /api/rds/raw` serves the last 128 groups exactly as the tuner handed them over, and `tools/rdscap.py` polls it into `test/fixtures/rds/`. That is the only way to get real groups off this radio, because there is no serial cable on it, and a decoder tested only against groups somebody wrote out is tested against a broadcast nobody transmits. It costs 2560 bytes of RAM, because the web handler keeps its own copy of the ring so that the lock is released before anything goes out on the network, and about 40 lines. The ring is emptied on every retune so it never holds two stations, and until that has happened it serves nothing rather than what it still holds.

**The clock is published in the station's own local time.** Group 4A carries UTC and the offset from it as two separate things. The offset is applied in `core/`, date included, because it carries the time over midnight and every caller doing that arithmetic again is every caller getting a chance to get it wrong. The offset is still published beside the time, to be shown rather than added again.

**The decoder can be switched off, and that is the only RDS setting that ships.** `rds` in the settings, on by default. It earns its place on a measurement rather than on taste: the radio task wakes for the 43 ms RDS deadline as well as for its own 100 ms tuner poll, so on FM it comes round every 31 ms with RDS on and every 99 ms with it off, measured on 13 September 2026. That is two thirds of the radio task's wakeups, and it is what the power management in phase 6 will want to be able to give back.

Switched off, the state document says `"rds":{"off":true}` and the raw endpoint says so in words. Neither shows an empty block, because an empty block is what a station carrying no RDS looks like and somebody who switched it off and forgot would have no way to tell the two apart.

The block error tolerance the reference firmware offers as "Show RDS errors" is deliberately **not** a setting. Every value looser than clean-only was measured to publish corrupt text, so it would be a switch whose only effect is to put wrong words on the screen, and anyone who wants the raw error bits already has `GET /api/rds/raw`.

**Only the European programme type names ship.** North America uses different names for the same 32 numbers. That table earns its place when a North American band plan does, because a radio on the European FM plan cannot be receiving an RBDS station.

The residual risks, written down because RULES.md asks for it. A station that rotates its name has each name published in turn and nothing says it is a rotation, so once the RDS block reaches the screen in phase 4 it will read as a radio that cannot make up its mind; stitching a long name together out of the passes is a phase 4 question and is ticketed. And no station reachable from here sends an alternative frequency list, sends the time, or changes its identifier under the dial, so those three paths are proved against groups written by hand rather than against a broadcast.

### Licence

GPLv3, inherited from PE5PVB. Keep the original copyright and state what
changed.

## Open

Nothing. Every question raised in the design discussion is settled above.
