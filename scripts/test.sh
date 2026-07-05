#!/bin/sh
# Tests for osearch.
#
# Usage:
#   ./scripts/test.sh --fast    # unit tests + cheap smoke checks (seconds)
#   ./scripts/test.sh           # full suite: unit + integration search runs (minutes)
set -e

MODE=full
case "$1" in
  --fast) MODE=fast ;;
  "" ) ;;
  *) echo "Unknown option: $1" >&2; echo "Usage: $0 [--fast]" >&2; exit 2 ;;
esac

if [ -x ./build/osearch ]; then
  OSEARCH=./build/osearch
elif [ -x ./osearch ]; then
  OSEARCH=./osearch
else
  echo "No osearch binary found. Build first." >&2
  exit 1
fi

# ---------------------------------------------------------------------------
# Fast layer: unit tests + trivial smoke checks.
# ---------------------------------------------------------------------------

echo "=== Unit tests (ctest) ==="
ctest --test-dir build --output-on-failure

echo ""
echo "=== Verify DEBUG build compiles ==="
g++ -std=gnu++26 -DDEBUG -I ~/GSL/include -fsyntax-only src/*.cc
echo "DEBUG build: OK"

if [ "$MODE" = fast ]; then
  echo ""
  echo "=== FAST PASS (skipped integration search runs; use ./scripts/test.sh for full) ==="
  exit 0
fi

# ---------------------------------------------------------------------------
# Slow layer: full integration search runs (each compiles + executes many
# variants of a real benchmark; takes on the order of minutes).
#
# All runs are capped (-k): these tests exercise code paths and output
# formats, not search exhaustiveness — and an uncapped time-mode search can
# noise-adopt the -fprofile-use pseudo-flag early, after which every
# remaining candidate pays cc-pgo.sh's three-phase build (hours, not
# minutes). The caps still rank -fprofile-use high enough to exercise the
# PGO wrapper path.
# ---------------------------------------------------------------------------

echo ""
echo "=== Test: -q (quiet mode) ==="
OUT=$($OSEARCH -q -k 20 config/gcc16.osearch benchmarks/fftbench.c)
if echo "$OUT" | grep -q "^Progress indicators"; then
  echo "FAIL: -q did not suppress progress output" >&2; exit 1
fi
echo "OK: progress output suppressed"

echo ""
echo "=== Test: -Q (quick mode cap) ==="
OUT=$($OSEARCH -q -Q 5 config/gcc16.osearch benchmarks/fftbench.c)
if ! echo "$OUT" | grep -q "capped from"; then
  echo "FAIL: -Q did not produce 'capped from' message" >&2; exit 1
fi
echo "OK: -Q cap applied"

echo ""
echo "=== Test: -k (top-ranked subset) ==="
if $OSEARCH -q -k 10 config/gcc16.osearch benchmarks/fftbench.c >/dev/null 2>&1; then
  echo "OK: -k ran"
else
  echo "FAIL: -k run errored" >&2; exit 1
fi

echo ""
echo "=== Test: default (single flag search) ==="
$OSEARCH -k 20 config/gcc16.osearch benchmarks/fftbench.c

echo ""
echo "=== Test: -l 2 (flag pair search) ==="
$OSEARCH -l 2 -k 12 config/gcc16.osearch benchmarks/fftbench.c

echo ""
echo "=== Test: -s (optimize for size) ==="
$OSEARCH -s -k 20 config/gcc16.osearch benchmarks/fftbench.c

echo ""
echo "=== Test: -i (extra compile options) ==="
$OSEARCH -i "-march=native" -k 20 config/gcc16.osearch benchmarks/fftbench.c

echo ""
echo "=== ALL PASS ==="
