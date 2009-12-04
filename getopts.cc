#include "getopts.hh"

#include <unistd.h>
#include <string>
using std::string;
#include <cstdlib>
#include <climits>
#include <cassert>

/** \file getopts.cc Options processing. Use the opt_reg_t constructor
 *  to register an option. Use get_opts() to parse all options. */

static const unsigned arg_max = 10;

struct opt_strs_t {
  opt_fun_t opt_funs[UCHAR_MAX+1];
  string getopt_str;
  string usage_short;
  string usage_long;
  arg_fun_t arg_funs[arg_max];
};

static opt_strs_t *p = NULL;

// *p has to be initialized (to hold empty strings) before it is
// used. Do it dynamically, to make sure that happens.
static void init_p() {
  if (p == NULL) {
    static opt_strs_t opt_strs;
    p = &opt_strs;
  }
}

/** Register options to process later with get_opts()
 * \param c option character returned by getopt()
 * \param fun function (procedure) to handle the option
 * \param go_str string fragment used in getopt()
 * \param u_syn string fragment used in the usage synopsis
 * \param u_full string fragment used in the full usage description */

opt_reg_t::opt_reg_t(unsigned char c, opt_fun_t fun, char const *go_str,
		     char const *u_syn, char const *u_full) {
  init_p();
  p->opt_funs[c] = fun;
  p->getopt_str += go_str;
  p->usage_short += u_syn;
  p->usage_long += u_full;
}

opt_reg_t::opt_reg_t(unsigned ind, arg_fun_t fun,
		     char const *u_syn, char const *u_full) {
  init_p();
  assert(ind < arg_max);
  p->arg_funs[ind] = fun;
  p->usage_short += u_syn;
  p->usage_long += u_full;
}

static void usage(char* prog) {
  fprintf(stderr, "usage: %s%s code_file\n\n"
	  "  Search for an optimal combination of compiler options.\n\n"
	  "  The 'code_file' is the source code to be compiled.\n\n"
	  "%s"
	  , prog, p->usage_short.c_str(), p->usage_long.c_str());
  exit(EXIT_FAILURE);
}

/** Parse all options regsitred with opt_reg_t. */

void get_opts(int& argc, char**& argv) {

  for (int c; (c = getopt(argc, argv, p->getopt_str.c_str())) != -1;)
    if (p->opt_funs[c] != NULL)
      (*p->opt_funs[c])();
    else
      usage(argv[0]);

  for (unsigned i = 0; i < arg_max && p->arg_funs[i] != NULL; ++i) {
    (*p->arg_funs[i])(argv[optind++]);
  }

  if (argc - optind < 0) {
    usage(argv[0]);
  }
}
