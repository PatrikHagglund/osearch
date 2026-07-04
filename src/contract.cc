#include <contracts>
#include <cstdio>
#include <cstdlib>

/// C++26 contract violation handler. Called when a pre/post/contract_assert
/// fails at runtime (under the default "enforce" semantic).
[[noreturn]] void handle_contract_violation(
    const std::contracts::contract_violation &v);

[[noreturn]] void handle_contract_violation(
    const std::contracts::contract_violation &v) {
  auto loc = v.location();
  std::fprintf(stderr, "contract violation: %s at %s:%u\n",
               v.comment(), loc.file_name(), loc.line());
  std::abort();
}
