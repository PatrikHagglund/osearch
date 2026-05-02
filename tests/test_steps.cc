/// \file
/// Runtime tests for steps_t / delta_t (see steps.hh / steps.cc).
///
/// steps.cc references several externs we don't want to drag in for a
/// unit test (conf, progress, measure, my_rand). We provide minimal
/// stubs here; the tests only exercise delta_t methods plus
/// steps_t::store(), which never touch measure() or my_rand(). str()
/// is avoided (it formats via conf.flags), leaving alt_diff() and
/// operator< as the interesting targets.

#include "../steps.hh"

#include "../measure.hh" // obj_t, measure() decl
#include "../my_rand.hh" // my_rand() decl
#include "../point.hh"
#include "../print.hh" // progress_t
#include "../read_conf.hh" // conf_t, flag::simple_t

#include "check.hh"

// --- Stubs for externs referenced by steps.cc -------------------------------

conf_t conf{};
progress_t progress{};

// Defined here so the linker is satisfied; never called by these tests.
// (Can't mark [[noreturn]] since the declarations in the headers aren't;
// silence the -Wsuggest-attribute=noreturn warning locally instead.)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
obj_t measure(point_t const&) {
  std::fprintf(stderr, "unexpected call to measure() in unit test\n");
  std::abort();
}
int16_t my_rand() {
  std::fprintf(stderr, "unexpected call to my_rand() in unit test\n");
  std::abort();
}
#pragma GCC diagnostic pop

// --- Fixture ----------------------------------------------------------------

static void setup_conf() {
  static flag::simple_t fa{"-fa"};
  static flag::simple_t fb{"-fb"};
  static flag::simple_t fc{"-fc"};
  conf.flags.clear();
  conf.flags.push_back(&fa);
  conf.flags.push_back(&fb);
  conf.flags.push_back(&fc);
}

// Build a point with the given flag values.
static point_t make_point(uint8_t a, uint8_t b, uint8_t c) {
  point_t p; // sized by conf
  p.val[0] = a;
  p.val[1] = b;
  p.val[2] = c;
  return p;
}

// --- delta_t tests ----------------------------------------------------------

static void test_delta_ctor_preserves_fields() {
  point_t const p = make_point(1, 0, 0);
  point_t const p_prev = make_point(0, 0, 0);
  delta_t const d(p, p_prev, /*equal=*/false, obj_t{-7});

  CHECK(d.p == p);
  CHECK(d.p_prev == p_prev);
  CHECK(!d.equal);
  CHECK(d.diff == obj_t{-7});
}

// alt_diff flips the sign when any option was REMOVED going from
// p_prev to p (i.e., p has a 0 where p_prev had non-zero).
static void test_alt_diff_forward_unchanged() {
  // Adding a flag: 0 -> 1 (no removals).
  point_t const p_prev = make_point(0, 0, 0);
  point_t const p      = make_point(1, 0, 0);
  delta_t const d(p, p_prev, false, obj_t{-5});
  CHECK_EQ(d.alt_diff(), obj_t{-5});
}

static void test_alt_diff_backward_flips_sign() {
  // Removing a flag: 1 -> 0 counts as backwards.
  point_t const p_prev = make_point(1, 0, 0);
  point_t const p      = make_point(0, 0, 0);
  delta_t const d(p, p_prev, false, obj_t{-5});
  CHECK_EQ(d.alt_diff(), obj_t{5}); // sign flipped
}

static void test_alt_diff_mixed_is_backward() {
  // Any removal makes the whole delta "backward".
  point_t const p_prev = make_point(1, 0, 0);
  point_t const p      = make_point(0, 1, 0);
  delta_t const d(p, p_prev, false, obj_t{-3});
  CHECK_EQ(d.alt_diff(), obj_t{3});
}

static void test_alt_diff_inf_passes_through() {
  // obj_t_inf cannot be negated; alt_diff() must keep it as-is even
  // when the delta is "backward".
  point_t const p_prev = make_point(1, 0, 0);
  point_t const p      = make_point(0, 0, 0);
  delta_t const d(p, p_prev, false, obj_t_inf);
  CHECK_EQ(d.alt_diff(), obj_t_inf);
}

static void test_delta_less_uses_alt_diff() {
  // Two forward deltas: the one with smaller diff sorts first.
  point_t const p_prev = make_point(0, 0, 0);
  delta_t const better (make_point(1, 0, 0), p_prev, false, obj_t{-10});
  delta_t const worse  (make_point(0, 1, 0), p_prev, false, obj_t{-2});
  CHECK(better < worse);
  CHECK(!(worse < better));
}

// --- steps_t tests ----------------------------------------------------------

static void test_steps_store_sorts() {
  steps_t s;
  point_t const p_prev = make_point(0, 0, 0);
  delta_t const d_big  (make_point(1, 0, 0), p_prev, false, obj_t{-10});
  delta_t const d_small(make_point(0, 1, 0), p_prev, false, obj_t{-2});

  s.store(d_big);
  s.store(d_small);

  CHECK_EQ(s.done.size(), size_t{2});
  // done is sorted ascending by alt_diff(); both forward, so by diff.
  CHECK(s.done[0].diff < s.done[1].diff);
  CHECK_EQ(s.done[0].diff, obj_t{-10});
  CHECK_EQ(s.done[1].diff, obj_t{-2});
}

static void test_steps_default_state() {
  steps_t const s;
  CHECK_EQ(s.done.size(), size_t{0});
  // delta_info default-initializes to its first enumerator (valid).
  CHECK(s.delta_info == delta_info_t::valid);
}

// --- driver -----------------------------------------------------------------

int main() {
  setup_conf();

  test_delta_ctor_preserves_fields();
  test_alt_diff_forward_unchanged();
  test_alt_diff_backward_flips_sign();
  test_alt_diff_mixed_is_backward();
  test_alt_diff_inf_passes_through();
  test_delta_less_uses_alt_diff();

  test_steps_store_sorts();
  test_steps_default_state();

  return TEST_REPORT();
}
