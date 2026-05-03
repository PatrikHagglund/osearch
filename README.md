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
  -l max_level  max number of options to alter at once (default 1)
  -Q n          quick mode: sample at most 'n' combinations per level
  -q            suppress progress output
  -j            output results as JSON
```

Example:

```sh
./build/osearch config/gcc16-test.osearch benchmarks/fftbench.c
```

## Configuration

XML profiles in `config/` define available flags for different compilers:

| Config | Description |
|--------|-------------|
| `gcc16-test.osearch` | GCC 16, key optimization flags (60 flags) |
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

- Full GCC 16 config with all 280 optimization flags
- Add a clang/LLVM config file
- Priority-based flag exploration: evaluate flags in config-file order
  (or by explicit `priority` attribute) instead of randomly, so the
  most-likely-useful flags are tested first
- Cross-benchmark aggregation script: run osearch `-j` on all benchmarks,
  rank flags by average improvement, and reorder the config file accordingly
- Use C++26 reflection (`-freflection`) for JSON serialization and CLI option registration once compiler support matures
