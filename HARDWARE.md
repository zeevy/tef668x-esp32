# ATS-125 hardware notes

Everything known about the target board. Facts in here come from one of three
places, and each section says which: read off the chip with a tool, read off the
PE5PVB firmware source that runs on this radio, or taken from the seller's
product listing. Nothing here is guessed. Anything unconfirmed is marked as
open.

The listing is the weakest of the three, because it is a manufacturer claim
rather than a measurement, and it already disagrees with this unit in one place.
It is kept in its own section at the bottom for that reason.

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
| 15 | Tuner crystal sense, ADC. Read on 2026-09-12 |

Pins 34, 35, 36 and 39 are input only on the ESP32. Pin 35 is on ADC1, which is
the ADC that still works while Wi-Fi is running.

### The tuner has its own crystal, and which one is fitted is read

The 40 MHz crystal in the table above is the ESP32's. The tuner has a separate
one, and the same firmware runs on boards with different ones, so it is read
rather than assumed. A voltage on pin 15 says which:

| Reading on pin 15 | Crystal |
|---|---|
| Near 0 | 9.216 MHz |
| About 1050 | 12 MHz |
| About 2250 | 55 MHz |
| Anything else | 4 MHz |

A reading may sit up to 300 counts away from those. **This unit reads 0, so it
has a 9.216 MHz crystal**, confirmed over HTTP on 12 September 2026.

Getting this wrong does not fail loudly. An earlier version of this table had
the windows shifted by one, so the radio told the chip it had a 4 MHz crystal.
Everything still started, the chip still answered, and the signal level sat at
about -3 dBuV on every frequency, which reads as a radio with no aerial. Fixing
the mapping alone took 104.0 MHz from -3 dBuV to 25 dBuV.

Pin 15 is on ADC2, which stops working once Wi-Fi is running. The tuner is
therefore brought up before Wi-Fi, and it has to stay that way.

### Bringing the tuner up, in order

Learned by getting it wrong on 12 September 2026. Each of these was found only
by running it on the radio, and each failure looks like a working I2C bus.

1. **Wait 2 ms after every write.** That wait is also what sits between a
   query's write and its read. Without it every read returns zeros, and the
   transfer reports success.
2. **Patch, then set the clock, then power on, then write the register
   defaults.** The chip cannot answer the identification until the first three
   are done. A patched chip with no clock acknowledges every write and returns
   zeros to every read.
3. **Operation mode 0 is working, 1 is standby.** It reads backwards. A chip
   left in standby answers everything and returns a quality status of 0xFFFA
   with nonsense in every field, which looks exactly like a broken decoder.
4. **Bandwidth mode 0 pins it, mode 1 lets the chip adapt.** That reads
   backwards too. Mode 0 with a bandwidth of zero pins the bandwidth at zero:
   the chip reports its narrowest setting, never finds a stereo pilot, and
   looks like a radio with no aerial again.
5. **Stereo is not in the quality word.** It is bit 15 of the signal status,
   command 133 on the FM module. Reading bit 15 of the quality word gives an
   answer that looks plausible and is always wrong.
6. **The reset command is five bytes**, `1E 5A 01 5A 5A`. A three byte version
   is acknowledged just the same and resets nothing, so the patch then lands
   on top of whatever state the chip was already in.
7. **Coming back from AM to FM needs more than tuning the FM module.** Setting
   operation mode 0 is only half of it: the chip also wants an FM preset tune,
   which the working firmware does as a fixed jump to 100.00 MHz. Without it
   one AM tune leaves the chip on its AM front end for good, and every FM
   station afterwards reads the same fixed nonsense, 832 dBuV and a 4 kHz
   bandwidth. It reads as a broken decoder rather than a chip on the wrong
   side.
8. **Do not trust the chip's "already patched" answer after an update over the
   air.** The ESP32 reboots and the tuner does not, so it comes up still
   holding the previous firmware's clock and register settings. Patch it every
   time, or a firmware update cannot change anything about the tuner until
   someone power cycles the radio.

Once all of that is right the readings are sane: level in tenths of a dBuV,
bandwidth in tenths of a kilohertz, modulation in tenths of a percent.

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

