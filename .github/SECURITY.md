# Security

## Report a problem

Please report a security problem privately, through [GitHub's private reporting](https://github.com/zeevy/tef668x-esp32/security/advisories/new). Do not open a public issue for it.

Say what you found, the firmware version and build (`ver` and `bid` in `/api/state`), and the steps to show it.

## Supported versions

Only the latest release gets fixes. Until the first release, that is `master`.

## What is in scope

- The web server, the web page and the control API.
- The access PIN and the sign in sessions.
- The firmware update paths: the `/update` upload and the rollback.
- Wi-Fi setup and the hotspot.

## Known limits

These are known and written down. They are not new reports.

- The web page and the API use plain HTTP on the local network. There is no HTTPS.
- The hotspot has no password. It is meant for Wi-Fi setup. With Hotspot set to On it stays open, so anyone in range can reach the web page.
- The access PIN starts as `000000` until the owner changes it.
