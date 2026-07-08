# osearch — Option Search/Optimization

[![CI](https://github.com/PatrikHagglund/osearch/actions/workflows/ci.yml/badge.svg)](https://github.com/PatrikHagglund/osearch/actions/workflows/ci.yml)

A C++ tool that searches for optimal compiler optimization flags by compiling benchmark programs with different flag combinations and measuring their performance. It uses heuristic search over the compiler flag space to find the best-performing set of options.

Rewrite of [ACOVEA](https://web.archive.org/web/20071230224418/http://www.coyotegulch.com/products/acovea/) (Analysis of Compiler Options via Evolutionary Algorithm).

## Building

Requires GCC 16+ (C++26), [GSL](https://github.com/microsoft/GSL), and [libexpat](https://libexpat.github.io/).

```sh
git clone https://github.com/microsoft/GSL.git ~/GSL
cmake -G Ninja -B build
ninja -C build
```

## Usage

```sh
./build/osearch [options] config_file code_file

  -i in_opts    extra options for compiling 'code_file'
  -s            optimize for binary size instead of execution time
  -p            optimize for retired instruction count (counted in-harness
                via perf_event_open around run(); deterministic, requires
                Linux perf_event support, perf_event_paranoid <= 2)
  -u            optimize for retired-op count (AMD Zen ex_ret_ops).
                Near-deterministic like -p, but prices microcoded
                instructions (gathers, wide shuffles) honestly where an
                instruction count sees 1
  -c            optimize for core cycles (run self-pins to its CPU).
                Closest to real time but noisy on a shared host: sampled
                -n times taking the minimum; raise -n and -T accordingly
  -l max_level  max number of options to alter at once (default 1)
  -k n          quick mode: restrict the search to the 'n' highest-ranked
                options for the active objective (0 = all, default). Ranking
                comes from the config's per-flag effectiveness weights.
  -Q n          quick mode: sample at most 'n' combinations per level
  -n samples    number of samples per measurement (take minimum, default 3)
  -T permille   adoption threshold in 1/1000 units; only adopt a flag if
                improvement >= threshold (default 0, strict <)
  -q            suppress progress output
  -j            output results as JSON
```

Example:

```sh
./build/osearch config/gcc16.osearch benchmarks/fftbench.c
```

## Configuration

XML profiles in `config/` define available flags for different compilers. Each
modern profile is a single annotated file serving both objectives: options
carry per-flag effectiveness weights (`w_speed` / `w_size`) that order and bias
the search. A full run audits every option; `-k n` (or `-Q n`) restricts to the
top-ranked options for a quick run — no separate "test" config needed. See
[`annotate.sh`](scripts/annotate.sh) for regenerating the weights from measured data.

| Config | Description |
|--------|-------------|
| `gcc16.osearch` | GCC 16, full annotated flag set (227 flags) |
| `clang22.osearch` | Clang/LLVM 22, full annotated flag set + LLVM pass control (107 flags) |

Both configs route compiles through
[`scripts/cc-pgo.sh`](scripts/cc-pgo.sh) (run osearch from the repo root): a
plain passthrough, except that the pseudo-flag `-fprofile-use` triggers a full
profile-guided-optimization build — instrument, one training run on the
benchmark itself, recompile with the profile — so the search can adopt or
reject PGO per benchmark like any other option (see TODO.md item 1 for the
A/B measurements). PGO candidates cost two compiles plus a training run.

## Benchmarks

The `benchmarks/` directory contains C programs used as test workloads:

- FFT, Huffman coding, tree operations, linear algebra, evolutionary algorithm, astronomical calculations, and more.

Each benchmark implements four symbols consumed by the shared harness `main.ic`:

```c
char const* name;   // benchmark name
void init();        // allocate/setup
void run();         // the timed workload
void clean();       // teardown
```

`main.ic` provides `main()`, which calls `init()`, measures `run()`, calls
`clean()`, and prints the measured value to stdout. What is measured is
selected by an option: elapsed microseconds via `CLOCK_THREAD_CPUTIME_ID`
(default), retired instructions (`-p`), retired ops (`-u`, AMD Zen), core
cycles (`-c`, self-pinned), or all three counters from one atomic group read
(`-g`, tab-separated — used for audits/reporting rather than as a search
objective). Pass `-v` for a labeled line plus the benchmark's output checksum
on stderr.

To build benchmarks standalone (with their own `main`):

```sh
make -C benchmarks
```

To build with `LINK` mode (compiles `main.ic` together with the benchmark
via `-DLINK -x c main.ic`):

```sh
make -C benchmarks all2
```

osearch uses the standalone mode: it compiles the benchmark source with
candidate flags, runs the resulting binary, and parses the printed
microsecond value as the objective.

## Results

See [RESULTS.md](RESULTS.md) for measured retired-op (`-u`, the primary
speed objective) and code-size (`-s`) results per benchmark on GCC 16 and
Clang 22, the head-to-head compiler comparisons (including an
instructions/ops/cycles audit of every winner), and the search-methodology
caveats.

## Components

| File | Purpose |
|------|---------|
| `main.cc` | Entry point; parses args, reads config, runs search loop |
| `search.cc` | Core search algorithm |
| `compile.cc` | Compiles benchmarks with candidate flag sets |
| `execute.cc` | Runs compiled benchmarks |
| `measure.cc` | Measures execution performance |
| `point.cc` | Represents a point in the optimization space (a set of flags) |
| `read_conf.cc` | Reads XML configuration files (libexpat) |
| `steps.cc` | Manages search steps/iterations |
| `obj.cc` | Objective function evaluation |
| `getopts.cc` | Command-line argument parsing |
| `print.cc` | Progress/output reporting |

### Parallel compilation

The search loop batches up to `hardware_concurrency` steps and compiles
their candidate points in parallel using `std::jthread` (see
`compile_batch()` in `compile.cc`). Measurement (execution + timing)
remains sequential to avoid interference. On a 16-core machine with the
60-flag test config this gives ~1.6× wall-clock speedup.

## Testing

Two layers:

- **Unit tests** (`tests/`) — small, fast, no external processes. Registered
  with CTest via CMake.
- **Integration smoke tests** (`scripts/test.sh`) — end-to-end search runs on real
  benchmarks.

Unit tests combine two complementary styles:

- **Compile-time tests** using `static_assert` for anything `constexpr`
  (e.g. `obj_t` comparison and arithmetic in `tests/test_obj_static.cc`).
  A "failure" is a build error; the runtime `main()` is a no-op.
- **Runtime tests** using a tiny `CHECK` / `CHECK_EQ` macro defined in
  `tests/check.hh` for things that cannot be evaluated at compile time
  (e.g. `point_t::popcnt()` and `to_string()` in `tests/test_point.cc`,
  `delta_t::alt_diff()` and `steps_t::store()` sorting in
  `tests/test_steps.cc`).
  Each test file is its own executable; `main()` returns non-zero on
  failure and CTest records pass/fail from the exit status.

No external dependency is added — `tests/check.hh` is ~60 lines of plain
C++ and reuses the main target's compile flags.

Run:

```sh
./scripts/test.sh --fast    # ctest + DEBUG syntax check (~seconds)
./scripts/test.sh           # fast layer + full integration search runs (minutes)
ctest --test-dir build --output-on-failure   # unit tests only
```

Add a new unit test by dropping `tests/test_foo.cc` next to the existing
ones and registering it in `CMakeLists.txt`:

```cmake
osearch_add_test(test_foo tests/test_foo.cc)
```


## Design choices

The codebase is built with `-fno-exceptions -fno-rtti -fno-unwind-tables`
to enforce zero-overhead C++ discipline:

- **Contracts** (C++26 `pre`/`post`/`contract_assert`) replace assertions
  and document function invariants. A custom `handle_contract_violation`
  handler in `contract.cc` aborts with a diagnostic on failure.
- **No virtual dispatch** — flag types use `std::variant` + `std::visit`
  instead of inheritance.
- **No exceptions** — errors are handled via return values or `_Exit()`.
- **Value semantics** — `point_t` uses `std::inplace_vector` (trivially
  copyable), config flags are stored by value in a `std::vector<variant>`.
- **Flat maps** — associative containers use a sorted `std::vector` of
  pairs (`flat_map.hh`) instead of `std::map` for cache-friendliness.

## Dependencies

- GCC 16+ (uses C++26: `std::inplace_vector`, contracts, ranges)
- [GSL](https://github.com/microsoft/GSL) (Guidelines Support Library)
- [libexpat](https://libexpat.github.io/) (XML parsing)
- CMake 3.22+ and Ninja (build system)

## TODO

Open work items live in [TODO.md](TODO.md).

## Search methodology

### Effectiveness-ranked options

Each compiler has a **single** annotated config (`gcc16.osearch`,
`clang22.osearch`) serving both speed and size. Every option carries a
per-objective effectiveness weight:

    <flag type="simple" value="-funroll-loops" w_speed="9" w_size="-3" />

(positive = usually helps that objective, negative = usually hurts, 0 =
neutral/unknown). The search orders each level's candidates by the *active*
objective's weight (`size` under `-s`, otherwise `speed`) before any cap, so one
file is well-ordered for both objectives at once. The former curated `*-test`
configs are gone: a quick run is `-k n` (top-`n` ranked options) on the full
config, an audit is the same file with no cap.

**Why it fits the search.** At `-l 1`/`-l 2` the search explores options in the
ranked order (`steps.cc`), and `-Q n` caps each level to the first `n` — so the
weights make `-Q n` search the top-`n` options, and `-k n` does the same while
composing cleanly with `-l 2` (and a future `-r`), where `-Q` caps *pairs*, not
options. At `-T 0` the order also steers plain greedy adoption.

**Keep it soft.** Weights only *order* and *bias* — never exclude. Mean
effectiveness is a prior; per-benchmark it varies (`-march=native` helps FP,
hurts integer size), so an uncapped run still reaches every option to find
cross-objective surprises.

**Populate from data.** [`annotate.sh`](scripts/annotate.sh) measures each option's
per-objective marginal effect (uncapped `-l 1`, size and perf modes) and writes
the weights back into the config, so they are regenerated rather than
hand-curated and refreshed as compilers change. `enum` options (the `-O` level,
cost models, …) are structural and always ranked top.

### Noise-robust search

The greedy adoption strategy amplifies measurement noise: a flag that
wins by luck sends the search down a wrong branch. Plan for addressing
this, in order:

1. **Measurement cache** — already in place (`results` map in `measure.cc`
   keyed by `pset_t`, the output-binary id). Two different flag sets
   that compile to the same binary share a measurement.

2. **Adoption threshold** — `-T permille` option (units of 0.1%, so
   `-T 3` = 0.3%). Only adopts a flag if improvement ≥ threshold.
   With perf mode (`-p`), `-T 3` is the minimum that gives 100%
   reproducible flag sets across runs on tested benchmarks (fft, lin,
   tree, mat1). Defaults: 0 (strict `<`). Suggested:
   `-T 3` with `-p`, `-T 20` (2%) with time.

3. **Post-search validation pass** — after the main search, try
   removing each adopted flag against the final baseline; drop any
   whose removal doesn't hurt beyond the threshold. Catches
   noise-driven false positives and flags that became redundant
   after later adoptions. Always on; uses the same `-T` threshold.

### Time-mode reproducibility

Wall-clock time measurements are inherently noisy on shared/general-
purpose hardware (laptops, VMs, containers) — CPU frequency scaling,
thermal throttling, preemption, and memory traffic from other
processes all inject variance.

On the tested system the raw variance of a single compiled binary is
~30% between runs (minimum ~52ms, maximum ~78ms for a fixed `-Ofast`
build of fftbench). With that much noise, **no combination of `-n` and
`-T` yields 100% reproducible flag sets** — the greedy adoption path
diverges even at `-T 500` (50%), because single-flag "wins" driven by
momentary slow baseline measurements can't be distinguished from real
improvements.

Practical guidance:

- Prefer `-u` (retired ops) as the speed objective — effectively
  deterministic, and it prices microcoded instructions (gathers, wide
  shuffles) honestly where an instruction count sees 1. This changes
  search *outcomes*, not just scores: under `-u` the search found a
  clang/mat1bench binary ~9% faster in cycles that `-p` structurally
  could not choose (it retires 3× the instructions but avoids
  `vgatherqpd`; see TODO.md item 2 and RESULTS.md).
- `-p` (retired instructions) is the portable fallback: `-u` counts AMD
  Zen's `ex_ret_ops` raw event, `-p` works on any CPU with perf support.
- Prefer `-s` (size) when optimizing for binary size.
- `-c` (cycles) is the closest to real time and much better behaved
  than wall-clock (the run self-pins, counts only own-thread user-space
  cycles, and `-n` takes the minimum of one-sided noise), but still not
  deterministic — CV 0.1–9% per binary on a shared host, worst for
  memory-bound workloads. Use it to audit `-p`/`-u` results rather than
  as the primary objective, with raised `-n` and `-T`.
- Use time mode as a sanity check only. `-n 5 -T 20` reduces variance
  but does not eliminate it. Pinning (`taskset`), disabling turbo, and
  setting the CPU governor to `performance` help but don't fully fix
  the issue on typical hardware.

### Perf speed

Hardware counting is done in-harness: `main.ic` opens a
`perf_event_open` counter (user-space only) and brackets exactly
`run()` with `RESET`/`ENABLE`/`DISABLE`, then prints the count just
like the time value. `-p` counts `PERF_COUNT_HW_INSTRUCTIONS`, `-u`
the AMD Zen raw event `0xC1` (`ex_ret_ops`), `-c`
`PERF_COUNT_HW_CPU_CYCLES` (self-pinned), and the harness-only `-g`
reads all three atomically as one group.

Compared to spawning `perf stat <binary>` per sample, this:

- Removes the ~50ms fork+exec startup of the `perf` tool per sample.
- Excludes process startup, the dynamic loader, `init()`, `clean()`,
  and exit from the count — for short benchmarks these fixed
  instructions otherwise dominate and dilute the signal, exactly as
  the timer already excludes them in time mode.
