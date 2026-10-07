# Radio Screen

The radio screen is the main screen. It shows the band, the station, the frequency, how the signal is, and four settings at the bottom. [Controls](Controls.md) says what each key does on it.

![The radio screen on FM 106.40 with the station MAGIC](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/fm.png)

The colours named on this page are those of the Nightwatch theme in the pictures. Other [themes](Display-and-Themes.md) use other colours in the same places.

From top to bottom:

1. **Header:** while Touch is On the menu symbol, then the band, and at the right the status marks and the clock.
2. **Frequency panel**, the large filled box: the station name, the frequency, the preset number, the modulation meter and the signal level.
3. **Text line:** the radio text, or the date.
4. **Tuning scale.**
5. **Four tiles:** tuning mode, squelch, filter width and volume.

## 1. Header

| Item | What it shows |
|---|---|
| Menu symbol | Four lines at the top left, only while **Controls > Touch** is On. A tap on it opens the menu, see [Touch Screen](Touch-Screen.md#the-radio-screen) |
| Band | `FM`, `OIRT`, `LW`, `MW` or `SW`. On shortwave inside a broadcast band, the metre band follows in grey, for example `31 m` |
| Wi-Fi mark | Always shown. On your network: green bars, 3 to 0 by the Wi-Fi signal. Joining: a search mark in grey. Serving the hotspot: a hotspot mark in amber. Wi-Fi off: a crossed mark in grey |
| Battery | Hidden on a new radio. Display > Battery set to `Percent` shows a filled shape, and `Volts` shows the voltage. It turns to the theme's fault colour, red in Nightwatch, at 20 % or less |
| Sleep mark | A person in bed, only while Auto Off is on. It turns amber in the last 5 minutes |
| Clock | `HH:MM`, at the far right. It is hidden until the radio has the time from the network |

## 2. Frequency panel

| Item | What it shows |
|---|---|
| Name line | One of these, in this order: a short message for 1.5 seconds (such as `Logged 106.40`), the station's RDS name, the preset's name, or `---` |
| Frequency | FM and OIRT in MHz with two decimals, `106.40`. AM bands in kHz, `738` |
| Preset | `P12` above the unit, when the frequency is a stored preset |
| Unit | `MHz` or `kHz` |
| Modulation meter | 14 bars with a peak mark: how loud the broadcast is modulated, not the signal strength. Hidden while there is no reading |
| Level | The signal level in dBµV, as a whole number. Display > Level Offset is added to it |

While you type a frequency, the digits show in place of the frequency, for example `104-`, and the preset number and unit are hidden.

## 3. Text line

On FM, the station's radio text, in blue, scrolling round. With no radio text, and on AM, it shows the date, for example `WEDNESDAY, 30th September 2026`. Before the radio has the time from the network, the line is empty. For the few seconds of an update check it says `Checking for updates…` in amber instead (see [Updating](Updating.md#from-github-on-the-radio)).

## 4. Tuning scale

The tuned frequency stays in the middle, under the pointer, and the scale moves past it. There is a mark every 100 kHz on FM and every 10 kHz on AM. Every tenth mark is long and has a number. The four marks on each side of the pointer rise with the signal: full height is 60 dBµV on FM and 45 dBµV on AM. Where the two ends of the band meet, the scale shows a dotted join.

## 5. Tiles

| Tile | Values | What it means |
|---|---|---|
| Tuning mode | `MAN`, `AUTO`, `MEM`, `MTR` | What a turn of the tuning knob does. See [Tuning](Tuning.md#tuning-modes) |
| `SQ:` | `OFF`, `AUTO`, or a level such as `15dB` | Squelch off, automatic, or Manual at that level, set with the volume knob |
| `BW:` | `DYN`, or a width such as `84k` or `6k` | The filter width. `DYN` means the tuner picks the width by itself, on FM and OIRT |
| `V:` | `-6dB`, or `MUTE` | The volume. `MUTE` shows in red. The value turns grey while the squelch holds the sound back |

## Other bands and states

| Picture | What it shows |
|---|---|
| ![MW 738 kHz](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/mw.png) | Medium wave, 738 kHz. No RDS, so the name line is `---` and the text line is the date. Muted, so `V:MUTE` is red. Low battery in red, Wi-Fi off |
| ![SW 9420 kHz](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/sw.png) | Shortwave, 9420 kHz in the `31 m` band, on preset P07 with the name `RADIO ROMANIA` |
| ![FM squelched](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/fm-squelched.png) | Manual squelch at 15 dB, a weak 12 dBµV signal, and the volume in grey because the squelch holds the sound back |
| ![FM logged](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/fm-logged.png) | `Logged 106.40` on the name line, just after a station log hold |
| ![FM typing](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/fm-typing.png) | `104-` being typed on the keypad |
| ![OIRT](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/oirt.png) | OIRT 70.30 MHz with RDS, and the hotspot mark in amber |

If the tuner fails at start up, the radio screen is replaced by a screen titled **Tuner** with the fault under it.
