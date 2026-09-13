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
| 4 | Read the banner | `running from` says `app0` and `cnf`. The MAC matches `a0:b7:65:05:35:b4` |
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
| 12 | Read the status table | Network is your SSID, mode is `joined a network`, running from is `app0`, image is `cnf` |
| 13 | Open `http://tef668x.local/status.json` | JSON with `brd`, `ver`, `slt`, `cnf`, `net`, `ip`, `hep`, `up` |
| 14 | Try to open the firmware upload form without entering a PIN | There is no upload form. Only the PIN form is shown |
| 15 | Enter `000000` | The page reloads and now shows the Wi-Fi form, the firmware form, a Change PIN form and the reboot button |
| 15b | Look at the top of the page | A red banner says the radio is on the default PIN |
| 15c | Check `/status.json` | `dpn` is `true` |
| 15d | Change the PIN to something else, then reload | The red banner is gone, `dpn` is `false`, and you are signed out. The old PIN no longer works |

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
| 21 | Reconnect serial and read the banner, or open `/status.json` | `slt` has changed to `app1`. This proves both slots work |
| 22 | Wait 15 seconds, then reload `/status.json` | `cnf` is `true`. The self check passed and the image is kept |
| 23 | Run the same upload again | `slt` goes back to `app0`. The two slots alternate |

### Over the air update from the browser, no cable

| # | Do this | Expect |
|---|---|---|
| 24 | Open the page on a phone, check the layout is readable and the buttons are reachable one handed | Bootstrap 5 is loaded from a CDN, so this needs the radio to be on a network with internet. On the setup access point the page falls back to the built in stylesheet and must still be usable |
| 24b | Enter the PIN, pick `.pio/build/ats125/firmware.bin` and press Upload and reboot | The page says the radio is rebooting into the new image |
| 25 | Wait 20 seconds and reload the page | The page comes back. The `slt` has changed again |
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
| 34 | Open `/status.json` | `slt` is the one from check 30, and `ver` is the working firmware. A hung image was rolled back with no cable |
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
| 40 | `curl -s http://tef668x.local/status.json` and read `tun` | `prt` is `TEF6686`, `pch` is 102, and `fsi`, `frd` and `dr` are all false. That is what this part is |
| 41 | Read `xtl` and `xad` in the same reply | `xad` is 0 and `xtl` is `9.216 MHz`. A different crystal here means the radio is deaf, so it is worth reading every time |
| 42 | Flash again over the air, then read `xtl` again | Still `9.216 MHz`, not `not read`. The tuner is patched on every start, because after an update the chip keeps the old firmware's settings |

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
| 49 | Tune 846 and 1377 | Both receive. 1377 shows as `1377`, with nothing between the digits |
| 50 | Tune 102800 straight after a medium wave station | It receives properly again. If every FM station reads `sig` 8320 and `bw` 4, the chip is stuck on its AM side and the FM preset wake is broken |
| 51 | Alternate FM and MW five times | The last reading matches the first for the same station |

### The control API

| # | Do this | Expect |
|---|---|---|
| 52 | `curl -i -X POST -d 'khz=102800' http://tef668x.local/api/tune` with no session | `403` and `Enter the access PIN first.` |
| 53 | Sign in, then `-d 'khz=50000'` | `400` and `That frequency is in no band.` 50 MHz is between OIRT and shortwave |
| 54 | `-d 'khz=0'` and `-d 'khz=abc'` | `400` and a plain reason. Never a 500, and never a 200 that quietly did nothing |
| 55 | `-d 'khz=4294967295'` | `400`, not a tune to something absurd |
| 56 | `GET /api/state` with no session | `200` and JSON. Reads are open, writes are not |
| 57 | Compare `/api/state` and `/status.json` | Byte for byte the same document, apart from `hep` and `up`. They share one builder |
| 58 | `POST /api/step -d 'stp=2'` on FM | `200` and the frequency it landed on, not just `stepped 2`. Two steps of the current step size up |
| 59 | `POST /api/step -d 'stp=-1'` | `200` and one step back down. Same frequency as before check 58, less one step |
| 60 | `POST /api/band -d 'bnd=SW'` then straight away `POST /api/mode -d 'mod=MeterBand'` | Both `200`. Sent back to back, this used to refuse the mode because the band change had not been applied yet |
| 61 | `POST /api/mode -d 'mod=MeterBand'` while on MW | `400` and `that tuning mode is not available here`. Meter band is shortwave only |
| 62 | `POST /api/band -d 'bnd=AIR'` | `400` and the list of real band names |
| 62a | Tune 11900 on SW | Shows `11900`, not `11 900` |
| 63 | `POST /api/mode -d 'mod=Wobble'` | `400` and the list of real modes |
| 64 | `POST /api/volume -d 'db=99'` | `400` saying the range. Then `db=0` gives `200 volume 0 dB` |
| 65 | `POST /api/mute -d 'on=1'` then `on=0` | Audio stops and comes back. `mut` in `/api/state` follows |
| 66 | `POST /api/bandwidth -d 'khz=110'` then `khz=0` | `200 bandwidth 110 kHz` then `200 bandwidth automatic`. On AM, `khz=0` is refused |
| 67 | `POST /api/step-size -d 'khz=50'` on FM, then step once | The step moves 50 kHz, and `stp` in `/api/state` reads 50 |
| 68 | Send eight writes back to back with no pause | Every reply names the state that request reached. None of them names a frequency the radio is not on |
| 69 | Every `/api/*` write with no session | `403`. Check all eight, not one of them |
| 70 | On MW, `POST /api/step-size -d 'khz=9'` | `200 step 9 kHz`. Medium wave offers 9 kHz |
| 71 | On FM, `POST /api/step-size -d 'khz=9'` | `400` and `that band does not offer that step`. A refused command must never answer 200 |
| 72 | On MW, `POST /api/bandwidth -d 'khz=0'` | `400` and `that bandwidth is not allowed here`. There is no automatic bandwidth on AM |
| 73 | Time a `GET /api/state` against a `POST /api/step` | The write costs about 10 ms more than the read. If it costs 100 ms more, the radio task has stopped waking on its queue |
| 74 | `GET /api/settings` with no session | `403`. This one is not an open read, the struct holds the passphrase |
| 75 | `GET /api/settings` signed in | The stored `sid`, `pss` and `dpn`. Never the passphrase itself and never the PIN |
| 76 | `POST /api/settings` with no arguments | `400` and `Give ssid, or pin, or both.` |
| 77 | `POST /api/settings -d 'pin=12345'` and `-d 'pin=abcdef'` | `400` and `A PIN is six digits.` Nothing is saved |
| 78 | `POST /api/settings -d 'sid='` | `400`. An empty network name is refused before anything is written |
| 79 | `POST /api/settings -d 'pin=000000'` | `200`, the session ends, and the next write is `403` until you sign in again |
| 80 | `POST /api/settings -d 'sid=<your network>&pwd=<the passphrase>'` | `200`, then the radio reconnects and answers on the same address. **Needs someone at the radio.** A wrong passphrase here leaves it on its own access point, and only a person standing there can fix it |

### The cycle endpoint

The same four things the panel buttons do, over HTTP, so they can be checked without a person at the radio.

