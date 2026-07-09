#include "measure.hh"

#include "compile.hh"
#include "execute.hh"
#include "getopts.hh" // opt_init_t
#include "point.hh"
#include "print.hh"

#include "flat_map.hh"

#include <algorithm> // std::max, std::min
#include <cstdint>   // INT64_MAX
#include <cstdlib>   // EXIT_SUCCESS
#include <string>    // std::stol
#include <utility>   // std::pair

/// \file
/// Measure (compile and sample) given a point_t, using measure(). Store results
/// in the results mapping.

// BEGIN option handling for 'size'

/// Flag to tell if we optimize for size, or execution time.
static bool opt_size = false;

/// Option helper function.
static void opt_s() { opt_size = true; }

bool objective_is_size() { return opt_size; }

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
         "  -p \t\tuse retired instruction count (counted in-harness via\n"
         "  \t\tperf_event_open around run()) rather than wall-clock time.\n"
         "  \t\tDeterministic; requires Linux perf_event support\n"
         "  \t\t(perf_event_paranoid <= 2)\n"),
     1);

// END

// BEGIN option handling for 'uops' (retired-op count)

/// Flag to tell if we optimize for retired ops (micro/macro-ops).
static bool opt_uops = false;

/// Option helper function.
static void opt_u() { opt_uops = true; }

/// Option helper variable.
static int dummy_u_ =
    (opt_reg_t::append(
         'u', opt_u, "u", "  [-u]",
         "  -u \t\tuse retired-op count (AMD Zen ex_ret_ops, counted\n"
         "  \t\tin-harness like -p). Near-deterministic, and prices\n"
         "  \t\tmicrocoded instructions (gathers, wide shuffles) honestly\n"
         "  \t\twhere -p counts 1\n"),
     1);

// END

// BEGIN option handling for 'cycles'

/// Flag to tell if we optimize for core cycles.
static bool opt_cycles = false;

/// Option helper function.
static void opt_c() { opt_cycles = true; }

/// Option helper variable.
static int dummy_c_ =
    (opt_reg_t::append(
         'c', opt_c, "c", "  [-c]",
         "  -c \t\tuse core cycles (counted in-harness like -p; the run\n"
         "  \t\tself-pins to its CPU). Closest to real time but noisy on\n"
         "  \t\ta shared host: sampled -n times taking the minimum —\n"
         "  \t\traise -n and -T accordingly\n"),
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

// BEGIN option handling for 'cycle validation' (-C permille)

/// If nonzero, re-score the post-search validation pass under cycles:
/// paired interleaved baseline/trial runs, compared by their minimums,
/// keeping a flag only if it improves cycles by at least this many
/// permille. The search objective itself is untouched (hybrid search:
/// deterministic -u/-p/-s greedy steps, real-cycle final validation).
static unsigned long opt_cvalidate_permille = 0;

/// Option helper function.
static void opt_C() { opt_cvalidate_permille = strtoul(optarg, nullptr, 0); }

/// Option helper variable.
static int dummy_C_ =
    (opt_reg_t::append(
         'C', opt_C, "C:", "  [-C permille]",
         "  -C permille \tafter the search, re-check each adopted flag under\n"
         "  \t\tcycles (paired interleaved runs, min-of-pairs); keep it only\n"
         "  \t\tif it improves cycles by >= permille (0.1%% units; ~25-30\n"
         "  \t\tclears this host's noise). Search objective is unchanged\n"),
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
         : opt_uops   ? "retired ops (perf)"
         : opt_cycles ? "cycles (perf, min of -n samples)"
                      : "time (output value)");
}

