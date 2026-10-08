# Band Scope

![The band scope over the whole FM band](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/band-scope.png)

The band scope draws the level of every channel of the band you are on, FM, OIRT, MW, LW or SW, so you can see at a glance where the stations are. It is the Scope page of [DX mode](DX-Mode.md#scope-page) on its own, on every band: you do not have to open DX mode to use it.

## Open it

- Tap the tuning scale on the radio screen, with Touch On.
- Or open the menu and pick **Go To > Band Scope**.

It sweeps as soon as it opens. A sweep reads every channel at the band's default step, muted. While it sweeps, the header says `sweeping`. After that it says how old the sweep is: `now`, `3 min ago` and so on.

| Band | Read through | Whole band | Span round the dial |
|---|---|---|---|
| FM and OIRT | DX mode's filter width, **Scan Bandwidth** (114 kHz at the start), so a level here means the same as on DX mode's Scope page | About 4 seconds for FM | 3.6 MHz, under a second |
| MW and LW | Your own AM filter width | About 7.5 seconds for MW, 2 for LW | 360 kHz, about 2 seconds on MW |
| SW | Your own AM filter width | Too many channels for one sweep, so SW shows the span only | 360 kHz, about 4 seconds |

On AM each channel is read 40 ms after the tune. Measured on this radio, an AM reading taken sooner is too high on average, and 1 reading in 20 is off by 6 to 9 dB; a channel just above a strong station reads low at first. From 40 ms a reading is as good as one taken much later, at every AM width. On FM 5 ms is enough.

The page closes when the band changes.

## What it shows

| Part | What it shows |
|---|---|
| The bars | The level of each channel in the last sweep |
| White ticks along the top | Your presets on this band |
| Blue squares along the top | Stations caught in DX mode, from the [Catches page](DX-Mode.md#catches-page). FM only, since DX mode is FM |
| Dashed line | The floor: the level that a quarter of the channels are below. The whole band only |
| Dial mark | Where the dial is: a short bar at the foot of the chart and of the strip |
| Cursor | The thin line, on the channel the foot shows |
| Foot | The cursor's frequency, in MHz on FM and kHz on AM, and its level |
| Header | The band, for example `FM Scope` or `MW Scope`, how old the sweep is, `Full` or the span, and the clock |

A peak with no tick and no square is a station you have not stored and have not caught.

The strip under the chart is DX mode's rise over a baseline. The band scope keeps no baseline, so the strip is a flat line here.

## The whole band or the span round the dial

![The band scope over 3.6 MHz round the dial](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/band-scope-span.png)

A tap of BAND switches between the whole band (`Full`) and the span round the dial, the same width the tuning scale shows: `3.6 MHz` on FM and `360 kHz` on AM. It sweeps again. The span has wide bars, so one channel is easy to pick, and it sweeps quicker. Near the end of the band the span moves inside the band. The span has no floor line, since a span round a strong station gives a wrong floor. On SW, BAND sweeps the span again.

## Medium wave

![The band scope on medium wave](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/band-scope-mw.png)

The same page on MW, at night indoors, with one station heard, 738 kHz, stored as a preset. The hump in the middle is noise, not stations: only 738 could be heard. The band scope is a quick way to see such noise.

## Controls

| Control | Tap or turn | Hold |
|---|---|---|
| Tuning knob, turn | Moves the cursor one channel | |
| Tuning knob, press | Sweeps again, or stops the sweep that is running | Tunes to the cursor's channel |
| ENTER | The same as the knob press | The same as the knob hold |
| BAND | The whole band or the span, and sweeps again. On SW, the span again | Nothing |
| BW | Nothing | Nothing |
| MODE | Closes the band scope | The menu |
| Keypad | Nothing | |
| Volume knob | The volume | |

By touch, with Touch On:

| Where | Touch | Does |
|---|---|---|
| The chart and the strip under it | Tap or drag | Moves the cursor to the channel under the finger |
| The chart and the strip under it | Hold | Moves the cursor there and tunes to it, as the tuning knob held |
| The left or right arrow button | Tap | Moves the cursor one channel |
| The frequency or the level tile | Tap | Tunes to the cursor's channel |
| The Sweep button | Tap | Sweeps again. The screen is not read while a sweep runs, so the knob's press stops one |
| The left half of the top line, with the title | Tap | Closes the band scope, as a tap of MODE |
| The right half of the top line, with `Full` or the span | Tap | The whole band or the span, as a tap of BAND |

It stays open after a tune, so you can step from station to station. It closes when you open the menu or another screen, and when the band leaves FM. A sweep that is running stops when it closes. The check for new firmware waits while it is open.

From a PC, `POST /api/scope` makes the same sweep, see [HTTP API](HTTP-API.md#band-scope).