| # | Do this | Expect |
|---|---|---|
| 104 | `POST /api/cycle -d 'wht=band'` five times | LW, MW, SW, OIRT, FM in turn, each returning to the frequency that band was left on |
| 105 | `POST /api/cycle -d 'wht=bandwidth'` on FM, seventeen times | The whole FM list and back to automatic |
| 106 | Tune to 738, then cycle the bandwidth five times with no pause | Every value is 3, 4, 6 or 8 kHz. An FM width such as 56 here means the caller is working from a stale copy of the band |
| 107 | `POST /api/cycle -d 'wht=mode'` ten times on FM | Manual, Auto and Memory only. Never Meter band |
| 108 | The same on SW | Meter band appears |
| 109 | `POST /api/cycle -d 'wht=mute'` twice | `muted` then `unmuted`, and the audio follows |
| 110 | `POST /api/cycle -d 'wht=wobble'` | `400` and the list of things that can be cycled |
| 111 | `POST /api/cycle` with no session | `403` |
| 112 | Tune to 102800, then tune to 738, then cycle the band round to FM | FM 102.80, not 87.50. Leaving a band by tuning must remember it, the same as leaving it with the BAND button |
| 113 | Check the reply of every write | It names what changed, read back after the radio settled. A bandwidth change must not answer with a frequency |

### The knob, the buttons and the keypad

These need a person at the radio. Watch what the radio saw with `curl -s http://tef668x.local/api/state`, in the `inp` field: `clk`, `prs`, `lst` and `typ`.

| # | Do this | Expect |
|---|---|---|
| 81 | Turn the knob one click at a time | The frequency moves by the step size, one step per click, in the right direction |
| 82 | Turn it one click the other way | It comes back to where it was. Ten clicks out and ten back must land on the same frequency |
| 83 | Spin the knob fast | It moves much further per click. Six steps per click at full speed, against one when turning slowly |
| 84 | Turn the knob at power on without touching it first | Nothing moves until you actually turn it. The first reading only says where the knob is resting |
| 85 | Press the knob | Audio stops and `lst` reads `muted`. Press again and it comes back |
| 86 | Hold the knob for a second | `PUSH long`. Its menu is phase 4, so nothing else happens |
| 87 | Press BAND five times | LW, MW, SW, OIRT, FM in turn, and each band returns to the frequency it was left on |
| 88 | Hold BAND for a second | `BAND long`. The RDS screen is phase 4 |
| 89 | Press BW repeatedly on FM | The full list in turn: automatic, 56, 64, 72, 84, 97, 114, 133, 151, 168, 184, 200, 217, 236, 254, 287, 311 kHz, then back to automatic |
| 90 | Press BW repeatedly on MW | 3, 4, 6, 8 kHz and round again. There is no automatic on AM |
| 90a | Listen to each of those four on a strong medium wave station | 4 kHz is the clearest, which is the default a band change picks. 3 kHz is muffled, 8 kHz lets the neighbour in |
| 91 | Press MODE repeatedly on FM | Manual, Auto, Memory, Manual. Meter band is skipped, because it is shortwave only |
| 92 | Press MODE repeatedly on SW | Meter band appears in the cycle |
| 93 | Tap MODE four times quickly | Four mode changes, not two. A quick pair must not be swallowed as one double press |
| 94 | Press every digit 0 to 9 in turn | `typ` grows by the right digit each time. A wrong digit means the line map is wrong, no digit at all means the switch or the wiring |
| 95 | Press DX | `lst` reads `DX`. This is expander line 2, the one the old firmware skips |
| 96 | Type 1028 then enter, on FM | Tunes FM 102.80. The digits are read in the band you are already on |
| 97 | Type 738 then enter, on MW | Tunes MW 738 kHz |
| 98 | Type 1000 then enter on MW, then the same on FM | MW 1000 kHz the first time, FM 100.00 the second. The band in use decides which reading was meant |
| 99 | Type nine digits | The ninth is refused and `lst` reads `too many digits`. The first eight are kept |
| 100 | Type three digits and wait five seconds | The part typed number is dropped, so the next person does not continue it |
| 101 | Type 28000 then enter | `28000 is in no band`, and the radio does not move |
| 102 | Hold two keys at once | Nothing is typed. There is no way to tell which one was meant |
| 103 | Turn the knob while pressing keys | Both work. They are on the same I2C bus as the tuner, so a signal reading of exactly 0 for level, bandwidth and modulation at once means the bus lock has been lost |

### The display

The panel needed four things that could not be read off a datasheet, and each
fails in a way that still looks like a working screen. Check all four.

| # | Do this | Expect |
|---|---|---|
| 114 | Look at the screen at power on | The boot text, then the band, frequency, signal and stereo. Not a blank screen and not a white one |
| 115 | Read the text | It reads normally. Mirrored text means only one of `MX` and `MY` is set, which is a flip rather than a rotation |
| 116 | Check the screen is the right way up | The band is top left. Upside down means the other landscape rotation |
| 117 | Look at the edges | The picture fills the glass. Everything squeezed against one side means the width and height are the wrong way round |
| 118 | Check the background is black and the band name is amber | An amber that looks blue and a background that looks white mean the colour inversion is off |
| 119 | Check the background is black rather than dark blue | A blue background means the power and gamma registers were not sent. Pure black, white, red, green and blue all look right even when they are missing, so this is the only check that catches it |
| 120 | Tune across the band with the knob | The frequency follows without flicker, and nothing of the previous number is left behind when it gets shorter, such as 100.00 to 99.90 |
| 121 | Switch FM to MW | The unit changes from MHz to kHz and moves to sit against the shorter number |
| 122 | Tune 1377 on MW | Reads `1377`, with nothing between the digits |
| 123 | Press the knob to mute | `muted` appears at the foot, and clears when unmuted |
| 124 | Change the tuning mode | The mode at the top right follows |
| 125 | Pull the antenna out and push it in while watching the signal | The dBuV reading follows |
| 126 | Leave it on a station for a minute | The signal reading updates and nothing else flickers. Only what changed is redrawn |
| 127 | Power the radio on and watch and listen | The screen lights up before any sound comes out. Audio first reads as a fault rather than as a fast start |
| 128 | Tune from 108.00 down to 99.50, then to 738, then to 162 | Nothing of the old number or the old unit is left behind. This is the check that failed as `kHz MHzzHz` |

### Things this build cannot be tested for

Not written yet, so do not look for them: touch, RTC, battery reading, telemetry, RDS, memory channels, the volume AGC, and seek in Auto mode. Selecting Auto is possible, but the knob still steps manually there. That is issue 15.

### The volume knob

| # | Do this | Expect |
|---|---|---|
| 133 | Turn the knob from one end to the other | The volume follows all the way. At the very bottom it goes silent, and just above that it is quiet but audible |
| 134 | Listen while turning it | The sound stays continuous. Breaking up means the tuner is being muted and retuned for a volume change, which it must not be |
| 135 | Leave the knob alone for a minute and watch `pdb` in `/api/state` | It does not move. The converter jitters by a few counts and acting on that would send a volume command for ever |
| 136 | Set the knob about a third up, then restart the radio | It comes up at that volume, not at full and then jumping when the knob is first touched |
| 137 | Change the volume over HTTP, then turn the knob | The knob wins, which is what a physical control has to do |

