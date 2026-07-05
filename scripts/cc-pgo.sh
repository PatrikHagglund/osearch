#!/bin/bash
# Compile wrapper that turns the pseudo-flag -fprofile-use into a full
# profile-guided-optimization build: instrument -> training run -> recompile
# with the profile. The config prime commands route every compile through
# this script; when -fprofile-use is NOT among the options it execs the
# compiler unchanged, so the wrapper costs nothing for ordinary candidates.
#
# The training input is the benchmark itself (deterministic), i.e. this
# measures best-case PGO — see TODO.md item 1.
#
# Usage (from a config prime command, run from the repo root):
#   scripts/cc-pgo.sh gcc -std=gnu23 OSEARCH_OPTS -o OSEARCH_OUT OSEARCH_IN
set -u

pgo=0
args=()
for a in "$@"; do
  if [ "$a" = "-fprofile-use" ]; then pgo=1; else args+=("$a"); fi
done

# No pseudo-flag: plain compile.
[ $pgo = 1 ] || exec "$@"

# Output file (the instrumented binary is built at the same path, run for
# training, then overwritten by the final compile — identical command and
# -o both phases keeps GCC's .gcda naming consistent).
out=""
for ((i = 0; i + 1 < ${#args[@]}; ++i)); do
  [ "${args[i]}" = "-o" ] && out="${args[i+1]}"
done
if [ -z "$out" ]; then
  echo "cc-pgo.sh: no -o output file among arguments" >&2
  exit 1
fi

# Per-invocation profile directory: candidates compile from parallel
# threads (compile_batch), so profiles must not be shared.
dir=$(mktemp -d) || exit 1
trap 'rm -rf "$dir"' EXIT

case "$(basename "$1")" in
  gcc*|g++*)
    "${args[@]}" -fprofile-generate -fprofile-dir="$dir" &&
    "$out" >/dev/null 2>&1 &&
    "${args[@]}" -fprofile-use -fprofile-dir="$dir"
    ;;
  clang*)
    "${args[@]}" -fprofile-generate="$dir" &&
    "$out" >/dev/null 2>&1 &&
    llvm-profdata merge -o "$dir/prof.profdata" "$dir"/*.profraw &&
    "${args[@]}" -fprofile-use="$dir/prof.profdata"
    ;;
  *)
    echo "cc-pgo.sh: unrecognized compiler '$1'" >&2
    exit 1
    ;;
esac
