#ifndef PRINT_HH
#define PRINT_HH

// output streams
#include <ostream>

#include <iostream> // std::cout

constexpr inline std::ostream &o1 = std::cout;
constexpr inline std::ostream &o2 = std::cout;
constexpr inline std::ostream &o3 = std::cout;

// progress

#ifdef DEBUG
#include "point.hh"
#endif

#include <map>
#include <cstdio>

struct progress_t {
  explicit progress_t() = default;
  // storage
  std::map<char, unsigned> cnts{};
  std::ostream *o{&o2};
  bool silent{false};
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
