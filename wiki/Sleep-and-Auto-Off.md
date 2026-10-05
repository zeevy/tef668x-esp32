# Sleep and Auto Off

The radio can go to sleep by itself when it is left alone, or when you ask it to. In sleep the screen, the tuner and Wi-Fi are off. A press of the tuning knob wakes it.

## Auto Off

Auto Off puts the radio to sleep after it has been left alone for a set time. It is off on a new radio.

Set it in the menu at **System > Auto Off**: **Off**, or 5 to 600 minutes (10 hours) in steps of 5. The HTTP API takes any whole minute from 1 to 600, as `slp` in `POST /api/settings`, so a short time can be set to try it out. The web page has no Auto Off control.

The count starts again every time the radio is used:

- a key press or a turn of a knob, the volume knob too
- a change made from the web page or the HTTP API

Just reading the radio's state from a browser or a script does not count, so a web page left open does not keep the radio awake all night.

### The countdown

| Time left | What you see and hear |
|---|---|
| More than 5 minutes | A small sleep mark, a person in bed, in the screen header |
| The last 5 minutes | The sleep mark changes to the theme's main colour, amber in Nightwatch |
| The last 30 seconds | The sound fades down |
| Time is up | **Going to Sleep** and **Press the knob to wake** for 5 seconds, then the radio sleeps |

![Going to Sleep, Press the knob to wake](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/sleeping.png)

Press a key or turn a knob at any point before it sleeps, even while **Going to Sleep** shows, and it stays awake: the sound comes back and the count starts again. A change from the web page or the API starts the count again too, but once **Going to Sleep** shows, only a key or a knob keeps it awake.

### What keeps it awake

These keep the radio awake while they run. The count starts once they stop.

- A station scan.
- The DX scanner.
- A firmware update, while it is written and while it is on trial.

## Sleep now

To put the radio to sleep at once, open the menu and go to **Go To > Sleep**. It shows **Going to Sleep** for 5 seconds with the sound fading, then sleeps. Turn a knob or press any key or button in those 5 seconds and it stays awake.

From a script, `POST /api/sleep` does the same. It needs the access PIN, and it is refused with 409 while a new firmware is written or on trial, since sleep would cut the update off or undo it. The menu shows `Update on trial - wait` in that case.

## Waking up

Press the tuning knob. The radio starts as it does when you switch it on, with the boot screen, and comes back on the station it was on with your settings. It joins Wi-Fi again by itself. **Menu > Diagnostics > Reset Reason** then says `Deep Sleep`.

The radio saves its settings before it sleeps, so nothing is lost.
