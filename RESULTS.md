# Benchmark Results

Config: `config/gcc16-test.osearch` (speed) / `config/gcc16-size.osearch` (size)
Compiler: GCC 16.0.1
Date: 2026-05-04

Sections below are ordered by measurement reliability:

1. **Size (-s)** — 100% deterministic binary size
2. **Instructions (-p)** — deterministic retired instruction count (via `perf stat`)
3. **Time (default)** — wall-clock time; noisy, results may vary between runs

## Size optimization (-s), Level 1 (full search)

Results are **100% reproducible** (deterministic binary size).

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| almabench   | 2104          | -Os -march=native -flto -fno-reorder-functions -fno-tree-vrp -fno-caller-saves -fno-tree-loop-im |
| distbench   | 694           | -Os -march=native -flto |
| evobench    | 1386          | -Os -march=native -flto -fno-reorder-functions -fno-tree-vrp -fno-move-loop-invariants -fno-inline-functions-called-once |
| fftbench    | 919           | -Os -march=native -flto -fno-reorder-functions -fno-caller-saves -fno-tree-loop-optimize |
| huffbench   | 1709          | -Os -flto -fno-reorder-functions -fno-tree-vrp -fno-move-loop-invariants -fno-tree-loop-im -fno-schedule-insns2 |
| linbench    | 1193          | -Os -march=native -flto -fno-reorder-functions -fno-tree-vrp -fno-move-loop-invariants -fno-caller-saves |
| linsmall    | 1193          | -Os -march=native -flto -fno-reorder-functions -fno-tree-vrp -fno-move-loop-invariants -fno-caller-saves |
| mat1bench   | 624           | -Os -flto -fno-reorder-functions -fno-tree-vrp -fno-tree-loop-im |
| treebench   | 3838          | -Os -march=native -flto -fno-reorder-functions -fno-move-loop-invariants -fno-caller-saves -fno-tree-loop-im -fno-tree-tail-merge -fno-code-hoisting -fno-optimize-sibling-calls -fno-tree-loop-optimize |

### Size observations

- `-Os -march=native -flto` is the foundation for most benchmarks
- `-fno-reorder-functions` and `-fno-tree-vrp` help size across most benchmarks
- `-fno-move-loop-invariants` reduces size (avoids code duplication)
- huffbench and mat1bench don't benefit from `-march=native` (integer-heavy,
  no vectorizable loops → AVX instruction encoding just adds size)
- almabench grew from 409B (earlier, with `-ffast-math`) to 2104B since
  `-ffast-math` was removed and the volatile sink now keeps the real
  computation live (the earlier 409B measured code that was mostly
  dead-code-eliminated)

## Instruction count (-p), Level 1 (full search)

Uses `perf stat -e instructions:u` — deterministic measurement of retired
user-space instructions. Results are highly reproducible (perf counter
variance ~20 parts per billion).

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| almabench   | 341,808,230     | -Ofast -march=native -fno-align-functions -fno-align-loops -fno-move-loop-invariants -fno-peephole2 -fno-tree-slp-vectorize |
| distbench   | 28,608,329      | -Ofast -march=native -fno-align-loops -fno-peephole2 |
| evobench    | 563,585,042     | -Ofast -march=native -fno-code-hoisting -fno-caller-saves -fno-ivopts -fno-move-loop-invariants |
| fftbench    | 490,384,382     | -Ofast -march=native -fno-align-functions -fno-align-loops -fno-gcse |
| huffbench   | 1,249,274,564   | -flto -O3 -fno-code-hoisting -fno-ivopts -fno-move-loop-invariants -fno-tree-loop-vectorize |
| linbench    | 160,110,284     | -Ofast -march=native -flto -fno-code-hoisting -fno-align-loops -fno-crossjumping -fno-peephole2 -fno-reorder-functions -fno-tree-loop-distribute-patterns -fno-tree-pre |
| linsmall    | 160,110,264     | -Ofast -march=native -flto -fno-code-hoisting -fno-align-loops -fno-peephole2 -fno-reorder-functions -fno-tree-loop-distribute-patterns -fno-tree-pre |
| mat1bench   | 79,171,242      | -Ofast -march=native -fno-align-functions -fno-expensive-optimizations |
| treebench   | 852,882,319     | -march=native -flto -O3 -fno-cse-follow-jumps -fno-ipa-cp -fno-align-functions -fno-align-loops -fno-gcse -fno-ipa-sra -fno-schedule-insns2 -fno-tree-loop-distribute-patterns -fno-tree-loop-vectorize -fno-tree-slp-vectorize |

