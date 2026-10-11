# Security

## Report a problem

Please report a security problem privately, through [GitHub's private reporting](https://github.com/zeevy/tef668x-esp32/security/advisories/new). Do not open a public issue for it.

Say what you found, the firmware version and build (`ver` and `bid` in `/api/state`), and the steps to show it.

## Supported versions

Only the latest release gets fixes. Until the first release, that is `master`.

## What is in scope

- The web server, the web page and the control API, with the live state WebSocket `GET /api/live`.
- The access PIN and the sign in sessions.
- The firmware update paths: the `/update` upload and the rollback.
- Wi-Fi setup and the hotspot.

## Known limits

These are known and written down. They are not new reports.

- The web page and the API use plain HTTP on the local network. There is no HTTPS.
- The hotspot has no password. It is meant for Wi-Fi setup. With Hotspot set to On it stays open, so anyone in range can reach the web page.
- The access PIN starts as `000000` until the owner changes it.
- Anyone who can reach the radio can read it with no PIN, for example the state, the station log with its notes, the presets, the DX catches and the sweeps. Only the stored settings, `GET /api/settings`, need the PIN to read.
- Every change needs the PIN, except `POST /wifi` on the setup hotspot, which saves a Wi-Fi network for the radio to join. With Hotspot set to On, that needs the PIN too. A right PIN opens one session, a cookie that is `HttpOnly` and `SameSite=Strict` and lasts 30 minutes. Signing in from another browser ends the session before. After five wrong PINs, signing in is locked for 60 seconds.
- `GET /api/live` sends the same state as `/api/state`, with no PIN, to at most two pages at once. It accepts no commands.
- The state never holds the name of the owner's Wi-Fi network. It holds the hotspot's name only while the radio is on the hotspot.
- The web page is one file in the firmware. It loads nothing from the internet.
