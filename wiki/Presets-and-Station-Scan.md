# Presets and Station Scan

The radio has 99 presets. A station scan fills the empty ones with the stations it finds. Presets can also be written from a spreadsheet and loaded over Wi-Fi.

## What a preset holds

| Part | What it is |
|---|---|
| Slot | 1 to 99 |
| Band and frequency | Any band |
| Filter width | A width the band offers, or automatic |
| Name | Up to 16 plain characters. It shows on the radio screen when the station sends no RDS name |
| PI | The station's RDS code. The radio learns it by itself the first time it hears that station's RDS code confirmed, and uses it to check that the right station is on the frequency |
| Programme type and country | What the station says it plays, such as `Pop Music`, and its extended country code (ECC), which with the PI names the country. Taken when you save on that station, or learnt later, once the station's PI is confirmed on the preset. A value once kept is not changed by another station |
| Saved | When the preset was saved, by the radio's clock. Empty when the clock was not set, and for presets saved by a firmware before this one kept the time |

Presets are kept in the radio's flash, over power cycles and updates.

## Listen to a preset

There are two ways on the radio:

- **Tuning mode MEM.** Press MODE until the first tile shows `MEM`. Each click of the tuning knob goes to the next stored preset, across all bands, and round again. The radio screen shows the preset number, such as `P03`.
- **The menu.** Open **Stations > Presets**. It lists the stored presets in slot order, each as `P03` with its name, and the frequency. Press a row to open its card, then **Tune**.

A preset whose frequency is not on the band plan in use, for example after a change of FM Band Plan, is skipped in MEM mode. On its card, **Tune** shows the note `Not on the band plan` and tunes nothing.

### A preset's card

