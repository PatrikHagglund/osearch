#ifndef GETOPTS_HH
#define GETOPTS_HH

#include "compat.hh" // NONNULL
#ifdef DEBUG
#include "print.hh"
#endif

#include <unistd.h> // optind, optarg

#include <gsl/gsl> // czstring, not_null

#include <span>
#include <string_view> // string_view
#include <climits> // UCHAR_MAX

/// \file
/// Register (opt_reg_t) and process (get_opts()) command line options.

/// Define for handling 'argv' as an array of *const* strings. See main_args().
#define USE_CZSTRING
#ifdef USE_CZSTRING
/// See USE_CZSTRING
using argv_czstring = gsl::czstring;
#else
/// See USE_CZSTRING
using argv_czstring = gsl::zstring<>;
#endif

/// Parse command options. Accumulated in different source files with opt_reg_t::append().
void get_opts(std::span<const NONNULL(argv_czstring)> args);

using opt_fun0_t = void (*)();
using arg_fun0_t = void (*)(NONNULL(gsl::czstring));
#ifdef __clang__
// _Nonnull doesn't seems to work if not done in two steps.
using opt_fun_t = NONNULL(opt_fun0_t);
using arg_fun_t = NONNULL(arg_fun0_t);
#else
// can't use 'brace-enclosed initializer list' with gsl::not_null
using opt_fun_t = opt_fun0_t;
using arg_fun_t = arg_fun0_t;
#endif

/// Data for command line options.
struct opt_strs_t {
  /// See opt_reg_t / fun.
  opt_fun_t opt_funs[UCHAR_MAX + 1]{};
  /// See opt_reg_t / go_str.
  std::string getopt_str;
  /// See opt_reg_t / u_syn.
  std::string usage_short;
  /// See opt_reg_t / u_full.
  std::string usage_long;
  /// See opt_reg_t / fun.
  static constexpr const unsigned arg_max = 10;
  /// See opt_reg_t / fun.
  arg_fun_t arg_funs[arg_max]{};
};

/** Register options to process later with get_opts()
 * \param c option character returned by getopt()
 * \param fun function (procedure) to handle the option
 * \param go_str string fragment used in getopt()
 * \param u_syn string fragment used in the usage synopsis
 * \param u_full string fragment used in the full usage description */

struct opt_reg_t {
  // has to be initialized (to hold empty strings) before it is used
  static inline opt_strs_t opt_strs;
  static void append(unsigned char c, opt_fun_t fun, NONNULL(gsl::czstring) go_str,
		     NONNULL(gsl::czstring) u_syn, NONNULL(gsl::czstring) u_full) noexcept {
#ifdef DEBUG
  o3 << "opt_reg_t 1\n";
  o3 << go_str << '\n';
  o3 << u_syn << '\n';
  o3 << u_full << '\n';
#endif
    opt_strs.opt_funs[c] = fun;
    opt_strs.getopt_str += go_str;
    opt_strs.usage_short += u_syn;
    opt_strs.usage_long += u_full;
  }
  static void append(unsigned ind, arg_fun_t fun, NONNULL(gsl::czstring) u_syn,
		     NONNULL(gsl::czstring) u_full) noexcept {
#ifdef DEBUG
  o3 << "opt_reg_t 2\n";
  o3 << u_syn << '\n';
  o3 << u_full << '\n';
#endif
  contract_assert(ind < opt_strs_t::arg_max);
  opt_strs.arg_funs[ind] = fun;
  opt_strs.usage_short += u_syn;
  opt_strs.usage_long += u_full;
  }
};

#endif
