# Troubleshooting

Common problems and what to do about each.

## The USB upload fails

**`Failed to connect to ESP32: No serial data received`.** The radio is not in download mode. Its USB chip cannot start download mode by itself, so running the upload again does not help. Hold BOOT, tap RESET, let go of BOOT, then start the upload again. See [First Install over USB](First-Install-over-USB.md).

**No serial port shows up.** Try another USB cable: some only charge. Then try another USB port. If it still does not show, install the FTDI VCP driver. See [Before You Start](Before-You-Start.md#the-usb-serial-chip).

**More than one port.** Name the radio's port: `pio run -e ats125 -t upload --upload-port PORT`.

**The screen stays dark after the upload.** Tap RESET. If it is still dark, switch the radio off and on.

## The radio cannot be found on the network

- **Check that it is on Wi-Fi.** On the radio, open **Menu > Connectivity > Network Info**. Connection Status should say `Connected`.
- **Use the address instead of the name.** Some phones and networks do not find names ending in `.local`. Use the IP Address from Network Info: `http://192.168.1.40:8080`, with your own address.
- **Add `:8080`.** The web server is on port 8080, so `http://tef668x.local` alone does not open it.
- **Turn off a VPN on your computer or phone.** A VPN can take the traffic for your home network, and then the radio looks dead while it is fine.
- **Wait for the first lookup.** The first lookup of `tef668x.local` can take about 5 seconds.
- **Check Web Server.** If **Connectivity > Web Server** is Off, nothing answers on the network. Turn it on in the menu.

## Wi-Fi does not connect

- The radio needs a **2.4 GHz** network. It cannot see a 5 GHz one.
- If the passphrase was wrong, the radio starts its setup hotspot `tef668x-setup-XXXX` within 20 seconds, with Hotspot on Auto. Join it, open `http://192.168.4.1:8080`, and enter the details again.
- If **Hotspot** is Off, the radio never starts the hotspot. Set it to Auto in the menu, or use **Start Hotspot** on the [recovery screen](Recovery-Screen.md).
- If **Wi-Fi** is Off, turn it on in the menu at **Connectivity > Wi-Fi**. Only the menu, or Erase Settings on the recovery screen, can turn it back on.

See [Wi-Fi and Hotspot](Wi-Fi-and-Hotspot.md).

## A forgotten access PIN

Open the menu and go to **Connectivity > Web PIN**. The row shows the PIN on the radio's screen. To set a new one, press the row and set the digits. This needs no old PIN.

- **If you cannot use the menu:** use **Erase Settings** on the [recovery screen](Recovery-Screen.md). It puts every setting back to its default, the PIN `000000` and the Wi-Fi details too. The presets and the station log are kept.

After five wrong PINs, signing in is locked for a minute.

## An update did not take

- **`Update Failed` on the screen, or `Update failed` in the browser.** The file was not accepted, and the old firmware is still running. Check that you sent `firmware.bin` from `.pio/build/ats125/`, not another file.
- **The radio came back on the old firmware a few minutes after an update.** The new firmware did not reach the network its settings ask for, usually your Wi-Fi, and hold it for 10 seconds within 7 minutes, so the radio went back. **Menu > Diagnostics > Reset Reason** says `Rollback`. See [Updating](Updating.md#rollback-when-the-new-firmware-does-not-work).
- **`403 Enter the access PIN first.`** Sign in again. The session is lost at every restart.

## The radio hears nothing

- **Check the boot screen.** If the Tuner check shows a cross, the tuner did not start. **Menu > Diagnostics > Tuner** shows `None` then.
- **Every station reads a very low level, on every frequency.** This is what a wrong tuner crystal looks like. Check `xtl` in `http://tef668x.local:8080/api/state` against your radio. See [Supported Hardware](Supported-Hardware.md#the-tuner).
- **Squelch.** If the `V:` tile is grey, the squelch is holding the sound back. Set **Audio > Squelch > Squelch Mode** to Off to check.
- **Mute.** If the `V:` tile says `MUTE`, the radio is muted. Mute is turned off on the web page's Radio page.
- **The volume knob.** The bottom of its travel is -60 dB, which is almost silent.
- **The aerial.** Pull the FM aerial out fully. Weak stations come and go with it pushed in.

## The screen cannot be read

A theme you cannot read, a brightness of nothing, or the screen upside down: hold the tuning knob down while you switch the radio on. The [recovery screen](Recovery-Screen.md) always draws with a safe theme at full brightness.

## Seek stops on noise, or skips stations

Change **Seek Sensitivity**, in FM Reception for FM and AM Reception for AM. A higher number stops on weaker stations, and a lower number stops only on strong ones. See [Tuning](Tuning.md#seek).

## Keys do nothing at first

When the screen has dimmed, the first key press or knob turn only wakes it. Press again. To stop the screen dimming, set **Display > Dim After** to Never.

## Something else

Ask in [Discussions](https://github.com/zeevy/tef668x-esp32/discussions/categories/q-a), or open a [bug report](https://github.com/zeevy/tef668x-esp32/issues/new/choose) with the output of `/api/state`.