There is no analogue S-meter on this unit, so there is nothing to watch on pin 27. The pin is driven anyway.

### The FM features

None of these turns itself on except the bandwidth extension, which follows the signal.

| # | Do this | Expect |
|---|---|---|
| 153 | `GET /api/state` on FM | `ims`, `eq` and `mno` are all false. The same as the radio this replaces ships |
| 154 | `POST /api/fm -d 'ims=1'` on a station suffering multipath | Audibly cleaner. This is the feature with its own badge on the old radio |
| 155 | `POST /api/fm -d 'eq=1'` | The channel equalizer. Listen for a difference on a marginal station |
| 156 | `POST /api/fm -d 'mno=1'` then `mono=0` | Stereo stops and comes back. Different from the automatic blend, which drops to mono on its own as a signal weakens |
| 157 | `POST /api/fm -d 'ims=1'` on an AM band | `400` and `that only works on FM`. Not the message for a band that does not exist |
| 158 | Turn a feature on, change band and come back | It is still on, and still doing something. Crossing to AM and back puts the chip through its active mode, so they have to be re-sent |
| 159 | `POST /api/fm -d 'ims=5'` and with no arguments | `400` both times, and nothing changed |
| 160 | Watch `wid` on a strong local station | True. It needs a smoothed signal above 30 dBuV and a ratio above 15 dB |
| 161 | Watch `wid` on a weak station | False. A wide filter on a weak signal is mostly the neighbours |
| 162 | Tune from a strong station to an empty frequency and watch `wid` | It changes once, not several times a second. The readings are smoothed before anything decides on them |
| 163 | Compare `snr` against `sig` and `usn` | It rises with signal and falls with noise. The chip does not report it, so it is worked out from those two |

### The fades

| # | Do this | Expect |
|---|---|---|
| 170 | Power the radio on and listen | The volume comes up over about a second and a half rather than starting at full |
| 171 | Turn the knob right down, then power on | It comes up quiet, at the knob. Not loud and then quiet |
| 172 | Turn the knob during the fade at switch on | It follows. The fade lands wherever the knob now is rather than fighting it |
| 173 | Change band | The sound comes back over about half a second, not all at once |
| 174 | Type a frequency and press enter | The same gentle return |
| 175 | Turn the tuning knob one step at a time | No fade. Stepping the dial must stay instant, or the whole dial feels slow. A fade that restarts on every click never finishes either |
| 176 | Listen closely to a band change | It slides. If it arrives in a handful of jumps the volume is not being moved often enough during the fade |

### Weak signal handling and the noise blankers

All of this ships switched off, matching the radio it replaces. These checks are about proving the controls work, not about whether the features should be on.

| # | Do this | Expect |
|---|---|---|
| 164 | `GET /api/state` on a station | `cut`, `bld` and `hbl` are what the chip is applying now, not what it was told |
| 165 | On a strong station with everything off | All three read 0. The chip does nothing until there is no signal left |
| 166 | `POST /api/fm -d 'cut=40&bld=40&hbl=40'`, then tune to a station below 40 dBuV | `cut` and `bld` both move. `hbl` needs its ceiling, which the driver sends |
| 167 | Tune to a station above 40 dBuV | All three back to 0. A strong signal gets no processing |
| 168 | `POST /api/fm -d 'anb=80'` on medium wave | Accepted. This is a percentage, not a level in dBuV, and the usable range is 50 to 150 |
| 168a | `POST /api/fm -d 'anb=20'` | `400`. Between 1 and 49 is neither off nor usable, so it is refused rather than accepted into doing nothing |
| 168b | `POST /api/fm -d 'cut=40'` alone, then read `/api/state` | `bld` and `hbl` keep the values they had. Changing one of a group must not switch off the others |
| 169 | Judge any of these by ear | Blind, and without touching the antenna. A measurement and an unblinded impression both gave the wrong answer on the noise blanker |

### The squelch

Three modes, and the mode also decides what the front pot does. There is one knob, so it is the volume or the squelch and never both.

| # | Do this | Expect |
|---|---|---|
| 138 | `GET /api/state` on a fresh radio | `sql` is `Off`. Nothing goes quiet until you ask for it |
| 139 | `POST /api/squelch -d 'mod=auto'` on a station | `sqo` true, audio unchanged |
| 140 | Tune to an empty frequency | Goes quiet after about a second, and `sqo` turns false |
| 141 | Tune back to the station | Opens at once. Opening is not held, or the front of every station is clipped as the dial crosses it |
| 142 | Tune slowly across a band in auto | It opens on each station and stays quiet between. It must not chatter on and off on a weak one |
| 143 | `POST /api/squelch -d 'mod=manual'`, then turn the knob down | Opens. `sqa` follows the knob |
| 144 | Turn the knob up past the station's signal level | Shuts. Compare `sqa` against `sig` |
| 145 | Turn the knob to the very bottom | Always open, whatever the signal. Without this there is no way to listen to a weak station on purpose |
| 146 | `POST /api/squelch -d 'mod=manual&thr=250'` | `400`. The knob owns the threshold, and a second way to set it answered with a number the knob replaced a moment later |
| 147 | Back to off or auto | The volume goes at once to what the knob is pointing at, without waiting for the knob to be moved |
| 147a | Into manual from off | The threshold goes at once to what the knob is pointing at, not to whatever it was last time |
| 148 | Mute with the knob press while the squelch is open | It goes quiet and stays quiet. A deliberate mute must win over an open squelch |
| 149 | Unmute while the squelch is shut | It stays quiet, because the squelch is shut. Turn the squelch off and it plays |
| 150 | `POST /api/squelch -d 'mod=wobble'` and with no arguments | `400` and the list of modes, both times |
| 151 | Pull the antenna out in auto on a weak station | It closes. Push it back in and it opens |
| 152 | Tune onto a station in auto and listen to the first moment | No clipped opening. The squelch acts on the reading that opened it, not a tenth of a second later |

### Settings that survive a power cycle

Everything the radio is set to now lives in one place. `POST /api/save` writes it to NVS. `POST /api/settings` takes only the four that can be read at start up and nowhere else.

