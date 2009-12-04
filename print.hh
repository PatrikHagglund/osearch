#ifndef _PRINT_HH
#define _PRINT_HH

// output streams
#include <cstdio>

extern FILE* o1;
extern FILE* o2;
extern FILE* o3;

// progress

#ifdef DEBUG
#include "point.hh"
#endif

#include <map>
using std::map;
#include <cstdio>

struct progress_t {
  map<char, unsigned> cnts;
  FILE* o;
  bool silent;
  explicit progress_t();
  void print_symbols() const;
#ifdef DEBUG
  void tick(char sym, point_t p);
#else
  void tick(char sym);
#endif
  void newl() const;
  progress_t& operator=(progress_t const& p);
private:
  progress_t(progress_t const&);
};

extern progress_t progress;

#endif
