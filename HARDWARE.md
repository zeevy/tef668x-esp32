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
| 27 | Analog S-meter PWM. **No meter is fitted on this unit**, checked by looking at it on 12 September 2026. The pin is still driven, because the pin map comes from a firmware that runs on boards in this family that do have one |
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
decision 25.

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

### The backlight, and how dim it can go and still be read

Pin 2 drives the backlight through PWM. This firmware runs it at 5 kHz with 8
bits of duty, and treats the setting as a percentage, so 20 per cent is a duty
of 51 out of 255.

Measured on 13 September 2026, in daylight through a window, with the radio on
FM 106.40 and the plain text screen showing the frequency. The brightness was
stepped down from 70 per cent to 5 in eight stages over the web API, six
seconds each, and each stage was looked at.

| Brightness | What the frequency looked like |
|---|---|
| 70 to 18 per cent | Comfortable to read |
| 12 per cent | Between the two |
| 8 per cent | The number can still be identified |
| 5 per cent | Visible, with a little effort |

So the panel is legible all the way down to 5 per cent in daylight. Two numbers
come out of this. The dim level a radio ships with is 20 per cent, which sits
just above the comfortable floor, so a panel that has dimmed can still be read
at a glance rather than only made out. And the floor on the in use brightness
is 5 per cent, because that is the lowest setting still usable rather than a
round number: below it the only control for a panel nobody can read is the web
page that has just gone dark.

### How long a brightness change has to take to read as a fade

Measured on 13 September 2026, on the same panel and in the same daylight.

A fade at start up was built at four tenths of a second first, to match the
length of the start up chime. It could not be told from the light simply
switching on. That was checked properly rather than by impression: the fade
was switched on for one boot and off for another, in an order picked at random
and not looked at until afterwards, and the two boots were indistinguishable.
Repeated with the ramp reshaped to be even in perceived brightness rather than
in duty, and the answer was the same.

| Fade length | How it reads |
|---|---|
| 400 ms | Cannot be told from the light switching on |
| 1000 ms | Reads as a fade |
| 2500 ms | Obviously slow |

So the fades are one second, up and down. The two had been written as four
tenths up and one second down, on the reasoning that a fade up is waited for
and a dim is not. The measurement says four tenths is not a fade at all on
this panel, so both are a second now.

**The ramp is even in perceived brightness, not in duty.** An eye's response to
light is near enough the square root of it, so a straight line in duty spends
almost all its travel in a range that already looks bright. With the frequency
readable at 5 per cent, a straight line from 0 to 100 is clearly lit about
twenty milliseconds in. Interpolating the square root and squaring it back
puts the slow part of the travel down at the bottom, where a change can be
seen. That alone was not enough to make four tenths of a second visible, but
it is the right shape and it stays.

Taken in one light on one day, which is the limit of it. Daylight through a
window is the harder case than a dark room, so a dim level chosen here should
hold indoors at night. Direct sun has not been tried.

### How much the signal reading moves on a station that is not moving

Measured on 13 September 2026 on FM 106.40, sixty readings over twenty four
seconds, with the aerial and the radio left alone.

| | Spread | Standard deviation |
|---|---|---|
| `sig`, the reading off the chip | 3.2 dB | 0.60 |
| `sav`, the same after the one second average | 1.6 dB | 0.44 |

The average takes out about half of it. What is left is mostly the signal
really moving rather than noise, which is why a longer average helps far less
than it looks like it should: replaying the same capture through a three
second average changed the spread from 2.5 dB to 2.3 and cost up to three
seconds of lag.

**Smoothing the value is not the same as steadying the screen.** At one
decimal place the last digit still changed on forty of fifty nine consecutive
readings. Replaying the capture into a screen showing whole dB gave twelve
changes in twenty four seconds, and whole dB held until the level moves a
whole dB away gave one. That last one is what the panel does.

| What the screen does | Changes in 24 seconds |
|---|---|
| One decimal place, smoothed | about 40 |
| Whole dB, smoothed | 12 |
| Whole dB, smoothed, held until it moves 1 dB | 1 |
| Whole dB, three second average, held | 2 |

An empty FM channel here reads about -10 dBuV with the ultrasonic noise in the
hundreds, against 25 to 36 dBuV on a local station, so a level alone separates
the two easily.

### How long a person spends moving the dial before settling

Measured on 13 September 2026, 154 seconds of ordinary use recorded from
`inp.clk`, `inp.prs`, `inp.pot` and the frequency, sampled four times a
second. Tuning around, changing band, hunting for a station and settling on
one. 62 moments where something moved.

| Gap between one input and the next | |
|---|---|
| Median | 0.43 s |
| 90th percentile | 2.1 s |
| 95th percentile | 2.7 s |
| 99th percentile | 6.1 s |
| Longest in the session | 13.8 s |

