# ATS-125 hardware notes

Everything known about the target board. Facts in here were read off the chip or
taken from the working PE5PVB firmware source, not guessed. Anything unconfirmed
is marked as open.

## Chip

Read with `esptool flash-id` on 2026-09-12.

| | |
|---|---|
| Chip | ESP32-D0WD-V3, revision v3.1 |
| Cores | Dual core, 240 MHz, plus the LP core |
| Radio | Wi-Fi and Bluetooth |
| Crystal | 40 MHz |
| Flash | 8MB, 3.3V (manufacturer 0x85, device 0x2017) |
| eFuse | Vref calibration present, coding scheme none |
| MAC | a0:b7:65:05:35:b4 |

Command used:

```bash
~/.platformio/penv/bin/python -m esptool --port /dev/cu.usbserial-A5069RR4 \
  --baud 460800 flash-id
```

**No PSRAM.** The module shield reads `ESP-32U`, which is a WROOM-32U class
module with an external antenna connector. WROOM modules carry flash only. A
WROVER would be needed for PSRAM.

This matters for the rewrite. LVGL has to use partial draw buffers rather than a
full 150KB frame buffer, and the web server has to stream large responses rather
than build them in memory.

## Flashing

The USB adapter is an FT232R on `/dev/cu.usbserial-A5069RR4` and it is not wired
for auto reset. Every upload needs the board put into download mode by hand:
hold BOOT, tap RESET, release BOOT. Retrying the upload on its own never helps.

## Pin map

From the defines at the top of `TEF6686_ESP32.ino` in the PE5PVB firmware.

| Pin | Use |
|---|---|
| 34 | Rotary encoder A |
| 36 | Rotary encoder B |
| 39 | Rotary encoder push button |
| 35 | SQL potentiometer, ADC1 |
| 13 | Battery sense, ADC |
| 4 | BAND button |
| 25 | BW button |
| 26 | MODE button |
| 2 | Backlight PWM |
| 19 | Standby LED |
| 27 | Analog S-meter PWM |
| 33 | Touch controller interrupt |
| 14 | Keypad interrupt from the I/O expander |

Pins 34, 35, 36 and 39 are input only on the ESP32. Pin 35 is on ADC1, which is
the ADC that still works while Wi-Fi is running.

## Display

From `TFT_eSPI/User_Setup.h` in the PE5PVB fork of the library.

| | |
|---|---|
| Controller | ILI9341, 320x240 |
| Interface | SPI |
| TFT_CS | 5 |
| TFT_DC | 17 |
| TFT_RST | 16 |
| TOUCH_CS | 32 |
| SPI clock | 7.5 MHz write, 20 MHz read, 2.5 MHz touch |
| Colour order | RGB |

MOSI, MISO and SCK are the ESP32 VSPI defaults: 23, 19 and 18. Note that 19 is
also listed as the standby LED in the pin map above, which needs checking on the
real board before it is trusted.

**Touch is fitted and working.** An `XPT2046` in SOIC is on the main board,
marked `XPT2046 ABDDCB`, next to the PCA9555, and touch input works on this unit
with the PE5PVB firmware set to the touch model.

## I2C bus

`Wire.begin()` is called with no arguments, so it uses the ESP32 defaults,
SDA 21 and SCL 22.

| Address | Device | Notes |
|---|---|---|
| 0x64 | TEF6686 / TEF668x tuner | Main tuner. Needs a firmware patch blob loaded at boot |
| 0x20 | **PCA9555PW** 16 bit I/O expander | The numeric keypad, confirmed from the board photo. Register 0x06 is written 0xFFFF to set all pins as inputs, then register 0x00 is read as two bytes |
| 0x32 | RX8010 RTC | Real time clock, optional. The code probes for it |
| 0x12 | External converter | Only written when the converter setting is 200 or more. Not present on a stock unit |

## Flash layout today

The PE5PVB firmware ships this, and it only uses 4MB of the 8MB fitted.

