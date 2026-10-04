# Tuning

How to change the band and the frequency: with the tuning knob in four tuning modes, by seek, and by typing a frequency.

## Bands

BAND steps through the bands in this order: LW, MW, SW, OIRT, FM, then LW again.

| Band | Range | Steps | First step | Unit |
|---|---|---|---|---|
| LW, long wave | 144 to 513 kHz | 1, 9 kHz | 9 kHz | kHz |
| MW, medium wave, with MW Step 9 kHz | 522 to 1791 kHz | 1, 9 kHz | 9 kHz | kHz |
| MW, medium wave, with MW Step 10 kHz | 520 to 1720 kHz | 1, 10 kHz | 10 kHz | kHz |
| SW, shortwave | 1700 to 27000 kHz | 1, 5 kHz | 5 kHz | kHz |
| OIRT, the eastern European FM band | 65.00 to 74.00 MHz | 10, 30 kHz | 30 kHz | MHz |
| FM | By the band plan, below | 50, 100, 200 kHz | 100 kHz | MHz |

The FM range is set by **FM Reception > Band Plan**:

| Band Plan | FM range |
|---|---|
| Worldwide, the default | 87.5 to 108 MHz |
| 87-108 MHz | 87 to 108 MHz |
| Wide | 76 to 108 MHz |
| Japan | 76 to 95 MHz |
| Full | 65 to 108 MHz. A frequency from 65 to 74 MHz that you type, or send from the API, goes to the OIRT band |

**AM Reception > MW Step** sets the medium wave spacing: 9 kHz for Europe, Africa and Asia, 10 kHz for the Americas. It moves the band edges too. Band Plan and MW Step take effect after a restart.

The step size is set per band: **FM Reception > Tuning Step**, which can be changed while the radio is on FM, and **AM Reception > Tuning Step** with a row each for LW, MW and SW.

### Each band remembers its place

When you leave a band, it keeps its frequency, filter width and step. When you come back, it starts where you left it. This is kept over a power cycle. A band you have never used starts at its bottom edge.

On the very first start the radio is on FM 104.00 MHz, in MAN mode, with a 100 kHz step.

## Tuning modes

MODE steps through the tuning modes. The mode is shown in the first tile on the radio screen, and it is the same for every band.

| Mode | What a turn of the tuning knob does |
|---|---|
| `MAN`, manual | Steps by the step size. A fast turn moves 2, 4 or 6 steps for each click. At a band edge it goes round to the other edge |
| `AUTO`, seek | Seeks the next station in that direction. A turn the same way stops the seek. A turn the other way seeks the other way |
| `MEM`, presets | Goes to the next stored preset, one per click, across all bands, and round again. See [Presets and Station Scan](Presets-and-Station-Scan.md) |
| `MTR`, metre bands | Shortwave only. Steps only inside the broadcast bands from 160 m to 11 m, and jumps over the gaps between them |

If you leave shortwave while in MTR, the mode goes back to MAN. In DX mode the knob always steps like MAN.

## Seek

A seek starts with a turn of the tuning knob in AUTO mode, or from the HTTP API. It moves one step at a time, and stops on a station, not on noise.

A channel counts as a station when all of these hold:

- the noise reading is low enough, by the Seek Sensitivity
- on FM, the multipath reading is low enough and the signal is strong enough, also by the Seek Sensitivity
- the channel is centred, within 20 kHz on FM or 2 kHz on AM
- the squelch opens on it

**Seek Sensitivity** is a row in FM Reception and another in AM Reception, from 1 to 6, with 4 at the start. A higher number stops on weaker stations. At 4 on FM, a station must have a noise reading (USN) under 12.0 %, multipath under 32.0 % and a level of at least 10.0 dBµV.

A seek goes round at the band edge and keeps going. If it goes round the whole band and finds nothing, it stops where it is. Any other key or command also stops it.

**Controls > Band Edge Beep** beeps when the knob steps over a band edge in MAN mode. It is off at the start. A seek does not beep.

## Typing a frequency

Type the digits on the keypad and press ENTER. There is no decimal point to type. The radio reads the number as kHz and multiplies it by 10, 100 and so on until it falls inside a band. It tries the band you are on first, then LW, MW, SW, OIRT and FM.

So `1064` tunes:

| On this band | It tunes |
|---|---|
| FM | 106.40 MHz |
| MW | 1064 kHz |
| SW | 10640 kHz |

- When the number does not fit the band you are on but fits more than one other band, a list opens, for example `Tune 1064 to`, with `Medium Wave 1064 kHz`, `Shortwave 10640 kHz` and `FM 106.40 MHz`. Turn to pick one and press to tune.
- When it fits no band, the name line says the number `is in no band`, and the digits are cleared.
- Up to 7 digits are taken. The digits are dropped 10 seconds after the last one.
- A turn of the knob or any other button drops the digits. ENTER held drops them and writes the station to the station log.
- A typed frequency is tuned as typed. It is not moved to the step size.

## From the HTTP API

```bash
curl -s -b jar -d khz=102800 $R/api/tune     # tune FM 102.80 MHz
curl -s -b jar -d stp=-1 $R/api/step         # one step down
curl -s -b jar -d bnd=MW $R/api/band         # change band
curl -s -b jar -d dir=up $R/api/seek         # seek up
```

These need the access PIN first. See [HTTP API](HTTP-API.md#signing-in).
