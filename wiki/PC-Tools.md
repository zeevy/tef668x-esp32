# PC Tools

The radio can be worked from a PC with [XDR-GTK](https://github.com/kkonradpl/xdr-gtk), the program DX listeners use for a tuner on the desk, and with [FM-DX Webserver](https://github.com/NoobishSVK/fm-dx-webserver), which puts a tuner on a web page. Both speak the same XDR protocol, and the radio answers it over Wi-Fi on TCP port 7373, the port xdrd uses. Through XDR-GTK, RDS Spy and StationList work too.

The radio stays yours while a PC is connected: the knobs, the keys, the touch screen, the menu and the web page all keep working, and what you change on the radio shows in the PC program.

![A PC connected: the laptop mark left of the Wi-Fi mark](https://raw.githubusercontent.com/zeevy/tef668x-esp32/master/assets/screens/fm-pc-link.png)

## Turn it on

1. On the radio, open the menu, then **Connectivity > PC Link**, and save **On**. Or, on the web page, **Network > PC Link > On**.
2. Look up the address to type in **Connectivity > Network Info > PC Link**: for example `tef668x.local:7373`.

Off, which is how a new radio comes, nothing listens on port 7373.

The password is the radio's **Web PIN**, the six digits you sign in to the web page with. There is no other password to set.

## XDR-GTK

1. In XDR-GTK, open the connection window and choose the network connection, not a serial port.
2. Host: `tef668x.local`, or the radio's IP address from Network Info. Port: `7373`. Password: the Web PIN.
3. Connect. The laptop mark shows in the radio's header.

XDR-GTK's own code after its v1.2 release has a **TEF668X mode** in its settings, under Interface. Turn it on there: the bandwidth list then has the TEF668x filters, 56 to 311 kHz, and the two toggles read cEQ and iMS. Release v1.2 has no such mode; it still works, and a width it asks for goes to the nearest one the radio has.

### RDS Spy and StationList through XDR-GTK

XDR-GTK passes the radio's data on: its own RDS Spy server (port 7376 on the PC) feeds [RDS Spy](https://rdsspy.com/), and its StationList option feeds [StationList](https://zeiterfassung.3sdesign.de/station_list.htm). Set those up in XDR-GTK; the radio needs nothing more.

## FM-DX Webserver

1. In its setup, connect it to xdrd over the network, not to a serial port: IP `tef668x.local` or the radio's address, port `7373`, password the Web PIN.
2. Choose the `tef` device profile.

The web server's sound comes from a sound input on the PC it runs on, not over this link.

## What the radio does with each setting

A PC changes the radio the same way the knob or the web page does, and is told back what is really in force. A control the radio cannot follow goes back to the radio's value after a moment.

| In the PC program | On the radio |
|---|---|
| Frequency | Tunes there, changing band when the frequency is in another. FM goes to the nearest 10 kHz |
| FM or AM | FM, or medium wave |
| Bandwidth | The nearest width the band has. Auto is the FM automatic width |
| De-emphasis | 50 µs, 75 µs or off |
| cEQ and iMS | The channel equalizer and multipath suppression |
| Mono | Forced mono on or off. The MPX output is not on this radio |
| Volume | 100 is 0 dB and each step down is 0.6 dB less; 0 mutes. A mute from a PC is lifted when the last PC leaves |
| Squelch | 0 turns the squelch off. A level turns Off into Auto; a squelch already on stays as it is. The PC shows where the squelch opens, in dBf |
| AGC, antenna, attenuation, rotator | Fixed on this radio: the highest AGC start, one aerial, no attenuation, no rotator |
| Sampling interval | How often the signal is sent, from 66 ms, the usual, to 1000 ms. Back to 66 ms when every PC has left |
| Spectral scan | Not yet |

The signal is sent in dBf, which is dBµV plus 11.25; XDR-GTK and FM-DX Webserver show it in dBµV again.

## Good to know

- **Three PCs at most.** A fourth is closed at once; XDR-GTK then says `Authentication error.`, though the PIN is right. `ref` in `/api/state` counts them.
- **Five wrong PINs** lock the link for a minute, and only the link: the web page's sign in is not touched. A PC still set to an old PIN keeps the link locked this way, so after changing the Web PIN, change it in XDR-GTK and FM-DX Webserver too.
- **Changing the Web PIN** signs every PC out, as it signs out the web page.
- **A DX level sweep** stops the link for its 3 to 4 seconds; the lines start again after it.
- A PC that connects and does not start within 10 seconds is closed, so a lost connection cannot hold one of the three places.

`GET /api/state` shows the link under `pcl`; see [HTTP API](HTTP-API.md).
