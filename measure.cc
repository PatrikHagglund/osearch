#include "measure.hh"

#include "compile.hh"
#include "execute.hh"
#include "getopts.hh" // opt_init_t
#include "point.hh"
#include "print.hh"

#include <map>
using std::map;

#include <cassert>
#include <cstdlib> // EXIT_SUCCESS

// BEGIN option handling for 'size'

static bool size = false;

static void opt_s() { size = true; }

static opt_reg_t
    opt_reg('s', opt_s, "s", "  [-s]",
            "  -s \t\tuse the size of the generated binary rather than\n"
            "  \t\tthe output (running time)\n");

// END

void summary_first_measure() {
  summary_first_compile();
  fprintf(o3, "\nOptimized for: %s", size ? "size" : "time (output value)");
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

  string cmd = size ? string("size -A ") + tmp_file.path +
                          " | grep .text | awk '{ print $2 }'"
                    : tmp_file.path;

  cmd_res_t cmd_res = execute(cmd);

  return obj_t(cmd_res.status == EXIT_SUCCESS ? cmd_res.output : "inf");
}

// all results
using results_t = map<pset_t, obj_t>;
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
    assert(results.find(pset) == results.end());
    results[pset] = sample(pset);
  }

  return results[pset];
}

point_t get_min_point() {

  // if we don't have any previous mesurments, make one on a default value
  if (results.begin() == results.end()) {
    measure(point_t());
  }

  auto min_i = results.begin();
  for (auto i = results.begin(); i != results.end(); ++i) {
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
