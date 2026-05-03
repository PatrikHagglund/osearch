# Benchmark Results

Config: `config/gcc16-test.osearch` (61 flags, priority-ordered)
Compiler: GCC 16.0.1
Date: 2026-05-03

## Level 1 (full search, 61 combinations)

| Benchmark   | Time (µs) | Best flags |
|-------------|-----------|------------|
| almabench   | 1         | -Ofast |
| distbench   | 8400      | -O2 -fno-caller-saves -fno-asynchronous-unwind-tables -fno-toplevel-reorder -march=native |
| evobench    | 355126    | -O2 -fno-optimize-sibling-calls -fipa-cp-clone |
| fftbench    | 78111     | -Ofast -fno-inline-functions -fno-align-functions -fno-caller-saves -fno-crossjumping -fno-move-loop-invariants -march=native |
| huffbench   | 95122     | -Ofast -fno-tree-loop-optimize |
| linbench    | 21080     | -Ofast -fno-code-hoisting -fno-caller-saves -fno-expensive-optimizations -fno-move-loop-invariants -march=native |
| linsmall    | 23029     | -Ofast -fno-inline-functions -fno-align-functions -fno-align-loops -march=native |
| mat1bench   | 9020      | -Ofast -fno-inline-functions -fno-align-functions -fno-align-loops -fno-schedule-insns2 -march=native |
| treebench   | 162040    | -O3 -fno-cse-follow-jumps -fno-crossjumping -fno-ipa-sra |

## Level 2, Q=10 (10 flag-pair combinations sampled)

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

## Observations

- `-Ofast` or `-O3` is the best base level for most benchmarks
- `-march=native` consistently helps (enables AVX/SSE)
- `-fno-inline-functions` and `-fno-align-functions` help several benchmarks
  (smaller code → better icache behavior)
- `-fno-caller-saves` helps compute-heavy benchmarks (fft, lin, dist)
- Level 2 with Q=10 finds reasonable results quickly but misses some
  flag combinations that level 1 discovers through greedy adoption

## Size optimization (-s), Level 1 (full search)

Results are **100% reproducible** (deterministic binary size).

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| almabench   | 409           | -Os -fno-reorder-functions -fno-tree-vrp -ffast-math -march=native -flto |
| distbench   | 694           | -Os -march=native -flto |
| evobench    | 1376          | -Os -fno-caller-saves -fno-reorder-functions -fno-tree-tail-merge -fno-tree-vrp -march=native -flto |
| fftbench    | 1193          | -Os -fno-caller-saves -fno-move-loop-invariants -fno-reorder-functions -fno-tree-vrp -march=native -flto |
| huffbench   | 1709          | -Os -fno-move-loop-invariants -fno-reorder-functions -fno-schedule-insns2 -fno-tree-loop-im -fno-tree-vrp -flto |
| linbench    | 1193          | -Os -fno-caller-saves -fno-move-loop-invariants -fno-reorder-functions -fno-tree-vrp -march=native -flto |
| linsmall    | 1193          | -Os -fno-caller-saves -fno-move-loop-invariants -fno-reorder-functions -fno-tree-vrp -march=native -flto |
| mat1bench   | 624           | -Os -fno-reorder-functions -fno-tree-loop-im -fno-tree-vrp -flto |
| treebench   | 3838          | -Os -fno-code-hoisting -fno-caller-saves -fno-move-loop-invariants -fno-optimize-sibling-calls -fno-reorder-functions -fno-tree-loop-im -fno-tree-loop-optimize -fno-tree-pre -march=native -flto |

## Size observations

- `-Os` is always the best base level for size (as expected)
- `-flto` (link-time optimization) consistently reduces size
- `-fno-reorder-functions` and `-fno-tree-vrp` help size across most benchmarks
- `-march=native` helps size too (more efficient instruction encoding)
- `-fno-move-loop-invariants` reduces size (avoids code duplication)

## Reproducibility

- **Size (-s):** 100% reproducible — identical results across runs
- **Time:** Noisy — different runs may find different "best" flags due to
  measurement variance (e.g., fftbench: 42295µs vs 37201µs across runs)
