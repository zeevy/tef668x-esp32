# Credits and Licence

## Licence

This firmware is free software under the GNU General Public License, version 3 (GPLv3). You may use it, change it and share it, and a changed version you share must be GPLv3 too, with its source. It comes with no warranty. The full text is in [LICENSE](https://github.com/zeevy/tef668x-esp32/blob/master/LICENSE).

## Where it comes from

It takes its knowledge of the hardware, and many ideas, from the [PE5PVB TEF6686_ESP32](https://github.com/PE5PVB/TEF6686_ESP32) firmware by Sjef Verhoeven, PE5PVB, which runs on the same radios. That firmware showed how this hardware works: the pins, the tuner's start up and its patch. It is GPLv3, so this project is GPLv3 too. Apart from the tuner's patch data, the two share no code.

The country tables of the RDS decoder were checked against Oona Räisänen's [redsea](https://github.com/windytan/redsea) RDS decoder (MIT).

## Parts made by others

The firmware carries some parts made by others, each under its own licence. [THIRD_PARTY_LICENSES.md](https://github.com/zeevy/tef668x-esp32/blob/master/THIRD_PARTY_LICENSES.md) has their full texts.

| Part | What it is for | Licence |
|---|---|---|
| [LVGL](https://lvgl.io) 9.5.0 | The graphics library that draws the screens. Downloaded at build time | MIT |
| [Roboto Condensed](https://fonts.google.com/specimen/Roboto+Condensed) | The font on the screen, stored as bitmaps | SIL Open Font License 1.1 |
| [Material Symbols](https://fonts.google.com/icons) | The icons on the screen, stored as bitmaps | Apache 2.0 |
| [Preact](https://preactjs.com) 11.0.1 | The library the web page is built with. Built into the page | MIT |
| [Preact Signals](https://github.com/preactjs/signals) 2.11.3 | Holds the data the web page shows, and redraws a part when its data changes. Built into the page | MIT |
| [Tailwind CSS](https://tailwindcss.com) 4.3.3 | The look of the web page. The styles it makes are built into the page | MIT |
| [Lucide](https://lucide.dev) 1.55.0 | The icons on the web page, copied into the page | ISC, and MIT for the icons that come from Feather |
| TEF668x patch data | NXP's firmware for the tuner, which it needs at every power on | See below |

The web page is built and tested with [Vite](https://vite.dev) 8.3.4, vite-plugin-singlefile 2.3.3 and [Playwright](https://playwright.dev) 1.64.0. They are not in the firmware.

### The tuner's patch data

The TEF668x tuner has no usable firmware of its own, so the radio writes NXP's patch to it at every start. The bytes are NXP's. They are carried here as the GPLv3 PE5PVB firmware carries the same bytes. NXP has not published terms for them that this project knows of.

## Trademarks

NXP and TEF668x belong to NXP Semiconductors. ESP32 belongs to Espressif Systems. This project is not made by or linked to either of them, or to the makers of the ATS-125.