Tuning is bursty. Nearly every gap inside a burst is under three seconds, and
only one gap in the whole session passed ten. That is what sets how long the
radio waits with nothing happening before it saves itself:

| Idle before saving | Saves in that session |
|---|---|
| 3 s | 5 |
| 5 s | 3 |
| 10 s | 2 |
| 15 s | 1 |

Ten seconds is used. It is about 1.6 times the 99th percentile gap, so a burst
of hunting across a band is one write rather than forty, which is the flash
wear question. Fifteen would save one further write per session and leave a
longer window in which switching the radio off loses the station just found,
and closing that window is the point of saving at all.

### How to hear the FM de-emphasis, and what hides it

Measured on 13 September 2026. This is written down because the obvious way to
test it does not work, and two blind tests concluded the feature was broken
before the method was changed.

**Listen to hiss, not to music.** De-emphasis is a treble shelf. The loudest
treble a radio makes is hiss, so an empty FM channel is the test signal and a
station playing music is not. On an empty channel with de-emphasis off the
hiss is plainly brighter and louder, on the internal speaker, with no
headphones. With music on a station the same change could not be heard at all,
twice, because this speaker rolls off the top of a song enough to hide it.

Four blind A/B pairs, off against 50 us, all four called correctly, one of them
with the order reversed so an order effect cannot explain it.

**Switch the weak signal handling off first.** The chip has its own treble roll
off, and it moves on its own. With the high cut start at 35 dBuV and a station
reading 33 to 37, the reported `cut` flickered between 0 and 6 as the signal
crossed the threshold. That is a treble change coming and going during a test
about treble, and it was running through the first de-emphasis test unnoticed.
With the start at 0 the reading holds steady at 0.

| Weak signal high cut start | Station at 33 to 37 dBuV | Reported `cut` |
|---|---|---|
| 35 dBuV | signal crosses the threshold | flickers 0 to 6 |
| 0, off | any | steady 0 |

So the standing rule for any listening test on this radio: read `cut`, `bld`
and `hbl` in `/api/state` first and make sure all three are 0.

### The shoulder of a strong station looks exactly like a station

Measured on 13 September 2026. This is why the auto squelch has a level floor
and why the floor is judged on a smoothed reading.

The channel either side of a strong station is not empty. The sidebands of the
station next door reach into it, so the tuner reports low ultrasonic noise and
low multipath there, which is everything the squelch looks at. On 102.0 MHz,
beside 101.9, the reading passed the noise and multipath test on 10 of 12
samples, and the audio sat open on a channel with nothing worth hearing.

**Level separates them and stability separates them better.** A station holds
its level. A shoulder does not.

| Channel | What it is | Raw level, dBuV | Smoothed by the radio |
|---|---|---|---|
| stations | six of them | 16.4 to 42.2 | 25.3 to 41.3 |
| 102000 | shoulder of 101.9 | 1.1 to 13.2 | 4.6 to 6.8 |
| 91000 | shoulder of 91.1 | 4.9 to 24.8 | 13.0 to 16.2 |
| 105000 | genuinely empty | -2.5 to 9.0 | 0.7 to 3.5 |

The raw level on a shoulder reaches 24.8 dBuV, above the 19.6 dBuV of the
weakest station in the seek sweep, so a floor on the reading alone cannot
separate them. Smoothed, the shoulders fall to 16.2 and below while a station
barely moves.

**The smoothing has to be the radio's own.** `signalAverage` forgets per
reading, not per millisecond, so a capture taken at three readings a second
smooths over three and a third seconds where the radio smooths over one. An
earlier attempt fitted the floor to a replay of a three hertz capture and got
a shoulder figure of 18.2 dBuV, which is not a number this radio ever
produces. The radio's own smoothed level is captured alongside the raw one
instead.

**What it did on the radio.** Before, 23 of 25 readings open with four
changes of state. After, measured the same way over thirty readings:

| Channel | Open | Changes of state |
|---|---|---|
| 102.0, shoulder | 0 of 30 | 0 |
| 91.0, shoulder | 0 of 30 | 0 |
| 105.0, empty | 0 of 30 | 0 |
| 92.7, station | 30 of 30 | 0 |
| 101.9, station | 30 of 30 | 0 |

**Opening is still quick.** Tuning from a shut shoulder straight to a station,
the audio opened 0.30, 0.31 and 0.41 seconds after the tune command over three
trials, including the HTTP round trip and the tuner settling. It was 0.17 to
0.25 s before opening required two of the last three readings, so about 0.15 s
of that is the price of not opening on one reading. The level average is
started again on every retune, without which it would be about a second.

The working, the capture and the honest limits are in
`test/fixtures/squelch/README.md`. The margin between the highest smoothed
shoulder reading and the lowest station reading is about 1.4 dB across two
days, which is not comfortable, and a third day of measuring is worth doing.

