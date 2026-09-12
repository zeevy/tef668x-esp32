"""Pick the upload protocol from the shape of the upload port.

The documented command for an over the air flash is

    TEF_OTA_PIN=<pin> pio run -e ats125 -t upload --upload-port <ip>

PlatformIO does not switch from serial to espota on its own, so this script
looks at the port and sets the protocol. A port that looks like an IP address
or a .local name means espota, anything else means the serial cable.

The radio asks the uploader for its access PIN. That is read from the
TEF_OTA_PIN environment variable rather than written into this file, so the
PIN never lands in the repository. Without it espota is refused by the radio.

Runs as a pre script, so the platform builder sees the value this sets.
"""

import os
import re

Import("env")  # noqa: F821  provided by PlatformIO

_IPV4 = re.compile(r"^\d{1,3}(?:\.\d{1,3}){3}$")


def _looks_like_network(port):
    if not port:
        return False
    return bool(_IPV4.match(port)) or port.endswith(".local")


def _configure():
    config = env.GetProjectConfig()  # noqa: F821
    section = "env:" + env["PIOENV"]  # noqa: F821

    port = env.subst("$UPLOAD_PORT") or ""  # noqa: F821
    if not port:
        try:
            port = config.get(section, "upload_port", "")
        except Exception:
            port = ""

    if not _looks_like_network(port):
        return

    config.set(section, "upload_protocol", "espota")
    env.Replace(UPLOAD_PROTOCOL="espota")  # noqa: F821
    print("Upload port %s looks like a network address, using espota." % port)

    pin = os.environ.get("TEF_OTA_PIN", "").strip()
    if pin:
        env.Append(UPLOAD_FLAGS=["--auth=" + pin])  # noqa: F821
    else:
        print(
            "TEF_OTA_PIN is not set. The radio will refuse this upload. "
            "Read the PIN off the serial banner and run it again as "
            "TEF_OTA_PIN=<pin> pio run -e ats125 -t upload --upload-port %s"
            % port
        )


_configure()
