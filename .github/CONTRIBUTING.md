# Contributing

Issues and pull requests are welcome. For anything bigger than a small fix, open an issue first, so we can agree on the idea before you spend time on code.

## Set up

Follow [Building and flashing](../README.md#building-and-flashing) in the README. The checks need PlatformIO, gcovr and clang-format at the exact versions it lists. Other clang-format versions format the code differently.

## Make a change

1. Cut a branch from the latest `master`. One change per branch.
2. Run `tools/check.sh`. It builds the firmware, runs the unit tests, checks coverage (95% at least), runs static analysis and checks the formatting. CI runs the same script.
3. If you have a radio, flash it and check the change on the radio. Say in the pull request what you checked.
4. Open a pull request into `master`. It is squash merged, so the whole change becomes one commit.

## How the code is kept

- `src/core/` is plain C. It knows nothing about the hardware or the screen, and it runs on a PC. Logic goes there, with unit tests in `test/`.
- Every text the panel shows is one line in `src/lang/en.h`. The code uses `txt(STR_...)`, never a quoted string.
- A new setting is a new field in the settings struct. No hand written byte offsets.
- Signal, noise and timing limits come from readings measured on a radio, never from a guess. Say in the pull request where the number came from.
- A comment explains the code in plain words and makes sense on its own.
- Every library and tool is pinned to an exact version, never a range.

## Licence

This project is GPLv3. By sending a pull request you agree that your change is released under the same licence.
