# Benchmark Results

## Environment

- **CPU:** AMD EPYC 9354 (Zen 4) — AVX-512 + AVX2; `-march=native` ⇒ `znver4`
- **OS:** Fedora Linux 44 (container)
- **Compilers:** GCC 16.1.1, Clang 22.1.7
- **Date:** 2026-07-04 (full `-p`/`-s` re-run after the checksum rework
  and the `-std=gnu23` pin)
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
- **GCC vs Clang, size:** GCC wins 7 of 8 (all but treebench), by
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
matching `scripts/aggregate.sh`.

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
| distbench   | 23,512,257      | -O3 -ffast-math -march=native -funroll-all-loops -fvect-cost-model=unlimited |
| mat1bench   | 28,962,060      | -O3 -ffast-math -march=native -fno-align-loops -fno-tree-loop-distribute-patterns -funroll-all-loops -ffp-contract=on |
| almabench   | 347,361,658 ⚠️  | -O3 -ffast-math -march=native -fno-dce -fno-guess-branch-probability -fno-ipa-modref -fno-move-loop-invariants -fno-plt -fno-tree-reassoc -funroll-all-loops -ffp-contract=on -fira-algorithm=priority -fvect-cost-model=unlimited |
| fftbench    | 448,263,893     | -Os -ffast-math -march=native -fno-cprop-registers -fno-plt -ffp-contract=on -freorder-blocks-algorithm=simple |
| linbench    | 96,244,520      | -O3 -ffast-math -march=native -flto -fno-align-loops -fno-caller-saves -fno-forward-propagate -fno-if-conversion -fno-if-conversion2 -fno-plt -fno-sched-dep-count-heuristic -fno-tree-loop-distribute-patterns -funroll-all-loops -fira-algorithm=priority -fira-region=one -fvect-cost-model=unlimited |
| evobench    | 571,063,803     | -O3 -ffast-math -march=native -flto -fno-align-loops -fno-caller-saves -fno-code-hoisting -fno-if-conversion -fno-move-loop-invariants -fno-plt -fno-schedule-insns2 -fno-tree-sink -funroll-all-loops -ffp-contract=on -fira-region=all |
| treebench   | 839,656,256     | -O3 -ffast-math -march=native -flto -fno-align-loops -fno-asynchronous-unwind-tables -fno-code-hoisting -fno-dse -fno-forward-propagate -fno-if-conversion -fno-ipa-modref -fno-move-loop-invariants -fno-plt -fno-tree-loop-distribute-patterns -fno-tree-reassoc -fno-tree-slp-vectorize -ftracer -freorder-blocks-algorithm=simple -fvect-cost-model=very-cheap |
| huffbench   | 1,003,034,350   | -O3 -ffast-math -flto -fno-align-functions -fno-align-loops -fno-asynchronous-unwind-tables -fno-caller-saves -fno-code-hoisting -fno-cse-follow-jumps -fno-dse -fno-forward-propagate -fno-if-conversion -fno-if-conversion2 -fno-ipa-bit-cp -fno-plt -fno-shrink-wrap -fno-ssa-phiopt -fno-tree-dominator-opts -funroll-all-loops |

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
- `-flto` helps the larger benchmarks (linbench, evobench, treebench,
  huffbench); mat1bench no longer needs it.