![A preset's card](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/menu-station-preset.png)

A press on a preset in **Stations > Presets** opens its card: the frequency and the name; under them the slot such as `P03`, then the PI, the country and the programme type, each when the preset has it; `Saved` and when; and tiles for the band and the width (`Auto` or its own).

- **Tune** tunes to it and leaves the card open, so you can hear it.
- **Delete** clears the slot. The first press turns the button red and says `Press again`; a second press within 5 seconds clears it and goes back to the list. Turning the knob off it, going back or waiting 5 seconds lets it go. The last preset cleared takes you back to **Stations**.
- Turn the knob to move between the two, and press it to act. MODE, or a hold of the knob, goes back to the list, and so does a tap on the title while Touch is On.

If the preset was moved or cleared on the web page while its card was open, **Tune** and **Delete** say `Changed since this opened` and do nothing; go back and open it again. If the radio cannot write the clear, it says `Not deleted`.

The keypad always types a frequency, never a preset number.

## Saving and changing presets

Presets cannot be renamed on the radio itself; a preset can be cleared from its card, above. They are written in five ways:

- **Menu > Go To > Save Current Station** on the radio. It saves the station on the dial in the lowest free slot, with its band, the width the radio is set to and its RDS name, and the radio screen says `Saved as P10`. A station already saved is not saved again: the menu stays open with `Already P05`. With all 99 slots used it says `Every slot is taken`
- a station scan, which fills empty slots
- the web page's [Presets](Web-Page.md#presets) screen: save the current station, add one by its frequency, change, rename or delete one, and drag one to another slot. A station already saved is not saved again
- the HTTP API, one preset at a time
- a CSV file loaded over Wi-Fi

## Station scan

A station scan walks one band and saves each station it finds to the next empty preset. The four scan rows are in the menu's **Stations** group, between Presets and Station Log:

| Row | Band | Step |
|---|---|---|
| FM Station Scan | FM | 100 kHz |
| AM Station Scan | Medium wave | 9 or 10 kHz, by MW Step |
| SW Station Scan | The shortwave metre bands from 160 m to 11 m only, 1000 channels | 5 kHz |
| LW Station Scan | Long wave | 9 kHz |

OIRT is not scanned.

- A channel counts as a station by the same test as [seek](Tuning.md#seek), at that band's **Seek Sensitivity**. On AM each channel is read twice and judged on the quieter reading.
- A station that already has a preset within 100 kHz on FM, or within half a step on AM, is skipped.
- A found station gets the lowest free slot, with automatic width and no name. A used slot is never overwritten.
- The sound is muted while it scans. A scan of another band switches to that band first and comes back at the end.
- A scan takes from about 10 seconds to 4 minutes. FM takes about a minute.
- While it runs, the row shows its progress, for example `Scan 12/206`, and the menu does not time out. Auto Off waits for it.

To stop a scan, press the same row again: the radio goes back to where it was. Tuning, seeking or changing the band also stops it, and the radio stays where you put it. The presets found up to then are kept either way.

When it ends, the row shows `Saved 12`, `Stopped, 3 saved`, or `Full, 5 not saved` when stations were found after all 99 slots were in use.

## Presets as a CSV file

A CSV file can be opened in any spreadsheet, so a whole band plan can be written there and loaded in one go.

### The format

```
slot,band,frequency,bandwidth,name,pi,pty,ptn,ecc,country,saved
1,FM,106400,0,MAGIC,1064,10,Pop Music,E2,IN,2026-10-12 15:40
2,MW,738,0,,,,,,,
```

| Column | What goes in it |
|---|---|
| slot | 1 to 99 |
| band | `FM`, `OIRT`, `LW`, `MW` or `SW`, in any case |
| frequency | In kHz, so 106.4 MHz is `106400` |
| bandwidth | The filter width in kHz, or `0` for automatic |
| name | Up to 16 plain characters. Put it in double quotes if it has a comma in it |
| pi | Four hex digits, or empty |
| pty | The programme type, 1 to 31, or empty |
| ptn | The programme type's name, in the RDS region the radio is set to. Only for reading: an import passes over it |
| ecc | The extended country code, two hex digits, or empty |
| country | The country the PI and the ECC name, as two letters. Only for reading: an import passes over it |
| saved | When it was saved, `2026-10-12 15:40` in UTC, or empty. A time that cannot be read, for example one a spreadsheet rewrote, imports as empty |

Blank lines and lines starting with `#` are skipped. Files from an earlier firmware, with only the first six columns or without the `pi` column too, are also taken.

### Export

```bash
curl -s -o presets.csv http://tef668x.local:8080/api/presets.csv
```

This needs no PIN. It has every stored preset. On the web page's [Presets](Web-Page.md#presets) screen, **Download** gives the same file, and **Upload** imports one, with a choice of **Merge** or **Replace** as below.

### Import

Send the file as the body, with the content type `text/csv`, after signing in with the access PIN:

```bash
R=http://tef668x.local:8080
curl -s -c jar -d pin=000000 $R/auth
curl -s -b jar --data-binary @presets.csv -H 'Content-Type: text/csv' \
     "$R/api/presets/import?mode=merge"
```

| Mode | What it does |
|---|---|
| `merge`, the default | Fills only the empty slots named in the file. A slot already in use is left as it is. Lines that cannot be read are counted and skipped |
| `replace` | Replaces every preset with the file. It is all or nothing: if one line cannot be read, nothing changes and the answer names the line |

The answer says what happened, for example `12 of 12 lines imported, 0 refused, 0 already taken, 0 names cut short`.

A `replace` with only the header line clears every preset. It is the only way to clear them.

The import does not check the band plan. A preset outside the band plan in use is stored, but cannot be tuned until the band plan allows it.

## One preset from the HTTP API

`POST /api/presets` needs the access PIN.

```bash
curl -s -b jar -d do=set -d slot=3 -d khz=98300 -d name=NEWS $R/api/presets
curl -s -b jar -d do=recall -d slot=3 $R/api/presets
curl -s -b jar -d do=move -d slot=3 -d to=10 $R/api/presets
curl -s -b jar -d do=clear -d slot=10 $R/api/presets
```

| Field | What it is |
|---|---|
| `do` | `set` to store, `recall` to tune, `move` to put it in another slot, `clear` to delete it |
| `slot` | 1 to 99 |
| `to` | For `move`, the slot it goes to, 1 to 99. When that slot holds a preset, the two swap. The name, width and PI go with it |
| `khz` | The frequency in kHz. The band is worked out from it |
| `bw` | The filter width in kHz, optional. `0` for automatic |
| `name` | The name, optional. Left out, the old name stays. Sent empty, the name is cleared |

The answer is one line, for example `Preset 3 is FM 98.30 MHz`, `Preset 3 moved to 10.`, `Presets 3 and 10 swapped.` or `Preset 10 deleted.` A recall, move or clear of an empty slot answers 404 `Preset 3 is empty.`

A station scan can also be started from the web page's [Presets](Web-Page.md#presets) screen, which shows its progress and result, or from the API: `POST /api/scan` with `bnd=FM`, `MW`, `SW` or `LW`.