```
nvs         data nvs      0x9000   0x4000
app         app  factory  0x10000  0x3B0000
coredump    data coredump 0x3C0000 0x10000
spiffs      data spiffs   0x3D0000 0x30000
```

There is a single application slot, so over the air update is impossible with
this table, not merely unimplemented. A rewrite should use a table with two
application slots and a much larger filesystem.

Where the current image goes: about 3.5MB total, of which roughly 2.2MB is
read only data rather than code. The large items are the 23 language tables, the
US callsign lookup in `src/usa_stations.cpp`, and the Chinese font set.

## Air band

The `HAS_AIR_BAND` build option in the PE5PVB firmware is for a separate
converter board. The TEF668x AM tuner stops around 27 MHz while the air band is
108 to 137 MHz, so `SelectBand()` parks the tuner at a fixed 10.7 MHz IF and the
extra board does the real tuning. Without that board the radio hears nothing on
that band.

The option also does not build as shipped. `#define HAS_AIR_BAND` sits 24 lines
below `#include "src/constants.h"`, so the preprocessor skips every air band
block in that header and the build fails with 47 undeclared symbol errors.
Moving the define above the include fixes the build, but it is still only
defined in the `.ino`, so `gui.cpp` compiles without it and the two halves of
the program disagree about the values in `RADIO_AM_BAND_SELECTION`. A correct
fix would pass it as a build flag so every file sees it.

**Open: does this unit have the converter board?** The stock firmware showed an
AIR option in the menu but no stations could be heard. That is consistent with
the board being absent, and also with air band traffic simply being quiet, since
it is AM and intermittent unless you are near an airport.

## Licence

The PE5PVB project is GPLv3. Any fork stays GPLv3, keeps the original copyright,
and states what was changed.

## Parts on the board

Read off photographs of the opened radio taken on 2026-09-11. The photos are not
kept in this repo, only what was read from them.

| Part | Marking | Role |
|---|---|---|
| ESP32 module | `ESP-32U`, `XX0H65`, FCC `3O869-ZY-32U` | WROOM-32U class, external antenna on u.FL. No PSRAM |
| I/O expander | `PCA9555PW UWM 2436` | Keypad on I2C 0x20 |
| Touch controller | `XPT2046 ABDDCB` | Resistive touch, own SPI chip select |
| Tuner | NXP logo, `F6602`, `33 05`, `KSD0061` | The TEF6686. See the note below |
| SQL pot | `B10K` | 10k linear, the squelch and volume knob |
| PCB | `ATS125 20250501`, `565160A P7 251024` | Board revision and date |

There is a second u.FL connector labelled `BT ANT1` next to the speaker, plus
the whip antenna feeding the module's own u.FL. USB-C for charging and serial.

### Which TEF chip is it

**TEF6686 Lithio**, confirmed from the boot splash on 2026-09-12. The tuner
reports its identity over I2C and the firmware maps the low byte of the device
word to a model. 14 means TEF6686.

The laser mark on the package reads `F6602 / 33 05 / KSD0061`, which matches no
published NXP type number, so the mark is not a reliable way to identify these
parts. Always read the device word instead.

What this variant does not have:

| Feature | Available here | Why |
|---|---|---|
| FMSI stereo improvement | No | TEF6687 and TEF6689 only |
| Full search RDS | No | TEF6687 and TEF6689 only |
| Digital radio (DR) | No | TEF6688 and TEF6689 only |

For the rewrite this means the tuner driver should still read the device word
and publish a capability set, so a board with a TEF6687 or TEF6689 gets those
features and the UI simply does not offer them on a TEF6686. That is the same
pattern as the board headers: detect what is there, do not hard code it.

## Open items

1. Whether pin 19 is really both the standby LED and SPI MISO, or whether one of
   the two sources is wrong.
4. Whether the air band converter board is fitted. Nothing in the photos looks
   like one, which supports the theory that it is absent.
