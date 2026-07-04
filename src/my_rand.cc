#include "my_rand.hh"

#include <random> // default_random_engine, uniform_int_distribution

/// \file
/// Function my_rand()

int16_t my_rand() {
  // Seed with a real random value, if available
  // std::random_device r;
  // std::default_random_engine e(r());
  // NOLINTNEXTLINE(cert-msc51-cpp);
  static std::default_random_engine e(0);
  static std::uniform_int_distribution<int16_t> uniform_dist;
  return uniform_dist(e);
}
