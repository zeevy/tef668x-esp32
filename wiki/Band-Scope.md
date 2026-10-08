# Band Scope

![The band scope over the whole FM band](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/band-scope.png)

The band scope draws the level of every channel of the FM band, so you can see at a glance where the stations are. It is the Scope page of [DX mode](DX-Mode.md#scope-page) on its own: you do not have to open DX mode to use it. FM only for now.

## Open it

- Tap the tuning scale on the radio screen, with Touch On.
- Or open the menu and pick **Go To > Band Scope**.

On AM it does not open, and shows `Switch to FM first`.

It sweeps as soon as it opens. A sweep reads every channel at the band's default step, muted, through DX mode's filter width, **Scan Bandwidth** (114 kHz at the start), so a level here means the same as on DX mode's Scope page. The whole FM band takes about 4 seconds, and the span under a second. While it sweeps, the header says `sweeping`. After that it says how old the sweep is: `now`, `3 min ago` and so on.

## What it shows

| Part | What it shows |
|---|---|
| The bars | The level of each channel in the last sweep |
| White ticks along the top | Your presets on this band |
| Blue squares along the top | Stations caught in DX mode, from the [Catches page](DX-Mode.md#catches-page) |
| Dashed line | The floor: the level that a quarter of the channels are below. The whole band only |
| Dial mark | Where the dial is: a short bar at the foot of the chart and of the strip |
| Cursor | The thin line, on the channel the foot shows |
| Foot | The cursor's frequency and level |
| Header | `FM Scope`, how old the sweep is, `Full` or `3.6 MHz`, and the clock |

A peak with no tick and no square is a station you have not stored and have not caught.

The strip under the chart is DX mode's rise over a baseline. The band scope keeps no baseline, so the strip is a flat line here.

## The whole band or the span round the dial

![The band scope over 3.6 MHz round the dial](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/band-scope-span.png)

A tap of BAND switches between the whole band (`Full`) and 3.6 MHz round the dial (`3.6 MHz`), the same width the tuning scale shows, and sweeps again. The span has wide bars, so one channel is easy to pick, and it sweeps in under a second. Near the end of the band the span moves inside the band. The span has no floor line, since a span round a strong station gives a wrong floor.

## Controls

| Control | Tap or turn | Hold |
|---|---|---|
| Tuning knob, turn | Moves the cursor one channel | |
| Tuning knob, press | Sweeps again, or stops the sweep that is running | Tunes to the cursor's channel |
| ENTER | The same as the knob press | The same as the knob hold |
| BAND | The whole band or the span, and sweeps again | Nothing |
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
| The right half of the top line, with `Full` or `3.6 MHz` | Tap | The whole band or the span, as a tap of BAND |

It stays open after a tune, so you can step from station to station. It closes when you open the menu or another screen, and when the band leaves FM. A sweep that is running stops when it closes. The check for new firmware waits while it is open.

From a PC, `POST /api/scope` makes the same sweep, see [HTTP API](HTTP-API.md#band-scope).