| # | Do this | Expect |
|---|---|---|
| 177 | `GET /api/settings` on a fresh radio | FM 87.5 to 108, 9 kHz medium wave, squelch `Off`, band `4`, every FM feature 0, de-emphasis 50, AM width 4 |
| 178 | Tune to a station, change a few FM settings, then `POST /api/save` | It says where it will come up and which squelch mode |
| 179 | Power cycle | It comes up on that station, on that band, with those settings. The old default of 104.0 MHz is gone |
| 180 | Listen to the first second after that power cycle | Still one fade up and no burst of noise from anywhere else. The frequency is set before the task unmutes |
| 181 | `GET /api/settings` after the save | Every field matches what `/api/state` said before the power cycle, except the volume, which is not stored |
| 182 | Change the volume knob, `POST /api/save`, power cycle | It comes up at whatever the knob is pointing at, not at a stored number |
| 183 | `POST /api/save` while on an AM band at 6 kHz, then power cycle | It comes up on that AM band at 6 kHz |
| 184 | `POST /api/settings -d 'spc=1'` | Accepted, and the reply says it needs a reboot |
| 185 | Reboot, then step across medium wave | 10 kHz steps. Before the reboot it is still 9 |
| 185a | With 738 kHz saved, change the spacing to 10 kHz and reboot | It comes up on 738, where the station is. Changing a setting on the radio does not move a transmitter, and medium wave has a 1 kHz step so 738 is a frequency the radio can reach |
| 185b | Fine tune medium wave to 737 with the 1 kHz step, save, reboot | Still 737. The step in use is not stored, so a radio that snapped to the default step would undo a deliberate fine tune every restart |
| 186 | `POST /api/settings -d 'rgn=1'` then reboot on FM | The band is 76 to 95 MHz. Stepping past 95.00 wraps round to 76.00 and back the other way, which is what bandStepUp is documented to do |
| 187 | With region 1 stored and a station at 102.8 saved, reboot | It comes up somewhere inside 76 to 95, not on a frequency the band no longer has |
| 188 | `POST /api/settings -d 'edr=1'` then reboot | The tuning knob counts the other way |
| 189 | `POST /api/settings -d 'rgn=9'` | `400`, and nothing is stored. Read it back to check |
| 190 | `POST /api/settings` with no arguments | `400` and the list of what it takes |
| 191 | `POST /api/fm -d 'dem=75'` on FM | Accepted, and **proven audible on 13 September 2026**. Four blind A/B pairs on an empty FM channel, off against 50 us, all four called correctly, including one pair with the order reversed so an order effect cannot explain it. See rows 191a and 191b for how to run it, because the method matters more than the patience |
| 191a | To hear de-emphasis, listen to hiss and not to music | Tune to an **empty** channel, not a station. De-emphasis is a treble shelf, and the loudest treble a radio makes is hiss. With it off the hiss is plainly brighter and louder. An earlier attempt used music on a station and heard nothing twice, which is what the internal speaker does to the top end of a song |
| 191b | Before any listening test, check `cut`, `bld` and `hbl` in `/api/state` | All three should read 0, or something else is changing the audio while you listen. The first de-emphasis test was run with the weak signal high cut start at 35 dBuV on a station reading 33 to 37, so the treble roll off was switching itself on and off during a test about treble. Switch the three weak signal starts off for the duration |
| 192 | `POST /api/fm -d 'dem=60'` | `400`. Only 50, 75 and off are real |
| 193 | `POST /api/fm -d 'dem=75'` on a medium wave band | Accepted. It belongs to the country, not to the band you are on |
| 194 | Save with de-emphasis 75, power cycle, listen to FM | It is written again on every start, which `GET /api/state` confirms, and it is audible by the method in 191a |
| 195 | Open `/radio` | Three cards: Listening to, Reception, Band plan and knob. Every box shows what the radio is set to now, not the defaults |
| 196 | Change a box in Reception and press Apply | The line at the top of the page says what the radio reached. The page does not reload |
| 197 | Open Weak signal and noise blankers, set High cut to 10 and press Apply | The line goes red and says 0, or 20 to 60 |
| 198 | `POST /api/fm -d 'ims=1'` and `-d 'cut=40'` on medium wave | `400` both times, naming which argument failed and then what the radio is actually set to. Not from the page: the FM only controls are not rendered on an AM band, which is 208 |
| 199 | Press Keep these settings, then power cycle | The radio comes up as it was left |
| 200 | Flash this build over a radio that has never saved anything | It still comes up on 104.0 MHz. An update must not move where the radio starts |
| 201 | Set a comfortable volume, put the squelch in manual, save, then power cycle | It comes up at that volume, not at whatever the squelch knob maps to. In manual the knob is the squelch and never touches the volume |
| 201a | Do the same with the squelch knob at the very bottom | Still that volume. The knob position must not decide the volume in this mode |
| 202 | From that state, put the squelch back to off or auto | The volume goes at once to what the knob is pointing at |
| 202a | On a stereo station, `POST /api/fm -d 'mno=1'` and look at the radio's screen | The screen stops saying stereo. It used to keep saying it, because forcing mono leaves the pilot where it was and the screen was reading the pilot |
| 202b | Read `/api/state` in that state | `st` false, `plt` true. One says what you are hearing, the other what the station is sending |
| 202c | `POST /api/fm -d 'mno=0'` | Both go back to true, and the screen says stereo again |
| 203 | On FM, `POST /api/bandwidth -d 'khz=4'` | `400`. 4 kHz is an AM width, and on FM it pins the filter narrower than a station and the radio goes quiet and reads as broken |
| 204 | On FM, `POST /api/bandwidth -d 'khz=110'` | `400`. Between two real widths is not a near miss |
| 205 | On medium wave, `POST /api/bandwidth -d 'khz=114'` and `khz=0` | `400` both times. 0 is the FM automatic setting and the AM side has no answer for it |
| 206 | Open `/radio` while on FM | The Reception card has no bandwidth box. On FM the tuner picks the width itself |
| 207 | Open `/radio` while on medium wave | Reception has a bandwidth select of 3, 4, 6 and 8, showing the width the radio is on |
| 208 | Open `/radio` while on medium wave and look at Reception | De-emphasis and both blankers are there. iMS, EQ, mono and the three levels are not, with a line saying they appear on FM |
| 209 | Set de-emphasis to 75 from the page while on medium wave | Accepted. Before this it took the whole post down with it |
| 210 | `GET /api/state` after changing de-emphasis | `dem` says what the tuner is set to. `fnb` and `anb` are there too |

### The volume knob and the S-meter

| # | Do this | Expect |
|---|---|---|
| 250 | Open `/radio` and look at The volume knob card | It says whether this knob has been measured, and with what travel |
| 251 | Press Measure, then turn the volume knob | The volume does not change while measuring. That is deliberate: finding the loud end stop should not sweep the volume to full |
| 252 | Watch `pcl` in `/api/state` while turning | `min` falls and `max` rises as the knob reaches further |
| 253 | Turn to both ends, press Done | It says the travel it measured and that it is stored. On this unit that is 0 to 4095, the whole converter |
| 254 | Turn the volume knob after that | It works again, across the whole travel, and the bottom of the travel still mutes |
| 255 | Press Measure and Done without turning anything | `400`, and it says the knob did not sweep. Nothing changes |
| 256 | Press Measure, sweep, then press Cancel, then press Done | Nothing is stored. Cancel has to forget the sweep, not just stop it |
| 257 | Press Measure and walk away for three minutes | The knob starts working again on its own. A calibration left open must not leave the radio with no volume control |
| 258 | Power cycle after a successful Done | The travel is still the measured one |
| 259 | Put the squelch in manual after calibrating | The whole knob is usable as a threshold, not just part of it |
| 260 | Nothing to do. This unit has no analogue meter fitted, confirmed by looking on 12 September 2026 and again on 13 September. Pin 27 is still driven, because the pin map comes from a firmware that runs on boards in this family that do have one | |

### iMS and the channel equalizer, from the button

Both are FM reception features, and both are about multipath: the signal arriving by more than one path, direct plus reflections. iMS suppresses the smearing that causes. EQ corrects the frequency response the reflections notch out. Neither is a tone control.

