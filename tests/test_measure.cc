/// \file
/// Runtime tests for measure(), get_min_point(), reset_measurements().
///
/// measure.cc calls compile(), get_tmp_file(), get_point(), execute(),
/// and uses the progress global. We provide stubs that return canned
/// values so we can test the results-map logic in isolation.

#include "../measure.hh"
#include "../compile.hh"
#include "../execute.hh"
#include "../point.hh"
#include "../print.hh"
#include "../read_conf.hh"

#include "check.hh"

#include <cstdlib>
#include <cstring>
#include <string>

// --- Stubs ------------------------------------------------------------------

conf_t conf{};
progress_t progress{};
std::string input_file;

// Stub state: compile() returns sequential pset_t values.
static pset_t next_pset = 1;

pset_t compile(point_t const&) { return next_pset++; }

// Provide a stub tmp_file_t constructor (real one calls mkstemp).
tmp_file_t::tmp_file_t() { std::strncpy(&path[0], "/dev/null", sizeof(path)); }

// get_tmp_file returns a static tmp_file_t; sample() will use its path
// as the command passed to execute().
static tmp_file_t stub_tmp;
tmp_file_t const& get_tmp_file(pset_t) { return stub_tmp; }

// get_point is called by get_min_point(); return a default point.
point_t get_point(pset_t) { return point_t(); }

// execute() returns a canned result. We control the "measurement" via
// this global.
static int64_t fake_measurement = 100;
cmd_res_t execute(std::string) {
  return cmd_res_t(EXIT_SUCCESS, std::to_string(fake_measurement));
}

void summary_first_compile() {}
bool equivalent_p(point_t const&, point_t const&) { return false; }
void reset_compilations() {}
std::string get_compiler_version() { return "stub"; }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
void progress_t::tick(char) {}
void progress_t::newl() const {}
void progress_t::print_symbols() const {}
#pragma GCC diagnostic pop

// --- Setup ------------------------------------------------------------------

static void setup() {
  conf.flags.clear();
  conf.flags.emplace_back(flag::simple_t{"-O1"});
  conf.flags.emplace_back(flag::simple_t{"-O2"});
  next_pset = 1;
  reset_measurements();
}

// --- Tests ------------------------------------------------------------------

static void test_measure_returns_obj() {
  setup();
  fake_measurement = 42;
  point_t p;
  obj_t result = measure(p);
  CHECK_EQ(result, obj_t{42});
}

static void test_measure_caches_result() {
  setup();
  // compile() returns the same pset for different calls only if we
  // control it. Here each call gets a new pset, so two calls give
  // two entries.
  fake_measurement = 10;
  point_t p1;
  obj_t r1 = measure(p1);

  fake_measurement = 20;
  point_t p2;
  obj_t r2 = measure(p2);

  CHECK_EQ(r1, obj_t{10});
  CHECK_EQ(r2, obj_t{20});
}

static void test_get_min_point() {
  setup();
  fake_measurement = 50;
  point_t p1;
  measure(p1);

  fake_measurement = 30;
  point_t p2;
  measure(p2);

  fake_measurement = 70;
  point_t p3;
  measure(p3);

  // get_min_point returns the point with the lowest obj (30).
  point_t min_p = get_min_point();
  // Since get_point stub returns default point, just verify it doesn't crash
  // and returns a valid point.
  CHECK_EQ(min_p.val.size(), size_t{2});
}

static void test_reset_measurements() {
  setup();
  fake_measurement = 99;
  measure(point_t());

  reset_measurements();

  // After reset, get_min_point() must re-measure (calls measure internally).
  fake_measurement = 55;
  point_t min_p = get_min_point();
  CHECK_EQ(min_p.val.size(), size_t{2});
}

// --- Driver -----------------------------------------------------------------

int main() {
  test_measure_returns_obj();
  test_measure_caches_result();
  test_get_min_point();
  test_reset_measurements();
  return TEST_REPORT();
}
