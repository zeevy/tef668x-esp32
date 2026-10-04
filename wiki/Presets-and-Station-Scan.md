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

Presets are kept in the radio's flash, over power cycles and updates.

## Listen to a preset

There are two ways on the radio:

- **Tuning mode MEM.** Press MODE until the first tile shows `MEM`. Each click of the tuning knob goes to the next stored preset, across all bands, and round again. The radio screen shows the preset number, such as `P03`.
- **The menu.** Open **Stations > Presets**. It lists the stored presets in slot order, each as `P03` with its name, and the frequency. Press a row to tune it. The menu stays open, so you can try the next one.

A preset whose frequency is not on the band plan in use, for example after a change of FM Band Plan, is skipped in MEM mode, and in the menu it shows `Not on the band plan`.

The keypad always types a frequency, never a preset number.

## Saving and changing presets

Presets cannot be saved, renamed or cleared on the radio itself. They are written in three ways:

- a station scan, which fills empty slots
- the HTTP API, one preset at a time
- a CSV file loaded over Wi-Fi

## Station scan

A station scan walks one band and saves each station it finds to the next empty preset. Open the menu and go to **Stations > Station Scans**:

| Row | Band | Step |
|---|---|---|
| FM Station Scan | FM | 100 kHz |
| AM Station Scan | Medium wave | 9 or 10 kHz, by MW Step |
| SW Station Scan | The shortwave broadcast bands only, about 1000 channels | 5 kHz |
| LW Station Scan | Long wave | 9 kHz |

OIRT is not scanned.

- A channel counts as a station by the same test as [seek](Tuning.md#seek), at that band's **Seek Sensitivity**. On AM each channel is read twice and judged on the quieter reading.
- A station that already has a preset within 100 kHz on FM, or within half a step on AM, is skipped.
- A found station gets the lowest free slot, with automatic width and no name. A used slot is never overwritten.
- The sound is muted while it scans. A scan of another band switches to that band first and comes back at the end.
- A scan takes from about 10 seconds to 4 minutes. FM takes about a minute.
- While it runs, the row shows its progress, `Scan` and the channel count, and the menu does not time out. Auto Off waits for it.

To stop a scan, press the same row again: the radio goes back to where it was. Tuning, seeking or changing the band also stops it, and the radio stays where you put it. The presets found up to then are kept either way.

When it ends, the row shows `Saved` and how many were saved, `Stopped` and how many, or `Full` and how many could not be saved because all 99 slots were in use.

## Presets as a CSV file

A CSV file can be opened in any spreadsheet, so a whole band plan can be written there and loaded in one go.

### The format

```
slot,band,frequency,bandwidth,name,pi
1,FM,106400,0,MAGIC,1064
2,MW,738,0,,
```

| Column | What goes in it |
|---|---|
| slot | 1 to 99 |
| band | `FM`, `OIRT`, `LW`, `MW` or `SW`, in any case |
| frequency | In kHz, so 106.4 MHz is `106400` |
| bandwidth | The filter width in kHz, or `0` for automatic |
| name | Up to 16 plain characters. Put it in double quotes if it has a comma in it |
| pi | Four hex digits, or empty |

Blank lines and lines starting with `#` are skipped. A file without the `pi` column is also taken.

### Export

```bash
curl -s -o presets.csv http://tef668x.local:8080/api/presets.csv
```

This needs no PIN. It has every stored preset.

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
```

| Field | What it is |
|---|---|
| `do` | `set` to store, `recall` to tune |
| `slot` | 1 to 99 |
| `khz` | The frequency in kHz. The band is worked out from it |
| `bw` | The filter width in kHz, optional. `0` for automatic |
| `name` | The name, optional. Left out, the old name stays. Sent empty, the name is cleared |

The answer is one line, for example `Preset 3 is FM 98.30 MHz`. A recall of an empty slot answers 404 `Preset 3 is empty.`

A station scan can also be started from the API: `POST /api/scan` with `bnd=FM`, `MW`, `SW` or `LW`.
