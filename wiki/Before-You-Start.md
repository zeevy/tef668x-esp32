# Before You Start

What you need, and what to know, before you put this firmware on your radio.

## What you need

- **An ATS-125 radio.** See [Supported Hardware](Supported-Hardware.md).
- **A USB cable that carries data.** Some cables only charge, and then the computer does not see the radio at all.
- **A computer** with Windows, macOS or Linux, and Python 3 and git on it.
- **PlatformIO 6.2.0**, the tool that builds the firmware and puts it on the radio. Install it with pip:

  ```bash
  pip install platformio==6.2.0
  ```

- **A Wi-Fi network on 2.4 GHz.** The ESP32 has no 5 GHz radio. Wi-Fi is needed for updates after the first install, for the web page and for the clock, which is set from the network. The radio itself works without Wi-Fi.

There is no ready made firmware file to download yet, so the first install builds the firmware from the source on your computer. A release download will come later.

## The USB serial chip

The radio talks to the computer through a USB serial chip. The test radio has an FT232R. The seller lists a CH340 for this model, so your radio may have either.

When the radio is plugged in, the computer shows a new serial port:

| System | FT232R | CH340 |
|---|---|---|
| Windows | A `COM` port | A `COM` port |
| macOS | `/dev/cu.usbserial-...` | `/dev/cu.wchusbserial...` |
| Linux | `/dev/ttyUSB0` or similar | `/dev/ttyUSB0` or similar |

If no port shows up, try another cable first. If it still does not show, install the driver for your chip: the FTDI VCP driver from [ftdichip.com](https://ftdichip.com/drivers/vcp-drivers/) for an FT232R, or the CH340 driver from WCH for a CH340.

The FT232R on the test radio has no auto reset. So for the [first install](First-Install-over-USB.md) you put the radio into download mode by hand, with its BOOT and RESET buttons. A radio with a CH340 may do this by itself; if the upload works without the buttons, you do not need them. After the first install, every update goes [over Wi-Fi](Updating.md).

## Know the risks

- **The install replaces the stock firmware.** [Back up the stock firmware](Back-Up-the-Stock-Firmware.md) first, so you can go back to it.
- **A USB install has no rollback.** If an upload is cut off, the radio does not start until it is flashed again. The ESP32 can always be put into download mode, because that mode is in the chip itself, so a cut off upload can be fixed with the same cable.
- **An [update over Wi-Fi](Updating.md) is safer.** It is written to a second slot. If the new firmware does not start and join your network, the radio goes back to the old one by itself.
- **It has been tested on one radio**, an ATS-125 with a TEF6686 tuner.
- **The setup hotspot has no password, and the access PIN starts as `000000`.** Change the PIN after the first start.
- The firmware comes with no warranty, as its GPLv3 licence says.
