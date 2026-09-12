"""Link the native test binary with coverage instrumentation.

build_flags reaches the compiler but not the linker, so --coverage has to be
appended to LINKFLAGS by hand or the gcov symbols come out undefined.
"""

Import("env")  # noqa: F821  provided by PlatformIO

env.Append(LINKFLAGS=["--coverage"])  # noqa: F821
