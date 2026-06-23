# Benchmark Results

## Environment

- **CPU:** AMD EPYC 9354 (Zen 4) — AVX-512 + AVX2; `-march=native` ⇒ `znver4`
- **OS:** Fedora Linux 44 (container)
- **Compilers:** GCC 16.1.1, Clang 22.1.7
- **Date:** 2026-06-22
- **Configs:** `gcc16-test` (57 flags) / `gcc16` (214) / `gcc16-size` (56);
  `clang22-test` (56) / `clang22` (103) — all under `config/`

`-p` counts are user-space retired instructions for `run()` only, via the
in-harness `perf_event_open` counter. Absolute size/time figures are larger
than pre-2026-06 revisions because DCE-prevention `volatile` sinks added real
code and work to several benchmarks' `run()`.

## Summary

- **For reproducible optimization use `-s` (size) or `-p` (instructions)** —
  both are deterministic. Wall-clock time is noisy and its flag picks diverge.
- **GCC vs Clang, instructions:** Clang 22 wins 5 of 8, GCC 3 (distbench,
  almabench, huffbench). Biggest gaps: fftbench (Clang −47%), almabench
  (GCC −46%). See [comparison](#gcc-16-vs-clang-22--instructions--p).
- **GCC vs Clang, size:** GCC wins 5 of 8, but most races are within a few
  percent. See [comparison](#gcc-16-vs-clang-22--size--s).
- **Full vs test config:** GCC's 214-flag config beats its 57-flag set by up
  to −59% (`-funroll-all-loops`, `-fno-if-conversion`); Clang's 103-flag config
  barely helps (≤1.5%) because `-O3` already enables nearly all passes.

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

Config: `config/gcc16-size.osearch` (56 flags)

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 1135          | -Os -march=native -flto |
| mat1bench   | 1079          | -Os -flto -fno-reorder-functions -fno-tree-loop-im |
| almabench   | 2506          | -Os -march=native -flto -fno-reorder-functions -fno-tree-vrp -fno-caller-saves -fno-tree-loop-im -fno-gcse |
| fftbench    | 1350          | -Os -march=native -flto -fno-reorder-functions -fno-move-loop-invariants -fno-caller-saves -fno-tree-loop-optimize |
| linbench    | 1673          | -Os -march=native -flto -fno-reorder-functions -fno-tree-vrp -fno-move-loop-invariants |
| evobench    | 1783          | -Os -march=native -flto -fno-reorder-functions -fno-move-loop-invariants |
| treebench   | 4330          | -Os -flto -fno-reorder-functions -fno-move-loop-invariants -fno-tree-loop-im -fno-tree-tail-merge -fno-code-hoisting -fno-optimize-sibling-calls -fno-tree-loop-optimize |
| huffbench   | 2128          | -Os -flto -fno-reorder-functions -fno-move-loop-invariants -fno-caller-saves -fno-tree-loop-im |

### Size observations

- `-Os -flto` is the foundation for every benchmark
- `-march=native` helps the FP benchmarks (distbench, almabench, fftbench,
  linbench, evobench) but not the integer ones (mat1bench, treebench,
  huffbench — AVX encoding wastes bytes)
- `-fno-reorder-functions` and `-fno-move-loop-invariants` help most benchmarks
- treebench is the largest (4330 B) — recursive tree traversal has many code paths

## Instruction count (-p), Level 1 (full search)

Config: `config/gcc16-test.osearch` (57 flags)

The total is reproducible to ~1 ppm across runs; the `-fno-*` flags beyond
the `-O`/`-march` base are adopted at the default `-T 0` threshold and
include marginal, noise-level picks that vary between runs (use `-T 3` for
a stable set).

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 28,013,326      | -O3 -ffast-math -march=native -flto -fno-align-loops |
| mat1bench   | 71,388,516      | -O3 -march=native -fno-inline-functions -fno-asynchronous-unwind-tables |
| almabench   | 351,675,664     | -O3 -ffast-math -march=native -fno-align-functions -fno-align-loops -fno-move-loop-invariants -fno-peephole2 -fno-reorder-functions -fno-tree-slp-vectorize |
| fftbench    | 476,580,150     | -O3 -ffast-math -march=native -flto -fno-caller-saves -fno-align-loops -fno-optimize-sibling-calls -fno-asynchronous-unwind-tables |
| linbench    | 146,559,925     | -O3 -ffast-math -march=native -flto -fno-align-functions -fno-peephole2 -fno-tree-loop-distribute-patterns -fno-tree-pre |
| evobench    | 563,218,850     | -O3 -ffast-math -march=native -flto -fno-code-hoisting -fno-gcse -fno-ivopts |
| treebench   | 852,135,816     | -O3 -ffast-math -march=native -flto -fno-cse-follow-jumps -fno-code-hoisting -fno-ipa-cp -fno-align-loops -fno-gcse -fno-optimize-sibling-calls -fno-tree-loop-distribute-patterns -fno-tree-loop-vectorize -fno-tree-slp-vectorize |
| huffbench   | 1,096,625,613   | -O3 -ffast-math -flto -fno-align-functions -fno-inline-small-functions -fno-move-loop-invariants -fno-peephole2 -fno-tree-pre -fno-tree-vrp |

### Instruction count observations

- All benchmarks build on `-O3` (the config exposes `-O3` and
  `-ffast-math` as separate flags; `-Ofast` = `-O3 -ffast-math`).
- The FP group adopts `-ffast-math`; the integer-heavy treebench/huffbench
  also pick it up, but its effect there is within `-T 0` noise — it does
  not change their integer codegen materially.
- `-march=native` helps everyone except huffbench (pure integer/branch
  workload), matching the workload grouping.
- `-flto` helps the larger benchmarks (fft, evo, tree, huff).
- `-fno-align-loops` recurs across most benchmarks (icache locality).
- treebench picks up the most `-fno-*` flags (recursive traversal has the
  largest search surface).

## Instruction count (-p), Full config (214 flags)

Config: `config/gcc16.osearch`

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 23,512,261      | -O3 -ffast-math -march=native -fno-align-functions -fno-caller-saves -fno-cprop-registers -fno-forward-propagate -fno-guess-branch-probability -funroll-all-loops |
| mat1bench   | 28,962,062      | -O3 -ffast-math -march=native -flto -funroll-all-loops -ffp-contract=on (+ many marginal -fno-* flags) |
| almabench   | 318,972,713     | -O3 -ffast-math -march=native -flto -fno-caller-saves -fno-code-hoisting -fno-if-conversion -fno-plt -fno-tree-coalesce-vars -fno-tree-pre -frename-registers (+ many marginal -fno-* flags) |
| fftbench    | 448,263,892     | -Os -march=native -fno-asynchronous-unwind-tables -fno-dse -fno-plt -fno-sched-dep-count-heuristic -freorder-blocks-algorithm=simple |
| linbench    | 93,222,726      | -O3 -ffast-math -march=native -funroll-all-loops -fno-if-conversion -fno-tree-pre -fno-tree-coalesce-vars -flive-range-shrinkage -ffp-contract=on (+ many marginal -fno-* flags) |
| evobench    | 561,052,637     | -O3 -ffast-math -march=native -flto -fno-if-conversion -fno-ivopts -fno-plt -fno-tree-dominator-opts -fsingle-precision-constant (+ marginal -fno-* flags) |
| treebench   | 848,010,023     | -O3 -flto -fno-if-conversion -fno-ipa-cp -fno-tree-loop-distribute-patterns -fno-tree-loop-vectorize (+ many marginal -fno-* flags) |
| huffbench   | 897,288,212     | -O3 -march=native -flto -fno-if-conversion -fno-tree-ch -finline-stringops -ftracer (+ many marginal -fno-* flags) |

### GCC full config observations

The 214-flag config beats the 57-flag test config on every benchmark
(largest gains: mat1bench −59%, linbench −36%, huffbench −18%). Notable
flags:

- **`-funroll-all-loops`** — large win on linbench (93.9M, −36% vs the
  57-flag config), plus mat1bench and distbench. Off by default even at `-O3`.
- **`-fno-if-conversion`** (and `-fno-if-conversion2`) — helps the
  branch-sensitive benchmarks (linbench, evobench, treebench, huffbench);
  keeping branches lets the predictor work.
- **`-fno-plt`** — broadly adopted (almabench, fftbench, evobench,
  huffbench): avoids PLT indirection on hot calls.
- **`-ffp-contract=on`** — helps mat1bench (FMA contraction).
- **`-finline-stringops`** — helps linbench and huffbench.
- **`-fno-tree-ch`** + **`-ftracer`** — both help huffbench (branch-heavy).
- fftbench is the outlier: the full search settles on **`-Os`** (448M),
  beating `-O3` on instruction count — smaller, tighter code paths win here.

## Time (default), Level 1 (full search, -n 3)

Config: `config/gcc16-test.osearch` (57 flags)

**Noisy** — results vary between runs. Use `-n 5` or higher for more stable results.

| Benchmark   | Time (µs) | Best flags |
|-------------|-----------|------------|
| distbench   | 10028     | -O3 -march=native |
| mat1bench   | 13026     | -O3 -ffast-math -march=native -flto -fno-peephole2 |
| almabench   | 51425     | -O3 -ffast-math -march=native -fno-caller-saves -fno-gcse -fno-move-loop-invariants -fno-tree-slp-vectorize |
| fftbench    | 50725     | -O3 -ffast-math -flto -fno-align-functions -fno-align-loops -fno-optimize-sibling-calls -fno-optimize-strlen -fno-peephole2 |
| linbench    | 33899     | -O3 -ffast-math -march=native -fno-code-hoisting -fno-caller-saves -fno-ivopts |
| evobench    | 192824    | -O3 -ffast-math -march=native -fno-cse-follow-jumps -fno-code-hoisting -fno-caller-saves |
| treebench   | 206701    | -O3 -ffast-math -march=native -flto -fno-cse-follow-jumps -fno-code-hoisting -fno-align-functions -fno-tree-loop-optimize |
| huffbench   | 136957    | -O3 -ffast-math -fno-inline-functions -fno-code-hoisting -fno-ipa-cp -fno-align-functions -fno-tree-loop-vectorize -fno-tree-tail-merge -fno-tree-vrp |

### Level 2, Q=10

Shallower search; faster but may miss combinations that deep greedy search finds.

| Benchmark   | Time (µs) | Best flags |
|-------------|-----------|------------|
| distbench   | 10015     | -O3 -march=native -fno-inline-functions |
| mat1bench   | 13054     | -O3 -ffast-math -march=native |
| almabench   | 53439     | -O3 -ffast-math -march=native -fno-align-functions |
| fftbench    | 54334     | -O3 -ffast-math -march=native |
| linbench    | 36771     | -O3 -ffast-math -march=native |
| evobench    | 192941    | -O3 -ffast-math -march=native |
| treebench   | 211958    | -O3 -flto |
| huffbench   | 148791    | -O3 -ffast-math -flto |

### Time observations

- `-O3 -ffast-math` (equivalent to the old `-Ofast`) wins most benchmarks;
  `-march=native` helps most
- `-flto` shows up inconsistently — time noise can hide or expose its benefit
- Level 1 and Level 2/Q10 reach similar best times (within time-mode noise);
  L2's greedy search tends to adopt fewer flags
- These are a single noisy snapshot (see Reproducibility) — treat the
  absolute numbers and marginal flags with caution

## Reproducibility

- **Size (-s):** 100% reproducible across runs
- **Instructions (-p):** totals reproducible to ~1 ppm; at the default
  `-T 0` the marginal `-fno-*` picks vary run-to-run, but `-T 3` yields a
  stable flag set (e.g. fftbench → `-O3 -march=native`)
- **Time (default, -n 3):** Values stable within ~10%, but greedy adoption
  paths can diverge due to residual noise. For noise-free optimization,
  use `-s` or `-p` instead.

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

Unlike GCC — where the full config beat the test config by up to −59% — the
103-flag Clang config barely improves on the 56-flag test config: best case
mat1bench −1.5%, most within ±0.2%, and linbench is even slightly *worse*
(109.93M vs 109.63M — the greedy search diverges with the larger flag set).
This reflects Clang's design: `-O3` already enables nearly all optimization
passes, most extra `-mllvm` toggles are already-on defaults, and the granular
FP flags are subsumed by `-ffast-math`. The flags that do show up beyond the
test config's picks:

- `-flto=thin` matches or beats full `-flto` on almabench, linbench,
  evobench, and huffbench (and links faster).
- `-mllvm -force-vector-width=8` helps mat1bench and treebench.
- `-ffp-contract=on` helps linbench and evobench.
- `-mno-vzeroupper` recurs (avoids `vzeroupper` insertion overhead).

## GCC 16 vs Clang 22 — Instructions (-p)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 | Clang 22 | Winner | Δ |
|-----------|--------|----------|--------|---|
| distbench | 28,013,326 | 39,092,625 | GCC | −28% |
| mat1bench | 71,388,516 | 55,116,556 | Clang | −23% |
| almabench | 351,675,664 | 646,256,576 | GCC | −46% |
| fftbench | 476,580,150 | 254,507,216 | Clang | −47% |
| linbench | 146,559,925 | 109,626,776 | Clang | −25% |
| evobench | 563,218,850 | 557,615,961 | Clang | −1.0% |
| treebench | 852,135,816 | 783,642,592 | Clang | −8.0% |
| huffbench | 1,096,625,613 | 1,190,829,141 | GCC | −7.9% |

Clang 22 wins 5 of 8 benchmarks on instruction count; GCC wins distbench
(−28%), almabench (−46%), and huffbench (−7.9%). Clang's largest win is
fftbench (−47%); the benchmarks are otherwise close (mat1bench −23%,
linbench −25%, treebench −8.0%, evobench −1.0%).

(Note: the earlier revision of this table reported almabench as Clang
−85%. That was a measurement artifact — almabench's run() overwrote a
single `position[]` each iteration and only the last was observed, so
under `-ffast-math` Clang eliminated ~89% of the 160000 iterations. With
the result now accumulated across all iterations, GCC wins almabench.)

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
| distbench | 1135 | 1130 | Clang | −0.4% |
| mat1bench | 1079 | 1117 | GCC | −3% |
| almabench | 2506 | 2735 | GCC | −8% |
| fftbench | 1350 | 1388 | GCC | −3% |
| linbench | 1673 | 1669 | Clang | −0.2% |
| evobench | 1783 | 1790 | GCC | −0.4% |
| treebench | 4330 | 4205 | Clang | −3% |
| huffbench | 2128 | 2214 | GCC | −4% |

GCC wins 5 of 8 on code size, but the races are close — most within a few
percent. GCC's `-Os` is tightest on the larger benchmarks (almabench −8%,
huffbench −4%); Clang's `-Oz` edges out the rest (distbench, linbench,
treebench). The wide distbench gap from the earlier revision (−25%) is gone:
the DCE-prevention sink now dominates that tiny benchmark's code, so the two
compilers land within 0.4%.
