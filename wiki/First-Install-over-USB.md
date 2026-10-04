# First Install over USB

The first install goes over the USB cable. After that, every update goes [over Wi-Fi](Updating.md).

There are two ways: install a ready made release, or build the firmware yourself from the source. Both need the radio in download mode.

Before you start, read [Before You Start](Before-You-Start.md) and [back up the stock firmware](Back-Up-the-Stock-Firmware.md).

## Put the radio into download mode

Plug the radio into the computer, then:

1. Hold **BOOT**.
2. Tap **RESET**.
3. Let go of **BOOT**.

<!-- photo: the USB port of the ATS-125, with the BOOT and RESET buttons marked -->

A radio with a CH340 USB chip may go into download mode by itself. If the install below connects without the buttons, you do not need them.

## Install a release

Each release on the [Releases page](https://github.com/zeevy/tef668x-esp32/releases) carries `firmware-ats125-full.bin`: the bootloader, the partition table and the firmware in one file, for a first install. You need esptool 5.x, the flash tool for the ESP32: `pip install esptool`, or the copy PlatformIO installs at `~/.platformio/penv/bin/esptool` on macOS and Linux.

1. Download `firmware-ats125-full.bin` from the latest release.
2. Put the radio into download mode, as above.
3. Write the file at address 0, using your own port in place of `PORT`:

   ```bash
   esptool --port PORT --baud 460800 write-flash 0 firmware-ats125-full.bin
   ```

4. Start the radio, as below.

This file erases the stored settings, the Wi-Fi network and the access PIN with them, and the presets, as a first install should. The station log and the DX data are kept. For an update of a radio that already runs this firmware, use `firmware-ats125.bin` over Wi-Fi instead: see [Updating](Updating.md).

The page of each release shows the sha256 of each file, so you can check your download.

## Build and install

### 1. Get the source

```bash
git clone https://github.com/zeevy/tef668x-esp32.git
cd tef668x-esp32
```

### 2. Build it

```bash
pio run -e ats125
```

The first build downloads the ESP32 tools and the LVGL library, so it takes longer than the ones after it. It ends with `SUCCESS`. The firmware is then at `.pio/build/ats125/firmware.bin`.

### 3. Upload

Put the radio into download mode, as above, then:

```bash
pio run -e ats125 -t upload
```

PlatformIO finds the serial port by itself. If more than one USB serial adapter is plugged in, name the radio's port:

```bash
pio run -e ats125 -t upload --upload-port PORT
```

## If it does not connect

If the install stops with `Failed to connect to ESP32: No serial data received`, the radio is not in download mode. Do the BOOT and RESET steps again, then run the command again. Running it again without the buttons does not help.

## Start the radio

When the install is done, tap **RESET**. The FT232R USB chip cannot restart the radio by itself. A radio with a CH340 may restart by itself. If the screen stays dark, switch the radio off and on again.

The radio shows its boot screen and starts. With no Wi-Fi stored yet, it starts its own setup hotspot. Go on with [First Start and Wi-Fi](First-Start-and-Wi-Fi.md).
