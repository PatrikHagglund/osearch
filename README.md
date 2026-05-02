# osearch — Option Search/Optimization

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
  -q            suppress progress output
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

## Testing

```sh
./test.sh
```

## Dependencies

- GCC 16+ (uses C++26: `std::inplace_vector`, contracts, ranges)
- [GSL](https://github.com/microsoft/GSL) (Guidelines Support Library)
- [libexpat](https://libexpat.github.io/) (XML parsing)
- CMake 3.22+ and Ninja (build system)

## TODO

- Run with `-l 2` to search flag pairs (may find synergies)
- Full GCC 16 config with all 280 optimization flags
- Add a clang/LLVM config file
- Parallelize compilations (per-step is embarrassingly parallel)
- Output results as JSON/CSV for analysis
- Add a `--quick` mode that tests fewer flags
- Add unit tests (at minimum for obj_t, point_t, and measure logic)
- Add CI configuration (GitHub Actions or similar)
- Verify zero-overhead code generation for embedded use (see EMBEDDED-CHECK.md)
