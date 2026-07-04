/// \file
/// Runtime tests for point_t (see point.hh / point.cc).
///
/// point_t's default constructor depends on the global `conf` from
/// read_conf.hh (it sizes its internal vector to conf.flags.size()). To
/// keep this test independent of the XML parser, we supply our own
/// definition of `conf` here and populate it with a handful of dummy
/// simple_t flags.

#include "point.hh"
#include "read_conf.hh" // conf_t, flag::simple_t
#include "check.hh"

// One-definition of the global `conf`. This replaces the definition in
// read_conf.cc (which we do not link into this test).
conf_t conf{};

static void setup_conf() {
  // Three simple_t flags: "-fa", "-fb", "-fc".
  conf.flags.clear();
  conf.flags.emplace_back(flag::simple_t{"-fa"});
  conf.flags.emplace_back(flag::simple_t{"-fb"});
  conf.flags.emplace_back(flag::simple_t{"-fc"});
}

static void test_default_ctor_sizes_from_conf() {
  point_t const p;
  CHECK_EQ(p.val.size(), conf.flags.size());
  // All values start at zero.
  for (auto const v : p.val) {
    CHECK_EQ(v, uint8_t{0});
  }
}

static void test_active_count_zero_by_default() {
  point_t const p;
  CHECK_EQ(p.active_count(), 0U);
}

static void test_active_count_counts_nonzero() {
  point_t p;
  p.val[0] = 1;
  p.val[2] = 1;
  CHECK_EQ(p.active_count(), 2U);

  // Setting to zero decrements count.
  p.val[0] = 0;
  CHECK_EQ(p.active_count(), 1U);
}

static void test_to_string_empty_when_all_zero() {
  point_t const p;
  CHECK_EQ(p.to_string(), std::string{});
}

static void test_to_string_lists_enabled_flags() {
  point_t p;
  p.val[0] = 1; // -fa
  p.val[2] = 1; // -fc
  // Each enabled flag is followed by a single space.
  CHECK_EQ(p.to_string(), std::string{"-fa -fc "});
}

static void test_equality_and_ordering() {
  point_t a;
  point_t b;
  CHECK(a == b);
  CHECK(!(a < b));
  CHECK(!(b < a));

  b.val[1] = 1;
  CHECK(!(a == b));
  // Default <=> is lexicographic; a has 0 at index 1, b has 1 there.
  CHECK(a < b);
  CHECK(b > a);
}

int main() {
  setup_conf();

  test_default_ctor_sizes_from_conf();
  test_active_count_zero_by_default();
  test_active_count_counts_nonzero();
  test_to_string_empty_when_all_zero();
  test_to_string_lists_enabled_flags();
  test_equality_and_ordering();

  return TEST_REPORT();
}
