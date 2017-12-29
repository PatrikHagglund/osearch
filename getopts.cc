#include "getopts.hh"
#include "assume.hh" // NONNULL
#ifdef DEBUG
#include "print.hh"
#endif

#include <gsl/gsl> // czstring, not_null

#include <iostream> // std::cerr

#include <cstdlib>
#include <string>
#include <unistd.h>

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
  exit(EXIT_FAILURE);
}

/** Parse all options regsitred with opt_reg_t. */

// wrapper for getopt
static int my_getopt(STD::span<NONNULL(gsl::zstring<>)> args,
                     NONNULL(gsl::czstring<>) optstring) {

  int argc = args.size();
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  auto argv = reinterpret_cast<char *const *>(args.data());
  return getopt(argc, argv, optstring);
}

void get_opts(STD::span<NONNULL(gsl::zstring<>)> args) {
#ifdef DEBUG
  o3 << "opt_strs\n";
  o3 << opt_reg_t::opt_strs.getopt_str << '\n';
  o3 << opt_reg_t::opt_strs.usage_short << '\n';
  o3 << opt_reg_t::opt_strs.usage_long << '\n';
#endif
  NONNULL(gsl::czstring<>) optstring(opt_reg_t::opt_strs.getopt_str.c_str());
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

  if (args.size() - optind < 0) {
    usage(std::string_view(args[0]));
  }
}