- treebench/huffbench pick up the most `-fno-*` flags (largest search surface).
  huffbench `-p` is the one benchmark where quick mode visibly trails the
  former hand-curated result (see [Quick vs audit](#quick-vs-audit)): several of
  its winning flags are huffbench-specific and rank low on the weights.

### Clang 22 — instructions

Config: `config/clang22.osearch` (106 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 39,092,623      | -O2 -march=native -ffast-math -mllvm -inline-threshold=500 |
| mat1bench   | 54,306,103      | -O3 -flto -march=native -ffast-math -mno-vzeroupper -mllvm -force-vector-width=8 |
| almabench   | 215,973,084     | -Os -flto=thin -march=native -ffast-math -fveclib=libmvec -fno-omit-frame-pointer -fno-asynchronous-unwind-tables -fno-plt -fvisibility=hidden -mno-vzeroupper -mllvm -unroll-threshold=200 |
| fftbench    | 254,507,215     | -O2 -flto -march=native -ffp-contract=fast -fno-plt -fno-builtin -mllvm -force-vector-width=8 -mllvm -inline-threshold=300 |
| linbench    | 109,760,818     | -O2 -flto -march=native -ffast-math -fno-asynchronous-unwind-tables -fno-plt -mllvm -unroll-threshold=800 |
| evobench    | 557,312,755     | -O3 -flto=thin -march=native -ffast-math -ffp-contract=on -mno-vzeroupper |
| treebench   | 777,948,383     | -O3 -flto=thin -fno-delete-null-pointer-checks -fno-asynchronous-unwind-tables -fno-plt -fno-builtin -fvisibility=hidden -mllvm -enable-gvn-hoist -mllvm -enable-newgvn |
| huffbench   | 1,190,827,221   | -O3 -flto -march=native -ffast-math -fno-omit-frame-pointer -fno-plt -mprefer-vector-width=256 |

#### Observations

- `-march=native` + LTO (`-flto` or `-flto=thin`) is the near-universal base
  (treebench alone skips `-march`); the `-O` level varies: distbench,
  fftbench, and linbench pick `-O2`, almabench `-Os`, the rest `-O3` — lower
  levels minimise instruction count for several kernels.
- `-flto=thin` matches or beats full `-flto` on almabench, evobench, and
  treebench (and links faster).
- `-fno-plt` consistently helps (mat1bench, almabench, fftbench, linbench,
  treebench): avoids PLT indirection on hot calls.
- `-mllvm -enable-gvn-hoist` helps evobench and treebench (hoisting redundant
  code in branch-heavy traversal).
- `-mllvm -inline-threshold` / `-unroll-threshold` tune distbench (500),
  fftbench (300), linbench (800), and almabench (200);
  `-mllvm -force-vector-width=8` helps mat1bench and fftbench.
  `-mno-vzeroupper` recurs (avoids `vzeroupper` overhead).

### GCC 16 vs Clang 22 — Instructions (-p)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 | Clang 22 | Winner | Δ |
|-----------|--------|----------|--------|---|
| distbench | 23,512,257 | 39,092,623 | GCC | −40% |
| mat1bench | 28,962,060 | 54,306,103 | GCC | −47% |
| almabench | 347,361,658 | 215,973,084 | Clang | −38% |
| fftbench | 448,263,893 | 254,507,215 | Clang | −43% |
| linbench | 96,244,520 | 109,760,818 | GCC | −12% |
| evobench | 571,063,803 | 557,312,755 | Clang | −2% |
| treebench | 839,656,256 | 777,948,383 | Clang | −7% |
| huffbench | 1,003,034,350 | 1,190,827,221 | GCC | −16% |

It's 4–4: GCC wins distbench (−40%), mat1bench (−47%), linbench (−12%), and
huffbench (−16%); Clang wins fftbench (−43%), almabench (−38%), treebench
(−7%), and evobench (−2%). (Almabench flipped from "GCC −46%" when
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

Each ≥40% delta was root-caused: rebuild the winner with its exact flags,
confirm the published count reproduces to within a few instructions, then
read the hot loops in the disassembly and test counterfactuals (add/remove
one flag, or split a phase out by patching in an early return). Two
possibilities were ruled out first:

- *Dead-code elimination:* every benchmark's full output feeds a printed
  checksum (see top), and all eight checksums agree bit-exactly between the
  two compilers at `-O2` — both sides do all the intended work.
- *Vector width:* both compilers use 8-wide zmm on the FP kernels; no gap
  comes from one side staying at 256-bit.

What remains is, in each case, **one discrete codegen decision** in one hot
loop — detailed below.

**distbench (GCC −40%): which loop to vectorize.** The kernel is
`r[i] += dist(v1[i], v2[j])` over 4000×4000 points stored as
array-of-structs `{x,y,z}`. GCC vectorizes the *outer* `i` loop: it
transposes 8 `v1[i]` points into three zmm registers (x-, y-, z-lanes)
once per block, then each `j` step is 3 `vbroadcastsd` of `v2[j]`'s
scalars + 3 `vsubpd` + `vmulpd` + 2 `vfmadd` + `vsqrtpd` + `vaddpd` —
**zero shuffles**, ~11 instructions per 8 distances (1.4/distance;
16M distances × 1.4 ≈ the measured 23.5M). Clang vectorizes the *inner*
`j` loop, which forces it to load the interleaved `{x,y,z}` structs and
transpose them in registers: 24 `vpermt2pd` shuffles per 32 distances, 90
instructions per iteration (2.8/distance ≈ the measured 39.1M). For AoS
layouts, outer-loop vectorization turns the transpose problem into cheap
broadcasts; LLVM's vectorizer only handles inner loops (outer-loop
vectorization is still an experimental VPlan path).

**mat1bench (GCC −47%): loop interchange.** The source is the naive
`c[i][j] += a[i][k] * b[k][j]` with `k` innermost — a dot product whose
`b`-access walks a *column* (stride 3600 B). GCC interchanges the nest
into the classic broadcast-FMA GEMM microkernel: `c[i][j..j+7]` stays in
a zmm accumulator for the entire `k` loop, `a[i][k]` is a broadcast,
`b[k][j..j+7]` streams unit-stride, unrolled 9× — 0.30 instructions per
multiply-add (91.1M MACs → measured 29.0M). Clang keeps the loop order as
written and vectorizes the `k` reduction, so every 8 elements of `b` cost
`vgatherqpd` + `kxnorb` (mask reset) + `vxorpd` (destination zero) —
0.60 instructions per MAC (measured 54.3M). LLVM's LoopInterchange pass
is off by default, and even `-mllvm -enable-loopinterchange` doesn't fire
on this nest, so no config flag can close this gap. Note the instruction
count *understates* the real cost: a zmm gather decodes into many µops on
Zen 4, so the cycle gap is larger than 2×.

**almabench (was "GCC −46%", now Clang −38% the other way): a config
artifact, not a compiler gap.** The workload is dominated by `sin`/`cos`
series evaluation. Under `-ffast-math` GCC auto-vectorizes those calls
into glibc's vector math library (libmvec: `_ZGVeN8v_sin/cos`, 8 lanes per
call); Clang supports the same lowering but only when asked via
`-fveclib=libmvec` — and the config never offered it, so Clang's search
could not escape scalar `sin`/`cos`. Adding the flag (verified an exact
no-op on the other seven benchmarks) let the search adopt it as its
single largest step ever (−434.6M) and land at 216.0M vs GCC's 347.4M.
Moral: a "compiler A beats compiler B" result on libm-heavy code is often
really a *defaults* difference — worth auditing the config whenever one
compiler wins by an implausible margin.

**fftbench (Clang −43%): idiom recognition, mostly.** Splitting the
measurement by phase (early-return after the permutation):

| Phase | GCC (winner) | Clang (winner) |
|-------|-------------:|---------------:|
| bit-reverse permutation | 161.0M | 16.2M |
| butterfly stages | 287.3M | 238.3M |
| **total** | **448.3M** | **254.5M** |

- *Bit-reverse — 75% of the gap.* Clang's loop-idiom recognition turns the
  shift/or loop in `bit_reverse()` into `llvm.bitreverse`, and znver4 has
  GFNI, so each index costs a single `vgf2p8affineqb` (byte-wise bit
  reversal in the Galois field unit) + `bswap` + `bextr` — ~7 instructions.
  GCC has no bit-reverse idiom recognition and emits the 20-iteration loop
  as written, ~160 instructions per index. It doesn't even fully unroll it:
  the default `max-completely-peel-times=16` is below the 20 iterations,
  and raising the param only gets 161M → 110M — still scalar shift/or,
  nowhere near the GFNI lowering.
- *Butterflies — 25% of the gap.* Both compilers emit scalar FMA
  butterflies (`vfmsub231sd`/`vfmadd231sd`) — the twiddle-factor recurrence
  is a loop-carried dependence neither can vectorize — but Clang unrolls
  the innermost loop 2× with slightly denser addressing.

**Takeaways.** None of the big gaps is a broad "compiler X generates
better code" effect. Each is a single pass-level decision: outer-loop
vectorization (distbench), loop interchange (mat1bench), vector-libm
defaults (almabench), idiom recognition + peel limits (fftbench). The
remaining ≤16% deltas (linbench, huffbench, treebench, evobench) were not
root-caused. Two caveats when reading the table: `-p` counts retired
instructions, not cycles — shuffle- and gather-heavy code (Clang's
distbench/mat1bench losses) costs relatively more in time than in count —
and a gap this size is as likely to be one missing flag or one missed
idiom as it is a quality difference, so check the disassembly before
crediting the compiler.

## Code size (-s)

Fully deterministic: `.text` byte counts are 100% reproducible across runs.
Sizes are +23…+196 B over the 2026-06-28 tables because the DCE-proof
checksum code (sinks, `clean()` checksum loops, the `-v` print path) is
real `.text` in every binary; the flag families are unchanged.

### GCC 16 — size

Config: `config/gcc16.osearch` (226 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 1103          | -Os -march=native -fno-if-conversion -fno-math-errno -fno-tree-loop-im -fno-tree-sink |
| mat1bench   | 1073          | -Os -flto -fno-if-conversion -fno-move-loop-stores -fno-tree-sink -fira-algorithm=priority -fira-region=all |
| almabench   | 2374          | -Os -march=native -flto -fno-caller-saves -fno-cse-follow-jumps -fno-if-conversion -fno-math-errno -fno-reorder-functions -fno-ssa-phiopt -fno-tree-loop-im -fno-tree-sink |
| fftbench    | 1359          | -Os -march=native -flto -fno-caller-saves -fno-reorder-functions -fno-thread-jumps -fno-tree-sink |
| linbench    | 1652          | -Os -march=native -flto -fno-guess-branch-probability -fno-if-conversion -fno-tree-sink -ffinite-math-only -ffp-contract=on |
| evobench    | 1804          | -Os -march=native -flto -fno-caller-saves -fno-reorder-functions -fno-ssa-phiopt -fno-tree-sink -fno-tree-tail-merge -ffp-contract=off |
| treebench   | 4423          | -Os -march=native -flto -fno-forward-propagate -fno-guess-branch-probability -fno-if-conversion -fno-move-loop-invariants -fno-optimize-sibling-calls -fno-schedule-insns2 -fno-thread-jumps -fno-tree-forwprop -fno-tree-scev-cprop -fgraphite -fira-region=all |
| huffbench   | 2097          | -Os -flto -fno-caller-saves -fno-cse-follow-jumps -fno-guess-branch-probability -fno-if-conversion -fno-move-loop-invariants -fno-schedule-insns2 -fno-thread-jumps -fno-tree-forwprop -fno-tree-scev-cprop -fno-tree-sink |

#### Observations

- `-Os -flto` is the foundation for every benchmark except distbench,
  which now drops `-flto`
- `-march=native` helps most benchmarks but not the pure-integer mat1bench and
  huffbench, which omit it (AVX encoding wastes bytes)
- `-fno-tree-sink`, `-fno-if-conversion`, `-fno-reorder-functions` and
  `-fno-thread-jumps` recur as the broad size-reducers
- treebench is the largest (4423 B) — recursive tree traversal has many code paths

### Clang 22 — size

Config: `config/clang22.osearch` (106 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 1156          | -Oz -flto -mtune=native -ffp-contract=fast -fno-builtin -mavx2 -mllvm -enable-newgvn |
| mat1bench   | 1145          | -Oz -flto -mtune=native |
| almabench   | 2760          | -Oz -flto=thin -march=native -ffinite-math-only -ffp-contract=fast -fno-omit-frame-pointer -fno-builtin -mllvm -enable-newgvn |
| fftbench    | 1452          | -Oz -flto -march=native -ffp-contract=fast -mno-vzeroupper |
| linbench    | 1697          | -Oz -flto -mtune=native -ffinite-math-only -fno-signed-zeros |
| evobench    | 1867          | -Oz -flto -march=native -fno-signed-zeros -ffp-contract=off -fno-inline-functions -fno-builtin -mllvm -enable-newgvn |
| treebench   | 4361          | -Oz -flto -mtune=native -fno-optimize-sibling-calls -mllvm -enable-newgvn |
| huffbench   | 2277          | -Oz -flto=thin -mtune=native -mllvm -enable-newgvn |

#### Observations

- `-Oz` + LTO (`-flto` or `-flto=thin`) wins for every benchmark
- `-march=native` only pays where wide vectors earn their encoding bytes
  (almabench, fftbench, evobench); the rest take `-mtune=native`
  (distbench adds just `-mavx2`)
- `-mllvm -enable-newgvn` helps size broadly (fftbench, evobench, huffbench,
  almabench, treebench) — merging redundant code; `-enable-gvn-hoist`
  additionally helps treebench
- `-fno-builtin` helps a few benchmarks (avoids inlining libc)

### GCC 16 vs Clang 22 — Size (-s)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 (.text) | Clang 22 (.text) | Winner | Δ |
|-----------|----------------|------------------|--------|---|
| distbench | 1103 | 1156 | GCC | −5% |
| mat1bench | 1073 | 1145 | GCC | −6% |
| almabench | 2374 | 2760 | GCC | −14% |
| fftbench | 1359 | 1452 | GCC | −6% |
| linbench | 1652 | 1697 | GCC | −3% |
| evobench | 1804 | 1867 | GCC | −3% |
| treebench | 4423 | 4361 | Clang | −1% |
| huffbench | 2097 | 2277 | GCC | −8% |

GCC wins 7 of 8 on code size — only treebench goes to Clang, by 1%. GCC's
`-Os` is consistently tighter, by 3–14% (largest on almabench and huffbench).

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
  non-greedy search (TODO.md item 3) would address.
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
  TODO.md item 3.
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
