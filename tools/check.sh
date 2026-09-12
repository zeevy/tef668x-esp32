#!/usr/bin/env bash
#
# Every gate, in one command.
#
#   tools/check.sh          run every gate, then report what failed
#   tools/check.sh format   run one gate by name
#
# Every gate runs even when an earlier one fails, so one run tells you
# everything that is wrong rather than only the first thing.
#
# This is the same script CI runs, so a green run here means the same thing as
# a green run there. While nothing is pushed, this is the only place the gates
# run at all.

set -o pipefail

COVERAGE_FLOOR=95

# Formatters disagree between versions, so the version is pinned and checked.
# Apple ships 21, Ubuntu ships 18, and they format this tree differently. A
# format gate that says different things in different places is worse than no
# format gate. Install it with:
#   python3 -m pip install --break-system-packages clang-format==23.1.1
CLANG_FORMAT_VERSION=23.1.1

cd "$(dirname "$0")/.." || exit 1

# Find a clang-format that is the pinned version, and only that version. Every
# entry on PATH is tried, not just the first match, because a different version
# earlier on PATH must not hide the right one further along.
find_clang_format() {
  local dir c
  while IFS= read -r dir; do
    for c in "$dir/clang-format" "$dir/clang-format-${CLANG_FORMAT_VERSION%%.*}"; do
      [ -x "$c" ] || continue
      if "$c" --version 2>/dev/null | grep -q "$CLANG_FORMAT_VERSION"; then
        echo "$c"
        return
      fi
    done
  done < <(printf '%s\n' "${PATH//:/$'\n'}")

  for c in /Library/Developer/CommandLineTools/usr/bin/clang-format \
           /opt/homebrew/opt/llvm/bin/clang-format; do
    [ -x "$c" ] || continue
    if "$c" --version 2>/dev/null | grep -q "$CLANG_FORMAT_VERSION"; then
      echo "$c"
      return
    fi
  done
  echo ""
}

# Apple's clang writes coverage data that only llvm-cov can read.
gcov_executable() {
  if [ "$(uname)" = "Darwin" ]; then
    echo "xcrun llvm-cov gcov"
  else
    echo "gcov"
  fi
}

failed=0
run() {
  local name="$1"
  shift
  printf '\n== %s\n' "$name"
  if "$@"; then
    printf '   ok\n'
  else
    printf '   FAILED\n'
    failed=1
  fi
}

gate_build() {
  # Our own objects are removed first. A compiler only warns about what it
  # actually compiles, so an incremental build reports nothing and a warning
  # gate built on one can never fire.
  rm -rf .pio/build/ats125/src

  local log
  log=$(mktemp)
  pio run -e ats125 >"$log" 2>&1
  local rc=$?
  tail -3 "$log"

  if [ "$rc" -ne 0 ]; then
    grep -iE 'error' "$log" | head -20
    rm -f "$log"
    return 1
  fi

  # Warnings from our own sources are failures. The framework headers trip
  # -Wshadow in their own code and there is nothing useful to do about that,
  # so only src/ counts.
  if grep -E '^src/' "$log" | grep -i warning; then
    printf '   warnings in src/, listed above\n'
    rm -f "$log"
    return 1
  fi
  rm -f "$log"
  return 0
}

# Where the per test coverage files are kept, so one run of the suite serves
# both the test gate and the coverage gate.
COVERAGE_DIR=""

# Run every test folder on its own and keep each one's coverage.
#
# PlatformIO builds a separate binary per folder and each build overwrites the
# .gcda files rather than adding to them, so reading them once at the end
# reports only whichever folder ran last. Each is saved as it goes and merged
# afterwards.
#
# Both gates call this and it only does the work once per invocation.
run_suite() {
  if [ -n "$COVERAGE_DIR" ]; then
    return "$SUITE_RESULT"
  fi
  COVERAGE_DIR=$(mktemp -d) || {
    echo "   could not make a working directory"
    COVERAGE_DIR=""
    return 1
  }
  SUITE_RESULT=0
  COVERAGE_RESULT=0
  SUITE_COUNT=0

  local gcov t
  gcov=$(gcov_executable)
  for t in test/test_*; do
    [ -d "$t" ] || continue
    t=$(basename "$t")
    SUITE_COUNT=$((SUITE_COUNT + 1))
    rm -rf .pio/build/native
    if ! pio test -e native -f "$t" 2>&1 | tail -4; then
      SUITE_RESULT=1
      continue
    fi
    # Coverage collection failing is not a test failure. Keeping them apart
    # stops the test gate reporting red when every test passed.
    if ! gcovr --gcov-executable "$gcov" --root . --filter 'src/core/' \
               --json "$COVERAGE_DIR/$t.json" .pio/build/native >/dev/null 2>&1; then
      echo "   could not read coverage for $t"
      COVERAGE_RESULT=1
    fi
  done

  if [ "$SUITE_COUNT" -eq 0 ]; then
    echo "   no test folders were found, so nothing ran"
    SUITE_RESULT=1
  fi
  return "$SUITE_RESULT"
}

