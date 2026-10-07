# FAQ

### Which radios does it run on?

The ATS-125 today. Other radios with a TEF668x tuner and an ESP32 can be added later, each with its own board header. See [Supported Hardware](Supported-Hardware.md).

### Is it a fork of the PE5PVB firmware?

No. It is a new firmware written from the start. It learned how the hardware works from the PE5PVB TEF6686_ESP32 firmware and shares no code with it, apart from the tuner's patch data. See [Credits and Licence](Credits-and-Licence.md).

### Can I go back to the stock firmware?

Yes, if you saved a copy first. See [Back Up the Stock Firmware](Back-Up-the-Stock-Firmware.md).

### Is there a ready made firmware file to download?

Yes. Each release on the [Releases page](https://github.com/zeevy/tef668x-esp32/releases) has `firmware-ats125-full.bin` for a first install over USB, and `firmware-ats125.bin` for updates over Wi-Fi. See [First Install over USB](First-Install-over-USB.md) and [Updating](Updating.md).

### Do I need the USB cable for every update?

No. Only the first install goes over USB. After that, updates go over Wi-Fi, and the radio goes back to the old firmware by itself if a new one does not work. See [Updating](Updating.md).

### Does the radio update itself?

Only when you say yes. Turn on **System > Check for Updates**, and the radio looks on GitHub for a newer release once it is on your network, and offers it on the screen. Nothing is installed until you say yes: **Update** on the radio, or **Install** on the web page's System page. See [Updating](Updating.md#from-github-on-the-radio).

### Does the radio need Wi-Fi or the internet?

No. It works as a radio with Wi-Fi off. Wi-Fi is needed for the web page, the HTTP API, updates and the clock. The internet is needed only for the clock, which is set from a time server.

### Does touch work?

Yes. The radio screen, the frequency keypad, the menu, the bandwidth page, the RDS screen and DX mode's pages can be worked by finger, as [Touch Screen](Touch-Screen.md) describes. While Touch is On, three lines at the top left of the radio screen open the menu. To check that your screen's touch works, watch `tch` in [GET /api/state](HTTP-API.md#get-apistate) while you press the screen. If the screen ever acts on its own, turn it off at **Controls > Touch**, or on the [recovery screen](Recovery-Screen.md) when the menu cannot be used. To calibrate it, see [Touch Screen](Touch-Screen.md).

### Why is the clock wrong, or not shown?

The clock is set from the network. It is hidden until the radio has the time, and it shows UTC until you set your offset at **Connectivity > Network Time**. It does not change for summer time by itself. See [First Start and Wi-Fi](First-Start-and-Wi-Fi.md#set-the-clock).

### Does the RDS clock time set the radio's clock?

No. It is shown as CT on the first RDS page, but the radio's clock comes from the network only.

### How do I save a preset on the radio?

You cannot, on the radio itself. A station scan fills the empty presets, and presets can be set one at a time over the HTTP API or loaded from a CSV file. See [Presets and Station Scan](Presets-and-Station-Scan.md).

### How do I delete a station from the station log?

You cannot. The log keeps the last 250 stations and drops the oldest when it is full. See [Station Log](Station-Log.md).

### Why does the first key press do nothing?

The screen had dimmed, and the first key press or knob turn only wakes it. See [Display and Themes](Display-and-Themes.md#brightness-and-dimming).

### Why is the battery not shown?

It is off on a new radio. Turn it on at **Display > Battery**. While Wi-Fi is on, the battery is read only once, at start up, because Wi-Fi uses the part of the chip that reads it.

### I forgot the access PIN. What now?

Set a new one in the menu at **Connectivity > Web PIN**. No old PIN is needed there. See [Troubleshooting](Troubleshooting.md#a-forgotten-access-pin).

### Why is the web page on port 8080?

The web server listens on port 8080, so the address is always `http://tef668x.local:8080`, with the `:8080`.

### Can two people use the web page at once?

Both can look at it, but only one browser can be signed in at a time. Signing in on a second browser signs the first out. See [Web Page](Web-Page.md#signing-in).

### What is DX mode for?

Finding far away FM stations. It shows the station in detail, sweeps the band for signals that come up, scans for stations you have not heard before, and keeps a list of what it caught. See [DX Mode](DX-Mode.md).

### Can I control the radio from a script?

Yes. Everything the screen can do, the HTTP API can do. See [HTTP API](HTTP-API.md).
