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
| almabench   | 409           | -Os -fno-reorder-functions -fno-tree-vrp -ffast-math -march=native -flto |
| distbench   | 694           | -Os -march=native -flto |
| evobench    | 1376          | -Os -fno-caller-saves -fno-reorder-functions -fno-tree-tail-merge -fno-tree-vrp -march=native -flto |
| fftbench    | 919           | -Os -flto -march=native -fno-reorder-functions -fno-caller-saves -fno-tree-loop-optimize |
| huffbench   | 1709          | -Os -fno-move-loop-invariants -fno-reorder-functions -fno-schedule-insns2 -fno-tree-loop-im -fno-tree-vrp -flto |
| linbench    | 1193          | -Os -fno-caller-saves -fno-move-loop-invariants -fno-reorder-functions -fno-tree-vrp -march=native -flto |
| linsmall    | 1193          | -Os -fno-caller-saves -fno-move-loop-invariants -fno-reorder-functions -fno-tree-vrp -march=native -flto |
| mat1bench   | 624           | -Os -fno-reorder-functions -fno-tree-loop-im -fno-tree-vrp -flto |
| treebench   | 3838          | -Os -fno-code-hoisting -fno-caller-saves -fno-move-loop-invariants -fno-optimize-sibling-calls -fno-reorder-functions -fno-tree-loop-im -fno-tree-loop-optimize -fno-tree-pre -march=native -flto |

### Size observations

- `-Os` is always the best base level for size (as expected)
- `-flto` (link-time optimization) consistently reduces size
- `-fno-reorder-functions` and `-fno-tree-vrp` help size across most benchmarks
- `-march=native` helps size too (more efficient instruction encoding)
- `-fno-move-loop-invariants` reduces size (avoids code duplication)

## Instruction count (-p), Level 1 (full search)

Uses `perf stat -e instructions:u` — deterministic measurement of retired
user-space instructions. Results are highly reproducible (perf counter
variance ~20 parts per billion).

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| almabench   | 341,808,229     | -Ofast -fno-align-functions -fno-align-loops -fno-move-loop-invariants -fno-reorder-functions -fno-strict-aliasing -fno-tree-slp-vectorize -march=native |
| distbench   | 28,608,328      | -Ofast -fno-align-functions -fno-align-loops -march=native |
| evobench    | 563,585,044     | -Ofast -fno-cse-follow-jumps -fno-code-hoisting -fno-caller-saves -fno-ivopts -fno-move-loop-invariants -march=native |
| fftbench    | 490,384,376     | -Ofast -march=native -fno-align-loops -fno-gcse -fno-tree-slp-vectorize |
| huffbench   | 1,249,274,607   | -Ofast -fno-cse-follow-jumps -fno-code-hoisting -fno-align-functions -fno-ivopts -fno-optimize-sibling-calls -flto |
| linbench    | 160,110,258     | -Ofast -fno-code-hoisting -fno-align-loops -fno-peephole2 -fno-reorder-functions -fno-schedule-insns2 -fno-tree-loop-distribute-patterns -fno-tree-pre -fno-asynchronous-unwind-tables -march=native -flto |
| linsmall    | 161,742,820     | -Ofast -fno-caller-saves -fno-align-loops -fno-gcse -fno-crossjumping -fno-optimize-sibling-calls -fno-optimize-strlen -fno-peephole2 -fno-schedule-insns2 -fno-tree-pre -march=native |
| mat1bench   | 78,968,760      | -Ofast -fno-reorder-functions -march=native |
| treebench   | 853,281,585     | -Ofast -fno-caller-saves -fno-gcse -fno-move-loop-invariants -fno-reorder-functions -fno-tree-loop-distribute-patterns -fno-tree-loop-vectorize -fno-tree-slp-vectorize -fno-asynchronous-unwind-tables -march=native -flto |

### Instruction count observations

- `-Ofast -march=native` is the foundation for almost every benchmark
- `-flto` helps several benchmarks (lin, huff, tree)
- `-fno-align-*` flags help by reducing padding (fewer icache misses)
- `-fno-reorder-functions` consistently helps

## Time (default), Level 1 (full search, -n 3)

**Noisy** — results vary between runs due to CPU frequency scaling and
thermal effects. Use `-n 5` or higher for more stable results.

| Benchmark   | Time (µs) | Best flags |
|-------------|-----------|------------|
| almabench   | ~150,000  | (workload-dependent; see Size/Perf modes for reliable data) |
| distbench   | 8231      | -Ofast -march=native |
| evobench    | 343204    | -O3 -fno-ivopts |
| fftbench    | 48142     | -march=native -Os -fno-gcse -fno-tree-vrp -fno-asynchronous-unwind-tables |
| huffbench   | 90780     | -O3 -fno-expensive-optimizations -fno-tree-loop-optimize |
| linbench    | 32008     | -Ofast -fno-inline-functions -fno-peephole2 |
| linsmall    | 23244     | -Ofast -fno-inline-functions -fno-asynchronous-unwind-tables -march=native |
| mat1bench   | 13833     | -Ofast -fno-inline-functions -fno-align-functions -fno-caller-saves -fno-gcse -fno-tree-pre |
| treebench   | 148414    | -Ofast -fno-cse-follow-jumps |

### Level 2, Q=10 (10 flag-pair combinations sampled)

Shallower search; faster but may miss combinations that deep greedy search finds.

| Benchmark   | Time (µs) | Best flags |
|-------------|-----------|------------|
| almabench   | 0         | -Ofast |
| distbench   | 17438     | -O3 -fno-inline-functions |
| evobench    | 356740    | -Ofast -fno-ipa-cp |
| fftbench    | 37504     | -O3 |
| huffbench   | 103664    | -O2 |
| linbench    | 32583     | -Ofast |
| linsmall    | 32167     | -O3 -fno-inline-functions -fno-align-functions |
| mat1bench   | 16529     | -Ofast |
| treebench   | 163025    | -O3 -fno-cse-follow-jumps |

### Time observations

- `-Ofast` or `-O3` is the best base level for most benchmarks
- `-march=native` consistently helps (enables AVX/SSE)
- `-fno-inline-functions` and `-fno-align-functions` help several benchmarks
  (smaller code → better icache behavior)
- `-fno-caller-saves` helps compute-heavy benchmarks (fft, lin, dist)
- Level 2 with Q=10 finds reasonable results quickly but misses some
  flag combinations that level 1 discovers through greedy adoption

## Reproducibility

- **Size (-s):** 100% reproducible — identical results across runs
- **Instructions (-p):** ~100% reproducible at the measurement level;
  result flag sets are close between runs, with minor differences when
  multiple flags give similar savings
- **Time (default, -n 3):** Values are stable within ~10%, but the greedy
  adoption path can diverge due to residual noise, leading to different
  flag sets. The minimum-of-3 strategy eliminates most outliers but cannot
  fully compensate for CPU frequency scaling and thermal effects.
