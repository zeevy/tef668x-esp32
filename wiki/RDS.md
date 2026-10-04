# RDS

RDS is the data an FM station sends along with its sound: its name, its programme type, a line of text, other frequencies it is on, and more. The radio shows the most used parts on the radio screen, and all of it on four RDS pages.

## On the radio screen

- The **name line** in the amber panel shows the station's RDS name. With no RDS name it shows the preset's name, or `---`.
- The **text line** under it shows the radio text, scrolling round. With no radio text it shows the date, once the radio has the time from the network.

## Open the RDS pages

On FM or OIRT, hold **BAND**. On AM the radio says `Switch to FM first`. You can also open them from **Menu > Go To > RDS**, and over DX mode with a hold of BAND on any DX page.

| Control | What it does |
|---|---|
| Tuning knob, turn | The next or previous page. From the last page it goes round to the first. It does not tune |
| ENTER | The next page |
| Hold the tuning knob, hold BAND, or tap MODE | Closes the RDS pages. Over DX mode, it goes back to DX mode |
| Hold ENTER | Writes the station to the station log |

The pages always open on page 1, and they do not time out. The header shows the frequency, the page number such as `1/4`, and the clock.

## Page 1, RDS

![RDS page 1: MAGIC, Easy Listening, PI 1064](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/rds-station.png)

| Field | What it shows |
|---|---|
| `Stereo` or `Mono` in the header | What the station says it sends. This is the station's own flag, not the stereo pilot the tuner hears |
| Name | The station's name, in the amber panel |
| Programme type | Its name, such as `Easy Listening`, and `PTY` with its number, 0 to 31 |
| PI | The station's four digit code, in hex. Dim while it has been heard only once, with `?` for each digit that changed between two hearings. `0000` when the station sends zero |
| ECC | The extended country code, in hex, at the top right of the PI tile |
| Under the PI | The country as a two letter code, such as `IN`. `no ECC heard` when the station sends no country code. `not listed` when the code has no country. In North America mode, the call letters. On a preset with a stored PI: `P05`, or `not P05` in red when another station is on that frequency |
| TP | Traffic programme: the station carries traffic news. Lit means yes |
| TA | Traffic announcement on air now. Lit means yes |
| `Music` or `Speech` | What is on air now. `M/S` until it is sent |
| AREA | How far the station reaches: `Local`, `International`, `National`, `Supra-Regional`, or `Regional 1` to `12` |
| CT | The station's own clock time, `HH:MM` |
| PTYN | The station's own name for its programme type, up to 8 characters |
| LANG | The language of the programme |

A field with nothing received shows `-`.

## Page 2, Radio Text

![RDS page 2, Radio Text](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/rds-text.png)

- The radio text, up to 64 characters, wrapped over lines. A character the screen cannot show appears as `_`.
- **RT+** rows: up to three parts of the text that the station marks, such as `TITLE` and `ARTIST`. A play mark shows while the station says the item is playing.
- With no RT+ the page says `No RT+ heard from this station`, or `No RT+ tags for this text yet`.

## Page 3, Networks

![RDS page 3, Networks](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/rds-networks.png)

| Part | What it shows |
|---|---|
| ALTERNATIVE FREQUENCIES | Other frequencies the same station is on, up to 8 tiles such as `98.30`. With more than 8: 7 tiles and `+` the rest. The frequency you are on is left out |
| OTHER STATIONS, EON | Up to 3 other stations of the same network: PI, name, two of their frequencies, and a TA mark, red while they have a traffic announcement on air. A station shows after it is heard twice |

`None heard` shows when a list is empty.

## Page 4, Decoder

![RDS page 4, Decoder](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/rds-decoder.png)

This page shows how well RDS is being received, over the last minute at most. The header says how long that is, for example `last 13 s`.

| Tile | What it shows |
|---|---|
| SYNC | How long the decoder has been locked, such as `41s` or `2h05m`, in green. `No` when it is not locked. `Off` when RDS Decoder is off |
| GROUPS/S | RDS groups received each second. A clean station sends about 11.4 |
| BLER | Block error rate: the share of blocks that were lost |
| LOST | Lost blocks out of all blocks |
| EACH BLOCK A-D | For each of the four blocks in a group: clean, fixed by the tuner, or lost. `all clean` when none had errors |
| GROUP TYPES SENT | The kinds of group the station sends, most first, such as `0A 50 %` |

## When a field is shown

The decoder uses only blocks that arrived clean. A block the tuner had to correct is counted on page 4 but not used. Most fields are shown only after they were received twice the same way, so a half arrived name never shows: the PI, the programme type, the name, PTYN, ECC, the language, the stereo flag, RT+ and the EON stations.

Some are shown at once: TP, TA and music or speech from the latest group, the radio text once all its characters have arrived, the alternative frequencies on the first hearing, and the clock time from one clean group.

Some stations send a long name in pieces, a few characters at a time. The radio joins two to four pieces into one name, for example `SAMPLE 9` and `5` into `SAMPLE 95`.

When the signal goes and the decoder loses its lock, SYNC shows `No`, but the fields keep their last values. They clear when you tune, when a new PI is confirmed, or when RDS is turned off.

## Settings

Both are in **FM Reception > RDS**.

| Row | Values | What it does |
|---|---|---|
| RDS Decoder | On, Off. On at the start | Turns RDS reading on or off |
| RDS Region | Europe, North America. Europe at the start | North America uses the RBDS programme type names, shows call letters worked out from the PI under the PI, and shows AREA only for PIs that carry it there. The PI itself still shows in hex |

The RDS clock time is shown as CT, but it never sets the radio's clock. The clock comes from the network only.
