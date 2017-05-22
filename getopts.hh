#ifndef GETOPTS_HH
#define GETOPTS_HH

#include <unistd.h> // optind, optarg

void get_opts(int& argc, char**& argv);

using opt_fun_t = void (*)();
using arg_fun_t = void (*)(const char *);

struct opt_reg_t {
  opt_reg_t(unsigned char c, opt_fun_t fun, char const *go_str,
	    char const *u_syn, char const *u_full);
  opt_reg_t(unsigned ind, arg_fun_t fun,
	    char const *u_syn, char const *u_full);
};

#endif
