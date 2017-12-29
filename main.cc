/** \mainpage osearch - Option Search/Optimization
    Use osearch to find a good set of options for your compiler.
 */

/** \file main.cc */

#include "compile.hh"   // for reset_compilations
#include "getopts.hh"   // for get_opts
#include "measure.hh"   // for reset_measurements
#include "print.hh"     // for progress, progress_t
#include "read_conf.hh" // for read_conf, summary_first_read_conf
#include "search.hh"    // for search, summary_exit, summary_first

#include <bits/getopt_core.h> // for optind

#ifdef __clang__
#include <span> // not yet in GCC
#define STD std // NOLINT(cppcoreguidelines-macro-usage)
#else
#define STD gsl
#endif
#include <gsl/gsl> // for muti_span

#include <cstdlib> // for EXIT_SUCCESS

/** Parse arguments using get_opts(). Read configuration file using
    read_conf(). */

#if 0
// convert from span of zstring to span of czstring
// TODO(uabpath): This kind of conversions is messy.
static STD::span<NONNULL(gsl::czstring<>)> span_not_null_cstring_cast(STD::span<NONNULL(gsl::zstring<>)> args) {
  auto argc = args.size();
  auto argv = args.data();
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto argv2 = reinterpret_cast<NONNULL(gsl::czstring<>) *>(argv);
  auto args2 = gsl::multi_span<NONNULL(gsl::czstring<>)>(argv2, argc);
  return args2;
}
#endif

// convert main 'argc', 'argv' to a single 'args' not_null zstring span
static STD::span<NONNULL(gsl::zstring<>)> main_args(int argc, char **argv) {

  // Say that argv contians non-null (in this case const) c-strings.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto argv2 = reinterpret_cast<NONNULL(gsl::zstring<>) *>(argv);

  auto args = gsl::multi_span<NONNULL(gsl::zstring<>)>(argv2, argc);
  return args;
}

static int my_main(STD::span<NONNULL(gsl::zstring<>)> args) {

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
