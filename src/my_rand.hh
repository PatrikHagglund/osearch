#ifndef MY_RAND_HH
#define MY_RAND_HH

#include <cstdint>  // int16_t

/// \file
/// Function my_rand()

/// Simple replacement for rand().
/// clang-tidy: "rand() has limited randomness; use C++11 random library instead [cert-msc30-c]"
int16_t my_rand();

#endif // MY_RAND_HH