MODE long press walks the four combinations. BAND long and the knob held long are reserved for the RDS screen and the menu, both of which arrive in phase 4 and neither of which does anything yet.

**On a clean strong station expect to hear nothing.** That is the right answer, not a broken feature. The difference shows on a station that is suffering.

| # | Do this | Expect |
|---|---|---|
| 261 | Long press MODE four times on FM | It walks off, iMS, EQ, both, and back to off. The panel says which |
| 262 | Short press MODE | Still the tuning mode, unchanged. The long press must not disturb it |
| 263 | Watch the panel through the cycle | `iMS` and `EQ` appear and disappear beside `stereo`. Nothing is pushed off the row |
| 264 | Long press MODE on medium wave | It says it only works on FM. Nothing changes, and the panel shows neither |
| 265 | `POST /api/cycle -d 'wht=features'` | The same four states in the same order, and the reply names which one |
| 265a | Mute the radio with iMS and EQ on | `muted` is the only thing in the fault colour. iMS and EQ stay in their own colour, because they have not gone wrong |
| 266 | Set iMS on the web page, then long press MODE | The cycle carries on from what the page set, not from where it was before |
| 267 | Turn iMS on and off on a station with obvious multipath, blind | Judged by ear, order picked at random, revealed afterwards. Needs a station that is actually suffering. See the note below for how to make one |
| 268 | The same for EQ | The same rules. A null result is a result: record it as unproven rather than shipping it as working |
| 269 | Turn both on, save, power cycle | They come back on, and the panel says so |

**How to get a station with multipath, and how to run these two.** On a normal day every local station here reads a multipath of 29 to 30, which is the clean end, and on those two the right answer is that both features are inaudible. Collapsing the telescopic aerial gives the conditions: the direct path drops while the reflections stay, and on 13 September 2026 that took 106.4 MHz from signal 375, noise 9, multipath 29 to signal 35, noise 76, multipath 124, still in stereo and still listenable. Four of the six local stations collapse into noise at that point and only 101.9 and 106.4 stay usable.

Balance the order by hand. Two trials with the feature on A and two with it on B, shuffled, not four independent random picks. Four independent picks came out the same way three times running on 13 September 2026, which is a 1 in 8 chance, and a score built on that cannot tell the feature apart from a preference for whichever was heard second.

Both were run that way on 13 September 2026 on 106.4 with the aerial down. iMS: the iMS-on side picked in 3 of 3 judged trials, one trial called the same, picks split across both positions. EQ: the EQ-on side picked in 4 of 4, picks split across both positions.

### Sounds and fades

The polish from ticket 19: nothing here changes what the radio receives, all of it changes how using it feels. Every one of them can be switched off, on `/radio` under Sounds and fades.

**The ramp is only noticed when it is missing.** Set it to Off, listen, set it back, and the difference is a click you stop hearing.

| # | Do this | Expect |
|---|---|---|
| 270 | Press mute on a station with the ramp at 120 ms | It goes quiet over about a tenth of a second, not with a click |
| 271 | Unmute | It comes back on a ramp too, over the same length, not as a step from silence to full |
| 272 | Set the ramp to Off and press mute | It cuts instantly, which is what Off has to mean |
| 273 | Put the squelch in auto and tune to an empty frequency | It closes on a ramp rather than clicking shut |
| 274 | Step back onto the station one click at a time, not by typing the frequency | It opens at once. Opening is not ramped, or the front of every station is soft. Arrive by a step: a typed frequency and a band change both count as a jump and get a fade in of their own, which sounds the same and hides what this row is asking about |
| 275 | Turn the key beeps off, then press BW to change the bandwidth on a station | No click. The ramp takes the audio down, the filter changes while it is muted, and it comes back up. Turn the beeps off first or set the bandwidth over the API: with Beep on set to Every press the button sounds by design, and that beep is easily read as the click this row is looking for |
| 276 | Press BW while the radio is muted | Still silent. It must not unmute to cover its own click |
| 277 | Set Beep on to Keypad only, press a keypad digit | A short tick, about 2000 Hz. Every key, including ones that go on to be refused |
| 277a | With Keypad only, long press MODE | Silent. That setting means the keypad and nothing else |
| 277b | Set Beep on to Keypad and long presses, long press MODE | A noticeably longer tone, about four times the keypad tick |
| 277c | With that setting, short press BAND or BW | Silent. The band or the filter changing is the feedback, so a beep adds nothing |
| 277d | Set Beep on to Every press, press BAND | A short tick |
| 277e | `POST /api/settings -d 'bpk=4'` | `400`. There are four modes, 0 to 3 |
| 277f | `POST /api/beep -d 'ms=2000'` | A two second tone. This exists because fifty milliseconds is not long enough to tell "it did not sound" from "it sounded and I missed it" |
| 278 | Turn the key beep on and mute the radio, then press a key | Silent. The beep goes through the same output mute, and beeping at somebody who asked for quiet is the wrong way round |
| 279 | Turn the band edge beep on, and turn the dial past the top of a band | It beeps as it wraps to the bottom, and not on any ordinary step |
| 280 | Seek across the band with the edge beep on | No beeping while it hunts. A seek is exempt from the ramp too, or 120 ms on every channel would be most of the settle time |
| 281 | Press Reboot on the System page while listening | The audio goes down and mutes before it restarts. No click at the end |
| 282 | Upload firmware from the System page while listening | The same. The reboot path is shared |
| 282a | Upload over espota, which needs the PIN and the same subnet. See the note below | The same as 282. This is a different path from `/update` and needed its own row |
| 282b | Turn the knob or press a button during the second between the ramp and the restart | Nothing comes back. Once the hush has run the radio stops writing to the tuner, or the knob would put the volume straight back and two tasks would be on the I2C bus at once |
| 282c | Start an espota upload and kill it partway, then listen | The audio comes back on its own, on a fade. An update hushes the radio the moment the transfer starts, and a transfer that breaks has no reboot coming to undo that. Check by ear: `mut` and `hmu` in the state document both read false either way, so the document cannot tell you |
| 283 | Set all five off, the ramp, both beeps, the chime and the panel fade, then use the radio | It behaves exactly as it did before this change. Anything that cannot be switched off is a mistake. The chime and the panel fade are the two that ship on, which is decisions 27 and 28. The panel dim ships off, so there is nothing to switch off there |
| 284 | Change any of them and power cycle | They come back as set |
| 285 | Power cycle with Chime at start up on | A single longer tone as it comes up, before any station audio. It cannot come earlier than that: the tone generator is inside the tuner, so there is nothing to beep with until the patch has gone in |
| 286 | Listen to what follows the chime | The station fades in afterwards, as it always did. The chime must not leave the audio muted or leave the path on the generator |
| 287 | Set Chime at start up off and power cycle | Silent until the station arrives. Check it again after that power cycle: a switch that says Saved and changes nothing is how this one went wrong the first time |
| 288 | Power cycle with the volume knob right down | The chime is quiet too. It is played at the volume the radio is about to come up at, so it also tells you how loud the knob is set |
| 289 | Power cycle with the tuner disconnected or failing to start | No chime, and the radio still comes up and is reachable. A chime that needs the tuner must not become a reason the radio does not boot |