MOSI and SCK are the ESP32 VSPI defaults, 23 and 18. MISO would be 19, which is
also listed as the standby LED. **That conflict does not have to be settled.**
Nothing reads back from the panel, so this firmware never wires MISO. See
decision 26.

### What the panel actually needed, found by looking at it

Driven for the first time on 12 September 2026. Four things could not be worked
out from the datasheet or from the pin map, and each was settled by putting
something on the glass and looking.

| | |
|---|---|
| Orientation | `MADCTL` = `MV` and `BGR`. Landscape is `MV` alone or `MV` with **both** `MX` and `MY`. Setting only one of those two is a mirror, not a rotation, and reads backwards |
| Size | 320 across by 240 down in this orientation. Having width and height the other way round drew everything into a 240 wide box against the left hand edge |
| Inversion | `INVON`, command 0x21, is needed. Without it the amber came out blue and the near black background came out white |
| Gamma | The datasheet's recommended power and gamma registers have to be sent. Without them the panel still shows pure red, green, blue, black and white correctly, so it looks fine, but the bottom of the scale is lifted hard |

**The design's background colour cannot be shown on this panel.** `#0B0F13` is
red 1, green 3 and blue 2 once it is in the panel's 5, 6 and 5 bits, and it
renders as a solid medium blue rather than as a near black. Pure black renders
correctly. Measured with a bar of each side by side, so the screen uses black.

The colour order line above says RGB, which is the name of that setting in
TFT_eSPI. In the controller's own terms it is the `BGR` bit being set.

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

## Vendor specification

From the seller's product listing, supplied 12 September 2026. **These are
manufacturer claims, not measurements, and not read off this board.** They are
kept separate from everything above for that reason. Where a claim disagrees
with what was read off this unit, the reading wins and the disagreement is
noted.

| | Claimed |
|---|---|
| Main chip | ESP32-WROOM-32U |
| Bluetooth | A separate JIELI transceiver chip, not the ESP32 radio |
| Screen | 2.4 inch IPS LCD, 320x240, resistive touch |
| Audio | TI TPA6211A1 amplifier, 40mm full range speaker |
| Battery | 3.7V, 2500mAh lithium polymer |
| Antenna | 820mm telescopic, plus a 3.5mm external antenna port |
| USB | Type-C, with a CH340 USB to serial chip |
| Size, weight | 120 x 63 x 25 mm excluding the antenna, about 350g |
| Case | Aluminium alloy |

### Two claims that matter and disagree with this unit

**The USB serial chip.** The listing says CH340. This unit has an FT232R: the
port is `/dev/cu.usbserial-A5069RR4`, and that name shape is FTDI. A CH340
appears as `/dev/cu.wchusbserial...`. The listing also says the software version
varies between batches, so the serial chip plainly varies too.

This matters because the CH340 is commonly wired for auto reset and the FT232R
on this unit is not. A CH340 batch may not need the BOOT and RESET dance at all.
Nothing in the firmware should assume either way, and `docs/test-checklist.md`
should say the button step is for this batch.

**Bluetooth is a separate chip.** The JIELI part does both directions: the radio
as a Bluetooth speaker, and radio audio out to Bluetooth headphones. That
explains the second u.FL connector marked `BT ANT1` next to the speaker.

The consequence for power management is that `btStop()` turns off the ESP32's
own radio and does nothing at all to the JIELI. Any battery saving claimed for
Bluetooth has to be measured with the JIELI in the picture.

**Open: how the JIELI chip is controlled.** Probably a UART or a few GPIOs, but
nothing in the PE5PVB source refers to it, so it may be wired straight to the
buttons and never touched by the ESP32.

### Band ranges and default steps

From the same listing. These line up with what the PE5PVB firmware does, so they
are a reasonable starting point for the band plan, but each edge still gets a
test and each one is confirmed against the tuner before it is trusted.

