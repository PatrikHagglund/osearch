#!/bin/sh
# Cross-benchmark aggregation: run osearch on all benchmarks, rank flags
# by average improvement, and print a reordered flag list.
#
# Usage:
#   ./aggregate.sh [config_file]
#
# Requires: osearch built in ./build/, python3 for JSON parsing.
set -e

CONFIG="${1:-config/gcc16-test.osearch}"
OSEARCH="./build/osearch"
BENCHMARKS="benchmarks/*.c"
Q=20  # quick cap per level (enough to get past -O levels to actual flags)

if [ ! -x "$OSEARCH" ]; then
  echo "Error: $OSEARCH not found. Build first." >&2
  exit 1
fi

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

echo "Running osearch -j -l 1 -Q $Q on all benchmarks..." >&2

for bench in $BENCHMARKS; do
  name=$(basename "$bench" .c)
  echo "  $name..." >&2
  "$OSEARCH" -j -l 1 -Q "$Q" -q "$CONFIG" "$bench" > "$TMPDIR/$name.json" 2>/dev/null || true
done

echo "" >&2
echo "Aggregating results..." >&2

# Parse all JSON files, extract flag→diff pairs, compute average diff per flag.
python3 - "$TMPDIR" <<'PYTHON'
import json, sys, os, re
from collections import defaultdict

results_dir = sys.argv[1]
flag_diffs = defaultdict(list)  # flag_name -> [diff values]

for fname in sorted(os.listdir(results_dir)):
    if not fname.endswith('.json'):
        continue
    path = os.path.join(results_dir, fname)
    with open(path) as f:
        content = f.read().strip()
    if not content or '[' not in content:
        continue
    # Extract the JSON array (skip any text before it)
    json_start = content.rfind('[')
    json_str = content[json_start:]
    try:
        data = json.loads(json_str)
    except json.JSONDecodeError:
        continue
    for entry in data:
        if entry.get('equal'):
            continue  # skip flags that had no effect
        diff_str = str(entry.get('diff', ''))
        if diff_str == 'inf':
            continue  # skip failed compilations
        try:
            diff = int(diff_str)
        except ValueError:
            continue
        # Extract flag name from the "flags" field (format: "diff flagname")
        flags_field = entry.get('flags', '')
        # Remove leading diff number and space
        flag_name = re.sub(r'^-?\d+\s+', '', flags_field).strip()
        if flag_name.startswith('!'):
            flag_name = flag_name[1:]  # removal of flag
            diff = -diff  # invert: removing a good flag is bad
        if flag_name:
            flag_diffs[flag_name].append(diff)

# Rank by average diff (lower = better improvement)
ranked = sorted(flag_diffs.items(), key=lambda x: sum(x[1]) / len(x[1]))

print("# Flags ranked by average improvement (negative = faster)")
print("# flag\tavg_diff\tcount")
for flag, diffs in ranked:
    avg = sum(diffs) / len(diffs)
    print(f"{flag}\t{avg:.0f}\t{len(diffs)}")
PYTHON