**The espota command.** `pio run -e ats125 -t upload --upload-port <ip>` on its own fails with `Authentication Failed`. The OTA listener is given the access PIN as its password (`main.cpp:286`), and `pio run` has no flag to pass one, so call the tool directly:

```bash
python3 ~/.platformio/packages/framework-arduinoespressif32/tools/espota.py \
  -i <radio ip> -p 3232 -a <access PIN> -f .pio/build/ats125/firmware.bin -r
```

That gets past authentication, and then the radio opens a connection back to the machine running the command, at the address and port espota advertises. If that connection never arrives the tool waits ten seconds and reports `No response from device`. On 13 September 2026 it got that far and no further, with the radio on 192.168.30.152 and the machine on 192.168.10.118: the local firewall permits incoming connections to python and stealth mode is off, and the firewall between the two subnets forwards one way only, so the radio cannot open a connection back. `POST /update` has no such requirement and works from anywhere on the network, which is why it is the one the other rows use.

What this row adds over 282 is one thing only: that ArduinoOTA's `onStart` callback fires. Both routes call the same `radioHush()`, at `ota_service.cpp:25` and `web_update.cpp:2981`, so everything after that moment is shared code. Run it from a machine on the radio's own subnet.

### The panel light

The rest of ticket 19, and the same rule: nothing here changes what the radio receives. The controls are on `/radio` under The panel light.

**Start with the API half.** `GET /api/state` now carries `pnl`, with `lit` for how bright the panel is and `dim` for whether it has been left alone long enough to drop. A dim and a wake are both silent, so without that the only way to check either is to sit and watch the radio.

```bash
R=http://tef668x.local
curl -s $R/api/state | python3 -c 'import json,sys; print(json.load(sys.stdin)["pnl"])'
```

| # | Do this | Expect |
|---|---|---|
| 295 | Power cycle with Fade up at start on, and watch the panel | The light comes up over about a second, not as a snap. The boot banner is already on the glass as it does, so the fade reveals it rather than the text filling in after. A second is measured, not chosen: four tenths could not be told from the light switching on in a blind comparison. See the backlight section of HARDWARE.md |
| 295a | If you cannot tell 295 from a snap, do it blind | Switch Fade up at start off for one boot and on for the next, without looking at which is which, and say which one faded. An impression of a fade you were expecting to see is not evidence. This is how the four tenths of a second version was caught doing nothing visible |
| 296 | Set Fade up at start to Off, save, power cycle | The light snaps on. That is what Off has to mean. This one is read at start up, so it needs the power cycle to show |
| 297 | Set Brightness to 30 and apply | The panel dims straight away, while you are looking at it. A brightness that only took effect at the next start could not be chosen at all |
| 298 | Set Brightness back to 100 and apply | Full again, at once |
| 299 | `POST /api/settings -d 'blt=4'` | `400`, and nothing stored. 5 per cent is the floor, and it is measured rather than round: it is the lowest setting at which the frequency could still be read off this panel in daylight. Below it the only control for a panel nobody can read is the page that has just gone dark |
| 300 | `POST /api/settings -d 'blt=5'` then `-d 'blt=100'` | `200` both times. The boundary itself is allowed, one below is not |
| 301 | `POST /api/settings -d 'bds=241'` | `400`. Four minutes is the longest delay |
| 302 | Set Dim after to 10 seconds and leave the radio alone, with Dimmed at the default 20 | After ten seconds the panel drops to about a fifth brightness, over about a second. Gradual, not a step |
| 302a | Read the frequency on the dimmed panel | Still comfortable to read, not merely visible. 20 per cent sits just above the comfortable floor measured on this panel, which is 18. See the backlight section of HARDWARE.md |
| 303 | Read `pnl` while it is dimmed | `dim` is true and `lit` is the Dimmed setting. This is the check that tells a dim too subtle to see from one that never ran |
| 304 | Turn the knob one click | Full brightness comes straight back, with no fade. Read `pnl` again: `dim` is false and `lit` is back to the Brightness setting |
| 305 | Repeat 302 and wake it with a panel button instead | The same. BAND, BW, MODE and the knob press all count |
| 306 | Repeat 302 and wake it with a keypad key | The same |
| 307 | Repeat 302 and wake it by turning the volume pot | The same. Only a real turn counts: the pot is read twenty times a second and never sits perfectly still, so counting every reading would mean the radio was never left alone |
| 308 | With Dim after at 10, keep turning the knob every few seconds for a minute | It never dims. Every input starts the clock again |
| 309 | Press a key while the panel is partway through dropping | It goes straight back to full. It must not carry on darkening under the hand of the person pressing |
| 310 | Set Dimmed to 0 and let it dim, then press any key | The panel goes completely dark and comes straight back. Nothing is stranded by it, which is why the dim level has no floor and the brightness does |
| 310a | Leave the radio alone for a minute with Dim after at 0, then set it to 10 and apply | The panel stays full for ten more seconds and then dims. The delay is measured from when you changed it, not against a radio that was already idle. Setting it from the browser would otherwise drop the panel the instant you pressed Apply |
| 310b | With Dim after at 10, wait nine seconds and change it to 30 | It dims thirty seconds after the change, not one second after it |
| 311 | While the panel is dimmed, set Dim after to 0 and apply | The panel comes straight back to full. Nothing else would lift it: a dimmed panel is only raised by an input, and changing a setting from a browser is not one |
| 312 | While the panel is dimmed, change Dimmed to 60 and apply | It moves to the new level and stays dimmed |
| 313 | Set Dim after to 0 and leave the radio for five minutes | It never dims. 0 means never, and that is what a radio nobody has told otherwise ships with |
| 314 | Change every panel setting, save, power cycle | They all come back as set |
| 315 | Power cycle a radio that was on the firmware before this one | Brightness 100, Dimmed 20, Dim after 0, Fade up on. A stored settings blob from version 6 is four bytes shorter, and what it never had has to come back as the default rather than out of the old padding |

### The smoothed signal meter

`GET /api/state` now carries both readings inside `tuner`: `sig` is the level exactly as it came off the chip, and `sav` is the same level smoothed. The panel and the analogue meter show `sav`. A sweep wants the reading it took, and a person wants the signal.

