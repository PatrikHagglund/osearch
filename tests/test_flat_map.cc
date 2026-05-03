#include "flat_map.hh"
#include "check.hh"

#include <string>

static void test_empty() {
  flat_map<int, int> m;
  CHECK(m.empty());
  CHECK(!m.contains(0));
}

static void test_insert_and_lookup() {
  flat_map<int, std::string> m;
  m[3] = "three";
  m[1] = "one";
  m[2] = "two";
  CHECK_EQ(m.size(), size_t{3});
  CHECK(m.contains(1));
  CHECK(m.contains(2));
  CHECK(m.contains(3));
  CHECK(!m.contains(4));
  CHECK(m[1] == std::string{"one"});
  CHECK(m[2] == std::string{"two"});
  CHECK(m[3] == std::string{"three"});
}

static void test_insert_method() {
  flat_map<int, int> m;
  m.insert({2, 20});
  m.insert({1, 10});
  m.insert({2, 99}); // duplicate key — should not overwrite
  CHECK_EQ(m.size(), size_t{2});
  CHECK(m[2] == 20);
}

static void test_iteration_is_sorted() {
  flat_map<int, int> m;
  m[5] = 50;
  m[1] = 10;
  m[3] = 30;
  int prev = -1;
  for (auto it = m.cbegin(); it != m.cend(); ++it) {
    CHECK(it->first > prev);
    prev = it->first;
  }
}

int main() {
  test_empty();
  test_insert_and_lookup();
  test_insert_method();
  test_iteration_is_sorted();
  return TEST_REPORT();
}
