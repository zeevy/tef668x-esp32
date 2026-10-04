# DX Mode

DX mode is for finding far away FM stations. It has four pages: **DX** shows the station you are on in detail, **Scope** sweeps the whole band, **Scanner** walks the band and stops on stations, and **Catches** lists the stations it has confirmed.

## Open and leave

DX mode works on FM and OIRT. On AM the radio says `Switch to FM first`.

- **Open:** press DX, or **Menu > Go To > DX Mode**. **DX Scanner > Start Scan** opens it and starts a scan.
- **Leave:** press DX, tap MODE, or hold the tuning knob on the DX or Scanner page. DX mode also closes if the radio leaves FM.

DX mode always opens on the DX page. On entry the filter is set to **Scan Bandwidth**, 114 kHz at the start: the narrowest width that keeps RDS whole and stops a strong station next door from being decoded on this channel. This width is not saved: your own width comes back when you leave.

| Control | On every page |
|---|---|
| BAND | The next page: DX, Scope, Scanner, Catches, and round again |
| BAND, held | The RDS pages, over DX mode |
| ENTER | The next page |
| ENTER, held | Writes the station to the station log |
| BW | On the DX page, the next filter width for DX mode |
| BW, held | The bandwidth page |

While the scanner runs, any key or turn only stops it. The tuning knob does something different on each page, as below.

## DX page

![The DX page](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/dx-station.png)

Turn the knob to tune by the step size. Press it for the RDS pages. Hold it to leave DX mode.

