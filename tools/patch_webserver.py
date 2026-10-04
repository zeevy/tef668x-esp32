"""Cap the size of a POST body before the framework reads it.

WebServer's own _parseRequest mallocs and reads a POST body, plain or url
encoded, entirely into heap before any handler of ours runs, sized to
whatever the client's own Content-Length header declares. requireAuth
never gets a chance to refuse a large one first, because the memory is
already spent by the time it would run. The framework version this project
pins gives no hook to refuse it first.

This patches Parsing.cpp in place, right before readBytesWithTimeout, to
answer 413 and close the connection before it allocates anything for a
plain or url encoded body over core/web_limits.h's
WEB_MAX_POST_BODY_BYTES. Multipart uploads never reach that call, and
canRaw handlers, the firmware upload at /update among them, read in
small fixed chunks regardless of declared size, so neither needs the cap
and neither is touched by it. The check has to sit at this call site
rather than as soon as Content-Length is parsed from the header, since
the request's Content-Type, and so whether this call runs at all, is not
yet known that early.

Runs as a pre script, so the platform builder compiles the patched file.
Idempotent: a marker in the inserted lines lets a repeat build and a fresh
checkout both work without re-patching or failing. If the vendored file no
longer contains the line this expects, most likely because platformio.ini
now pins a newer framework release with this file reshaped, the build
fails rather than shipping without the cap, because the only way to see that
a large POST body is no longer refused is to send one.
"""

import os
import sys

Import("env")  # noqa: F821  provided by PlatformIO

_MARKER = "tef668x body cap"
# Also taken as already patched: the marker builds before this one wrote, so a
# framework copy they patched is not patched a second time.
_OLD_MARKERS = ("ticket 28",)
_INCLUDE_TARGET = '#include "detail/mimetable.h"'
_INCLUDE_PATCH = _INCLUDE_TARGET + '\n#include "core/web_limits.h"'
_CAP_TARGET = "    } else if (!isForm) {\n"
_CAP_PATCH = (
    _CAP_TARGET
    + "      if (webPostBodyTooLarge(_clientContentLength, WEB_MAX_POST_BODY_BYTES)) {"
    " // " + _MARKER + "\n"
    '        client.println(F("HTTP/1.1 413 Payload Too Large"));\n'
    '        client.println(F("Connection: close"));\n'
    "        client.println();\n"
    "        return false;\n"
    "      }\n"
)


def _patch():
    platform = env.PioPlatform()  # noqa: F821
    path = os.path.join(
        platform.get_package_dir("framework-arduinoespressif32"),
        "libraries",
        "WebServer",
        "src",
        "Parsing.cpp",
    )
    with open(path, "r") as f:
        text = f.read()

    if _MARKER in text or any(m in text for m in _OLD_MARKERS):
        return

    if _INCLUDE_TARGET not in text or _CAP_TARGET not in text:
        sys.exit(
            "tools/patch_webserver.py: expected lines not found in %s, "
            "the vendored WebServer library has changed shape. Update "
            "_INCLUDE_TARGET and _CAP_TARGET to match it, so POST bodies "
            "stay capped by size and a large upload cannot use up the "
            "heap." % path
        )

    text = text.replace(_INCLUDE_TARGET, _INCLUDE_PATCH, 1)
    text = text.replace(_CAP_TARGET, _CAP_PATCH, 1)
    with open(path, "w") as f:
        f.write(text)
    print(
        "tools/patch_webserver.py: capped POST bodies at "
        "WEB_MAX_POST_BODY_BYTES in %s" % path
    )


_patch()
