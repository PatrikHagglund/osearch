# osearch — Option Search/Optimization

A C++ tool that searches for optimal compiler optimization flags by compiling benchmark programs with different flag combinations and measuring their performance. It uses heuristic search over the compiler flag space to find the best-performing set of options.

Successor/rewrite of [ACOVEA](http://www.coyotegulch.com/products/acovea/) (Analysis of Compiler Options via Evolutionary Algorithm).

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

## Configuration

XML profiles in `config/` define available flags for different compilers and architectures:

- GCC 4.3, 4.8
- LLVM-GCC 4.0
- Architectures: Core2, Athlon XP

## Benchmarks

The `benchmarks/` directory contains C programs used as test workloads:

- FFT, Huffman coding, tree operations, linear algebra, evolutionary algorithm, astronomical calculations, and more.

## Dependencies

- [GSL](https://github.com/microsoft/GSL) (Guidelines Support Library)
- [libexpat](https://libexpat.github.io/) (XML parsing)

## Building

```sh
cmake -G Ninja -H. -Bbuild
ninja -C build
```

Adjust `CMakeLists.txt` if your GSL installation path differs from the default.
