# Benchmark Results

Config: `config/gcc16-test.osearch` (speed) / `config/gcc16-size.osearch` (size)

Instruction-count (`-p`) tables re-measured 2026-06-22 with the in-harness
`perf_event_open` counter (GCC 16.1.1, Clang 22.1.7). Size (`-s`) and time
tables are from the earlier run (GCC 16.0.1, 2026-05-04) and are unaffected
by the counter change.

Benchmarks are grouped by workload type and sorted by instruction count:

- **FP/vectorizable** (prefers `-Ofast`): distbench, mat1bench, almabench, fftbench, linbench, evobench
- **Integer/branch-heavy** (prefers `-O3`): treebench, huffbench

Sections are ordered by measurement reliability:

1. **Size (-s)** — 100% deterministic
2. **Instructions (-p)** — deterministic (in-harness `perf_event_open`)
3. **Time (default)** — wall-clock time; noisy

## Size optimization (-s), Level 1 (full search)

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 694           | -Os -march=native -flto |
| mat1bench   | 624           | -Os -flto -fno-reorder-functions -fno-tree-vrp -fno-tree-loop-im |
| almabench   | 2104          | -Os -march=native -flto -fno-reorder-functions -fno-tree-vrp -fno-caller-saves -fno-tree-loop-im |
| fftbench    | 919           | -Os -march=native -flto -fno-reorder-functions -fno-caller-saves -fno-tree-loop-optimize |
| linbench    | 1193          | -Os -march=native -flto -fno-reorder-functions -fno-tree-vrp -fno-move-loop-invariants -fno-caller-saves |
| evobench    | 1386          | -Os -march=native -flto -fno-reorder-functions -fno-tree-vrp -fno-move-loop-invariants -fno-inline-functions-called-once |
| treebench   | 3838          | -Os -march=native -flto -fno-reorder-functions -fno-move-loop-invariants -fno-caller-saves -fno-tree-loop-im -fno-tree-tail-merge -fno-code-hoisting -fno-optimize-sibling-calls -fno-tree-loop-optimize |
| huffbench   | 1709          | -Os -flto -fno-reorder-functions -fno-tree-vrp -fno-move-loop-invariants -fno-tree-loop-im -fno-schedule-insns2 |

### Size observations

- `-Os -flto` is the foundation; `-march=native` helps all except
  mat1bench and huffbench (integer-heavy, AVX encoding wastes bytes)
- `-fno-reorder-functions` helps every benchmark
- `-fno-tree-vrp` and `-fno-move-loop-invariants` are also broadly useful
- treebench is the largest (3838B) — recursive tree traversal has many code paths

## Instruction count (-p), Level 1 (full search)

Counts user-space retired instructions for `run()` only, via an
in-harness `perf_event_open` counter. The total is reproducible to
~1 ppm across runs; the `-fno-*` flags beyond the `-O`/`-march` base are
adopted at the default `-T 0` threshold and include marginal,
noise-level picks that vary between runs (use `-T 3` for a stable set).

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

### Full config observations

The 214-flag config beats the 60-flag test config on every benchmark
(largest gains: mat1bench −59%, linbench −36%, huffbench −18%). Notable
flags:

- **`-funroll-all-loops`** — large win on linbench (93.9M, −36% vs the
  60-flag config), plus mat1bench and distbench. Off by default even at `-O3`.
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

**Noisy** — results vary between runs. Use `-n 5` or higher for more stable results.

| Benchmark   | Time (µs) | Best flags |
|-------------|-----------|------------|
| distbench   | 7596      | -O3 -march=native |
| mat1bench   | 8268      | -Ofast -march=native -fno-align-functions -fno-ivopts |
| almabench   | 39367     | -Ofast -march=native -fno-inline-functions -fno-align-functions |
| fftbench    | 53084     | -O3 -march=native -fno-optimize-sibling-calls |
| linbench    | 22440     | -Ofast -march=native -fno-expensive-optimizations |
| evobench    | 133646    | -Ofast -march=native -fno-inline-small-functions |
| treebench   | 135146    | -O3 -fno-tree-loop-distribute-patterns |
| huffbench   | 89432     | -O3 -fno-cse-follow-jumps -fno-ivopts |

### Level 2, Q=10

Shallower search; faster but may miss combinations that deep greedy search finds.

