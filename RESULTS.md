# Benchmark Results

## Environment

- **CPU:** AMD EPYC 9354 (Zen 4) — AVX-512 + AVX2; `-march=native` ⇒ `znver4`
- **OS:** Fedora Linux 44 (container)
- **Compilers:** GCC 16.1.1, Clang 22.1.7
- **Date:** 2026-06-22
- **Configs:** `gcc16-test` (105 flags, speed *and* size) / `gcc16` (214, full);
  `clang22-test` (56) / `clang22` (103) — all under `config/`

`-p` counts are user-space retired instructions for `run()` only, via the
in-harness `perf_event_open` counter. Absolute size/time figures are larger
than pre-2026-06 revisions because DCE-prevention `volatile` sinks added real
code and work to several benchmarks' `run()`.

## Summary

- **For reproducible optimization use `-s` (size) or `-p` (instructions)** —
  both are deterministic. Wall-clock time is too noisy here to optimize on
  (see [Time](#time-default)).
- **One curated config per compiler.** `gcc16-test` and `clang22-test` each
  search both speed and size (their `-O` level is an enum spanning `-O3…-Os`),
  and each reproduces its full counterpart within ~1% — so the full configs
  are kept only as audits, not as separate result tables.
- **GCC vs Clang, instructions:** GCC 16 wins 5 of 8, Clang 3 (fftbench,
  evobench, treebench). Biggest gaps: mat1bench (GCC −47%), fftbench (Clang
  −43%). See [comparison](#gcc-16-vs-clang-22--instructions--p).
- **GCC vs Clang, size:** GCC wins 7 of 8 (all but treebench), several by
  ~3–14%. See [comparison](#gcc-16-vs-clang-22--size--s).

## Benchmarks

| Benchmark | Workload | Group |
|-----------|----------|-------|
| distbench | 3D point-distance sums, brute-force O(n²) `sqrt` | FP / vectorizable |
| mat1bench | dense N×N matrix multiply | FP / vectorizable |
| almabench | planetary ephemeris (`sin`/`cos`/`atan2`/`sqrt`) | FP / vectorizable |
| fftbench  | radix-2 Cooley–Tukey FFT, N = 2²⁰ | FP / vectorizable |
| linbench  | LU decomposition + solve | FP / vectorizable |
| evobench  | genetic-algorithm function optimizer | FP / vectorizable |
| treebench | B-tree insert / find / remove | integer / branch |
| huffbench | Huffman compress + decompress | integer / branch |

FP benchmarks prefer `-O3 -ffast-math`; the integer/branch ones prefer plain
`-O3`. Every table below uses this order (FP-heavy first, then integer-heavy),
matching `aggregate.sh`.

## Size optimization (-s), Level 1 (full search)

Config: `config/gcc16-test.osearch` (105 flags)

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 1045          | -Os -march=native -flto -fno-reorder-functions -fno-tree-loop-im -fno-if-conversion -fno-thread-jumps -fno-tree-sink -fno-tree-ter -fno-ssa-phiopt -fno-ipa-ra -fno-math-errno |
| mat1bench   | 1029          | -Os -flto -fno-reorder-functions -fno-tree-loop-im -fno-if-conversion -fno-thread-jumps -fno-tree-sink -fno-tree-ter -fno-ssa-phiopt -fira-region=all |
| almabench   | 2348          | -Os -ffast-math -march=native -flto -fno-gcse -fno-reorder-functions -fno-tree-vrp -fno-tree-coalesce-vars -fno-tree-sink |
| fftbench    | 1319          | -Os -march=native -flto -fno-reorder-functions -fno-tree-loop-optimize -fno-thread-jumps |
| linbench    | 1620          | -Os -march=native -flto -fno-caller-saves -fno-expensive-optimizations -fno-tree-vrp -ffp-contract=on -fno-if-conversion -fno-guess-branch-probability -fno-thread-jumps -fno-tree-sink -ffinite-math-only |
| evobench    | 1742          | -Os -march=native -flto -fno-caller-saves -fno-reorder-functions -fno-tree-tail-merge -fno-tree-ch -fno-tree-dce -fno-tree-sink -fno-ree |
| treebench   | 4238          | -Os -march=native -flto -fno-code-hoisting -fno-caller-saves -fno-optimize-sibling-calls -fno-reorder-functions -fno-tree-loop-im -fno-tree-loop-optimize -fno-tree-pre -fno-tree-dominator-opts -fno-forward-propagate -fno-thread-jumps -fno-dce -fira-region=all |
| huffbench   | 2007          | -Os -flto -fno-cse-follow-jumps -fno-code-hoisting -fno-caller-saves -fno-move-loop-invariants -fno-reorder-functions -fno-schedule-insns2 -fno-tree-loop-im -fno-if-conversion -fno-tree-coalesce-vars -fno-tree-dominator-opts -fno-thread-jumps -fno-tree-sink -fno-tree-ter -fno-tree-forwprop -fno-tree-scev-cprop -fgraphite |

### Size observations

- `-Os -flto` is the foundation for every benchmark
- `-march=native` helps the FP benchmarks (distbench, almabench, fftbench,
  linbench) but not the integer ones (mat1bench, treebench, huffbench —
  AVX encoding wastes bytes)
- `-fno-tree-sink`, `-fno-tree-ter`, `-fno-thread-jumps` and
  `-fno-reorder-functions` recur as the broad size-reducers
- treebench is the largest (4238 B) — recursive tree traversal has many code paths

## Instruction count (-p), Level 1 (full search)

Config: `config/gcc16-test.osearch` (105 flags)

Totals are reproducible to ~1 ppm per binary; the `-fno-*` tail beyond the
`-O`/`-march`/`-flto` base is adopted at the default `-T 0` threshold and
includes marginal picks that vary between runs (use `-T 3` for a stable set).

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 23,512,262      | -O2 -ffast-math -march=native -fno-align-functions -fno-align-loops -fno-reorder-functions -funroll-all-loops |
| mat1bench   | 28,962,064      | -O3 -ffast-math -march=native -flto -fno-align-functions -fno-caller-saves -fno-align-loops -fno-peephole2 -funroll-all-loops -ffp-contract=on -fno-tree-ter |
| almabench   | 346,401,630 ⚠️  | -O3 -ffast-math -march=native -fno-caller-saves -fno-move-loop-invariants -fno-peephole2 -fno-tree-slp-vectorize -fno-plt -frename-registers -flive-range-shrinkage -fno-guess-branch-probability -fno-tree-sra -fno-tree-ter |
| fftbench    | 448,263,898     | -Os -ffast-math -march=native -fno-peephole2 -fno-schedule-insns2 -fno-plt -freorder-blocks-algorithm=simple |
| linbench    | 93,945,051      | -O3 -ffast-math -march=native -fno-cse-follow-jumps -fno-align-loops -fno-gcse -fno-crossjumping -fno-peephole2 -fno-tree-loop-distribute-patterns -fno-tree-pre -funroll-all-loops -ffp-contract=on -fno-if-conversion -fno-if-conversion2 -fno-plt -fno-tree-coalesce-vars -flive-range-shrinkage -fno-thread-jumps |
| evobench    | 560,777,646     | -O3 -ffast-math -march=native -fno-cse-follow-jumps -fno-caller-saves -fno-ivopts -fno-move-loop-invariants -fno-optimize-sibling-calls -fno-schedule-insns2 -ffp-contract=on -fno-if-conversion -fno-plt -fno-tree-dominator-opts -fno-forward-propagate -fno-guess-branch-probability |
| treebench   | 845,338,030     | -O3 -flto -fno-code-hoisting -fno-align-functions -fno-align-loops -fno-peephole2 -fno-store-merging -fno-tree-loop-distribute-patterns -fno-tree-loop-vectorize -fno-tree-slp-vectorize -fno-if-conversion -fno-plt -fno-tree-coalesce-vars -flive-range-shrinkage -fno-sched-dep-count-heuristic -fno-early-inlining -fno-tree-reassoc -fno-gcse-after-reload |
| huffbench   | 885,420,323     | -O2 -march=native -flto -fno-cse-follow-jumps -fno-align-functions -fno-caller-saves -fno-align-loops -fno-ivopts -fno-optimize-sibling-calls -fno-peephole2 -fno-reorder-functions -fno-tree-loop-im -fno-tree-pre -fno-tree-vrp -fipa-cp-clone -fsplit-paths -funroll-all-loops -fno-if-conversion -fno-plt -fno-tree-coalesce-vars -finline-stringops -fno-forward-propagate -fno-sched-dep-count-heuristic -fno-ssa-phiopt -fno-tree-ccp -fno-tree-fre |

> ⚠️ **almabench:** the `-l 1` greedy search lands at 346.4M, ~8% above the
> best reachable result (319.6M). The winning flags *are* in this config, but
> the optimum needs a coordinated multi-flag combination: `-flto` only helps
> almabench in one specific co-set (giving 319.6M) and is strongly harmful
> elsewhere (+29% if added to the greedy's path), so single-flag hill-climbing
> can't reach it. See [Search limitations](#search-limitations).

### Instruction count observations

- All benchmarks build on `-O3`/`-O2` + `-ffast-math` (`-Ofast` = `-O3
  -ffast-math`); fftbench prefers `-Os`, distbench/huffbench `-O2`.
- `-funroll-all-loops` is a broad win (distbench, mat1bench, linbench, huffbench).
- `-fno-if-conversion` and `-fno-plt` recur on the branch-heavy / call-heavy
  benchmarks (linbench, evobench, treebench, huffbench).
- `-march=native` helps everyone except huffbench (pure integer/branch).
- `-flto` helps the larger benchmarks (mat1, tree, huff).
- treebench/huffbench pick up the most `-fno-*` flags (largest search surface).

## Full config audit

The 214-flag `config/gcc16.osearch` is kept only to audit the curated config,
not as a separate result set. Re-running it confirms `gcc16-test` reproduces
the exhaustive search within ~1% on every benchmark and metric — and often
beats it, because the smaller, higher-signal space steers the greedy search
away from bad branches. The lone exception is **almabench `-p`** (curated
346.4M vs full 319.6M, +8%), a greedy local-optimum (below). For size the
curated config matches or beats the full config on all 8 benchmarks. The
curated config got here by promoting the full config's winning flags into it
(`-funroll-all-loops`, `-fno-if-conversion`, `-fno-plt`, `-fno-tree-sink`,
the granular FP relaxations, …).

## Search limitations

osearch is greedy hill-climbing (`-l 1` toggles one flag at a time, keeping
improvements). Two effects surface in the tables:

- **Local optima.** When the best result needs a *coordinated* multi-flag
  combination, greedy can't reach it. almabench `-p` is the clear case:
  `-flto` helps it only together with a specific co-set (→ 319.6M) and is +29%
  *worse* on any other path, so single-flag steps reject it and settle at
  346.4M. Pair search (`-l 2`) explores two-flag moves but is impractically
  slow on a 105-flag config; multiple random restarts or a non-greedy search
  (the genetic approach of the ACOVEA ancestor) would be the real fix — see
  the README TODO.
- **Path sensitivity.** At the default `-T 0` the marginal `-fno-*` tail is
  noise-level, so the exact flag set — and occasionally the total (almabench
  `-p` ranges 311–346M) — varies between runs. The other benchmarks' totals
  are stable.

## Time (default)

No per-benchmark time table: wall-clock search is too noisy on this shared
host to optimize on. With the unified config's `-O` enum now exposing `-Os`,
the greedy search adopts `-Os` for compute-bound FP benchmarks (almabench,
fftbench) from a momentarily slow `-O3` baseline — even with `-n 3 -T 20`
(the doc's noise-robust settings) — producing results that are clearly *not*
fastest (e.g. almabench at `-Os` ≈ 84 ms vs ~51 ms at `-O3 -ffast-math`).
The old time tables only looked sane because the previous speed-only config
couldn't offer `-Os`.

Use `-s` or `-p` for reproducible optimization. If you must use time mode,
pin the CPU (`taskset`), disable turbo, set the `performance` governor, and
raise `-n`/`-T`; even then the greedy path can diverge (see
[Search limitations](#search-limitations) and Reproducibility below).

## Reproducibility

- **Size (-s):** 100% reproducible across runs
- **Instructions (-p):** totals reproducible to ~1 ppm; at the default
  `-T 0` the marginal `-fno-*` picks vary run-to-run, but `-T 3` yields a
  stable flag set (e.g. fftbench → `-O3 -march=native`)
- **Time (default):** too noisy on this shared host to optimize on — the
  greedy search even adopts `-Os` for compute-bound FP benchmarks. Use `-s`
  or `-p` instead (see [Time](#time-default)).

---

## Clang 22 — Instruction count (-p), Level 1

Config: `config/clang22-test.osearch` (56 flags)

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 39,092,625      | -O3 -ffast-math -march=native |
| mat1bench   | 55,116,556      | -O2 -ffast-math -march=native -flto |
| almabench   | 646,256,576     | -Os -ffast-math -march=native -flto -fno-slp-vectorize -fno-plt -mllvm -unroll-threshold=200 |
| fftbench    | 254,507,216     | -O2 -ffast-math -march=native -flto -fno-omit-frame-pointer -fno-plt -fno-builtin -mllvm -inline-threshold=300 |
| linbench    | 109,626,776     | -O3 -ffast-math -march=native -fno-inline-functions -fno-strict-overflow -fno-asynchronous-unwind-tables -fno-plt -mllvm -unroll-threshold=800 |
| evobench    | 557,615,961     | -O3 -ffast-math -march=native -flto -mllvm -enable-gvn-hoist |
| treebench   | 783,642,592     | -Os -march=native -fno-vectorize -fno-slp-vectorize -fno-delete-null-pointer-checks -fno-asynchronous-unwind-tables -fno-optimize-sibling-calls -fno-plt -mllvm -inline-threshold=1000 -mllvm -enable-gvn-hoist -mllvm -enable-newgvn |
| huffbench   | 1,190,829,141   | -O3 -march=native -flto -fno-omit-frame-pointer |

### Clang 22 observations

- `-ffast-math -march=native -flto` is the universal base; the `-O` level
  varies: evobench picks `-O3`, mat1bench/fftbench `-O2`, almabench/treebench
  `-Os` — lower levels minimise instruction count for several kernels.
- `-fno-plt` consistently helps (almabench, linbench, fftbench, treebench):
  avoids PLT indirection on hot calls.
- `-mllvm -enable-gvn-hoist` helps evobench and treebench (hoisting
  redundant code in branch-heavy traversal); treebench also takes
  `-enable-newgvn`.
- `-mllvm -inline-threshold=1000` is critical for treebench (recursive
  workload); fftbench tunes a lower `-inline-threshold=300`.
- `-mllvm -unroll-threshold` helps linbench (800) and almabench (200).

## Clang 22 — Instruction count (-p), Full config (103 flags)

Config: `config/clang22.osearch` (103 flags)

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 39,092,624      | -O3 -march=native -ffast-math -fno-plt |
| mat1bench   | 54,306,103      | -O3 -flto -march=native -ffast-math -fno-asynchronous-unwind-tables -fno-plt -mllvm -force-vector-width=8 |
| almabench   | 646,096,419     | -Os -flto=thin -march=native -ffast-math -fno-omit-frame-pointer -fno-plt -mno-vzeroupper -mllvm -unroll-threshold=200 |
| fftbench    | 254,507,216     | -O3 -flto -march=native -ffast-math -fno-plt -fno-builtin -mllvm -inline-threshold=300 |
| linbench    | 109,930,033     | -O3 -flto=thin -march=native -ffast-math -ffp-contract=on -fno-strict-overflow -fno-delete-null-pointer-checks -fno-plt |
| evobench    | 557,312,659     | -O3 -flto=thin -march=native -ffast-math -ffp-contract=on -fvisibility=hidden -mno-vzeroupper -mllvm -enable-gvn-hoist |
| treebench   | 782,312,747     | -Os -march=native -fno-slp-vectorize -fno-plt -fno-optimize-sibling-calls -fno-builtin -mno-vzeroupper -mllvm -extra-vectorizer-passes -mllvm -force-vector-width=8 -mllvm -inline-threshold=1000 -mllvm -unroll-threshold=200 -mllvm -enable-gvn-hoist |
| huffbench   | 1,190,827,231   | -O3 -flto=thin -march=native -fno-omit-frame-pointer -fno-plt |

### Clang full config observations

The 103-flag Clang config barely improves on the 56-flag test config: best
case mat1bench −1.5%, most within ±0.2%, and linbench is even slightly *worse*
(109.93M vs 109.63M — the greedy search diverges with the larger flag set).
This reflects Clang's design: `-O3` already enables nearly all optimization
passes, most extra `-mllvm` toggles are already-on defaults, and the granular
FP flags are subsumed by `-ffast-math`. So `clang22-test` matched its full
config out of the box — whereas `gcc16-test` only reached full-config quality
after promoting ~50 of the full config's winning flags into it (see
[Full config audit](#full-config-audit)). The Clang flags that show up beyond
the test config's picks:

- `-flto=thin` matches or beats full `-flto` on almabench, linbench,
  evobench, and huffbench (and links faster).
- `-mllvm -force-vector-width=8` helps mat1bench and treebench.
- `-ffp-contract=on` helps linbench and evobench.
- `-mno-vzeroupper` recurs (avoids `vzeroupper` insertion overhead).

## GCC 16 vs Clang 22 — Instructions (-p)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 | Clang 22 | Winner | Δ |
|-----------|--------|----------|--------|---|
| distbench | 23,512,262 | 39,092,625 | GCC | −40% |
| mat1bench | 28,962,064 | 55,116,556 | GCC | −47% |
| almabench | 346,401,630 | 646,256,576 | GCC | −46% |
| fftbench | 448,263,898 | 254,507,216 | Clang | −43% |
| linbench | 93,945,051 | 109,626,776 | GCC | −14% |
| evobench | 560,777,646 | 557,615,961 | Clang | −0.6% |
| treebench | 845,338,030 | 783,642,592 | Clang | −7.3% |
| huffbench | 885,420,323 | 1,190,829,141 | GCC | −26% |

GCC 16 wins 5 of 8 on instruction count (distbench −40%, mat1bench −47%,
almabench −46%, linbench −14%, huffbench −26%); Clang wins fftbench (−43%),
treebench (−7.3%), and evobench (−0.6%).

(Both columns are the curated configs at full-config quality, so this is an
apples-to-apples comparison. Earlier revisions measured GCC with a weaker
speed-only config and reported "Clang wins 5/8" — that was the config, not
the compiler. GCC almabench is the `-l 1` result of 346M; the reachable
319.6M would widen GCC's lead further.)

---

## Clang 22 — Size optimization (-s), Level 1

Config: `config/clang22-test.osearch` (56 flags)

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 1130          | -Oz -flto -ffp-contract=fast -fno-plt |
| mat1bench   | 1117          | -Oz -flto |
| almabench   | 2735          | -Oz -march=native -flto -fno-slp-vectorize -ffp-contract=fast -ffinite-math-only -fno-omit-frame-pointer -fno-builtin -mllvm -enable-newgvn |
| fftbench    | 1388          | -Oz -march=native -flto -ffp-contract=fast -fno-builtin -mllvm -enable-newgvn |
| linbench    | 1669          | -Oz -march=native -flto -ffinite-math-only |
| evobench    | 1790          | -Oz -march=native -flto -fno-inline-functions -fno-signed-zeros -fno-builtin -mllvm -enable-newgvn |
| treebench   | 4205          | -Oz -march=native -flto -fno-inline-functions -fno-optimize-sibling-calls -mllvm -enable-gvn-hoist -mllvm -enable-newgvn |
| huffbench   | 2214          | -Oz -flto -mllvm -enable-newgvn |

### Clang 22 size observations

- `-Oz -flto` wins for every benchmark (including fftbench)
- `-march=native` helps the FP benchmarks but not the integer ones
  (mat1bench, huffbench omit it)
- `-mllvm -enable-newgvn` now helps size broadly (fftbench, evobench,
  huffbench, almabench, treebench) — merging redundant code; `-enable-gvn-hoist`
  additionally helps treebench
- `-fno-builtin` helps a few benchmarks (avoids inlining libc)

## GCC 16 vs Clang 22 — Size (-s)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 (.text) | Clang 22 (.text) | Winner | Δ |
|-----------|----------------|------------------|--------|---|
| distbench | 1045 | 1130 | GCC | −8% |
| mat1bench | 1029 | 1117 | GCC | −8% |
| almabench | 2348 | 2735 | GCC | −14% |
| fftbench | 1319 | 1388 | GCC | −5% |
| linbench | 1620 | 1669 | GCC | −3% |
| evobench | 1742 | 1790 | GCC | −3% |
| treebench | 4238 | 4205 | Clang | −0.8% |
| huffbench | 2007 | 2214 | GCC | −9% |

GCC wins 7 of 8 on code size — only treebench goes to Clang, by 0.8%. GCC's
`-Os` is consistently tighter, by 3–14% (largest on almabench and huffbench).
(Earlier revisions had GCC winning only 5/8; the unified `gcc16-test` now
searches the size-reducing flags that previously lived only in the full
config, so its `-s` results improved across the board.)
