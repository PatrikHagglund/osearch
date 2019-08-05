#ifndef PRINT_HH
#define PRINT_HH

// output streams
#include <ostream>

#include <iostream> // std::cout

/// \file
/// Progress printouts, using progress_t and its progress_t::tick() method.

/// Different levels of printout, o1,
constexpr inline std::ostream &o1 = std::cout;
/// o2,
constexpr inline std::ostream &o2 = std::cout;
/// o3
constexpr inline std::ostream &o3 = std::cout;

// progress

#ifdef DEBUG
#include "point.hh"
#endif

#include <map>

/// Class for progress printouts.
struct progress_t {
  explicit progress_t() = default;
  // storage
  std::map<char, unsigned> cnts{};
  std::ostream *o{&o2};
  /// Suppress '.' progress printouts.
  bool silent{false};
  /// Print out summary of symbols used for progress indications.
  void print_symbols() const;
#ifdef DEBUG
  void tick(char sym, point_t const &p);
#else
  void tick(char sym);
#endif
  void newl() const;
};

extern progress_t progress;

#endif
