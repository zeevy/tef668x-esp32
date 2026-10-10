"""Pin the ESP-IDF components of a framework rebuild to the shipped ones.

custom_sdkconfig in platformio.ini makes the platform rebuild the
framework's ESP-IDF libraries. ESP-IDF's component manager then fetches
the components below. Left alone it takes, for each one, the newest
release that every component asking for it allows, so two builds of one
commit can differ: a dependencies.lock does not stop that, since the
requirements one component makes of another ignore it.

The platform builds its own main component from a template it copies to
.dummy/ in the project folder, when that folder is missing. This script
copies the template there before every build and adds an exact pin for
each component, at the version the framework's shipped libraries were
built with, read from the versions.txt they carry. So a rebuild takes
exactly what shipped, and a platform upgrade brings its own versions.

After the link it checks what the last rebuild took and stops the build
if a component is missing from the list or not at its pinned version.
"""

import os
import re
import shutil

Import("env")  # noqa: F821  provided by PlatformIO

# What a rebuild for the ESP32 takes. A component the rebuild takes that is
# not here stops the build, so a platform upgrade that adds one is seen.
_COMPONENTS = (
    "chmorgan/esp-libhelix-mp3",
    "espressif/cbor",
    "espressif/esp-dsp",
    "espressif/esp-modbus",
    "espressif/esp-zboss-lib",
    "espressif/esp-zigbee-lib",
    "espressif/esp_daylight",
    "espressif/esp_diag_data_store",
    "espressif/esp_diagnostics",
    "espressif/esp_insights",
    "espressif/esp_modem",
    "espressif/esp_rainmaker",
    "espressif/esp_schedule",
    "espressif/esp_secure_cert_mgr",
    "espressif/jsmn",
    "espressif/json_generator",
    "espressif/json_parser",
    "espressif/lan867x",
    "espressif/lan86xx_common",
    "espressif/libsodium",
    "espressif/mdns",
    "espressif/network_provisioning",
    "espressif/qrcode",
    "espressif/rmaker_cmd_resp",
    "espressif/rmaker_common",
    "espressif/rmaker_common_events",
    "espressif/rmaker_console",
    "espressif/rmaker_system_ctrl",
    "espressif/rmaker_time_sync",
    "espressif/rmaker_work_queue",
    "joltwallet/littlefs",
)
# Built from the library builder's own repository, at the builder's commit.
_FB_GFX = "espressif/fb_gfx"

_ME = "tools/pin_components.py"


def _fail(message):
    print(_ME + ": " + message)
    env.Exit(1)  # noqa: F821


def _shipped_versions(libs_dir, mcu):
    path = os.path.join(libs_dir, mcu, "versions.txt")
    versions = {}
    for line in open(path, encoding="utf-8"):
        name, _, rest = line.partition(":")
        words = rest.split()
        if name == "lib-builder" and len(words) == 2:
            versions[_FB_GFX] = words[1]
        elif "__" in name and len(words) == 1:
            versions[name.replace("__", "/", 1)] = words[0]
    return versions


def _write_main_manifest(template_dir, dummy_dir, versions):
    shutil.rmtree(dummy_dir, ignore_errors=True)
    shutil.copytree(template_dir, dummy_dir)
    path = os.path.join(dummy_dir, "idf_component.yml")
    text = open(path, encoding="utf-8").read()
    pinned, n = re.subn(
        r"(  espressif/fb_gfx:\n    version: )\S+",
        lambda m: m.group(1) + '"' + versions[_FB_GFX] + '"', text)
    if n != 1:
        _fail("the platform's main component no longer names fb_gfx as "
              "expected in " + path)
    lines = [pinned.rstrip("\n")]
    for name in _COMPONENTS:
        lines.append("  " + name + ":")
        lines.append('    version: "==' + versions[name] + '"')
    open(path, "w", encoding="utf-8").write("\n".join(lines) + "\n")


def _built_version(component_dir):
    path = os.path.join(component_dir, "idf_component.yml")
    if not os.path.isfile(path):
        return None
    for line in open(path, encoding="utf-8"):
        match = re.match(r"""^version:\s*['"]?([^'"\s]+)""", line)
        if match:
            return match.group(1)
    return None


def _check_built(target, source, env):
    managed_dir = os.path.join(project_dir, "managed_components")
    entries = os.listdir(managed_dir) if os.path.isdir(managed_dir) else []
    wrong = []
    for entry in sorted(entries):
        name = entry.replace("__", "/", 1)
        if name == _FB_GFX:
            continue
        if name not in _COMPONENTS:
            wrong.append(name + " is not pinned")
            continue
        built = _built_version(os.path.join(managed_dir, entry))
        if built != versions[name]:
            wrong.append("%s is %s, pinned %s" % (name, built, versions[name]))
    if wrong:
        print(_ME + ": the framework rebuild took components that are not "
              "the shipped ones: " + "; ".join(wrong))
        return 1
    return 0


platform = env.PioPlatform()  # noqa: F821
mcu = env.BoardConfig().get("build.mcu")  # noqa: F821
project_dir = env.subst("$PROJECT_DIR")  # noqa: F821
libs_dir = platform.get_package_dir("framework-arduinoespressif32-libs")
versions = _shipped_versions(libs_dir, mcu)
missing = [n for n in _COMPONENTS + (_FB_GFX,) if n not in versions]
if missing:
    _fail("versions.txt does not name " + ", ".join(missing))

_write_main_manifest(
    os.path.join(platform.get_dir(), "builder", "build_lib"),
    os.path.join(project_dir, ".dummy"), versions)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", _check_built)  # noqa: F821
