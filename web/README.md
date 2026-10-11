# The radio's web page

The page the radio serves in a browser, as one Preact app. It is built into one `index.html`, which the firmware stores gzipped and serves from flash.

## Run it

Needs Node 24.

```bash
npm ci               # once, and after package.json changes
npm run dev          # the page with recorded data, no radio needed
npm run dev:radio    # the page on the live radio, nothing flashed
npm run build        # dist/index.html and its gzipped size
npm run record       # record the radio's answers into mock/, for npm run dev
npm test             # the browser tests, on a phone and a laptop size, against the recorded radio
```

The dev server prints a Network address. Open it on a phone on the same Wi-Fi to see the page at phone size.

`npm run dev:radio` forwards every API call to `http://tef668x.lan:8080`. Point it at another address with `RADIO=http://192.168.1.40:8080 npm run dev:radio`. `npm run record` takes `RADIO` and the PIN in `TEF_PIN` the same way. It only reads. Before it saves, it replaces the home network's name, the hotspot's name and every IP address, and empties the PC Link's addresses and every log note, since the recordings go into the public repository. Read `git diff web/mock` before a commit.

In `npm run dev` a write changes nothing and answers `ok`. It also turns the `/api/live` socket away, so the page polls `/api/state` every 5 s, as it does on the radio while the socket is closed.

The tests need their browser once: `npx playwright install --only-shell chromium`.

## Put it into the firmware

From the repository root:

```bash
npm --prefix web run build && python3 tools/web_page.py
```

`tools/web_page.py` gzips `web/dist/index.html` into `src/net/web_page.h`, which the firmware sends as it is. Commit that header with the change to the page. `tools/check.sh web` fails when the header is not what the page builds to now.

## Where things are

| | |
|---|---|
| `src/app.jsx` | The screens, one hash route each, with the view each route shows |
| `src/frame.jsx` | The frame round every screen: the header, the tabs on a phone or the menu on a wider screen, the PIN prompt and the footer |
| `src/screens/` | One file a screen. Presets & Log is two, `presets.jsx` and `log.jsx` |
| `src/ui/` | The parts the screens are built from: `kit.jsx` has the buttons, cards and other small parts, and the other files have bigger parts, such as the station on the dial and the sweep chart |
| `src/data/` | The calls to the radio, the commands and changes the screens send, the band plan, the radio's limits, and how the page writes the radio's numbers and readings |
| `src/data/radio.js` | Every call to the radio. Live data comes over the `/api/live` WebSocket, twice a second. While the socket is closed, the page polls `/api/state` every 5 s. A hidden tab does neither |
| `src/data/reads.js` | The lists the page keeps from the radio: presets, the log, DX catches and the sweeps, and the stored settings. Each is read again when its number in the state moves. A failed read says so and is tried again 5 s later |
| `mock/` | The radio's answers, recorded by `npm run record` |
| `tests/` | The browser tests, one file a screen |

Which folder may use which:

- `src/screens/` use `src/ui/` and `src/data/`.
- `src/ui/` uses `src/data/`.
- `src/data/` uses neither.
- Only `src/app.jsx` uses `src/frame.jsx` and `src/screens/`. No screen uses another screen.