| Part | What it shows |
|---|---|
| Header | `DX`, a stereo mark when there is a pilot, `1/4` and the clock |
| Panel | The station name, filled in a character at a time as it arrives, the programme type and the frequency |
| PI | Amber with a tick: confirmed. A clock mark: heard, but not confirmed as this channel's own. A `?` digit: some digits not yet sure. `0000` with `NO ID`: the station sends zero. Grey: nothing heard |
| Country | Two letters, worked out from the ECC and the PI, once the PI is confirmed. `?` with no ECC. In North America mode, the call letters |
| Preset line | `P05` when this is the station stored in that preset. `not P05` in red when another station is confirmed there |
| Six readings | LEVEL in dBµV, USN (noise) in %, WAM (multipath) in %, OFFSET in kHz, BW (the tuner's own filter reading) in kHz, and MOD (modulation) in % |
| History | 60 bars, one a second, the peak level of each second, from 0 to 70 dBµV. A gap means no reading. It starts again when you tune |
| Blocks A to D | Four segments for a clean block, three or two for an error the tuner fixed, one for an error it could not fix |

A PI is **confirmed** when it was received clean twice and the station's carrier is within the seek offset window of the frequency you are on. So the PI of the next channel's strong station is never confirmed here.

## Scope page

![The Scope page](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/dx-scope.png)

The Scope sweeps the whole band and draws the level of every channel, so you can see where something is coming up.

Press the knob to sweep. A sweep reads every channel of the band at the default step, muted, at the DX width. It takes about 4 seconds for the whole FM band. While it sweeps, the header says `sweeping`. After that it says how old the sweep is: `now`, `3 min ago` and so on.

Turn the knob to move the cursor one channel. Hold it to tune to the cursor.

| Part | What it shows |
|---|---|
| The bars | This sweep's level on each channel |
| White tick | The baseline: the usual level of that channel |
| Grey tick | The highest level since DX mode was opened |
| Dashed line | The floor |
| Green mark | Where the dial is |
| Strip below | How far each channel is above its baseline (amber) or below it (grey) |
| `MEDIAN OF n` | The baseline is the middle value of the last n sweeps, not counting this one. The last 8 sweeps are kept over a power cycle. `FIXED` when a baseline was set from the web page |
| `FLOOR` | The level that a quarter of the channels are below |
| Foot | The cursor's frequency and level, and `RISE`: how far it is above its baseline, in dB |

A rise over the baseline on a channel is the sign of a station coming in that is not usually there.

## Scanner page

![The Scanner page while a scan runs](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/dx-scan-run.png)

The scanner tunes one channel after another, waits for RDS, and stops on a station by the **Stop Condition**.

Press the knob to start the scan, or to go on after it stopped. While it is stopped, turning the knob tunes. Hold it to leave DX mode.

- A new scan starts with a sweep, then begins at the first channel of the range.
- On each channel it waits up to **Scan Dwell**, 2.5 seconds at the start, for a PI. It moves on as soon as a PI is confirmed and the stop condition does not stop on it.
- With **Stop Condition** at **New Stations Only**, it stops on a PI it has never caught before. It keeps a list of 512 caught PIs over power cycles.
- At the end of the range the dial goes back to where it started and the page shows `Ready` again. With **Loop Band** On, it goes round again.
- Channels that belong to another band on your band plan are skipped. A scan needs RDS on, and says `RDS is Off` otherwise.

The header shows how many stations were found on this round. The page shows the range being scanned (such as `BAND + PRESETS` or `WHOLE BAND`), the stop rule (`STOP ON NEW`, `STOP ON PI` or `NO STOP`), the dwell and the seconds left, a bar of how far it has got, and the station on the channel now. When it stops on a new station it shows its name and a `NEW` mark.

## Catches page

![The Catches page](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/dx-catches.png)

A catch is a PI confirmed as the tuned channel's own while DX mode is open. The list holds the last 32, newest first, and is kept until the radio is switched off, so leaving DX mode does not clear it. The same PI on two channels 100 kHz apart is one catch, kept on the stronger channel.

Each row shows the time it was last heard, the frequency, the PI, the name, the country, a green NEW mark for a PI never caught before, the best level, and `×` with how many times it was confirmed.

Turn the knob to move the cursor. Press it to tune to that catch. Hold it to write that catch to the station log. With no catches, the page says `No Catches Yet`, and holding the knob leaves DX mode.

## Logging new stations

With **Auto-Log Stations** On, which it is at the start, each NEW catch is written to the [station log](Station-Log.md) by itself, once: when its name arrives, or when you tune away or leave DX mode. With it Off, a NEW catch is marked as caught at once and is never logged by itself later.

**Log Radio Text**, On at the start, adds the station's radio text to every log entry, made by hand or by itself, when the text is whole and belongs to that station.

## Watching the presets

With **Watch Presets** On, which it is at the start, DX mode checks your presets in the background while you listen. Every 2 seconds it reads one preset of the band you are on, in the **Preset Range**. When a preset reads more than 8 dB above its usual level twice in a row, the header shows it, for example `98.30 up 9 dB`. It also beeps, but only when Controls > Key Beeps is set to something other than Off. Key Beeps is Off at the start.

The watch pauses during a scan, a sweep or a Learn Local Stations pass.

## Learn Local Stations

**DX Scanner > Learn Local Stations** opens DX mode on the Scanner page and walks the whole band once, without stopping. Every PI it hears is marked as caught, so the scanner never stops on your local stations as new ones. It adds nothing to the catches or the station log. Run it once at home, with RDS on.

## DX Scanner settings

All are in the menu's **DX Scanner** group.

| Row | Values | At the start | What it does |
|---|---|---|---|
| Start Scan | | | Opens DX mode and starts a scan |
| Scan Dwell | 0.5 to 30.0 s, in steps of 0.5 s | 2.5 s | How long to wait on each channel for a PI |
| Stop Condition | New Stations Only, Any PI, Never | New Stations Only | When the scanner stops. With Never, a new station still holds the dwell until its name arrives |
| Scan Range | Band + Presets, Whole Band, Presets Only | Band + Presets | What to scan. **Band + Presets** walks the band and skips the channels stored as presets, since you know those. **Presets Only** walks the presets from Preset Start to Preset End |
| Preset Range | Preset Start and Preset End, 1 to 99 | 1 to 99 | The presets that Presets Only scans and Watch Presets watches |
| Scan Bandwidth | 56 to 311 kHz, in the tuner's 16 widths | 114 kHz | The filter width in DX mode |
| Loop Band | On, Off | Off | Start again at the end of the range |
| Mute During Scan | On, Off | On | No sound while the scanner runs |
| Auto-Log Stations | On, Off | On | Log new catches by themselves |
| Log Radio Text | On, Off | On | Add the radio text to log entries |
| Watch Presets | On, Off | On | Check the presets in the background |
| Learn Local Stations | | | Mark the stations you can hear now as caught |

Scan Dwell, Loop Band and Stop Condition reach a scan that is running. Scan Range and Preset Range apply at the next scan. Scan Bandwidth applies the next time DX mode opens.

## For FMLIST

`GET /api/dx.csv` gives the catches as a CSV file in the TEF logbook format, which the FMLIST converter takes as it is:

```bash
curl -s -OJ http://tef668x.local:8080/api/dx.csv
```

The header is `Date,Time,Frequency,PI,Signal,Stereo,TA,TP,PTY,ECC,PS,Radiotext`. The date and time are in UTC. It needs no PIN.

The web page's **DX** page shows the same catches, a chart of the last sweep, and the RDS of the station you are on.
