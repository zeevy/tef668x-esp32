# Contributing

Issues and pull requests are welcome. The full guide is [CONTRIBUTING](https://github.com/zeevy/tef668x-esp32/blob/master/.github/CONTRIBUTING.md) in the repository. This page is the short version.

## Ask a question

Ask in [Discussions](https://github.com/zeevy/tef668x-esp32/discussions/categories/q-a). Issues are for bugs and feature requests.

## Report a bug

Open a [bug report](https://github.com/zeevy/tef668x-esp32/issues/new/choose). The form asks for:

- what happened, the steps to make it happen, and what you expected
- the radio, and the firmware version and build: **Menu > About > Firmware Version** and **Build**
- how often it happens
- the output of `/api/state`, which you get with `curl -s http://tef668x.local:8080/api/state` or by opening that address in a browser

A security problem goes in a [private report](https://github.com/zeevy/tef668x-esp32/security/advisories/new), not a public issue.

## Ask for a feature

Open a [feature request](https://github.com/zeevy/tef668x-esp32/issues/new/choose). It asks for the problem first, then your idea, then other ways you thought of. For anything bigger than a small change, agree on the idea in an issue before you write code.

## Send a change

1. Cut a branch from the latest `master`. One change per branch.
2. Run `tools/check.sh`. Every gate must pass. See [Building from Source](Building-from-Source.md).
3. If you have a radio, flash the change and check it there. Say in the pull request what you checked.
4. Open a pull request into `master`. The template asks what changed, why, and how it was tested. It is squash merged, so the whole change becomes one commit.

How the code is kept:

- Logic goes in `src/core/`, in plain C, with unit tests.
- Every text the screen shows is one line in `src/lang/en.h`.
- A new setting is a new field in the settings struct.
- Signal, noise and timing limits come from readings measured on a radio, never from a guess. Say where the number came from.
- Every library and tool is pinned to an exact version.

## Change the wiki

This wiki is published from the `wiki/` folder of the repository each time `master` changes. To change a page, edit it there, in a pull request. An edit made on the wiki itself is replaced by the next publish.

- When a change alters what a person sees or does, change the wiki page in the same pull request.
- Links between pages are written `[Controls](Controls.md)`.
- Pictures of the radio's screens are linked from `assets/` in the repository. Pictures only the wiki uses go in `wiki/images/`.

## Licence

The project is GPLv3. By sending a pull request you agree that your change is released under the same licence.
