# Display and Themes

The colours, the brightness and dimming, the screen rotation, the battery mark and the language of the screen. All of these are in the menu's **Display** group.

## Themes

![The radio screen in each of the fifteen themes](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/themes.png)

There are 15 themes and a custom one. Every text and mark in the 15 themes has a contrast of at least 4.5:1 against its background, checked by a test in the code.

The radio uses two of them: a **Day Theme** from 06:00 to 17:59 and a **Night Theme** from 18:00 to 05:59, by the network clock. Until the radio has the time, it uses the day theme. Set them at **Display > Theme**. A theme takes effect when you press to save it, not as you turn through the list. Each theme in the list shows four colour samples.

| Theme | Made for |
|---|---|
| Nightwatch | Indoors, in the evening. The night theme at the start |
| Slate | Long evenings, on a grey ground |
| Daylight | Direct sun |
| Paper | Hard sun, black on white |
| LCD | Shade and bright rooms |
| Red Night | A dark sky. Only red and orange, to keep your eyes used to the dark |
| Ember | The bedside |
| Clear | Colours that people with colour blindness can tell apart |
| Clear Day | Clear, for outdoors. The day theme at the start |
| High Contrast | Low vision, with a contrast of 7:1 |
| Mono | No colour |
| Phosphor | Green, with high contrast |
| Hi-Fi, Violet, Blossom | Their look |
| Custom | Your own colours |

When the hour moves from day to night or back, the screen changes to the other theme.

### The custom theme

Choose **Custom** in the menu like any other theme. Its colours are set on the web page: the **Display** group of the [Settings](Web-Page.md#settings) screen, under **The Custom theme's colours**, with eight colour pickers: Background, Tile and row, Radio, Broadcast, Measurement, Good, Fault and Dead. A change shows on the radio at once while Custom is in use.

The contrast test does not cover the custom theme, so check that your colours can be read.

## Brightness and dimming

| Row | Values | At the start | What it does |
|---|---|---|---|
| Brightness | 5 to 100 %, in steps of 5 | 100 % | The screen light in use |
| Dim Level | 0 to 100 %, in steps of 5 | 20 % | The screen light after Dim After |
| Dim After | Never, or 5 to 240 seconds, in steps of 5 | Never | How long with no input before the screen dims |

Brightness changes as you turn the knob, so you can see the result. The lowest Brightness is 5 % because that is the lowest that can still be read in daylight.

The screen light fades up over 750 ms when the radio starts, and dims over 750 ms. It does not dim when Dim Level is at or above Brightness. Any input brings full brightness back at once. The first key press or knob turn on a dimmed screen only wakes it, and does nothing else. The volume knob wakes it and also changes the volume.

## Display Rotation

**Display Rotation** is **Normal** or **Upside Down**, Normal at the start. It turns the screen when you press to save it, with no restart. It is the same setting as **Rotate Display** on the [recovery screen](Recovery-Screen.md), which you open by holding the tuning knob down at power on.

## Battery

**Battery** chooses how the battery shows in the header of the radio screen:

| Value | What shows |
|---|---|
| Off, at the start | Nothing |
| Percent | A battery shape, filled to the level, with no number |
| Volts | The same battery shape, filled to the level, and the voltage after it, such as `3.9` |

The mark turns to the theme's fault colour, red in most themes, at 20 % or less. The [web page](Web-Page.md#the-frame)'s header follows this setting too: it shows the per cent or the volts, and no battery with Battery Off. The percentage is the voltage mapped from 3.0 V (empty) to 4.2 V (full), not a measure of the charge left.

The battery is read once, at start up, because the ESP32's Wi-Fi uses the same part of the chip that reads it. While Wi-Fi is off it is read every second. **Menu > Diagnostics > Battery Voltage** shows the reading, with `at start` after it when it is the reading from start up.

## Battery Check

**Battery Check**, Off or On, Off at first, reads the battery again while Wi-Fi is on. Every 5 minutes, when nobody is using the radio, it stops Wi-Fi for a moment, reads the battery, and starts Wi-Fi again. Nobody is using it when no key, knob or touch came in the last 5 minutes, or the screen is dimmed to nothing; and no web page is open, no web request came in the last minute, no PC is on the PC Link and no phone is on the radio's hotspot. It waits while a seek, a station scan, a DX scan, a sweep or an update runs, and while a new firmware is on trial. The radio is off the network for the few seconds it takes to join again, or to start its hotspot when that is how it runs. The reading shows in the header at once, and **Battery Voltage** shows it with its age, such as `3.85 V, 4 min ago`.

## Language

The screen can show its texts in these languages:

| Language | As the list shows it |
|---|---|
| English | English |
| Chinese, simplified | 简体中文 |
| German | Deutsch |
| Russian | Русский |
| Dutch | Nederlands |
| French | Français |
| Spanish | Español |
| Italian | Italiano |
| Portuguese | Português |
| Polish | Polski |
| Czech | Čeština |
| Croatian | Hrvatski |
| Serbian, in Latin letters | Srpski |
| Romanian | Română |
| Hungarian | Magyar |

Pick one at **Display > Language**. Each language is listed by its own name, in its own script, the same in every language. The picker says **Applies after restart** on its cursor row: the screen changes the next time the radio starts. Restart it with **System > Restart Radio**, or turn it off and on.

In every language the row's own name ends with the English word, such as `Sprache · Language` or `语言 · Language`, so you can find it on a screen you cannot read. It is the last row of the **Display** group.

These stay English in every language:

- The boot screen and the [recovery screen](Recovery-Screen.md). They are drawn before the radio puts its language in force.
- The [web page](Web-Page.md), the [HTTP API](HTTP-API.md), the PC Link and the CSV files of the presets and the station log. The web page and the CSV import read some of these words back as values.
- The theme names, the RDS programme language on RDS page 1 (`LANG`), the band names (FM, MW, SW, LW, OIRT), the units (MHz, kHz, dBµV, dB, %, s, min) and the clock (`17:08`).

Numbers keep the point in every language, as in `98.30 MHz` and `3.95 V`.

To get back to English, pick **English** in the same row and restart. From a computer, send `plg=0` to [`POST /api/settings`](HTTP-API.md#settings) and restart. **Erase Settings** on the recovery screen also brings English back, with every other setting at its first value.

The translations were made for this firmware, using the words radios in each country use. To correct a word, open an issue on GitHub with the language, the screen, the text you see and the text it should be.

