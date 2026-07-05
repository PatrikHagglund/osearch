# TODO

All open work items. Items 1–2 are measurement/exploration notes (with
findings recorded inline); item 3 is the longer-term design backlog. The
methodology behind the search and its objectives is documented in
[README.md § Search methodology](README.md#search-methodology).

## 1. Profile-guided optimization (PGO) as a searchable option

**Status:** done, 2026-07-05 — step 1 (fixed A/B, results below) gave a clear
go; step 2 is implemented: `scripts/cc-pgo.sh` + the `-fprofile-use`
pseudo-flag in both configs (verified to reproduce the A/B numbers exactly,
and adopted by a quick `-p` search on clang/treebench). **Remaining:** re-run
the RESULTS.md tables with the flag available.

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

### Step 1 findings (2026-07-05)

Each benchmark's best-known `-p` flag set (RESULTS.md tables), rebuilt ± PGO.
All baselines reproduced the published counts to within a few instructions
(note: Clang was bumped 22.1.7 → 22.1.8 by a package update during setup —
counts unaffected). PGO builds are just as deterministic as plain ones
(repeat runs within a few instructions). Δ = PGO vs base, retired instructions:

| Benchmark | GCC 16 Δ | Clang 22 Δ |
|-----------|---------:|-----------:|
| distbench | +0.01%   | +0.00%     |
| mat1bench | −0.01%   | −0.00%     |
| almabench | −0.10%   | −0.07%     |
| fftbench  | −0.00%   | −0.82%     |
| linbench  | **+3.01%** | −0.49%   |
| evobench  | +0.09%   | **+15.67%** |
| treebench | **+1.06%** | **−19.93%** |
| huffbench | **−3.63%** | **−3.77%** |

- **Far from flat, and benchmark-specific in *both* directions** (−19.9% to
  +15.7%) — exactly the profile of a good *searchable* option: the greedy
  search will adopt it where it wins (Clang treebench, huffbench both
  compilers) and reject it where it regresses (Clang evobench, GCC linbench).
  As a blanket default it would be a wash.
- The vectorized FP kernels (distbench, mat1bench, almabench, fftbench) are
  essentially immune — their hot loops are already shaped by
  `-ffast-math -march=native`; PGO has nothing to add that `-p` can see.
- **Clang treebench −19.9%:** the big win. All hot B-tree functions
  (`promote_internal`, `redistribute`, `concatenate`) got *smaller* and the
  dynamic count dropped 20% — profile-driven inlining/if-conversion doing
  real work on the branch-heaviest benchmark.
- **Clang evobench +15.7%:** PGO's `optimize()` is *smaller* (2811 B vs
  3088 B) yet retires 16% more instructions — the profile talked Clang out of
  unrolling/vectorizing the hot loop (likely optimizing expected cycles, not
  count). A concrete instance of TODO item 2: `-p` may be *mis-scoring* this
  binary; on a cycle metric it might actually be a win. Re-check once a
  µops/cycles counter exists.
- **Re-scored under a cycle counter (same day — see item 2 findings):** the
  two "regressions" are artifacts of counting instructions (clang evobench
  +15.7% → +0.5% cycles; GCC linbench +3.0% → ~0%), and the wins are larger
  in cycles (clang treebench −24%). `-p` under-credits PGO, as predicted.
- `.text` moves a lot under PGO, usually down (Clang distbench 5441 → 2541 B:
  cold-path splitting) but sometimes up (GCC treebench 10465 → 17899 B). If a
  PGO pseudo-flag is ever offered in `-s` mode it needs its own weight.
- Two checksum footnotes, both FP-tolerance, not correctness: GCC linbench
  differs in the last 3 of 17 digits (fast-math reassociation changes with
  different unrolling — same tolerance as the GCC-vs-Clang comparison), and
  fftbench's checksum is *ill-conditioned* for cross-build comparison (it sums
  a spectrum that cancels to ≈0; published value −2.8e−09, so an absolute
  wobble of 5e−10 looks huge in relative terms). Worth fixing separately:
  sum `|re| + |im|` instead, making the fftbench checksum well-conditioned.

**Step 2 — wrapper integration (go).**

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

### First-step findings (2026-07-05)

Done with a scratchpad probe variant of `main.ic` (`-g`: group of
instructions-leader + raw `0xC1` `ex_ret_ops` + cycles, read atomically):
21 runs pinned (`taskset -c 5`) for all 16 best-flag builds, the 5 big PGO
movers from item 1, and 2 unpinned controls.

**Determinism, run-to-run per binary:**

- *Instructions:* exact (spread ≤ 1 count) — as known.
- *Retired µops:* spread ≤ ~900 ppm, CV ≤ 240 ppm. Deterministic enough for
  any realistic `-T` threshold — essentially free fidelity.
