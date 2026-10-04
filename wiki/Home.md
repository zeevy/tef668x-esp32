# tef668x-esp32

Open source firmware for radio receivers built around the NXP TEF668x tuner and an ESP32. The first supported radio is the **ATS-125**, a portable FM and AM receiver with a 320x240 colour display.

![The radio screen, the RDS station page and the DX band scope](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/hero.png)

It works as a full radio today: FM and AM, seek, squelch, RDS, 99 presets, station scans, a station log, DX mode, a network clock, a menu on the radio, a browser page, an HTTP control API and updates over Wi-Fi with rollback. Touch input, other screen layouts, live telemetry and a spectrum view are still to come.

This is a ground up rewrite, not a fork. It takes its hardware knowledge and many ideas from [PE5PVB/TEF6686_ESP32](https://github.com/PE5PVB/TEF6686_ESP32) and shares no code with it, apart from the tuner's patch data. It is GPLv3.

## Where to start

The wiki is being written. Until the pages below are here, the [README](https://github.com/zeevy/tef668x-esp32#readme) has the short version of each topic:

- [Building and flashing](https://github.com/zeevy/tef668x-esp32#building-and-flashing)
- [First start](https://github.com/zeevy/tef668x-esp32#first-start)
- [Controls](https://github.com/zeevy/tef668x-esp32#controls)
- [Control API](https://github.com/zeevy/tef668x-esp32#control-api)

## Help

- Questions: [Discussions](https://github.com/zeevy/tef668x-esp32/discussions/categories/q-a)
- Bugs and ideas: [Issues](https://github.com/zeevy/tef668x-esp32/issues/new/choose)
