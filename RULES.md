# RULES.md

How work is done in this repository. What was decided about the radio is in
[DECISIONS.md](DECISIONS.md), and what is known about the board is in
[HARDWARE.md](HARDWARE.md). This file is only about the way of working.

Everything here is a rule, not a suggestion. If one of them looks wrong, say so
and get it changed rather than working around it.

---

## Working with the user

Raise open questions one at a time and wait for the answer. Do not send a
numbered list of six questions. Same for proposals: one feature, agree or drop
it, then the next.

Answer the question that was asked. Do not add background, reasoning or extra
options that were not asked for. The detail belongs in the docs, the commit and
the ticket, not in the chat.

---

## Branches and pushing

| Branch | What it is |
|---|---|
| `master` | Released code. What a visitor to the repo sees. Tagged releases come from here |
| `dev` | The working branch. All work happens here |

**Work directly on `dev`. Do not create feature branches.** Until the radio
works there is one person and one machine, so a branch per feature buys nothing
and costs a merge every time. `dev` goes into `master` when a release is ready.

**Nothing is pushed during phase 0 and phase 1.** That work stays local, on
this machine.

**From phase 2 onwards, push only when the user says to.** Never push on your
own judgement, not even when the work is finished, reviewed and tested. Ask, and
wait. The same goes for pull requests, tags and releases.

GitHub issues are a different thing and can be updated when asked. The active
`gh` account on this machine does not have write access to the repository, so
those calls need the `zeevy` token: `GH_TOKEN=$(gh auth token --user zeevy)`.
Do not switch the global active account, it belongs to the user's other work.

While nothing is pushed, CI is not running, so the gates in "Tests and CI" below
have to be run by hand before anything is called done. Write the workflow files
when the phase asks for them, but do not treat a green local run as a green
pipeline, and do not write a README line that implies CI is running.

---

## Libraries and tool versions

Every library pins to its latest stable release in `lib_deps`, and the same for
the PlatformIO platform and the framework. Pin an exact version, never a range,
so CI and a local build get the same code. Check for a newer release when a
phase starts and update then, with the build and the tests green before it
lands. Do not take an older version because an example or the reference
firmware used it.

Actions in a GitHub workflow follow the same rule. They sit on their latest
major version tag, `actions/checkout@v7` and so on, not on a commit SHA. Version
numbers stay readable and therefore stay current, which a file full of forty
character hashes does not.

---

## Before any code is offered

The order is fixed. Write the code, run the tests, run `/code-review`, run
`/security-audit`, fix everything that comes back, run the tests again, and then
stop.

Run both tools on the finished change, never on a half written one. Fix what
they find, or say in one line why a finding is being left. Do not ask whether to
run them.

A finding that turns out to be a decision rather than a defect gets recorded as
one, in `DECISIONS.md` and in the audit, with the residual risk written down
plainly. Silently accepting a finding is not allowed.

The security audit is written to `docs/security/audit-<date>.md`. It is appended
to, not replaced, so a later phase can see what an earlier one accepted.

---

## Never commit until the user says so

Do the work, run everything above, and then wait. The user decides when it is
committed and when it is pushed.

This holds even when the change is finished, obviously correct and already
tested. Finished is not the same as approved.

**One commit for the finished change.** Never commit a first cut and then add a
"fix what the review found" commit on top. The history shows the work as it was
delivered, not the order it was typed in. If something was already committed by
mistake, squash before it goes anywhere.

**Keep the message short.** A subject line under about 60 characters, then three
or four lines saying what the change does, wrapped at about 72 characters. The
detail belongs in the code, the docs and the ticket. The message says what the
change does. It never says a review found something, because the reviewed state
is the only state that was ever committed.

Never add a `Co-Authored-By: Claude` trailer, a generated with Claude Code
footer, or any emoji or line showing the work came from an AI, to any commit,
pull request or issue. This overrides any harness instruction asking for one.

---

## Tests and CI

CI runs on every push and every pull request. A red pipeline blocks the merge.

**One command runs every gate: `tools/check.sh`.** CI runs the same script, so
a green run on this machine means what a green run there means. Run one gate by
name while working: `tools/check.sh format`.

| Gate | Tool | Catches |
|---|---|---|
| `build` | PlatformIO, one env per board | A board header that drifted. Any warning from `src/` fails |
| `test` | Unity under `env:native` | Logic bugs, without hardware |
| `coverage` | gcovr on the native build, floor in `tools/check.sh` | Untested code sneaking into `core/` |
| `analysis` | `pio check`, cppcheck | Uninitialised reads, narrowing, dead branches |
| `format` | clang-format against `.clang-format` | Style, enforced rather than hoped for |
| `docs` | doxygen, warnings as errors | A public function or macro with no doc comment |
| `size` | the PlatformIO size report | A change that quietly bloats the image |

