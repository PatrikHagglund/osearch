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

/// \file
/// Measure (compile and sample) given a point_t, using measure(). Store results
/// in the results mapping.

// BEGIN option handling for 'size'

/// Flag to tell if we optimize for size, or execution time.
static bool opt_size = false;

/// Option helper function.
/// \todo Possible to use lambda function?
static void opt_s() { opt_size = true; }

/// Option helper varaible.
static int dummy_ =
    (opt_reg_t::append(
         's', opt_s, "s", "  [-s]",
         "  -s \t\tuse the size of the generated binary rather than\n"
         "  \t\tthe output (running time)\n"),
     1);

// END

void summary_first_measure() {
  summary_first_compile();
  o3 << "\nOptimized for: " << (opt_size ? "size" : "time (output value)");
}

/// Compute the objective function (generated code size if opt_size,
/// otherwise execution time) for a given pset_t.
/// \param pset pset_t
/// \return obj_t
static obj_t sample(pset_t pset) {
  if (pset == pset_invalid) {
    return obj_t_inf;
  }

#ifdef DEBUG
  progress.tick('*', point_t());
#else
  progress.tick('*');
#endif

  tmp_file_t const &tmp_file = get_tmp_file(pset);

  const std::string foo = std::string("size -A ") +
                          std::string(tmp_file.get_path()) +
                          " | grep .text | awk '{ print $2 }'";
  const std::string bar = std::string(tmp_file.get_path());
  const std::string cmd = opt_size ? foo : bar;

  // string cmd = size ? string("size -A ") + tmp_file.path +
  //                         " | grep .text | awk '{ print $2 }'"
  //                   : tmp_file.path;

  const cmd_res_t cmd_res = execute(cmd);
  const obj_t obj = cmd_res.status == EXIT_SUCCESS
                        ? obj_t(std::stol(cmd_res.output))
                        : obj_t_inf;
  return obj;
}

/// Mapping of all results.
using results_t = std::map<pset_t, obj_t>;
/// Mapping of all results.
static results_t results;

/// Check if sampled for a given pset_t (executable file).
/// \param pset pset_t
/// \return true if sampled, false otherwise
/// \todo Make a member function
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
