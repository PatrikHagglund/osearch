# Benchmark Results

## Environment

- **CPU:** AMD EPYC 9354 (Zen 4) — AVX-512 + AVX2; `-march=native` ⇒ `znver4`
- **OS:** Fedora Linux 44 (container)
- **Compilers:** GCC 16.1.1, Clang 22.1.8
- **Date:** 2026-07-09 (speed tables under `-u`, the primary speed
  objective; all 32 searches re-run after the config weights were
  regenerated under `-u`. Each row is the best *verified* quick-run result
  across the two ranking revisions — greedy paths differ between rankings,
  see [Quick vs audit](#quick-vs-audit). PGO searchable throughout via the
  `-fprofile-use` pseudo-flag expanded by `scripts/cc-pgo.sh`)
- **Configs:** one annotated file per compiler — `gcc16` (227 flags) and
  `clang22` (107 flags), under `config/`. Each carries per-flag `w_speed` /
  `w_size` weights; a *quick* run restricts to the top-ranked options
  (`-k n` / `-Q n`), an *audit* run uncaps and reaches every option. The tables
  below are quick runs; an audit matches them within ~1–2% except where a
  workload's best flags are benchmark-specific (see
  [Quick vs audit](#quick-vs-audit)).

`-u` counts are user-space retired macro-ops (AMD Zen `ex_ret_ops`) for
`run()` only, via the in-harness `perf_event_open` counter. The harness can
also count instructions (`-p`, the portable fallback) and self-pinned core
cycles (`-c`), or all three atomically (`-g`) — the [audit
table](#audit-instructions-and-cycles-on-the--u-winners) below uses `-g`.
Absolute size/time figures are larger than pre-2026-06 revisions because
DCE-prevention sinks and the harness's counter code are real `.text` and
work in every binary.

Every benchmark folds its full computed output into a checksum
(`bench_result` in `main.ic`) that is printed under `-v`, so no compiler at
any optimization level may dead-code-eliminate the intended computation —
and the checksums double as a correctness check: all eight are bit-identical
between GCC 16 and Clang 22 at `-O2` (strict FP), and agree to ≥10
significant digits under `-O3 -march=native -ffast-math -flto`. (fftbench's
checksum sums `|re|+|im|` so it is well-conditioned; the signed spectrum sum
cancels to ~0 and made harmless FP wobble look large.)

## Summary

- **The speed objective is `-u` (retired ops); size is `-s`.** Both
  deterministic in practice. `-u` prices microcoded instructions honestly
  where an instruction count sees 1 — and that changed real outcomes: the
  `-u` search found a clang/mat1bench binary **~9% faster in cycles** than
  the `-p` winner by fleeing AVX-512 gathers, a binary `-p` structurally
  could not choose. `-p` (instructions) remains as the portable fallback
  (`ex_ret_ops` is Zen-specific) and is reported in the audit table.
  Wall-clock time is too noisy here to optimize on (see
  [Time](#time-default)); cycles (`-c`) is the ground-truth *audit* metric
  but not reproducible enough to search on.
- **PGO is searchable and pays.** The `-fprofile-use` pseudo-flag (expanded
  into instrument → train → recompile by `scripts/cc-pgo.sh`) was adopted in
  10 of the 16 `-u` speed searches — biggest wins clang/treebench,
  gcc/almabench (whose documented greedy local optimum it dissolved),
  huffbench on both compilers — and correctly rejected where it costs ops
  (clang/evobench). No size search adopted it. Training input = benchmark
  input, so this is best-case PGO.
- **One annotated config per compiler.** `gcc16` and `clang22` each search both
  speed and size (their `-O` level is an enum spanning `-O3…-Os`/`-Oz`) and
  carry per-flag effectiveness weights, so a quick `-k n` run searches the
  top-ranked options and an uncapped audit reaches the rest. The former
  separate "curated" and "full" configs are now one file each.
- **GCC vs Clang, retired ops:** 4–4. Biggest gaps: mat1bench (GCC −82%),
  fftbench (Clang −38%), huffbench (GCC −26%) — the classic gaps each
  root-caused to a single hot-loop codegen decision, see
  [Why the big gaps](#why-the-big-gaps).
  See [comparison](#gcc-16-vs-clang-22--retired-ops--u).
- **On cycles the picture shifts:** Clang 4, GCC 1, three washes — ops fix
  the gather blindness that instruction counts had, but still hide Clang's
  2.1× huffbench cycle win. See the [audit
  table](#audit-instructions-and-cycles-on-the--u-winners).
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

## Retired ops (-u)

The primary reproducible performance metric: user-space retired macro-ops
for `run()` (AMD Zen `ex_ret_ops`), counted in-harness and stable to
~100 ppm per binary. Unlike an instruction count, this prices microcoded
instructions (gathers, wide shuffles) at what they actually cost the
decoder — and the search exploits that: see mat1bench below, where the
`-u` objective found a binary ~9% *faster in cycles* than the `-p` winner
by retiring 3× the instructions. Instructions (`-p`) remain the portable
fallback (the ops event is Zen-specific) and are reported per winner in the
[audit table](#audit-instructions-and-cycles-on-the--u-winners).

### GCC 16 — retired ops

Config: `config/gcc16.osearch` (227 flags), quick mode (`-k 80`), greedy `-l 1`.

The `-fno-*` tail beyond the `-O`/`-march`/`-flto` base is adopted at the
default `-T 0` threshold and includes marginal picks that vary between runs
(use `-T 3` for a stable set).

| Benchmark   | Retired ops     | Best flags |
|-------------|-----------------|------------|
| distbench   | 23,021,840      | -O2 -ffast-math -march=native -flto -fno-dce -funroll-all-loops -ffp-contract=on -fira-region=one -freorder-blocks-algorithm=simple |
| mat1bench   | 27,668,187      | -O2 -ffast-math -march=native -funroll-all-loops -ffp-contract=on -freorder-blocks-algorithm=simple |
| almabench   | 329,950,059     | -O2 -ffast-math -march=native -flto **-fprofile-use** -fno-align-loops -fno-caller-saves -fno-crossjumping -fno-cse-follow-jumps -fno-if-conversion -fno-peephole2 -fno-plt -fno-schedule-insns2 -fno-tree-slp-vectorize -fno-tree-ter -funroll-all-loops -fira-algorithm=priority |
| fftbench    | 393,782,288     | -O3 -ffast-math -march=native -flto -fno-tree-loop-distribute-patterns -fno-tree-ter -funroll-all-loops -fexcess-precision=standard -ffp-contract=on |
| linbench    | 112,264,690     | -O3 -ffast-math -march=native **-fprofile-use** -fno-align-functions -fno-align-loops -fno-dse -fno-forward-propagate -fno-if-conversion -fno-omit-frame-pointer -fno-peephole2 -funroll-all-loops -fvect-cost-model=unlimited |
| evobench    | 654,551,385     | -O3 -ffast-math -march=native **-fprofile-use** -fno-align-functions -fno-align-jumps -fno-align-loops -fno-caller-saves -fno-code-hoisting -fno-crossjumping -fno-if-conversion -fno-move-loop-invariants -fno-omit-frame-pointer -fno-shrink-wrap -fno-tree-dominator-opts -fno-tree-vrp -funroll-all-loops -ffp-contract=on -freorder-blocks-algorithm=simple |
| treebench   | 672,141,072     | -O3 -flto **-fprofile-use** -fno-align-jumps -fno-align-loops -fno-code-hoisting -fno-dse -fno-forward-propagate -fno-gcse -fno-ipa-ra -fno-tree-loop-distribute-patterns -fira-region=one |
| huffbench   | 742,407,036     | -O3 -march=native **-fprofile-use** -fno-align-loops -fno-asynchronous-unwind-tables -fno-code-hoisting -fno-cprop-registers -fno-if-conversion -fno-if-conversion2 -funroll-all-loops -fira-algorithm=priority -fira-region=one |

#### Observations

- `-fprofile-use` (PGO) adopted by 5 of 8 (almabench, linbench, evobench,
  treebench, huffbench). linbench is new versus the `-p` search: PGO cost it
  +3% *instructions* (rejected) but pays in *ops* (adopted) — the metric
  changes what the search keeps.
- Versus the ops that the former `-p` winners scored: fftbench −7%
  (393.8M vs 424.2M — a different flag shape, `-O3 -flto
  -funroll-all-loops` instead of `-Os`), huffbench −2.6%, linbench −3.1%,
  the rest equal. One counter-case: treebench lands 2% *more* ops than the
  `-p` winner's binary scored (greedy paths differ), yet its cycles are
  ~4% better — at these margins neither count is the last word.
- `-funroll-all-loops` is adopted by all eight — with loop overhead priced
  in ops it is an unambiguous win.
- `-fno-if-conversion` recurs on almost every benchmark; `-march=native` on
  all but the refreshed treebench winner.

### Clang 22 — retired ops

Config: `config/clang22.osearch` (107 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | Retired ops     | Best flags |
|-------------|-----------------|------------|
| distbench   | 39,098,514      | -O2 -flto -march=native -ffast-math -finline-hint-functions |
| mat1bench   | 156,443,246     | -O2 -flto -ffast-math -fno-asynchronous-unwind-tables **-mavx2 -mfma** -mllvm -force-vector-width=8 -mllvm -unroll-threshold=200 |
| almabench   | 282,447,153     | -O3 -flto=thin -march=native **-fprofile-use** -ffast-math -fveclib=libmvec -fno-omit-frame-pointer -fno-plt -fno-jump-tables -fvisibility=hidden -mno-vzeroupper -mprefer-vector-width=256 -mllvm -unroll-threshold=200 |
| fftbench    | 243,405,254     | -O3 -march=native **-fprofile-use** -fno-omit-frame-pointer -fno-direct-access-external-data -fno-builtin -mllvm -unroll-threshold=200 -mllvm -enable-ext-tsp-block-placement |
| linbench    | 120,838,290     | -O3 -march=native **-fprofile-use** -ffast-math -ffp-contract=on -ffunction-sections |
| evobench    | 645,080,055     | -O3 -flto=thin -march=native -ffast-math -ffp-contract=on -fno-plt -fno-optimize-sibling-calls -fno-builtin -mno-vzeroupper |
| treebench   | 602,757,776     | -O3 -flto=thin -march=native **-fprofile-use** -ffast-math -fno-unroll-loops -fno-strict-overflow -fno-asynchronous-unwind-tables -fno-plt -fno-optimize-sibling-calls -fno-jump-tables -fno-builtin -mllvm -enable-gvn-sink |
| huffbench   | 1,005,035,442   | -O3 -flto **-fprofile-use** -fno-inline-functions -fno-omit-frame-pointer -fno-asynchronous-unwind-tables -fno-plt -fno-direct-access-external-data -mllvm -unroll-threshold=200 -mllvm -enable-newgvn |

#### Observations

- **mat1bench is the headline of the `-u` switch.** The search dropped
  `-march=native` for `-mavx2 -mfma`: without AVX-512 there is no
  `vgatherqpd`, so the microcode explosion disappears — 156.4M ops versus
  the 586.3M the `-p` winner scored (−73%). The binary retires 3× the
  instructions (159.4M vs 54.3M — under `-p` it could never be chosen), yet
  runs **~9% faster in cycles** (145.0M vs 160.0M min). The objective did
  not just re-score the search; it steered it to genuinely better code.
- `-fprofile-use` (PGO) adopted by 5 of 8 (almabench, fftbench, linbench,
  treebench, huffbench). Rejected for evobench — PGO costs it +13.5% ops,
  the same artifact as its +15.7% instructions; only cycles show it
  neutral, and cycles aren't searchable.
- treebench refines its PGO win further (602.8M ops, now with `-ffast-math
  -fno-unroll-loops`); huffbench sheds 2% more ops than its `-p` winner.
- `-mllvm -enable-ext-tsp-block-placement` (profile-driven block layout)
  again rides along with PGO on fftbench.

### GCC 16 vs Clang 22 — Retired ops (-u)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 | Clang 22 | Winner | Δ |
|-----------|--------|----------|--------|---|
| distbench | 23,021,840 | 39,098,514 | GCC | −41% |
| mat1bench | 27,668,187 | 156,443,246 | GCC | −82% |
| almabench | 329,950,059 | 282,447,153 | Clang | −14% |
| fftbench | 393,782,288 | 243,405,254 | Clang | −38% |
| linbench | 112,264,690 | 120,838,290 | GCC | −7% |
| evobench | 654,551,385 | 645,080,055 | Clang | −1% |
| treebench | 672,141,072 | 602,757,776 | Clang | −10% |
| huffbench | 742,407,036 | 1,005,035,442 | GCC | −26% |

Still 4–4: GCC wins distbench (−41%), mat1bench (−82%), linbench (−7%), and
huffbench (−26%); Clang wins fftbench (−38%), almabench (−14%), treebench
(−10%), and evobench (−1%). Compared to the instruction-count head-to-head,
mat1bench widens sharply (−47% → −82%: even after Clang's search fled to
AVX2 to escape the gathers, GCC's interchanged broadcast-FMA kernel does the
same work in a fraction of the ops) and huffbench widens (−18% → −26%) —
but the cycles audit below shows ops still overstate GCC on the
stall-bound benchmarks.

(Both columns are the single annotated config in quick mode (`-k 80`), so
this is an apples-to-apples comparison. The per-flag `w_speed` weights that
rank quick-mode candidates were measured under `-p`; they order well for
`-u` too, but regenerating them with `scripts/annotate.sh` under `-u` would
make the ranking native.)

#### Why the big gaps

*(This analysis was done in instruction counts on the pre-PGO winners of
2026-07-04; the mechanisms — vectorization strategy, loop interchange,
veclib defaults, idiom recognition — are properties of the compilers and
still hold. Two later developments layer on top: the treebench gap is PGO
effectiveness, not a codegen idiom, and the mat1bench story gained a coda —
under the `-u` objective Clang's search escapes the gather kernel entirely
by dropping to AVX2, recovering ~9% in cycles; see the Clang observations
above.)*

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

### Audit: instructions and cycles on the -u winners

Each `-u` winner above, rebuilt and measured with the harness's `-g` mode
(instructions + retired ops + cycles in one atomic counter-group read;
median of 21 self-pinned runs, in millions). This is also where the
portable instruction counts (`-p`) live now that ops are the headline:

| Benchmark | GCC insns / ops / cycles | Clang insns / ops / cycles | Cycles winner | (-u winner) |
|-----------|--------------------------|----------------------------|---------------|-------------|
| distbench | 23.5 / 23.0 / 32.1 | 39.1 / 39.1 / 32.4 | ~tie (GCC −1%) | GCC −41% |
| mat1bench | 29.0 / 27.7 / 55.1 | 159.4 / 156.5 / 146.2 | GCC −62% | GCC −82% |
| almabench | 311.5 / 330.0 / 158.7 | 216.7 / 282.5 / 106.6 | Clang −33% | Clang −14% |
| fftbench | 406.6 / 393.8 / 151.8 | 252.3 / 243.4 / 117.7 | Clang −22% | Clang −38% |
| linbench | 96.2 / 112.3 / 115.9 | 109.0 / 120.9 / 116.0 | wash | GCC −7% |
| evobench | 566.8 / 654.6 / 614.9 | 557.1 / 645.1 / 616.6 | wash | Clang −1% |
| treebench | 842.5 / 672.3 / 681.7 | 616.4 / 602.8 / 475.3 | Clang −30% | Clang −10% |
| huffbench | 949.6 / 742.5 / 858.3 | 1,163.8 / 1,005.0 / 400.4 | **Clang −53%** | **GCC −26%** |

On cycles the scoreboard is Clang 4, GCC 1 (mat1bench), three washes
(distbench, linbench, evobench) — versus 4–4 on ops. The recurring lessons
(measured in detail in TODO.md item 2):

- **Ops fix the microcode blindness but can overshoot.** mat1bench under
  `-u` escaped the gather kernel and gained ~9% real cycles — the metric
  working as intended — yet the remaining "GCC −82%" op gap corresponds to
  −62% in cycles: GCC's broadcast-FMA kernel retires very few ops but
  doesn't run proportionally faster (port-limited, not decode-limited).
- **Neither instructions nor ops see stalls.** huffbench still flips
  outright: GCC retires 26% fewer ops, yet takes 2.1× the cycles (IPC 1.1
  vs 2.9). linbench's −7% op win is a cycle wash.
- **PGO looks better in cycles than in counts** — clang/treebench is −30%
  in cycles vs −10% in ops against GCC's PGO'd winner.
- **Ops-optimal ≠ cycle-optimal at the margins:** the 2026-07-09 gcc
  treebench refresh gained 0.7% ops over the previous winner but costs ~5%
  more cycles (681.7M vs 647.8M, both re-measured back-to-back) — the class
  of trade `-C` (hybrid cycle validation) exists to catch.
- Cycle medians on this shared host carry 0.1–7% run-to-run spread
  (fftbench the worst — its 16 MB working set is at the mercy of physical
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
| treebench   | 5073          | -Os -flto -fno-code-hoisting -fno-guess-branch-probability -fno-if-conversion -fno-ira-share-spill-slots -fno-ivopts -fno-tree-forwprop -fno-tree-scev-cprop -fno-tree-sink -fira-region=all |
| huffbench   | 2597          | -Os -flto -fno-caller-saves -fno-guess-branch-probability -fno-if-conversion -fno-move-loop-invariants -fno-ssa-phiopt -fno-thread-jumps -fno-tree-forwprop -fno-tree-scev-cprop -fno-tree-sink -fno-tree-tail-merge -fira-algorithm=priority -fira-region=all |

#### Observations

- `-Os -flto` is now the foundation for all eight (distbench rejoined)
- `-march=native` only pays where wide vectors earn their encoding bytes
  (almabench, evobench); the counter-heavy harness tips the others away
- `-fno-tree-sink`, `-fno-if-conversion`, `-fno-guess-branch-probability`
  and `-fno-thread-jumps` recur as the broad size-reducers
- `-fprofile-use` was never adopted in size mode (the A/B showed its `.text`
  effect is mixed: often smaller, sometimes much larger)
- treebench is the largest (5073 B) — recursive tree traversal has many code paths

### Clang 22 — size

Config: `config/clang22.osearch` (107 flags), quick mode (`-k 80`), greedy `-l 1`.

| Benchmark   | .text (bytes) | Best flags |
|-------------|---------------|------------|
| distbench   | 1649          | -Oz -flto=thin -march=native -ffp-contract=off -mno-vzeroupper |
| mat1bench   | 1650          | -Oz -flto=thin -march=native -mno-vzeroupper |
| almabench   | 3284          | -Oz -flto=thin -march=native -ffinite-math-only -ffp-contract=fast -fno-omit-frame-pointer -fno-builtin -mno-vzeroupper |
| fftbench    | 1959          | -Oz -flto=thin -march=native -ffp-contract=fast -mno-vzeroupper |
| linbench    | 2188          | -Oz -flto=thin -march=native -ffinite-math-only -mno-vzeroupper |
| evobench    | 2352          | -Oz -flto -march=native -fno-signed-zeros -ffp-contract=off -finline-hint-functions -fno-builtin -mno-vzeroupper |
| treebench   | 4895          | -Oz -flto -march=native -fno-optimize-sibling-calls -mno-vzeroupper |
| huffbench   | 2854          | -Oz -flto=thin -mtune=native |

#### Observations

- `-Oz` + LTO wins for every benchmark; `-flto=thin` now edges full `-flto`
  in most cases
- `-march=native` is adopted almost everywhere this round (only huffbench
  keeps `-mtune=native`) — with the harness's counter code in every binary,
  the `-march` encoding cost is a smaller relative penalty
- `-mno-vzeroupper` is adopted by all eight under the 2026-07-09 ranking
  (saves the `vzeroupper` instruction bytes); `-mllvm -enable-newgvn`
  dropped out of every winner
- `-fprofile-use` was never adopted in size mode

### GCC 16 vs Clang 22 — Size (-s)

Δ is the winner's reduction relative to the other compiler.

| Benchmark | GCC 16 (.text) | Clang 22 (.text) | Winner | Δ |
|-----------|----------------|------------------|--------|---|
| distbench | 1585 | 1649 | GCC | −4% |
| mat1bench | 1560 | 1650 | GCC | −5% |
| almabench | 2970 | 3284 | GCC | −10% |
| fftbench | 1917 | 1959 | GCC | −2% |
| linbench | 2154 | 2188 | GCC | −2% |
| evobench | 2397 | 2352 | Clang | −2% |
| treebench | 5073 | 4895 | Clang | −4% |
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

Use `-s` or `-u` (or the portable `-p`) for reproducible optimization. If
you must use time mode,
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

- **GCC huffbench** remains the widest gap (measured in the `-p` era: quick
  1.00B vs 0.98B for an uncapped audit vs ~0.89B for the former hand-curated
  config). Its best flag set is *specific to that workload* (e.g.
  `-fipa-cp-clone`, `-fsplit-paths`, `-finline-stringops`), so those options
  rank low on the cross-benchmark weights. This is greedy *path-dependence*,
  not coverage — the flags are reachable; greedy simply doesn't take that
  path. It is the clearest price of replacing a hand-curated set with
  data-driven weights — and the kind of case `-l 2` or a non-greedy search
  (TODO.md item 3) would address.

(almabench was the other callout — a greedy local optimum that no `-k`
escaped — until PGO dissolved it; see below.)

**Ranking revisions move greedy paths.** Re-running all 32 searches after
the 2026-07-09 weight regeneration left 13 of 16 speed results within
±0.8% and *improved* 7 of 8 Clang size rows (adopted into the tables), but
some paths flipped the other way: clang/mat1bench's `-mavx2` gather escape
(156.4M ops) is found under the previous ranking but not the current one
(199.2M — same config, same `-k 80`), and most GCC size rows were 2–5%
better under the previous ranking (kept). The published rows are therefore
the best verified result per benchmark; every one reproduces from its
listed flags regardless of which ranking revision found it.

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
  hand-curated result (GCC huffbench, see [Quick vs audit](#quick-vs-audit)).
  The `w_speed` weights themselves were annotated under `-p`; regenerating
  them under `-u` (`scripts/annotate.sh`) would make the quick-mode ranking
  native to the current objective.
- **Path sensitivity.** At the default `-T 0` the marginal `-fno-*` tail is
  noise-level, so the exact flag set varies between runs; totals are stable.

### Reproducibility

- **Size (-s):** 100% reproducible across runs
- **Retired ops (-u) and instructions (-p):** totals reproducible to
  ~100 ppm (`-u`) / ~1 ppm (`-p`) — every `-u` winner above re-verified
  within 0.2% on rebuild; at the default `-T 0` the marginal `-fno-*` picks
  vary run-to-run, but `-T 3` yields a stable flag set. PGO builds are just
  as deterministic (the training run is deterministic, so the profile is
  too). Note `-u` counts AMD Zen's `ex_ret_ops` (raw event `0xC1`) — on
  non-AMD hardware use `-p`, whose numbers are in the audit table.
- **Cycles (-c):** 0.1–7% run-to-run spread even self-pinned (fftbench up to
  ~20%); an audit metric, not a search objective.
- **Time (default):** too noisy on this shared host to optimize on — the
  greedy search even adopts `-Os` for compute-bound FP benchmarks. Use `-s`
  or `-u` instead (see [Time](#time-default)).
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
