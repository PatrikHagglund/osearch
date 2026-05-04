#include "measure.hh"

#include "compile.hh"
#include "execute.hh"
#include "getopts.hh" // opt_init_t
#include "point.hh"
#include "print.hh"

#include "flat_map.hh"

#include <cstdlib> // EXIT_SUCCESS

/// \file
/// Measure (compile and sample) given a point_t, using measure(). Store results
/// in the results mapping.

// BEGIN option handling for 'size'

/// Flag to tell if we optimize for size, or execution time.
static bool opt_size = false;

/// Option helper function.
static void opt_s() { opt_size = true; }

/// Option helper varaible.
static int dummy_ =
    (opt_reg_t::append(
         's', opt_s, "s", "  [-s]",
         "  -s \t\tuse the size of the generated binary rather than\n"
         "  \t\tthe output (running time)\n"),
     1);

// END

// BEGIN option handling for 'perf' (instruction count)

/// Flag to tell if we optimize for retired instruction count.
static bool opt_perf = false;

/// Option helper function.
static void opt_p() { opt_perf = true; }

/// Option helper variable.
static int dummy_p_ =
    (opt_reg_t::append(
         'p', opt_p, "p", "  [-p]",
         "  -p \t\tuse retired instruction count (via perf stat) rather\n"
         "  \t\tthan wall-clock time. Deterministic, but requires 'perf'\n"),
     1);

// END

// BEGIN option handling for 'num_samples'

/// Number of samples per measurement (take minimum). Default 3.
static unsigned long num_samples = 3;

/// Option helper function.
static void opt_n() { num_samples = strtoul(optarg, nullptr, 0); }

/// Option helper variable.
static int dummy_n_ =
    (opt_reg_t::append('n', opt_n, "n:", "  [-n samples]",
                       "  -n samples \tnumber of samples per measurement "
                       "(take minimum, default 1)\n"),
     1);

// END

// BEGIN option handling for 'threshold' (adoption threshold, permille)

/// Adoption threshold in permille (1/1000). A flag is adopted only if
/// its improvement is at least this fraction of the baseline. Default 0.
static unsigned long opt_threshold_permille = 0;

/// Option helper function.
static void opt_T() { opt_threshold_permille = strtoul(optarg, nullptr, 0); }

/// Option helper variable.
static int dummy_T_ =
    (opt_reg_t::append('T', opt_T, "T:", "  [-T permille]",
                       "  -T permille \tadoption threshold in units of "
                       "0.1%%; only adopt a flag\n"
                       "  \t\tif improvement >= threshold (default 0, i.e. "
                       "strict <)\n"),
     1);

// END

/// Return true if `new_val` is a sufficient improvement over `baseline`
/// to warrant adoption. Uses the `-T permille` threshold.
bool is_improvement(obj_t new_val, obj_t baseline) {
  if (!new_val.is_finite()) return false;
  if (!baseline.is_finite()) return true;  // any finite beats inf
  if (opt_threshold_permille == 0) return new_val < baseline;
  // Require new_val <= baseline - threshold, where threshold is a
  // fraction of the baseline magnitude (positive).
  obj_t const delta = baseline - new_val;  // positive if improvement
  if (!(obj_t() < delta)) return false;    // no improvement at all
  // Compare delta * 1000 vs baseline * threshold_permille, using
  // int64 arithmetic on the underlying values via subtraction.
  // We need: delta >= baseline * threshold_permille / 1000.
  // Using obj_t's operator- directly to avoid exposing internals:
  // compute threshold_delta = baseline - (baseline * (1000 - T) / 1000).
  // Simpler path: re-express using int64 via a helper.
  // obj_t has no public multiplication; compute via the diff between
  // two obj_t built from its to_string round-trip. That's ugly; instead
  // we add a free friend approach. Here we just convert through string,
  // which is correct but slow — but this is called O(flags) per step,
  // negligible.
  int64_t const base_i = std::stoll(baseline.to_string());
  int64_t const delta_i = std::stoll(delta.to_string());
  int64_t const min_delta = (base_i * static_cast<int64_t>(opt_threshold_permille)) / 1000;
  return delta_i >= min_delta;
}

void summary_first_measure() {
  summary_first_compile();
  o3 << "\nOptimized for: "
     << (opt_size     ? "size"
         : opt_perf   ? "retired instructions (perf)"
                      : "time (output value)");
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

  const std::string size_cmd = std::string("size -A ") +
                               std::string(tmp_file.get_path()) +
                               " | grep .text | awk '{ print $2 }'";
  const std::string perf_cmd = std::string("perf stat -x, -e instructions:u ") +
                               std::string(tmp_file.get_path()) +
                               " 2>&1 | grep instructions:u | cut -d, -f1";
  const std::string run_cmd = std::string(tmp_file.get_path());
  const std::string cmd = opt_size ? size_cmd : opt_perf ? perf_cmd : run_cmd;

  const cmd_res_t cmd_res = execute(cmd);
  const obj_t obj = cmd_res.status == EXIT_SUCCESS
                        ? obj_t(std::stol(cmd_res.output))
                        : obj_t_inf;

  // For time measurements, take the minimum of num_samples runs.
  // Size and perf measurements are deterministic; a single sample is enough.
  if (opt_size || opt_perf || num_samples <= 1 || !obj.is_finite()) {
    return obj;
  }
  obj_t best = obj;
  for (unsigned long i = 1; i < num_samples; ++i) {
    const cmd_res_t r = execute(cmd);
    if (r.status != EXIT_SUCCESS) return obj_t_inf;
    obj_t val(std::stol(r.output));
    if (val < best) best = val;
  }
  return best;
}

/// Mapping of all results.
using results_t = flat_map<pset_t, obj_t>;
/// Mapping of all results.
static results_t results;

/// Check if sampled for a given pset_t (executable file).
/// \param pset pset_t
/// \return true if sampled, false otherwise
/// \todo Make a member function
static bool result_sampled(pset_t pset) {
  return results.contains(pset);
}

obj_t measure(const point_t &p) {
#ifdef DEBUG
  progress.tick('.', p);
#else
  progress.tick('.');
#endif

  pset_t const pset = compile(p);

  for (; !result_sampled(pset);) {
    contract_assert(!results.contains(pset));
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

point_t validate() {
  point_t p = get_min_point();
  obj_t res = measure(p);

  o1 << "\n## Validation pass (trying to remove each adopted flag):\n"
     << res.to_string() << " " << p.to_string() << "\n";

  bool dropped_any = true;
  while (dropped_any) {
    dropped_any = false;
    for (size_t i = 0; i < p.val.size(); ++i) {
      if (p.val[i] == 0) continue;  // already inactive
      point_t p_trial = p;
      p_trial.val[i] = 0;
      obj_t const res_trial = measure(p_trial);
      // Drop if trial is not worse than current p (within threshold).
      // !is_improvement(res, res_trial) means res does not significantly
      // beat res_trial — i.e. the flag we'd remove is not helping.
      if (!is_improvement(res, res_trial)) {
        o1 << "\nValidation dropped flag at index " << i
           << ", res " << res_trial.to_string()
           << " (was " << res.to_string() << ")";
        p = p_trial;
        res = res_trial;
        dropped_any = true;
      }
    }
  }
  o1 << "\n## After validation:\n"
     << res.to_string() << " " << p.to_string() << "\n";
  return p;
}

void reset_measurements() {
  results = results_t();
  bool const was_silent = progress.silent;
  progress = progress_t();
  progress.silent = was_silent;
}