### Instruction count observations

- `-Ofast -march=native` is the foundation for most benchmarks
- `-flto` helps lin*, huff, tree
- **huffbench and treebench prefer `-O3` over `-Ofast`** — huffman coding
  and tree walking are integer/pointer-heavy and don't benefit from the
  unsafe math optimizations in `-Ofast`
- `-fno-align-*` flags help by reducing padding (fewer icache misses)
- `-fno-reorder-functions` consistently helps

## Time (default), Level 1 (full search, -n 3)

**Noisy** — results vary between runs due to CPU frequency scaling and
thermal effects. Use `-n 5` or higher for more stable results.

| Benchmark   | Time (µs) | Best flags |
|-------------|-----------|------------|
| almabench   | 39384     | -Ofast -march=native -fno-caller-saves |
| distbench   | 7582      | -Ofast -march=native -fno-align-loops |
| evobench    | 296869    | -march=native -flto -O2 -fno-caller-saves -fno-align-loops -fno-gcse -ftree-partial-pre |
| fftbench    | 91771     | -Ofast -march=native |
| huffbench   | 96019     | -O2 -fno-align-functions |
| linbench    | 76833     | -Ofast -march=native -flto -fno-inline-functions -fno-align-loops |
| linsmall    | 74533     | -Ofast -march=native -fno-inline-functions -fno-caller-saves -fno-align-loops -fno-gcse -fno-tree-pre |
| mat1bench   | 20495     | -Ofast -march=native -fno-expensive-optimizations |
| treebench   | 158942    | -Ofast -fno-code-hoisting -fno-ipa-cp |

### Level 2, Q=10 (10 flag-pair combinations sampled)

Shallower search; faster but may miss combinations that deep greedy search finds.

| Benchmark   | Time (µs) | Best flags |
|-------------|-----------|------------|
| almabench   | 37857     | -Ofast -march=native |
| distbench   | 7308      | -Ofast -march=native -flto |
| evobench    | 141348    | -Ofast -march=native -fno-inline-functions -fno-cse-follow-jumps -fno-ipa-cp |
| fftbench    | 67346     | -Ofast -march=native |
| huffbench   | 101613    | -flto -O3 -fno-ipa-cp |
| linbench    | 25656     | -march=native -O3 -fno-inline-functions |
| linsmall    | 22435     | -Ofast -march=native -fno-inline-functions |
| mat1bench   | 10171     | -Ofast -march=native |
| treebench   | 168361    | -O3 |

### Time observations

- `-Ofast -march=native` is the most common winner (speed benchmarks)
- `-O3` wins for integer-heavy workloads (huffbench, treebench, linbench-l2)
- `-flto` helps several benchmarks (dist, lin, huff)
- Level 2 with Q=10 can find simpler, comparable flag sets quickly —
  when greedy adoption is noise-driven, the shorter search is more robust
- evobench L1 time (296ms) is ~2× evobench L2 (141ms) — classic example
  of noise sending the L1 greedy search down a wrong branch

## Reproducibility

- **Size (-s):** 100% reproducible — identical results across runs
- **Instructions (-p):** ~100% reproducible at the measurement level;
  result flag sets are close between runs, with minor differences when
  multiple flags give similar savings
- **Time (default, -n 3):** Values are stable within ~10%, but the greedy
  adoption path can diverge due to residual noise, leading to different
  flag sets. The minimum-of-3 strategy eliminates most outliers but cannot
  fully compensate for CPU frequency scaling and thermal effects.
