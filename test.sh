#!/bin/sh
# Smoke test: build and run osearch on a single benchmark.
set -e

# Build if needed
if [ ! -x ./osearch ]; then
  if [ -d build ]; then
    ninja -C build
    OSEARCH=./build/osearch
  else
    echo "No osearch binary found. Build first." >&2
    exit 1
  fi
else
  OSEARCH=./osearch
fi

echo "=== Smoke test: fftbench with gcc48-test config ==="
$OSEARCH config/gcc48-test.osearch benchmarks/fftbench.c

echo ""
echo "=== Verify DEBUG build compiles ==="
g++ -std=gnu++26 -DDEBUG -I ~/GSL/include -fsyntax-only *.cc
echo "DEBUG build: OK"

echo ""
echo "=== PASS ==="
