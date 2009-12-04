#ifndef _GETOPTS_HH
#define _GETOPTS_HH

void get_opts(int& argc, char**& argv);

typedef void (*opt_fun_t)();
typedef void (*arg_fun_t)(char const *);

struct opt_reg_t {
  opt_reg_t(unsigned char c, opt_fun_t fun, char const *go_str,
	    char const *u_syn, char const *u_full);
  opt_reg_t(unsigned ind, arg_fun_t fun,
	    char const *u_syn, char const *u_full);
};

extern "C" {
  extern int optind;
  extern char *optarg;
}

#endif
