#ifndef COMPILE_HH
#define COMPILE_HH

#include "point.hh"
#include "execute.hh" // tmp_file_t

#include <string>
using std::string;
#include <utility>
using std::pair;

extern string input_file;

void summary_first_compile();

// set of points compiling to the same executable
typedef unsigned pset_t;

pset_t error_pset(); // handle for set of "invalid" points

pset_t compile(point_t p);

bool equivalent_p(point_t p1, point_t p2);

tmp_file_t const& get_tmp_file(pset_t pset);

point_t get_point(pset_t pset);

void reset_compilations();

// other

string get_compiler_version();

#endif
