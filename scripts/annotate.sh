#!/bin/sh
# Populate per-flag effectiveness weights (w_speed / w_size) in a config file
# from measured data, so the search can order/bias options (see the
# "Effectiveness-ranked options" notes in README and osearch's -k/-Q/-r).
#
# Usage:
#   ./scripts/annotate.sh <measure_config> <target_config>
#
#   <measure_config>  config searched to MEASURE per-flag effect (use the
#                     curated subset: smaller and faster, and exactly the
#                     high-prior flags worth ranking).
#   <target_config>   config REWRITTEN in place with w_speed/w_size attributes.
#                     Typically the full superset config; flags absent from
#                     <measure_config> keep weight 0 (reached only in an
#                     uncapped "audit" run, never in quick -k/-Q mode).
#
# Method: run a -l 1 search per benchmark in size mode (-> w_size) and
# perf/retired-instruction mode (-> w_speed). Each run's JSON gives every
# sampled option's marginal effect at the local optimum; weights are the
# per-objective mean effect, normalised to small integers. The search is capped
# with -Q (QCAP, default 60) for practicality — an uncapped sweep over the full
# flag set is minutes per benchmark; the cap covers the high-prior options,
# which is all the ranking needs. Weights are a soft PRIOR: they only order/bias
# the search, never exclude. Regenerate as compilers change.
#
# Requires: osearch built in ./build/, python3.
set -e

MEASURE_CFG="$1"
TARGET_CFG="$2"
QCAP="${3:-60}"
if [ -z "$MEASURE_CFG" ] || [ -z "$TARGET_CFG" ]; then
  echo "usage: $0 <measure_config> <target_config> [qcap]" >&2
  exit 1
fi

OSEARCH="./build/osearch"
[ -x "$OSEARCH" ] || { echo "Error: $OSEARCH not found. Build first." >&2; exit 1; }

BENCHMARKS="\
benchmarks/distbench.c \
benchmarks/mat1bench.c \
benchmarks/almabench.c \
benchmarks/fftbench.c \
benchmarks/linbench.c \
benchmarks/evobench.c \
benchmarks/treebench.c \
benchmarks/huffbench.c"

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

for mode in size perf; do
  case "$mode" in size) F="-s" ;; perf) F="-p" ;; esac
  echo "Measuring $mode (uncapped -l 1) on $MEASURE_CFG ..." >&2
  for bench in $BENCHMARKS; do
    name=$(basename "$bench" .c)
    echo "  $mode $name..." >&2
    "$OSEARCH" -j -l 1 $F -Q "$QCAP" -q "$MEASURE_CFG" "$bench" \
        > "$TMPDIR/${mode}_${name}.json" 2>/dev/null || true
  done
done

echo "Injecting weights into $TARGET_CFG ..." >&2
python3 "$(dirname "$0")/annotate.py" "$TMPDIR" "$MEASURE_CFG" "$TARGET_CFG"