| # | Do this | Expect |
|---|---|---|
| 316 | Tune to a steady FM station and watch the number on the panel | It reads as whole dBuV with no decimal point, and it sits still. Measured on FM 106.40 with the signal steady: the raw reading swings about 3 dB, the smoothing takes that to about 1.6, and whole dB with hysteresis gives one change in twenty four seconds. Smoothing alone was not enough, because a decimal place moved the last digit ten times a second whatever the number underneath was doing |
| 316a | If the number does move, check whether the signal is moving first | `pnl.sdb` is what the panel shows and `tun.sav` is the smoothed level behind it. Poll both. The number following a signal that really moved is the meter working, not failing. On the same station an hour later `sav` ranged 28.6 to 33.6 dBuV on its own and the panel changed eleven times in thirty two seconds, correctly. Only a number moving while `sav` stays inside about a dB is a fault |
| 317 | Nothing to do on this unit | The smoothed level also drives the analogue S-meter on pin 27, but **no meter is fitted here**, checked by looking at the board on 12 September 2026 and recorded in HARDWARE.md. The pin is driven anyway, because the pin map comes from a firmware that runs on boards in this family that do have one. So the needle half of this change is written but unproven, and it stays unproven until a board with a meter is in hand |
| 318 | Poll `sig` and `sav` together for ten seconds | `sig` jumps about and `sav` follows it slowly. If the two are always equal the smoothing is not running |
| 319 | Tune from a strong station to an empty channel, such as 106.40 then 105.00 | The panel number falls to the new level at once and does not sit at the old station's figure. Use an empty channel, not another station: two stations of similar strength prove nothing, and this row was first run between two that both read about 24 dBuV. An empty FM channel here reads about -10 dBuV with the noise in the hundreds |
| 320 | Change band from FM to medium wave and back | The same. The average is started again on every retune and at the end of a seek, so the first reading from the new station is taken as the answer rather than averaged in with the old one. For the tenth of a second before that reading arrives, `sig` and `sav` both still describe where the dial was, which is deliberate: a zero would read as a real level of 0.0 dBuV |
| 321 | Seek across the FM band | The number moves as the sweep passes stations, and settles quickly once it stops |
| 322 | Tune to an empty channel and read `sig` and `sav` | Both sit near -10 dBuV rather than holding the last station's figure. A failed read is a different thing and shows `no reading` on the panel, but an I2C read cannot be made to fail on demand, so that path is covered by the tests rather than here. What this row checks is that a reading which arrived and happens to be low is shown as low |

### Every band remembers its own settings, and the radio saves itself

Ticket 24. Two things that go together: each band keeps what it was left set
to, and that gets written down without anybody asking.

**Start with the API half.** `GET /api/state` now carries `asv`, with `n` for
how many automatic saves have been written since boot, `dif` for whether what
the radio is set to differs from what is stored, and `due` for how long until
the next one in milliseconds. A save that never happens and one that happens
constantly both look the same from outside, and the second only shows up years
later as a worn out sector.

```bash
R=http://tef668x.local
curl -s $R/api/state | python3 -c 'import json,sys; print(json.load(sys.stdin)["asv"])'
```

| # | Do this | Expect |
|---|---|---|
| 323 | On medium wave set the width to 6 kHz, change to FM, change back | Still 6 kHz. This is the defect the ticket was opened around: the width was thrown away by the band change while `/api/settings` went on reporting it, so the radio ran 4 while everything said 6 |
| 324 | On medium wave set the step to 1 kHz, go to FM, come back | Still 1 kHz. FM has its own step of 100 and keeps it |
| 325 | On shortwave set the tuning mode to Meter band, go to FM, come back | Still Meter band. It is the one mode only shortwave has, so it is also the one that must not follow you to another band |
| 326 | Set a different width on medium wave and on shortwave, then move between them | Each keeps its own. One stored width used to mean a choice made on medium wave followed you to shortwave |
| 327 | Go to a band you have never used on this radio | It takes that band's own default, not the last band's. An unset band stores a zero, and zero means default for the frequency, the width, the step and the mode alike |
| 328 | Set up three bands differently, wait for `asv.n` to go up, then power cycle | Every band comes back exactly as you left it, and the radio comes up on the band and frequency it was on |
| 329 | Tune to a station, wait about ten seconds, power cycle without touching anything else | It comes up on that station. Nothing had to be saved by hand. This is the row a person would actually ask for |
| 330 | Tune to a station and power cycle within a second or two | It comes up on the previous station. Ten seconds of quiet is the price of not writing to flash on every click, and this row exists so that is a known limit rather than a surprise |
| 331 | Watch `asv.due` count down after you stop tuning | It falls to 0 over about ten seconds and then `n` goes up by one and `dif` goes false |
| 332 | Spin the dial across a band, then stop | `n` goes up by exactly one, not once per channel. Measured over the API: twenty retunes in ten seconds cost one write |
| 333 | Start a seek and watch `asv` while it runs | `n` does not move while `skg` is true. The dial moves every 50 ms during a seek and none of those channels is a station anybody chose |
| 334 | Let the seek stop on a station and wait | It saves that station about ten seconds later, like any other tuning |
| 335 | Tune away from a station and back to it again | `dif` goes true and then false, and `n` does not move. There is nothing to write when you end up where you started |
| 336 | Press Keep these settings by hand, then watch `asv` | `n` does not jump straight afterwards. A manual save starts the wait again, so the two do not both write |
| 337 | Change the FM band plan or the medium wave spacing, then go to a band whose stored step is no longer offered | The band takes its own default step rather than walking off the channel grid. The band plan can move under a stored value |
| 339 | Turn the volume knob, leave it alone for twenty seconds, and watch `asv` | `n` does not move. The volume follows the knob, so it is deliberately not a reason to write. Decision 26 says the stored volume is read back in manual squelch only |
| 340 | Power cycle without touching anything, and watch `asv` for twenty seconds | `n` stays 0 and `dif` stays false. What comes out of the settings has to go back in unchanged, or the radio writes to flash on every start. Do this a few times with the volume knob in different places: the start volume is mapped from one pot reading with no hysteresis, so a knob near the boundary between two dB is the case that used to make it write |
| 341 | In manual squelch, set a volume, wait, power cycle | It comes up at that volume. This is the one mode where the stored volume is read, so it is also the one mode where it still has to be kept |
| 342 | Change the Wi-Fi details, the PIN, or calibrate the knob, while a tuning change is part way through its wait | Only one write happens. Every endpoint that writes NVS starts the wait again, so the manual and automatic paths cannot take turns writing |
| 338 | Power cycle a radio that was on the firmware before this one | It comes up on its stored band and frequency, and every band reads as unset until you visit it. A version 7 blob is 148 bytes and has no room for any of this |

### Every page control actually does something

The keys and arguments were shortened to three letters. Four tables of argument names inside the handlers were missed by that rename, so the page posted the new name and the handler looked for the old one. Two of them failed **silently**: the endpoint answered 200 and reported the value unchanged.

This is the check that would have caught it, and it is worth running after any rename.

| # | Do this | Expect |
|---|---|---|
| 290 | For each control on `/radio`, change it and read the reply | The reply names the new value, not the old one. A 200 that reports the old value is a control that did nothing |
| 290a | Post a nonsense argument to each endpoint, such as `-d 'zzz=1'`, and an out of range value to each one that has a range, and read the refusal | Every name the refusal offers is one the handler really reads. Both kinds matter: the missing argument message and the bad value message are different strings, and the de-emphasis one was still saying `deemph` after all the missing argument messages had been put right. A refusal that names the argument it wants is part of the feature, and after the three letter rename five of them still named the old long form: `band` for `bnd`, `mode` for `mod`, `action` for `act`, and `mono`, `blend`, `hiblend`, `amnb`, `fmnb` and `deemph` in the FM one. Following any of those got a second 400 |
| 291 | `POST /api/fm -d 'bld=30'` then `-d 'hbl=35'` | The reply shows blend 30 and hiblend 35. These two were the silent ones |
| 292 | `POST /api/fm -d 'mno=1'` then `mno=0` | stereo and mono in the reply. This one at least answered 400 when it was wrong |
| 293 | `POST /api/settings -d 'rgn=4'` | `200`. The band plan controls were posting a name the handler did not know |
| 294 | Compare the names in `README.md` against the tables in `web_update.cpp` | They agree. The documents were right and the code was not |

