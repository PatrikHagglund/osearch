#include "measure.hh"
#include "assume.hh"

#include "compile.hh"
#include "execute.hh"
#include "getopts.hh" // opt_init_t
#include "point.hh"
#include "print.hh"

#include <map>

#include <iostream>

#include <cstdlib> // EXIT_SUCCESS

// BEGIN option handling for 'size'

static bool size = false;

static void opt_s() { size = true; }

static int _dummy =
    (opt_reg_t::append(
         's', opt_s, "s", "  [-s]",
         "  -s \t\tuse the size of the generated binary rather than\n"
         "  \t\tthe output (running time)\n"),
     1);

// END

void summary_first_measure() {
  summary_first_compile();
  o3 << "\nOptimized for: " << (size ? "size" : "time (output value)");
}

static obj_t sample(pset_t pset) {
  if (pset == error_pset()) {
    return obj_t("inf");
  }

#ifdef DEBUG
  progress.tick('*', point_t());
#else
  progress.tick('*');
#endif

  tmp_file_t const &tmp_file = get_tmp_file(pset);

  std::string foo = std::string("size -A ") + std::string(tmp_file.get_path()) +
                    " | grep .text | awk '{ print $2 }'";
  std::string bar = std::string(tmp_file.get_path());
  std::string cmd = size ? foo : bar;

  // string cmd = size ? string("size -A ") + tmp_file.path +
  //                         " | grep .text | awk '{ print $2 }'"
  //                   : tmp_file.path;

  cmd_res_t cmd_res = execute(cmd);

  return obj_t(cmd_res.status == EXIT_SUCCESS ? cmd_res.output : "inf");
}

// all results
using results_t = std::map<pset_t, obj_t>;
static results_t results;

// check if alredy done for a given executable file
static bool result_sampled(pset_t pset) {
  return results.find(pset) != results.end();
}

obj_t measure(const point_t &p) {
#ifdef DEBUG
  progress.tick('.', p);
#else
  progress.tick('.');
#endif

  pset_t pset = compile(p);

  for (; !result_sampled(pset);) {
    GNUC_BUILTIN_ASSUME(results.find(pset) == results.end());
    results[pset] = sample(pset);
  }

  return results[pset];
}

point_t get_min_point() {

  // if we don't have any previous mesurments, make one on a default value
  if (results.empty()) {
    measure(point_t());
  }

  auto min_i = results.cbegin();
  for (auto i = results.cbegin(); i != results.cend(); ++i) {
    // ignore that measurments may use different number of samples
    if (i->second < min_i->second) {
      min_i = i;
    }
  }
  return get_point(min_i->first);
}

void reset_measurements() {
  results = results_t();
  progress = progress_t();
}
