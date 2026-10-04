# First Install over USB

The first install goes over the USB cable. After that, every update goes [over Wi-Fi](Updating.md).

There is no ready made firmware file to download yet, so you build the firmware from the source on your computer. A release download will come later.

Before you start, read [Before You Start](Before-You-Start.md) and [back up the stock firmware](Back-Up-the-Stock-Firmware.md).

## 1. Get the source

```bash
git clone https://github.com/zeevy/tef668x-esp32.git
cd tef668x-esp32
```

## 2. Build it

```bash
pio run -e ats125
```

The first build downloads the ESP32 tools and the LVGL library, so it takes longer than the ones after it. It ends with `SUCCESS`. The firmware is then at `.pio/build/ats125/firmware.bin`.

## 3. Put the radio into download mode

Plug the radio into the computer, then:

1. Hold **BOOT**.
2. Tap **RESET**.
3. Let go of **BOOT**.

<!-- photo: the USB port of the ATS-125, with the BOOT and RESET buttons marked -->

## 4. Upload

```bash
pio run -e ats125 -t upload
```

PlatformIO finds the serial port by itself. If more than one USB serial adapter is plugged in, name the radio's port:

```bash
pio run -e ats125 -t upload --upload-port PORT
```

If the upload stops with `Failed to connect to ESP32: No serial data received`, the radio is not in download mode. Do step 3 again, then run the upload again. Running the upload again without the buttons does not help.

## 5. Start the radio

When the upload is done, tap **RESET**. The FT232R USB chip cannot restart the radio by itself. A radio with a CH340 may restart by itself. If the screen stays dark, switch the radio off and on again.

The radio shows its boot screen and starts. With no Wi-Fi stored yet, it starts its own setup hotspot. Go on with [First Start and Wi-Fi](First-Start-and-Wi-Fi.md).
