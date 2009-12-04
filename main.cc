/** \mainpage osearch - Option Search/Optimization
    Use osearch to find a good set of options for your compiler.
 */

/** \file main.cc */

#include "search.hh" // search
#include "measure.hh" // reset_measurements
#include "compile.hh" // input_file, reset_compilations
#include "read_conf.hh" // read_conf
#include "getopts.hh" // get_opts
#include "print.hh"

#include <cstdlib>

/** Parse arguments using get_opts(). Read configuration file using
    read_conf(). */

int main(int argc, char **argv) {

  get_opts(argc, argv);

  read_conf();

  progress.print_symbols();

  for (; optind < argc; ++optind) {

    input_file = argv[optind];

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
