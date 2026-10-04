# Updating

After the first install, every update goes over Wi-Fi. No cable and no buttons are needed. The radio keeps the old firmware, and goes back to it by itself if the new one does not work.

You need a new firmware file, `firmware.bin`. Today you build it yourself: `pio run -e ats125` writes it to `.pio/build/ats125/firmware.bin`. A release download will come later.

Your settings, presets and station log stay as they are.

## From the web page

1. Open the radio's web page, `http://tef668x.local:8080`, and go to the **System** page.
2. Sign in with your access PIN.
3. Under **Firmware**, pick the `.bin` file and press **Upload and reboot**.

The upload takes about 20 seconds. When it is written, the page says `Update written` and the radio restarts into the new firmware.

## With curl

Sign in once, which stores a session cookie in the file `jar`, then send the file:

```bash
R=http://tef668x.local:8080
curl -s -c jar -d pin=000000 $R/auth
curl -s -b jar -F firmware=@.pio/build/ats125/firmware.bin $R/update
```

Use your own PIN in place of `000000`. The radio answers with a short page:

| Answer | What it means |
|---|---|
| 200, `Update written` | The image is written. The radio restarts into it |
| 403, `Enter the access PIN first.` | Not signed in. Run the `/auth` line again |
| 400, `Nothing uploaded` | The request had no file in it |
| 500, `Update failed` | The image was not accepted. The radio is still running the old one |

The session is lost when the radio restarts, so sign in again before the next update.

## What the screen shows

When the upload starts, the radio saves what it is tuned to, turns the sound down, and shows the progress:

![The update screen: UPDATING FIRMWARE, 62 %, Do not remove power](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/veil.png)

Keep the radio powered until it restarts. If the write fails, the screen shows **Update Failed** and **Previous firmware kept** for a few seconds, and the radio goes back to playing with the old firmware.

![Update Failed, Previous firmware kept](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/update-failed.png)

## Rollback: when the new firmware does not work

The radio has two firmware slots. A new firmware is written to the slot that is not running, and starts on trial. It has to prove that the next update can still reach it: it must be on the network its settings ask for, and stay there for 10 seconds, within 7 minutes of starting. Then it is marked good. If it does not manage that, or if it crashes before, the radio restarts and goes back to the old firmware by itself.

```mermaid
flowchart TD
    U([Firmware sent to the radio]) --> S["Save the settings<br/>turn the sound down"]
    S --> W["Write to the other slot<br/>progress on the screen"]
    W -- "write failed" --> X(["Update Failed<br/>old firmware kept"])
    W -- "written" --> B["Restart into the new firmware,<br/>on trial"]
    B --> J{"On the wanted network<br/>for 10 s, within 7 minutes?"}
    J -- yes --> G(["Marked good"])
    J -- no --> RB(["Restart, and the old<br/>firmware comes back"])
```

The wanted network depends on the Connectivity settings:

| Settings | The new firmware must be |
|---|---|
| Hotspot on Auto, the usual case | Joined to your Wi-Fi. Falling back to the setup hotspot does not count, because then the next update could not reach it |
| Hotspot set to On | Serving its hotspot |

The 7 minutes leave time for a second try at your network, so a router that is down for a moment does not throw away a good firmware.

While the new firmware is on trial, **Menu > System > Restart Radio** does not restart it, and says `Update on trial - wait`, because a restart then would bring the old firmware back.

## Check which firmware is running

- **Menu > About > Firmware Version** shows the version, and **Build** shows the git commit it was built from.
- **Menu > Diagnostics > Boot Source** shows which slot it started from, `app0` or `app1`. **Reset Reason** says `Update` after an update, and `Rollback` after a new firmware did not pass its check in time and the old one came back.
- The **System** page shows the firmware, the slot it runs from, and the **Image**: `confirmed`, or `on trial, waiting for the self check`.

## Go back to the firmware before

To go back to the older firmware by hand, use the recovery screen: hold the tuning knob down while you switch the radio on, choose **Restore Previous Firmware**, and press again to confirm. The row shows `None` when there is no older firmware to go back to.