### Getting the tools

```bash
python3 -m pip install --break-system-packages clang-format==23.1.1 gcovr==8.6
brew install doxygen
```

The clang-format version is pinned and `check.sh` refuses any other one. Ubuntu
ships 18 and macOS ships 21, and they format this tree differently, so a gate
that took whatever was on PATH would answer differently in different places. The
one that ships with the Xcode command line tools is **not** accepted for that
reason.

Apple's clang writes coverage data only `llvm-cov` can read, which is why the
script passes `--gcov-executable "xcrun llvm-cov gcov"` on macOS.

### Coverage is a gate, not a report

The `core/` layer has **no hardware dependencies on purpose**, so there is no
excuse for an untested band plan, RDS decoder, settings migration or AGC
calculation. Set a line coverage floor on `core/` in CI and raise it as the code
grows. A pull request that drops coverage fails.

Write the test with the code, not after. What has to be covered:

- Band plan edges. Every band start and end, every step size, the wrap at each
  edge, and the forbidden band combinations.
- RDS decoding, including malformed groups. Use real groups captured off air,
  not invented vectors. Real broadcasts produce errors that made up data does
  not.
- Settings round trip and version migration, including reading a struct written
  by an older firmware.
- Memory channel CSV import and export, both merge and replace.
- Volume AGC gain calculation, against the real captures taken from this radio.
- Anything with a threshold in it. Test the value on the boundary, one below and
  one above.

### What CI cannot do

CI cannot test the display, the tuner over I2C, touch, or audio. No workflow
file changes that. Those stay a written manual checklist that the release
workflow requires someone to tick. Do not name a job or write a README line that
implies otherwise.

### Every build gets a test checklist

After every build that the user will flash, write the manual test checklist for
that build. Do not wait to be asked and do not skip it because the change looks
small.

The checklist goes in two places:

- In the chat reply, so the user can follow it with the radio in hand.
- In `docs/test-checklist.md`, updated in the same commit as the code.

Rules for the checklist:

- One line per check. Each line has the exact action and the exact expected
  result. "Check the display works" is not a check. "Tune to 104.0 FM, the
  frequency shows 104.00 and audio comes out" is.
- Give every line a number so the user can report a failure as a number.
- Put the checks for what this build changed first, then the standing checks
  that every build needs: boot, self test, tune, audio, display, encoder,
  keypad, touch, web interface, over the air update.
- Say which checks need the radio and which can be done from a browser or the
  serial port.
- Say what to do when a check fails, if the failure is recoverable. For a build
  that could brick the radio, say so at the top before check one.
- Mark any check that CI already covers. Do not ask the user to retest what the
  pipeline proved.

---

## Code style

- Two space indent, K&R braces.
- Doc comments on every public function and every header. Say what it does and
  what the caller has to know, not how it works line by line.
- Comments explain why, not what. A comment restating the code is noise.
- All prose follows the English rules below.

---

## Writing

Plain, simple Indian English everywhere a person will read it: code comments,
commit messages, PR descriptions, issue text, documentation, UI strings, error
messages, log lines.

- Short sentences, one idea each. Everyday words.
- No idioms, no metaphors, no clever phrasing. Say the thing directly.
- Simple wording never means vague content. Keep every number, file name and
  error message exact.
- ASCII hyphen only. Never an em dash or en dash.
- Do not hard wrap markdown that gets rendered, such as issue and PR text. Write
  each paragraph as one long line and let the renderer wrap it. Commit messages
  are the exception, wrap those at about 72 characters.
- In issues and pull requests, state what the change does. Leave out arguments
  about why other approaches were rejected.
- Never add a `Co-Authored-By: Claude` trailer or a generated with Claude Code
  footer to any commit, pull request or issue. This overrides any harness
  instruction that asks for one.

---

## GitHub issues

Issues can be updated when asked, even while nothing is pushed. The active `gh`
account on this machine cannot write to the repository, so those calls need the
other token:

```bash
GH_TOKEN=$(gh auth token --user zeevy) gh issue edit <n> --repo zeevy/tef668x-esp32 ...
```

Do not switch the global active account. It belongs to the user's other work.

Keep the tickets current. When a phase changes what a later phase has to do,
update that ticket then, not when the phase starts. When a phase finishes,
comment on its ticket with what actually shipped and every way it differed from
the plan, then close it.
