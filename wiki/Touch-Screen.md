# Touch Screen

The ATS-125's screen has a resistive touch panel, read by an XPT2046 controller. The radio screen, the frequency keypad, the menu's lists, its lists of choices and its values with a bar, the bandwidth page, the RDS screen and DX mode's four pages can be worked by touch, below. The other parts, the Web PIN, the Restart Radio question and the update offer, are still worked with the knobs, the buttons and the keypad, or from the web page. A touch does what a knob or a key does there, with two more: the V tile mutes, and a tap on the frequency opens the frequency keypad.

## The radio screen

![The parts of the radio screen a touch acts on](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/touch-radio.png)

| Where | Touch | Does |
|---|---|---|
| The band name, the left half of the top line | Tap | The next band, as a tap of BAND |
| The right half of the top line, with the menu symbol | Tap | Opens the menu, as a press of the tuning knob |
| The station name, the upper part of the frequency panel | Tap | Opens the RDS screen, as BAND held. On FM only; on AM it does nothing |
| The frequency, the lower part of the frequency panel, from the top of its digits | Tap | Opens the [frequency keypad](#the-frequency-keypad) |
| Either part of the frequency panel | Hold | Logs the station, as the tuning knob held |
| The scale | Drag | Tunes as the scale moves with your finger: one mark for every 8 pixels, 100 kHz on FM and 10 kHz on AM, onto the nearest channel of the band, and never onto another band where two bands overlap. Drag to the left to go up, as when you slide a dial strip under a fixed pointer. Past the end of the band it goes round to the other end |
| The tuning mode tile | Tap | The next tuning mode, as a tap of MODE |
| The SQ tile | Tap | Opens **Squelch Mode** straight away. Keep a choice with the knob, or go back, and you are on the radio screen again |
| The BW tile | Tap | Opens the bandwidth page, as BW held |
| The V tile | Tap | Mutes the sound, and a second tap unmutes it. The tile reads MUTE while muted |

The line of radio text under the frequency panel is too thin to be a target, and does nothing.

## The frequency keypad

![The frequency keypad, and the parts a touch acts on](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/touch-keypad.png)

A tap on the frequency opens the keypad, to type a frequency by touch. The panel at the top shows the digits typed so far and a dash for the next one, the way the radio screen shows a number typed on the keys. The top line shows the band and its unit, and the clock.

| Where | Touch | Does |
|---|---|---|
| A digit | Tap | Types the digit, as its key on the radio |
| The backspace key, at the left of the bottom row | Tap | Takes back the last digit |
| OK | Tap | Closes the keypad and tunes the number, as ENTER |
| Cancel, or the top line | Tap | Drops the number and closes the keypad |

The number is read as in [Tuning](Tuning.md#typing-a-frequency): no decimal point, the band you are on first, and a list to pick from when it fits more than one other band.

The radio's own keys work on the keypad too. The digits type. ENTER and a press of the tuning knob are OK. MODE and DX are Cancel, and MODE held drops the number and opens the menu. A turn of the knob, BAND and BW do nothing.

With no key for **Controls > Keypad Timeout**, 20 seconds on a new radio, the keypad closes and the number is dropped. The keypad does not open while the menu, DX mode, the RDS screen or the bandwidth page is up.

## The menu

![The parts of the menu a touch acts on](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/touch-menu.png)

On the list of groups, on the rows of a group, and on a list of choices, such as Squelch Mode, Display Rotation or the themes:

| Where | Touch | Does |
|---|---|---|
| A row | Tap | The same as turning the tuning knob to that row and pressing it: a group opens, a value opens to be changed, a Go To row does what it says, and on a list of choices the choice is kept. A row that only shows something does nothing |
| The top line, with the title | Tap | Back, as a tap of MODE: from a list of choices to its group with the old choice kept, from a group to the list of groups, and from the list of groups out of the menu |
| The list | Swipe up or down | The next or the previous six rows. The list stops at its first and last row |

A touch belongs to the list it started on: if the list changes before the finger lifts, the touch does nothing.

On a value with a bar, such as Brightness or Squelch Floor:

| Where | Touch | Does |
|---|---|---|
| The bar, with its limits under it | Tap, drag or hold | The value moves to where the finger is, at once, as the tuning knob would turn it there. A finger held still for 1.5 s sets the value under it, and must lift before it can move it again. A value with many steps, such as Squelch Floor or Network Time, is hard to hit exactly by finger: get near, then turn the knob for the last step |
| The value panel, above the bar | Tap | Keeps the value, as a press of the tuning knob |
| The top line, with the title | Tap | Back: the old value comes back |

## The bandwidth page

![The parts of the bandwidth page a touch acts on](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/touch-bw.png)

| Where | Touch | Does |
|---|---|---|
| A width | Tap | Uses that width, as turning the tuning knob to it and pressing. The page stays up, so widths can be compared by ear |
| iMS or EQ, on FM | Tap | Turns the switch on or off |
| The top line, with the title | Tap | Closes the page, as a tap of MODE. Over DX mode, back to the DX page |

Over DX mode the page sets DX mode's own width and has no AUTO.

## The RDS screen

![The parts of the RDS screen a touch acts on](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/touch-rds.png)

| Where | Touch | Does |
|---|---|---|
| Anywhere | Swipe left or right | The next or the previous page, going round from the last to the first, as turning the tuning knob |
| The right half of the top line, with the page position | Tap | The next page |
| The left half of the top line, with the title | Tap | Closes the screen, as a tap of MODE. Over DX mode, back to the DX page |

## DX mode

![The parts of the DX page a touch acts on](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/touch-dx.png)

On every DX page:

| Where | Touch | Does |
|---|---|---|
| Anywhere but the Scope page's chart | Swipe left or right | The next or the previous page, going round |
| The right half of the top line, with the page position | Tap | The next page, as a tap of BAND |
| The left half of the top line, with the title | Tap | Leaves DX mode, as a tap of MODE |

On the DX page also:

| Where | Touch | Does |
|---|---|---|
| The station panel or the PI tile | Tap | Opens the RDS screen over DX mode, as a press of the tuning knob |
| The readings under them | Tap | Opens the bandwidth page with DX mode's widths, as BW held |

On the Scope page also:

![The Scope page with Touch On, and the parts a touch acts on](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/touch-scope.png)

With Touch On, the foot row of the Scope page is five blocks: a button to move the cursor left, the cursor's frequency, a Sweep button, the cursor's level, and a button to move the cursor right. The rise moves up into the chart's top line. With Touch Off the page is as before, one tile with all three readings.

| Where | Touch | Does |
|---|---|---|
| The chart and the strip under it | Tap or drag | Moves the cursor to the channel under the finger. The chart has about 1.4 pixels a channel, so use the arrow buttons or the knob for the last channel |
| The chart and the strip under it | Hold | Moves the cursor there and tunes to it, as the tuning knob held |
| The left or right arrow button | Tap | Moves the cursor one channel, as a click of the tuning knob |
| The frequency or the level tile | Tap | Tunes to the cursor's channel |
| The Sweep button in the middle | Tap | Sweeps the band, as a press of the tuning knob. A sweep takes about 4 seconds; the screen is not read while it runs, so the button is dimmed then, and the knob's press stops a sweep |

On the Scanner page also: a tap on the scan panel starts a scan, or goes on with one that was stopped, as a press of the tuning knob.

On the Catches page also:

| Where | Touch | Does |
|---|---|---|
| A catch | Tap | Tunes to it, as turning the tuning knob to it and pressing |
| A catch | Hold | Logs it, as the tuning knob held |
| The list | Swipe up or down | The next or the previous six catches |

While a DX scan runs, any touch only stops it, as any key does. In its first seconds, while it sweeps the band, the screen is not read, so only a key stops it then.

## What any touch does

- A touch stops a running DX scan and does nothing else, as any key does.
- Outside the frequency keypad, a touch clears a frequency you were typing on the radio's keys and does nothing else.
- A hold is a finger kept still for 1.5 seconds. With **Key Beeps** at Short & Long Press, a hold beeps long. At Every Press, a hold beeps long and a tap or a swipe beeps short. A tap on the frequency keypad is a key: it beeps from Keypad Only up.
- While the screen is dark, and while the start-up, going to sleep or update screens show, the radio does not read the touch panel, and a touch does not wake the screen or end those screens. Use the knob, a key or the volume knob. This keeps a touch in a pocket or a bag from doing anything. A finger already on the screen when it lights up does nothing until it is lifted.
- During a DX level sweep, about 4 seconds, the touch panel is not read either, so its readings stay clean. The calibration screen is the one exception to both.
- A finger or a stuck panel held still does one thing at most, its hold after 1.5 seconds, and then nothing more until it lifts.

## Turn touch on or off

**Controls > Touch** in the menu, On or Off. It is On on a new radio. Off, the radio does not read the touch panel at all. Use it for a panel that touches itself, for example a cracked one.

When the menu cannot be used, the [recovery screen](Recovery-Screen.md) has the same switch as its **Touch** row.

## Calibrate the touch screen

Each panel sits a little differently in its radio. A calibration teaches the radio where your panel's readings fall on the screen. Until you make one, the radio uses a map taken from one ATS-125, which can be some pixels off on yours.

![A ring being held during calibration](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/touch-cal-mark2.png)

1. Open **Controls > Calibrate Touch** in the menu.
2. A ring shows near one corner. Put your finger or a pen in the ring and hold still. The ring fills, turns into a tick, and the next ring shows. Lift before you hold the next one.
3. Do the same for all five rings: the four corners, then the middle. If you slide away or lift before a ring is full, it starts again.
4. A dot shows. Tap it once.
5. The screen says **Calibration kept** and how far the tap landed from the dot, or **Not kept** and why: the rings did not make a calibration, the tap landed more than 24 pixels across or down from the dot, or it could not be saved. A calibration that is not kept leaves the old one in use.

While the rings or the dot show, turning or pressing the tuning knob, or any other button or key, leaves the calibration, and the old calibration stays. On **Not kept**, press the knob to try again, or turn it to leave. On **Calibration kept**, press the knob to go back.

A kept calibration is used at once and is kept through restarts and updates. **Erase Settings** on the recovery screen removes it with the other settings, and the radio goes back to the built-in map.

The recovery screen has a **Calibrate Touch** row too, worked the same way with the knob, for when the menu cannot be reached.

If **Display Rotation** is changed from the web page while the rings or the dot show, the calibration starts again from the first ring.

The calibration screen reads the touch panel even with **Controls > Touch** set to Off.

## See what the panel reads

A gesture can also be sent from a computer with `POST /api/touch`, see the [HTTP API](HTTP-API.md#touch).

`GET /api/state` shows the touch panel as `tch` inside `inp`: whether something is on the screen, the raw readings, which calibration is in use, and where the last touch landed in screen pixels. The [HTTP API](HTTP-API.md#get-apistate) page lists every field. To check a calibration, press a spot you know, such as the middle of a tile, and read `px` and `py`.