gate_test() {
  run_suite
}

gate_coverage() {
  run_suite || {
    echo "   the tests did not all pass, so coverage means nothing"
    return 1
  }
  if [ "$COVERAGE_RESULT" -ne 0 ]; then
    echo "   coverage could not be collected for every test"
    return 1
  fi

  local args=() f
  for f in "$COVERAGE_DIR"/*.json; do
    [ -f "$f" ] || continue
    args+=(--add-tracefile "$f")
  done
  if [ ${#args[@]} -eq 0 ]; then
    echo "   no coverage was produced"
    return 1
  fi

  gcovr "${args[@]}" --fail-under-line "$COVERAGE_FLOOR" 2>&1 | tail -10
  return ${PIPESTATUS[0]}
}

gate_analysis() {
  pio check -e ats125 --skip-packages \
    --fail-on-defect high --fail-on-defect medium 2>&1 | tail -6
  return ${PIPESTATUS[0]}
}

gate_format() {
  local cf
  cf=$(find_clang_format)
  if [ -z "$cf" ]; then
    echo "   clang-format $CLANG_FORMAT_VERSION not found."
    echo "   Versions format this tree differently, so only this one counts."
    echo "   python3 -m pip install --break-system-packages clang-format==$CLANG_FORMAT_VERSION"
    return 1
  fi
  local bad=0
  while IFS= read -r f; do
    if ! "$cf" --dry-run -Werror "$f" >/dev/null 2>&1; then
      echo "   not formatted: $f"
      bad=1
    fi
  done < <(find src test \( -name '*.c' -o -name '*.cpp' -o -name '*.h' \))
  [ "$bad" -eq 0 ] || echo "   run: $cf -i \$(find src test -name '*.c' -o -name '*.cpp' -o -name '*.h')"
  return $bad
}

# A public function is documented on its declaration in the header, never again
# on its definition. Two copies drift apart, and doxygen on the CI machine is an
# older version that treats a doc block with no @param as an error, so this
# fails there after passing here. Catch it locally instead.
gate_doc_placement() {
  found=0
  for f in $(find src -name '*.c' -o -name '*.cpp'); do
    # A /** */ block whose next line is a definition that does not start with
    # static. awk keeps the line number of the block's opening.
    awk -v file="$f" '
      /^\/\*\*/ { inblock = 1; start = NR; next }
      inblock && /\*\// { inblock = 0; expect = 1; next }
      inblock { next }
      expect {
        expect = 0
        # Only a function definition matters, and only a non static one.
        if ($0 ~ /^[A-Za-z_].*\(/ && $0 !~ /^static/ && $0 !~ /^typedef/) {
          printf "   %s:%d: doc comment on a public definition. Document it in the header instead.\n", file, start
          exit 1
        }
      }
    ' "$f" || found=1
  done
  return $found
}

gate_docs() {
  command -v doxygen >/dev/null 2>&1 || { echo "doxygen not found"; return 1; }
  gate_doc_placement || return 1
  mkdir -p build/doc
  doxygen Doxyfile
}

gate_size() {
  # Not a pass or fail, a number to watch. CI posts it on a pull request.
  pio run -e ats125 2>&1 | grep -E 'RAM:|Flash:'
}

case "${1:-all}" in
  build)    run "build, ats125"        gate_build ;;
  test)     run "unit tests, native"   gate_test ;;
  coverage) run "coverage on core/"    gate_coverage ;;
  analysis) run "static analysis"      gate_analysis ;;
  format)   run "format"               gate_format ;;
  docs)     run "doc comments"         gate_docs ;;
  size)     run "size"                 gate_size ;;
  all)
    run "build, ats125"      gate_build
    run "unit tests, native" gate_test
    run "coverage on core/"  gate_coverage
    run "static analysis"    gate_analysis
    run "format"             gate_format
    run "doc comments"       gate_docs
    run "size"               gate_size
    ;;
  *)
    echo "unknown gate: $1"
    echo "one of: build test coverage analysis format docs size all"
    exit 2
    ;;
esac

[ -n "$COVERAGE_DIR" ] && rm -rf "$COVERAGE_DIR"

printf '\n'
if [ "$failed" -eq 0 ]; then
  printf 'All gates passed.\n'
else
  printf 'Something failed. See above.\n'
fi
exit "$failed"
