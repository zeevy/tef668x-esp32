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
- Every text the screen shows is one line in `src/lang/en.h`. A new text is also a line in each other language's file, `src/lang/<code>.h`, unless it reads the same as the English one. See [Correct a translation or add a language](#correct-a-translation-or-add-a-language).
- A new setting is a new field in the settings struct.
- Signal, noise and timing limits come from readings measured on a radio, never from a guess. Say where the number came from.
- Every library and tool is pinned to an exact version.

## Correct a translation or add a language

The screen's texts in other languages are in `src/lang/<code>.h`, one line per text: the id from `src/lang/en.h` and the text. A text the file leaves out shows in English. To correct a word, change its line there, or open an issue with the language, the screen, what it says and what it should say.

To add a language:

1. Write `src/lang/<code>.h` with the ids of `en.h`, in its order, leaving out the texts that read the same as English. Keep each text about as wide as the English one: the screen has no room to spare. A `printf` format keeps the same conversions as the English one; to put them in another order, number every one, as in `%2$s %1$d`.
2. Leave out the texts that stay English in every language: the boot and recovery screens' (`BOOT_`, and `RECOVERY_` but for `RECOVERY_PRESS_AGAIN`), the RDS programme languages (`LANG_`), the theme and language names (`THEME_`, `LANGUAGE_`), the units (`COMMON_UNIT_`), the band codes, the clock and the metre band. A unit test fails if a language holds one.
3. Add the language at the end of `TxtLanguage` in `src/core/strings.h`. In `src/core/strings.c`, include its file, add its block of texts like the others, and add it at the end of the language list. Add its own name as a `LANGUAGE_` line in `en.h`. The number is stored in every radio's settings, so a language never moves.
4. Cut the fonts again with `tools/make_lvgl_font.py`, so the letters the language uses are in them. The `fonts` gate fails until they are.
5. Draw every screen in the language and check it. See [Building from Source](Building-from-Source.md#draw-the-screen-pictures).

## Change the wiki

This wiki is published from the `wiki/` folder of the repository each time a change to that folder reaches `master`. To change a page, edit it there, in a pull request. An edit made on the wiki itself is replaced by the next publish.

- When a change alters what a person sees or does, change the wiki page in the same pull request.
- Links between pages are written `[Controls](Controls.md)`.
- Pictures of the radio's screens are linked from `assets/` in the repository. Pictures only the wiki uses go in `wiki/images/`.

## Licence

The project is GPLv3. By sending a pull request you agree that your change is released under the same licence.