/// The first time a perf-mode run fails because the in-harness counter
/// could not be opened, print a clear diagnostic. Without this the user
/// only sees every measurement come back as `inf` with no explanation,
/// because the benchmark's stderr is captured and discarded. Detected via
/// the harness's "perf_event_open" message (see main.ic).
static void warn_if_perf_unavailable(std::string const &output) {
  static bool warned = false;
  if (warned || output.find("perf_event_open") == std::string::npos) return;
  warned = true;
  std::string::size_type const nl = output.find('\n');
  std::cerr << "\nWARNING: -p/-u/-c hardware counting failed; every "
               "measurement will be 'inf'.\n"
               "The benchmark could not open a hardware counter. "
               "Check that the\n"
               "kernel has CONFIG_PERF_EVENTS and perf_event_paranoid <= 2, "
               "and that a container\n"
               "(if any) is not blocking perf_event_open via seccomp (e.g. run "
               "with\n"
               "--security-opt seccomp=unconfined). -u additionally needs an "
               "AMD Zen PMU.\n"
               "Underlying error: "
            << output.substr(0, nl) << "\n";
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
  // The harness (main.ic) counts retired instructions for run() in-process
  // via perf_event_open when passed -p, and prints the count like the time
  // value. This excludes process startup, init(), clean(), and the perf
  // tool's own fork+exec, which dominate the count for short benchmarks.
  const std::string run_cmd =
      std::string(tmp_file.get_path()) +
      (opt_perf ? " -p" : opt_uops ? " -u" : opt_cycles ? " -c" : "");
  const std::string cmd = opt_size ? size_cmd : run_cmd;
  bool const counter_mode = opt_perf || opt_uops || opt_cycles;

  const cmd_res_t cmd_res = execute(cmd);
  if (counter_mode && cmd_res.status != EXIT_SUCCESS)
    warn_if_perf_unavailable(cmd_res.output);
  const obj_t obj = cmd_res.status == EXIT_SUCCESS
                        ? obj_t(std::stol(cmd_res.output))
                        : obj_t_inf;

  // For time and cycle measurements, take the minimum of num_samples runs
  // (their noise is one-sided: interference only adds). Size, instruction
  // and op counts are deterministic; a single sample is enough.
  if (opt_size || opt_perf || opt_uops || num_samples <= 1 ||
      !obj.is_finite()) {
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

  if (pset == pset_invalid)
    return obj_t_inf;

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

/// Paired interleaved cycle measurement of two binaries: alternate
/// with-flag / without-flag executions back-to-back so both see the same
/// slow drift (frequency, neighbor load), and compare minimums — cycle
/// noise is one-sided, so the minimum is the clean estimate on each side.
/// Returns {min_with, min_without}, or {-1, -1} on any run failure.
static std::pair<int64_t, int64_t> paired_cycles(pset_t with_f,
                                                 pset_t without_f) {
  const std::string with_cmd =
      std::string(get_tmp_file(with_f).get_path()) + " -c";
  const std::string without_cmd =
      std::string(get_tmp_file(without_f).get_path()) + " -c";
  unsigned long const pairs = std::max(num_samples, 5UL);
  int64_t min_w = INT64_MAX;
  int64_t min_wo = INT64_MAX;
  for (unsigned long i = 0; i < pairs; ++i) {
    cmd_res_t const rw = execute(with_cmd);
    cmd_res_t const rwo = execute(without_cmd);
    if (rw.status != EXIT_SUCCESS || rwo.status != EXIT_SUCCESS) {
      warn_if_perf_unavailable(rw.output);
      return {-1, -1};
    }
    min_w = std::min(min_w, std::stol(rw.output));
    min_wo = std::min(min_wo, std::stol(rwo.output));
  }
  return {min_w, min_wo};
}

/// Second validation pass under real cycles (-C): for each still-active
/// flag, keep it only if the with-flag binary beats the without-flag
/// binary by at least opt_cvalidate_permille in paired-minimum cycles.
/// Flags whose removal doesn't change the binary are left alone (the
/// objective pass already keeps the option string minimal for those).
static point_t validate_cycles(point_t p) {
  o1 << "\n## Cycle validation pass (-C " << opt_cvalidate_permille
     << ", paired minimums):\n" << p.to_string() << "\n";

  bool dropped_any = true;
  while (dropped_any) {
    dropped_any = false;
    for (size_t i = 0; i < p.val.size(); ++i) {
      if (p.val[i] == 0) continue;  // already inactive
      point_t p_trial = p;
      p_trial.val[i] = 0;
      pset_t const with_f = compile(p);
      pset_t const without_f = compile(p_trial);
      if (with_f == pset_invalid || without_f == pset_invalid ||
          with_f == without_f)  // same binary: nothing to measure
        continue;
      auto const [min_w, min_wo] = paired_cycles(with_f, without_f);
      if (min_w < 0)
        return p;  // cycles unavailable; keep the objective-passed point
      // Keep the flag only if it wins by >= threshold of the without side.
      int64_t const needed =
          (min_wo * static_cast<int64_t>(opt_cvalidate_permille)) / 1000;
      if (min_wo - min_w < needed) {
        o1 << "\nCycle validation dropped flag at index " << i << ": with "
           << min_w << " vs without " << min_wo << " cycles (needed -"
           << needed << ")";
        p = p_trial;
        dropped_any = true;
      }
    }
  }
  o1 << "\n## After cycle validation:\n" << p.to_string() << "\n";
  return p;
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

  if (opt_cvalidate_permille != 0)
    p = validate_cycles(p);

  return p;
}

void reset_measurements() {
  results = results_t();
  bool const was_silent = progress.silent;
  progress = progress_t();
  progress.silent = was_silent;
}
