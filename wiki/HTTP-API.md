# HTTP API

Everything the radio's screen can do, a script can do over HTTP. Every answer says what the radio actually did, and a refusal says why in plain words. The [web page](Web-Page.md) uses this same API.

The radio answers at `http://tef668x.local:8080`, or at its address on your network. The examples set it once:

```bash
R=http://tef668x.local:8080
```

Every sample answer on this page was taken from a radio running the firmware this wiki describes. The network name and address in them are placeholders.

## Signing in

Reading needs no PIN, except `/api/settings`. Every change needs the access PIN: send it once to `/auth`, which answers with a session cookie, and send the cookie with each change.

```bash
curl -s -c jar -d pin=000000 $R/auth      # sign in, keep the cookie in the file jar
curl -s -b jar -d khz=102800 $R/api/tune  # use the cookie
```

```
HTTP/1.1 303 See Other
Set-Cookie: tefsid=<32 hex>; Path=/; Max-Age=1800; HttpOnly; SameSite=Strict
Location: /
```

| Rule | What it means |
|---|---|
| The PIN | Six digits, `000000` on a new radio. Change it on the web page or in the menu, see [First Start and Wi-Fi](First-Start-and-Wi-Fi.md#change-the-access-pin) |
| The session | Lasts 30 minutes from signing in. Using it does not make it longer |
| One at a time | Signing in again, from anywhere, ends the session before |
| It ends | When the radio restarts, the PIN changes, or the web server is turned off |
| Wrong PINs | After five wrong PINs, `/auth` is locked for 60 seconds, for everyone. A right PIN starts the count again |

A change without the cookie:

```
$ curl -s -d khz=106400 $R/api/tune
Enter the access PIN first.                     [403]
```

A wrong PIN, and the sixth wrong PIN in a row:

```
$ curl -s -d pin=111111 $R/auth
Wrong PIN: That PIN is not right.               [403, an HTML page]
Too many tries: Too many wrong PINs. Try again in a minute.   [429, with Retry-After]
```

## How answers read

Most answers are one line of plain text, ending in a newline. The status code says how it went:

| Code | What it means |
|---|---|
| 200 | Done. The line says the state it reached, such as `FM 106.40 MHz` |
| 202 | The radio took it but has not confirmed it yet. Read `/api/state` to see where it got to. Do not send it again |
| 400 | Refused, and the line says why, for example `That is not a band. Use one of LW MW SW OIRT FM.` |
| 403 | Not signed in: `Enter the access PIN first.` |
| 404 | Under `/api/`, no such route or the wrong method: `Nothing on the API answers this. Check the path, and whether it takes GET or POST.` Also an empty preset or a catch no longer in the list. Any other unknown path is sent to `/` |
| 409 | Not now. For a key: `The last key sent has not been handled yet. Send again.` |
| 413 | A form or text body over 8192 bytes. A firmware upload is not limited by this |
| 429 | Too many wrong PINs |
| 500 | It could not be stored |
| 502 | `Not done: the tuner did not take it, and the radio is trying again.` |
| 503 | `The radio is busy. Try again in a moment.` Nothing was taken |

Fields are sent as form fields (`curl -d name=value`) or in the query string. A seek, a scan, a sweep and a Learn Local Stations pass answer at once and run on their own: read `/api/state` or `/api/dx` to follow them.

While a DX level sweep runs, about 4 seconds, the radio answers nothing. A request sent then is answered when the sweep ends.

## Reading the radio

### GET /api/state

Everything about the radio, as one JSON object. No PIN. `/status.json` is the same.

```bash
curl -s $R/api/state
```

<details><summary>A sample answer</summary>

```json
{"brd":"ats125","ver":"0.2.0","bid":"0824096+","slt":"app1","cnf":true,"upd":{"st":"none","ver":null,"sz":null},"net":"station","rssi":-65,"bars":1,"rst":"software (update)","ip":"192.168.1.40","dpn":true,"hep":118628,"hmn":49716,"hmp":40848,"hlb":42996,"stk":{"rad":2352,"lop":7344},"up":620,"slp":null,"pnl":{"lit":90,"dim":false,"sdb":24,"swp":30,"lvu":11480,"lvt":22060,"lvp":53,"lvb":10172,"lvm":13272,"psh":{"n":55,"px":250920,"us":129455,"max":2941,"ref":14370}},"bat":{"fit":true,"mv":null,"boot":4100},"clk":{"syn":true,"age":614,"now":"22:08"},"asv":{"n":5,"dif":false,"due":0,"bad":false,"idl":10000},"pst":{"n":16,"bad":false,"lost":0},"log":{"fit":true,"n":75},"inp":{"pad":true,"clk":0,"prs":0,"lst":"","lms":0,"typ":"","lns":65535,"pot":1511,"pdb":-22,"tch":{"pen":false,"dn":5,"rd":661,"x":749,"y":2345,"z1":266,"z2":2915}},"tun":{"prt":"TEF6686","pch":102,"xad":0,"xtl":"9.216 MHz","fsi":false,"frd":false,"dr":false,"bnd":"FM","khz":106400,"f":"106.40","unt":"MHz","stp":100,"bws":0,"dxw":0,"vol":-22,"mut":false,"tmd":"Auto","pst":9,"seq":8953,"ims":true,"eq":true,"mno":false,"dem":50,"fnb":0,"anb":100,"ahc":47,"lhc":52,"asm":28,"lsm":34,"snr":20,"cut":0,"bld":0,"hbl":0,"wid":false,"skg":false,"skf":false,"bep":false,"sql":"Off","sqo":true,"hmu":false,"sig":233,"sav":234,"usn":25,"wam":36,"off":56,"bw":236,"mod":35,"st":true,"plt":true,"qst":1000,"lvo":0,"agc":{"on":true,"gn":-1,"avg":433,"set":true},"rds":{"syn":true,"pi":"1064","psp":"1064","pim":"match","blk":[0,0,0,0],"pty":12,"ptn":"Easy Listening","tp":false,"ta":false,"ms":"speech","ps":" MAGIC  ","psh":" MAGIC  ","psm":255,"rt":"O PARVATHI - CHILD - DEVADASU - K. RANI + UDATHA SAROJINI - MAGI","min":{"ms":51664,"grp":532,"blk":[[531,0,1],[526,1,5],[526,3,3],[526,1,5]],"typ":{"0A":263,"2A":263}},"grp":532,"use":531,"cor":5,"bad":14},"scn":false,"scd":0,"sct":0,"scf":0,"sca":0,"scr":0,"scc":null}}
```

</details>

Numbers are whole numbers. A value with a fraction is sent in tenths: `"sig":168` is 16.8 dBµV.

| Key | What it is |
|---|---|
| `brd`, `ver`, `bid` | The board, the firmware version, and the git commit it was built from, with `+` when built with changes not committed |
| `slt`, `cnf` | The firmware slot it runs from, and whether that firmware is confirmed (false while on trial after an update) |
| `upd` | The update check: `st` is `off`, `wait`, `checking`, `none`, `found` or `failed`; `ver` and `sz` are the newer release's version and image size in bytes while `st` is `found`, and null otherwise |
| `net`, `rssi`, `bars`, `ip` | The network state (`station`, `joining`, `ap` for the hotspot, `offline`), the Wi-Fi signal in dBm and in bars, the address |
| `rst` | Why the radio last started, for example `power` or `software (update)` |
| `dpn` | True while the access PIN is still `000000` |
| `hep`, `hmn`, `hmp`, `hlb` | Free memory now, the least since start, the least in the start before, and the largest free block, in bytes |
| `up`, `slp` | Seconds since start, and seconds until Auto Off, or null when it is off |
| `pnl` | The screen: `lit` the light in per cent, `dim` whether it is dimmed, and LVGL memory figures |
| `bat` | The battery: `fit`, `boot` the millivolts read at start up, `mv` the millivolts now (null while Wi-Fi is on) |
| `clk` | The clock: `syn` whether it has the time, `now` the time as `HH:MM` |
| `pst`, `log` | How many presets and station log entries are stored |
| `inp` | The controls: `pot` and `pdb` the volume knob, `typ` digits being typed, `lst` the last key in words, and `tch` the touch screen, below |
| `tun` | The tuner and the station, below |

The `tch` object inside `inp` shows what the touch controller reads. A touch does nothing on the radio yet, so this is there to check the touch screen works:

| Key | What it is |
|---|---|
| `pen` | True while a finger or a pen is on the screen |
| `dn` | How many times the touch controller's pen line has gone low since start. A light touch can break contact and count more than once |
| `rd` | How many readings have been taken since start. Readings are taken only while something is on the screen, up to 100 a second. A very short tap, made while the radio is busy redrawing, can be missed by `pen`, `dn` and `rd` alike |
| `x`, `y` | Where the last reading was, in the touch controller's own steps, 0 to 4095. These are not screen pixels: on the ATS-125, with Display Rotation at Normal, `x` grows down the screen and `y` grows from right to left |
| `z1`, `z2` | The two contact readings of the last reading, 0 to 4095. `z1` is near 0 with nothing on the screen. A finger pressed harder reads higher; a pen reads about the same however hard it is pressed |

`x`, `y`, `z1` and `z2` are null until the first reading. They keep the last reading after the finger lifts, and a reading taken just as a finger lifts can be far from where it was.

The `tun` object:

| Key | What it is |
|---|---|
| `prt`, `pch`, `xtl` | The tuner chip, its patch, and its crystal |
| `bnd`, `khz`, `f`, `unt` | The band, the frequency in kHz, and the frequency as the screen shows it, with its unit |
| `stp`, `bws`, `dxw` | The step in kHz, the filter width set (0 for automatic), and DX mode's width (0 while DX mode is closed) |
| `vol`, `mut`, `tmd`, `pst` | The volume in dB, mute, the tuning mode (`Manual`, `Auto`, `Presets`, `Meter band`), and the preset number (0 for none) |
| `ims`, `eq`, `mno`, `dem` | iMS, the equaliser, forced mono, and de-emphasis in µs |
| `fnb`, `anb`, `ahc`, `lhc`, `asm`, `lsm` | The noise blankers in per cent, the AM high cut and soft mute levels in dBµV |
| `sig`, `sav` | The signal level now and its average, in tenths of a dBµV |
| `usn`, `wam` | Noise, and on FM multipath or on AM co-channel, in tenths of a per cent |
| `off`, `bw`, `mod` | The offset in tenths of a kHz, the filter width the tuner reports in kHz, and the modulation in per cent |
| `snr` | Signal to noise in dB, worked out by the firmware |
| `st`, `plt` | Stereo heard, and the stereo pilot |
| `lvo` | The level offset in dB, to add to the levels to match the screen |
| `sql`, `sqo` | The squelch mode, and whether the squelch is open |
| `skg`, `skf` | A seek is running, and the last seek found a station |
| `agc` | The volume AGC: `on`, `gn` the gain now in dB |
| `rds` | RDS, on FM only: `pi`, `ps` the name, `pty` and `ptn` the programme type, `tp`, `ta`, `ms` speech or music, `rt` the radio text, `af` the alternative frequencies, and the decoder counts. A field shows only once it has been received |
| `scn`, `scd`, `sct`, `scf`, `sca`, `scc` | A station scan: running, channels done and in total, then stations found, stations added, and whether it completed |

### GET /api/settings

The stored settings, as JSON. Needs the PIN.

```bash
curl -s -b jar $R/api/settings
```

```json
{"sid":"MyNetwork","pss":true,"dpn":true,"ldd":true,"sql":"Off","sbd":4,"sfq":106400,"svl":-30,"ims":1,"eq":1,"mno":0,"cut":0,"bld":0,"hbl":0,"fnb":0,"anb":100,"ahc":47,"lhc":52,"asm":28,"lsm":34,"dem":50,"abw":4,"tzo":"+05:30","rgn":3,"spc":0,"enc":0,"edr":0,"fsn":4,"asn":4,"smu":120,"bpk":2,"bpe":1,"bps":1,"sqf":12,"blt":90,"bdm":0,"bds":90,"blf":1,"rds":1,"ntp":1,"bat":1,"agt":70,"agb":0,"thm":10,"thn":0,"rot":0,"dst":0,"dsc":0,"dmf":50,"dml":99,"dlp":0,"dmu":0,"dal":1,"ddw":25,"dbw":114,"rrg":0,"drt":1,"dwt":1,"fof":0,"aof":0,"hsp":0,"web":1,"wif":1,"slp":0,"cst":["#141415","#09121a","#1b2a36","#d3d9de","#b78624","#0d567d","#e51f33","#270602","#ebe5e5"]}
```

Most keys are the same as for [POST /api/settings](#post-apisettings) below. `sid` is the network name and `pss` says whether a passphrase is stored; the passphrase itself is never sent. `tzo` is the UTC offset and `cst` the custom theme's colours. The rest (`sql`, `sbd`, `sfq`, `svl`, `ims` and the other reception keys, `abw`) are the stored radio settings, which are changed with the tuning and sound routes and kept with `/api/save`.

### GET /api/screen

What the screen shows now, one JSON object per line. The first line names the screen, the page and the theme. Then each text, with its place, colour and font, and each box.

```
{"screen":"radio","page":0,"dim":false,"theme":"Nightwatch"}
{"x":0,"y":0,"w":320,"h":240,"fill":"#080C10","fillRole":"ground|header"}
{"x":12,"y":3,"w":25,"h":23,"c":"#F8B000","role":"radio","font":"title","text":"FM"}
{"x":272,"y":7,"w":36,"h":17,"c":"#E0E4E0","role":"measurement","font":"small","text":"21:51"}
{"x":12,"y":32,"w":296,"h":88,"fill":"#F8B000","fillRole":"radio"}
...
```

No PIN.

## Tuning

All need the PIN.

| Route | Fields | Sample answer |
|---|---|---|
| `POST /api/tune` | `khz`, the frequency in kHz | `FM 106.40 MHz` |
| `POST /api/step` | `stp`, steps up (positive) or down (negative) | `FM 106.50 MHz` |
| `POST /api/band` | `bnd`: `LW`, `MW`, `SW`, `OIRT` or `FM` | `FM 106.40 MHz` |
| `POST /api/seek` | `dir`: `up` or `down` | `seeking up` |
| `POST /api/mode` | `mod`: `Manual`, `Auto`, `Presets` or `MeterBand` | `mode Manual`. MeterBand answers `mode Meter band` |
| `POST /api/step-size` | `khz`, a step the band offers. `bnd` sets another band's step | `step 100 kHz` |
| `POST /api/cycle` | `wht`: `band`, `bandwidth`, `mode`, `mute` or `features`. Steps it on: band, width and mode as their buttons do, mute on and off, and `features` through off, iMS, EQ and both | `mode Presets` |

```bash
curl -s -b jar -d khz=106400 $R/api/tune
curl -s -b jar -d stp=1 $R/api/step
curl -s -b jar -d bnd=FM $R/api/band
curl -s -b jar -d dir=up $R/api/seek
```

In tuning mode Auto, `/api/step` seeks, as the knob does. Set `mod=Manual` first to step.

Refusals, as the radio sent them:

```
That frequency is in no band.                         [400, khz=200000]
That is not a band. Use one of LW MW SW OIRT FM.      [400, bnd=AIR]
That is not a direction. Use up or down.              [400, dir=sideways]
```

## Sound

All need the PIN.

| Route | Fields | Sample answer |
|---|---|---|
| `POST /api/volume` | `db`, -60 to 24. The next turn of the volume knob replaces it | `volume -18 dB` |
| `POST /api/mute` | `on`: `1` or `0` | `muted`, `unmuted` |
| `POST /api/bandwidth` | `khz`, a width the band offers, `0` for automatic on FM and OIRT. `bnd` sets another band's width | `bandwidth automatic` |
| `POST /api/squelch` | `mod`: `off`, `auto` or `manual`. The Manual level comes from the volume knob only | `squelch Off` |
| `POST /api/fm` | Any of the reception settings below | The whole reception state, below |
| `POST /api/beep` | `ms`, 1 to 3000, and `hz`, 100 to 15000 (2000 if left out) | `beeping for 150 ms` |

`/api/fm` takes `ims`, `eq` and `mno` (0 or 1, FM only), `cut`, `bld` and `hbl` (0, or 20 to 60 dBµV, FM only), `fnb` and `anb` (0, or 50 to 150 %), `ahc` and `lhc` (0, or 20 to 60 dBµV), `asm` and `lsm` (0 to 50 dBµV), and `dem` (0, 50 or 75). It answers with every setting:

```
$ curl -s -b jar -d ims=1 -d eq=1 $R/api/fm
iMS on, EQ on, stereo, weak signal cut 0 blend 0 hiblend 0, blanker am 100 fm 0, am high cut 47 lw 52, am soft mute 28 lw 34, de-emphasis 50 us
```

```
The radio refused it: that bandwidth is not allowed here     [400, khz=99]
That is not a squelch mode. Use off, auto or manual.         [400, mod=foo]
```

## Keys

`POST /api/key` presses a control as a person would. It needs the PIN.

| Field | Values |
|---|---|
| `k` | `BAND`, `BW`, `MODE`, `PUSH` (the tuning knob's press), `ENTER`, `DX`, or a digit `0` to `9` |
| `e` | `short` or `long`. `DX` and the digits have no long press |
| `turn` | Turns the tuning knob by -20 to 20 clicks, not 0, in place of `k` |

```
$ curl -s -b jar -d k=BAND -d e=long $R/api/key
pressed BAND long
$ curl -s -b jar -d k=DX -d e=long $R/api/key
That key has no long press.                                   [400]
$ curl -s -b jar -d k=FOO $R/api/key
Give k, one of BAND BW MODE PUSH ENTER DX 0 to 9, or turn.    [400]
```

Only one key can wait at a time. A key sent before the one before it was handled gets 409: send it again. Read `/api/screen` to see the result of a key.

## Presets

See [Presets and Station Scan](Presets-and-Station-Scan.md) for the CSV format.

| Route | PIN | Fields | Sample answer |
|---|---|---|---|
| `POST /api/presets` | Yes | `do=set` with `slot`, `khz`, and `bw` and `name` if wanted | `Preset 50 is FM 98.30 MHz` |
| `POST /api/presets` | Yes | `do=recall` with `slot` | `FM 98.30 MHz` |
| `GET /api/presets.csv` | No | | The CSV file |
| `POST /api/presets/import` | Yes | `mode=merge` or `mode=replace` in the query, the CSV as the body, `Content-Type: text/csv` | `16 of 16 lines imported, 0 refused, 0 already taken, 0 names cut short` |
| `POST /api/scan` | Yes | `bnd`: `FM`, `MW`, `SW` or `LW`, FM if left out | `scanning FM` |

```
$ curl -s $R/api/presets.csv
slot,band,frequency,bandwidth,name,pi
1,FM,91100,0,,3712
2,FM,92700,0,,
3,FM,93500,0,,0935
...
```

```
Preset 99 is empty.                                                   [404, do=recall slot=99]
do has to be set or recall.                                           [400, do=clear]
0 of 1 lines imported, 0 refused, 1 already taken, 0 names cut short  [200, merge into a used slot]
Line 1 could not be read, so nothing was changed.                     [400, replace with a bad file]
Give bnd, one of FM MW SW LW, or leave it out for FM.                 [400, scan bnd=OIRT]
```

There is no way to clear one preset. A `replace` with only the header line clears them all.

## Station log

See [Station Log](Station-Log.md).

| Route | PIN | What it does |
|---|---|---|
| `GET /api/log` | No | The log, one JSON object per line, oldest first, after a first line with the count |
| `GET /api/log.csv` | No | The log as a CSV file |
| `POST /api/log` | Yes | Logs the station the radio is on. Answers `logged`, or 409 when the station is already logged or the radio is still tuning |

```
$ curl -s $R/api/log
{"n":75}
{"time":1789662597,"real":true,"band":"FM","khz":102800,"level_dbuv":342,"usn":8,"multipath":30,"cochannel":0,"snr":25,"stereo":true,"bw_khz":184,"name":null,"pi":null,"rt":null}
...
$ curl -s $R/api/log.csv
time,real,band,khz,level_dbuv,usn,multipath,cochannel,snr,stereo,bandwidth_khz,name,pi,rt
2026-09-17 21:59,1,FM,102800,342,8,30,0,25,1,184,,,
...
```

`time` in the JSON is seconds since 1970 in UTC, and `real` says whether the clock was set. `level_dbuv`, `usn`, `multipath` and `cochannel` are in tenths.

## DX mode

See [DX Mode](DX-Mode.md).

| Route | PIN | Fields | Sample answer |
|---|---|---|---|
| `POST /api/dx` | Yes | `on`: `1` or `0`. `khz`: DX mode's width. `scan`: `1` or `0` | `DX on, 114 kHz`, `DX off` |
| `POST /api/dx` | Yes | `sweep=1`, alone, while DX mode is open | `sweeping, GET /api/dx/sweep has it in about 4 s` |
| `POST /api/dx` | Yes | `baseline`: `now` keeps the last sweep as the baseline, `auto` goes back to the median | |
| `POST /api/dx` | Yes | `learn=1` alone: Learn Local Stations. `log` (the place in the list) or `logid` (a catch's `id`): log that catch | |
| `GET /api/dx` | No | | DX mode's state, then one line per catch |
| `GET /api/dx/sweep` | No | | The last sweep, with the level of every channel |
| `GET /api/dx.csv` | No | | The catches as a CSV file for FMLIST |

```
$ curl -s $R/api/dx
{"on":true,"page":0,"cursor":0,"n":1,"dropped":0,"sweeping":false,"sweep_rev":10597,"scan":"idle","scan_khz":0,"scan_found":0,"scan_passed":0,"scan_total":0,"lvo":0,"watch":true,"watch_n":0,"watch_up":null}
{"i":0,"id":17592,"time":1791130887,"real":true,"band":"FM","khz":106400,"pi":"1064","name":" MAGIC  ","country":null,"new":false,"count":1,"level_dbuv":364,"logged":false,"due":true}
```

```
$ curl -s $R/api/dx/sweep
{"rev":10598,"running":false,"abandoned":false,"time":1791130892,"real":true,"took_ms":3247,"width":114,"low":87000,"step":100,"count":211,"floor":-72,"baseline":"fixed","baseline_sweeps":1,"lvo":0,"level":[-68,-67,-58,...],...}
```

The sweep's `level`, `baseline_level`, `rise` and `peak` lists have one value per channel from `low` in steps of `step` kHz, in tenths of a dBµV, with null for no reading.

```
$ curl -s -OJ $R/api/dx.csv
Date,Time,Frequency,PI,Signal,Stereo,TA,TP,PTY,ECC,PS,Radiotext
04-10-2026,16:21:26,106.40 MHz,1064,36.4 dBμV,•, , ,12,--, MAGIC  ,
```

## Settings

### POST /api/settings

Changes stored settings. Needs the PIN. Send any mix of the keys:

```
$ curl -s -b jar -d blt=90 $R/api/settings
Saved and in use now.
$ curl -s -b jar -d rot=90 $R/api/settings
The rotation is 0 or 180.                                   [400]
```

A setting read at start up answers `Read at start up, so reboot for that to take effect.`

| Key | Values | What it is |
|---|---|---|
| `sid`, `pwd` | Text | The Wi-Fi network name and passphrase. The radio moves to that network at once |
| `pin` | Six digits | A new access PIN. It ends the session |
| `tzo` | `+05:30` style, `-12:00` to `+14:00` | The clock's offset from UTC |
| `ntp` | 0, 1 | Network time |
| `rgn` | 0 to 4 | FM band plan: Full, Japan, Wide, 87-108 MHz, Worldwide. After a restart |
| `spc` | 0, 1 | MW step 9 or 10 kHz. After a restart |
| `fsn`, `asn` | 1 to 6 | Seek sensitivity on FM and AM |
| `sqf` | 0 to 40 | Squelch floor in dBµV, 0 for off |
| `agt`, `agb` | 0 or 30 to 80; 0 to 8 | Volume AGC target in per cent (0 for off), and boost in dB |
| `smu` | 0 to 500 | Mute ramp in ms |
| `bpk`, `bpe`, `bps` | 0 to 3; 0, 1; 0, 1 | Key beeps, band edge beep, start chime (after a restart) |
| `blt`, `bdm`, `bds`, `blf` | 5 to 100; 0 to 100; 0 to 240; 0, 1 | Brightness, dim level, seconds before dimming (0 never), start up fade (after a restart) |
| `thm`, `thn` | 0 to 15 | Day and night theme: 0 Nightwatch, 1 Daylight, 2 Red Night, 3 Phosphor, 4 Clear, 5 Custom, 6 Slate, 7 Paper, 8 LCD, 9 Ember, 10 Clear Day, 11 High Contrast, 12 Mono, 13 Hi-Fi, 14 Violet, 15 Blossom |
| `tc0` to `tc8` | `#rrggbb` | The custom theme's colours |
| `rot` | 0, 180 | Screen rotation |
| `bat` | 0 to 2 | Battery mark: off, per cent, volts |
| `fof`, `aof` | -25 to 15 | FM and AM level offset in dB |
| `rds`, `rrg` | 0, 1 | RDS decoder; RDS region Europe or North America |
| `hsp` | 0 to 2 | Hotspot: Auto, On, Off |
| `web`, `wif` | 0, 1 | Web server and Wi-Fi. 0 can only be undone from the radio's menu, or by Erase Settings on the recovery screen |
| `slp` | 0 to 600 | Auto Off in minutes, 0 for off |
| `upc` | 0, 1 | Check for Updates. Turned on, the radio looks in this start too |
| `enc`, `edr` | 0, 1 | Encoder type and direction. After a restart |
| `dst`, `dsc` | 0 to 2 | DX stop condition; DX scan range |
| `dmf`, `dml` | 1 to 99 | DX preset range |
| `ddw` | 5 to 300 | DX dwell in tenths of a second |
| `dbw` | An FM width | DX scan bandwidth in kHz |
| `dlp`, `dmu`, `dal`, `drt`, `dwt` | 0, 1 | Loop band, mute during scan, auto-log, log radio text, watch presets |

### POST /api/save

Stores the station and the radio's settings now, so the radio starts this way. Needs the PIN.

```
$ curl -s -b jar -X POST $R/api/save
Saved. It will come up on 106.40 MHz, squelch Off.
```

## System

| Route | PIN | What it does |
|---|---|---|
| `POST /update` | Yes | Installs a firmware, sent as the form field `firmware`. See [Updating](Updating.md) |
| `POST /update/install` | Yes | Installs the newer release the update check found, from GitHub. Answers 202, then downloads and writes it and restarts into it. A failed download or sha256 leaves the old firmware running, with the update still offered. Answers 409 when nothing newer was found, while a new firmware is on trial, or during a station scan |
| `POST /reboot` | Yes | Restarts the radio. Refused with 409 while a new firmware is on trial |
| `POST /api/sleep` | Yes | Puts the radio to sleep. Answers `Going to sleep. Press the knob to wake the radio.` Refused with 409 while a new firmware is written or on trial |
| `POST /setpin` | Yes | Sets a new PIN, field `pin`. Ends the session |
| `POST /wifi` | Yes, except on the setup hotspot | Saves a network, fields `sid` and `pwd` |

`/update`, `/update/install`, `/reboot`, `/setpin`, `/wifi` and a refused `/auth` answer with a short HTML page, made for the browser. A right PIN on `/auth` gets the 303 with no body, and a call without a session gets the plain text 403.

## For measuring

These are used to measure the radio and are not needed for everyday use: `POST /api/seek/settle` (tune, wait and read the signal several times), `GET` and `POST /api/afcheck` (read one channel over and over), `POST /api/pot` (teach the radio the ends of the volume knob), `GET /api/dx/timing.csv` (how long RDS took to lock on each DX scan channel), and `GET /api/rds/raw` (the last raw RDS groups):

```
$ curl -s $R/api/rds/raw
# khz=106400 total=6288 held=128 lost=0 stat=8200 read=1
6160 1064 2190 4B41 5348 00
6161 1064 0180 E0CD 204D 00
...
```
