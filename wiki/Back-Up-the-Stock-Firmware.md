# Back Up the Stock Firmware

Before you install this firmware, save a copy of everything on the radio's flash. With that copy you can put the radio back exactly as it came, with the stock firmware and its settings.

You need the radio, a USB cable that carries data, and esptool, the flash tool for the ESP32. The commands below are for esptool 5.x.

## Get esptool

- If PlatformIO is installed, it already has a copy: `~/.platformio/penv/bin/esptool` on macOS and Linux.
- Or install it on its own with `pip install esptool`.

Check that it runs:

```bash
esptool version
```

## Find the serial port

Plug the radio into the computer and look for its port:

| System | Where to look | Looks like |
|---|---|---|
| macOS | `ls /dev/cu.usbserial-* /dev/cu.wchusbserial*` | `/dev/cu.usbserial-AB12CD34` for an FT232R, `/dev/cu.wchusbserial...` for a CH340 |
| Linux | `ls /dev/ttyUSB*` | `/dev/ttyUSB0` |
| Windows | Device Manager, under Ports (COM & LPT) | `COM3` |

Use your own port name in place of `PORT` in the commands below.

## Put the radio into download mode

The FT232R USB chip of the test radio cannot start download mode by itself, so you do it with two buttons. A radio with a CH340 chip may do it by itself: if esptool connects without the buttons, skip this step.

1. Hold **BOOT**.
2. Tap **RESET**.
3. Let go of **BOOT**.

<!-- photo: the USB port of the ATS-125, with the BOOT and RESET buttons marked -->

The radio now waits for the computer. Its firmware does not run in this mode.

## Read the whole flash

The ATS-125 has 8 MB of flash, which is `0x800000` bytes. Read all of it, from address 0:

```bash
esptool --port PORT --baud 460800 read-flash 0 0x800000 ats125-stock.bin
```

If esptool says `Failed to connect to ESP32: No serial data received`, the radio is not in download mode. Do the BOOT and RESET steps again, then run the command again.

When it is done, the file must be exactly 8,388,608 bytes. Keep it somewhere safe, away from the build folder.

Tap **RESET** to start the stock firmware again.

## Write it back

To go back to the stock firmware later, put the radio into download mode, then write the copy back from address 0:

```bash
esptool --port PORT --baud 460800 write-flash 0 ats125-stock.bin
```

Tap **RESET** when it is done. The radio starts the stock firmware, with the settings it had when you made the copy.
