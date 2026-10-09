# Sound and Bandwidth

The filter width, the volume and mute, squelch, the volume AGC, and the settings that clean up a weak or noisy station.

## Filter width

The filter width sets how much of the band around the frequency the tuner lets through. A narrow filter keeps a strong station next door out. A wide filter gives better sound on a clean station.

| Band | Widths | At the start |
|---|---|---|
| FM, OIRT | Automatic, then 56, 64, 72, 84, 97, 114, 133, 151, 168, 184, 200, 217, 236, 254, 287, 311 kHz | Automatic |
| LW, MW, SW | 3, 4, 6, 8 kHz | 4 kHz |

**Automatic** is on FM and OIRT only: the tuner narrows the filter by itself when a strong station is next to it. The filter width tile, with the up and down arrow, shows `DYN` while it is automatic, and the width, such as `84k`, when it is fixed.

Each band keeps its own width, over a power cycle too.

### Change the width

- **Tap BW** on the radio screen for the next width. From the last it goes round to the first.
- **Hold BW** for the bandwidth page, below.
- The AM widths are also in the menu at **AM Reception > Filter Bandwidth**, with a row each for LW, MW and SW. They can be set from any band.

### The bandwidth page

![The bandwidth page on FM](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/bw-fm.png)

Hold BW on the radio screen, the RDS pages or DX mode, or use **Menu > Go To > Bandwidth**. The header shows the band and the place of the cursor, such as `1/19`.

- On FM it shows 19 tiles: AUTO, the 16 widths, **iMS** and **EQ**. On AM it shows the 4 widths.
- The width in use, and a switch that is on, are filled in the theme's main colour. A ring marks the cursor, which starts on the width in use.
- While AUTO is in use, a note under the tiles says which width the tuner picked, for example `Auto: 217 kHz`.

Turn the knob to move between tiles. Press the knob or ENTER to use that width at once, or to turn iMS or EQ on or off. The page stays open, so you can try the next one. Hold the knob, or tap BW or MODE, to close it. It also closes after one minute with no input, and when the band changes.

Over DX mode there is no AUTO tile, and the width you pick is DX mode's own, which is not saved.

![The bandwidth page on MW](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/bw-mw.png)

## Volume and mute

The volume knob sets the volume from -60 dB to 0 dB. See [Controls](Controls.md#the-volume-knob). The volume tile, with the speaker, shows it, for example `-20dB`.

There is no mute key on the radio. Mute is on the web page's Radio page, or `POST /api/mute on=1`. While muted, the volume tile shows a speaker with a cross and `MUTE`, in red.

**Audio > Mute Ramp** sets how long the sound fades before a mute, before the squelch closes, and around a change of filter width: 0 to 500 ms in steps of 10, 120 ms at the start. At 0 the sound is cut at once. A seek and a station scan do not use the ramp.

## Squelch

Squelch keeps the sound off while there is no station worth hearing. Set it at **Audio > Squelch > Squelch Mode**:

| Mode | What it does |
|---|---|
| Off, at the start | The sound is always on |
| Auto | The radio judges the signal itself. On FM it opens when the noise is under 12.0 %, multipath under 23.0 %, the offset under 10.0 kHz and the level at or above the Squelch Floor. On AM it opens when the noise is under 12.0 % and the offset under 2.0 kHz. It closes after 1 second of bad readings |
| Manual | The volume knob sets the squelch level instead of the volume, from -10 to 92 dBuV. The sound opens when the signal is above that level. The bottom of the knob's travel means always open |

| Row | Values | At the start |
|---|---|---|
| Squelch Mode | Off, Auto, Manual | Off |
| Squelch Level | Read only: the Manual level the volume knob set. `Off` until the knob has set one | |
| Squelch Floor | Off, or 1 to 40 dBuV | 15 dBuV |

The squelch tile, with the waves, shows `OFF`, `AUTO`, or the Manual level such as `15dB`. While the squelch holds the sound back, the volume in the volume tile turns grey.

In Manual squelch the volume stays at the stored volume, -20 dB at the start, or what the web page or `/api/volume` set.

## Volume AGC

The volume AGC brings stations to a similar loudness, so one station is not much louder than the next. It works from a slow average of each station's modulation, and changes the gain the tuner gets, never the knob's volume.

| Row | Values | At the start | What it does |
|---|---|---|---|
| Audio > Volume AGC | Off, or 30 to 80 % in steps of 2 | Off | The target. A lower target makes stations more even, and everything quieter. While it runs, the row shows the target and the gain now, such as `50 %, -3 dB` |
| Audio > AGC Boost | Cut Only, or 1 to 8 dB | Cut Only | The most it may raise a quiet station. Cut Only never raises one |

The most it lowers a loud station is 12 dB.

## FM settings

In the menu's **FM Reception** group. These work on FM and OIRT. Set them while the radio is on one of those bands: on AM, most of them are not taken.

| Row | Values | At the start | What it does |
|---|---|---|---|
| De-emphasis | Off, 50 us, 75 us | 50 us | The treble cut that matches the broadcast. 75 us in the Americas and South Korea, 50 us elsewhere |
| Stereo > Force Mono | Off, On | Off | Plays every station in mono |
| Stereo > Stereo Blend | Off, or 20 to 60 dBuV | Off | Below this signal level, the sound blends towards mono, which is less noisy |
| Stereo > Stereo / High Blend | Off, or 20 to 60 dBuV | Off | Below this level, blends to mono and cuts the treble together |
| High Cut | Off, or 20 to 60 dBuV | Off | Below this level, cuts the treble, where the noise of a weak signal is |
| Noise Blanker | Off, or 50 to 150 % in steps of 5 | Off | Blanks short bursts of noise, such as from a car engine |
| iMS | Off, On | Off | Multipath suppression: less distortion from signals reflected off hills and buildings |
| Equalizer | Off, On | Off | The channel equaliser, another help against multipath |

iMS and EQ can also be turned on and off on the bandwidth page.

## AM settings

In the menu's **AM Reception** group. Each row can be set from any band.

| Row | Values | At the start | What it does |
|---|---|---|---|
| Filter Bandwidth > LW, MW, SW | 3, 4, 6, 8 kHz | 4 kHz | The filter width of each band |
| Noise Blanker | Off, or 50 to 150 % in steps of 5 | 100 % | Blanks short bursts of noise |
| High Cut > MW / SW | Off, or 20 to 60 dBuV | 47 dBuV | Below this level, cuts the treble |
| High Cut > LW | Off, or 20 to 60 dBuV | 52 dBuV | The same, for long wave |
| Soft Mute > MW / SW | 0 to 50 dBuV | 28 dBuV | Below this level, turns the sound down, so the noise between stations is quieter |
| Soft Mute > LW | 0 to 50 dBuV | 34 dBuV | The same, for long wave |

## From the web page and the API

The web page's **FM & RDS** page has the FM and AM settings, and its **Radio** page has the volume, mute and squelch mode. From a script:

```bash
curl -s -b jar -d khz=0 $R/api/bandwidth        # automatic width on FM
curl -s -b jar -d db=-20 $R/api/volume          # volume -20 dB
curl -s -b jar -d on=1 $R/api/mute              # mute
curl -s -b jar -d mod=auto $R/api/squelch       # Auto squelch
curl -s -b jar -d ims=1 -d eq=1 $R/api/fm       # iMS and EQ on
```

These need the access PIN first.
