/** \mainpage osearch - Option Search/Optimization
    Use osearch to find a good set of options for your compiler.
 */

/// \file
/// For the main() function.
///
/// zstring/czstring are provided by GSL
/// http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#SS-views.

#include "compile.hh"   // for reset_compilations
#include "getopts.hh"   // for get_opts, USE_CZSTRING
#include "measure.hh"   // for reset_measurements
#include "print.hh"     // for progress, progress_t
#include "read_conf.hh" // for read_conf, summary_first_read_conf
#include "search.hh"    // for search, summary_exit, summary_first

#include <bits/getopt_core.h> // for optind

#include <gsl/gsl> // for zstring/czstring
#include <span>

#include <cstdlib> // for EXIT_SUCCESS

#ifdef USE_CZSTRING
/// Convert from span of zstring to span of czstring.
/// (Just try to add more const.)
static std::span<NONNULL(gsl::czstring)>
span_not_null_cstring_cast(std::span<NONNULL(gsl::zstring)> args) {
  auto argc = args.size();
  auto *argv = args.data();
#ifdef __clang__
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  auto *argv2 = const_cast<NONNULL(gsl::czstring) *>(argv);
#else
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto *argv2 = reinterpret_cast<NONNULL(gsl::czstring) *>(argv);
#endif
  auto args2 = std::span<NONNULL(gsl::czstring)>(argv2, argc);
  return args2;
}
#endif

/// Convert main 'argc', 'argv' to a single 'args' not_null czstring span.
static std::span<NONNULL(argv_czstring)> main_args(int argc, char **argv) {

  // Say that argv contians non-null (in this case const) c-strings.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto *argv2 = reinterpret_cast<NONNULL(gsl::zstring) *>(argv);
  auto args = std::span<NONNULL(gsl::zstring)>(argv2, argc);
#ifdef USE_CZSTRING
  auto args2 = span_not_null_cstring_cast(args);
#else
  auto args2 = args;
#endif
  return args2;
}

/// Parse arguments using get_opts(). Read configuration file using read_conf().

/// Wrapper for C++ main, to pass argc and argv as a single list of strings.
static int my_main(std::span<const NONNULL(argv_czstring)> args) {

  get_opts(args);

  read_conf();

  progress.print_symbols();

  for (; optind < static_cast<typeof(optind)>(args.size()); ++optind) {

    input_file = args[optind];

    {
      reset_compilations();
      reset_measurements();

      summary_first_read_conf();
      summary_first();

      // main loop
      search();

      progress.silent = true;
      summary_exit();
      progress.silent = false;
    }
  }

  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {

  auto args = main_args(argc, argv);

  return my_main(args);
}
