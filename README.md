# osearch — Option Search/Optimization

[![CI](https://github.com/PatrikHagglund/osearch/actions/workflows/ci.yml/badge.svg)](https://github.com/PatrikHagglund/osearch/actions/workflows/ci.yml)

A C++ tool that searches for optimal compiler optimization flags by compiling benchmark programs with different flag combinations and measuring their performance. It uses heuristic search over the compiler flag space to find the best-performing set of options.

Successor/rewrite of [ACOVEA](https://web.archive.org/web/20071230224418/http://www.coyotegulch.com/products/acovea/) (Analysis of Compiler Options via Evolutionary Algorithm).

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
[`annotate.sh`](annotate.sh) for regenerating the weights from measured data.

| Config | Description |
|--------|-------------|
| `gcc16.osearch` | GCC 16, full annotated flag set (226 flags) |
| `clang22.osearch` | Clang/LLVM 22, full annotated flag set + LLVM pass control (105 flags) |
| `gcc48-test.osearch` | GCC 4.8, small test set |
| `gcc48.osearch` | GCC 4.8, full flag set |
| `gcc43_full.osearch` | GCC 4.3, full flag set |

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

`main.ic` provides `main()`, which calls `init()`, times `run()` using
`CLOCK_THREAD_CPUTIME_ID`, calls `clean()`, and prints the elapsed
microseconds to stdout. Pass `-v` for a labeled line.

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
- **Integration smoke tests** (`test.sh`) — end-to-end search runs on real
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
./test.sh --fast    # ctest + DEBUG syntax check (~seconds)
./test.sh           # fast layer + full integration search runs (minutes)
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

- Use C++26 reflection (`-freflection`) for JSON serialization and CLI option registration once compiler support matures (blocked on compiler support)
- Deeper search to escape greedy local optima: a non-greedy search (simulated annealing / genetic, as in the ACOVEA ancestor). The `-l 1` greedy can't reach optima needing a coordinated multi-flag move — e.g. almabench `-p`, where `-flto` only helps in a specific co-set (see RESULTS.md "Search limitations"). The ranked-restart plan below is the lighter near-term mitigation

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

**Populate from data.** [`annotate.sh`](annotate.sh) measures each option's
per-objective marginal effect (uncapped `-l 1`, size and perf modes) and writes
the weights back into the config, so they are regenerated rather than
hand-curated and refreshed as compilers change. `enum` options (the `-O` level,
cost models, …) are structural and always ranked top.

**Remaining:** restart seeding — bias a future `random_point()` so an option's
on-probability rises with its weight, making `-r` restarts begin near promising
regions (the proper multi-restart escape for greedy local optima; pure-random
restart proved impractically slow).

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

- Prefer `-p` (retired instructions) for reproducible optimization.
  Same optimization target for most CPU-bound code, fully deterministic.
- Prefer `-s` (size) when optimizing for binary size.
- Use time mode as a sanity check only. `-n 5 -T 20` reduces variance
  but does not eliminate it. Pinning (`taskset`), disabling turbo, and
  setting the CPU governor to `performance` help but don't fully fix
  the issue on typical hardware.

### Perf speed

Instruction counting is done in-harness: `main.ic` opens a
`perf_event_open` counter for `PERF_COUNT_HW_INSTRUCTIONS` (user-space
only) and brackets exactly `run()` with `RESET`/`ENABLE`/`DISABLE`,
then prints the count just like the time value. This is what `-p`
parses.

Compared to spawning `perf stat <binary>` per sample, this:

- Removes the ~50ms fork+exec startup of the `perf` tool per sample.
- Excludes process startup, the dynamic loader, `init()`, `clean()`,
  and exit from the count — for short benchmarks these fixed
  instructions otherwise dominate and dilute the signal, exactly as
  the timer already excludes them in time mode.

Still open to investigate:
- Sample multiple flag sets in parallel (extend `compile_batch()` to
  cover measurement, but beware contention between measurements).
