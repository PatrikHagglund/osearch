# TODO — exploration notes

Working notes for measurement/exploration work. Longer-term *design* TODOs
(non-greedy search, `-r` restart seeding, C++26 reflection) live in
[README.md § TODO](README.md#todo).

## 1. Profile-guided optimization (PGO) as a searchable option

**Status:** exploring — step 1 (fixed A/B) running.

PGO is not a flag but a build *protocol* — instrument → training run →
recompile — so it can't be a plain `<flag>` line in a config. It can still be
integrated: `execute()` runs the prime command through `popen()` (i.e.
`/bin/sh -c`, see `src/execute.cc`), so the prime command can be a wrapper
script that expands a pseudo-flag into the three-phase build.

**Step 1 — fixed A/B (cheap, decides go/no-go).** Take the best-known `-p`
flag set per benchmark per compiler from RESULTS.md and rebuild each ± PGO by
hand: 8 benchmarks × 2 compilers, measure retired instructions (`bin -p`),
`.text` size, and verify the `-v` checksum is unchanged. If PGO moves `-p` by
more than the ~±10-instruction noise floor on several benchmarks, proceed to
step 2; if it is flat, record the negative result here and stop.

**Step 2 — wrapper integration (only if step 1 pays).**

- `scripts/cc-pgo.sh` + one pseudo-flag per config, e.g.
  `<flag type="simple" value="-fprofile-use" w_speed="?" w_size="0" />`;
  prime command becomes
  `scripts/cc-pgo.sh gcc -std=gnu23 OSEARCH_OPTS -o OSEARCH_OUT OSEARCH_IN`.
- Wrapper: if `-fprofile-use` absent → exec the compiler unchanged (zero
  overhead for every other candidate). If present → strip it and run:
  instrumented compile → one training run (output discarded) → final compile
  with profile feedback.
- **Profile isolation is mandatory:** `compile_batch()` compiles candidates
  from parallel threads, so each PGO build needs its own `mktemp -d` profile
  directory (GCC `-fprofile-dir=$D`; concurrent builds would otherwise corrupt
  each other's `.gcda`/`.profraw`).
- Per-compiler difference absorbed by the wrapper — GCC:
  `-fprofile-generate` / `-fprofile-use` (+ `-fprofile-dir`); Clang:
  `-fprofile-generate=$D` → `llvm-profdata merge` → `-fprofile-use=$D/prof.profdata`.

**Costs / caveats to keep in mind:**

- Search time roughly triples once the flag is adopted: every candidate build
  becomes two compiles plus a full (instrumented, 2–5× slower) benchmark run.
- `-p` understates PGO: code layout, I-cache, and branch-prediction wins don't
  change the retired-instruction count. What `-p` sees is PGO's effect on
  inlining / unrolling / if-conversion / value-specialization decisions.
- Training input == measurement input here, so this measures *best-case* PGO.
  That is the question being asked ("what can perfect profile knowledge add on
  top of flag search"), but label it as such in RESULTS.md.
- `-fauto-profile` (sampling-based) is not viable in this container — no
  `perf` binary.

## 2. A better objective than raw instruction count (`-p`)

**Problem:** retired instructions ≠ time. RESULTS.md documents the failure
mode: a zmm `vgatherqpd` counts as one instruction but decodes into many µops
(Clang's mat1bench loss costs more in cycles than the −47% count gap shows),
and shuffle-heavy code is similarly undercounted. Wall-clock time is too noisy
on this shared host to optimize on; `-p` was chosen for its ~1 ppm
determinism, not its fidelity.

Candidates, in rough order of promise:

1. **Retired µops** — Zen 4's `ex_ret_ops` PMU event (raw event, PMC 0xC1)
   via the same in-harness `perf_event_open`. Retired-µop counts should be
   near-deterministic like instruction counts (they count architectural
   retirement, not speculation) while pricing gathers/shuffles/divides far
   more honestly. Likely the best determinism-vs-fidelity trade.
2. **Cycles** (`PERF_COUNT_HW_CPU_CYCLES`) — the true objective, but subject
   to frequency scaling and neighbor noise on a shared host. Quantify the
   actual run-to-run spread (pinned CPU, N repeats, median) before judging;
   might be usable with the existing `-n`/`-T` noise controls.
3. **Hybrid** — *optimize* on a deterministic counter (instructions or µops),
   *report* cycles/time as a sanity column in RESULTS.md, flagging benchmarks
   where the two objectives disagree.
4. **Static modeling** (`llvm-mca` on the hot loops) — deterministic, but
   models loops in isolation, not whole programs; a reporting aid, not an
   objective.

**First step:** extend `benchmarks/main.ic` to open a *group* of counters
(instructions + µops + cycles read together in one `perf_event_open` group —
one run measures all three), behind a new option letter. Then measure each
counter's run-to-run spread over ~20 runs on the gather/shuffle-heavy binaries
(Clang mat1bench/distbench winners) to pick the objective empirically.