| Band | Range | Step |
|---|---|---|
| FM | 65 to 108 MHz | 50, 100 or 200 kHz, default 100 kHz |
| FM, OIRT | 65 to 74 MHz | 30 kHz |
| SW | 1700 to 27000 kHz | 5 kHz |
| MW, 9 kHz regions | 522 to 1791 kHz | 9 kHz |
| MW, 10 kHz regions | 520 to 1720 kHz | 10 kHz |
| LW | 144 to 513 kHz | 9 kHz |

The listing also gives four FM sub ranges that the old firmware offers as
choices: 76 to 95, 76 to 108, 87 to 108 and 87.5 to 108 MHz. These are regional
band plans rather than different hardware.

### Claimed sensitivity and selectivity

Useful only as a sanity check on measured values. Nothing in the firmware sets a
threshold from this table.

| Band | Sensitivity, telescopic antenna | Selectivity |
|---|---|---|
| FM | 0.5 uV or better at S/N 30 dB | 60 dB or better at ±150 kHz |
| SW | 10 uV or better at S/N 20 dB | 60 dB or better, BW 3 kHz at ±5 kHz |
| MW | 10 uV or better at S/N 20 dB | 60 dB or better, BW 3 kHz at ±9 kHz |
| LW | 10 uV or better at S/N 20 dB | 60 dB or better, BW 3 kHz at ±9 kHz |

### The reported signal level is offset by 7 dB

The working PE5PVB firmware writes a level offset of -70, meaning -7.0 dB, to
both the FM and AM sides on every start. Its own offset setting defaults to 0
and it still writes that -70, so it is a calibration of what the chip reports
rather than a preference.

Everything that firmware measures is therefore on a scale 7 dB below the
chip's raw reading. That includes every threshold in it, and the captures in
`test/fixtures/agc/`, which were taken from this radio while it was running.

This firmware writes the same -70, so that those numbers keep their meaning.
Whether -7.0 dB is right in absolute terms is not known and cannot be settled
without a signal generator. What is known is that two scales is how a
threshold ends up 7 dB out.

Signal figures recorded in this repository before 12 September 2026 are on the
raw scale and are about 7 dB higher than the radio now reports.

## Open items

1. ~~Whether pin 19 is really both the standby LED and SPI MISO.~~ **Closed, by
   not needing the answer.** Nothing reads from the panel or the touch
   controller, so MISO is never wired and the two never contend. A driver that
   reads the panel back would have to settle it first.
2. How the JIELI Bluetooth chip is controlled, and whether the ESP32 talks to it
   at all.
3. Which USB serial chip other batches carry, and whether those are wired for
   auto reset.
4. **The FM tuning is about 50 ppm high, and the reference crystal is the
   likely cause.** Measured on 12 September 2026 across nine local stations,
   the tuner's own offset reading ranged from -0.6 to +8.8 kHz and averaged
   about +5 kHz. On the three medium wave stations it was -0.2, -0.7 and 0.0
   kHz, which is effectively zero.

   That difference is the evidence. An error in the tuning arithmetic would
   show on both bands, because both go through the same code, and a
   transmitter being off frequency would not line up across nine of them. An
   error in the reference clock scales with the frequency being received, so
   +5 kHz at 100 MHz is about 50 ppm, and the same 50 ppm at 738 kHz is 0.04
   kHz, far too small to see. 50 ppm is ordinary for a cheap crystal.

   It changes nothing for listening: FM channels are 100 kHz apart, so 5 kHz
   never reaches the next station.

   **The PE5PVB firmware has no frequency trim either.** It was searched on 12
   September 2026 and the only calibration in it is for the analogue meter and
   the touchscreen. So it runs on this same radio with this same error, and its
   RDS decodes well enough that the captures in `test/fixtures/` came from it.
   That is real evidence that 50 ppm is tolerable, and it takes RDS off the
   list of things this threatens.

   What is left is the band scan in phase 6, where a systematic offset shifts
   every peak by the same amount. That is worth correcting, and it is also the
   easiest place to measure the correction, since a scan across a band of
   known stations gives the error directly.

   Nothing is being adjusted until it has been measured that way. Dialling a
   trim until the number looks tidy is how a guessed threshold gets in.
4. Whether the air band converter board is fitted. Nothing in the photos looks
   like one, which supports the theory that it is absent.
