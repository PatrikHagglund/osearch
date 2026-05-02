#!/bin/sh
# Smoke tests for osearch.
set -e

if [ -x ./build/osearch ]; then
  OSEARCH=./build/osearch
elif [ -x ./osearch ]; then
  OSEARCH=./osearch
else
  echo "No osearch binary found. Build first." >&2
  exit 1
fi

echo "=== Test: default (single flag search) ==="
$OSEARCH config/gcc16-test.osearch benchmarks/fftbench.c

echo ""
echo "=== Test: -l 2 (flag pair search) ==="
$OSEARCH -l 2 config/gcc16-test.osearch benchmarks/fftbench.c

echo ""
echo "=== Test: -s (optimize for size) ==="
$OSEARCH -s config/gcc16-test.osearch benchmarks/fftbench.c

echo ""
echo "=== Test: -i (extra compile options) ==="
$OSEARCH -i "-march=native" config/gcc16-test.osearch benchmarks/fftbench.c

echo ""
echo "=== Test: -q (quiet mode) ==="
OUT=$($OSEARCH -q config/gcc16-test.osearch benchmarks/fftbench.c)
if echo "$OUT" | grep -q "^Progress indicators"; then
  echo "FAIL: -q did not suppress progress output" >&2; exit 1
fi
echo "OK: progress output suppressed"

echo ""
echo "=== Verify DEBUG build compiles ==="
g++ -std=gnu++26 -DDEBUG -I ~/GSL/include -fsyntax-only *.cc
echo "DEBUG build: OK"

echo ""
echo "=== ALL PASS ==="
