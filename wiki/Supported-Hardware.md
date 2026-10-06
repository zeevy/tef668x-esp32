# Supported Hardware

The firmware runs on radios built around an NXP TEF668x tuner and an ESP32. Today it supports one radio, the **ATS-125**.

<!-- photo: the front of the ATS-125, with the screen, both knobs, the BAND, BW and MODE buttons and the keypad -->

## ATS-125

The ATS-125 is a portable FM and AM receiver with a 320 x 240 colour screen.

| Part | What it is | Used by the firmware |
|---|---|---|
| Processor | ESP32-D0WD-V3, two cores at 240 MHz, in a WROOM-32U class module with a connector for an outside Wi-Fi aerial | Yes |
| Memory | 8 MB flash, no PSRAM | Yes |
| Tuner | NXP TEF6686 | Yes |
| Screen | ILI9341, 320 x 240 pixels, colour | Yes |
| Touch | XPT2046 touch controller on the main board, for the screen's resistive touch | Read, not used yet |
| Knobs | The tuning knob, which also presses, and the volume knob | Yes |
| Buttons | BAND, BW and MODE | Yes |
| Keypad | Digits 0 to 9, ENTER and DX, read through a PCA9555 chip | Yes |
| Battery | One lithium polymer cell, 3.7 V, 2500 mAh | The level. While Wi-Fi is on, it is read once at start up |
| USB | USB-C, for charging and for the first install. The test radio has an FT232R USB serial chip with no auto reset, so its first install needs the BOOT and RESET buttons. The seller lists a CH340 chip for this model, so other batches may have that instead | Yes |
| Clock chip | An RX8010 real time clock, on some radios | Not used. The clock is set from the network |
| Standby LED | | Not used |

While Wi-Fi is on, the battery is read only at start up, because Wi-Fi uses the same part of the ESP32 while it runs. With Wi-Fi off it is read every second.

The radio's Bluetooth is a separate chip, with its own aerial connector marked `BT ANT1`. This firmware does not control it, and does not use the ESP32's own Bluetooth either.

## The tuner

TEF668x is a family of tuner chips. At every start the firmware reads which one is fitted, and loads the patch it asks for. The tuner needs this patch at every power on before it can tune anything.

- It knows the TEF6686, TEF6687, TEF6688 and TEF6689, and carries both patches they use, 102 and 205.
- Only the TEF6686 with patch 102 has been tried, because that is what the test radio has.
- The extra features of the bigger chips (stereo improvement, full search RDS, digital radio) are read but not used yet.
- The tuner has its own crystal, and radios in this family are made with different ones. The firmware reads a sense pin that says 9.216, 12 or 55 MHz. If the reading matches none of them, it uses 4 MHz. The crystal it chose is `xtl` in `http://tef668x.local:8080/api/state`. A wrong crystal does not stop the radio: it tunes, but every station reads as no signal.

To see the chip on your radio, open the menu and go to **Diagnostics > Tuner**. It shows the chip and the patch, for example `TEF6686 patch 102`. If the tuner does not answer at start up, the boot screen marks the Tuner check as failed. The radio still joins Wi-Fi, so a new firmware can be installed.

## Not supported yet

- **Touch.** The firmware reads the screen's touch controller, can be [calibrated](Touch-Screen.md#calibrate-the-touch-screen) and shows its readings in the HTTP API, but outside the calibration screen a touch does nothing on the radio yet. Everything is done with the knobs, the buttons and the keypad, or from the web page.
- **Other screen layouts.** There is one layout, for a 320 x 240 screen.
- **Other radios.** Every detail of a board is kept in one header in the code, so another TEF668x radio can be added as a new board header and a new build environment. If you have one, ask in [Discussions](https://github.com/zeevy/tef668x-esp32/discussions/categories/q-a) or open a [feature request](https://github.com/zeevy/tef668x-esp32/issues/new/choose).
