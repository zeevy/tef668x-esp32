# Station Log

The station log is a list of the stations you have heard, written by the radio when you ask. It is separate from the [presets](Presets-and-Station-Scan.md). It keeps the time, the frequency, the signal readings and the RDS details of each.

## Log a station

On the radio screen, hold the tuning knob down. After 600 ms the radio writes the station and says what happened on the name line for 1.5 seconds:

| Message | What it means |
|---|---|
| `Logged 106.40` | The station was written, with its frequency |
| `Already Logged` | The log already has this station: the same band and frequency, and the same PI when both have one |
| `Still tuning - wait` | A seek, a station scan or a DX scan is running. Try again when it stops |
| `Not logged` | The radio could not read the station, or could not write the log file |

Other ways to log:

- **Hold ENTER**, on the radio screen, the RDS pages and the DX pages. On the RDS and DX pages the message shows in the header.
- **Menu > Go To > Log Current Station.** It closes the menu and logs.
- **DX mode** can log new stations by itself. See below.

## What is kept

| Item | What it is |
|---|---|
| Time | The date and time from the network clock. If the radio has no time yet, the time since it started |
| Band and frequency | |
| Signal | The level in dBµV, USN (noise), multipath, co-channel and SNR, the filter width, and stereo |
| Name and PI | The station's RDS name and PI, only when they belong to that channel |
| Radio text | When DX Scanner > Log Radio Text is On, which it is at the start |

The log holds 250 stations. When it is full, the oldest one is dropped to make room, so a new one is never refused.

There is no way to delete a station or clear the log, on the radio or over Wi-Fi.

## View it on the radio

Open the menu and go to **Stations > Station Log**. The newest station is first. Each row shows the RDS name, or `PI` and the code when there was no name, or the band, and the frequency. Press a row to tune to that frequency. The menu stays open.

![The Station Log in the menu](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/menu-station-log.png)

## Export it

As a CSV file for a spreadsheet:

```bash
curl -s -o logbook.csv http://tef668x.local:8080/api/log.csv
```

The columns are `time,real,band,khz,level_dbuv,usn,multipath,cochannel,snr,stereo,bandwidth_khz,name,pi,rt`. The time is written as `YYYY-MM-DD HH:MM` at the clock's UTC offset, or as `+` and milliseconds since start when the radio had no time.

`level_dbuv`, `usn`, `multipath` and `cochannel` are in tenths: `452` means 45.2 dBµV. The level includes the band's Level Offset.

As JSON, one station per line, oldest first, after a first line with the count:

```bash
curl -s http://tef668x.local:8080/api/log
```

Both need no PIN. To log the station the radio is on from a script, `POST /api/log` with the access PIN. It answers 409 when the station is already logged or the radio is still tuning.

## Logging from DX mode

With **DX Scanner > Auto-Log Stations** On, which it is at the start, DX mode writes each new station to the log by itself: one whose PI it has never caught before and which is not in the log yet. It writes it when the RDS name arrives, or when you tune away or leave DX mode. The same check for a station already logged and the same 250 limit apply.

**DX Scanner > Learn Local Stations** marks the stations you can hear now as already caught, so they are never logged as new. It writes nothing to the log.
