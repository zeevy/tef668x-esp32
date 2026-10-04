# Glossary

The words and short forms the radio, its menu and this wiki use.

## Signal readings

| Term | What it means |
|---|---|
| dBµV | Decibels above one microvolt: the strength of the signal at the aerial input. The radio screen shows it as `dBµV` and the menu as `dBuV`. Higher is stronger |
| Level | The signal strength, in dBµV |
| USN | Ultrasonic noise: the noise the tuner hears above the audio band, in per cent. A clean station reads low. Seek and squelch use it to tell a station from noise |
| WAM | Wideband AM: on FM, how much the signal is spoiled by reflections from hills and buildings (multipath), in per cent. Lower is better |
| Multipath | The same signal reaching the aerial by more than one path, which distorts the sound. WAM measures it |
| Offset | How far the station's carrier is from the tuned frequency, in kHz. A station centred on the frequency reads near 0 |
| SNR | Signal to noise ratio, in dB. The firmware works it out from the tuner's readings; the tuner does not report it |
| Co-channel | On AM, another station on the same frequency |
| MOD | Modulation: how strongly the broadcast audio drives the signal, in per cent. The modulation meter on the radio screen shows it |
| BW | The filter width, in kHz |

## RDS

| Term | What it means |
|---|---|
| RDS | Radio Data System: data an FM station sends with its sound. RBDS is the North American form of it |
| PI | Programme Identification: a four digit hex code for the station, the same on all its frequencies |
| PS | Programme Service name: the station's name, up to 8 characters |
| PTY | Programme Type: a number from 0 to 31 for the kind of programme, such as news or pop music |
| PTYN | Programme Type Name: the station's own name for its programme type |
| RT | Radio Text: a line of up to 64 characters, such as the song playing |
| RT+ | Radio Text Plus: marks parts of the radio text, such as the title and the artist |
| ECC | Extended Country Code: with the PI, it tells the country of the station |
| AF | Alternative Frequencies: other frequencies the same station is on |
| EON | Enhanced Other Networks: data about other stations of the same network |
| TP | Traffic Programme: the station carries traffic news |
| TA | Traffic Announcement: a traffic announcement is on air now |
| CT | Clock Time: the time the station sends |
| BLER | Block Error Rate: the share of RDS blocks lost to errors |
| Group, block | RDS is sent in groups of four blocks, A to D. Each group type, such as `0A`, carries a different kind of data |

## Tuning and sound

| Term | What it means |
|---|---|
| FM, OIRT | The FM broadcast band, and the eastern European FM band from 65 to 74 MHz |
| LW, MW, SW | Long wave, medium wave and shortwave, the AM bands |
| Metre band | A shortwave broadcast band named by its wavelength, such as `31 m` |
| Band plan | The FM range for your part of the world |
| Step | How far one click of the knob moves the frequency |
| Seek | Moving through the band until a station is found |
| Squelch | Keeping the sound off while there is no station worth hearing |
| AGC | Automatic Gain Control. The volume AGC brings stations to a similar loudness |
| iMS | NXP's multipath suppression in the tuner |
| EQ | The tuner's channel equaliser, another help against multipath |
| De-emphasis | A treble cut that undoes the treble lift every FM station adds before sending. 50 µs in most of the world, 75 µs in the Americas and South Korea |
| Stereo blend | Blending stereo towards mono on a weak signal, which is less noisy |
| High cut | Cutting the treble on a weak signal, where its noise is |
| Soft mute | On AM, turning the sound down on a weak signal |
| Noise blanker | Blanking short bursts of noise, such as from a car engine |
| Preset | A stored station, 1 to 99 |
| DX | Listening for far away stations. A catch is a station DX mode confirmed |

## Network and firmware

| Term | What it means |
|---|---|
| Hotspot | The radio's own Wi-Fi network, `tef668x-setup-XXXX` |
| mDNS, `.local` | How the radio's name, `tef668x.local`, is found on your network without a server |
| NTP | Network Time Protocol: how the radio gets the time from the internet |
| PIN | The six digit access PIN that guards every change made over Wi-Fi |
| OTA | Over the air: a firmware update sent over Wi-Fi |
| Rollback | The radio going back to the old firmware by itself when a new one does not work |
| Slot | One of the two places in flash a firmware is kept, `app0` and `app1` |
| Download mode | The mode in which the ESP32 takes a firmware over USB, started with BOOT and RESET |
| TEF668x | NXP's family of tuner chips: TEF6686, TEF6687, TEF6688 and TEF6689 |
| Patch | NXP's firmware for the tuner, which the radio writes to it at every start |
| LVGL | The graphics library that draws the screens |