### The web pages

Four pages instead of one. The split is not only tidiness: one page held every form, and it was the largest String the web server ever built.

| # | Do this | Expect |
|---|---|---|
| 211 | Open `/` signed out | Status, the red banner if the PIN is still 000000, and a PIN form. No forms that change anything |
| 212 | Look at the nav on any page | Home, Radio, Network, System, with the page you are on filled in amber |
| 213 | Open `/radio` signed out, enter the PIN | You land back on `/radio`, not on Home |
| 214 | Open `/system` signed out, enter the PIN | You land back on `/system` |
| 215 | Post to `/auth` with `nxt=http://example.com`, and try `//evil.com`, `/radio?x=1` and `javascript:alert(1)` too | You land on Home every time. The field is an allowlist of the exact paths the radio serves, so anything else falls back to Home rather than being sanitised and followed |
| 216 | Post to `/auth` with `nxt=/radio`, then `nxt=/system` | You land on that page. The field is `nxt`, not `next`: the three letter rename covered this one too |
| 217 | `/` while receiving | A Radio card with the frequency large, the signal, the squelch, and a button through to `/radio`. Read once: it does not refresh on its own, which is what the websocket in phase 5 is for |
| 218 | Force mono and reload `/` | The Radio card stops saying stereo |
| 219 | `/network` | Two cards, Wi-Fi and Access PIN, nothing else |
| 220 | `/system` | Four cards: This image, How it is doing, Firmware, Reboot. What the image is stays still, what it is doing changes every second |
| 221 | `/radio` | Only the radio. No firmware form anywhere near the reboot button |
| 222 | The red default PIN banner | On Home and Network, and it links to Network where the PIN is changed |
| 223 | Load any page with the CDN blocked | Still readable, and the nav still looks like a nav. The radio's own access point has no internet |

### Working the radio from the page

The Listening to card is the reason the Radio page exists. Before this it was four forms of settings with no way to change station.

| # | Do this | Expect |
|---|---|---|
| 224 | Press the right chevron beside the frequency | One step up, and the big frequency changes without the page reloading |
| 225 | Press the left chevron | One step back down |
| 226 | Type a frequency in the box and press Go | It tunes there. A frequency in no band goes red with a reason |
| 227 | Change the Band select | It changes band, and the page reloads, because which settings apply depends on the band |
| 228 | Drag the volume slider and let go | The number beside the label follows your finger, and the radio changes when you let go, not on every pixel |
| 229 | Change the Squelch select | It changes at once. In Manual the front knob becomes the squelch and stops setting the volume |
| 230 | Press Mute or unmute twice | It goes quiet and comes back. The line at the top says which |
| 231 | Press Keep these settings | It says where the radio will come up and which squelch mode |
| 232 | Do any of the above with the radio unplugged from the network mid press | The line says the radio did not answer, rather than the page hanging |
| 233 | Press Apply in Reception on an AM band | Only the settings that band can take are sent. Nothing is refused for being an FM idea |

### Auto seek

Seek walks the dial and decides what is a station. Three gates do that, and each rejects something the other two cannot: noise rejects an empty channel, multipath rejects one that is mostly reflections, and level rejects the shoulder of a strong station, which is quiet and clean and is not a station. The numbers behind them are in `test/fixtures/seek/`.

| # | Do this | Expect |
|---|---|---|
| 234 | Tune to the bottom of FM, then press Seek up repeatedly | It stops on each local station in turn and on nothing between them. The list is under Known stations below |
| 235 | Keep pressing past the top | It wraps to the bottom and carries on, rather than stopping at the band edge |
| 236 | Seek down from the top | The same stations, in the other order |
| 237 | Watch the frequency on the web page during a seek | It follows the dial while it hunts and settles when it stops. The page does not have to be reloaded |
| 238 | Listen during a seek | Silent while it moves, and the station fades in when it stops. Not a second of every station and every patch of noise on the way |
| 239 | Press Seek, then turn the tuning knob while it is running | It stops at once and the knob takes over. Any command cancels a seek |
| 240 | Press Seek, then press Mute while it is running | The same. It stops and the audio comes back rather than being left muted |
| 241 | Seek on a band with nothing on it, such as long wave here | It gives up after one full pass, a few seconds, and the audio comes back. `skf` is false |
| 242 | `GET /api/state` during a seek | `skg` true. After it stops, `skg` false and `skf` says whether it found anything |
| 243 | `POST /api/seek -d 'dir=sideways'` and with no arguments | `400` both times |
| 244 | Set Seek sensitivity to 1 and seek the band | It stops on fewer stations, the strong ones only |
| 245 | Set it to 6 and seek the band | It stops far more often, including on things that are not stations. That is what the setting is for |
| 246 | `POST /api/settings -d 'fsn=9'` | `400`, and nothing stored |
| 247 | Change the sensitivity, then seek | It takes effect at once. This is the one setting on that page that needs no reboot |
| 248 | Change the sensitivity, save, power cycle, seek | Still the same sensitivity |
| 249 | Seek on medium wave in the evening | Stops on the stations that are up. The AM rule has no multipath gate, because the chip puts a different measurement in that field on the AM side |

### Reception and audio

| # | Do this | Expect |
|---|---|---|
| 129 | Listen to a medium wave station | Comfortable at volume 0. Hiss over the top of a strong station means the tuner is back on its power up defaults |
| 130 | Watch `usn` on a medium wave station | Low hundreds. Around a thousand on a station reading 25 dBuV or better means the AM settings did not reach the chip |
| 131 | Listen to FM | Not dull and not shrill. Both are what a wrong de-emphasis sounds like, and 50 us is right everywhere except the Americas |
| 132 | Tune away from any station on FM | It goes to mono and the treble rolls off rather than hissing in stereo |

Measured on 738 kHz on 12 September 2026, before and after the tuner was told how to receive:

| | Before | After |
|---|---|---|
| Ultrasonic noise | 1025 | 138 |
| Sounded | hissy | clear |

**The level scale changed with it.** The tuner is now told to report levels 7.0 dB lower, which is what the old firmware does, so that its thresholds and the captures in `test/fixtures/agc/` mean the same thing here. Signal figures written down before that change are on the old scale. The RF gain point moved at the same time, so the two do not simply cancel.

### Known stations, for testing

Taken off air from this radio. Signal alone does not say whether something is a station: ultrasonic noise `usn` is the better test, low for a station and in the thousands for noise.

| Band | Frequencies |
|---|---|
| FM | 102.8, 98.3, 101.9, 93.5, 91.1, 92.7, 94.3, 106.4, 104.0 |
| MW | 738, 846, 882, 1377 |
| SW | 11900 and 15530 were receivable. 9620, 9740, 9870, 9500, 9700 and 11800 were not, at the time of testing |

Shortwave is not like the other bands. A frequency that is dead now can carry a strong station three hours later, so a shortwave check that finds nothing is not a failure by itself.

### Already covered by CI, do not retest by hand

Partition table arithmetic, the settings struct round trip, the rejection of a truncated or a wrong version blob, the access PIN derivation, the PIN attempt gate and its one minute lockout, the millisecond wraparound, and the build itself.
