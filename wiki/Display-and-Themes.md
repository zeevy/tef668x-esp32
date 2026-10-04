# Display and Themes

The colours, the brightness and dimming, the screen rotation, the battery mark and the level offset. All of these are in the menu's **Display** group.

## Themes

![The radio screen in each of the fifteen themes](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/themes.png)

There are 15 themes and a custom one. Every text and mark in the 15 themes has a contrast of at least 4.5:1 against its background, checked by a test in the code.

The radio uses two of them: a **Day Theme** from 06:00 to 17:59 and a **Night Theme** from 18:00 to 05:59, by the network clock. Until the radio has the time, it uses the day theme. Set them at **Display > Theme**. A theme takes effect when you press to keep it, not as you turn through the list. Each theme in the list shows four colour samples.

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

Choose **Custom** in the menu like any other theme. Its colours are set on the web page: **Settings > Display > Theme**, in **Custom's colours**, with eight colour pickers: Background, Tile and row, Radio, Broadcast, Measurement, Good, Fault and Dead. A change shows on the radio at once while Custom is in use.

The contrast test does not cover the custom theme, so check that your colours can be read.

## Brightness and dimming

| Row | Values | At the start | What it does |
|---|---|---|---|
| Brightness | 5 to 100 %, in steps of 5 | 100 % | The screen light in use |
| Dim Level | 0 to 100 %, in steps of 5 | 20 % | The screen light after Dim After |
| Dim After | Never, or 5 to 240 seconds, in steps of 5 | Never | How long with no input before the screen dims |

Brightness changes as you turn the knob, so you can see the result. The lowest Brightness is 5 % because that is the lowest that can still be read in daylight.

The screen dims over 750 ms. It does not dim when Dim Level is at or above Brightness. Any input brings full brightness back at once. The first key press or knob turn on a dimmed screen only wakes it, and does nothing else. The volume knob wakes it and also changes the volume.

## Display Rotation

**Display Rotation** is **Normal** or **Upside Down**, Normal at the start. It turns the screen when you press to keep it, with no restart. It is the same setting as **Rotate Display** on the [recovery screen](Recovery-Screen.md), which you open by holding the tuning knob down at power on.

## Battery

**Battery** chooses how the battery shows in the header of the radio screen:

| Value | What shows |
|---|---|
| Off, at the start | Nothing |
| Percent | A battery shape, filled to the level, with no number |
| Volts | A small battery and the voltage, such as `3.9` |

The mark turns to the theme's fault colour, red in most themes, at 20 % or less. The percentage is the voltage mapped from 3.0 V (empty) to 4.2 V (full), not a measure of the charge left.

The battery is read once, at start up, because the ESP32's Wi-Fi uses the same part of the chip that reads it. While Wi-Fi is off it is read every second. **Menu > Diagnostics > Battery Voltage** shows the reading, with `at start` after it when it is the reading from start up.

## Level Offset

**Display > Level Offset** has two rows, **FM Level Offset** and **AM Level Offset**: -25 to +15 dB in steps of 1, 0 dB at the start. The offset is added to every signal level the radio shows on its screen, the web page, and the station log and DX exports, so the readings can match another receiver's. In the HTTP API, `/api/state` and `/api/dx` send the level as read, with the offset in a separate field, `lvo`. The station log exports include the offset. The FM value is for FM and OIRT, the AM value for LW, MW and SW. Seek, squelch and the other limits do not use it.

## Startup Fade

**Startup Fade**, On at the start, fades the screen light up over 750 ms when the radio starts. Off makes it come on at once. It takes effect after a restart.
