#!/bin/sh
# Cross-benchmark aggregation: run osearch on all benchmarks, rank flags
# by average improvement, and optionally reorder the config file.
#
# Usage:
#   ./aggregate.sh [--mode=time|perf|size] [--reorder] [config_file]
#
#   --mode=time  (default) execution time
#   --mode=perf  retired instructions (needs Linux perf_event support)
#   --mode=size  binary .text size (deterministic)
#
# Requires: osearch built in ./build/, python3 for JSON parsing.
set -e

MODE=time
REORDER=false
CONFIG=""

for arg in "$@"; do
  case "$arg" in
    --mode=time) MODE=time ;;
    --mode=perf) MODE=perf ;;
    --mode=size) MODE=size ;;
    --reorder)   REORDER=true ;;
    --*)         echo "Unknown option: $arg" >&2; exit 1 ;;
    *)           CONFIG="$arg" ;;
  esac
done

# Default config depends on mode.
if [ -z "$CONFIG" ]; then
  case "$MODE" in
    size) CONFIG="config/gcc16-size.osearch" ;;
    *)    CONFIG="config/gcc16-test.osearch" ;;
  esac
fi

case "$MODE" in
  time) OSEARCH_MODE_FLAG="" ;;
  perf) OSEARCH_MODE_FLAG="-p" ;;
  size) OSEARCH_MODE_FLAG="-s" ;;
esac
OSEARCH="./build/osearch"
# Explicit benchmark order: FP-heavy first (benefits from -Ofast), then
# integer-heavy at the end (prefers -O3). Within each group, sorted by
# instruction count (ascending).
BENCHMARKS="\
benchmarks/distbench.c \
benchmarks/mat1bench.c \
benchmarks/almabench.c \
benchmarks/fftbench.c \
benchmarks/linbench.c \
benchmarks/evobench.c \
benchmarks/treebench.c \
benchmarks/huffbench.c"
Q=20  # quick cap per level (enough to get past -O levels to actual flags)

if [ ! -x "$OSEARCH" ]; then
  echo "Error: $OSEARCH not found. Build first." >&2
  exit 1
fi

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

echo "Running osearch -j -l 1 -Q $Q${OSEARCH_MODE_FLAG:+ $OSEARCH_MODE_FLAG} on all benchmarks (mode=$MODE)..." >&2

for bench in $BENCHMARKS; do
  name=$(basename "$bench" .c)
  echo "  $name..." >&2
  "$OSEARCH" -j -l 1 -Q "$Q" $OSEARCH_MODE_FLAG -q "$CONFIG" "$bench" > "$TMPDIR/$name.json" 2>/dev/null || true
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

if [ "$REORDER" = true ]; then
  echo "" >&2
  echo "Reordering $CONFIG..." >&2
  python3 - "$TMPDIR" "$CONFIG" <<'REORDER_PY'
import json, sys, os, re
from collections import defaultdict

results_dir = sys.argv[1]
config_path = sys.argv[2]

# Collect flag rankings (same logic as above).
flag_diffs = defaultdict(list)
for fname in sorted(os.listdir(results_dir)):
    if not fname.endswith('.json'):
        continue
    with open(os.path.join(results_dir, fname)) as f:
        content = f.read()
    if '[' not in content:
        continue
    try:
        data = json.loads(content[content.rfind('['):])
    except json.JSONDecodeError:
        continue
    for entry in data:
        if entry.get('equal'):
            continue
        diff = entry.get('diff')
        if diff is None:
            continue
        flags_field = entry.get('flags', '')
        flag_name = re.sub(r'^-?\d+\s+', '', flags_field).strip()
        if flag_name.startswith('!'):
            flag_name = flag_name[1:]
            diff = -diff
        if flag_name:
            flag_diffs[flag_name].append(diff)

# Rank: highest average diff first (most impactful flags first).
rank = {flag: sum(diffs)/len(diffs) for flag, diffs in flag_diffs.items()}

# Read config, reorder <flag> lines by rank (unranked flags go last).
with open(config_path) as f:
    lines = f.readlines()

flag_lines = []
other_lines = []
for line in lines:
    if re.match(r'\s*<flag\s', line):
        flag_lines.append(line)
    else:
        other_lines.append((len(flag_lines), line))

def flag_sort_key(line):
    m = re.search(r'value="([^"]+)"', line)
    if m:
        val = m.group(1)
        return rank.get(val, 0)  # highest diff = most impactful
    return 0

flag_lines.sort(key=flag_sort_key, reverse=True)

# Reconstruct: other lines in original positions, flags reordered.
with open(config_path, 'w') as f:
    fi = 0
    for orig_fi, line in other_lines:
        while fi < orig_fi and fi < len(flag_lines):
            f.write(flag_lines[fi])
            fi += 1
        f.write(line)
    while fi < len(flag_lines):
        f.write(flag_lines[fi])
        fi += 1

print(f"Reordered {len(flag_lines)} flags in {config_path}", file=sys.stderr)
REORDER_PY
fi
