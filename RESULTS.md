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

## Instruction count (-p), Full config (214 flags)

Config: `config/gcc16.osearch`

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | (improved)      | -Ofast -march=native -funroll-all-loops -fno-if-conversion -ftracer -fno-tree-ter -fira-algorithm=priority |
| mat1bench   | (improved)      | -Ofast -march=native -funroll-all-loops -ffp-contract=on -fno-if-conversion -fschedule-insns -finline-stringops |
| almabench   | (improved)      | -Ofast -march=native -fno-tree-slp-vectorize -fno-plt -fno-tree-reassoc -frename-registers -flive-range-shrinkage |
| fftbench    | (improved)      | -Ofast -march=native -funroll-all-loops -fno-guess-branch-probability -ffp-contract=on -fno-tree-forwprop -fno-plt |
| linbench    | ~105,547,000    | -Ofast -march=native -flto -fno-if-conversion -fno-toplevel-reorder -fno-tree-pre -funroll-all-loops -fno-tree-coalesce-vars |
| evobench    | (improved)      | -Ofast -march=native -fno-ivopts -fno-thread-jumps -fno-tree-coalesce-vars -fno-if-conversion -fno-guess-branch-probability |
| treebench   | (improved)      | -Ofast -march=native -flto -fno-tree-loop-vectorize -fno-tree-loop-distribute-patterns -fno-if-conversion -fno-forward-propagate |
| huffbench   | (improved)      | -Ofast -march=native -fno-tree-ch -fno-if-conversion -ftracer -fno-align-loops -fno-tree-dominator-opts |

### Full config observations

The 214-flag config finds significantly better results than the 60-flag
test config. Key new flags discovered:

- **`-funroll-all-loops`** — massive win on linbench (−34%), fftbench,
  mat1bench, distbench. Disabled by default even at `-O3`.
- **`-fno-if-conversion`** — helps almost every benchmark; disabling
  if-conversion lets the branch predictor work better on these workloads.
- **`-fno-tree-pre`** — helps linbench (partial redundancy elimination
  hurts this workload).
- **`-ftracer`** — helps distbench and huffbench (superblock formation).
- **`-ffp-contract=on`** — helps fftbench and mat1bench (FMA contraction).
- **`-fno-tree-ch`** — critical for huffbench (loop header copying hurts
  branch-heavy code).
- **`-fno-tree-coalesce-vars`** — helps linbench and evobench.

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

---

## Clang 22 — Instruction count (-p), Level 1

Config: `config/clang22-test.osearch`
Compiler: Clang 22.1.6
Date: 2026-05-31

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 520,719         | -Ofast -march=native -flto |
| almabench   | 29,707,495      | -Ofast -march=native -flto -fno-slp-vectorize -fno-plt -mllvm -enable-newgvn |
| mat1bench   | 62,575,618      | -Ofast -march=native -flto -fno-plt |
| linbench    | 120,980,423     | -Ofast -march=native -mllvm -unroll-threshold=800 -fno-plt |
| fftbench    | 464,810,489     | -Ofast -march=native -flto -fno-plt |
| evobench    | 557,928,450     | -Ofast -march=native -mllvm -enable-ext-tsp-block-placement -flto -mllvm -force-vector-width=4 |
| treebench   | 777,878,493     | -Ofast -march=native -flto -fno-unroll-loops -mllvm -inline-threshold=1000 -mllvm -enable-gvn-hoist -fno-strict-overflow |
| huffbench   | 1,300,985,017   | -Ofast -march=native -flto -fno-omit-frame-pointer |

### Clang 22 observations

- `-Ofast -march=native -flto` is the universal baseline — always adopted
- `-fno-plt` consistently helps (avoids PLT indirection overhead)
- Most `-mllvm` pass flags show no effect (`=`), meaning Clang 22 at
  `-Ofast` already enables them — the config is useful for finding exceptions
- `-mllvm -enable-newgvn` is a mixed bag: helps almabench but destroys
  mat1bench (+244M instructions)
- `-mllvm -enable-ext-tsp-block-placement` helps evobench (better branch layout)
- `-mllvm -inline-threshold=1000` is critical for treebench (recursive workload)
- `-fno-unroll-loops` is consistently one of the worst flags, except treebench
  where it helps (loop unrolling hurts branch-heavy recursive code)

### GCC 16 vs Clang 22 — Instructions (-p)

| Benchmark | GCC 16 | Clang 22 | Winner | Δ |
|-----------|--------|----------|--------|---|
| distbench | 28,608,329 | 520,719 | Clang | −98% |
| mat1bench | 78,968,740 | 62,575,618 | Clang | −21% |
| almabench | 341,808,229 | 29,707,495 | Clang | −91% |
| fftbench | 491,432,350 | 464,810,489 | Clang | −5.4% |
| linbench | 160,110,261 | 120,980,423 | Clang | −24% |
| evobench | 563,876,638 | 557,928,450 | Clang | −1.1% |
| treebench | 852,882,315 | 777,878,493 | Clang | −8.8% |
| huffbench | 1,249,274,519 | 1,300,985,017 | GCC | −3.9% |

Clang 22 wins 7 of 8 benchmarks on instruction count. The massive wins on
distbench (−98%) and almabench (−91%) suggest Clang's vectorizer and FP
pipeline are significantly more effective on small FP kernels. GCC wins only
on huffbench (integer/branch-heavy).

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