### The volume pot never sits still, and that is why the dim works

Measured on 13 September 2026, over half a minute with nobody touching the
radio: `inp.pot` wandered between 3254 and 3258, four counts out of 4095.

That is small, but it is not nothing, and it decides whether the panel ever
dims. The idle clock is restarted by any input, and the volume pot is read
twenty times a second. A radio that counted every reading as an input would
never be idle and would never dim, with nothing anywhere saying why. Only a
move past the deadband in `core/input.h` counts, which this wander does not
reach. Confirmed by watching the panel stay dimmed for twenty eight seconds
while the reading moved over that range.

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

### Only one audio channel is heard

Measured on 13 September 2026 with the tuner's own tone generator, AUDIO module command 24. That command takes two amplitude and frequency pairs, and what the two are is not written down anywhere: the reference firmware sets both the same, so reading it settles nothing.

Four tones were played through the speaker, three seconds each:

| Slot one | Slot two | Heard |
|---|---|---|
| 400 Hz | 400 Hz | low |
| 1500 Hz | 1500 Hz | high |
| 400 Hz | 1500 Hz | low |
| 1500 Hz | 400 Hz | high |

The last two are the answer. What comes out follows slot one and slot two is inaudible, so the two slots are the left and right output channels rather than two independent generators. That fits the TPA6211A1 above, which is a mono amplifier.

**So two tones cannot be sounded at once through the speaker.** Anything that needs a pair, DTMF being the obvious one, cannot be done with this generator on this radio. A stereo headphone output would put one tone in each ear, which is still not a pair.
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

## What the tuner can do, and what this firmware asks it to

Surveyed on 12 September 2026, against the feature list NXP publishes for the
TEF668x family and against the complete command set in the working PE5PVB
firmware. **NXP does not publish a command reference.** Only a short datasheet
is public, so that firmware's own header is the best record of the command
numbers and their arguments, and everything here was checked against it.

| Chip feature | Command | This firmware |
|---|---|---|
| Tuning, FM and AM | 1 | Used |
| Channel bandwidth, fixed or adaptive | 10 | Used |
| RF AGC, both bands | 11 | Used, at the reference's values |
| AM RF attenuation | 12 | Used |
| AM co-channel detection | 14 | Used |
| FM multipath suppression, iMS | 20 | Used, off by default |
| FM channel equalizer, EQ | 22 | Used, off by default |
| Noise blanker, both bands | 23, 24 | Used, off by default. A percentage, 0 or 50 to 150, not a level in dBuV |
| FM de-emphasis | 31 | Used, 50 us |
| FM stereo improvement, FMSI | 32 | **Not fitted.** This part reports it absent, so there is nothing to send |
| Level offset | 39 | Used, -7.0 dB |
| Soft mute | 45 | Used, off on FM and on for AM |
| High cut against level, noise, multipath | 52, 53, 54 | Sent. Off unless a start level is given |
| High cut ceiling | 55 | Used, 7 kHz |
| Stereo blend against level, noise, multipath | 62, 63, 64 | Sent. Off unless a start level is given |
| Forced mono | 66 | Used |
| Stereo high blend | 72, 73, 74, 75 | Sent. Off unless a start level is given. Needs its ceiling, 75, or the other three do nothing |
| RDS | 81 | Used. Mode 1, sent on every FM tune, which also restarts the decoder |
| MPX output | 85 | Not used. The default is the one we want |
| Adaptive bandwidth extension | 86 | Used, follows the signal |
| Stereo band blend | 90, 91, 92 | Belongs to FMSI, which this part does not have |
| RDS data | 131 | Used. Read every 43 ms on FM. Twelve bytes: a status word, blocks A to D, and an error word |
| Quality status | 128 | Used |
| Quality data | 129 | Not used. What it adds over 128 is not documented anywhere public |
| Processing status | 134 | Read every poll on FM. Says how much high cut, stereo blend and stereo high blend the chip is applying now |
| Signal status | 133 | Used, for the stereo pilot |
| Volume, mute | Audio 10, 11 | Used |
| Audio input select, wave generator | Audio 12, 24 | Not used. Together they make the key beep |
| Operation mode, identification | Appl 1, 128, 130 | Used |
| Tuner GPIO | Appl 3 | Not used. Antenna switching, reachable only through a protocol that is out of scope |

### What the chip is actually applying, measured

Read from the processing status, command 134, which the reference firmware
implements and never calls. All figures FM, 12 September 2026.

With the weak signal handling off, as this radio and the reference both ship:

| Station | Level | Treble roll off | Stereo blend |
|---|---|---|---|
| 102.8 | 44.8 dBuV | none | none |
| 106.4 | 33.4 dBuV | none | none |
| 104.0 | 18.9 dBuV | none | none |
| empty | below zero | none | blending |