| Benchmark   | Time (µs) | Best flags |
|-------------|-----------|------------|
| distbench   | 8238      | -Ofast -march=native -flto -fno-inline-functions |
| mat1bench   | 9555      | -Ofast -march=native -flto |
| almabench   | 40711     | -Ofast -march=native -fno-inline-functions -fno-caller-saves |
| fftbench    | 56067     | -O3 -fno-inline-functions |
| linbench    | 22494     | -Ofast -march=native -flto -fno-inline-functions |
| evobench    | 132824    | -Ofast -march=native -flto |
| treebench   | 159705    | -Ofast -fno-cse-follow-jumps -fno-ipa-cp |
| huffbench   | 97578     | -Ofast |

### Time observations

- `-Ofast -march=native` wins most speed benchmarks
- `-O3` wins for integer-heavy (huff, tree) and occasionally others due to noise
- `-flto` shows up inconsistently (time noise can hide/expose its benefit)
- Level 2 Q=10 finds comparable or better results than Level 1 for evobench,
  suggesting Level 1's greedy search went astray there

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

Config: `config/clang22-test.osearch`
Compiler: Clang 22.1.7
Date: 2026-06-22

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 39,092,625      | -O3 -ffast-math -march=native |
| almabench   | 646,256,576     | -Os -ffast-math -march=native -flto -fno-slp-vectorize -fno-plt -mllvm -unroll-threshold=200 |
| mat1bench   | 55,116,556      | -O2 -ffast-math -march=native -flto |
| linbench    | 109,626,776     | -O3 -ffast-math -march=native -fno-inline-functions -fno-strict-overflow -fno-asynchronous-unwind-tables -fno-plt -mllvm -unroll-threshold=800 |
| fftbench    | 254,507,216     | -O2 -ffast-math -march=native -flto -fno-omit-frame-pointer -fno-plt -fno-builtin -mllvm -inline-threshold=300 |
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

Config: `config/clang22.osearch`
Compiler: Clang 22.1.7
Date: 2026-06-22

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

### Full config observations

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

### GCC 16 vs Clang 22 — Instructions (-p)

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

Config: `config/clang22-test.osearch`
Compiler: Clang 22.1.6
Date: 2026-05-31

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 522           | -Oz -march=native -flto |
| mat1bench   | 683           | -Oz -march=native -flto -fno-builtin |
| fftbench    | 982           | -Os -march=native -flto -ffp-contract=fast |
| linbench    | 1231          | -Oz -march=native -flto |
| evobench    | 1434          | -Oz -march=native -flto -fno-inline-functions -fno-signed-zeros |
| huffbench   | 1848          | -Oz -flto |
| almabench   | 2311          | -Oz -march=native -flto -fno-slp-vectorize -ffp-contract=fast -ffinite-math-only -fno-omit-frame-pointer -fno-builtin |
| treebench   | 3817          | -Oz -march=native -flto -fno-optimize-sibling-calls -mllvm -enable-gvn-hoist -mllvm -enable-newgvn |

### Clang 22 size observations

- `-Oz` wins over `-Os` for all benchmarks except fftbench
- `-Oz -march=native -flto` is the universal size baseline
- `-fno-builtin` helps size on some benchmarks (avoids inlining libc)
- `-mllvm -enable-gvn-hoist` and `-mllvm -enable-newgvn` help treebench size
  (merging redundant code paths in recursive traversal)

### GCC 16 vs Clang 22 — Size (-s)

| Benchmark | GCC 16 (.text) | Clang 22 (.text) | Winner | Δ |
|-----------|----------------|------------------|--------|---|
| distbench | 694 | 522 | Clang | −25% |
| mat1bench | 624 | 683 | GCC | −9% |
| fftbench | 919 | 982 | GCC | −6% |
| linbench | 1193 | 1231 | GCC | −3% |
| evobench | 1386 | 1434 | GCC | −3% |
| huffbench | 1709 | 1848 | GCC | −8% |
| almabench | 2104 | 2311 | GCC | −9% |
| treebench | 3838 | 3817 | Clang | −0.5% |

GCC wins 6 of 8 on code size. GCC's `-Os` produces tighter code overall,
likely due to more aggressive function-level size heuristics. Clang wins
on distbench (−25%, small FP kernel benefits from Clang's vectorizer even
at `-Oz`) and treebench (marginal).
