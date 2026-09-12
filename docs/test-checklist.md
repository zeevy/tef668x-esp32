# Manual test checklist

CI builds the firmware, runs the core tests and checks the style. It cannot test the display, the tuner, touch, audio or a real over the air update. Those are checked here, by hand, against the radio.

Work down the list. Report a failure by its number.

## Build 0.1.0, phase 0, escape the cable

**Read this before you flash.**

This build replaces the PE5PVB firmware with a partition table that has two application slots. After it goes on, the radio is not a radio. It has no tuner code, no display code and no menu. It joins Wi-Fi, serves a web page and can replace its own firmware. That is all it does, and that is the point of the phase.

Going back to the old firmware means a cable flash and a different partition table. Do not run this on a radio you need working today.

This flash needs the cable and the boot button. Every flash after it does not.

### Flashing, at the radio, with the cable

| # | Do this | Expect |
|---|---|---|
| 1 | Plug the radio into USB. Run `ls /dev/cu.usbserial-*` | `/dev/cu.usbserial-A5069RR4` is listed |
| 2 | Hold BOOT, tap RESET, release BOOT. Then run `pio run -e ats125 -t upload` | Upload runs to `Hard resetting via RTS pin` and does not stop with `Failed to connect to ESP32: No serial data received`. If it does, you did not get the button timing. Try again, there is no other fix |
| 3 | Run `pio device monitor -e ats125` and tap RESET | A banner appears with `tef668x-esp32`, the board, the firmware version, the running partition, the MAC and the access PIN |
| 4 | Read the banner | `running from` says `app0` and `confirmed`. The MAC matches `a0:b7:65:05:35:b4` |
| 5 | Read the PIN line | It says `000000   <- still the default`, followed by a warning that anyone on the network can replace the firmware |

### First boot with no stored credentials, at the radio

| # | Do this | Expect |
|---|---|---|
| 6 | Read the banner from check 3 | It says `access point tef668x-setup-35B4, open` and `setup page http://192.168.4.1/` |
| 7 | On a phone or laptop, look at the Wi-Fi networks | `tef668x-setup-35B4` is there and is open |
| 8 | Join it and open `http://192.168.4.1/` | The status page loads. Mode says `access point, setup`. A Wi-Fi form is shown with no PIN asked for, because this is the recovery path |
| 9 | Enter your network name and passphrase, press Save and join | The page says it is trying that network. The access point disappears within about 30 seconds |
| 10 | Look at the serial monitor | It prints the new credentials being saved, then the address it got on your network |

### On the network, from a browser

| # | Do this | Expect |
|---|---|---|
| 11 | Open `http://tef668x.local/` | The status page loads. If mDNS does not work on your network, use the address from check 10 and note that check 11 failed |
| 12 | Read the status table | Network is your SSID, mode is `joined a network`, running from is `app0`, image is `confirmed` |
| 13 | Open `http://tef668x.local/status.json` | JSON with `board`, `ver`, `slot`, `confirmed`, `mode`, `ip`, `heap`, `up` |
| 14 | Try to open the firmware upload form without entering a PIN | There is no upload form. Only the PIN form is shown |
| 15 | Enter `000000` | The page reloads and now shows the Wi-Fi form, the firmware form, a Change PIN form and the reboot button |
| 15b | Look at the top of the page | A red banner says the radio is on the default PIN |
| 15c | Check `/status.json` | `defaultPin` is `true` |
| 15d | Change the PIN to something else, then reload | The red banner is gone, `defaultPin` is `false`, and you are signed out. The old PIN no longer works |

### The PIN gate

| # | Do this | Expect |
|---|---|---|
| 16 | Open the page in a private window. Enter a wrong PIN four times | Each try says `That PIN is not right`. No lockout yet |
| 17 | Enter a wrong PIN a fifth time, then try the right one | The fifth try is refused, and the right PIN is refused too, with `Too many wrong PINs. Try again in a minute` |
| 18 | Wait one minute. Enter the right PIN | It is accepted |

### Over the air update from PlatformIO, no cable

| # | Do this | Expect |
|---|---|---|
| 19 | Unplug the USB cable from the radio. Leave it on battery | The radio stays on your network |
| 20 | Run `TEF_OTA_PIN=<your PIN> pio run -e ats125 -t upload --upload-port tef668x.local`. This needs the radio to be able to open a connection back to your machine, so it fails across subnets or through a host firewall. If it does, use the browser upload in check 24 instead | The build prints `Upload port tef668x.local looks like a network address, using espota`, the upload runs to 100 percent, and the radio reboots |
| 21 | Reconnect serial and read the banner, or open `/status.json` | `slot` has changed to `app1`. This proves both slots work |
| 22 | Wait 15 seconds, then reload `/status.json` | `confirmed` is `true`. The self check passed and the image is kept |
| 23 | Run the same upload again | `slot` goes back to `app0`. The two slots alternate |

### Over the air update from the browser, no cable

| # | Do this | Expect |
|---|---|---|
| 24 | Open the page on a phone, check the layout is readable and the buttons are reachable one handed | Bootstrap 5 is loaded from a CDN, so this needs the radio to be on a network with internet. On the setup access point the page falls back to the built in stylesheet and must still be usable |
| 24b | Enter the PIN, pick `.pio/build/ats125/firmware.bin` and press Upload and reboot | The page says the radio is rebooting into the new image |
| 25 | Wait 20 seconds and reload the page | The page comes back. The `slot` has changed again |
| 26 | Upload a file that is not firmware, such as a text file | The page says the image was not accepted and the radio is still running the old one. The radio does not reboot |

### The upload endpoint refuses anyone without the PIN

