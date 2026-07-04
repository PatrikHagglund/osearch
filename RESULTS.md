# Benchmark Results

## Environment

- **CPU:** AMD EPYC 9354 (Zen 4) — AVX-512 + AVX2; `-march=native` ⇒ `znver4`
- **OS:** Fedora Linux 44 (container)
- **Compilers:** GCC 16.1.1, Clang 22.1.7
- **Date:** 2026-06-28
- **Configs:** one annotated file per compiler — `gcc16` (226 flags) and
  `clang22` (106 flags), under `config/`. Each carries per-flag `w_speed` /
  `w_size` weights; a *quick* run restricts to the top-ranked options
  (`-k n` / `-Q n`), an *audit* run uncaps and reaches every option. The tables
  below are quick runs; an audit matches them within ~1–2% except where a
  workload's best flags are benchmark-specific (see
  [Quick vs audit](#quick-vs-audit)).

`-p` counts are user-space retired instructions for `run()` only, via the
in-harness `perf_event_open` counter. Absolute size/time figures are larger
than pre-2026-06 revisions because DCE-prevention sinks added real code and
work to several benchmarks' `run()`.

Every benchmark folds its full computed output into a checksum
(`bench_result` in `main.ic`) that is printed under `-v`, so no compiler at
any optimization level may dead-code-eliminate the intended computation —
and the checksums double as a correctness check: all eight are bit-identical
between GCC 16 and Clang 22 at `-O2` (strict FP), and agree to ≥10
significant digits under `-O3 -march=native -ffast-math -flto`.

## Summary

- **For reproducible optimization use `-s` (size) or `-p` (instructions)** —
  both are deterministic. Wall-clock time is too noisy here to optimize on
  (see [Time](#time-default)).
- **One annotated config per compiler.** `gcc16` and `clang22` each search both
  speed and size (their `-O` level is an enum spanning `-O3…-Os`/`-Oz`) and
  carry per-flag effectiveness weights, so a quick `-k n` run searches the
  top-ranked options and an uncapped audit reaches the rest. The former
  separate "curated" and "full" configs are now one file each.
- **GCC vs Clang, instructions:** 4–4. Biggest gaps: mat1bench (GCC −47%),
  fftbench (Clang −43%) — each root-caused to a single hot-loop codegen
  decision, see [Why the big gaps](#why-the-big-gaps). One former "GCC −46%"
  (almabench) was a missing `-fveclib=libmvec` in the Clang config, not a
  compiler gap. See [comparison](#gcc-16-vs-clang-22--instructions--p).
- **GCC vs Clang, size:** GCC wins 7 of 8 (all but treebench), several by
  ~2–14%. See [comparison](#gcc-16-vs-clang-22--size--s).

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

## Instruction count (-p)

The primary reproducible performance metric: user-space retired instructions
for `run()`, counted in-harness and stable to ~1 ppm per binary.

### GCC 16 — instructions

Config: `config/gcc16.osearch` (226 flags), quick mode (`-k 80`), greedy `-l 1`.

Totals are reproducible to ~1 ppm per binary; the `-fno-*` tail beyond the
`-O`/`-march`/`-flto` base is adopted at the default `-T 0` threshold and
includes marginal picks that vary between runs (use `-T 3` for a stable set).

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 23,512,262      | -O3 -ffast-math -march=native -fno-align-loops -fno-shrink-wrap-separate -funroll-all-loops -ffp-contract=on -freorder-blocks-algorithm=simple |
| mat1bench   | 28,962,065      | -O3 -ffast-math -march=native -flto -fno-align-loops -funroll-all-loops -ffp-contract=on -fira-algorithm=priority |
| almabench   | 347,361,650 ⚠️  | -O3 -ffast-math -march=native -fno-asynchronous-unwind-tables -fno-dce -fno-forward-propagate -fno-guess-branch-probability -fno-if-conversion -fno-ipa-modref -fno-peephole2 -fno-plt -fno-tree-reassoc -funroll-all-loops -ffp-contract=on -fira-algorithm=priority |
| fftbench    | 448,263,887     | -Os -march=native -flto -fno-guess-branch-probability -fno-plt -fno-thread-jumps -fno-tree-loop-distribute-patterns -freorder-blocks-algorithm=simple |
| linbench    | 96,244,522      | -O3 -ffast-math -march=native -fno-align-functions -fno-align-jumps -fno-align-loops -fno-caller-saves -fno-if-conversion -fno-if-conversion2 -fno-plt -fno-tree-slp-vectorize -funroll-all-loops -fira-algorithm=priority -fira-region=one |
| evobench    | 571,153,955     | -O3 -ffast-math -march=native -flto -fno-align-loops -fno-caller-saves -fno-code-hoisting -fno-if-conversion -fno-move-loop-invariants -fno-plt -fno-schedule-insns2 -fno-tree-sink -funroll-all-loops -ffp-contract=on -fira-algorithm=priority |
| treebench   | 845,933,030     | -O3 -ffast-math -march=native -flto -fno-align-loops -fno-asynchronous-unwind-tables -fno-code-hoisting -fno-dse -fno-if-conversion -fno-tree-loop-distribute-patterns -fno-tree-reassoc -fira-region=one |
| huffbench   | 1,003,034,388   | -O3 -flto -fno-align-functions -fno-align-loops -fno-asynchronous-unwind-tables -fno-caller-saves -fno-code-hoisting -fno-dce -fno-if-conversion -fno-plt -fno-shrink-wrap -fno-tree-dominator-opts -funroll-all-loops |

> ⚠️ **almabench:** the `-l 1` greedy search lands at ~347M, ~8% above the
> best reachable result (319.6M). The winning flags *are* in this config, but
> the optimum needs a coordinated multi-flag combination: `-flto` only helps
> almabench in one specific co-set (giving 319.6M) and is strongly harmful
> elsewhere (+29% if added to the greedy's path), so single-flag hill-climbing
> can't reach it. See [Search limitations](#search-limitations).

#### Observations

- All benchmarks build on `-O3` + `-ffast-math` (`-Ofast` = `-O3 -ffast-math`);
  fftbench prefers `-Os`.
- `-funroll-all-loops` is a broad win (distbench, mat1bench, linbench, huffbench).
- `-fno-if-conversion` and `-fno-plt` recur on the branch-heavy / call-heavy
  benchmarks (linbench, evobench, treebench, huffbench).
- `-march=native` helps everyone except huffbench (pure integer/branch).
- `-flto` helps the larger benchmarks (mat1, tree, huff).
- treebench/huffbench pick up the most `-fno-*` flags (largest search surface).
  huffbench `-p` is the one benchmark where quick mode visibly trails the
  former hand-curated result (see [Quick vs audit](#quick-vs-audit)): several of
  its winning flags are huffbench-specific and rank low on the weights.

### Clang 22 — instructions

Config: `config/clang22.osearch` (106 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 39,092,624      | -O2 -march=native -ffast-math |
| mat1bench   | 54,306,100      | -O3 -flto -march=native -ffast-math -fno-asynchronous-unwind-tables -fno-plt -mllvm -force-vector-width=8 |
| almabench   | 215,973,084     | -Os -flto=thin -march=native -ffast-math -fveclib=libmvec -fno-omit-frame-pointer -fno-asynchronous-unwind-tables -fno-plt -fvisibility=hidden -mno-vzeroupper -mllvm -unroll-threshold=200 |
| fftbench    | 254,507,208     | -O3 -flto -march=native -ffast-math -fno-asynchronous-unwind-tables -fno-plt -fno-builtin -mllvm -inline-threshold=300 |
| linbench    | 109,869,645     | -O3 -flto -march=native -ffast-math -ffp-contract=on -fno-plt -mno-vzeroupper -mllvm -inline-threshold=1000 -mllvm -unroll-threshold=800 |
| evobench    | 557,312,655     | -O3 -flto=thin -march=native -ffast-math -ffp-contract=on -fno-asynchronous-unwind-tables -fno-optimize-sibling-calls -mno-vzeroupper -mllvm -enable-gvn-hoist |
| treebench   | 770,321,839     | -O3 -flto=thin -fno-delete-null-pointer-checks -fno-plt -fno-direct-access-external-data -fno-builtin -mllvm -enable-gvn-hoist |
| huffbench   | 1,190,827,257   | -O3 -flto -march=native -fno-omit-frame-pointer -fno-plt |

#### Observations

- `-march=native -ffast-math` + LTO (`-flto` or `-flto=thin`) is the universal
  base; the `-O` level varies: most pick `-O3`, distbench `-O2`, almabench
  `-Os` — lower levels minimise instruction count for a couple of kernels.
- `-flto=thin` matches or beats full `-flto` on almabench, evobench, and
  treebench (and links faster).
- `-fno-plt` consistently helps (mat1bench, almabench, fftbench, linbench,
  treebench): avoids PLT indirection on hot calls.
- `-mllvm -enable-gvn-hoist` helps evobench and treebench (hoisting redundant
  code in branch-heavy traversal).
- `-mllvm -inline-threshold` / `-unroll-threshold` tune linbench (1000 / 800),
  fftbench (300), and almabench (200); `-mllvm -force-vector-width=8` helps
  mat1bench. `-mno-vzeroupper` recurs (avoids `vzeroupper` overhead).

### GCC 16 vs Clang 22 — Instructions (-p)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 | Clang 22 | Winner | Δ |
|-----------|--------|----------|--------|---|
| distbench | 23,512,262 | 39,092,624 | GCC | −40% |
| mat1bench | 28,962,065 | 54,306,100 | GCC | −47% |
| almabench | 347,361,650 | 215,973,084 | Clang | −38% |
| fftbench | 448,263,887 | 254,507,208 | Clang | −43% |
| linbench | 96,244,522 | 109,869,645 | GCC | −12% |
| evobench | 571,153,955 | 557,312,655 | Clang | −2% |
| treebench | 845,933,030 | 770,321,839 | Clang | −9% |
| huffbench | 1,003,034,388 | 1,190,827,257 | GCC | −16% |

It's 4–4: GCC wins distbench (−40%), mat1bench (−47%), linbench (−12%), and
huffbench (−16%); Clang wins fftbench (−43%), almabench (−38%), treebench
(−9%), and evobench (−2%). (Almabench flipped from "GCC −46%" when
`-fveclib=libmvec` was added to the Clang config — see
[Why the big gaps](#why-the-big-gaps).)

(Both columns are the single annotated config in quick mode (`-k 80`), so this
is an apples-to-apples comparison. Earlier revisions measured GCC with a weaker
speed-only config and reported "Clang wins 5/8" — that was the config, not the
compiler. Two quick-mode caveats: GCC almabench is the `-l 1` result of ~347M
(the reachable 319.6M would narrow Clang's almabench win to −32%) and GCC
huffbench quick lands at 1.00B, ~13% above the best the former hand-curated set
found — a greedy path-dependence, see [Quick vs audit](#quick-vs-audit).)

#### Why the big gaps

Each ≥40% delta was root-caused from the winning binaries' disassembly. The
checksums (see top) verify both compilers compute the same result in every
case — none of these gaps is dead-code elimination.

- **distbench (GCC −40%).** Both compilers vectorize 8-wide (zmm), but GCC
  vectorizes the *outer* `i` loop: it transposes 8 `v1[i]` points into
  registers once, then each `j` iteration broadcasts `v2[j]`'s three scalars
  and computes 8 distances with zero shuffles — ~1.4 instructions per
  distance. Clang vectorizes the *inner* `j` loop, so it loads interleaved
  `{x,y,z}` structs and pays 24 `vpermt2pd` de-interleave shuffles per 32
  distances — ~2.8 instructions per distance.
- **mat1bench (GCC −47%).** GCC interchanges the `j`/`k` loops into the
  classic broadcast-FMA GEMM microkernel: `c[i][j..j+7]` lives in a zmm
  accumulator across the entire `k` loop, rows of `b` stream unit-stride,
  9× unrolled — 0.30 instructions per multiply-add. Clang keeps the
  dot-product order as written (LLVM's LoopInterchange pass is off by
  default, and `-mllvm -enable-loopinterchange` doesn't fire on this nest),
  so `b[k][j]` is a strided *column* access costing `vgatherqpd` +
  `kxnorb` mask reset + `vxorpd` per 8 elements — 0.60 instructions per
  multiply-add.
- **almabench (was "GCC −46%") — a config artifact, not a compiler gap.**
  GCC auto-vectorizes `sin`/`cos` calls into glibc's vector math library
  (libmvec, `_ZGVeN8v_sin/cos`) under `-ffast-math`; Clang only uses libmvec
  when told to via `-fveclib=libmvec`, which the config didn't offer. Adding
  the flag (an exact no-op on the other benchmarks) drops Clang from 646M to
  216M — flipping the winner to Clang by a wide margin. The option is now in
  `clang22.osearch`, and the tables above reflect the re-run.
- **fftbench (Clang −43%).** Split by measuring the two phases separately:
  - *Bit-reverse permutation — 75% of the gap.* Clang recognizes the
    shift/or loop in `bit_reverse()` as the `llvm.bitreverse` idiom and,
    with znver4's GFNI, lowers it to `vgf2p8affineqb` + `bswap` + `bextr`:
    16.2M instructions for the whole pass. GCC has no bit-reverse idiom
    recognition and keeps the 20-iteration loop — 161.0M instructions (it
    doesn't even fully unroll it: the default
    `max-completely-peel-times=16` is below the 20 iterations).
  - *Butterfly stages — 25% of the gap.* 287.3M (GCC) vs 238.3M (Clang):
    both emit scalar FMA butterflies (the twiddle recurrence is a loop
    dependence neither can vectorize), but Clang unrolls the innermost loop
    2× with slightly denser code.

The pattern: the remaining big deltas are single hot-loop codegen decisions
(outer-loop vectorization, loop interchange, idiom recognition), not broad
code-quality differences — and one "compiler win" was really a missing
config flag.

## Code size (-s)

Fully deterministic: `.text` byte counts are 100% reproducible across runs.

### GCC 16 — size

Config: `config/gcc16.osearch` (226 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 1050          | -Os -march=native -flto -fno-if-conversion -fno-math-errno -fno-reorder-functions -fno-thread-jumps -fno-tree-loop-im -fno-tree-sink |
| mat1bench   | 1032          | -Os -flto -fno-if-conversion -fno-move-loop-stores -fno-reorder-functions -fno-ssa-phiopt -fno-thread-jumps -fno-tree-sink -fira-region=all |
| almabench   | 2350          | -Os -march=native -flto -fno-caller-saves -fno-cse-follow-jumps -fno-if-conversion -fno-math-errno -fno-reorder-functions -fno-ssa-phiopt -fno-tree-loop-im -fno-tree-sink |
| fftbench    | 1315          | -Os -march=native -flto -fno-caller-saves -fno-thread-jumps -fno-tree-sink |
| linbench    | 1629          | -Os -march=native -flto -fno-guess-branch-probability -fno-if-conversion -fno-ssa-phiopt -fno-tree-sink -ffinite-math-only -ffp-contract=on |
| evobench    | 1748          | -Os -march=native -flto -fno-caller-saves -fno-reorder-functions -fno-tree-sink -fno-tree-tail-merge |
| treebench   | 4227          | -Os -march=native -flto -fno-dce -fno-forward-propagate -fno-if-conversion -fno-optimize-sibling-calls -fno-reorder-functions -fno-schedule-insns2 -fno-thread-jumps -fno-tree-forwprop -fno-tree-scev-cprop -fgraphite -fira-region=all |
| huffbench   | 2025          | -Os -flto -fno-caller-saves -fno-cse-follow-jumps -fno-guess-branch-probability -fno-if-conversion -fno-move-loop-invariants -fno-schedule-insns2 -fno-thread-jumps -fno-tree-forwprop -fno-tree-scev-cprop -fno-tree-sink |

#### Observations

- `-Os -flto` is the foundation for every benchmark
- `-march=native` helps most benchmarks but not the pure-integer mat1bench and
  huffbench, which omit it (AVX encoding wastes bytes)
- `-fno-tree-sink`, `-fno-if-conversion`, `-fno-reorder-functions` and
  `-fno-thread-jumps` recur as the broad size-reducers
- treebench is the largest (4227 B) — recursive tree traversal has many code paths

### Clang 22 — size

Config: `config/clang22.osearch` (106 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 1128          | -Oz -flto -mtune=native -ffp-contract=fast -fno-plt |
| mat1bench   | 1115          | -Oz -flto -mtune=native |
| almabench   | 2720          | -Oz -flto=thin -march=native -ffinite-math-only -ffp-contract=fast -fno-omit-frame-pointer -fno-builtin -mllvm -enable-newgvn |
| fftbench    | 1388          | -Oz -flto -march=native -ffp-contract=fast -fno-builtin -mllvm -enable-newgvn |
| linbench    | 1666          | -Oz -flto -march=native -ffinite-math-only -mno-vzeroupper |
| evobench    | 1789          | -Oz -flto -march=native -fno-signed-zeros -ffp-contract=off -fno-inline-functions -fno-builtin -mllvm -enable-newgvn |
| treebench   | 4202          | -Oz -flto -march=native -fno-inline-functions -fno-optimize-sibling-calls -mno-vzeroupper -mllvm -enable-gvn-hoist -mllvm -enable-newgvn |
| huffbench   | 2203          | -Oz -flto=thin -mtune=native -mllvm -enable-newgvn |

#### Observations

- `-Oz` + LTO (`-flto` or `-flto=thin`) wins for every benchmark
- `-march=native` helps the FP benchmarks but not the integer ones
  (mat1bench, huffbench take `-mtune=native` instead)
- `-mllvm -enable-newgvn` helps size broadly (fftbench, evobench, huffbench,
  almabench, treebench) — merging redundant code; `-enable-gvn-hoist`
  additionally helps treebench
- `-fno-builtin` helps a few benchmarks (avoids inlining libc)

### GCC 16 vs Clang 22 — Size (-s)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 (.text) | Clang 22 (.text) | Winner | Δ |
|-----------|----------------|------------------|--------|---|
| distbench | 1050 | 1128 | GCC | −7% |
| mat1bench | 1032 | 1115 | GCC | −7% |
| almabench | 2350 | 2720 | GCC | −14% |
| fftbench | 1315 | 1388 | GCC | −5% |
| linbench | 1629 | 1666 | GCC | −2% |
| evobench | 1748 | 1789 | GCC | −2% |
| treebench | 4227 | 4202 | Clang | −0.6% |
| huffbench | 2025 | 2203 | GCC | −8% |

GCC wins 7 of 8 on code size — only treebench goes to Clang, by 0.6%. GCC's
`-Os` is consistently tighter, by 2–14% (largest on almabench and huffbench).

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

## Methodology & caveats

### Quick vs audit

The result tables are *quick* runs (`-k 80`: the 80 highest-ranked options for
the active objective). The per-flag weights put broadly-effective options at the
front of the search, so quick reproduces a thorough search within ~1–2% on most
benchmarks, and `-k` lets you trade speed for reach without a second config.

Two benchmarks are worth calling out:

- **huffbench `-p`** (quick 1.00B) is the widest gap. Its best flag set is
  *specific to that workload* (e.g. `-fipa-cp-clone`, `-fsplit-paths`,
  `-finline-stringops`), so those options rank low on the cross-benchmark
  weights. This is greedy *path-dependence*, not coverage: widening to `-k 130`
  only nudges it to 0.98B, and even an uncapped audit over all 226 flags (with
  `-finline-stringops` among its picks) reaches just 0.98B — so the flags are
  reachable; greedy simply doesn't take that path from the full space. A
  smaller, hand-chosen space (the former curated config, ~0.89B) happened to
  steer the hill-climb down a better path. It is the clearest price of replacing
  a hand-curated set with data-driven weights — and the kind of case `-l 2` or a
  non-greedy search (README TODO) would address.
- **almabench `-p`** is a true greedy local optimum that no `-k` escapes (see
  below).

Everywhere else, one annotated file in quick mode is within noise of a thorough
search, which was the point of converging the configs.

### Search limitations

osearch is greedy hill-climbing (`-l 1` toggles one flag at a time, keeping
improvements). Two effects surface in the tables:

- **Local optima.** When the best result needs a *coordinated* multi-flag
  combination, greedy can't reach it. almabench `-p` is the clear case:
  `-flto` helps it only together with a specific co-set (→ 319.6M) and is +29%
  *worse* on any other path, so single-flag steps reject it and settle at
  ~347M. Pair search (`-l 2`) explores two-flag moves but is impractically
  slow on a full config; multiple random restarts or a non-greedy search
  (the genetic approach of the ACOVEA ancestor) would be the real fix — see
  the README TODO.
- **Coverage vs. the weights.** Quick mode trusts the cross-benchmark weights
  to front-load the useful options. A workload whose winning flags are specific
  to it (and so rank low on average) can be under-served; a larger `-k` widens
  reach, but greedy path-dependence means it does not always recover a
  hand-curated result (huffbench `-p`, see [Quick vs audit](#quick-vs-audit)).
- **Path sensitivity.** At the default `-T 0` the marginal `-fno-*` tail is
  noise-level, so the exact flag set — and occasionally the total (almabench
  `-p` ranges 311–347M) — varies between runs. The other benchmarks' totals
  are stable.

### Reproducibility

- **Size (-s):** 100% reproducible across runs
- **Instructions (-p):** totals reproducible to ~1 ppm; at the default
  `-T 0` the marginal `-fno-*` picks vary run-to-run, but `-T 3` yields a
  stable flag set (e.g. fftbench → `-O3 -march=native`)
- **Time (default):** too noisy on this shared host to optimize on — the
  greedy search even adopts `-Os` for compute-bound FP benchmarks. Use `-s`
  or `-p` instead (see [Time](#time-default)).
- **Rebuilding a winner by hand:** match the harness invocation —
  `gcc -std=gnu23 <flags> bench.c -lm -lrt` (both configs pin
  `-std=gnu23` in their prime command). The `gnu` part matters, not the
  year: any strict-ISO mode (`-std=c99`, `-std=c23`, …) restricts GCC's
  FP contraction when `-ffast-math` isn't among the winning flags — e.g.
  the fftbench GCC winner measures 448.3M under `gnu23` (identical to the
  gnu-mode default) but 490.2M under `c23` or `c99` (+9%, the
  un-contracted butterflies). `gnu23` is also what GCC 16 defaults to;
  pinning it aligns Clang (default `gnu17`) to the same language mode.
