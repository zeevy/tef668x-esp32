# tef668x-esp32

Open source firmware for radio receivers built around the NXP TEF668x tuner and an ESP32. The first supported radio is the **ATS-125**, a portable FM and AM receiver with a 320x240 colour display.

![The radio screen, the RDS station page and the DX band scope](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/hero.png)

It works as a full radio today: FM and AM, seek, squelch, RDS, 99 presets, station scans, a station log, DX mode, a network clock, a menu on the radio, a browser page, an HTTP control API and updates over Wi-Fi with rollback. Touch input, other screen layouts, live telemetry and a spectrum view are still to come.

This is a ground up rewrite, not a fork. It takes its hardware knowledge and many ideas from [PE5PVB/TEF6686_ESP32](https://github.com/PE5PVB/TEF6686_ESP32) and shares no code with it, apart from the tuner's patch data. It is GPLv3.

## Where to start

1. [Supported Hardware](Supported-Hardware.md): the radio this runs on, and what works on it.
2. [Before You Start](Before-You-Start.md): what you need, and the risks, before you install.
3. [Back Up the Stock Firmware](Back-Up-the-Stock-Firmware.md): save a copy of the radio's flash, so you can go back.
4. [First Install over USB](First-Install-over-USB.md): build the firmware and put it on the radio.
5. [First Start and Wi-Fi](First-Start-and-Wi-Fi.md): the boot screen, the setup hotspot, the access PIN and the clock.
6. [Updating](Updating.md): new firmware over Wi-Fi, and how the radio goes back by itself when one does not work.

## Using the radio

- [Controls](Controls.md): every button, knob and key, on every screen.
- [Radio Screen](Radio-Screen.md): what each part of the main screen shows.
- [Tuning](Tuning.md): the bands, the tuning modes, seek and typing a frequency.
- [Presets and Station Scan](Presets-and-Station-Scan.md): the 99 presets, filling them with a scan, and CSV import and export.
- [Station Log](Station-Log.md): writing down the stations you hear, and exporting the log.

The wiki is still being written. Until its other pages are here, the README's [Control API](https://github.com/zeevy/tef668x-esp32#control-api) section has the short version of the HTTP API.

## Help

- Questions: [Discussions](https://github.com/zeevy/tef668x-esp32/discussions/categories/q-a)
- Bugs and ideas: [Issues](https://github.com/zeevy/tef668x-esp32/issues/new/choose)
