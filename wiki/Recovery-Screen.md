# Recovery Screen

The recovery screen is a way back when the normal screen cannot be used: a theme you cannot read, a screen turned the wrong way, a brightness of nothing, Wi-Fi details that no longer work, or a new firmware you want to undo.

![The recovery screen with its five rows](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/recovery.png)

## Open it

Hold the tuning knob down while you switch the radio on, and keep holding it until the **Recovery** screen shows. Then let go.

The recovery screen does not use your display settings. It always draws with a safe theme, at full brightness, so it can be read whatever was set before. It does not time out: it waits until you choose a row.

Only the tuning knob works here. Turn it to move between rows, and press it to choose.

## The five rows

| Row | What it does |
|---|---|
| **Rotate Display** | Turns the screen over, between `Normal` and `Upside Down`, then restarts. The row shows the rotation stored now |
| **Start Hotspot** | Sets Connectivity > Hotspot to On, then restarts. The radio then serves its own hotspot at every start, until you change that setting. Wi-Fi must be on for this |
| **Restore Previous Firmware** | Goes back to the firmware that ran before the last update over Wi-Fi, then restarts. It shows `None` when there is no older firmware, as after an install over USB |
| **Erase Settings** | Puts every setting back to its default, Wi-Fi details and the access PIN too, then restarts. The presets and the station log are kept |
| **Exit & Start Radio** | Restarts into the normal radio, changing nothing |

Every row but **Exit & Start Radio** asks first. The first press shows `Press again` on the row and says what will happen on the line at the bottom. Press again to do it. Turn the knob to another row to cancel.

If a row could not save its change, it shows `Failed` and the radio stays on the recovery screen.

## After Start Hotspot

With the hotspot set to On, the radio does not try your Wi-Fi at all. If Wi-Fi or Web Server was turned off, turn it on in the menu at **Connectivity** first. Join its network, `tef668x-setup-XXXX`, and open `http://192.168.4.1:8080`. Here the Wi-Fi form needs the access PIN: sign in first, then enter your network's details. Saving a network sets the hotspot back to Auto, and the radio joins that network.

The hotspot has no password. While it is set to On, anyone in range can reach the radio's web page, so keep your own access PIN set.
