#ifndef TESTS_CHECK_HH
#define TESTS_CHECK_HH

/// \file
/// Minimal runtime test helpers for use with CTest.
///
/// Each test is a small executable whose main() returns the number of
/// failed CHECK()s (capped at 1 for exit status). CTest records pass/fail
/// from the exit code. Failures are printed with file:line and expression.
///
/// Usage:
///   #include "check.hh"
///   int main() {
///     CHECK(1 + 1 == 2);
///     CHECK_EQ(foo(), 42);
///     return test_report();
///   }

#include <cstdio>

namespace tests {
inline int failures = 0;
inline int checks = 0;

/// Summarize results and return suitable main() exit code.
/// \return 0 if all CHECKs passed, 1 otherwise.
inline int report(char const* name = "tests") {
  std::fprintf(stderr, "[%s] %d checks, %d failures\n",
               name, checks, failures);
  return failures == 0 ? 0 : 1;
}
} // namespace tests

#define CHECK(expr)                                                     \
  do {                                                                  \
    ++::tests::checks;                                                  \
    if (!(expr)) {                                                      \
      ++::tests::failures;                                              \
      std::fprintf(stderr, "FAIL %s:%d: CHECK(%s)\n",                   \
                   __FILE__, __LINE__, #expr);                          \
    }                                                                   \
  } while (0)

#define CHECK_EQ(a, b)                                                  \
  do {                                                                  \
    ++::tests::checks;                                                  \
    auto const& _a = (a);                                               \
    auto const& _b = (b);                                               \
    if (!(_a == _b)) {                                                  \
      ++::tests::failures;                                              \
      std::fprintf(stderr, "FAIL %s:%d: CHECK_EQ(%s, %s)\n",            \
                   __FILE__, __LINE__, #a, #b);                         \
    }                                                                   \
  } while (0)

#define TEST_REPORT() ::tests::report()

#endif // TESTS_CHECK_HH