- *Cycles, pinned:* workload-dependent. CV ≈ 0.05–0.1% on compute-bound
  binaries (distbench, evobench) but up to 9% on memory-bound ones. Noise is
  one-sided (interference only adds cycles), so **min-of-N beats median**:
  median sits within ~1% of min for 20 of 23 cases. The pathology is GCC
  fftbench: median 28% above min — its 16 MB working set makes cycle cost
  depend on run-to-run physical page placement; irreducible by repetition.
- *Cycles, unpinned:* CV up to 22%. Pinning is mandatory.

**Fidelity — cycles re-rank the head-to-head** (medians, pinned; Δ = winner
vs loser):

| Benchmark | GCC cycles | Clang cycles | Winner (cycles) | (was, insns) |
|-----------|-----------:|-------------:|-----------------|--------------|
| distbench | 32.1M | 32.4M | ~tie (GCC −0.8%) | GCC −40% |
| mat1bench | 55.1M | 168.2M | GCC −67% | GCC −47% |
| almabench | 166.8M | 107.4M | Clang −36% | Clang −38% |
| fftbench | 180.4M | 115.9M | Clang −36% | Clang −43% |
| linbench | 116.7M | 123.7M | GCC −6% | GCC −12% |
| evobench | 617.3M | 613.0M | Clang −0.7% | Clang −2% |
| treebench | 658.7M | 634.2M | Clang −4% | Clang −7% |
| huffbench | 886.3M | 434.8M | **Clang −51%** | **GCC −16%** |

- *distbench:* the "GCC −40%" evaporates — Clang's 24 shuffles per iteration
  cost instruction count, not time (the port pipes them fine). Cycles: a tie.
- *mat1bench:* the gap *grows* to −67%. µops caught this exactly: Clang's
  gather kernel is 586M µops from 54M instructions (10.8× microcode
  expansion) vs GCC's 27.7M µops.
- *huffbench flips outright:* GCC retires 16% fewer instructions **and** 24%
  fewer µops, yet takes 2.0× the cycles (IPC 0.93 vs 2.45) — stall-dominated,
  invisible to *both* deterministic counters.
- Cycle scoreboard: Clang 5, GCC 2, one tie — vs 4–4 on instructions.

**PGO re-scored in cycles** (cross-ref item 1): clang evobench "+15.7%
instructions" is **+0.5% cycles** (not a real regression — hypothesis
confirmed), GCC linbench "+3.0%" is +0.3% median / −1.5% min (also ~neutral),
and the wins grow: clang treebench −24%, GCC huffbench −8.6%, clang huffbench
−6.5%. PGO binaries also run 5–20× *steadier* in cycles (clang treebench CV
2.0% → 0.2%) — better layout stabilizes timing. Conclusion: `-p`
systematically under-credits PGO.

**Integration status (2026-07-05):**

1. ✅ `benchmarks/main.ic`: `-u` (retired ops), `-c` (cycles, self-pinned via
   `sched_setaffinity`) and `-g` (all three counters in one atomic
   `PERF_FORMAT_GROUP` read) implemented alongside `-p`, which is unchanged.
2. ✅ osearch: `-u` objective (deterministic, single sample) and `-c`
   objective (min of `-n` samples — cycle noise is one-sided) pass through to
   the harness. µops is the recommended speed objective on this host; cycles
   the audit metric (fftbench-class memory-layout variance won't converge
   regardless of `-n`).
3. ⬜ `scripts/aggregate.sh` / RESULTS.md: add a cycles column next to `-p`
   (via the harness `-g` mode on each winner binary) so count-vs-time
   divergence (distbench, huffbench) is visible in the tables — natural to do
   together with the pending PGO-enabled re-run of the tables (item 1).

## 3. Design backlog (moved from README)

- **Non-greedy search** to escape greedy local optima: simulated annealing /
  genetic (as in the ACOVEA ancestor). The `-l 1` greedy can't reach optima
  needing a coordinated multi-flag move — e.g. almabench `-p`, where `-flto`
  only helps in a specific co-set (see RESULTS.md "Search limitations"). The
  restart seeding below is the lighter near-term mitigation.
- **Restart seeding** — bias a future `random_point()` so an option's
  on-probability rises with its effectiveness weight, making `-r` restarts
  begin near promising regions (the proper multi-restart escape for greedy
  local optima; pure-random restart proved impractically slow).
- **Parallel measurement** — sample multiple flag sets in parallel (extend
  `compile_batch()` to cover measurement), but beware contention between
  measurements: fine for `-s`/`-p`/`-u` (deterministic), wrong for `-c`/time.
- **C++26 reflection** (`-freflection`) for JSON serialization and CLI option
  registration, once compiler support matures (blocked on compiler support).
