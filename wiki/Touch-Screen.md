# Touch Screen

The ATS-125's screen has a resistive touch panel, read by an XPT2046 controller. The radio screen can be worked by touch, below. The other screens, the menu, the bandwidth page, the RDS screen and DX mode, are still worked with the knobs, the buttons and the keypad, or from the web page. A touch never does anything a knob or a key cannot do.

## The radio screen

![The parts of the radio screen a touch acts on](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/touch-radio.png)

| Where | Touch | Does |
|---|---|---|
| The band name, the left half of the top line | Tap | The next band, as a tap of BAND |
| The right half of the top line | Tap | Opens the menu, as a press of the tuning knob |
| The amber panel | Tap | Opens the RDS screen, as BAND held. On FM only; on AM it does nothing |
| The amber panel | Hold | Logs the station, as the tuning knob held |
| The scale | Drag | Tunes as the scale moves with your finger: one mark for every 8 pixels, 100 kHz on FM and 10 kHz on AM, onto the nearest channel of the band, and never onto another band where two bands overlap. Drag to the left to go up, as when you slide a dial strip under a fixed pointer. Past the end of the band it goes round to the other end |
| The tuning mode tile | Tap | The next tuning mode, as a tap of MODE |
| The SQ tile | Tap | Opens **Squelch Mode** straight away. Keep a choice with the knob, or go back, and you are on the radio screen again |
| The BW tile | Tap | Opens the bandwidth page, as BW held |
| The V tile | Tap | Mutes the sound, and a second tap unmutes it. The tile reads MUTE while muted |

The line of radio text under the amber panel is too thin to be a target, and does nothing.

## What any touch does

- A touch stops a running DX scan and does nothing else, as any key does.
- A touch clears a frequency you were typing on the keypad and does nothing else.
- A hold is a finger kept still for 1.5 seconds. With **Key Beeps** at Short & Long Press, a hold beeps long. At Every Press, a hold beeps long and a tap or a swipe beeps short.
- While the screen is dark, and while the start-up, going to sleep or update screens show, the radio does not read the touch panel, and a touch does not wake the screen or end those screens. Use the knob, a key or the volume knob. This keeps a touch in a pocket or a bag from doing anything. A finger already on the screen when it lights up does nothing until it is lifted.

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
