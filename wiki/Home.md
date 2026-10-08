# tef668x-esp32

Open source firmware for radio receivers built around the NXP TEF668x tuner and an ESP32. The first supported radio is the **ATS-125**, a portable FM and AM receiver with a 320x240 colour display.

![The radio screen, the RDS station page and the DX band scope](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/hero.png)

It works as a full radio today: FM and AM, seek, squelch, RDS, 99 presets, station scans, a station log, DX mode, a network clock, a menu on the radio, a browser page, an HTTP control API and updates over Wi-Fi with rollback. Touch input, other screen layouts, live telemetry and a spectrum view are still to come.

This is a ground up rewrite, not a fork. It takes its hardware knowledge and many ideas from [PE5PVB/TEF6686_ESP32](https://github.com/PE5PVB/TEF6686_ESP32) and shares no code with it, apart from the tuner's patch data. It is GPLv3.

## Where to start

1. [Supported Hardware](Supported-Hardware.md): the radio this runs on, and what works on it.
2. [Before You Start](Before-You-Start.md): what you need, and the risks, before you install.
3. [Back Up the Stock Firmware](Back-Up-the-Stock-Firmware.md): save a copy of the radio's flash, so you can go back.
4. [First Install over USB](First-Install-over-USB.md): install a release, or build the firmware yourself, and put it on the radio.
5. [First Start and Wi-Fi](First-Start-and-Wi-Fi.md): the boot screen, the setup hotspot, the access PIN and the clock.
6. [Updating](Updating.md): new firmware over Wi-Fi, and how the radio goes back by itself when one does not work.

## Using the radio

- [Controls](Controls.md): every button, knob and key, on every screen.
- [Radio Screen](Radio-Screen.md): what each part of the main screen shows.
- [Tuning](Tuning.md): the bands, the tuning modes, seek and typing a frequency.
- [Presets and Station Scan](Presets-and-Station-Scan.md): the 99 presets, filling them with a scan, and CSV import and export.
- [Station Log](Station-Log.md): writing down the stations you hear, and exporting the log.
- [RDS](RDS.md): the station data on the radio screen and the four RDS pages.
- [DX Mode](DX-Mode.md): the DX, Scope, Scanner and Catches pages for finding far away stations.
- [Band Scope](Band-Scope.md): the level of every channel of the FM band, with your presets and catches marked.
- [Sound and Bandwidth](Sound-and-Bandwidth.md): filter width, volume, squelch, the volume AGC and the reception settings.
- [Display and Themes](Display-and-Themes.md): the themes, brightness and dimming, rotation and the battery mark.
- [Sleep and Auto Off](Sleep-and-Auto-Off.md): the sleep timer, and putting the radio to sleep.
- [Touch Screen](Touch-Screen.md): turning touch on or off, and calibrating it.
- [Menu Guide](Menu-Guide.md): every row of the menu, with its values and what it does.

## Network

- [Wi-Fi and Hotspot](Wi-Fi-and-Hotspot.md): how the radio picks its network, and the Wi-Fi, Hotspot and Web Server switches.
- [Web Page](Web-Page.md): each page of the radio's own web page, with pictures.
- [HTTP API](HTTP-API.md): every route, with curl examples and real answers from the radio.
- [PC Tools](PC-Tools.md): XDR-GTK and FM-DX Webserver over Wi-Fi, and RDS Spy and StationList through XDR-GTK.

## Help

- [Recovery Screen](Recovery-Screen.md): the way back when the normal screen cannot be used.
- [Troubleshooting](Troubleshooting.md): common problems and what to do.
- [FAQ](FAQ.md): short answers to common questions.
- [Glossary](Glossary.md): the words and short forms the radio uses.

## Project

- [Building from Source](Building-from-Source.md): the build, the tests, the checks and the pictures.
- [Contributing](Contributing.md): questions, bug reports, feature requests, changes and the wiki.
- [Credits and Licence](Credits-and-Licence.md): where the firmware comes from, and the parts made by others.

## Questions and bugs

- Questions: [Discussions](https://github.com/zeevy/tef668x-esp32/discussions/categories/q-a)
- Bugs and ideas: [Issues](https://github.com/zeevy/tef668x-esp32/issues/new/choose)
