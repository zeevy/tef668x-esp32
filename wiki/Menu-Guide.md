# Menu Guide

Every group and every row of the radio's menu, with its values, its value on a new radio, and what it does.

![The menu's group list](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/menu-groups.png)

## Moving in the menu

Open the menu with a press of the tuning knob on the radio screen, or hold MODE on any screen.

| Control | What it does |
|---|---|
| Turn the tuning knob | Moves one row. Lists stop at the ends |
| Press the knob, or ENTER | Opens a group or sub-group, starts changing a value, or runs an action |
| Hold the knob, hold ENTER, or tap MODE | Goes back one level |
| Hold MODE | Closes the menu |

The group list shows how many rows each group has. A row that opens a sub-group shows its row count and an arrow. Inside a sub-group, the title shows the path, for example `Audio > Squelch`. The menu opens again where you left it, until the radio restarts. With no input for one minute it closes, except while a station scan runs.

### Changing a value

Press a row to change it. Depending on the row you get a bar with its two ends, a list with a tick on the saved choice, the Web PIN's digit editor, or No and Yes.

- Most rows act at once as you turn, so you hear or see the result. Some act only when you press to keep the value: the themes, Display Rotation, Wi-Fi, Hotspot, Web Server and Web PIN.
- Press to keep the value. Hold, or tap MODE, to put the old value back. A bar row whose value you moved then shows `Not Saved`.
- Rows marked **after restart** below show `Applies after restart`, and take effect the next time the radio starts.

### Notes

A short note shows when something could not be done: in a list, on the row in place of its value, and on a value screen, on its bottom line. It clears on the next turn or press.

| Note | When |
|---|---|
| `Radio busy - try again` | The radio could not do it at that moment, for example a scan was refused or a row could not be read |
| `Not on the band plan` | A preset or log entry whose frequency is not on its band under the band plan in use |
| `Switch to FM first` | DX Mode, Start Scan or Learn Local Stations on AM |
| `Turn on RDS first` | Learn Local Stations with the RDS Decoder off |
| `Update on trial - wait` | Restart Radio while a new firmware is on trial, or Sleep while one is written or on trial |
| `No update found` | Firmware Update pressed while no newer release is known |
| `Not saved - restored` | The setting could not be saved, so the old value came back |
| `Radio busy - not restored` | A cancel could not reach the tuner, so the tuner kept the new value |

## Go To

Each row does one thing and closes the menu. When it cannot, for example DX Mode on AM, the menu stays open and shows a note.

| Row | What it does |
|---|---|
| Next Band | The next band, as a tap of BAND |
| Bandwidth | Opens the bandwidth page, as a hold of BW |
| RDS | Opens the RDS pages, as a hold of BAND |
| DX Mode | Opens DX mode. FM and OIRT only |
| Log Current Station | Writes the station to the station log |
| Sleep | The radio fades and sleeps until the knob is pressed. See [Sleep and Auto Off](Sleep-and-Auto-Off.md) |

## Stations

See [Presets and Station Scan](Presets-and-Station-Scan.md) and [Station Log](Station-Log.md).

| Row | What it does |
|---|---|
| Presets | A list of the stored presets, such as `P03 NEWS` and the frequency. Press one to tune it. The menu stays open |
| FM Station Scan | Walks the FM band and saves each station found to an empty preset, except stations already stored. Press again to stop |
| AM Station Scan | The same, for medium wave |
| SW Station Scan | The same, for the shortwave broadcast bands |
| LW Station Scan | The same, for long wave |
| Station Log | The station log, newest first. Press an entry to tune it. The menu stays open |

## Audio

See [Sound and Bandwidth](Sound-and-Bandwidth.md).

| Row | Values | New radio | What it does |
|---|---|---|---|
| Squelch > Squelch Mode | Off, Auto, Manual | Off | Off: sound always on. Auto: the radio judges the signal. Manual: the volume knob sets the squelch level |
| Squelch > Squelch Level | Read only | | The Manual level the volume knob last set, in dBuV. `Off` until the knob has set one |
| Squelch > Squelch Floor | Off, 1 to 40 dBuV | 15 dBuV | The level an FM signal must reach for Auto squelch to open |
| Volume AGC | Off, 30 to 80 % in steps of 2 | Off | Brings stations to a similar loudness. A lower target is more even and quieter |
| AGC Boost | Cut Only, 1 to 8 dB | Cut Only | The most the AGC may raise a quiet station |
| Mute Ramp | 0 to 500 ms in steps of 10 | 120 ms | How long the sound fades before a mute, a squelch close or a width change. 0 cuts at once |

## FM Reception

| Row | Values | New radio | What it does |
|---|---|---|---|
| Band Plan | Full, Japan, Wide, 87-108 MHz, Worldwide | Worldwide | The FM range. See [Tuning](Tuning.md#bands). **After restart** |
| Tuning Step | 50, 100, 200 kHz | 100 kHz | The FM step. It can be changed only while the radio is on FM |
| Seek Sensitivity | 1 to 6 | 4 | How weak a station seek and the station scan stop on. Higher stops on weaker stations |
| De-emphasis | Off, 50 us, 75 us | 50 us | 75 us in the Americas and South Korea, 50 us elsewhere |
| Stereo > Force Mono | Off, On | Off | Every station in mono |
| Stereo > Stereo Blend | Off, 20 to 60 dBuV | Off | Below this level, blends towards mono |
| Stereo > Stereo / High Blend | Off, 20 to 60 dBuV | Off | Below this level, blends to mono and cuts the treble |
| High Cut | Off, 20 to 60 dBuV | Off | Below this level, cuts the treble |
| Noise Blanker | Off, 50 to 150 % in steps of 5 | Off | Blanks short bursts of noise |
| iMS | Off, On | Off | Multipath suppression |
| Equalizer | Off, On | Off | The channel equaliser |
| RDS > RDS Decoder | Off, On | On | Reads RDS |
| RDS > RDS Region | Europe, North America | Europe | North America uses RBDS programme types and call letters. See [RDS](RDS.md#settings) |

Force Mono, the blends, High Cut, iMS and Equalizer are taken only while the radio is on FM or OIRT. Set them there.

## AM Reception

Every row here can be set from any band.

| Row | Values | New radio | What it does |
|---|---|---|---|
| MW Step | 9 kHz, 10 kHz | 9 kHz | The medium wave spacing: 522 to 1791 kHz at 9 kHz, 520 to 1720 kHz at 10 kHz. **After restart** |
| Tuning Step > LW | 1, 9 kHz | 9 kHz | The long wave step |
| Tuning Step > MW | 1, 9 kHz, or 1, 10 kHz with MW Step 10 kHz | 9 kHz | The medium wave step |
| Tuning Step > SW | 1, 5 kHz | 5 kHz | The shortwave step |
| Seek Sensitivity | 1 to 6 | 4 | As on FM, for the AM bands |
| Filter Bandwidth > LW, MW, SW | 3, 4, 6, 8 kHz | 4 kHz | The filter width of each band |
| Noise Blanker | Off, 50 to 150 % in steps of 5 | 100 % | Blanks short bursts of noise |
| High Cut > MW / SW | Off, 20 to 60 dBuV | 47 dBuV | Below this level, cuts the treble |
| High Cut > LW | Off, 20 to 60 dBuV | 52 dBuV | The same, for long wave |
| Soft Mute > MW / SW | 0 to 50 dBuV | 28 dBuV | Below this level, turns the sound down |
| Soft Mute > LW | 0 to 50 dBuV | 34 dBuV | The same, for long wave |

## DX Scanner

See [DX Mode](DX-Mode.md).

| Row | Values | New radio | What it does |
|---|---|---|---|
| Start Scan | | | Opens DX mode and starts a scan. FM and OIRT only |
| Scan Dwell | 0.5 to 30.0 s in steps of 0.5 | 2.5 s | How long to wait on each channel for a PI |
| Stop Condition | New Stations Only, Any PI, Never | New Stations Only | When the scanner stops |
| Scan Range | Band + Presets, Whole Band, Presets Only | Band + Presets | What to scan. Band + Presets walks the band and skips the stored presets. Applies at the next scan |
| Preset Range > Preset Start | 1 to 99 | 1 | The first preset for Presets Only and Watch Presets |
| Preset Range > Preset End | 1 to 99 | 99 | The last one |
| Scan Bandwidth | 56 to 311 kHz, the tuner's 16 widths | 114 kHz | The filter width in DX mode. Applies the next time DX mode opens |
| Loop Band | Off, On | Off | Goes round the band again at the end |
| Mute During Scan | Off, On | On | No sound while the scanner runs |
| Auto-Log Stations | Off, On | On | Writes each new catch to the station log |
| Log Radio Text | Off, On | On | Adds the radio text to each log entry |
| Watch Presets | Off, On | On | Checks the presets in the background in DX mode |
| Learn Local Stations | | | Walks the band once and marks every station heard as caught. Needs FM and RDS |

## Display

See [Display and Themes](Display-and-Themes.md).

| Row | Values | New radio | What it does |
|---|---|---|---|
| Theme > Day Theme | The 15 themes and Custom | Clear Day | The theme from 06:00 to 17:59. Acts when kept |
| Theme > Night Theme | The 15 themes and Custom | Nightwatch | The theme from 18:00 to 05:59. Acts when kept |
| Brightness | 5 to 100 % in steps of 5 | 100 % | The screen light |
| Dim Level | 0 to 100 % in steps of 5 | 20 % | The light after Dim After |
| Dim After | Never, 5 to 240 s in steps of 5 | Never | How long with no input before the screen dims |
| Display Rotation | Normal, Upside Down | Normal | Turns the screen over. Acts when kept, no restart |
| Battery | Off, Percent, Volts | Off | How the battery shows in the header |
| Level Offset > FM Level Offset | -25 to +15 dB | 0 dB | Added to every FM and OIRT level shown |
| Level Offset > AM Level Offset | -25 to +15 dB | 0 dB | Added to every LW, MW and SW level shown |
| Startup Fade | Off, On | On | Fades the screen up at start. **After restart** |

## Connectivity

See [First Start and Wi-Fi](First-Start-and-Wi-Fi.md). All but Network Time act when you press to keep them.

| Row | Values | New radio | What it does |
|---|---|---|---|
| Wi-Fi | Off, On | On | Off stops Wi-Fi and the hotspot, and with them the web page and the API. Only this row, or Erase Settings on the recovery screen, turns it back on |
| Hotspot | Auto, On, Off | Auto | Auto: the radio's own hotspot only when your network cannot be joined or none is stored. On: always, in place of your network. Off: never |
| Web Server | Off, On | On | The web page, the API and updates over Wi-Fi |
| Web PIN | Six digits | 000000 | The access PIN. The row shows the six digits. Turn to set a digit, press for the next one, or type the digits on the keypad. Saved on the sixth digit |
| Network Time | Off, -12:00 to +14:00 in steps of 15 minutes | +00:00 | Sets the clock from the network, at this offset from UTC |
| Network Info | Read only | | Connection Status, Web Address, IP Address, Wi-Fi Network (Hotspot Name on the hotspot), Wi-Fi Signal, MAC Address |

## Controls

| Row | Values | New radio | What it does |
|---|---|---|---|
| Encoder > Encoder Type | Standard, Optical | Standard | The kind of tuning knob fitted. **After restart** |
| Encoder > Encoder Direction | Normal, Reversed | Normal | Which way the tuning knob turns. **After restart** |
| Key Beeps | Off, Keypad Only, Short & Long Press, Every Press | Off | When the radio beeps for a key |
| Band Edge Beep | Off, On | Off | Beeps when the knob steps over a band edge |
| Startup Chime | Off, On | On | A tone at start, once the tuner is ready. **After restart** |
| Keypad Timeout | 5 s to 60 s in steps of 5 s | 20 s | How long a number part typed on the keys or the [frequency keypad](Touch-Screen.md#the-frequency-keypad) waits for the next key. Then the number is dropped and the keypad closes |
| Touch | Off, On | On | Whether the radio reads the touch screen. Off leaves it alone, for a screen that touches itself; only the calibration screen still reads it. The [recovery screen](Recovery-Screen.md) has the same switch |
| Calibrate Touch | | | Opens the touch calibration: five rings to hold, then a dot to tap. See [Touch Screen](Touch-Screen.md#calibrate-the-touch-screen) |

## System

| Row | Values | New radio | What it does |
|---|---|---|---|
| Auto Off | Off, 5 to 600 minutes in steps of 5 | Off | Minutes with no input before the radio sleeps |
| Check for Updates | Off, On | Off | Looks on GitHub for a newer release once the radio is on the network, at every start, and when the menu closes after it is turned on. See [Updating](Updating.md#from-github-on-the-radio) |
| Firmware Update | Press | | While a newer release is known, it is called **Update to** and the version, with the size: press it for the offer, a box with the versions and the size and the buttons **Update** and **Later**. Otherwise its value says why there is nothing to install, `Up to date`, `Not checked`, `Checking`, `Check failed`, or `Off` before any check in this start, and a press shows `No update found` |
| Restart Radio | No, Yes | No | Press, turn to Yes, press again. The radio restarts |

## Diagnostics

All read only.

| Row | What it shows |
|---|---|
| Boot Source | The firmware slot it runs from, `app0` or `app1` |
| Uptime | How long since start, such as `42 min` or `3 h 5 m` |
| Battery Voltage | The battery, such as `3.95 V`, with `at start` when it is the reading from start up |
| Tuner | The tuner chip and patch, such as `TEF6686 patch 102` |
| Reset Reason | Why the radio last started: Power On, Software, Panic, IRQ Watchdog, Task Watchdog, Watchdog, Deep Sleep, Brownout, Reset Pin, Unknown, Restart Asked, Boot Watchdog, Rollback, Display Fault or Update |
| CPU Core 0, CPU Core 1 | How busy each core was over the last second |
| Free Heap, Lowest Heap, Largest Block | Free memory now, the least since start, and the largest free block |
| LVGL Pool | The screen library's memory in use and its size |
| Chip | The ESP32 model and revision |
| Flash Size | The flash size, read from the chip |

## About

| Row | What it shows |
|---|---|
| Firmware Version | The version |
| Build | The git commit it was built from, with `+` when built with changes not yet committed |
| Developer | zeevy |
| License | GPLv3 |
| GitHub | github.com/zeevy/tef668x-esp32 |