| # | Do this | Expect |
|---|---|---|
| 27 | From a terminal, run `curl -i -X POST http://tef668x.local/update` | `403 Forbidden` and `Enter the access PIN first.` The radio does **not** reboot. Check `/status.json` and confirm `up` kept counting |
| 28 | Run `curl -i -X POST -F 'note=hello' http://tef668x.local/update` | `403 Forbidden`, and again no reboot |
| 29 | Sign in with the PIN in a browser, then run the same curl without the cookie | Still `403`. A session in one browser does not let an unauthenticated client through |

### Rollback, the check this phase exists for

This is the one that decides whether the phase worked. Do not skip it.

| # | Do this | Expect |
|---|---|---|
| 30 | Note which partition is running from `/status.json` | Write it down |
| 31 | Edit `src/main.cpp`. Put `while (true) { delay(1000); }` on the line **after** `bootWatchdogArm();`, not before it, and build | The build succeeds |
| 32 | Upload that image over the air, either way | It uploads and the radio reboots into it. It then does nothing, because setup never finishes |
| 33 | Wait two minutes without touching the radio | Serial prints `setup did not finish in time, restarting`, the radio restarts, and comes back on the network on its own |
| 34 | Open `/status.json` | `slot` is the one from check 30, and `ver` is the working firmware. A hung image was rolled back with no cable |
| 35 | Undo the edit from check 31 | The working source is back |

A hang placed **before** `bootWatchdogArm()`, which is the first statement in `setup()`, is the one case that still needs the cable. There is nothing running at that point to catch it. Keep that line first.

### Recovery when the network details are wrong

| # | Do this | Expect |
|---|---|---|
| 36 | Enter the PIN, then save a network name that does not exist | The page says it is trying |
| 37 | Wait about 30 seconds and look at the Wi-Fi networks on your phone | `tef668x-setup-35B4` is back |
| 38 | Join it, open `http://192.168.4.1/`, enter the right details | The radio rejoins your network. Wrong credentials never needed the cable |
| 39 | With the radio on your network, reboot your router and watch `/status.json` | The radio stays on the network once the router is back. It only drops to the access point if the network really has gone |

## Build 0.2.0, phase 2, the tuner

This build makes it a radio. It tunes, it plays audio, and it can be driven
from a script over Wi-Fi.

Flashing is over the air now. No cable, no boot button.

```bash
curl -s -c /tmp/c -X POST -d 'pin=<your PIN>' http://tef668x.local/auth
curl -s -b /tmp/c -F "firmware=@.pio/build/ats125/firmware.bin" \
     http://tef668x.local/update
```

### The tuner comes up

| # | Do this | Expect |
|---|---|---|
| 40 | `curl -s http://tef668x.local/status.json` and read `tuner` | `part` is `TEF6686`, `patch` is 102, and `fmsi`, `fsrds` and `dr` are all false. That is what this part is |
| 41 | Read `xtal` and `xtalAdc` in the same reply | `xtalAdc` is 0 and `xtal` is `9.216 MHz`. A different crystal here means the radio is deaf, so it is worth reading every time |
| 42 | Flash again over the air, then read `xtal` again | Still `9.216 MHz`, not `not read`. The tuner is patched on every start, because after an update the chip keeps the old firmware's settings |

### It receives

Stations known to be on air at this location. Substitute your own.

| # | Do this | Expect |
|---|---|---|
| 43 | Extend the telescopic antenna | Without it every reading sits near the noise floor and it looks like a firmware fault |
| 44 | `curl -s -b /tmp/c -X POST -d 'khz=102800' http://tef668x.local/api/tune` | `FM 102.80 MHz`, and audio you can hear |
| 45 | Read `status.json` | `sig` above 300, `usn` under 50, `bw` around 236, `st` true. `sig` is in tenths, so 300 means 30 dBuV |
| 46 | Tune 98300, 101900, 93500, 91100, 92700, 94300, 106400, 104000 in turn | Each one receives. Weak ones may not show `st` true, which is honest rather than wrong |
| 47 | Watch `mod` on 92.7 and 104.0 | It goes above 100. That is real over modulation, not an error, and it is the case the AGC fixtures were kept for |

### It receives on medium wave, and comes back

The band switch is where this went wrong once, so check it in both directions.

| # | Do this | Expect |
|---|---|---|
| 48 | Tune 738 | `MW 738 kHz`, `sig` around 190, and audio |
| 49 | Tune 846 and 1377 | Both receive. 1377 shows as `1 377` |
| 50 | Tune 102800 straight after a medium wave station | It receives properly again. If every FM station reads `sig` 8320 and `bw` 4, the chip is stuck on its AM side and the FM preset wake is broken |
| 51 | Alternate FM and MW five times | The last reading matches the first for the same station |

### The control API

| # | Do this | Expect |
|---|---|---|
| 52 | `curl -i -X POST -d 'khz=102800' http://tef668x.local/api/tune` with no session | `403` and `Enter the access PIN first.` |
| 53 | Sign in, then `-d 'khz=50000'` | `400` and `That frequency is in no band.` 50 MHz is between OIRT and shortwave |
| 54 | `-d 'khz=0'` and `-d 'khz=abc'` | `400` and a plain reason. Never a 500, and never a 200 that quietly did nothing |
| 55 | `-d 'khz=4294967295'` | `400`, not a tune to something absurd |

### Things this build cannot be tested for

Not written yet, so do not look for them: display, touch, encoder, keypad, RTC, battery reading, telemetry, RDS.

### Already covered by CI, do not retest by hand

Partition table arithmetic, the settings struct round trip, the rejection of a truncated or a wrong version blob, the access PIN derivation, the PIN attempt gate and its one minute lockout, the millisecond wraparound, and the build itself.
