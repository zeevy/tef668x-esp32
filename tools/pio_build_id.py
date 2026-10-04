"""Write the git commit this firmware is built from into build_id.h.

The About group in the menu shows it as Build, so a person can tell which
build is on the radio. When the firmware's own files have changes not yet
committed, a + follows the hash, since the hash alone would name a commit the
image does not match. Without git, or outside a checkout, the text is empty
and the menu says the build is unknown rather than showing a made up hash.

The header goes in the build directory, not the source tree, and is only
written again when its text changes, so only the one source that includes
it compiles again.

Runs as a pre script, so the include path is in place before anything
compiles.
"""

import os
import subprocess

Import("env")  # noqa: F821  provided by PlatformIO

# The files a build is made from. A change anywhere else, such as the docs,
# does not change the image.
_SOURCES = ["src", "platformio.ini", "partitions.csv"]


def _git(*args):
    return subprocess.run(
        ["git", *args],
        cwd=env["PROJECT_DIR"],  # noqa: F821
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()


def _build_id():
    try:
        commit = _git("rev-parse", "--short", "HEAD")
        changed = _git("status", "--porcelain", "--", *_SOURCES)
    except (OSError, subprocess.CalledProcessError):
        return ""
    return commit + ("+" if changed else "")


def _write():
    out_dir = os.path.join(env.subst("$BUILD_DIR"), "generated")  # noqa: F821
    os.makedirs(out_dir, exist_ok=True)
    path = os.path.join(out_dir, "build_id.h")
    text = '#define FIRMWARE_BUILD "%s"\n' % _build_id()
    try:
        with open(path) as f:
            old = f.read()
    except OSError:
        old = None
    if old != text:
        with open(path, "w") as f:
            f.write(text)
    env.Append(CPPPATH=[out_dir])  # noqa: F821


_write()
