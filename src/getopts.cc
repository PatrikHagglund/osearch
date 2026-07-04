#include "getopts.hh"
#include "compat.hh" // NONNULL
#ifdef DEBUG
#include "print.hh"
#endif

#include <gsl/gsl> // czstring, not_null

#include <iostream> // std::cerr

#include <cstdlib>  // _Exit
#include <string>   // std::string
#include <utility>
#include <unistd.h> // getopt

/// \file
/// Register (opt_reg_t) and process (get_opts()) command line options.

/**
 *  Options processing. Use the opt_reg_t constructor
 *  to register an option. Use get_opts() to parse all options.
 */

[[noreturn]] static void usage(std::string_view prog) {
  std::cerr << "usage: " << prog << opt_reg_t::opt_strs.usage_short
            << " code_file\n\n"
               "  Search for an optimal combination of compiler options.\n\n"
               "  The 'code_file' is the source code to be compiled.\n\n"
            << opt_reg_t::opt_strs.usage_long;
  _Exit(EXIT_FAILURE);
}

/// Wrapper for getopt(), hiding casts.
static int my_getopt(std::span<const NONNULL(argv_czstring)> args,
                     NONNULL(gsl::czstring) optstring) {

  auto argc = static_cast<int>(args.size());
#if !defined(__clang__) || !defined(USE_CZSTRING)
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto *argv = reinterpret_cast<char *const *>(args.data());
#else
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  const auto *argv = const_cast<char *const *>(args.data());
#endif
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  return getopt(argc, argv, optstring);
}

/** Parse all options regsitred with opt_reg_t. */

void get_opts(std::span<const NONNULL(argv_czstring)> args) {
#ifdef DEBUG
  o3 << "opt_strs\n";
  o3 << opt_reg_t::opt_strs.getopt_str << '\n';
  o3 << opt_reg_t::opt_strs.usage_short << '\n';
  o3 << opt_reg_t::opt_strs.usage_long << '\n';
#endif
  NONNULL(gsl::czstring) optstring(opt_reg_t::opt_strs.getopt_str.c_str());
  for (int c; (c = my_getopt(args, optstring)) != -1;) {
    if (opt_reg_t::opt_strs.opt_funs[c] != nullptr) {
      (*opt_reg_t::opt_strs.opt_funs[c])();
    } else {
      usage(std::string_view(args[0]));
    }
  }

  for (unsigned i = 0;
       i < opt_strs_t::arg_max && opt_reg_t::opt_strs.arg_funs[i] != nullptr;
       ++i) {
    (*opt_reg_t::opt_strs.arg_funs[i])(args[optind++]);
  }

  if (std::cmp_less(args.size(), optind)) {
    usage(std::string_view(args[0]));
  }
}
