#ifndef GETOPTS_HH
#define GETOPTS_HH

#include "assume.hh" // GNUC_BUILTIN_ASSUME, NONNULL
#ifdef DEBUG
#include "print.hh"
#endif

#include <unistd.h> // optind, optarg

#include <gsl/gsl> // span, czstring, not_null

#ifdef __clang__
#include <span> // not yet in GCC
#define STD std
#else
#define STD gsl
#endif

#include <string_view> // string_view
#include <climits> // UCHAR_MAX

void get_opts(STD::span<NONNULL(gsl::zstring<>)> args);

using opt_fun0_t = void (*)();
using arg_fun0_t = void (*)(NONNULL(gsl::czstring<>));
#ifdef __clang__
// _Nonnull doesn't seems to work if not done in two steps.
using opt_fun_t = NONNULL(opt_fun0_t);
using arg_fun_t = NONNULL(arg_fun0_t);
#else
// can't use 'brace-enclosed initializer list' with gsl::not_null
using opt_fun_t = opt_fun0_t;
using arg_fun_t = arg_fun0_t;
#endif

struct opt_strs_t {
  opt_fun_t opt_funs[UCHAR_MAX + 1]{};
  std::string getopt_str;
  std::string usage_short;
  std::string usage_long;
  static constexpr const unsigned arg_max = 10;
  arg_fun_t arg_funs[arg_max]{};
};

/** Register options to process later w
ith get_opts()
 * \param c option character returned by getopt()
 * \param fun function (procedure) to handle the option
 * \param go_str string fragment used in getopt()
 * \param u_syn string fragment used in the usage synopsis
 * \param u_full string fragment used in the full usage description */

struct opt_reg_t {
  // has to be initialized (to hold empty strings) before it is used
  static inline opt_strs_t opt_strs;
  static void append(unsigned char c, opt_fun_t fun, NONNULL_IMPLICIT(gsl::czstring<>) go_str,
		     NONNULL_IMPLICIT(gsl::czstring<>) u_syn, NONNULL_IMPLICIT(gsl::czstring<>) u_full) noexcept {
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
  static void append(unsigned ind, arg_fun_t fun, NONNULL_IMPLICIT(gsl::czstring<>) u_syn,
		     NONNULL_IMPLICIT(gsl::czstring<>) u_full) noexcept {
#ifdef DEBUG
  o3 << "opt_reg_t 2\n";
  o3 << u_syn << '\n';
  o3 << u_full << '\n';
#endif
  GNUC_BUILTIN_ASSUME(ind < opt_strs_t::arg_max);
  opt_strs.arg_funs[ind] = fun;
  opt_strs.usage_short += u_syn;
  opt_strs.usage_long += u_full;
  }
};

#endif