So it does nothing at all until there is no signal left. With the start level
set to 40 dBuV, which is above every local station here:

| Station | Level | Treble roll off | Stereo blend |
|---|---|---|---|
| 102.8 | 44.8 dBuV | 0 | 0 |
| 106.4 | 33.4 dBuV | 47 | 92 |
| 104.0 | 32.9 dBuV | 51 | 96 |

**Those three numbers have no unit that anybody has established.** The
reference divides each raw word by ten and never uses the result, so there is
nothing to say what it counts. They were written down here as kHz at first,
which was a guess and a poor one: a reading of 47 on a station being rolled
off is not 47 kHz of audio.

What they are good for is that they move. Zero means the mechanism is doing
nothing and rising means it is, which is enough to tell a feature that is
working from one that was never switched on. That is the whole reason for
reading them.

The mechanism works and is controllable. Whether it sounds better is a
separate question and has not been settled.

**The stereo high blend needs its ceiling set.** Commands 72, 73 and 74 do
nothing at all unless command 75 has given it a limit. That is why it read
zero while the other two moved.

### The AM noise blanker does not show an audible benefit here

Tested on shortwave 11900 kHz on 12 September 2026, because impulse noise is
what makes shortwave tiring and the blanker is the chip's answer to it.

Three attempts, and the first two were wrong:

1. A single before and after measurement showed the noise reading falling from
   221 to 81. Alternating four times showed both states wandering between 50
   and 225, so that was drift, not the blanker.
2. Listening with it switched on and off sounded clearly better with it on.
   The antenna was being moved at the time, which is the same mistake again.
3. A blind test, four pairs with the blanker in a random half of each and the
   listener not told which. Two pairs were judged. **One of two**, which is
   chance.

So there is no evidence it helps here, and it stays off, which is also how the
reference ships it. It is worth noting what nearly happened: a measurement, a
listening impression and the fact that the chip offers the feature all pointed
the same way, and all three were worthless. The blind test cost ten minutes.

The reading the tuner gives for noise is continuous, and a blanker works on
impulses, so that number may never be able to settle this question. Anything
about this feature has to be decided by ear, blind.

### The four that were doing nothing, and now can

All four are wired up now. Three are still off by default, matching the
reference, because off is where it ships and there is no evidence yet for
changing that.

- **High cut** rolls the treble off as a signal weakens.
- **Stereo blend** moves towards mono as a signal weakens.
- **Stereo high blend** does both together, and NXP lists it as a headline
  feature of the part. It needs its ceiling set or the other three writes land
  on a mechanism with no limit and do nothing, which is exactly what it did
  at first.
- **Processing status** says how much of each the chip is applying, and is
  read on every poll.

On a portable with a whip antenna, trading stereo separation and treble for
less hiss is usually a good bargain, which is what these do. Whether it is a
good bargain here is still a listening question, but it is no longer an
invisible one.

### What the RDS decoder in the chip does, measured

Read over `GET /api/rds/raw` on 13 September 2026. The captures and the full working are in `test/fixtures/rds/README.md`.

**The status word.** Command 131 returns twelve bytes: a status word, blocks A to D, and an error word. Only three bits of the status are used here and only two are documented anywhere public.

| Bit | Meaning | Seen |
|---|---|---|
| 15 | A group is waiting in the registers | `8200` on a station sending groups |
| 13 | Only block A is real. Not a group | Never seen on the six stations here |
| 9 | The decoder is locked to an RDS bit stream | `0200` on a locked station between groups |

A station with no RDS reads `0000` with the read reporting success, and so does the AM side. So the status word alone cannot tell a station without RDS from a bus that is not answering, which is why the driver reports whether the read worked as a separate flag.

**The error word** carries two bits per block, block A in the top pair. 0 clean, 1 or 2 corrected, 3 not corrected.

**The chip protects block A hardest.** Over 2777 groups in the two weak captures, at about 10 dBuV, block A came back clean every single time while blocks B, C and D were corrected 310, 247 and 245 times between them and lost 12 times. So the programme identifier is the field most likely to survive a bad signal, and the station name the least.

**A block the chip reports as corrected can still be wrong.** Measured on those same two captures: text decoded from blocks reported as corrected at the smallest error level produced `MAwQCFM VINTOO...` where the station sent `MAGICFM VINTOO...`, and a text whose terminator had been corrupted so it ran on into the padding. Only blocks reported clean are safe to decode from. This is why `core/rds.c` throws away corrected blocks, which is stricter than the reference firmware.

**Detuning does not produce a degraded stream.** 100 kHz either side of 93.5, 98.3 and 106.4 gives no lock at all rather than a lock with errors. The decoder either locks cleanly or does not lock. Collapsing the whip does work: every station drops from about 45 dBuV to about 10 and the two strongest stay locked while their blocks start failing.

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
