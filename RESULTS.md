# Benchmark Results

## Environment

- **CPU:** AMD EPYC 9354 (Zen 4) — AVX-512 + AVX2; `-march=native` ⇒ `znver4`
- **OS:** Fedora Linux 44 (container)
- **Compilers:** GCC 16.1.1, Clang 22.1.8
- **Date:** 2026-07-05 (full `-p`/`-s` re-run with PGO searchable — the
  `-fprofile-use` pseudo-flag expanded by `scripts/cc-pgo.sh` — and the
  `-u`/`-c`/`-g` counter modes in the harness)
- **Configs:** one annotated file per compiler — `gcc16` (227 flags) and
  `clang22` (107 flags), under `config/`. Each carries per-flag `w_speed` /
  `w_size` weights; a *quick* run restricts to the top-ranked options
  (`-k n` / `-Q n`), an *audit* run uncaps and reaches every option. The tables
  below are quick runs; an audit matches them within ~1–2% except where a
  workload's best flags are benchmark-specific (see
  [Quick vs audit](#quick-vs-audit)).

`-p` counts are user-space retired instructions for `run()` only, via the
in-harness `perf_event_open` counter. The harness can also count retired ops
(`-u`) and self-pinned core cycles (`-c`), or all three atomically (`-g`) —
the [audit table](#audit-retired-ops-and-cycles-on-the--p-winners) below uses
`-g`. Absolute size/time figures are larger than pre-2026-06 revisions
because DCE-prevention sinks and the harness's counter code are real `.text`
and work in every binary.

Every benchmark folds its full computed output into a checksum
(`bench_result` in `main.ic`) that is printed under `-v`, so no compiler at
any optimization level may dead-code-eliminate the intended computation —
and the checksums double as a correctness check: all eight are bit-identical
between GCC 16 and Clang 22 at `-O2` (strict FP), and agree to ≥10
significant digits under `-O3 -march=native -ffast-math -flto`. (fftbench's
checksum sums `|re|+|im|` so it is well-conditioned; the signed spectrum sum
cancels to ~0 and made harmless FP wobble look large.)

## Summary

- **For reproducible optimization use `-s` (size), `-p` (instructions) or
  `-u` (retired ops)** — all deterministic. Wall-clock time is too noisy here
  to optimize on (see [Time](#time-default)); cycles (`-c`) is the best
  ground-truth *audit* metric but not reproducible enough to search on.
- **PGO is searchable and pays.** The `-fprofile-use` pseudo-flag (expanded
  into instrument → train → recompile by `scripts/cc-pgo.sh`) was adopted in
  11 of the 16 speed searches — biggest wins clang/treebench (−21%),
  gcc/almabench (−10%, escaping a documented greedy local optimum),
  huffbench (−6% GCC / −4% Clang) — and correctly rejected where the A/B
  said it regresses (clang/evobench, gcc/linbench). No size search adopted
  it. Training input = benchmark input, so this is best-case PGO.
- **One annotated config per compiler.** `gcc16` and `clang22` each search both
  speed and size (their `-O` level is an enum spanning `-O3…-Os`/`-Oz`) and
  carry per-flag effectiveness weights, so a quick `-k n` run searches the
  top-ranked options and an uncapped audit reaches the rest. The former
  separate "curated" and "full" configs are now one file each.
- **GCC vs Clang, instructions:** 4–4. Biggest gaps: mat1bench (GCC −47%),
  fftbench (Clang −44%), treebench (Clang −26%, PGO-driven) — the older gaps
  each root-caused to a single hot-loop codegen decision, see
  [Why the big gaps](#why-the-big-gaps).
  See [comparison](#gcc-16-vs-clang-22--instructions--p).
- **On cycles the picture shifts:** Clang 5, GCC 2, one wash — instruction
  counts under-price Clang's gathers (mat1bench) but also hide its 2×
  huffbench cycle win. See the [audit
  table](#audit-retired-ops-and-cycles-on-the--p-winners).
- **GCC vs Clang, size:** GCC wins 6 of 8, by ~2–10%; Clang takes evobench
  (−1%) and treebench (−4%). See [comparison](#gcc-16-vs-clang-22--size--s).

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

Config: `config/gcc16.osearch` (227 flags), quick mode (`-k 80`), greedy `-l 1`.

Totals are reproducible to ~1 ppm per binary; the `-fno-*` tail beyond the
`-O`/`-march`/`-flto` base is adopted at the default `-T 0` threshold and
includes marginal picks that vary between runs (use `-T 3` for a stable set).

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 23,512,257      | -O3 -ffast-math -march=native -fno-align-loops -funroll-all-loops |
| mat1bench   | 28,958,911      | -O3 -ffast-math -march=native **-fprofile-use** -fno-align-loops -fno-asynchronous-unwind-tables -fno-tree-loop-distribute-patterns -funroll-all-loops -ffp-contract=on -fira-algorithm=priority |
| almabench   | 311,361,518     | -O2 -ffast-math -march=native -flto **-fprofile-use** -fno-caller-saves -fno-cse-follow-jumps -fno-if-conversion -fno-move-loop-invariants -fno-peephole2 -fno-plt -fno-reorder-functions -fno-sched-dep-count-heuristic -fno-thread-jumps -fno-tree-ter -funroll-all-loops -fira-region=all |
| fftbench    | 448,263,892     | -Os -ffast-math -march=native -flto -fno-plt -fno-tree-sink -ffp-contract=on -freorder-blocks-algorithm=simple |
| linbench    | 96,244,520      | -O3 -ffast-math -march=native -flto -fno-align-jumps -fno-align-loops -fno-caller-saves -fno-dse -fno-forward-propagate -fno-if-conversion -fno-if-conversion2 -fno-plt -fno-tree-loop-distribute-patterns -funroll-all-loops -fira-algorithm=priority -fira-region=one -fvect-cost-model=unlimited |
| evobench    | 566,866,541     | -O3 -ffast-math -march=native **-fprofile-use** -fno-align-jumps -fno-asynchronous-unwind-tables -fno-early-inlining -fno-if-conversion -fno-move-loop-invariants -fno-plt -fno-thread-jumps -fno-tree-dominator-opts -funroll-all-loops -ffp-contract=on |
| treebench   | 833,805,037     | -O3 -flto **-fprofile-use** -fno-align-jumps -fno-asynchronous-unwind-tables -fno-dce -fno-dse -fno-forward-propagate -fno-gcse -fno-guess-branch-probability -fno-inline-functions-called-once -fno-ipa-modref -fno-ipa-sra -fno-ira-share-spill-slots -fno-lra-remat -fno-move-loop-invariants -fno-plt -fno-schedule-insns2 -fno-ssa-phiopt -fno-tree-dominator-opts -fno-tree-loop-distribute-patterns -fno-tree-reassoc -fno-tree-slp-vectorize |
| huffbench   | 944,629,971     | -O3 -ffast-math -march=native **-fprofile-use** -fno-align-functions -fno-align-loops -fno-dce -fno-if-conversion -fno-if-conversion2 -fno-ipa-bit-cp -fno-plt -fno-ssa-phiopt -fno-toplevel-reorder -funroll-all-loops -fira-algorithm=priority |

> **almabench's greedy local optimum is gone.** The previous table carried a
> warning: greedy `-l 1` landed at ~347M, ~8% above the best reachable 319.6M,
> because `-flto` only helps almabench inside one specific co-set. With PGO
> searchable, the greedy path changes: adopting `-fprofile-use` makes `-flto`
> a straight win, and the search now lands at **311.4M** — below the former
> "unreachable" 319.6M. A coordinated-move limitation can dissolve when a new
> option reshapes the adoption path (see
> [Search limitations](#search-limitations)).

#### Observations

- `-fprofile-use` (PGO) was adopted by 5 of 8: almabench (−10.4% vs the
  pre-PGO table), huffbench (−5.8%), evobench and treebench (−0.7%),
  mat1bench (marginal). Rejected for distbench/fftbench/linbench —
  consistent with the fixed A/B (TODO.md item 1).
- All benchmarks build on `-O3` + `-ffast-math`; fftbench prefers `-Os` and
  almabench now `-O2` (+PGO +LTO).
- `-funroll-all-loops` is a broad win (distbench, mat1bench, almabench,
  linbench, evobench, huffbench).
- `-fno-if-conversion` and `-fno-plt` recur on the branch-heavy / call-heavy
  benchmarks; `-march=native` helps everyone except treebench this round
  (huffbench adopts it under PGO).
- treebench picks up the most `-fno-*` flags (largest search surface).
  huffbench `-p` remains the one benchmark where quick mode visibly trails
  the former hand-curated result (see [Quick vs audit](#quick-vs-audit)).

### Clang 22 — instructions

Config: `config/clang22.osearch` (107 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | Instructions    | Best flags |
|-------------|-----------------|------------|
| distbench   | 39,092,624      | -O3 -march=native -ffast-math |
| mat1bench   | 54,305,654      | -O3 -flto -march=native **-fprofile-use** -ffast-math -mllvm -force-vector-width=8 |
| almabench   | 215,813,021     | -O3 -flto=thin -march=native **-fprofile-use** -ffast-math -fveclib=libmvec -fno-omit-frame-pointer -fno-plt -fno-direct-access-external-data -mno-vzeroupper -mllvm -unroll-threshold=200 |
| fftbench    | 252,279,010     | -O3 -march=native **-fprofile-use** -ffast-math -fno-plt -fno-builtin -mllvm -enable-ext-tsp-block-placement |
| linbench    | 108,958,981     | -O3 -flto=thin -march=native **-fprofile-use** -ffast-math -ffp-contract=on -fno-asynchronous-unwind-tables -fno-plt -fno-jump-tables -mno-vzeroupper |
| evobench    | 557,312,637     | -O3 -flto=thin -march=native -ffast-math -ffp-contract=on -fno-optimize-sibling-calls -mno-vzeroupper -mllvm -enable-gvn-hoist |
| treebench   | 612,854,757     | -Os -flto=thin -march=native **-fprofile-use** -fno-strict-overflow -fno-plt -fno-optimize-sibling-calls -fno-builtin -mno-vzeroupper -mllvm -inline-threshold=500 -mllvm -enable-gvn-sink |
| huffbench   | 1,145,983,651   | -O3 -flto -march=native **-fprofile-use** -fno-inline-functions -fno-plt -mllvm -unroll-threshold=200 |

#### Observations

- `-fprofile-use` (PGO) was adopted by 6 of 8 — treebench is the headline
  (−21.2% vs the pre-PGO table: 777.9M → 612.9M), then huffbench (−3.8%),
  fftbench (−0.9%), linbench (−0.7%), almabench and mat1bench (marginal).
  Correctly *rejected* for evobench, where the A/B measured +15.7%
  instructions (a `-p` artifact — see the [audit
  table](#audit-retired-ops-and-cycles-on-the--p-winners): its cycles are
  unchanged under PGO).
- `-march=native` is now universal (PGO brought treebench along); LTO is
  near-universal, and `-flto=thin` beats full `-flto` wherever both compete.
  With PGO in the mix every benchmark picks `-O3` except treebench (`-Os`).
- `-fno-plt` consistently helps (avoids PLT indirection on hot calls);
  `-mno-vzeroupper` recurs.
- fftbench now skips LTO and instead adopts
  `-mllvm -enable-ext-tsp-block-placement` (profile-driven basic-block
  layout — only useful *because* PGO is in the flag set).

### GCC 16 vs Clang 22 — Instructions (-p)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 | Clang 22 | Winner | Δ |
|-----------|--------|----------|--------|---|
| distbench | 23,512,257 | 39,092,624 | GCC | −40% |
| mat1bench | 28,958,911 | 54,305,654 | GCC | −47% |
| almabench | 311,361,518 | 215,813,021 | Clang | −31% |
| fftbench | 448,263,892 | 252,279,010 | Clang | −44% |
| linbench | 96,244,520 | 108,958,981 | GCC | −12% |
| evobench | 566,866,541 | 557,312,637 | Clang | −2% |
| treebench | 833,805,037 | 612,854,757 | Clang | −26% |
| huffbench | 944,629,971 | 1,145,983,651 | GCC | −18% |

It's 4–4: GCC wins distbench (−40%), mat1bench (−47%), linbench (−12%), and
huffbench (−18%); Clang wins fftbench (−44%), almabench (−31%), treebench
(−26%), and evobench (−2%). PGO moved two margins: treebench widened from
−7% to −26% (Clang's PGO is far more effective there) and almabench narrowed
from −38% to −31% (GCC's PGO+LTO escape). Earlier still, almabench flipped
from "GCC −46%" when `-fveclib=libmvec` was added to the Clang config — see
[Why the big gaps](#why-the-big-gaps).

(Both columns are the single annotated config in quick mode (`-k 80`), so this
is an apples-to-apples comparison. Earlier revisions measured GCC with a weaker
speed-only config and reported "Clang wins 5/8" — that was the config, not the
compiler. Quick-mode caveat: GCC huffbench quick lands at 0.94B, above the
~0.89B the former hand-curated set found pre-PGO — a greedy path-dependence,
see [Quick vs audit](#quick-vs-audit).)

#### Why the big gaps

*(This analysis was done on the pre-PGO winners of 2026-07-04; the
mechanisms — vectorization strategy, loop interchange, veclib defaults,
idiom recognition — are properties of the compilers and still hold. The
treebench gap is newer: it is PGO effectiveness, not a codegen idiom, and is
covered under the tables above.)*

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

### Audit: retired ops and cycles on the -p winners

Each `-p` winner above, rebuilt and measured with the harness's `-g` mode
(instructions + retired ops + cycles in one atomic counter-group read;
median of 21 self-pinned runs, in millions):

| Benchmark | GCC insns / ops / cycles | Clang insns / ops / cycles | Cycles winner | (-p winner) |
|-----------|--------------------------|----------------------------|---------------|-------------|
| distbench | 23.5 / 23.0 / 32.1 | 39.1 / 39.1 / 32.4 | ~tie (GCC −1%) | GCC −40% |
| mat1bench | 29.0 / 27.7 / 54.4 | 54.3 / 586.3 / 161.6 | GCC −66% | GCC −47% |
| almabench | 311.4 / 329.8 / 158.7 | 215.8 / 282.5 / 106.9 | Clang −33% | Clang −31% |
| fftbench | 448.3 / 424.2 / 146.9 | 252.3 / 243.4 / 116.0 | Clang −21% | Clang −44% |
| linbench | 96.2 / 115.9 / 117.2 | 109.0 / 120.8 / 113.4 | Clang −3% | GCC −12% |
| evobench | 566.9 / 655.1 / 616.2 | 557.3 / 645.5 / 615.5 | wash | Clang −2% |
| treebench | 833.8 / 663.5 / 677.0 | 612.9 / 609.5 / 475.8 | Clang −30% | Clang −26% |
| huffbench | 944.6 / 762.1 / 839.4 | 1,146.0 / 1,020.7 / 400.1 | **Clang −52%** | **GCC −18%** |

On cycles the scoreboard is Clang 5, GCC 2 (mat1bench decisively, distbench
barely), evobench a wash — versus 4–4 on instructions. The recurring
lessons (measured in detail in TODO.md item 2):

- **Instruction counts under-price microcoded instructions.** Clang's
  mat1bench gather kernel is 586M ops from 54M instructions (10.8×); its
  real cycle loss is −66%, worse than the −47% the `-p` table shows.
  Conversely distbench's "GCC −40%" is a wall of cheap shuffles — a cycle
  tie.
- **Neither instructions nor ops see stalls.** huffbench flips outright:
  GCC retires 18% fewer instructions and 24% fewer ops, yet takes 2.1× the
  cycles (IPC 1.1 vs 2.9). linbench flips the same way, mildly.
- **PGO looks better in cycles than in counts** — clang/treebench is −30%
  in cycles vs −21% in instructions, and the A/B's apparent PGO
  "regressions" (clang/evobench, gcc/linbench) vanish under cycles.
- Cycle medians on this shared host carry 0.1–7% run-to-run spread
  (fftbench up to ~20% — its 16 MB working set is at the mercy of physical
  page placement), which is why cycles audit the tables instead of driving
  the search.

## Code size (-s)

Fully deterministic: `.text` byte counts are 100% reproducible across runs.
Sizes are re-based +482…+673 B over the 2026-07-04 tables: the harness's
counter machinery (`open_counter`, the CPU-pinning helper, the mode switch)
is real `.text` in every binary, on top of the earlier +23…+196 B for the
DCE-proof checksum code. The flag families are unchanged; the harness is now
a large fraction of the smallest binaries, which compresses the relative
GCC-vs-Clang deltas.

### GCC 16 — size

Config: `config/gcc16.osearch` (227 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 1585          | -Os -flto -fno-guess-branch-probability -fno-if-conversion -fno-math-errno -fno-ssa-phiopt -fno-thread-jumps -fno-tree-loop-im -fno-tree-sink -fno-tree-tail-merge |
| mat1bench   | 1560          | -Os -flto -fno-guess-branch-probability -fno-if-conversion -fno-move-loop-stores -fno-ssa-phiopt -fno-thread-jumps -fno-tree-sink -fira-algorithm=priority -fira-region=all |
| almabench   | 2970          | -Os -march=native -flto -fno-caller-saves -fno-guess-branch-probability -fno-if-conversion -fno-math-errno -fno-ssa-phiopt -fno-tree-sink -fno-tree-tail-merge |
| fftbench    | 1917          | -Os -flto -fno-caller-saves -fno-guess-branch-probability -fno-if-conversion -fno-move-loop-invariants -fno-ssa-phiopt -fno-thread-jumps -fno-tree-loop-im -fno-tree-sink -fira-region=all |
| linbench    | 2154          | -Os -flto -fno-guess-branch-probability -fno-if-conversion -fno-move-loop-invariants -fno-ssa-phiopt -fno-thread-jumps -fno-tree-sink -fno-tree-tail-merge -ffinite-math-only |
| evobench    | 2397          | -Os -march=native -flto -fno-caller-saves -fno-guess-branch-probability -fno-if-conversion -fno-tree-sink -fno-tree-tail-merge -ffp-contract=off |
| treebench   | 5096          | -Os -flto -fno-code-hoisting -fno-forward-propagate -fno-guess-branch-probability -fno-if-conversion -fno-move-loop-invariants -fno-optimize-sibling-calls -fno-tree-dce -fno-tree-scev-cprop -fno-tree-sink |
| huffbench   | 2597          | -Os -flto -fno-caller-saves -fno-guess-branch-probability -fno-if-conversion -fno-move-loop-invariants -fno-ssa-phiopt -fno-thread-jumps -fno-tree-forwprop -fno-tree-scev-cprop -fno-tree-sink -fno-tree-tail-merge -fira-algorithm=priority -fira-region=all |

#### Observations

- `-Os -flto` is now the foundation for all eight (distbench rejoined)
- `-march=native` only pays where wide vectors earn their encoding bytes
  (almabench, evobench); the counter-heavy harness tips the others away
- `-fno-tree-sink`, `-fno-if-conversion`, `-fno-guess-branch-probability`
  and `-fno-thread-jumps` recur as the broad size-reducers
- `-fprofile-use` was never adopted in size mode (the A/B showed its `.text`
  effect is mixed: often smaller, sometimes much larger)
- treebench is the largest (5096 B) — recursive tree traversal has many code paths

### Clang 22 — size

Config: `config/clang22.osearch` (107 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 1680          | -Oz -flto=thin -march=native -ffp-contract=off |
| mat1bench   | 1681          | -Oz -flto=thin -march=native |
| almabench   | 3306          | -Oz -flto=thin -march=native -ffinite-math-only -ffp-contract=fast -fno-omit-frame-pointer -fno-builtin -mllvm -enable-newgvn |
| fftbench    | 1971          | -Oz -flto=thin -march=native -ffp-contract=fast |
| linbench    | 2200          | -Oz -flto=thin -march=native -ffinite-math-only |
| evobench    | 2376          | -Oz -flto -march=native -fno-signed-zeros -ffp-contract=off -fno-inline-functions -fno-builtin |
| treebench   | 4903          | -Oz -flto -march=native -fno-optimize-sibling-calls -mllvm -enable-newgvn |
| huffbench   | 2854          | -Oz -flto=thin -mtune=native |

#### Observations

- `-Oz` + LTO wins for every benchmark; `-flto=thin` now edges full `-flto`
  in most cases
- `-march=native` is adopted almost everywhere this round (only huffbench
  keeps `-mtune=native`) — with the harness's counter code in every binary,
  the `-march` encoding cost is a smaller relative penalty
- `-mllvm -enable-newgvn` still helps almabench and treebench
- `-fprofile-use` was never adopted in size mode

### GCC 16 vs Clang 22 — Size (-s)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 (.text) | Clang 22 (.text) | Winner | Δ |
|-----------|----------------|------------------|--------|---|
| distbench | 1585 | 1680 | GCC | −6% |
| mat1bench | 1560 | 1681 | GCC | −7% |
| almabench | 2970 | 3306 | GCC | −10% |
| fftbench | 1917 | 1971 | GCC | −3% |
| linbench | 2154 | 2200 | GCC | −2% |
| evobench | 2397 | 2376 | Clang | −1% |
| treebench | 5096 | 4903 | Clang | −4% |
| huffbench | 2597 | 2854 | GCC | −9% |

GCC wins 6 of 8 on code size, by 2–10% (largest on almabench and huffbench);
Clang takes treebench (−4%) and, newly, evobench (−1%). The margins are
tighter than the previous 3–14% partly because the fixed ~0.5 KB harness now
dilutes the benchmark-code difference in every binary.

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

One benchmark is worth calling out:

- **huffbench `-p`** (quick 0.94B with PGO) remains the widest gap. Its best
  flag set is *specific to that workload* (e.g. `-fipa-cp-clone`,
  `-fsplit-paths`, `-finline-stringops`), so those options rank low on the
  cross-benchmark weights. This is greedy *path-dependence*, not coverage:
  pre-PGO, widening to `-k 130` only nudged 1.00B to 0.98B and even an
  uncapped audit reached just 0.98B — so the flags are reachable; greedy
  simply doesn't take that path. A smaller, hand-chosen space (the former
  curated config, ~0.89B pre-PGO) happened to steer the hill-climb down a
  better path. It is the clearest price of replacing a hand-curated set with
  data-driven weights — and the kind of case `-l 2` or a non-greedy search
  (TODO.md item 3) would address.

(almabench `-p` was the other callout — a greedy local optimum that no `-k`
escaped — until PGO dissolved it; see below.)

Everywhere else, one annotated file in quick mode is within noise of a thorough
search, which was the point of converging the configs.

### Search limitations

osearch is greedy hill-climbing (`-l 1` toggles one flag at a time, keeping
improvements). Two effects surface in the tables:

- **Local optima.** When the best result needs a *coordinated* multi-flag
  combination, greedy can't reach it. almabench `-p` was the documented
  case: `-flto` helped it only together with a specific co-set (→ 319.6M)
  and was +29% *worse* on any other path, so single-flag steps rejected it
  and settled at ~347M. Instructively, adding `-fprofile-use` to the config
  *dissolved* this optimum — once PGO is adopted, `-flto` becomes a straight
  single-flag win and greedy now reaches 311.4M, below the formerly
  "unreachable" 319.6M. The general limitation stands (it took a new option
  to reshape the path, not a smarter search); pair search (`-l 2`) is
  impractically slow on a full config, and multiple random restarts or a
  non-greedy search (the genetic approach of the ACOVEA ancestor) remain the
  real fix — see TODO.md item 3.
- **Coverage vs. the weights.** Quick mode trusts the cross-benchmark weights
  to front-load the useful options. A workload whose winning flags are specific
  to it (and so rank low on average) can be under-served; a larger `-k` widens
  reach, but greedy path-dependence means it does not always recover a
  hand-curated result (huffbench `-p`, see [Quick vs audit](#quick-vs-audit)).
- **Path sensitivity.** At the default `-T 0` the marginal `-fno-*` tail is
  noise-level, so the exact flag set varies between runs; totals are stable.

### Reproducibility

- **Size (-s):** 100% reproducible across runs
- **Instructions (-p) and retired ops (-u):** totals reproducible to ~1 ppm
  (`-u` to ~100 ppm); at the default `-T 0` the marginal `-fno-*` picks vary
  run-to-run, but `-T 3` yields a stable flag set. PGO builds are just as
  deterministic (the training run is deterministic, so the profile is too).
- **Cycles (-c):** 0.1–7% run-to-run spread even self-pinned (fftbench up to
  ~20%); an audit metric, not a search objective.
- **Time (default):** too noisy on this shared host to optimize on — the
  greedy search even adopts `-Os` for compute-bound FP benchmarks. Use `-s`,
  `-p` or `-u` instead (see [Time](#time-default)).
- **Rebuilding a winner by hand:** match the harness invocation —
  `scripts/cc-pgo.sh gcc -std=gnu23 <flags> -o out bench.c -lm -lrt` from the
  repo root (the wrapper is a passthrough unless the flags include
  `-fprofile-use`, in which case it runs the full instrument → train →
  recompile protocol; every winner above was machine-verified to reproduce
  this way). The `gnu` part of `-std=gnu23` matters, not the year: any
  strict-ISO mode (`-std=c99`, `-std=c23`, …) restricts GCC's FP contraction
  when `-ffast-math` isn't among the winning flags — e.g. the fftbench GCC
  winner measures 448.3M under `gnu23` (identical to the gnu-mode default)
  but 490.2M under `c23` or `c99` (+9%, the un-contracted butterflies).
  `gnu23` is also what GCC 16 defaults to; pinning it aligns Clang (default
  `gnu17`) to the same language mode.
