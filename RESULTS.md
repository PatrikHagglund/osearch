# Benchmark Results

Config: `config/gcc16-test.osearch` (speed) / `config/gcc16-size.osearch` (size)
Compiler: GCC 16.0.1
Date: 2026-05-04

Benchmarks are grouped by workload type and sorted by instruction count:

- **FP/vectorizable** (prefers `-Ofast`): distbench, mat1bench, almabench, fftbench, linbench, evobench
- **Integer/branch-heavy** (prefers `-O3`): treebench, huffbench

Sections are ordered by measurement reliability:

1. **Size (-s)** — 100% deterministic
2. **Instructions (-p)** — deterministic (via `perf stat`)
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

Uses `perf stat -e instructions:u` — results are reproducible to ~20 ppb.

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 28,608,329      | -Ofast -march=native -fno-align-loops -fno-peephole2 |
| mat1bench   | 78,968,740      | -Ofast -march=native -fno-expensive-optimizations -fno-reorder-functions |
| almabench   | 341,808,229     | -Ofast -march=native -fno-align-loops -fno-move-loop-invariants -fno-peephole2 -fno-reorder-functions -fno-tree-slp-vectorize -fno-asynchronous-unwind-tables |
| fftbench    | 491,432,350     | -Ofast -march=native -flto -fno-align-loops -fno-gcse -fno-schedule-insns2 -fno-tree-pre |
| linbench    | 160,110,261     | -Ofast -march=native -flto -fno-code-hoisting -fno-align-loops -fno-peephole2 -fno-tree-loop-distribute-patterns -fno-tree-pre -fno-asynchronous-unwind-tables |
| evobench    | 563,876,638     | -Ofast -march=native -flto -fno-code-hoisting -fno-expensive-optimizations -fno-ivopts |
| treebench   | 852,882,315     | -O3 -march=native -flto -fno-cse-follow-jumps -fno-ipa-cp -fno-align-functions -fno-align-loops -fno-gcse -fno-ipa-sra -fno-tree-loop-distribute-patterns -fno-tree-loop-vectorize -fno-tree-slp-vectorize |
| huffbench   | 1,249,274,519   | -O3 -flto -fno-cse-follow-jumps -fno-code-hoisting -fno-caller-saves -fno-ivopts |

### Instruction count observations

- The FP group picks `-Ofast`; the integer group cleanly picks `-O3`,
  confirming the workload-based grouping
- `-march=native` helps everyone except huffbench (pure integer/branch workload)
- `-flto` helps the larger benchmarks (fft, lin, evo, tree, huff)
- `-fno-align-*` flags help via icache locality
- treebench/huffbench pick up more `-fno-*` flags (more search surface
  since `-O3` enables fewer aggressive optimizations than `-Ofast`)

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
- **Instructions (-p):** ~100% reproducible; flag sets match closely
- **Time (default, -n 3):** Values stable within ~10%, but greedy adoption
  paths can diverge due to residual noise. For noise-free optimization,
  use `-s` or `-p` instead.
